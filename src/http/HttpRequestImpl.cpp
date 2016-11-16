
#include "cupcake/internal/http/HttpRequestImpl.h"

using namespace Cupcake;

HttpRequestImpl::HttpRequestImpl(HttpMethod method,
    StringRef url,
    const std::vector<String>& headerNames,
    const std::vector<String>& headerValues,
    HttpInputStream& inputStream) :
    method(method),
    url(url),
    headerNames(headerNames),
    headerValues(headerValues),
    inputStream(inputStream)
{}

const HttpMethod HttpRequestImpl::getMethod() const {
    return method;
}

const StringRef HttpRequestImpl::getUrl() const {
    return url;
}

uint32_t HttpRequestImpl::getHeaderCount() const {
    return headerNames.size();
}

std::tuple<StringRef, StringRef> HttpRequestImpl::getHeader(uint32_t index) const {
    return std::make_tuple(headerNames[index], headerValues[index]);
}

std::tuple<StringRef, bool> HttpRequestImpl::getHeader(const StringRef headerName) const {
    for (size_t i = 0; i < headerNames.size(); i++) {
        if (headerNames[i].engEqualsIgnoreCase(headerName)) {
            return std::make_tuple(headerValues[i], true);
        }
    }
    return std::make_tuple(StringRef(), false);
}

HttpInputStream& HttpRequestImpl::getInputStream() {
    return inputStream;
}
