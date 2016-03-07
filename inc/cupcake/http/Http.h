
#ifndef CUPCAKE_HTTP_H
#define CUPCAKE_HTTP_H

#include "cupcake/http/HttpError.h"
#include "cupcake/text/StringRef.h"

#include <tuple>

/*
 * Generic HTTP related definitions common to any underlying implementation.
 */

namespace Cupcake {

enum class HttpMethod {
    GET,
    HEAD,
    POST,
    PUT,
    PATCH,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT
};

class HttpInputStream {
public:
    virtual std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) = 0;
    virtual HttpError close() = 0;
};

class HttpOutputStream {
public:
    virtual std::tuple<uint32_t, HttpError> write(const char* buffer, uint32_t bufferLen) = 0;
    virtual HttpError close() = 0;
};

class HttpRequest {
public:
    virtual const HttpMethod getMethod() const = 0;
    virtual const StringRef getUrl() const = 0;

    virtual const uint32_t getHeaderCount() const = 0;
    virtual std::tuple<StringRef, StringRef> getHeader(uint32_t index) const = 0;
    virtual const StringRef getHeader(const StringRef headerName) const = 0;

    virtual HttpInputStream& getInputStream() = 0;
};

class HttpResponse {
    virtual HttpError setStatus(uint32_t code, StringRef statusText) = 0;
    virtual HttpError addHeader(StringRef headerName, StringRef headerValue) = 0;

    virtual HttpOutputStream& getOutputStream() = 0;
};

typedef std::function<void(HttpRequest& request, HttpResponse& response)> httpHandler;

}

#endif // CUPCAKE_HTTP_H
