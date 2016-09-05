
#include "cupcake_priv/http/BufferedReader.h"

#include <algorithm>
#include <cassert>

using namespace Cupcake;

BufferedReader::BufferedReader() :
    socket(nullptr),
    buffer(),
    bufferLen(0),
    startIndex(0),
    endIndex(0)
{}

void BufferedReader::init(StreamSource* readSocket, size_t initialBufferSize) {
    socket = readSocket;
    bufferLen = initialBufferSize;
    buffer.reset(new char[initialBufferSize]);
}

std::tuple<uint32_t, HttpError> BufferedReader::read(char* destBuffer, uint32_t destBufferLen) {
    size_t available = endIndex - startIndex;

    // If there is available data, just move it into the dest buffer
    if (available != 0) {
        uint32_t copyLen = std::min((uint32_t)available, destBufferLen);
        std::memcpy(destBuffer, buffer.get() + startIndex, copyLen);
        startIndex += copyLen;
        return std::make_tuple(copyLen, HttpError::Ok);
    }

    startIndex = 0;
    endIndex = 0;

    // Do a vectored read into the destination buffer, and then the internal buffer.
    // This should minimize system calls while working reasonable well for both small
    // and large destination buffers.
    INet::IoBuffer ioBufs[2];
    ioBufs[0].buffer = destBuffer;
    ioBufs[0].bufferLen = destBufferLen;
    ioBufs[1].buffer = buffer.get();
    ioBufs[1].bufferLen = bufferLen;

    uint32_t bytesCopied = 0;
    HttpError err;
    std::tie(bytesCopied, err) = socket->readv(ioBufs, 2);

    if (err != HttpError::Ok) {
        return std::make_tuple(0, err);
    }

    if (bytesCopied > destBufferLen) {
        endIndex = bytesCopied - destBufferLen;
        return std::make_tuple(destBufferLen, HttpError::Ok);
    }
    return std::make_tuple(bytesCopied, HttpError::Ok);
}

std::tuple<StringRef, HttpError> BufferedReader::readLine(uint32_t maxLength) {
    bool foundLineEnd = false;
    uint32_t searchIndex = startIndex;

    do {
        uint32_t lineEndIndex;
        uint32_t newStartIndex;

        // Look for a newline in existing data
        for (uint32_t i = searchIndex; !foundLineEnd && i < endIndex; i++) {
            if (buffer[i] != '\n') {
                continue;
            }

            if (i != 0 && buffer[i - 1] == '\r') {
                lineEndIndex = i - 1;
            } else {
                lineEndIndex = i;
            }
            newStartIndex = i + 1;
            foundLineEnd = true;
        }

        if (foundLineEnd) {
            uint32_t oldStartIndex = startIndex;
            startIndex = newStartIndex;
            return std::make_tuple(StringRef(buffer.get()+oldStartIndex, lineEndIndex - oldStartIndex), HttpError::Ok);
        }

        searchIndex = endIndex;

        // Discard old data still in the buffer, or increase buffer size if needed
        if (startIndex != 0) {
            uint32_t available = endIndex - startIndex;
            std::memmove(buffer.get(), buffer.get() + startIndex, available);
            searchIndex -= startIndex;
            startIndex = 0;
            endIndex = available;
        } else if (endIndex == bufferLen) {
            if (bufferLen >= maxLength) {
                return std::make_tuple(StringRef(), HttpError::LineTooLong);
            }

            // TODO: Overflow check
            uint32_t newBufferLen = std::max(bufferLen * 2, maxLength + 2); // +2 to allow for \r\n
            std::unique_ptr<char[]> newPtr(new char[newBufferLen]);
            std::memcpy(newPtr.get(), buffer.get(), bufferLen);
            buffer.swap(newPtr);
            bufferLen = newBufferLen;
        }

        // Read some data
        uint32_t bytesRead;
        HttpError err;
        std::tie(bytesRead, err) = socket->read(buffer.get() + endIndex, bufferLen - endIndex);

        if (err == HttpError::Eof) {
            // If we have no data available, just return the error back
            if (startIndex == endIndex) {
                return std::make_tuple(StringRef(), HttpError::Eof);
            }

            // But if we do have something, consider that a line and return it
            uint32_t oldStartIndex = startIndex;
            uint32_t oldEndIndex = endIndex;
            startIndex = 0;
            endIndex = 0;
            return std::make_tuple(StringRef(buffer.get() + oldStartIndex, oldEndIndex - oldStartIndex), HttpError::Ok);
        } else if (err != HttpError::Ok) {
            return std::make_tuple(StringRef(), err);
        }

        endIndex += bytesRead;

    } while (true);
}

HttpError BufferedReader::readFixedLength(char* buffer, uint32_t byteCount) {
    // TODO

    return HttpError::Ok;
}
