
#ifndef CUPCAKE_HTTP_RESPONSE_IMPL
#define CUPCAKE_HTTP_RESPONSE_IMPL

#include "cupcake/http/Http.h"
#include "cupcake/http/HttpError.h"

#include "cupcake_priv/http/BufferedContentLengthWriter.h"
#include "cupcake_priv/http/ChunkedWriter.h"
#include "cupcake_priv/http/ContentLengthWriter.h"
#include "cupcake_priv/http/StreamSource.h"

#include <vector>

namespace Cupcake {

class HttpResponseImpl : public HttpResponse {
public:
    // TODO: Probably need more parameters to support 100 Continue properly
    HttpResponseImpl(HttpVersion version, StreamSource* streamSource);
    void setStatus(uint32_t code, StringRef statusText) override;
    void addHeader(StringRef headerName, StringRef headerValue) override;

    std::tuple<HttpOutputStream*, HttpError> getOutputStream() override;

    HttpError close() override;

    // For special case of buffering data for unknown Content-Length
    HttpError writeHeadersAndBody(const char* content, size_t contentLen);

private:
    enum class ResponseStatus;

    HttpResponseImpl(const HttpResponseImpl&) = delete;
    HttpResponseImpl& operator=(const HttpResponseImpl&) = delete;

    HttpError parseHeaders();
    HttpError writeHeaders();

    HttpVersion version;
    StreamSource* streamSource;
    ResponseStatus respStatus;

    HttpOutputStream* httpOutputStream;
    BufferedContentLengthWriter bufferedContentLengthWriter;
    ContentLengthWriter contentLengthWriter;
    ChunkedWriter chunkedWriter;

    // TODO: Avoid allocations for strings
    uint32_t statusCode;
    String statusText;
    std::vector<String> headerNames;
    std::vector<String> headerValues;
    bool statusSet;
    bool setContentLength;
    uint64_t contentLength;
    bool setTeChunked;
    mutable bool bodyWritten;
};

}

#endif // CUPCAKE_HTTP_RESPONSE_IMPL
