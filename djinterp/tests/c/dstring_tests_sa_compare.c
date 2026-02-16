/******************************************************************************
* djinterp [test]                                       dstring_tests_compare.c
*
*   Unit tests for d_string comparison functions:
*     - d_string_cmp
*     - d_string_cmp_cstr
*     - d_string_ncmp
*     - d_string_ncmp_cstr
*     - d_string_casecmp
*     - d_string_casecmp_cstr
*     - d_string_ncasecmp
*     - d_string_ncasecmp_cstr
*     - d_string_equals
*     - d_string_equals_cstr
*     - d_string_equals_ignore_case
*     - d_string_equals_cstr_ignore_case
*
* path:      \src\test\dstring_tests_compare.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "..\..\tests\dstring_tests_sa.h"


/******************************************************************************
* d_tests_sa_dstring_cmp
******************************************************************************/

/*
d_tests_sa_dstring_cmp
  Tests d_string_cmp() which compares two d_strings lexicographically.

Test cases:
  1.  Both NULL returns 0
  2.  First NULL, second non-NULL returns negative
  3.  First non-NULL, second NULL returns positive
  4.  Equal strings return 0
  5.  First less than second returns negative
  6.  First greater than second returns positive
  7.  Empty strings equal
  8.  Empty vs non-empty
  9.  Prefix comparison (shorter < longer when prefix matches)
  10. Case sensitive comparison

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_cmp
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str1;
    struct d_string*      str2;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_cmp", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: both NULL returns 0
    result = d_string_cmp(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "both_null_returns_0",
        result, 0,
        "comparing two NULL strings should return 0"
    );

    // test 2: first NULL returns negative
    str2 = d_string_new_from_cstr("test");

    if (str2)
    {
        result = d_string_cmp(NULL, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "first_null_negative",
            result < 0,
            "NULL < non-NULL should return negative"
        );

        d_string_free(str2);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("fn_skip", false, "skip");
    }

    // test 3: second NULL returns positive
    str1 = d_string_new_from_cstr("test");

    if (str1)
    {
        result = d_string_cmp(str1, NULL);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "second_null_positive",
            result > 0,
            "non-NULL > NULL should return positive"
        );

        d_string_free(str1);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("sn_skip", false, "skip");
    }

    // test 4: equal strings return 0
    str1 = d_string_new_from_cstr("Hello");
    str2 = d_string_new_from_cstr("Hello");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "equal_strings_return_0",
            result, 0,
            "equal strings should return 0"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("eq_skip", false, "skip");
    }

    // test 5: first less than second
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Banana");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "apple_less_than_banana",
            result < 0,
            "'Apple' < 'Banana' should return negative"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("lt_skip", false, "skip");
    }

    // test 6: first greater than second
    str1 = d_string_new_from_cstr("Zebra");
    str2 = d_string_new_from_cstr("Apple");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "zebra_greater_than_apple",
            result > 0,
            "'Zebra' > 'Apple' should return positive"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("gt_skip", false, "skip");
    }

    // test 7: empty strings equal
    str1 = d_string_new();
    str2 = d_string_new();

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_strings_equal",
            result, 0,
            "two empty strings should be equal"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("emp_skip", false, "skip");
    }

    // test 8: empty vs non-empty
    str1 = d_string_new();
    str2 = d_string_new_from_cstr("A");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_less_than_nonempty",
            result < 0,
            "empty string < non-empty string"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("evn_skip", false, "skip");
    }

    // test 9: prefix comparison
    str1 = d_string_new_from_cstr("Hello");
    str2 = d_string_new_from_cstr("HelloWorld");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "prefix_less_than_full",
            result < 0,
            "'Hello' < 'HelloWorld' (prefix is less)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("pfx_skip", false, "skip");
    }

    // test 10: case sensitive - uppercase < lowercase in ASCII
    str1 = d_string_new_from_cstr("ABC");
    str2 = d_string_new_from_cstr("abc");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "case_sensitive_upper_less",
            result < 0,
            "'ABC' < 'abc' (uppercase has lower ASCII values)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("cs_skip", false, "skip");
    }

    // test 11: same string compared to itself
    str1 = d_string_new_from_cstr("SelfCompare");

    if (str1)
    {
        result = d_string_cmp(str1, str1);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "self_compare_equals_0",
            result, 0,
            "string compared to itself should return 0"
        );

        d_string_free(str1);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("self_skip", false, "skip");
    }

    // test 12: difference in middle
    str1 = d_string_new_from_cstr("ABCDE");
    str2 = d_string_new_from_cstr("ABXDE");

    if (str1 && str2)
    {
        result = d_string_cmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "middle_difference",
            result < 0,
            "'ABCDE' < 'ABXDE' (C < X)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("mid_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_cmp_cstr
******************************************************************************/

/*
d_tests_sa_dstring_cmp_cstr
  Tests d_string_cmp_cstr() which compares a d_string with a C string.

Test cases:
  1. Both NULL returns 0
  2. d_string NULL returns negative
  3. C string NULL returns positive
  4. Equal strings
  5. d_string less than C string
  6. d_string greater than C string
  7. Empty d_string vs empty C string

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_cmp_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_cmp_cstr", 9);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: both NULL returns 0
    result = d_string_cmp_cstr(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "both_null_returns_0",
        result, 0,
        "comparing NULL d_string with NULL C string returns 0"
    );

    // test 2: d_string NULL returns negative
    result = d_string_cmp_cstr(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "dstring_null_negative",
        result < 0,
        "NULL d_string < non-NULL C string"
    );

    // test 3: C string NULL returns positive
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_cmp_cstr(str, NULL);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "cstr_null_positive",
            result > 0,
            "non-NULL d_string > NULL C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cn_skip", false, "skip");
    }

    // test 4: equal strings
    str = d_string_new_from_cstr("Match");

    if (str)
    {
        result = d_string_cmp_cstr(str, "Match");

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "equal_returns_0",
            result, 0,
            "equal d_string and C string should return 0"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("eq_skip", false, "skip");
    }

    // test 5: d_string less than C string
    str = d_string_new_from_cstr("AAA");

    if (str)
    {
        result = d_string_cmp_cstr(str, "BBB");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "dstring_less_than_cstr",
            result < 0,
            "'AAA' < 'BBB'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("lt_skip", false, "skip");
    }

    // test 6: d_string greater than C string
    str = d_string_new_from_cstr("ZZZ");

    if (str)
    {
        result = d_string_cmp_cstr(str, "AAA");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "dstring_greater_than_cstr",
            result > 0,
            "'ZZZ' > 'AAA'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("gt_skip", false, "skip");
    }

    // test 7: empty vs empty
    str = d_string_new();

    if (str)
    {
        result = d_string_cmp_cstr(str, "");

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_vs_empty",
            result, 0,
            "empty d_string equals empty C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("emp_skip", false, "skip");
    }

    // test 8: case sensitive
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_cmp_cstr(str, "Hello");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "case_sensitive",
            result > 0,
            "'hello' > 'Hello' (lowercase > uppercase in ASCII)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cs_skip", false, "skip");
    }

    // test 9: prefix
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        result = d_string_cmp_cstr(str, "Testing");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "prefix_less",
            result < 0,
            "'Test' < 'Testing'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pfx_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncmp
******************************************************************************/

/*
d_tests_sa_dstring_ncmp
  Tests d_string_ncmp() which compares up to n characters of two d_strings.

Test cases:
  1. n = 0 always returns 0
  2. Compare first character only
  3. Compare partial match
  4. Full compare when n > length
  5. Strings differ after n characters (should be equal)
  6. Strings differ within n characters

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncmp
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str1;
    struct d_string*      str2;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_ncmp", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: n = 0 always returns 0
    str1 = d_string_new_from_cstr("ABC");
    str2 = d_string_new_from_cstr("XYZ");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_0_returns_0",
            result, 0,
            "comparing 0 characters should always return 0"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
    }

    // test 2: compare first character only
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Apricot");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 1);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "first_char_equal",
            result, 0,
            "first character 'A' == 'A'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("fc_skip", false, "skip");
    }

    // test 3: compare partial match (first 2 chars)
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Apricot");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "first_two_chars_equal",
            result, 0,
            "first two characters 'Ap' == 'Ap'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("f2_skip", false, "skip");
    }

    // test 4: compare 3 chars - should differ
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Apricot");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 3);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "third_char_differs",
            result != 0,
            "'App' != 'Apr' (p < r)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("f3_skip", false, "skip");
    }

    // test 5: full compare when n > length
    str1 = d_string_new_from_cstr("Hi");
    str2 = d_string_new_from_cstr("Hi");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 100);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_greater_than_length",
            result, 0,
            "comparing with n > length should work correctly"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("ngl_skip", false, "skip");
    }

    // test 6: strings differ after n characters
    str1 = d_string_new_from_cstr("TestABC");
    str2 = d_string_new_from_cstr("TestXYZ");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 4);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "differ_after_n",
            result, 0,
            "'Test' == 'Test' (first 4 chars)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("dan_skip", false, "skip");
    }

    // test 7: NULL handling
    result = d_string_ncmp(NULL, NULL, 5);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "both_null",
        result, 0,
        "comparing NULL strings returns 0"
    );

    // test 8: different lengths, equal prefix
    str1 = d_string_new_from_cstr("AB");
    str2 = d_string_new_from_cstr("ABCDEF");

    if (str1 && str2)
    {
        result = d_string_ncmp(str1, str2, 2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "equal_prefix_n",
            result, 0,
            "'AB' == 'AB' (first 2 chars)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("epn_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncmp_cstr
******************************************************************************/

/*
d_tests_sa_dstring_ncmp_cstr
  Tests d_string_ncmp_cstr() which compares up to n characters of a d_string
  with a C string.

Test cases:
  1. n = 0 always returns 0
  2. Partial comparison equal
  3. Partial comparison different
  4. n > string length

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncmp_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_ncmp_cstr", 6);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: n = 0 always returns 0
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        result = d_string_ncmp_cstr(str, "XYZ", 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_0_returns_0",
            result, 0,
            "comparing 0 characters returns 0"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
    }

    // test 2: partial comparison equal
    str = d_string_new_from_cstr("HelloWorld");

    if (str)
    {
        result = d_string_ncmp_cstr(str, "HelloThere", 5);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "partial_equal",
            result, 0,
            "'Hello' == 'Hello' (first 5 chars)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pe_skip", false, "skip");
    }

    // test 3: partial comparison different
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        result = d_string_ncmp_cstr(str, "AXC", 2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_different",
            result != 0,
            "'AB' != 'AX'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pd_skip", false, "skip");
    }

    // test 4: n > string length
    str = d_string_new_from_cstr("Short");

    if (str)
    {
        result = d_string_ncmp_cstr(str, "Short", 100);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_greater_length",
            result, 0,
            "n > length should still work"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ngl_skip", false, "skip");
    }

    // test 5: NULL d_string
    result = d_string_ncmp_cstr(NULL, "test", 4);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_dstring",
        result < 0,
        "NULL d_string < non-NULL C string"
    );

    // test 6: NULL C string
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_ncmp_cstr(str, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_cstr",
            result > 0,
            "non-NULL d_string > NULL C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nc_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_casecmp
******************************************************************************/

/*
d_tests_sa_dstring_casecmp
  Tests d_string_casecmp() which compares two d_strings case-insensitively.

Test cases:
  1. Both NULL returns 0
  2. Same case equal
  3. Different case equal (case insensitive)
  4. Mixed case equal
  5. Actually different strings
  6. Empty strings

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_casecmp
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str1;
    struct d_string*      str2;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_casecmp", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: both NULL returns 0
    result = d_string_casecmp(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "both_null_returns_0",
        result, 0,
        "comparing two NULL strings returns 0"
    );

    // test 2: same case equal
    str1 = d_string_new_from_cstr("Hello");
    str2 = d_string_new_from_cstr("Hello");

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "same_case_equal",
            result, 0,
            "'Hello' == 'Hello'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("sc_skip", false, "skip");
    }

    // test 3: different case equal (all lowercase vs all uppercase)
    str1 = d_string_new_from_cstr("hello");
    str2 = d_string_new_from_cstr("HELLO");

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "case_insensitive_equal",
            result, 0,
            "'hello' == 'HELLO' (case insensitive)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("ci_skip", false, "skip");
    }

    // test 4: mixed case equal
    str1 = d_string_new_from_cstr("HeLLo WoRLd");
    str2 = d_string_new_from_cstr("hEllO wOrld");

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "mixed_case_equal",
            result, 0,
            "'HeLLo WoRLd' == 'hEllO wOrld' (case insensitive)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("mc_skip", false, "skip");
    }

    // test 5: actually different strings
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Banana");

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "different_strings",
            result < 0,
            "'Apple' < 'Banana' even case insensitive"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("df_skip", false, "skip");
    }

    // test 6: empty strings
    str1 = d_string_new();
    str2 = d_string_new();

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_strings_equal",
            result, 0,
            "empty strings are equal"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("emp_skip", false, "skip");
    }

    // test 7: numbers and special chars unaffected
    str1 = d_string_new_from_cstr("Test123!");
    str2 = d_string_new_from_cstr("TEST123!");

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "numbers_special_equal",
            result, 0,
            "'Test123!' == 'TEST123!' (numbers/special unchanged)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 8: prefix relationship maintained
    str1 = d_string_new_from_cstr("abc");
    str2 = d_string_new_from_cstr("ABCDEF");

    if (str1 && str2)
    {
        result = d_string_casecmp(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "prefix_less",
            result < 0,
            "'abc' < 'ABCDEF' (shorter prefix)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("pf_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_casecmp_cstr
******************************************************************************/

/*
d_tests_sa_dstring_casecmp_cstr
  Tests d_string_casecmp_cstr() which compares a d_string with a C string
  case-insensitively.

Test cases:
  1. Equal case insensitive
  2. Different strings
  3. NULL handling

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_casecmp_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_casecmp_cstr", 5);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: equal case insensitive
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_casecmp_cstr(str, "HELLO WORLD");

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "case_insensitive_equal",
            result, 0,
            "'Hello World' == 'HELLO WORLD' (case insensitive)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ci_skip", false, "skip");
    }

    // test 2: different strings
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        result = d_string_casecmp_cstr(str, "xyz");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "different_strings",
            result < 0,
            "'ABC' < 'xyz' (a < x)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("df_skip", false, "skip");
    }

    // test 3: NULL d_string
    result = d_string_casecmp_cstr(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_dstring",
        result < 0,
        "NULL d_string < non-NULL C string"
    );

    // test 4: NULL C string
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_casecmp_cstr(str, NULL);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_cstr",
            result > 0,
            "non-NULL d_string > NULL C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nc_skip", false, "skip");
    }

    // test 5: both NULL
    result = d_string_casecmp_cstr(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "both_null",
        result, 0,
        "both NULL returns 0"
    );

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncasecmp
******************************************************************************/

/*
d_tests_sa_dstring_ncasecmp
  Tests d_string_ncasecmp() which compares up to n characters of two d_strings
  case-insensitively.

Test cases:
  1. n = 0 returns 0
  2. Partial case insensitive equal
  3. Partial case insensitive different
  4. Full comparison

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncasecmp
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str1;
    struct d_string*      str2;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_ncasecmp", 5);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: n = 0 returns 0
    str1 = d_string_new_from_cstr("ABC");
    str2 = d_string_new_from_cstr("xyz");

    if (str1 && str2)
    {
        result = d_string_ncasecmp(str1, str2, 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_0_returns_0",
            result, 0,
            "comparing 0 chars returns 0"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
    }

    // test 2: partial case insensitive equal
    str1 = d_string_new_from_cstr("HelloWorld");
    str2 = d_string_new_from_cstr("HELLOTEST");

    if (str1 && str2)
    {
        result = d_string_ncasecmp(str1, str2, 5);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "partial_case_insensitive_equal",
            result, 0,
            "'Hello' == 'HELLO' (first 5 chars, case insensitive)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("pci_skip", false, "skip");
    }

    // test 3: partial case insensitive different
    str1 = d_string_new_from_cstr("abc");
    str2 = d_string_new_from_cstr("AXC");

    if (str1 && str2)
    {
        result = d_string_ncasecmp(str1, str2, 2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_different",
            result != 0,
            "'ab' != 'AX' (b != X)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("pd_skip", false, "skip");
    }

    // test 4: full comparison case insensitive
    str1 = d_string_new_from_cstr("Test");
    str2 = d_string_new_from_cstr("TEST");

    if (str1 && str2)
    {
        result = d_string_ncasecmp(str1, str2, 100);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "full_case_insensitive",
            result, 0,
            "'Test' == 'TEST' full comparison"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("fc_skip", false, "skip");
    }

    // test 5: NULL handling
    result = d_string_ncasecmp(NULL, NULL, 5);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "both_null",
        result, 0,
        "both NULL returns 0"
    );

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncasecmp_cstr
******************************************************************************/

/*
d_tests_sa_dstring_ncasecmp_cstr
  Tests d_string_ncasecmp_cstr() which compares up to n characters of a
  d_string with a C string case-insensitively.

Test cases:
  1. Partial case insensitive equal
  2. Partial case insensitive different

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncasecmp_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_ncasecmp_cstr", 4);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: partial case insensitive equal
    str = d_string_new_from_cstr("HelloWorld");

    if (str)
    {
        result = d_string_ncasecmp_cstr(str, "HELLOTEST", 5);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "partial_case_insensitive_equal",
            result, 0,
            "'Hello' == 'HELLO' (first 5 chars)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pci_skip", false, "skip");
    }

    // test 2: partial case insensitive different
    str = d_string_new_from_cstr("abc");

    if (str)
    {
        result = d_string_ncasecmp_cstr(str, "AXC", 3);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_different",
            result != 0,
            "'abc' != 'AXC'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pd_skip", false, "skip");
    }

    // test 3: n = 0 returns 0
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_ncasecmp_cstr(str, "XYZ", 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_0_returns_0",
            result, 0,
            "comparing 0 chars returns 0"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
    }

    // test 4: NULL handling
    result = d_string_ncasecmp_cstr(NULL, "test", 4);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_dstring",
        result < 0,
        "NULL d_string < non-NULL C string"
    );

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_equals
******************************************************************************/

/*
d_tests_sa_dstring_equals
  Tests d_string_equals() which returns true if two d_strings are equal.

Test cases:
  1. Both NULL returns true
  2. One NULL returns false
  3. Equal strings return true
  4. Different strings return false
  5. Empty strings equal
  6. Case sensitive (different case = not equal)

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_equals
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str1;
    struct d_string*      str2;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_equals", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: both NULL returns true
    result = d_string_equals(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "both_null_true",
        result,
        "two NULL strings are considered equal"
    );

    // test 2: first NULL, second non-NULL returns false
    str2 = d_string_new_from_cstr("test");

    if (str2)
    {
        result = d_string_equals(NULL, str2);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "first_null_false",
            result,
            "NULL != non-NULL"
        );

        d_string_free(str2);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("fn_skip", false, "skip");
    }

    // test 3: first non-NULL, second NULL returns false
    str1 = d_string_new_from_cstr("test");

    if (str1)
    {
        result = d_string_equals(str1, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "second_null_false",
            result,
            "non-NULL != NULL"
        );

        d_string_free(str1);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("sn_skip", false, "skip");
    }

    // test 4: equal strings return true
    str1 = d_string_new_from_cstr("Hello World");
    str2 = d_string_new_from_cstr("Hello World");

    if (str1 && str2)
    {
        result = d_string_equals(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "equal_strings_true",
            result,
            "'Hello World' == 'Hello World'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("eq_skip", false, "skip");
    }

    // test 5: different strings return false
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Banana");

    if (str1 && str2)
    {
        result = d_string_equals(str1, str2);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "different_strings_false",
            result,
            "'Apple' != 'Banana'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("df_skip", false, "skip");
    }

    // test 6: empty strings equal
    str1 = d_string_new();
    str2 = d_string_new();

    if (str1 && str2)
    {
        result = d_string_equals(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_strings_equal",
            result,
            "two empty strings are equal"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("emp_skip", false, "skip");
    }

    // test 7: case sensitive - different case not equal
    str1 = d_string_new_from_cstr("Hello");
    str2 = d_string_new_from_cstr("hello");

    if (str1 && str2)
    {
        result = d_string_equals(str1, str2);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "case_sensitive_false",
            result,
            "'Hello' != 'hello' (case sensitive)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("cs_skip", false, "skip");
    }

    // test 8: same object equals itself
    str1 = d_string_new_from_cstr("Self");

    if (str1)
    {
        result = d_string_equals(str1, str1);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "self_equals_true",
            result,
            "string equals itself"
        );

        d_string_free(str1);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("self_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_equals_cstr
******************************************************************************/

/*
d_tests_sa_dstring_equals_cstr
  Tests d_string_equals_cstr() which returns true if a d_string equals a
  C string.

Test cases:
  1. Both NULL returns true
  2. d_string NULL returns false
  3. C string NULL returns false
  4. Equal strings
  5. Different strings
  6. Case sensitive

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_equals_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_equals_cstr", 7);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: both NULL returns true
    result = d_string_equals_cstr(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "both_null_true",
        result,
        "NULL d_string and NULL C string are equal"
    );

    // test 2: d_string NULL returns false
    result = d_string_equals_cstr(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "dstring_null_false",
        result,
        "NULL d_string != non-NULL C string"
    );

    // test 3: C string NULL returns false
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_equals_cstr(str, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "cstr_null_false",
            result,
            "non-NULL d_string != NULL C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cn_skip", false, "skip");
    }

    // test 4: equal strings
    str = d_string_new_from_cstr("Match");

    if (str)
    {
        result = d_string_equals_cstr(str, "Match");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "equal_strings_true",
            result,
            "'Match' == 'Match'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("eq_skip", false, "skip");
    }

    // test 5: different strings
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        result = d_string_equals_cstr(str, "XYZ");

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "different_strings_false",
            result,
            "'ABC' != 'XYZ'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("df_skip", false, "skip");
    }

    // test 6: case sensitive
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        result = d_string_equals_cstr(str, "test");

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "case_sensitive_false",
            result,
            "'Test' != 'test' (case sensitive)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cs_skip", false, "skip");
    }

    // test 7: empty strings
    str = d_string_new();

    if (str)
    {
        result = d_string_equals_cstr(str, "");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_equal",
            result,
            "empty d_string == empty C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("emp_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_equals_ignore_case
******************************************************************************/

/*
d_tests_sa_dstring_equals_ignore_case
  Tests d_string_equals_ignore_case() which returns true if two d_strings
  are equal ignoring case.

Test cases:
  1. Both NULL returns true
  2. Same case equal
  3. Different case equal
  4. Actually different strings
  5. Mixed case equal

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_equals_ignore_case
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str1;
    struct d_string*      str2;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_equals_ignore_case", 6);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: both NULL returns true
    result = d_string_equals_ignore_case(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "both_null_true",
        result,
        "two NULL strings are equal"
    );

    // test 2: same case equal
    str1 = d_string_new_from_cstr("Hello");
    str2 = d_string_new_from_cstr("Hello");

    if (str1 && str2)
    {
        result = d_string_equals_ignore_case(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "same_case_true",
            result,
            "'Hello' == 'Hello'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("sc_skip", false, "skip");
    }

    // test 3: different case equal
    str1 = d_string_new_from_cstr("HELLO");
    str2 = d_string_new_from_cstr("hello");

    if (str1 && str2)
    {
        result = d_string_equals_ignore_case(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "different_case_true",
            result,
            "'HELLO' == 'hello' (ignore case)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("dc_skip", false, "skip");
    }

    // test 4: actually different strings
    str1 = d_string_new_from_cstr("Apple");
    str2 = d_string_new_from_cstr("Banana");

    if (str1 && str2)
    {
        result = d_string_equals_ignore_case(str1, str2);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "different_strings_false",
            result,
            "'Apple' != 'Banana'"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("df_skip", false, "skip");
    }

    // test 5: mixed case equal
    str1 = d_string_new_from_cstr("HeLLo WoRLd");
    str2 = d_string_new_from_cstr("hEllO wOrld");

    if (str1 && str2)
    {
        result = d_string_equals_ignore_case(str1, str2);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "mixed_case_true",
            result,
            "'HeLLo WoRLd' == 'hEllO wOrld' (ignore case)"
        );

        d_string_free(str1);
        d_string_free(str2);
    }
    else
    {
        if (str1) d_string_free(str1);
        if (str2) d_string_free(str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("mc_skip", false, "skip");
    }

    // test 6: one NULL returns false
    str1 = d_string_new_from_cstr("test");

    if (str1)
    {
        result = d_string_equals_ignore_case(str1, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "one_null_false",
            result,
            "non-NULL != NULL"
        );

        d_string_free(str1);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("on_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_equals_cstr_ignore_case
******************************************************************************/

/*
d_tests_sa_dstring_equals_cstr_ignore_case
  Tests d_string_equals_cstr_ignore_case() which returns true if a d_string
  equals a C string ignoring case.

Test cases:
  1. Equal ignore case
  2. Different strings
  3. NULL handling

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_equals_cstr_ignore_case
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_equals_cstr_ignore_case", 5);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: equal ignore case
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_equals_cstr_ignore_case(str, "HELLO WORLD");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "equal_ignore_case_true",
            result,
            "'Hello World' == 'HELLO WORLD' (ignore case)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("eic_skip", false, "skip");
    }

    // test 2: different strings
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        result = d_string_equals_cstr_ignore_case(str, "XYZ");

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "different_strings_false",
            result,
            "'ABC' != 'XYZ'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("df_skip", false, "skip");
    }

    // test 3: both NULL
    result = d_string_equals_cstr_ignore_case(NULL, NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "both_null_true",
        result,
        "both NULL are equal"
    );

    // test 4: d_string NULL
    result = d_string_equals_cstr_ignore_case(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "dstring_null_false",
        result,
        "NULL d_string != non-NULL C string"
    );

    // test 5: C string NULL
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_equals_cstr_ignore_case(str, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "cstr_null_false",
            result,
            "non-NULL d_string != NULL C string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cn_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_compare_all
******************************************************************************/

/*
d_tests_sa_dstring_compare_all
  Runs all comparison tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all comparison test results.
*/
struct d_test_object*
d_tests_sa_dstring_compare_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Comparison Functions", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all comparison tests
    group->elements[child_idx++] = d_tests_sa_dstring_cmp();
    group->elements[child_idx++] = d_tests_sa_dstring_cmp_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_ncmp();
    group->elements[child_idx++] = d_tests_sa_dstring_ncmp_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_casecmp();
    group->elements[child_idx++] = d_tests_sa_dstring_casecmp_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_ncasecmp();
    group->elements[child_idx++] = d_tests_sa_dstring_ncasecmp_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_equals();
    group->elements[child_idx++] = d_tests_sa_dstring_equals_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_equals_ignore_case();
    group->elements[child_idx++] = d_tests_sa_dstring_equals_cstr_ignore_case();

    return group;
}



