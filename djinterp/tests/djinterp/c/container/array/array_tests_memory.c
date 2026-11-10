/*******************************************************************************
* djinterp [container]                                      array_tests_memory.c
*
* Unit tests for `array.h` memory management functions.
* Part of the array suite, on DTest's C face.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_memory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_free
  Tests the `d_array_free` function.
  Tests the following:
  - freeing allocated array
  - handling NULL safely
*/
bool d_tests_array_free(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;

    d_test_suite_test(_suite, "array_free");

    // test freeing allocated array
    arr = d_array_new(sizeof(int), 10);

    if (arr)
    {
        d_array_free(arr);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_free` successfully frees array",
                                        true,
                                        "`d_array_free` failed");
    }

    // test handling NULL safely
    d_array_free(NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_free` handles NULL safely",
                                    true,
                                    "`d_array_free` crashed on NULL");

    return test_result;
}

/*
d_tests_array_s_free
  Tests the `d_array_s_free` function.
  Tests the following:
  - freeing allocated array
  - handling NULL safely
*/
bool d_tests_array_s_free(
    struct dj_test_suite* _suite
)
{
    bool              test_result = true;
    struct dj_array_s* arr;

    d_test_suite_test(_suite, "array_s_free");

    // test freeing allocated array
    arr = d_array_s_new(sizeof(int), 10);

    if (arr)
    {
        d_array_s_free(arr);

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "`d_array_s_free` successfully frees array",
                                        true,
                                        "`d_array_s_free` failed");
    }

    // test handling NULL safely
    d_array_s_free(NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "`d_array_s_free` handles NULL safely",
                                    true,
                                    "`d_array_s_free` crashed on NULL");

    return test_result;
}


/*
===============================================================================
                        MEMORY TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_array_run_memory_tests
  Runs all memory management tests.
*/
bool d_tests_array_run_memory_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result = true;

    d_test_suite_block(_suite, "array_run_memory_tests");

    printf("\n----------------------------------------\n");
    printf("Running Memory Management Tests\n");

    overall_result &= d_tests_array_free(_suite);
    overall_result &= d_tests_array_s_free(_suite);

    return overall_result;
}
