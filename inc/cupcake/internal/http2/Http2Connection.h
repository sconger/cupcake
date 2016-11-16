
#ifndef CUPCAKE_HTTP_CONNECTION_H
#define CUPCAKE_HTTP_CONNECTION_H

#include "cupcake/internal/text/String.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/http/Http.h"
#include "cupcake/internal/http/BufferedReader.h"
#include "cupcake/internal/http/HandlerMap.h"
#include "cupcake/internal/http/StreamSource.h"

#include <tuple>
#include <vector>

namespace Cupcake {

/*
 * Wrapper around a connection that will be treated as HTTP2 traffic.
 */
class Http2Connection {
public:
    Http2Connection(StreamSource* streamSource,
        BufferedReader& bufReader,
        const HandlerMap* handlerMap,
        bool skipPreface);
    ~Http2Connection();

    void run();

private:
    Http2Connection(const Http2Connection&) = delete;
    Http2Connection& operator=(const Http2Connection&) = delete;

    enum Frame : uint8_t;

    HttpError innerRun();
    HttpError checkPreface();
    HttpError readFrameHeader();
    Frame getFrameType();
    uint32_t getFrameLength();
    HttpError handleSettingsFrame(bool allowAck);
    HttpError sendPrefaceAndSettings();
    HttpError sendSettingsAck();

    const HandlerMap* handlerMap;
    BufferedReader& bufReader;
    StreamSource* streamSource;
    char frameHeader[9];
};

}

#endif // CUPCAKE_HTTP2_CONNECTION_H
