
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
    StreamSourceSocket streamSource(std::move(acceptSocket));

    server.addHandler("/empty", [](HttpRequest& request, HttpResponse& response) {
        response.setStatus(204, "No Content");
    });

    Async::runAsync([&streamSource, &server, &serverError, &serverMutex, &serverCond, &isShutdown] {
        HttpError err = server.start(&streamSource);

        std::unique_lock<std::mutex> lock(serverMutex);
        isShutdown = true;
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
    socketErr = requestSocket.write(request.data(), request.length());
    if (socketErr != SocketError::Ok) {
        testf("Failed to write to HTTP socket with: %d", socketErr);
        return false;
    }
    uint32_t totalBytesRead = 0;
    do {
        std::tie(bytesXfer, socketErr) = requestSocket.read(responseBuffer + totalBytesRead, sizeof(responseBuffer) - totalBytesRead);
        if (socketErr != SocketError::Ok) {
            testf("Failed to read from HTTP socket with: %d", socketErr);
            return false;
        }
        totalBytesRead += bytesXfer;
    } while (bytesXfer != 0);
    if (totalBytesRead != expectedResponse.length()) {
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
    Socket acceptSocket;
    HttpServer server;
    HttpError serverError;
    std::mutex serverMutex;
    std::condition_variable serverCond;
    bool isShutdown = false;

    bool gotExpectedPost = false;
    HttpError postError = HttpError::Ok;
    bool hadCorrectHeader = false;

    SocketError socketErr = initAcceptingSocket(acceptSocket, Addrinfo::getLoopback(INet::Protocol::Ipv6, 0));
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
        bool doneReading = false;
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

        StringRef contentLengthHeader;
        bool hasHeader;
        std::tie(contentLengthHeader, hasHeader) = request.getHeader("Content-Length");
        if (!hasHeader) {
            hadCorrectHeader = false;
        } else {
            hadCorrectHeader = contentLengthHeader.equals("19");
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

    uint32_t bytesXfer;
    StringRef request =
        "POST /formthingy HTTP/1.0\r\n"
        "Content-Length: 19\r\n"
        "\r\n"
        "Some form post data";
    StringRef expectedResponse = "HTTP/1.0 200 OK\r\n\r\n";
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
    socketErr = requestSocket.write(request.data(), request.length());
    if (socketErr != SocketError::Ok) {
        testf("Failed to write to HTTP socket with: %d", socketErr);
        return false;
    }
    uint32_t totalBytesRead = 0;
    do {
        std::tie(bytesXfer, socketErr) = requestSocket.read(responseBuffer + totalBytesRead, sizeof(responseBuffer) - totalBytesRead);
        if (socketErr != SocketError::Ok) {
            testf("Failed to read from HTTP socket with: %d", socketErr);
            return false;
        }
        totalBytesRead += bytesXfer;
    } while (bytesXfer != 0);
    if (totalBytesRead != expectedResponse.length()) {
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
        testf("Http request did not have expected Content-Length header");
        return false;
    }

    return true;
}

bool test_http1_contentlen_response() {
    return true;
}

bool test_http1_chunked_request() {
    Socket acceptSocket;
    HttpServer server;
    HttpError serverError;
    std::mutex serverMutex;
    std::condition_variable serverCond;
    bool isShutdown = false;

    bool gotExpectedPost = false;
    HttpError postError = HttpError::Ok;
    bool hadCorrectHeader = false;

    SocketError socketErr = initAcceptingSocket(acceptSocket, Addrinfo::getLoopback(INet::Protocol::Ipv6, 0));
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
        bool doneReading = false;
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

        StringRef contentLengthHeader;
        bool hasHeader;
        std::tie(contentLengthHeader, hasHeader) = request.getHeader("Transfer-Encoding");
        if (!hasHeader) {
            hadCorrectHeader = false;
        } else {
            hadCorrectHeader = contentLengthHeader.equals("chunked");
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

    uint32_t bytesXfer;
    StringRef request =
        "POST /formthingy HTTP/1.0\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Some \r\n"
        "e\r\n"
        "form post data\r\n"
        "0\r\n"
        "\r\n";
    StringRef expectedResponse = "HTTP/1.0 200 OK\r\n\r\n";
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
    socketErr = requestSocket.write(request.data(), request.length());
    if (socketErr != SocketError::Ok) {
        testf("Failed to write to HTTP socket with: %d", socketErr);
        return false;
    }
    uint32_t totalBytesRead = 0;
    do {
        std::tie(bytesXfer, socketErr) = requestSocket.read(responseBuffer + totalBytesRead, sizeof(responseBuffer) - totalBytesRead);
        if (socketErr != SocketError::Ok) {
            testf("Failed to read from HTTP socket with: %d", socketErr);
            return false;
        }
        totalBytesRead += bytesXfer;
    } while (bytesXfer != 0);
    if (totalBytesRead != expectedResponse.length()) {
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
        testf("Http request did not have expected Content-Length header");
        return false;
    }

    return true;
}

bool test_http1_chunked_response() {
    return true;
}

bool test_http1_keepalive() {
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

    uint32_t bytesXfer;
    std::vector<StringRef> requests = {
        "GET /index.html HTTP/1.0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "GET /index.html HTTP/1.0\r\n"
        "\r\n",
        "GET /index.html HTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n"
        "GET /index.html HTTP/1.0\r\n"
        "\r\n",
        "GET /index.html HTTP/1.0\r\n"
        "\r\n"
        "GET /index.html HTTP/1.0\r\n"
        "\r\n",
    };
    // TODO: Shouldn't expect last newline
    std::vector<StringRef> expectedResponses = {
        "HTTP/1.0 200 OK\r\nContent-Length: 11\r\n\r\nHello World\r\n"
        "HTTP/1.0 200 OK\r\nContent-Length: 11\r\n\r\nHello World\r\n",
        "HTTP/1.0 200 OK\r\nContent-Length: 11\r\n\r\nHello World\r\n",
        "HTTP/1.0 200 OK\r\nContent-Length: 11\r\n\r\nHello World\r\n",
    };

    for (size_t i = 0; i < requests.size(); i++) {
        StringRef request = requests[i];
        StringRef expectedResponse = expectedResponses[i];

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
        socketErr = requestSocket.write(request.data(), request.length());
        if (socketErr != SocketError::Ok) {
            testf("Failed to write to HTTP socket with: %d", socketErr);
            return false;
        }
        uint32_t totalBytesRead = 0;
        do {
            std::tie(bytesXfer, socketErr) = requestSocket.read(responseBuffer + totalBytesRead, sizeof(responseBuffer) - totalBytesRead);
            if (socketErr != SocketError::Ok) {
                testf("Failed to read from HTTP socket with: %d", socketErr);
                return false;
            }
            totalBytesRead += bytesXfer;
        } while (bytesXfer != 0);
        if (totalBytesRead != expectedResponse.length()) {
            testf("Did not receive expected response. Got:\n%.*s", (size_t)bytesXfer, responseBuffer);
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
