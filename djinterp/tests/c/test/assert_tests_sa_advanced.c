/*******************************************************************************
* djinterp [test]                                    assert_tests_sa_advanced.c
*
*   Advanced tests for d_assert module.
*   Tests: integration scenarios, stress tests
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_advanced.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_integration
  Tests complex assertion chains and integration scenarios.
  Tests the following:
  - pointer assertion works in chain
  - comparison assertion works in chain
  - string assertion works in chain
  - array assertion works in chain
  - all assertions can be combined
*/
struct d_test_object*
d_tests_sa_assert_integration
(
    void
)
{
    struct d_test_object* group;
    int                   values[3];
    int*                  ptrs[3];
    struct d_assert*   assert1;
    struct d_assert*   assert2;
    struct d_assert*   assert3;
    struct d_assert*   assert4;
    bool                  test_ptr_assert;
    bool                  test_cmp_assert;
    bool                  test_str_assert;
    bool                  test_arr_assert;
    bool                  test_combined;
    size_t                idx;

    // setup test data
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    ptrs[0] = &values[0];
    ptrs[1] = &values[1];
    ptrs[2] = &values[2];

    // create assertions
    assert1 = d_assert_nonnull(ptrs[0],
                               "First pointer valid", "First pointer NULL");
    assert2 = d_assert_lt(&values[0], &values[1], d_test_int_comparator,
                          "10 < 20", "10 >= 20");
    assert3 = d_assert_str_eq("test", 4,
                              "test", 4,
                              "Strings match", "Strings differ");
    assert4 = d_assert_arrays_eq(values, values, 3, sizeof(int),
                                 d_test_int_comparator,
                                 "Arrays equal", "Arrays differ");

    // verify assertions
    test_ptr_assert = (assert1 != NULL) && (assert1->result == true);
    test_cmp_assert = (assert2 != NULL) && (assert2->result == true);
    test_str_assert = (assert3 != NULL) && (assert3->result == true);
    test_arr_assert = (assert4 != NULL) && (assert4->result == true);
    test_combined = test_ptr_assert && test_cmp_assert && 
                    test_str_assert && test_arr_assert;

    // cleanup
    d_assert_free(assert1);
    d_assert_free(assert2);
    d_assert_free(assert3);
    d_assert_free(assert4);

    // build result tree
    group = d_test_object_new_interior("Integration Scenarios", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("pointer_assertion",
                                           test_ptr_assert,
                                           "pointer assertion works in chain");
    group->elements[idx++] = D_ASSERT_TRUE("comparison_assertion",
                                           test_cmp_assert,
                                           "comparison assertion works in chain");
    group->elements[idx++] = D_ASSERT_TRUE("string_assertion",
                                           test_str_assert,
                                           "string assertion works in chain");
    group->elements[idx++] = D_ASSERT_TRUE("array_assertion",
                                           test_arr_assert,
                                           "array assertion works in chain");
    group->elements[idx++] = D_ASSERT_TRUE("combined_assertions",
                                           test_combined,
                                           "all assertions can be combined");

    return group;
}

/*
d_tests_sa_assert_stress
  Tests assertion system under stress conditions.
  Tests the following:
  - creates many assertions successfully
  - all assertions have correct results
  - cleanup works for many assertions
*/
struct d_test_object*
d_tests_sa_assert_stress
(
    void
)
{
    struct d_test_object*  group;
    struct d_assert**   assertions;
    int*                   val1;
    int*                   val2;
    int                    valid_count;
    int                    i;
    bool                   test_creation;
    bool                   test_all_valid;
    bool                   test_cleanup;
    size_t                 idx;

    assertions = NULL;
    test_creation = false;
    test_all_valid = false;
    test_cleanup = false;

    assertions = (struct d_assert**)malloc(
        D_TEST_ASSERT_STRESS_COUNT * sizeof(struct d_assert*));

    if (assertions)
    {
        // create many assertions
        for (i = 0; i < D_TEST_ASSERT_STRESS_COUNT; i++)
        {
            val1 = d_test_create_int_standalone(i);
            val2 = d_test_create_int_standalone(i);

            if (val1 && val2)
            {
                assertions[i] = d_assert_eq(val1, val2, d_test_int_comparator,
                                            "Values equal", "Values not equal");
            }
            else
            {
                assertions[i] = NULL;
            }

            free(val1);
            free(val2);
        }

        // test that first and last were created
        test_creation = (assertions[0] != NULL) && 
                        (assertions[D_TEST_ASSERT_STRESS_COUNT - 1] != NULL);

        // count valid assertions
        valid_count = 0;
        for (i = 0; i < D_TEST_ASSERT_STRESS_COUNT; i++)
        {
            if (assertions[i] && assertions[i]->result == true)
            {
                valid_count++;
            }
        }
        test_all_valid = (valid_count == D_TEST_ASSERT_STRESS_COUNT);

        // cleanup all assertions
        for (i = 0; i < D_TEST_ASSERT_STRESS_COUNT; i++)
        {
            d_assert_free(assertions[i]);
        }
        test_cleanup = true;  // if we got here without crash

        free(assertions);
    }

    // build result tree
    group = d_test_object_new_interior("Stress Tests", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("creation",
                                           test_creation,
                                           "creates many assertions successfully");
    group->elements[idx++] = D_ASSERT_TRUE("all_valid",
                                           test_all_valid,
                                           "all assertions have correct results");
    group->elements[idx++] = D_ASSERT_TRUE("cleanup",
                                           test_cleanup,
                                           "cleanup works for many assertions");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_advanced_all
  Runs all advanced tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_advanced_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Advanced Tests", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_integration();
    group->elements[idx++] = d_tests_sa_assert_stress();

    return group;
}