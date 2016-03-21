
#include "unit/UnitTest.h"
#include "unit/net/Socket_test.h"

#include "cupcake/net/AddrInfo.h"
#include "cupcake/net/Socket.h"
#include "cupcake/util/Cleaner.h"

#include <mutex>
#include <thread>
#include <vector>

using namespace Cupcake;

bool test_socket_basic() {
    SocketError err;
    SocketError writeErr;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);

    Socket socket;
    err = socket.init(INet::Protocol::Ipv4);
    if (err != SocketError::Ok) {
        testf("Failed to init socket with: %d", err);
        return false;
    }

    err = socket.bind(bindAddr);
    if (err != SocketError::Ok) {
        testf("Failed to bind to IPv4 ADDR_ANY with: %d", err);
        return false;
    }

    err = socket.listen();
    if (err != SocketError::Ok) {
        testf("Failed to listen on bound socket with: %d", err);
        return false;
    }

    SockAddr connectAddr = Addrinfo::getLoopback(INet::Protocol::Ipv4, socket.getLocalAddress().getPort());

    std::thread writeThread([connectAddr, &writeErr] {
        Socket connectSocket;

        writeErr = connectSocket.init(INet::Protocol::Ipv4);
        if (writeErr != SocketError::Ok) {
            connectSocket.close();
            return;
        }

        writeErr = connectSocket.connect(connectAddr);
        if (writeErr != SocketError::Ok) {
            connectSocket.close();
            return;
        }

        std::tie(std::ignore, writeErr) = connectSocket.write("Howdy", 5);
        connectSocket.close();
    });

    Socket acceptedSocket;
    std::tie(acceptedSocket, err) = socket.accept();
    if (err != SocketError::Ok) {
        testf("Failed to accept socket with: %d", err);
        socket.close();
        return false;
    }

    char readBuffer[100] = {1, 2, 3, 4, 5};
    uint32_t bytesRead;
    std::tie(bytesRead, err) = acceptedSocket.read(readBuffer, 5);

    writeThread.join();

    if (err != SocketError::Ok) {
        testf("Failed to write to socket with: %d", err);
        acceptedSocket.close();
        socket.close();
        return false;
    }

    if (std::memcmp(readBuffer, "Howdy", 5) != 0) {
        testf("Did not read expected string back from socket");
        acceptedSocket.close();
        socket.close();
        return false;
    }

    acceptedSocket.close();
    socket.close();
    return true;
}

bool test_socket_accept_multiple() {
    SocketError err;
    SocketError writeErr;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);

    Socket socket;
    err = socket.init(INet::Protocol::Ipv4);
    if (err != SocketError::Ok) {
        testf("Failed to init socket with: %d", err);
        return false;
    }

    err = socket.bind(bindAddr);
    if (err != SocketError::Ok) {
        testf("Failed to bind to IPv4 ADDR_ANY with: %d", err);
        return false;
    }

    err = socket.listen();
    if (err != SocketError::Ok) {
        testf("Failed to listen on bound socket with: %d", err);
        return false;
    }

    SockAddr connectAddr = Addrinfo::getLoopback(INet::Protocol::Ipv4, socket.getLocalAddress().getPort());

    std::vector<std::thread> connectThreads;
    std::vector<SocketError> connectErrors;
    std::mutex connectDataLock;

    for (auto i = 0; i < 10; i++) {
        connectThreads.emplace_back(std::thread([connectAddr, &connectErrors, &connectDataLock] {
            Socket connectSocket;
            SocketError connectError;

            connectError = connectSocket.init(INet::Protocol::Ipv4);
            if (connectError != SocketError::Ok) {
                connectErrors.push_back(connectError);
                return;
            }

            connectError = connectSocket.connect(connectAddr);

            {
                std::lock_guard<std::mutex> lock(connectDataLock); // Multiple threads push errors
                connectErrors.push_back(connectError);
            }

            connectSocket.close();
        }));
    }

    for (auto i = 0; i < 10; i++) {
        Socket acceptedSocket;
        std::tie(acceptedSocket, err) = socket.accept();
        if (err != SocketError::Ok) {
            testf("Failed to accept socket with: %d", err);
            socket.close();
            return false;
        }

        acceptedSocket.close();
    }

    for (std::thread& thread : connectThreads) {
        thread.join();
    }

    socket.close();

    for (SocketError connectError : connectErrors) {
        if (connectError != SocketError::Ok) {
            testf("One of the connection attempts failed with: %d", connectError);
            return false;
        }
    }
    
    return true;
}

bool test_socket_set_options() {
    Socket socket;
    SocketError err;

    err = socket.init(INet::Protocol::Ipv6);
    if (err != SocketError::Ok) {
        testf("Failed to init socket with: %d", err);
        return false;
    }

    // Difficult to validate these do anything as there is some platform variation on reading the
    // values back, so just testing that a reasonable value doesn't fail.
    err = socket.setReadBuf(1024);
    if (err != SocketError::Ok) {
        testf("Failed to set socket read buffer with: %d", err);
        return false;
    }

    err = socket.setWriteBuf(1024);
    if (err != SocketError::Ok) {
        testf("Failed to set socket read buffer with: %d", err);
        return false;
    }

    return true;
}