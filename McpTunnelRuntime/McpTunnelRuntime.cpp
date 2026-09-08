#include "McpTunnelRuntime.h"

#ifdef PLATFORM_POSIX
#include <sys/stat.h>
#endif

namespace Upp {

namespace {

String NormalizedId(String value)
{
    value = ToLower(TrimBoth(value));
    String out;
    bool dash = false;
    for(int i = 0; i < value.GetCount(); ++i) {
        int c = value[i];
        if(IsAlNum(c) || c == '_' || c == '.') {
            out.Cat(c);
            dash = false;
        }
        else if(!dash && !out.IsEmpty()) {
            out.Cat('-');
            dash = true;
        }
    }
    while(out.EndsWith("-"))
        out = out.Left(out.GetCount() - 1);
    return out;
}

bool IsCanonicalChannel(const String& channel)
{
    if(channel.IsEmpty())
        return false;
    for(int i = 0; i < channel.GetCount(); ++i) {
        int c = channel[i];
        if(!(IsAlNum(c) || c == '-' || c == '_' || c == '.'))
            return false;
    }
    return true;
}

}

String McpTunnelCredentialSourceId(McpTunnelCredentialSource source)
{
    return source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT ? "environment" : "session";
}

McpTunnelCredentialSource McpTunnelCredentialSourceFromId(const String& id)
{
    return id == "environment" ? MCP_TUNNEL_CREDENTIAL_ENVIRONMENT
                               : MCP_TUNNEL_CREDENTIAL_SESSION;
}

String McpTunnelDefaultMachineId()
{
    String id = GetComputerName();
    if(id.IsEmpty())
        id = GetEnv("COMPUTERNAME");
    if(id.IsEmpty())
        id = GetEnv("HOSTNAME");
    id = NormalizedId(id);
    return id.IsEmpty() ? String("local-machine") : id;
}

String McpTunnelCommandForExecutable(const String& path)
{
    String command = TrimBoth(path);
    command.Replace("\\", "/");
    if(command.GetCount() >= 2 && command[0] == '"' && command[command.GetCount() - 1] == '"')
        return command;
    if(command.Find(' ') >= 0 || command.Find('\t') >= 0)
        command = "\"" + command + "\"";
    return command;
}

String McpTunnelNormalizeServiceCommand(const String& command)
{
    String value = TrimBoth(command);
#ifdef PLATFORM_WIN32
    if(value.IsEmpty())
        return value;

    // The upstream tunnel runtime parses stdio commands with shell-like
    // backslash escaping, even on Windows. Treat a plain/quoted *.exe value as
    // one executable path and canonicalize separators before persistence and
    // launch. This also repairs older pasted Windows paths on load.
    String candidate = value;
    if(candidate.GetCount() >= 2 && candidate[0] == '"' && candidate[candidate.GetCount() - 1] == '"')
        candidate = candidate.Mid(1, candidate.GetCount() - 2);

    if(ToLower(candidate).EndsWith(".exe"))
        return McpTunnelCommandForExecutable(candidate);

    // For command forms with arguments, normalize only an obvious leading
    // drive-qualified executable token. Advanced arguments remain untouched.
    int split = value.GetCount();
    for(int i = 0; i < value.GetCount(); ++i)
        if(value[i] == ' ' || value[i] == '\t') {
            split = i;
            break;
        }
    if(split > 2) {
        String token = value.Left(split);
        if(token.GetCount() >= 3 && IsAlpha(token[0]) && token[1] == ':' && token.Find('\\') >= 0) {
            token.Replace("\\", "/");
            value = token + value.Mid(split);
        }
    }
#endif
    return value;
}

ValueMap McpTunnelServiceToValue(const McpTunnelService& service)
{
    ValueMap out;
    out.Add("id", service.id);
    out.Add("name", service.name);
    out.Add("channel", service.channel);
    out.Add("command", McpTunnelNormalizeServiceCommand(service.command));
    out.Add("enabled", service.enabled);
    return out;
}

McpTunnelService McpTunnelServiceFromValue(const Value& value)
{
    McpTunnelService service;
    if(!value.Is<ValueMap>())
        return service;
    service.id = AsString(value["id"]);
    service.name = AsString(value["name"]);
    service.channel = AsString(value["channel"]);
    service.command = McpTunnelNormalizeServiceCommand(AsString(value["command"]));
    service.enabled = IsNull(value["enabled"]) || (bool)value["enabled"];
    if(service.channel.IsEmpty())
        service.channel = "main";
    return service;
}

ValueMap McpTunnelProfileToValue(const McpTunnelProfile& profile)
{
    ValueMap out;
    out.Add("id", profile.id);
    out.Add("name", profile.name);
    out.Add("machine_id", profile.machine_id);
    out.Add("tunnel_id", profile.tunnel_id);
    out.Add("runtime_path", profile.runtime_path);
    out.Add("credential_ref", profile.credential_ref);
    out.Add("credential_source", McpTunnelCredentialSourceId(profile.credential_source));
    out.Add("auto_connect", profile.auto_connect);
    out.Add("remember_profile", profile.remember_profile);
    ValueArray services;
    for(const McpTunnelService& service : profile.services)
        services.Add(McpTunnelServiceToValue(service));
    out.Add("services", services);
    return out;
}

McpTunnelProfile McpTunnelProfileFromValue(const Value& value, int schema_version)
{
    McpTunnelProfile profile;
    if(!value.Is<ValueMap>())
        return profile;

    profile.id = AsString(value["id"]);
    profile.name = AsString(value["name"]);
    profile.machine_id = AsString(value["machine_id"]);
    profile.tunnel_id = AsString(value["tunnel_id"]);
    profile.runtime_path = AsString(value["runtime_path"]);
    profile.credential_ref = AsString(value["credential_ref"]);
    profile.credential_source = McpTunnelCredentialSourceFromId(AsString(value["credential_source"]));
    profile.auto_connect = !IsNull(value["auto_connect"]) && (bool)value["auto_connect"];
    profile.remember_profile = IsNull(value["remember_profile"]) || (bool)value["remember_profile"];

    Value services_value = value["services"];
    if(services_value.Is<ValueArray>()) {
        ValueArray services = services_value;
        for(int i = 0; i < services.GetCount(); ++i) {
            McpTunnelService service = McpTunnelServiceFromValue(services[i]);
            if(!service.id.IsEmpty())
                profile.services.Add(pick(service));
        }
    }

    // Schema 1 migration: the old TaskTrack profile contained one mcp_path.
    if(schema_version <= 1 && profile.services.IsEmpty()) {
        String old_mcp_path = AsString(value["mcp_path"]);
        if(!old_mcp_path.IsEmpty()) {
            McpTunnelService service;
            service.id = "tasktrack";
            service.name = "TaskTrack";
            service.channel = "main";
            service.command = McpTunnelCommandForExecutable(old_mcp_path);
            profile.services.Add(pick(service));
        }
        // Existing installs used the environment contract. Preserve that on migration.
        profile.credential_source = MCP_TUNNEL_CREDENTIAL_ENVIRONMENT;
    }

    if(profile.machine_id.IsEmpty())
        profile.machine_id = McpTunnelDefaultMachineId();
    return profile;
}

McpTunnelProfile McpTunnelDuplicateProfile(const McpTunnelProfile& source,
                                           const String& new_id,
                                           const String& new_name)
{
    McpTunnelProfile out;
    out.id = new_id;
    out.name = new_name;
    out.machine_id = source.machine_id;
    out.runtime_path = source.runtime_path;
    out.credential_source = MCP_TUNNEL_CREDENTIAL_SESSION;
    out.auto_connect = false;
    out.remember_profile = source.remember_profile;
    for(const McpTunnelService& source_service : source.services) {
        McpTunnelService service;
        service.id = source_service.id;
        service.name = source_service.name;
        service.channel = source_service.channel;
        service.command = source_service.command;
        service.enabled = source_service.enabled;
        out.services.Add(pick(service));
    }
    // Deliberately do not copy tunnel_id or any credential secret.
    return out;
}

McpTunnelSessionCredentials::Entry::~Entry()
{
    volatile byte* p = ~bytes;
    for(int i = 0; i < size; ++i)
        p[i] = 0;
}

bool McpTunnelSessionCredentials::Entry::Matches(const McpTunnelProfile& profile) const
{
    return ref == profile.credential_ref && profile_id == profile.id &&
           machine_id == profile.machine_id && tunnel_id == profile.tunnel_id &&
           runtime_path == profile.runtime_path;
}

bool McpTunnelSessionCredentials::Set(McpTunnelProfile& profile, const String& secret)
{
    if(secret.IsEmpty() || profile.id.IsEmpty() || profile.machine_id.IsEmpty() ||
       profile.tunnel_id.IsEmpty() || profile.runtime_path.IsEmpty())
        return false;
    Clear(profile);
    profile.credential_ref = AsString(Uuid::Create());
    Entry& entry = entries_.Add();
    entry.ref = profile.credential_ref;
    entry.profile_id = profile.id;
    entry.machine_id = profile.machine_id;
    entry.tunnel_id = profile.tunnel_id;
    entry.runtime_path = profile.runtime_path;
    entry.bytes.Alloc(secret.GetCount());
    entry.size = secret.GetCount();
    memcpy(~entry.bytes, ~secret, entry.size);
    return true;
}

bool McpTunnelSessionCredentials::Contains(const McpTunnelProfile& profile) const
{
    for(const Entry& entry : entries_)
        if(entry.Matches(profile))
            return true;
    return false;
}

String McpTunnelSessionCredentials::Read(const McpTunnelProfile& profile) const
{
    for(const Entry& entry : entries_)
        if(entry.Matches(profile))
            return String((const char*)~entry.bytes, entry.size);
    return String();
}

void McpTunnelSessionCredentials::Clear(McpTunnelProfile& profile)
{
    for(int i = entries_.GetCount() - 1; i >= 0; --i)
        if(entries_[i].profile_id == profile.id)
            entries_.Remove(i);
    profile.credential_ref.Clear();
}

void McpTunnelSessionCredentials::InvalidateChangedBinding(McpTunnelProfile& profile)
{
    if(!profile.credential_ref.IsEmpty() && !Contains(profile))
        Clear(profile);
}

bool McpTunnelValidateProfile(const McpTunnelProfile& profile, String& error)
{
    error.Clear();
    if(profile.id.Find('\0') >= 0 || profile.machine_id.Find('\0') >= 0 ||
       profile.tunnel_id.Find('\0') >= 0 || profile.runtime_path.Find('\0') >= 0) {
        error = "Profile identifiers and runtime path cannot contain NUL bytes.";
        return false;
    }
    if(profile.id.IsEmpty() || profile.name.IsEmpty()) {
        error = "Profile id and name are required.";
        return false;
    }
    if(profile.machine_id.IsEmpty()) {
        error = "Machine id is required.";
        return false;
    }
    if(profile.tunnel_id.IsEmpty()) {
        error = "Tunnel ID is not set.";
        return false;
    }
    if(profile.runtime_path.IsEmpty()) {
        error = "Tunnel runtime executable is not set.";
        return false;
    }

    Index<String> ids, channels;
    int main_count = 0;
    int enabled_count = 0;
    for(const McpTunnelService& service : profile.services) {
        if(!service.enabled)
            continue;
        enabled_count++;
        if(service.id.IsEmpty() || service.name.IsEmpty()) {
            error = "Every enabled service requires an id and display name.";
            return false;
        }
        if(ids.Find(service.id) >= 0) {
            error = "Enabled service ids must be unique.";
            return false;
        }
        ids.Add(service.id);
        if(!IsCanonicalChannel(service.channel)) {
            error = "MCP channel names may contain only letters, digits, '.', '_' and '-'.";
            return false;
        }
        if(service.channel == "harpoon") {
            error = "The MCP channel name 'harpoon' is reserved by the OpenAI tunnel runtime.";
            return false;
        }
        if(channels.Find(service.channel) >= 0) {
            error = "Enabled MCP channels must be unique.";
            return false;
        }
        channels.Add(service.channel);
        if(service.channel == "main")
            main_count++;
        if(service.command.IsEmpty()) {
            error = "Every enabled MCP service requires a command.";
            return false;
        }
#ifdef PLATFORM_WIN32
        String normalized_command = McpTunnelNormalizeServiceCommand(service.command);
        if(normalized_command.Find('\\') >= 0) {
            error = "Windows MCP service commands cannot contain raw backslashes because the tunnel runtime treats them as escapes. Use forward slashes for paths.";
            return false;
        }
#endif
        if(service.command.Find(',') >= 0 || service.command.Find('\n') >= 0 || service.command.Find('\r') >= 0) {
            error = "MCP service commands cannot contain commas or newlines in channel-qualified runtime bindings.";
            return false;
        }
    }
    if(enabled_count == 0) {
        error = "At least one MCP service must be enabled.";
        return false;
    }
    if(enabled_count > 32) {
        error = "The OpenAI tunnel runtime supports at most 32 enabled MCP channels.";
        return false;
    }
    if(main_count != 1) {
        error = "Exactly one enabled MCP service must use the main channel.";
        return false;
    }
    return true;
}

Vector<String> McpTunnelBuildRunArgs(const McpTunnelProfile& profile,
                                     const String& control_plane_api_key_ref,
                                     const String& health_url_file,
                                     const String& log_file)
{
    Vector<String> args;
    args.Add("run");
    args.Add("--control-plane.api-key");
    args.Add(control_plane_api_key_ref);
    args.Add("--control-plane.tunnel-id");
    args.Add(profile.tunnel_id);
    args.Add("--control-plane.base-url");
    args.Add("https://api.openai.com");
    args.Add("--log.http-raw-unsafe=false");

    for(const McpTunnelService& service : profile.services) {
        if(!service.enabled)
            continue;
        args.Add("--mcp.command");
        args.Add("channel=" + service.channel + ",command=" + McpTunnelNormalizeServiceCommand(service.command));
    }

    args.Add("--health.listen-addr");
    args.Add("127.0.0.1:0");
    args.Add("--health.url-file");
    args.Add(health_url_file);
    args.Add("--log.file");
    args.Add(log_file);
    return args;
}

String McpTunnelBuildChildEnvironment(const McpTunnelProfile& profile)
{
    return McpTunnelBuildChildEnvironment(profile, Environment());
}

String McpTunnelBuildChildEnvironment(const McpTunnelProfile& profile,
                                     const VectorMap<String, String>& environment)
{
    Vector<String> entries;
    // Only operating-system/session plumbing is inherited. In particular no
    // vendor config selectors, proxy overrides, loader paths or credential bags.
    static const char* allowed[] = {
        "PATH", "PATHEXT", "SYSTEMROOT", "WINDIR", "COMSPEC", "TEMP", "TMP",
        "USERPROFILE", "APPDATA", "LOCALAPPDATA", "PROGRAMDATA", "HOMEDRIVE",
        "HOMEPATH", "USERNAME", "COMPUTERNAME", "HOME", "USER", "LOGNAME",
        "TMPDIR", "LANG", "LC_ALL", "LC_CTYPE", "DISPLAY", "WAYLAND_DISPLAY",
        "XDG_RUNTIME_DIR", "DBUS_SESSION_BUS_ADDRESS", "XAUTHORITY"
    };
    Index<String> names;
    for(int i = 0; i < environment.GetCount(); ++i) {
        String name = environment.GetKey(i);
        String canonical = ToUpper(name);
        bool permit = false;
        for(const char* item : allowed)
            if(canonical == item) {
                permit = true;
                break;
            }
        if(!permit || names.Find(canonical) >= 0 || environment[i].Find('\0') >= 0)
            continue;
        names.Add(canonical);
        entries.Add(name + "=" + environment[i]);
    }

    entries.Add("MCP_TUNNEL_REMOTE=1");
    entries.Add("MCP_TUNNEL_MACHINE_ID=" + profile.machine_id);
    entries.Add("MCP_TUNNEL_PROFILE_ID=" + profile.id);
    Sort(entries);

    String block;
    for(const String& entry : entries) {
        block << entry;
        block.Cat(0);
    }
    block.Cat(0);
    return block;
}

McpTunnelRuntime::McpTunnelRuntime()
{
}

McpTunnelRuntime::~McpTunnelRuntime()
{
    Stop();
#ifdef PLATFORM_WIN32
    // Last-resort kill-on-close if the bounded stop could not confirm exit.
    // Keep the ownership handle until process exit in that exceptional case.
    if(job_)
        CloseHandle(job_);
#endif
    if(!health_url_file_.IsEmpty())
        DeleteFile(health_url_file_);
    if(!runtime_log_file_.IsEmpty())
        DeleteFile(runtime_log_file_);
}

bool McpTunnelRuntime::LoadHealthUrl()
{
    if(health_url_file_.IsEmpty() || !FileExists(health_url_file_))
        return false;

    String url = TrimBoth(LoadFile(health_url_file_));
    if(url.IsEmpty())
        return false;
    while(url.EndsWith("/"))
        url = url.Left(url.GetCount() - 1);
    health_url_ = url;
    return true;
}

void McpTunnelRuntime::DrainOutput()
{
    if(!started_)
        return;
    for(int i = 0; i < 8; ++i) {
        String out, err;
        process_.Read2(out, err);
        if(out.IsEmpty() && err.IsEmpty())
            break;
        runtime_output_ << out << err;
        if(runtime_output_.GetCount() > 6000)
            runtime_output_ = runtime_output_.Right(6000);
    }
}

bool McpTunnelRuntime::ProbeHealth(const String& suffix, int& status, String& error)
{
    status = 0;
    error.Clear();
    if(health_url_.IsEmpty() && !LoadHealthUrl()) {
        error = "Health URL is not available yet.";
        return false;
    }

    HttpRequest request(~(health_url_ + suffix));
    request.RequestTimeout(2000).MaxRetries(0).MaxRedirect(0).MaxContentSize(4096);
    request.Execute();
    status = request.GetStatusCode();
    if(request.IsSuccess())
        return true;
    error = request.GetErrorDesc();
    if(error.IsEmpty())
        error = Format("HTTP %d %s", status, request.GetReasonPhrase());
    return false;
}

bool McpTunnelRuntime::Start(const McpTunnelProfile& profile, const String& control_plane_api_key)
{
    Stop();
    if(started_) // A failed stop retains ownership; never start a second tree.
        return false;
    last_error_.Clear();

    String validation_error;
    if(!McpTunnelValidateProfile(profile, validation_error)) {
        last_error_ = validation_error;
        return false;
    }
    if(!FileExists(profile.runtime_path)) {
        last_error_ = "The OpenAI tunnel runtime executable was not found.";
        return false;
    }
    if(control_plane_api_key.IsEmpty() || control_plane_api_key.GetCount() > 4096) {
        last_error_ = "The control-plane API key must contain between 1 and 4096 bytes.";
        return false;
    }

    if(!health_url_file_.IsEmpty())
        DeleteFile(health_url_file_);
    if(!runtime_log_file_.IsEmpty())
        DeleteFile(runtime_log_file_);

    String key_reference;
#ifdef PLATFORM_WIN32
    if(!key_pipe_.Open(last_error_))
        return false;
    key_reference = key_pipe_.GetReference();
    String sid = McpTunnelWindowsUserSid();
    Vector<WCHAR> owner_name = ToSystemCharsetW("Global\\McpTunnelRuntime-" + sid);
    owner_name.Add(0);
    // Object existence is the lease (rather than recursively acquiring a mutex
    // twice on the GUI thread). Closing/crashing releases it automatically.
    owner_ = CreateMutexW(NULL, FALSE, owner_name.begin());
    DWORD owner_error = ::GetLastError();
    if(!owner_ || owner_error == ERROR_ALREADY_EXISTS) {
        if(owner_)
            CloseHandle(owner_);
        owner_ = NULL;
        key_pipe_.Close();
        last_error_ = "Another tunnel manager owns this Windows user's runtime, or ownership is unavailable.";
        return false;
    }
    job_ = CreateJobObjectW(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!job_ || !SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        Stop();
        last_error_ = "Cannot create the runtime process-tree container.";
        return false;
    }
#else
    // The official file: resolver can read the inherited anonymous stdin pipe.
    // Fail closed on platforms without this device, never write a secret file.
    struct stat stdin_info;
    if(lstat("/dev/stdin", &stdin_info) != 0) {
        last_error_ = "This platform has no /dev/stdin credential pipe reference.";
        return false;
    }
    key_reference = "file:/dev/stdin";
#endif

    health_url_file_ = GetTempFileName("mcp-tunnel-health-");
    SaveFile(health_url_file_, "");
    runtime_log_file_ = GetTempFileName("mcp-tunnel-runtime-");
    DeleteFile(runtime_log_file_);
    runtime_output_.Clear();
    health_url_.Clear();
    healthy_ = false;
    ready_ = false;

    Vector<String> args = McpTunnelBuildRunArgs(profile, key_reference, health_url_file_, runtime_log_file_);
    String child_environment = McpTunnelBuildChildEnvironment(profile);
    process_.NoConvertCharset();
    bool launched = process_.Start(~McpTunnelCommandForExecutable(profile.runtime_path), args, ~child_environment);
    child_environment.Clear();

    if(!launched) {
        Stop();
        last_error_ = "Unable to start the OpenAI tunnel runtime.";
        return false;
    }

    started_ = true;
#ifdef PLATFORM_WIN32
    // The unchanged vendor blocks reading the credential before it creates MCP
    // children. Assign the job BEFORE releasing any key bytes. Fail closed if
    // the host's job policy does not allow containment.
    if(!AssignProcessToJobObject(job_, process_.GetProcessHandle())) {
        Stop();
        last_error_ = "Cannot contain the runtime process tree; no credential was released.";
        return false;
    }
    String handoff_error;
    if(!key_pipe_.Send(control_plane_api_key, GetProcessId(process_.GetProcessHandle()), 5000, handoff_error)) {
        Stop();
        last_error_ = handoff_error;
        return false;
    }
#else
    process_.Write(control_plane_api_key);
#endif
    process_.CloseWrite();
    for(int i = 0; i < 40; ++i) {
        DrainOutput();
        if(LoadHealthUrl()) {
            break;
        }
        if(!process_.IsRunning())
            break;
        Sleep(100);
    }
    Refresh();
    return started_;
}

void McpTunnelRuntime::Refresh()
{
    DrainOutput();
#ifdef PLATFORM_WIN32
    bool running = started_ && WaitForSingleObject(process_.GetProcessHandle(), 0) == WAIT_TIMEOUT;
#else
    bool running = started_ && process_.IsRunning();
#endif
    if(!running) {
        if(started_) {
            int code = process_.GetExitCode();
#ifdef PLATFORM_WIN32
            DWORD actual_code = 0;
            if(GetExitCodeProcess(process_.GetProcessHandle(), &actual_code))
                code = (int)actual_code;
#endif
            Stop();
            last_error_ = Format("Tunnel runtime exited with code %d.", code);
        }
        healthy_ = false;
        ready_ = false;
        return;
    }

    LoadHealthUrl();
    int health_status = 0, ready_status = 0;
    String health_error, ready_error;
    healthy_ = ProbeHealth("/healthz", health_status, health_error);
    ready_ = ProbeHealth("/readyz", ready_status, ready_error);

    if(ready_ || healthy_)
        last_error_.Clear();
    else if(!health_error.IsEmpty())
        last_error_ = health_error;
}

void McpTunnelRuntime::Stop()
{
#ifdef PLATFORM_WIN32
    key_pipe_.Close();
    if(job_) {
        TerminateJobObject(job_, 255);
        bool empty = false;
        for(int i = 0; i < 500; ++i) {
            JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting = {};
            if(QueryInformationJobObject(job_, JobObjectBasicAccountingInformation,
                                         &accounting, sizeof(accounting), NULL) && !accounting.ActiveProcesses) {
                empty = true;
                break;
            }
            Sleep(10);
        }
        if(!empty) {
            started_ = true;
            healthy_ = ready_ = false;
            last_error_ = "Runtime process-tree shutdown did not complete; ownership retained. Retry Stop.";
            return;
        }
        CloseHandle(job_);
        job_ = NULL;
    }
#endif
    if(started_)
        process_.Kill();
#ifdef PLATFORM_WIN32
    if(owner_) {
        CloseHandle(owner_);
        owner_ = NULL;
    }
#endif
    started_ = false;
    healthy_ = false;
    ready_ = false;
    health_url_.Clear();
    last_error_.Clear();
}

McpTunnelRuntime::State McpTunnelRuntime::GetState() const
{
    if(!last_error_.IsEmpty() && !ready_)
        return FAULT;
    if(ready_)
        return READY;
    if(started_)
        return CONNECTING;
    return STOPPED;
}

String McpTunnelRuntime::GetDiagnostics() const
{
    String out = runtime_output_;
    String log = runtime_log_file_.IsEmpty() ? String() : LoadFile(runtime_log_file_);
    if(!IsNull(log) && !log.IsEmpty()) {
        if(log.GetCount() > 3000)
            log = log.Right(3000);
        if(!out.IsEmpty())
            out << "\n";
        out << log;
    }
    return out;
}

}
