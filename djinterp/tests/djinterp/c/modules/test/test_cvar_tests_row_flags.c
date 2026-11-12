/*******************************************************************************
* djinterp [c][test]                                 test_cvar_tests_row_flags.c
*
* test_cvar.h's suite, block "cvar_row_flag_all": DTestRegistryRowFlag's
* IS_REQUIRED, IS_CONFIG and IS_METADATA bits, each a distinct power of two, no
* two overlapping.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_row_flags.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


//==============================================================================
// 1.  REGISTRY ROW FLAG TESTS
//==============================================================================

/*
d_tests_cvar_row_flag_is_required
  Tests the D_TEST_REGISTRY_FLAG_IS_REQUIRED enum value.
  Tests the following:
  - IS_REQUIRED equals (1u << 0), i.e. 0x01
  - IS_REQUIRED is non-zero
*/
bool
d_tests_cvar_row_flag_is_required(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_flag_is_required");

    bool result = true;

    // test 1: IS_REQUIRED equals bit 0
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_is_required_bit0",
                              D_TEST_REGISTRY_FLAG_IS_REQUIRED == (1u << 0),
                              "IS_REQUIRED should be (1u << 0)") && result;

    // test 2: IS_REQUIRED is non-zero
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_is_required_nonzero",
                              D_TEST_REGISTRY_FLAG_IS_REQUIRED != 0,
                              "IS_REQUIRED should be non-zero") && result;

    return result;
}

/*
d_tests_cvar_row_flag_is_config
  Tests the D_TEST_REGISTRY_FLAG_IS_CONFIG enum value.
  Tests the following:
  - IS_CONFIG equals (1u << 1), i.e. 0x02
  - IS_CONFIG does not overlap with IS_REQUIRED
*/
bool
d_tests_cvar_row_flag_is_config(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_flag_is_config");

    bool result = true;

    // test 1: IS_CONFIG equals bit 1
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_is_config_bit1",
                              D_TEST_REGISTRY_FLAG_IS_CONFIG == (1u << 1),
                              "IS_CONFIG should be (1u << 1)") && result;

    // test 2: IS_CONFIG does not overlap IS_REQUIRED
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_is_config_no_overlap_required",
                              (D_TEST_REGISTRY_FLAG_IS_CONFIG & D_TEST_REGISTRY_FLAG_IS_REQUIRED) == 0,
                              "IS_CONFIG should not overlap IS_REQUIRED") && result;

    return result;
}

/*
d_tests_cvar_row_flag_is_metadata
  Tests the D_TEST_REGISTRY_FLAG_IS_METADATA enum value.
  Tests the following:
  - IS_METADATA equals (1u << 2), i.e. 0x04
  - IS_METADATA does not overlap IS_CONFIG or IS_REQUIRED
*/
bool
d_tests_cvar_row_flag_is_metadata(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_flag_is_metadata");

    bool result = true;

    // test 1: IS_METADATA equals bit 2
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_is_metadata_bit2",
                              D_TEST_REGISTRY_FLAG_IS_METADATA == (1u << 2),
                              "IS_METADATA should be (1u << 2)") && result;

    // test 2: IS_METADATA does not overlap IS_CONFIG or IS_REQUIRED
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_is_metadata_no_overlap",
                              (D_TEST_REGISTRY_FLAG_IS_METADATA & (D_TEST_REGISTRY_FLAG_IS_CONFIG | D_TEST_REGISTRY_FLAG_IS_REQUIRED)) == 0,
                              "IS_METADATA should not overlap IS_CONFIG or IS_REQUIRED") && result;

    return result;
}

/*
d_tests_cvar_row_flag_unique_bits
  Tests that all DTestRegistryRowFlag values are unique powers of 2.
  Tests the following:
  - Each flag from TEST_FN through SESSION occupies exactly one bit
  - All 9 flags have distinct values
*/
bool
d_tests_cvar_row_flag_unique_bits(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_flag_unique_bits");

    bool     result   = true;
    uint32_t flags[9] = {0};

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
    for (size_t i = 0; i < 9; i++)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "row_flag_power_of_2",
                                  ( (flags[i] != 0) &&
                                    ((flags[i] & (flags[i] - 1)) == 0) ),
                                  "Each registry row flag should be a power of 2") && result;
    }

    // test 2: all flags are distinct
    bool all_unique = true;

    // each flag against every later one, until two match
    for (size_t i = 0; ( (i < 9) && (all_unique) ); i++)
    {
        // each later flag
        for (size_t j = i + 1; ( (j < 9) && (all_unique) ); j++)
        {
            // two flags with one value
            if (flags[i] == flags[j])
            {
                all_unique = false;
            }
        }
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_all_distinct",
                              all_unique,
                              "All 9 registry row flags should have distinct values") && result;

    return result;
}

/*
d_tests_cvar_row_flag_no_overlap
  Tests that no two DTestRegistryRowFlag values share any bits.
  Tests the following:
  - Bitwise OR of all flags has exactly 9 bits set (bits 0-8)
  - No bit is claimed by more than one flag
*/
bool
d_tests_cvar_row_flag_no_overlap(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "cvar_row_flag_no_overlap");

    bool result = true;

    // combine all flags
    const uint32_t combined = D_TEST_REGISTRY_FLAG_IS_REQUIRED |
                              D_TEST_REGISTRY_FLAG_IS_CONFIG   |
                              D_TEST_REGISTRY_FLAG_IS_METADATA |
                              D_TEST_REGISTRY_FLAG_TEST_FN     |
                              D_TEST_REGISTRY_FLAG_ASSERTS     |
                              D_TEST_REGISTRY_FLAG_TESTS       |
                              D_TEST_REGISTRY_FLAG_BLOCKS      |
                              D_TEST_REGISTRY_FLAG_MODULES     |
                              D_TEST_REGISTRY_FLAG_SESSION;

    // test 1: combined value should be 0x1FF (bits 0 through 8)
    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_combined_0x1FF",
                              combined == 0x1FFu,
                              "OR of all 9 flags should be 0x1FF") && result;

    // test 2: count bits - should be exactly 9
    uint32_t bit_count = 0;
    uint32_t temp      = combined;

    // count the bits set in the union of the flags
    while (temp)
    {
        bit_count += (temp & 1u);
        temp     >>= 1;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "row_flag_9_bits_set",
                              bit_count == 9,
                              "Combined flags should have exactly 9 bits set") && result;

    return result;
}

/*
d_tests_cvar_row_flag_all
  Aggregation function that runs all registry row flag tests.
*/
bool
d_tests_cvar_row_flag_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "cvar_row_flag_all");

    bool result = true;

    result = d_tests_cvar_row_flag_is_required(_suite) && result;
    result = d_tests_cvar_row_flag_is_config(_suite)   && result;
    result = d_tests_cvar_row_flag_is_metadata(_suite) && result;
    result = d_tests_cvar_row_flag_unique_bits(_suite)  && result;
    result = d_tests_cvar_row_flag_no_overlap(_suite)   && result;

    return result;
}
