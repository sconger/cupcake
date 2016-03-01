
#include "unit/UnitTest.h"
#include "unit/net/Socket_test.h"

#include "cupcake/net/AddrInfo.h"
#include "cupcake/net/Socket.h"
#include "cupcake/util/Cleaner.h"

#include <thread>
#include <vector>

bool test_socket_basic() {
    SocketError err;
    SocketError writeErr;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);

    Socket socket;
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

        writeErr = connectSocket.connect(connectAddr);
        if (writeErr != SocketError::Ok) {
            connectSocket.close();
            return;
        }

        Result<uint32_t, SocketError> res = connectSocket.write("Howdy", 5);
        if (!res.ok()) {
            writeErr = res.error();
        }

        connectSocket.close();
    });

    Result<Socket, SocketError> acceptRes = socket.accept();
    if (!acceptRes.ok()) {
        testf("Failed to accept socket with: %d", acceptRes.error());
        socket.close();
        return false;
    }

    Socket acceptedSocket = std::move(acceptRes.get());
    char readBuffer[100] = {1, 2, 3, 4, 5};
    Result<uint32_t, SocketError> readRes = acceptedSocket.read(readBuffer, 5);

    writeThread.join();

    if (!readRes.ok()) {
        testf("Failed to write to socket with: %d", readRes.error());
        acceptedSocket.close();
        socket.close();
        return false;
    }

    if (::memcmp(readBuffer, "Howdy", 5) != 0) {
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

    for (auto i = 0; i < 10; i++) {
        connectThreads.emplace_back(std::thread([connectAddr, &connectErrors] {
            Socket connectSocket;
            connectErrors.push_back(connectSocket.connect(connectAddr));
            connectSocket.close();
        }));
    }

    for (auto i = 0; i < 10; i++) {
        Result<Socket, SocketError> acceptRes = socket.accept();
        if (!acceptRes.ok()) {
            testf("Failed to accept socket with: %d", acceptRes.error());
            socket.close();
            return false;
        }

        Socket acceptedSocket = std::move(acceptRes.get());
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