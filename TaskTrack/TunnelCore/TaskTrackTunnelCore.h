#ifndef _TaskTrack_TunnelCore_TaskTrackTunnelCore_h_
#define _TaskTrack_TunnelCore_TaskTrackTunnelCore_h_

#include <Core/Core.h>

namespace Upp {

struct TaskTrackTunnelProbe {
    int schema_version = 1;
    int sequence = 0;
    String message;
    String updated_at;
    String source;
    String tunnel_id;
};

struct TaskTrackTunnelActivity {
    int schema_version = 1;
    int64 received = 0;
    int64 sent = 0;
    String last_method;
    String last_tool;
    String updated_at;
};

String TaskTrackTunnelProbePath();
String TaskTrackTunnelActivityPath();

bool TaskTrackTunnelLoadProbe(TaskTrackTunnelProbe& probe, String& error);
bool TaskTrackTunnelSaveProbe(const TaskTrackTunnelProbe& probe, String& error);

bool TaskTrackTunnelLoadActivity(TaskTrackTunnelActivity& activity, String& error);
bool TaskTrackTunnelSaveActivity(const TaskTrackTunnelActivity& activity, String& error);
bool TaskTrackTunnelResetActivity(String& error);
bool TaskTrackTunnelRecordReceived(const String& method, const String& tool, String& error);
bool TaskTrackTunnelRecordSent(String& error);
bool TaskTrackTunnelIsRemoteSession();

ValueMap TaskTrackTunnelProbeStatusValue();

}

#endif
