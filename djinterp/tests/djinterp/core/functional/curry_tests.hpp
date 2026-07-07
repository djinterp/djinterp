/******************************************************************************
* djinterp [functional]                                        curry_tests.hpp
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
* path:      /tests/djinterp/core/functional/curry_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_CURRY_
#define DJINTERP_TESTING_FUNCTIONAL_CURRY_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../core/functional/curry.hpp"
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
// in both runtime invocations and unevaluated trait contexts at every
// supported language standard.  Each carries an explicit, fixed arity so the
// SFINAE-driven invocation/extension branches of the curry machinery resolve
// deterministically.

// add2
//   helper: binary integer sum; exercises the arity-2 invoke path.
struct add2
{
    D_CONSTEXPR int operator()(int _a, int _b) const
    {
        return _a + _b;
    }
};

// add3
//   helper: ternary integer sum; the canonical curry target.
struct add3
{
    D_CONSTEXPR int operator()(int _a, int _b, int _c) const
    {
        return _a + _b + _c;
    }
};

// add4
//   helper: quaternary integer sum; drives the multi-argument extension
// branch (supplying two args at a time is insufficient until the fourth).
struct add4
{
    D_CONSTEXPR int operator()(int _a, int _b, int _c, int _d) const
    {
        return _a + _b + _c + _d;
    }
};

// sub2
//   helper: order-sensitive binary difference; used to observe flip().
struct sub2
{
    D_CONSTEXPR int operator()(int _a, int _b) const
    {
        return _a - _b;
    }
};

// nullary_seven
//   helper: zero-argument callable returning a fixed value; exercises the
// nullary invocation paths of curry and curry_n.
struct nullary_seven
{
    D_CONSTEXPR int operator()() const
    {
        return 7;
    }
};

// echo_int
//   helper: unary callable returning its argument; exercises arity-1 invoke
// and degenerate single-argument uncurry.
struct echo_int
{
    D_CONSTEXPR int operator()(int _x) const
    {
        return _x;
    }
};

// digits3
//   helper: ternary callable whose result depends on argument order, used to
// confirm flip() swaps only the first two arguments.
struct digits3
{
    D_CONSTEXPR int operator()(int _a, int _b, int _c) const
    {
        return (_a * 100) + (_b * 10) + _c;
    }
};

// is_positive
//   helper: unary predicate (returns bool).
struct is_positive
{
    D_CONSTEXPR bool operator()(int _x) const
    {
        return _x > 0;
    }
};

// less_than
//   helper: binary predicate (returns bool).
struct less_than
{
    D_CONSTEXPR bool operator()(int _a, int _b) const
    {
        return _a < _b;
    }
};

// always_true
//   helper: nullary predicate (returns bool).
struct always_true
{
    D_CONSTEXPR bool operator()() const
    {
        return true;
    }
};

// returns_void
//   helper: unary callable returning void; a non-predicate (its result is
// not convertible to bool).
struct returns_void
{
    void operator()(int) const
    {
        return;
    }
};

// returns_pointer
//   helper: unary callable returning a pointer; a predicate by the
// bool-convertibility rule.
struct returns_pointer
{
    D_CONSTEXPR void* operator()(int) const
    {
        return nullptr;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One function per like-group semantic section of curry.hpp.  No comments
// adorn these declarations per the project style guide; see the matching
// definition in each section's .cpp for the per-function scope description.

void test_internal_machinery(test::test_handler& _h);

void test_predicate_traits(test::test_handler& _h);

void test_curry_factory(test::test_handler& _h);
void test_curry_n_factory(test::test_handler& _h);

void test_uncurry(test::test_handler& _h);

void test_flip(test::test_handler& _h);

void test_identity(test::test_handler& _h);
void test_always_constant(test::test_handler& _h);
void test_never(test::test_handler& _h);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////

extern const test::test_module_info curry_module_info;

void curry_module_run_all(test::test_runner_ctx& _ctx);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_CURRY_
