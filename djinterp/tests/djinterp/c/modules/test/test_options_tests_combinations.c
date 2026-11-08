/*******************************************************************************
* djinterp [test]                              test_options_tests_combinations.c
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_combinations.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_options_tests.h"


/******************************************************************************
 * IV. MESSAGE FLAG COMBINATION TESTS
 *****************************************************************************/

/*
d_tests_options_msg_count_combinations
  Tests the count-related combination macros.
  Tests the following:
  - D_TEST_MSG_COUNT_FAIL_ALL combines all counter fail flags
  - D_TEST_MSG_COUNT_PASS_ALL combines all counter pass flags
  - D_TEST_MSG_COUNT_ALL combines both fail and pass
  - Fail and pass count flags are disjoint
*/
bool d_tests_options_msg_count_combinations(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint32_t count_fail;
    uint32_t count_pass;

    d_test_suite_test(_suite, "config_msg_count_combinations");

    result = true;

    count_fail = D_TEST_MSG_COUNT_FAIL_ALL;
    count_pass = D_TEST_MSG_COUNT_PASS_ALL;

    // test 1: COUNT_FAIL_ALL has all 4 fail counter bits
    result = D_TEST_CHECK_MSG(_suite,
                              "count_fail_has_asserts",
                              (count_fail & D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL) != 0,
                              "COUNT_FAIL_ALL should include asserts fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_fail_has_tests",
                              (count_fail & D_TEST_MSG_FLAG_COUNT_TESTS_FAIL) != 0,
                              "COUNT_FAIL_ALL should include tests fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_fail_has_blocks",
                              (count_fail & D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL) != 0,
                              "COUNT_FAIL_ALL should include blocks fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_fail_has_modules",
                              (count_fail & D_TEST_MSG_FLAG_COUNT_MODULES_FAIL) != 0,
                              "COUNT_FAIL_ALL should include modules fail") && result;

    // test 2: COUNT_PASS_ALL has all 4 pass counter bits
    result = D_TEST_CHECK_MSG(_suite,
                              "count_pass_has_asserts",
                              (count_pass & D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS) != 0,
                              "COUNT_PASS_ALL should include asserts pass") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "count_pass_has_modules",
                              (count_pass & D_TEST_MSG_FLAG_COUNT_MODULES_PASS) != 0,
                              "COUNT_PASS_ALL should include modules pass") && result;

    // test 3: COUNT_ALL = fail | pass
    result = D_TEST_CHECK_MSG(_suite,
                              "count_all_is_fail_or_pass",
                              D_TEST_MSG_COUNT_ALL == (count_fail | count_pass),
                              "COUNT_ALL should equal COUNT_FAIL_ALL | COUNT_PASS_ALL") && result;

    // test 4: fail and pass counter flags are disjoint
    result = D_TEST_CHECK_MSG(_suite,
                              "count_fail_pass_disjoint",
                              (count_fail & count_pass) == 0,
                              "Fail and pass counter flags should be disjoint") && result;

    // test 5: COUNT_ALL equals 0xFF (all counter bits)
    result = D_TEST_CHECK_MSG(_suite,
                              "count_all_value",
                              D_TEST_MSG_COUNT_ALL == 0xFFu,
                              "COUNT_ALL should be 0xFF") && result;

    return result;
}

/*
d_tests_options_msg_print_combinations
  Tests the print-related combination macros.
  Tests the following:
  - D_TEST_MSG_PRINT_FAIL_ALL combines all print fail flags
  - D_TEST_MSG_PRINT_PASS_ALL combines all print pass flags
  - D_TEST_MSG_PRINT_ALL combines both fail and pass
  - Print flags reside in bits 8-15
*/
bool d_tests_options_msg_print_combinations(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint32_t print_fail;
    uint32_t print_pass;

    d_test_suite_test(_suite, "config_msg_print_combinations");

    result = true;

    print_fail = D_TEST_MSG_PRINT_FAIL_ALL;
    print_pass = D_TEST_MSG_PRINT_PASS_ALL;

    // test 1: PRINT_FAIL_ALL has all 4 fail print bits
    result = D_TEST_CHECK_MSG(_suite,
                              "print_fail_has_asserts",
                              (print_fail & D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL) != 0,
                              "PRINT_FAIL_ALL should include asserts fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "print_fail_has_modules",
                              (print_fail & D_TEST_MSG_FLAG_PRINT_MODULES_FAIL) != 0,
                              "PRINT_FAIL_ALL should include modules fail") && result;

    // test 2: PRINT_PASS_ALL has all 4 pass print bits
    result = D_TEST_CHECK_MSG(_suite,
                              "print_pass_has_asserts",
                              (print_pass & D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS) != 0,
                              "PRINT_PASS_ALL should include asserts pass") && result;

    // test 3: PRINT_ALL = fail | pass
    result = D_TEST_CHECK_MSG(_suite,
                              "print_all_is_fail_or_pass",
                              D_TEST_MSG_PRINT_ALL == (print_fail | print_pass),
                              "PRINT_ALL should equal PRINT_FAIL_ALL | PRINT_PASS_ALL") && result;

    // test 4: fail and pass print flags are disjoint
    result = D_TEST_CHECK_MSG(_suite,
                              "print_fail_pass_disjoint",
                              (print_fail & print_pass) == 0,
                              "Fail and pass print flags should be disjoint") && result;

    // test 5: PRINT_ALL equals 0xFF00 (all print bits)
    result = D_TEST_CHECK_MSG(_suite,
                              "print_all_value",
                              D_TEST_MSG_PRINT_ALL == 0xFF00u,
                              "PRINT_ALL should be 0xFF00") && result;

    return result;
}

/*
d_tests_options_msg_all
  Tests the D_TEST_MSG_ALL combination.
  Tests the following:
  - MSG_ALL equals COUNT_ALL | PRINT_ALL
  - MSG_ALL equals 0xFFFF (all message bits)
  - MSG_ALL equals the message mask
*/
bool d_tests_options_msg_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_msg_all");

    result = true;

    // test 1: MSG_ALL = COUNT_ALL | PRINT_ALL
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_all_is_count_or_print",
                              D_TEST_MSG_ALL == (D_TEST_MSG_COUNT_ALL | D_TEST_MSG_PRINT_ALL),
                              "MSG_ALL should equal COUNT_ALL | PRINT_ALL") && result;

    // test 2: MSG_ALL is 0xFFFF
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_all_value",
                              D_TEST_MSG_ALL == 0xFFFFu,
                              "MSG_ALL should be 0xFFFF") && result;

    // test 3: MSG_ALL equals message mask
    result = D_TEST_CHECK_MSG(_suite,
                              "msg_all_equals_mask",
                              D_TEST_MSG_ALL == D_TEST_MASK_MESSAGE_FLAGS,
                              "MSG_ALL should equal D_TEST_MASK_MESSAGE_FLAGS") && result;

    return result;
}

/*
d_tests_options_msg_category_combinations
  Tests the category-specific message combinations.
  Tests the following:
  - Each *_FAIL_ONLY combines count + print for that category
  - Each *_PASS_ONLY combines count + print for that category
  - Each *_ALL combines fail + pass for that category
*/
bool d_tests_options_msg_category_combinations(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "config_msg_category_combinations");

    result = true;

    // asserts
    result = D_TEST_CHECK_MSG(_suite,
                              "asserts_fail_only",
                              D_TEST_MSG_ASSERTS_FAIL_ONLY == (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL | D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL),
                              "ASSERTS_FAIL_ONLY should combine count + print fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "asserts_pass_only",
                              D_TEST_MSG_ASSERTS_PASS_ONLY == (D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS | D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS),
                              "ASSERTS_PASS_ONLY should combine count + print pass") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "asserts_all",
                              D_TEST_MSG_ASSERTS_ALL == (D_TEST_MSG_ASSERTS_FAIL_ONLY | D_TEST_MSG_ASSERTS_PASS_ONLY),
                              "ASSERTS_ALL should combine fail + pass") && result;

    // tests
    result = D_TEST_CHECK_MSG(_suite,
                              "tests_fail_only",
                              D_TEST_MSG_TESTS_FAIL_ONLY == (D_TEST_MSG_FLAG_COUNT_TESTS_FAIL | D_TEST_MSG_FLAG_PRINT_TESTS_FAIL),
                              "TESTS_FAIL_ONLY should combine count + print fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "tests_all",
                              D_TEST_MSG_TESTS_ALL == (D_TEST_MSG_TESTS_FAIL_ONLY | D_TEST_MSG_TESTS_PASS_ONLY),
                              "TESTS_ALL should combine fail + pass") && result;

    // blocks
    result = D_TEST_CHECK_MSG(_suite,
                              "blocks_fail_only",
                              D_TEST_MSG_BLOCKS_FAIL_ONLY == (D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL | D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL),
                              "BLOCKS_FAIL_ONLY should combine count + print fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "blocks_all",
                              D_TEST_MSG_BLOCKS_ALL == (D_TEST_MSG_BLOCKS_FAIL_ONLY | D_TEST_MSG_BLOCKS_PASS_ONLY),
                              "BLOCKS_ALL should combine fail + pass") && result;

    // modules
    result = D_TEST_CHECK_MSG(_suite,
                              "modules_fail_only",
                              D_TEST_MSG_MODULES_FAIL_ONLY == (D_TEST_MSG_FLAG_COUNT_MODULES_FAIL | D_TEST_MSG_FLAG_PRINT_MODULES_FAIL),
                              "MODULES_FAIL_ONLY should combine count + print fail") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "modules_all",
                              D_TEST_MSG_MODULES_ALL == (D_TEST_MSG_MODULES_FAIL_ONLY | D_TEST_MSG_MODULES_PASS_ONLY),
                              "MODULES_ALL should combine fail + pass") && result;

    return result;
}

/*
d_tests_options_combination_all
  Aggregation function that runs all message flag combination tests.
*/
bool d_tests_options_combination_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "config_combination_all");

    result = true;

    result = d_tests_options_msg_count_combinations(_suite) && result;
    result = d_tests_options_msg_print_combinations(_suite) && result;
    result = d_tests_options_msg_all(_suite) && result;
    result = d_tests_options_msg_category_combinations(_suite) && result;

    return result;
}
