
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

std::tuple<uint32_t, HttpError> BufferedReader::read(char* destBuffer, uint32_t bufferLen) {
    uint32_t bytesCopied = 0;
    size_t available = endIndex - startIndex;
    if (available != 0) {
        if (bufferLen < available) {
            std::memcpy(destBuffer, buffer.get(), bufferLen);
            startIndex += bufferLen;
            return std::make_tuple(bufferLen, HttpError::Ok);
        } else {
            std::memcpy(destBuffer, buffer.get(), available);
            startIndex = 0;
            endIndex = 0;
            destBuffer += available;
            bufferLen -= available;
        }
    }

    // Read directly to the dest buffer once the existing buffering is gone
    uint32_t bytesRead;
    HttpError err;
    std::tie(bytesRead, err) = socket->read(destBuffer, bufferLen);

    return std::make_tuple(bytesRead+bytesCopied, err);
}

std::tuple<StringRef, HttpError> BufferedReader::readLine(size_t maxLength) {
    bool foundLineEnd = false;
    size_t searchIndex = startIndex;

    do {
        size_t lineEndIndex;
        size_t newStartIndex;

        // Look for a newline in existing data
        for (size_t i = searchIndex; !foundLineEnd && i < endIndex; i++) {
            if (buffer[i] == '\n') {
                if (i != endIndex - 1 && buffer[i - 1] == '\r') {
                    lineEndIndex = i - 1;
                } else {
                    lineEndIndex = i;
                }
                newStartIndex = i;
                foundLineEnd = true;
            }
        }

        if (foundLineEnd) {
            size_t oldStartIndex = startIndex;
            startIndex = newStartIndex;
            return std::make_tuple(StringRef(buffer.get()+oldStartIndex, lineEndIndex), HttpError::Ok);
        }

        // Increase buffer size if needed
        if (searchIndex == bufferLen) {
            if (bufferLen == maxLength) {
                return std::make_tuple(StringRef(), HttpError::LineTooLong);
            }

            size_t newBufferLen = std::max(bufferLen * 2, maxLength+2); // +2 to allow for \r\n
            std::unique_ptr<char[]> newPtr(new char[newBufferLen]);
            std::memcpy(newPtr.get(), buffer.get(), bufferLen);
            buffer.swap(newPtr);
            bufferLen = newBufferLen;
        }

        // Read some data
        uint32_t bytesRead;
        HttpError err;
        std::tie(bytesRead, err) = socket->read(buffer.get() + searchIndex, bufferLen - searchIndex);

        if (err != HttpError::Ok) {
            return std::make_tuple(StringRef(), err);
        }

        endIndex += (size_t)bytesRead;

    } while (true);
}

HttpError BufferedReader::readAtLeast(size_t byteCount) {
    // TODO

    return HttpError::Ok;
}

const char* BufferedReader::data() const {
    return buffer.get();
}

