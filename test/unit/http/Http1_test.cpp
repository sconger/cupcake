
#include "unit/http/Http1_test.h"
#include "unit/UnitTest.h"

#include "cupcake/http/HttpServer.h"
#include "cupcake/net/Addrinfo.h"
#include "cupcake/net/Socket.h"
#include "cupcake/async/Async.h"
#include "cupcake_priv/http/StreamSourceSocket.h"

#include <condition_variable>

using namespace Cupcake;

static
SocketError initAcceptingSocket(Socket& socket, SockAddr& sockAddr) {
    SocketError err = socket.init(sockAddr.getFamily());

    if (err != SocketError::Ok) {
        return err;
    }

    err = socket.bind(sockAddr);

    if (err != SocketError::Ok) {
        return err;
    }

    err = socket.listen();

    if (err != SocketError::Ok) {
        return err;
    }

    return SocketError::Ok;
}

bool test_http1_empty() {
    Socket acceptSocket;
    HttpServer server;
    HttpError serverError;
    std::mutex serverMutex;
    std::condition_variable serverCond;
    bool isShutdown = false;

    SocketError socketErr = initAcceptingSocket(acceptSocket, Addrinfo::getLoopback(INet::Protocol::Ipv6, 0));
    if (socketErr != SocketError::Ok) {
        testf("Failed to bind socket for accept with: %d", socketErr);
        return false;
    }

    uint16_t boundPort = acceptSocket.getLocalAddress().getPort();

    server.addHandler("/empty", [](HttpRequest& request, HttpResponse& response) {
        response.setStatus(204, "No Content");
    });

    Async::runAsync([&acceptSocket, &server, &serverError, &serverMutex, &serverCond, &isShutdown] {
        StreamSourceSocket streamSource(std::move(acceptSocket));
        HttpError err = server.start(&streamSource);

        std::unique_lock<std::mutex> lock(serverMutex);
        serverError = err;
        serverCond.notify_one();
    });

    uint32_t bytesXfer;
    StringRef request = "GET /empty HTTP/1.0\r\n\r\n";
    StringRef expectedResponse = "HTTP/1.0 204 No Content\r\n\r\n";
    char responseBuffer[1024];

    Socket requestSocket;
    socketErr = requestSocket.init(INet::Protocol::Ipv6);
    if (socketErr != SocketError::Ok) {
        testf("Failed to init socket with: %d", socketErr);
        return false;
    }
    socketErr = requestSocket.connect(Addrinfo::getLoopback(INet::Protocol::Ipv6, boundPort));
    if (socketErr != SocketError::Ok) {
        testf("Failed to connect to HTTP socket with: %d", socketErr);
        return false;
    }
    std::tie(bytesXfer, socketErr) = requestSocket.write(request.data(), request.length());
    if (socketErr != SocketError::Ok) {
        testf("Failed to write to HTTP socket with: %d", socketErr);
        return false;
    }
    std::tie(bytesXfer, socketErr) = requestSocket.read(responseBuffer, sizeof(responseBuffer));
    if (socketErr != SocketError::Ok) {
        testf("Failed to read from HTTP socket with: %d", socketErr);
        return false;
    }
    if (bytesXfer != expectedResponse.length()) {
        testf("Did not receive expected response. Got:\n%.*s", (size_t)bytesXfer, responseBuffer);
        return false;
    }
    if (std::memcmp(expectedResponse.data(), responseBuffer, expectedResponse.length()) != 0) {
        testf("Did not receive expected response. Got:\n%.*s", expectedResponse.length(), responseBuffer);
        return false;
    }

    server.shutdown();

    // Wait for server shutdown and check its error
    std::unique_lock<std::mutex> lock(serverMutex);
    serverCond.wait(lock, [&isShutdown] {return isShutdown;});

    if (serverError != HttpError::Ok) {
        testf("Failed to start HTTP server with: %d", serverError);
        return false;
    }

    return true;
}

bool test_http1_contentlen_request() {
    return true;
}

bool test_http1_contentlen_response() {
    return true;
}

bool test_http1_chunked_request() {
    return true;
}

bool test_http1_chunked_response() {
    return true;
}

bool test_http1_keepalive() {
    return true;
}
