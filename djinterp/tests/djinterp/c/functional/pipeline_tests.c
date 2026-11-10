/*******************************************************************************
* djinterp [functional]                                         pipeline_tests.c
*
*
* path:      /tests/djinterp/c/functional/pipeline_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./pipeline_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_pipeline_all
  Runs all pipeline unit tests across all sections:
  i.   pipeline creation (begin, begin_copy)
  ii.  pipeline operations (map, filter, fold, for_each, take, skip, chaining)
  iii. pipeline finalization (end, free)
*/
bool d_tests_pipeline_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    all_passed = true;

    // i.   pipeline creation
    all_passed &= d_tests_pipeline_creation_all(_suite);

    // ii.  pipeline operations (chainable)
    all_passed &= d_tests_pipeline_operations_all(_suite);

    // iii. pipeline finalization
    all_passed &= d_tests_pipeline_finalization_all(_suite);

    return all_passed;
}
