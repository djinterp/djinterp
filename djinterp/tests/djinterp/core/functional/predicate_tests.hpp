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
#include "../djinterp.hpp"
#include "./predicate.hpp"


#ifndef D_KEYWORD_TESTING
#  define D_KEYWORD_TESTING testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
#  define NS_TESTING D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   LIGHTWEIGHT TEST HARNESS                            ///
///////////////////////////////////////////////////////////////////////////////

// test_registry
//   class: minimal pass/fail tally for predicate unit tests. Each runtime
// check increments either the pass or fail counter and records the first
// failure's location for reporting. The harness is deliberately decoupled
// from the DTest session/tree machinery so the predicate tests stay small
// and buildable in isolation.
class test_registry
{
public:
    test_registry()
        : m_checks(0),
          m_failures(0),
          m_first_fail_file(0),
          m_first_fail_line(0),
          m_first_fail_expr(0)
    {}

    // record
    //   function: record the outcome of a single runtime check.
    void
    record(
        bool        _ok,
        const char* _expr,
        const char* _file,
        int         _line
    )
    {
        ++m_checks;

        // record the location of the first observed failure
        if (!_ok)
        {
            if (m_failures == 0)
            {
                m_first_fail_file = _file;
                m_first_fail_line = _line;
                m_first_fail_expr = _expr;
            }

            ++m_failures;
        }

        return;
    }

    std::size_t checks()   const { return m_checks; }
    std::size_t failures() const { return m_failures; }
    bool        passed()   const { return (m_failures == 0); }

    const char* first_fail_file() const { return m_first_fail_file; }
    int         first_fail_line() const { return m_first_fail_line; }
    const char* first_fail_expr() const { return m_first_fail_expr; }

private:
    std::size_t m_checks;
    std::size_t m_failures;
    const char* m_first_fail_file;
    int         m_first_fail_line;
    const char* m_first_fail_expr;
};


// D_TESTING_CHECK
//   macro: record a single boolean check against a test_registry named
// `_reg`. Evaluates `_expr` exactly once. Use inside the per-section test
// functions for every runtime assertion.
#define D_TESTING_CHECK(_reg, _expr)                                          \
    (_reg).record((_expr), #_expr, __FILE__, __LINE__)


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
// Each function registers its checks against the supplied registry and
// returns the number of failures it observed (0 == all passed).

// and / or / xor combinators + factories
std::size_t test_predicate_binary(test_registry& _reg);

// not combinator + factory
std::size_t test_predicate_not(test_registry& _reg);

// nand / nor combinators + factories
std::size_t test_predicate_nand_nor(test_registry& _reg);

// all_of / any_of / none_of variadic folds (C++11+)
std::size_t test_predicate_variadic(test_registry& _reg);

// structural is_predicate_* / is_predicate_combinator traits (C++11+)
std::size_t test_predicate_traits(test_registry& _reg);

// behavioral is_predicate trait + C++20 concepts (C++11+)
std::size_t test_predicate_behavioral(test_registry& _reg);


// run_all_predicate_tests
//   function: drives every section against a fresh registry and returns the
// total number of failures across all sections (0 == the full suite passed).
std::size_t run_all_predicate_tests(test_registry& _reg);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PREDICATE_TESTS_
