
#ifndef CUPCAKE_SOCKET_IMPL_WIN_H
#define CUPCAKE_SOCKET_IMPL_WIN_H

#ifdef _WIN32

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

namespace Cupcake {

/*
 * OS specific socket implementation.
 */
class SocketImpl {
public:
    SocketImpl();
    ~SocketImpl();

    SocketError init(INet::Protocol prot);
    SocketError close();

    SockAddr getLocalAddress() const;
    SockAddr getRemoteAddress() const;

    SocketError bind(const SockAddr& sockAddr);
    SocketError listen();
    SocketError listen(uint32_t queue);

    std::tuple<Socket, SocketError> accept();
    SocketError connect(const SockAddr& sockAddr);

    std::tuple<uint32_t, SocketError> read(char* buffer, uint32_t bufferLen);
    std::tuple<uint32_t, SocketError> readv(INet::IoBuffer* buffers, uint32_t bufferCount);
    SocketError write(const char* buffer, uint32_t bufferLen);
    SocketError writev(const INet::IoBuffer* buffers, uint32_t bufferCount);

    SocketError shutdownRead();
    SocketError shutdownWrite();
    SocketError setReadBuf(uint32_t bufferSize);
    SocketError setWriteBuf(uint32_t bufferSize);

private:
    class AcceptAwaiter;
    class ConnectAwaiter;
    class ReadAwaiter;
    class WriteAwaiter;

    SocketImpl(const SocketImpl&) = delete;
    SocketImpl(SocketImpl&&) = delete;
    SocketImpl& operator=(const SocketImpl&) = delete;
    SocketImpl& operator=(SocketImpl&&) = delete;

    std::future<void> accept_co(SOCKET preparedSocket, PTP_IO preparedPtpIo, std::tuple<Socket, SocketError>* res);
    std::future<void> connect_co(const SockAddr& sockAddr, SocketError* res);
    std::future<void> read_co(INet::IoBuffer* buffers, uint32_t bufferCount, std::tuple<uint32_t, SocketError>* res);
    std::future<void> write_co(const INet::IoBuffer* buffers, uint32_t bufferCount, SocketError* res);

    SOCKET socket;
    PTP_IO ptpIo;

    SockAddr localAddr;
    SockAddr remoteAddr;
};

}

#endif // _WIN32

#endif // CUPCAKE_SOCKET_IMPL_WIN_H
