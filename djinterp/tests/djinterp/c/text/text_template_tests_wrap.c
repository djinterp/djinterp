/*******************************************************************************
* djinterp [text]                                     text_template_tests_wrap.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_wrap.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * KEY WRAPPING TESTS
 *****************************************************************************/

/*
d_tests_text_template_wrap_keys
  Tests d_text_template_wrap_keys for buffer-based key wrapping.
  Tests the following:
  - wraps a single key with default markers
  - wraps multiple keys in one pass
  - leaves non-matching text unchanged
  - handles input with no key matches
  - handles empty input
  - longest match wins
  - wraps with custom markers
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL input
  - returns ERROR_NULL_PARAM for NULL buffer
  - returns ERROR_BUFFER_TOO_SMALL when buffer insufficient
*/
bool d_tests_text_template_wrap_keys(struct dj_test_suite* _suite)
{
    struct dj_text_template*     tmpl;
    enum dj_text_template_error  err;
    char                        buf[D_TEST_TMPL_BUFFER_SIZE];
    bool                        test_single;
    bool                        test_multiple;
    bool                        test_passthrough;
    bool                        test_no_match;
    bool                        test_empty;
    bool                        test_longest;
    bool                        test_custom;
    bool                        test_null_tmpl;
    bool                        test_null_input;
    bool                        test_null_buf;
    bool                        test_buf_small;

    /* --- single key with default markers (% / %) --- */
    /* input: "Hello name" => "Hello %name%" */
    tmpl = d_text_template_new();
    test_single = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "name", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "Hello name",
                                        buf,
                                        sizeof(buf));
        test_single = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                        (d_strequals(buf,
                                     d_strnlen(buf, sizeof(buf)),
                                     "Hello %name%",
                                     12)) );
        d_text_template_free(tmpl);
    }

    /* --- multiple keys --- */
    /* input: "first and last" => "%first% and %last%" */
    tmpl = d_text_template_new();
    test_multiple = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "first", "unused");
        d_text_template_bind_string(tmpl, "last", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "first and last",
                                        buf,
                                        sizeof(buf));
        test_multiple = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "%first% and %last%",
                                       18)) );
        d_text_template_free(tmpl);
    }

    /* --- non-matching text passes through --- */
    tmpl = d_text_template_new();
    test_passthrough = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "KEY", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "nothing to wrap",
                                        buf,
                                        sizeof(buf));
        test_passthrough = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                             (d_strequals(buf,
                                          d_strnlen(buf, sizeof(buf)),
                                          "nothing to wrap",
                                          15)) );
        d_text_template_free(tmpl);
    }

    /* --- no bindings at all --- */
    tmpl = d_text_template_new();
    test_no_match = false;
    if (tmpl)
    {
        err = d_text_template_wrap_keys(tmpl,
                                        "some text",
                                        buf,
                                        sizeof(buf));
        test_no_match = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "some text",
                                       9)) );
        d_text_template_free(tmpl);
    }

    /* --- empty input --- */
    tmpl = d_text_template_new();
    test_empty = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "X", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "",
                                        buf,
                                        sizeof(buf));
        test_empty = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (buf[0] == '\0') );
        d_text_template_free(tmpl);
    }

    /* --- longest match wins --- */
    /* keys: "var" and "variable" */
    /* input: "variable" => "%variable%" (not "%var%iable") */
    tmpl = d_text_template_new();
    test_longest = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "var", "unused");
        d_text_template_bind_string(tmpl, "variable", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "variable",
                                        buf,
                                        sizeof(buf));
        test_longest = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                         (d_strequals(buf,
                                      d_strnlen(buf, sizeof(buf)),
                                      "%variable%",
                                      10)) );
        d_text_template_free(tmpl);
    }

    /* --- custom markers ({{ / }}) --- */
    tmpl = d_text_template_new_with_markers("{{", "}}");
    test_custom = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "item", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "buy item",
                                        buf,
                                        sizeof(buf));
        test_custom = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                        (d_strequals(buf,
                                     d_strnlen(buf, sizeof(buf)),
                                     "buy {{item}}",
                                     12)) );
        d_text_template_free(tmpl);
    }

    /* --- NULL template --- */
    err = d_text_template_wrap_keys(NULL, "test", buf, sizeof(buf));
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    /* --- NULL input --- */
    tmpl = d_text_template_new();
    test_null_input = false;
    if (tmpl)
    {
        err = d_text_template_wrap_keys(tmpl, NULL, buf, sizeof(buf));
        test_null_input = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    /* --- NULL buffer --- */
    tmpl = d_text_template_new();
    test_null_buf = false;
    if (tmpl)
    {
        err = d_text_template_wrap_keys(tmpl, "test", NULL, 100);
        test_null_buf = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    /* --- buffer too small --- */
    tmpl = d_text_template_new();
    test_buf_small = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "key", "unused");
        err = d_text_template_wrap_keys(tmpl,
                                        "key",
                                        buf,
                                        3);   /* too small for "%key%" */
        test_buf_small = (err == D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL);
        d_text_template_free(tmpl);
    }

    /* --- build result tree --- */
    d_test_suite_test(_suite, "d_text_template_wrap_keys");

    D_TEST_CHECK_MSG(_suite, "single", test_single, "wraps single key");
    D_TEST_CHECK_MSG(_suite, "multiple", test_multiple, "wraps multiple keys");
    D_TEST_CHECK_MSG(_suite,
                     "passthrough",
                     test_passthrough,
                     "non-matching text unchanged");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "no bindings passes through");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty input");
    D_TEST_CHECK_MSG(_suite, "longest", test_longest, "longest match wins");
    D_TEST_CHECK_MSG(_suite,
                     "custom",
                     test_custom,
                     "wraps with custom markers");
    D_TEST_CHECK_MSG(_suite,
                     "null_tmpl",
                     test_null_tmpl,
                     "ERROR_NULL_PARAM for NULL template");
    D_TEST_CHECK_MSG(_suite,
                     "null_input",
                     test_null_input,
                     "ERROR_NULL_PARAM for NULL input");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "ERROR_NULL_PARAM for NULL buffer");
    D_TEST_CHECK_MSG(_suite,
                     "buf_small",
                     test_buf_small,
                     "ERROR_BUFFER_TOO_SMALL on overflow");

    return true;
}

/*
d_tests_text_template_wrap_keys_alloc
  Tests d_text_template_wrap_keys_alloc for allocating key wrapping.
  Tests the following:
  - allocates and returns correct wrapped result
  - result is null-terminated
  - caller can free the result
  - handles no matches (passthrough)
  - returns ERROR_NULL_PARAM for NULL inputs
  - handles empty input
*/
bool d_tests_text_template_wrap_keys_alloc(struct dj_test_suite* _suite)
{
    struct dj_text_template*     tmpl;
    enum dj_text_template_error  err;
    char*                       result;
    bool                        test_correct;
    bool                        test_null_term;
    bool                        test_freeable;
    bool                        test_no_match;
    bool                        test_null_param;
    bool                        test_empty;

    /* --- correct result --- */
    tmpl = d_text_template_new();
    test_correct   = false;
    test_null_term = false;
    test_freeable  = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "city", "unused");
        err = d_text_template_wrap_keys_alloc(tmpl,
                                              "visit city soon",
                                              &result);
        test_correct = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                         (result != NULL) &&
                         (d_strequals(result,
                                      d_strnlen(result, 256),
                                      "visit %city% soon",
                                      17)) );

        // test 2: null-terminated
        if (result)
        {
            test_null_term = (result[17] == '\0');
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

    /* --- no match passthrough --- */
    tmpl = d_text_template_new();
    test_no_match = false;
    result = NULL;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "ZZZ", "unused");
        err = d_text_template_wrap_keys_alloc(tmpl,
                                              "no match",
                                              &result);
        test_no_match = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (result != NULL) &&
                          (d_strequals(result,
                                       d_strnlen(result, 256),
                                       "no match",
                                       8)) );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- NULL params --- */
    err = d_text_template_wrap_keys_alloc(NULL, "test", &result);
    test_null_param = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    /* --- empty input --- */
    tmpl = d_text_template_new();
    test_empty = false;
    result = NULL;
    if (tmpl)
    {
        err = d_text_template_wrap_keys_alloc(tmpl, "", &result);
        test_empty = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (result != NULL) &&
                       (result[0] == '\0') );
        if (result)
        {
            free(result);
        }

        d_text_template_free(tmpl);
    }

    /* --- build result tree --- */
    d_test_suite_test(_suite, "d_text_template_wrap_keys_alloc");

    D_TEST_CHECK_MSG(_suite,
                     "correct",
                     test_correct,
                     "allocates correct wrapped result");
    D_TEST_CHECK_MSG(_suite,
                     "null_term",
                     test_null_term,
                     "result is null-terminated");
    D_TEST_CHECK_MSG(_suite, "freeable", test_freeable, "result can be freed");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "no match passes through");
    D_TEST_CHECK_MSG(_suite,
                     "null_param",
                     test_null_param,
                     "ERROR_NULL_PARAM for NULL inputs");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty input");

    return true;
}

/*
d_tests_text_template_wrap_all
  Runs all key wrapping tests.
  Tests the following:
  - d_text_template_wrap_keys
  - d_text_template_wrap_keys_alloc
*/
bool d_tests_text_template_wrap_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Key Wrapping");

    d_tests_text_template_wrap_keys(_suite);
    d_tests_text_template_wrap_keys_alloc(_suite);

    return true;
}
