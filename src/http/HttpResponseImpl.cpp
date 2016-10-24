
#include "cupcake_priv/http/HttpResponseImpl.h"

#include "cupcake_priv/http/CommaListIterator.h"
#include "cupcake_priv/text/Strconv.h"

#include <memory>

using namespace Cupcake;

enum class HttpResponseImpl::ResponseStatus {
    HEADERS,
    CLOSED
};

HttpResponseImpl::HttpResponseImpl(HttpVersion version, StreamSource* streamSource) :
    version(version),
    streamSource(streamSource),
    respStatus(ResponseStatus::HEADERS),
    httpOutputStream(nullptr),
    contentLengthWriter(),
    chunkedWriter(),
    bodyWritten(false),
    setContentLength(false),
    contentLength(0),
    setTeChunked(false)
{}

void HttpResponseImpl::setStatus(uint32_t code, StringRef statusText) {
    this->statusCode = code;
    this->statusText = statusText;
    statusSet = true;
}

void HttpResponseImpl::addHeader(StringRef headerName, StringRef headerValue) {
    headerNames.push_back(headerName);
    headerValues.push_back(headerValue);
}

std::tuple<HttpOutputStream*, HttpError> HttpResponseImpl::getOutputStream() {
    if (respStatus != ResponseStatus::HEADERS) {
        return std::make_tuple(nullptr, HttpError::InvalidState);
    }
    respStatus = ResponseStatus::CLOSED; // Not really, but no need for a BODY state

    HttpError err = parseHeaders();
    if (err != HttpError::Ok) {
        return std::make_tuple(nullptr, err);
    }

    if (!setContentLength && !setTeChunked) {
        // TODO: Flip to buffered Content-Length if HTTP1.0
        if (version == HttpVersion::Http1_0) {
            return std::make_tuple(nullptr, HttpError::InvalidHeader);
        }

        headerNames.push_back("Transfer_Encoding");
        headerValues.push_back("chunked");
    }

    err = writeHeaders();
    if (err != HttpError::Ok) {
        return std::make_tuple(nullptr, err);
    }
    return std::make_tuple(httpOutputStream, HttpError::Ok);
}

HttpError HttpResponseImpl::close() {
    if (respStatus == ResponseStatus::CLOSED) {
        return HttpError::Ok;
    }
    if (respStatus != ResponseStatus::HEADERS) {
        return HttpError::InvalidState;
    }
    respStatus = ResponseStatus::CLOSED;

    HttpError err = parseHeaders();
    if (err != HttpError::Ok) {
        return err;
    }

    return writeHeaders();
}

HttpError HttpResponseImpl::parseHeaders() {
    if (!statusSet ||
        (statusCode <= 100 || statusCode >= 600) ||
        statusText.length() == 0) {
        return HttpError::InvalidHeader;
    }

    for (size_t i = 0; i < headerNames.size(); i++) {
        const String& headerName = headerNames[i];
        const String& headerValue = headerValues[i];

        if (headerName.engEqualsIgnoreCase("Content-Length")) {
            if (setContentLength) {
                return HttpError::InvalidHeader;
            }

            bool validNumber;
            std::tie(contentLength, validNumber) = Strconv::parseUint64(headerValue);

            if (!validNumber) {
                return HttpError::InvalidHeader;
            }

            contentLengthWriter.init(streamSource, contentLength);
            setContentLength = true;
        } else if (headerName.engEqualsIgnoreCase("Transfer-Encoding")) {
            // Only supported in Http1.1
            if (version == HttpVersion::Http1_0) {
                return HttpError::InvalidHeader;
            }
            // Considered chunked if the last value in the comma separated list is "chunked"
            CommaListIterator commaIter(headerValue);
            setTeChunked = commaIter.getLast().engEqualsIgnoreCase("chunked");
        }
    }

    if (setContentLength) {
        contentLengthWriter.init(streamSource, contentLength);
        httpOutputStream = &contentLengthWriter;
    } else if (setTeChunked) {
        chunkedWriter.init(streamSource);
        httpOutputStream = &chunkedWriter;
    }

    return HttpError::Ok;
}

HttpError HttpResponseImpl::writeHeaders() {
    size_t buffersNeeded = 5 + (4 * headerNames.size());
    // TODO: malloca
    std::auto_ptr<INet::IoBuffer> ioBufHolder(new INet::IoBuffer[buffersNeeded]);
    INet::IoBuffer* ioBufs = ioBufHolder.get();

    char codeBuffer[12];
    size_t codeBytes = Strconv::uint32ToStr(statusCode, codeBuffer, sizeof(codeBuffer));
    codeBuffer[codeBytes] = ' ';

    if (version == HttpVersion::Http1_0) {
        ioBufs[0].buffer = "HTTP/1.0 ";
    } else {
        ioBufs[0].buffer = "HTTP/1.1 ";
    }
    ioBufs[0].bufferLen = 9;
    ioBufs[1].buffer = codeBuffer;
    ioBufs[1].bufferLen = codeBytes + 1;
    ioBufs[2].buffer = (char*)statusText.data();
    ioBufs[2].bufferLen = (uint32_t)statusText.length();
    ioBufs[3].buffer = "\r\n";
    ioBufs[3].bufferLen = 2;

    for (size_t i = 0; i < headerNames.size(); i++) {
        const String& headerName = headerNames[i];
        const String& headerValue = headerValues[i];

        size_t offset = (4 * i) + 4;
        ioBufs[offset].buffer = (char*)headerName.data();
        ioBufs[offset].bufferLen = (uint32_t)headerName.length();
        ioBufs[offset + 1].buffer = ": ";
        ioBufs[offset + 1].bufferLen = 2;
        ioBufs[offset + 2].buffer = (char*)headerValue.data();
        ioBufs[offset + 2].bufferLen = (uint32_t)headerValue.length();
        ioBufs[offset + 3].buffer = "\r\n";
        ioBufs[offset + 3].bufferLen = 2;
    }

    ioBufs[buffersNeeded - 1].buffer = "\r\n";
    ioBufs[buffersNeeded - 1].bufferLen = 2;

    return streamSource->writev(ioBufs, buffersNeeded);
}
