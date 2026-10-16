/*******************************************************************************
* djinterp [test]                                           ptr_array_tests_sa.h
*
*   Unit test declarations for `ptr_array.h` module.
*   Provides comprehensive testing of all d_ptr_array functions including
* constructors, element manipulation, append/prepend operations, resize
* operations, search functions, utility functions, and destructors.
*
*
* path:      /tests/djinterp/c/container/array/ptr_array_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.31
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_PTR_ARRAY_SA_
#define DJINTERP_TESTS_PTR_ARRAY_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/array/ptr_array.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_ptr_array_new(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_default_size(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_from_arr(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_from_args(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_copy(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_merge(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_slice(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_new_slice_range(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_ptr_array_constructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_ptr_array_append_element(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_append_elements(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_append_array(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_contains(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_fill(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_find(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_insert_element(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_insert_elements(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_insert_array(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_is_empty(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_prepend_element(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_prepend_elements(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_prepend_array(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_reverse(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_resize_amount(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_resize_factor(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_sort(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_ptr_array_element_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_ptr_array_free(struct d_test_counter* _counter);
bool d_tests_sa_ptr_array_deep_free(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_ptr_array_destructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_ptr_array_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_PTR_ARRAY_SA_
