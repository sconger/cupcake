
#ifndef CUPCAKE_HUFFMAN_DATA_H
#define CUPCAKE_HUFFMAN_DATA_H

#include <cstdint>

namespace Cupcake {

namespace HuffmanData {

struct huffmanEncodeData {
    uint8_t bitCount;
    uint32_t value;
};

struct huffmanDecodeData {
    uint8_t state;
    uint8_t flags;
    uint8_t symbol;
};

extern const huffmanEncodeData huffmanEncodeTable[256];
extern const huffmanDecodeData huffmanDecodeTable[256][16];

}

}

#endif // CUPCAKE_HUFFMAN_DATA_H
