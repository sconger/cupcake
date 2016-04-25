
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "unit/UnitTest.h"

#include "unit/text/String_test.h"
#include "unit/net/AddrInfo_test.h"
#include "unit/net/Socket_test.h"
#include "unit/net/INet_test.h"
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

    // Util
    RUN_TEST(test_pathtrie_exactmatch);
    RUN_TEST(test_pathtrie_regex);
    RUN_TEST(test_pathtrie_collision);

    // Socket functionality
    RUN_TEST(test_inet_gethostname);

    RUN_TEST(test_addrinfo_addrlookup);
    RUN_TEST(test_addrinfo_asynclookup);

    RUN_TEST(test_socket_basic);
    RUN_TEST(test_socket_accept_multiple);
    RUN_TEST(test_socket_set_options);

    if (testRes) {
        printf("FAILURE: Not all tests passed.\n");
    } else {
        printf("SUCCESS: All tests passed.\n");
    }

    Cupcake::cleanup();

    return testRes;
}
