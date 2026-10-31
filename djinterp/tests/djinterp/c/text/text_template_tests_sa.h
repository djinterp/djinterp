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

struct dj_test_object* d_tests_sa_text_template_new(void);
struct dj_test_object* d_tests_sa_text_template_new_with_markers(void);
struct dj_test_object* d_tests_sa_text_template_free(void);
struct dj_test_object* d_tests_sa_text_template_clear(void);
struct dj_test_object* d_tests_sa_text_template_lifecycle_all(void);


/******************************************************************************
 * MARKER CONFIGURATION TESTS
 *****************************************************************************/

struct dj_test_object* d_tests_sa_text_template_set_markers(void);
struct dj_test_object* d_tests_sa_text_template_get_prefix(void);
struct dj_test_object* d_tests_sa_text_template_get_suffix(void);
struct dj_test_object* d_tests_sa_text_template_marker_all(void);


/******************************************************************************
 * BINDING MANAGEMENT TESTS
 *****************************************************************************/

struct dj_test_object* d_tests_sa_text_template_bind_string(void);
struct dj_test_object* d_tests_sa_text_template_bind_string_n(void);
struct dj_test_object* d_tests_sa_text_template_bind_template(void);
struct dj_test_object* d_tests_sa_text_template_bind_function(void);
struct dj_test_object* d_tests_sa_text_template_bind_list(void);
struct dj_test_object* d_tests_sa_text_template_unbind(void);
struct dj_test_object* d_tests_sa_text_template_update_string(void);
struct dj_test_object* d_tests_sa_text_template_has_binding(void);
struct dj_test_object* d_tests_sa_text_template_binding_count(void);
struct dj_test_object* d_tests_sa_text_template_binding_all(void);


/******************************************************************************
 * RENDERING TESTS
 *****************************************************************************/

struct dj_test_object* d_tests_sa_text_template_render(void);
struct dj_test_object* d_tests_sa_text_template_render_alloc(void);
struct dj_test_object* d_tests_sa_text_template_render_length(void);
struct dj_test_object* d_tests_sa_text_template_render_nested(void);
struct dj_test_object* d_tests_sa_text_template_render_all(void);


/******************************************************************************
 * KEY WRAPPING TESTS
 *****************************************************************************/

struct dj_test_object* d_tests_sa_text_template_wrap_keys(void);
struct dj_test_object* d_tests_sa_text_template_wrap_keys_alloc(void);
struct dj_test_object* d_tests_sa_text_template_wrap_all(void);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

struct dj_test_object* d_tests_sa_text_template_error_string(void);
struct dj_test_object* d_tests_sa_text_template_utility_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

struct dj_test_object* d_tests_sa_text_template_run_all(void);


#endif  // DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_
