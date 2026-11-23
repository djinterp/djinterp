/*******************************************************************************
* djinterp [c][net]                                           net_tests_assert.h
*
* The check net's suites make, in DTest's C face.
*   D_TESTS_NET_ASSERT records one check against the test open in a suite
* (test_suite.h 3.2), named for the function it checks and saying what it
* checks, at the caller's file and line. The suites took this shape on the
* retired standalone tier, so the macro keeps its arguments: the condition,
* the test's name, the message, the suite. A suite of NULL records nothing
* and returns the condition alone, for a helper that checks a step and lets
* its caller report it.
*
*
* path:      /tests/djinterp/c/modules/net/net_tests_assert.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.19
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_ASSERT_H
#define DJINTERP_NET_NET_TESTS_ASSERT_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


/**
 * @brief Records one check against the test open in `_suite`.
 *
 * @note Call it through D_TESTS_NET_ASSERT, which passes the caller's file
 *       and line.
 *
 * @param[in]     _condition  the condition checked.
 * @param[in]     _test_name  the function under test; the check's name.
 * @param[in]     _message    what the check verifies; printed when it fails.
 * @param[in,out] _suite      the suite, or `NULL` to record nothing.
 * @param[in]     _file       the caller's file, as __FILE__ gives it.
 * @param[in]     _line       the caller's line.
 * @return `_condition`.
 */
bool d_tests_net_assert(
    bool                  _condition,
    const char*           _test_name,
    const char*           _message,
    struct dj_test_suite* _suite,
    const char*           _file,
    int                   _line
);

// D_TESTS_NET_ASSERT
//   macro: d_tests_net_assert at the caller's file and line.
#define D_TESTS_NET_ASSERT(_condition, _test_name, _message, _suite)         \
    d_tests_net_assert((_condition), (_test_name), (_message), (_suite),     \
                       __FILE__, __LINE__)


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_ASSERT_H
