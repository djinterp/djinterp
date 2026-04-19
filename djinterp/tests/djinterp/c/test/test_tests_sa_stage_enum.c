#include "./test_tests_sa.h"


/*
d_tests_sa_test_stage_enum_values
  Tests that all DTestStage enum values are unique.
  Tests the following:
  - all six DTestStage values are pairwise unique
  - D_TEST_STAGE_SETUP == 0
  - D_TEST_STAGE_TEAR_DOWN == 1
  - D_TEST_STAGE_ON_SUCCESS == 2
  - D_TEST_STAGE_ON_FAILURE == 3
  - D_TEST_STAGE_BEFORE == 4
  - D_TEST_STAGE_AFTER == 5
*/
bool
d_tests_sa_test_stage_enum_values
(
    struct d_test_counter* _counter
)
{
    int  values[6];
    int  i;
    int  j;
    bool result;

    result = true;

    values[0] = (int)D_TEST_STAGE_SETUP;
    values[1] = (int)D_TEST_STAGE_TEAR_DOWN;
    values[2] = (int)D_TEST_STAGE_ON_SUCCESS;
    values[3] = (int)D_TEST_STAGE_ON_FAILURE;
    values[4] = (int)D_TEST_STAGE_BEFORE;
    values[5] = (int)D_TEST_STAGE_AFTER;

    // verify expected values
    result = d_assert_standalone(values[0] == 0,
                                 "d_test_stage_enum_values",
                                 "D_TEST_STAGE_SETUP == 0",
                                 _counter) && result;

    result = d_assert_standalone(values[1] == 1,
                                 "d_test_stage_enum_values",
                                 "D_TEST_STAGE_TEAR_DOWN == 1",
                                 _counter) && result;

    result = d_assert_standalone(values[2] == 2,
                                 "d_test_stage_enum_values",
                                 "D_TEST_STAGE_ON_SUCCESS == 2",
                                 _counter) && result;

    result = d_assert_standalone(values[3] == 3,
                                 "d_test_stage_enum_values",
                                 "D_TEST_STAGE_ON_FAILURE == 3",
                                 _counter) && result;

    result = d_assert_standalone(values[4] == 4,
                                 "d_test_stage_enum_values",
                                 "D_TEST_STAGE_BEFORE == 4",
                                 _counter) && result;

    result = d_assert_standalone(values[5] == 5,
                                 "d_test_stage_enum_values",
                                 "D_TEST_STAGE_AFTER == 5",
                                 _counter) && result;

    // verify all pairs are unique
    for (i = 0; i < 6; i++)
    {
        for (j = i + 1; j < 6; j++)
        {
            result = d_assert_standalone(
                         values[i] != values[j],
                         "d_test_stage_enum_values",
                         "DTestStage values are pairwise unique",
                         _counter) && result;
        }
    }

    return result;
}

/*
d_tests_sa_test_stage_enum_coverage
  Tests that DTestStage enum covers all expected lifecycle stages.
  Tests the following:
  - there are exactly 6 defined stages
  - stages cover the full lifecycle: setup, teardown, success, failure,
    before, after
  - stages are contiguous (0 through 5)
*/
bool
d_tests_sa_test_stage_enum_coverage
(
    struct d_test_counter* _counter
)
{
    int  min_val;
    int  max_val;
    int  range;
    bool result;

    result = true;

    // compute range
    min_val = (int)D_TEST_STAGE_SETUP;
    max_val = (int)D_TEST_STAGE_AFTER;
    range   = max_val - min_val + 1;

    // verify there are exactly 6 stages
    result = d_assert_standalone(range == 6,
                                 "d_test_stage_enum_coverage",
                                 "DTestStage range covers exactly 6 values",
                                 _counter) && result;

    // verify min is 0
    result = d_assert_standalone(min_val == 0,
                                 "d_test_stage_enum_coverage",
                                 "minimum stage value is 0",
                                 _counter) && result;

    // verify max is 5
    result = d_assert_standalone(max_val == 5,
                                 "d_test_stage_enum_coverage",
                                 "maximum stage value is 5",
                                 _counter) && result;

    // verify contiguity: each stage == previous + 1
    result = d_assert_standalone(
                 (int)D_TEST_STAGE_TEAR_DOWN == (int)D_TEST_STAGE_SETUP + 1,
                 "d_test_stage_enum_coverage",
                 "TEAR_DOWN is SETUP + 1",
                 _counter) && result;

    result = d_assert_standalone(
                 (int)D_TEST_STAGE_ON_SUCCESS == (int)D_TEST_STAGE_TEAR_DOWN + 1,
                 "d_test_stage_enum_coverage",
                 "ON_SUCCESS is TEAR_DOWN + 1",
                 _counter) && result;

    result = d_assert_standalone(
                 (int)D_TEST_STAGE_ON_FAILURE == (int)D_TEST_STAGE_ON_SUCCESS + 1,
                 "d_test_stage_enum_coverage",
                 "ON_FAILURE is ON_SUCCESS + 1",
                 _counter) && result;

    result = d_assert_standalone(
                 (int)D_TEST_STAGE_BEFORE == (int)D_TEST_STAGE_ON_FAILURE + 1,
                 "d_test_stage_enum_coverage",
                 "BEFORE is ON_FAILURE + 1",
                 _counter) && result;

    result = d_assert_standalone(
                 (int)D_TEST_STAGE_AFTER == (int)D_TEST_STAGE_BEFORE + 1,
                 "d_test_stage_enum_coverage",
                 "AFTER is BEFORE + 1",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_test_stage_enum_all
  Aggregation function for all DTestStage enum tests.
*/
bool
d_tests_sa_test_stage_enum_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_stage_enum_values(_counter) && result;
    result = d_tests_sa_test_stage_enum_coverage(_counter) && result;

    return result;
}
