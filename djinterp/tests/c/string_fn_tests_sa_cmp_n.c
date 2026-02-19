#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * LENGTH-AWARE COMPARISON TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strcmp_n
  Tests d_strcmp_n for length-aware string comparison.
  Tests the following:
  - returns 0 for identical strings
  - returns negative for lexicographically smaller
  - returns positive for lexicographically larger
  - shorter string compares less when prefix matches
  - handles NULL inputs
  - handles zero-length strings
*/
struct d_test_object*
d_tests_string_fn_strcmp_n
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_identical;
    bool                  test_less_than;
    bool                  test_greater_than;
    bool                  test_prefix_shorter;
    bool                  test_null_handling;
    bool                  test_empty_strings;
    size_t                idx;

    // test 1: identical strings
    result = d_strcmp_n("hello", 5, "hello", 5);
    test_identical = (result == 0);

    // test 2: lexicographically less
    result = d_strcmp_n("apple", 5, "banana", 6);
    test_less_than = (result < 0);

    // test 3: lexicographically greater
    result = d_strcmp_n("zebra", 5, "aardvark", 8);
    test_greater_than = (result > 0);

    // test 4: shorter string with matching prefix is less
    result = d_strcmp_n("Hello", 5, "HelloWorld", 10);
    test_prefix_shorter = (result < 0);

    // test 5: NULL handling
    int r1 = d_strcmp_n(NULL, 0, "test", 4);
    int r2 = d_strcmp_n("test", 4, NULL, 0);
    int r3 = d_strcmp_n(NULL, 0, NULL, 0);
    test_null_handling = (r1 < 0) && (r2 > 0) && (r3 == 0);

    // test 6: empty strings
    result = d_strcmp_n("", 0, "", 0);
    int r4 = d_strcmp_n("", 0, "a", 1);
    int r5 = d_strcmp_n("a", 1, "", 0);
    test_empty_strings = (result == 0) && (r4 < 0) && (r5 > 0);

    // build result tree
    group = d_test_object_new_interior("d_strcmp_n", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("identical",
                                           test_identical,
                                           "returns 0 for identical strings");
    group->elements[idx++] = D_ASSERT_TRUE("less_than",
                                           test_less_than,
                                           "returns negative for lesser string");
    group->elements[idx++] = D_ASSERT_TRUE("greater_than",
                                           test_greater_than,
                                           "returns positive for greater string");
    group->elements[idx++] = D_ASSERT_TRUE("prefix_shorter",
                                           test_prefix_shorter,
                                           "shorter prefix compares less");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("empty_strings",
                                           test_empty_strings,
                                           "handles empty strings");

    return group;
}

/*
d_tests_string_fn_strncmp_n
  Tests d_strncmp_n for bounded length-aware comparison.
  Tests the following:
  - compares only first n characters
  - ignores differences beyond n
  - returns 0 for zero count
  - handles n larger than both strings
  - handles NULL inputs
  - handles strings shorter than n
*/
struct d_test_object*
d_tests_string_fn_strncmp_n
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_n_chars;
    bool                  test_ignore_beyond_n;
    bool                  test_zero_count;
    bool                  test_n_larger;
    bool                  test_null_handling;
    bool                  test_shorter_than_n;
    size_t                idx;

    // test 1: compare first n characters
    result = d_strncmp_n("HelloABC", 8, "HelloXYZ", 8, 5);
    test_n_chars = (result == 0);

    // test 2: ignores differences beyond n
    result = d_strncmp_n("TestABC", 7, "TestXYZ", 7, 4);
    test_ignore_beyond_n = (result == 0);

    // test 3: zero count
    result = d_strncmp_n("different", 9, "strings", 7, 0);
    test_zero_count = (result == 0);

    // test 4: n larger than both strings
    result = d_strncmp_n("short", 5, "short", 5, 100);
    test_n_larger = (result == 0);

    // test 5: NULL handling
    int r1 = d_strncmp_n(NULL, 0, "test", 4, 4);
    int r2 = d_strncmp_n("test", 4, NULL, 0, 4);
    test_null_handling = (r1 < 0) && (r2 > 0);

    // test 6: one string shorter than n
    result = d_strncmp_n("Hi", 2, "HiThere", 7, 5);
    test_shorter_than_n = (result < 0);

    // build result tree
    group = d_test_object_new_interior("d_strncmp_n", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("n_chars",
                                           test_n_chars,
                                           "compares first n characters");
    group->elements[idx++] = D_ASSERT_TRUE("ignore_beyond_n",
                                           test_ignore_beyond_n,
                                           "ignores differences beyond n");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "returns 0 for zero count");
    group->elements[idx++] = D_ASSERT_TRUE("n_larger",
                                           test_n_larger,
                                           "handles n larger than strings");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("shorter_than_n",
                                           test_shorter_than_n,
                                           "handles string shorter than n");

    return group;
}

/*
d_tests_string_fn_strcasecmp_n
  Tests d_strcasecmp_n for length-aware case-insensitive comparison.
  Tests the following:
  - returns 0 for case-different equal strings
  - returns negative for lesser string
  - returns positive for greater string
  - handles NULL inputs
  - handles empty strings
  - shorter prefix compares less
*/
struct d_test_object*
d_tests_string_fn_strcasecmp_n
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_case_diff;
    bool                  test_less_than;
    bool                  test_greater_than;
    bool                  test_null_handling;
    bool                  test_empty_strings;
    bool                  test_prefix_shorter;
    size_t                idx;

    // test 1: case-different but equal
    result = d_strcasecmp_n("HeLLo", 5, "hEllO", 5);
    test_case_diff = (result == 0);

    // test 2: less than
    result = d_strcasecmp_n("APPLE", 5, "banana", 6);
    test_less_than = (result < 0);

    // test 3: greater than
    result = d_strcasecmp_n("zebra", 5, "AARDVARK", 8);
    test_greater_than = (result > 0);

    // test 4: NULL handling
    int r1 = d_strcasecmp_n(NULL, 0, "test", 4);
    int r2 = d_strcasecmp_n("test", 4, NULL, 0);
    int r3 = d_strcasecmp_n(NULL, 0, NULL, 0);
    test_null_handling = (r1 < 0) && (r2 > 0) && (r3 == 0);

    // test 5: empty strings
    result = d_strcasecmp_n("", 0, "", 0);
    test_empty_strings = (result == 0);

    // test 6: shorter prefix
    result = d_strcasecmp_n("HELLO", 5, "helloworld", 10);
    test_prefix_shorter = (result < 0);

    // build result tree
    group = d_test_object_new_interior("d_strcasecmp_n", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("case_diff",
                                           test_case_diff,
                                           "returns 0 for case-different equals");
    group->elements[idx++] = D_ASSERT_TRUE("less_than",
                                           test_less_than,
                                           "returns negative for lesser string");
    group->elements[idx++] = D_ASSERT_TRUE("greater_than",
                                           test_greater_than,
                                           "returns positive for greater string");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("empty_strings",
                                           test_empty_strings,
                                           "handles empty strings");
    group->elements[idx++] = D_ASSERT_TRUE("prefix_shorter",
                                           test_prefix_shorter,
                                           "shorter prefix compares less");

    return group;
}

/*
d_tests_string_fn_strncasecmp_n
  Tests d_strncasecmp_n for bounded length-aware case-insensitive comparison.
  Tests the following:
  - compares first n characters case-insensitively
  - ignores differences beyond n
  - returns 0 for zero count
  - handles n larger than strings
  - handles NULL inputs
  - handles mixed case within n
*/
struct d_test_object*
d_tests_string_fn_strncasecmp_n
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_n_chars;
    bool                  test_ignore_beyond_n;
    bool                  test_zero_count;
    bool                  test_n_larger;
    bool                  test_null_handling;
    bool                  test_mixed_within_n;
    size_t                idx;

    // test 1: compare first n chars case-insensitively
    result = d_strncasecmp_n("HELLOABC", 8, "helloXYZ", 8, 5);
    test_n_chars = (result == 0);

    // test 2: ignores beyond n
    result = d_strncasecmp_n("TESTabc", 7, "testXYZ", 7, 4);
    test_ignore_beyond_n = (result == 0);

    // test 3: zero count
    result = d_strncasecmp_n("different", 9, "STRINGS", 7, 0);
    test_zero_count = (result == 0);

    // test 4: n larger
    result = d_strncasecmp_n("Short", 5, "SHORT", 5, 100);
    test_n_larger = (result == 0);

    // test 5: NULL handling
    int r1 = d_strncasecmp_n(NULL, 0, "test", 4, 4);
    int r2 = d_strncasecmp_n("test", 4, NULL, 0, 4);
    test_null_handling = (r1 < 0) && (r2 > 0);

    // test 6: mixed case within n
    result = d_strncasecmp_n("AbCdEf", 6, "aBcDeF", 6, 6);
    test_mixed_within_n = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_strncasecmp_n", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("n_chars",
                                           test_n_chars,
                                           "compares first n chars case-insensitively");
    group->elements[idx++] = D_ASSERT_TRUE("ignore_beyond_n",
                                           test_ignore_beyond_n,
                                           "ignores differences beyond n");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "returns 0 for zero count");
    group->elements[idx++] = D_ASSERT_TRUE("n_larger",
                                           test_n_larger,
                                           "handles n larger than strings");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("mixed_within_n",
                                           test_mixed_within_n,
                                           "handles mixed case within n");

    return group;
}

/*
d_tests_string_fn_strequals
  Tests d_strequals for length-aware equality check.
  Tests the following:
  - returns true for identical strings
  - returns false for different strings
  - short-circuits on length mismatch
  - handles NULL inputs
  - handles empty strings
  - returns false for partial match
*/
struct d_test_object*
d_tests_string_fn_strequals
(
    void
)
{
    struct d_test_object* group;
    bool                  result;
    bool                  test_identical;
    bool                  test_different;
    bool                  test_length_mismatch;
    bool                  test_null_handling;
    bool                  test_empty_strings;
    bool                  test_partial_match;
    size_t                idx;

    // test 1: identical
    result = d_strequals("hello", 5, "hello", 5);
    test_identical = (result == true);

    // test 2: different content
    result = d_strequals("hello", 5, "world", 5);
    test_different = (result == false);

    // test 3: different lengths
    result = d_strequals("hello", 5, "helloworld", 10);
    test_length_mismatch = (result == false);

    // test 4: NULL handling
    bool r1 = d_strequals(NULL, 0, NULL, 0);
    bool r2 = d_strequals(NULL, 0, "test", 4);
    bool r3 = d_strequals("test", 4, NULL, 0);
    test_null_handling = (r1 == true) && (r2 == false) && (r3 == false);

    // test 5: empty strings
    result = d_strequals("", 0, "", 0);
    test_empty_strings = (result == true);

    // test 6: partial match (same prefix, different length)
    result = d_strequals("Hello", 5, "Hell", 4);
    test_partial_match = (result == false);

    // build result tree
    group = d_test_object_new_interior("d_strequals", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("identical",
                                           test_identical,
                                           "returns true for identical strings");
    group->elements[idx++] = D_ASSERT_TRUE("different",
                                           test_different,
                                           "returns false for different strings");
    group->elements[idx++] = D_ASSERT_TRUE("length_mismatch",
                                           test_length_mismatch,
                                           "short-circuits on length mismatch");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("empty_strings",
                                           test_empty_strings,
                                           "handles empty strings");
    group->elements[idx++] = D_ASSERT_TRUE("partial_match",
                                           test_partial_match,
                                           "returns false for partial match");

    return group;
}

/*
d_tests_string_fn_strequals_nocase
  Tests d_strequals_nocase for case-insensitive equality.
  Tests the following:
  - returns true for case-different equal strings
  - returns false for different strings
  - short-circuits on length mismatch
  - handles NULL inputs
  - handles mixed alphanumeric
  - handles empty strings
*/
struct d_test_object*
d_tests_string_fn_strequals_nocase
(
    void
)
{
    struct d_test_object* group;
    bool                  result;
    bool                  test_case_diff;
    bool                  test_different;
    bool                  test_length_mismatch;
    bool                  test_null_handling;
    bool                  test_mixed_alnum;
    bool                  test_empty_strings;
    size_t                idx;

    // test 1: case-different but equal
    result = d_strequals_nocase("HeLLo", 5, "hEllO", 5);
    test_case_diff = (result == true);

    // test 2: different content
    result = d_strequals_nocase("HELLO", 5, "WORLD", 5);
    test_different = (result == false);

    // test 3: length mismatch
    result = d_strequals_nocase("HELLO", 5, "HELLOWORLD", 10);
    test_length_mismatch = (result == false);

    // test 4: NULL handling
    bool r1 = d_strequals_nocase(NULL, 0, NULL, 0);
    bool r2 = d_strequals_nocase(NULL, 0, "test", 4);
    test_null_handling = (r1 == true) && (r2 == false);

    // test 5: mixed alphanumeric
    result = d_strequals_nocase("Test123", 7, "TEST123", 7);
    test_mixed_alnum = (result == true);

    // test 6: empty strings
    result = d_strequals_nocase("", 0, "", 0);
    test_empty_strings = (result == true);

    // build result tree
    group = d_test_object_new_interior("d_strequals_nocase", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("case_diff",
                                           test_case_diff,
                                           "returns true for case-different equals");
    group->elements[idx++] = D_ASSERT_TRUE("different",
                                           test_different,
                                           "returns false for different strings");
    group->elements[idx++] = D_ASSERT_TRUE("length_mismatch",
                                           test_length_mismatch,
                                           "short-circuits on length mismatch");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("mixed_alnum",
                                           test_mixed_alnum,
                                           "handles mixed alphanumeric");
    group->elements[idx++] = D_ASSERT_TRUE("empty_strings",
                                           test_empty_strings,
                                           "handles empty strings");

    return group;
}

/*
d_tests_string_fn_length_aware_cmp_all
  Runs all length-aware comparison tests.
  Tests the following:
  - d_strcmp_n
  - d_strncmp_n
  - d_strcasecmp_n
  - d_strncasecmp_n
  - d_strequals
  - d_strequals_nocase
*/
struct d_test_object*
d_tests_string_fn_length_aware_cmp_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Length-Aware Comparison", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strcmp_n();
    group->elements[idx++] = d_tests_string_fn_strncmp_n();
    group->elements[idx++] = d_tests_string_fn_strcasecmp_n();
    group->elements[idx++] = d_tests_string_fn_strncasecmp_n();
    group->elements[idx++] = d_tests_string_fn_strequals();
    group->elements[idx++] = d_tests_string_fn_strequals_nocase();

    return group;
}
