
#ifndef CUPCAKE_SOCKET_IMPL_H
#define CUPCAKE_SOCKET_IMPL_H

#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/net/Socket.h"
#include "cupcake/net/SocketError.h"

#include <future>

#include <Winsock2.h>
#include <Windows.h>

// There doesn't seem to be a definition of std::experimental::task at the moment, so
// this is a hacky work around. Copied from:
// https://github.com/GorNishanov/await/blob/master/2015_CppCon/SuperLean/Awaitable.cpp
/*
namespace std {
    namespace experimental {
        struct task {
            ~task() {}
            struct promise_type {
                task get_return_object() { return task{}; }
                void return_void() {}
                bool initial_suspend() { return false; }
                bool final_suspend() { return false; }
            };
        };
    }
}
*/

/*
 * OS specific socket implementation.
 */
class SocketImpl {
public:
    SocketImpl();
    ~SocketImpl();

    void close();

    SockAddr getLocalAddress() const;
    SockAddr getRemoteAddress() const;

    SocketError bind(const SockAddr& sockAddr);
    SocketError listen();
    SocketError listen(uint32_t queue);

    Result<Socket, SocketError> accept();
    SocketError connect(const SockAddr& sockAddr);
    Result<uint32_t, SocketError> read(char* buffer, uint32_t bufferLen);
    Result<uint32_t, SocketError> write(const char* buffer, uint32_t bufferLen);

private:
    class AcceptAwaiter;
    class ConnectAwaiter;
    class ReadAwaiter;
    class WriteAwaiter;

    SocketImpl(const SocketImpl&) = delete;
    SocketImpl(SocketImpl&&) = delete;
    SocketImpl& operator=(const SocketImpl&) = delete;
    SocketImpl& operator=(SocketImpl&&) = delete;

    SocketError initSocket(int family);

    static SocketError initSocket(SOCKET* pSocket, int family);

    static void CALLBACK completionCallback(PTP_CALLBACK_INSTANCE instance,
        PVOID context,
        PVOID overlapped,
        ULONG ioResult,
        ULONG_PTR numberOfBytesTransferred,
        PTP_IO ptpIo);

    class CompletionResult {
    public:
        CompletionResult() : bytesTransfered(0), error(0) {}
        ULONG_PTR bytesTransfered;
        DWORD error;
    };

    // The objects holding the OVERLAPPED are pointers at the moment to make
    // the object movable.
    class OverlappedData {
    public:
        OverlappedData();

        OVERLAPPED overlapped;
        void* coroutineHandle;
        CompletionResult* completionResult;
    };

    std::future<void> accept_co(Result<Socket, SocketError>* res);
    std::future<void> connect_co(const SockAddr& sockAddr, SocketError* res);
    std::future<void> read_co(char* buffer, uint32_t bufferLen, Result<uint32_t, SocketError>* res);
    std::future<void> write_co(const char* buffer, uint32_t bufferLen, Result<uint32_t, SocketError>* res);

    SOCKET socket;
    PTP_IO ptpIo;

    SockAddr localAddr;
    SockAddr remoteAddr;
};

#endif // CUPCAKE_SOCKET_IMPL_H
