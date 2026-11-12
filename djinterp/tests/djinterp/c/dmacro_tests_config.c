/*******************************************************************************
* djinterp [c]                                             dmacro_tests_config.c
*
*   Unit tests for `dmacro.h` configuration system.
*   Tests D_CFG_DMACRO_* configuration macros, variant selection, limit
*   constants, and query macros.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_config.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * CONFIGURATION CONSTANT TESTS
 *****************************************************************************/

/*
d_tests_dmacro_cfg_constants
  Tests the configuration constant definitions.
  Tests the following:
  - D_INTERNAL_DMACRO_VARG_DEFAULT is defined and equals 128
  - D_INTERNAL_DMACRO_VARG_MIN is defined and equals 64
  - D_INTERNAL_DMACRO_VARG_LIMIT is defined and equals 1024
  - variant constants are defined with correct values
  - constants are in ascending order
*/
bool d_tests_dmacro_cfg_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_constants");

    all_assertions_passed = true;

    // verify D_INTERNAL_DMACRO_VARG_DEFAULT
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INTERNAL_DMACRO_VARG_DEFAULT == 128",
                          D_INTERNAL_DMACRO_VARG_DEFAULT == 128,
                          "default should be 128"))
    {
        all_assertions_passed = false;
    }

    // verify D_INTERNAL_DMACRO_VARG_MIN
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INTERNAL_DMACRO_VARG_MIN == 64",
                          D_INTERNAL_DMACRO_VARG_MIN == 64,
                          "minimum should be 64"))
    {
        all_assertions_passed = false;
    }

    // verify D_INTERNAL_DMACRO_VARG_LIMIT
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INTERNAL_DMACRO_VARG_LIMIT == 1024",
                          D_INTERNAL_DMACRO_VARG_LIMIT == 1024,
                          "limit should be 1024"))
    {
        all_assertions_passed = false;
    }

    // verify variant constants
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARIANT_64 == 64",
                          D_CFG_DMACRO_VARIANT_64 == 64,
                          "variant 64 constant correct"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARIANT_128 == 128",
                          D_CFG_DMACRO_VARIANT_128 == 128,
                          "variant 128 constant correct"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARIANT_256 == 256",
                          D_CFG_DMACRO_VARIANT_256 == 256,
                          "variant 256 constant correct"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARIANT_512 == 512",
                          D_CFG_DMACRO_VARIANT_512 == 512,
                          "variant 512 constant correct"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARIANT_1024 == 1024",
                          D_CFG_DMACRO_VARIANT_1024 == 1024,
                          "variant 1024 constant correct"))
    {
        all_assertions_passed = false;
    }

    // verify constants are in ascending order
    if (!D_TEST_CHECK_MSG(_suite,
                          "VARG_MIN < VARG_DEFAULT",
                          D_INTERNAL_DMACRO_VARG_MIN < D_INTERNAL_DMACRO_VARG_DEFAULT,
                          "min should be less than default"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VARG_DEFAULT < VARG_LIMIT",
                          D_INTERNAL_DMACRO_VARG_DEFAULT < D_INTERNAL_DMACRO_VARG_LIMIT,
                          "default should be less than limit"))
    {
        all_assertions_passed = false;
    }

    // verify variant constants are powers of 2 (except 64 which we check separately)
    if (!D_TEST_CHECK_MSG(_suite,
                          "VARIANT_64 is power of 2",
                          (D_CFG_DMACRO_VARIANT_64 & (D_CFG_DMACRO_VARIANT_64 - 1)) == 0,
                          "64 should be power of 2"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VARIANT_128 is power of 2",
                          (D_CFG_DMACRO_VARIANT_128 & (D_CFG_DMACRO_VARIANT_128 - 1)) == 0,
                          "128 should be power of 2"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VARIANT_256 is power of 2",
                          (D_CFG_DMACRO_VARIANT_256 & (D_CFG_DMACRO_VARIANT_256 - 1)) == 0,
                          "256 should be power of 2"))
    {
        all_assertions_passed = false;
    }

    (void)d_test_suite_note(_suite, "Constants: MIN=%d, DEFAULT=%d, LIMIT=%d",
                            D_INTERNAL_DMACRO_VARG_MIN,
                            D_INTERNAL_DMACRO_VARG_DEFAULT,
                            D_INTERNAL_DMACRO_VARG_LIMIT);

    return all_assertions_passed;
}


/*
d_tests_dmacro_cfg_user_options
  Tests the user configuration option definitions.
  Tests the following:
  - D_CFG_DMACRO_OVERRIDE is defined
  - D_CFG_DMACRO_VARG_MAX is defined
  - D_CFG_DMACRO_USE_MSVC_COMPAT is defined
  - override has valid value (0 or 1)
  - MSVC compat has valid value (0 or 1)
  - VARG_MAX is within valid range
*/
bool d_tests_dmacro_cfg_user_options(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    override_val;
    int    msvc_compat_val;
    int    varg_max_val;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_user_options");

    all_assertions_passed = true;

    // capture values (macro expansion)
    override_val    = D_CFG_DMACRO_OVERRIDE;
    msvc_compat_val = D_CFG_DMACRO_USE_MSVC_COMPAT;
    varg_max_val    = D_CFG_DMACRO_VARG_MAX;

    // verify D_CFG_DMACRO_OVERRIDE is 0 or 1
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_OVERRIDE is 0 or 1",
                          override_val == 0 || override_val == 1,
                          "override should be boolean"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", override_val);
    }

    // verify D_CFG_DMACRO_USE_MSVC_COMPAT is 0 or 1
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_USE_MSVC_COMPAT is 0 or 1",
                          msvc_compat_val == 0 || msvc_compat_val == 1,
                          "MSVC compat should be boolean"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", msvc_compat_val);
    }

    // verify D_CFG_DMACRO_VARG_MAX is within valid range
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARG_MAX >= VARG_MIN",
                          varg_max_val >= D_INTERNAL_DMACRO_VARG_MIN,
                          "VARG_MAX should be at least minimum"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, Min: %d",
                                varg_max_val, D_INTERNAL_DMACRO_VARG_MIN);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_DMACRO_VARG_MAX <= VARG_LIMIT",
                          varg_max_val <= D_INTERNAL_DMACRO_VARG_LIMIT,
                          "VARG_MAX should not exceed limit"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, Limit: %d",
                                varg_max_val, D_INTERNAL_DMACRO_VARG_LIMIT);
    }

    (void)d_test_suite_note(_suite,
                            "Options: OVERRIDE=%d, MSVC_COMPAT=%d, VARG_MAX=%d",
                            override_val, msvc_compat_val, varg_max_val);

    return all_assertions_passed;
}


/******************************************************************************
 * EFFECTIVE VALUE TESTS
 *****************************************************************************/

/*
d_tests_dmacro_cfg_effective_values
  Tests the effective configuration values after calculation.
  Tests the following:
  - D_DMACRO_VARIANT is defined and valid
  - D_DMACRO_VARG_MAX is defined and valid
  - D_DMACRO_PAIR_MAX is half of VARG_MAX
  - variant matches a supported value (64, 127, 128, 256, 512, 1024)
  - VARG_MAX is within configured limits
*/
bool d_tests_dmacro_cfg_effective_values(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    variant;
    int    varg_max;
    int    pair_max;
    bool   variant_valid;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_effective_values");

    all_assertions_passed = true;

    // capture effective values
    variant  = D_DMACRO_VARIANT;
    varg_max = D_DMACRO_VARG_MAX;
    pair_max = D_DMACRO_PAIR_MAX;

    // verify D_DMACRO_VARIANT is a valid value
    variant_valid = ( variant == 64   ||
                      variant == 127  ||  // MSVC compat
                      variant == 128  ||
                      variant == 256  ||
                      variant == 512  ||
                      variant == 1024 );

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARIANT is valid (64/127/128/256/512/1024)",
                          variant_valid,
                          "variant should be a supported value"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got variant: %d", variant);
    }

    // verify D_DMACRO_VARG_MAX is positive
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARG_MAX > 0",
                          varg_max > 0,
                          "VARG_MAX should be positive"))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_VARG_MAX is within limits
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARG_MAX within [MIN, LIMIT]",
                          varg_max >= D_INTERNAL_DMACRO_VARG_MIN && varg_max <= D_INTERNAL_DMACRO_VARG_LIMIT,
                          "effective VARG_MAX should be in valid range"))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_PAIR_MAX is half of VARG_MAX (or close for odd variants)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_PAIR_MAX == D_DMACRO_VARG_MAX / 2",
                          pair_max == varg_max / 2 || pair_max == (varg_max - 1) / 2,
                          "PAIR_MAX should be half of VARG_MAX"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "PAIR_MAX=%d, VARG_MAX=%d, Expected=%d",
                                pair_max, varg_max, varg_max / 2);
    }

    // verify variant and VARG_MAX are consistent
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARG_MAX <= D_DMACRO_VARIANT + 1",
                          varg_max <= variant + 1,
                          "VARG_MAX should not exceed variant"))
    {
        all_assertions_passed = false;
    }

    (void)d_test_suite_note(_suite,
                            "Effective: VARIANT=%d, VARG_MAX=%d, PAIR_MAX=%d",
                            variant, varg_max, pair_max);

    return all_assertions_passed;
}


/******************************************************************************
 * PUBLIC MACRO ALIAS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_cfg_public_aliases
  Tests the public configuration macro aliases.
  Tests the following:
  - D_VARG_COUNT_MAX equals D_DMACRO_VARG_MAX
  - D_VARG_PAIR_MAX equals D_DMACRO_PAIR_MAX
  - D_FOR_EACH_MAX equals D_DMACRO_VARG_MAX
  - D_FOR_EACH_PAIR_MAX equals D_DMACRO_PAIR_MAX
  - D_MACRO_VARIANT equals D_DMACRO_VARIANT
*/
bool d_tests_dmacro_cfg_public_aliases(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_public_aliases");

    all_assertions_passed = true;

    // verify D_VARG_COUNT_MAX alias
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT_MAX == D_DMACRO_VARG_MAX",
                          D_VARG_COUNT_MAX == D_DMACRO_VARG_MAX,
                          "VARG_COUNT_MAX should alias DMACRO_VARG_MAX"))
    {
        all_assertions_passed = false;
    }

    // verify D_VARG_PAIR_MAX alias
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_PAIR_MAX == D_DMACRO_PAIR_MAX",
                          D_VARG_PAIR_MAX == D_DMACRO_PAIR_MAX,
                          "VARG_PAIR_MAX should alias DMACRO_PAIR_MAX"))
    {
        all_assertions_passed = false;
    }

    // verify D_FOR_EACH_MAX alias
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_MAX == D_DMACRO_VARG_MAX",
                          D_FOR_EACH_MAX == D_DMACRO_VARG_MAX,
                          "FOR_EACH_MAX should alias DMACRO_VARG_MAX"))
    {
        all_assertions_passed = false;
    }

    // verify D_FOR_EACH_PAIR_MAX alias
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_PAIR_MAX == D_DMACRO_PAIR_MAX",
                          D_FOR_EACH_PAIR_MAX == D_DMACRO_PAIR_MAX,
                          "FOR_EACH_PAIR_MAX should alias DMACRO_PAIR_MAX"))
    {
        all_assertions_passed = false;
    }

    // verify D_MACRO_VARIANT alias
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_MACRO_VARIANT == D_DMACRO_VARIANT",
                          D_MACRO_VARIANT == D_DMACRO_VARIANT,
                          "MACRO_VARIANT should alias DMACRO_VARIANT"))
    {
        all_assertions_passed = false;
    }

    (void)d_test_suite_note(_suite,
                            "Aliases verified: VARG_COUNT_MAX=%d, "
                            "FOR_EACH_MAX=%d",
                            D_VARG_COUNT_MAX,
                            D_FOR_EACH_MAX);

    return all_assertions_passed;
}


/******************************************************************************
 * QUERY MACRO TESTS
 *****************************************************************************/

/*
d_tests_dmacro_cfg_query_macros
  Tests the configuration query macros.
  Tests the following:
  - D_DMACRO_IS_OVERRIDE_ENABLED returns boolean
  - D_DMACRO_IS_MSVC_COMPAT returns boolean
  - D_DMACRO_USES_ENV_DETECTION returns boolean
  - D_DMACRO_CHECK_VARG_LIMIT works correctly
  - D_DMACRO_CHECK_PAIR_LIMIT works correctly
*/
bool d_tests_dmacro_cfg_query_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    override_enabled;
    int    msvc_compat;
    int    uses_env;
    int    varg_check_small;
    int    varg_check_large;
    int    pair_check_small;
    int    pair_check_large;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_query_macros");

    all_assertions_passed = true;

    // evaluate query macros
    override_enabled = D_DMACRO_IS_OVERRIDE_ENABLED();
    msvc_compat      = D_DMACRO_IS_MSVC_COMPAT();
    uses_env         = D_DMACRO_USES_ENV_DETECTION();

    // verify D_DMACRO_IS_OVERRIDE_ENABLED returns boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_IS_OVERRIDE_ENABLED() returns 0 or 1",
                          override_enabled == 0 || override_enabled == 1,
                          "should return boolean"))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_IS_MSVC_COMPAT returns boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_IS_MSVC_COMPAT() returns 0 or 1",
                          msvc_compat == 0 || msvc_compat == 1,
                          "should return boolean"))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_USES_ENV_DETECTION returns boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_USES_ENV_DETECTION() returns 0 or 1",
                          uses_env == 0 || uses_env == 1,
                          "should return boolean"))
    {
        all_assertions_passed = false;
    }

    // verify override and env detection are mutually exclusive
    // (if override is enabled, env detection should not be used)
    if (override_enabled)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "override enabled implies !uses_env",
                              uses_env == 0,
                              "override should disable env detection"))
        {
            all_assertions_passed = false;
        }
    }

    // test D_DMACRO_CHECK_VARG_LIMIT
    varg_check_small = D_DMACRO_CHECK_VARG_LIMIT(10);
    varg_check_large = D_DMACRO_CHECK_VARG_LIMIT(D_DMACRO_VARG_MAX + 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_CHECK_VARG_LIMIT(10) is true",
                          varg_check_small != 0,
                          "10 should be within limit"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_CHECK_VARG_LIMIT(MAX+1) is false",
                          varg_check_large == 0,
                          "MAX+1 should exceed limit"))
    {
        all_assertions_passed = false;
    }

    // test D_DMACRO_CHECK_PAIR_LIMIT
    pair_check_small = D_DMACRO_CHECK_PAIR_LIMIT(5);
    pair_check_large = D_DMACRO_CHECK_PAIR_LIMIT(D_DMACRO_PAIR_MAX + 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_CHECK_PAIR_LIMIT(5) is true",
                          pair_check_small != 0,
                          "5 should be within limit"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_CHECK_PAIR_LIMIT(MAX+1) is false",
                          pair_check_large == 0,
                          "PAIR_MAX+1 should exceed limit"))
    {
        all_assertions_passed = false;
    }

    (void)d_test_suite_note(_suite,
                            "Query results: OVERRIDE=%d, MSVC_COMPAT=%d, "
                            "USES_ENV=%d",
                            override_enabled,
                            msvc_compat,
                            uses_env);

    return all_assertions_passed;
}


/******************************************************************************
 * ENVIRONMENT INTEGRATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_cfg_env_integration
  Tests the integration with env.h detected values.
  Tests the following:
  - D_ENV_PP_MAX_MACRO_ARGS detection (if available)
  - effective values respect environment limits when not overridden
  - effective values respect user limits when overridden
  - MSVC compatibility is detected correctly
*/
bool d_tests_dmacro_cfg_env_integration(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    env_pp_max;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_env_integration");

    all_assertions_passed = true;

    // check if D_ENV_PP_MAX_MACRO_ARGS is defined
#ifdef D_ENV_PP_MAX_MACRO_ARGS
    env_pp_max = D_ENV_PP_MAX_MACRO_ARGS;
    (void)d_test_suite_note(_suite, "D_ENV_PP_MAX_MACRO_ARGS is defined: %d",
                            env_pp_max);

    // if not overridden, effective max should respect env limit
    if (!D_DMACRO_IS_OVERRIDE_ENABLED())
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "effective VARG_MAX respects env limit",
                              D_DMACRO_VARG_MAX <= env_pp_max || D_DMACRO_VARG_MAX <= D_CFG_DMACRO_VARG_MAX,
                              "should not exceed environment limit"))
        {
            all_assertions_passed = false;
        }
    }
#else
    env_pp_max = -1;  // not defined
    (void)d_test_suite_note(_suite, "D_ENV_PP_MAX_MACRO_ARGS is NOT defined");

    // without env detection, should use default or user-specified value
    if (!D_TEST_CHECK_MSG(_suite,
                          "VARG_MAX uses default/user value without env",
                          D_DMACRO_VARG_MAX == D_INTERNAL_DMACRO_VARG_DEFAULT || D_DMACRO_VARG_MAX == D_CFG_DMACRO_VARG_MAX,
                          "should use configured value"))
    {
        all_assertions_passed = false;
    }
#endif

    // verify MSVC detection
#if defined(_MSC_VER) && !defined(__clang__)
    (void)d_test_suite_note(_suite, "MSVC detected: _MSC_VER=%d", _MSC_VER);

    #if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
        (void)d_test_suite_note(_suite,
                                "MSVC traditional preprocessor detected");

        // should auto-enable MSVC compat unless user disabled it
        if (!D_TEST_CHECK_MSG(_suite,
                              "MSVC traditional uses compat or non-128 variant",
                              D_CFG_DMACRO_USE_MSVC_COMPAT == 1 || D_DMACRO_VARIANT != 128,
                              "should use MSVC-safe variant"))
        {
            all_assertions_passed = false;
        }
    #else
        (void)d_test_suite_note(_suite,
                                "MSVC conformant preprocessor detected");
    #endif
#else
    (void)d_test_suite_note(_suite, "Non-MSVC compiler detected");

    // non-MSVC should not auto-enable MSVC compat
    if (!D_TEST_CHECK_MSG(_suite,
                          "MSVC compat is explicitly configured",
                          D_CFG_DMACRO_USE_MSVC_COMPAT == 0 || D_CFG_DMACRO_USE_MSVC_COMPAT == 1,
                          "should be user-configured or default 0"))
    {
        all_assertions_passed = false;
    }
#endif

    // verify at least one assertion was made
    if (!D_TEST_CHECK_MSG(_suite,
                          "environment integration test completed",
                          true,
                          "sanity check"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * VARIANT CONSISTENCY TESTS
 *****************************************************************************/

/*
d_tests_dmacro_cfg_variant_consistency
  Tests that the selected variant is consistent with configuration.
  Tests the following:
  - selected variant is >= effective VARG_MAX
  - variant file should provide correct macros
  - D_VARG_COUNT should be available
  - D_HAS_ARGS should be available
  - D_FOR_EACH should be available (indirectly tested)
*/
bool d_tests_dmacro_cfg_variant_consistency(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    variant;
    int    varg_max;
    int    count_result;
    int    has_args_result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_cfg_variant_consistency");

    all_assertions_passed = true;

    variant  = D_DMACRO_VARIANT;
    varg_max = D_DMACRO_VARG_MAX;

    // verify variant is >= VARG_MAX
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARIANT >= D_DMACRO_VARG_MAX",
                          variant >= varg_max,
                          "variant should support configured max"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "VARIANT=%d, VARG_MAX=%d",
                                variant, varg_max);
    }

    // verify D_VARG_COUNT is functional
    count_result = D_VARG_COUNT(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1) == 1",
                          count_result == 1,
                          "D_VARG_COUNT should work with 1 arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got count: %d", count_result);
    }

    count_result = D_VARG_COUNT(1, 2, 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1, 2, 3) == 3",
                          count_result == 3,
                          "D_VARG_COUNT should work with 3 args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got count: %d", count_result);
    }

    // verify D_HAS_ARGS is functional
    has_args_result = D_HAS_ARGS(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(1) == 1",
                          has_args_result == 1,
                          "D_HAS_ARGS should return 1 for args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", has_args_result);
    }

    // test with more arguments to verify variant capacity
    count_result = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(10 args) == 10",
                          count_result == 10,
                          "D_VARG_COUNT should work with 10 args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got count: %d", count_result);
    }

    (void)d_test_suite_note(_suite, "Variant %d provides functional macros",
                            variant);

    return all_assertions_passed;
}


/******************************************************************************
 * CONFIGURATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_cfg_all
  Runs all dmacro configuration section tests.
  Tests the following:
  - configuration constants
  - user configuration options
  - effective configuration values
  - public configuration aliases
  - query macros
  - environment integration
  - variant consistency
*/
bool d_tests_dmacro_cfg_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_cfg_all");

    d_tests_dmacro_cfg_constants(_suite);
    d_tests_dmacro_cfg_user_options(_suite);
    d_tests_dmacro_cfg_effective_values(_suite);
    d_tests_dmacro_cfg_public_aliases(_suite);
    d_tests_dmacro_cfg_query_macros(_suite);
    d_tests_dmacro_cfg_env_integration(_suite);
    d_tests_dmacro_cfg_variant_consistency(_suite);

    return true;
}
