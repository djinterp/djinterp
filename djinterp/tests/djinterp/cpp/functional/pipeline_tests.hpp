/*******************************************************************************
* djinterp [test]                                             pipeline_tests.hpp
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
*   Each section function is a `bool()` unit test: it records its findings
* on a local verdict with this header's record_assertion and returns it.
* The spec provider lists the functions for run_module, so the suite runs
* and reports like every other DTest module.
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests.hpp
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

#ifndef DJINTERP_TEST_PIPELINE_TESTS_
#define DJINTERP_TEST_PIPELINE_TESTS_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/pipeline.hpp"
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

// D_PIPELINE_CHECK
//   macro: one check of a converted body, its text the message.
#define D_PIPELINE_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER CALLABLES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Named function objects (rather than lambdas) so the same types can be used
// in both runtime invocations and unevaluated trait/concept contexts at every
// supported language standard. int is the canonical pipeline element type.

// DFnDouble
//   helper: int -> int mapper.
struct DFnDouble
{
    int operator()(
        int _x
    ) const
    {
        return _x * 2;
    }
};

// fn_to_string
//   helper: int -> std::string, a type-changing mapper. Its result is NOT
// convertible to bool, so it is a mapper but not a predicate.
struct fn_to_string
{
    std::string operator()(
        int _x
    ) const
    {
        return std::to_string(_x);
    }
};

// fn_is_even / fn_is_positive
//   helpers: int -> bool predicates.
struct fn_is_even
{
    bool operator()(
        int _x
    ) const
    {
        return (_x % 2) == 0;
    }
};

struct fn_is_positive
{
    bool operator()(
        int _x
    ) const
    {
        return _x > 0;
    }
};

// DFnAdd
//   helper: (int, int) -> int, the binary step for fold / reduce.
struct DFnAdd
{
    int operator()(
        int _a,
        int _b
    ) const
    {
        return _a + _b;
    }
};

// DFnMul
//   helper: (int, int) -> int, a binary combiner for zip_with.
struct DFnMul
{
    int operator()(
        int _a,
        int _b
    ) const
    {
        return _a * _b;
    }
};

// DFnLess
//   helper: (int, int) -> bool comparator for sorted().
struct DFnLess
{
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return _a < _b;
    }
};

// DFnGreater
//   helper: (int, int) -> bool comparator for a descending sort.
struct DFnGreater
{
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return _a > _b;
    }
};

// DFnEqMod10
//   helper: (int, int) -> bool, equality modulo 10, for distinct(eq).
struct DFnEqMod10
{
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return (_a % 10) == (_b % 10);
    }
};

// DFnParity
//   helper: int -> int, a key function (0 for even, 1 for odd) for group_by.
struct DFnParity
{
    int operator()(
        int _x
    ) const
    {
        return _x & 1;
    }
};

// DFnExplode
//   helper: int -> std::vector<int> yielding { x, x + 100 }, for flat_map.
struct DFnExplode
{
    std::vector<int> operator()(
        int _x
    ) const
    {
        std::vector<int> v;
        v.push_back(_x);
        v.push_back(_x + 100);
        return v;
    }
};

// DFnStringOnly
//   helper: callable only with std::string; not callable with int. A
// negative case for is_pipeline_mapper / is_pipeline_predicate over int.
struct DFnStringOnly
{
    bool operator()(
        const std::string&
    ) const
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

bool test_creation();

bool test_map();
bool test_filter();
bool test_distinct();
bool test_flat_map();

bool test_take();
bool test_skip();
bool test_slice();
bool test_reorder();

bool test_fold();
bool test_reduce();
bool test_group_by();
bool test_partition();
bool test_zip_with();

bool test_query();
bool test_for_each();

bool test_accessors();
bool test_iteration();

bool test_error_propagation();

bool test_traits();

bool test_factory();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// pipeline_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
pipeline_spec()
{
    return dt::module_spec{
        "pipeline",
        "template function pipeline for chaining operations (C++)",
        {
            dt::block_spec{
                "creation",
                "Creation",
                {
                    { "test_creation",
                      "creation",
                      &test_creation },
                }
            },
            dt::block_spec{
                "transform",
                "Transform",
                {
                    { "test_map",
                      "map",
                      &test_map },
                    { "test_filter",
                      "filter",
                      &test_filter },
                    { "test_distinct",
                      "distinct",
                      &test_distinct },
                    { "test_flat_map",
                      "flat map",
                      &test_flat_map },
                }
            },
            dt::block_spec{
                "slicing",
                "Slicing",
                {
                    { "test_take",
                      "take",
                      &test_take },
                    { "test_skip",
                      "skip",
                      &test_skip },
                    { "test_slice",
                      "slice",
                      &test_slice },
                    { "test_reorder",
                      "reorder",
                      &test_reorder },
                }
            },
            dt::block_spec{
                "aggregate",
                "Aggregate",
                {
                    { "test_fold",
                      "fold",
                      &test_fold },
                    { "test_reduce",
                      "reduce",
                      &test_reduce },
                    { "test_group_by",
                      "group by",
                      &test_group_by },
                    { "test_partition",
                      "partition",
                      &test_partition },
                    { "test_zip_with",
                      "zip with",
                      &test_zip_with },
                }
            },
            dt::block_spec{
                "query",
                "Query",
                {
                    { "test_query",
                      "query",
                      &test_query },
                    { "test_for_each",
                      "for each",
                      &test_for_each },
                }
            },
            dt::block_spec{
                "accessors",
                "Accessors",
                {
                    { "test_accessors",
                      "accessors",
                      &test_accessors },
                    { "test_iteration",
                      "iteration",
                      &test_iteration },
                }
            },
            dt::block_spec{
                "errors",
                "Errors",
                {
                    { "test_error_propagation",
                      "error propagation",
                      &test_error_propagation },
                }
            },
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "test_traits",
                      "traits",
                      &test_traits },
                }
            },
            dt::block_spec{
                "factory",
                "Factory",
                {
                    { "test_factory",
                      "factory",
                      &test_factory },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_PIPELINE_TESTS_
