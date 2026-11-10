/*******************************************************************************
* djinterp [test]                                          text_template_tests.h
*
*   Unit tests for the text template module (text_template.h).
*   Tests cover template lifecycle, marker configuration, binding management,
* rendering (including nested template expansion), key wrapping utility,
* and utility functions.
*
*
* path:      /tests/djinterp/c/text/text_template_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_
#define DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_ 1

// djinterp
#include "../../../../inc/djinterp/c/text/text_template.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's
                                                                // C face


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

bool d_tests_text_template_setup(void);
bool d_tests_text_template_teardown(void);


/******************************************************************************
 * TEMPLATE LIFECYCLE TESTS
 *****************************************************************************/

bool d_tests_text_template_new(struct dj_test_suite* _suite);
bool d_tests_text_template_new_with_markers(struct dj_test_suite* _suite);
bool d_tests_text_template_free(struct dj_test_suite* _suite);
bool d_tests_text_template_clear(struct dj_test_suite* _suite);
bool d_tests_text_template_lifecycle_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MARKER CONFIGURATION TESTS
 *****************************************************************************/

bool d_tests_text_template_set_markers(struct dj_test_suite* _suite);
bool d_tests_text_template_get_prefix(struct dj_test_suite* _suite);
bool d_tests_text_template_get_suffix(struct dj_test_suite* _suite);
bool d_tests_text_template_marker_all(struct dj_test_suite* _suite);


/******************************************************************************
 * BINDING MANAGEMENT TESTS
 *****************************************************************************/

bool d_tests_text_template_bind_string(struct dj_test_suite* _suite);
bool d_tests_text_template_bind_string_n(struct dj_test_suite* _suite);
bool d_tests_text_template_bind_template(struct dj_test_suite* _suite);
bool d_tests_text_template_bind_function(struct dj_test_suite* _suite);
bool d_tests_text_template_bind_list(struct dj_test_suite* _suite);
bool d_tests_text_template_unbind(struct dj_test_suite* _suite);
bool d_tests_text_template_update_string(struct dj_test_suite* _suite);
bool d_tests_text_template_has_binding(struct dj_test_suite* _suite);
bool d_tests_text_template_binding_count(struct dj_test_suite* _suite);
bool d_tests_text_template_binding_all(struct dj_test_suite* _suite);


/******************************************************************************
 * RENDERING TESTS
 *****************************************************************************/

bool d_tests_text_template_render(struct dj_test_suite* _suite);
bool d_tests_text_template_render_alloc(struct dj_test_suite* _suite);
bool d_tests_text_template_render_length(struct dj_test_suite* _suite);
bool d_tests_text_template_render_nested(struct dj_test_suite* _suite);
bool d_tests_text_template_render_all(struct dj_test_suite* _suite);


/******************************************************************************
 * KEY WRAPPING TESTS
 *****************************************************************************/

bool d_tests_text_template_wrap_keys(struct dj_test_suite* _suite);
bool d_tests_text_template_wrap_keys_alloc(struct dj_test_suite* _suite);
bool d_tests_text_template_wrap_all(struct dj_test_suite* _suite);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

bool d_tests_text_template_error_string(struct dj_test_suite* _suite);
bool d_tests_text_template_utility_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

bool d_tests_text_template_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TEXT_TEMPLATE_TESTS_STANDALONE_
