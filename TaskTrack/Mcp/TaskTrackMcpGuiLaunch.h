#ifndef _TaskTrack_Mcp_TaskTrackMcpGuiLaunch_h_
#define _TaskTrack_Mcp_TaskTrackMcpGuiLaunch_h_

#include <Core/Core.h>

namespace Upp {

#ifdef PLATFORM_WIN32

inline String TaskTrackMcpQuoteWindowsArgument(const String& argument)
{
    String out;
    out << '"';
    const char *p = argument;
    for(;;) {
        int slashes = 0;
        while(*p == '\\') {
            ++p;
            ++slashes;
        }
        if(*p == '\0') {
            out.Cat('\\', slashes * 2);
            break;
        }
        if(*p == '"') {
            out.Cat('\\', slashes * 2 + 1);
            out << '"';
            ++p;
            continue;
        }
        out.Cat('\\', slashes);
        out.Cat(*p++);
    }
    out << '"';
    return out;
}

inline bool TaskTrackMcpLaunchVisibleGui(const String& executable,
                                         const Vector<String>& args,
                                         String& error)
{
    String command = TaskTrackMcpQuoteWindowsArgument(executable);
    for(const String& arg : args)
        command << ' ' << TaskTrackMcpQuoteWindowsArgument(arg);

    Vector<WCHAR> cmd = ToSystemCharsetW(command);
    cmd.Add(0);
    Vector<WCHAR> cwd = ToSystemCharsetW(GetFileFolder(executable));
    cwd.Add(0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    BOOL ok = CreateProcessW(NULL, cmd.begin(), NULL, NULL, FALSE,
                             NORMAL_PRIORITY_CLASS, NULL, cwd.begin(),
                             &si, &pi);
    if(!ok) {
        error = Format("Unable to start visible TaskTrack GUI (Windows error %d).",
                       (int)GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

#else

inline bool TaskTrackMcpLaunchVisibleGui(const String& executable,
                                         const Vector<String>& args,
                                         String& error)
{
    LocalProcess process;
    process.DoubleFork();
    if(!process.Start(~executable, args)) {
        error = "Unable to start TaskTrack GUI.";
        return false;
    }
    process.Detach();
    return true;
}

#endif

} // namespace Upp

#endif
