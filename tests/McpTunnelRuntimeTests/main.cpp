#include <Core/Core.h>
#include <McpTunnelRuntime/McpTunnelRuntime.h>

using namespace Upp;

namespace {

struct TestState {
    int passed = 0;
    Vector<String> failed;

    void Check(bool condition, const String& message)
    {
        if(condition)
            passed++;
        else
            failed.Add(message);
    }
};

McpTunnelProfile MakeProfile()
{
    McpTunnelProfile profile;
    profile.id = "curt-main";
    profile.name = "Curt Workstation";
    profile.machine_id = "curt-main";
    profile.tunnel_id = "tunnel_test";
    profile.runtime_path = "C:/tools/tunnel-client.exe";
    profile.credential_source = MCP_TUNNEL_CREDENTIAL_SESSION;

    McpTunnelService tasktrack;
    tasktrack.id = "tasktrack";
    tasktrack.name = "TaskTrack";
    tasktrack.channel = "main";
    tasktrack.command = "C:/apps/TaskTrackMcp.exe";
    profile.services.Add(pick(tasktrack));

    McpTunnelService patchtrack;
    patchtrack.id = "patchtrack";
    patchtrack.name = "PatchTrack";
    patchtrack.channel = "patchtrack";
    patchtrack.command = "C:/apps/patchtrack_mcp.exe";
    profile.services.Add(pick(patchtrack));
    return profile;
}

bool HasArgPair(const Vector<String>& args, const String& flag, const String& value)
{
    for(int i = 0; i + 1 < args.GetCount(); ++i)
        if(args[i] == flag && args[i + 1] == value)
            return true;
    return false;
}

}

CONSOLE_APP_MAIN
{
#ifdef PLATFORM_WIN32
    const Vector<String>& command = CommandLine();
    if(command.GetCount() && command[0] == "--owned-child") {
        Sleep(60000);
        return;
    }
    if(command.GetCount() && command[0] == "run") {
        // Controllable runtime fixture: read the key before spawning children,
        // mirroring the vendor's configuration-before-lifecycle ordering.
        String reference, health_file;
        for(int i = 0; i + 1 < command.GetCount(); ++i) {
            if(command[i] == "--control-plane.api-key") reference = command[i + 1];
            if(command[i] == "--health.url-file") health_file = command[i + 1];
        }
        Vector<WCHAR> path = ToSystemCharsetW(reference.Mid(5));
        path.Add(0);
        HANDLE pipe = CreateFileW(path.begin(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(pipe == INVALID_HANDLE_VALUE) { SetExitCode(2); return; }
        char bytes[4096];
        DWORD count;
        while(ReadFile(pipe, bytes, sizeof(bytes), &count, NULL) && count) {}
        CloseHandle(pipe);
        Vector<String> args;
        args.Add("--owned-child");
        LocalProcess child;
        if(!child.Start(~McpTunnelCommandForExecutable(GetExeFilePath()), args)) { SetExitCode(3); return; }
        Cout() << "owned-child-pid:" << (int)GetProcessId(child.GetProcessHandle()) << "\n";
        Cout() << "owned-root-pid:" << (int)GetCurrentProcessId() << "\n";
        Cout().Flush();
        SaveFile(health_file, "http://127.0.0.1:1");
        Sleep(60000);
        return;
    }
    if(command.GetCount() && command[0] == "--manager-fixture") {
        McpTunnelRuntime runtime;
        McpTunnelProfile profile = MakeProfile();
        profile.runtime_path = GetExeFilePath();
        if(!runtime.Start(profile, "dummy-pipe-secret")) { SetExitCode(4); return; }
        Cout() << runtime.GetRuntimeOutput();
        Cout().Flush();
        Sleep(60000);
        return;
    }
    if(command.GetCount() == 2 && command[0] == "--pipe-reader") {
        Vector<WCHAR> path = ToSystemCharsetW(command[1]);
        path.Add(0);
        HANDLE pipe = CreateFileW(path.begin(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        String value;
        if(pipe != INVALID_HANDLE_VALUE) {
            char bytes[128];
            DWORD count;
            while(ReadFile(pipe, bytes, sizeof(bytes), &count, NULL) && count && value.GetCount() < 8192)
                value.Cat(bytes, count);
            CloseHandle(pipe);
        }
        bool ok = value == "dummy-pipe-secret";
        Cout() << (ok ? "pipe-read-ok" : "pipe-read-failed") << "\n";
        SetExitCode(ok ? 0 : 1);
        return;
    }
#endif
    TestState t;

#ifdef PLATFORM_WIN32
    auto pipe_test = [&](bool wrong_reader) {
        McpTunnelKeyPipe pipe;
        String error;
        bool opened = pipe.Open(error);
        t.Check(opened, "private key pipe creation failed");
        if(!opened)
            return;
        Vector<String> args;
        args.Add("--pipe-reader");
        args.Add(pipe.GetReference().Mid(5));
        LocalProcess reader;
        bool launched = reader.Start(~McpTunnelCommandForExecutable(GetExeFilePath()), args);
        t.Check(launched, "key pipe fixture launch failed");
        if(!launched)
            return;
        bool sent = pipe.Send("dummy-pipe-secret",
                              wrong_reader ? GetCurrentProcessId() : GetProcessId(reader.GetProcessHandle()),
                              1000, error);
        t.Check(sent != wrong_reader, "pipe accepted wrong reader or rejected intended reader");
        t.Check(pipe.GetReference().IsEmpty(), "pipe reference retained after handoff");
        String output;
        for(int i = 0; i < 200 && reader.IsRunning(); ++i) {
            String part;
            reader.Read(part);
            output << part;
            Sleep(10);
        }
        t.Check(!reader.IsRunning(), "pipe reader did not receive EOF");
        if(!reader.IsRunning()) {
            String part;
            reader.Read(part);
            output << part;
            t.Check((reader.GetExitCode() == 0) != wrong_reader, "pipe fixture got unexpected secret bytes");
        }
        t.Check(output.Find("dummy-pipe-secret") < 0, "fixture echoed a secret");
        reader.Kill();
    };
    pipe_test(false);
    pipe_test(true);
    McpTunnelKeyPipe unattended;
    String pipe_error;
    t.Check(unattended.Open(pipe_error), "timeout pipe creation failed");
    t.Check(!unattended.Send("dummy-pipe-secret", GetCurrentProcessId(), 50, pipe_error),
            "pipe with no reader did not time out");
    t.Check(unattended.GetReference().IsEmpty(), "timeout left pipe open");

    auto pid_from_output = [](const String& output, const char* marker) -> DWORD {
        int at = output.Find(marker);
        return at < 0 ? 0 : (DWORD)atoi(~output.Mid(at + (int)strlen(marker)));
    };
    McpTunnelProfile fixture = MakeProfile();
    fixture.runtime_path = GetExeFilePath();
    for(int cycle = 0; cycle < 2; ++cycle) {
        McpTunnelRuntime runtime, duplicate_owner;
        bool started = runtime.Start(fixture, "dummy-pipe-secret");
        t.Check(started, "contained runtime fixture did not start: " + runtime.GetLastError());
        if(!started)
            continue;
        runtime.Refresh();
        DWORD child_pid = pid_from_output(runtime.GetRuntimeOutput(), "owned-child-pid:");
        HANDLE child = child_pid ? OpenProcess(SYNCHRONIZE, FALSE, child_pid) : NULL;
        t.Check(child != NULL, "fixture child was not running");
        t.Check(!duplicate_owner.Start(fixture, "dummy-pipe-secret"), "second manager acquired runtime ownership");
        if(cycle == 1) {
            DWORD root_pid = pid_from_output(runtime.GetRuntimeOutput(), "owned-root-pid:");
            HANDLE root = root_pid ? OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, root_pid) : NULL;
            t.Check(root != NULL, "fixture root handle unavailable");
            if(root) {
                TerminateProcess(root, 77);
                WaitForSingleObject(root, 2000);
                CloseHandle(root);
                runtime.Refresh();
                t.Check(!runtime.IsStarted(), "runtime exit was not detected");
                t.Check(runtime.GetLastError().Find("77") >= 0, "runtime exit code lost");
            }
        }
        runtime.Stop();
        t.Check(!runtime.IsStarted(), "stop retained a running fixture");
        if(child) {
            t.Check(WaitForSingleObject(child, 2000) == WAIT_OBJECT_0, "stop/runtime crash orphaned child");
            CloseHandle(child);
        }
    }
    {
        LocalProcess manager;
        Vector<String> args;
        args.Add("--manager-fixture");
        bool launched = manager.Start(~McpTunnelCommandForExecutable(GetExeFilePath()), args);
        t.Check(launched, "manager crash fixture launch failed");
        if(launched) {
            String output;
            DWORD child_pid = 0;
            for(int i = 0; i < 1000 && !child_pid; ++i) {
                String part;
                manager.Read(part);
                output << part;
                child_pid = pid_from_output(output, "owned-child-pid:");
                Sleep(10);
            }
            HANDLE child = child_pid ? OpenProcess(SYNCHRONIZE, FALSE, child_pid) : NULL;
            t.Check(child != NULL, "manager crash fixture child was not running");
            HANDLE manager_exit = NULL;
            DuplicateHandle(GetCurrentProcess(), manager.GetProcessHandle(), GetCurrentProcess(),
                            &manager_exit, SYNCHRONIZE, FALSE, 0);
            manager.Kill();
            if(manager_exit) {
                t.Check(WaitForSingleObject(manager_exit, 2000) == WAIT_OBJECT_0,
                        "manager termination did not complete");
                CloseHandle(manager_exit);
            }
            if(child) {
                t.Check(WaitForSingleObject(child, 2000) == WAIT_OBJECT_0, "manager crash orphaned descendant");
                CloseHandle(child);
            }
            McpTunnelRuntime recovered;
            t.Check(recovered.Start(fixture, "dummy-pipe-secret"), "manager crash retained ownership lock");
            recovered.Stop();
        }
    }

    // Optional unchanged-vendor compatibility proof. Deliberately invalid MCP
    // command fails startup; the control-plane endpoint is loopback, never OpenAI.
    if(command.GetCount() == 2 && command[0] == "--vendor-runtime") {
        McpTunnelKeyPipe pipe;
        String error;
        bool opened = pipe.Open(error);
        t.Check(opened, "vendor pipe creation failed");
        if(opened) {
            Vector<String> args;
            for(const char* arg : {"run", "--control-plane.api-key"})
                args.Add(arg);
            args.Add(pipe.GetReference());
            for(const char* arg : {"--control-plane.tunnel-id", "tunnel_0123456789abcdef0123456789abcdef",
                                   "--control-plane.base-url", "http://127.0.0.1:1",
                                   "--health.listen-addr", "127.0.0.1:0",
                                   "--mcp.command", "nonexistent-tasktrack-security-fixture.exe"})
                args.Add(arg);
            LocalProcess vendor;
            bool launched = vendor.Start(~McpTunnelCommandForExecutable(command[1]), args,
                                          ~McpTunnelBuildChildEnvironment(MakeProfile()));
            t.Check(launched, "vendor runtime failed to launch");
            if(launched) {
                t.Check(pipe.Send("dummy-pipe-secret", GetProcessId(vendor.GetProcessHandle()), 3000, error),
                        "vendor runtime did not consume key pipe");
                String output;
                for(int i = 0; i < 500 && vendor.IsRunning(); ++i) {
                    String part;
                    vendor.Read(part);
                    output << part;
                    Sleep(10);
                }
                String tail;
                vendor.Read(tail);
                output << tail;
                t.Check(!vendor.IsRunning(), "vendor fixture failed to exit within deadline");
                t.Check(output.Find("start stdio command") >= 0,
                        "vendor did not get past key resolution to expected MCP launch failure");
                t.Check(output.Find("dummy-pipe-secret") < 0, "vendor logs exposed dummy key");
                vendor.Kill();
            }
        }
    }
#endif

    McpTunnelSessionCredentials credentials;
    McpTunnelProfile owner = MakeProfile();
    t.Check(credentials.Set(owner, "dummy-owner-key"), "session key could not be bound");
    String first_ref = owner.credential_ref;
    t.Check(credentials.Read(owner) == "dummy-owner-key", "bound key unavailable");
    McpTunnelProfile other = McpTunnelDuplicateProfile(owner, "other", "Other");
    other.tunnel_id = owner.tunnel_id;
    t.Check(other.credential_ref.IsEmpty() && !credentials.Contains(other),
            "duplicate inherited session credential");
    t.Check(credentials.Set(other, "dummy-other-key"), "second key could not be bound");
    t.Check(credentials.Read(owner) == "dummy-owner-key", "second profile replaced first key");
    owner.name = "Renamed";
    t.Check(credentials.Contains(owner), "display rename invalidated credential");
    owner.tunnel_id = "changed-tunnel";
    t.Check(!credentials.Contains(owner), "changed tunnel could read old key");
    credentials.InvalidateChangedBinding(owner);
    t.Check(owner.credential_ref.IsEmpty(), "changed binding retained reference");
    owner.tunnel_id = "tunnel_test";
    t.Check(!credentials.Contains(owner), "restoring tunnel resurrected invalidated key");
    credentials.Set(owner, "dummy-replacement");
    t.Check(owner.credential_ref != first_ref, "replacement reused credential reference");
    owner.runtime_path = "C:/other/runtime.exe";
    t.Check(!credentials.Contains(owner), "different executable could receive key");
    credentials.InvalidateChangedBinding(owner);
    credentials.Set(owner, "dummy-new-runtime");
    owner.machine_id = "different-machine";
    t.Check(!credentials.Contains(owner), "different machine could receive key");
    credentials.Clear(other);
    t.Check(!credentials.Contains(other) && other.credential_ref.IsEmpty(), "clear retained key");
    McpTunnelProfile automation = MakeProfile();
    automation.credential_source = MCP_TUNNEL_CREDENTIAL_ENVIRONMENT;
    McpTunnelProfile automation_copy = McpTunnelDuplicateProfile(automation, "copy", "Copy");
    t.Check(automation_copy.credential_source == MCP_TUNNEL_CREDENTIAL_SESSION,
            "duplicate implicitly opted into shared automation key");

    McpTunnelProfile profile = MakeProfile();
    String error;
    t.Check(McpTunnelCommandForExecutable("C:/Program Files/TaskTrack/TaskTrackMcp.exe") ==
            "\"C:/Program Files/TaskTrack/TaskTrackMcp.exe\"",
            "executable command with spaces was not quoted");
    t.Check(McpTunnelCommandForExecutable("C:\\TaskTrack\\TaskTrackMcp.exe") ==
            "C:/TaskTrack/TaskTrackMcp.exe",
            "executable command did not normalize Windows separators");
#ifdef PLATFORM_WIN32
    t.Check(McpTunnelNormalizeServiceCommand("E:\\apps\\github\\upp_patchtrack\\bin\\windows-x64\\PatchTrackMcp.exe") ==
            "E:/apps/github/upp_patchtrack/bin/windows-x64/PatchTrackMcp.exe",
            "pasted Windows MCP executable path was not canonicalized");
    t.Check(McpTunnelNormalizeServiceCommand("E:\\Program Files\\PatchTrack\\PatchTrackMcp.exe") ==
            "\"E:/Program Files/PatchTrack/PatchTrackMcp.exe\"",
            "Windows MCP executable path with spaces was not canonicalized/quoted");
    McpTunnelProfile windows_path = MakeProfile();
    windows_path.services[1].command = "E:\\apps\\github\\upp_patchtrack\\bin\\windows-x64\\PatchTrackMcp.exe";
    Vector<String> windows_args = McpTunnelBuildRunArgs(windows_path, "file:/dev/stdin", "health.url", "runtime.log");
    t.Check(HasArgPair(windows_args, "--mcp.command",
                       "channel=patchtrack,command=E:/apps/github/upp_patchtrack/bin/windows-x64/PatchTrackMcp.exe"),
            "runtime boundary did not canonicalize persisted Windows PatchTrack command");
#endif
    t.Check(McpTunnelValidateProfile(profile, error), "valid two-service machine profile rejected: " + error);

    Vector<String> args = McpTunnelBuildRunArgs(profile, "file:/dev/stdin", "health.url", "runtime.log");
    t.Check(HasArgPair(args, "--control-plane.tunnel-id", "tunnel_test"),
            "runtime args missing tunnel id");
    t.Check(HasArgPair(args, "--control-plane.api-key", "file:/dev/stdin"),
            "runtime args do not preserve the pipe credential reference");
    t.Check(HasArgPair(args, "--mcp.command",
                       "channel=main,command=C:/apps/TaskTrackMcp.exe"),
            "runtime args missing main TaskTrack binding");
    t.Check(HasArgPair(args, "--mcp.command",
                       "channel=patchtrack,command=C:/apps/patchtrack_mcp.exe"),
            "runtime args missing PatchTrack channel binding");
    t.Check(HasArgPair(args, "--health.url-file", "health.url"),
            "runtime args missing health URL file");

    String child_env = McpTunnelBuildChildEnvironment(profile);
    VectorMap<String, String> seeded_env;
    seeded_env.Add("SystemRoot", "C:/Windows");
    seeded_env.Add("PATH", "C:/tools");
    for(const char* name : {"CONTROL_PLANE_API_KEY", "OPENAI_API_KEY", "OPENAI_ADMIN_KEY",
                           "PRIVATE_SERVICE_KEY", "CONTROL_PLANE_BASE_URL",
                           "TUNNEL_CLIENT_PROFILE_FILE", "LOG_HTTP_RAW_UNSAFE", "MCP_COMMAND",
                           "HTTPS_PROXY", "LD_PRELOAD", "PYTHONPATH", "mcp_tunnel_remote"})
        seeded_env.Add(name, "dummy-sensitive-or-unsafe");
    String filtered_env = McpTunnelBuildChildEnvironment(profile, seeded_env);
    t.Check(filtered_env.Find("dummy-sensitive-or-unsafe") < 0,
            "allowlist leaked seeded secret or vendor configuration");
    t.Check(filtered_env.Find("SystemRoot=C:/Windows") >= 0 && filtered_env.Find("PATH=C:/tools") >= 0,
            "allowlist removed required OS environment");
    t.Check(filtered_env.Find("MCP_TUNNEL_REMOTE=1") >= 0,
            "ambient environment replaced remote marker");
    t.Check(HasArgPair(args, "--control-plane.base-url", "https://api.openai.com"),
            "launch did not pin control-plane destination");
    t.Check(FindIndex(args, String("--log.http-raw-unsafe=false")) >= 0,
            "raw HTTP logging was not explicitly disabled");
    McpTunnelProfile injected_env = MakeProfile();
    injected_env.machine_id.Cat(0);
    injected_env.machine_id << "CONTROL_PLANE_BASE_URL=https://unexpected.invalid";
    t.Check(!McpTunnelValidateProfile(injected_env, error), "profile accepted environment injection");
    t.Check(child_env.Find("CONTROL_PLANE_API_KEY=") < 0,
            "runtime child environment exposes CONTROL_PLANE_API_KEY");
    t.Check(child_env.Find("OPENAI_API_KEY=") < 0,
            "runtime child environment exposes OPENAI_API_KEY");
    t.Check(child_env.Find("OPENAI_ADMIN_KEY=") < 0,
            "runtime child environment exposes OPENAI_ADMIN_KEY");
    t.Check(child_env.Find("MCP_TUNNEL_REMOTE=1") >= 0,
            "child environment missing generic remote marker");
    t.Check(child_env.Find("MCP_TUNNEL_MACHINE_ID=curt-main") >= 0,
            "child environment missing machine identity");
    t.Check(FindIndex(args, String("test-secret")) < 0,
            "runtime argv contains an API key value");

    McpTunnelProfile duplicate = McpTunnelDuplicateProfile(profile, "curt-copy", "Curt copy");
    t.Check(duplicate.tunnel_id.IsEmpty(), "duplicated profile copied tunnel id");
    t.Check(!duplicate.auto_connect, "duplicated profile copied auto-connect");
    t.Check(duplicate.services.GetCount() == 2, "duplicated profile lost service bindings");
    t.Check(duplicate.services.GetCount() == 2 && duplicate.services[1].channel == "patchtrack",
            "duplicated profile changed service channel");

    ValueMap value = McpTunnelProfileToValue(profile);
    McpTunnelProfile roundtrip = McpTunnelProfileFromValue(value, 2);
    t.Check(roundtrip.machine_id == profile.machine_id, "profile round-trip lost machine id");
    t.Check(roundtrip.credential_source == MCP_TUNNEL_CREDENTIAL_SESSION,
            "profile round-trip lost credential source");
    t.Check(roundtrip.services.GetCount() == 2, "profile round-trip lost services");

    ValueMap legacy;
    legacy.Add("id", "legacy");
    legacy.Add("name", "Legacy");
    legacy.Add("tunnel_id", "tunnel_legacy");
    legacy.Add("runtime_path", "C:/tools/tunnel-client.exe");
    legacy.Add("mcp_path", "C:/apps/TaskTrackMcp.exe");
    legacy.Add("auto_connect", true);
    legacy.Add("remember_profile", true);
    McpTunnelProfile migrated = McpTunnelProfileFromValue(legacy, 1);
    t.Check(migrated.services.GetCount() == 1, "schema-1 migration did not create TaskTrack service");
    t.Check(migrated.services.GetCount() == 1 && migrated.services[0].channel == "main",
            "schema-1 migration did not preserve main channel");
    t.Check(migrated.credential_source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT,
            "schema-1 migration did not preserve environment credential contract");

    McpTunnelProfile duplicate_channel = MakeProfile();
    duplicate_channel.services[1].channel = "main";
    error.Clear();
    t.Check(!McpTunnelValidateProfile(duplicate_channel, error),
            "duplicate main channel was accepted");

    McpTunnelProfile no_main = MakeProfile();
    no_main.services[0].channel = "tasktrack";
    error.Clear();
    t.Check(!McpTunnelValidateProfile(no_main, error),
            "profile without main channel was accepted");

    McpTunnelProfile duplicate_id = MakeProfile();
    duplicate_id.services[1].id = "tasktrack";
    error.Clear();
    t.Check(!McpTunnelValidateProfile(duplicate_id, error),
            "duplicate service id was accepted");

    McpTunnelProfile disabled = MakeProfile();
    disabled.services[1].enabled = false;
    error.Clear();
    t.Check(McpTunnelValidateProfile(disabled, error),
            "disabled secondary service made valid main profile fail");

    McpTunnelProfile reserved = MakeProfile();
    reserved.services[1].channel = "harpoon";
    error.Clear();
    t.Check(!McpTunnelValidateProfile(reserved, error),
            "reserved harpoon channel was accepted");

    McpTunnelProfile comma_command = MakeProfile();
    comma_command.services[1].command = "C:/apps/tool,withcomma.exe";
    error.Clear();
    t.Check(!McpTunnelValidateProfile(comma_command, error),
            "comma-delimited service command was accepted");

    McpTunnelProfile too_many = MakeProfile();
    too_many.services.Clear();
    for(int i = 0; i < 33; ++i) {
        McpTunnelService service;
        service.id = Format("service-%d", i);
        service.name = service.id;
        service.channel = i == 0 ? String("main") : Format("channel-%d", i);
        service.command = Format("C:/apps/service-%d.exe", i);
        too_many.services.Add(pick(service));
    }
    error.Clear();
    t.Check(!McpTunnelValidateProfile(too_many, error),
            "more than 32 enabled channels were accepted");

    Cout() << "mcp-tunnel-runtime-tests: " << t.passed << " passed, "
           << t.failed.GetCount() << " failed\n";
    for(const String& failure : t.failed)
        Cout() << " - " << failure << "\n";

    SetExitCode(t.failed.IsEmpty() ? 0 : 1);
}
