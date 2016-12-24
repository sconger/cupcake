
#ifndef CUPCAKE_HTTP_REQUEST_IMPL
#define CUPCAKE_HTTP_REQUEST_IMPL

#include "cupcake/http/Http.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/internal/http/ContentLengthReader.h"
#include "cupcake/internal/http/RequestData.h"

#include <memory>
#include <vector>

namespace Cupcake {

class HttpRequestImpl : public HttpRequest {
public:
    HttpRequestImpl(RequestData& requestData,
        HttpInputStream& inputStream);

    const HttpMethod getMethod() const override;
    const StringRef getUrl() const override;

    uint32_t getHeaderCount() const override;
    std::tuple<StringRef, StringRef> getHeader(uint32_t index) const override;
    std::tuple<StringRef, bool> getHeader(const StringRef headerName) const override;

    HttpInputStream& getInputStream() override;

private:
    RequestData& requestData;
    HttpInputStream& inputStream;
};

}

#endif // CUPCAKE_HTTP_REQUEST_IMPL