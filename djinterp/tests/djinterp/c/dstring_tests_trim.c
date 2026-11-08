/*******************************************************************************
* djinterp [test]                                           dstring_tests_trim.c
*
*
* path:      /tests/djinterp/c/dstring_tests_trim.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dstring_tests.h"


/******************************************************************************
 * SECTION 12: TRIMMING FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * I. IN-PLACE TRIMMING TESTS
 *****************************************************************************/

/*
d_tests_dstring_trim
  Tests d_string_trim function (in-place, both sides).
  Tests the following:
  - trimming whitespace from both sides
  - trimming only leading whitespace
  - trimming only trailing whitespace
  - string with no whitespace to trim
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_trim(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_trim");

    // test: trimming whitespace from both sides
    str = d_string_new_from_cstr("  hello world  ");

    if (str)
    {
        result = d_string_trim(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_both_sides",
                         result && d_string_equals_cstr(str, "hello world"),
                         "should trim whitespace from both sides");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_both_sides",
                         false,
                         "failed to allocate test string");
    }

    // test: trimming only leading whitespace
    str = d_string_new_from_cstr("   hello");

    if (str)
    {
        result = d_string_trim(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_leading_only",
                         result && d_string_equals_cstr(str, "hello"),
                         "should trim leading whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_leading_only",
                         false,
                         "failed to allocate test string");
    }

    // test: trimming only trailing whitespace
    str = d_string_new_from_cstr("hello   ");

    if (str)
    {
        result = d_string_trim(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_trailing_only",
                         result && d_string_equals_cstr(str, "hello"),
                         "should trim trailing whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_trailing_only",
                         false,
                         "failed to allocate test string");
    }

    // test: string with no whitespace to trim
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_trim(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_no_whitespace",
                         result && d_string_equals_cstr(str, "hello"),
                         "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_no_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_trim(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_all_whitespace",
                         result && d_string_is_empty(str),
                         "should result in empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_all_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_empty_string",
                         result && d_string_is_empty(str),
                         "should remain empty");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trim_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_trim_left
  Tests d_string_trim_left function (in-place, left side only).
  Tests the following:
  - trimming leading whitespace
  - string with no leading whitespace
  - string with only trailing whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_trim_left(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_trim_left");

    // test: trimming leading whitespace
    str = d_string_new_from_cstr("   hello world");

    if (str)
    {
        result = d_string_trim_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_leading",
                         result && d_string_equals_cstr(str, "hello world"),
                         "should trim leading whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_leading",
                         false,
                         "failed to allocate test string");
    }

    // test: string with no leading whitespace
    str = d_string_new_from_cstr("hello   ");

    if (str)
    {
        result = d_string_trim_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_no_leading",
                         result && d_string_equals_cstr(str, "hello   "),
                         "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_no_leading",
                         false,
                         "failed to allocate test string");
    }

    // test: string with both leading and trailing whitespace
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result = d_string_trim_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_preserves_trailing",
                         result && d_string_equals_cstr(str, "hello  "),
                         "should only trim leading whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_preserves_trailing",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_trim_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_all_whitespace",
                         result && d_string_is_empty(str),
                         "should result in empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_all_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_empty_string",
                         result && d_string_is_empty(str),
                         "should remain empty");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_left_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim_left(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trim_left_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_trim_right
  Tests d_string_trim_right function (in-place, right side only).
  Tests the following:
  - trimming trailing whitespace
  - string with no trailing whitespace
  - string with only leading whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_trim_right(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_trim_right");

    // test: trimming trailing whitespace
    str = d_string_new_from_cstr("hello world   ");

    if (str)
    {
        result = d_string_trim_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_trailing",
                         result && d_string_equals_cstr(str, "hello world"),
                         "should trim trailing whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_trailing",
                         false,
                         "failed to allocate test string");
    }

    // test: string with no trailing whitespace
    str = d_string_new_from_cstr("   hello");

    if (str)
    {
        result = d_string_trim_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_no_trailing",
                         result && d_string_equals_cstr(str, "   hello"),
                         "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_no_trailing",
                         false,
                         "failed to allocate test string");
    }

    // test: string with both leading and trailing whitespace
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result = d_string_trim_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_preserves_leading",
                         result && d_string_equals_cstr(str, "  hello"),
                         "should only trim trailing whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_preserves_leading",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_trim_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_all_whitespace",
                         result && d_string_is_empty(str),
                         "should result in empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_all_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_empty_string",
                         result && d_string_is_empty(str),
                         "should remain empty");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_right_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim_right(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trim_right_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_trim_chars
  Tests d_string_trim_chars function (in-place, custom characters).
  Tests the following:
  - trimming custom characters from both sides
  - trimming single custom character
  - trimming multiple custom characters
  - no characters to trim
  - string that is all trim characters
  - empty string handling
  - NULL string handling
  - NULL chars handling
*/
bool d_tests_dstring_trim_chars(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_trim_chars");

    // test: trimming custom characters from both sides
    str = d_string_new_from_cstr("---hello world---");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_both_sides",
                         result && d_string_equals_cstr(str, "hello world"),
                         "should trim dashes from both sides");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_both_sides",
                         false,
                         "failed to allocate test string");
    }

    // test: trimming single custom character
    str = d_string_new_from_cstr("xxxhelloxxx");

    if (str)
    {
        result = d_string_trim_chars(str, "x");
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_single_char",
                         result && d_string_equals_cstr(str, "hello"),
                         "should trim 'x' from both sides");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_single_char",
                         false,
                         "failed to allocate test string");
    }

    // test: trimming multiple custom characters
    str = d_string_new_from_cstr(".-.-hello-.-.");

    if (str)
    {
        result = d_string_trim_chars(str, ".-");
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_multiple_chars",
                         result && d_string_equals_cstr(str, "hello"),
                         "should trim '.' and '-' from both sides");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_multiple_chars",
                         false,
                         "failed to allocate test string");
    }

    // test: no characters to trim
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_no_match",
                         result && d_string_equals_cstr(str, "hello"),
                         "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_no_match",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all trim characters
    str = d_string_new_from_cstr("-----");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_all_trim",
                         result && d_string_is_empty(str),
                         "should result in empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_all_trim",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_empty_string",
                         result && d_string_is_empty(str),
                         "should remain empty");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trim_chars_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim_chars(NULL, "-");
    D_TEST_CHECK_MSG(_suite,
                     "trim_chars_null_string",
                     !(result),
                     "should return false for NULL string");

    // test: NULL chars handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_trim_chars(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trim_chars_null_chars",
                     !(result),
                     "should return false for NULL chars");

    d_string_free(str);

    return true;
}


/******************************************************************************
 * II. NON-MODIFYING TRIMMING TESTS (RETURN NEW STRING)
 *****************************************************************************/

/*
d_tests_dstring_trimmed
  Tests d_string_trimmed function (returns new string, both sides).
  Tests the following:
  - creating trimmed copy from string with whitespace
  - original string unchanged
  - string with no whitespace to trim
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_trimmed(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      result_str;

    d_test_suite_test(_suite, "d_string_trimmed");

    // test: creating trimmed copy from string with whitespace
    str = d_string_new_from_cstr("  hello world  ");

    if (str)
    {
        result_str = d_string_trimmed(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_both_sides",
                         result_str && d_string_equals_cstr(result_str, "hello world"),
                         "should create trimmed copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_both_sides",
                         false,
                         "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result_str = d_string_trimmed(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_original_unchanged",
                         d_string_equals_cstr(str, "  hello  "),
                         "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_original_unchanged",
                         false,
                         "failed to allocate test string");
    }

    // test: string with no whitespace to trim
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result_str = d_string_trimmed(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_no_whitespace",
                         result_str && d_string_equals_cstr(result_str, "hello"),
                         "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_no_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result_str = d_string_trimmed(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_all_whitespace",
                         result_str && d_string_is_empty(result_str),
                         "should create empty string");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_all_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_trimmed(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_empty_string",
                         result_str && d_string_is_empty(result_str),
                         "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_trimmed(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trimmed_null_string",
                     (result_str) == NULL,
                     "should return NULL for NULL string");

    return true;
}

/*
d_tests_dstring_trimmed_left
  Tests d_string_trimmed_left function (returns new string, left side only).
  Tests the following:
  - creating left-trimmed copy
  - original string unchanged
  - string with no leading whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_trimmed_left(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      result_str;

    d_test_suite_test(_suite, "d_string_trimmed_left");

    // test: creating left-trimmed copy
    str = d_string_new_from_cstr("   hello world  ");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_leading",
                         result_str && d_string_equals_cstr(result_str, "hello world  "),
                         "should create left-trimmed copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_leading",
                         false,
                         "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("  hello");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_original_unchanged",
                         d_string_equals_cstr(str, "  hello"),
                         "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_original_unchanged",
                         false,
                         "failed to allocate test string");
    }

    // test: string with no leading whitespace
    str = d_string_new_from_cstr("hello   ");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_no_leading",
                         result_str && d_string_equals_cstr(result_str, "hello   "),
                         "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_no_leading",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_all_whitespace",
                         result_str && d_string_is_empty(result_str),
                         "should create empty string");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_all_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_empty_string",
                         result_str && d_string_is_empty(result_str),
                         "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_left_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_trimmed_left(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trimmed_left_null_string",
                     (result_str) == NULL,
                     "should return NULL for NULL string");

    return true;
}

/*
d_tests_dstring_trimmed_right
  Tests d_string_trimmed_right function (returns new string, right side only).
  Tests the following:
  - creating right-trimmed copy
  - original string unchanged
  - string with no trailing whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_trimmed_right(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      result_str;

    d_test_suite_test(_suite, "d_string_trimmed_right");

    // test: creating right-trimmed copy
    str = d_string_new_from_cstr("  hello world   ");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_trailing",
                         result_str && d_string_equals_cstr(result_str, "  hello world"),
                         "should create right-trimmed copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_trailing",
                         false,
                         "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("hello  ");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_original_unchanged",
                         d_string_equals_cstr(str, "hello  "),
                         "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_original_unchanged",
                         false,
                         "failed to allocate test string");
    }

    // test: string with no trailing whitespace
    str = d_string_new_from_cstr("   hello");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_no_trailing",
                         result_str && d_string_equals_cstr(result_str, "   hello"),
                         "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_no_trailing",
                         false,
                         "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_all_whitespace",
                         result_str && d_string_is_empty(result_str),
                         "should create empty string");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_all_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_empty_string",
                         result_str && d_string_is_empty(result_str),
                         "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "trimmed_right_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_trimmed_right(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "trimmed_right_null_string",
                     (result_str) == NULL,
                     "should return NULL for NULL string");

    return true;
}


/******************************************************************************
 * TRIM ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_dstring_trim_all
  Runs all trimming function tests for dstring module.
  Tests the following:
  - in-place trimming functions (trim, trim_left, trim_right, trim_chars)
  - non-modifying trimming functions (trimmed, trimmed_left, trimmed_right)
*/
bool d_tests_dstring_trim_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Trimming Functions");

    // in-place trimming tests
    d_tests_dstring_trim(_suite);
    d_tests_dstring_trim_left(_suite);
    d_tests_dstring_trim_right(_suite);
    d_tests_dstring_trim_chars(_suite);

    // non-modifying trimming tests
    d_tests_dstring_trimmed(_suite);
    d_tests_dstring_trimmed_left(_suite);
    d_tests_dstring_trimmed_right(_suite);

    return true;
}
