
#include "unit/http/Http1_test.h"
#include "unit/UnitTest.h"

#include "cupcake/http/HttpServer.h"
#include "cupcake/net/Addrinfo.h"
#include "cupcake/net/Socket.h"
#include "cupcake/async/Async.h"
#include "cupcake_priv/http/StreamSourceSocket.h"

#include <condition_variable>
#include <vector>

using namespace Cupcake;

// TODO: Put in common file
static
std::tuple<Socket, SocketError> getAcceptingSocket(const SockAddr& sockAddr) {
    Socket socket;
    SocketError err = socket.init(sockAddr.getFamily());
    if (err != SocketError::Ok) {
        return std::make_tuple(Socket(), err);
    }

    err = socket.bind(sockAddr);
    if (err != SocketError::Ok) {
        return std::make_tuple(Socket(), err);
    }

    err = socket.listen();
    if (err != SocketError::Ok) {
        return std::make_tuple(Socket(), err);
    }

    return std::make_tuple(std::move(socket), SocketError::Ok);
}

static
std::tuple<Socket, SocketError> getConnectedSocket(const SockAddr sockAddr) {
    Socket requestSocket;
    SocketError socketErr = requestSocket.init(sockAddr.getFamily());
    if (socketErr != SocketError::Ok) {
        return std::make_tuple(Socket(), socketErr);
    }
    socketErr = requestSocket.connect(sockAddr);
    if (socketErr != SocketError::Ok) {
        return std::make_tuple(Socket(), socketErr);
    }
    return std::make_tuple(std::move(requestSocket), SocketError::Ok);
}

static
std::tuple<uint32_t, SocketError> readFully(Socket* socket, char* buffer, size_t bufferLen) {
    SocketError socketErr;
    uint32_t totalBytesRead = 0;
    uint32_t bytesXfer = 0;
    do {
        std::tie(bytesXfer, socketErr) = socket->read(buffer + totalBytesRead, bufferLen - totalBytesRead);
        if (socketErr != SocketError::Ok) {
            //testf("Failed to read from HTTP socket with: %d", socketErr);
            return std::make_tuple(0, socketErr);
        }
        totalBytesRead += bytesXfer;
        if (totalBytesRead == bufferLen) {
            // Give up as something went wrong. Test should fail.
            return std::make_tuple(totalBytesRead, SocketError::Ok);
        }
    } while (bytesXfer != 0);
    return std::make_tuple(totalBytesRead, SocketError::Ok);
}

bool test_http1_1_chunked_request() {
    Socket acceptSocket;
    SocketError socketErr;
    HttpServer server;
    HttpError serverError;
    std::mutex serverMutex;
    std::condition_variable serverCond;
    bool isShutdown = false;

    bool gotExpectedPost = false;
    HttpError postError = HttpError::Ok;
    bool hadCorrectHeader = false;

    std::tie(acceptSocket, socketErr) = ::getAcceptingSocket(Addrinfo::getLoopback(INet::Protocol::Ipv6, 0));
    if (socketErr != SocketError::Ok) {
        testf("Failed to bind socket for accept with: %d", socketErr);
        return false;
    }

    uint16_t boundPort = acceptSocket.getLocalAddress().getPort();
    StreamSourceSocket streamSource(std::move(acceptSocket));

    server.addHandler("/formthingy", [&gotExpectedPost, &postError, &hadCorrectHeader](HttpRequest& request, HttpResponse& response) {
        char readBuffer[1024];
        HttpInputStream& httpInputStream = request.getInputStream();

        HttpError readErr;
        uint32_t readBytes;
        uint32_t totalRead = 0;
        do {
            std::tie(readBytes, readErr) = httpInputStream.read(readBuffer + totalRead, 3); // Intentionally small reads
            totalRead += readBytes;
        } while (readErr == HttpError::Ok);

        if (readErr != HttpError::Eof) {
            postError = readErr;
            return;
        }

        postError = httpInputStream.close();

        const StringRef postData = "Some form post data";
        if (totalRead != postData.length()) {
            gotExpectedPost = false;
        } else {
            gotExpectedPost = std::memcmp(postData.data(), readBuffer, postData.length()) == 0;
        }

        StringRef transferEncodingHeader;
        bool hasHeader;
        std::tie(transferEncodingHeader, hasHeader) = request.getHeader("Transfer-Encoding");
        if (!hasHeader) {
            hadCorrectHeader = false;
        } else {
            hadCorrectHeader = transferEncodingHeader.equals("chunked");
        }

        response.setStatus(200, "OK");
    });

    Async::runAsync([&streamSource, &server, &serverError, &serverMutex, &serverCond, &isShutdown] {
        HttpError err = server.start(&streamSource);

        std::unique_lock<std::mutex> lock(serverMutex);
        isShutdown = true;
        serverError = err;
        serverCond.notify_one();
    });

    StringRef request =
        "POST /formthingy HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "Some \r\n"
        "e\r\n"
        "form post data\r\n"
        "0\r\n"
        "\r\n";
    StringRef expectedResponse = "HTTP/1.1 200 OK\r\n\r\n";
    char responseBuffer[1024];

    Socket requestSocket;
    std::tie(requestSocket, socketErr) = getConnectedSocket(Addrinfo::getLoopback(INet::Protocol::Ipv6, boundPort));
    if (socketErr != SocketError::Ok) {
        testf("Failed to connect to HTTP socket with: %d", socketErr);
        return false;
    }
    socketErr = requestSocket.write(request.data(), request.length());
    if (socketErr != SocketError::Ok) {
        testf("Failed to write to HTTP socket with: %d", socketErr);
        return false;
    }
    uint32_t totalBytesRead = 0;
    std::tie(totalBytesRead, socketErr) = readFully(&requestSocket, responseBuffer, sizeof(responseBuffer));
    if (socketErr != SocketError::Ok) {
        testf("Failed to read from HTTP socket with: %d", socketErr);
        return false;
    }
    if (totalBytesRead != expectedResponse.length()) {
        testf("Did not receive expected response. Got:\n%.*s", (size_t)totalBytesRead, responseBuffer);
        return false;
    }
    if (std::memcmp(expectedResponse.data(), responseBuffer, expectedResponse.length()) != 0) {
        testf("Did not receive expected response. Got:\n%.*s", expectedResponse.length(), responseBuffer);
        return false;
    }

    server.shutdown();

    // Wait for server shutdown and check its error
    std::unique_lock<std::mutex> lock(serverMutex);
    serverCond.wait(lock, [&isShutdown] {return isShutdown; });

    if (serverError != HttpError::Ok) {
        testf("Failed to start HTTP server with: %d", serverError);
        return false;
    }

    if (!gotExpectedPost) {
        testf("Did not read back expected post data");
        return false;
    }
    if (postError != HttpError::Ok) {
        testf("Received error when reading post data: %d", postError);
        return false;
    }
    if (!hadCorrectHeader) {
        testf("Http request did not have expected Transfer-Encoding header");
        return false;
    }

    return true;
}

bool test_http1_1_keepalive() {
    Socket acceptSocket;
    SocketError socketErr;
    HttpServer server;
    HttpError serverError;
    std::mutex serverMutex;
    std::condition_variable serverCond;
    bool isShutdown = false;

    std::tie(acceptSocket, socketErr) = getAcceptingSocket(Addrinfo::getLoopback(INet::Protocol::Ipv6, 0));
    if (socketErr != SocketError::Ok) {
        testf("Failed to bind socket for accept with: %d", socketErr);
        return false;
    }

    uint16_t boundPort = acceptSocket.getLocalAddress().getPort();
    StreamSourceSocket streamSource(std::move(acceptSocket));

    server.addHandler("/index.html", [](HttpRequest& request, HttpResponse& response) {
        response.setStatus(200, "OK");
        response.addHeader("Content-Length", "11");

        HttpOutputStream* outputStream;
        std::tie(outputStream, std::ignore) = response.getOutputStream();
        outputStream->write("Hello World", 11);
        outputStream->close();
    });

    Async::runAsync([&streamSource, &server, &serverError, &serverMutex, &serverCond, &isShutdown] {
        HttpError err = server.start(&streamSource);

        std::unique_lock<std::mutex> lock(serverMutex);
        isShutdown = true;
        serverError = err;
        serverCond.notify_one();
    });

    std::vector<StringRef> requests = {
        "GET /index.html HTTP/1.1\r\n"
        "Connection: keep-alive\r\n"
        "Host: localhost\r\n"
        "\r\n"
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n",

        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n"
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n",
    };
    std::vector<StringRef> expectedResponses = {
        "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World"
        "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World"
        "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World",

        "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World",
    };

    for (size_t i = 0; i < requests.size(); i++) {
        StringRef request = requests[i];
        StringRef expectedResponse = expectedResponses[i];

        char responseBuffer[1024];

        Socket requestSocket;
        std::tie(requestSocket, socketErr) = getConnectedSocket(Addrinfo::getLoopback(INet::Protocol::Ipv6, boundPort));
        if (socketErr != SocketError::Ok) {
            testf("Failed to connect to HTTP socket with: %d", socketErr);
            return false;
        }
        socketErr = requestSocket.write(request.data(), request.length());
        if (socketErr != SocketError::Ok) {
            testf("Failed to write to HTTP socket with: %d", socketErr);
            return false;
        }
        uint32_t totalBytesRead = 0;
        std::tie(totalBytesRead, socketErr) = readFully(&requestSocket, responseBuffer, sizeof(responseBuffer));
        if (socketErr != SocketError::Ok) {
            testf("Failed to read from HTTP socket with: %d", socketErr);
            return false;
        }
        if (totalBytesRead != expectedResponse.length()) {
            testf("Did not receive expected response. Got:\n%.*s", (size_t)totalBytesRead, responseBuffer);
            return false;
        }
        if (std::memcmp(expectedResponse.data(), responseBuffer, expectedResponse.length()) != 0) {
            testf("Did not receive expected response. Got:\n%.*s", expectedResponse.length(), responseBuffer);
            return false;
        }
    }

    server.shutdown();

    // Wait for server shutdown and check its error
    std::unique_lock<std::mutex> lock(serverMutex);
    serverCond.wait(lock, [&isShutdown] {return isShutdown; });

    if (serverError != HttpError::Ok) {
        testf("Failed to start HTTP server with: %d", serverError);
        return false;
    }

    return true;
}
