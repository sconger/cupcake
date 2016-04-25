
#ifndef CUPCAKE_HTTP_CONNECTION_H
#define CUPCAKE_HTTP_CONNECTION_H

#include "cupcake/text/String.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/http/Http.h"
#include "cupcake_priv/http/BufferedReader.h"
#include "cupcake_priv/http/HandlerMap.h"
#include "cupcake_priv/http/StreamSource.h"

#include <tuple>
#include <vector>

namespace Cupcake {

/*
 * Wrapper around a connection that will be treated as HTTP traffic.
 */
class HttpConnection {
public:
    HttpConnection(StreamSource* streamSource, const HandlerMap* handlerMap);
    ~HttpConnection();

    void run();

private:
    enum class HttpState;

    HttpConnection(const HttpConnection&) = delete;
    HttpConnection& operator=(const HttpConnection&) = delete;

    HttpError innerRun();

    std::tuple<StringRef, HttpError> readLine();
    bool parseRequestLine(const StringRef line);
    bool parseHeaderLine(const StringRef line);
    HttpError sendStatus(uint32_t code, const StringRef reasonPhrase);

    const HandlerMap* handlerMap;
    BufferedReader bufReader;
    StreamSource* streamSource;
    HttpState state;

    HttpMethod curMethod;
    HttpVersion curVersion;
    String curUrl;
    std::vector<String> curHeaderNames;
    std::vector<String> curHeaderValues;
};

}

#endif // CUPCAKE_HTTP_CONNECTION_H
