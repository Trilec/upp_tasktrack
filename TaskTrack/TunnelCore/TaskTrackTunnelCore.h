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

String TaskTrackTunnelProbePath();

bool TaskTrackTunnelLoadProbe(TaskTrackTunnelProbe& probe, String& error);
bool TaskTrackTunnelSaveProbe(const TaskTrackTunnelProbe& probe, String& error);

ValueMap TaskTrackTunnelProbeStatusValue();

}

#endif
