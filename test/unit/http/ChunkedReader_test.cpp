
#include "unit/http/ChunkedReader_test.h"

#include "unit/UnitTest.h"

#include "cupcake/text/String.h"
#include "cupcake_priv/http/ChunkedReader.h"

#include <algorithm>

using namespace Cupcake;

class ReadTestSource : public StreamSource {
public:
    ReadTestSource(const char* sourceData, size_t dataLen) :
        sourceData(sourceData),
        dataLen(dataLen),
        readCount(0) {}

    std::tuple<StreamSource*, HttpError> accept() override {
        return std::make_tuple(nullptr, HttpError::Ok);
    }
    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) override {
        if (dataLen == 0) {
            return std::make_tuple(0, HttpError::Eof);
        }

        readCount++;

        uint32_t copyLen = std::min((uint32_t)dataLen, bufferLen);
        std::memcpy(buffer, sourceData, copyLen);
        sourceData += copyLen;
        dataLen -= copyLen;
        return std::make_tuple(copyLen, HttpError::Ok);
    }
    std::tuple<uint32_t, HttpError> readv(INet::IoBuffer* buffers, uint32_t bufferCount) override {
        if (dataLen == 0) {
            return std::make_tuple(0, HttpError::Eof);
        }

        readCount++;

        uint32_t bytesCopied = 0;
        for (uint32_t i = 0; i < bufferCount && dataLen > 0; i++) {
            uint32_t copyLen = std::min((uint32_t)dataLen, buffers[i].bufferLen);
            std::memcpy(buffers[i].buffer, sourceData, copyLen);
            sourceData += copyLen;
            dataLen -= copyLen;
            bytesCopied += copyLen;
        }
        return std::make_tuple(bytesCopied, HttpError::Ok);
    }
    std::tuple<uint32_t, HttpError> write(const char* buffer, uint32_t bufferLen) override {
        return std::make_tuple(0, HttpError::Ok);
    }
    std::tuple<uint32_t, HttpError> writev(const INet::IoBuffer* buffers, uint32_t bufferCount) override {
        return std::make_tuple(0, HttpError::Ok);
    }
    HttpError close() override {
        return HttpError::Ok;
    }

    uint32_t readCount;

private:
    const char* sourceData;
    size_t dataLen;
};

// Basic happy case test
bool test_chunkedreader_basic() {
    const String inputData =
        "5\r\n"
        "abcde\r\n"
        "3\r\n"
        "fgh\r\n"
        "1\r\n"
        "i\r\n"
        "0\r\n"
        "\r\n";

    ReadTestSource testSource(inputData.data(), inputData.length());
    BufferedReader bufferedReader;
    bufferedReader.init(&testSource, 1024);
    ChunkedReader reader(bufferedReader);

    StringRef expected = "abcdefghi";
    char readBuffer[1024];
    size_t readIndex = 0;
    HttpError err;
    uint32_t bytesRead;

    while (true) {
        // Byte at a time to test state machine
        std::tie(bytesRead, err) = reader.read(readBuffer + readIndex, 1);

        if (err == HttpError::Ok) {
            readIndex += bytesRead;
            if (readIndex > expected.length()) {
                testf("Read more than expected data");
                return false;
            }
        } else if (err == HttpError::Eof) {
            break;
        } else {
            testf("Read failed with: %d", err);
            return false;
        }
    }

    if (readIndex != expected.length() ||
        std::memcmp(readBuffer, expected.data(), expected.length())) {
        testf("Did not read back expected data");
        return false;
    }

    std::tie(bytesRead, err) = reader.read(readBuffer, sizeof(readBuffer));

    if (err != HttpError::Eof) {
        testf("Did not see expected EOF error at end of stream");
        return false;
    }
    if (bytesRead != 0) {
        testf("Read bytes on EOF read");
        return false;
    }

    return true;
}

// Tests reading an empty stream
bool test_chunkedreader_empty() {
    const String inputData =
        "0\r\n"
        "\r\n";

    ReadTestSource testSource(inputData.data(), inputData.length());
    BufferedReader bufferedReader;
    bufferedReader.init(&testSource, 1024);
    ChunkedReader reader(bufferedReader);

    char readBuffer[1024];
    HttpError err;
    uint32_t bytesRead;

    std::tie(bytesRead, err) = reader.read(readBuffer, sizeof(readBuffer));

    if (err != HttpError::Eof) {
        testf("Did not see expected EOF");
        return false;
    }
    if (bytesRead != 0) {
        testf("Read bytes on EOF read");
        return false;
    }

    return true;
}

// This tests reading a piece of chunked data that does not immediately end in \r\n.
// Should be considered a client error
bool test_chunkedreader_bad_data_line() {
    const String inputData =
        "5\r\n"
        "abcde(extra)\r\n"
        "0\r\n"
        "\r\n";

    ReadTestSource testSource(inputData.data(), inputData.length());
    BufferedReader bufferedReader;
    bufferedReader.init(&testSource, 1024);
    ChunkedReader reader(bufferedReader);

    char readBuffer[1024];
    size_t readIndex = 0;
    HttpError err;
    uint32_t bytesRead;

    std::tie(bytesRead, err) = reader.read(readBuffer, sizeof(readBuffer));

    if (err != HttpError::ClientError) {
        testf("Did not see expected client error error code");
        return false;
    }
    if (bytesRead != 0) {
        testf("Read bytes on errored read");
        return false;
    }

    return true;
}

// Tests that chunked extensions are ignored
bool test_chunkedreader_extension() {
    const String inputData =
        "5;name=value\r\n"
        "abcde\r\n"
        "3\r\n"
        "fgh\r\n"
        "1\r\n"
        "i\r\n"
        "0;name=value\r\n"
        "\r\n";

    ReadTestSource testSource(inputData.data(), inputData.length());
    BufferedReader bufferedReader;
    bufferedReader.init(&testSource, 1024);
    ChunkedReader reader(bufferedReader);

    StringRef expected = "abcdefghi";
    char readBuffer[1024];
    size_t readIndex = 0;
    HttpError err;
    uint32_t bytesRead;

    while (true) {
        std::tie(bytesRead, err) = reader.read(readBuffer + readIndex, sizeof(readBuffer) - readIndex);

        if (err == HttpError::Ok) {
            readIndex += bytesRead;
            if (readIndex > expected.length()) {
                testf("Read more than expected data");
                return false;
            }
        } else if (err == HttpError::Eof) {
            break;
        } else {
            testf("Read failed with: %d", err);
            return false;
        }
    }

    if (readIndex != expected.length() ||
        std::memcmp(readBuffer, expected.data(), expected.length())) {
        testf("Did not read back expected data");
        return false;
    }

    return true;
}

// Tests that trailing headers are ignored
bool test_chunkedreader_trailing_headers() {
    const String inputData =
        "a\r\n"
        "0123456789\r\n"
        "0\r\n"
        "Some-Header: headerval\r\n"
        "Another-Header: 11\r\n"
        "\r\n";

    ReadTestSource testSource(inputData.data(), inputData.length());
    BufferedReader bufferedReader;
    bufferedReader.init(&testSource, 1024);
    ChunkedReader reader(bufferedReader);

    StringRef expected = "0123456789";
    char readBuffer[1024];
    size_t readIndex = 0;
    HttpError err;
    uint32_t bytesRead;

    while (true) {
        std::tie(bytesRead, err) = reader.read(readBuffer + readIndex, sizeof(readBuffer) - readIndex);

        if (err == HttpError::Ok) {
            readIndex += bytesRead;
            if (readIndex > expected.length()) {
                testf("Read more than expected data");
                return false;
            }
        } else if (err == HttpError::Eof) {
            break;
        } else {
            testf("Read failed with: %d", err);
            return false;
        }
    }

    if (readIndex != expected.length() ||
        std::memcmp(readBuffer, expected.data(), expected.length())) {
        testf("Did not read back expected data");
        return false;
    }

    return true;
}
