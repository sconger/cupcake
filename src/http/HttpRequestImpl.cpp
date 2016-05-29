
#include "cupcake_priv/http/HttpRequestImpl.h"

using namespace Cupcake;

HttpRequestImpl::HttpRequestImpl(HttpMethod method,
    StringRef url,
    std::vector<String>& headerNames,
    std::vector<String>& headerValues,
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
    StringRef headerName(headerNames[index]);
    StringRef headerValue(headerValues[index]);
    return std::make_tuple(headerName, headerValue);
}

std::tuple<StringRef, bool> HttpRequestImpl::getHeader(const StringRef headerName) const {
    for (const String& testName : headerNames) {
        StringRef testNameRef(testName);
        if (testNameRef.engEqualsIgnoreCase(headerName)) {
            return std::make_tuple(testNameRef, true);
        }
    }
    return std::make_tuple(StringRef(), false);
}

HttpInputStream& HttpRequestImpl::getInputStream() {
    return inputStream;
}
