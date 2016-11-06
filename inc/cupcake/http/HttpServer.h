
#ifndef CUPCAKE_HTTP_SERVER_H
#define CUPCAKE_HTTP_SERVER_H

#include "cupcake/http/Http.h"
#include "cupcake/http/HttpError.h"
#include "cupcake/net/SockAddr.h"
#include "cupcake/text/StringRef.h"

#include "cupcake_priv/http/StreamSource.h"

#include "cupcake_priv/http/HandlerMap.h"

#include <memory>

namespace Cupcake {

/*
 * An HTTP server implementation.
 *
 * Paths can be specified exactly "/images/default.gif", or end with an asterix "/images/*".
 */
class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    bool addHandler(const StringRef path, HttpHandler handler);

    // Note: Delete ownership of the socket is NOT taken
    HttpError start(StreamSource* streamSource);

    void shutdown();

private:
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    HttpError acceptLoop();

    StreamSource* streamSource;
    HandlerMap handlerMap;
    bool started;
};

}

#endif // CUPCAKE_HTTP_SERVER_H
