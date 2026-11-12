/*******************************************************************************
* djinterp [c]                                              dstring_tests_join.c
*
*
* path:      /tests/djinterp/c/dstring_tests_join.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dstring_tests.h"


/******************************************************************************
 * SECTION 14: JOIN FUNCTIONS
 *****************************************************************************/

/*
d_tests_dstring_join
  Tests d_string_join function (joins dj_string array with C string separator).
  Signature: dj_string* d_string_join(const dj_string* const* _strings, size_t _count, const char* _delimiter)
  Tests the following:
  - joining multiple strings with separator
  - joining two strings
  - joining single string (no separator added)
  - joining with empty separator
  - joining with empty strings in array
  - joining zero strings
  - NULL array handling
  - NULL separator handling
*/
bool d_tests_dstring_join(struct dj_test_suite* _suite)
{
    struct dj_string*      result;
    struct dj_string*      parts[5];

    d_test_suite_test(_suite, "d_string_join");

    // test: joining multiple strings with separator
    parts[0] = d_string_new_from_cstr("hello");
    parts[1] = d_string_new_from_cstr("world");
    parts[2] = d_string_new_from_cstr("test");

    if (parts[0] && parts[1] && parts[2])
    {
        result = d_string_join((const struct dj_string* const*)parts, 3, ", ");
        D_TEST_CHECK_MSG(_suite,
                         "join_multiple_strings",
                         result && d_string_equals_cstr(result, "hello, world, test"),
                         "should join with separator");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "join_multiple_strings",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(parts[0]);
    d_string_free(parts[1]);
    d_string_free(parts[2]);

    // test: joining two strings
    parts[0] = d_string_new_from_cstr("hello");
    parts[1] = d_string_new_from_cstr("world");

    if (parts[0] && parts[1])
    {
        result = d_string_join((const struct dj_string* const*)parts, 2, "-");
        D_TEST_CHECK_MSG(_suite,
                         "join_two_strings",
                         result && d_string_equals_cstr(result, "hello-world"),
                         "should join two strings with separator");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "join_two_strings",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(parts[0]);
    d_string_free(parts[1]);

    // test: joining single string (no separator added)
    parts[0] = d_string_new_from_cstr("hello");

    if (parts[0])
    {
        result = d_string_join((const struct dj_string* const*)parts, 1, ", ");
        D_TEST_CHECK_MSG(_suite,
                         "join_single_string",
                         result && d_string_equals_cstr(result, "hello"),
                         "should return copy of single string");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "join_single_string",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(parts[0]);

    // test: joining with empty separator
    parts[0] = d_string_new_from_cstr("a");
    parts[1] = d_string_new_from_cstr("b");
    parts[2] = d_string_new_from_cstr("c");

    if (parts[0] && parts[1] && parts[2])
    {
        result = d_string_join((const struct dj_string* const*)parts, 3, "");
        D_TEST_CHECK_MSG(_suite,
                         "join_empty_separator",
                         result && d_string_equals_cstr(result, "abc"),
                         "should concatenate without separator");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "join_empty_separator",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(parts[0]);
    d_string_free(parts[1]);
    d_string_free(parts[2]);

    // test: joining with empty strings in array
    parts[0] = d_string_new_from_cstr("a");
    parts[1] = d_string_new_from_cstr("");
    parts[2] = d_string_new_from_cstr("b");

    if (parts[0] && parts[1] && parts[2])
    {
        result = d_string_join((const struct dj_string* const*)parts, 3, ",");
        D_TEST_CHECK_MSG(_suite,
                         "join_with_empty_strings",
                         result && d_string_equals_cstr(result, "a,,b"),
                         "should include empty string in result");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "join_with_empty_strings",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(parts[0]);
    d_string_free(parts[1]);
    d_string_free(parts[2]);

    // test: joining zero strings
    result = d_string_join(NULL, 0, ",");
    D_TEST_CHECK_MSG(_suite,
                     "join_zero_strings",
                     result && d_string_is_empty(result),
                     "should return empty string for zero count");

    d_string_free(result);

    // test: NULL array handling
    result = d_string_join(NULL, 5, ",");
    D_TEST_CHECK_MSG(_suite,
                     "join_null_array",
                     (result) == NULL,
                     "should return NULL for NULL array with non-zero count");

    // test: NULL separator handling
    parts[0] = d_string_new_from_cstr("hello");
    parts[1] = d_string_new_from_cstr("world");
    result   = d_string_join((const struct dj_string* const*)parts, 2, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "join_null_separator",
                     (result) == NULL,
                     "should return NULL for NULL separator");

    d_string_free(parts[0]);
    d_string_free(parts[1]);

    return true;
}

/*
d_tests_dstring_join_cstr
  Tests d_string_join_cstr function (joins C string array with C string sep).
  Signature: dj_string* d_string_join_cstr(const char* const* _strings, size_t _count, const char* _delimiter)
  Tests the following:
  - joining multiple C strings with separator
  - joining two C strings
  - joining single C string
  - joining with empty separator
  - joining zero strings
  - NULL array handling
  - NULL separator handling
*/
bool d_tests_dstring_join_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      result;
    const char*           parts[5];

    d_test_suite_test(_suite, "d_string_join_cstr");

    // test: joining multiple C strings with separator
    parts[0] = "hello";
    parts[1] = "world";
    parts[2] = "test";
    result   = d_string_join_cstr(parts, 3, ", ");
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_multiple",
                     result && d_string_equals_cstr(result, "hello, world, test"),
                     "should join with separator");

    d_string_free(result);

    // test: joining two C strings
    parts[0] = "hello";
    parts[1] = "world";
    result   = d_string_join_cstr(parts, 2, "-");
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_two_strings",
                     result && d_string_equals_cstr(result, "hello-world"),
                     "should join two strings with separator");

    d_string_free(result);

    // test: joining single C string
    parts[0] = "hello";
    result   = d_string_join_cstr(parts, 1, ", ");
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_single_string",
                     result && d_string_equals_cstr(result, "hello"),
                     "should return copy of single string");

    d_string_free(result);

    // test: joining with empty separator
    parts[0] = "a";
    parts[1] = "b";
    parts[2] = "c";
    result   = d_string_join_cstr(parts, 3, "");
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_empty_separator",
                     result && d_string_equals_cstr(result, "abc"),
                     "should concatenate without separator");

    d_string_free(result);

    // test: joining zero strings
    result = d_string_join_cstr(NULL, 0, ",");
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_zero_strings",
                     result && d_string_is_empty(result),
                     "should return empty string for zero count");

    d_string_free(result);

    // test: NULL array handling
    result = d_string_join_cstr(NULL, 5, ",");
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_null_array",
                     (result) == NULL,
                     "should return NULL for NULL array with non-zero count");

    // test: NULL separator handling
    parts[0] = "hello";
    parts[1] = "world";
    result   = d_string_join_cstr(parts, 2, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "join_cstr_null_separator",
                     (result) == NULL,
                     "should return NULL for NULL separator");

    return true;
}

/*
d_tests_dstring_concat
  Tests d_string_concat function (variadic concatenation).
  Signature: dj_string* d_string_concat(size_t _count, ...)
  Takes count followed by that many dj_string* arguments.
  Tests the following:
  - concatenating multiple strings
  - concatenating two strings
  - concatenating single string
  - concatenating with empty string
  - concatenating zero strings
  - NULL pointer in arguments
*/
bool d_tests_dstring_concat(struct dj_test_suite* _suite)
{
    struct dj_string*      result;
    struct dj_string*      s1;
    struct dj_string*      s2;
    struct dj_string*      s3;

    d_test_suite_test(_suite, "d_string_concat");

    // test: concatenating multiple strings
    s1 = d_string_new_from_cstr("hello");
    s2 = d_string_new_from_cstr(" ");
    s3 = d_string_new_from_cstr("world");

    if (s1 && s2 && s3)
    {
        result = d_string_concat(3, s1, s2, s3);
        D_TEST_CHECK_MSG(_suite,
                         "concat_multiple_strings",
                         result && d_string_equals_cstr(result, "hello world"),
                         "should concatenate all strings");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "concat_multiple_strings",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(s1);
    d_string_free(s2);
    d_string_free(s3);

    // test: concatenating two strings
    s1 = d_string_new_from_cstr("hello");
    s2 = d_string_new_from_cstr("world");

    if (s1 && s2)
    {
        result = d_string_concat(2, s1, s2);
        D_TEST_CHECK_MSG(_suite,
                         "concat_two_strings",
                         result && d_string_equals_cstr(result, "helloworld"),
                         "should concatenate two strings");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "concat_two_strings",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(s1);
    d_string_free(s2);

    // test: concatenating single string
    s1 = d_string_new_from_cstr("hello");

    if (s1)
    {
        result = d_string_concat(1, s1);
        D_TEST_CHECK_MSG(_suite,
                         "concat_single_string",
                         result && d_string_equals_cstr(result, "hello"),
                         "should return copy of single string");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "concat_single_string",
                         false,
                         "failed to allocate test string");
    }

    d_string_free(s1);

    // test: concatenating with empty string
    s1 = d_string_new_from_cstr("a");
    s2 = d_string_new_from_cstr("");
    s3 = d_string_new_from_cstr("b");

    if (s1 && s2 && s3)
    {
        result = d_string_concat(3, s1, s2, s3);
        D_TEST_CHECK_MSG(_suite,
                         "concat_with_empty_string",
                         result && d_string_equals_cstr(result, "ab"),
                         "should handle empty string in middle");

        d_string_free(result);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "concat_with_empty_string",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(s1);
    d_string_free(s2);
    d_string_free(s3);

    // test: concatenating zero strings
    result = d_string_concat(0);
    D_TEST_CHECK_MSG(_suite,
                     "concat_zero_strings",
                     result && d_string_is_empty(result),
                     "should return empty string for zero count");

    d_string_free(result);

    // test: NULL pointer in arguments
    s1     = d_string_new_from_cstr("hello");
    result = d_string_concat(2, s1, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "concat_null_argument",
                     (result) == NULL,
                     "should return NULL when argument is NULL");

    d_string_free(s1);

    return true;
}


/******************************************************************************
 * JOIN ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_dstring_join_all
  Runs all join function tests for dstring module.
  Tests the following:
  - join function (dj_string array with C string separator)
  - join_cstr function (C string array with C string separator)
  - concat function (variadic dj_string concatenation)
*/
bool d_tests_dstring_join_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Join Functions");

    d_tests_dstring_join(_suite);
    d_tests_dstring_join_cstr(_suite);
    d_tests_dstring_concat(_suite);

    return true;
}
