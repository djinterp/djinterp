#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * II. VALUE UNION TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_value_union_ptr
  Tests the pointer member of the d_test_value union.
  Tests the following:
  - ptr member is accessible and can store a pointer
  - ptr member can store NULL
*/
bool
d_tests_sa_cvar_value_union_ptr
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;
    int                dummy;

    result = true;

    // test 1: ptr member stores a valid pointer
    dummy   = 42;
    val.ptr = &dummy;

    result = d_assert_standalone(
        val.ptr == &dummy,
        "value_union_ptr_stores_pointer",
        "ptr member should store a valid pointer",
        _counter) && result;

    // test 2: ptr member stores NULL
    val.ptr = NULL;

    result = d_assert_standalone(
        val.ptr == NULL,
        "value_union_ptr_stores_null",
        "ptr member should store NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_value_union_size_t
  Tests the size_t member of the d_test_value union.
  Tests the following:
  - z member is accessible and stores zero
  - z member stores a large value
*/
bool
d_tests_sa_cvar_value_union_size_t
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    // test 1: z member stores zero
    val.z = 0;

    result = d_assert_standalone(
        val.z == 0,
        "value_union_size_t_zero",
        "z member should store zero",
        _counter) && result;

    // test 2: z member stores a large value
    val.z = 999999;

    result = d_assert_standalone(
        val.z == 999999,
        "value_union_size_t_large",
        "z member should store large values",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_value_union_uint32
  Tests the uint32_t member of the d_test_value union.
  Tests the following:
  - u32 member stores zero
  - u32 member stores max uint32 value
*/
bool
d_tests_sa_cvar_value_union_uint32
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    // test 1: u32 member stores zero
    val.u32 = 0;

    result = d_assert_standalone(
        val.u32 == 0,
        "value_union_uint32_zero",
        "u32 member should store zero",
        _counter) && result;

    // test 2: u32 member stores UINT32_MAX
    val.u32 = UINT32_MAX;

    result = d_assert_standalone(
        val.u32 == UINT32_MAX,
        "value_union_uint32_max",
        "u32 member should store UINT32_MAX",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_value_union_uint16
  Tests the uint16_t member of the d_test_value union.
  Tests the following:
  - u16 member stores zero
  - u16 member stores UINT16_MAX
*/
bool
d_tests_sa_cvar_value_union_uint16
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    // test 1: u16 member stores zero
    val.u16 = 0;

    result = d_assert_standalone(
        val.u16 == 0,
        "value_union_uint16_zero",
        "u16 member should store zero",
        _counter) && result;

    // test 2: u16 member stores UINT16_MAX
    val.u16 = UINT16_MAX;

    result = d_assert_standalone(
        val.u16 == UINT16_MAX,
        "value_union_uint16_max",
        "u16 member should store UINT16_MAX",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_value_union_int32
  Tests the int32_t member of the d_test_value union.
  Tests the following:
  - i32 member stores zero
  - i32 member stores negative value
  - i32 member stores positive value
*/
bool
d_tests_sa_cvar_value_union_int32
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    // test 1: i32 member stores zero
    val.i32 = 0;

    result = d_assert_standalone(
        val.i32 == 0,
        "value_union_int32_zero",
        "i32 member should store zero",
        _counter) && result;

    // test 2: i32 member stores negative value
    val.i32 = -42;

    result = d_assert_standalone(
        val.i32 == -42,
        "value_union_int32_negative",
        "i32 member should store negative values",
        _counter) && result;

    // test 3: i32 member stores positive value
    val.i32 = INT32_MAX;

    result = d_assert_standalone(
        val.i32 == INT32_MAX,
        "value_union_int32_max",
        "i32 member should store INT32_MAX",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_value_union_bool
  Tests the bool member of the d_test_value union.
  Tests the following:
  - b member stores true
  - b member stores false
*/
bool
d_tests_sa_cvar_value_union_bool
(
    struct d_test_counter* _counter
)
{
    bool               result;
    union d_test_value val;

    result = true;

    // test 1: b member stores true
    val.b = true;

    result = d_assert_standalone(
        val.b == true,
        "value_union_bool_true",
        "b member should store true",
        _counter) && result;

    // test 2: b member stores false
    val.b = false;

    result = d_assert_standalone(
        val.b == false,
        "value_union_bool_false",
        "b member should store false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_value_union_all
  Aggregation function that runs all value union tests.
*/
bool
d_tests_sa_cvar_value_union_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Value Union\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_value_union_ptr(_counter)    && result;
    result = d_tests_sa_cvar_value_union_size_t(_counter) && result;
    result = d_tests_sa_cvar_value_union_uint32(_counter)  && result;
    result = d_tests_sa_cvar_value_union_uint16(_counter)  && result;
    result = d_tests_sa_cvar_value_union_int32(_counter)   && result;
    result = d_tests_sa_cvar_value_union_bool(_counter)    && result;

    return result;
}
