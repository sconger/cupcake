
#include "unit/http/BufferedReader_test.h"
#include "unit/UnitTest.h"

#include "cupcake_priv/http/BufferedReader.h"
#include "cupcake_priv/http/StreamSource.h"
#include "cupcake/text/String.h"

#include <algorithm>
#include <vector>

using namespace Cupcake;

class TestSource : public StreamSource {
public:
    TestSource(const char* sourceData, size_t dataLen) :
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

bool test_bufferedreader_basic() {
    TestSource testSource("Hello World", 11);
    BufferedReader bufReader;
    bufReader.init(&testSource, 100);

    char destBytes[20];
    size_t destIndex = 0;

    while (1) {
        uint32_t bytesRead;
        HttpError err;
        std::tie(bytesRead, err) = bufReader.read(destBytes + destIndex, 1);

        if (err == HttpError::Eof) {
            break;
        } else if (err != HttpError::Ok) {
            testf("Unnexpected error reading from buffered reader");
            return false;
        }
        destIndex++;
        if (destIndex > 11) {
            testf("Read too many bytes");
            return false;
        }
    }

    if (destIndex != 11) {
        testf("Did not read expected number of bytes");
        return false;
    }

    if (std::memcmp("Hello World", destBytes, 11) != 0) {
        testf("Did not read back expected data");
        return false;
    }

    if (testSource.readCount != 1) {
        testf("More than one read to source stream occurred (did not buffer correctly)");
        return false;
    }

    return true;
}

bool test_bufferedreader_readline() {
    String testData = "line1\r\nline2\nline3\n\nline5";
    TestSource testSource(testData.c_str(), testData.length());
    BufferedReader bufReader;
    bufReader.init(&testSource, 4); // Intentionally small

    std::vector<String> lines;

    while (1) {
        StringRef line;
        HttpError err;
        std::tie(line, err) = bufReader.readLine(100);

        if (err == HttpError::Eof) {
            break;
        } else if (err != HttpError::Ok) {
            testf("Unnexpected error reading from buffered reader");
            return false;
        }
        lines.push_back(String(line));
    }

    if (lines.size() != 5) {
        testf("Read %u lines instead of %u", lines.size(), 5);
        return false;
    }

    std::vector<String> expected = {"line1", "line2", "line3", "", "line5"};

    for (size_t i = 0; i < expected.size(); i++) {
        const String& expectedStr = expected[i];
        const String& actualStr = lines[i];
        if (expectedStr != actualStr) {
            testf("Expected \"%s\" for line %u, but received \"%s\"", expectedStr.c_str(), i, actualStr.c_str());
            return false;
        }
    }

    return true;
}
