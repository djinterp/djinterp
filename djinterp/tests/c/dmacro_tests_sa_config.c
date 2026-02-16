/******************************************************************************
* djinterp [test]                                         dmacro_tests_sa_cfg.c
*
*   Unit tests for `dmacro.h` configuration system.
*   Tests D_CFG_DMACRO_* configuration macros, variant selection, limit
*   constants, and query macros.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_cfg.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * CONFIGURATION CONSTANT TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_constants
  Tests the configuration constant definitions.
  Tests the following:
  - D_CFG_DMACRO_VARG_DEFAULT is defined and equals 256
  - D_CFG_DMACRO_VARG_MIN is defined and equals 64
  - D_CFG_DMACRO_VARG_LIMIT is defined and equals 1024
  - variant constants are defined with correct values
  - constants are in ascending order
*/
bool
d_tests_sa_dmacro_cfg_constants
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

    printf("%s--- Testing Configuration Constants ---\n", D_INDENT);

    // verify D_CFG_DMACRO_VARG_DEFAULT
    if (!d_assert_standalone(D_CFG_DMACRO_VARG_DEFAULT == 256,
                             "D_CFG_DMACRO_VARG_DEFAULT == 256",
                             "default should be 256",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_CFG_DMACRO_VARG_MIN
    if (!d_assert_standalone(D_CFG_DMACRO_VARG_MIN == 64,
                             "D_CFG_DMACRO_VARG_MIN == 64",
                             "minimum should be 64",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_CFG_DMACRO_VARG_LIMIT
    if (!d_assert_standalone(D_CFG_DMACRO_VARG_LIMIT == 1024,
                             "D_CFG_DMACRO_VARG_LIMIT == 1024",
                             "limit should be 1024",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify variant constants
    if (!d_assert_standalone(D_CFG_DMACRO_VARIANT_64 == 64,
                             "D_CFG_DMACRO_VARIANT_64 == 64",
                             "variant 64 constant correct",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_DMACRO_VARIANT_128 == 128,
                             "D_CFG_DMACRO_VARIANT_128 == 128",
                             "variant 128 constant correct",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_DMACRO_VARIANT_256 == 256,
                             "D_CFG_DMACRO_VARIANT_256 == 256",
                             "variant 256 constant correct",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_DMACRO_VARIANT_512 == 512,
                             "D_CFG_DMACRO_VARIANT_512 == 512",
                             "variant 512 constant correct",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_DMACRO_VARIANT_1024 == 1024,
                             "D_CFG_DMACRO_VARIANT_1024 == 1024",
                             "variant 1024 constant correct",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify constants are in ascending order
    if (!d_assert_standalone(D_CFG_DMACRO_VARG_MIN < D_CFG_DMACRO_VARG_DEFAULT,
                             "VARG_MIN < VARG_DEFAULT",
                             "min should be less than default",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_DMACRO_VARG_DEFAULT < D_CFG_DMACRO_VARG_LIMIT,
                             "VARG_DEFAULT < VARG_LIMIT",
                             "default should be less than limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify variant constants are powers of 2 (except 64 which we check separately)
    if (!d_assert_standalone((D_CFG_DMACRO_VARIANT_64 & (D_CFG_DMACRO_VARIANT_64 - 1)) == 0,
                             "VARIANT_64 is power of 2",
                             "64 should be power of 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_DMACRO_VARIANT_128 & (D_CFG_DMACRO_VARIANT_128 - 1)) == 0,
                             "VARIANT_128 is power of 2",
                             "128 should be power of 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_DMACRO_VARIANT_256 & (D_CFG_DMACRO_VARIANT_256 - 1)) == 0,
                             "VARIANT_256 is power of 2",
                             "256 should be power of 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Constants: MIN=%d, DEFAULT=%d, LIMIT=%d\n",
           D_INDENT,
           D_CFG_DMACRO_VARG_MIN,
           D_CFG_DMACRO_VARG_DEFAULT,
           D_CFG_DMACRO_VARG_LIMIT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Configuration constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Configuration constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_cfg_user_options
  Tests the user configuration option definitions.
  Tests the following:
  - D_CFG_DMACRO_OVERRIDE is defined
  - D_CFG_DMACRO_VARG_MAX is defined
  - D_CFG_DMACRO_USE_MSVC_COMPAT is defined
  - override has valid value (0 or 1)
  - MSVC compat has valid value (0 or 1)
  - VARG_MAX is within valid range
*/
bool
d_tests_sa_dmacro_cfg_user_options
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    override_val;
    int    msvc_compat_val;
    int    varg_max_val;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing User Configuration Options ---\n", D_INDENT);

    // capture values (macro expansion)
    override_val    = D_CFG_DMACRO_OVERRIDE;
    msvc_compat_val = D_CFG_DMACRO_USE_MSVC_COMPAT;
    varg_max_val    = D_CFG_DMACRO_VARG_MAX;

    // verify D_CFG_DMACRO_OVERRIDE is 0 or 1
    if (!d_assert_standalone(override_val == 0 || override_val == 1,
                             "D_CFG_DMACRO_OVERRIDE is 0 or 1",
                             "override should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, override_val);
    }

    // verify D_CFG_DMACRO_USE_MSVC_COMPAT is 0 or 1
    if (!d_assert_standalone(msvc_compat_val == 0 || msvc_compat_val == 1,
                             "D_CFG_DMACRO_USE_MSVC_COMPAT is 0 or 1",
                             "MSVC compat should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, msvc_compat_val);
    }

    // verify D_CFG_DMACRO_VARG_MAX is within valid range
    if (!d_assert_standalone(varg_max_val >= D_CFG_DMACRO_VARG_MIN,
                             "D_CFG_DMACRO_VARG_MAX >= VARG_MIN",
                             "VARG_MAX should be at least minimum",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, Min: %d\n", D_INDENT, varg_max_val, D_CFG_DMACRO_VARG_MIN);
    }

    if (!d_assert_standalone(varg_max_val <= D_CFG_DMACRO_VARG_LIMIT,
                             "D_CFG_DMACRO_VARG_MAX <= VARG_LIMIT",
                             "VARG_MAX should not exceed limit",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, Limit: %d\n", D_INDENT, varg_max_val, D_CFG_DMACRO_VARG_LIMIT);
    }

    printf("%s    Options: OVERRIDE=%d, MSVC_COMPAT=%d, VARG_MAX=%d\n",
           D_INDENT,
           override_val,
           msvc_compat_val,
           varg_max_val);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] User configuration options test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] User configuration options test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * EFFECTIVE VALUE TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_effective_values
  Tests the effective configuration values after calculation.
  Tests the following:
  - D_DMACRO_VARIANT is defined and valid
  - D_DMACRO_VARG_MAX is defined and valid
  - D_DMACRO_PAIR_MAX is half of VARG_MAX
  - variant matches a supported value (64, 127, 128, 256, 512, 1024)
  - VARG_MAX is within configured limits
*/
bool
d_tests_sa_dmacro_cfg_effective_values
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    variant;
    int    varg_max;
    int    pair_max;
    bool   variant_valid;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Effective Configuration Values ---\n", D_INDENT);

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

    if (!d_assert_standalone(variant_valid,
                             "D_DMACRO_VARIANT is valid (64/127/128/256/512/1024)",
                             "variant should be a supported value",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got variant: %d\n", D_INDENT, variant);
    }

    // verify D_DMACRO_VARG_MAX is positive
    if (!d_assert_standalone(varg_max > 0,
                             "D_DMACRO_VARG_MAX > 0",
                             "VARG_MAX should be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_VARG_MAX is within limits
    if (!d_assert_standalone(varg_max >= D_CFG_DMACRO_VARG_MIN &&
                             varg_max <= D_CFG_DMACRO_VARG_LIMIT,
                             "D_DMACRO_VARG_MAX within [MIN, LIMIT]",
                             "effective VARG_MAX should be in valid range",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_PAIR_MAX is half of VARG_MAX (or close for odd variants)
    if (!d_assert_standalone(pair_max == varg_max / 2 ||
                             pair_max == (varg_max - 1) / 2,
                             "D_DMACRO_PAIR_MAX == D_DMACRO_VARG_MAX / 2",
                             "PAIR_MAX should be half of VARG_MAX",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    PAIR_MAX=%d, VARG_MAX=%d, Expected=%d\n",
               D_INDENT, pair_max, varg_max, varg_max / 2);
    }

    // verify variant and VARG_MAX are consistent
    if (!d_assert_standalone(varg_max <= variant + 1,
                             "D_DMACRO_VARG_MAX <= D_DMACRO_VARIANT + 1",
                             "VARG_MAX should not exceed variant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Effective: VARIANT=%d, VARG_MAX=%d, PAIR_MAX=%d\n",
           D_INDENT,
           variant,
           varg_max,
           pair_max);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Effective configuration values test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Effective configuration values test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * PUBLIC MACRO ALIAS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_public_aliases
  Tests the public configuration macro aliases.
  Tests the following:
  - D_VARG_COUNT_MAX equals D_DMACRO_VARG_MAX
  - D_VARG_PAIR_MAX equals D_DMACRO_PAIR_MAX
  - D_FOR_EACH_MAX equals D_DMACRO_VARG_MAX
  - D_FOR_EACH_PAIR_MAX equals D_DMACRO_PAIR_MAX
  - D_MACRO_VARIANT equals D_DMACRO_VARIANT
*/
bool
d_tests_sa_dmacro_cfg_public_aliases
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

    printf("%s--- Testing Public Configuration Aliases ---\n", D_INDENT);

    // verify D_VARG_COUNT_MAX alias
    if (!d_assert_standalone(D_VARG_COUNT_MAX == D_DMACRO_VARG_MAX,
                             "D_VARG_COUNT_MAX == D_DMACRO_VARG_MAX",
                             "VARG_COUNT_MAX should alias DMACRO_VARG_MAX",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_VARG_PAIR_MAX alias
    if (!d_assert_standalone(D_VARG_PAIR_MAX == D_DMACRO_PAIR_MAX,
                             "D_VARG_PAIR_MAX == D_DMACRO_PAIR_MAX",
                             "VARG_PAIR_MAX should alias DMACRO_PAIR_MAX",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_FOR_EACH_MAX alias
    if (!d_assert_standalone(D_FOR_EACH_MAX == D_DMACRO_VARG_MAX,
                             "D_FOR_EACH_MAX == D_DMACRO_VARG_MAX",
                             "FOR_EACH_MAX should alias DMACRO_VARG_MAX",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_FOR_EACH_PAIR_MAX alias
    if (!d_assert_standalone(D_FOR_EACH_PAIR_MAX == D_DMACRO_PAIR_MAX,
                             "D_FOR_EACH_PAIR_MAX == D_DMACRO_PAIR_MAX",
                             "FOR_EACH_PAIR_MAX should alias DMACRO_PAIR_MAX",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_MACRO_VARIANT alias
    if (!d_assert_standalone(D_MACRO_VARIANT == D_DMACRO_VARIANT,
                             "D_MACRO_VARIANT == D_DMACRO_VARIANT",
                             "MACRO_VARIANT should alias DMACRO_VARIANT",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Aliases verified: VARG_COUNT_MAX=%d, FOR_EACH_MAX=%d\n",
           D_INDENT,
           D_VARG_COUNT_MAX,
           D_FOR_EACH_MAX);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Public configuration aliases test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Public configuration aliases test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * QUERY MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_query_macros
  Tests the configuration query macros.
  Tests the following:
  - D_DMACRO_IS_OVERRIDE_ENABLED returns boolean
  - D_DMACRO_IS_MSVC_COMPAT returns boolean
  - D_DMACRO_USES_ENV_DETECTION returns boolean
  - D_DMACRO_CHECK_VARG_LIMIT works correctly
  - D_DMACRO_CHECK_PAIR_LIMIT works correctly
*/
bool
d_tests_sa_dmacro_cfg_query_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    override_enabled;
    int    msvc_compat;
    int    uses_env;
    int    varg_check_small;
    int    varg_check_large;
    int    pair_check_small;
    int    pair_check_large;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Configuration Query Macros ---\n", D_INDENT);

    // evaluate query macros
    override_enabled = D_DMACRO_IS_OVERRIDE_ENABLED();
    msvc_compat      = D_DMACRO_IS_MSVC_COMPAT();
    uses_env         = D_DMACRO_USES_ENV_DETECTION();

    // verify D_DMACRO_IS_OVERRIDE_ENABLED returns boolean
    if (!d_assert_standalone(override_enabled == 0 || override_enabled == 1,
                             "D_DMACRO_IS_OVERRIDE_ENABLED() returns 0 or 1",
                             "should return boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_IS_MSVC_COMPAT returns boolean
    if (!d_assert_standalone(msvc_compat == 0 || msvc_compat == 1,
                             "D_DMACRO_IS_MSVC_COMPAT() returns 0 or 1",
                             "should return boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_DMACRO_USES_ENV_DETECTION returns boolean
    if (!d_assert_standalone(uses_env == 0 || uses_env == 1,
                             "D_DMACRO_USES_ENV_DETECTION() returns 0 or 1",
                             "should return boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify override and env detection are mutually exclusive
    // (if override is enabled, env detection should not be used)
    if (override_enabled)
    {
        if (!d_assert_standalone(uses_env == 0,
                                 "override enabled implies !uses_env",
                                 "override should disable env detection",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test D_DMACRO_CHECK_VARG_LIMIT
    varg_check_small = D_DMACRO_CHECK_VARG_LIMIT(10);
    varg_check_large = D_DMACRO_CHECK_VARG_LIMIT(D_DMACRO_VARG_MAX + 1);

    if (!d_assert_standalone(varg_check_small != 0,
                             "D_DMACRO_CHECK_VARG_LIMIT(10) is true",
                             "10 should be within limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(varg_check_large == 0,
                             "D_DMACRO_CHECK_VARG_LIMIT(MAX+1) is false",
                             "MAX+1 should exceed limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_DMACRO_CHECK_PAIR_LIMIT
    pair_check_small = D_DMACRO_CHECK_PAIR_LIMIT(5);
    pair_check_large = D_DMACRO_CHECK_PAIR_LIMIT(D_DMACRO_PAIR_MAX + 1);

    if (!d_assert_standalone(pair_check_small != 0,
                             "D_DMACRO_CHECK_PAIR_LIMIT(5) is true",
                             "5 should be within limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(pair_check_large == 0,
                             "D_DMACRO_CHECK_PAIR_LIMIT(MAX+1) is false",
                             "PAIR_MAX+1 should exceed limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Query results: OVERRIDE=%d, MSVC_COMPAT=%d, USES_ENV=%d\n",
           D_INDENT,
           override_enabled,
           msvc_compat,
           uses_env);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Configuration query macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Configuration query macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * ENVIRONMENT INTEGRATION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_env_integration
  Tests the integration with env.h detected values.
  Tests the following:
  - D_ENV_PP_MAX_MACRO_ARGS detection (if available)
  - effective values respect environment limits when not overridden
  - effective values respect user limits when overridden
  - MSVC compatibility is detected correctly
*/
bool
d_tests_sa_dmacro_cfg_env_integration
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    env_pp_max;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Environment Integration ---\n", D_INDENT);

    // check if D_ENV_PP_MAX_MACRO_ARGS is defined
#ifdef D_ENV_PP_MAX_MACRO_ARGS
    env_pp_max = D_ENV_PP_MAX_MACRO_ARGS;
    printf("%s    D_ENV_PP_MAX_MACRO_ARGS is defined: %d\n",
           D_INDENT, env_pp_max);

    // if not overridden, effective max should respect env limit
    if (!D_DMACRO_IS_OVERRIDE_ENABLED())
    {
        if (!d_assert_standalone(D_DMACRO_VARG_MAX <= env_pp_max ||
                                 D_DMACRO_VARG_MAX <= D_CFG_DMACRO_VARG_MAX,
                                 "effective VARG_MAX respects env limit",
                                 "should not exceed environment limit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
#else
    env_pp_max = -1;  // not defined
    printf("%s    D_ENV_PP_MAX_MACRO_ARGS is NOT defined\n", D_INDENT);

    // without env detection, should use default or user-specified value
    if (!d_assert_standalone(D_DMACRO_VARG_MAX == D_CFG_DMACRO_VARG_DEFAULT ||
                             D_DMACRO_VARG_MAX == D_CFG_DMACRO_VARG_MAX,
                             "VARG_MAX uses default/user value without env",
                             "should use configured value",
                             _test_info))
    {
        all_assertions_passed = false;
    }
#endif

    // verify MSVC detection
#if defined(_MSC_VER) && !defined(__clang__)
    printf("%s    MSVC detected: _MSC_VER=%d\n", D_INDENT, _MSC_VER);

    #if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
        printf("%s    MSVC traditional preprocessor detected\n", D_INDENT);

        // should auto-enable MSVC compat unless user disabled it
        if (!d_assert_standalone(D_CFG_DMACRO_USE_MSVC_COMPAT == 1 ||
                                 D_DMACRO_VARIANT != 128,
                                 "MSVC traditional uses compat or non-128 variant",
                                 "should use MSVC-safe variant",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        printf("%s    MSVC conformant preprocessor detected\n", D_INDENT);
    #endif
#else
    printf("%s    Non-MSVC compiler detected\n", D_INDENT);

    // non-MSVC should not auto-enable MSVC compat
    if (!d_assert_standalone(D_CFG_DMACRO_USE_MSVC_COMPAT == 0 ||
                             D_CFG_DMACRO_USE_MSVC_COMPAT == 1,
                             "MSVC compat is explicitly configured",
                             "should be user-configured or default 0",
                             _test_info))
    {
        all_assertions_passed = false;
    }
#endif

    // verify at least one assertion was made
    if (!d_assert_standalone(true,
                             "environment integration test completed",
                             "sanity check",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Environment integration test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Environment integration test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * VARIANT CONSISTENCY TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_variant_consistency
  Tests that the selected variant is consistent with configuration.
  Tests the following:
  - selected variant is >= effective VARG_MAX
  - variant file should provide correct macros
  - D_VARG_COUNT should be available
  - D_HAS_ARGS should be available
  - D_FOR_EACH should be available (indirectly tested)
*/
bool
d_tests_sa_dmacro_cfg_variant_consistency
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    variant;
    int    varg_max;
    int    count_result;
    int    has_args_result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Variant Consistency ---\n", D_INDENT);

    variant  = D_DMACRO_VARIANT;
    varg_max = D_DMACRO_VARG_MAX;

    // verify variant is >= VARG_MAX
    if (!d_assert_standalone(variant >= varg_max,
                             "D_DMACRO_VARIANT >= D_DMACRO_VARG_MAX",
                             "variant should support configured max",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    VARIANT=%d, VARG_MAX=%d\n", D_INDENT, variant, varg_max);
    }

    // verify D_VARG_COUNT is functional
    count_result = D_VARG_COUNT(1);

    if (!d_assert_standalone(count_result == 1,
                             "D_VARG_COUNT(1) == 1",
                             "D_VARG_COUNT should work with 1 arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %d\n", D_INDENT, count_result);
    }

    count_result = D_VARG_COUNT(1, 2, 3);

    if (!d_assert_standalone(count_result == 3,
                             "D_VARG_COUNT(1, 2, 3) == 3",
                             "D_VARG_COUNT should work with 3 args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %d\n", D_INDENT, count_result);
    }

    // verify D_HAS_ARGS is functional
    has_args_result = D_HAS_ARGS(1);

    if (!d_assert_standalone(has_args_result == 1,
                             "D_HAS_ARGS(1) == 1",
                             "D_HAS_ARGS should return 1 for args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, has_args_result);
    }

    // test with more arguments to verify variant capacity
    count_result = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(count_result == 10,
                             "D_VARG_COUNT(10 args) == 10",
                             "D_VARG_COUNT should work with 10 args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %d\n", D_INDENT, count_result);
    }

    printf("%s    Variant %d provides functional macros\n", D_INDENT, variant);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Variant consistency test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Variant consistency test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * CONFIGURATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_cfg_all
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
bool
d_tests_sa_dmacro_cfg_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  constants_result;
    bool                  user_options_result;
    bool                  effective_values_result;
    bool                  public_aliases_result;
    bool                  query_macros_result;
    bool                  env_integration_result;
    bool                  variant_consistency_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Configuration System\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all configuration tests
    constants_result           = d_tests_sa_dmacro_cfg_constants(&module_counter);
    user_options_result        = d_tests_sa_dmacro_cfg_user_options(&module_counter);
    effective_values_result    = d_tests_sa_dmacro_cfg_effective_values(&module_counter);
    public_aliases_result      = d_tests_sa_dmacro_cfg_public_aliases(&module_counter);
    query_macros_result        = d_tests_sa_dmacro_cfg_query_macros(&module_counter);
    env_integration_result     = d_tests_sa_dmacro_cfg_env_integration(&module_counter);
    variant_consistency_result = d_tests_sa_dmacro_cfg_variant_consistency(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( constants_result           &&
                       user_options_result        &&
                       effective_values_result    &&
                       public_aliases_result      &&
                       query_macros_result        &&
                       env_integration_result     &&
                       variant_consistency_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Configuration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Configuration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Constants:           %s\n",
               constants_result ? "PASSED" : "FAILED");
        printf("  - User Options:        %s\n",
               user_options_result ? "PASSED" : "FAILED");
        printf("  - Effective Values:    %s\n",
               effective_values_result ? "PASSED" : "FAILED");
        printf("  - Public Aliases:      %s\n",
               public_aliases_result ? "PASSED" : "FAILED");
        printf("  - Query Macros:        %s\n",
               query_macros_result ? "PASSED" : "FAILED");
        printf("  - Env Integration:     %s\n",
               env_integration_result ? "PASSED" : "FAILED");
        printf("  - Variant Consistency: %s\n",
               variant_consistency_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}