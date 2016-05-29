
#include "unit/UnitTest.h"
#include "unit/net/Socket_test.h"

#include "cupcake/net/AddrInfo.h"
#include "cupcake/net/Socket.h"

#include <mutex>
#include <thread>
#include <vector>

using namespace Cupcake;

bool test_socket_basic() {
    SocketError err;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);

    Socket socket;
    Socket connectingSocket;
    Socket acceptedSocket;

    err = socket.init(INet::Protocol::Ipv4);
    if (err != SocketError::Ok) {
        testf("Failed to init socket with: %d", err);
        return false;
    }
    err = connectingSocket.init(INet::Protocol::Ipv4);
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

    err = connectingSocket.connect(connectAddr);
    if (err != SocketError::Ok) {
        testf("Failed to connect with: %d", err);
        return false;
    }

    std::tie(acceptedSocket, err) = socket.accept();
    if (err != SocketError::Ok) {
        testf("Failed to accept socket with: %d", err);
        return false;
    }

    uint32_t bytesWritten;
    std::tie(bytesWritten, err) = connectingSocket.write("Howdy", 5);
    if (err != SocketError::Ok) {
        testf("Failed to write to socket with: %d", err);
        return false;
    }
    if (bytesWritten != 5) {
        testf("Did not write expected number of bytes");
        return false;
    }

    char readBuffer[100] = {1, 2, 3, 4, 5};
    uint32_t bytesRead;
    std::tie(bytesRead, err) = acceptedSocket.read(readBuffer, 5);

    if (err != SocketError::Ok) {
        testf("Failed to write to socket with: %d", err);
        return false;
    }

    if (std::memcmp(readBuffer, "Howdy", 5) != 0) {
        testf("Did not read expected string back from socket");
        return false;
    }

    return true;
}

bool test_socket_vector() {
    SocketError err;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);

    Socket socket;
    Socket connectingSocket;
    Socket acceptedSocket;

    err = socket.init(INet::Protocol::Ipv4);
    if (err != SocketError::Ok) {
        testf("Failed to init socket with: %d", err);
        return false;
    }
    err = connectingSocket.init(INet::Protocol::Ipv4);
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

    err = connectingSocket.connect(connectAddr);
    if (err != SocketError::Ok) {
        testf("Failed to connect with: %d", err);
        return false;
    }

    std::tie(acceptedSocket, err) = socket.accept();
    if (err != SocketError::Ok) {
        testf("Failed to accept socket with: %d", err);
        return false;
    }

    char writeData1[] = { 1, 2, 3, 4, 5 };
    char writeData2[] = { 6, 7, 8 };
    char writeData3[] = { 9, 10 };
    INet::IoBuffer writeBuffers[3];
    writeBuffers[0].buffer = writeData1;
    writeBuffers[0].bufferLen = 5;
    writeBuffers[1].buffer = writeData2;
    writeBuffers[1].bufferLen = 3;
    writeBuffers[2].buffer = writeData3;
    writeBuffers[2].bufferLen = 2;

    uint32_t bytesWritten;
    std::tie(bytesWritten, err) = connectingSocket.writev(writeBuffers, 3);
    if (err != SocketError::Ok) {
        testf("Failed to write to socket with: %d", err);
        return false;
    }
    if (bytesWritten != 10) {
        testf("Did not write expected number of bytes");
        return false;
    }

    char readBuffer1[4];
    char readBuffer2[6];
    INet::IoBuffer readBuffers[2];
    readBuffers[0].buffer = readBuffer1;
    readBuffers[0].bufferLen = 4;
    readBuffers[1].buffer = readBuffer2;
    readBuffers[1].bufferLen = 6;

    uint32_t bytesRead;
    std::tie(bytesRead, err) = acceptedSocket.readv(readBuffers, 2);
    if (err != SocketError::Ok) {
        testf("Failed to read from socket with: %d", err);
        return false;
    }
    if (bytesRead != 10) {
        testf("Did not read expected number of bytes");
        return false;
    }

    char expected1[] = { 1, 2, 3, 4 };
    char expected2[] = { 5, 6, 7, 8, 9, 10 };

    if (std::memcmp(readBuffer1, expected1, 4) != 0 ||
        std::memcmp(readBuffer2, expected2, 6) != 0) {
        testf("Did not read expected string back from socket");
        return false;
    }

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
        }));
    }

    for (auto i = 0; i < 10; i++) {
        Socket acceptedSocket;
        std::tie(acceptedSocket, err) = socket.accept();
        if (err != SocketError::Ok) {
            testf("Failed to accept socket with: %d", err);
            return false;
        }
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
