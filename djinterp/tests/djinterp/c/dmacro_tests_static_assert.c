/*******************************************************************************
* djinterp [c]                                      dmacro_tests_static_assert.c
*
*   Unit tests for `dmacro.h` compile-time assertions section.
*   Tests D_ASSERT_SAME_SIZE macro for compile-time type size validation.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_static_assert.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


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
d_tests_dmacro_assert_same_size
  Tests D_ASSERT_SAME_SIZE macro for compile-time type size validation.
  Tests the following:
  - D_ASSERT_SAME_SIZE passes for types with equal sizes
  - Compile-time assertions at file scope work
  - Runtime verification of size relationships
*/
bool d_tests_dmacro_assert_same_size(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_assert_same_size");

    all_assertions_passed = true;

    // the compile-time assertions above already verified correctness
    // here we do runtime verification that the sizes match

    // verify int sizes
    if (!D_TEST_CHECK_MSG(_suite,
                          "sizeof(int) == sizeof(unsigned int)",
                          sizeof(int) == sizeof(unsigned int),
                          "signed and unsigned int same size"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "int: %zu, unsigned int: %zu",
                                sizeof(int), sizeof(unsigned int));
    }

    // verify char sizes
    if (!D_TEST_CHECK_MSG(_suite,
                          "sizeof(char) == sizeof(unsigned char)",
                          sizeof(char) == sizeof(unsigned char),
                          "signed and unsigned char same size"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "char: %zu, unsigned char: %zu",
                                sizeof(char), sizeof(unsigned char));
    }

    // verify short sizes
    if (!D_TEST_CHECK_MSG(_suite,
                          "sizeof(short) == sizeof(unsigned short)",
                          sizeof(short) == sizeof(unsigned short),
                          "signed and unsigned short same size"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "short: %zu, unsigned short: %zu",
                                sizeof(short), sizeof(unsigned short));
    }

    // verify long sizes
    if (!D_TEST_CHECK_MSG(_suite,
                          "sizeof(long) == sizeof(unsigned long)",
                          sizeof(long) == sizeof(unsigned long),
                          "signed and unsigned long same size"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "long: %zu, unsigned long: %zu",
                                sizeof(long), sizeof(unsigned long));
    }

    // verify pointer sizes (all pointers should be same size)
    if (!D_TEST_CHECK_MSG(_suite,
                          "sizeof(int*) == sizeof(void*)",
                          sizeof(int*) == sizeof(void*),
                          "all pointer types same size"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "int*: %zu, void*: %zu",
                                sizeof(int*), sizeof(void*));
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "sizeof(char*) == sizeof(void*)",
                          sizeof(char*) == sizeof(void*),
                          "char* and void* same size"))
    {
        all_assertions_passed = false;
    }

    // verify that compilation succeeded (proves static asserts passed)
    if (!D_TEST_CHECK_MSG(_suite,
                          "file-scope D_ASSERT_SAME_SIZE compiled",
                          1,
                          "compile-time assertions passed"))
    {
        all_assertions_passed = false;
    }

    // test with struct types
    struct dj_size_test_a { int x; int y; };
    struct dj_size_test_b { int a; int b; };

    // these should be same size (both 2 ints)
    if (!D_TEST_CHECK_MSG(_suite,
                          "two structs with same layout have same size",
                          sizeof(struct dj_size_test_a) == sizeof(struct dj_size_test_b),
                          "struct size comparison"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "struct a: %zu, struct b: %zu",
                                sizeof(struct dj_size_test_a),
                                sizeof(struct dj_size_test_b));
    }

    return all_assertions_passed;
}


/******************************************************************************
 * COMPILE-TIME ASSERTIONS MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_static_assert_all
  Runs all compile-time assertion tests.
  Tests the following:
  - D_ASSERT_SAME_SIZE for type size validation
*/
bool d_tests_dmacro_static_assert_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_static_assert_all");

    d_tests_dmacro_assert_same_size(_suite);

    return true;
}
