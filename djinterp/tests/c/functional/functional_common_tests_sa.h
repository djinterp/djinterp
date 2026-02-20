/******************************************************************************
* djinterp [test]                                functional_common_tests_sa.h
*
*   Standalone unit tests for the functional_common module.
*   Provides comprehensive coverage for all utility functions declared in
* functional_common.h including identity, constant, comparison, predicate,
* map/filter, fold, and iteration/query operations.
*
*
* path:      \inc\test\functional\functional_common_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.10
******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_COMMON_STANDALONE_
#define DJINTERP_TESTING_FUNCTIONAL_COMMON_STANDALONE_ 1

#include <stddef.h>
#include <stdio.h>
#include "..\..\inc\djinterp.h"
#include "..\..\inc\dmemory.h"
#include "..\..\inc\string_fn.h"
#include "..\..\inc\functional\functional_common.h"
#include "..\..\inc\test\test_standalone.h"


// i.    identity function tests
bool d_tests_sa_functional_identity_transformer(struct d_test_counter* _counter);
bool d_tests_sa_functional_identity_predicate(struct d_test_counter* _counter);

// ii.   constant function tests
bool d_tests_sa_functional_constant_true(struct d_test_counter* _counter);
bool d_tests_sa_functional_constant_false(struct d_test_counter* _counter);

// iii.  comparison utility tests
bool d_tests_sa_functional_compare_int(struct d_test_counter* _counter);
bool d_tests_sa_functional_compare_size_t(struct d_test_counter* _counter);
bool d_tests_sa_functional_compare_double(struct d_test_counter* _counter);
bool d_tests_sa_functional_equal_int(struct d_test_counter* _counter);
bool d_tests_sa_functional_equal_size_t(struct d_test_counter* _counter);

// iv.   predicate utility tests
bool d_tests_sa_functional_is_null(struct d_test_counter* _counter);
bool d_tests_sa_functional_is_not_null(struct d_test_counter* _counter);

// v.    fold tests
bool d_tests_sa_functional_fold_left(struct d_test_counter* _counter);
bool d_tests_sa_functional_fold_right(struct d_test_counter* _counter);

// vi.   iteration and query tests
bool d_tests_sa_functional_for_each(struct d_test_counter* _counter);
bool d_tests_sa_functional_for_each_const(struct d_test_counter* _counter);
bool d_tests_sa_functional_any(struct d_test_counter* _counter);
bool d_tests_sa_functional_all(struct d_test_counter* _counter);
bool d_tests_sa_functional_none(struct d_test_counter* _counter);
bool d_tests_sa_functional_count_if(struct d_test_counter* _counter);
bool d_tests_sa_functional_find_if(struct d_test_counter* _counter);

// vii.  all tests aggregation
bool d_tests_sa_functional_common_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TEST_FUNCTIONAL_COMMON_SA_
