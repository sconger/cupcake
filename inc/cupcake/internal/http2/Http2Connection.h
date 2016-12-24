
#ifndef CUPCAKE_HTTP_CONNECTION_H
#define CUPCAKE_HTTP_CONNECTION_H

#include "cupcake/internal/text/String.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/http/Http.h"
#include "cupcake/internal/http/BufferedReader.h"
#include "cupcake/internal/http/HandlerMap.h"
#include "cupcake/internal/http/StreamSource.h"

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>
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

    class DataBuf {
    public:
        DataBuf();

        char* getPtr();
        const char* getPtr() const;

    private:
        std::unique_ptr<char[]> ptr;
    };

    class Stream {
    public:
        Stream();

        void addHeader(const StringRef name, const StringRef value);
        void setPriority(uint32_t streamId, uint8_t weight);

    private:
        std::vector<String> headerNames;
        std::vector<String> headerValues;
        uint32_t priorityParent;
        uint8_t priorityWeight;
    };

    HttpError innerRun();
    HttpError checkPreface();
    HttpError readFrameHeader();
    Frame getFrameType();
    uint8_t getFrameFlags();
    uint32_t getFrameLength();
    uint32_t getFrameStreamId();

    HttpError handleDataFrame();
    HttpError handleHeadersFrame();
    HttpError handlePriorityFrame();
    HttpError handleRstFrame();
    HttpError handleSettingsFrame(bool allowAck);
    HttpError handlePushPromiseFrame();
    HttpError handlePingFrame();
    HttpError handleGoAwayFrame();
    HttpError handleWindowUpdateFrame();
    HttpError handleContinuationFrame();
    HttpError readHpack(uint32_t length);

    static uint32_t read3Byte(const char* data);
    static uint32_t read4Byte(const char* data);

    void writeTask();
    HttpError writeLoop();

    HttpError sendPrefaceAndSettings();
    HttpError sendSettingsAck();

    // Shared stream reference
    StreamSource* streamSource;

    // Data for the reader
    const HandlerMap* handlerMap;
    BufferedReader& bufReader;
    char frameHeader[9];
    std::unordered_map<uint32_t, Stream> streams;

    // Data requiring locking
    std::mutex mutex;
    std::condition_variable cond;
    std::deque<DataBuf> buffers;
};

}

#endif // CUPCAKE_HTTP2_CONNECTION_H
