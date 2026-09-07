#ifndef _McpTunnelRuntime_McpTunnelKeyPipe_h_
#define _McpTunnelRuntime_McpTunnelKeyPipe_h_

#include <Core/Core.h>

namespace Upp {

#ifdef PLATFORM_WIN32
// A one-use, local-only pipe for the vendor's existing file: resolver.
// No plaintext file is created. Close all handles on every launch outcome.
class McpTunnelKeyPipe : NoCopy {
    HANDLE pipe_ = INVALID_HANDLE_VALUE;
    String reference_;
public:
    ~McpTunnelKeyPipe() { Close(); }
    bool Open(String& error);
    bool Send(const String& secret, DWORD expected_pid, int timeout_ms, String& error);
    void Close();
    const String& GetReference() const { return reference_; }
};

String McpTunnelWindowsUserSid();
#endif

}
#endif
