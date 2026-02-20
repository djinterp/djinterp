/*******************************************************************************
* djinterp [test]                                     assert_tests_sa_utility.c
*
*   Utility function tests for d_assert module.
*   Tests: d_assert_default_compare
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_utility.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_default_compare
  Tests d_assert_default_compare function for default pointer comparison.
  Tests the following:
  - returns non-zero for same pointer
  - returns zero for different pointers
  - returns zero when comparing with NULL
*/
struct d_test_object*
d_tests_sa_assert_default_compare
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int*                  ptr1;
    int*                  ptr2;
    int                   result_same;
    int                   result_diff;
    int                   result_null;
    bool                  test_same_ptr;
    bool                  test_diff_ptr;
    bool                  test_null_ptr;
    size_t                idx;

    val1 = 42;
    val2 = 42;
    ptr1 = &val1;
    ptr2 = &val2;

    // test 1: same pointer
    result_same = d_assert_default_compare(ptr1, ptr1);
    test_same_ptr = (result_same != 0);

    // test 2: different pointers
    result_diff = d_assert_default_compare(ptr1, ptr2);
    test_diff_ptr = (result_diff == 0);

    // test 3: comparison with NULL
    result_null = d_assert_default_compare(ptr1, NULL);
    test_null_ptr = (result_null == 0);

    // build result tree
    group = d_test_object_new_interior("d_assert_default_compare", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("same_pointer",
                                           test_same_ptr,
                                           "returns non-zero for same pointer");
    group->elements[idx++] = D_ASSERT_TRUE("different_pointers",
                                           test_diff_ptr,
                                           "returns zero for different pointers");
    group->elements[idx++] = D_ASSERT_TRUE("null_pointer",
                                           test_null_ptr,
                                           "returns zero when comparing with NULL");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_utility_all
  Runs all utility function tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_utility_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Utility Functions", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_default_compare();

    return group;
}