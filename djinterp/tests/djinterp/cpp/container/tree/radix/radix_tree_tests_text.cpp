/*******************************************************************************
* djinterp [cpp][container]                            radix_tree_tests_text.cpp
*
*   text_radix_tree.hpp: the byte-indexed tree and its child map.
*
*
* path:      /tests/djinterp/cpp/container/tree/radix/radix_tree_tests_text.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "radix_tree_tests.hpp"  // D_RX_CHECK, the declarations
#include <djinterp/cpp/container/tree/radix/text_radix_tree.hpp>  // under test

// std
#include <new>     // std::bad_alloc
#include <string>  // std::string
#include <vector>  // std::vector


NS_DJINTERP
NS_TESTING

namespace
{
    using DRxText = DDynTextRadixTree<int>;

    // words
    //   fixture: keys sharing prefixes, one a prefix of others ("rom").
    const char* const words[] = {
        "romane", "romanus", "romulus", "rubens", "ruber", "rubicon",
        "rubicundus", "rom"
    };
    const std::size_t word_count = sizeof(words) / sizeof(words[0]);

    // filled
    //   fixture: a tree holding every word, each word's index its value.
    DRxText filled()
    {
        DRxText     tree;
        std::size_t i;

        for (i = 0; i < word_count; ++i)
        {
            tree.insert(words[i], static_cast<int>(i));
        }

        return tree;
    }
}

/*
tests_radix_tree_text_insert_find_erase
  each key, in and out
  Tests the following:
  - each word inserts and is found with its value, "rom" too, though it is
    a prefix of three others
  - a prefix that was not inserted ("ru", "roma") is not a key
  - erasing "rom" leaves "romane"; erasing "romane" leaves "rom" gone too
*/
bool tests_radix_tree_text_insert_find_erase()
{
    DRxText     tree = filled();
    std::size_t i;

    D_RX_CHECK(tree.size() == word_count);

    for (i = 0; i < word_count; ++i)
    {
        D_RX_CHECK(tree.contains(words[i]));
        D_RX_CHECK(*tree.find(words[i]) == static_cast<int>(i));
    }

    D_RX_CHECK(!tree.contains("ru"));
    D_RX_CHECK(!tree.contains("roma"));
    D_RX_CHECK(!tree.contains("romanes"));
    D_RX_CHECK(!tree.contains(""));

    D_RX_CHECK(!tree.try_insert("rom", 99));
    D_RX_CHECK(tree.at("rom") == 7);

    D_RX_CHECK(tree.erase("rom"));
    D_RX_CHECK(!tree.contains("rom"));
    D_RX_CHECK(tree.contains("romane"));
    D_RX_CHECK(tree.erase("romane"));
    D_RX_CHECK(tree.contains("romanus"));
    D_RX_CHECK(!tree.erase("romane"));
    D_RX_CHECK(tree.size() == (word_count - 2));

    return true;
}

/*
tests_radix_tree_text_prefixes
  the prefix queries
  Tests the following:
  - has_prefix holds for a stored key's prefix, and not for a stranger
  - count_with_prefix counts the keys under a prefix, the prefix itself
    when it is a key
  - longest_match is the length of the longest stored key that is a
    prefix of the probe, 0 when none is
*/
bool tests_radix_tree_text_prefixes()
{
    DRxText tree = filled();

    D_RX_CHECK(tree.has_prefix("rub"));
    D_RX_CHECK(tree.has_prefix("romanu"));
    D_RX_CHECK(!tree.has_prefix("rx"));

    D_RX_CHECK(tree.count_with_prefix("rom") == 4);
    D_RX_CHECK(tree.count_with_prefix("rub") == 4);
    D_RX_CHECK(tree.count_with_prefix("rubic") == 2);
    D_RX_CHECK(tree.count_with_prefix("z") == 0);

    D_RX_CHECK(tree.longest_match("romanesque") == 6);
    D_RX_CHECK(tree.longest_match("romeo") == 3);
    D_RX_CHECK(tree.longest_match("rub") == 0);

    return true;
}

/*
tests_radix_tree_text_structure
  the keys recovered from the structure
  Tests the following:
  - keys() holds every key once, in byte order
  - the tree is compressed and round-trips, before and after erasures
*/
bool tests_radix_tree_text_structure()
{
    DRxText                        tree = filled();
    const std::vector<std::string> keys = tree.keys();
    std::size_t                    i;

    D_RX_CHECK(keys.size() == word_count);

    for (i = 1; i < keys.size(); ++i)
    {
        D_RX_CHECK(keys[i - 1] < keys[i]);
    }

    D_RX_CHECK(keys.front() == "rom");
    D_RX_CHECK(tree.is_compressed());
    D_RX_CHECK(tree.round_trips());

    tree.erase("rubens");
    tree.erase("ruber");

    D_RX_CHECK(tree.is_compressed());
    D_RX_CHECK(tree.round_trips());

    return true;
}

/*
tests_radix_tree_text_child_map
  the child map, which keys a node's children by their first byte
  Tests the following:
  - set and get by byte; test and size follow
  - rank counts the set bytes below
  - erase removes one; sole names the one child that is left
*/
bool tests_radix_tree_text_child_map()
{
    int                 a = 1;
    int                 b = 2;
    int                 c = 3;
    DRadixChildMap<int> map;

    D_RX_CHECK(map.empty());

    map.set('m', &a);
    map.set('b', &b);
    map.set('z', &c);

    D_RX_CHECK(map.size() == 3);
    D_RX_CHECK(map.test('m'));
    D_RX_CHECK(!map.test('x'));
    D_RX_CHECK(map.get('b') == &b);
    D_RX_CHECK(map.get('x') == nullptr);
    D_RX_CHECK(map.rank('m') == 1);
    D_RX_CHECK(map.rank('z') == 2);

    map.erase('m');
    map.erase('b');

    D_RX_CHECK(map.size() == 1);
    D_RX_CHECK(map.sole() == &c);

    map.clear();
    D_RX_CHECK(map.empty());

    return true;
}

/*
tests_radix_tree_text_fixed
  the fixed tree, filled past its slab
  Tests the following:
  - the insert that does not fit throws bad_alloc, as documented
  - the tree is then the tree it was: every key that fit, none that did
    not, compressed and round-tripping (until 2026.11.23 a split that ran
    out after its lower half kept the split: a one-child node, and a slot
    of the slab lost)
  - so the last key that fit can be erased and inserted again
*/
bool tests_radix_tree_text_fixed()
{
    DFixedTextRadixTree<int, 8> tree;
    std::size_t                 inserted = 0;
    bool                        threw    = false;

    D_RX_CHECK(tree.capacity() == 8);

    while ( (inserted < word_count) && !threw )
    {
        try
        {
            tree.insert(words[inserted], static_cast<int>(inserted));
            ++inserted;
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }
    }

    D_RX_CHECK(threw);
    D_RX_CHECK(inserted > 1);
    D_RX_CHECK(tree.size() == inserted);
    D_RX_CHECK(!tree.contains(words[inserted]));
    D_RX_CHECK(tree.is_compressed());
    D_RX_CHECK(tree.round_trips());

    for (std::size_t i = 0; i < inserted; ++i)
    {
        D_RX_CHECK(tree.contains(words[i]));
    }

    D_RX_CHECK(tree.erase(words[inserted - 1]));
    D_RX_CHECK(tree.insert(words[inserted - 1], 0));
    D_RX_CHECK(tree.size() == inserted);

    return true;
}

NS_END  // testing
NS_END  // djinterp
