
#include "cupcake_priv/http/HttpResponseImpl.h"

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
    contentLengthWriter(streamSource),
    bodyWritten(false)
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

    HttpError err;
    std::tie(std::ignore, err) = streamSource->writev(ioBufs, 4);
    return err;
}

HttpError HttpResponseImpl::addHeader(StringRef headerName, StringRef headerValue) {
    // TODO: Detect chunked header

    INet::IoBuffer ioBufs[4];
    ioBufs[0].buffer = (char*)headerName.data();
    ioBufs[0].bufferLen = (uint32_t)headerName.length();
    ioBufs[1].buffer = ": ";
    ioBufs[1].bufferLen = 2;
    ioBufs[2].buffer = (char*)headerValue.data();
    ioBufs[2].bufferLen = (uint32_t)headerValue.length();
    ioBufs[3].buffer = "/r/n";
    ioBufs[3].bufferLen = 2;

    HttpError err;
    std::tie(std::ignore, err) = streamSource->writev(ioBufs, 4);
    return err;
}

HttpOutputStream& HttpResponseImpl::getOutputStream() const {
    bodyWritten = true;
    return *httpOutputStream;
}

HttpError HttpResponseImpl::addBlankLineIfNeeded() {
    HttpError err = HttpError::Ok;
    if (!bodyWritten) {
        std::tie(std::ignore, err) = streamSource->write("\r\n", 2);
    }
    return err;
}
