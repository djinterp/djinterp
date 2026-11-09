/*******************************************************************************
* djinterp [c/test]                                               test_counter.h
*
* Tallies for the C test framework: assertions evaluated and held, tests run
* and passed, as test_printer and the C++ session report them.
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
* path:      /inc/djinterp/c/modules/test/test_counter.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TALLY TYPE
    ----------
    1.  the tally
         1.  dj_test_counter
2.  OPERATIONS
    ----------
    1.  reset and sum
*/

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
//   The field set is not invented. It is what the code that uses this type
// already reads and writes: `tests_passed`, `tests_total`,
// `assertions_passed` and `assertions_total`, and nothing else. `size_t`, as
// every other count in the C framework.
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


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_TEST_COUNTER_H
