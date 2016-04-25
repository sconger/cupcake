
#include "cupcake_priv/http/HttpConnection.h"

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
    state(HttpState::Headers)
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

        if (line.length() != 0) {
            break;
        }

        if (!parseHeaderLine(line)) {
            return HttpError::ClientError;
        }
    } while (true);

    HttpHandler handler;
    bool foundHandler;
    std::tie(handler, foundHandler) = this->handlerMap->getHandler(curUrl);

    if (!foundHandler) {
        return sendStatus(404, "Not Found");
    }

    return HttpError::Ok;
}

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
    } else if (version.equals("HTTP/1.1")) {
        curVersion = HttpVersion::Http1_1;
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
    while (headerName.startsWith(' ') ||
           headerName.startsWith('\t')) {
        headerValue = headerValue.substring(1);
    }

    curHeaderNames.emplace_back(headerName);
    curHeaderValues.emplace_back(headerValue);
    return true;
}

HttpError HttpConnection::sendStatus(uint32_t code, const StringRef reasonPhrase) {
    char codeBuffer[12];
    int codeBytes = ::snprintf(codeBuffer, sizeof(codeBuffer), "%u", code);
    codeBuffer[codeBytes] = ' ';

    // TODO: writev
    HttpError err;
    std::tie(std::ignore, err) = streamSource->write(codeBuffer, codeBytes+1);
    if (err != HttpError::Ok) {
        return err;
    }

    std::tie(std::ignore, err) = streamSource->write(reasonPhrase.data(), reasonPhrase.length());
    return err;
}
