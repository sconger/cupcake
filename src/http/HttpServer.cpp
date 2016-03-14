
#include "cupcake/http/HttpServer.h"

#include "cupcake/async/Async.h"

using namespace Cupcake;

HttpServer::HttpServer() :
    started(false)
{}

HttpServer::~HttpServer() {
    shutdown();
}

bool HttpServer::addHandler(const StringRef path, HttpHandler handler) {
    return true;
}

SocketError HttpServer::start(const SockAddr& sockAddr) {
    if (started) {
        return SocketError::InvalidState;
    }

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

    while (true) {
        Result<Socket, SocketError> res = socket.accept();

        if (!res.ok()) {
            return res.error();
        }

        Socket acceptedSocket = std::move(res.get());

        // TODO: Actually do something
    }

    return SocketError::Ok;
}

void HttpServer::shutdown() {
    if (!started) {
        return;
    }

    socket.close();
}
