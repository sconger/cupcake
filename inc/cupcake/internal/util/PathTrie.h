
#ifndef CUPCAKE_PATH_TRIE_H
#define CUPCAKE_PATH_TRIE_H

#include "cupcake/internal/text/String.h"
#include "cupcake/text/StringRef.h"

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>

namespace Cupcake {

/*
 * A trie that supports lookups on paths in the form of exact matches, or paths
 * that end in '*', matching anything past that point.
 */
template<typename T>
class PathTrie {
public:
    PathTrie() {}

    bool addNode(const StringRef prefix, const T& value) {
        StringRef prefixStr = prefix;
        bool regex = prefix.endsWith('*');
        if (regex) {
            prefixStr = prefixStr.substring(0, prefixStr.length() - 1);
        }
        return addNodeRecursive(&root, prefixStr, value, regex);
    }

    std::tuple<T, bool> find(const StringRef path) const {
        return findRecursive(&root, path);
    }

private:
    class TrieNode {
    public:
        TrieNode() :
            regexEnd(false),
            hasValue(false),
            value(),
            children()
        {}

        TrieNode(T val) :
            regexEnd(false),
            hasValue(true),
            value(val),
            children()
        {}

        String prefix;
        bool regexEnd;
        bool hasValue;
        T value;
        std::unique_ptr<TrieNode> children[256];
    };

    static bool addNodeRecursive(TrieNode* root, const StringRef insertPrefix, const T& value, bool regexEnd) {
        if (insertPrefix.length() == 0) {
            if (root->hasValue) {
                return false; // Duplicate
            }
            root->value = value;
            root->hasValue = true;
            root->regexEnd = regexEnd;
            return true;
        }

        unsigned char c = (unsigned char)insertPrefix.charAt(0);

        TrieNode* child = root->children[c].get();
        if (child) {
            bool anyDiff = false;
            size_t diffIndex;
            size_t diffEnd = std::min(insertPrefix.length(), child->prefix.length());

            for (diffIndex = 0; diffIndex < diffEnd; diffIndex++) {
                if (child->prefix[diffIndex] != insertPrefix[diffIndex]) {
                    anyDiff = true;
                    break;
                }
            }

            if (anyDiff) {
                std::unique_ptr<TrieNode> commonNode(new TrieNode());
                std::unique_ptr<TrieNode> insertedNode(new TrieNode(value));

                commonNode->prefix = insertPrefix.substring(0, diffIndex);
                child->prefix = child->prefix.substring(diffIndex);

                insertedNode->prefix = insertPrefix.substring(diffIndex);
                insertedNode->regexEnd = regexEnd;

                commonNode->children[(unsigned char)child->prefix[0]].swap(root->children[c]); // need to assign the unique_ptr
                commonNode->children[(unsigned char)insertedNode->prefix[0]].swap(insertedNode);
                root->children[c].swap(commonNode);
            } else {
                if (child->prefix.length() <= insertPrefix.length()) {
                    return addNodeRecursive(child, insertPrefix.substring(child->prefix.length()), value, regexEnd);
                }

                // Regex would be a collision here
                if (regexEnd) {
                    return false;
                }

                // Create a new shorter node with the common prefix portion
                std::unique_ptr<TrieNode> shorterNode(new TrieNode(value));
                shorterNode->prefix = child->prefix.substring(0, insertPrefix.length());

                // Make the current child the not-common suffix, and make it a child of the shorter node
                child->prefix = child->prefix.substring(insertPrefix.length(), child->prefix.length());
                shorterNode->children[(unsigned char)child->prefix[0]].swap(root->children[c]); // need to assign the unique_ptr

                // Replace the current child with the shorter node
                root->children[c].swap(shorterNode);
            }

        } else {
            std::unique_ptr<TrieNode> newChild(new TrieNode(value));
            newChild->prefix = insertPrefix;
            newChild->regexEnd = regexEnd;
            root->children[c].swap(newChild);
        }

        return true;
    }

    static std::tuple<T, bool> findRecursive(const TrieNode* root, const StringRef pathSuffix) {
        if (pathSuffix.length() == 0) {
            if (!root->hasValue) {
                return std::make_tuple(T(), false);
            }
            return std::make_tuple(root->value, true);
        }

        if (root->regexEnd) {
            return std::make_tuple(root->value, true);
        }

        unsigned char c = (unsigned char)pathSuffix[0];
        TrieNode* child = root->children[c].get();
        if (!child || child->prefix.length() > pathSuffix.length()) {
            return std::make_tuple(T(), false);
        }

        if (!pathSuffix.startsWith(child->prefix)) {
            return std::make_tuple(T(), false);
        }

        return findRecursive(child, pathSuffix.substring(child->prefix.length()));
    }

    TrieNode root;
};

}

#endif
