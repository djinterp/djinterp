/*******************************************************************************
* djinterp [test]                                       text_template_tests_sa.h
*
*   Unit tests for the text template module (text_template.h).
*   Tests cover template lifecycle, marker configuration, binding management,
* rendering (including nested template expansion), key wrapping utility,
* and utility functions.
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_
#define DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_ 1

// djinterp
#include "../../../../inc/djinterp/c/text/text_template.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_TMPL_BUFFER_SIZE
//   constant: standard buffer size for template tests.
#define D_TEST_TMPL_BUFFER_SIZE      512

// D_TEST_TMPL_SMALL_BUFFER
//   constant: deliberately small buffer for overflow tests.
#define D_TEST_TMPL_SMALL_BUFFER     8


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

bool d_tests_sa_text_template_setup(void);
bool d_tests_sa_text_template_teardown(void);


/******************************************************************************
 * TEMPLATE LIFECYCLE TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_new(void);
struct d_test_object* d_tests_sa_text_template_new_with_markers(void);
struct d_test_object* d_tests_sa_text_template_free(void);
struct d_test_object* d_tests_sa_text_template_clear(void);
struct d_test_object* d_tests_sa_text_template_lifecycle_all(void);


/******************************************************************************
 * MARKER CONFIGURATION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_set_markers(void);
struct d_test_object* d_tests_sa_text_template_get_prefix(void);
struct d_test_object* d_tests_sa_text_template_get_suffix(void);
struct d_test_object* d_tests_sa_text_template_marker_all(void);


/******************************************************************************
 * BINDING MANAGEMENT TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_bind_string(void);
struct d_test_object* d_tests_sa_text_template_bind_string_n(void);
struct d_test_object* d_tests_sa_text_template_bind_template(void);
struct d_test_object* d_tests_sa_text_template_bind_function(void);
struct d_test_object* d_tests_sa_text_template_bind_list(void);
struct d_test_object* d_tests_sa_text_template_unbind(void);
struct d_test_object* d_tests_sa_text_template_update_string(void);
struct d_test_object* d_tests_sa_text_template_has_binding(void);
struct d_test_object* d_tests_sa_text_template_binding_count(void);
struct d_test_object* d_tests_sa_text_template_binding_all(void);


/******************************************************************************
 * RENDERING TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_render(void);
struct d_test_object* d_tests_sa_text_template_render_alloc(void);
struct d_test_object* d_tests_sa_text_template_render_length(void);
struct d_test_object* d_tests_sa_text_template_render_nested(void);
struct d_test_object* d_tests_sa_text_template_render_all(void);


/******************************************************************************
 * KEY WRAPPING TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_wrap_keys(void);
struct d_test_object* d_tests_sa_text_template_wrap_keys_alloc(void);
struct d_test_object* d_tests_sa_text_template_wrap_all(void);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_error_string(void);
struct d_test_object* d_tests_sa_text_template_utility_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

struct d_test_object* d_tests_sa_text_template_run_all(void);


#endif  // DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_
