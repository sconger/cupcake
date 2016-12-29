
#include "unit/http2/Hpack_test.h"

#include "unit/UnitTest.h"

#include "cupcake/internal/http2/HpackDecoder.h"
#include "cupcake/internal/text/Strconv.h"

using namespace Cupcake;

static
bool decode(HpackTable* hpackTable, RequestData* requestData, const char* data, size_t dataLen) {
    HpackDecoder decoder(hpackTable, requestData, data, dataLen);
    return decoder.decode();
}

bool test_hpack_indexed_header() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    // Test that static values work
    {
        const char index32[] = {(char)0b1001'0000};
        if (!decode(&hpackTable, &requestData, index32, sizeof(index32))) {
            testf("Failed to parse indexed header entry");
            return false;
        }
        if (requestData.getHeaderCount() != 1 ||
            !requestData.getHeaderName(0).equals("accept-encoding") ||
            !requestData.getHeaderValue(0).equals("gzip, deflate")) {
            testf("Indexed header entry did not cause expected request data update");
            return false;
        }
    }

    {
        const char index61[] = {(char)0b1011'1101};
        if (!decode(&hpackTable, &requestData, index61, sizeof(index61))) {
            testf("Failed to parse indexed header entry");
            return false;
        }
        if (requestData.getHeaderCount() != 2 ||
            !requestData.getHeaderName(1).equals("www-authenticate") ||
            !requestData.getHeaderValue(1).equals("")) {
            testf("Indexed header entry did not cause expected request data update");
            return false;
        }
    }

    // Test that a dynamic valid works
    {
        hpackTable.add("testName", "testValue");

        const char index62[] = {(char)0b1011'1110};
        if (!decode(&hpackTable, &requestData, index62, sizeof(index62))) {
            testf("Failed to parse indexed header entry");
            return false;
        }
        if (requestData.getHeaderCount() != 3 ||
            !requestData.getHeaderName(2).equals("testName") ||
            !requestData.getHeaderValue(2).equals("testValue")) {
            testf("Indexed header entry did not cause expected request data update");
            return false;
        }
    }

    // Add enough values to the dynamic table that it takes 2 bytes to index it
    // and make sure we can look up one of those values
    {
        for (uint32_t i = 0; i < 200; i++) {
            char numberBuffer[50];
            size_t strLen = Strconv::uint32ToStr(i, numberBuffer, sizeof(numberBuffer));
            StringRef numberStr(numberBuffer, strLen);
            hpackTable.add(numberStr, numberStr);
        }

        const char index200[] = {(char)0b1111'1111, (char)0b0100'1001};
        if (!decode(&hpackTable, &requestData, index200, sizeof(index200))) {
            testf("Failed to parse indexed header entry");
            return false;
        }
        if (requestData.getHeaderCount() != 4 ||
            !requestData.getHeaderName(3).equals("61") ||
            !requestData.getHeaderValue(3).equals("61")) {
            testf("Indexed header entry did not cause expected request data update");
            return false;
        }
    }

    return true;
}

bool test_hpack_indexed_header_invalid() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    // Test an entry that doesn't exist is a parse error
    {
        const char index124[] = {(char)0b1111'1110};
        if (decode(&hpackTable, &requestData, index124, sizeof(index124))) {
            testf("Returned success for index where no entry exists");
            return false;
        }
    }

    // Test truncated index fails to parse
    {
        const char truncatedIndex1[] = {(char)0b1111'1111};
        if (decode(&hpackTable, &requestData, truncatedIndex1, sizeof(truncatedIndex1))) {
            testf("Returned success for truncated index");
            return false;
        }
    }
    {
        const char truncatedIndex2[] = {(char)0b1111'1111, (char)0b1000'0000};
        if (decode(&hpackTable, &requestData, truncatedIndex2, sizeof(truncatedIndex2))) {
            testf("Returned success for truncated index");
            return false;
        }
    }
    return true;
}

bool test_hpack_incremental_indexing() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    // Test use of a static header name
    // Index of accept-charset with huffman coded "value" as the value
    // 1110111, 00011, 101000, 101101, 00101
    {
        const char acceptCharset[] = {(char)0b0100'1111, (char)0b1000'0100, (char)0b1110'1110, (char)0b0011'1010, (char)0b0010'1101, (char)0b0010'1111};
        if (!decode(&hpackTable, &requestData, acceptCharset, sizeof(acceptCharset))) {
            testf("Failed to parse incremental indexing data");
            return false;
        }
        if (requestData.getHeaderCount() != 1 ||
            !requestData.getHeaderName(0).equals("accept-charset") ||
            !requestData.getHeaderValue(0).equals("value")) {
            testf("Indexed header entry did not cause expected request data update");
            return false;
        }
        if (!hpackTable.hasEntryAtIndex(62) ||
            !hpackTable.nameAtIndex(62).equals("accept-charset") ||
            !hpackTable.valueAtIndex(62).equals("value")) {
            testf("Did not see expected update to hpack table after incremental indexing entry");
            return false;
        }
    }

    // Test use of non-static header name
    {
        const char nameValue[] = {(char)0b0100'0000, (char)0b0000'0100, 'n', 'a', 'm', 'e', (char)0b0000'0101, 'v', 'a', 'l', 'u', 'e'};
        if (!decode(&hpackTable, &requestData, nameValue, sizeof(nameValue))) {
            testf("Failed to parse incremental indexing data");
            return false;
        }
        if (requestData.getHeaderCount() != 2 ||
            !requestData.getHeaderName(1).equals("name") ||
            !requestData.getHeaderValue(1).equals("value")) {
            testf("Indexed header entry did not cause expected request data update");
            return false;
        }
        if (!hpackTable.hasEntryAtIndex(62) ||
            !hpackTable.nameAtIndex(62).equals("name") ||
            !hpackTable.valueAtIndex(62).equals("value")) {
            testf("Did not see expected update to hpack table after incremental indexing entry");
            return false;
        }
    }

    return true;
}

bool test_hpack_incremental_indexing_invalid() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    // Test an entry that doesn't exist is a parse error
    {
        const char badIndex[] = {(char)0b0111'1111, (char)0b1000'1111, (char)0b1000'0100, (char)0b1110'1110, (char)0b0011'1010, (char)0b0010'1101, (char)0b0010'1111};
        if (decode(&hpackTable, &requestData, badIndex, sizeof(badIndex))) {
            testf("Returned success for index where no entry exists");
            return false;
        }
    }

    // Test various truncation cases
    {
        const char truncatedValue[] = {(char)0b0100'1111, (char)0b1000'0100, (char)0b1110'1110, (char)0b0011'1010, (char)0b0010'1101};
        if (decode(&hpackTable, &requestData, truncatedValue, sizeof(truncatedValue))) {
            testf("Returned success for truncated value");
            return false;
        }
    }
    {
        const char noName[] = {(char)0b0100'0001};
        if (decode(&hpackTable, &requestData, noName, sizeof(noName))) {
            testf("Returned success for truncated data");
            return false;
        }
    }
    {
        const char truncatedName[] = {(char)0b0100'0000, (char)0b0000'0100, 'n', 'a'};
        if (decode(&hpackTable, &requestData, truncatedName, sizeof(truncatedName))) {
            testf("Returned success for truncated index");
            return false;
        }
    }
    return true;
}

bool test_hpack_without_indexing() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    // Test use of a static header name
    // Index of accept-charset with huffman coded "value" as the value
    // 1110111, 00011, 101000, 101101, 00101
    {
        const char acceptCharset[] = {(char)0b0000'1111, (char)0b0000'0000, (char)0b1000'0100, (char)0b1110'1110, (char)0b0011'1010, (char)0b0010'1101, (char)0b0010'1111};
        if (!decode(&hpackTable, &requestData, acceptCharset, sizeof(acceptCharset))) {
            testf("Failed to parse non-indexed data");
            return false;
        }
        if (requestData.getHeaderCount() != 1 ||
            !requestData.getHeaderName(0).equals("accept-charset") ||
            !requestData.getHeaderValue(0).equals("value")) {
            testf("Non-indexed entry did not cause expected request data update");
            return false;
        }
        if (hpackTable.hasEntryAtIndex(62)) {
            testf("Non-indexed entry caused hpack table change");
            return false;
        }
    }

    // Test use of non-static header name
    {
        const char nameValue[] = {(char)0b0001'0000, (char)0b0000'0100, 'n', 'a', 'm', 'e', (char)0b0000'0101, 'v', 'a', 'l', 'u', 'e'};
        if (!decode(&hpackTable, &requestData, nameValue, sizeof(nameValue))) {
            testf("Failed to parse non-indexed data");
            return false;
        }
        if (requestData.getHeaderCount() != 2 ||
            !requestData.getHeaderName(1).equals("name") ||
            !requestData.getHeaderValue(1).equals("value")) {
            testf("Non-indexed entry did not cause expected request data update");
            return false;
        }
        if (hpackTable.hasEntryAtIndex(62)) {
            testf("Non-indexed entry caused hpack table change");
            return false;
        }
    }
    return true;
}

bool test_hpack_without_indexing_invalid() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    // Test an entry that doesn't exist is a parse error
    {
        const char badIndex[] = {(char)0b0000'1111, (char)0b1000'1111, (char)0b1000'0100, (char)0b1110'1110, (char)0b0011'1010, (char)0b0010'1101, (char)0b0010'1111};
        if (decode(&hpackTable, &requestData, badIndex, sizeof(badIndex))) {
            testf("Returned success for index where no entry exists");
            return false;
        }
    }

    // Test various truncation cases
    {
        const char truncatedValue[] = {(char)0b0000'1111, (char)0b0000'0000, (char)0b1000'0100, (char)0b1110'1110, (char)0b0011'1010, (char)0b0010'1101};
        if (decode(&hpackTable, &requestData, truncatedValue, sizeof(truncatedValue))) {
            testf("Returned success for truncated value");
            return false;
        }
    }
    {
        const char noName[] = {(char)0b0000'0001};
        if (decode(&hpackTable, &requestData, noName, sizeof(noName))) {
            testf("Returned success for truncated data");
            return false;
        }
    }
    {
        const char truncatedName[] = {(char)0b0000'0000, (char)0b0000'0100, 'n', 'a'};
        if (decode(&hpackTable, &requestData, truncatedName, sizeof(truncatedName))) {
            testf("Returned success for truncated index");
            return false;
        }
    }
    return true;
}

bool test_hpack_table_size_change() {
    RequestData requestData;
    HpackTable hpackTable;
    hpackTable.init(std::numeric_limits<uint32_t>::max());

    hpackTable.add("1234", "1234"); // 8 + 32 = 40

    const char setSize40[] = {(char)0b0011'1111, (char)0b0000'1001};
    if (!decode(&hpackTable, &requestData, setSize40, sizeof(setSize40))) {
        testf("Faled to parse table size update");
        return false;
    }

    if (!hpackTable.hasEntryAtIndex(62)) {
        testf("Change to size removed entry when it should not have");
        return false;
    }

    const char setSize39[] = {(char)0b0011'1111, (char)0b0000'1000};
    if (!decode(&hpackTable, &requestData, setSize39, sizeof(setSize39))) {
        testf("Faled to parse table size update");
        return false;
    }

    if (hpackTable.hasEntryAtIndex(62)) {
        testf("Change to size failed to removed entry when it should have");
        return false;
    }

    if (hpackTable.add("1234", "5678")) {
        testf("Was able to add to hpack table when value does not fit");
        return false;
    }

    if (!hpackTable.add("1234", "567")) {
        testf("Was not able to add to hpack table when value should have fit");
        return false;
    }

    return true;
}
