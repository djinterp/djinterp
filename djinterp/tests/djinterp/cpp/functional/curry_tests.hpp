/*******************************************************************************
* djinterp [functional]                                          curry_tests.hpp
*
*   DTest declarations and shared fixtures for the curry.hpp unit-test
* suite.  Declares one section function per like-group semantic section of
* curry.hpp (internal machinery, predicate traits & concepts, curry
* factories, uncurrying, argument transformations, constant-valued
* combinators) plus the module identity / run-all entry points consumed by
* the session runner.  Definitions live in the per-section .cpp files; this
* header carries only declarations and the helper callable types the
* sections share.
*
*   Each section function is a `bool()` unit test: it records its findings
* on a local verdict with this header's record_assertion and returns it.
* The spec provider lists the functions for run_module, so the suite runs
* and reports like every other DTest module.
*
*
* path:      /tests/djinterp/cpp/functional/curry_tests.hpp
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

#ifndef DJINTERP_TESTING_FUNCTIONAL_CURRY_
#define DJINTERP_TESTING_FUNCTIONAL_CURRY_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/curry.hpp"
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

// D_CURRY_CHECK
//   macro: one check of a converted body, its text the message.
#define D_CURRY_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER CALLABLES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Named function objects (rather than lambdas) so the same types can be used
// in both runtime invocations and unevaluated trait contexts at every
// supported language standard.  Each carries an explicit, fixed arity so the
// SFINAE-driven invocation/extension branches of the curry machinery resolve
// deterministically.

// add2
//   helper: binary integer sum; exercises the arity-2 invoke path.
struct add2
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b
    ) const
    {
        return _a + _b;
    }
};

// add3
//   helper: ternary integer sum; the canonical curry target.
struct add3
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b,
        int _c
    ) const
    {
        return _a + _b + _c;
    }
};

// add4
//   helper: quaternary integer sum; drives the multi-argument extension
// branch (supplying two args at a time is insufficient until the fourth).
struct DAdd4
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b,
        int _c,
        int _d
    ) const
    {
        return _a + _b + _c + _d;
    }
};

// sub2
//   helper: order-sensitive binary difference; used to observe flip().
struct sub2
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b
    ) const
    {
        return _a - _b;
    }
};

// DNullarySeven
//   helper: zero-argument callable returning a fixed value; exercises the
// nullary invocation paths of curry and curry_n.
struct DNullarySeven
{
    D_CONSTEXPR
    int operator()() const
    {
        return 7;
    }
};

// DEchoInt
//   helper: unary callable returning its argument; exercises arity-1 invoke
// and degenerate single-argument uncurry.
struct DEchoInt
{
    D_CONSTEXPR
    int operator()(
        int _x
    ) const
    {
        return _x;
    }
};

// digits3
//   helper: ternary callable whose result depends on argument order, used to
// confirm flip() swaps only the first two arguments.
struct DDigits3
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b,
        int _c
    ) const
    {
        return (_a * 100) + (_b * 10) + _c;
    }
};

// is_positive
//   helper: unary predicate (returns bool).
struct is_positive
{
    D_CONSTEXPR
    bool operator()(
        int _x
    ) const
    {
        return _x > 0;
    }
};

// less_than
//   helper: binary predicate (returns bool).
struct less_than
{
    D_CONSTEXPR
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return _a < _b;
    }
};

// always_true
//   helper: nullary predicate (returns bool).
struct always_true
{
    D_CONSTEXPR
    bool operator()() const
    {
        return true;
    }
};

// returns_void
//   helper: unary callable returning void; a non-predicate (its result is
// not convertible to bool).
struct returns_void
{
    void operator()(
        int
    ) const
    {
        return;
    }
};

// DReturnsPointer
//   helper: unary callable returning a pointer; a predicate by the
// bool-convertibility rule.
struct DReturnsPointer
{
    D_CONSTEXPR
    void* operator()(
        int
    ) const
    {
        return nullptr;
    }
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One function per like-group semantic section of curry.hpp.  No comments
// adorn these declarations per the project style guide; see the matching
// definition in each section's .cpp for the per-function scope description.

bool test_internal_machinery();

bool test_predicate_traits();

bool test_curry_factory();
bool test_curry_n_factory();

bool test_uncurry();

bool test_flip();

bool test_identity();
bool test_always_constant();
bool test_never();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// curry_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
curry_spec()
{
    return dt::module_spec{
        "curry",
        "function currying, uncurrying, and related combinators (C++)",
        {
            dt::block_spec{
                "internal",
                "Internal",
                {
                    { "test_internal_machinery",
                      "Exercises the C++11 index-sequence fallback and the "
                      "std::apply stand-in that underpin every curry "
                      "invocation",
                      &test_internal_machinery },
                }
            },
            dt::block_spec{
                "predicate_traits",
                "Predicate traits",
                {
                    { "test_predicate_traits",
                      "predicate traits",
                      &test_predicate_traits },
                }
            },
            dt::block_spec{
                "curry",
                "Curry",
                {
                    { "test_curry_factory",
                      "Exercises the auto-arity curry() factory and every "
                      "branch of its DCurryHelper accumulator",
                      &test_curry_factory },
                    { "test_curry_n_factory",
                      "Exercises the explicit-arity curry_n<N>() factory, "
                      "both the counting curry_n_helper and its zero-arity "
                      "terminal specialization",
                      &test_curry_n_factory },
                }
            },
            dt::block_spec{
                "uncurry",
                "Uncurry",
                {
                    { "test_uncurry",
                      "Exercises uncurry() and both operator() overloads of "
                      "DUncurryHelper",
                      &test_uncurry },
                }
            },
            dt::block_spec{
                "flip",
                "Flip",
                {
                    { "test_flip",
                      "Exercises flip() and both operator() overloads of "
                      "DFlipHelper",
                      &test_flip },
                }
            },
            dt::block_spec{
                "combinators",
                "Combinators",
                {
                    { "test_identity",
                      "Exercises the identity function object "
                      "(DIdentityFnHelper)",
                      &test_identity },
                    { "test_always_constant",
                      "Exercises always() and its constant() alias "
                      "(DAlwaysHelper)",
                      &test_always_constant },
                    { "test_never",
                      "Exercises the never predicate primitive (DNeverHelper)",
                      &test_never },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_CURRY_
