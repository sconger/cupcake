
#include "cupcake/internal/http/HttpRequestImpl.h"

using namespace Cupcake;

HttpRequestImpl::HttpRequestImpl(RequestData& requestData,
    HttpInputStream& inputStream) :
    requestData(requestData),
    inputStream(inputStream)
{}

const HttpMethod HttpRequestImpl::getMethod() const {
    return requestData.getMethod();
}

const StringRef HttpRequestImpl::getUrl() const {
    return requestData.getUrl();
}

uint32_t HttpRequestImpl::getHeaderCount() const {
    return requestData.getHeaderCount();
}

std::tuple<StringRef, StringRef> HttpRequestImpl::getHeader(uint32_t index) const {
    return std::make_tuple(requestData.getHeaderName(index), requestData.getHeaderValue(index));
}

std::tuple<StringRef, bool> HttpRequestImpl::getHeader(const StringRef headerName) const {
    // TODO: This should probably merge into a comma delimited list if there are multiple
    for (size_t i = 0; i < requestData.getHeaderCount(); i++) {
        const StringRef testName = requestData.getHeaderName(i);
        if (testName.engEqualsIgnoreCase(headerName)) {
            return std::make_tuple(requestData.getHeaderValue(i), true);
        }
    }
    return std::make_tuple(StringRef(), false);
}

HttpInputStream& HttpRequestImpl::getInputStream() {
    return inputStream;
}
