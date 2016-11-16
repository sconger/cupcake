
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

constexpr uint8_t ACK = 1;

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

    // Read and validate the preface that should be sent along
    err = checkPreface();
    if (err != HttpError::Ok) {
        return err;
    }

    // Send a preface and settings back
    err = sendPrefaceAndSettings();
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

uint32_t Http2Connection::getFrameLength() {
    return ((frameHeader[0] << 16) &
            (frameHeader[1] << 8) &
            (frameHeader[2]));
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