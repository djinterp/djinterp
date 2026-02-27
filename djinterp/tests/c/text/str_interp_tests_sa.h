/******************************************************************************
* djinterp [test]                                      str_interp_tests_sa.h
*
*   Unit tests for the string interpolation module (string_interp.h).
*   Tests cover context lifecycle, specifier management, interpolation
* functions, convenience wrappers, and utility functions.
*
*
* path:      /inc/tests/c/text/str_interp_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.26
******************************************************************************/

#ifndef DJINTERP_TEXT_STR_INTERP_TESTS_STANDALONE_
#define DJINTERP_TEXT_STR_INTERP_TESTS_STANDALONE_ 1

#include "../../../inc/c/test/test_standalone.h"
#include "../../../inc/c/text/str_interp.h"


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

bool d_tests_sa_str_interp_setup(void);
bool d_tests_sa_str_interp_teardown(void);


/******************************************************************************
 * CONTEXT MANAGEMENT TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_str_interp_context_new(void);
struct d_test_object* d_tests_sa_str_interp_context_free(void);
struct d_test_object* d_tests_sa_str_interp_context_clear(void);
struct d_test_object* d_tests_sa_str_interp_context_all(void);


/******************************************************************************
 * SPECIFIER MANAGEMENT TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_str_interp_add_specifier(void);
struct d_test_object* d_tests_sa_str_interp_add_specifier_n(void);
struct d_test_object* d_tests_sa_str_interp_remove_specifier(void);
struct d_test_object* d_tests_sa_str_interp_update_specifier(void);
struct d_test_object* d_tests_sa_str_interp_get_specifier(void);
struct d_test_object* d_tests_sa_str_interp_has_specifier(void);
struct d_test_object* d_tests_sa_str_interp_specifier_all(void);


/******************************************************************************
 * INTERPOLATION FUNCTION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_str_interp_interp(void);
struct d_test_object* d_tests_sa_str_interp_alloc(void);
struct d_test_object* d_tests_sa_str_interp_length(void);
struct d_test_object* d_tests_sa_str_interp_interp_all(void);


/******************************************************************************
 * CONVENIENCE FUNCTION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_str_interp_quick(void);
struct d_test_object* d_tests_sa_str_interp_convenience_all(void);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_str_interp_error_string(void);
struct d_test_object* d_tests_sa_str_interp_utility_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

struct d_test_object* d_tests_sa_str_interp_run_all(void);


#endif  // DJINTERP_TEXT_STR_INTERP_TESTS_STANDALONE_
