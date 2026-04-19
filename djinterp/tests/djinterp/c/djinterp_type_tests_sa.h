/******************************************************************************
* djinterp [test]                              djinterp_header_type_tests_sa.h
*
* This is a test file for `djinterp.h` unit tests.
* For the file itself, go to `\inc\c\djinterp.h`.
* Note: this module is required to build DTest, so it uses `test_standalone.h`,
* rather than DTest for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
* 
* 
* path:      \test\djinterp_header_type_tests_sa.h             
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.11.05
******************************************************************************/

#ifndef DJINTERP_TESTING_HEADER_TYPES_
#define DJINTERP_TESTING_HEADER_TYPES_ 1

#include <stdlib.h>
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\test\test_standalone.h"


// `d_index` function tests (in header order)
bool d_tests_sa_index_convert_fast(struct d_test_counter* _test_info);
bool d_tests_sa_index_convert_safe(struct d_test_counter* _test_info);
bool d_tests_sa_index_is_valid(struct d_test_counter* _test_info);

// macro utility tests (in logical order)
bool d_tests_sa_macro_array_utilities(struct d_test_counter* _test_info);
bool d_tests_sa_macro_index_validation(struct d_test_counter* _test_info);
bool d_tests_sa_macro_negative_indexing(struct d_test_counter* _test_info);

// integration and consistency tests
bool d_tests_sa_function_macro_consistency(struct d_test_counter* _test_info);
bool d_tests_sa_extreme_edge_cases(struct d_test_counter* _test_info);

// comprehensive test suite runners
bool d_tests_sa_index_core_all(struct d_test_counter* _test_info);
bool d_tests_sa_macro_all(struct d_test_counter* _test_info);
bool d_tests_sa_integration_all(struct d_test_counter* _test_info);
bool d_tests_sa_djinterp_core_all(struct d_test_counter* _test_info);
bool d_tests_sa_djinterp_core_all_extended(struct d_test_counter* _test_info);


#endif	// DJINTERP_TESTING_HEADER_TYPES_




