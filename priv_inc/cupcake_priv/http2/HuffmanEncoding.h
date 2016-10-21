
#ifndef CUPCAKE_HUFFMAN_ENCODING
#define CUPCAKE_HUFFMAN_ENCODING

#include <vector>

namespace Cupcake {

/*
 * HPACK Huffman encoding functionality.
 */
namespace HuffmanEncoding {
    void encode(std::vector<char>* dest, const char* data, size_t dataLen);
    void decode(std::vector<char>* dest, const char* data, size_t dataLen);
}

}

#endif // CUPCAKE_HUFFMAN_ENCODING
