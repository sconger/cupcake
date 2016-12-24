
#ifndef CUPCAKE_HTTP_CONNECTION_H
#define CUPCAKE_HTTP_CONNECTION_H

#include "cupcake/internal/text/String.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/http/Http.h"
#include "cupcake/internal/http/BufferedReader.h"
#include "cupcake/internal/http/HandlerMap.h"
#include "cupcake/internal/http/RequestData.h"
#include "cupcake/internal/http/StreamSource.h"

#include <tuple>
#include <vector>

namespace Cupcake {

/*
 * Wrapper around a connection that will be treated as HTTP traffic.
 */
class HttpConnection {
public:
    enum class UpgradeType {
        None,
        H2C_Upgrade
    };
public:
    HttpConnection(StreamSource* streamSource, BufferedReader& bufReader, const HandlerMap* handlerMap);
    ~HttpConnection();

    UpgradeType run();

private:
    HttpConnection(const HttpConnection&) = delete;
    HttpConnection& operator=(const HttpConnection&) = delete;

    enum class HttpState;

    // Helper for passing back error state
    class Status {
    public:
        uint32_t code;
        const char* reasonPhrase;

        constexpr Status() : code(0), reasonPhrase(nullptr) {}
        constexpr Status(uint32_t status, const char* statusMessage) :
            code(status), reasonPhrase(statusMessage) {}
        bool ok() {return code == 0;}
    };

    std::tuple<UpgradeType, HttpError> innerRun();

    std::tuple<bool, HttpError> checkPreface();
    Status parseRequestLine(const StringRef line);
    Status parseHeaderLine(const StringRef line);
    Status parseSpecialHeaders();
    Status checkAndFixupHeaders();
    HttpError sendStatus(uint32_t code, const StringRef reasonPhrase);

    const HandlerMap* handlerMap;
    BufferedReader& bufReader;
    StreamSource* streamSource;
    HttpState state;

    RequestData requestData;
    bool keepAlive;
    bool hasContentLength;
    uint64_t contentLength;
    bool isChunked;
    bool hasHost;
};

}

#endif // CUPCAKE_HTTP_CONNECTION_H
