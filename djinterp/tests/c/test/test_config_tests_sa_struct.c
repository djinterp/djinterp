#include ".\test_config_tests_sa.h"


/******************************************************************************
 * IX. CORE STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_config_struct_members
  Tests the d_test_config structure members.
  Tests the following:
  - flags member is accessible and writable
  - settings member is accessible
  - stage_hooks member is accessible
  - Members have correct types (by assignment compatibility)
*/
bool
d_tests_sa_config_struct_members
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_test_config config_stack;

    result = true;

    // test 1: flags member is accessible
    config_stack.flags = 0x12345678u;

    result = d_assert_standalone(
        config_stack.flags == 0x12345678u,
        "struct_flags_accessible",
        "flags member should be accessible and writable",
        _counter) && result;

    // test 2: settings member is accessible
    config_stack.settings = NULL;

    result = d_assert_standalone(
        config_stack.settings == NULL,
        "struct_settings_accessible",
        "settings member should be accessible",
        _counter) && result;

    // test 3: stage_hooks member is accessible
    config_stack.stage_hooks = NULL;

    result = d_assert_standalone(
        config_stack.stage_hooks == NULL,
        "struct_stage_hooks_accessible",
        "stage_hooks member should be accessible",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_struct_size
  Tests the d_test_config structure size and layout.
  Tests the following:
  - Structure has non-zero size
  - Size is at least large enough for its members
*/
bool
d_tests_sa_config_struct_size
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t config_size;

    result      = true;
    config_size = sizeof(struct d_test_config);

    // test 1: non-zero size
    result = d_assert_standalone(
        config_size > 0,
        "struct_size_nonzero",
        "d_test_config should have non-zero size",
        _counter) && result;

    // test 2: at least sizeof(uint32_t) + 2 pointers
    result = d_assert_standalone(
        config_size >= (sizeof(uint32_t) +
                        sizeof(struct d_min_enum_map*) +
                        sizeof(struct d_min_enum_map*)),
        "struct_size_minimum",
        "d_test_config should fit flags + two pointers",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_struct_all
  Aggregation function that runs all core structure tests.
*/
bool
d_tests_sa_config_struct_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Core Structure\n");
    printf("  --------------------------\n");

    result = d_tests_sa_config_struct_members(_counter) && result;
    result = d_tests_sa_config_struct_size(_counter) && result;

    return result;
}
