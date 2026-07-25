/******************************************************************************
* djinterp [test]                                       test_context_tests.hpp
*
*   Declarations for the unit-test suite covering test_context.hpp.  Each free
* function exercises one facet of the DTest document layer's focus type and
* returns true if every check inside it passed.  Tests are grouped into
* translation units by the section of test_context.hpp they cover:
*
*   - test_context_tests_context.cpp   -> I.  test_context (the focus struct)
*   - test_context_tests_builders.cpp  -> II. at_run / at_module / at_unit /
*                                              at_check (the focus builders)
*
*   The lone shared check helper, test_context_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean.  The D_CTX_CHECK macro is the intended call site.
*
*   PORTABILITY (why the bodies are C++17-gated):
*   test_context.hpp self-suppresses below C++17 (it builds on the document
* stack, which is C++17), so test_context, at_run, at_module, at_unit and
* at_check simply do not exist under an older standard.  Every test body is
* therefore wrapped in `#if D_ENV_LANG_IS_CPP17_OR_HIGHER ... #else return
* true;`, so the suite still COMPILES at C++11/14 (each unit passing vacuously)
* and does real work at C++17 and above - the same self-suppression the header
* under test performs.  The check helper, the fixture and the declarations name
* only the C++11 report node types, so they stay unconditional.
*
*   Fixtures.  report_fixture holds one default-constructed instance of each of
* the four report nodes (test_report / report_module / report_unit /
* report_check), giving a test four stable, DISTINCT addresses to hand to the
* focus parts - which is all the builders ever do with them (store, never
* dereference), so default-constructed nodes are sufficient and a heavier,
* populated report would witness nothing more.
*
*   NOTE: the entities under test live in djinterp::test; the tests themselves
* live, flat, in djinterp::testing (hence the `dt` alias below names the
* focus type and its builders).
*
*
* path:      /tests/djinterp/test/test_context/test_context_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_TEST_CONTEXT_TESTS_
#define DJINTERP_TEST_CONTEXT_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
// djinterp  -- framework header first, then the header under test (which pulls
// in test_report.hpp for the four focus-part node types).
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_context.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt names the focus type and its builders, which live in djinterp::test; the
// tests live here in djinterp::testing.  Declared once for the whole suite.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixture + check helper: normal (section-file) mode only


// =========================================================================
//  shared check helper
// =========================================================================

// test_context_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_context_check(
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
//  shared fixture -- one of each report node, for distinct focus addresses
// =========================================================================

// report_fixture
//   fixture: a default-constructed instance of each of the four report node
// types.  Its members have distinct addresses, so a test can build any focus
// shape (at_run .. at_check) from &f.run / &f.module_ / &f.unit / &f.check and
// then assert each part landed in the right field.  All four node types are
// C++11 (test_report.hpp), so this fixture is unconditional even though the
// focus type it feeds is C++17-only.
struct report_fixture
{
    ::djinterp::test::test_report   run;
    ::djinterp::test::report_module module_;
    ::djinterp::test::report_unit   unit;
    ::djinterp::test::report_check  check;
};

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// I.  test_context (the focus struct)
bool tests_context_default_members();
bool tests_context_aggregate_init();
bool tests_context_field_assignment();
bool tests_context_copy_semantics();

// II. focus builders (at_run / at_module / at_unit / at_check)
bool tests_builder_at_run();
bool tests_builder_at_module();
bool tests_builder_at_unit();
bool tests_builder_at_check();
bool tests_builder_nullptr_accepted();
bool tests_builder_pointer_distinctness();
bool tests_builder_const_pointees();
bool tests_builder_index_verbatim();
bool tests_builder_focus_depth_invariant();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Mirrors this suite as data - each unit test paired with its name and a
//  one-line descriptor - so run_module / run_suite can lower it into the
//  six-kind tree and drive the report.  References only the test_fn_ptr
//  declarations above; no fixtures required.
// =========================================================================

inline dt::module_spec
context_spec()
{
    return dt::module_spec{
        "test_context",
        "The DTest document-layer focus: a fat nullable context naming the "
        "four report depths (run / module / unit / check), and the focus "
        "builders that address one specific node.",
        {
            dt::block_spec{
                "context",
                "The focus struct: default members, aggregate initialization, "
                "field assignment, and value (copy) semantics.",
                {
                    { "context_default_members",
                      "a default context is all-null pointers and zero indices",
                      &tests_context_default_members },
                    { "context_aggregate_init",
                      "aggregate init fills named fields; the rest fall to their defaults",
                      &tests_context_aggregate_init },
                    { "context_field_assignment",
                      "each field is independently assignable with no cross-talk",
                      &tests_context_field_assignment },
                    { "context_copy_semantics",
                      "a context copies by value; the copy is independent",
                      &tests_context_copy_semantics },
                }
            },
            dt::block_spec{
                "builders",
                "The focus builders: at_run / at_module / at_unit / at_check, "
                "their default index arguments, null tolerance, and the "
                "focus-depth invariant.",
                {
                    { "builder_at_run",
                      "at_run carries only the run; deeper parts stay null",
                      &tests_builder_at_run },
                    { "builder_at_module",
                      "at_module carries run + module + index (default 0)",
                      &tests_builder_at_module },
                    { "builder_at_unit",
                      "at_unit carries run + module + unit + indices (default 0)",
                      &tests_builder_at_unit },
                    { "builder_at_check",
                      "at_check carries every part and every index (default 0)",
                      &tests_builder_at_check },
                    { "builder_nullptr_accepted",
                      "every builder stores a null pointer without dereferencing it",
                      &tests_builder_nullptr_accepted },
                    { "builder_pointer_distinctness",
                      "distinct nodes land in their own fields with no cross-wiring",
                      &tests_builder_pointer_distinctness },
                    { "builder_const_pointees",
                      "the builders accept pointers to const report nodes",
                      &tests_builder_const_pointees },
                    { "builder_index_verbatim",
                      "an index is stored exactly as given (no hidden offset)",
                      &tests_builder_index_verbatim },
                    { "builder_focus_depth_invariant",
                      "each builder nulls every deeper part and zeroes every unset index",
                      &tests_builder_focus_depth_invariant },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_CTX_CHECK
//   macro: evaluates its expression exactly once and routes the result
// through test_context_check, capturing the expression text and source
// location.  Variadic so expressions containing top-level commas (e.g. an
// aggregate initializer, or a multi-argument builder call) need no defensive
// parentheses.
#define D_CTX_CHECK(...)                                                       \
    ::djinterp::testing::test_context_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_CONTEXT_TESTS_
