#ifndef _McpTunnelRuntime_McpTunnelRuntime_h_
#define _McpTunnelRuntime_McpTunnelRuntime_h_

#include <Core/Core.h>

namespace Upp {

enum McpTunnelCredentialSource {
    MCP_TUNNEL_CREDENTIAL_SESSION = 0,
    MCP_TUNNEL_CREDENTIAL_ENVIRONMENT = 1,
};

struct McpTunnelService : Moveable<McpTunnelService> {
    String id;
    String name;
    String channel = "main";
    String command;
    bool enabled = true;
};

struct McpTunnelProfile : Moveable<McpTunnelProfile> {
    String id;
    String name;
    String machine_id;
    String tunnel_id;
    String runtime_path;
    McpTunnelCredentialSource credential_source = MCP_TUNNEL_CREDENTIAL_SESSION;
    bool auto_connect = false;
    bool remember_profile = true;
    Vector<McpTunnelService> services;
};

String McpTunnelCredentialSourceId(McpTunnelCredentialSource source);
McpTunnelCredentialSource McpTunnelCredentialSourceFromId(const String& id);
String McpTunnelDefaultMachineId();
String McpTunnelCommandForExecutable(const String& path);

ValueMap McpTunnelServiceToValue(const McpTunnelService& service);
McpTunnelService McpTunnelServiceFromValue(const Value& value);
ValueMap McpTunnelProfileToValue(const McpTunnelProfile& profile);
McpTunnelProfile McpTunnelProfileFromValue(const Value& value, int schema_version = 2);

McpTunnelProfile McpTunnelDuplicateProfile(const McpTunnelProfile& source,
                                           const String& new_id,
                                           const String& new_name);
bool McpTunnelValidateProfile(const McpTunnelProfile& profile, String& error);
Vector<String> McpTunnelBuildRunArgs(const McpTunnelProfile& profile,
                                     const String& control_plane_api_key_ref,
                                     const String& health_url_file,
                                     const String& log_file);
String McpTunnelBuildChildEnvironment(const McpTunnelProfile& profile);

class McpTunnelRuntime : NoCopy {
public:
    enum State {
        STOPPED,
        CONNECTING,
        READY,
        ERROR,
    };

    McpTunnelRuntime();
    ~McpTunnelRuntime();

    bool Start(const McpTunnelProfile& profile, const String& control_plane_api_key);
    void Refresh();
    void Stop();

    State GetState() const;
    bool IsStarted() const { return started_; }
    bool IsHealthy() const { return healthy_; }
    bool IsReady() const { return ready_; }

    const String& GetHealthUrl() const { return health_url_; }
    const String& GetLastError() const { return last_error_; }
    String GetRuntimeOutput() const { return runtime_output_; }
    String GetDiagnostics() const;

private:
    LocalProcess process_;
    bool started_ = false;
    bool healthy_ = false;
    bool ready_ = false;
    String health_url_;
    String health_url_file_;
    String runtime_log_file_;
    String secret_file_;
    String runtime_output_;
    String last_error_;

    bool LoadHealthUrl();
    bool CreateSecretFile(const String& secret);
    void DeleteSecretFile();
    void DrainOutput();
    bool ProbeHealth(const String& suffix, int& status, String& error);
};

}

#endif
