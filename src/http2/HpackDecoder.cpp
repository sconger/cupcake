
#include "cupcake/internal/http2/HpackDecoder.h"

#include "cupcake/internal/http2/HuffmanEncoding.h"

using namespace Cupcake;

HpackDecoder::HpackDecoder(HpackTable* hpackTable,
        RequestData* requestData,
        const char* data,
        size_t dataLen) :
    hpackTable(hpackTable),
    requestData(requestData),
    data(data),
    dataEnd(data + dataLen) {

}

bool HpackDecoder::decode() {
    while (data < dataEnd) {
        uint8_t startingByte = (uint8_t)*data;
        if (startingByte & 0b1000'0000) {
            if (!readIndexedHeaderField()) {
                return false;
            }
        } else if ((startingByte & 0b1100'0000) == 0b0100'0000) {
            if (!readLiteralHeaderIncrementalIndexing()) {
                return false;
            }
        } else if ((startingByte & 0b1110'0000) == 0b0010'0000) {
            if (!readTableSizeUpdate()) {
                return false;
            }
        } else if ((startingByte & 0b1111'0000) == 0b0001'0000) {
            if (!readLiteralHeaderNeverIndexed()) {
                return false;
            }
        } else if ((startingByte & 0b1111'0000) == 0b0000'0000) {
            if (!readLiteralHeaderNoIndexing()) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

bool HpackDecoder::readIndexedHeaderField() {
    uint32_t index = 0;
    uint8_t firstByteData = (uint8_t)*data & 0x7F;

    if (firstByteData < 0x7F) {
        // Zero index is not allowed
        if (firstByteData == 0) {
            return false;
        }
        index = firstByteData;
    } else {
        bool res = readNumberAfterPrefix(&index);
        if (!res) {
            return false;
        }
    }
    if (!hpackTable->hasEntryAtIndex(index)) {
        return false;
    }

    if (index <= 61) {
        requestData->addStaticHeaderName(hpackTable->nameAtIndex(index));
        requestData->addStaticHeaderValue(hpackTable->valueAtIndex(index));
    } else {
        requestData->addHeaderName(hpackTable->nameAtIndex(index));
        requestData->addHeaderValue(hpackTable->valueAtIndex(index));
    }

    return true;
}

bool HpackDecoder::readLiteralHeaderIncrementalIndexing() {
    bool res;
    uint8_t firstByteData = (uint8_t)*data & 0x3F;

    if (firstByteData == 0) {
        // If the index is not set, we need to read both name and value
        res = readStringLiteral(nameBuffer);
        if (!res) {
            return false;
        }
        res = readStringLiteral(valueBuffer);
        if (!res) {
            return false;
        }
        StringRef headerName(nameBuffer.data(), nameBuffer.size());
        StringRef headerValue(valueBuffer.data(), valueBuffer.size());
        requestData->addHeaderName(headerName);
        requestData->addHeaderValue(headerValue);
        hpackTable->add(headerName, headerValue);
    } else {
        uint32_t index = 0;
        if (firstByteData < 0x3F) {
            index = firstByteData;
        } else {
            res = readNumberAfterPrefix(&index);
            if (!res) {
                return false;
            }
        }
        if (!hpackTable->hasEntryAtIndex(index)) {
            return false;
        }
        res = readStringLiteral(valueBuffer);
        if (!res) {
            return false;
        }
        StringRef headerName(hpackTable->nameAtIndex(index));
        StringRef headerValue(valueBuffer.data(), valueBuffer.size());
        if (index <= 61) {
            requestData->addStaticHeaderName(headerName);
        } else {
            requestData->addHeaderName(headerName);
        }
        requestData->addHeaderValue(headerValue);
        hpackTable->add(headerName, headerValue);
    }

    return true;
}

bool HpackDecoder::readLiteralHeaderNoIndexing() {
    return true;
}

bool HpackDecoder::readLiteralHeaderNeverIndexed() {
    return true;
}

bool HpackDecoder::readTableSizeUpdate() {
    return true;
}

bool HpackDecoder::nextByte(uint8_t* value) {
    if (data < dataEnd - 1) {
        data++;
        (*value) = (uint8_t)*data;
        return true;
    }
    return false;
}

bool HpackDecoder::readNumberAfterPrefix(uint32_t* value) {
    // TODO: Should probably make optimized path where there are more than 5 bytes left
    // The resVal != 0 checks are to guard against zero values that should have
    // fit in the prefix.
    uint32_t resVal = 0;
    uint8_t byte;

    // First 7 bit extension
    bool res = nextByte(&byte);
    if (!res) {
        return false;
    }
    resVal = (byte & 0x7F);
    if (byte & 0x80) {
        (*value) = resVal;
        return resVal != 0;
    }

    // Second 7 bit extension
    res = nextByte(&byte);
    if (!res) {
        return false;
    }
    resVal += (byte & 0x7F) << 7;
    if (byte & 0x80) {
        (*value) = resVal;
        return resVal != 0;
    }

    // Third 7 bit extension
    res = nextByte(&byte);
    if (!res) {
        return false;
    }
    resVal += (byte & 0x7F) << 14;
    if (byte & 0x80) {
        (*value) = resVal;
        return resVal != 0;
    }

    // Fourth 7 bit extension
    res = nextByte(&byte);
    if (!res) {
        return false;
    }
    resVal += (byte & 0x7F) << 21;
    if (byte & 0x80) {
        (*value) = resVal;
        return resVal != 0;
    }

    // And we can only support 4 bits of a fifth byte
    res = nextByte(&byte);
    if (!res) {
        return false;
    }
    // This should cover both the case of value too large and continuation
    if (byte > 0xF) {
        return false;
    }
    resVal += byte << 28;
    (*value) = resVal;
    return resVal != 0;
}

bool HpackDecoder::readStringLiteral(std::vector<char>& buffer) {
    uint8_t initialLengthAndHuffman;
    bool res = nextByte(&initialLengthAndHuffman);
    if (!res) {
        return false;
    }
    uint32_t length;
    uint8_t firstByteLength = initialLengthAndHuffman & 0x7F;
    bool huffman = !!(initialLengthAndHuffman & 0x80);

    if (firstByteLength != 0) {
        length = firstByteLength;
    } else {
        res = readNumberAfterPrefix(&length);
        if (!res) {
            return false;
        }
    }

    size_t available = dataEnd - data;
    if (available < length) {
        return false;
    }

    buffer.clear();
    if (huffman) {
        HuffmanEncoding::decode(&buffer, data, length);
    } else {
        buffer.assign(data, data + length);
    }

    data += length;
    return true;
}
