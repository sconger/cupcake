
#ifndef CUPCAKE_HTTP_SERVER_H
#define CUPCAKE_HTTP_SERVER_H

#include "cupcake/http/Http.h"
#include "cupcake/http/HttpError.h"
#include "cupcake/net/Sockaddr.h"
#include "cupcake/text/StringRef.h"

#include "cupcake_priv/http/StreamSource.h"

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

    // TODO: Add other variants
    HttpError start(const SockAddr& sockAddr);

    void shutdown();

private:
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    HttpError acceptLoop();

    std::unique_ptr<StreamSource> streamSource;
    bool started;
};

}

#endif // CUPCAKE_HTTP_SERVER_H
