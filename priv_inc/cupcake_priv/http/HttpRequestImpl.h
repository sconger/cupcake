
#ifndef CUPCAKE_HTTP_REQUEST_IMPL
#define CUPCAKE_HTTP_REQUEST_IMPL

#include "cupcake/http/Http.h"
#include "cupcake/text/StringRef.h"

#include "cupcake_priv/http/ContentLengthReader.h"

#include <memory>
#include <vector>

namespace Cupcake {

class HttpRequestImpl : public HttpRequest {
public:
    HttpRequestImpl(HttpMethod method,
        StringRef url,
        const std::vector<String>& headerNames,
        const std::vector<String>& headerValues,
        HttpInputStream& inputStream);

    const HttpMethod getMethod() const override;
    const StringRef getUrl() const override;

    uint32_t getHeaderCount() const override;
    std::tuple<StringRef, StringRef> getHeader(uint32_t index) const override;
    std::tuple<StringRef, bool> getHeader(const StringRef headerName) const override;

    HttpInputStream& getInputStream() override;

private:
    HttpMethod method;
    StringRef url;
    const std::vector<String>& headerNames;
    const std::vector<String>& headerValues;

    HttpInputStream& inputStream;
};

}

#endif // CUPCAKE_HTTP_REQUEST_IMPL