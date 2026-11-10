/*******************************************************************************
* djinterp [test]                                             str_interp_tests.h
*
*   Unit tests for the string interpolation module (string_interp.h).
*   Tests cover context lifecycle, specifier management, interpolation
* functions, convenience wrappers, and utility functions.
*
*
* path:      /tests/djinterp/c/text/str_interp_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.26
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEXT_STR_INTERP_TESTS_STANDALONE_
#define DJINTERP_TEXT_STR_INTERP_TESTS_STANDALONE_ 1

// djinterp
#include "../../../../inc/djinterp/c/text/str_interp.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_INTERP_BUFFER_SIZE
//   constant: standard buffer size for interpolation tests.
#define D_TEST_INTERP_BUFFER_SIZE      512

// D_TEST_INTERP_SMALL_BUFFER
//   constant: deliberately small buffer for overflow tests.
#define D_TEST_INTERP_SMALL_BUFFER     8


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

bool d_tests_str_interp_setup(void);
bool d_tests_str_interp_teardown(void);


/******************************************************************************
 * CONTEXT MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_str_interp_context_new(struct dj_test_suite* _suite);
bool d_tests_str_interp_context_free(struct dj_test_suite* _suite);
bool d_tests_str_interp_context_clear(struct dj_test_suite* _suite);
bool d_tests_str_interp_context_all(struct dj_test_suite* _suite);


/******************************************************************************
 * SPECIFIER MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_str_interp_add_specifier(struct dj_test_suite* _suite);
bool d_tests_str_interp_add_specifier_n(struct dj_test_suite* _suite);
bool d_tests_str_interp_remove_specifier(struct dj_test_suite* _suite);
bool d_tests_str_interp_update_specifier(struct dj_test_suite* _suite);
bool d_tests_str_interp_get_specifier(struct dj_test_suite* _suite);
bool d_tests_str_interp_has_specifier(struct dj_test_suite* _suite);
bool d_tests_str_interp_specifier_all(struct dj_test_suite* _suite);


/******************************************************************************
 * INTERPOLATION FUNCTION TESTS
 *****************************************************************************/

bool d_tests_str_interp_interp(struct dj_test_suite* _suite);
bool d_tests_str_interp_alloc(struct dj_test_suite* _suite);
bool d_tests_str_interp_length(struct dj_test_suite* _suite);
bool d_tests_str_interp_interp_all(struct dj_test_suite* _suite);


/******************************************************************************
 * CONVENIENCE FUNCTION TESTS
 *****************************************************************************/

bool d_tests_str_interp_quick(struct dj_test_suite* _suite);
bool d_tests_str_interp_convenience_all(struct dj_test_suite* _suite);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

bool d_tests_str_interp_error_string(struct dj_test_suite* _suite);
bool d_tests_str_interp_utility_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

bool d_tests_str_interp_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TEXT_STR_INTERP_TESTS_STANDALONE_
