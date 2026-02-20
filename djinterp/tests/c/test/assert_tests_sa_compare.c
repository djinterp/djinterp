/*******************************************************************************
* djinterp [test]                                     assert_tests_sa_compare.c
*
*   Comparison function tests for d_assert module.
*   Tests: d_assert_eq, d_assert_neq, d_assert_lt, d_assert_lt_eq,
*          d_assert_gt, d_assert_gt_eq
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_compare.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_eq
  Tests d_assert_eq function for equality comparison.
  Tests the following:
  - correctly identifies equal values
  - correctly identifies unequal values
  - compares pointers when comparator is NULL
*/
struct d_test_object*
d_tests_sa_assert_eq
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    struct d_assert*   eq_true;
    struct d_assert*   eq_false;
    struct d_assert*   eq_ptr;
    bool                  test_equal_values;
    bool                  test_unequal_values;
    bool                  test_null_comparator;
    size_t                idx;

    val1 = 10;
    val2 = 10;
    val3 = 20;

    // test 1: equality with custom comparator
    eq_true = d_assert_eq(&val1, &val2, d_test_int_comparator,
                          "Equal", "Not equal");
    test_equal_values = (eq_true != NULL) && (eq_true->result == true);

    // test 2: inequality with custom comparator
    eq_false = d_assert_eq(&val1, &val3, d_test_int_comparator,
                           "Equal", "Not equal");
    test_unequal_values = (eq_false != NULL) && (eq_false->result == false);

    // test 3: NULL comparator (pointer comparison)
    eq_ptr = d_assert_eq(&val1, &val1, NULL,
                         "Same pointer", "Different pointer");
    test_null_comparator = (eq_ptr != NULL) && (eq_ptr->result == true);

    // cleanup
    d_assert_free(eq_true);
    d_assert_free(eq_false);
    d_assert_free(eq_ptr);

    // build result tree
    group = d_test_object_new_interior("d_assert_eq", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("equal_values",
                                           test_equal_values,
                                           "correctly identifies equal values");
    group->elements[idx++] = D_ASSERT_TRUE("unequal_values",
                                           test_unequal_values,
                                           "correctly identifies unequal values");
    group->elements[idx++] = D_ASSERT_TRUE("null_comparator",
                                           test_null_comparator,
                                           "compares pointers when comparator is NULL");

    return group;
}

/*
d_tests_sa_assert_neq
  Tests d_assert_neq function for inequality comparison.
  Tests the following:
  - correctly identifies unequal values
  - correctly identifies equal values
*/
struct d_test_object*
d_tests_sa_assert_neq
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    struct d_assert*   neq_true;
    struct d_assert*   neq_false;
    bool                  test_unequal_values;
    bool                  test_equal_values;
    size_t                idx;

    val1 = 15;
    val2 = 25;
    val3 = 15;

    // test 1: inequality
    neq_true = d_assert_neq(&val1, &val2, d_test_int_comparator,
                            "Not equal", "Equal");
    test_unequal_values = (neq_true != NULL) && (neq_true->result == true);

    // test 2: equality (should fail)
    neq_false = d_assert_neq(&val1, &val3, d_test_int_comparator,
                             "Not equal", "Equal");
    test_equal_values = (neq_false != NULL) && (neq_false->result == false);

    // cleanup
    d_assert_free(neq_true);
    d_assert_free(neq_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_neq", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("unequal_values",
                                           test_unequal_values,
                                           "correctly identifies unequal values");
    group->elements[idx++] = D_ASSERT_TRUE("equal_values",
                                           test_equal_values,
                                           "correctly identifies equal values");

    return group;
}

/*
d_tests_sa_assert_lt
  Tests d_assert_lt function for less-than comparison.
  Tests the following:
  - correctly identifies less than
  - correctly identifies not less than
*/
struct d_test_object*
d_tests_sa_assert_lt
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    struct d_assert*   lt_true;
    struct d_assert*   lt_false;
    bool                  test_less_than;
    bool                  test_not_less_than;
    size_t                idx;

    val1 = 5;
    val2 = 10;
    val3 = 3;

    // test 1: less than
    lt_true = d_assert_lt(&val1, &val2, d_test_int_comparator,
                          "Less than", "Not less than");
    test_less_than = (lt_true != NULL) && (lt_true->result == true);

    // test 2: not less than
    lt_false = d_assert_lt(&val2, &val3, d_test_int_comparator,
                           "Less than", "Not less than");
    test_not_less_than = (lt_false != NULL) && (lt_false->result == false);

    // cleanup
    d_assert_free(lt_true);
    d_assert_free(lt_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_lt", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("less_than",
                                           test_less_than,
                                           "correctly identifies less than");
    group->elements[idx++] = D_ASSERT_TRUE("not_less_than",
                                           test_not_less_than,
                                           "correctly identifies not less than");

    return group;
}

/*
d_tests_sa_assert_lt_eq
  Tests d_assert_lt_eq function for less-than-or-equal comparison.
  Tests the following:
  - correctly identifies equal values
  - correctly identifies less than
  - correctly identifies greater than
*/
struct d_test_object*
d_tests_sa_assert_lt_eq
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    int                   val4;
    struct d_assert*   lte_equal;
    struct d_assert*   lte_less;
    struct d_assert*   lte_false;
    bool                  test_equal;
    bool                  test_less;
    bool                  test_greater;
    size_t                idx;

    val1 = 7;
    val2 = 7;
    val3 = 12;
    val4 = 4;

    // test 1: equal values
    lte_equal = d_assert_lt_eq(&val1, &val2, d_test_int_comparator,
                               "Less or equal", "Greater");
    test_equal = (lte_equal != NULL) && (lte_equal->result == true);

    // test 2: less than
    lte_less = d_assert_lt_eq(&val4, &val1, d_test_int_comparator,
                              "Less or equal", "Greater");
    test_less = (lte_less != NULL) && (lte_less->result == true);

    // test 3: greater than (should fail)
    lte_false = d_assert_lt_eq(&val3, &val1, d_test_int_comparator,
                               "Less or equal", "Greater");
    test_greater = (lte_false != NULL) && (lte_false->result == false);

    // cleanup
    d_assert_free(lte_equal);
    d_assert_free(lte_less);
    d_assert_free(lte_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_lt_eq", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("equal_values",
                                           test_equal,
                                           "correctly identifies equal values");
    group->elements[idx++] = D_ASSERT_TRUE("less_than",
                                           test_less,
                                           "correctly identifies less than");
    group->elements[idx++] = D_ASSERT_TRUE("greater_than",
                                           test_greater,
                                           "correctly identifies greater than");

    return group;
}

/*
d_tests_sa_assert_gt
  Tests d_assert_gt function for greater-than comparison.
  Tests the following:
  - correctly identifies greater than
  - correctly identifies not greater than
*/
struct d_test_object*
d_tests_sa_assert_gt
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    struct d_assert*   gt_true;
    struct d_assert*   gt_false;
    bool                  test_greater_than;
    bool                  test_not_greater_than;
    size_t                idx;

    val1 = 15;
    val2 = 8;
    val3 = 20;

    // test 1: greater than
    gt_true = d_assert_gt(&val1, &val2, d_test_int_comparator,
                          "Greater than", "Not greater than");
    test_greater_than = (gt_true != NULL) && (gt_true->result == true);

    // test 2: not greater than
    gt_false = d_assert_gt(&val2, &val3, d_test_int_comparator,
                           "Greater than", "Not greater than");
    test_not_greater_than = (gt_false != NULL) && (gt_false->result == false);

    // cleanup
    d_assert_free(gt_true);
    d_assert_free(gt_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_gt", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("greater_than",
                                           test_greater_than,
                                           "correctly identifies greater than");
    group->elements[idx++] = D_ASSERT_TRUE("not_greater_than",
                                           test_not_greater_than,
                                           "correctly identifies not greater than");

    return group;
}

/*
d_tests_sa_assert_gt_eq
  Tests d_assert_gt_eq function for greater-than-or-equal comparison.
  Tests the following:
  - correctly identifies equal values
  - correctly identifies greater than
  - correctly identifies less than
*/
struct d_test_object*
d_tests_sa_assert_gt_eq
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    int                   val4;
    struct d_assert*   gte_equal;
    struct d_assert*   gte_greater;
    struct d_assert*   gte_false;
    bool                  test_equal;
    bool                  test_greater;
    bool                  test_less;
    size_t                idx;

    val1 = 9;
    val2 = 9;
    val3 = 15;
    val4 = 3;

    // test 1: equal values
    gte_equal = d_assert_gt_eq(&val1, &val2, d_test_int_comparator,
                               "Greater or equal", "Less");
    test_equal = (gte_equal != NULL) && (gte_equal->result == true);

    // test 2: greater than
    gte_greater = d_assert_gt_eq(&val3, &val1, d_test_int_comparator,
                                 "Greater or equal", "Less");
    test_greater = (gte_greater != NULL) && (gte_greater->result == true);

    // test 3: less than (should fail)
    gte_false = d_assert_gt_eq(&val4, &val1, d_test_int_comparator,
                               "Greater or equal", "Less");
    test_less = (gte_false != NULL) && (gte_false->result == false);

    // cleanup
    d_assert_free(gte_equal);
    d_assert_free(gte_greater);
    d_assert_free(gte_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_gt_eq", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("equal_values",
                                           test_equal,
                                           "correctly identifies equal values");
    group->elements[idx++] = D_ASSERT_TRUE("greater_than",
                                           test_greater,
                                           "correctly identifies greater than");
    group->elements[idx++] = D_ASSERT_TRUE("less_than",
                                           test_less,
                                           "correctly identifies less than");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_compare_all
  Runs all comparison function tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_compare_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Comparison Functions", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_eq();
    group->elements[idx++] = d_tests_sa_assert_neq();
    group->elements[idx++] = d_tests_sa_assert_lt();
    group->elements[idx++] = d_tests_sa_assert_lt_eq();
    group->elements[idx++] = d_tests_sa_assert_gt();
    group->elements[idx++] = d_tests_sa_assert_gt_eq();

    return group;
}