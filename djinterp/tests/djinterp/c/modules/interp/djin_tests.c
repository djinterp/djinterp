/*******************************************************************************
* djinterp [test]                                                   djin_tests.c
*
*   The suite's aggregation: the set's tests, then the text's.
*
*
* path:      /tests/djinterp/c/modules/interp/djin_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
// djinterp
#include "./djin_tests.h"  // the suite's tests


/*
d_tests_djin_run_all
  Module-level aggregation: every djin test, the set's first.
*/
bool
d_tests_djin_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    result = d_tests_djin_set_all(_suite)  && result;
    result = d_tests_djin_text_all(_suite) && result;

    return result;
}
