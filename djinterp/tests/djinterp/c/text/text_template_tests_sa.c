/*******************************************************************************
* djinterp [text]                                       text_template_tests_sa.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests_sa.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_text_template_setup
  Sets up test environment for text template tests.
*/
bool
d_tests_sa_text_template_setup
(
    void
)
{
    /* no specific setup needed; present for consistency */

    return true;
}

/*
d_tests_sa_text_template_teardown
  Cleans up test environment.
*/
bool
d_tests_sa_text_template_teardown
(
    void
)
{
    /* no specific teardown needed; present for consistency */

    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_sa_text_template_run_all
  Master test runner for all text_template module tests.
  Tests the following:
  - Template lifecycle (new, new_with_markers, free, clear)
  - Marker configuration (set_markers, get_prefix, get_suffix)
  - Binding management (bind_string, bind_string_n, bind_template, unbind,
    update_string, has_binding)
  - Rendering (render, render_alloc, render_length, nested templates)
  - Key wrapping (wrap_keys, wrap_keys_alloc)
  - Utility functions (error_string)
*/
struct d_test_object*
d_tests_sa_text_template_run_all
(
    void
)
{
    struct d_test_object* group;
    bool                  setup_ok;
    size_t                idx;

    // setup test environment
    setup_ok = d_tests_sa_text_template_setup();

    if (!setup_ok)
    {
        printf("Failed to setup text_template test environment\n");

        return NULL;
    }

    // create master group
    group = d_test_object_new_interior("text_template Module Tests", 6);

    if (!group)
    {
        d_tests_sa_text_template_teardown();

        return NULL;
    }

    // run all test categories
    idx = 0;
    group->elements[idx++] = d_tests_sa_text_template_lifecycle_all();
    group->elements[idx++] = d_tests_sa_text_template_marker_all();
    group->elements[idx++] = d_tests_sa_text_template_binding_all();
    group->elements[idx++] = d_tests_sa_text_template_render_all();
    group->elements[idx++] = d_tests_sa_text_template_wrap_all();
    group->elements[idx++] = d_tests_sa_text_template_utility_all();

    // cleanup
    d_tests_sa_text_template_teardown();

    return group;
}
