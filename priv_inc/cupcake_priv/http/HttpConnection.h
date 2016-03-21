
#ifndef CUPCAKE_HTTP_CONNECTION_H
#define CUPCAKE_HTTP_CONNECTION_H

#include "cupcake/net/Socket.h"
#include "cupcake/net/SocketError.h"
#include "cupcake/text/StringRef.h"

#include "cupcake_priv/http/StreamSource.h"

#include <tuple>

namespace Cupcake {

/*
 * Wrapper around a connection that will be treated as HTTP traffic.
 */
class HttpConnection {
public:
    HttpConnection(StreamSource* streamSource);
    ~HttpConnection();

    void run();

private:
    enum HttpState;

    HttpConnection(const HttpConnection&) = delete;
    HttpConnection& operator=(const HttpConnection&) = delete;

    std::tuple<StringRef, SocketError> readLine();
    bool parseRequestLine();
    bool parseHeaderLine();
    SocketError sendStatus(uint32_t code, const StringRef reasonPhrase);

    StreamSource* streamSource;
    HttpState state;
};

}

#endif // CUPCAKE_HTTP_CONNECTION_H
