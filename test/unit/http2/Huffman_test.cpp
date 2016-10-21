
#include "unit/UnitTest.h"
#include "unit/http2/Huffman_test.h"

#include "cupcake_priv/http2/HuffmanEncoding.h"

using namespace Cupcake;

bool test_hpack_huffman_encode() {
    struct testData {
        const char* input;
        size_t inputLen;
        std::vector<char> expected;
    };

    testData tests[2] = {
        {
            "abcd", 4,
            // 00011, 100011, 00100, 100100, EOS
            {0b00011100, 0b01100100, (char)0b10010011}
        },
        {
            "\x00\x01\x02\x03\x04", 5,
            // 11111111 | 11000
            // 11111111 | 11111111 | 1011000
            // 11111111 | 11111111 | 11111110 | 0010
            // 11111111 | 11111111 | 11111110 | 0011
            // 11111111 | 11111111 | 11111110 | 0100
            // 
            // 1111'1111, 1100'0111, 1111'1111, 1111'1101, 1000'1111, 1111'1111, 1111'1111, 1110'0010,
            // 1111'1111, 1111'1111, 1111'1110, 0011'1111, 1111'1111, 1111'1111, 1110'0100
            {(char)0b1111'1111, (char)0b1100'0111, (char)0b1111'1111, (char)0b1111'1101, (char)0b1000'1111, (char)0b1111'1111, (char)0b1111'1111, (char)0b1110'0010,
             (char)0b1111'1111, (char)0b1111'1111, (char)0b1111'1110, (char)0b0011'1111, (char)0b1111'1111, (char)0b1111'1111, (char)0b1110'0100}
        }
    };

    for (size_t i = 0; i < 2; i++) {
        testData* data = &tests[i];
        std::vector<char> dest;
        HuffmanEncoding::encode(&dest, data->input, data->inputLen);
        if (dest != data->expected) {
            testf("Failed to encode as expected");
            return false;
        }
    }

    return true;
}
