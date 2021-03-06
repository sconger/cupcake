
#include "cupcake/internal/http/HttpConnection.h"

#include "cupcake/internal/http/ChunkedReader.h"
#include "cupcake/internal/http/CommaListIterator.h"
#include "cupcake/internal/http/ContentLengthReader.h"
#include "cupcake/internal/http/HttpRequestImpl.h"
#include "cupcake/internal/http/HttpResponseImpl.h"
#include "cupcake/internal/http/NullReader.h"
#include "cupcake/internal/text/Strconv.h"

#include <unordered_map>

using namespace Cupcake;

static
std::unordered_map<StringRef, HttpMethod> methodLookupMap = {
    {"CONNECT", HttpMethod::Connect},
    {"DELETE", HttpMethod::Delete},
    {"GET", HttpMethod::Get},
    {"HEAD", HttpMethod::Head},
    {"OPTIONS", HttpMethod::Options},
    {"POST", HttpMethod::Post},
    {"PUT", HttpMethod::Put},
    {"TRACE", HttpMethod::Trace},
};

enum class HttpConnection::HttpState {
    Headers,
    Body,
    Failed,
};

HttpConnection::HttpConnection(StreamSource* streamSource, BufferedReader& bufReader, const HandlerMap* handlerMap) :
    handlerMap(handlerMap),
    bufReader(bufReader),
    streamSource(streamSource),
    state(HttpState::Headers),
    keepAlive(false),
    hasContentLength(false),
    contentLength(0),
    isChunked(false),
    hasHost(false)
{}

HttpConnection::~HttpConnection() {
    // TODO
}

HttpConnection::UpgradeType HttpConnection::run() {
    // TODO: log
    HttpError err;
    UpgradeType upgradeType;
    std::tie(upgradeType, err) = innerRun();
    err = streamSource->close();

    return upgradeType;
}

std::tuple<HttpConnection::UpgradeType, HttpError> HttpConnection::innerRun() {
    HttpError err;
    StringRef line;
    Status status;
    bool http2Preface;

    // Check for an HTTP2 preface (client knows HTTP2 is supported)
    std::tie(http2Preface, err) = bufReader.peekMatch("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
    if (err != HttpError::Ok) {
        return std::make_tuple(UpgradeType::None, err);
    }
    if (http2Preface) {
        return std::make_tuple(UpgradeType::H2C_Upgrade, HttpError::Ok);
    }

    do {
        state = HttpState::Headers;
        requestData.reset();
        keepAlive = false;
        hasContentLength = false;
        contentLength = 0;
        isChunked = false;
        hasHost = false;

        // Read the request line
        std::tie(line, err) = bufReader.readLine(64 * 1024); // TODO: Define limit somewhere
        if (err != HttpError::Ok) {
            return std::make_tuple(UpgradeType::None, err);
        }

        status = parseRequestLine(line);
        if (!status.ok()) {
            break;
        }

        // Read the headers
        do {
            std::tie(line, err) = bufReader.readLine(1 * 1024 * 1024); // TODO: Define limit somewhere
            if (err != HttpError::Ok) {
                return std::make_tuple(UpgradeType::None, err);
            }

            // Switches to body state on empty line
            status = parseHeaderLine(line);
            if (!status.ok()) {
                break;
            }
        } while (state == HttpState::Headers);

        // Go through the headers so far and parse out special ones we need to pay attention to
        status = parseSpecialHeaders();
        if (!status.ok()) {
            break;
        }

        // Deal with contradictory headers
        status = checkAndFixupHeaders();
        if (!status.ok()) {
            break;
        }

        // Lookup a handler for the URL
        HttpHandler handler;
        bool foundHandler;
        std::tie(handler, foundHandler) = handlerMap->getHandler(requestData.getUrl());

        // If there is no handler, just 404 and loop
        if (!foundHandler) {
            err = sendStatus(404, "Not Found");
            if (err != HttpError::Ok) {
                return std::make_tuple(UpgradeType::None, err);
            }
            continue;
        }

        // Set up a reader based on the request
        ChunkedReader chunkedReader(bufReader);
        ContentLengthReader contentLengthReader(bufReader, contentLength);
        NullReader nullReader;

        HttpInputStream* inputStream;
        if (isChunked) {
            inputStream = &chunkedReader;
        } else if (hasContentLength) {
            inputStream = &contentLengthReader;
        } else {
            inputStream = &nullReader;
        }

        // Create request and response objects
        HttpRequestImpl requestImpl(requestData, *inputStream);
        HttpResponseImpl responseImpl(requestData.getVersion(), streamSource);

        // Run the user handler
        handler(requestImpl, responseImpl);

        err = responseImpl.close();
        if (err != HttpError::Ok) {
            return std::make_tuple(UpgradeType::None, err);
        }
    } while (keepAlive);

    // If we exit the main loop because we need to emit a status, it should be a
    // fatal one requiring closing the connection.
    if (!status.ok()) {
        err = sendStatus(status.code, status.reasonPhrase);
        if (err != HttpError::Ok) {
            return std::make_tuple(UpgradeType::None, err);
        }
    }

    return std::make_tuple(UpgradeType::None, HttpError::Ok);
}

HttpConnection::Status HttpConnection::parseRequestLine(const StringRef line) {
    // Assuming exactly two spaces at the moment
    ptrdiff_t firstSpaceIndex = line.indexOf(' ');
    if (firstSpaceIndex == -1 || firstSpaceIndex == line.length() - 1) {
        return Status(400, "Bad Request");
    }

    ptrdiff_t secondSpaceIndex = line.indexOf(' ', firstSpaceIndex + 1);
    if (secondSpaceIndex == -1) {
        return Status(400, "Bad Request");
    }

    // Extract method
    StringRef methodStr = line.substring(0, firstSpaceIndex);
    auto methodLookup = methodLookupMap.find(methodStr);
    if (methodLookup == methodLookupMap.end()) {
        // https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1.1
        return Status(501, "Not Implemented");
    }

    requestData.setMethod(methodLookup->second);

    // Extract URL
    StringRef url = line.substring(firstSpaceIndex + 1, secondSpaceIndex);

    if (url.length() == 0) {
        return Status(400, "Bad Request");
    }

    requestData.setUrl(url);

    // Extract the version
    StringRef version = line.substring(secondSpaceIndex + 1, line.length());

    if (version.equals("HTTP/1.0")) {
        requestData.setVersion(HttpVersion::Http1_0);
        keepAlive = false;
    } else if (version.equals("HTTP/1.1")) {
        requestData.setVersion(HttpVersion::Http1_1);
        keepAlive = true;
    } else {
        if (version.startsWith("HTTP/")) {
            return Status(505, "HTTP Version Not Supported");
        } else {
            return Status(400, "Bad Request");
        }
    }

    return Status();
}

HttpConnection::Status HttpConnection::parseHeaderLine(const StringRef line) {
    // Check for end of headers
    if (line.length() == 0) {
        state = HttpState::Body;
        return Status();
    }

    // Check pad length
    size_t padLength = 0;
    while (line.charAt(padLength) == ' ' ||
           line.charAt(padLength) == '\t') {
        padLength++;
    }

    // If padded, treat as extension of previous line
    if (padLength != 0) {
        requestData.appendToHeaderValue(line.substring(padLength));
        return Status();
    }

    // If not padded, try to split
    ptrdiff_t colonIndex = line.indexOf(':');
    if (colonIndex == -1) {
        return Status(400, "Bad Request");
    }

    // The spec only specifies leading whitespace on the value, but also
    // right trimming the header name.
    StringRef headerName = line.substring(0, colonIndex);
    while (headerName.endsWith(' ') ||
           headerName.endsWith('\t')) {
        headerName = headerName.substring(0, headerName.length() - 1);
    }

    StringRef headerValue = line.substring(colonIndex + 1, line.length());
    while (headerValue.startsWith(' ') ||
           headerValue.startsWith('\t')) {
        headerValue = headerValue.substring(1);
    }

    // There is the possibility of multiple headers with the same name.
    // In that case, we want to append it as a comma separated value to
    // the previous header.
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    /*
    for (size_t i = 0; i < requestData.getHeaderCount(); i++) {
        const String& existingHeaderName = requestData.getHeaderName(i);
        if (headerName.engEqualsIgnoreCase(existingHeaderName)) {
            String& existingValue = headerValues[i];
            existingValue.appendChar(',');
            existingValue.append(headerValue);
            return Status();
        }
    }
    */

    requestData.addHeaderName(headerName);
    requestData.addHeaderValue(headerValue);
    return Status();
}

// TODO: Store index of these headers so we don't need to find them?
HttpConnection::Status HttpConnection::parseSpecialHeaders() {
    for (size_t i = 0; i < requestData.getHeaderCount(); i++) {
        const StringRef headerName = requestData.getHeaderName(i);
        const StringRef headerValue = requestData.getHeaderValue(i);
        if (headerName.engEqualsIgnoreCase("Content-Length")) {
            bool validNumber;
            std::tie(contentLength, validNumber) = Strconv::parseUint64(headerValue);

            if (!validNumber) {
                return Status(400, "Bad Request");
            }
            hasContentLength = true;
        } else if (headerName.engEqualsIgnoreCase("Transfer-Encoding")) {
            // Chunked if value after last comma is "chunked"
            StringRef last = CommaListIterator(headerValue).getLast();
            isChunked = last.engEqualsIgnoreCase("chunked");

            // Chunked is only supported for HTTP1.1
            if (requestData.getVersion() == HttpVersion::Http1_0 && isChunked) {
                return Status(400, "Bad Request");
            }
        } else if (headerName.engEqualsIgnoreCase("Connection")) {
            // The Connection header is a comma separater list
            CommaListIterator connectionIter(headerValue);
            bool closeFound = false;
            bool keepAliveFound = false;
            StringRef connectionNext = connectionIter.next();
            while (connectionNext.length() != 0) {
                if (connectionNext.engEqualsIgnoreCase("close")) {
                    closeFound = true;
                    keepAlive = false;
                } else if (connectionNext.engEqualsIgnoreCase("keep-alive")) {
                    keepAliveFound = true;
                    keepAlive = true;
                }
                connectionNext = connectionIter.next();
            }

            if (closeFound && keepAliveFound) {
                return Status(400, "Bad Request");
            }
        } else if (headerName.engEqualsIgnoreCase("Host")) {
            if (hasHost) {
                return Status(400, "Bad Request");
            }
            hasHost = true;
        }
    }
    return Status();
}

HttpConnection::Status HttpConnection::checkAndFixupHeaders() {
    // If we get both a content length and a chunked, we should ignore the content length
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.4
    if (hasContentLength && isChunked) {
        hasContentLength = false;
        contentLength = 0;
    }

    // Http1.1 Requires clients to send a host field
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1.2
    if (requestData.getVersion() == HttpVersion::Http1_1 &&
        !hasHost) {
        return Status(400, "Bad Request");
    }

    return Status();
}

HttpError HttpConnection::sendStatus(uint32_t code, const StringRef reasonPhrase) {
    char codeBuffer[12];
    size_t codeBytes = Strconv::uint32ToStr(code, codeBuffer, sizeof(codeBuffer));
    codeBuffer[codeBytes] = ' ';

    INet::IoBuffer ioBufs[4];

    if (requestData.getVersion() == HttpVersion::Http1_0) {
        ioBufs[0].buffer = (char*)"HTTP/1.0 ";
    } else {
        ioBufs[0].buffer = (char*)"HTTP/1.1 ";
    }
    ioBufs[0].bufferLen = 9;
    ioBufs[1].buffer = codeBuffer;
    ioBufs[1].bufferLen = codeBytes + 1;
    ioBufs[2].buffer = (char*)reasonPhrase.data();
    ioBufs[2].bufferLen = (uint32_t)reasonPhrase.length();
    ioBufs[3].buffer = (char*)"\r\n\r\n";
    ioBufs[3].bufferLen = 4;

    return streamSource->writev(ioBufs, 3);
}
