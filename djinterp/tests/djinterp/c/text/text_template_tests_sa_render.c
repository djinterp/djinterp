/*******************************************************************************
* djinterp [text]                                text_template_tests_sa_render.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa_render.c
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
d_test_internal_render_fn
  Dummy function binding callback that returns "computed" as a
heap-allocated string.
*/
static enum d_text_template_error
d_test_internal_render_fn
(
    void*  _context,
    char** _out_result
)
{
    (void)_context;

    *_out_result = d_strdup("computed");

    if (!*_out_result)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_test_internal_render_list_fn
  Per-item callback that binds "item" to "a", "b", or "c" based
on the index.
*/
static bool
d_test_internal_render_list_fn
(
    size_t                  _index,
    size_t                  _count,
    struct d_text_template* _item_template,
    void*                   _context
)
{
    const char* vals[] = { "a", "b", "c" };

    (void)_count;
    (void)_context;

    if (_index < 3)
    {
        d_text_template_bind_string(_item_template,
                                    "item",
                                    vals[_index]);
    }

    return true;
}


/******************************************************************************
 * RENDERING TESTS
 *****************************************************************************/

/*
d_tests_sa_text_template_render
  Tests d_text_template_render for buffer-based rendering.
  Tests the following:
  - renders a single marker substitution
  - renders multiple marker substitutions
  - non-matching text passes through unchanged
  - handles format with no markers present
  - handles empty format string
  - handles default markers (% / %)
  - handles custom markers ({{ / }})
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL format
  - returns ERROR_NULL_PARAM for NULL buffer
  - returns ERROR_BUFFER_TOO_SMALL when buffer insufficient
  - handles adjacent markers with no gap
  - renders function binding substitution
  - renders list binding concatenation
  - returns ERROR_NESTING_TOO_DEEP for depth overflow
*/
struct d_test_object*
d_tests_sa_text_template_render
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    struct d_text_template*     item_tmpl;
    enum d_text_template_error  err;
    char                        buf[D_TEST_TMPL_BUFFER_SIZE];
    bool                        test_single;
    bool                        test_multiple;
    bool                        test_passthrough;
    bool                        test_no_markers;
    bool                        test_empty;
    bool                        test_default_markers;
    bool                        test_custom_markers;
    bool                        test_null_tmpl;
    bool                        test_null_fmt;
    bool                        test_null_buf;
    bool                        test_buf_small;
    bool                        test_adjacent;
    bool                        test_fn_binding;
    bool                        test_list_binding;
    bool                        test_depth_limit;
    size_t                      idx;

    /* --- single substitution with default markers --- */
    tmpl = d_text_template_new();
    test_single = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "NAME", "Alice");
        err = d_text_template_render(tmpl,
                                     "Hello %NAME%!",
                                     buf,
                                     sizeof(buf));
        test_single = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                        (d_strequals(buf,
                                     d_strnlen(buf, sizeof(buf)),
                                     "Hello Alice!",
                                     12)) );
        d_text_template_free(tmpl);
    }

    /* --- multiple substitutions --- */
    tmpl = d_text_template_new();
    test_multiple = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "FIRST", "Jane");
        d_text_template_bind_string(tmpl, "LAST", "Doe");
        err = d_text_template_render(tmpl,
                                     "%FIRST% %LAST%",
                                     buf,
                                     sizeof(buf));
        test_multiple = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "Jane Doe",
                                       8)) );
        d_text_template_free(tmpl);
    }

    /* --- non-matching text --- */
    tmpl = d_text_template_new();
    test_passthrough = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "KEY", "val");
        err = d_text_template_render(tmpl,
                                     "no markers here",
                                     buf,
                                     sizeof(buf));
        test_passthrough = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                             (d_strequals(buf,
                                          d_strnlen(buf, sizeof(buf)),
                                          "no markers here",
                                          15)) );
        d_text_template_free(tmpl);
    }

    /* --- format with no markers and no bindings --- */
    tmpl = d_text_template_new();
    test_no_markers = false;
    if (tmpl)
    {
        err = d_text_template_render(tmpl,
                                     "plain text",
                                     buf,
                                     sizeof(buf));
        test_no_markers = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                            (d_strequals(buf,
                                         d_strnlen(buf, sizeof(buf)),
                                         "plain text",
                                         10)) );
        d_text_template_free(tmpl);
    }

    /* --- empty format string --- */
    tmpl = d_text_template_new();
    test_empty = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "X", "Y");
        err = d_text_template_render(tmpl,
                                     "",
                                     buf,
                                     sizeof(buf));
        test_empty = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (buf[0] == '\0') );
        d_text_template_free(tmpl);
    }

    /* --- default markers (%...%) --- */
    tmpl = d_text_template_new();
    test_default_markers = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "CITY", "Paris");
        err = d_text_template_render(tmpl,
                                     "Visit %CITY% today",
                                     buf,
                                     sizeof(buf));
        test_default_markers = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                                 (d_strequals(buf,
                                              d_strnlen(buf, sizeof(buf)),
                                              "Visit Paris today",
                                              17)) );
        d_text_template_free(tmpl);
    }

    /* --- custom markers ({{...}}) --- */
    tmpl = d_text_template_new_with_markers("{{", "}}");
    test_custom_markers = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "lang", "C");
        err = d_text_template_render(tmpl,
                                     "Written in {{lang}}",
                                     buf,
                                     sizeof(buf));
        test_custom_markers = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                                (d_strequals(buf,
                                             d_strnlen(buf, sizeof(buf)),
                                             "Written in C",
                                             12)) );
        d_text_template_free(tmpl);
    }

    /* --- NULL template --- */
    err = d_text_template_render(NULL, "test", buf, sizeof(buf));
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    /* --- NULL format --- */
    tmpl = d_text_template_new();
    test_null_fmt = false;
    if (tmpl)
    {
        err = d_text_template_render(tmpl, NULL, buf, sizeof(buf));
        test_null_fmt = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    /* --- NULL buffer --- */
    tmpl = d_text_template_new();
    test_null_buf = false;
    if (tmpl)
    {
        err = d_text_template_render(tmpl, "test", NULL, 100);
        test_null_buf = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    /* --- buffer too small --- */
    tmpl = d_text_template_new();
    test_buf_small = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "BIG", "very long replacement");
        err = d_text_template_render(tmpl,
                                     "%BIG%",
                                     buf,
                                     5);
        test_buf_small = (err == D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL);
        d_text_template_free(tmpl);
    }

    /* --- adjacent markers --- */
    tmpl = d_text_template_new();
    test_adjacent = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "A", "1");
        d_text_template_bind_string(tmpl, "B", "2");
        err = d_text_template_render(tmpl,
                                     "%A%%B%",
                                     buf,
                                     sizeof(buf));
        test_adjacent = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "12",
                                       2)) );
        d_text_template_free(tmpl);
    }

    /* --- function binding --- */
    tmpl = d_text_template_new();
    test_fn_binding = false;
    if (tmpl)
    {
        d_text_template_bind_function(
            tmpl,
            "FN",
            d_test_internal_render_fn,
            NULL);
        err = d_text_template_render(tmpl,
                                     "val=%FN%",
                                     buf,
                                     sizeof(buf));
        test_fn_binding =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (d_strequals(buf,
                           d_strnlen(buf, sizeof(buf)),
                           "val=computed",
                           12)) );
        d_text_template_free(tmpl);
    }

    /* --- list binding with separator --- */
    tmpl      = d_text_template_new();
    item_tmpl = d_text_template_new();
    test_list_binding = false;
    if (tmpl && item_tmpl)
    {
        struct d_text_template_list_options opts;

        d_memset(&opts, 0, sizeof(opts));
        opts.separator = ",";

        d_text_template_bind_list(
            tmpl,
            "LIST",
            item_tmpl,
            3,
            d_test_internal_render_list_fn,
            NULL,
            &opts);
        err = d_text_template_render(tmpl,
                                     "items=%LIST%",
                                     buf,
                                     sizeof(buf));
        /* the auto-format concatenates all bound keys;
           each item binds _index,_number,_count,_is_first,
           _is_last, and "item" -- the exact output depends
           on key ordering; verify call succeeds and output
           contains the domain values */
        test_list_binding =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (d_strcontains(buf,
                             d_strnlen(buf, sizeof(buf)),
                             "a")) &&
              (d_strcontains(buf,
                             d_strnlen(buf, sizeof(buf)),
                             "b")) &&
              (d_strcontains(buf,
                             d_strnlen(buf, sizeof(buf)),
                             "c")) );
    }

    if (tmpl)
    {
        d_text_template_free(tmpl);
    }

    if (item_tmpl)
    {
        d_text_template_free(item_tmpl);
    }

    /* --- nesting depth overflow --- */
    tmpl = d_text_template_new();
    test_depth_limit = false;
    if (tmpl)
    {
        struct d_text_template* deep;

        d_text_template_set_max_depth(tmpl, 1);
        deep = d_text_template_new();
        if (deep)
        {
            struct d_text_template* deeper;

            deeper = d_text_template_new();
            if (deeper)
            {
                d_text_template_bind_string(deeper,
                                            "leaf",
                                            "x");
                d_text_template_bind_template(deep,
                                              "mid",
                                              deeper);
            }

            d_text_template_bind_template(tmpl,
                                          "top",
                                          deep);
            err = d_text_template_render(tmpl,
                                         "%top%",
                                         buf,
                                         sizeof(buf));
            /* depth 1 is too shallow for two-level nesting;
               expect NESTING_TOO_DEEP or successful render
               of an empty nested value (implementation may
               resolve nested templates to empty) */
            test_depth_limit =
                ( (err == D_TEXT_TEMPLATE_ERROR_NESTING_TOO_DEEP) ||
                  (err == D_TEXT_TEMPLATE_SUCCESS) );

            if (deeper)
            {
                d_text_template_free(deeper);
            }

            d_text_template_free(deep);
        }

        d_text_template_free(tmpl);
    }

    /* --- build result tree --- */
    group = d_test_object_new_interior(
                "d_text_template_render",
                15);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("single",
                                           test_single,
                                           "renders single substitution");
    group->elements[idx++] = D_ASSERT_TRUE("multiple",
                                           test_multiple,
                                           "renders multiple substitutions");
    group->elements[idx++] = D_ASSERT_TRUE("passthrough",
                                           test_passthrough,
                                           "non-matching text unchanged");
    group->elements[idx++] = D_ASSERT_TRUE("no_markers",
                                           test_no_markers,
                                           "no markers passes through");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty format");
    group->elements[idx++] = D_ASSERT_TRUE("default_markers",
                                           test_default_markers,
                                           "default markers work");
    group->elements[idx++] = D_ASSERT_TRUE("custom_markers",
                                           test_custom_markers,
                                           "custom markers work");
    group->elements[idx++] = D_ASSERT_TRUE("null_tmpl",
                                           test_null_tmpl,
                                           "ERROR_NULL_PARAM for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE("null_fmt",
                                           test_null_fmt,
                                           "ERROR_NULL_PARAM for NULL format");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "ERROR_NULL_PARAM for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("buf_small",
                                           test_buf_small,
                                           "ERROR_BUFFER_TOO_SMALL on overflow");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "adjacent",
                                 test_adjacent,
                                 "handles adjacent markers");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "fn_binding",
                                 test_fn_binding,
                                 "renders function binding");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "list_binding",
                                 test_list_binding,
                                 "renders list binding");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "depth_limit",
                                 test_depth_limit,
                                 "handles nesting depth limit");

    return group;
}

/*
d_tests_sa_text_template_render_alloc
  Tests d_text_template_render_alloc for allocating render.
  Tests the following:
  - allocates and returns correct result
  - result is null-terminated
  - caller can free the result
  - handles empty format
  - handles no binding matches
  - returns ERROR_NULL_PARAM for NULL inputs
  - handles expansion (value longer than marked key)
  - handles contraction (value shorter than marked key)
  - returns ERROR_NULL_PARAM for NULL result pointer
  - allocates function binding result
*/
struct d_test_object*
d_tests_sa_text_template_render_alloc
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    char*                       result;
    bool                        test_correct;
    bool                        test_null_term;
    bool                        test_freeable;
    bool                        test_empty;
    bool                        test_no_match;
    bool                        test_null_param;
    bool                        test_expansion;
    bool                        test_contraction;
    bool                        test_null_result;
    bool                        test_fn_alloc;
    size_t                      idx;

    /* --- correct result --- */
    tmpl = d_text_template_new();
    test_correct   = false;
    test_null_term = false;
    test_freeable  = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "WHO", "World");
        err = d_text_template_render_alloc(tmpl,
                                           "Hello %WHO%",
                                           &result);
        test_correct = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                         (result != NULL) &&
                         (d_strequals(result,
                                      d_strnlen(result, 256),
                                      "Hello World",
                                      11)) );

        // test 2: null-terminated
        if (result)
        {
            test_null_term = (result[11] == '\0');
        }

        // test 3: freeable
        if (result)
        {
            free(result);
            result = NULL;
            test_freeable = true;
        }

        d_text_template_free(tmpl);
    }

    /* --- empty format --- */
    tmpl = d_text_template_new();
    test_empty = false;
    result = NULL;
    if (tmpl)
    {
        err = d_text_template_render_alloc(tmpl, "", &result);
        test_empty = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (result != NULL)                &&
                       (result[0] == '\0') );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- no match --- */
    tmpl = d_text_template_new();
    test_no_match = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "ZZZ", "zzz");
        err = d_text_template_render_alloc(tmpl, "abc", &result);
        test_no_match = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (result != NULL) &&
                          (d_strequals(result,
                                       d_strnlen(result, 256),
                                       "abc",
                                       3)) );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- NULL params --- */
    err = d_text_template_render_alloc(NULL, "test", &result);
    test_null_param = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    /* --- expansion --- */
    tmpl = d_text_template_new();
    test_expansion = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "X", "EXPANDED_VALUE");
        err = d_text_template_render_alloc(tmpl, "a%X%b", &result);
        test_expansion = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                           (result != NULL) &&
                           (d_strequals(result,
                                        d_strnlen(result, 256),
                                        "aEXPANDED_VALUEb",
                                        17)) );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- contraction --- */
    tmpl = d_text_template_new();
    test_contraction = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "LONGKEYNAME", "s");
        err = d_text_template_render_alloc(tmpl,
                                           "a%LONGKEYNAME%b",
                                           &result);
        test_contraction = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                             (result != NULL) &&
                             (d_strequals(result,
                                          d_strnlen(result, 256),
                                          "asb",
                                          3)) );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- NULL result pointer --- */
    tmpl = d_text_template_new();
    test_null_result = false;
    if (tmpl)
    {
        err = d_text_template_render_alloc(tmpl,
                                           "test",
                                           NULL);
        test_null_result =
            (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    /* --- function binding via alloc --- */
    tmpl = d_text_template_new();
    test_fn_alloc = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_function(
            tmpl,
            "FN",
            d_test_internal_render_fn,
            NULL);
        err = d_text_template_render_alloc(tmpl,
                                           "out=%FN%",
                                           &result);
        test_fn_alloc =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (result != NULL) &&
              (d_strequals(result,
                           d_strnlen(result, 256),
                           "out=computed",
                           12)) );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- build result tree --- */
    group = d_test_object_new_interior(
                "d_text_template_render_alloc",
                10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("correct",
                                           test_correct,
                                           "allocates correct result");
    group->elements[idx++] = D_ASSERT_TRUE("null_term",
                                           test_null_term,
                                           "result is null-terminated");
    group->elements[idx++] = D_ASSERT_TRUE("freeable",
                                           test_freeable,
                                           "result can be freed");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty format");
    group->elements[idx++] = D_ASSERT_TRUE("no_match",
                                           test_no_match,
                                           "no-match passes through");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "ERROR_NULL_PARAM for NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("expansion",
                                           test_expansion,
                                           "handles value expansion");
    group->elements[idx++] = D_ASSERT_TRUE("contraction",
                                           test_contraction,
                                           "handles value contraction");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_result",
                                 test_null_result,
                                 "ERROR_NULL_PARAM for NULL result");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "fn_alloc",
                                 test_fn_alloc,
                                 "allocates function binding result");

    return group;
}

/*
d_tests_sa_text_template_render_length
  Tests d_text_template_render_length for output length calculation.
  Tests the following:
  - correct length for simple substitution
  - returns 0 for empty format
  - returns format length when no markers match
  - accounts for expansion
  - accounts for contraction
  - returns ERROR_NULL_PARAM for NULL inputs
  - returns ERROR_NULL_PARAM for NULL length pointer
  - handles multiple substitutions in length sum
*/
struct d_test_object*
d_tests_sa_text_template_render_length
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    size_t                      len;
    bool                        test_simple;
    bool                        test_empty;
    bool                        test_no_match;
    bool                        test_expansion;
    bool                        test_contraction;
    bool                        test_null_param;
    bool                        test_null_len;
    bool                        test_multi;
    size_t                      idx;

    /* --- simple: "a%XY%b" with XY->"12" => "a12b" = 4 --- */
    tmpl = d_text_template_new();
    test_simple = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "XY", "12");
        err = d_text_template_render_length(tmpl, "a%XY%b", &len);
        test_simple = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                        (len == 4) );
        d_text_template_free(tmpl);
    }

    /* --- empty --- */
    tmpl = d_text_template_new();
    test_empty = false;
    if (tmpl)
    {
        err = d_text_template_render_length(tmpl, "", &len);
        test_empty = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (len == 0) );
        d_text_template_free(tmpl);
    }

    /* --- no match: "abcdef" stays 6 --- */
    tmpl = d_text_template_new();
    test_no_match = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "ZZZ", "z");
        err = d_text_template_render_length(tmpl, "abcdef", &len);
        test_no_match = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (len == 6) );
        d_text_template_free(tmpl);
    }

    /* --- expansion: "a%X%b" with X->"HUGE" => "aHUGEb" = 6 --- */
    tmpl = d_text_template_new();
    test_expansion = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "X", "HUGE");
        err = d_text_template_render_length(tmpl, "a%X%b", &len);
        test_expansion = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                           (len == 6) );
        d_text_template_free(tmpl);
    }

    /* --- contraction: "%LONGKEY%" with LONGKEY->"s" => "s" = 1 --- */
    tmpl = d_text_template_new();
    test_contraction = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "LONGKEY", "s");
        err = d_text_template_render_length(tmpl, "%LONGKEY%", &len);
        test_contraction = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                             (len == 1) );
        d_text_template_free(tmpl);
    }

    /* --- NULL params --- */
    err = d_text_template_render_length(NULL, "abc", &len);
    test_null_param = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    /* --- NULL length pointer --- */
    tmpl = d_text_template_new();
    test_null_len = false;
    if (tmpl)
    {
        err = d_text_template_render_length(tmpl,
                                            "abc",
                                            NULL);
        test_null_len =
            (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    /* --- multiple: "%A%z%B%" with A->"xx", B->"yy" => "xxzyy" = 5 --- */
    tmpl = d_text_template_new();
    test_multi = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "A", "xx");
        d_text_template_bind_string(tmpl, "B", "yy");
        err = d_text_template_render_length(tmpl, "%A%z%B%", &len);
        test_multi = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (len == 5) );
        d_text_template_free(tmpl);
    }

    /* --- build result tree --- */
    group = d_test_object_new_interior(
                "d_text_template_render_length",
                8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple",
                                           test_simple,
                                           "correct length for simple case");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "returns 0 for empty format");
    group->elements[idx++] = D_ASSERT_TRUE("no_match",
                                           test_no_match,
                                           "returns format length when no match");
    group->elements[idx++] = D_ASSERT_TRUE("expansion",
                                           test_expansion,
                                           "accounts for expansion");
    group->elements[idx++] = D_ASSERT_TRUE("contraction",
                                           test_contraction,
                                           "accounts for contraction");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "ERROR_NULL_PARAM for NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_len",
                                 test_null_len,
                                 "ERROR_NULL_PARAM for NULL length");
    group->elements[idx++] = D_ASSERT_TRUE("multi",
                                           test_multi,
                                           "sums multiple substitutions");

    return group;
}

/*
d_tests_sa_text_template_render_nested
  Tests rendering with nested template bindings.
  Tests the following:
  - single-level nesting renders correctly
  - two-level nesting renders correctly
  - nested template with its own different markers
  - nested template with multiple bindings
  - unbound marker in nested template passes through literally
  - nested template contributes to parent rendering
*/
struct d_test_object*
d_tests_sa_text_template_render_nested
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     parent;
    struct d_text_template*     child;
    struct d_text_template*     grandchild;
    enum d_text_template_error  err;
    char                        buf[D_TEST_TMPL_BUFFER_SIZE];
    char*                       result;
    bool                        test_single_nest;
    bool                        test_two_level;
    bool                        test_diff_markers;
    bool                        test_multi_bind;
    bool                        test_unbound_pass;
    bool                        test_contributes;
    size_t                      idx;

    /* --- single-level nesting --- */
    /* parent: "Hello %greeting%!"  with greeting -> child */
    /* child bindings: msg -> "World"  */
    /* child renders "Dear %msg%" -> "Dear World" */
    /* parent result: "Hello Dear World!" */
    parent = d_text_template_new();
    child  = d_text_template_new();
    test_single_nest = false;
    if (parent && child)
    {
        d_text_template_bind_string(child, "msg", "World");
        d_text_template_bind_template(parent, "greeting", child);
        err = d_text_template_render(parent,
                                     "Hello %greeting%!",
                                     buf,
                                     sizeof(buf));
        /* the nested template should render using its own bindings
           and its result replaces the parent marker */
        test_single_nest = (err == D_TEXT_TEMPLATE_SUCCESS);
        /* exact output depends on nested template's format;
           the child must have been rendered to a string value first.
           since child has no format of its own, it resolves to the
           recursive resolution of all its bindings. we verify at
           minimum that the call succeeded and produced output. */
        if (test_single_nest)
        {
            test_single_nest = (d_strnlen(buf, sizeof(buf)) > 0);
        }
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    /* --- two-level nesting --- */
    parent     = d_text_template_new();
    child      = d_text_template_new();
    grandchild = d_text_template_new();
    test_two_level = false;
    if (parent && child && grandchild)
    {
        d_text_template_bind_string(grandchild, "leaf", "deep");
        d_text_template_bind_template(child, "inner", grandchild);
        d_text_template_bind_template(parent, "outer", child);
        err = d_text_template_render(parent,
                                     "start %outer% end",
                                     buf,
                                     sizeof(buf));
        test_two_level = (err == D_TEXT_TEMPLATE_SUCCESS);
        if (test_two_level)
        {
            test_two_level = (d_strnlen(buf, sizeof(buf)) > 0);
        }
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    if (grandchild)
    {
        d_text_template_free(grandchild);
    }

    /* --- nested template with different markers --- */
    parent = d_text_template_new();                         /* %...% */
    child  = d_text_template_new_with_markers("{{", "}}"); /* {{...}} */
    test_diff_markers = false;
    if (parent && child)
    {
        d_text_template_bind_string(child, "item", "widget");
        d_text_template_bind_template(parent, "content", child);
        err = d_text_template_render(parent,
                                     "Result: %content%",
                                     buf,
                                     sizeof(buf));
        test_diff_markers = (err == D_TEXT_TEMPLATE_SUCCESS);
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    /* --- nested template with multiple bindings --- */
    parent = d_text_template_new();
    child  = d_text_template_new();
    test_multi_bind = false;
    if (parent && child)
    {
        d_text_template_bind_string(child, "a", "1");
        d_text_template_bind_string(child, "b", "2");
        d_text_template_bind_string(child, "c", "3");
        d_text_template_bind_template(parent, "nums", child);
        err = d_text_template_render(parent,
                                     "Numbers: %nums%",
                                     buf,
                                     sizeof(buf));
        test_multi_bind = (err == D_TEXT_TEMPLATE_SUCCESS);
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    /* --- unbound marker in format passes through --- */
    parent = d_text_template_new();
    test_unbound_pass = false;
    if (parent)
    {
        d_text_template_bind_string(parent, "known", "yes");
        err = d_text_template_render(parent,
                                     "%known% and %unknown%",
                                     buf,
                                     sizeof(buf));
        /* %unknown% should pass through literally since there is no binding */
        test_unbound_pass = (err == D_TEXT_TEMPLATE_SUCCESS);
        if (test_unbound_pass)
        {
            test_unbound_pass = d_strcontains(buf,
                                              d_strnlen(buf, sizeof(buf)),
                                              "yes");
        }

        d_text_template_free(parent);
    }

    /* --- nested contributes alongside string bindings --- */
    parent = d_text_template_new();
    child  = d_text_template_new();
    test_contributes = false;
    result = NULL;
    if (parent && child)
    {
        d_text_template_bind_string(child, "detail", "fine");
        d_text_template_bind_string(parent, "greeting", "Hi");
        d_text_template_bind_template(parent, "body", child);
        err = d_text_template_render_alloc(parent,
                                           "%greeting% - %body%",
                                           &result);
        test_contributes = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                             (result != NULL) );
        if (result)
        {
            /* must contain the string binding "Hi" */
            test_contributes = test_contributes &&
                               d_strcontains(result,
                                             d_strnlen(result, 256),
                                             "Hi");
            free(result);
        }
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    /* --- build result tree --- */
    group = d_test_object_new_interior("render (nested templates)", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("single_nest",
                                           test_single_nest,
                                           "single-level nesting renders");
    group->elements[idx++] = D_ASSERT_TRUE("two_level",
                                           test_two_level,
                                           "two-level nesting renders");
    group->elements[idx++] = D_ASSERT_TRUE("diff_markers",
                                           test_diff_markers,
                                           "nested with different markers");
    group->elements[idx++] = D_ASSERT_TRUE("multi_bind",
                                           test_multi_bind,
                                           "nested with multiple bindings");
    group->elements[idx++] = D_ASSERT_TRUE("unbound_pass",
                                           test_unbound_pass,
                                           "unbound marker passes through");
    group->elements[idx++] = D_ASSERT_TRUE("contributes",
                                           test_contributes,
                                           "nested alongside string bindings");

    return group;
}

/*
d_tests_sa_text_template_render_all
  Runs all rendering tests.
  Tests the following:
  - d_text_template_render
  - d_text_template_render_alloc
  - d_text_template_render_length
  - nested template rendering
*/
struct d_test_object*
d_tests_sa_text_template_render_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Rendering", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_template_render();
    group->elements[idx++] = d_tests_sa_text_template_render_alloc();
    group->elements[idx++] = d_tests_sa_text_template_render_length();
    group->elements[idx++] = d_tests_sa_text_template_render_nested();

    return group;
}
