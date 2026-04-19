#include "./str_interp_tests_sa.h"


/******************************************************************************
 * CONVENIENCE FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_str_interp_quick
  Tests d_str_interp_quick for single-call interpolation.
  Tests the following:
  - replaces specifiers from parallel name/value arrays
  - handles multiple name-value pairs
  - handles empty name/value arrays (null-terminated at index 0)
  - returns ERROR_NULL_PARAM for NULL buffer
  - returns ERROR_NULL_PARAM for NULL input
  - returns ERROR_NULL_PARAM for NULL names
  - returns ERROR_NULL_PARAM for NULL values
  - returns ERROR_BUFFER_TOO_SMALL when buffer is insufficient
  - handles no match (arrays have entries but nothing matches)
  - result matches expected string
*/
struct d_test_object*
d_tests_sa_str_interp_quick
(
    void
)
{
    struct d_test_object*   group;
    enum d_str_interp_error err;
    char                    buf[D_TEST_INTERP_BUFFER_SIZE];
    bool                    test_single_pair;
    bool                    test_multi_pair;
    bool                    test_empty_arrays;
    bool                    test_null_buf;
    bool                    test_null_input;
    bool                    test_null_names;
    bool                    test_null_values;
    bool                    test_buf_small;
    bool                    test_no_match;
    bool                    test_result;
    size_t                  idx;

    /* --- single name-value pair --- */
    {
        const char* names[]  = { "NAME", NULL };
        const char* values[] = { "Bob",  NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "Hi NAME",
                                 names,
                                 values);
        test_single_pair = ( (err == D_STR_INTERP_SUCCESS) &&
                             (d_strequals(buf,
                                          d_strnlen(buf, sizeof(buf)),
                                          "Hi Bob",
                                          6)) );
    }

    /* --- multiple pairs --- */
    {
        const char* names[]  = { "CITY", "YEAR", NULL };
        const char* values[] = { "Paris", "2026", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "CITY in YEAR",
                                 names,
                                 values);
        test_multi_pair = ( (err == D_STR_INTERP_SUCCESS) &&
                            (d_strequals(buf,
                                         d_strnlen(buf, sizeof(buf)),
                                         "Paris in 2026",
                                         13)) );
    }

    /* --- empty arrays (immediate NULL terminator) --- */
    {
        const char* names[]  = { NULL };
        const char* values[] = { NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "nothing changes",
                                 names,
                                 values);
        test_empty_arrays = ( (err == D_STR_INTERP_SUCCESS) &&
                              (d_strequals(buf,
                                           d_strnlen(buf, sizeof(buf)),
                                           "nothing changes",
                                           15)) );
    }

    /* --- NULL buffer --- */
    {
        const char* names[]  = { "K", NULL };
        const char* values[] = { "V", NULL };

        err = d_str_interp_quick(NULL,
                                 100,
                                 "test",
                                 names,
                                 values);
        test_null_buf = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- NULL input --- */
    {
        const char* names[]  = { "K", NULL };
        const char* values[] = { "V", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 NULL,
                                 names,
                                 values);
        test_null_input = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- NULL names array --- */
    {
        const char* values[] = { "V", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "test",
                                 NULL,
                                 values);
        test_null_names = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- NULL values array --- */
    {
        const char* names[] = { "K", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "test",
                                 names,
                                 NULL);
        test_null_values = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- buffer too small --- */
    {
        const char* names[]  = { "X", NULL };
        const char* values[] = { "a very long replacement string", NULL };

        err = d_str_interp_quick(buf,
                                 5,
                                 "X",
                                 names,
                                 values);
        test_buf_small = (err == D_STR_INTERP_ERROR_BUFFER_TOO_SMALL);
    }

    /* --- no match (keys present but don't appear in input) --- */
    {
        const char* names[]  = { "ZZZ", NULL };
        const char* values[] = { "nope", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "abc",
                                 names,
                                 values);
        test_no_match = ( (err == D_STR_INTERP_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "abc",
                                       3)) );
    }

    /* --- verify a more complex interpolation result --- */
    {
        const char* names[]  = { "GREETING", "TARGET", NULL };
        const char* values[] = { "Hola",     "Mundo",  NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "GREETING TARGET!",
                                 names,
                                 values);
        test_result = ( (err == D_STR_INTERP_SUCCESS) &&
                        (d_strequals(buf,
                                     d_strnlen(buf, sizeof(buf)),
                                     "Hola Mundo!",
                                     11)) );
    }

    /* --- build result tree --- */
    group = d_test_object_new_interior("d_str_interp_quick", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("single_pair",
                                           test_single_pair,
                                           "single name-value pair works");
    group->elements[idx++] = D_ASSERT_TRUE("multi_pair",
                                           test_multi_pair,
                                           "multiple pairs work");
    group->elements[idx++] = D_ASSERT_TRUE("empty_arrays",
                                           test_empty_arrays,
                                           "empty arrays pass through");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "ERROR_NULL_PARAM for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_input",
                                           test_null_input,
                                           "ERROR_NULL_PARAM for NULL input");
    group->elements[idx++] = D_ASSERT_TRUE("null_names",
                                           test_null_names,
                                           "ERROR_NULL_PARAM for NULL names");
    group->elements[idx++] = D_ASSERT_TRUE("null_values",
                                           test_null_values,
                                           "ERROR_NULL_PARAM for NULL values");
    group->elements[idx++] = D_ASSERT_TRUE("buf_small",
                                           test_buf_small,
                                           "ERROR_BUFFER_TOO_SMALL on overflow");
    group->elements[idx++] = D_ASSERT_TRUE("no_match",
                                           test_no_match,
                                           "no match passes through");
    group->elements[idx++] = D_ASSERT_TRUE("result",
                                           test_result,
                                           "complex interpolation correct");

    return group;
}

/*
d_tests_sa_str_interp_convenience_all
  Runs all convenience function tests.
  Tests the following:
  - d_str_interp_quick
*/
struct d_test_object*
d_tests_sa_str_interp_convenience_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Convenience Functions", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_str_interp_quick();

    return group;
}
