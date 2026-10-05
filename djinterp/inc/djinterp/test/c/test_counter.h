/*******************************************************************************
* djinterp [test]                                                 test_counter.h
*
* Tallies for the C test framework.
*   `struct d_test_counter` was used by 47 files and declared by none. The
* type was incomplete everywhere, so `test_standalone.h` -- which embeds it by
* value -- could not compile, and nor could the standalone test tier beneath
* it. This header supplies the definition.
*
*
* path:      /inc/djinterp/test/c/test_counter.h
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
#ifndef DJINTERP_TEST_C_TEST_COUNTER_H
#define DJINTERP_TEST_C_TEST_COUNTER_H 1

// std
#include <stddef.h>  // size_t

// djinterp
#include "../../c/djinterp.h"  // framework root


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
// d_test_counter
//   struct: assertion and test tallies for one module, or aggregated for a
// suite.
//
//   The field set is not invented. It is what the 47 files that use this type
// already read and write, at 685 sites: `tests_passed`, `tests_total`,
// `assertions_passed` and `assertions_total`, and nothing else. `size_t`
// matches the neighbouring counts in `d_test_standalone_suite_results`.
//
//   A failure count is deliberately absent. Every site derives it as
// total - passed, and storing it too would let the two disagree.
struct d_test_counter
{
    size_t assertions_passed;   // assertions that held
    size_t assertions_total;    // assertions evaluated
    size_t tests_passed;        // tests that passed
    size_t tests_total;         // tests run
};


D_EXTERN_C_END


#endif  // DJINTERP_TEST_C_TEST_COUNTER_H
