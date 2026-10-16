/*******************************************************************************
* djinterp [test]                                            pipeline_tests_sa.h
*
*   Unit tests for `pipeline.h` in the functional module.
*   For the file itself, go to `\inc\functional\pipeline.h`.
*   Note: this module is required to build DTest, so it does not use DTest
* for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      /tests/djinterp/c/functional/pipeline_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.10
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_PIPELINE_
#define DJINTERP_TESTING_FUNCTIONAL_PIPELINE_ 1

// std
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/dmemory.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/functional_common.h"
#include "../../../../inc/djinterp/c/functional/pipeline.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


// i.    pipeline creation tests
bool d_tests_sa_pipeline_begin(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_begin_copy(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_creation_all(struct d_test_counter* _test_info);

// ii.   pipeline operation tests (chainable)
bool d_tests_sa_pipeline_map(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_filter(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_fold(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_for_each(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_take(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_skip(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_chaining(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_operations_all(struct d_test_counter* _test_info);

// iii.  pipeline finalization tests
bool d_tests_sa_pipeline_end(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_free(struct d_test_counter* _test_info);
bool d_tests_sa_pipeline_finalization_all(struct d_test_counter* _test_info);

// iv.   comprehensive test runner
bool d_tests_sa_pipeline_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_PIPELINE_
