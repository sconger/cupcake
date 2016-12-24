
#include "cupcake/internal/http2/Http2Connection.h"

#include "cupcake/internal/async/Async.h"

using namespace Cupcake;

// Frame types fro HTTP2 spec
enum Http2Connection::Frame : uint8_t  {
    DATA = 0,
    HEADERS = 1,
    PRIORITY = 2,
    RST_STREAM = 3,
    SETTINGS = 4,
    PUSH_PROMISE = 5,
    PING = 6,
    GOAWAY = 7,
    WINDOW_UPDATE = 8,
    CONTINUATION = 9,
};

constexpr uint8_t ACK = 0x1;

constexpr uint8_t END_STREAM = 0x1;
constexpr uint8_t END_HEADERS = 0x4;
constexpr uint8_t PADDED = 0x8;
constexpr uint8_t PRIORITY = 0x20;

constexpr size_t DATA_BUF_LEN = 4064;

Http2Connection::DataBuf::DataBuf() :
    ptr(new char[DATA_BUF_LEN])
{}

char* Http2Connection::DataBuf::getPtr() {
    return ptr.get();
}

const char* Http2Connection::DataBuf::getPtr() const {
    return ptr.get();
}


Http2Connection::Http2Connection(StreamSource* streamSource,
    BufferedReader& bufReader,
    const HandlerMap* handlerMap,
    bool skipPreface) :
    streamSource(streamSource),
    bufReader(bufReader),
    handlerMap(handlerMap)
{}

Http2Connection::~Http2Connection() {

}

void Http2Connection::run() {
    HttpError err = innerRun();
    // TODO: log
    streamSource->close();
}

HttpError Http2Connection::innerRun() {
    HttpError err;

    // TODO: Replace with a call to write and just use the callback
    Async::runAsync([this] {
        this->writeTask();
    });

    // Read and validate the preface that should be sent along
    err = checkPreface();
    if (err != HttpError::Ok) {
        return err;
    }

    // Read the required settings frame
    err = readFrameHeader();
    if (err != HttpError::Ok) {
        return err;
    }
    if (getFrameType() != Frame::SETTINGS) {
        return HttpError::ClientError;
    }
    err = handleSettingsFrame(false);
    if (err != HttpError::Ok) {
        return err;
    }

    // And then loop handling normal messages
    while (true) {
        err = readFrameHeader();
        if (err != HttpError::Ok) {
            return err;
        }

        uint8_t frameType = (uint8_t)frameHeader[3];
        switch (frameType) {
        case DATA:
            handleDataFrame();
            break;
        case HEADERS:
            handleHeadersFrame();
            break;
        case PRIORITY:
            handlePriorityFrame();
            break;
        case RST_STREAM:
            handleRstFrame();
            break;
        case SETTINGS:
            handleSettingsFrame(true);
            break;
        case PUSH_PROMISE:
            handlePushPromiseFrame();
            break;
        case PING:
            handlePingFrame();
            break;
        case GOAWAY:
            handleGoAwayFrame();
            break;
        case WINDOW_UPDATE:
            handleWindowUpdateFrame();
            break;
        case CONTINUATION:
            handleContinuationFrame();
            break;
        }
    }

    return HttpError::Ok;
}

HttpError Http2Connection::checkPreface() {
    char prefaceBuffer[24];

    HttpError err = bufReader.readFixedLength(prefaceBuffer, 24);
    if (err != HttpError::Ok) {
        return err;
    }
    if (std::memcmp(prefaceBuffer, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) != 0) {
        return HttpError::ClientError;
    }
    return HttpError::Ok;
}

HttpError Http2Connection::readFrameHeader() {
    HttpError err = bufReader.readFixedLength(frameHeader, sizeof(frameHeader));
    if (err != HttpError::Ok) {
        return err;
    }
    // We don't support any extension so reject unknown frame types
    uint8_t frameType = (uint8_t)frameHeader[3];
    if (frameType > 9) {
        return HttpError::ClientError;
    }
    return HttpError::Ok;
}

Http2Connection::Frame Http2Connection::getFrameType() {
    return (Frame)frameHeader[3];
}

uint8_t Http2Connection::getFrameFlags() {
    return (uint8_t)frameHeader[4];
}

uint32_t Http2Connection::getFrameLength() {
    return read3Byte(&frameHeader[0]);
}

uint32_t Http2Connection::getFrameStreamId() {
    uint32_t idAndReserved = read4Byte(&frameHeader[5]);
    return idAndReserved ^ 0x80000000; // Need to ignore reserved bit
}

HttpError Http2Connection::handleDataFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handleHeadersFrame() {
    uint32_t streamId = getFrameStreamId();

    if (streamId == 0 ||
        streamId % 2 != 0) {
        return HttpError::ClientError;
    }

    uint32_t frameLength = getFrameLength();
    uint8_t flags = getFrameFlags();

    uint8_t padding = 0;
    uint32_t dependency = 0;
    bool exclusive = false;
    uint8_t weight = 0;

    bool endStream = flags & END_STREAM;
    bool endHeaders = flags & END_HEADERS;
    bool padded = flags & PADDED;
    bool priority = flags & PRIORITY;

    if (padded) {
        HttpError err = bufReader.readFixedLength((char*)&padding, 1);
        if (err != HttpError::Ok) {
            return err;
        }
    }
    if (priority) {
        char priorityData[5];
        HttpError err = bufReader.readFixedLength(priorityData, sizeof(priorityData));
        if (err != HttpError::Ok) {
            return err;
        }
        weight = (uint8_t)priorityData[0];

        uint32_t dependencyAndExclusive = read4Byte(&priorityData[1]);
        dependency = dependencyAndExclusive ^ 0x80000000;
        exclusive = !!(dependencyAndExclusive & 0x80000000);
    }

    HttpError err = readHpack(frameLength);
    if (err != HttpError::Ok) {
        return err;
    }

    if (padding != 0) {
        err = bufReader.discard(padding);
        if (err != HttpError::Ok) {
            return err;
        }
    }

    return HttpError::Ok;
}

HttpError Http2Connection::handlePriorityFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handleRstFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handleSettingsFrame(bool allowAck) {
    uint32_t length = getFrameLength();
    // Should be a list of 48 bit values
    if (length % 6 != 0) {
        return HttpError::ClientError;
    }

    uint8_t flags = (uint8_t)frameHeader[4];
    if (allowAck) {
        if (flags != 0 &&
            flags != ACK) {
            return HttpError::ClientError;
        }
    } else {
        if (flags != 0) {
            return HttpError::ClientError;
        }
    }
    
    if (frameHeader[5] != 0 ||
        frameHeader[6] != 0 ||
        frameHeader[7] != 0 ||
        frameHeader[8] != 0) {
        return HttpError::ClientError;
    }
    return HttpError::Ok;
}

HttpError Http2Connection::handlePushPromiseFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handlePingFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handleGoAwayFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handleWindowUpdateFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::handleContinuationFrame() {
    return HttpError::Ok;
}

HttpError Http2Connection::readHpack(uint32_t length) {
    return HttpError::Ok;
}

uint32_t Http2Connection::read3Byte(const char* data) {
    return (data[0] << 16) |
        (data[1] << 8) |
        (data[2]);
}

uint32_t Http2Connection::read4Byte(const char* data) {
    return (data[0] << 24) |
        (data[1] << 16) |
        (data[2] << 8) |
        (data[3]);
}

void Http2Connection::writeTask() {
    HttpError err = writeLoop();
    // TODO: Log
}

HttpError Http2Connection::writeLoop() {
    // Send a preface and settings
    HttpError err = sendPrefaceAndSettings();
    if (err != HttpError::Ok) {
        return err;
    }

    // TODO
    return HttpError::Ok;
}

HttpError Http2Connection::sendPrefaceAndSettings() {
    constexpr char settingsFrame[9] = {0, 0, 0, Frame::SETTINGS, 0, 0, 0, 0, 0};

    INet::IoBuffer buffers[2];
    buffers[0].buffer = (char*)"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    buffers[0].bufferLen = 24;
    buffers[1].buffer = (char*)settingsFrame;
    buffers[1].bufferLen = sizeof(settingsFrame);

    return streamSource->writev(buffers, 2);
}

HttpError Http2Connection::sendSettingsAck() {
    constexpr char settingsFrame[9] = {0, 0, 0, Frame::SETTINGS, ACK, 0, 0, 0, 0};
    return streamSource->write(settingsFrame, sizeof(settingsFrame));
}
