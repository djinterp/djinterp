/******************************************************************************
* djinterp [test]                                        test_common_tests.hpp
*
*   Declarations for the unit-test suite covering test_common.hpp.  Each
* free function exercises one entity of the DTest common header and returns
* true if every check inside it passed, false otherwise.  Tests are grouped
* into translation units by the section of test_common.hpp they cover:
*
*   - test_common_tests_type_id.cpp    -> I.   TEST TYPE IDENTIFICATION
*   - test_common_tests_status.cpp     -> II.  STATUS CLASSIFICATION
*   - test_common_tests_event.cpp      -> III. EVENT SYSTEM
*   - test_common_tests_constexpr.cpp  -> IV.  CONSTEXPR SUPPORT MACROS
*
*   The lone shared helper, test_common_check, reports a failing check (with
* its stringized expression and source location) and forwards the boolean so
* the calling test can accumulate the running result.  The D_TC_CHECK macro is
* the intended call site: it captures the expression text, file, and line.
*
*   NOTE: the entities under test live in djinterp::test; the tests themselves
* live, flat, in djinterp::testing.
*
*
* path:      /inc/djinterp/test/test_common_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_TESTS_
#define DJINTERP_TEST_COMMON_TESTS_ 1

// std
#include <cstdio>
// djinterp
#include "test_common.hpp"


NS_DJINTERP
NS_TESTING


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


NS_END  // testing
NS_END  // djinterp


// D_TC_CHECK
//   macro: evaluates its argument exactly once and routes it through
// test_common_check, capturing the expression text and source location.
// Yields the boolean result for accumulation at the call site.  Variadic so
// that a condition containing a top-level comma (e.g. a multi-argument trait
// such as std::is_same<A, B>::value) passes through as a single argument.
#define D_TC_CHECK(...)                                                       \
    ::djinterp::testing::test_common_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_COMMON_TESTS_
