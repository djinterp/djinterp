/*******************************************************************************
* djinterp [container]                            registry_common_tests_strcmp.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_common_tests_strcmp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_strcmp_equal_strings
  Tests d_registry_strcmp with equal strings.
  Tests the following:
  - case-sensitive comparison of identical strings returns 0
  - case-insensitive comparison of identical strings returns 0
*/
bool d_tests_registry_strcmp_equal_strings(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_equal_strings");

    result = true;

    // test 1: case-sensitive equal strings
    cmp_result = d_registry_strcmp("hello", "hello", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_equal_case_sensitive",
                                  cmp_result == 0,
                                  "Equal strings with case-sensitive should return 0") && result;

    // test 2: case-insensitive equal strings
    cmp_result = d_registry_strcmp("hello", "hello", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_equal_case_insensitive",
                                  cmp_result == 0,
                                  "Equal strings with case-insensitive should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_less_than
  Tests d_registry_strcmp when first string is lexicographically less.
  Tests the following:
  - case-sensitive: "abc" < "abd" returns negative
  - case-sensitive: "abc" < "abcd" returns negative
  - case-insensitive: "abc" < "abd" returns negative
*/
bool d_tests_registry_strcmp_less_than(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_less_than");

    result = true;

    // test 1: case-sensitive "abc" < "abd"
    cmp_result = d_registry_strcmp("abc", "abd", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_less_than_char_diff",
                                  cmp_result < 0,
                                  "\"abc\" < \"abd\" should return negative") && result;

    // test 2: case-sensitive prefix is less "abc" < "abcd"
    cmp_result = d_registry_strcmp("abc", "abcd", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_less_than_prefix",
                                  cmp_result < 0,
                                  "\"abc\" < \"abcd\" should return negative") && result;

    // test 3: case-insensitive "ABC" < "abd"
    cmp_result = d_registry_strcmp("ABC", "abd", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_less_than_case_insensitive",
                                  cmp_result < 0,
                                  "\"ABC\" < \"abd\" (case-insensitive) should return negative") && result;

    return result;
}


/*
d_tests_registry_strcmp_greater_than
  Tests d_registry_strcmp when first string is lexicographically greater.
  Tests the following:
  - case-sensitive: "abd" > "abc" returns positive
  - case-sensitive: "abcd" > "abc" returns positive
  - case-insensitive: "ABD" > "abc" returns positive
*/
bool d_tests_registry_strcmp_greater_than(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_greater_than");

    result = true;

    // test 1: case-sensitive "abd" > "abc"
    cmp_result = d_registry_strcmp("abd", "abc", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_greater_than_char_diff",
                                  cmp_result > 0,
                                  "\"abd\" > \"abc\" should return positive") && result;

    // test 2: case-sensitive longer string "abcd" > "abc"
    cmp_result = d_registry_strcmp("abcd", "abc", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_greater_than_longer",
                                  cmp_result > 0,
                                  "\"abcd\" > \"abc\" should return positive") && result;

    // test 3: case-insensitive "ABD" > "abc"
    cmp_result = d_registry_strcmp("ABD", "abc", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_greater_than_case_insensitive",
                                  cmp_result > 0,
                                  "\"ABD\" > \"abc\" (case-insensitive) should return positive") && result;

    return result;
}


/*
d_tests_registry_strcmp_both_null
  Tests d_registry_strcmp when both strings are NULL.
  Tests the following:
  - NULL == NULL should return 0 (case-sensitive)
  - NULL == NULL should return 0 (case-insensitive)
*/
bool d_tests_registry_strcmp_both_null(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_both_null");

    result = true;

    // test 1: both NULL, case-sensitive
    cmp_result = d_registry_strcmp(NULL, NULL, true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_both_null_case_sensitive",
                                  cmp_result == 0,
                                  "Both NULL with case-sensitive should return 0") && result;

    // test 2: both NULL, case-insensitive
    cmp_result = d_registry_strcmp(NULL, NULL, false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_both_null_case_insensitive",
                                  cmp_result == 0,
                                  "Both NULL with case-insensitive should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_first_null
  Tests d_registry_strcmp when first string is NULL.
  Tests the following:
  - NULL < non-NULL should return negative (case-sensitive)
  - NULL < non-NULL should return negative (case-insensitive)
*/
bool d_tests_registry_strcmp_first_null(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_first_null");

    result = true;

    // test 1: first NULL, case-sensitive
    cmp_result = d_registry_strcmp(NULL, "hello", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_first_null_case_sensitive",
                                  cmp_result < 0,
                                  "NULL < non-NULL (case-sensitive) should return negative") && result;

    // test 2: first NULL, case-insensitive
    cmp_result = d_registry_strcmp(NULL, "hello", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_first_null_case_insensitive",
                                  cmp_result < 0,
                                  "NULL < non-NULL (case-insensitive) should return negative") && result;

    return result;
}


/*
d_tests_registry_strcmp_second_null
  Tests d_registry_strcmp when second string is NULL.
  Tests the following:
  - non-NULL > NULL should return positive (case-sensitive)
  - non-NULL > NULL should return positive (case-insensitive)
*/
bool d_tests_registry_strcmp_second_null(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_second_null");

    result = true;

    // test 1: second NULL, case-sensitive
    cmp_result = d_registry_strcmp("hello", NULL, true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_second_null_case_sensitive",
                                  cmp_result > 0,
                                  "non-NULL > NULL (case-sensitive) should return positive") && result;

    // test 2: second NULL, case-insensitive
    cmp_result = d_registry_strcmp("hello", NULL, false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_second_null_case_insensitive",
                                  cmp_result > 0,
                                  "non-NULL > NULL (case-insensitive) should return positive") && result;

    return result;
}


/*
d_tests_registry_strcmp_case_sensitive_diff
  Tests d_registry_strcmp with case differences in case-sensitive mode.
  Tests the following:
  - "Hello" != "hello" in case-sensitive mode
  - "ABC" != "abc" in case-sensitive mode
  - uppercase < lowercase in ASCII ordering
*/
bool d_tests_registry_strcmp_case_sensitive_diff(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_case_sensitive_diff");

    result = true;

    // test 1: "Hello" vs "hello" should differ
    cmp_result = d_registry_strcmp("Hello", "hello", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_sensitive_Hello_hello",
                                  cmp_result != 0,
                                  "\"Hello\" vs \"hello\" (case-sensitive) should differ") && result;

    // test 2: uppercase 'H' (0x48) < lowercase 'h' (0x68) in ASCII
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_case_sensitive_uppercase_less",
                              cmp_result < 0,
                              "\"Hello\" < \"hello\" in ASCII ordering") && result;

    // test 3: "ABC" vs "abc"
    cmp_result = d_registry_strcmp("ABC", "abc", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_sensitive_ABC_abc",
                                  cmp_result < 0,
                                  "\"ABC\" < \"abc\" in ASCII ordering") && result;

    return result;
}


/*
d_tests_registry_strcmp_case_insensitive_equal
  Tests d_registry_strcmp with case differences in case-insensitive mode.
  Tests the following:
  - "Hello" == "hello" in case-insensitive mode
  - "ABC" == "abc" in case-insensitive mode
  - "HeLLo WoRLd" == "hello world" in case-insensitive mode
*/
bool d_tests_registry_strcmp_case_insensitive_equal(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_case_insensitive_equal");

    result = true;

    // test 1: "Hello" vs "hello"
    cmp_result = d_registry_strcmp("Hello", "hello", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_Hello_hello",
                                  cmp_result == 0,
                                  "\"Hello\" == \"hello\" (case-insensitive) should return 0") && result;

    // test 2: "ABC" vs "abc"
    cmp_result = d_registry_strcmp("ABC", "abc", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_ABC_abc",
                                  cmp_result == 0,
                                  "\"ABC\" == \"abc\" (case-insensitive) should return 0") && result;

    // test 3: mixed case
    cmp_result = d_registry_strcmp("HeLLo WoRLd", "hello world", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_mixed",
                                  cmp_result == 0,
                                  "\"HeLLo WoRLd\" == \"hello world\" (case-insensitive) should return 0") && result;

    // test 4: all uppercase vs all lowercase
    cmp_result = d_registry_strcmp("REGISTRY_KEY", "registry_key", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_all_caps",
                                  cmp_result == 0,
                                  "\"REGISTRY_KEY\" == \"registry_key\" (case-insensitive) should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_case_insensitive_diff
  Tests d_registry_strcmp with different strings in case-insensitive mode.
  Tests the following:
  - "Hello" != "World" in case-insensitive mode
  - "APPLE" < "BANANA" in case-insensitive mode
*/
bool d_tests_registry_strcmp_case_insensitive_diff(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_case_insensitive_diff");

    result = true;

    // test 1: completely different strings
    cmp_result = d_registry_strcmp("Hello", "World", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_diff_strings",
                                  cmp_result != 0,
                                  "\"Hello\" vs \"World\" (case-insensitive) should differ") && result;

    // test 2: "APPLE" < "BANANA" (case-insensitive)
    cmp_result = d_registry_strcmp("APPLE", "BANANA", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_APPLE_BANANA",
                                  cmp_result < 0,
                                  "\"APPLE\" < \"BANANA\" (case-insensitive) should return negative") && result;

    // test 3: "banana" > "APPLE" (case-insensitive)
    cmp_result = d_registry_strcmp("banana", "APPLE", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_case_insensitive_banana_APPLE",
                                  cmp_result > 0,
                                  "\"banana\" > \"APPLE\" (case-insensitive) should return positive") && result;

    return result;
}


/*
d_tests_registry_strcmp_empty_strings
  Tests d_registry_strcmp with empty strings.
  Tests the following:
  - "" == "" returns 0 (case-sensitive)
  - "" == "" returns 0 (case-insensitive)
*/
bool d_tests_registry_strcmp_empty_strings(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_empty_strings");

    result = true;

    // test 1: both empty, case-sensitive
    cmp_result = d_registry_strcmp("", "", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_empty_both_case_sensitive",
                                  cmp_result == 0,
                                  "\"\" == \"\" (case-sensitive) should return 0") && result;

    // test 2: both empty, case-insensitive
    cmp_result = d_registry_strcmp("", "", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_empty_both_case_insensitive",
                                  cmp_result == 0,
                                  "\"\" == \"\" (case-insensitive) should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_empty_vs_nonempty
  Tests d_registry_strcmp with empty string vs non-empty string.
  Tests the following:
  - "" < "hello" returns negative
  - "hello" > "" returns positive
*/
bool d_tests_registry_strcmp_empty_vs_nonempty(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_empty_vs_nonempty");

    result = true;

    // test 1: empty < non-empty (case-sensitive)
    cmp_result = d_registry_strcmp("", "hello", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_empty_vs_nonempty_first_case_sensitive",
                                  cmp_result < 0,
                                  "\"\" < \"hello\" (case-sensitive) should return negative") && result;

    // test 2: non-empty > empty (case-sensitive)
    cmp_result = d_registry_strcmp("hello", "", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_nonempty_vs_empty_case_sensitive",
                                  cmp_result > 0,
                                  "\"hello\" > \"\" (case-sensitive) should return positive") && result;

    // test 3: empty < non-empty (case-insensitive)
    cmp_result = d_registry_strcmp("", "HELLO", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_empty_vs_nonempty_case_insensitive",
                                  cmp_result < 0,
                                  "\"\" < \"HELLO\" (case-insensitive) should return negative") && result;

    // test 4: non-empty > empty (case-insensitive)
    cmp_result = d_registry_strcmp("HELLO", "", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_nonempty_vs_empty_case_insensitive",
                                  cmp_result > 0,
                                  "\"HELLO\" > \"\" (case-insensitive) should return positive") && result;

    return result;
}


/*
d_tests_registry_strcmp_prefix_strings
  Tests d_registry_strcmp with prefix relationships.
  Tests the following:
  - "test" < "testing" (prefix is less)
  - "testing" > "test" (longer is greater)
  - case-insensitive prefix comparison
*/
bool d_tests_registry_strcmp_prefix_strings(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_prefix_strings");

    result = true;

    // test 1: prefix < longer string
    cmp_result = d_registry_strcmp("test", "testing", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_prefix_less_than",
                                  cmp_result < 0,
                                  "\"test\" < \"testing\" should return negative") && result;

    // test 2: longer string > prefix
    cmp_result = d_registry_strcmp("testing", "test", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_prefix_greater_than",
                                  cmp_result > 0,
                                  "\"testing\" > \"test\" should return positive") && result;

    // test 3: case-insensitive prefix
    cmp_result = d_registry_strcmp("TEST", "testing", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_prefix_case_insensitive",
                                  cmp_result < 0,
                                  "\"TEST\" < \"testing\" (case-insensitive) should return negative") && result;

    // test 4: exact match with same length
    cmp_result = d_registry_strcmp("TEST", "test", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_prefix_exact_match",
                                  cmp_result == 0,
                                  "\"TEST\" == \"test\" (case-insensitive) should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_special_chars
  Tests d_registry_strcmp with special characters.
  Tests the following:
  - strings with underscores
  - strings with dashes
  - strings with dots
  - strings with mixed special characters
*/
bool d_tests_registry_strcmp_special_chars(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_special_chars");

    result = true;

    // test 1: underscore strings equal
    cmp_result = d_registry_strcmp("test_key", "test_key", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_special_underscore_equal",
                                  cmp_result == 0,
                                  "\"test_key\" == \"test_key\" should return 0") && result;

    // test 2: underscore vs dash
    cmp_result = d_registry_strcmp("test_key", "test-key", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_special_underscore_vs_dash",
                                  cmp_result != 0,
                                  "\"test_key\" vs \"test-key\" should differ") && result;

    // test 3: dot in string
    cmp_result = d_registry_strcmp("config.value", "config.value", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_special_dot_equal",
                                  cmp_result == 0,
                                  "\"config.value\" == \"config.value\" should return 0") && result;

    // test 4: mixed special chars ordering
    cmp_result = d_registry_strcmp("a.b", "a_b", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_special_dot_vs_underscore",
                                  cmp_result < 0,
                                  "\"a.b\" < \"a_b\" (ASCII: '.' < '_') should return negative") && result;

    // test 5: special chars with case insensitivity
    cmp_result = d_registry_strcmp("Test_KEY", "test_key", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_special_underscore_case_insensitive",
                                  cmp_result == 0,
                                  "\"Test_KEY\" == \"test_key\" (case-insensitive) should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_numeric_chars
  Tests d_registry_strcmp with numeric characters.
  Tests the following:
  - strings with digits
  - numeric ordering
  - mixed alphanumeric strings
*/
bool d_tests_registry_strcmp_numeric_chars(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp_result;

    d_test_suite_test(_suite, "registry_strcmp_numeric_chars");

    result = true;

    // test 1: numeric strings equal
    cmp_result = d_registry_strcmp("key123", "key123", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_numeric_equal",
                                  cmp_result == 0,
                                  "\"key123\" == \"key123\" should return 0") && result;

    // test 2: lexicographic ordering of numbers (not numeric!)
    cmp_result = d_registry_strcmp("key2", "key10", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_numeric_lex_order",
                                  cmp_result > 0,
                                  "\"key2\" > \"key10\" (lexicographic, '2' > '1') should return positive") && result;

    // test 3: pure digit strings
    cmp_result = d_registry_strcmp("123", "456", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_pure_digits",
                                  cmp_result < 0,
                                  "\"123\" < \"456\" should return negative") && result;

    // test 4: alphanumeric comparison
    cmp_result = d_registry_strcmp("abc123", "abc124", true);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_alphanumeric_diff",
                                  cmp_result < 0,
                                  "\"abc123\" < \"abc124\" should return negative") && result;

    // test 5: numeric suffix case insensitive
    cmp_result = d_registry_strcmp("KEY123", "key123", false);
    result     = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_numeric_case_insensitive",
                                  cmp_result == 0,
                                  "\"KEY123\" == \"key123\" (case-insensitive) should return 0") && result;

    return result;
}


/*
d_tests_registry_strcmp_all
  Aggregation function that runs all d_registry_strcmp tests.
*/
bool d_tests_registry_strcmp_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_strcmp_all");

    result = true;

    result = d_tests_registry_strcmp_equal_strings(_suite) && result;
    result = d_tests_registry_strcmp_less_than(_suite) && result;
    result = d_tests_registry_strcmp_greater_than(_suite) && result;
    result = d_tests_registry_strcmp_both_null(_suite) && result;
    result = d_tests_registry_strcmp_first_null(_suite) && result;
    result = d_tests_registry_strcmp_second_null(_suite) && result;
    result = d_tests_registry_strcmp_case_sensitive_diff(_suite) && result;
    result = d_tests_registry_strcmp_case_insensitive_equal(_suite) && result;
    result = d_tests_registry_strcmp_case_insensitive_diff(_suite) && result;
    result = d_tests_registry_strcmp_empty_strings(_suite) && result;
    result = d_tests_registry_strcmp_empty_vs_nonempty(_suite) && result;
    result = d_tests_registry_strcmp_prefix_strings(_suite) && result;
    result = d_tests_registry_strcmp_special_chars(_suite) && result;
    result = d_tests_registry_strcmp_numeric_chars(_suite) && result;

    return result;
}
