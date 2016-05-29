
#ifndef CUPCAKE_HTTP_RESPONSE_IMPL
#define CUPCAKE_HTTP_RESPONSE_IMPL

#include "cupcake/http/Http.h"
#include "cupcake/http/HttpError.h"

#include "cupcake_priv/http/StreamSource.h"

namespace Cupcake {

class HttpResponseImpl : public HttpResponse {
public:
    HttpResponseImpl(StreamSource& streamSource);
    HttpError setStatus(uint32_t code, StringRef statusText) override;
    HttpError addHeader(StringRef headerName, StringRef headerValue) override;

    HttpOutputStream& getOutputStream() const override;

private:
    HttpResponseImpl(const HttpResponseImpl&) = delete;
    HttpResponseImpl& operator=(const HttpResponseImpl&) = delete;

    StreamSource& streamSource;
};

}

#endif // CUPCAKE_HTTP_RESPONSE_IMPL
