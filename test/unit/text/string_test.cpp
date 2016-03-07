
#include "unit/text/String_test.h"
#include "unit/UnitTest.h"

#include "cupcake/text/String.h"

#include <cstring>
#include <utility>

static const char* longStr =
        "12345678901234567890123456789012345678901234567890";

bool test_string_create() {
    // Raw array
    {
        String rawInit("1234567890", 5);
        if (std::strcmp(rawInit.c_str(), "12345") != 0) {
            testf("Initialization of String with raw buffer did not produce expected value: 12345");
            return false;
        }
    }

    {
        String rawInitLong(longStr, std::strlen(longStr));
        if (std::strcmp(rawInitLong.c_str(), longStr) != 0) {
            testf("Initialization of String with long raw buffer did not produce expected value: %s",
                    longStr);
        }
    }

    // C string
    {
        String cInit("12345");
        if (std::strcmp(cInit.c_str(), "12345") != 0) {
            testf("Initialization of String with c-string did not produce expected value: 12345");
            return false;
        }
    }

    {
        String cInitLong(longStr);
        if (std::strcmp(cInitLong.c_str(), longStr) != 0) {
            testf("Initialization of String with long c-string did not produce expected value: %s",
                    longStr);
        }
    }

    // String object
    {
        String str("12345");
        String stringInit(str);
        if (std::strcmp(stringInit.c_str(), "12345") != 0) {
            testf("Initialization of String with String did not produce expected value: 12345");
            return false;
        }
    }

    {
        String strLong(longStr);
        String stringInitLong(strLong);
        if (std::strcmp(stringInitLong.c_str(), longStr) != 0) {
            testf("Initialization of String with long String did not produce expected value: %s",
                    longStr);
        }
    }

    // StringRef object
    {
        String stringrefInit(StringRef("12345"));
        if (std::strcmp(stringrefInit.c_str(), "12345") != 0) {
            testf("Initialization of String with StringRef did not produce expected value: 12345");
            return false;
        }
    }

    {
        String stringrefInitLong{StringRef(longStr)};
        if (std::strcmp(stringrefInitLong.c_str(), longStr) != 0) {
            testf("Initialization of String with long StringRef did not produce expected value: %s",
                    longStr);
        }
    }

    // Move construct
    {
        String toMove("12345");
        String moveInit(std::move(toMove));
        if (std::strcmp(moveInit.c_str(), "12345") != 0) {
            testf("Initialization of String by move did not produce expected value: 12345");
            return false;
        }
        if (toMove.length() != 0) {
            testf("Initialization of String by move did not clear moved string");
            return false;
        }
        if (std::strcmp(toMove.c_str(), "") != 0) {
            testf("Initialization of String by move did not clear moved string");
            return false;
        }
    }

    {
        String toMoveLong(longStr);
        String moveInitLong(std::move(toMoveLong));
        if (std::strcmp(moveInitLong.c_str(), longStr) != 0) {
            testf("Initialization of String with long String move did not produce expected value: %s",
                    longStr);
        }
        if (toMoveLong.length() != 0) {
            testf("Initialization of String by long String move did not clear moved string");
            return false;
        }
        if (std::strcmp(toMoveLong.c_str(), "") != 0) {
            testf("Initialization of String by move did not clear moved string");
            return false;
        }
    }

    return true;
}

bool test_string_append() {
    // Append of a String
    {
        String appString("abc");
        appString.append(String("def"));
        if (appString != "abcdef") {
            testf("Append of String to String with .append produced incorrect result");
            return false;
        }
    }

    {
        String appString("abc");
        appString += String("def");
        if (appString != "abcdef") {
            testf("Append of String to String with += produced incorrect result");
            return false;
        }
    }

    {
        String appString = String("abc") + String("d") + String("ef");
        if (appString != "abcdef") {
            testf("Append of String to String with + produced incorrect result");
            return false;
        }
    }

    // Append of a StringRef
    {
        String appStringRef("abc");
        appStringRef.append(StringRef("def"));
        if (appStringRef != "abcdef") {
            testf("Append of StringRef to String with .append produced incorrect result");
            return false;
        }
    }

    {
        String appStringRef("abc");
        appStringRef += StringRef("def");
        if (appStringRef != "abcdef") {
            testf("Append of StringRef to String with += produced incorrect result");
            return false;
        }
    }

    {
        String appStringRef = String("abc") + StringRef("de") + StringRef("f");
        if (appStringRef != "abcdef") {
            testf("Append of StringRef to String with + produced incorrect result");
            return false;
        }
    }

    // Append of raw data
    {
        String appRaw("abc");
        appRaw.append("defhij", 3);
        if (appRaw != "abcdef") {
            testf("Append of raw data to String with .append produced incorrect result");
            return false;
        }
    }

    // Append of c-string
    {
        String appCString("abc");
        appCString.append("def");
        if (appCString != "abcdef") {
            testf("Append of c-string to String with .append produced incorrect result");
            return false;
        }
    }

    {
        String appCString("abc");
        appCString += "def";
        if (appCString != "abcdef") {
            testf("Append of c-string to String with += produced incorrect result");
            return false;
        }
    }

    {
        String appCString = String("abc") + "d" + "ef";
        if (appCString != "abcdef") {
            testf("Append of c-string to String with + produced incorrect result");
            return false;
        }
    }

    // Append of a char
    {
        String appChar("abc");
        appChar.appendChar('z');
        if (appChar != "abcz") {
            testf("Append of char to String with .appendChar produced incorrect result");
            return false;
        }
    }

    {
        String appChar("abc");
        appChar += 'z';
        if (appChar != "abcz") {
            testf("Append of char to String with += produced incorrect result");
            return false;
        }
    }

    {
        String appChar = String("abc") + 'd' + 'e';
        if (appChar != "abcde") {
            testf("Append of char to String with + produced incorrect result");
            return false;
        }
    }
    return true;
}

bool test_string_startsWith() {
    // Positive cases
    if (!String("12345").startsWith("123")) {
        testf("String.startsWith returned false when asking if \"12345\" starts with \"123\"");
        return false;
    }

    if (!String("12345").startsWith("12345")) {
        testf("String.startsWith returned false for identical \"12345\" strings");
        return false;
    }

    if (!String("").startsWith("")) {
        testf("String.startsWith returned false for empty strings");
        return false;
    }

    if (!String("something").startsWith("")) {
        testf("String.startsWith returned false when asking if \"something\" starts with empty string");
        return false;
    }

    // Negative cases
    if (String("abc").startsWith("def")) {
        testf("String.startsWith returned true when asking if \"abc\" starts with \"def\"");
        return false;
    }

    if (String("").startsWith("abc")) {
        testf("String.startsWith returned true when asking if empty string starts with \"abc\"");
        return false;
    }

    if (String("defabc").startsWith("abc")) {
        testf("String.startsWith returned true when asking if \"defabc\" starts with \"abc\"");
        return false;
    }

    return true;
}

bool test_string_endsWith() {
    // Positive cases
    if (!String("12345").endsWith("345")) {
        testf("String.endsWith returned false when asking if \"12345\" ends with \"345\"");
        return false;
    }

    if (!String("12345").endsWith("12345")) {
        testf("String.endsWith returned false for identical \"12345\" strings");
        return false;
    }

    if (!String("").endsWith("")) {
        testf("String.endsWith returned false for empty strings");
        return false;
    }

    if (!String("something").endsWith("")) {
        testf("String.endsWith returned false when asking if \"something\" ends with empty string");
        return false;
    }

    // Negative cases
    if (String("abc").endsWith("def")) {
        testf("String.endsWith returned true when asking if \"abc\" ends with \"def\"");
        return false;
    }

    if (String("").endsWith("abc")) {
        testf("String.endsWith returned true when asking if empty string ends with \"abc\"");
        return false;
    }

    if (String("abcdef").endsWith("abc")) {
        testf("String.endsWith returned true when asking if \"abcdef\" ends with \"abc\"");
        return false;
    }

    return true;
}

bool test_string_indexOf() {
    {
        auto index = String("012345").indexOf("12345");
        if (index != 1) {
            testf("String(\"012345\").indexOf(\"12345\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("12345").indexOf("12345");
        if (index != 0) {
            testf("String(\"12345\").indexOf(\"12345\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("").indexOf("");
        if (index != 0) {
            testf("String(\"\").indexOf(\"\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abc").indexOf("");
        if (index != 0) {
            testf("String(\"abc\").indexOf(\"\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abc").indexOf("bcd");
        if (index != -1) {
            testf("String(\"abc\").indexOf(\"bcd\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abcabc").indexOf("abc", 1);
        if (index != 3) {
            testf("String(\"abcabc\").indexOf(\"abc\", 1) returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abcdefabc").indexOf("abc", 7);
        if (index != -1) {
            testf("String(\"abcdefabc\").indexOf(\"abc\", 7); returned: %d", index);
            return false;
        }
    }

    {
        static const char* longSearchStr =
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "abcdefabcde123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789";

        auto index = String(longSearchStr).indexOf("abc");
        if (index != 200) {
            testf("String(longString).indexOf(\"abc\"); returned %d instead of expected 200", index);
            return false;
        }

        index = String(longSearchStr).indexOf("abc", 201);
        if (index != 206) {
            testf("String(longString).indexOf(\"abc\", 201); returned %d instead of expected 206", index);
            return false;
        }

        index = String(longSearchStr).indexOf("notthere", 0);
        if (index != -1) {
            testf("String(longString).indexOf(\"notthere\", 0); returned %d instead of expected -1", index);
            return false;
        }
    }

    return true;
}

bool test_string_lastIndexOf() {
    {
        auto index = String("012345").lastIndexOf("12345");
        if (index != 1) {
            testf("String(\"012345\").lastIndexOf(\"12345\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("12345").lastIndexOf("12345");
        if (index != 0) {
            testf("String(\"12345\").lastIndexOf(\"12345\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("").lastIndexOf("");
        if (index != 0) {
            testf("String(\"\").lastIndexOf(\"\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abc").lastIndexOf("");
        if (index != 0) {
            testf("String(\"abc\").lastIndexOf(\"\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abc").lastIndexOf("bcd");
        if (index != -1) {
            testf("String(\"abc\").lastIndexOf(\"bcd\") returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("abcabc").lastIndexOf("abc", 4);
        if (index != 0) {
            testf("String(\"abcabc\").lastIndexOf(\"abc\", 4) returned: %d", index);
            return false;
        }
    }

    {
        auto index = String("afbcdeabc").lastIndexOf("abc", 4);
        if (index != -1) {
            testf("String(\"abcdefabc\").lastIndexOf(\"abc\", 4); returned: %d", index);
            return false;
        }
    }

    {
        static const char* longSearchStr =
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "abcdefabcde123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789"
                "01234567890123456789012345678901234567890123456789";

        auto index = String(longSearchStr).lastIndexOf("abc");
        if (index != 206) {
            testf("String(longString).lastIndexOf(\"abc\"); returned %d instead of expected 206", index);
            return false;
        }

        index = String(longSearchStr).lastIndexOf("abc", 205);
        if (index != 200) {
            testf("String(longString).lastIndexOf(\"abc\", 201); returned %d instead of expected 200", index);
            return false;
        }

        index = String(longSearchStr).lastIndexOf("notthere", 0);
        if (index != -1) {
            testf("String(longString).lastIndexOf(\"notthere\", 0); returned %d instead of expected -1", index);
            return false;
        }
    }

    return true;
}

bool test_string_substring() {
    if (!String("").substring(0).equals("")) {
        testf("String(\"\").substring(0) != \"\"");
        return false;
    }

    if (!String("abc").substring(0).equals("abc")) {
        testf("String(\"abc\").substring(0) != \"abc\"");
        return false;
    }

    if (!String("abc").substring(1).equals("bc")) {
        testf("String(\"abc\").substring(1) != \"bc\"");
        return false;
    }

    if (!String("abc").substring(3).equals("")) {
        testf("String(\"abc\").substring(3) != \"\"");
        return false;
    }

    if (!String("foobar").substring(1, 4).equals("oob")) {
        testf("String(\"foobar\").substring(1, 4) != \"oob\"");
        return false;
    }

    return true;
}
