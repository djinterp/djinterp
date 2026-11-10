/*******************************************************************************
* djinterp [text]                                   text_template_tests_marker.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_marker.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * MARKER CONFIGURATION TESTS
 *****************************************************************************/

/*
d_tests_text_template_set_markers
  Tests d_text_template_set_markers for changing markers post-creation.
  Tests the following:
  - changes markers successfully
  - prefix updated correctly
  - suffix updated correctly
  - prefix_length cached correctly
  - suffix_length cached correctly
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL prefix
  - returns ERROR_NULL_PARAM for NULL suffix
  - returns ERROR_INVALID_MARKER for empty prefix
  - returns ERROR_INVALID_MARKER for empty suffix
  - single-char markers accepted
  - repeated set_markers replaces old markers
  - existing bindings survive marker change
*/
bool d_tests_text_template_set_markers(struct dj_test_suite* _suite)
{
    struct dj_text_template*     tmpl;
    enum dj_text_template_error  err;
    const char*                 pfx;
    const char*                 sfx;
    bool                        test_success;
    bool                        test_prefix;
    bool                        test_suffix;
    bool                        test_pfx_len;
    bool                        test_sfx_len;
    bool                        test_null_tmpl;
    bool                        test_null_pfx;
    bool                        test_null_sfx;
    bool                        test_empty_pfx;
    bool                        test_empty_sfx;
    bool                        test_single_char;
    bool                        test_repeated;
    bool                        test_bindings_kept;

    tmpl = d_text_template_new();

    test_success   = false;
    test_prefix    = false;
    test_suffix    = false;
    test_pfx_len   = false;
    test_sfx_len   = false;

    if (tmpl)
    {
        // test 1: set markers succeeds
        err = d_text_template_set_markers(tmpl, "[[", "]]");
        test_success = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: prefix updated
        pfx = d_text_template_get_prefix(tmpl);
        test_prefix = ( (pfx != NULL) &&
                        (d_strequals(pfx,
                                     d_strnlen(pfx, 32),
                                     "[[",
                                     2)) );

        // test 3: suffix updated
        sfx = d_text_template_get_suffix(tmpl);
        test_suffix = ( (sfx != NULL) &&
                        (d_strequals(sfx,
                                     d_strnlen(sfx, 32),
                                     "]]",
                                     2)) );

        // test 4: prefix_length cached
        test_pfx_len = (tmpl->marker.prefix_length == 2);

        // test 5: suffix_length cached
        test_sfx_len = (tmpl->marker.suffix_length == 2);

        d_text_template_free(tmpl);
    }

    // test 6: NULL template
    err = d_text_template_set_markers(NULL, "a", "b");
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // test 7: NULL prefix
    tmpl = d_text_template_new();
    test_null_pfx = false;
    if (tmpl)
    {
        err = d_text_template_set_markers(tmpl, NULL, "}");
        test_null_pfx =
            (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // test 8: NULL suffix
    tmpl = d_text_template_new();
    test_null_sfx = false;
    if (tmpl)
    {
        err = d_text_template_set_markers(tmpl, "{", NULL);
        test_null_sfx =
            (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // test 9: empty prefix
    tmpl = d_text_template_new();
    test_empty_pfx = false;
    if (tmpl)
    {
        err = d_text_template_set_markers(tmpl, "", "}");
        test_empty_pfx =
            (err == D_TEXT_TEMPLATE_ERROR_INVALID_MARKER);
        d_text_template_free(tmpl);
    }

    // test 10: empty suffix
    tmpl = d_text_template_new();
    test_empty_sfx = false;
    if (tmpl)
    {
        err = d_text_template_set_markers(tmpl, "{", "");
        test_empty_sfx =
            (err == D_TEXT_TEMPLATE_ERROR_INVALID_MARKER);
        d_text_template_free(tmpl);
    }

    // test 11: single-char markers accepted
    tmpl = d_text_template_new();
    test_single_char = false;
    if (tmpl)
    {
        err = d_text_template_set_markers(tmpl, "#", "#");
        test_single_char =
            ( (err == D_TEXT_TEMPLATE_SUCCESS)  &&
              (tmpl->marker.prefix_length == 1) &&
              (tmpl->marker.suffix_length == 1) );
        d_text_template_free(tmpl);
    }

    // test 12: repeated set_markers replaces old markers
    tmpl = d_text_template_new();
    test_repeated = false;
    if (tmpl)
    {
        d_text_template_set_markers(tmpl, "<<", ">>");
        err = d_text_template_set_markers(tmpl,
                                          "${",
                                          "}");

        pfx = d_text_template_get_prefix(tmpl);
        sfx = d_text_template_get_suffix(tmpl);
        test_repeated =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (pfx != NULL)                   &&
              (sfx != NULL)                   &&
              (d_strequals(pfx,
                           d_strnlen(pfx, 32),
                           "${",
                           2))                &&
              (d_strequals(sfx,
                           d_strnlen(sfx, 32),
                           "}",
                           1)) );
        d_text_template_free(tmpl);
    }

    // test 13: existing bindings survive marker change
    tmpl = d_text_template_new();
    test_bindings_kept = false;
    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "key", "val");
        err = d_text_template_set_markers(tmpl,
                                          "{{",
                                          "}}");
        test_bindings_kept =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (tmpl->binding_count == 1)       &&
              (d_text_template_has_binding(tmpl, "key")) );
        d_text_template_free(tmpl);
    }

    // build result tree
    d_test_suite_test(_suite, "d_text_template_set_markers");

    D_TEST_CHECK_MSG(_suite,
                     "success",
                     test_success,
                     "set_markers returns SUCCESS");
    D_TEST_CHECK_MSG(_suite, "prefix", test_prefix, "prefix updated correctly");
    D_TEST_CHECK_MSG(_suite, "suffix", test_suffix, "suffix updated correctly");
    D_TEST_CHECK_MSG(_suite,
                     "pfx_len",
                     test_pfx_len,
                     "prefix_length cached correctly");
    D_TEST_CHECK_MSG(_suite,
                     "sfx_len",
                     test_sfx_len,
                     "suffix_length cached correctly");
    D_TEST_CHECK_MSG(_suite,
                     "null_tmpl",
                     test_null_tmpl,
                     "ERROR_NULL_PARAM for NULL tmpl");
    D_TEST_CHECK_MSG(_suite,
                     "null_pfx",
                     test_null_pfx,
                     "ERROR_NULL_PARAM for NULL prefix");
    D_TEST_CHECK_MSG(_suite,
                     "null_sfx",
                     test_null_sfx,
                     "ERROR_NULL_PARAM for NULL suffix");
    D_TEST_CHECK_MSG(_suite,
                     "empty_pfx",
                     test_empty_pfx,
                     "INVALID_MARKER for empty prefix");
    D_TEST_CHECK_MSG(_suite,
                     "empty_sfx",
                     test_empty_sfx,
                     "INVALID_MARKER for empty suffix");
    D_TEST_CHECK_MSG(_suite,
                     "single_char",
                     test_single_char,
                     "single-char markers accepted");
    D_TEST_CHECK_MSG(_suite,
                     "repeated",
                     test_repeated,
                     "repeated call replaces old markers");
    D_TEST_CHECK_MSG(_suite,
                     "bindings_kept",
                     test_bindings_kept,
                     "bindings survive marker change");

    return true;
}

/*
d_tests_text_template_get_prefix
  Tests d_text_template_get_prefix for prefix retrieval.
  Tests the following:
  - returns default prefix on new template
  - returns updated prefix after set_markers
  - returns NULL for NULL template
*/
bool d_tests_text_template_get_prefix(struct dj_test_suite* _suite)
{
    struct dj_text_template* tmpl;
    const char*             pfx;
    bool                    test_default;
    bool                    test_after_set;
    bool                    test_null;

    // test 1: default
    tmpl = d_text_template_new();
    test_default = false;
    if (tmpl)
    {
        pfx = d_text_template_get_prefix(tmpl);
        test_default = ( (pfx != NULL) &&
                         (d_strequals(pfx,
                                      d_strnlen(pfx, 32),
                                      D_TEXT_TEMPLATE_DEFAULT_PREFIX,
                                      d_strnlen(D_TEXT_TEMPLATE_DEFAULT_PREFIX,
                                                32))) );
    }

    // test 2: after set_markers
    test_after_set = false;
    if (tmpl)
    {
        d_text_template_set_markers(tmpl, "@@", "@@");
        pfx = d_text_template_get_prefix(tmpl);
        test_after_set = ( (pfx != NULL) &&
                           (d_strequals(pfx,
                                        d_strnlen(pfx, 32),
                                        "@@",
                                        2)) );
        d_text_template_free(tmpl);
    }

    // test 3: NULL template
    pfx = d_text_template_get_prefix(NULL);
    test_null = (pfx == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_text_template_get_prefix");

    D_TEST_CHECK_MSG(_suite, "default", test_default, "returns default prefix");
    D_TEST_CHECK_MSG(_suite,
                     "after_set",
                     test_after_set,
                     "returns updated prefix");
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     test_null,
                     "returns NULL for NULL template");

    return true;
}

/*
d_tests_text_template_get_suffix
  Tests d_text_template_get_suffix for suffix retrieval.
  Tests the following:
  - returns default suffix on new template
  - returns updated suffix after set_markers
  - returns NULL for NULL template
*/
bool d_tests_text_template_get_suffix(struct dj_test_suite* _suite)
{
    struct dj_text_template* tmpl;
    const char*             sfx;
    bool                    test_default;
    bool                    test_after_set;
    bool                    test_null;

    // test 1: default
    tmpl = d_text_template_new();
    test_default = false;
    if (tmpl)
    {
        sfx = d_text_template_get_suffix(tmpl);
        test_default = ( (sfx != NULL) &&
                         (d_strequals(sfx,
                                      d_strnlen(sfx, 32),
                                      D_TEXT_TEMPLATE_DEFAULT_SUFFIX,
                                      d_strnlen(D_TEXT_TEMPLATE_DEFAULT_SUFFIX,
                                                32))) );
    }

    // test 2: after set_markers
    test_after_set = false;
    if (tmpl)
    {
        d_text_template_set_markers(tmpl, "##", "##");
        sfx = d_text_template_get_suffix(tmpl);
        test_after_set = ( (sfx != NULL) &&
                           (d_strequals(sfx,
                                        d_strnlen(sfx, 32),
                                        "##",
                                        2)) );
        d_text_template_free(tmpl);
    }

    // test 3: NULL template
    sfx = d_text_template_get_suffix(NULL);
    test_null = (sfx == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_text_template_get_suffix");

    D_TEST_CHECK_MSG(_suite, "default", test_default, "returns default suffix");
    D_TEST_CHECK_MSG(_suite,
                     "after_set",
                     test_after_set,
                     "returns updated suffix");
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     test_null,
                     "returns NULL for NULL template");

    return true;
}

/*
d_tests_text_template_marker_all
  Runs all marker configuration tests.
  Tests the following:
  - d_text_template_set_markers
  - d_text_template_get_prefix
  - d_text_template_get_suffix
*/
bool d_tests_text_template_marker_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Marker Configuration");

    d_tests_text_template_set_markers(_suite);
    d_tests_text_template_get_prefix(_suite);
    d_tests_text_template_get_suffix(_suite);

    return true;
}
