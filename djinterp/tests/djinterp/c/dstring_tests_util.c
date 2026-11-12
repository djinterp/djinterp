/*******************************************************************************
* djinterp [c]                                              dstring_tests_util.c
*
*
* path:      /tests/djinterp/c/dstring_tests_util.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dstring_tests.h"


/******************************************************************************
 * SECTION 15: UTILITY FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * I. VALIDATION TESTS
 *****************************************************************************/

/*
d_tests_dstring_is_valid
  Tests d_string_is_valid function.
  Tests the following:
  - valid string returns true
  - empty string is valid
  - NULL string returns false
*/
bool d_tests_dstring_is_valid(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_is_valid");

    // test: valid string returns true
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_is_valid(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_valid_normal_string",
                         result == true,
                         "should return true for valid string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_valid_normal_string",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string is valid
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_is_valid(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_valid_empty_string",
                         result == true,
                         "should return true for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_valid_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string returns false
    result = d_string_is_valid(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "is_valid_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_is_ascii
  Tests d_string_is_ascii function.
  Tests the following:
  - string with only ASCII characters returns true
  - string with non-ASCII characters returns false
  - empty string returns true
  - NULL string handling
*/
bool d_tests_dstring_is_ascii(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_is_ascii");

    // test: string with only ASCII characters returns true
    str = d_string_new_from_cstr("Hello, World! 123");

    if (str)
    {
        result = d_string_is_ascii(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_ascii_all_ascii",
                         result == true,
                         "should return true for ASCII-only string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_ascii_all_ascii",
                         false,
                         "failed to allocate test string");
    }

    // test: string with non-ASCII characters returns false
    str = d_string_new_from_cstr("Hello \xC0\xC1");

    if (str)
    {
        result = d_string_is_ascii(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_ascii_non_ascii",
                         !(result),
                         "should return false for non-ASCII characters");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_ascii_non_ascii",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string returns true
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_is_ascii(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_ascii_empty_string",
                         result == true,
                         "should return true for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_ascii_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_is_ascii(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "is_ascii_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_is_numeric
  Tests d_string_is_numeric function.
  Tests the following:
  - string with only digits returns true
  - string with letters returns false
  - string with spaces returns false
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_is_numeric(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_is_numeric");

    // test: string with only digits returns true
    str = d_string_new_from_cstr("1234567890");

    if (str)
    {
        result = d_string_is_numeric(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_all_digits",
                         result == true,
                         "should return true for all digits");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_all_digits",
                         false,
                         "failed to allocate test string");
    }

    // test: string with letters returns false
    str = d_string_new_from_cstr("123abc");

    if (str)
    {
        result = d_string_is_numeric(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_with_letters",
                         !(result),
                         "should return false for string with letters");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_with_letters",
                         false,
                         "failed to allocate test string");
    }

    // test: string with spaces returns false
    str = d_string_new_from_cstr("123 456");

    if (str)
    {
        result = d_string_is_numeric(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_with_spaces",
                         !(result),
                         "should return false for string with spaces");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_with_spaces",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_is_numeric(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_empty_string",
                         !(result),
                         "should return false for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_numeric_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_is_numeric(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "is_numeric_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_is_alpha
  Tests d_string_is_alpha function.
  Tests the following:
  - string with only letters returns true
  - string with digits returns false
  - string with spaces returns false
  - mixed case letters returns true
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_is_alpha(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_is_alpha");

    // test: string with only letters returns true
    str = d_string_new_from_cstr("HelloWorld");

    if (str)
    {
        result = d_string_is_alpha(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_all_letters",
                         result == true,
                         "should return true for all letters");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_all_letters",
                         false,
                         "failed to allocate test string");
    }

    // test: string with digits returns false
    str = d_string_new_from_cstr("Hello123");

    if (str)
    {
        result = d_string_is_alpha(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_with_digits",
                         !(result),
                         "should return false for string with digits");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_with_digits",
                         false,
                         "failed to allocate test string");
    }

    // test: string with spaces returns false
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_is_alpha(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_with_spaces",
                         !(result),
                         "should return false for string with spaces");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_with_spaces",
                         false,
                         "failed to allocate test string");
    }

    // test: mixed case letters returns true
    str = d_string_new_from_cstr("AbCdEfGh");

    if (str)
    {
        result = d_string_is_alpha(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_mixed_case",
                         result == true,
                         "should return true for mixed case letters");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_mixed_case",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_is_alpha(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_empty_string",
                         !(result),
                         "should return false for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alpha_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_is_alpha(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "is_alpha_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_is_alnum
  Tests d_string_is_alnum function.
  Tests the following:
  - string with only letters returns true
  - string with only digits returns true
  - string with letters and digits returns true
  - string with special characters returns false
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_is_alnum(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_is_alnum");

    // test: string with only letters returns true
    str = d_string_new_from_cstr("HelloWorld");

    if (str)
    {
        result = d_string_is_alnum(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_all_letters",
                         result == true,
                         "should return true for all letters");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_all_letters",
                         false,
                         "failed to allocate test string");
    }

    // test: string with only digits returns true
    str = d_string_new_from_cstr("1234567890");

    if (str)
    {
        result = d_string_is_alnum(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_all_digits",
                         result == true,
                         "should return true for all digits");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_all_digits",
                         false,
                         "failed to allocate test string");
    }

    // test: string with letters and digits returns true
    str = d_string_new_from_cstr("Hello123World456");

    if (str)
    {
        result = d_string_is_alnum(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_mixed",
                         result == true,
                         "should return true for letters and digits");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_mixed",
                         false,
                         "failed to allocate test string");
    }

    // test: string with special characters returns false
    str = d_string_new_from_cstr("Hello_World!");

    if (str)
    {
        result = d_string_is_alnum(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_with_special",
                         !(result),
                         "should return false for special characters");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_with_special",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_is_alnum(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_empty_string",
                         !(result),
                         "should return false for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_alnum_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_is_alnum(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "is_alnum_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_is_whitespace
  Tests d_string_is_whitespace function.
  Tests the following:
  - string with only spaces returns true
  - string with tabs and newlines returns true
  - string with non-whitespace returns false
  - mixed whitespace and text returns false
  - empty string handling
  - NULL string handling
*/
bool d_tests_dstring_is_whitespace(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_is_whitespace");

    // test: string with only spaces returns true
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_is_whitespace(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_spaces_only",
                         result == true,
                         "should return true for spaces only");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_spaces_only",
                         false,
                         "failed to allocate test string");
    }

    // test: string with tabs and newlines returns true
    str = d_string_new_from_cstr(" \t\n\r ");

    if (str)
    {
        result = d_string_is_whitespace(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_mixed_whitespace",
                         result == true,
                         "should return true for tabs and newlines");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_mixed_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: string with non-whitespace returns false
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_is_whitespace(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_non_whitespace",
                         !(result),
                         "should return false for non-whitespace");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_non_whitespace",
                         false,
                         "failed to allocate test string");
    }

    // test: mixed whitespace and text returns false
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result = d_string_is_whitespace(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_mixed_content",
                         !(result),
                         "should return false for mixed content");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_mixed_content",
                         false,
                         "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_is_whitespace(str);
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_empty_string",
                         !(result),
                         "should return false for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "is_whitespace_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_is_whitespace(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "is_whitespace_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}


/******************************************************************************
 * II. COUNTING TESTS
 *****************************************************************************/

/*
d_tests_dstring_count_char
  Tests d_string_count_char function.
  Tests the following:
  - counting character with multiple occurrences
  - counting character with single occurrence
  - counting character not present
  - counting in empty string
  - NULL string handling
*/
bool d_tests_dstring_count_char(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                result;

    d_test_suite_test(_suite, "d_string_count_char");

    // test: counting character with multiple occurrences
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_count_char(str, 'l');
        D_TEST_CHECK_MSG(_suite,
                         "count_char_multiple",
                         (result) == (3),
                         "should count 3 occurrences of 'l'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_char_multiple",
                         false,
                         "failed to allocate test string");
    }

    // test: counting character with single occurrence
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_count_char(str, 'h');
        D_TEST_CHECK_MSG(_suite,
                         "count_char_single",
                         (result) == (1),
                         "should count 1 occurrence of 'h'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_char_single",
                         false,
                         "failed to allocate test string");
    }

    // test: counting character not present
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_count_char(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "count_char_not_present",
                         (result) == (0),
                         "should count 0 occurrences of 'z'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_char_not_present",
                         false,
                         "failed to allocate test string");
    }

    // test: counting in empty string
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_count_char(str, 'a');
        D_TEST_CHECK_MSG(_suite,
                         "count_char_empty_string",
                         (result) == (0),
                         "should count 0 in empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_char_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_count_char(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "count_char_null_string",
                     (result) == (0),
                     "should return 0 for NULL string");

    return true;
}

/*
d_tests_dstring_count_substr
  Tests d_string_count_substr function.
  Tests the following:
  - counting substring with multiple occurrences
  - counting substring with single occurrence
  - counting substring not present
  - counting overlapping substrings
  - counting in empty string
  - NULL string handling
  - NULL substring handling
*/
bool d_tests_dstring_count_substr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                result;

    d_test_suite_test(_suite, "d_string_count_substr");

    // test: counting substring with multiple occurrences
    str = d_string_new_from_cstr("abcabcabc");

    if (str)
    {
        result = d_string_count_substr(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_multiple",
                         (result) == (3),
                         "should count 3 occurrences of 'abc'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_multiple",
                         false,
                         "failed to allocate test string");
    }

    // test: counting substring with single occurrence
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_count_substr(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_single",
                         (result) == (1),
                         "should count 1 occurrence of 'world'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_single",
                         false,
                         "failed to allocate test string");
    }

    // test: counting substring not present
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_count_substr(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_not_present",
                         (result) == (0),
                         "should count 0 occurrences of 'xyz'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_not_present",
                         false,
                         "failed to allocate test string");
    }

    // test: counting overlapping substrings (non-overlapping count)
    str = d_string_new_from_cstr("aaaa");

    if (str)
    {
        result = d_string_count_substr(str, "aa");
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_overlapping",
                         result >= 2,
                         "should count at least 2 non-overlapping 'aa'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_overlapping",
                         false,
                         "failed to allocate test string");
    }

    // test: counting in empty string
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_count_substr(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_empty_string",
                         (result) == (0),
                         "should count 0 in empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "count_substr_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_count_substr(NULL, "abc");
    D_TEST_CHECK_MSG(_suite,
                     "count_substr_null_string",
                     (result) == (0),
                     "should return 0 for NULL string");

    // test: NULL substring handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_count_substr(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "count_substr_null_substr",
                     (result) == (0),
                     "should return 0 for NULL substring");

    d_string_free(str);

    return true;
}


/******************************************************************************
 * III. HASHING TESTS
 *****************************************************************************/

/*
d_tests_dstring_hash
  Tests d_string_hash function.
  Tests the following:
  - same string produces same hash
  - different strings produce different hashes (usually)
  - empty string produces valid hash
  - NULL string handling
*/
bool d_tests_dstring_hash(struct dj_test_suite* _suite)
{
    struct dj_string*      str1;
    struct dj_string*      str2;
    size_t                hash1;
    size_t                hash2;

    d_test_suite_test(_suite, "d_string_hash");

    // test: same string produces same hash
    str1 = d_string_new_from_cstr("hello world");
    str2 = d_string_new_from_cstr("hello world");

    if (str1 && str2)
    {
        hash1 = d_string_hash(str1);
        hash2 = d_string_hash(str2);
        D_TEST_CHECK_MSG(_suite,
                         "hash_same_string",
                         (hash1) == (hash2),
                         "same strings should produce same hash");

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "hash_same_string",
                         false,
                         "failed to allocate test strings");

        d_string_free(str1);
        d_string_free(str2);
    }

    // test: different strings produce different hashes (usually)
    str1 = d_string_new_from_cstr("hello");
    str2 = d_string_new_from_cstr("world");

    if (str1 && str2)
    {
        hash1 = d_string_hash(str1);
        hash2 = d_string_hash(str2);
        D_TEST_CHECK_MSG(_suite,
                         "hash_different_strings",
                         hash1 != hash2,
                         "different strings should (usually) produce different hashes");

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "hash_different_strings",
                         false,
                         "failed to allocate test strings");

        d_string_free(str1);
        d_string_free(str2);
    }

    // test: empty string produces valid hash
    str1 = d_string_new_from_cstr("");

    if (str1)
    {
        hash1 = d_string_hash(str1);
        D_TEST_CHECK_MSG(_suite,
                         "hash_empty_string",
                         true,
                         "empty string should produce valid hash");

        d_string_free(str1);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "hash_empty_string",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    hash1 = d_string_hash(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "hash_null_string",
                     (hash1) == (0),
                     "should return 0 for NULL string");

    return true;
}


/******************************************************************************
 * UTIL ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_dstring_util_all
  Runs all utility function tests for dstring module.
  Tests the following:
  - validation functions (is_valid, is_ascii, is_numeric, is_alpha,
    is_alnum, is_whitespace)
  - counting functions (count_char, count_substr)
  - hashing function (hash)
*/
bool d_tests_dstring_util_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Utility Functions");

    // validation tests
    d_tests_dstring_is_valid(_suite);
    d_tests_dstring_is_ascii(_suite);
    d_tests_dstring_is_numeric(_suite);
    d_tests_dstring_is_alpha(_suite);
    d_tests_dstring_is_alnum(_suite);
    d_tests_dstring_is_whitespace(_suite);

    // counting tests
    d_tests_dstring_count_char(_suite);
    d_tests_dstring_count_substr(_suite);

    // hashing tests
    d_tests_dstring_hash(_suite);

    return true;
}
