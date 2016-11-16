
#include "cupcake/internal/http/HttpResponseImpl.h"

#include "cupcake/internal/http/CommaListIterator.h"
#include "cupcake/internal/text/Strconv.h"

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
    if (httpOutputStream) {
        return std::make_tuple(httpOutputStream, HttpError::Ok);
    }
    
    if (respStatus != ResponseStatus::HEADERS) {
        return std::make_tuple(nullptr, HttpError::InvalidState);
    }
    respStatus = ResponseStatus::CLOSED; // Not really, but no need for a BODY state
    
    HttpError err = parseHeaders();
    if (err != HttpError::Ok) {
        return std::make_tuple(nullptr, err);
    }
    
    if (!setContentLength && !setTeChunked) {
        // In the special case that it's HTTP1.0, and no content-length was specified,
        // return a writer that buffers the content
        if (version == HttpVersion::Http1_0) {
            bufferedContentLengthWriter.init(this);
            httpOutputStream = &bufferedContentLengthWriter;
            return std::make_tuple(httpOutputStream, HttpError::Ok);
        }
        
        headerNames.push_back("Transfer-Encoding");
        headerValues.push_back("chunked");
        chunkedWriter.init(streamSource);
        httpOutputStream = &chunkedWriter;
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

HttpError HttpResponseImpl::writeHeadersAndBody(const char* bufferedContent, size_t bufferedContentLen) {
    
    // If this is the special case of an HTTP1/0 response where we're buffering
    // content of an unknown length, append a content length header, and make
    // room in the iovec array for the data.
    if (bufferedContentLen != 0) {
        char contentLenBuffer[20];
        size_t contentLengthStrLen = Strconv::int64ToStr((uint64_t)bufferedContentLen, contentLenBuffer, sizeof(contentLenBuffer));
        headerNames.push_back("Content-Length");
        headerValues.push_back(StringRef(contentLenBuffer, contentLengthStrLen));
    }
    
    size_t buffersNeeded = 5 + (4 * headerNames.size());
    if (bufferedContentLen != 0) {
        buffersNeeded++;
    }
    
    // TODO: malloca
    std::auto_ptr<INet::IoBuffer> ioBufHolder(new INet::IoBuffer[buffersNeeded]);
    INet::IoBuffer* ioBufs = ioBufHolder.get();
    
    char codeBuffer[12];
    size_t codeBytes = Strconv::uint32ToStr(statusCode, codeBuffer, sizeof(codeBuffer));
    codeBuffer[codeBytes] = ' ';
    
    if (version == HttpVersion::Http1_0) {
        ioBufs[0].buffer = (char*)"HTTP/1.0 ";
    } else {
        ioBufs[0].buffer = (char*)"HTTP/1.1 ";
    }
    ioBufs[0].bufferLen = 9;
    ioBufs[1].buffer = codeBuffer;
    ioBufs[1].bufferLen = codeBytes + 1;
    ioBufs[2].buffer = (char*)statusText.data();
    ioBufs[2].bufferLen = (uint32_t)statusText.length();
    ioBufs[3].buffer = (char*)"\r\n";
    ioBufs[3].bufferLen = 2;
    
    for (size_t i = 0; i < headerNames.size(); i++) {
        const String& headerName = headerNames[i];
        const String& headerValue = headerValues[i];
        
        size_t offset = (4 * i) + 4;
        ioBufs[offset].buffer = (char*)headerName.data();
        ioBufs[offset].bufferLen = (uint32_t)headerName.length();
        ioBufs[offset + 1].buffer = (char*)": ";
        ioBufs[offset + 1].bufferLen = 2;
        ioBufs[offset + 2].buffer = (char*)headerValue.data();
        ioBufs[offset + 2].bufferLen = (uint32_t)headerValue.length();
        ioBufs[offset + 3].buffer = (char*)"\r\n";
        ioBufs[offset + 3].bufferLen = 2;
    }
    
    if (bufferedContentLen != 0) {
        // TODO: Ideally, we'd handle content length over 2 gigs... but *shrug*
        ioBufs[buffersNeeded - 2].buffer = (char*)"\r\n";
        ioBufs[buffersNeeded - 2].bufferLen = 2;
        ioBufs[buffersNeeded - 1].buffer = (char*)bufferedContent;
        ioBufs[buffersNeeded - 1].bufferLen = (uint32_t)bufferedContentLen;
    } else {
        ioBufs[buffersNeeded - 1].buffer = (char*)"\r\n";
        ioBufs[buffersNeeded - 1].bufferLen = 2;
    }
    
    return streamSource->writev(ioBufs, buffersNeeded);
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
    return writeHeadersAndBody(nullptr, 0);
}
