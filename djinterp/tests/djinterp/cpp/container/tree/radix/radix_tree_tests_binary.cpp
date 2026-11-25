/*******************************************************************************
* djinterp [cpp][container]                          radix_tree_tests_binary.cpp
*
*   binary_radix_tree.hpp: the bit-indexed tree, dynamic and fixed, on 32-bit
* keys.
*
*
* path:      /tests/djinterp/cpp/container/tree/radix/radix_tree_tests_binary.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "radix_tree_tests.hpp"  // D_RX_CHECK, the declarations
#include <djinterp/cpp/container/tree/radix/binary_radix_tree.hpp>  // under test

// std
#include <new>  // std::bad_alloc


NS_DJINTERP
NS_TESTING

namespace
{
    using DRxTree = DDynBinaryRadixTree<re_std::uint32_t, int>;

    // keys
    //   fixture: keys that share long prefixes and keys that share none.
    const re_std::uint32_t keys[] = {
        0x00000000u, 0x00000001u, 0x0000FFFFu, 0x80000000u,
        0x80000001u, 0xC0A80001u, 0xC0A80002u, 0xFFFFFFFFu
    };
    const std::size_t key_count = sizeof(keys) / sizeof(keys[0]);
}

/*
tests_radix_tree_binary_insert_find_erase
  each key, in and out
  Tests the following:
  - an empty tree finds nothing
  - each new key inserts (true), and is found with its value
  - a second insert of a key adds no entry; try_insert leaves the value
  - erase removes exactly its key; a missing key erases nothing
*/
bool tests_radix_tree_binary_insert_find_erase()
{
    DRxTree     tree;
    std::size_t i;

    D_RX_CHECK(tree.empty());
    D_RX_CHECK(tree.find(keys[0]) == nullptr);

    for (i = 0; i < key_count; ++i)
    {
        D_RX_CHECK(tree.insert(keys[i], static_cast<int>(i)));
    }

    D_RX_CHECK(tree.size() == key_count);

    for (i = 0; i < key_count; ++i)
    {
        D_RX_CHECK(tree.contains(keys[i]));
        D_RX_CHECK(*tree.find(keys[i]) == static_cast<int>(i));
        D_RX_CHECK(tree.at(keys[i]) == static_cast<int>(i));
    }

    D_RX_CHECK(!tree.contains(0x00000002u));
    D_RX_CHECK(!tree.try_insert(keys[3], 99));
    D_RX_CHECK(*tree.find(keys[3]) == 3);
    tree.insert(keys[3], 33);
    D_RX_CHECK(tree.size() == key_count);

    D_RX_CHECK(tree.erase(keys[5]));
    D_RX_CHECK(!tree.contains(keys[5]));
    D_RX_CHECK(tree.contains(keys[6]));
    D_RX_CHECK(!tree.erase(keys[5]));
    D_RX_CHECK(!tree.erase(0x12345678u));
    D_RX_CHECK(tree.size() == (key_count - 1));

    tree.clear();
    D_RX_CHECK(tree.empty());
    D_RX_CHECK(!tree.contains(keys[0]));

    return true;
}

/*
tests_radix_tree_binary_structure
  compression and the round trip
  Tests the following:
  - after the inserts the tree is compressed and every key round-trips
  - after erasing every other key it is still compressed: erase merges
    back the node a dropped leaf leaves with one child
  - a copy holds the same keys
*/
bool tests_radix_tree_binary_structure()
{
    DRxTree     tree;
    std::size_t i;

    for (i = 0; i < key_count; ++i)
    {
        tree.insert(keys[i], static_cast<int>(i));
    }

    D_RX_CHECK(tree.is_compressed());
    D_RX_CHECK(tree.round_trips());

    for (i = 0; i < key_count; i += 2)
    {
        D_RX_CHECK(tree.erase(keys[i]));
    }

    D_RX_CHECK(tree.is_compressed());
    D_RX_CHECK(tree.round_trips());
    D_RX_CHECK(tree.size() == (key_count / 2));

    DRxTree copy(tree);

    for (i = 1; i < key_count; i += 2)
    {
        D_RX_CHECK(copy.contains(keys[i]));
    }

    D_RX_CHECK(!copy.contains(keys[0]));

    return true;
}

/*
tests_radix_tree_binary_fixed
  the fixed tree, filled past its slab
  Tests the following:
  - its capacity is the template's: nodes, the root among them
  - the insert that does not fit throws bad_alloc, as documented
  - the tree is then the tree it was: every key that fit, none that did
    not, compressed and round-tripping (until 2026.11.23 a split that ran
    out after its first node kept that node: a one-child node, and a slot
    of the slab lost)
  - so the last key that fit can be erased and inserted again
*/
bool tests_radix_tree_binary_fixed()
{
    DFixedBinaryRadixTree<re_std::uint32_t, int, 8> tree;
    std::size_t                                    inserted = 0;
    bool                                           threw    = false;

    D_RX_CHECK(tree.capacity() == 8);

    while ( (inserted < key_count) && !threw )
    {
        try
        {
            tree.insert(keys[inserted], static_cast<int>(inserted));
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
    D_RX_CHECK(!tree.contains(keys[inserted]));
    D_RX_CHECK(tree.is_compressed());
    D_RX_CHECK(tree.round_trips());

    for (std::size_t i = 0; i < inserted; ++i)
    {
        D_RX_CHECK(tree.contains(keys[i]));
    }

    D_RX_CHECK(tree.erase(keys[inserted - 1]));
    D_RX_CHECK(tree.insert(keys[inserted - 1], 0));
    D_RX_CHECK(tree.size() == inserted);

    return true;
}

NS_END  // testing
NS_END  // djinterp
