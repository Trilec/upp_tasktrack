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
    profile.credential_source = MCP_TUNNEL_CREDENTIAL_WINDOWS;
    profile.credential_ref = McpTunnelDefaultCredentialRef(profile.id);

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
    TestState t;

    McpTunnelProfile profile = MakeProfile();
    String error;
    t.Check(McpTunnelValidateProfile(profile, error), "valid two-service machine profile rejected: " + error);

    Vector<String> args = McpTunnelBuildRunArgs(profile, "health.url", "runtime.log");
    t.Check(HasArgPair(args, "--control-plane.tunnel-id", "tunnel_test"),
            "runtime args missing tunnel id");
    t.Check(HasArgPair(args, "--mcp.command",
                       "channel=main,command=C:/apps/TaskTrackMcp.exe"),
            "runtime args missing main TaskTrack binding");
    t.Check(HasArgPair(args, "--mcp.command",
                       "channel=patchtrack,command=C:/apps/patchtrack_mcp.exe"),
            "runtime args missing PatchTrack channel binding");
    t.Check(HasArgPair(args, "--health.url-file", "health.url"),
            "runtime args missing health URL file");

    String old_key = GetEnv("CONTROL_PLANE_API_KEY");
    String child_env = McpTunnelBuildChildEnvironment(profile, "test-secret");
    t.Check(child_env.Find("CONTROL_PLANE_API_KEY=test-secret") >= 0,
            "child environment missing runtime key");
    t.Check(child_env.Find("MCP_TUNNEL_REMOTE=1") >= 0,
            "child environment missing generic remote marker");
    t.Check(child_env.Find("MCP_TUNNEL_MACHINE_ID=curt-main") >= 0,
            "child environment missing machine identity");
    t.Check(GetEnv("CONTROL_PLANE_API_KEY") == old_key,
            "building child environment changed parent process environment");

    McpTunnelProfile duplicate = McpTunnelDuplicateProfile(profile, "curt-copy", "Curt copy");
    t.Check(duplicate.tunnel_id.IsEmpty(), "duplicated profile copied tunnel id");
    t.Check(duplicate.credential_ref != profile.credential_ref,
            "duplicated profile reused credential reference");
    t.Check(!duplicate.auto_connect, "duplicated profile copied auto-connect");
    t.Check(duplicate.services.GetCount() == 2, "duplicated profile lost service bindings");
    t.Check(duplicate.services.GetCount() == 2 && duplicate.services[1].channel == "patchtrack",
            "duplicated profile changed service channel");

    ValueMap value = McpTunnelProfileToValue(profile);
    McpTunnelProfile roundtrip = McpTunnelProfileFromValue(value, 2);
    t.Check(roundtrip.machine_id == profile.machine_id, "profile round-trip lost machine id");
    t.Check(roundtrip.credential_source == MCP_TUNNEL_CREDENTIAL_WINDOWS,
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

    Cout() << "mcp-tunnel-runtime-tests: " << t.passed << " passed, "
           << t.failed.GetCount() << " failed\n";
    for(const String& failure : t.failed)
        Cout() << " - " << failure << "\n";

    SetExitCode(t.failed.IsEmpty() ? 0 : 1);
}
