/*******************************************************************************
* djinterp [test]                                      functional_common_tests.h
*
*   Standalone unit tests for the functional_common module.
*   Provides comprehensive coverage for all utility functions declared in
* functional_common.h including identity, constant, comparison, predicate,
* map/filter, fold, and iteration/query operations.
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.10
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_COMMON_STANDALONE_
#define DJINTERP_TESTING_FUNCTIONAL_COMMON_STANDALONE_ 1

// std
#include <stddef.h>
#include <stdio.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/functional_common.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// i.    identity function tests
bool d_tests_functional_identity_transformer(
    struct dj_test_suite* _suite
);
bool d_tests_functional_identity_predicate(
    struct dj_test_suite* _suite
);

// ii.   constant function tests
bool d_tests_functional_constant_true(
    struct dj_test_suite* _suite
);
bool d_tests_functional_constant_false(
    struct dj_test_suite* _suite
);

// iii.  comparison utility tests
bool d_tests_functional_compare_int(
    struct dj_test_suite* _suite
);
bool d_tests_functional_compare_size_t(
    struct dj_test_suite* _suite
);
bool d_tests_functional_compare_double(
    struct dj_test_suite* _suite
);
bool d_tests_functional_equal_int(
    struct dj_test_suite* _suite
);
bool d_tests_functional_equal_size_t(
    struct dj_test_suite* _suite
);

// iv.   predicate utility tests
bool d_tests_functional_is_null(
    struct dj_test_suite* _suite
);
bool d_tests_functional_is_not_null(
    struct dj_test_suite* _suite
);

// v.    fold tests
bool d_tests_functional_fold_left(
    struct dj_test_suite* _suite
);
bool d_tests_functional_fold_right(
    struct dj_test_suite* _suite
);

// vi.   iteration and query tests
bool d_tests_functional_for_each(
    struct dj_test_suite* _suite
);
bool d_tests_functional_for_each_const(
    struct dj_test_suite* _suite
);
bool d_tests_functional_any(
    struct dj_test_suite* _suite
);
bool d_tests_functional_all(
    struct dj_test_suite* _suite
);
bool d_tests_functional_none(
    struct dj_test_suite* _suite
);
bool d_tests_functional_count_if(
    struct dj_test_suite* _suite
);
bool d_tests_functional_find_if(
    struct dj_test_suite* _suite
);

// vii.  the fc groups: higher-order functions, quantifiers and utilities,
// each with its own aggregation
//   higher-order functions (map, filter, folds, for_each)
bool d_tests_fc_map(
    struct dj_test_suite* _suite
);
bool d_tests_fc_filter(
    struct dj_test_suite* _suite
);
bool d_tests_fc_fold_left(
    struct dj_test_suite* _suite
);
bool d_tests_fc_fold_right(
    struct dj_test_suite* _suite
);
bool d_tests_fc_for_each(
    struct dj_test_suite* _suite
);
bool d_tests_fc_higher_order_all(
    struct dj_test_suite* _suite
);
//   quantifiers
bool d_tests_fc_any(
    struct dj_test_suite* _suite
);
bool d_tests_fc_all(
    struct dj_test_suite* _suite
);
bool d_tests_fc_none(
    struct dj_test_suite* _suite
);
bool d_tests_fc_count_if(
    struct dj_test_suite* _suite
);
bool d_tests_fc_find_if(
    struct dj_test_suite* _suite
);
bool d_tests_fc_quantifier_all(
    struct dj_test_suite* _suite
);
//   utilities
bool d_tests_fc_identity_functions(
    struct dj_test_suite* _suite
);
bool d_tests_fc_constant_functions(
    struct dj_test_suite* _suite
);
bool d_tests_fc_comparison_utilities(
    struct dj_test_suite* _suite
);
bool d_tests_fc_predicate_utilities(
    struct dj_test_suite* _suite
);
bool d_tests_fc_utility_all(
    struct dj_test_suite* _suite
);

// viii. all tests aggregation
bool d_tests_functional_common_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TEST_FUNCTIONAL_COMMON_SA_
