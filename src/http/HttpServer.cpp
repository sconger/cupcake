
#include "cupcake/http/HttpServer.h"

#include "cupcake/async/Async.h"

#include "cupcake_priv/http/HttpConnection.h"
#include "cupcake_priv/http/StreamSourceSocket.h"

using namespace Cupcake;

HttpServer::HttpServer() :
    started(false)
{}

HttpServer::~HttpServer() {
    shutdown();
}

bool HttpServer::addHandler(const StringRef path, HttpHandler handler) {
    return handlerMap.addHandler(path, handler);
}

HttpError HttpServer::start(const SockAddr& sockAddr) {
    if (started) {
        return HttpError::InvalidState;
    }

    // TODO: Generally need error brains
    Socket socket;

    SocketError err = socket.init(sockAddr.getFamily());

    if (err != SocketError::Ok) {
        return HttpError::IoError;
    }

    err = socket.bind(sockAddr);

    if (err != SocketError::Ok) {
        return HttpError::IoError;
    }

    err = socket.listen();

    if (err != SocketError::Ok) {
        return HttpError::IoError;
    }

    streamSource.reset(new StreamSourceSocket(std::move(socket)));

    return acceptLoop();
}

HttpError HttpServer::acceptLoop() {
    const HandlerMap* handlerMapPtr = &handlerMap;

    while (true) {
        StreamSource* acceptedSocket;
        HttpError err;
        std::tie(acceptedSocket, err) = streamSource->accept();

        if (err != HttpError::Ok) {
            return err;
        }

        Async::runAsync([&acceptedSocket, handlerMapPtr] {
            HttpConnection httpConnection(acceptedSocket, handlerMapPtr);
            try {
                httpConnection.run();
            }
            catch (...) {
                // TODO: log
            }
        });
    }
}

void HttpServer::shutdown() {
    if (!started) {
        return;
    }

    if (streamSource) {
        streamSource->close();
    }
}
