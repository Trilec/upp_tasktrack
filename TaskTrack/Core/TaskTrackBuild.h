#ifndef _TaskTrack_Core_TaskTrackBuild_h_
#define _TaskTrack_Core_TaskTrackBuild_h_

/*
    TaskTrack build identity
    ========================

    Identifies the exact executable build. Release candidates carry an rc
    suffix; accepted releases use the same value as TaskTrackVersion().

    Copyright (c) 2026 Curtis Edwards
    Licensed under the GNU General Public License, version 3. See LICENSE.
*/

#include <Core/Core.h>

namespace Upp {

inline String TaskTrackBuildVersion()
{
    return "0.3.0";
}

} // namespace Upp

#endif
