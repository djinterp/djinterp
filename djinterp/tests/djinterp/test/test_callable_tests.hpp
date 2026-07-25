/******************************************************************************
* djinterp [test]                                      test_callable_tests.hpp
*
*   Declarations for the unit-test suite covering test_callable.hpp.  Each free
* function exercises one facet of test_callable_table - the out-of-line store
* of deferred test work (a table of std::function<bool()> thunks keyed by
* 1-based test_callable_id) - and returns true if every check inside it passed.
* Tests are grouped into translation units by the section of the class they
* cover:
*
*   - test_callable_tests_binding.cpp     -> add / size / empty / clear / has
*                                            (storage, id discipline, queries)
*   - test_callable_tests_invocation.cpp  -> invoke (bound / unbound / sentinel
*                                            / empty row / re-run / const)
*   - test_callable_tests_composition.cpp -> compose_and / compose_or (the
*                                            boolean algebra over thunks)
*   - test_callable_tests_transform.cpp   -> transform (row rewriting)
*
*   The lone shared check helper, test_callable_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean.  The D_CALL_CHECK macro is the intended call site.
*
*   Fixtures.  counting_thunk is the workhorse: a bool()-returning callable
* that returns a fixed value AND bumps a caller-owned counter each time it
* runs.  Handed to the table by value, its internal counter POINTER is shared,
* so a test can witness two things the truth tables alone cannot - that a
* short-circuited clause is never evaluated (its counter stays put), and that
* invoke re-runs the thunk on every call rather than memoizing.  An empty
* thunk_type{} models the "row exists but holds no callable" state that
* separates has() from mere index-in-range.
*
*   PORTABILITY: the class is C++11 (std::function, lambdas), present at every
* standard this suite is built under, so - unlike a C++17-gated header - the
* test bodies need no language guard.
*
*   NOTE: test_callable_table lives in djinterp::test; the tests live, flat, in
* djinterp::testing (hence the `dt` alias below).
*
*
* path:      /tests/djinterp/test/test_callable/test_callable_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_TEST_CALLABLE_TESTS_
#define DJINTERP_TEST_CALLABLE_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <functional>
// djinterp  -- framework header first, then the header under test.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_callable.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt names the table (and the id / sentinel), which live in djinterp::test;
// the tests live here in djinterp::testing.  Declared once for the whole suite.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// test_callable_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_callable_check(
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


// =========================================================================
//  shared fixtures
// =========================================================================

// thunk_type
//   the table's own deferred-boolean type, re-exported for brevity.
using thunk_type = ::djinterp::test::test_callable_table::thunk_type;

// counting_thunk
//   fixture: a bool() callable returning a fixed value and, if given a
// non-null counter, incrementing it on every call.  Copied by value into the
// table (std::function stores a copy), but the counter POINTER is shared - so
// the counter reveals whether, and how often, this thunk actually ran.  That
// is what lets a test prove short-circuit evaluation (a skipped clause leaves
// its counter at zero) and re-invocation (two invoke calls -> counter two).
struct counting_thunk
{
    bool value;
    int* calls;   // may be null

    bool operator()() const
    {
        if (calls != nullptr)
        {
            ++*calls;
        }

        return value;
    }
};

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// binding: add / size / empty / clear / has
bool tests_binding_add_issues_1based_ids();
bool tests_binding_empty_and_size();
bool tests_binding_has_sentinel_and_out_of_range();
bool tests_binding_has_empty_row_is_false();
bool tests_binding_clear_empties();
bool tests_binding_add_after_clear_reissues();

// invocation: invoke
bool tests_invoke_bound_returns_thunk_result();
bool tests_invoke_unbound_and_sentinel_false();
bool tests_invoke_empty_row_false();
bool tests_invoke_reruns_each_call();
bool tests_invoke_const_table();

// composition: compose_and / compose_or
bool tests_compose_and_truth_table();
bool tests_compose_or_truth_table();
bool tests_compose_and_short_circuits();
bool tests_compose_or_short_circuits();
bool tests_compose_chaining_accumulates();
bool tests_compose_mixed_and_or();
bool tests_compose_unbound_is_noop();
bool tests_compose_empty_row_binds_clause();
bool tests_compose_survives_reallocation();

// transform: transform
bool tests_transform_wraps_existing();
bool tests_transform_receives_current_thunk();
bool tests_transform_unbound_is_noop();
bool tests_transform_empty_row_is_noop();
bool tests_transform_to_empty_disables_row();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Mirrors this suite as data - each unit test paired with its name and a
//  one-line descriptor - so run_module / run_suite can lower it into the
//  six-kind tree and drive the report.  References only the test_fn_ptr
//  declarations above; no fixtures required.
// =========================================================================

inline dt::module_spec
callable_spec()
{
    return dt::module_spec{
        "test_callable",
        "The test_callable_table: an out-of-line store of deferred boolean "
        "thunks keyed by 1-based id - binding, invocation, boolean-algebra "
        "composition, and row-rewriting transform.",
        {
            dt::block_spec{
                "binding",
                "Storage and queries: add issues stable 1-based ids; size / "
                "empty / clear; has distinguishes sentinel, out-of-range, and "
                "empty rows.",
                {
                    { "binding_add_issues_1based_ids",
                      "add returns stable, monotonic, 1-based ids",
                      &tests_binding_add_issues_1based_ids },
                    { "binding_empty_and_size",
                      "empty and size track the row count",
                      &tests_binding_empty_and_size },
                    { "binding_has_sentinel_and_out_of_range",
                      "has rejects the sentinel and any out-of-range id",
                      &tests_binding_has_sentinel_and_out_of_range },
                    { "binding_has_empty_row_is_false",
                      "an added empty thunk occupies a row but has() is false",
                      &tests_binding_has_empty_row_is_false },
                    { "binding_clear_empties",
                      "clear drops every row; prior ids read as absent",
                      &tests_binding_clear_empties },
                    { "binding_add_after_clear_reissues",
                      "add after clear reissues id 1 against the new row",
                      &tests_binding_add_after_clear_reissues },
                }
            },
            dt::block_spec{
                "invocation",
                "Running a row: invoke returns the thunk's result, reads absent "
                "rows as non-passing, and re-runs on each call.",
                {
                    { "invoke_bound_returns_thunk_result",
                      "invoke returns the bound thunk's boolean",
                      &tests_invoke_bound_returns_thunk_result },
                    { "invoke_unbound_and_sentinel_false",
                      "invoke of an unbound or sentinel id is false, never throws",
                      &tests_invoke_unbound_and_sentinel_false },
                    { "invoke_empty_row_false",
                      "invoke of an empty row is false",
                      &tests_invoke_empty_row_false },
                    { "invoke_reruns_each_call",
                      "invoke re-runs the thunk every call (no memoization)",
                      &tests_invoke_reruns_each_call },
                    { "invoke_const_table",
                      "has / invoke / size / empty work through a const table",
                      &tests_invoke_const_table },
                }
            },
            dt::block_spec{
                "composition",
                "Boolean algebra over thunks: compose_and / compose_or, their "
                "short-circuit semantics, chaining, and the unbound / empty-row "
                "edges.",
                {
                    { "compose_and_truth_table",
                      "compose_and yields existing AND clause across all cases",
                      &tests_compose_and_truth_table },
                    { "compose_or_truth_table",
                      "compose_or yields existing OR clause across all cases",
                      &tests_compose_or_truth_table },
                    { "compose_and_short_circuits",
                      "a false left operand skips the AND clause",
                      &tests_compose_and_short_circuits },
                    { "compose_or_short_circuits",
                      "a true left operand skips the OR clause",
                      &tests_compose_or_short_circuits },
                    { "compose_chaining_accumulates",
                      "repeated compose folds clauses onto the whole thunk",
                      &tests_compose_chaining_accumulates },
                    { "compose_mixed_and_or",
                      "mixed AND/OR composition folds left in call order",
                      &tests_compose_mixed_and_or },
                    { "compose_unbound_is_noop",
                      "composing an unbound or sentinel id changes nothing",
                      &tests_compose_unbound_is_noop },
                    { "compose_empty_row_binds_clause",
                      "composing onto an empty row binds the clause alone",
                      &tests_compose_empty_row_binds_clause },
                    { "compose_survives_reallocation",
                      "a composed thunk keeps working after the table grows",
                      &tests_compose_survives_reallocation },
                }
            },
            dt::block_spec{
                "transform",
                "Row rewriting: transform replaces a row with a mapper applied "
                "to its current thunk, with the has() guard and the disabling "
                "edge.",
                {
                    { "transform_wraps_existing",
                      "transform replaces a row via the mapper (e.g. negate)",
                      &tests_transform_wraps_existing },
                    { "transform_receives_current_thunk",
                      "the mapper receives the row's current thunk",
                      &tests_transform_receives_current_thunk },
                    { "transform_unbound_is_noop",
                      "transform of an unbound / sentinel id does not call the mapper",
                      &tests_transform_unbound_is_noop },
                    { "transform_empty_row_is_noop",
                      "transform of an empty row is a no-op (unlike compose)",
                      &tests_transform_empty_row_is_noop },
                    { "transform_to_empty_disables_row",
                      "a mapper returning an empty thunk disables the row",
                      &tests_transform_to_empty_disables_row },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_CALL_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through test_callable_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g. a
// multi-argument call) need no defensive parentheses.
#define D_CALL_CHECK(...)                                                      \
    ::djinterp::testing::test_callable_check(                                  \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_CALLABLE_TESTS_
