/*******************************************************************************
* djinterp [test]                                        assert_tests_sa_null.c
*
*   Null checking function tests for d_assert module.
*   Tests: d_assert_null, d_assert_nonnull
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_null.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_null
  Tests d_assert_null function for NULL pointer checking.
  Tests the following:
  - correctly identifies NULL pointer
  - correctly identifies non-NULL pointer
*/
struct d_test_object*
d_tests_sa_assert_null
(
    void
)
{
    struct d_test_object* group;
    int                   value;
    int*                  null_ptr;
    int*                  valid_ptr;
    struct d_assert*   null_true;
    struct d_assert*   null_false;
    bool                  test_null_ptr;
    bool                  test_nonnull_ptr;
    size_t                idx;

    value     = 42;
    null_ptr  = NULL;
    valid_ptr = &value;

    // test 1: NULL pointer
    null_true = d_assert_null(null_ptr,
                              "Pointer is NULL", "Pointer is not NULL");
    test_null_ptr = (null_true != NULL) && (null_true->result == true);

    // test 2: non-NULL pointer
    null_false = d_assert_null(valid_ptr,
                               "Pointer is NULL", "Pointer is not NULL");
    test_nonnull_ptr = (null_false != NULL) && (null_false->result == false);

    // cleanup
    d_assert_free(null_true);
    d_assert_free(null_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_null", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_pointer",
                                           test_null_ptr,
                                           "correctly identifies NULL pointer");
    group->elements[idx++] = D_ASSERT_TRUE("nonnull_pointer",
                                           test_nonnull_ptr,
                                           "correctly identifies non-NULL pointer");

    return group;
}

/*
d_tests_sa_assert_nonnull
  Tests d_assert_nonnull function for non-NULL pointer checking.
  Tests the following:
  - correctly identifies non-NULL pointer
  - correctly identifies NULL pointer
*/
struct d_test_object*
d_tests_sa_assert_nonnull
(
    void
)
{
    struct d_test_object* group;
    int                   value;
    int*                  null_ptr;
    int*                  valid_ptr;
    struct d_assert*   nonnull_true;
    struct d_assert*   nonnull_false;
    bool                  test_nonnull_ptr;
    bool                  test_null_ptr;
    size_t                idx;

    value     = 42;
    null_ptr  = NULL;
    valid_ptr = &value;

    // test 1: non-NULL pointer
    nonnull_true = d_assert_nonnull(valid_ptr,
                                    "Pointer is not NULL", "Pointer is NULL");
    test_nonnull_ptr = (nonnull_true != NULL) && (nonnull_true->result == true);

    // test 2: NULL pointer
    nonnull_false = d_assert_nonnull(null_ptr,
                                     "Pointer is not NULL", "Pointer is NULL");
    test_null_ptr = (nonnull_false != NULL) && (nonnull_false->result == false);

    // cleanup
    d_assert_free(nonnull_true);
    d_assert_free(nonnull_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_nonnull", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("nonnull_pointer",
                                           test_nonnull_ptr,
                                           "correctly identifies non-NULL pointer");
    group->elements[idx++] = D_ASSERT_TRUE("null_pointer",
                                           test_null_ptr,
                                           "correctly identifies NULL pointer");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_null_all
  Runs all null checking function tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_null_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Null Checking Functions", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_null();
    group->elements[idx++] = d_tests_sa_assert_nonnull();

    return group;
}