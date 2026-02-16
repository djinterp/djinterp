/******************************************************************************
* djinterp [test]                                  dmacro_tests_sa_array_util.c
*
*   Unit tests for `dmacro.h` array utilities section (Section II).
*   Tests compile-time array sizing macros (D_ARRAY_COUNT, D_ARRAY_COUNT_SAFE,
*   D_ARRAY_COUNT_T) and array generation macros (D_MAKE_ARRAY,
*   D_MAKE_STRING_ARRAY).
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_array_util.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * D_ARRAY_COUNT TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_array_count_basic
  Tests the D_ARRAY_COUNT macro for static arrays.
  Tests the following:
  - D_ARRAY_COUNT returns correct count for int arrays
  - D_ARRAY_COUNT returns correct count for char arrays
  - D_ARRAY_COUNT returns correct count for struct arrays
  - D_ARRAY_COUNT returns correct count for pointer arrays
  - D_ARRAY_COUNT works with arrays of different sizes (1, 5, 10, 100)
*/
bool
d_tests_sa_dmacro_array_count_basic
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    int_arr_1[1]     = { 0 };
    int    int_arr_5[5]     = { 0 };
    int    int_arr_10[10]   = { 0 };
    int    int_arr_100[100] = { 0 };
    char   char_arr[20]     = { '\0' };
    double double_arr[7]    = { 0.0f };
    void*  ptr_arr[15];
    size_t count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ARRAY_COUNT Basic Functionality ---\n", D_INDENT);

    // test with single element array
    count = D_ARRAY_COUNT(int_arr_1);

    if (!d_assert_standalone(count == 1,
                             "D_ARRAY_COUNT(int[1]) == 1",
                             "single element array should have count 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 5 element array
    count = D_ARRAY_COUNT(int_arr_5);

    if (!d_assert_standalone(count == 5,
                             "D_ARRAY_COUNT(int[5]) == 5",
                             "5 element array should have count 5",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 10 element array
    count = D_ARRAY_COUNT(int_arr_10);

    if (!d_assert_standalone(count == 10,
                             "D_ARRAY_COUNT(int[10]) == 10",
                             "10 element array should have count 10",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 100 element array
    count = D_ARRAY_COUNT(int_arr_100);

    if (!d_assert_standalone(count == 100,
                             "D_ARRAY_COUNT(int[100]) == 100",
                             "100 element array should have count 100",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with char array
    count = D_ARRAY_COUNT(char_arr);

    if (!d_assert_standalone(count == 20,
                             "D_ARRAY_COUNT(char[20]) == 20",
                             "char array should have correct count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with double array
    count = D_ARRAY_COUNT(double_arr);

    if (!d_assert_standalone(count == 7,
                             "D_ARRAY_COUNT(double[7]) == 7",
                             "double array should have correct count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with pointer array
    count = D_ARRAY_COUNT(ptr_arr);

    if (!d_assert_standalone(count == 15,
                             "D_ARRAY_COUNT(void*[15]) == 15",
                             "pointer array should have correct count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_ARRAY_COUNT basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_ARRAY_COUNT basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_array_count_initialized
  Tests D_ARRAY_COUNT with initialized arrays.
  Tests the following:
  - D_ARRAY_COUNT works with brace-initialized arrays
  - D_ARRAY_COUNT works with string literal arrays
  - D_ARRAY_COUNT works with compound literal arrays (C99)
  - D_ARRAY_COUNT returns correct count for auto-sized arrays
*/
bool
d_tests_sa_dmacro_array_count_initialized
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    initialized[] = {1, 2, 3, 4, 5};
    char   str_arr[]     = "hello";
    int    auto_sized[]  = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    size_t count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ARRAY_COUNT with Initialized Arrays ---\n", D_INDENT);

    // test with brace-initialized array
    count = D_ARRAY_COUNT(initialized);

    if (!d_assert_standalone(count == 5,
                             "D_ARRAY_COUNT({1,2,3,4,5}) == 5",
                             "brace-initialized array should have count 5",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with string literal (includes null terminator)
    count = D_ARRAY_COUNT(str_arr);

    if (!d_assert_standalone(count == 6,
                             "D_ARRAY_COUNT(\"hello\") == 6",
                             "string array includes null terminator",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu (expected 6 for \"hello\" + null)\n", D_INDENT, count);
    }

    // test with auto-sized 10 element array
    count = D_ARRAY_COUNT(auto_sized);

    if (!d_assert_standalone(count == 10,
                             "D_ARRAY_COUNT(auto-sized 10 elements) == 10",
                             "auto-sized array should have correct count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // verify values are accessible via index
    if (!d_assert_standalone(initialized[D_ARRAY_COUNT(initialized) - 1] == 5,
                             "array[D_ARRAY_COUNT(array) - 1] is last element",
                             "can use D_ARRAY_COUNT for indexing",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_ARRAY_COUNT initialized arrays test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_ARRAY_COUNT initialized arrays test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_array_count_structs
  Tests D_ARRAY_COUNT with struct arrays.
  Tests the following:
  - D_ARRAY_COUNT works with arrays of structs
  - D_ARRAY_COUNT returns element count, not byte size
  - D_ARRAY_COUNT handles structs with padding correctly
*/
bool
d_tests_sa_dmacro_array_count_structs
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
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

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ARRAY_COUNT with Struct Arrays ---\n", D_INDENT);

    // suppress unused variable warnings
    (void)struct_arr;
    (void)small_arr;

    // test with struct array
    count = D_ARRAY_COUNT(struct_arr);

    if (!d_assert_standalone(count == 8,
                             "D_ARRAY_COUNT(struct_arr[8]) == 8",
                             "struct array should have correct element count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with small struct array
    count = D_ARRAY_COUNT(small_arr);

    if (!d_assert_standalone(count == 25,
                             "D_ARRAY_COUNT(small_struct[25]) == 25",
                             "small struct array should have correct count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // verify D_ARRAY_COUNT returns count, not byte size
    element_size = sizeof(struct_arr[0]);
    total_size   = sizeof(struct_arr);

    if (!d_assert_standalone(D_ARRAY_COUNT(struct_arr) == total_size / element_size,
                             "D_ARRAY_COUNT == sizeof(arr) / sizeof(arr[0])",
                             "D_ARRAY_COUNT formula is correct",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    total=%zu, element=%zu, count=%zu\n",
               D_INDENT, total_size, element_size, count);
    }

    // verify count is independent of struct size
    if (!d_assert_standalone(D_ARRAY_COUNT(struct_arr) != sizeof(struct_arr),
                             "D_ARRAY_COUNT != sizeof (unless element size is 1)",
                             "count is not byte size for multi-byte elements",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    struct test_struct: size=%zu, arr[8] total=%zu\n",
           D_INDENT,
           sizeof(struct test_struct),
           sizeof(struct_arr));

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_ARRAY_COUNT struct arrays test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_ARRAY_COUNT struct arrays test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_ARRAY_COUNT_SAFE TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_array_count_safe
  Tests the D_ARRAY_COUNT_SAFE macro for variadic element counting.
  Tests the following:
  - D_ARRAY_COUNT_SAFE counts variadic int elements correctly
  - D_ARRAY_COUNT_SAFE counts variadic char elements correctly
  - D_ARRAY_COUNT_SAFE returns 0 for empty (if supported)
  - D_ARRAY_COUNT_SAFE works with type parameter
*/
bool
d_tests_sa_dmacro_array_count_safe
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    size_t count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ARRAY_COUNT_SAFE ---\n", D_INDENT);

    // test with single int
    count = D_ARRAY_COUNT_SAFE(int, 42);

    if (!d_assert_standalone(count == 1,
                             "D_ARRAY_COUNT_SAFE(int, 42) == 1",
                             "single element should count as 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 3 ints
    count = D_ARRAY_COUNT_SAFE(int, 1, 2, 3);

    if (!d_assert_standalone(count == 3,
                             "D_ARRAY_COUNT_SAFE(int, 1, 2, 3) == 3",
                             "three elements should count as 3",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with 5 ints
    count = D_ARRAY_COUNT_SAFE(int, 10, 20, 30, 40, 50);

    if (!d_assert_standalone(count == 5,
                             "D_ARRAY_COUNT_SAFE(int, 10..50) == 5",
                             "five elements should count as 5",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with char type
    count = D_ARRAY_COUNT_SAFE(char, 'a', 'b', 'c', 'd');

    if (!d_assert_standalone(count == 4,
                             "D_ARRAY_COUNT_SAFE(char, 'a'..'d') == 4",
                             "four char elements should count as 4",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with double type
    count = D_ARRAY_COUNT_SAFE(double, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0);

    if (!d_assert_standalone(count == 6,
                             "D_ARRAY_COUNT_SAFE(double, 1.0..6.0) == 6",
                             "six double elements should count as 6",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_ARRAY_COUNT_SAFE test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_ARRAY_COUNT_SAFE test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_ARRAY_COUNT_T TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_array_count_t
  Tests the D_ARRAY_COUNT_T macro for typed variadic counting.
  Tests the following:
  - D_ARRAY_COUNT_T counts variadic elements with specified type
  - D_ARRAY_COUNT_T works with int type
  - D_ARRAY_COUNT_T works with pointer types
  - D_ARRAY_COUNT_T counts correctly for larger lists
*/
bool
d_tests_sa_dmacro_array_count_t
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    size_t count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ARRAY_COUNT_T ---\n", D_INDENT);

    // test with single int
    count = D_ARRAY_COUNT_T(int, 1);

    if (!d_assert_standalone(count == 1,
                             "D_ARRAY_COUNT_T(int, 1) == 1",
                             "single int element should count as 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with multiple ints
    count = D_ARRAY_COUNT_T(int, 1, 2, 3, 4, 5, 6, 7, 8);

    if (!d_assert_standalone(count == 8,
                             "D_ARRAY_COUNT_T(int, 1..8) == 8",
                             "eight int elements should count as 8",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %zu\n", D_INDENT, count);
    }

    // test with const char* (string literals)
    count = D_ARRAY_COUNT_T(const char*, "a", "b", "c");

    if (!d_assert_standalone(count == 3,
                             "D_ARRAY_COUNT_T(const char*, 3 strings) == 3",
                             "three string literals should count as 3",
                             _test_info))
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

        if (!d_assert_standalone(count == 3,
                                 "D_ARRAY_COUNT_T(void*, 3 pointers) == 3",
                                 "three pointers should count as 3",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    Got: %zu\n", D_INDENT, count);
        }
    }

    // test count matches array size using compound literal
    {
        int temp_arr[] = {D_UNPACK(100, 200, 300, 400, 500)};
        count = D_ARRAY_COUNT(temp_arr);

        if (!d_assert_standalone(count == 5,
                                 "D_UNPACK elements in array == 5",
                                 "D_ARRAY_COUNT should match D_UNPACK element count",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_ARRAY_COUNT_T test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_ARRAY_COUNT_T test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_MAKE_ARRAY TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_make_array
  Tests the D_MAKE_ARRAY macro for generating array declarations.
  Tests the following:
  - D_MAKE_ARRAY creates valid int array
  - D_MAKE_ARRAY initializes values correctly
  - D_MAKE_ARRAY creates correct array size
  - D_MAKE_ARRAY works with different types
*/
bool
d_tests_sa_dmacro_make_array
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    size_t count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_MAKE_ARRAY ---\n", D_INDENT);

    // create int array with D_MAKE_ARRAY
    D_MAKE_ARRAY(int, test_ints, 10, 20, 30, 40, 50);

    // verify count
    count = D_ARRAY_COUNT(test_ints);

    if (!d_assert_standalone(count == 5,
                             "D_MAKE_ARRAY creates array with 5 elements",
                             "array should have 5 elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %zu\n", D_INDENT, count);
    }

    // verify values
    if (!d_assert_standalone(test_ints[0] == 10,
                             "test_ints[0] == 10",
                             "first element should be 10",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(test_ints[2] == 30,
                             "test_ints[2] == 30",
                             "third element should be 30",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(test_ints[4] == 50,
                             "test_ints[4] == 50",
                             "fifth element should be 50",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // create double array
    D_MAKE_ARRAY(double, test_doubles, 1.5, 2.5, 3.5);

    count = D_ARRAY_COUNT(test_doubles);

    if (!d_assert_standalone(count == 3,
                             "D_MAKE_ARRAY(double) creates 3 element array",
                             "double array should have 3 elements",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(test_doubles[1] == 2.5,
                             "test_doubles[1] == 2.5",
                             "second double should be 2.5",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // create char array
    D_MAKE_ARRAY(char, test_chars, 'x', 'y', 'z');

    if (!d_assert_standalone(test_chars[0] == 'x' && 
                             test_chars[1] == 'y' && 
                             test_chars[2] == 'z',
                             "D_MAKE_ARRAY(char) initializes correctly",
                             "char array should have x, y, z",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_MAKE_ARRAY test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_MAKE_ARRAY test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_MAKE_STRING_ARRAY TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_make_string_array
  Tests the D_MAKE_STRING_ARRAY macro for generating string arrays.
  Tests the following:
  - D_MAKE_STRING_ARRAY creates valid string array
  - D_MAKE_STRING_ARRAY stringifies identifiers correctly
  - D_MAKE_STRING_ARRAY creates correct array size
  - strings are null-terminated and comparable
*/
bool
d_tests_sa_dmacro_make_string_array
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    size_t count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_MAKE_STRING_ARRAY ---\n", D_INDENT);

    // create string array from identifiers
    D_MAKE_STRING_ARRAY(test_strings, foo, bar, baz);

    // verify count
    count = D_ARRAY_COUNT(test_strings);

    if (!d_assert_standalone(count == 3,
                             "D_MAKE_STRING_ARRAY creates 3 element array",
                             "should have 3 strings",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %zu\n", D_INDENT, count);
    }

    // verify strings match expected values
    if (!d_assert_standalone(strcmp(test_strings[0], "foo") == 0,
                             "test_strings[0] == \"foo\"",
                             "first string should be \"foo\"",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, test_strings[0]);
    }

    if (!d_assert_standalone(strcmp(test_strings[1], "bar") == 0,
                             "test_strings[1] == \"bar\"",
                             "second string should be \"bar\"",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, test_strings[1]);
    }

    if (!d_assert_standalone(strcmp(test_strings[2], "baz") == 0,
                             "test_strings[2] == \"baz\"",
                             "third string should be \"baz\"",
                             _test_info))
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

    if (!d_assert_standalone(count == 4,
                             "D_MAKE_STRING_ARRAY with 4 long names",
                             "should have 4 strings",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(strcmp(long_names[0], "hello_world") == 0,
                             "long_names[0] == \"hello_world\"",
                             "underscore in identifier preserved",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, long_names[0]);
    }

    // test with numeric-like identifiers
    D_MAKE_STRING_ARRAY(num_names, x1, y2, z3);

    if (!d_assert_standalone(strcmp(num_names[0], "x1") == 0 &&
                             strcmp(num_names[1], "y2") == 0 &&
                             strcmp(num_names[2], "z3") == 0,
                             "D_MAKE_STRING_ARRAY with alphanumeric names",
                             "alphanumeric identifiers work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Created arrays: test_strings, long_names, num_names\n", D_INDENT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_MAKE_STRING_ARRAY test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_MAKE_STRING_ARRAY test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * ARRAY UTILITIES MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_array_all
  Runs all array utilities section tests.
  Tests the following:
  - D_ARRAY_COUNT with various array types
  - D_ARRAY_COUNT_SAFE variadic counting
  - D_ARRAY_COUNT_T typed variadic counting
  - D_MAKE_ARRAY array generation
  - D_MAKE_STRING_ARRAY string array generation
*/
bool
d_tests_sa_dmacro_array_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  count_basic_result;
    bool                  count_initialized_result;
    bool                  count_structs_result;
    bool                  count_safe_result;
    bool                  count_t_result;
    bool                  make_array_result;
    bool                  make_string_array_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Array Utilities (Section II)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all array utility tests
    count_basic_result       = d_tests_sa_dmacro_array_count_basic(&module_counter);
    count_initialized_result = d_tests_sa_dmacro_array_count_initialized(&module_counter);
    count_structs_result     = d_tests_sa_dmacro_array_count_structs(&module_counter);
    count_safe_result        = d_tests_sa_dmacro_array_count_safe(&module_counter);
    count_t_result           = d_tests_sa_dmacro_array_count_t(&module_counter);
    make_array_result        = d_tests_sa_dmacro_make_array(&module_counter);
    make_string_array_result = d_tests_sa_dmacro_make_string_array(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( count_basic_result       &&
                       count_initialized_result &&
                       count_structs_result     &&
                       count_safe_result        &&
                       count_t_result           &&
                       make_array_result        &&
                       make_string_array_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Array Utilities Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Array Utilities Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - D_ARRAY_COUNT Basic:       %s\n",
               count_basic_result ? "PASSED" : "FAILED");
        printf("  - D_ARRAY_COUNT Initialized: %s\n",
               count_initialized_result ? "PASSED" : "FAILED");
        printf("  - D_ARRAY_COUNT Structs:     %s\n",
               count_structs_result ? "PASSED" : "FAILED");
        printf("  - D_ARRAY_COUNT_SAFE:        %s\n",
               count_safe_result ? "PASSED" : "FAILED");
        printf("  - D_ARRAY_COUNT_T:           %s\n",
               count_t_result ? "PASSED" : "FAILED");
        printf("  - D_MAKE_ARRAY:              %s\n",
               make_array_result ? "PASSED" : "FAILED");
        printf("  - D_MAKE_STRING_ARRAY:       %s\n",
               make_string_array_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}