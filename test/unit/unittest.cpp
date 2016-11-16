
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "unit/UnitTest.h"

#include "unit/http/BufferedReader_test.h"
#include "unit/http/BufferedWriter_test.h"
#include "unit/http/ChunkedReader_test.h"
#include "unit/http/ChunkedWriter_test.h"
#include "unit/http/CommaListIterator_test.h"
#include "unit/http/Http1_test.h"
#include "unit/http/Http1_1_test.h"
#include "unit/http2/Huffman_test.h"
#include "unit/text/String_test.h"
#include "unit/text/Strconv_test.h"
#include "unit/net/AddrInfo_test.h"
#include "unit/net/Socket_test.h"
#include "unit/util/PathTrie_test.h"

#include "cupcake/Cupcake.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static const char* testName = NULL;
static int testRes;

int testf(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    size_t rawLen = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char* raw = new char[rawLen+1];

    va_start(args, fmt);
    vsnprintf(raw, rawLen+1, fmt, args);
    va_end(args);

    int res = printf("%s: %s\n", testName, raw);
    delete[] raw;
    return res;
}

#define RUN_TEST(x) do { \
    testName = # x; \
    if (x()) { \
        printf("PASSED: " # x "\n"); \
    } else { \
        printf("FAILED: " # x "\n"); \
        testRes = 1; \
    } \
} while(0)

int main(int argc, const char** argv) {
    testRes = 0;

    Cupcake::init();

    // Text
    RUN_TEST(test_string_create);
    RUN_TEST(test_string_append);
    RUN_TEST(test_string_indexOf);
    RUN_TEST(test_string_lastIndexOf);
    RUN_TEST(test_string_startsWith);
    RUN_TEST(test_string_endsWith);
    RUN_TEST(test_string_substring);

    RUN_TEST(test_strconv_int32ToStr);
    RUN_TEST(test_strconv_int64ToStr);
    RUN_TEST(test_strconv_uint32ToStr);
    RUN_TEST(test_strconv_uint64ToStr);
    RUN_TEST(test_strconv_parseInt32);
    RUN_TEST(test_strconv_parseInt64);
    RUN_TEST(test_strconv_parseUint32);
    RUN_TEST(test_strconv_parseUint64);

    // Util
    RUN_TEST(test_pathtrie_exactmatch);
    RUN_TEST(test_pathtrie_regex);
    RUN_TEST(test_pathtrie_collision);

    // Socket functionality
    RUN_TEST(test_addrinfo_addrlookup);
    RUN_TEST(test_addrinfo_asynclookup);

    RUN_TEST(test_socket_basic);
    RUN_TEST(test_socket_vector);
    RUN_TEST(test_socket_accept_multiple);
    RUN_TEST(test_socket_set_options);

    // HTTP functionality
    RUN_TEST(test_bufferedreader_basic);
    RUN_TEST(test_bufferedreader_readline);
    RUN_TEST(test_bufferedreader_readfixed);
    RUN_TEST(test_bufferedreader_peekfixed);
    RUN_TEST(test_bufferedwriter_basic);
    RUN_TEST(test_bufferedwriter_flush);

    RUN_TEST(test_chunkedreader_basic);
    RUN_TEST(test_chunkedreader_empty);
    RUN_TEST(test_chunkedreader_bad_data_line);
    RUN_TEST(test_chunkedreader_extension);
    RUN_TEST(test_chunkedreader_trailing_headers);
    RUN_TEST(test_chunkedwriter_basic);
    RUN_TEST(test_chunkedwriter_empty);

    RUN_TEST(test_commalistiterator_next);
    RUN_TEST(test_commalistiterator_getLast);

    RUN_TEST(test_http1_empty);
    RUN_TEST(test_http1_contentlen_request);
    RUN_TEST(test_http1_auto_contentlen_response);
    RUN_TEST(test_http1_request_with_transfer_encoding);
    RUN_TEST(test_http1_response_with_transfer_encoding);
    RUN_TEST(test_http1_keepalive);

    RUN_TEST(test_http1_1_chunked_request);
    RUN_TEST(test_http1_1_chunked_response);
    RUN_TEST(test_http1_1_auto_chunked_response);
    RUN_TEST(test_http1_1_keepalive);

    // Http2 functionality
    RUN_TEST(test_hpack_huffman_encode);
    RUN_TEST(test_hpack_huffman_decode);

    if (testRes) {
        printf("FAILURE: Not all tests passed.\n");
    } else {
        printf("SUCCESS: All tests passed.\n");
    }

    Cupcake::cleanup();

    return testRes;
}
