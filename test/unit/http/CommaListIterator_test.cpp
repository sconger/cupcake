
#include "unit/UnitTest.h"
#include "unit/http/CommaListIterator_test.h"

#include "cupcake/internal/http/CommaListIterator.h"

#include <vector>

using namespace Cupcake;

bool test_commalistiterator_next() {
    const std::vector<StringRef> testValues = {
        " uno ",
        "one,two,three",
        " one, two\t, three",
        ", \t,,",
    };
    const std::vector<std::vector<StringRef>> expected = {
        {"uno"},
        {"one", "two", "three"},
        {"one", "two", "three"},
        {"", "", "", ""},
    };

    for (size_t i = 0; i < testValues.size(); i++) {
        CommaListIterator iter(testValues[i]);

        auto iterExpected = expected[i];
        for (StringRef test : iterExpected) {
            StringRef next = iter.next();
            if (test != next) {
                testf("Did not see expected value when iterating through comma list");
                return false;
            }
        }
    }

    return true;
}

bool test_commalistiterator_getLast() {
    const std::vector<StringRef> testValues = {
        " uno ",
        "one,two,three",
        " one, two\t, three",
        ", \t,,",
    };
    const std::vector<StringRef> expected = {
        "uno",
        "three",
        "three",
        "",
    };

    for (size_t i = 0; i < testValues.size(); i++) {
        CommaListIterator iter(testValues[i]);

        StringRef last = iter.getLast();
        if (expected[i] != last) {
            testf("Did not see expected last value");
            return false;
        }
    }

    return true;
}
