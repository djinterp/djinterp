/*******************************************************************************
* djinterp [test]                                      dmacro_tests_array_util.c
*
*   Unit tests for `dmacro.h` array utilities section (Section II).
*   Tests compile-time array sizing macros (D_ARRAY_COUNT, D_ARRAY_COUNT_SAFE,
*   D_ARRAY_COUNT_T) and array generation macros (D_MAKE_ARRAY,
*   D_MAKE_STRING_ARRAY).
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_array_util.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * D_ARRAY_COUNT TESTS
 *****************************************************************************/

/*
d_tests_dmacro_array_count_basic
  Tests the D_ARRAY_COUNT macro for static arrays.
  Tests the following:
  - D_ARRAY_COUNT returns correct count for int arrays
  - D_ARRAY_COUNT returns correct count for char arrays
  - D_ARRAY_COUNT returns correct count for struct arrays
  - D_ARRAY_COUNT returns correct count for pointer arrays
  - D_ARRAY_COUNT works with arrays of different sizes (1, 5, 10, 100)
*/
bool d_tests_dmacro_array_count_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    int_arr_1[1]     = { 0 };
    int    int_arr_5[5]     = { 0 };
    int    int_arr_10[10]   = { 0 };
    int    int_arr_100[100] = { 0 };
    char   char_arr[20]     = { '\0' };
    double double_arr[7]    = { 0.0f };
    void*  ptr_arr[15];
    size_t count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_array_count_basic");

    all_assertions_passed = true;

    // test with single element array
    count = D_ARRAY_COUNT(int_arr_1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(int[1]) == 1",
                          count == 1,
                          "single element array should have count 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 5 element array
    count = D_ARRAY_COUNT(int_arr_5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(int[5]) == 5",
                          count == 5,
                          "5 element array should have count 5"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 10 element array
    count = D_ARRAY_COUNT(int_arr_10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(int[10]) == 10",
                          count == 10,
                          "10 element array should have count 10"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 100 element array
    count = D_ARRAY_COUNT(int_arr_100);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(int[100]) == 100",
                          count == 100,
                          "100 element array should have count 100"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with char array
    count = D_ARRAY_COUNT(char_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(char[20]) == 20",
                          count == 20,
                          "char array should have correct count"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with double array
    count = D_ARRAY_COUNT(double_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(double[7]) == 7",
                          count == 7,
                          "double array should have correct count"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with pointer array
    count = D_ARRAY_COUNT(ptr_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(void*[15]) == 15",
                          count == 15,
                          "pointer array should have correct count"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_array_count_initialized
  Tests D_ARRAY_COUNT with initialized arrays.
  Tests the following:
  - D_ARRAY_COUNT works with brace-initialized arrays
  - D_ARRAY_COUNT works with string literal arrays
  - D_ARRAY_COUNT works with compound literal arrays (C99)
  - D_ARRAY_COUNT returns correct count for auto-sized arrays
*/
bool d_tests_dmacro_array_count_initialized(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    initialized[] = {1, 2, 3, 4, 5};
    char   str_arr[]     = "hello";
    int    auto_sized[]  = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    size_t count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_array_count_initialized");

    all_assertions_passed = true;

    // test with brace-initialized array
    count = D_ARRAY_COUNT(initialized);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT({1,2,3,4,5}) == 5",
                          count == 5,
                          "brace-initialized array should have count 5"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with string literal (includes null terminator)
    count = D_ARRAY_COUNT(str_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(\"hello\") == 6",
                          count == 6,
                          "string array includes null terminator"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu (expected 6 for \"hello\" + null)\n", D_INDENT, count);
    }

    // test with auto-sized 10 element array
    count = D_ARRAY_COUNT(auto_sized);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(auto-sized 10 elements) == 10",
                          count == 10,
                          "auto-sized array should have correct count"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // verify values are accessible via index
    if (!D_TEST_CHECK_MSG(_suite,
                          "array[D_ARRAY_COUNT(array) - 1] is last element",
                          initialized[D_ARRAY_COUNT(initialized) - 1] == 5,
                          "can use D_ARRAY_COUNT for indexing"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_array_count_structs
  Tests D_ARRAY_COUNT with struct arrays.
  Tests the following:
  - D_ARRAY_COUNT works with arrays of structs
  - D_ARRAY_COUNT returns element count, not byte size
  - D_ARRAY_COUNT handles structs with padding correctly
*/
bool d_tests_dmacro_array_count_structs(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    size_t count;
    size_t element_size;
    size_t total_size;

    // test struct with different member types (may have padding)
    struct test_struct
    {
        int   a;
        char  b;
        int   c;
    };

    struct test_struct struct_arr[8];

    // small struct
    struct small_struct
    {
        char x;
        char y;
    };

    struct small_struct small_arr[25];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_array_count_structs");

    all_assertions_passed = true;

    // suppress unused variable warnings
    (void)struct_arr;
    (void)small_arr;

    // test with struct array
    count = D_ARRAY_COUNT(struct_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(struct_arr[8]) == 8",
                          count == 8,
                          "struct array should have correct element count"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with small struct array
    count = D_ARRAY_COUNT(small_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT(small_struct[25]) == 25",
                          count == 25,
                          "small struct array should have correct count"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // verify D_ARRAY_COUNT returns count, not byte size
    element_size = sizeof(struct_arr[0]);
    total_size   = sizeof(struct_arr);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT == sizeof(arr) / sizeof(arr[0])",
                          D_ARRAY_COUNT(struct_arr) == total_size / element_size,
                          "D_ARRAY_COUNT formula is correct"))
    {
        all_assertions_passed = false;
        printf("%s    total=%zu, element=%zu, count=%zu\n",
               D_INDENT, total_size, element_size, count);
    }

    // verify count is independent of struct size
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT != sizeof (unless element size is 1)",
                          D_ARRAY_COUNT(struct_arr) != sizeof(struct_arr),
                          "count is not byte size for multi-byte elements"))
    {
        all_assertions_passed = false;
    }

    printf("%s    struct test_struct: size=%zu, arr[8] total=%zu\n",
           D_INDENT,
           sizeof(struct test_struct),
           sizeof(struct_arr));

    return all_assertions_passed;
}


/******************************************************************************
 * D_ARRAY_COUNT_SAFE TESTS
 *****************************************************************************/

/*
d_tests_dmacro_array_count_safe
  Tests the D_ARRAY_COUNT_SAFE macro for variadic element counting.
  Tests the following:
  - D_ARRAY_COUNT_SAFE counts variadic int elements correctly
  - D_ARRAY_COUNT_SAFE counts variadic char elements correctly
  - D_ARRAY_COUNT_SAFE returns 0 for empty (if supported)
  - D_ARRAY_COUNT_SAFE works with type parameter
*/
bool d_tests_dmacro_array_count_safe(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    size_t count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_array_count_safe");

    all_assertions_passed = true;

    // test with single int
    count = D_ARRAY_COUNT_SAFE(int, 42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_SAFE(int, 42) == 1",
                          count == 1,
                          "single element should count as 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 3 ints
    count = D_ARRAY_COUNT_SAFE(int, 1, 2, 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_SAFE(int, 1, 2, 3) == 3",
                          count == 3,
                          "three elements should count as 3"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 5 ints
    count = D_ARRAY_COUNT_SAFE(int, 10, 20, 30, 40, 50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_SAFE(int, 10..50) == 5",
                          count == 5,
                          "five elements should count as 5"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with char type
    count = D_ARRAY_COUNT_SAFE(char, 'a', 'b', 'c', 'd');

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_SAFE(char, 'a'..'d') == 4",
                          count == 4,
                          "four char elements should count as 4"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with double type
    count = D_ARRAY_COUNT_SAFE(double, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_SAFE(double, 1.0..6.0) == 6",
                          count == 6,
                          "six double elements should count as 6"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_ARRAY_COUNT_T TESTS
 *****************************************************************************/

/*
d_tests_dmacro_array_count_t
  Tests the D_ARRAY_COUNT_T macro for typed variadic counting.
  Tests the following:
  - D_ARRAY_COUNT_T counts variadic elements with specified type
  - D_ARRAY_COUNT_T works with int type
  - D_ARRAY_COUNT_T works with pointer types
  - D_ARRAY_COUNT_T counts correctly for larger lists
*/
bool d_tests_dmacro_array_count_t(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    size_t count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_array_count_t");

    all_assertions_passed = true;

    // test with single int
    count = D_ARRAY_COUNT_T(int, 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_T(int, 1) == 1",
                          count == 1,
                          "single int element should count as 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with multiple ints
    count = D_ARRAY_COUNT_T(int, 1, 2, 3, 4, 5, 6, 7, 8);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_T(int, 1..8) == 8",
                          count == 8,
                          "eight int elements should count as 8"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with const char* (string literals)
    count = D_ARRAY_COUNT_T(const char*, "a", "b", "c");

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT_T(const char*, 3 strings) == 3",
                          count == 3,
                          "three string literals should count as 3"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with void* type
    {
        int   x = 1;
        int   y = 2;
        int   z = 3;
        void* dummy;

        count = D_ARRAY_COUNT_T(void*, &x, &y, &z);

        // suppress unused warning
        dummy = &x;
        (void)dummy;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ARRAY_COUNT_T(void*, 3 pointers) == 3",
                              count == 3,
                              "three pointers should count as 3"))
        {
            all_assertions_passed = false;
            printf("%s    Got: %zu\n", D_INDENT, count);
        }
    }

    // test count matches array size using compound literal
    {
        int temp_arr[] = {D_UNPACK(100, 200, 300, 400, 500)};
        count = D_ARRAY_COUNT(temp_arr);

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_UNPACK elements in array == 5",
                              count == 5,
                              "D_ARRAY_COUNT should match D_UNPACK element count"))
        {
            all_assertions_passed = false;
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_MAKE_ARRAY TESTS
 *****************************************************************************/

/*
d_tests_dmacro_make_array
  Tests the D_MAKE_ARRAY macro for generating array declarations.
  Tests the following:
  - D_MAKE_ARRAY creates valid int array
  - D_MAKE_ARRAY initializes values correctly
  - D_MAKE_ARRAY creates correct array size
  - D_MAKE_ARRAY works with different types
*/
bool d_tests_dmacro_make_array(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    size_t count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_make_array");

    all_assertions_passed = true;

    // create int array with D_MAKE_ARRAY
    D_MAKE_ARRAY(int, test_ints, 10, 20, 30, 40, 50);

    // verify count
    count = D_ARRAY_COUNT(test_ints);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MAKE_ARRAY creates array with 5 elements",
                          count == 5,
                          "array should have 5 elements"))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %zu\n", D_INDENT, count);
    }

    // verify values
    if (!D_TEST_CHECK_MSG(_suite,
                          "test_ints[0] == 10",
                          test_ints[0] == 10,
                          "first element should be 10"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "test_ints[2] == 30",
                          test_ints[2] == 30,
                          "third element should be 30"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "test_ints[4] == 50",
                          test_ints[4] == 50,
                          "fifth element should be 50"))
    {
        all_assertions_passed = false;
    }

    // create double array
    D_MAKE_ARRAY(double, test_doubles, 1.5, 2.5, 3.5);

    count = D_ARRAY_COUNT(test_doubles);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MAKE_ARRAY(double) creates 3 element array",
                          count == 3,
                          "double array should have 3 elements"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "test_doubles[1] == 2.5",
                          test_doubles[1] == 2.5,
                          "second double should be 2.5"))
    {
        all_assertions_passed = false;
    }

    // create char array
    D_MAKE_ARRAY(char, test_chars, 'x', 'y', 'z');

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MAKE_ARRAY(char) initializes correctly",
                          test_chars[0] == 'x' && test_chars[1] == 'y' && test_chars[2] == 'z',
                          "char array should have x, y, z"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_MAKE_STRING_ARRAY TESTS
 *****************************************************************************/

/*
d_tests_dmacro_make_string_array
  Tests the D_MAKE_STRING_ARRAY macro for generating string arrays.
  Tests the following:
  - D_MAKE_STRING_ARRAY creates valid string array
  - D_MAKE_STRING_ARRAY stringifies identifiers correctly
  - D_MAKE_STRING_ARRAY creates correct array size
  - strings are null-terminated and comparable
*/
bool d_tests_dmacro_make_string_array(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    size_t count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_make_string_array");

    all_assertions_passed = true;

    // create string array from identifiers
    D_MAKE_STRING_ARRAY(test_strings, foo, bar, baz);

    // verify count
    count = D_ARRAY_COUNT(test_strings);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MAKE_STRING_ARRAY creates 3 element array",
                          count == 3,
                          "should have 3 strings"))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %zu\n", D_INDENT, count);
    }

    // verify strings match expected values
    if (!D_TEST_CHECK_MSG(_suite,
                          "test_strings[0] == \"foo\"",
                          strcmp(test_strings[0], "foo") == 0,
                          "first string should be \"foo\""))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, test_strings[0]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "test_strings[1] == \"bar\"",
                          strcmp(test_strings[1], "bar") == 0,
                          "second string should be \"bar\""))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, test_strings[1]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "test_strings[2] == \"baz\"",
                          strcmp(test_strings[2], "baz") == 0,
                          "third string should be \"baz\""))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, test_strings[2]);
    }

    // test with longer identifier names
    D_MAKE_STRING_ARRAY(long_names,
                        hello_world,
                        test_identifier,
                        another_one,
                        final_entry);

    count = D_ARRAY_COUNT(long_names);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MAKE_STRING_ARRAY with 4 long names",
                          count == 4,
                          "should have 4 strings"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "long_names[0] == \"hello_world\"",
                          strcmp(long_names[0], "hello_world") == 0,
                          "underscore in identifier preserved"))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, long_names[0]);
    }

    // test with numeric-like identifiers
    D_MAKE_STRING_ARRAY(num_names, x1, y2, z3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MAKE_STRING_ARRAY with alphanumeric names",
                          strcmp(num_names[0], "x1") == 0 && strcmp(num_names[1], "y2") == 0 && strcmp(num_names[2], "z3") == 0,
                          "alphanumeric identifiers work"))
    {
        all_assertions_passed = false;
    }

    printf("%s    Created arrays: test_strings, long_names, num_names\n", D_INDENT);

    return all_assertions_passed;
}


/******************************************************************************
 * ARRAY UTILITIES MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_array_all
  Runs all array utilities section tests.
  Tests the following:
  - D_ARRAY_COUNT with various array types
  - D_ARRAY_COUNT_SAFE variadic counting
  - D_ARRAY_COUNT_T typed variadic counting
  - D_MAKE_ARRAY array generation
  - D_MAKE_STRING_ARRAY string array generation
*/
bool d_tests_dmacro_array_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_array_all");

    d_tests_dmacro_array_count_basic(_suite);
    d_tests_dmacro_array_count_initialized(_suite);
    d_tests_dmacro_array_count_structs(_suite);
    d_tests_dmacro_array_count_safe(_suite);
    d_tests_dmacro_array_count_t(_suite);
    d_tests_dmacro_make_array(_suite);
    d_tests_dmacro_make_string_array(_suite);

    return true;
}
