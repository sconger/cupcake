
#ifndef CUPCAKE_HTTP_RESPONSE_IMPL
#define CUPCAKE_HTTP_RESPONSE_IMPL

#include "cupcake/http/Http.h"
#include "cupcake/http/HttpError.h"

#include "cupcake_priv/http/ContentLengthWriter.h"
#include "cupcake_priv/http/StreamSource.h"

namespace Cupcake {

class HttpResponseImpl : public HttpResponse {
public:
    // TODO: Probably need more parameters to support 100 Continue properly
    HttpResponseImpl(HttpVersion version, StreamSource* streamSource);
    HttpError setStatus(uint32_t code, StringRef statusText) override;
    HttpError addHeader(StringRef headerName, StringRef headerValue) override;

    HttpOutputStream& getOutputStream() const override;

    HttpError addBlankLineIfNeeded();

private:
    enum class ResponseStatus;

    HttpResponseImpl(const HttpResponseImpl&) = delete;
    HttpResponseImpl& operator=(const HttpResponseImpl&) = delete;

    HttpVersion version;
    StreamSource* streamSource;
    ResponseStatus respStatus;

    HttpOutputStream* httpOutputStream;
    ContentLengthWriter contentLengthWriter;
    mutable bool bodyWritten;
};

}

#endif // CUPCAKE_HTTP_RESPONSE_IMPL
