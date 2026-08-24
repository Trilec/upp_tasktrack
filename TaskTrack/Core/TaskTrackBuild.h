#ifndef _TaskTrack_Core_TaskTrackBuild_h_
#define _TaskTrack_Core_TaskTrackBuild_h_

#include <Core/Core.h>

namespace Upp {

// Changes for every supervisor validation candidate. Keep this separate from
// TaskTrackVersion(): the release version is promoted only after acceptance.
inline String TaskTrackBuildVersion()
{
    return "0.2.1-rc3";
}

} // namespace Upp

#endif
