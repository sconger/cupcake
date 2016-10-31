
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
    Get,
    Head,
    Post,
    Put,
    Patch,
    Delete,
    Trace,
    Options,
    Connect
};

enum class HttpVersion {
    Http1_0,
    Http1_1,
    Http2_0,
};

class HttpInputStream {
public:
    virtual std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) = 0;
    virtual HttpError close() = 0;
};

class HttpOutputStream {
public:
    virtual HttpError write(const char* buffer, uint32_t bufferLen) = 0;
    virtual HttpError flush() = 0;
    virtual HttpError close() = 0;
};

class HttpRequest {
public:
    virtual const HttpMethod getMethod() const = 0;
    virtual const StringRef getUrl() const = 0;

    virtual uint32_t getHeaderCount() const = 0;
    virtual std::tuple<StringRef, StringRef> getHeader(uint32_t index) const = 0;
    virtual std::tuple<StringRef, bool> getHeader(const StringRef headerName) const = 0;

    virtual HttpInputStream& getInputStream() = 0;
};

class HttpResponse {
public:
    virtual void setStatus(uint32_t code, StringRef statusText) = 0;
    virtual void addHeader(StringRef headerName, StringRef headerValue) = 0;

    virtual std::tuple<HttpOutputStream*, HttpError> getOutputStream() = 0;

    virtual HttpError close() = 0;
};

typedef std::function<void(HttpRequest& request, HttpResponse& response)> HttpHandler;

}

#endif // CUPCAKE_HTTP_H
