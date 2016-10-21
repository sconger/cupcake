
#include "cupcake_priv/http2/HuffmanEncoding.h"
#include "cupcake_priv/http2/HuffmanData.h"

using namespace Cupcake;

namespace Cupcake {

namespace HuffmanEncoding {

void encode(std::vector<char>* dest, const char* data, size_t dataLen) {
    uint8_t rembits = 8;
    uint8_t curByte = 0;
    const HuffmanData::huffmanEncodeData* encodeData;

    for (size_t i = 0; i < dataLen; i++) {
        uint8_t c = data[i];
        encodeData = &HuffmanData::huffmanEncodeTable[c];

        uint8_t bitCount = encodeData->bitCount;

        // Loop over the bytes of the current value looked up in the table
        while (1) {
            // If there are not enough bits left to fill the current byte, copy in what we can
            if (rembits > bitCount) {
                uint8_t mask = (uint8_t)(encodeData->value << (rembits - bitCount));
                curByte |= mask;

                rembits -= bitCount;
                break;
            }

            // Otherwise, there are enough bits to fill the current byte, so fill it and push it
            // onto the result
            uint8_t mask = (uint8_t)(encodeData->value >> (bitCount - rembits));
            curByte |= mask;

            bitCount -= rembits;
            rembits = 8;
            dest->push_back(curByte);
            curByte = 0;

            // Break if we exactly filled it
            if (bitCount == 0) {
                break;
            }
        }
    }

    // If there are still remaining bits, fill with the EOS character
    if (rembits < 8) {
        uint8_t mask = (uint8_t)(0x3fffffff >> (30 - rembits));
        curByte |= mask;
        dest->push_back(curByte);
    }
}

void decode(std::vector<char>* dest, const char* data, size_t dataLen) {

}

} // End namespace HuffmanEncoding

} // End namespace Cupcake
