/*******************************************************************************
* djinterp [text]                             text_template_tests_sa_lifecycle.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa_lifecycle.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests_sa.h"


/******************************************************************************
 * STATIC TEST HELPERS
 *****************************************************************************/

/*
d_test_internal_lifecycle_fn
  Dummy callback for function binding free tests.
*/
static enum d_text_template_error
d_test_internal_lifecycle_fn
(
    void*  _context,
    char** _out_result
)
{
    (void)_context;

    *_out_result = d_strdup("lifecycle");

    if (!*_out_result)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_test_internal_lifecycle_list_fn
  Dummy per-item callback for list binding free tests.
*/
static bool
d_test_internal_lifecycle_list_fn
(
    size_t                  _index,
    size_t                  _count,
    struct d_text_template* _item_template,
    void*                   _context
)
{
    (void)_index;
    (void)_count;
    (void)_item_template;
    (void)_context;

    return true;
}


/******************************************************************************
 * TEMPLATE LIFECYCLE TESTS
 *****************************************************************************/

/*
d_tests_sa_text_template_new
  Tests d_text_template_new for default template creation.
  Tests the following:
  - returns non-NULL pointer
  - binding_count is zero after creation
  - bindings array is non-NULL
  - binding_capacity is D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY
  - max_nesting_depth is D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH
  - default prefix is D_TEXT_TEMPLATE_DEFAULT_PREFIX
  - default suffix is D_TEXT_TEMPLATE_DEFAULT_SUFFIX
  - template is usable immediately after creation
  - multiple templates are independent
*/
struct d_test_object*
d_tests_sa_text_template_new
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    struct d_text_template* tmpl2;
    const char*             pfx;
    const char*             sfx;
    bool                    test_not_null;
    bool                    test_count_zero;
    bool                    test_bindings_ptr;
    bool                    test_capacity;
    bool                    test_depth;
    bool                    test_default_prefix;
    bool                    test_default_suffix;
    bool                    test_usable;
    bool                    test_independent;
    size_t                  idx;

    // test 1: returns non-NULL
    tmpl = d_text_template_new();
    test_not_null = (tmpl != NULL);

    // test 2: binding_count is zero
    test_count_zero    = false;
    test_bindings_ptr  = false;
    test_capacity      = false;
    test_depth         = false;
    if (tmpl)
    {
        test_count_zero = (tmpl->binding_count == 0);

        // test 3: bindings array is non-NULL
        test_bindings_ptr = (tmpl->bindings != NULL);

        // test 4: binding_capacity matches default
        test_capacity =
            (tmpl->binding_capacity ==
             D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY);

        // test 5: max_nesting_depth matches default
        test_depth =
            (tmpl->max_nesting_depth ==
             D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH);
    }

    // test 6: default prefix
    test_default_prefix = false;
    if (tmpl)
    {
        pfx = d_text_template_get_prefix(tmpl);
        test_default_prefix =
            ( (pfx != NULL) &&
              (d_strequals(
                   pfx,
                   d_strnlen(pfx, 32),
                   D_TEXT_TEMPLATE_DEFAULT_PREFIX,
                   d_strnlen(
                       D_TEXT_TEMPLATE_DEFAULT_PREFIX,
                       32))) );
    }

    // test 7: default suffix
    test_default_suffix = false;
    if (tmpl)
    {
        sfx = d_text_template_get_suffix(tmpl);
        test_default_suffix =
            ( (sfx != NULL) &&
              (d_strequals(
                   sfx,
                   d_strnlen(sfx, 32),
                   D_TEXT_TEMPLATE_DEFAULT_SUFFIX,
                   d_strnlen(
                       D_TEXT_TEMPLATE_DEFAULT_SUFFIX,
                       32))) );
    }

    // test 8: immediately usable
    test_usable = false;
    if (tmpl)
    {
        enum d_text_template_error err;
        err = d_text_template_bind_string(tmpl, "key", "val");
        test_usable = (err == D_TEXT_TEMPLATE_SUCCESS);
    }

    // test 9: multiple templates are independent
    tmpl2 = d_text_template_new();
    test_independent = false;
    if (tmpl && tmpl2)
    {
        test_independent =
            ( (tmpl != tmpl2)             &&
              (tmpl->binding_count == 1)  &&
              (tmpl2->binding_count == 0) );
    }

    // cleanup
    if (tmpl)
    {
        d_text_template_free(tmpl);
    }

    if (tmpl2)
    {
        d_text_template_free(tmpl2);
    }

    // build result tree
    group = d_test_object_new_interior(
                "d_text_template_new",
                9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "not_null",
                                 test_not_null,
                                 "returns non-NULL pointer");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "count_zero",
                                 test_count_zero,
                                 "binding_count is zero");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "bindings_ptr",
                                 test_bindings_ptr,
                                 "bindings array is non-NULL");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "capacity",
                                 test_capacity,
                                 "binding_capacity matches default");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "depth",
                                 test_depth,
                                 "max_nesting_depth matches default");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "default_prefix",
                                 test_default_prefix,
                                 "default prefix matches constant");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "default_suffix",
                                 test_default_suffix,
                                 "default suffix matches constant");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "usable",
                                 test_usable,
                                 "template is usable immediately");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "independent",
                                 test_independent,
                                 "multiple templates independent");

    return group;
}

/*
d_tests_sa_text_template_new_with_markers
  Tests d_text_template_new_with_markers for custom-marker creation.
  Tests the following:
  - returns non-NULL pointer with valid markers
  - prefix stored correctly
  - suffix stored correctly
  - handles asymmetric markers ("${" / "}")
  - handles multi-char markers ("{{" / "}}")
  - returns NULL for NULL prefix
  - returns NULL for NULL suffix
  - returns NULL for empty prefix
  - returns NULL for empty suffix
*/
struct d_test_object*
d_tests_sa_text_template_new_with_markers
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    const char*             pfx;
    const char*             sfx;
    bool                    test_not_null;
    bool                    test_prefix;
    bool                    test_suffix;
    bool                    test_asymmetric;
    bool                    test_multi_char;
    bool                    test_null_prefix;
    bool                    test_null_suffix;
    bool                    test_empty_prefix;
    bool                    test_empty_suffix;
    size_t                  idx;

    // test 1: valid markers
    tmpl = d_text_template_new_with_markers("<<", ">>");
    test_not_null = (tmpl != NULL);

    // test 2: prefix stored
    test_prefix = false;
    if (tmpl)
    {
        pfx = d_text_template_get_prefix(tmpl);
        test_prefix = ( (pfx != NULL) &&
                        (d_strequals(pfx,
                                     d_strnlen(pfx, 32),
                                     "<<",
                                     2)) );
    }

    // test 3: suffix stored
    test_suffix = false;
    if (tmpl)
    {
        sfx = d_text_template_get_suffix(tmpl);
        test_suffix = ( (sfx != NULL) &&
                        (d_strequals(sfx,
                                     d_strnlen(sfx, 32),
                                     ">>",
                                     2)) );
        d_text_template_free(tmpl);
    }

    // test 4: asymmetric markers
    tmpl = d_text_template_new_with_markers("${", "}");
    test_asymmetric = false;
    if (tmpl)
    {
        pfx = d_text_template_get_prefix(tmpl);
        sfx = d_text_template_get_suffix(tmpl);
        test_asymmetric = ( (pfx != NULL) &&
                            (sfx != NULL) &&
                            (d_strequals(pfx, d_strnlen(pfx, 32), "${", 2)) &&
                            (d_strequals(sfx, d_strnlen(sfx, 32), "}",  1)) );
        d_text_template_free(tmpl);
    }

    // test 5: multi-char markers
    tmpl = d_text_template_new_with_markers("{{", "}}");
    test_multi_char = false;
    if (tmpl)
    {
        pfx = d_text_template_get_prefix(tmpl);
        sfx = d_text_template_get_suffix(tmpl);
        test_multi_char = ( (pfx != NULL) &&
                            (sfx != NULL) &&
                            (d_strequals(pfx, d_strnlen(pfx, 32), "{{", 2)) &&
                            (d_strequals(sfx, d_strnlen(sfx, 32), "}}", 2)) );
        d_text_template_free(tmpl);
    }

    // test 6: NULL prefix
    tmpl = d_text_template_new_with_markers(NULL, "}");
    test_null_prefix = (tmpl == NULL);

    // test 7: NULL suffix
    tmpl = d_text_template_new_with_markers("{", NULL);
    test_null_suffix = (tmpl == NULL);

    // test 8: empty prefix
    tmpl = d_text_template_new_with_markers("", "}");
    test_empty_prefix = (tmpl == NULL);

    // test 9: empty suffix
    tmpl = d_text_template_new_with_markers("{", "");
    test_empty_suffix = (tmpl == NULL);

    // build result tree
    group = d_test_object_new_interior("d_text_template_new_with_markers", 9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("not_null",
                                           test_not_null,
                                           "returns non-NULL with valid markers");
    group->elements[idx++] = D_ASSERT_TRUE("prefix",
                                           test_prefix,
                                           "prefix stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("suffix",
                                           test_suffix,
                                           "suffix stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("asymmetric",
                                           test_asymmetric,
                                           "handles asymmetric markers");
    group->elements[idx++] = D_ASSERT_TRUE("multi_char",
                                           test_multi_char,
                                           "handles multi-char markers");
    group->elements[idx++] = D_ASSERT_TRUE("null_prefix",
                                           test_null_prefix,
                                           "returns NULL for NULL prefix");
    group->elements[idx++] = D_ASSERT_TRUE("null_suffix",
                                           test_null_suffix,
                                           "returns NULL for NULL suffix");
    group->elements[idx++] = D_ASSERT_TRUE("empty_prefix",
                                           test_empty_prefix,
                                           "returns NULL for empty prefix");
    group->elements[idx++] = D_ASSERT_TRUE("empty_suffix",
                                           test_empty_suffix,
                                           "returns NULL for empty suffix");

    return group;
}

/*
d_tests_sa_text_template_free
  Tests d_text_template_free for template destruction.
  Tests the following:
  - does not crash on NULL
  - frees empty template safely
  - frees template with string bindings
  - frees template with template bindings (does not free nested)
  - frees template with function bindings (context survives)
  - frees template with list bindings (item_template survives)
*/
struct d_test_object*
d_tests_sa_text_template_free
(
    void
)
{
    struct d_test_object*                group;
    struct d_text_template*              tmpl;
    struct d_text_template*              nested;
    struct d_text_template*              item_tmpl;
    struct d_text_template_list_options   opts;
    bool                                 test_null_safe;
    bool                                 test_empty;
    bool                                 test_with_strings;
    bool                                 test_with_nested;
    bool                                 test_with_function;
    bool                                 test_with_list;
    int                                  dummy_ctx;
    size_t                               idx;

    dummy_ctx = 77;

    // test 1: does not crash on NULL
    d_text_template_free(NULL);
    test_null_safe = true;

    // test 2: frees empty template
    tmpl = d_text_template_new();
    test_empty = (tmpl != NULL);
    if (tmpl)
    {
        d_text_template_free(tmpl);
    }

    // test 3: frees template with string bindings
    tmpl = d_text_template_new();
    test_with_strings = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "a", "alpha");
        d_text_template_bind_string(tmpl, "b", "bravo");
        d_text_template_bind_string(tmpl, "c", "charlie");
        test_with_strings = (tmpl->binding_count == 3);
        d_text_template_free(tmpl);
    }

    // test 4: frees template with template binding
    tmpl   = d_text_template_new();
    nested = d_text_template_new();
    test_with_nested = false;
    if (tmpl && nested)
    {
        d_text_template_bind_string(nested, "inner", "value");
        d_text_template_bind_template(tmpl, "child", nested);
        d_text_template_free(tmpl);

        // nested should still be valid (not owned)
        test_with_nested = (nested->binding_count == 1);
        d_text_template_free(nested);
    }
    else
    {
        if (tmpl)
        {
            d_text_template_free(tmpl);
        }

        if (nested)
        {
            d_text_template_free(nested);
        }
    }

    // test 5: frees template with function binding
    tmpl = d_text_template_new();
    test_with_function = false;
    if (tmpl)
    {
        d_text_template_bind_function(
            tmpl,
            "fn_key",
            d_test_internal_lifecycle_fn,
            &dummy_ctx);
        test_with_function = (tmpl->binding_count == 1);
        d_text_template_free(tmpl);

        // context (stack var) survives; reaching here = no crash
    }

    // test 6: frees template with list binding (with options)
    tmpl      = d_text_template_new();
    item_tmpl = d_text_template_new();
    test_with_list = false;
    if (tmpl && item_tmpl)
    {
        d_memset(&opts, 0, sizeof(opts));
        opts.separator  = ", ";
        opts.empty_text = "(empty)";

        d_text_template_bind_list(
            tmpl,
            "list_key",
            item_tmpl,
            3,
            d_test_internal_lifecycle_list_fn,
            NULL,
            &opts);
        test_with_list = (tmpl->binding_count == 1);
        d_text_template_free(tmpl);

        // item_tmpl should still be valid (not owned)
        test_with_list = ( (test_with_list) &&
                           (item_tmpl->binding_count == 0) );
        d_text_template_free(item_tmpl);
    }
    else
    {
        if (tmpl)
        {
            d_text_template_free(tmpl);
        }

        if (item_tmpl)
        {
            d_text_template_free(item_tmpl);
        }
    }

    // build result tree
    group = d_test_object_new_interior(
                "d_text_template_free",
                6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_safe",
                                 test_null_safe,
                                 "does not crash on NULL");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "empty",
                                 test_empty,
                                 "frees empty template safely");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "with_strings",
                                 test_with_strings,
                                 "frees template with string bindings");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "with_nested",
                                 test_with_nested,
                                 "nested template survives parent free");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "with_function",
                                 test_with_function,
                                 "frees template with function binding");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "with_list",
                                 test_with_list,
                                 "frees template with list binding");

    return group;
}

/*
d_tests_sa_text_template_clear
  Tests d_text_template_clear for clearing bindings.
  Tests the following:
  - clears all bindings
  - binding_count becomes zero
  - template is reusable after clear
  - does not crash on NULL
  - does not crash on already-empty template
  - markers are preserved after clear
  - max_nesting_depth is preserved after clear
  - binding_capacity is preserved after clear
*/
struct d_test_object*
d_tests_sa_text_template_clear
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    const char*             pfx;
    bool                    test_clears;
    bool                    test_count_zero;
    bool                    test_reusable;
    bool                    test_null_safe;
    bool                    test_empty_clear;
    bool                    test_markers_kept;
    bool                    test_depth_kept;
    bool                    test_capacity_kept;
    size_t                  saved_capacity;
    size_t                  idx;

    tmpl = d_text_template_new_with_markers("{{", "}}");

    test_clears        = false;
    test_count_zero    = false;
    test_reusable      = false;
    test_markers_kept  = false;
    test_depth_kept    = false;
    test_capacity_kept = false;

    if (tmpl)
    {
        // configure a non-default depth to verify preservation
        d_text_template_set_max_depth(tmpl, 5);

        d_text_template_bind_string(tmpl, "x", "1");
        d_text_template_bind_string(tmpl, "y", "2");
        d_text_template_bind_string(tmpl, "z", "3");

        // test 1: had bindings before clear
        test_clears = (tmpl->binding_count == 3);

        // snapshot capacity before clear
        saved_capacity = tmpl->binding_capacity;

        d_text_template_clear(tmpl);

        // test 2: count is zero
        test_count_zero = (tmpl->binding_count == 0);

        // test 3: reusable
        enum d_text_template_error err;
        err = d_text_template_bind_string(tmpl, "w", "4");
        test_reusable = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (tmpl->binding_count == 1) );

        // test 6: markers preserved
        pfx = d_text_template_get_prefix(tmpl);
        test_markers_kept =
            ( (pfx != NULL) &&
              (d_strequals(pfx,
                           d_strnlen(pfx, 32),
                           "{{",
                           2)) );

        // test 7: max_nesting_depth preserved
        test_depth_kept =
            (tmpl->max_nesting_depth == 5);

        // test 8: binding_capacity preserved
        test_capacity_kept =
            (tmpl->binding_capacity == saved_capacity);

        d_text_template_free(tmpl);
    }

    // test 4: does not crash on NULL
    d_text_template_clear(NULL);
    test_null_safe = true;

    // test 5: does not crash on empty template
    tmpl = d_text_template_new();
    test_empty_clear = false;
    if (tmpl)
    {
        d_text_template_clear(tmpl);
        test_empty_clear = (tmpl->binding_count == 0);
        d_text_template_free(tmpl);
    }

    // build result tree
    group = d_test_object_new_interior(
                "d_text_template_clear",
                8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "clears",
                                 test_clears,
                                 "had bindings before clear");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "count_zero",
                                 test_count_zero,
                                 "binding_count is zero after clear");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "reusable",
                                 test_reusable,
                                 "template is reusable after clear");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_safe",
                                 test_null_safe,
                                 "does not crash on NULL");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "empty_clear",
                                 test_empty_clear,
                                 "clears already-empty template");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "markers_kept",
                                 test_markers_kept,
                                 "markers preserved after clear");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "depth_kept",
                                 test_depth_kept,
                                 "max_nesting_depth preserved");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "capacity_kept",
                                 test_capacity_kept,
                                 "binding_capacity preserved");

    return group;
}

/*
d_tests_sa_text_template_lifecycle_all
  Runs all template lifecycle tests.
  Tests the following:
  - d_text_template_new
  - d_text_template_new_with_markers
  - d_text_template_free
  - d_text_template_clear
*/
struct d_test_object*
d_tests_sa_text_template_lifecycle_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Template Lifecycle", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_template_new();
    group->elements[idx++] = d_tests_sa_text_template_new_with_markers();
    group->elements[idx++] = d_tests_sa_text_template_free();
    group->elements[idx++] = d_tests_sa_text_template_clear();

    return group;
}
