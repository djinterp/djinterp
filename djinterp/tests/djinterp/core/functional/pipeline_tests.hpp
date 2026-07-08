/******************************************************************************
* djinterp [test]                                          pipeline_tests.hpp
*
*   DTest declarations and shared fixtures for the pipeline.hpp unit-test
* suite.  Declares one section function per like-group semantic section of
* pipeline.hpp (creation, the transforming operations, slicing / reordering,
* the aggregating operations, the query operations, accessors / iteration,
* error propagation, the SFINAE traits, and the convenience factory) plus the
* module identity / run-all entry points consumed by the session runner.
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
* path:      /inc/djinterp/test/pipeline_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TEST_PIPELINE_TESTS_
#define DJINTERP_TEST_PIPELINE_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../core/functional/pipeline.hpp"
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
// supported language standard. int is the canonical pipeline element type.

// fn_double
//   helper: int -> int mapper.
struct fn_double
{
    int operator()(int _x) const
    {
        return _x * 2;
    }
};

// fn_to_string
//   helper: int -> std::string, a type-changing mapper. Its result is NOT
// convertible to bool, so it is a mapper but not a predicate.
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
//   helper: (int, int) -> int, the binary step for fold / reduce.
struct fn_add
{
    int operator()(int _a, int _b) const
    {
        return _a + _b;
    }
};

// fn_mul
//   helper: (int, int) -> int, a binary combiner for zip_with.
struct fn_mul
{
    int operator()(int _a, int _b) const
    {
        return _a * _b;
    }
};

// fn_less
//   helper: (int, int) -> bool comparator for sorted().
struct fn_less
{
    bool operator()(int _a, int _b) const
    {
        return _a < _b;
    }
};

// fn_greater
//   helper: (int, int) -> bool comparator for a descending sort.
struct fn_greater
{
    bool operator()(int _a, int _b) const
    {
        return _a > _b;
    }
};

// fn_eq_mod10
//   helper: (int, int) -> bool, equality modulo 10, for distinct(eq).
struct fn_eq_mod10
{
    bool operator()(int _a, int _b) const
    {
        return (_a % 10) == (_b % 10);
    }
};

// fn_parity
//   helper: int -> int, a key function (0 for even, 1 for odd) for group_by.
struct fn_parity
{
    int operator()(int _x) const
    {
        return _x & 1;
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
// negative case for is_pipeline_mapper / is_pipeline_predicate over int.
struct fn_string_only
{
    bool operator()(const std::string&) const
    {
        return true;
    }
};

// accumulate_into
//   helper: a for_each consumer that adds each element into an external
// sink, so the side effect can be observed after iteration.
struct accumulate_into
{
    long* sink;

    explicit accumulate_into(long* _sink)
        : sink(_sink)
    {
    }

    void operator()(int _x) const
    {
        *sink += _x;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

void test_creation(test::test_handler&);

void test_map(test::test_handler&);
void test_filter(test::test_handler&);
void test_distinct(test::test_handler&);
void test_flat_map(test::test_handler&);

void test_take(test::test_handler&);
void test_skip(test::test_handler&);
void test_slice(test::test_handler&);
void test_reorder(test::test_handler&);

void test_fold(test::test_handler&);
void test_reduce(test::test_handler&);
void test_group_by(test::test_handler&);
void test_partition(test::test_handler&);
void test_zip_with(test::test_handler&);

void test_query(test::test_handler&);
void test_for_each(test::test_handler&);

void test_accessors(test::test_handler&);
void test_iteration(test::test_handler&);

void test_error_propagation(test::test_handler&);

void test_traits(test::test_handler&);

void test_factory(test::test_handler&);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////

extern const test::test_module_info pipeline_module_info;

void pipeline_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_PIPELINE_TESTS_
