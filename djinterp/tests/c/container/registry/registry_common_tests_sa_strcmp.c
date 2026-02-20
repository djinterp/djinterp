#include ".\registry_common_tests_sa.h"


/*
d_tests_sa_registry_strcmp_equal_strings
  Tests d_registry_strcmp with equal strings.
  Tests the following:
  - case-sensitive comparison of identical strings returns 0
  - case-insensitive comparison of identical strings returns 0
*/
bool
d_tests_sa_registry_strcmp_equal_strings
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: case-sensitive equal strings
    cmp_result = d_registry_strcmp("hello", "hello", true);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_equal_case_sensitive",
        "Equal strings with case-sensitive should return 0",
        _counter) && result;

    // test 2: case-insensitive equal strings
    cmp_result = d_registry_strcmp("hello", "hello", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_equal_case_insensitive",
        "Equal strings with case-insensitive should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_less_than
  Tests d_registry_strcmp when first string is lexicographically less.
  Tests the following:
  - case-sensitive: "abc" < "abd" returns negative
  - case-sensitive: "abc" < "abcd" returns negative
  - case-insensitive: "abc" < "abd" returns negative
*/
bool
d_tests_sa_registry_strcmp_less_than
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: case-sensitive "abc" < "abd"
    cmp_result = d_registry_strcmp("abc", "abd", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_less_than_char_diff",
        "\"abc\" < \"abd\" should return negative",
        _counter) && result;

    // test 2: case-sensitive prefix is less "abc" < "abcd"
    cmp_result = d_registry_strcmp("abc", "abcd", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_less_than_prefix",
        "\"abc\" < \"abcd\" should return negative",
        _counter) && result;

    // test 3: case-insensitive "ABC" < "abd"
    cmp_result = d_registry_strcmp("ABC", "abd", false);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_less_than_case_insensitive",
        "\"ABC\" < \"abd\" (case-insensitive) should return negative",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_greater_than
  Tests d_registry_strcmp when first string is lexicographically greater.
  Tests the following:
  - case-sensitive: "abd" > "abc" returns positive
  - case-sensitive: "abcd" > "abc" returns positive
  - case-insensitive: "ABD" > "abc" returns positive
*/
bool
d_tests_sa_registry_strcmp_greater_than
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: case-sensitive "abd" > "abc"
    cmp_result = d_registry_strcmp("abd", "abc", true);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_greater_than_char_diff",
        "\"abd\" > \"abc\" should return positive",
        _counter) && result;

    // test 2: case-sensitive longer string "abcd" > "abc"
    cmp_result = d_registry_strcmp("abcd", "abc", true);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_greater_than_longer",
        "\"abcd\" > \"abc\" should return positive",
        _counter) && result;

    // test 3: case-insensitive "ABD" > "abc"
    cmp_result = d_registry_strcmp("ABD", "abc", false);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_greater_than_case_insensitive",
        "\"ABD\" > \"abc\" (case-insensitive) should return positive",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_both_null
  Tests d_registry_strcmp when both strings are NULL.
  Tests the following:
  - NULL == NULL should return 0 (case-sensitive)
  - NULL == NULL should return 0 (case-insensitive)
*/
bool
d_tests_sa_registry_strcmp_both_null
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: both NULL, case-sensitive
    cmp_result = d_registry_strcmp(NULL, NULL, true);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_both_null_case_sensitive",
        "Both NULL with case-sensitive should return 0",
        _counter) && result;

    // test 2: both NULL, case-insensitive
    cmp_result = d_registry_strcmp(NULL, NULL, false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_both_null_case_insensitive",
        "Both NULL with case-insensitive should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_first_null
  Tests d_registry_strcmp when first string is NULL.
  Tests the following:
  - NULL < non-NULL should return negative (case-sensitive)
  - NULL < non-NULL should return negative (case-insensitive)
*/
bool
d_tests_sa_registry_strcmp_first_null
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: first NULL, case-sensitive
    cmp_result = d_registry_strcmp(NULL, "hello", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_first_null_case_sensitive",
        "NULL < non-NULL (case-sensitive) should return negative",
        _counter) && result;

    // test 2: first NULL, case-insensitive
    cmp_result = d_registry_strcmp(NULL, "hello", false);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_first_null_case_insensitive",
        "NULL < non-NULL (case-insensitive) should return negative",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_second_null
  Tests d_registry_strcmp when second string is NULL.
  Tests the following:
  - non-NULL > NULL should return positive (case-sensitive)
  - non-NULL > NULL should return positive (case-insensitive)
*/
bool
d_tests_sa_registry_strcmp_second_null
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: second NULL, case-sensitive
    cmp_result = d_registry_strcmp("hello", NULL, true);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_second_null_case_sensitive",
        "non-NULL > NULL (case-sensitive) should return positive",
        _counter) && result;

    // test 2: second NULL, case-insensitive
    cmp_result = d_registry_strcmp("hello", NULL, false);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_second_null_case_insensitive",
        "non-NULL > NULL (case-insensitive) should return positive",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_case_sensitive_diff
  Tests d_registry_strcmp with case differences in case-sensitive mode.
  Tests the following:
  - "Hello" != "hello" in case-sensitive mode
  - "ABC" != "abc" in case-sensitive mode
  - uppercase < lowercase in ASCII ordering
*/
bool
d_tests_sa_registry_strcmp_case_sensitive_diff
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: "Hello" vs "hello" should differ
    cmp_result = d_registry_strcmp("Hello", "hello", true);
    result     = d_assert_standalone(
        cmp_result != 0,
        "strcmp_case_sensitive_Hello_hello",
        "\"Hello\" vs \"hello\" (case-sensitive) should differ",
        _counter) && result;

    // test 2: uppercase 'H' (0x48) < lowercase 'h' (0x68) in ASCII
    result = d_assert_standalone(
        cmp_result < 0,
        "strcmp_case_sensitive_uppercase_less",
        "\"Hello\" < \"hello\" in ASCII ordering",
        _counter) && result;

    // test 3: "ABC" vs "abc"
    cmp_result = d_registry_strcmp("ABC", "abc", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_case_sensitive_ABC_abc",
        "\"ABC\" < \"abc\" in ASCII ordering",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_case_insensitive_equal
  Tests d_registry_strcmp with case differences in case-insensitive mode.
  Tests the following:
  - "Hello" == "hello" in case-insensitive mode
  - "ABC" == "abc" in case-insensitive mode
  - "HeLLo WoRLd" == "hello world" in case-insensitive mode
*/
bool
d_tests_sa_registry_strcmp_case_insensitive_equal
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: "Hello" vs "hello"
    cmp_result = d_registry_strcmp("Hello", "hello", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_case_insensitive_Hello_hello",
        "\"Hello\" == \"hello\" (case-insensitive) should return 0",
        _counter) && result;

    // test 2: "ABC" vs "abc"
    cmp_result = d_registry_strcmp("ABC", "abc", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_case_insensitive_ABC_abc",
        "\"ABC\" == \"abc\" (case-insensitive) should return 0",
        _counter) && result;

    // test 3: mixed case
    cmp_result = d_registry_strcmp("HeLLo WoRLd", "hello world", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_case_insensitive_mixed",
        "\"HeLLo WoRLd\" == \"hello world\" (case-insensitive) should return 0",
        _counter) && result;

    // test 4: all uppercase vs all lowercase
    cmp_result = d_registry_strcmp("REGISTRY_KEY", "registry_key", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_case_insensitive_all_caps",
        "\"REGISTRY_KEY\" == \"registry_key\" (case-insensitive) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_case_insensitive_diff
  Tests d_registry_strcmp with different strings in case-insensitive mode.
  Tests the following:
  - "Hello" != "World" in case-insensitive mode
  - "APPLE" < "BANANA" in case-insensitive mode
*/
bool
d_tests_sa_registry_strcmp_case_insensitive_diff
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: completely different strings
    cmp_result = d_registry_strcmp("Hello", "World", false);
    result     = d_assert_standalone(
        cmp_result != 0,
        "strcmp_case_insensitive_diff_strings",
        "\"Hello\" vs \"World\" (case-insensitive) should differ",
        _counter) && result;

    // test 2: "APPLE" < "BANANA" (case-insensitive)
    cmp_result = d_registry_strcmp("APPLE", "BANANA", false);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_case_insensitive_APPLE_BANANA",
        "\"APPLE\" < \"BANANA\" (case-insensitive) should return negative",
        _counter) && result;

    // test 3: "banana" > "APPLE" (case-insensitive)
    cmp_result = d_registry_strcmp("banana", "APPLE", false);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_case_insensitive_banana_APPLE",
        "\"banana\" > \"APPLE\" (case-insensitive) should return positive",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_empty_strings
  Tests d_registry_strcmp with empty strings.
  Tests the following:
  - "" == "" returns 0 (case-sensitive)
  - "" == "" returns 0 (case-insensitive)
*/
bool
d_tests_sa_registry_strcmp_empty_strings
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: both empty, case-sensitive
    cmp_result = d_registry_strcmp("", "", true);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_empty_both_case_sensitive",
        "\"\" == \"\" (case-sensitive) should return 0",
        _counter) && result;

    // test 2: both empty, case-insensitive
    cmp_result = d_registry_strcmp("", "", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_empty_both_case_insensitive",
        "\"\" == \"\" (case-insensitive) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_empty_vs_nonempty
  Tests d_registry_strcmp with empty string vs non-empty string.
  Tests the following:
  - "" < "hello" returns negative
  - "hello" > "" returns positive
*/
bool
d_tests_sa_registry_strcmp_empty_vs_nonempty
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: empty < non-empty (case-sensitive)
    cmp_result = d_registry_strcmp("", "hello", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_empty_vs_nonempty_first_case_sensitive",
        "\"\" < \"hello\" (case-sensitive) should return negative",
        _counter) && result;

    // test 2: non-empty > empty (case-sensitive)
    cmp_result = d_registry_strcmp("hello", "", true);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_nonempty_vs_empty_case_sensitive",
        "\"hello\" > \"\" (case-sensitive) should return positive",
        _counter) && result;

    // test 3: empty < non-empty (case-insensitive)
    cmp_result = d_registry_strcmp("", "HELLO", false);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_empty_vs_nonempty_case_insensitive",
        "\"\" < \"HELLO\" (case-insensitive) should return negative",
        _counter) && result;

    // test 4: non-empty > empty (case-insensitive)
    cmp_result = d_registry_strcmp("HELLO", "", false);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_nonempty_vs_empty_case_insensitive",
        "\"HELLO\" > \"\" (case-insensitive) should return positive",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_prefix_strings
  Tests d_registry_strcmp with prefix relationships.
  Tests the following:
  - "test" < "testing" (prefix is less)
  - "testing" > "test" (longer is greater)
  - case-insensitive prefix comparison
*/
bool
d_tests_sa_registry_strcmp_prefix_strings
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: prefix < longer string
    cmp_result = d_registry_strcmp("test", "testing", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_prefix_less_than",
        "\"test\" < \"testing\" should return negative",
        _counter) && result;

    // test 2: longer string > prefix
    cmp_result = d_registry_strcmp("testing", "test", true);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_prefix_greater_than",
        "\"testing\" > \"test\" should return positive",
        _counter) && result;

    // test 3: case-insensitive prefix
    cmp_result = d_registry_strcmp("TEST", "testing", false);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_prefix_case_insensitive",
        "\"TEST\" < \"testing\" (case-insensitive) should return negative",
        _counter) && result;

    // test 4: exact match with same length
    cmp_result = d_registry_strcmp("TEST", "test", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_prefix_exact_match",
        "\"TEST\" == \"test\" (case-insensitive) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_special_chars
  Tests d_registry_strcmp with special characters.
  Tests the following:
  - strings with underscores
  - strings with dashes
  - strings with dots
  - strings with mixed special characters
*/
bool
d_tests_sa_registry_strcmp_special_chars
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: underscore strings equal
    cmp_result = d_registry_strcmp("test_key", "test_key", true);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_special_underscore_equal",
        "\"test_key\" == \"test_key\" should return 0",
        _counter) && result;

    // test 2: underscore vs dash
    cmp_result = d_registry_strcmp("test_key", "test-key", true);
    result     = d_assert_standalone(
        cmp_result != 0,
        "strcmp_special_underscore_vs_dash",
        "\"test_key\" vs \"test-key\" should differ",
        _counter) && result;

    // test 3: dot in string
    cmp_result = d_registry_strcmp("config.value", "config.value", true);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_special_dot_equal",
        "\"config.value\" == \"config.value\" should return 0",
        _counter) && result;

    // test 4: mixed special chars ordering
    cmp_result = d_registry_strcmp("a.b", "a_b", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_special_dot_vs_underscore",
        "\"a.b\" < \"a_b\" (ASCII: '.' < '_') should return negative",
        _counter) && result;

    // test 5: special chars with case insensitivity
    cmp_result = d_registry_strcmp("Test_KEY", "test_key", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_special_underscore_case_insensitive",
        "\"Test_KEY\" == \"test_key\" (case-insensitive) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_numeric_chars
  Tests d_registry_strcmp with numeric characters.
  Tests the following:
  - strings with digits
  - numeric ordering
  - mixed alphanumeric strings
*/
bool
d_tests_sa_registry_strcmp_numeric_chars
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp_result;

    result = true;

    // test 1: numeric strings equal
    cmp_result = d_registry_strcmp("key123", "key123", true);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_numeric_equal",
        "\"key123\" == \"key123\" should return 0",
        _counter) && result;

    // test 2: lexicographic ordering of numbers (not numeric!)
    cmp_result = d_registry_strcmp("key2", "key10", true);
    result     = d_assert_standalone(
        cmp_result > 0,
        "strcmp_numeric_lex_order",
        "\"key2\" > \"key10\" (lexicographic, '2' > '1') should return positive",
        _counter) && result;

    // test 3: pure digit strings
    cmp_result = d_registry_strcmp("123", "456", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_pure_digits",
        "\"123\" < \"456\" should return negative",
        _counter) && result;

    // test 4: alphanumeric comparison
    cmp_result = d_registry_strcmp("abc123", "abc124", true);
    result     = d_assert_standalone(
        cmp_result < 0,
        "strcmp_alphanumeric_diff",
        "\"abc123\" < \"abc124\" should return negative",
        _counter) && result;

    // test 5: numeric suffix case insensitive
    cmp_result = d_registry_strcmp("KEY123", "key123", false);
    result     = d_assert_standalone(
        cmp_result == 0,
        "strcmp_numeric_case_insensitive",
        "\"KEY123\" == \"key123\" (case-insensitive) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_strcmp_all
  Aggregation function that runs all d_registry_strcmp tests.
*/
bool
d_tests_sa_registry_strcmp_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] String Comparison Functions (d_registry_strcmp)\n");
    printf("  -----------------------------------------------------------\n");

    result = d_tests_sa_registry_strcmp_equal_strings(_counter) && result;
    result = d_tests_sa_registry_strcmp_less_than(_counter) && result;
    result = d_tests_sa_registry_strcmp_greater_than(_counter) && result;
    result = d_tests_sa_registry_strcmp_both_null(_counter) && result;
    result = d_tests_sa_registry_strcmp_first_null(_counter) && result;
    result = d_tests_sa_registry_strcmp_second_null(_counter) && result;
    result = d_tests_sa_registry_strcmp_case_sensitive_diff(_counter) && result;
    result = d_tests_sa_registry_strcmp_case_insensitive_equal(_counter) && result;
    result = d_tests_sa_registry_strcmp_case_insensitive_diff(_counter) && result;
    result = d_tests_sa_registry_strcmp_empty_strings(_counter) && result;
    result = d_tests_sa_registry_strcmp_empty_vs_nonempty(_counter) && result;
    result = d_tests_sa_registry_strcmp_prefix_strings(_counter) && result;
    result = d_tests_sa_registry_strcmp_special_chars(_counter) && result;
    result = d_tests_sa_registry_strcmp_numeric_chars(_counter) && result;

    return result;
}
