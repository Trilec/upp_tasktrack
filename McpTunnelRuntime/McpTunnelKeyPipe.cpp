#include "McpTunnelKeyPipe.h"

#ifdef PLATFORM_WIN32
#include <sddl.h>

namespace Upp {

String McpTunnelWindowsUserSid()
{
    HANDLE token = NULL;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return String();
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, NULL, 0, &size);
    Buffer<byte> info(size);
    bool ok = GetTokenInformation(token, TokenUser, ~info, size, &size);
    CloseHandle(token);
    if(!ok)
        return String();
    LPWSTR sid = NULL;
    if(!ConvertSidToStringSidW(((TOKEN_USER*)~info)->User.Sid, &sid))
        return String();
    String result = WString(sid).ToString();
    LocalFree(sid);
    return result;
}

void McpTunnelKeyPipe::Close()
{
    if(pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }
    reference_.Clear();
}

bool McpTunnelKeyPipe::Open(String& error)
{
    Close();
    String sid = McpTunnelWindowsUserSid();
    if(sid.IsEmpty()) {
        error = "Cannot determine the credential pipe owner.";
        return false;
    }
    // Protected DACL: only this user. Remote clients are also rejected below.
    Vector<WCHAR> sddl = ToSystemCharsetW("D:P(A;;GA;;;" + sid + ")");
    sddl.Add(0);
    PSECURITY_DESCRIPTOR descriptor = NULL;
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.begin(), SDDL_REVISION_1,
                                                             &descriptor, NULL)) {
        error = "Cannot restrict credential pipe access.";
        return false;
    }
    SECURITY_ATTRIBUTES sa = { sizeof(sa), descriptor, FALSE };
    String path = "\\\\.\\pipe\\mcp-tunnel-key-" + AsString(Uuid::Create());
    Vector<WCHAR> wide_path = ToSystemCharsetW(path);
    wide_path.Add(0);
    pipe_ = CreateNamedPipeW(wide_path.begin(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED |
                            FILE_FLAG_FIRST_PIPE_INSTANCE,
                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                            1, 4096, 0, 0, &sa);
    LocalFree(descriptor);
    if(pipe_ == INVALID_HANDLE_VALUE) {
        error = "Cannot create the private credential pipe.";
        return false;
    }
    reference_ = "file:" + path;
    return true;
}

bool McpTunnelKeyPipe::Send(const String& secret, DWORD expected_pid, int timeout_ms, String& error)
{
    if(pipe_ == INVALID_HANDLE_VALUE || secret.IsEmpty() || secret.GetCount() > 4096) {
        error = "Runtime key must contain between 1 and 4096 bytes.";
        Close();
        return false;
    }
    OVERLAPPED operation = {};
    operation.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if(!operation.hEvent) {
        error = "Cannot create credential pipe completion event.";
        Close();
        return false;
    }
    DWORD count = 0;
    auto complete = [&]() {
        if(WaitForSingleObject(operation.hEvent, max(1, timeout_ms)) == WAIT_OBJECT_0)
            return !!GetOverlappedResult(pipe_, &operation, &count, FALSE);
        CancelIo(pipe_); // All operations are issued and cancelled on this thread.
        // Keep OVERLAPPED/event alive until cancellation completes.
        GetOverlappedResult(pipe_, &operation, &count, TRUE);
        return false;
    };
    bool connected = !!ConnectNamedPipe(pipe_, &operation);
    if(!connected) {
        DWORD code = GetLastError();
        connected = code == ERROR_PIPE_CONNECTED || (code == ERROR_IO_PENDING && complete());
    }
    ULONG client_pid = 0;
    typedef BOOL (WINAPI *ClientPidFn)(HANDLE, PULONG);
    ClientPidFn client_pid_fn = (ClientPidFn)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),
                                                         "GetNamedPipeClientProcessId");
    bool ok = connected && client_pid_fn && client_pid_fn(pipe_, &client_pid) && client_pid == expected_pid;
    if(ok) {
        ResetEvent(operation.hEvent);
        bool written = !!WriteFile(pipe_, ~secret, secret.GetCount(), &count, &operation);
        if(!written && GetLastError() == ERROR_IO_PENDING)
            written = complete();
        ok = written && count == (DWORD)secret.GetCount();
    }
    CloseHandle(operation.hEvent);
    // Close, do not DisconnectNamedPipe (which discards unread bytes). The
    // vendor reads until EOF; waiting for a health URL before close deadlocks.
    Close();
    if(!ok)
        error = "Runtime credential pipe timed out, failed, or had an unexpected reader.";
    return ok;
}

}
#endif
