/*******************************************************************************
* djinterp [text]                                          text_template_tests.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_text_template_setup
  Sets up test environment for text template tests.
*/
bool d_tests_text_template_setup(void)
{
    /* no specific setup needed; present for consistency */

    return true;
}

/*
d_tests_text_template_teardown
  Cleans up test environment.
*/
bool d_tests_text_template_teardown(void)
{
    /* no specific teardown needed; present for consistency */

    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_text_template_run_all
  Master test runner for all DTextTemplate module tests.
  Tests the following:
  - Template lifecycle (new, new_with_markers, free, clear)
  - Marker configuration (set_markers, get_prefix, get_suffix)
  - Binding management (bind_string, bind_string_n, bind_template, unbind,
    update_string, has_binding)
  - Rendering (render, render_alloc, render_length, nested templates)
  - Key wrapping (wrap_keys, wrap_keys_alloc)
  - Utility functions (error_string)
*/
bool d_tests_text_template_run_all(struct dj_test_suite* _suite)
{
    bool                  setup_ok;

    // setup test environment
    setup_ok = d_tests_text_template_setup();

    if (!setup_ok)
    {
        printf("Failed to setup text_template test environment\n");

        return false;
    }

    // create master group
    d_test_suite_block(_suite, "text_template Module Tests");

    // run all test categories
    d_tests_text_template_lifecycle_all(_suite);
    d_tests_text_template_marker_all(_suite);
    d_tests_text_template_binding_all(_suite);
    d_tests_text_template_render_all(_suite);
    d_tests_text_template_wrap_all(_suite);
    d_tests_text_template_utility_all(_suite);

    // cleanup
    d_tests_text_template_teardown();

    return true;
}
