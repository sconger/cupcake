
#ifndef CUPCAKE_REQUEST_DATA
#define CUPCAKE_REQUEST_DATA

#include "cupcake/http/Http.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/internal/text/String.h"

#include <vector>

namespace Cupcake {

/*
 * The HTTP parser needs to be able to pass information about the initial
 * request to the HTTP2 parser, so we need a wrapper for request information.
 *
 * TODO: Generally improve allocations
 */
class RequestData {
public:
    RequestData() = default;

    void setVersion(HttpVersion version);
    HttpVersion getVersion() const;

    void setMethod(HttpMethod method);
    HttpMethod getMethod() const;

    void setUrl(const StringRef url);
    const StringRef getUrl() const;

    void addHeaderName(const StringRef headerName);
    void addStaticHeaderName(const StringRef headerName);
    void addHeaderValue(const StringRef headerValue);
    void addStaticHeaderValue(const StringRef headerValue);
    void appendToHeaderValue(const StringRef extension);
    size_t getHeaderCount() const;
    const StringRef getHeaderName(size_t headerIndex) const;
    const StringRef getHeaderValue(size_t headerIndex) const;

    void reset();

private:
    RequestData(const RequestData&) = delete;
    RequestData& operator=(const RequestData&) = delete;

    // TODO: Probably want to allocate URL and header values our of a single buffer
    // and remember offset/length
    HttpVersion version;
    HttpMethod method;
    String url;
    std::vector<String> headerNames;
    std::vector<String> headerValues;
};

}

#endif // CUPCAKE_REQUEST_DATA
