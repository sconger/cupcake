
#include "cupcake_priv/http/HttpConnection.h"

#include "cupcake_priv/http/ChunkedReader.h"
#include "cupcake_priv/http/CommaListIterator.h"
#include "cupcake_priv/http/ContentLengthReader.h"
#include "cupcake_priv/http/HttpRequestImpl.h"
#include "cupcake_priv/http/HttpResponseImpl.h"
#include "cupcake_priv/http/NullReader.h"
#include "cupcake_priv/text/Strconv.h"

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

HttpConnection::HttpConnection(StreamSource* streamSource, const HandlerMap* handlerMap) :
    streamSource(streamSource),
    handlerMap(handlerMap),
    state(HttpState::Headers),
    keepAlive(false),
    hasContentLength(false),
    contentLength(0),
    isChunked(false)
{
    bufReader.init(streamSource, 2048); // TODO: Define read constant somewhere
}

HttpConnection::~HttpConnection() {
    // TODO
}

void HttpConnection::run() {
    HttpError err = innerRun();
    // TODO: log
}

HttpError HttpConnection::innerRun() {
    HttpError err;
    StringRef line;

    do {
        curHeaderNames.clear();
        curHeaderValues.clear();

        // Read the request line
        std::tie(line, err) = bufReader.readLine(64 * 1024); // TODO: Define limit somewhere
        if (err != HttpError::Ok) {
            return err;
        }

        if (!parseRequestLine(line)) {
            return HttpError::ClientError;
        }

        // Read the headers
        do {
            std::tie(line, err) = bufReader.readLine(1 * 1024 * 1024); // TODO: Define limit somewhere
            if (err != HttpError::Ok) {
                return err;
            }

            // Switches to body state on empty line
            if (!parseHeaderLine(line)) {
                return HttpError::ClientError;
            }
        } while (state == HttpState::Headers);

        // Go through the headers so far and parse out special ones we need to pay attention to
        bool headersOkay = parseSpecialHeaders();

        if (!headersOkay) {
            return HttpError::ClientError;
        }

        // Deal with contradictory headers
        headerFixup();

        HttpHandler handler;
        bool foundHandler;
        std::tie(handler, foundHandler) = handlerMap->getHandler(curUrl);

        if (!foundHandler) {
            return sendStatus(404, "Not Found");
        }

        // Hack at the moment to get StringRef header values. Implement pool allocater later.
        size_t headerCount = curHeaderNames.size();
        std::vector<StringRef> headerNamesStringRef;
        std::vector<StringRef> headerValuesStringRef;
        headerNamesStringRef.reserve(headerCount);
        headerValuesStringRef.reserve(headerCount);

        for (size_t i = 0; i < headerCount; i++) {
            headerNamesStringRef.push_back(curHeaderNames.at(i));
            headerValuesStringRef.push_back(curHeaderValues.at(i));
        }

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

        HttpRequestImpl requestImpl(curMethod,
            curUrl,
            headerNamesStringRef,
            headerValuesStringRef,
            *inputStream);

        HttpResponseImpl responseImpl(curVersion,
            streamSource);

        handler(requestImpl, responseImpl);

        // TODO: This blank line should really be added at the top of the loop if not the first request
        err = responseImpl.addBlankLineIfNeeded();
        if (err != HttpError::Ok) {
            return err;
        }

    } while (keepAlive);

    return streamSource->close();
}

// TODO: Pass errors back
bool HttpConnection::parseRequestLine(const StringRef line) {
    // Assuming exactly two spaces at the moment
    ptrdiff_t firstSpaceIndex = line.indexOf(' ');
    if (firstSpaceIndex == -1 || firstSpaceIndex == line.length() - 1) {
        sendStatus(400, "Bad Request");
        return false;
    }

    ptrdiff_t secondSpaceIndex = line.indexOf(' ', firstSpaceIndex + 1);
    if (secondSpaceIndex == -1) {
        sendStatus(400, "Bad Request");
        return false;
    }

    // Extract method
    StringRef methodStr = line.substring(0, firstSpaceIndex);
    auto methodLookup = methodLookupMap.find(methodStr);
    if (methodLookup == methodLookupMap.end()) {
        sendStatus(405, "Method Not Allowed");
        return false;
    }

    curMethod = methodLookup->second;

    // Extract URL
    StringRef url = line.substring(firstSpaceIndex + 1, secondSpaceIndex);

    if (url.length() == 0) {
        sendStatus(400, "Bad Request");
        return false;
    }

    curUrl = url;

    // Extract the version
    StringRef version = line.substring(secondSpaceIndex + 1, line.length());

    if (version.equals("HTTP/1.0")) {
        curVersion = HttpVersion::Http1_0;
        keepAlive = false;
    } else if (version.equals("HTTP/1.1")) {
        curVersion = HttpVersion::Http1_1;
        keepAlive = true;
    } else {
        if (version.startsWith("HTTP/")) {
            sendStatus(505, "HTTP Version Not Supported");
        } else {
            sendStatus(400, "Bad Request");
        }
        return false;
    }

    return true;
}

// TODO: Pass errors back
bool HttpConnection::parseHeaderLine(const StringRef line) {
    // Check for end of headers
    if (line.length() == 0) {
        state = HttpState::Body;
        return true;
    }

    // Check pad length
    size_t padLength = 0;
    while (line.charAt(padLength) == ' ' ||
           line.charAt(padLength) == '\t') {
        padLength++;
    }

    // If padded, treat as extension of previous line
    if (padLength != 0) {
        curHeaderValues.back().append(line.substring(padLength));
        return true;
    }

    // If not padded, try to split
    ptrdiff_t colonIndex = line.indexOf(':');
    if (colonIndex == -1) {
        sendStatus(400, "Bad Request");
        return false;
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
    for (size_t i = 0; i < curHeaderNames.size(); i++) {
        const String& existingHeaderName = curHeaderNames[i];
        if (headerName.engEqualsIgnoreCase(existingHeaderName)) {
            String& existingValue = curHeaderValues[i];
            existingValue.appendChar(',');
            existingValue.append(headerValue);
            return true;
        }
    }

    curHeaderNames.emplace_back(headerName);
    curHeaderValues.emplace_back(headerValue);
    return true;
}

// TODO: Store index of these headers so we don't need to find them?
bool HttpConnection::parseSpecialHeaders() {
    for (size_t i = 0; i < curHeaderNames.size(); i++) {
        const String& headerName = curHeaderNames[i];
        if (headerName.engEqualsIgnoreCase("Content-Length")) {
            bool validNumber;
            std::tie(contentLength, validNumber) = Strconv::parseUint64(curHeaderValues[i]);

            if (!validNumber) {
                sendStatus(400, "Invalid Content-Length header");
                return false;
            }
            hasContentLength = true;
        } else if (headerName.engEqualsIgnoreCase("Transfer-Encoding")) {
            // Chunked if value after last comma is "chunked"
            StringRef transferEncoding = curHeaderValues[i];
            StringRef last = CommaListIterator(transferEncoding).getLast();
            isChunked = last.engEqualsIgnoreCase("chunked");
        } else if (headerName.engEqualsIgnoreCase("Connection")) {
            // The Connection header is a comma separater list
            CommaListIterator connectionIter(curHeaderValues[i]);
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
                sendStatus(400, "Invalid Connection header");
                return false;
            }
        }
    }
    return true;
}

void HttpConnection::headerFixup() {
    // If we get both a content length and a chunked, we should ignore the content length
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.4
    if (hasContentLength && isChunked) {
        hasContentLength = false;
        contentLength = 0;
    }
}

HttpError HttpConnection::sendStatus(uint32_t code, const StringRef reasonPhrase) {
    char codeBuffer[12];
    size_t codeBytes = Strconv::uint32ToStr(code, codeBuffer, sizeof(codeBuffer));
    codeBuffer[codeBytes] = ' ';

    INet::IoBuffer ioBufs[3];
    ioBufs[0].buffer = codeBuffer;
    ioBufs[0].bufferLen = codeBytes + 1;
    ioBufs[1].buffer = (char*)reasonPhrase.data();
    ioBufs[1].bufferLen = (uint32_t)reasonPhrase.length();
    ioBufs[2].buffer = "\r\n\r\n";
    ioBufs[2].bufferLen = 4;

    return streamSource->writev(ioBufs, 3);
}
