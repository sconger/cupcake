
#ifndef CUPCAKE_HPACK_DECODER_H
#define CUPCAKE_HPACK_DECODER_H

#include "cupcake/internal/http/RequestData.h"
#include "cupcake/internal/http2/HpackTable.h"

#include <vector>

namespace Cupcake {

/*
 * Decodes HPACK data into header information.
 */
class HpackDecoder {
public:
    HpackDecoder(HpackTable* hpackTable,
        RequestData* requestData,
        const char* data,
        size_t dataLen);

    bool decode();

private:
    HpackDecoder(const HpackDecoder&) = delete;
    HpackDecoder& operator=(const HpackDecoder&) = delete;

    bool readIndexedHeaderField();
    bool readLiteralHeaderIncrementalIndexing();
    bool readLiteralHeaderNoIndexing();
    bool readLiteralHeaderNeverIndexed();
    bool readTableSizeUpdate();

    bool nextByte(uint8_t* value);
    bool readNumberAfterPrefix(uint32_t* value);
    bool readStringLiteral(std::vector<char>& buffer);

    HpackTable* hpackTable;
    RequestData* requestData;
    const char* data;
    const char* dataEnd;

    std::vector<char> nameBuffer;
    std::vector<char> valueBuffer;
};

}

#endif // CUPCAKE_HPACK_DECODER_H
