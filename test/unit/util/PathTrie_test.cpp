
#include "unit/util/PathTrie_test.h"

#include "unit/UnitTest.h"

#include "cupcake/internal/util/PathTrie.h"

#include <array>
#include <vector>

using namespace Cupcake;

bool test_pathtrie_exactmatch() {
    PathTrie<int> trie;

    trie.addNode("abc", 1);
    trie.addNode("ab", 2);
    trie.addNode("a", 3);
    trie.addNode("abcdef", 4);
    trie.addNode("abcdzf", 5);

    std::vector<int> vals;
    std::vector<bool> founds;
    std::array<const char*, 6> testVals{{"abc", "ab", "a", "abcdef", "aq", "abcdzf"}};
    std::array<int, 6> expectedVals{{1, 2, 3, 4, 0, 5}};

    for (size_t i = 0; i < testVals.size(); i++) {
        const char* str = testVals[i];
        int val;
        bool found;
        std::tie(val, found) = trie.find(str);
        vals.push_back(val);
        founds.push_back(found);
    }

    for (size_t i = 0; i < testVals.size(); i++) {
        int expected = expectedVals[i];
        if (expected == 0) {
            if (founds[i]) {
                testf("Found value for \"%s\", despite not being added", testVals[i]);
                return false;
            }
            continue;
        }
        if (!founds[i]) {
            testf("Failed to find value for \"%s\", despite being added", testVals[i]);
            return false;
        }
        if (vals[i] != expected) {
            testf("Got back %d instead of expected %d for \"%s\"", vals[i], expected, testVals[i]);
            return false;
        }
    }

    return true;
}

bool test_pathtrie_regex() {
    PathTrie<int> trie;

    trie.addNode("abc*", 1);
    trie.addNode("a", 2);

    std::vector<int> vals;
    std::vector<bool> founds;
    std::array<const char*, 5> testVals{{"abc", "abcd", "abczzzzzzzz", "ab", "a"}};
    std::array<int, 5> expectedVals{{1, 1, 1, 0, 2}};

    for (size_t i = 0; i < testVals.size(); i++) {
        const char* str = testVals[i];
        int val;
        bool found;
        std::tie(val, found) = trie.find(str);
        vals.push_back(val);
        founds.push_back(found);
    }

    for (size_t i = 0; i < testVals.size(); i++) {
        int expected = expectedVals[i];
        if (expected == 0) {
            if (founds[i]) {
                testf("Found value for \"%s\", despite not being added", testVals[i]);
                return false;
            }
            continue;
        }
        if (!founds[i]) {
            testf("Failed to find value for \"%s\", despite being added", testVals[i]);
            return false;
        }
        if (vals[i] != expected) {
            testf("Got back %d instead of expected %d for \"%s\"", vals[i], expected, testVals[i]);
            return false;
        }
    }

    return true;
}

bool test_pathtrie_collision() {
    PathTrie<int> trie;

    std::array<const char*, 8> paths = {{"aaaa", "aaa", "aaaa", "aaa", "a*", "aaa*", "aaaa*", "aaaaa*"}};
    std::array<bool, 8> expected = {{true, true, false, false, false, false, false, true}};

    for (size_t i = 0; i < paths.size(); i++) {
        const char* path = paths[i];
        bool res = trie.addNode(path, 1);

        if (res != expected[i]) {
            testf("Got %d instead of expected %d when adding node %d - %s", (int)res, (int)expected[i], i, path);
        }
    }

    return true;
}
