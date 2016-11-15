
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

HttpError HttpServer::start(StreamSource* streamSource) {
    if (started) {
        return HttpError::InvalidState;
    }
    started = true;

    this->streamSource = streamSource;

    return acceptLoop();
}

HttpError HttpServer::acceptLoop() {
    const HandlerMap* handlerMapPtr = &handlerMap;

    while (true) {
        StreamSource* acceptedSocket;
        HttpError err;
        std::tie(acceptedSocket, err) = streamSource->accept();

        if (err == HttpError::StreamClosed) {
            return HttpError::Ok;
        } else if (err != HttpError::Ok) {
            return err;
        }

        Async::runAsync([&acceptedSocket, handlerMapPtr] {
            BufferedReader bufReader;
            bufReader.init(acceptedSocket, 2048); // TODO: Define read constant somewhere
            HttpConnection httpConnection(acceptedSocket, bufReader, handlerMapPtr);
            try {
                HttpConnection::UpgradeType upgradeType = httpConnection.run();

                if (upgradeType != HttpConnection::UpgradeType::None) {
                    // TODO: Http2 upgrade
                }
            }
            catch (...) {
                // TODO: log
            }
            acceptedSocket->close();
        });
    }
}

void HttpServer::shutdown() {
    if (!started) {
        return;
    }

    if (streamSource) {
        streamSource->close();
        streamSource = nullptr;
    }
}
