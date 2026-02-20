#include ".\test_config_tests_sa.h"


/******************************************************************************
 * IV. MESSAGE FLAG COMBINATION TESTS
 *****************************************************************************/

/*
d_tests_sa_config_msg_count_combinations
  Tests the count-related combination macros.
  Tests the following:
  - D_TEST_MSG_COUNT_FAIL_ALL combines all counter fail flags
  - D_TEST_MSG_COUNT_PASS_ALL combines all counter pass flags
  - D_TEST_MSG_COUNT_ALL combines both fail and pass
  - Fail and pass count flags are disjoint
*/
bool
d_tests_sa_config_msg_count_combinations
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t count_fail;
    uint32_t count_pass;

    result = true;

    count_fail = D_TEST_MSG_COUNT_FAIL_ALL;
    count_pass = D_TEST_MSG_COUNT_PASS_ALL;

    // test 1: COUNT_FAIL_ALL has all 4 fail counter bits
    result = d_assert_standalone(
        (count_fail & D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL) != 0,
        "count_fail_has_asserts",
        "COUNT_FAIL_ALL should include asserts fail",
        _counter) && result;

    result = d_assert_standalone(
        (count_fail & D_TEST_MSG_FLAG_COUNT_TESTS_FAIL) != 0,
        "count_fail_has_tests",
        "COUNT_FAIL_ALL should include tests fail",
        _counter) && result;

    result = d_assert_standalone(
        (count_fail & D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL) != 0,
        "count_fail_has_blocks",
        "COUNT_FAIL_ALL should include blocks fail",
        _counter) && result;

    result = d_assert_standalone(
        (count_fail & D_TEST_MSG_FLAG_COUNT_MODULES_FAIL) != 0,
        "count_fail_has_modules",
        "COUNT_FAIL_ALL should include modules fail",
        _counter) && result;

    // test 2: COUNT_PASS_ALL has all 4 pass counter bits
    result = d_assert_standalone(
        (count_pass & D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS) != 0,
        "count_pass_has_asserts",
        "COUNT_PASS_ALL should include asserts pass",
        _counter) && result;

    result = d_assert_standalone(
        (count_pass & D_TEST_MSG_FLAG_COUNT_MODULES_PASS) != 0,
        "count_pass_has_modules",
        "COUNT_PASS_ALL should include modules pass",
        _counter) && result;

    // test 3: COUNT_ALL = fail | pass
    result = d_assert_standalone(
        D_TEST_MSG_COUNT_ALL == (count_fail | count_pass),
        "count_all_is_fail_or_pass",
        "COUNT_ALL should equal COUNT_FAIL_ALL | COUNT_PASS_ALL",
        _counter) && result;

    // test 4: fail and pass counter flags are disjoint
    result = d_assert_standalone(
        (count_fail & count_pass) == 0,
        "count_fail_pass_disjoint",
        "Fail and pass counter flags should be disjoint",
        _counter) && result;

    // test 5: COUNT_ALL equals 0xFF (all counter bits)
    result = d_assert_standalone(
        D_TEST_MSG_COUNT_ALL == 0xFFu,
        "count_all_value",
        "COUNT_ALL should be 0xFF",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_msg_print_combinations
  Tests the print-related combination macros.
  Tests the following:
  - D_TEST_MSG_PRINT_FAIL_ALL combines all print fail flags
  - D_TEST_MSG_PRINT_PASS_ALL combines all print pass flags
  - D_TEST_MSG_PRINT_ALL combines both fail and pass
  - Print flags reside in bits 8-15
*/
bool
d_tests_sa_config_msg_print_combinations
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t print_fail;
    uint32_t print_pass;

    result = true;

    print_fail = D_TEST_MSG_PRINT_FAIL_ALL;
    print_pass = D_TEST_MSG_PRINT_PASS_ALL;

    // test 1: PRINT_FAIL_ALL has all 4 fail print bits
    result = d_assert_standalone(
        (print_fail & D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL) != 0,
        "print_fail_has_asserts",
        "PRINT_FAIL_ALL should include asserts fail",
        _counter) && result;

    result = d_assert_standalone(
        (print_fail & D_TEST_MSG_FLAG_PRINT_MODULES_FAIL) != 0,
        "print_fail_has_modules",
        "PRINT_FAIL_ALL should include modules fail",
        _counter) && result;

    // test 2: PRINT_PASS_ALL has all 4 pass print bits
    result = d_assert_standalone(
        (print_pass & D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS) != 0,
        "print_pass_has_asserts",
        "PRINT_PASS_ALL should include asserts pass",
        _counter) && result;

    // test 3: PRINT_ALL = fail | pass
    result = d_assert_standalone(
        D_TEST_MSG_PRINT_ALL == (print_fail | print_pass),
        "print_all_is_fail_or_pass",
        "PRINT_ALL should equal PRINT_FAIL_ALL | PRINT_PASS_ALL",
        _counter) && result;

    // test 4: fail and pass print flags are disjoint
    result = d_assert_standalone(
        (print_fail & print_pass) == 0,
        "print_fail_pass_disjoint",
        "Fail and pass print flags should be disjoint",
        _counter) && result;

    // test 5: PRINT_ALL equals 0xFF00 (all print bits)
    result = d_assert_standalone(
        D_TEST_MSG_PRINT_ALL == 0xFF00u,
        "print_all_value",
        "PRINT_ALL should be 0xFF00",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_msg_all
  Tests the D_TEST_MSG_ALL combination.
  Tests the following:
  - MSG_ALL equals COUNT_ALL | PRINT_ALL
  - MSG_ALL equals 0xFFFF (all message bits)
  - MSG_ALL equals the message mask
*/
bool
d_tests_sa_config_msg_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: MSG_ALL = COUNT_ALL | PRINT_ALL
    result = d_assert_standalone(
        D_TEST_MSG_ALL == (D_TEST_MSG_COUNT_ALL | D_TEST_MSG_PRINT_ALL),
        "msg_all_is_count_or_print",
        "MSG_ALL should equal COUNT_ALL | PRINT_ALL",
        _counter) && result;

    // test 2: MSG_ALL is 0xFFFF
    result = d_assert_standalone(
        D_TEST_MSG_ALL == 0xFFFFu,
        "msg_all_value",
        "MSG_ALL should be 0xFFFF",
        _counter) && result;

    // test 3: MSG_ALL equals message mask
    result = d_assert_standalone(
        D_TEST_MSG_ALL == D_TEST_MASK_MESSAGE_FLAGS,
        "msg_all_equals_mask",
        "MSG_ALL should equal D_TEST_MASK_MESSAGE_FLAGS",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_msg_category_combinations
  Tests the category-specific message combinations.
  Tests the following:
  - Each *_FAIL_ONLY combines count + print for that category
  - Each *_PASS_ONLY combines count + print for that category
  - Each *_ALL combines fail + pass for that category
*/
bool
d_tests_sa_config_msg_category_combinations
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // asserts
    result = d_assert_standalone(
        D_TEST_MSG_ASSERTS_FAIL_ONLY ==
            (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
             D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL),
        "asserts_fail_only",
        "ASSERTS_FAIL_ONLY should combine count + print fail",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_ASSERTS_PASS_ONLY ==
            (D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS |
             D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS),
        "asserts_pass_only",
        "ASSERTS_PASS_ONLY should combine count + print pass",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_ASSERTS_ALL ==
            (D_TEST_MSG_ASSERTS_FAIL_ONLY | D_TEST_MSG_ASSERTS_PASS_ONLY),
        "asserts_all",
        "ASSERTS_ALL should combine fail + pass",
        _counter) && result;

    // tests
    result = d_assert_standalone(
        D_TEST_MSG_TESTS_FAIL_ONLY ==
            (D_TEST_MSG_FLAG_COUNT_TESTS_FAIL |
             D_TEST_MSG_FLAG_PRINT_TESTS_FAIL),
        "tests_fail_only",
        "TESTS_FAIL_ONLY should combine count + print fail",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_TESTS_ALL ==
            (D_TEST_MSG_TESTS_FAIL_ONLY | D_TEST_MSG_TESTS_PASS_ONLY),
        "tests_all",
        "TESTS_ALL should combine fail + pass",
        _counter) && result;

    // blocks
    result = d_assert_standalone(
        D_TEST_MSG_BLOCKS_FAIL_ONLY ==
            (D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL |
             D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL),
        "blocks_fail_only",
        "BLOCKS_FAIL_ONLY should combine count + print fail",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_BLOCKS_ALL ==
            (D_TEST_MSG_BLOCKS_FAIL_ONLY | D_TEST_MSG_BLOCKS_PASS_ONLY),
        "blocks_all",
        "BLOCKS_ALL should combine fail + pass",
        _counter) && result;

    // modules
    result = d_assert_standalone(
        D_TEST_MSG_MODULES_FAIL_ONLY ==
            (D_TEST_MSG_FLAG_COUNT_MODULES_FAIL |
             D_TEST_MSG_FLAG_PRINT_MODULES_FAIL),
        "modules_fail_only",
        "MODULES_FAIL_ONLY should combine count + print fail",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_MSG_MODULES_ALL ==
            (D_TEST_MSG_MODULES_FAIL_ONLY | D_TEST_MSG_MODULES_PASS_ONLY),
        "modules_all",
        "MODULES_ALL should combine fail + pass",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_combination_all
  Aggregation function that runs all message flag combination tests.
*/
bool
d_tests_sa_config_combination_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Message Flag Combinations\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_config_msg_count_combinations(_counter) && result;
    result = d_tests_sa_config_msg_print_combinations(_counter) && result;
    result = d_tests_sa_config_msg_all(_counter) && result;
    result = d_tests_sa_config_msg_category_combinations(_counter) && result;

    return result;
}
