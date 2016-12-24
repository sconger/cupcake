
#include "cupcake/internal/http/RequestData.h"

using namespace Cupcake;

void RequestData::setVersion(HttpVersion newVersion) {
    version = newVersion;
}

HttpVersion RequestData::getVersion() const {
    return version;
}

void RequestData::setMethod(HttpMethod newMethod) {
    method = newMethod;
}

HttpMethod RequestData::getMethod() const {
    return method;
}

void RequestData::setUrl(const StringRef newUrl) {
    url = newUrl;
}

const StringRef RequestData::getUrl() const {
    return url;
}

void RequestData::addHeaderName(const StringRef headerName) {
    headerNames.push_back(headerName);
}

void RequestData::addStaticHeaderName(const StringRef headerName) {
    // TODO: Avoid allocation
    addHeaderName(headerName);
}

void RequestData::addHeaderValue(const StringRef headerValue) {
    headerValues.push_back(headerValue);
}

void RequestData::addStaticHeaderValue(const StringRef headerValue) {
    // TODO: Avoid allocation
    addHeaderValue(headerValue);
}

void RequestData::appendToHeaderValue(const StringRef extension) {
    headerValues.back().append(extension);
}


size_t RequestData::getHeaderCount() const {
    return headerNames.size();
}

const StringRef RequestData::getHeaderName(size_t headerIndex) const {
    return headerNames.at(headerIndex);
}

const StringRef RequestData::getHeaderValue(size_t headerIndex) const {
    return headerValues.at(headerIndex);
}

void RequestData::reset() {
    method = HttpMethod();
    url.clear();
    headerNames.clear();
    headerValues.clear();
}