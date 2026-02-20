/*******************************************************************************
* djinterp [test]                                             assert_tests_sa.h
*
*   Unit tests for the assert module (assertion types and operations).
*   Tests cover assertion creation, comparison operations, string assertions,
*   null checking, array assertions, and utility functions.
*
*
* path:      \tests\assert_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#ifndef DJINTERP_TESTING_ASSERT_STANDALONE_
#define DJINTERP_TESTING_ASSERT_STANDALONE_ 1

#include "..\..\inc\test\test_standalone.h"
#include "..\..\inc\test\assert.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_ASSERT_SHORT_STR
//   constant: short test string.
#define D_TEST_ASSERT_SHORT_STR         "Hello"

// D_TEST_ASSERT_MEDIUM_STR
//   constant: medium test string.
#define D_TEST_ASSERT_MEDIUM_STR        "Hello, World! Testing 123"

// D_TEST_ASSERT_BUFFER_SIZE
//   constant: standard buffer size for tests.
#define D_TEST_ASSERT_BUFFER_SIZE       256

// D_TEST_ASSERT_STRESS_COUNT
//   constant: number of iterations for stress tests.
#define D_TEST_ASSERT_STRESS_COUNT      100


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

bool d_tests_assert_setup(void);
bool d_tests_assert_teardown(void);
int  d_test_int_comparator(const void* _a, const void* _b);
bool d_test_match_function(const void* _value);
int* d_test_create_int_standalone(int _value);


/******************************************************************************
 * CORE FUNCTION TESTS (assert_tests_sa_core.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assertion_new(void);
struct d_test_object* d_tests_sa_assertion_free(void);
struct d_test_object* d_tests_sa_assert(void);
struct d_test_object* d_tests_sa_assert_false(void);

// category runner
struct d_test_object* d_tests_sa_assert_core_all(void);


/******************************************************************************
 * COMPARISON FUNCTION TESTS (assert_tests_sa_compare.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assert_eq(void);
struct d_test_object* d_tests_sa_assert_neq(void);
struct d_test_object* d_tests_sa_assert_lt(void);
struct d_test_object* d_tests_sa_assert_lt_eq(void);
struct d_test_object* d_tests_sa_assert_gt(void);
struct d_test_object* d_tests_sa_assert_gt_eq(void);

// category runner
struct d_test_object* d_tests_sa_assert_compare_all(void);


/******************************************************************************
 * STRING FUNCTION TESTS (assert_tests_sa_string.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assert_str_eq(void);
struct d_test_object* d_tests_sa_assert_str_neq(void);

// category runner
struct d_test_object* d_tests_sa_assert_string_all(void);


/******************************************************************************
 * NULL CHECKING FUNCTION TESTS (assert_tests_sa_null.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assert_null(void);
struct d_test_object* d_tests_sa_assert_nonnull(void);

// category runner
struct d_test_object* d_tests_sa_assert_null_all(void);


/******************************************************************************
 * ARRAY FUNCTION TESTS (assert_tests_sa_array.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assert_array_is_valid(void);
struct d_test_object* d_tests_sa_assert_arrays_eq(void);

// category runner
struct d_test_object* d_tests_sa_assert_array_all(void);


/******************************************************************************
 * UTILITY FUNCTION TESTS (assert_tests_sa_utility.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assert_default_compare(void);

// category runner
struct d_test_object* d_tests_sa_assert_utility_all(void);


/******************************************************************************
 * ADVANCED TESTS (assert_tests_sa_advanced.c)
 *****************************************************************************/

// individual tests
struct d_test_object* d_tests_sa_assert_integration(void);
struct d_test_object* d_tests_sa_assert_stress(void);

// category runner
struct d_test_object* d_tests_sa_assert_advanced_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

struct d_test_object* d_tests_sa_assert_all(void);


#endif  // DJINTERP_TESTING_ASSERT_STANDALONE_
