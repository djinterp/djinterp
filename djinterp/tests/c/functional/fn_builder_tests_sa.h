/******************************************************************************
* djinterp [test]                                       fn_builder_tests_sa.h
*
*   Unit tests for `fn_builder.h` in the functional module.
*   For the file itself, go to `\inc\functional\fn_builder.h`.
*   Note: this module is required to build DTest, so it uses `test_standalone.h`,
* rather than DTest for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      \test\functional\fn_builder_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.10
******************************************************************************/

#ifndef DJINTERP_TESTING_FN_BUILDER_STANDALONE_
#define DJINTERP_TESTING_FN_BUILDER_STANDALONE_ 1

#include <stdlib.h>
#include "..\..\inc\djinterp.h"
#include "..\..\inc\dmemory.h"
#include "..\..\inc\string_fn.h"
#include "..\..\inc\test\test_standalone.h"
#include "..\..\inc\functional\functional_common.h"
#include "..\..\inc\functional\fn_builder.h"


// i.    builder creation tests
bool d_tests_sa_fn_builder_new(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_creation_all(struct d_test_counter* _test_info);

// ii.   fluent operation tests (chainable)
bool d_tests_sa_fn_builder_map(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_filter(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_and_then(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_where(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_chaining(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_grow(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_operations_all(struct d_test_counter* _test_info);

// iii.  builder execution tests
bool d_tests_sa_fn_builder_execute_validation(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execute_passthrough(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execute_transforms_only(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execute_predicates_only(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execute_combined(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execute_transform_failure(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execute_pingpong(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_execution_all(struct d_test_counter* _test_info);

// iv.   builder cleanup tests
bool d_tests_sa_fn_builder_free(struct d_test_counter* _test_info);
bool d_tests_sa_fn_builder_cleanup_all(struct d_test_counter* _test_info);

// v.    comprehensive test runner
bool d_tests_sa_fn_builder_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_FN_BUILDER_
