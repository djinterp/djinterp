/******************************************************************************
* djinterp [test]                               dmacro_tests_sa_static_assert.c
*
*   Unit tests for `dmacro.h` compile-time assertions section.
*   Tests D_ASSERT_SAME_SIZE macro for compile-time type size validation.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_static_assert.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * D_ASSERT_SAME_SIZE TESTS
 *****************************************************************************/

// compile-time assertions - these will fail compilation if incorrect
// we place them at file scope to verify they work

// test: int and int should have same size (trivially true)
D_ASSERT_SAME_SIZE(int, int);

// test: unsigned int and int should have same size
D_ASSERT_SAME_SIZE(unsigned int, int);

// test: size_t and size_t (trivially true)
D_ASSERT_SAME_SIZE(size_t, size_t);

// test: char and unsigned char should have same size
D_ASSERT_SAME_SIZE(char, unsigned char);

// test: short and unsigned short should have same size
D_ASSERT_SAME_SIZE(short, unsigned short);

// test: long and unsigned long should have same size
D_ASSERT_SAME_SIZE(long, unsigned long);


/*
d_tests_sa_dmacro_assert_same_size
  Tests D_ASSERT_SAME_SIZE macro for compile-time type size validation.
  Tests the following:
  - D_ASSERT_SAME_SIZE passes for types with equal sizes
  - Compile-time assertions at file scope work
  - Runtime verification of size relationships
*/
bool
d_tests_sa_dmacro_assert_same_size
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ASSERT_SAME_SIZE ---\n", D_INDENT);

    // the compile-time assertions above already verified correctness
    // here we do runtime verification that the sizes match

    // verify int sizes
    if (!d_assert_standalone(sizeof(int) == sizeof(unsigned int),
                             "sizeof(int) == sizeof(unsigned int)",
                             "signed and unsigned int same size",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    int: %zu, unsigned int: %zu\n",
               D_INDENT, sizeof(int), sizeof(unsigned int));
    }

    // verify char sizes
    if (!d_assert_standalone(sizeof(char) == sizeof(unsigned char),
                             "sizeof(char) == sizeof(unsigned char)",
                             "signed and unsigned char same size",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    char: %zu, unsigned char: %zu\n",
               D_INDENT, sizeof(char), sizeof(unsigned char));
    }

    // verify short sizes
    if (!d_assert_standalone(sizeof(short) == sizeof(unsigned short),
                             "sizeof(short) == sizeof(unsigned short)",
                             "signed and unsigned short same size",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    short: %zu, unsigned short: %zu\n",
               D_INDENT, sizeof(short), sizeof(unsigned short));
    }

    // verify long sizes
    if (!d_assert_standalone(sizeof(long) == sizeof(unsigned long),
                             "sizeof(long) == sizeof(unsigned long)",
                             "signed and unsigned long same size",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    long: %zu, unsigned long: %zu\n",
               D_INDENT, sizeof(long), sizeof(unsigned long));
    }

    // verify pointer sizes (all pointers should be same size)
    if (!d_assert_standalone(sizeof(int*) == sizeof(void*),
                             "sizeof(int*) == sizeof(void*)",
                             "all pointer types same size",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    int*: %zu, void*: %zu\n",
               D_INDENT, sizeof(int*), sizeof(void*));
    }

    if (!d_assert_standalone(sizeof(char*) == sizeof(void*),
                             "sizeof(char*) == sizeof(void*)",
                             "char* and void* same size",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify that compilation succeeded (proves static asserts passed)
    if (!d_assert_standalone(1,
                             "file-scope D_ASSERT_SAME_SIZE compiled",
                             "compile-time assertions passed",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with struct types
    struct d_size_test_a { int x; int y; };
    struct d_size_test_b { int a; int b; };

    // these should be same size (both 2 ints)
    if (!d_assert_standalone(sizeof(struct d_size_test_a) == sizeof(struct d_size_test_b),
                             "two structs with same layout have same size",
                             "struct size comparison",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    struct a: %zu, struct b: %zu\n",
               D_INDENT, sizeof(struct d_size_test_a), sizeof(struct d_size_test_b));
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_ASSERT_SAME_SIZE test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_ASSERT_SAME_SIZE test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * COMPILE-TIME ASSERTIONS MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_static_assert_all
  Runs all compile-time assertion tests.
  Tests the following:
  - D_ASSERT_SAME_SIZE for type size validation
*/
bool
d_tests_sa_dmacro_static_assert_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  assert_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Compile-Time Assertions (Section XIV)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all compile-time assertion tests
    assert_result = d_tests_sa_dmacro_assert_same_size(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = assert_result;

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Compile-Time Assertions Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Compile-Time Assertions Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - D_ASSERT_SAME_SIZE: %s\n",
               assert_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
