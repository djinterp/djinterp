/*******************************************************************************
* djinterp [test]                                        assert_tests_sa_core.c
*
*   Core function tests for d_assert module.
*   Tests: d_assert_new, d_assert_free, d_assert_true, d_assert_false
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_core.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_new
  Tests d_assert_new function for creating assertion objects.
  Tests the following:
  - creates assertion with true result
  - creates assertion with false result
  - stores message correctly
  - result field matches input
  - message field matches input
*/
struct d_test_object*
d_tests_sa_assert_new
(
    void
)
{
    struct d_test_object* group;
    struct d_assert*      assert_true;
    struct d_assert*      assert_false;
    bool                  test_true_creation;
    bool                  test_true_result;
    bool                  test_true_message;
    bool                  test_false_creation;
    bool                  test_false_result;
    bool                  test_false_message;
    size_t                idx;

    // test 1: create assertion with true result
    assert_true = d_assert_new(true, "Pass message", "Fail message");
    test_true_creation = (assert_true != NULL);

    // test 2: verify true result stored correctly
    test_true_result = test_true_creation && 
                       (assert_true->result == true);

    // test 3: verify message stored correctly (should be pass message)
    test_true_message = test_true_creation && 
                        (assert_true->message != NULL) &&
                        (strcmp(assert_true->message, "Pass message") == 0);

    // test 4: create assertion with false result
    assert_false = d_assert_new(false, "Pass message", "Fail message");
    test_false_creation = (assert_false != NULL);

    // test 5: verify false result stored correctly
    test_false_result = test_false_creation && 
                        (assert_false->result == false);

    // test 6: verify message stored correctly (should be fail message)
    test_false_message = test_false_creation && 
                         (assert_false->message != NULL) &&
                         (strcmp(assert_false->message, "Fail message") == 0);

    // cleanup
    d_assert_free(assert_true);
    d_assert_free(assert_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_new", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("true_creation",
                                           test_true_creation,
                                           "creates assertion with true result");
    group->elements[idx++] = D_ASSERT_TRUE("true_result",
                                           test_true_result,
                                           "stores true result correctly");
    group->elements[idx++] = D_ASSERT_TRUE("true_message",
                                           test_true_message,
                                           "stores pass message for true result");
    group->elements[idx++] = D_ASSERT_TRUE("false_creation",
                                           test_false_creation,
                                           "creates assertion with false result");
    group->elements[idx++] = D_ASSERT_TRUE("false_result",
                                           test_false_result,
                                           "stores false result correctly");
    group->elements[idx++] = D_ASSERT_TRUE("false_message",
                                           test_false_message,
                                           "stores fail message for false result");

    return group;
}

/*
d_tests_sa_assert_free
  Tests d_assert_free function for destroying assertion objects.
  Tests the following:
  - frees NULL without crash
  - frees valid assertion without crash
*/
struct d_test_object*
d_tests_sa_assert_free
(
    void
)
{
    struct d_test_object* group;
    struct d_assert*      assertion;
    bool                  test_free_null;
    bool                  test_free_valid;
    size_t                idx;

    // test 1: free NULL assertion (should not crash)
    d_assert_free(NULL);
    test_free_null = true;  // if we got here, it didn't crash

    // test 2: free valid assertion
    assertion = d_assert_new(true, "Pass", "Fail");
    
    if (assertion)
    {
        d_assert_free(assertion);
        test_free_valid = true;  // if we got here, it didn't crash
    }
    else
    {
        test_free_valid = false;
    }

    // build result tree
    group = d_test_object_new_interior("d_assert_free", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("free_null",
                                           test_free_null,
                                           "frees NULL without crash");
    group->elements[idx++] = D_ASSERT_TRUE("free_valid",
                                           test_free_valid,
                                           "frees valid assertion without crash");

    return group;
}

/*
d_tests_sa_assert_true
  Tests d_assert_true function for creating conditional assertions.
  Tests the following:
  - returns true result for true expression
  - returns false result for false expression
  - uses pass message for true expression
  - uses fail message for false expression
*/
struct d_test_object*
d_tests_sa_assert_true
(
    void
)
{
    struct d_test_object* group;
    struct d_assert*      assert_pass;
    struct d_assert*      assert_fail;
    bool                  test_true_expr_result;
    bool                  test_true_expr_message;
    bool                  test_false_expr_result;
    bool                  test_false_expr_message;
    size_t                idx;

    // test 1-2: assertion with true expression
    assert_pass = d_assert_true(true, "Pass message", "Fail message");
    test_true_expr_result = (assert_pass != NULL) && 
                            (assert_pass->result == true);
    test_true_expr_message = (assert_pass != NULL) && 
                             (assert_pass->message != NULL) &&
                             (strcmp(assert_pass->message, "Pass message") == 0);

    // test 3-4: assertion with false expression
    assert_fail = d_assert_true(false, "Pass message", "Fail message");
    test_false_expr_result = (assert_fail != NULL) && 
                             (assert_fail->result == false);
    test_false_expr_message = (assert_fail != NULL) && 
                              (assert_fail->message != NULL) &&
                              (strcmp(assert_fail->message, "Fail message") == 0);

    // cleanup
    d_assert_free(assert_pass);
    d_assert_free(assert_fail);

    // build result tree
    group = d_test_object_new_interior("d_assert_true", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("true_expr_result",
                                           test_true_expr_result,
                                           "returns true for true expression");
    group->elements[idx++] = D_ASSERT_TRUE("true_expr_message",
                                           test_true_expr_message,
                                           "uses pass message for true expression");
    group->elements[idx++] = D_ASSERT_TRUE("false_expr_result",
                                           test_false_expr_result,
                                           "returns false for false expression");
    group->elements[idx++] = D_ASSERT_TRUE("false_expr_message",
                                           test_false_expr_message,
                                           "uses fail message for false expression");

    return group;
}

/*
d_tests_sa_assert_false
  Tests d_assert_false function for creating negated assertions.
  Tests the following:
  - returns true result for false expression (negated)
  - returns false result for true expression (negated)
  - uses pass message for false expression
  - uses fail message for true expression
*/
struct d_test_object*
d_tests_sa_assert_false
(
    void
)
{
    struct d_test_object* group;
    struct d_assert*      assert_pass;
    struct d_assert*      assert_fail;
    bool                  test_false_expr_result;
    bool                  test_false_expr_message;
    bool                  test_true_expr_result;
    bool                  test_true_expr_message;
    size_t                idx;

    // test 1-2: false expression should pass (result = true)
    assert_pass = d_assert_false(false, "Pass message", "Fail message");
    test_false_expr_result = (assert_pass != NULL) && 
                             (assert_pass->result == true);
    test_false_expr_message = (assert_pass != NULL) && 
                              (assert_pass->message != NULL) &&
                              (strcmp(assert_pass->message, "Pass message") == 0);

    // test 3-4: true expression should fail (result = false)
    assert_fail = d_assert_false(true, "Pass message", "Fail message");
    test_true_expr_result = (assert_fail != NULL) && 
                            (assert_fail->result == false);
    test_true_expr_message = (assert_fail != NULL) && 
                             (assert_fail->message != NULL) &&
                             (strcmp(assert_fail->message, "Fail message") == 0);

    // cleanup
    d_assert_free(assert_pass);
    d_assert_free(assert_fail);

    // build result tree
    group = d_test_object_new_interior("d_assert_false", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("false_expr_result",
                                           test_false_expr_result,
                                           "returns true for false expression");
    group->elements[idx++] = D_ASSERT_TRUE("false_expr_message",
                                           test_false_expr_message,
                                           "uses pass message for false expression");
    group->elements[idx++] = D_ASSERT_TRUE("true_expr_result",
                                           test_true_expr_result,
                                           "returns false for true expression");
    group->elements[idx++] = D_ASSERT_TRUE("true_expr_message",
                                           test_true_expr_message,
                                           "uses fail message for true expression");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_core_all
  Runs all core function tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_core_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Core Functions", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_new();
    group->elements[idx++] = d_tests_sa_assert_free();
    group->elements[idx++] = d_tests_sa_assert_true();
    group->elements[idx++] = d_tests_sa_assert_false();

    return group;
}