/*******************************************************************************
* djinterp [test]                                  test_cvar_tests_value_union.c
*
* test_cvar.h's suite, block "cvar_value_union_all": each member of
* dj_test_value (pointer, size_t, uint32_t, uint16_t, int32_t, bool) holds its
* value.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_value_union.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 2.  VALUE UNION TESTS
//==============================================================================

/*
d_tests_cvar_value_union_ptr
  Tests the pointer member of the dj_test_value union.
  Tests the following:
  - ptr member is accessible and can store a pointer
  - ptr member can store NULL
*/
bool
d_tests_cvar_value_union_ptr(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_value_union_ptr");

    bool result = true;

    // test 1: ptr member stores a valid pointer
    int                 dummy = 42;
    union dj_test_value val   = {0};

    val.ptr = &dummy;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_ptr_stores_pointer",
                              val.ptr == &dummy,
                              "ptr member should store a valid pointer") && result;

    // test 2: ptr member stores NULL
    val.ptr = NULL;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_ptr_stores_null",
                              val.ptr == NULL,
                              "ptr member should store NULL") && result;

    return result;
}

/*
d_tests_cvar_value_union_size_t
  Tests the size_t member of the dj_test_value union.
  Tests the following:
  - z member is accessible and stores zero
  - z member stores a large value
*/
bool
d_tests_cvar_value_union_size_t(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_value_union_size_t");

    bool result = true;

    union dj_test_value val = {0};

    // test 1: z member stores zero
    val.z = 0;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_size_t_zero",
                              val.z == 0,
                              "z member should store zero") && result;

    // test 2: z member stores a large value
    val.z = 999999;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_size_t_large",
                              val.z == 999999,
                              "z member should store large values") && result;

    return result;
}

/*
d_tests_cvar_value_union_uint32
  Tests the uint32_t member of the dj_test_value union.
  Tests the following:
  - u32 member stores zero
  - u32 member stores max uint32 value
*/
bool
d_tests_cvar_value_union_uint32(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_value_union_uint32");

    bool result = true;

    union dj_test_value val = {0};

    // test 1: u32 member stores zero
    val.u32 = 0;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_uint32_zero",
                              val.u32 == 0,
                              "u32 member should store zero") && result;

    // test 2: u32 member stores UINT32_MAX
    val.u32 = UINT32_MAX;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_uint32_max",
                              val.u32 == UINT32_MAX,
                              "u32 member should store UINT32_MAX") && result;

    return result;
}

/*
d_tests_cvar_value_union_uint16
  Tests the uint16_t member of the dj_test_value union.
  Tests the following:
  - u16 member stores zero
  - u16 member stores UINT16_MAX
*/
bool
d_tests_cvar_value_union_uint16(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_value_union_uint16");

    bool result = true;

    union dj_test_value val = {0};

    // test 1: u16 member stores zero
    val.u16 = 0;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_uint16_zero",
                              val.u16 == 0,
                              "u16 member should store zero") && result;

    // test 2: u16 member stores UINT16_MAX
    val.u16 = UINT16_MAX;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_uint16_max",
                              val.u16 == UINT16_MAX,
                              "u16 member should store UINT16_MAX") && result;

    return result;
}

/*
d_tests_cvar_value_union_int32
  Tests the int32_t member of the dj_test_value union.
  Tests the following:
  - i32 member stores zero
  - i32 member stores negative value
  - i32 member stores positive value
*/
bool
d_tests_cvar_value_union_int32(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_value_union_int32");

    bool result = true;

    union dj_test_value val = {0};

    // test 1: i32 member stores zero
    val.i32 = 0;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_int32_zero",
                              val.i32 == 0,
                              "i32 member should store zero") && result;

    // test 2: i32 member stores negative value
    val.i32 = -42;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_int32_negative",
                              val.i32 == -42,
                              "i32 member should store negative values") && result;

    // test 3: i32 member stores positive value
    val.i32 = INT32_MAX;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_int32_max",
                              val.i32 == INT32_MAX,
                              "i32 member should store INT32_MAX") && result;

    return result;
}

/*
d_tests_cvar_value_union_bool
  Tests the bool member of the dj_test_value union.
  Tests the following:
  - b member stores true
  - b member stores false
*/
bool
d_tests_cvar_value_union_bool(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_value_union_bool");

    bool result = true;

    union dj_test_value val = {0};

    // test 1: b member stores true
    val.b = true;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_bool_true",
                              val.b == true,
                              "b member should store true") && result;

    // test 2: b member stores false
    val.b = false;

    result = D_TEST_CHECK_MSG(_suite,
                              "value_union_bool_false",
                              val.b == false,
                              "b member should store false") && result;

    return result;
}

/*
d_tests_cvar_value_union_all
  Aggregation function that runs all value union tests.
*/
bool
d_tests_cvar_value_union_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_value_union_all");

    bool result = true;

    result = d_tests_cvar_value_union_ptr(_suite)    && result;
    result = d_tests_cvar_value_union_size_t(_suite) && result;
    result = d_tests_cvar_value_union_uint32(_suite)  && result;
    result = d_tests_cvar_value_union_uint16(_suite)  && result;
    result = d_tests_cvar_value_union_int32(_suite)   && result;
    result = d_tests_cvar_value_union_bool(_suite)    && result;

    return result;
}
