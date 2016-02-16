
#ifndef CUPCAKE_SOCKET_H
#define CUPCAKE_SOCKET_H

#include "cupcake/Cupcake.h"
#include "cupcake/net/SockAddr.h"
#include "cupcake/net/SocketError.h"
#include "cupcake/text/StringRef.h"

#include <functional>

#if defined(__linux__)
// TODO
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#elif defined(WIN32)
#include <Winsock2.h>
#include <Windows.h>
#endif

/*
 * Asynchronous socket
 *
 * The underlying implementation varies by platform:
 * Linux: Epoll based
 * OSX: Grand central dispatch
 * Windows: Default Thread pool/Completion queue
 *
 * Not a movable type as it may share state with worker threads.
 */
class Socket {
public:
    typedef std::function<void(void* context, Socket* acceptedSock, SocketError err)> AcceptCallback;
    typedef std::function<void(void* context, SocketError err)> ConnectCallback;
    typedef std::function<void(void* context, uint64_t bytesXfer, SocketError err)> XferCallback;

    Socket() noexcept;
    ~Socket();

    void close();

    SockAddr getLocalAddress() const;
    SockAddr getRemoteAddress() const;

    SocketError bind(const SockAddr& sockAddr);
    SocketError listen();
    SocketError listen(uint32_t queue);

    SocketError startAccepting(void* context, const AcceptCallback& callback);
    SocketError accept(void* context, const AcceptCallback& callback);
    SocketError connect(void* context, const SockAddr& sockAddr, const ConnectCallback& callback);
    SocketError read(void* context, char* buffer, uint32_t bufferLen, const XferCallback& callback);
    SocketError write(void* context, const char* buffer, uint32_t bufferLen, const XferCallback& callback);

private:
    Socket(const Socket&) = delete;
	Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
	Socket& operator=(Socket&&) = delete;

    SocketError initSocket(int family);

#if defined(_WIN32)
    static SocketError initSocket(SOCKET* pSocket, int family);

    static void CALLBACK completionCallback(PTP_CALLBACK_INSTANCE instance,
        PVOID context,
        PVOID overlapped,
        ULONG ioResult,
        ULONG_PTR numberOfBytesTransferred,
        PTP_IO ptpIo);

    enum class IoType {
        Accept,
        Connect,
        Read,
        Write
    };

    struct acceptExValues {
		SOCKET acceptingSocket;
        SOCKET acceptedSocket;
        char addrBuffer[2 * (sizeof(SOCKADDR_STORAGE) + 16)];
    };

    struct xferValues {
        char* buffer;
        uint32_t bufferLen;
    };

	// The objects holding the OVERLAPPED are pointers at the moment to make
	// the object movable.
    class overlappedData {
	public:
		overlappedData(Socket* Socket);
		void reset();

        OVERLAPPED overlapped;
		Socket* socket;
        IoType ioType;
        void* context;

		// Outside union to avoid construct/destruct issues
		AcceptCallback acceptCallback;
		ConnectCallback connectCallback;
		XferCallback xferCallback;

        union {
            acceptExValues acceptData;
            xferValues xferData;
        };
    };

    static void onAccept(overlappedData* data);
    static void onConnect(overlappedData* data);
    static void onRead(overlappedData* data, uint64_t bytesXfer);
    static void onWrite(overlappedData* data, uint64_t bytesXfer);

    SOCKET socket;
    PTP_IO ptpIo;
    overlappedData readOverlapped;
    overlappedData writeOverlapped;
#elif defined(__APPLE__)
    int fd;

    AcceptCallback acceptCallback;
    ConnectCallback connectCallback;
    XferCallback readCallback;
    XferCallback writeCallback;
    void* readContext;
    void* writeContext;
    dispatch_io_t dispatchIo;
    dispatch_source_t acceptSource;
    bool autoAccept;
    dispatch_source_t connectSource;
    sockaddr_storage connectAddr;

    Error acceptCommon(void* context, const AcceptCallback& callback);
    bool tryAccept();
    bool tryConnect();
    static void acceptHandler(void* context);
    static void connectHandler(void* context);
#else

#endif

    SockAddr localAddr;
    SockAddr remoteAddr;
};

#endif // CUPCAKE_SOCKET_H
