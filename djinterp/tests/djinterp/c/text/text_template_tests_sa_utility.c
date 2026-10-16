/*******************************************************************************
* djinterp [text]                               text_template_tests_sa_utility.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa_utility.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests_sa.h"


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_text_template_error_string
  Tests d_text_template_error_string for error code descriptions.
  Tests the following:
  - returns non-NULL for every known error code
  - each error code has a distinct string
  - SUCCESS string contains "success"
  - NULL_PARAM string contains "null"
  - BUFFER_TOO_SMALL string contains "buffer"
  - KEY_NOT_FOUND string contains "not found"
  - NESTING_TOO_DEEP string contains "nesting"
  - ALLOCATION_FAILED string contains "allocation"
  - INVALID_MARKER string contains "marker"
  - INTERP_FAILED string contains "interp"
  - CYCLE_DETECTED string contains "cycle"
  - FUNCTION_FAILED string contains "function"
  - LIST_CALLBACK_FAILED string contains "callback"
  - LIST_ITEM_RENDER_FAILED string contains "list"
  - returns non-NULL for unknown/invalid error code
*/
struct d_test_object*
d_tests_sa_text_template_error_string
(
    void
)
{
    struct d_test_object* group;
    const char*           s_success;
    const char*           s_null;
    const char*           s_buffer;
    const char*           s_key;
    const char*           s_nest;
    const char*           s_alloc;
    const char*           s_marker;
    const char*           s_interp;
    const char*           s_cycle;
    const char*           s_func;
    const char*           s_list_cb;
    const char*           s_list_render;
    const char*           s_unknown;
    bool                  test_all_non_null;
    bool                  test_distinct;
    bool                  test_success_str;
    bool                  test_null_str;
    bool                  test_buffer_str;
    bool                  test_key_str;
    bool                  test_nest_str;
    bool                  test_alloc_str;
    bool                  test_marker_str;
    bool                  test_interp_str;
    bool                  test_cycle_str;
    bool                  test_func_str;
    bool                  test_list_cb_str;
    bool                  test_list_render_str;
    bool                  test_unknown_str;
    size_t                idx;

    // gather all error strings
    s_success     = d_text_template_error_string(D_TEXT_TEMPLATE_SUCCESS);
    s_null        = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
    s_buffer      = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL);
    s_key         = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND);
    s_nest        = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_NESTING_TOO_DEEP);
    s_alloc       = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED);
    s_marker      = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_INVALID_MARKER);
    s_interp      = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_INTERP_FAILED);
    s_cycle       = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_CYCLE_DETECTED);
    s_func        = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_FUNCTION_FAILED);
    s_list_cb     = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_LIST_CALLBACK_FAILED);
    s_list_render = d_text_template_error_string(D_TEXT_TEMPLATE_ERROR_LIST_ITEM_RENDER_FAILED);
    s_unknown     = d_text_template_error_string((enum d_text_template_error)999);

    // test 1: all non-NULL
    test_all_non_null = ( (s_success     != NULL) &&
                          (s_null        != NULL) &&
                          (s_buffer      != NULL) &&
                          (s_key         != NULL) &&
                          (s_nest        != NULL) &&
                          (s_alloc       != NULL) &&
                          (s_marker      != NULL) &&
                          (s_interp      != NULL) &&
                          (s_cycle       != NULL) &&
                          (s_func        != NULL) &&
                          (s_list_cb     != NULL) &&
                          (s_list_render != NULL) &&
                          (s_unknown     != NULL) );

    // test 2: distinct strings (spot-check several pairs)
    test_distinct = ( (s_success     != s_null)        &&
                      (s_success     != s_buffer)      &&
                      (s_null        != s_key)         &&
                      (s_key         != s_nest)        &&
                      (s_nest        != s_alloc)       &&
                      (s_marker      != s_interp)      &&
                      (s_interp      != s_cycle)       &&
                      (s_cycle       != s_func)        &&
                      (s_func        != s_list_cb)     &&
                      (s_list_cb     != s_list_render) );

    // test 3: SUCCESS contains "success"
    test_success_str = false;
    if (s_success)
    {
        test_success_str = d_strcontains(s_success,
                                         d_strnlen(s_success, 128),
                                         "success");
    }

    // test 4: NULL_PARAM contains "null"
    test_null_str = false;
    if (s_null)
    {
        test_null_str = d_strcontains(s_null,
                                      d_strnlen(s_null, 128),
                                      "null");
    }

    // test 5: BUFFER_TOO_SMALL contains "buffer"
    test_buffer_str = false;
    if (s_buffer)
    {
        test_buffer_str = d_strcontains(s_buffer,
                                        d_strnlen(s_buffer, 128),
                                        "buffer");
    }

    // test 6: KEY_NOT_FOUND contains "not found"
    test_key_str = false;
    if (s_key)
    {
        test_key_str = d_strcontains(s_key,
                                     d_strnlen(s_key, 128),
                                     "not found");
    }

    // test 7: NESTING_TOO_DEEP contains "nesting"
    test_nest_str = false;
    if (s_nest)
    {
        test_nest_str = d_strcontains(s_nest,
                                      d_strnlen(s_nest, 128),
                                      "nesting");
    }

    // test 8: ALLOCATION_FAILED contains "allocation"
    test_alloc_str = false;
    if (s_alloc)
    {
        test_alloc_str = d_strcontains(s_alloc,
                                       d_strnlen(s_alloc, 128),
                                       "allocation");
    }

    // test 9: INVALID_MARKER contains "marker"
    test_marker_str = false;
    if (s_marker)
    {
        test_marker_str = d_strcontains(s_marker,
                                        d_strnlen(s_marker, 128),
                                        "marker");
    }

    // test 10: INTERP_FAILED contains "interp"
    test_interp_str = false;
    if (s_interp)
    {
        test_interp_str = d_strcontains(s_interp,
                                        d_strnlen(s_interp, 128),
                                        "interp");
    }

    // test 11: CYCLE_DETECTED contains "cycle"
    test_cycle_str = false;
    if (s_cycle)
    {
        test_cycle_str = d_strcontains(s_cycle,
                                       d_strnlen(s_cycle, 128),
                                       "cycle");
    }

    // test 12: FUNCTION_FAILED contains "function"
    test_func_str = false;
    if (s_func)
    {
        test_func_str = d_strcontains(s_func,
                                      d_strnlen(s_func, 128),
                                      "function");
    }

    // test 13: LIST_CALLBACK_FAILED contains "callback"
    test_list_cb_str = false;
    if (s_list_cb)
    {
        test_list_cb_str = d_strcontains(s_list_cb,
                                         d_strnlen(s_list_cb, 128),
                                         "callback");
    }

    // test 14: LIST_ITEM_RENDER_FAILED contains "list"
    test_list_render_str = false;
    if (s_list_render)
    {
        test_list_render_str = d_strcontains(s_list_render,
                                             d_strnlen(s_list_render, 128),
                                             "list");
    }

    // test 15: unknown error returns non-empty string
    test_unknown_str = false;
    if (s_unknown)
    {
        test_unknown_str = (d_strnlen(s_unknown, 128) > 0);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_template_error_string", 15);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("all_non_null",
                                           test_all_non_null,
                                           "all error strings are non-NULL");
    group->elements[idx++] = D_ASSERT_TRUE("distinct",
                                           test_distinct,
                                           "each error code has distinct string");
    group->elements[idx++] = D_ASSERT_TRUE("success_str",
                                           test_success_str,
                                           "SUCCESS contains 'success'");
    group->elements[idx++] = D_ASSERT_TRUE("null_str",
                                           test_null_str,
                                           "NULL_PARAM contains 'null'");
    group->elements[idx++] = D_ASSERT_TRUE("buffer_str",
                                           test_buffer_str,
                                           "BUFFER_TOO_SMALL contains 'buffer'");
    group->elements[idx++] = D_ASSERT_TRUE("key_str",
                                           test_key_str,
                                           "KEY_NOT_FOUND contains 'not found'");
    group->elements[idx++] = D_ASSERT_TRUE("nest_str",
                                           test_nest_str,
                                           "NESTING_TOO_DEEP contains 'nesting'");
    group->elements[idx++] = D_ASSERT_TRUE("alloc_str",
                                           test_alloc_str,
                                           "ALLOCATION_FAILED contains text");
    group->elements[idx++] = D_ASSERT_TRUE("marker_str",
                                           test_marker_str,
                                           "INVALID_MARKER contains 'marker'");
    group->elements[idx++] = D_ASSERT_TRUE("interp_str",
                                           test_interp_str,
                                           "INTERP_FAILED contains 'interp'");
    group->elements[idx++] = D_ASSERT_TRUE("cycle_str",
                                           test_cycle_str,
                                           "CYCLE_DETECTED contains 'cycle'");
    group->elements[idx++] = D_ASSERT_TRUE("func_str",
                                           test_func_str,
                                           "FUNCTION_FAILED contains 'function'");
    group->elements[idx++] = D_ASSERT_TRUE("list_cb_str",
                                           test_list_cb_str,
                                           "LIST_CALLBACK_FAILED contains text");
    group->elements[idx++] = D_ASSERT_TRUE("list_render_str",
                                           test_list_render_str,
                                           "LIST_ITEM_RENDER_FAILED contains text");
    group->elements[idx++] = D_ASSERT_TRUE("unknown_str",
                                           test_unknown_str,
                                           "unknown code returns non-empty");

    return group;
}

/*
d_tests_sa_text_template_utility_all
  Runs all utility function tests.
  Tests the following:
  - d_text_template_error_string
*/
struct d_test_object*
d_tests_sa_text_template_utility_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Utility Functions", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_template_error_string();

    return group;
}
