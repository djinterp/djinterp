/******************************************************************************
* djinterp [test]                                        fn_builder_tests.hpp
*
*   DTest declarations and shared fixtures for the fn_builder.hpp unit-test
* suite.  Declares one section function per like-group semantic section of
* fn_builder.hpp (builder creation, the transforming fluent operations, the
* slicing / reordering operations, the execution overloads, the terminal
* operations, type erasure via boxed_fn_builder, and the SFINAE traits) plus
* the module identity / run-all entry points consumed by the session runner.
* Definitions live in the per-section .cpp files; this header carries only
* declarations and the helper callables the sections share.
*
*   Each section function has the framework's leaf signature
* `void(test::test_handler&)` and records its findings with
* test::record_assertion, so the totals roll up through the runner exactly
* like every other DTest module.
*
*
* TABLE OF CONTENTS
* =================
* I.    SHARED HELPER CALLABLES
* II.   SECTION FUNCTION DECLARATIONS
* III.  MODULE ENTRY POINTS
*
*
* path:      /inc/djinterp/test/fn_builder_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TEST_FN_BUILDER_TESTS_
#define DJINTERP_TEST_FN_BUILDER_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../core/functional/fn_builder.hpp"
#include "./test_common.hpp"
#include "./test_handler.hpp"
#include "./test_defaults.hpp"
#include "./test_runner.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER CALLABLES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Named function objects (rather than lambdas) so the same types can be used
// in both runtime invocations and unevaluated trait/concept contexts at every
// supported language standard. int is the canonical builder element type.

// fn_double / fn_add_one
//   helpers: int -> int mappers.
struct fn_double
{
    int operator()(int _x) const
    {
        return _x * 2;
    }
};

struct fn_add_one
{
    int operator()(int _x) const
    {
        return _x + 1;
    }
};

// fn_to_string
//   helper: int -> std::string, a type-changing mapper whose result is NOT
// convertible to bool (so it is a mapper but not a predicate).
struct fn_to_string
{
    std::string operator()(int _x) const
    {
        return std::to_string(_x);
    }
};

// fn_is_even / fn_is_positive
//   helpers: int -> bool predicates.
struct fn_is_even
{
    bool operator()(int _x) const
    {
        return (_x % 2) == 0;
    }
};

struct fn_is_positive
{
    bool operator()(int _x) const
    {
        return _x > 0;
    }
};

// fn_add
//   helper: (int, int) -> int, the binary step for fold (acc, elem).
struct fn_add
{
    int operator()(int _acc, int _x) const
    {
        return _acc + _x;
    }
};

// fn_less / fn_greater
//   helpers: (int, int) -> bool comparators for sorted().
struct fn_less
{
    bool operator()(int _a, int _b) const
    {
        return _a < _b;
    }
};

struct fn_greater
{
    bool operator()(int _a, int _b) const
    {
        return _a > _b;
    }
};

// fn_explode
//   helper: int -> std::vector<int> yielding { x, x + 100 }, for flat_map.
struct fn_explode
{
    std::vector<int> operator()(int _x) const
    {
        std::vector<int> v;
        v.push_back(_x);
        v.push_back(_x + 100);
        return v;
    }
};

// fn_string_only
//   helper: callable only with std::string; not callable with int. A
// negative case for is_fn_builder_mapper / is_fn_builder_predicate over int.
struct fn_string_only
{
    bool operator()(const std::string&) const
    {
        return true;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

void test_creation(test::test_handler&);

void test_map(test::test_handler&);
void test_filter(test::test_handler&);
void test_flat_map(test::test_handler&);

void test_take_skip(test::test_handler&);
void test_distinct(test::test_handler&);
void test_reverse_sort(test::test_handler&);

void test_execute(test::test_handler&);

void test_fold(test::test_handler&);
void test_count_any(test::test_handler&);

void test_boxed(test::test_handler&);

void test_traits(test::test_handler&);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////

extern const test::test_module_info fn_builder_module_info;

void fn_builder_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FN_BUILDER_TESTS_
