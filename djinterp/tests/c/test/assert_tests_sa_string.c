/*******************************************************************************
* djinterp [test]                                      assert_tests_sa_string.c
*
*   String function tests for d_assert module.
*   Tests: d_assert_str_eq, d_assert_str_neq
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_string.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_str_eq
  Tests d_assert_str_eq function for string equality comparison.
  Tests the following:
  - correctly identifies equal strings
  - correctly identifies unequal strings
  - handles empty strings
  - handles different length strings
  - handles NULL strings
*/
struct d_test_object*
d_tests_sa_assert_str_eq
(
    void
)
{
    struct d_test_object* group;
    const char*           str1;
    const char*           str2;
    const char*           str3;
    struct d_assert*      eq_same;
    struct d_assert*      eq_diff;
    struct d_assert*      eq_empty;
    struct d_assert*      eq_diff_len;
    struct d_assert*      eq_null_both;
    bool                  test_equal;
    bool                  test_unequal;
    bool                  test_empty;
    bool                  test_diff_len;
    bool                  test_null_both;
    size_t                idx;

    str1 = "hello";
    str2 = "hello";
    str3 = "world";

    // test 1: equal strings
    eq_same = d_assert_str_eq(str1, strlen(str1),
                              str2, strlen(str2),
                              "Strings equal", "Strings not equal");
    test_equal = (eq_same != NULL) && (eq_same->result == true);

    // test 2: unequal strings
    eq_diff = d_assert_str_eq(str1, strlen(str1),
                              str3, strlen(str3),
                              "Strings equal", "Strings not equal");
    test_unequal = (eq_diff != NULL) && (eq_diff->result == false);

    // test 3: empty strings
    eq_empty = d_assert_str_eq("", 0,
                               "", 0,
                               "Empty equal", "Empty not equal");
    test_empty = (eq_empty != NULL) && (eq_empty->result == true);

    // test 4: different length strings (same prefix)
    eq_diff_len = d_assert_str_eq("hello", 5,
                                  "hello world", 11,
                                  "Strings equal", "Strings not equal");
    test_diff_len = (eq_diff_len != NULL) && (eq_diff_len->result == false);

    // test 5: both NULL strings
    eq_null_both = d_assert_str_eq(NULL, 0,
                                   NULL, 0,
                                   "Both NULL equal", "Both NULL not equal");
    test_null_both = (eq_null_both != NULL) && (eq_null_both->result == true);

    // cleanup
    d_assert_free(eq_same);
    d_assert_free(eq_diff);
    d_assert_free(eq_empty);
    d_assert_free(eq_diff_len);
    d_assert_free(eq_null_both);

    // build result tree
    group = d_test_object_new_interior("d_assert_str_eq", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("equal_strings",
                                           test_equal,
                                           "correctly identifies equal strings");
    group->elements[idx++] = D_ASSERT_TRUE("unequal_strings",
                                           test_unequal,
                                           "correctly identifies unequal strings");
    group->elements[idx++] = D_ASSERT_TRUE("empty_strings",
                                           test_empty,
                                           "handles empty strings");
    group->elements[idx++] = D_ASSERT_TRUE("different_lengths",
                                           test_diff_len,
                                           "handles different length strings");
    group->elements[idx++] = D_ASSERT_TRUE("null_both",
                                           test_null_both,
                                           "handles both NULL strings");

    return group;
}

/*
d_tests_sa_assert_str_neq
  Tests d_assert_str_neq function for string inequality comparison.
  Tests the following:
  - returns true for unequal strings
  - returns false for equal strings
  - handles different length strings
*/
struct d_test_object*
d_tests_sa_assert_str_neq
(
    void
)
{
    struct d_test_object* group;
    const char*           str1;
    const char*           str2;
    const char*           str3;
    struct d_assert*      neq_diff;
    struct d_assert*      neq_same;
    struct d_assert*      neq_diff_len;
    bool                  test_unequal;
    bool                  test_equal;
    bool                  test_diff_len;
    size_t                idx;

    str1 = "hello";
    str2 = "hello";
    str3 = "world";

    // test 1: unequal strings (should pass - result true)
    neq_diff = d_assert_str_neq(str1, strlen(str1),
                                str3, strlen(str3),
                                "Strings not equal", "Strings equal");
    test_unequal = (neq_diff != NULL) && (neq_diff->result == true);

    // test 2: equal strings (should fail - result false)
    neq_same = d_assert_str_neq(str1, strlen(str1),
                                str2, strlen(str2),
                                "Strings not equal", "Strings equal");
    test_equal = (neq_same != NULL) && (neq_same->result == false);

    // test 3: different length strings (should pass - result true)
    neq_diff_len = d_assert_str_neq("hello", 5,
                                    "hello world", 11,
                                    "Strings not equal", "Strings equal");
    test_diff_len = (neq_diff_len != NULL) && (neq_diff_len->result == true);

    // cleanup
    d_assert_free(neq_diff);
    d_assert_free(neq_same);
    d_assert_free(neq_diff_len);

    // build result tree
    group = d_test_object_new_interior("d_assert_str_neq", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("unequal_strings",
                                           test_unequal,
                                           "returns true for unequal strings");
    group->elements[idx++] = D_ASSERT_TRUE("equal_strings",
                                           test_equal,
                                           "returns false for equal strings");
    group->elements[idx++] = D_ASSERT_TRUE("different_lengths",
                                           test_diff_len,
                                           "handles different length strings");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_string_all
  Runs all string function tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_string_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("String Functions", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_str_eq();
    group->elements[idx++] = d_tests_sa_assert_str_neq();

    return group;
}