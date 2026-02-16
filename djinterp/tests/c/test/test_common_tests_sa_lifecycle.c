#include ".\test_common_tests_sa.h"


/******************************************************************************
 * V. LIFECYCLE STAGE TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_test_stage_enum
  Tests the DTestStage enumeration definition.
  Tests the following:
  - DTestStage type exists
  - All expected enum values are defined
  - Enum values can be assigned to variables
  - Enum values can be used in switch statements
*/
bool
d_tests_sa_test_common_test_stage_enum
(
    struct d_test_counter* _counter
)
{
    bool           result;
    enum DTestStage stage;

    result = true;

    // test 1: DTestStage type can hold D_TEST_STAGE_SETUP
    stage  = D_TEST_STAGE_SETUP;
    result = d_assert_standalone(
        stage == D_TEST_STAGE_SETUP,
        "stage_setup_defined",
        "D_TEST_STAGE_SETUP should be defined",
        _counter) && result;

    // test 2: DTestStage type can hold D_TEST_STAGE_TEAR_DOWN
    stage  = D_TEST_STAGE_TEAR_DOWN;
    result = d_assert_standalone(
        stage == D_TEST_STAGE_TEAR_DOWN,
        "stage_tear_down_defined",
        "D_TEST_STAGE_TEAR_DOWN should be defined",
        _counter) && result;

    // test 3: DTestStage type can hold D_TEST_STAGE_ON_SUCCESS
    stage  = D_TEST_STAGE_ON_SUCCESS;
    result = d_assert_standalone(
        stage == D_TEST_STAGE_ON_SUCCESS,
        "stage_on_success_defined",
        "D_TEST_STAGE_ON_SUCCESS should be defined",
        _counter) && result;

    // test 4: DTestStage type can hold D_TEST_STAGE_ON_FAILURE
    stage  = D_TEST_STAGE_ON_FAILURE;
    result = d_assert_standalone(
        stage == D_TEST_STAGE_ON_FAILURE,
        "stage_on_failure_defined",
        "D_TEST_STAGE_ON_FAILURE should be defined",
        _counter) && result;

    // test 5: DTestStage type can hold D_TEST_STAGE_BEFORE
    stage  = D_TEST_STAGE_BEFORE;
    result = d_assert_standalone(
        stage == D_TEST_STAGE_BEFORE,
        "stage_before_defined",
        "D_TEST_STAGE_BEFORE should be defined",
        _counter) && result;

    // test 6: DTestStage type can hold D_TEST_STAGE_AFTER
    stage  = D_TEST_STAGE_AFTER;
    result = d_assert_standalone(
        stage == D_TEST_STAGE_AFTER,
        "stage_after_defined",
        "D_TEST_STAGE_AFTER should be defined",
        _counter) && result;

    // test 7: enum values can be used in switch statement
    {
        bool switch_works;

        switch_works = false;
        stage        = D_TEST_STAGE_SETUP;

        switch (stage)
        {
            case D_TEST_STAGE_SETUP:
                switch_works = true;
                break;
            case D_TEST_STAGE_TEAR_DOWN:
            case D_TEST_STAGE_ON_SUCCESS:
            case D_TEST_STAGE_ON_FAILURE:
            case D_TEST_STAGE_BEFORE:
            case D_TEST_STAGE_AFTER:
                switch_works = false;
                break;
            default:
                switch_works = false;
                break;
        }

        result = d_assert_standalone(
            switch_works,
            "stage_switch_statement",
            "DTestStage should work in switch statement",
            _counter) && result;
    }

    // test 8: enum can be converted to int
    {
        int stage_as_int;

        stage_as_int = (int)D_TEST_STAGE_SETUP;

        result = d_assert_standalone(
            stage_as_int == D_TEST_STAGE_SETUP,
            "stage_to_int",
            "DTestStage should be convertible to int",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_test_stage_values
  Tests the specific values assigned to DTestStage enumeration.
  Tests the following:
  - D_TEST_STAGE_SETUP equals 0
  - D_TEST_STAGE_TEAR_DOWN equals 1
  - D_TEST_STAGE_ON_SUCCESS equals 2
  - D_TEST_STAGE_ON_FAILURE equals 3
  - D_TEST_STAGE_BEFORE equals 4
  - D_TEST_STAGE_AFTER equals 5
  - Values are sequential
  - Values are unique
*/
bool
d_tests_sa_test_common_test_stage_values
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_TEST_STAGE_SETUP equals 0
    result = d_assert_standalone(
        D_TEST_STAGE_SETUP == 0,
        "stage_setup_value",
        "D_TEST_STAGE_SETUP should equal 0",
        _counter) && result;

    // test 2: D_TEST_STAGE_TEAR_DOWN equals 1
    result = d_assert_standalone(
        D_TEST_STAGE_TEAR_DOWN == 1,
        "stage_tear_down_value",
        "D_TEST_STAGE_TEAR_DOWN should equal 1",
        _counter) && result;

    // test 3: D_TEST_STAGE_ON_SUCCESS equals 2
    result = d_assert_standalone(
        D_TEST_STAGE_ON_SUCCESS == 2,
        "stage_on_success_value",
        "D_TEST_STAGE_ON_SUCCESS should equal 2",
        _counter) && result;

    // test 4: D_TEST_STAGE_ON_FAILURE equals 3
    result = d_assert_standalone(
        D_TEST_STAGE_ON_FAILURE == 3,
        "stage_on_failure_value",
        "D_TEST_STAGE_ON_FAILURE should equal 3",
        _counter) && result;

    // test 5: D_TEST_STAGE_BEFORE equals 4
    result = d_assert_standalone(
        D_TEST_STAGE_BEFORE == 4,
        "stage_before_value",
        "D_TEST_STAGE_BEFORE should equal 4",
        _counter) && result;

    // test 6: D_TEST_STAGE_AFTER equals 5
    result = d_assert_standalone(
        D_TEST_STAGE_AFTER == 5,
        "stage_after_value",
        "D_TEST_STAGE_AFTER should equal 5",
        _counter) && result;

    // test 7: values are sequential (no gaps from 0 to 5)
    result = d_assert_standalone(
        (D_TEST_STAGE_TEAR_DOWN == D_TEST_STAGE_SETUP + 1) &&
        (D_TEST_STAGE_ON_SUCCESS == D_TEST_STAGE_TEAR_DOWN + 1) &&
        (D_TEST_STAGE_ON_FAILURE == D_TEST_STAGE_ON_SUCCESS + 1) &&
        (D_TEST_STAGE_BEFORE == D_TEST_STAGE_ON_FAILURE + 1) &&
        (D_TEST_STAGE_AFTER == D_TEST_STAGE_BEFORE + 1),
        "stage_values_sequential",
        "DTestStage values should be sequential",
        _counter) && result;

    // test 8: all values are unique
    result = d_assert_standalone(
        (D_TEST_STAGE_SETUP != D_TEST_STAGE_TEAR_DOWN) &&
        (D_TEST_STAGE_SETUP != D_TEST_STAGE_ON_SUCCESS) &&
        (D_TEST_STAGE_SETUP != D_TEST_STAGE_ON_FAILURE) &&
        (D_TEST_STAGE_SETUP != D_TEST_STAGE_BEFORE) &&
        (D_TEST_STAGE_SETUP != D_TEST_STAGE_AFTER) &&
        (D_TEST_STAGE_TEAR_DOWN != D_TEST_STAGE_ON_SUCCESS) &&
        (D_TEST_STAGE_TEAR_DOWN != D_TEST_STAGE_ON_FAILURE) &&
        (D_TEST_STAGE_TEAR_DOWN != D_TEST_STAGE_BEFORE) &&
        (D_TEST_STAGE_TEAR_DOWN != D_TEST_STAGE_AFTER) &&
        (D_TEST_STAGE_ON_SUCCESS != D_TEST_STAGE_ON_FAILURE) &&
        (D_TEST_STAGE_ON_SUCCESS != D_TEST_STAGE_BEFORE) &&
        (D_TEST_STAGE_ON_SUCCESS != D_TEST_STAGE_AFTER) &&
        (D_TEST_STAGE_ON_FAILURE != D_TEST_STAGE_BEFORE) &&
        (D_TEST_STAGE_ON_FAILURE != D_TEST_STAGE_AFTER) &&
        (D_TEST_STAGE_BEFORE != D_TEST_STAGE_AFTER),
        "stage_values_unique",
        "DTestStage values should all be unique",
        _counter) && result;

    // test 9: values can be used as array indices
    {
        const char* stage_names[6];
        bool        index_ok;

        stage_names[D_TEST_STAGE_SETUP]      = "setup";
        stage_names[D_TEST_STAGE_TEAR_DOWN]  = "tear_down";
        stage_names[D_TEST_STAGE_ON_SUCCESS] = "on_success";
        stage_names[D_TEST_STAGE_ON_FAILURE] = "on_failure";
        stage_names[D_TEST_STAGE_BEFORE]     = "before";
        stage_names[D_TEST_STAGE_AFTER]      = "after";

        index_ok = (d_strcasecmp(stage_names[D_TEST_STAGE_SETUP], "setup") == 0) &&
                   (d_strcasecmp(stage_names[D_TEST_STAGE_AFTER], "after") == 0);

        result = d_assert_standalone(
            index_ok,
            "stage_as_array_index",
            "DTestStage values should be usable as array indices",
            _counter) && result;
    }

    // test 10: there are exactly 6 stage values (0 through 5)
    result = d_assert_standalone(
        D_TEST_STAGE_AFTER == 5,
        "stage_count",
        "There should be exactly 6 stages (0-5)",
        _counter) && result;

    // test 11: enum fits in minimal storage
    result = d_assert_standalone(
        sizeof(enum DTestStage) <= sizeof(int),
        "stage_size",
        "DTestStage should fit in int-sized storage or smaller",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_lifecycle_all
  Aggregation function that runs all lifecycle stage tests.
*/
bool
d_tests_sa_test_common_lifecycle_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Lifecycle Stages\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_test_common_test_stage_enum(_counter) && result;
    result = d_tests_sa_test_common_test_stage_values(_counter) && result;

    return result;
}
