/*******************************************************************************
* djinterp [test]                                             ssize64_tests_sa.h
*
* This is a test file for `ssize64.h` unit tests.
* For the file itself, go to `\inc\ssize64.h`.
* Note: this module is required to build DTest, so it does not use DTest for
* unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      /tests/djinterp/c/ssize64_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.11.11
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_HEADER_TYPES_
#define DJINTERP_TESTING_HEADER_TYPES_ 1

// std
#include <stdlib.h>
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"
#include "../../../inc/djinterp/test/c/test_counter.h"


// `ssize64` creation and access tests
bool d_tests_sa_ssize64_creation(struct d_test_counter* _test_info);
bool d_tests_sa_ssize64_access(struct d_test_counter* _test_info);

// `ssize64` arithmetic operation tests
bool d_tests_sa_ssize64_arithmetic_functions(struct d_test_counter* _test_info);
bool d_tests_sa_ssize64_arithmetic_macros(struct d_test_counter* _test_info);

// `ssize64` comparison tests
bool d_tests_sa_ssize64_comparison_ssize64(struct d_test_counter* _test_info);
bool d_tests_sa_ssize64_comparison_other_types(struct d_test_counter* _test_info);

// `ssize64` conversion and validation tests
bool d_tests_sa_ssize64_conversion_functions(struct d_test_counter* _test_info);
bool d_tests_sa_ssize64_validation_functions(struct d_test_counter* _test_info);
bool d_tests_sa_ssize64_edge_cases(struct d_test_counter* _test_info);

#endif  // DJINTERP_TESTING_HEADER_TYPES_
