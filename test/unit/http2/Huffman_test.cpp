
#include "unit/UnitTest.h"
#include "unit/http2/Huffman_test.h"

#include "cupcake/internal/http2/HuffmanEncoding.h"

#include <array>

using namespace Cupcake;

bool test_hpack_huffman_encode() {
    struct testData {
        std::vector<char> input;
        std::vector<char> expected;
    };

    std::array<testData, 3> tests = {{
        {
            {'a', 'b', 'c', 'd'},
            // 00011, 100011, 00100, 100100, EOS
            {0b0001'1100, 0b0110'0100, (char)0b1001'0011}
        },
        {
            {0, 1, 2, 3, 4},
            // 11111111 | 11000
            // 11111111 | 11111111 | 1011000
            // 11111111 | 11111111 | 11111110 | 0010
            // 11111111 | 11111111 | 11111110 | 0011
            // 11111111 | 11111111 | 11111110 | 0100
            {(char)0b1111'1111, (char)0b1100'0111, (char)0b1111'1111, (char)0b1111'1101, (char)0b1000'1111, (char)0b1111'1111, (char)0b1111'1111, (char)0b1110'0010,
             (char)0b1111'1111, (char)0b1111'1111, (char)0b1111'1110, (char)0b0011'1111, (char)0b1111'1111, (char)0b1111'1111, (char)0b1110'0100}
        },
        {
            {},
            {}
        }
    }};

    for (const testData& data : tests) {
        std::vector<char> dest;
        HuffmanEncoding::encode(&dest, data.input.data(), data.input.size());
        if (dest != data.expected) {
            testf("Failed to encode as expected");
            return false;
        }
    }

    return true;
}

bool test_hpack_huffman_decode() {
    struct testData {
        std::vector<char> input;
        std::vector<char> expected;
        bool success;
    };

    std::array<testData, 5> tests = {{
        // Flipped versions of the above encode test
        {
            {0b0001'1100, 0b0110'0100, (char)0b1001'0011},
            {'a', 'b', 'c', 'd'},
            true,
        },
        {
            {(char)0b1111'1111, (char)0b1100'0111, (char)0b1111'1111, (char)0b1111'1101, (char)0b1000'1111, (char)0b1111'1111, (char)0b1111'1111, (char)0b1110'0010,
             (char)0b1111'1111, (char)0b1111'1111, (char)0b1111'1110, (char)0b0011'1111, (char)0b1111'1111, (char)0b1111'1111, (char)0b1110'0100},
            {0, 1, 2, 3, 4},
            true
        },
        {
            {},
            {},
            true
        },
        // Decodings that should fail
        {
            {0b0000'0011}, // Longest starting sequence of 0s is 5
            {},
            false
        },
        {
            {(char)0b1010'1000}, // Valid 'n', but does not end in EOS character
            {},
            false
        }
    }};

    for (const testData& data : tests) {
        std::vector<char> dest;
        bool success = HuffmanEncoding::decode(&dest, data.input.data(), data.input.size());
        if (success != data.success) {
            if (success) {
                testf("Decode succeeded when it should have failed");
            } else {
                testf("Decode failed when it should have succeeded");
            }
            return false;
        }
        if (success && dest != data.expected) {
            testf("Failed to decode as expected");
            return false;
        }
    }

    return true;
}
