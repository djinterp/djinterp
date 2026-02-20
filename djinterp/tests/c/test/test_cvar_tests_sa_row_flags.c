#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * I. REGISTRY ROW FLAG TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_row_flag_is_required
  Tests the D_TEST_REGISTRY_FLAG_IS_REQUIRED enum value.
  Tests the following:
  - IS_REQUIRED equals (1u << 0), i.e. 0x01
  - IS_REQUIRED is non-zero
*/
bool
d_tests_sa_cvar_row_flag_is_required
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: IS_REQUIRED equals bit 0
    result = d_assert_standalone(
        D_TEST_REGISTRY_FLAG_IS_REQUIRED == (1u << 0),
        "row_flag_is_required_bit0",
        "IS_REQUIRED should be (1u << 0)",
        _counter) && result;

    // test 2: IS_REQUIRED is non-zero
    result = d_assert_standalone(
        D_TEST_REGISTRY_FLAG_IS_REQUIRED != 0,
        "row_flag_is_required_nonzero",
        "IS_REQUIRED should be non-zero",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_flag_is_config
  Tests the D_TEST_REGISTRY_FLAG_IS_CONFIG enum value.
  Tests the following:
  - IS_CONFIG equals (1u << 1), i.e. 0x02
  - IS_CONFIG does not overlap with IS_REQUIRED
*/
bool
d_tests_sa_cvar_row_flag_is_config
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: IS_CONFIG equals bit 1
    result = d_assert_standalone(
        D_TEST_REGISTRY_FLAG_IS_CONFIG == (1u << 1),
        "row_flag_is_config_bit1",
        "IS_CONFIG should be (1u << 1)",
        _counter) && result;

    // test 2: IS_CONFIG does not overlap IS_REQUIRED
    result = d_assert_standalone(
        (D_TEST_REGISTRY_FLAG_IS_CONFIG & D_TEST_REGISTRY_FLAG_IS_REQUIRED) == 0,
        "row_flag_is_config_no_overlap_required",
        "IS_CONFIG should not overlap IS_REQUIRED",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_flag_is_metadata
  Tests the D_TEST_REGISTRY_FLAG_IS_METADATA enum value.
  Tests the following:
  - IS_METADATA equals (1u << 2), i.e. 0x04
  - IS_METADATA does not overlap IS_CONFIG or IS_REQUIRED
*/
bool
d_tests_sa_cvar_row_flag_is_metadata
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: IS_METADATA equals bit 2
    result = d_assert_standalone(
        D_TEST_REGISTRY_FLAG_IS_METADATA == (1u << 2),
        "row_flag_is_metadata_bit2",
        "IS_METADATA should be (1u << 2)",
        _counter) && result;

    // test 2: IS_METADATA does not overlap IS_CONFIG or IS_REQUIRED
    result = d_assert_standalone(
        (D_TEST_REGISTRY_FLAG_IS_METADATA &
         (D_TEST_REGISTRY_FLAG_IS_CONFIG | D_TEST_REGISTRY_FLAG_IS_REQUIRED))
            == 0,
        "row_flag_is_metadata_no_overlap",
        "IS_METADATA should not overlap IS_CONFIG or IS_REQUIRED",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_flag_unique_bits
  Tests that all DTestRegistryRowFlag values are unique powers of 2.
  Tests the following:
  - Each flag from TEST_FN through SESSION occupies exactly one bit
  - All 9 flags have distinct values
*/
bool
d_tests_sa_cvar_row_flag_unique_bits
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t flags[9];
    size_t   i;
    size_t   j;
    bool     all_unique;

    result = true;

    flags[0] = D_TEST_REGISTRY_FLAG_IS_REQUIRED;
    flags[1] = D_TEST_REGISTRY_FLAG_IS_CONFIG;
    flags[2] = D_TEST_REGISTRY_FLAG_IS_METADATA;
    flags[3] = D_TEST_REGISTRY_FLAG_TEST_FN;
    flags[4] = D_TEST_REGISTRY_FLAG_ASSERTS;
    flags[5] = D_TEST_REGISTRY_FLAG_TESTS;
    flags[6] = D_TEST_REGISTRY_FLAG_BLOCKS;
    flags[7] = D_TEST_REGISTRY_FLAG_MODULES;
    flags[8] = D_TEST_REGISTRY_FLAG_SESSION;

    // test 1: each flag is a power of 2 (exactly one bit set)
    for (i = 0; i < 9; i++)
    {
        result = d_assert_standalone(
            (flags[i] != 0) && ((flags[i] & (flags[i] - 1)) == 0),
            "row_flag_power_of_2",
            "Each registry row flag should be a power of 2",
            _counter) && result;
    }

    // test 2: all flags are distinct
    all_unique = true;

    for (i = 0; i < 9 && all_unique; i++)
    {
        for (j = i + 1; j < 9 && all_unique; j++)
        {
            if (flags[i] == flags[j])
            {
                all_unique = false;
            }
        }
    }

    result = d_assert_standalone(
        all_unique,
        "row_flag_all_distinct",
        "All 9 registry row flags should have distinct values",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_flag_no_overlap
  Tests that no two DTestRegistryRowFlag values share any bits.
  Tests the following:
  - Bitwise OR of all flags has exactly 9 bits set (bits 0-8)
  - No bit is claimed by more than one flag
*/
bool
d_tests_sa_cvar_row_flag_no_overlap
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint32_t combined;
    uint32_t bit_count;
    uint32_t temp;

    result = true;

    // combine all flags
    combined = D_TEST_REGISTRY_FLAG_IS_REQUIRED |
               D_TEST_REGISTRY_FLAG_IS_CONFIG   |
               D_TEST_REGISTRY_FLAG_IS_METADATA |
               D_TEST_REGISTRY_FLAG_TEST_FN     |
               D_TEST_REGISTRY_FLAG_ASSERTS     |
               D_TEST_REGISTRY_FLAG_TESTS       |
               D_TEST_REGISTRY_FLAG_BLOCKS      |
               D_TEST_REGISTRY_FLAG_MODULES     |
               D_TEST_REGISTRY_FLAG_SESSION;

    // test 1: combined value should be 0x1FF (bits 0 through 8)
    result = d_assert_standalone(
        combined == 0x1FFu,
        "row_flag_combined_0x1FF",
        "OR of all 9 flags should be 0x1FF",
        _counter) && result;

    // test 2: count bits - should be exactly 9
    bit_count = 0;
    temp      = combined;

    while (temp)
    {
        bit_count += (temp & 1u);
        temp     >>= 1;
    }

    result = d_assert_standalone(
        bit_count == 9,
        "row_flag_9_bits_set",
        "Combined flags should have exactly 9 bits set",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_row_flag_all
  Aggregation function that runs all registry row flag tests.
*/
bool
d_tests_sa_cvar_row_flag_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Registry Row Flags\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_row_flag_is_required(_counter) && result;
    result = d_tests_sa_cvar_row_flag_is_config(_counter)   && result;
    result = d_tests_sa_cvar_row_flag_is_metadata(_counter) && result;
    result = d_tests_sa_cvar_row_flag_unique_bits(_counter)  && result;
    result = d_tests_sa_cvar_row_flag_no_overlap(_counter)   && result;

    return result;
}
