#ifndef _TaskTrack_TunnelApp_TunnelAppIcon_h_
#define _TaskTrack_TunnelApp_TunnelAppIcon_h_

#include <Draw/Draw.h>

namespace Upp {

#define IMAGECLASS TaskTrackTunnelAppImg
#define IMAGEFILE <TaskTrack/TunnelApp/TunnelApp.iml>
#include <Draw/iml_header.h>

inline Image TunnelAppIcon()
{
    return TaskTrackTunnelAppImg::IconTunnelApp();
}

}

#endif
