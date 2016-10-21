
#include "cupcake_priv/http/HttpResponseImpl.h"

#include "cupcake_priv/http/CommaListIterator.h"
#include "cupcake_priv/text/Strconv.h"

using namespace Cupcake;

enum class HttpResponseImpl::ResponseStatus {
    NEW,
    SENDING_HEADERS,
    SENDING_BODY
};

HttpResponseImpl::HttpResponseImpl(HttpVersion version, StreamSource* streamSource) :
    version(version),
    streamSource(streamSource),
    respStatus(ResponseStatus::NEW),
    httpOutputStream(nullptr),
    contentLengthWriter(),
    chunkedWriter(),
    bodyWritten(false),
    setContentLength(false),
    contentLength(0),
    setTeChunked(false)
{}

HttpError HttpResponseImpl::setStatus(uint32_t code, StringRef statusText) {
    if (respStatus != ResponseStatus::NEW) {
        return HttpError::InvalidState;
    }

    char codeBuffer[12];
    size_t codeBytes = Strconv::uint32ToStr(code, codeBuffer, sizeof(codeBuffer));
    codeBuffer[codeBytes] = ' ';

    INet::IoBuffer ioBufs[4];
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

    return streamSource->writev(ioBufs, 4);
}

HttpError HttpResponseImpl::addHeader(StringRef headerName, StringRef headerValue) {
    // If it's a Content-Length header, parse it and initialize the writer
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

    INet::IoBuffer ioBufs[4];
    ioBufs[0].buffer = (char*)headerName.data();
    ioBufs[0].bufferLen = (uint32_t)headerName.length();
    ioBufs[1].buffer = ": ";
    ioBufs[1].bufferLen = 2;
    ioBufs[2].buffer = (char*)headerValue.data();
    ioBufs[2].bufferLen = (uint32_t)headerValue.length();
    ioBufs[3].buffer = "\r\n";
    ioBufs[3].bufferLen = 2;

    return streamSource->writev(ioBufs, 4);
}

std::tuple<HttpOutputStream*, HttpError> HttpResponseImpl::getOutputStream() {
    HttpError err = HttpError::Ok;
    bodyWritten = true;

    if (setContentLength) {
        contentLengthWriter.init(streamSource, contentLength);
        httpOutputStream = &contentLengthWriter;
    } else if (setTeChunked) {
        chunkedWriter.init(streamSource);
        httpOutputStream = &chunkedWriter;
    } else {
        // TODO: Flip to buffered Content-Length if HTTP1.0
        if (version == HttpVersion::Http1_0) {
            return std::make_tuple(nullptr, HttpError::InvalidHeader);
        }

        err = addHeader("Transfer-Encoding", "Chunked");
        if (err != HttpError::Ok) {
            return std::make_tuple(nullptr, err);
        }

        chunkedWriter.init(streamSource);
        httpOutputStream = &chunkedWriter;
    }

    err = streamSource->write("\r\n", 2);
    return std::make_tuple(httpOutputStream, err);
}

HttpError HttpResponseImpl::addBlankLineIfNeeded() {
    if (!bodyWritten) {
        return streamSource->write("\r\n", 2);
    }
    return HttpError::Ok;
}
