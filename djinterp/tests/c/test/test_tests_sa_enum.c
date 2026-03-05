#include "./test_tests_sa.h"


/*
d_tests_sa_test_type_flag_enum_unique
  Tests that all DTestTypeFlag enum values are unique.
  Tests the following:
  - D_TEST_TYPE_UNKNOWN != D_TEST_TYPE_ASSERT
  - D_TEST_TYPE_ASSERT != D_TEST_TYPE_TEST_FN
  - D_TEST_TYPE_TEST_FN != D_TEST_TYPE_TEST
  - D_TEST_TYPE_TEST != D_TEST_TYPE_TEST_BLOCK
  - D_TEST_TYPE_TEST_BLOCK != D_TEST_TYPE_MODULE
  - no two values are equal across all pairs
*/
bool
d_tests_sa_test_type_flag_enum_unique
(
    struct d_test_counter* _counter
)
{
    int  values[6];
    int  i;
    int  j;
    bool result;

    result = true;

    values[0] = (int)D_TEST_TYPE_UNKNOWN;
    values[1] = (int)D_TEST_TYPE_ASSERT;
    values[2] = (int)D_TEST_TYPE_TEST_FN;
    values[3] = (int)D_TEST_TYPE_TEST;
    values[4] = (int)D_TEST_TYPE_TEST_BLOCK;
    values[5] = (int)D_TEST_TYPE_MODULE;

    // verify all pairs are unique
    for (i = 0; i < 6; i++)
    {
        for (j = i + 1; j < 6; j++)
        {
            result = d_assert_standalone(
                         values[i] != values[j],
                         "d_test_type_flag_enum_unique",
                         "DTestTypeFlag values are pairwise unique",
                         _counter) && result;
        }
    }

    return result;
}


/*
d_tests_sa_test_type_flag_enum_sequential
  Tests that DTestTypeFlag enum values are sequential starting from 0.
  Tests the following:
  - D_TEST_TYPE_UNKNOWN == 0
  - D_TEST_TYPE_ASSERT == 1
  - D_TEST_TYPE_TEST_FN == 2
  - D_TEST_TYPE_TEST == 3
  - D_TEST_TYPE_TEST_BLOCK == 4
  - D_TEST_TYPE_MODULE == 5
*/
bool
d_tests_sa_test_type_flag_enum_sequential
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone((int)D_TEST_TYPE_UNKNOWN == 0,
                                 "d_test_type_flag_enum_sequential",
                                 "D_TEST_TYPE_UNKNOWN == 0",
                                 _counter) && result;

    result = d_assert_standalone((int)D_TEST_TYPE_ASSERT == 1,
                                 "d_test_type_flag_enum_sequential",
                                 "D_TEST_TYPE_ASSERT == 1",
                                 _counter) && result;

    result = d_assert_standalone((int)D_TEST_TYPE_TEST_FN == 2,
                                 "d_test_type_flag_enum_sequential",
                                 "D_TEST_TYPE_TEST_FN == 2",
                                 _counter) && result;

    result = d_assert_standalone((int)D_TEST_TYPE_TEST == 3,
                                 "d_test_type_flag_enum_sequential",
                                 "D_TEST_TYPE_TEST == 3",
                                 _counter) && result;

    result = d_assert_standalone((int)D_TEST_TYPE_TEST_BLOCK == 4,
                                 "d_test_type_flag_enum_sequential",
                                 "D_TEST_TYPE_TEST_BLOCK == 4",
                                 _counter) && result;

    result = d_assert_standalone((int)D_TEST_TYPE_MODULE == 5,
                                 "d_test_type_flag_enum_sequential",
                                 "D_TEST_TYPE_MODULE == 5",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_enum_all
  Aggregation function for all DTestTypeFlag enum tests.
*/
bool
d_tests_sa_test_enum_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_type_flag_enum_unique(_counter) && result;
    result = d_tests_sa_test_type_flag_enum_sequential(_counter) && result;

    return result;
}
