/*******************************************************************************
* djinterp [container]                                   array_tests_sa_memory.c
*
* Unit tests for `array.h` memory management functions.
* Part of the array standalone test suite.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa_memory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_array_free
  Tests the `d_array_free` function.
  Tests the following:
  - freeing allocated array
  - handling NULL safely
*/
bool
d_tests_sa_array_free
(
    struct d_test_counter* _test_info
)
{
    bool            test_result = true;
    struct d_array* arr;

    printf("Testing `d_array_free`...\n");

    // test freeing allocated array
    arr = d_array_new(sizeof(int), 10);

    if (arr)
    {
        d_array_free(arr);

        test_result &= d_assert_standalone(
            true,
            "`d_array_free` successfully frees array",
            "`d_array_free` failed",
            _test_info
        );
    }

    // test handling NULL safely
    d_array_free(NULL);

    test_result &= d_assert_standalone(
        true,
        "`d_array_free` handles NULL safely",
        "`d_array_free` crashed on NULL",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_s_free
  Tests the `d_array_s_free` function.
  Tests the following:
  - freeing allocated array
  - handling NULL safely
*/
bool
d_tests_sa_array_s_free
(
    struct d_test_counter* _test_info
)
{
    bool              test_result = true;
    struct d_array_s* arr;

    printf("Testing `d_array_s_free`...\n");

    // test freeing allocated array
    arr = d_array_s_new(sizeof(int), 10);

    if (arr)
    {
        d_array_s_free(arr);

        test_result &= d_assert_standalone(
            true,
            "`d_array_s_free` successfully frees array",
            "`d_array_s_free` failed",
            _test_info
        );
    }

    // test handling NULL safely
    d_array_s_free(NULL);

    test_result &= d_assert_standalone(
        true,
        "`d_array_s_free` handles NULL safely",
        "`d_array_s_free` crashed on NULL",
        _test_info
    );

    _test_info->tests_total++;
    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}


/*
===============================================================================
                        MEMORY TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_sa_array_run_memory_tests
  Runs all memory management tests.
*/
bool
d_tests_sa_array_run_memory_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Memory Management Tests\n");
    printf("----------------------------------------\n");

    overall_result &= d_tests_sa_array_free(_test_info);
    overall_result &= d_tests_sa_array_s_free(_test_info);

    return overall_result;
}
