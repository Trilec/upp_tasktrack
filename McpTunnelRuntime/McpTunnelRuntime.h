#ifndef _McpTunnelRuntime_McpTunnelRuntime_h_
#define _McpTunnelRuntime_McpTunnelRuntime_h_

#include <Core/Core.h>

namespace Upp {

enum McpTunnelCredentialSource {
    MCP_TUNNEL_CREDENTIAL_WINDOWS = 0,
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
    McpTunnelCredentialSource credential_source = MCP_TUNNEL_CREDENTIAL_WINDOWS;
    String credential_ref;
    bool auto_connect = false;
    bool remember_profile = true;
    Vector<McpTunnelService> services;
};

String McpTunnelCredentialSourceId(McpTunnelCredentialSource source);
McpTunnelCredentialSource McpTunnelCredentialSourceFromId(const String& id);
String McpTunnelDefaultCredentialRef(const String& profile_id);
String McpTunnelDefaultMachineId();

ValueMap McpTunnelServiceToValue(const McpTunnelService& service);
McpTunnelService McpTunnelServiceFromValue(const Value& value);
ValueMap McpTunnelProfileToValue(const McpTunnelProfile& profile);
McpTunnelProfile McpTunnelProfileFromValue(const Value& value, int schema_version = 2);

McpTunnelProfile McpTunnelDuplicateProfile(const McpTunnelProfile& source,
                                           const String& new_id,
                                           const String& new_name);
bool McpTunnelValidateProfile(const McpTunnelProfile& profile, String& error);
Vector<String> McpTunnelBuildRunArgs(const McpTunnelProfile& profile,
                                     const String& health_url_file,
                                     const String& log_file);
String McpTunnelBuildChildEnvironment(const McpTunnelProfile& profile,
                                      const String& control_plane_api_key);

bool McpTunnelCredentialExists(const McpTunnelProfile& profile, String& error);
bool McpTunnelReadCredential(const McpTunnelProfile& profile, String& secret, String& error);
bool McpTunnelWriteCredential(const McpTunnelProfile& profile, const String& secret, String& error);
bool McpTunnelDeleteCredential(const McpTunnelProfile& profile, String& error);

}

#endif
