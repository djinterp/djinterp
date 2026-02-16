#include ".\test_common_tests_sa.h"


/******************************************************************************
 * SYMBOL DEFINITION TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_symbols_defined
  Tests that test symbols are properly defined.
  Tests the following:
  - D_TEST_SYMBOL_PASS is defined
  - D_TEST_SYMBOL_FAIL is defined
  - D_TEST_SYMBOL_SUCCESS is defined
  - D_TEST_SYMBOL_INFO is defined
  - D_TEST_SYMBOL_WARNING is defined
*/
struct d_test_object*
d_tests_sa_tc_symbols_defined
(
    void
)
{
    struct d_test_object* group;
    bool                  test_pass_defined;
    bool                  test_fail_defined;
    bool                  test_success_defined;
    bool                  test_info_defined;
    bool                  test_warning_defined;
    size_t                idx;

    // test symbols are defined (non-NULL, non-empty)
    test_pass_defined    = (D_TEST_SYMBOL_PASS != NULL) &&
                           (strlen(D_TEST_SYMBOL_PASS) > 0);
    test_fail_defined    = (D_TEST_SYMBOL_FAIL != NULL) &&
                           (strlen(D_TEST_SYMBOL_FAIL) > 0);
    test_success_defined = (D_TEST_SYMBOL_SUCCESS != NULL) &&
                           (strlen(D_TEST_SYMBOL_SUCCESS) > 0);
    test_info_defined    = (D_TEST_SYMBOL_INFO != NULL) &&
                           (strlen(D_TEST_SYMBOL_INFO) > 0);
    test_warning_defined = (D_TEST_SYMBOL_WARNING != NULL) &&
                           (strlen(D_TEST_SYMBOL_WARNING) > 0);

    // build result tree
    group = d_test_object_new_interior("Symbol Definitions", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("pass_defined",
                                           test_pass_defined,
                                           "D_TEST_SYMBOL_PASS is defined");
    group->elements[idx++] = D_ASSERT_TRUE("fail_defined",
                                           test_fail_defined,
                                           "D_TEST_SYMBOL_FAIL is defined");
    group->elements[idx++] = D_ASSERT_TRUE("success_defined",
                                           test_success_defined,
                                           "D_TEST_SYMBOL_SUCCESS is defined");
    group->elements[idx++] = D_ASSERT_TRUE("info_defined",
                                           test_info_defined,
                                           "D_TEST_SYMBOL_INFO is defined");
    group->elements[idx++] = D_ASSERT_TRUE("warning_defined",
                                           test_warning_defined,
                                           "D_TEST_SYMBOL_WARNING is defined");

    return group;
}


/******************************************************************************
 * D_TEST_PASS/FAIL CONSTANT TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_pass_fail_constants
  Tests D_TEST_PASS and D_TEST_FAIL constants.
  Tests the following:
  - D_TEST_PASS equals D_SUCCESS
  - D_TEST_FAIL equals D_FAILURE
  - D_TEST_PASS and D_TEST_FAIL are different
  - D_TEST_PASS is truthy
  - D_TEST_FAIL is falsy
*/
struct d_test_object*
d_tests_sa_tc_pass_fail_constants
(
    void
)
{
    struct d_test_object* group;
    bool                  test_pass_equals_success;
    bool                  test_fail_equals_failure;
    bool                  test_different;
    bool                  test_pass_truthy;
    bool                  test_fail_falsy;
    size_t                idx;

    // test constant values
    test_pass_equals_success = (D_TEST_PASS == D_SUCCESS);
    test_fail_equals_failure = (D_TEST_FAIL == D_FAILURE);
    test_different           = (D_TEST_PASS != D_TEST_FAIL);
    test_pass_truthy         = (D_TEST_PASS == true);
    test_fail_falsy          = (D_TEST_FAIL == false);

    // build result tree
    group = d_test_object_new_interior("Pass/Fail Constants", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("pass_equals_success",
                                           test_pass_equals_success,
                                           "D_TEST_PASS equals D_SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("fail_equals_failure",
                                           test_fail_equals_failure,
                                           "D_TEST_FAIL equals D_FAILURE");
    group->elements[idx++] = D_ASSERT_TRUE("different",
                                           test_different,
                                           "D_TEST_PASS and D_TEST_FAIL differ");
    group->elements[idx++] = D_ASSERT_TRUE("pass_truthy",
                                           test_pass_truthy,
                                           "D_TEST_PASS is true");
    group->elements[idx++] = D_ASSERT_TRUE("fail_falsy",
                                           test_fail_falsy,
                                           "D_TEST_FAIL is false");

    return group;
}


/******************************************************************************
 * SYMBOL MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_tc_symbols_all
  Runs all symbol and constant tests.
  Tests the following:
  - symbol definitions
  - pass/fail constants
*/
struct d_test_object*
d_tests_sa_tc_symbols_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Symbols and Constants", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_tc_symbols_defined();
    group->elements[idx++] = d_tests_sa_tc_pass_fail_constants();

    return group;
}
