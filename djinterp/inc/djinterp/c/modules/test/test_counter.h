/*******************************************************************************
* djinterp [c/test]                                               test_counter.h
*
* Tallies for the C test framework.
*   `struct dj_test_counter` was used by 47 files and declared by none. The
* type was incomplete everywhere, so `test_standalone.h` -- which embeds it by
* value -- could not compile, and nor could the standalone test tier beneath
* it. This header supplies the definition.
*   THE FIELD FORM (style #79, ruled 2026.11.04). The four fields are the API,
* as 690 uses already had them, and the two functions below only reset and
* sum them. The API that four files called and nothing defined
* (d_test_counter_init, _increment, _get, _free and the D_TEST_COUNT_*
* selectors) is not supplied; those call sites move to the fields. The
* counter and timer the owner asked for, derived from the C counter.h and
* timer.h with thread-safe versions, arrive as types of their own once those
* modules exist (lane D, c/util), so this struct does not change shape.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TALLY TYPE
*     ----------
*     1.  the tally
*          1.  dj_test_counter
* 2.  OPERATIONS
*     ----------
*     1.  reset and sum
*     2.  the standalone assertion
*
*
* path:      /inc/djinterp/c/modules/test/test_counter.h
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.05
*******************************************************************************/
#ifndef DJINTERP_C_TEST_TEST_COUNTER_H
#define DJINTERP_C_TEST_TEST_COUNTER_H 1

// std
#include <stddef.h>  // size_t

// djinterp
#include "../../djinterp.h"  // framework root


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TALLY TYPE
//==============================================================================
// One object carries both tallies a test run produces: how many assertions
// were evaluated and how many held, and the same for tests. Both pairs sit in
// one struct because a module reports them together and a suite aggregates
// them together.


// 1.1    the tally
//------------------------------------------------------------------------------
// 1.1.1
// dj_test_counter
//   struct: assertion and test tallies for one module, or aggregated for a
// suite.
//
//   The field set is not invented. It is what the 47 files that use this type
// already read and write, at 685 sites: `tests_passed`, `tests_total`,
// `assertions_passed` and `assertions_total`, and nothing else. `size_t`
// matches the neighbouring counts in `dj_test_standalone_suite_results`.
//
//   A failure count is deliberately absent. Every site derives it as
// total - passed, and storing it too would let the two disagree.
struct dj_test_counter
{
    size_t assertions_passed;   // assertions that held
    size_t assertions_total;    // assertions evaluated
    size_t tests_passed;        // tests that passed
    size_t tests_total;         // tests run
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    reset and sum
//------------------------------------------------------------------------------
// both accept NULL and then do nothing; neither allocates
void d_test_counter_reset(struct dj_test_counter* _counter);
void d_test_counter_add(struct dj_test_counter*       _dest,
                        const struct dj_test_counter* _source);

// 2.2    the standalone assertion
//------------------------------------------------------------------------------
//   Declared here, not only in test_standalone.h: the 339 counter-style
// suites include this header alone and call it, which GCC let through as an
// implicit declaration and Clang refuses. It is defined in test_standalone.c,
// which a counter-style suite links.

/**
 * @brief Counts one assertion into a tally and prints its line.
 *
 * @param[in]     _condition  whether the assertion held.
 * @param[in]     _test_name  the assertion's name.
 * @param[in]     _message    what a failure means.
 * @param[in,out] _test_info  the tally; `NULL` counts and prints nothing.
 * @return `_condition`.
 */
bool d_assert_standalone(bool                    _condition,
                         const char*             _test_name,
                         const char*             _message,
                         struct dj_test_counter* _test_info);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_TEST_COUNTER_H
