/*******************************************************************************
* djinterp [test]                                          test_common_tests.hpp
*
*   Declarations, shared check helper, and the authored suite spec for the
* test_common.hpp unit suite.  Two faces, selected by DTEST_SPEC_MODE:
*   - NORMAL mode (the four section TUs): includes the header under test and
*     exposes the check helper; the TUs define the tests_* bodies.
*   - SPEC mode (the runner): includes the authoring surface and exposes
*     common_spec(), the module_spec provider the runner hands to run_module.
* The tests_* DECLARATIONS are visible in both modes.  (Same shape as
* test_options / test_callable / test_context.)
*
*   Sections -> one translation unit each:
*   - test_common_tests_type_id.cpp    -> I.   TEST TYPE IDENTIFICATION
*   - test_common_tests_status.cpp     -> II.  STATUS CLASSIFICATION
*   - test_common_tests_event.cpp      -> III. EVENT SYSTEM
*   - test_common_tests_constexpr.cpp  -> IV.  CONSTEXPR SUPPORT MACROS
*
*   NOTE: the entities under test live in djinterp::test; the tests themselves
* live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/test/test_common_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                              created: 2026.06.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_TESTS_
#define DJINTERP_TEST_COMMON_TESTS_ 1

// std
#include <cstdio>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../inc/djinterp/test/test_common.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // check helper: normal (section-file) mode only

// test_common_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_common_check(
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

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// I.   TEST TYPE IDENTIFICATION
bool tests_test_type_id();
bool tests_test_callable_id();
bool tests_k_no_callable();

// II.  STATUS CLASSIFICATION
bool tests_test_status();

// III. EVENT SYSTEM
bool tests_test_event_id();
bool tests_test_event();

// IV.  CONSTEXPR SUPPORT MACROS
bool tests_d_test_constexpr();
bool tests_d_test_static_constexpr();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    The suite as data: one block per section, one { name, descriptor, &fn }
//  row per test.  References only the declarations above; no fixtures needed.
// =========================================================================

inline dt::module_spec
common_spec()
{
    return dt::module_spec{
        "test_common",
        "Foundational test types, status classification, and events - the DTest "
        "common header the rest of the framework is built on.",
        {
            dt::block_spec{
                "type_id",
                "The node-identity types: the kind id, the callable id, and the "
                "no-callable sentinel.",
                {
                    { "tests_test_type_id",
                      "test_type_id is the unsigned id type that tags a node's kind",
                      &tests_test_type_id },
                    { "tests_test_callable_id",
                      "test_callable_id is the 1-based id keying a node's deferred callable",
                      &tests_test_callable_id },
                    { "tests_k_no_callable",
                      "k_no_callable is the sentinel id (0) meaning 'no callable bound'",
                      &tests_k_no_callable },
                }
            },
            dt::block_spec{
                "status",
                "Status classification: the terminal-state enum.",
                {
                    { "tests_test_status",
                      "test_status enumerates the terminal states (passed / failed / skipped / "
                      "error / pending) with their documented underlying values",
                      &tests_test_status },
                }
            },
            dt::block_spec{
                "event",
                "The event system's identity and record types.",
                {
                    { "tests_test_event_id",
                      "test_event_id is the identifier type distinguishing event tags",
                      &tests_test_event_id },
                    { "tests_test_event",
                      "test_event is the event record pairing an id with its payload",
                      &tests_test_event },
                }
            },
            dt::block_spec{
                "constexpr",
                "The constexpr-support macros that adapt to the language level.",
                {
                    { "tests_d_test_constexpr",
                      "D_TEST_CONSTEXPR expands to constexpr on standards that allow it",
                      &tests_d_test_constexpr },
                    { "tests_d_test_static_constexpr",
                      "D_TEST_STATIC_CONSTEXPR is the static constexpr companion macro",
                      &tests_d_test_static_constexpr },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


// D_TC_CHECK
//   macro: evaluates its argument exactly once and routes it through
// test_common_check, capturing the expression text and source location.
// Variadic so a top-level comma (e.g. std::is_same<A, B>::value) passes through.
#define D_TC_CHECK(...)                                                       \
    ::djinterp::testing::test_common_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_COMMON_TESTS_
