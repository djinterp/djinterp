/*******************************************************************************
* djinterp [cpp][container]                                 radix_tree_tests.hpp
*
*   Declarations, the per-suite assertion macro and the spec for the
* container/tree/radix/ unit tests, part of the tree floor: the key traits
* (radix_tree_common.hpp), the bit-indexed tree (binary_radix_tree.hpp) and
* the byte-indexed tree (text_radix_tree.hpp). Two faces, selected by
* DTEST_SPEC_MODE: normal mode (the section TUs) and spec mode (the runner,
* which exposes radix_tree_spec()). Each section includes its own header and
* nothing else of the radix trees, so each is compiled alone.
*
*   The section .cpp files that DEFINE these predicates:
*     radix_tree_tests_common.cpp  - radix_tree_common.hpp
*     radix_tree_tests_binary.cpp  - binary_radix_tree.hpp
*     radix_tree_tests_text.cpp    - text_radix_tree.hpp
*
*
* path:      /tests/djinterp/cpp/container/tree/radix/radix_tree_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_RADIX_TREE_TESTS_
#define DJINTERP_TESTS_RADIX_TREE_TESTS_ 1

// std
#include <cstdio>  // std::printf
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec (spec)
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest's entities (djinterp::test).
namespace dt = ::djinterp::test;


// radix_tree_check
//   function: routes one D_RX_CHECK evaluation: prints the failing
// expression and its place, and hands the condition back.
inline
bool radix_tree_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_RX_CHECK
//   macro: fail the current test unless the condition holds, printing the
// expression that failed. Variadic, so a condition may hold top-level commas.
#define D_RX_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::radix_tree_check(                           \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


// I.    radix_tree_common.hpp   (radix_tree_tests_common.cpp)
bool tests_radix_tree_common_key_kinds();
bool tests_radix_tree_common_key_bits();
bool tests_radix_tree_common_node_probes();

// II.   binary_radix_tree.hpp   (radix_tree_tests_binary.cpp)
bool tests_radix_tree_binary_insert_find_erase();
bool tests_radix_tree_binary_structure();
bool tests_radix_tree_binary_fixed();

// III.  text_radix_tree.hpp   (radix_tree_tests_text.cpp)
bool tests_radix_tree_text_insert_find_erase();
bool tests_radix_tree_text_prefixes();
bool tests_radix_tree_text_structure();
bool tests_radix_tree_text_child_map();
bool tests_radix_tree_text_fixed();


#ifdef DTEST_SPEC_MODE

// radix_tree_spec
//   function: the suite's description: one block per section, one row per
// tests_* body.
inline
dt::module_spec radix_tree_spec()
{
    return dt::module_spec{
        "radix_tree",
        "The radix trees' floor: the key traits, the bit-indexed tree and "
        "the byte-indexed tree, each instantiated and checked against what "
        "its header documents.",
        {
            dt::block_spec{
                "common",
                "radix_tree_common.hpp: which keys are text and which binary, "
                "their bits, and the node probes.",
                {
                    { "tests_radix_tree_common_key_kinds",
                      "is_text_key, is_binary_key and the key category of "
                      "strings, integers, pointers and others.",
                      &tests_radix_tree_common_key_kinds },
                    { "tests_radix_tree_common_key_bits",
                      "Each binary key's bit carrier is unsigned and as wide "
                      "as the key, and keeps its bits.",
                      &tests_radix_tree_common_key_bits },
                    { "tests_radix_tree_common_node_probes",
                      "is_radix_node asks for a value_type and children.",
                      &tests_radix_tree_common_node_probes },
                }
            },
            dt::block_spec{
                "binary",
                "binary_radix_tree.hpp, on 32-bit keys.",
                {
                    { "tests_radix_tree_binary_insert_find_erase",
                      "Each key inserted is found with its value until it is "
                      "erased; a second insert of a key adds nothing.",
                      &tests_radix_tree_binary_insert_find_erase },
                    { "tests_radix_tree_binary_structure",
                      "The tree stays compressed and every key round-trips, "
                      "through inserts and erases.",
                      &tests_radix_tree_binary_structure },
                    { "tests_radix_tree_binary_fixed",
                      "A full fixed tree throws bad_alloc and is the tree it "
                      "was: compressed, its keys, its slab whole.",
                      &tests_radix_tree_binary_fixed },
                }
            },
            dt::block_spec{
                "text",
                "text_radix_tree.hpp, on string keys.",
                {
                    { "tests_radix_tree_text_insert_find_erase",
                      "Each key inserted is found with its value until it is "
                      "erased, including keys that are prefixes of others.",
                      &tests_radix_tree_text_insert_find_erase },
                    { "tests_radix_tree_text_prefixes",
                      "has_prefix, count_with_prefix and longest_match.",
                      &tests_radix_tree_text_prefixes },
                    { "tests_radix_tree_text_structure",
                      "keys() recovers every key in byte order; the tree "
                      "stays compressed and round-trips.",
                      &tests_radix_tree_text_structure },
                    { "tests_radix_tree_text_child_map",
                      "DRadixChildMap's set, get, rank, erase and sole.",
                      &tests_radix_tree_text_child_map },
                    { "tests_radix_tree_text_fixed",
                      "A full fixed tree throws bad_alloc and is the tree it "
                      "was: compressed, its keys, its slab whole.",
                      &tests_radix_tree_text_fixed },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_RADIX_TREE_TESTS_
