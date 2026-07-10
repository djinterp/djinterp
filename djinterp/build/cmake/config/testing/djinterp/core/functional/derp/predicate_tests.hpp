/******************************************************************************
* djinterp [functional]                                    predicate_tests.hpp
*
*   Unit-test header for predicate.hpp. Provides a lightweight, self-contained
* test harness in `djinterp::testing`, the shared helper predicate types every
* test section reuses, and the declarations of the per-section test entry
* points (one per like-group semantic section of predicate.hpp).
*
*   HARNESS:
*   The harness is intentionally minimal so it carries no dependency on the
* full DTest session/tree machinery. A `test_registry` collects named cases;
* each test function registers its checks against a registry reference and
* returns the number of failures it observed. `run_all_predicate_tests()`
* drives every section and reports an aggregate verdict. Compile-time checks
* live directly in the .cpp files as `static_assert`s (they fire at build
* time); runtime checks use the harness `D_TESTING_CHECK` macro so call
* behavior (short-circuit, perfect forwarding, evaluation counts) is
* exercised at run time.
*
*   SECTION MAP (one .cpp per section):
*     predicate_tests_binary.cpp   -- and / or / xor combinators + factories
*     predicate_tests_not.cpp      -- not combinator + factory
*     predicate_tests_nand_nor.cpp -- nand / nor combinators + factories
*     predicate_tests_variadic.cpp -- all_of / any_of / none_of folds
*     predicate_tests_traits.cpp   -- structural is_predicate_* traits
*     predicate_tests_behavioral.cpp -- behavioral is_predicate + concepts
*
*   PORTABILITY:
*   Compiles and passes under C++11 through C++20. Sections that exercise
* C++11+-only features (variadic folds, traits, concepts) are gated; under
* C++98 those entry points compile to no-op passes.
*
*
* path:      /inc/functional/predicate_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.30
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PREDICATE_TESTS_
#define DJINTERP_FUNCTIONAL_PREDICATE_TESTS_ 1

// std
#include <cstddef>
// djinterp
#include <djinterp/core/djinterp.hpp>
#include <djinterp/core/functional/predicate.hpp>
// DTest framework (run_session model: handler + record_assertion + session).
// Same include convention as the example suites (view_tests.hpp, ...); adjust
// the prefix if this header sits elsewhere relative to the test framework.
#include "./test_common.hpp"
#include "./test_handler.hpp"
#include "./test_defaults.hpp"
#include "./test_runner.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   ASSERTION BRIDGE                                    ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_CHECK
//   macro: records one boolean check against the DTest handler `_h`, using
// the stringified expression as the assertion label, by forwarding to
// test::record_assertion. Section bodies stay flat check-lists while results
// flow through the framework's handler / tree / report like every other
// run_session suite. Variadic so a top-level comma inside a multi-arg
// template-id passes through as a single expression.
#define D_TEST_CHECK(_h, ...)                                                 \
    ::djinterp::test::record_assertion((_h), (__VA_ARGS__), #__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////
///                II.  SHARED HELPER PREDICATE TYPES                       ///
///////////////////////////////////////////////////////////////////////////////
// These functors are the building blocks every section composes. They are
// plain function objects (not lambdas) so they are nameable in trait checks
// and usable identically across C++11 through C++20.

// is_positive
//   struct: unary predicate, true when its argument is greater than zero.
struct is_positive
{
    typedef bool result_type;

    D_CONSTEXPR bool
    operator()(int _value) const
    {
        return (_value > 0);
    }
};

// is_even
//   struct: unary predicate, true when its argument is divisible by two.
struct is_even
{
    typedef bool result_type;

    D_CONSTEXPR bool
    operator()(int _value) const
    {
        return ((_value % 2) == 0);
    }
};

// is_negative
//   struct: unary predicate, true when its argument is less than zero.
struct is_negative
{
    typedef bool result_type;

    D_CONSTEXPR bool
    operator()(int _value) const
    {
        return (_value < 0);
    }
};

// less_than
//   struct: binary relation, true when the first argument is strictly less
// than the second. Exercises the arity-2 operator() overloads.
struct less_than
{
    typedef bool result_type;

    D_CONSTEXPR bool
    operator()(int _a, int _b) const
    {
        return (_a < _b);
    }
};

// always_true
//   struct: nullary-of-meaning unary predicate that ignores its argument
// and always returns true. Useful for short-circuit probing.
struct always_true
{
    typedef bool result_type;

    D_CONSTEXPR bool
    operator()(int) const
    {
        return true;
    }
};

// always_false
//   struct: unary predicate that ignores its argument and always returns
// false. Useful for short-circuit probing.
struct always_false
{
    typedef bool result_type;

    D_CONSTEXPR bool
    operator()(int) const
    {
        return false;
    }
};

// counting_predicate
//   struct: unary predicate that increments a shared counter on each call
// and returns a fixed verdict. Used to verify short-circuit evaluation: a
// predicate that must not be evaluated should leave its counter untouched.
struct counting_predicate
{
    typedef bool result_type;

    int* count;
    bool verdict;

    counting_predicate()
        : count(0),
          verdict(true)
    {}

    counting_predicate(
        int* _count,
        bool _verdict
    )
        : count(_count),
          verdict(_verdict)
    {}

    bool
    operator()(int) const
    {
        // tally this invocation so callers can assert evaluation counts
        if (count)
        {
            ++(*count);
        }

        return verdict;
    }
};

// returns_int_predicate
//   struct: unary "predicate" whose call result is int rather than bool.
// Used to confirm the behavioral trait accepts bool-convertible (not just
// exactly-bool) results, and that combinators coerce correctly.
struct returns_int_predicate
{
    int
    operator()(int _value) const
    {
        return _value;  // nonzero -> true, zero -> false
    }
};

// not_a_predicate
//   struct: a type with no call operator at all. Used as a negative case
// for the behavioral is_predicate trait.
struct not_a_predicate
{
    int member;
};

// returns_void_predicate
//   struct: a callable whose result is void and therefore not contextually
// convertible to bool. Negative case for the behavioral trait.
struct returns_void_predicate
{
    void
    operator()(int) const
    {
        return;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. PER-SECTION TEST ENTRY POINTS                       ///
///////////////////////////////////////////////////////////////////////////////
// Each section has the framework's leaf signature void(test::test_handler&) and
// records its findings with D_TEST_CHECK (test::record_assertion), so the totals
// roll up through the runner exactly like every other run_session module.

// and / or / xor combinators + factories
void test_predicate_binary(test::test_handler&);

// not combinator + factory
void test_predicate_not(test::test_handler&);

// nand / nor combinators + factories
void test_predicate_nand_nor(test::test_handler&);

// all_of / any_of / none_of variadic folds (C++11+)
void test_predicate_variadic(test::test_handler&);

// structural is_predicate_* / is_predicate_combinator traits (C++11+)
void test_predicate_traits(test::test_handler&);

// behavioral is_predicate trait + C++20 concepts (C++11+)
void test_predicate_behavioral(test::test_handler&);


// predicate_module_info / predicate_module_run_all
//   The run_session wiring: module_info carries the module identity and
// predicate_module_run_all schedules every section against the shared engine.
// Both are DEFINED in predicate_tests_runner.cpp.
extern const test::test_module_info predicate_module_info;

void predicate_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PREDICATE_TESTS_
