
#ifndef CUPCAKE_HTTP_SERVER_H
#define CUPCAKE_HTTP_SERVER_H

#include "cupcake/http/Http.h"
#include "cupcake/net/Sockaddr.h"
#include "cupcake/net/Socket.h"
#include "cupcake/net/SocketError.h"
#include "cupcake/text/StringRef.h"

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

    SocketError start(const SockAddr& sockAddr);

    void shutdown();

private:
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    Socket socket;
    bool started;
};

}

#endif // CUPCAKE_HTTP_SERVER_H
