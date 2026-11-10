/*******************************************************************************
* djinterp [test]                                             fn_builder_tests.h
*
*   Unit tests for `fn_builder.h` in the functional module.
*   For the file itself, go to `\inc\functional\fn_builder.h`.
*   Note: this module is required to build DTest, so it does not use DTest
* for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.10
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_FN_BUILDER_STANDALONE_
#define DJINTERP_TESTING_FN_BUILDER_STANDALONE_ 1

// std
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/functional_common.h"
#include "../../../../inc/djinterp/c/functional/fn_builder.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// i.    builder creation tests
bool d_tests_fn_builder_new(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_creation_all(
    struct dj_test_suite* _suite
);

// ii.   fluent operation tests (chainable)
bool d_tests_fn_builder_map(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_filter(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_and_then(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_where(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_chaining(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_grow(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_operations_all(
    struct dj_test_suite* _suite
);

// iii.  builder execution tests
bool d_tests_fn_builder_execute_validation(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execute_passthrough(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execute_transforms_only(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execute_predicates_only(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execute_combined(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execute_transform_failure(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execute_pingpong(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_execution_all(
    struct dj_test_suite* _suite
);

// iv.   builder cleanup tests
bool d_tests_fn_builder_free(
    struct dj_test_suite* _suite
);
bool d_tests_fn_builder_cleanup_all(
    struct dj_test_suite* _suite
);

// v.    comprehensive test runner
bool d_tests_fn_builder_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTING_FN_BUILDER_
