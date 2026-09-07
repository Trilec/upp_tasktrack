#include "McpTunnelRuntime.h"

#ifdef PLATFORM_WIN32
#include <windows.h>
#include <wincred.h>
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
        out.Trim(out.GetCount() - 1);
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

#ifdef PLATFORM_WIN32
String CredentialTarget(const McpTunnelProfile& profile)
{
    return profile.credential_ref.IsEmpty()
        ? McpTunnelDefaultCredentialRef(profile.id)
        : profile.credential_ref;
}

String WinErrorText(const char *action)
{
    return Format("%s failed (Windows error %lu).", action, (unsigned long)GetLastError());
}
#endif

}

String McpTunnelCredentialSourceId(McpTunnelCredentialSource source)
{
    return source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT ? "environment" : "windows_credential_manager";
}

McpTunnelCredentialSource McpTunnelCredentialSourceFromId(const String& id)
{
    return id == "environment" ? MCP_TUNNEL_CREDENTIAL_ENVIRONMENT
                               : MCP_TUNNEL_CREDENTIAL_WINDOWS;
}

String McpTunnelDefaultCredentialRef(const String& profile_id)
{
    String id = NormalizedId(profile_id);
    if(id.IsEmpty())
        id = "default";
    return "Trilec.McpTunnel/" + id;
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

ValueMap McpTunnelServiceToValue(const McpTunnelService& service)
{
    ValueMap out;
    out.Add("id", service.id);
    out.Add("name", service.name);
    out.Add("channel", service.channel);
    out.Add("command", service.command);
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
    service.command = AsString(value["command"]);
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
    out.Add("credential_source", McpTunnelCredentialSourceId(profile.credential_source));
    out.Add("credential_ref", profile.credential_ref);
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
    profile.credential_source = McpTunnelCredentialSourceFromId(AsString(value["credential_source"]));
    profile.credential_ref = AsString(value["credential_ref"]);
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
            service.command = old_mcp_path;
            profile.services.Add(pick(service));
        }
        // Existing installs used the environment contract. Preserve that on migration.
        profile.credential_source = MCP_TUNNEL_CREDENTIAL_ENVIRONMENT;
    }

    if(profile.machine_id.IsEmpty())
        profile.machine_id = McpTunnelDefaultMachineId();
    if(profile.credential_ref.IsEmpty())
        profile.credential_ref = McpTunnelDefaultCredentialRef(profile.id);
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
    out.credential_source = source.credential_source;
    out.credential_ref = McpTunnelDefaultCredentialRef(new_id);
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

bool McpTunnelValidateProfile(const McpTunnelProfile& profile, String& error)
{
    error.Clear();
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
    }
    if(enabled_count == 0) {
        error = "At least one MCP service must be enabled.";
        return false;
    }
    if(main_count != 1) {
        error = "Exactly one enabled MCP service must use the main channel.";
        return false;
    }
    return true;
}

Vector<String> McpTunnelBuildRunArgs(const McpTunnelProfile& profile,
                                     const String& health_url_file,
                                     const String& log_file)
{
    Vector<String> args;
    args.Add("run");
    args.Add("--control-plane.api-key");
    args.Add("env:CONTROL_PLANE_API_KEY");
    args.Add("--control-plane.tunnel-id");
    args.Add(profile.tunnel_id);

    for(const McpTunnelService& service : profile.services) {
        if(!service.enabled)
            continue;
        args.Add("--mcp.command");
        args.Add("channel=" + service.channel + ",command=" + service.command);
    }

    args.Add("--health.listen-addr");
    args.Add("127.0.0.1:0");
    args.Add("--health.url-file");
    args.Add(health_url_file);
    args.Add("--log.file");
    args.Add(log_file);
    return args;
}

String McpTunnelBuildChildEnvironment(const McpTunnelProfile& profile,
                                      const String& control_plane_api_key)
{
    String block;
    const VectorMap<String, String>& environment = Environment();
    for(int i = 0; i < environment.GetCount(); ++i) {
        String name = environment.GetKey(i);
        if(!CompareNoCase(name, "CONTROL_PLANE_API_KEY") ||
           !CompareNoCase(name, "MCP_TUNNEL_REMOTE") ||
           !CompareNoCase(name, "MCP_TUNNEL_MACHINE_ID") ||
           !CompareNoCase(name, "MCP_TUNNEL_PROFILE_ID") ||
           !CompareNoCase(name, "TASKTRACK_TUNNEL_REMOTE"))
            continue;
        block << name << "=" << environment[i];
        block.Cat(0);
    }

    block << "CONTROL_PLANE_API_KEY=" << control_plane_api_key;
    block.Cat(0);
    block << "MCP_TUNNEL_REMOTE=1";
    block.Cat(0);
    block << "MCP_TUNNEL_MACHINE_ID=" << profile.machine_id;
    block.Cat(0);
    block << "MCP_TUNNEL_PROFILE_ID=" << profile.id;
    block.Cat(0);
    block.Cat(0);
    return block;
}

bool McpTunnelCredentialExists(const McpTunnelProfile& profile, String& error)
{
    error.Clear();
    if(profile.credential_source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT) {
        if(GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()) {
            error = "CONTROL_PLANE_API_KEY is not set.";
            return false;
        }
        return true;
    }

#ifdef PLATFORM_WIN32
    String target = CredentialTarget(profile);
    PCREDENTIALA credential = nullptr;
    if(!CredReadA(~target, CRED_TYPE_GENERIC, 0, &credential)) {
        if(GetLastError() == ERROR_NOT_FOUND)
            error = "No Windows Credential Manager key is stored for this profile.";
        else
            error = WinErrorText("CredRead");
        return false;
    }
    CredFree(credential);
    return true;
#else
    error = "Windows Credential Manager is only available on Windows.";
    return false;
#endif
}

bool McpTunnelReadCredential(const McpTunnelProfile& profile, String& secret, String& error)
{
    secret.Clear();
    error.Clear();

    if(profile.credential_source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT) {
        secret = GetEnv("CONTROL_PLANE_API_KEY");
        if(secret.IsEmpty()) {
            error = "CONTROL_PLANE_API_KEY is not set.";
            return false;
        }
        return true;
    }

#ifdef PLATFORM_WIN32
    String target = CredentialTarget(profile);
    PCREDENTIALA credential = nullptr;
    if(!CredReadA(~target, CRED_TYPE_GENERIC, 0, &credential)) {
        if(GetLastError() == ERROR_NOT_FOUND)
            error = "No Windows Credential Manager key is stored for this profile.";
        else
            error = WinErrorText("CredRead");
        return false;
    }
    secret = String((const char *)credential->CredentialBlob, (int)credential->CredentialBlobSize);
    CredFree(credential);
    if(secret.IsEmpty()) {
        error = "The stored Windows credential is empty.";
        return false;
    }
    return true;
#else
    error = "Windows Credential Manager is only available on Windows.";
    return false;
#endif
}

bool McpTunnelWriteCredential(const McpTunnelProfile& profile, const String& secret, String& error)
{
    error.Clear();
    if(profile.credential_source != MCP_TUNNEL_CREDENTIAL_WINDOWS) {
        error = "This profile uses the environment credential source.";
        return false;
    }
    if(secret.IsEmpty()) {
        error = "API key cannot be empty.";
        return false;
    }

#ifdef PLATFORM_WIN32
    String target = CredentialTarget(profile);
    CREDENTIALA credential;
    Zero(credential);
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<char *>(~target);
    credential.CredentialBlobSize = (DWORD)secret.GetCount();
    credential.CredentialBlob = (LPBYTE)~secret;
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<char *>("OpenAI Secure MCP Tunnel");
    if(!CredWriteA(&credential, 0)) {
        error = WinErrorText("CredWrite");
        return false;
    }
    return true;
#else
    error = "Windows Credential Manager is only available on Windows.";
    return false;
#endif
}

bool McpTunnelDeleteCredential(const McpTunnelProfile& profile, String& error)
{
    error.Clear();
    if(profile.credential_source != MCP_TUNNEL_CREDENTIAL_WINDOWS)
        return true;
#ifdef PLATFORM_WIN32
    String target = CredentialTarget(profile);
    if(CredDeleteA(~target, CRED_TYPE_GENERIC, 0))
        return true;
    if(GetLastError() == ERROR_NOT_FOUND)
        return true;
    error = WinErrorText("CredDelete");
    return false;
#else
    error = "Windows Credential Manager is only available on Windows.";
    return false;
#endif
}


McpTunnelRuntime::McpTunnelRuntime()
{
}

McpTunnelRuntime::~McpTunnelRuntime()
{
    Stop();
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
    request.Timeout(2000);
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
    if(control_plane_api_key.IsEmpty()) {
        last_error_ = "The control-plane API key is not available.";
        return false;
    }

    health_url_file_ = GetTempFileName("mcp-tunnel-health-");
    SaveFile(health_url_file_, "");
    runtime_log_file_ = GetTempFileName("mcp-tunnel-runtime-");
    DeleteFile(runtime_log_file_);
    runtime_output_.Clear();
    health_url_.Clear();
    healthy_ = false;
    ready_ = false;

    Vector<String> args = McpTunnelBuildRunArgs(profile, health_url_file_, runtime_log_file_);
    String child_environment = McpTunnelBuildChildEnvironment(profile, control_plane_api_key);
    bool launched = process_.Start(~profile.runtime_path, args, ~child_environment);
    child_environment.Clear();

    if(!launched) {
        last_error_ = "Unable to start the OpenAI tunnel runtime.";
        return false;
    }

    started_ = true;
    for(int i = 0; i < 40; ++i) {
        DrainOutput();
        if(LoadHealthUrl() || !process_.IsRunning())
            break;
        Sleep(100);
    }
    Refresh();
    return started_;
}

void McpTunnelRuntime::Refresh()
{
    DrainOutput();
    bool running = started_ && process_.IsRunning();
    if(!running) {
        if(started_) {
            String output;
            int code = process_.Finish(output);
            runtime_output_ << output;
            last_error_ = Format("Tunnel runtime exited with code %d.", code);
            process_.Kill();
        }
        started_ = false;
        healthy_ = false;
        ready_ = false;
        return;
    }

    LoadHealthUrl();
    int health_status = 0, ready_status = 0;
    String health_error, ready_error;
    healthy_ = ProbeHealth("/healthz", health_status, health_error);
    ready_ = ProbeHealth("/readyz", ready_status, ready_error);

    if(ready_)
        last_error_.Clear();
    else if(!healthy_ && !health_error.IsEmpty())
        last_error_ = health_error;
}

void McpTunnelRuntime::Stop()
{
    if(started_)
        process_.Kill();
    started_ = false;
    healthy_ = false;
    ready_ = false;
    health_url_.Clear();
    last_error_.Clear();
}

McpTunnelRuntime::State McpTunnelRuntime::GetState() const
{
    if(!last_error_.IsEmpty() && !ready_)
        return ERROR;
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
