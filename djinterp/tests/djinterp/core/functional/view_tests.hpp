/******************************************************************************
* djinterp [test]                                              view_tests.hpp
*
*   DTest declarations and shared fixtures for the view.hpp unit-test suite.
* Declares one section function per like-group semantic section of view.hpp
* (the SFINAE traits & concepts, the fundamental views, the source views, the
* basic and combining adapter views, the pipeline operators, and the terminal
* operators) plus the module identity / run-all entry points consumed by the
* session runner.  Definitions live in the per-section .cpp files; this header
* carries only declarations and the helper callables the sections share.
*
*   Each section function has the framework's leaf signature
* `void(test::test_handler&)` and records its findings with
* test::record_assertion, so the totals roll up through the runner exactly
* like every other DTest module.
*
*   Adapter / terminal factory functions live in djinterp::views and at
* djinterp scope respectively; the tests, being in djinterp::testing, reach
* the former as views::xxx and the latter unqualified.
*
*
* TABLE OF CONTENTS
* =================
* I.    SHARED HELPER CALLABLES
* II.   SECTION FUNCTION DECLARATIONS
* III.  MODULE ENTRY POINTS
*
*
* path:      /inc/djinterp/test/view_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TEST_VIEW_TESTS_
#define DJINTERP_TEST_VIEW_TESTS_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../core/functional/view.hpp"
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
// supported language standard.

// fn_double
//   helper: int -> int, doubles its argument (transform).
struct fn_double
{
    int operator()(int _x) const
    {
        return _x * 2;
    }
};

// fn_to_double
//   helper: int -> double, a type-changing transform.
struct fn_to_double
{
    double operator()(int _x) const
    {
        return static_cast<double>(_x) + 0.5;
    }
};

// fn_is_even
//   helper: int -> bool predicate.
struct fn_is_even
{
    bool operator()(int _x) const
    {
        return (_x % 2) == 0;
    }
};

// fn_less_than_5
//   helper: int -> bool predicate, used for take_while / drop_while.
struct fn_less_than_5
{
    bool operator()(int _x) const
    {
        return _x < 5;
    }
};

// fn_is_positive
//   helper: int -> bool predicate.
struct fn_is_positive
{
    bool operator()(int _x) const
    {
        return _x > 0;
    }
};

// fn_always_true / fn_always_false
//   helpers: constant predicates for take_while / drop_while edge cases.
struct fn_always_true
{
    bool operator()(int) const
    {
        return true;
    }
};

struct fn_always_false
{
    bool operator()(int) const
    {
        return false;
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

// counter
//   helper: stateful nullary generator returning 1, 2, 3, ... on each call.
// Used to drive generate_view (which holds its function mutably).
struct counter
{
    int m_value;

    counter()
        : m_value(0)
    {
    }

    int operator()()
    {
        return ++m_value;
    }
};

// accumulate_into
//   helper: a for_each consumer that adds each element into an external
// sink, so the side effect can be observed after the drain.
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

void test_core_traits(test::test_handler&);
void test_adapter_terminal_traits(test::test_handler&);

void test_ref_view(test::test_handler&);
void test_owning_view(test::test_handler&);
void test_iterator_pair_view(test::test_handler&);

void test_iota(test::test_handler&);
void test_repeat(test::test_handler&);
void test_generate(test::test_handler&);
void test_empty(test::test_handler&);
void test_single(test::test_handler&);

void test_transform(test::test_handler&);
void test_filter(test::test_handler&);
void test_take(test::test_handler&);
void test_drop(test::test_handler&);
void test_take_while(test::test_handler&);
void test_drop_while(test::test_handler&);

void test_enumerate(test::test_handler&);
void test_zip(test::test_handler&);
void test_concat(test::test_handler&);
void test_reverse(test::test_handler&);
void test_chunk(test::test_handler&);
void test_stride(test::test_handler&);

void test_pipeline_view_adapter(test::test_handler&);
void test_pipeline_container_lift(test::test_handler&);
void test_pipeline_chain(test::test_handler&);

void test_to_vector(test::test_handler&);
void test_to_container(test::test_handler&);
void test_count(test::test_handler&);
void test_fold(test::test_handler&);
void test_for_each(test::test_handler&);
void test_any_all_none(test::test_handler&);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////

extern const test::test_module_info view_module_info;

void view_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_VIEW_TESTS_
