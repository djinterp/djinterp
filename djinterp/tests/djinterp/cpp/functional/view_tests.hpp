/*******************************************************************************
* djinterp [test]                                                 view_tests.hpp
*
*   DTest declarations and shared fixtures for the view.hpp unit-test suite.
* Declares one section function per like-group semantic section of view.hpp
* (the SFINAE traits & concepts, the fundamental views, the source views, the
* basic and combining adapter views, the pipeline operators, and the terminal
* operators) plus the module identity / run-all entry points consumed by the
* session runner.  Definitions live in the per-section .cpp files; this header
* carries only declarations and the helper callables the sections share.
*
*   Each section function is a `bool()` unit test: it records its findings
* on a local verdict with this header's record_assertion and returns it.
* The spec provider lists the functions for run_module, so the suite runs
* and reports like every other DTest module.
*
*   Adapter / terminal factory functions live in djinterp::views and at
* djinterp scope respectively; the tests, being in djinterp::testing, reach
* the former as djinterp::xxx and the latter unqualified.
*
*
* path:      /tests/djinterp/cpp/functional/view_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED HELPER CALLABLES
      -----------------------

II.   SECTION FUNCTION DECLARATIONS
      -----------------------------

III.  MODULE ENTRY POINTS
      -------------------
*/

#ifndef DJINTERP_TEST_VIEW_TESTS_
#define DJINTERP_TEST_VIEW_TESTS_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/view.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// record_assertion
//   function: one check of a converted body: on failure it prints what was
// expected and clears the body's verdict.  (The bodies used DTest's retired
// handler API, record_assertion(handler, condition, what).)
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what
)
{
    if (!_condition)
    {
        std::printf("    ! %s\n", _what);
        _ok = false;
    }

    return;
}

// record_assertion (with detail)
//   function: the five-argument form the bodies also used: the detail and
// the extra text are printed with a failure.
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what,
    const char* _detail,
    const char* _extra = nullptr
)
{
    if (!_condition)
    {
        std::printf("    ! %s (%s%s%s)\n",
                    _what,
                    _detail ? _detail : "",
                    _extra ? "; " : "",
                    _extra ? _extra : "");
        _ok = false;
    }

    return;
}


// DUnitTestTally
//   struct: kept for the converted bodies' run_unit_test calls; a body's
// verdict is its bool now, so the tally holds nothing.
struct DUnitTestTally
{
};

// run_unit_test
//   function: runs one named unit inside a converted body, and prints its
// name when it was the unit that first failed.
template<typename Fn>
void run_unit_test(
    bool&           _ok,
    DUnitTestTally& _tally,
    const char*     _name,
    Fn              _unit
)
{
    const bool before = _ok;

    (void)_tally;
    _unit();

    if ( (before) &&
         (!_ok) )
    {
        std::printf("    [FAIL] %s\n", _name);
    }

    return;
}

// D_VIEW_CHECK
//   macro: one check of a converted body, its text the message.
#define D_VIEW_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER CALLABLES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Named function objects (rather than lambdas) so the same types can be used
// in both runtime invocations and unevaluated trait/concept contexts at every
// supported language standard.

// DFnDouble
//   helper: int -> int, doubles its argument (transform).
struct DFnDouble
{
    int operator()(
        int _x
    ) const
    {
        return _x * 2;
    }
};

// DFnToDouble
//   helper: int -> double, a type-changing transform.
struct DFnToDouble
{
    double operator()(
        int _x
    ) const
    {
        return static_cast<double>(_x) + 0.5;
    }
};

// fn_is_even
//   helper: int -> bool predicate.
struct fn_is_even
{
    bool operator()(
        int _x
    ) const
    {
        return (_x % 2) == 0;
    }
};

// DFnLessThan5
//   helper: int -> bool predicate, used for take_while / drop_while.
struct DFnLessThan5
{
    bool operator()(
        int _x
    ) const
    {
        return _x < 5;
    }
};

// fn_is_positive
//   helper: int -> bool predicate.
struct fn_is_positive
{
    bool operator()(
        int _x
    ) const
    {
        return _x > 0;
    }
};

// DFnAlwaysTrue / DFnAlwaysFalse
//   helpers: constant predicates for take_while / drop_while edge cases.
struct DFnAlwaysTrue
{
    bool operator()(
        int
    ) const
    {
        return true;
    }
};

struct DFnAlwaysFalse
{
    bool operator()(
        int
    ) const
    {
        return false;
    }
};

// DFnAdd
//   helper: (int, int) -> int, the binary step for fold (acc, elem).
struct DFnAdd
{
    int operator()(
        int _acc,
        int _x
    ) const
    {
        return _acc + _x;
    }
};

// counter
//   helper: stateful nullary generator returning 1, 2, 3, ... on each call.
// Used to drive DGenerateView (which holds its function mutably).
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

    void operator()(
        int _x
    ) const
    {
        *sink += _x;
    }
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

bool test_core_traits();
bool test_adapter_terminal_traits();

bool test_ref_view();
bool test_owning_view();
bool test_iterator_pair_view();

bool test_iota();
bool test_repeat();
bool test_generate();
bool test_empty();
bool test_single();

bool test_transform();
bool test_filter();
bool test_take();
bool test_drop();
bool test_take_while();
bool test_drop_while();

bool test_enumerate();
bool test_zip();
bool test_concat();
bool test_reverse();
bool test_chunk();
bool test_stride();

bool test_pipeline_view_adapter();
bool test_pipeline_container_lift();
bool test_pipeline_chain();

bool test_to_vector();
bool test_to_container();
bool test_count();
bool test_fold();
bool test_for_each();
bool test_any_all_none();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// view_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
view_spec()
{
    return dt::module_spec{
        "view",
        "lazy, pull-based views and a pipeline operator for functional "
        "dataflow (C++)",
        {
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "test_core_traits",
                      "core traits",
                      &test_core_traits },
                    { "test_adapter_terminal_traits",
                      "adapter terminal traits",
                      &test_adapter_terminal_traits },
                }
            },
            dt::block_spec{
                "fundamental",
                "Fundamental",
                {
                    { "test_ref_view",
                      "ref view",
                      &test_ref_view },
                    { "test_owning_view",
                      "owning view",
                      &test_owning_view },
                    { "test_iterator_pair_view",
                      "iterator pair view",
                      &test_iterator_pair_view },
                }
            },
            dt::block_spec{
                "source",
                "Source",
                {
                    { "test_iota",
                      "iota",
                      &test_iota },
                    { "test_repeat",
                      "repeat",
                      &test_repeat },
                    { "test_generate",
                      "generate",
                      &test_generate },
                    { "test_empty",
                      "empty",
                      &test_empty },
                    { "test_single",
                      "single",
                      &test_single },
                }
            },
            dt::block_spec{
                "adapters_basic",
                "Adapters basic",
                {
                    { "test_transform",
                      "transform",
                      &test_transform },
                    { "test_filter",
                      "filter",
                      &test_filter },
                    { "test_take",
                      "take",
                      &test_take },
                    { "test_drop",
                      "drop",
                      &test_drop },
                    { "test_take_while",
                      "take while",
                      &test_take_while },
                    { "test_drop_while",
                      "drop while",
                      &test_drop_while },
                }
            },
            dt::block_spec{
                "adapters_combining",
                "Adapters combining",
                {
                    { "test_enumerate",
                      "enumerate",
                      &test_enumerate },
                    { "test_zip",
                      "zip",
                      &test_zip },
                    { "test_concat",
                      "concat",
                      &test_concat },
                    { "test_reverse",
                      "reverse",
                      &test_reverse },
                    { "test_chunk",
                      "chunk",
                      &test_chunk },
                    { "test_stride",
                      "stride",
                      &test_stride },
                }
            },
            dt::block_spec{
                "pipeline",
                "Pipeline",
                {
                    { "test_pipeline_view_adapter",
                      "pipeline view adapter",
                      &test_pipeline_view_adapter },
                    { "test_pipeline_container_lift",
                      "pipeline container lift",
                      &test_pipeline_container_lift },
                    { "test_pipeline_chain",
                      "pipeline chain",
                      &test_pipeline_chain },
                }
            },
            dt::block_spec{
                "terminal",
                "Terminal",
                {
                    { "test_to_vector",
                      "to vector",
                      &test_to_vector },
                    { "test_to_container",
                      "to container",
                      &test_to_container },
                    { "test_count",
                      "count",
                      &test_count },
                    { "test_fold",
                      "fold",
                      &test_fold },
                    { "test_for_each",
                      "for each",
                      &test_for_each },
                    { "test_any_all_none",
                      "any all none",
                      &test_any_all_none },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_VIEW_TESTS_
