/*******************************************************************************
* djinterp [test]                                               pipeline_tests.h
*
*   Unit tests for `pipeline.h` in the functional module.
*   For the file itself, go to `\inc\functional\pipeline.h`.
*   Note: this module is required to build DTest, so it does not use DTest
* for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      /tests/djinterp/c/functional/pipeline_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.10
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_PIPELINE_
#define DJINTERP_TESTING_FUNCTIONAL_PIPELINE_ 1

// std
#include <errno.h>   // EINVAL, ENOMEM
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/functional_common.h"
#include "../../../../inc/djinterp/c/functional/pipeline.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// i.    pipeline creation tests
bool d_tests_pipeline_begin(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_begin_copy(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_creation_all(
    struct dj_test_suite* _suite
);

// ii.   pipeline operation tests (chainable)
bool d_tests_pipeline_map(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_filter(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_fold(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_fold_errors(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_for_each(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_take(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_skip(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_chaining(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_operations_all(
    struct dj_test_suite* _suite
);

// iii.  pipeline finalization tests
bool d_tests_pipeline_end(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_free(
    struct dj_test_suite* _suite
);
bool d_tests_pipeline_finalization_all(
    struct dj_test_suite* _suite
);

// iv.   comprehensive test runner
bool d_tests_pipeline_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTING_PIPELINE_
