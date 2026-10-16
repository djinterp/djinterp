/*******************************************************************************
* djinterp [text]                                text_template_tests_sa_marker.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa_marker.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests_sa.h"


/******************************************************************************
 * MARKER CONFIGURATION TESTS
 *****************************************************************************/

/*
d_tests_sa_text_template_set_markers
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
struct d_test_object*
d_tests_sa_text_template_set_markers
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
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
    size_t                      idx;

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
    group = d_test_object_new_interior(
                "d_text_template_set_markers",
                13);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "success",
                                 test_success,
                                 "set_markers returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "prefix",
                                 test_prefix,
                                 "prefix updated correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "suffix",
                                 test_suffix,
                                 "suffix updated correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "pfx_len",
                                 test_pfx_len,
                                 "prefix_length cached correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "sfx_len",
                                 test_sfx_len,
                                 "suffix_length cached correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_tmpl",
                                 test_null_tmpl,
                                 "ERROR_NULL_PARAM for NULL tmpl");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_pfx",
                                 test_null_pfx,
                                 "ERROR_NULL_PARAM for NULL prefix");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_sfx",
                                 test_null_sfx,
                                 "ERROR_NULL_PARAM for NULL suffix");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "empty_pfx",
                                 test_empty_pfx,
                                 "INVALID_MARKER for empty prefix");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "empty_sfx",
                                 test_empty_sfx,
                                 "INVALID_MARKER for empty suffix");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "single_char",
                                 test_single_char,
                                 "single-char markers accepted");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "repeated",
                                 test_repeated,
                                 "repeated call replaces old markers");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "bindings_kept",
                                 test_bindings_kept,
                                 "bindings survive marker change");

    return group;
}

/*
d_tests_sa_text_template_get_prefix
  Tests d_text_template_get_prefix for prefix retrieval.
  Tests the following:
  - returns default prefix on new template
  - returns updated prefix after set_markers
  - returns NULL for NULL template
*/
struct d_test_object*
d_tests_sa_text_template_get_prefix
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    const char*             pfx;
    bool                    test_default;
    bool                    test_after_set;
    bool                    test_null;
    size_t                  idx;

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
    group = d_test_object_new_interior("d_text_template_get_prefix", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("default",
                                           test_default,
                                           "returns default prefix");
    group->elements[idx++] = D_ASSERT_TRUE("after_set",
                                           test_after_set,
                                           "returns updated prefix");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "returns NULL for NULL template");

    return group;
}

/*
d_tests_sa_text_template_get_suffix
  Tests d_text_template_get_suffix for suffix retrieval.
  Tests the following:
  - returns default suffix on new template
  - returns updated suffix after set_markers
  - returns NULL for NULL template
*/
struct d_test_object*
d_tests_sa_text_template_get_suffix
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    const char*             sfx;
    bool                    test_default;
    bool                    test_after_set;
    bool                    test_null;
    size_t                  idx;

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
    group = d_test_object_new_interior("d_text_template_get_suffix", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("default",
                                           test_default,
                                           "returns default suffix");
    group->elements[idx++] = D_ASSERT_TRUE("after_set",
                                           test_after_set,
                                           "returns updated suffix");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "returns NULL for NULL template");

    return group;
}

/*
d_tests_sa_text_template_marker_all
  Runs all marker configuration tests.
  Tests the following:
  - d_text_template_set_markers
  - d_text_template_get_prefix
  - d_text_template_get_suffix
*/
struct d_test_object*
d_tests_sa_text_template_marker_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Marker Configuration", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_template_set_markers();
    group->elements[idx++] = d_tests_sa_text_template_get_prefix();
    group->elements[idx++] = d_tests_sa_text_template_get_suffix();

    return group;
}
