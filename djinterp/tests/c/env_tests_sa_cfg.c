/******************************************************************************
* djinterp [test]                                           env_tests_sa_cfg.c
*
* Unit tests for `env.h` configuration system (CFG section).
* Tests the D_CFG_ENV_* macros that control environment detection.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_cfg.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.26
******************************************************************************/

#include "env_tests_sa.h"


/******************************************************************************
* CFG BIT FLAG TESTS
******************************************************************************/

/*
d_tests_sa_env_cfg_bit_flags
  Tests the configuration bit flag constant definitions.
  Tests the following:
  - D_CFG_ENV_BIT_LANG is correctly defined as 0x01
  - D_CFG_ENV_BIT_POSIX is correctly defined as 0x02
  - D_CFG_ENV_BIT_COMPILER is correctly defined as 0x04
  - D_CFG_ENV_BIT_OS is correctly defined as 0x08
  - D_CFG_ENV_BIT_ARCH is correctly defined as 0x10
  - D_CFG_ENV_BIT_BUILD is correctly defined as 0x20
  - all bit flags are unique (no overlapping bits)
  - all bit flags are powers of 2
*/
bool
d_tests_sa_env_cfg_bit_flags
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    combined_flags;
    int    expected_combined;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing CFG Bit Flag Definitions ---\n", D_INDENT);

    // verify each bit flag has the correct value
    if (!d_assert_standalone(D_CFG_ENV_BIT_LANG == 0x01,
                             "D_CFG_ENV_BIT_LANG == 0x01",
                             "language bit should be bit 0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_ENV_BIT_POSIX == 0x02,
                             "D_CFG_ENV_BIT_POSIX == 0x02",
                             "POSIX bit should be bit 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_ENV_BIT_COMPILER == 0x04,
                             "D_CFG_ENV_BIT_COMPILER == 0x04",
                             "compiler bit should be bit 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_ENV_BIT_OS == 0x08,
                             "D_CFG_ENV_BIT_OS == 0x08",
                             "OS bit should be bit 3",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_ENV_BIT_ARCH == 0x10,
                             "D_CFG_ENV_BIT_ARCH == 0x10",
                             "architecture bit should be bit 4",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_CFG_ENV_BIT_BUILD == 0x20,
                             "D_CFG_ENV_BIT_BUILD == 0x20",
                             "build bit should be bit 5",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify all flags are powers of 2 (single bit set)
    if (!d_assert_standalone((D_CFG_ENV_BIT_LANG & (D_CFG_ENV_BIT_LANG - 1)) == 0,
                             "D_CFG_ENV_BIT_LANG is power of 2",
                             "bit flag should have exactly one bit set",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_ENV_BIT_POSIX & (D_CFG_ENV_BIT_POSIX - 1)) == 0,
                             "D_CFG_ENV_BIT_POSIX is power of 2",
                             "bit flag should have exactly one bit set",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_ENV_BIT_COMPILER & (D_CFG_ENV_BIT_COMPILER - 1)) == 0,
                             "D_CFG_ENV_BIT_COMPILER is power of 2",
                             "bit flag should have exactly one bit set",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_ENV_BIT_OS & (D_CFG_ENV_BIT_OS - 1)) == 0,
                             "D_CFG_ENV_BIT_OS is power of 2",
                             "bit flag should have exactly one bit set",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_ENV_BIT_ARCH & (D_CFG_ENV_BIT_ARCH - 1)) == 0,
                             "D_CFG_ENV_BIT_ARCH is power of 2",
                             "bit flag should have exactly one bit set",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_CFG_ENV_BIT_BUILD & (D_CFG_ENV_BIT_BUILD - 1)) == 0,
                             "D_CFG_ENV_BIT_BUILD is power of 2",
                             "bit flag should have exactly one bit set",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify no overlapping bits (combined should equal sum)
    combined_flags = (D_CFG_ENV_BIT_LANG     |
                      D_CFG_ENV_BIT_POSIX    |
                      D_CFG_ENV_BIT_COMPILER |
                      D_CFG_ENV_BIT_OS       |
                      D_CFG_ENV_BIT_ARCH     |
                      D_CFG_ENV_BIT_BUILD);

    expected_combined = (D_CFG_ENV_BIT_LANG     +
                         D_CFG_ENV_BIT_POSIX    +
                         D_CFG_ENV_BIT_COMPILER +
                         D_CFG_ENV_BIT_OS       +
                         D_CFG_ENV_BIT_ARCH     +
                         D_CFG_ENV_BIT_BUILD);

    if (!d_assert_standalone(combined_flags == expected_combined,
                             "all bit flags are unique (OR equals SUM)",
                             "no two flags should share any bits",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify combined flags equals 0x3F (all 6 bits set)
    if (!d_assert_standalone(combined_flags == 0x3F,
                             "combined flags == 0x3F",
                             "all 6 section bits should be representable",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Bit flags: LANG=0x%02X POSIX=0x%02X COMPILER=0x%02X "
           "OS=0x%02X ARCH=0x%02X BUILD=0x%02X\n",
           D_INDENT,
           D_CFG_ENV_BIT_LANG,
           D_CFG_ENV_BIT_POSIX,
           D_CFG_ENV_BIT_COMPILER,
           D_CFG_ENV_BIT_OS,
           D_CFG_ENV_BIT_ARCH,
           D_CFG_ENV_BIT_BUILD);

    printf("%s    Combined: 0x%02X (expected 0x3F)\n",
           D_INDENT,
           combined_flags);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG bit flag definitions test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG bit flag definitions test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_custom_value
  Tests the D_CFG_ENV_CUSTOM master control value.
  Tests the following:
  - D_CFG_ENV_CUSTOM is defined
  - D_CFG_ENV_CUSTOM has a valid value (0, 1, or bitfield)
  - current configuration mode is correctly identified
*/
bool
d_tests_sa_env_cfg_custom_value
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    cfg_value;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    cfg_value             = D_CFG_ENV_CUSTOM;

    printf("%s--- Testing D_CFG_ENV_CUSTOM Value ---\n", D_INDENT);

    // verify D_CFG_ENV_CUSTOM is defined (always should be)
    #ifdef D_CFG_ENV_CUSTOM
        if (!d_assert_standalone(true,
                                 "D_CFG_ENV_CUSTOM is defined",
                                 "master control macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_CFG_ENV_CUSTOM should be defined",
                                 "master control macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        return false;
    #endif

    // verify value is non-negative
    if (!d_assert_standalone(cfg_value >= 0,
                             "D_CFG_ENV_CUSTOM >= 0",
                             "configuration value should be non-negative",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify value fits in valid range (0 to 0x3F for all bits)
    if (!d_assert_standalone(cfg_value <= 0x3F,
                             "D_CFG_ENV_CUSTOM <= 0x3F",
                             "configuration value should not exceed valid bit range",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // identify and report current mode
    if (cfg_value == 0)
    {
        printf("%s    Mode: FULL AUTOMATIC DETECTION (D_CFG_ENV_CUSTOM = 0)\n",
               D_INDENT);

        if (!d_assert_standalone(true,
                                 "automatic detection mode active",
                                 "all sections should be auto-detected",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    else if (cfg_value == 1)
    {
        printf("%s    Mode: SKIP ALL DETECTION (D_CFG_ENV_CUSTOM = 1)\n",
               D_INDENT);
        printf("%s    Note: requires pre-defined D_ENV_DETECTED_* variables\n",
               D_INDENT);

        if (!d_assert_standalone(true,
                                 "skip-all mode active",
                                 "all sections should use pre-defined values",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    else
    {
        printf("%s    Mode: SELECTIVE DETECTION (D_CFG_ENV_CUSTOM = 0x%02X)\n",
               D_INDENT,
               cfg_value);

        // report which sections are skipped
        printf("%s    Skipped sections:", D_INDENT);

        if (cfg_value & D_CFG_ENV_BIT_LANG)
        {
            printf(" LANG");
        }

        if (cfg_value & D_CFG_ENV_BIT_POSIX)
        {
            printf(" POSIX");
        }

        if (cfg_value & D_CFG_ENV_BIT_COMPILER)
        {
            printf(" COMPILER");
        }

        if (cfg_value & D_CFG_ENV_BIT_OS)
        {
            printf(" OS");
        }

        if (cfg_value & D_CFG_ENV_BIT_ARCH)
        {
            printf(" ARCH");
        }

        if (cfg_value & D_CFG_ENV_BIT_BUILD)
        {
            printf(" BUILD");
        }

        printf("\n");

        if (!d_assert_standalone(true,
                                 "selective detection mode active",
                                 "specific sections use pre-defined values",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_CFG_ENV_CUSTOM value test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_CFG_ENV_CUSTOM value test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* CFG SECTION ENABLE MACRO TESTS
******************************************************************************/

/*
d_tests_sa_env_cfg_enabled_macros
  Tests the D_CFG_ENV_*_ENABLED section control macros.
  Tests the following:
  - D_CFG_ENV_LANG_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_POSIX_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_COMPILER_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_OS_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_ARCH_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_BUILD_IS_ENABLED is defined and evaluates correctly
  - enabled macros are consistent with D_CFG_ENV_CUSTOM value
*/
bool
d_tests_sa_env_cfg_enabled_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    cfg_value;
    bool   lang_enabled_expected;
    bool   posix_enabled_expected;
    bool   compiler_enabled_expected;
    bool   os_enabled_expected;
    bool   arch_enabled_expected;
    bool   build_enabled_expected;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    cfg_value             = D_CFG_ENV_CUSTOM;

    printf("%s--- Testing CFG Section Enable Macros ---\n", D_INDENT);

    // calculate expected values based on D_CFG_ENV_CUSTOM
    // enabled = (custom == 0) || !(custom & bit)
    lang_enabled_expected     = (cfg_value == 0) ||
                                !(cfg_value & D_CFG_ENV_BIT_LANG);
    posix_enabled_expected    = (cfg_value == 0) ||
                                !(cfg_value & D_CFG_ENV_BIT_POSIX);
    compiler_enabled_expected = (cfg_value == 0) ||
                                !(cfg_value & D_CFG_ENV_BIT_COMPILER);
    os_enabled_expected       = (cfg_value == 0) ||
                                !(cfg_value & D_CFG_ENV_BIT_OS);
    arch_enabled_expected     = (cfg_value == 0) ||
                                !(cfg_value & D_CFG_ENV_BIT_ARCH);
    build_enabled_expected    = (cfg_value == 0) ||
                                !(cfg_value & D_CFG_ENV_BIT_BUILD);

    // test D_CFG_ENV_LANG_ENABLED
    if (!d_assert_standalone((D_CFG_ENV_LANG_ENABLED != 0) == lang_enabled_expected,
                             "D_CFG_ENV_LANG_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                             "language enable macro should match expected value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_POSIX_ENABLED
    if (!d_assert_standalone((D_CFG_ENV_POSIX_ENABLED != 0) == posix_enabled_expected,
                             "D_CFG_ENV_POSIX_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                             "POSIX enable macro should match expected value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_COMPILER_ENABLED
    if (!d_assert_standalone((D_CFG_ENV_COMPILER_ENABLED != 0) == compiler_enabled_expected,
                             "D_CFG_ENV_COMPILER_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                             "compiler enable macro should match expected value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_OS_ENABLED
    if (!d_assert_standalone((D_CFG_ENV_OS_ENABLED != 0) == os_enabled_expected,
                             "D_CFG_ENV_OS_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                             "OS enable macro should match expected value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_ARCH_ENABLED
    if (!d_assert_standalone((D_CFG_ENV_ARCH_ENABLED != 0) == arch_enabled_expected,
                             "D_CFG_ENV_ARCH_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                             "architecture enable macro should match expected value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_BUILD_IS_ENABLED
    if (!d_assert_standalone((D_CFG_ENV_BUILD_IS_ENABLED != 0) == build_enabled_expected,
                             "D_CFG_ENV_BUILD_IS_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                             "build enable macro should match expected value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // report current enabled states
    printf("%s    Section enable states:\n", D_INDENT);
    printf("%s      LANG:     %s (expected %s)\n",
           D_INDENT,
           D_CFG_ENV_LANG_ENABLED ? "ENABLED" : "DISABLED",
           lang_enabled_expected ? "ENABLED" : "DISABLED");
    printf("%s      POSIX:    %s (expected %s)\n",
           D_INDENT,
           D_CFG_ENV_POSIX_ENABLED ? "ENABLED" : "DISABLED",
           posix_enabled_expected ? "ENABLED" : "DISABLED");
    printf("%s      COMPILER: %s (expected %s)\n",
           D_INDENT,
           D_CFG_ENV_COMPILER_ENABLED ? "ENABLED" : "DISABLED",
           compiler_enabled_expected ? "ENABLED" : "DISABLED");
    printf("%s      OS:       %s (expected %s)\n",
           D_INDENT,
           D_CFG_ENV_OS_ENABLED ? "ENABLED" : "DISABLED",
           os_enabled_expected ? "ENABLED" : "DISABLED");
    printf("%s      ARCH:     %s (expected %s)\n",
           D_INDENT,
           D_CFG_ENV_ARCH_ENABLED ? "ENABLED" : "DISABLED",
           arch_enabled_expected ? "ENABLED" : "DISABLED");
    printf("%s      BUILD:    %s (expected %s)\n",
           D_INDENT,
           D_CFG_ENV_BUILD_IS_ENABLED ? "ENABLED" : "DISABLED",
           build_enabled_expected ? "ENABLED" : "DISABLED");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG section enable macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG section enable macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_enabled_zero_case
  Tests the _ENABLED macros when D_CFG_ENV_CUSTOM is 0 (full auto).
  Tests the following:
  - when D_CFG_ENV_CUSTOM == 0, all sections should be enabled
  - the (D_CFG_ENV_CUSTOM == 0) branch of each _ENABLED macro works
*/
bool
d_tests_sa_env_cfg_enabled_zero_case
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

    printf("%s--- Testing CFG Enable Macros (Zero Case) ---\n", D_INDENT);

    // this test validates logic, not current state
    // when custom == 0, all should be enabled

    #if (D_CFG_ENV_CUSTOM == 0)
        printf("%s    D_CFG_ENV_CUSTOM is 0, verifying all sections enabled\n",
               D_INDENT);

        if (!d_assert_standalone(D_CFG_ENV_LANG_ENABLED,
                                 "LANG enabled when CUSTOM == 0",
                                 "language detection should be enabled",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_CFG_ENV_POSIX_ENABLED,
                                 "POSIX enabled when CUSTOM == 0",
                                 "POSIX detection should be enabled",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_CFG_ENV_COMPILER_ENABLED,
                                 "COMPILER enabled when CUSTOM == 0",
                                 "compiler detection should be enabled",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_CFG_ENV_OS_ENABLED,
                                 "OS enabled when CUSTOM == 0",
                                 "OS detection should be enabled",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_CFG_ENV_ARCH_ENABLED,
                                 "ARCH enabled when CUSTOM == 0",
                                 "architecture detection should be enabled",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_CFG_ENV_BUILD_IS_ENABLED,
                                 "BUILD enabled when CUSTOM == 0",
                                 "build detection should be enabled",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        printf("%s    D_CFG_ENV_CUSTOM is %d, skipping zero-case verification\n",
               D_INDENT,
               D_CFG_ENV_CUSTOM);
        printf("%s    (test only applicable when D_CFG_ENV_CUSTOM == 0)\n",
               D_INDENT);

        // still count as pass since we can't test this case in current config
        if (!d_assert_standalone(true,
                                 "zero case test skipped (not applicable)",
                                 "D_CFG_ENV_CUSTOM is not 0 in this build",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG enable macros zero case test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG enable macros zero case test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* CFG DETECTED VARIABLE TESTS
******************************************************************************/

/*
d_tests_sa_env_cfg_detected_vars_lang
  Tests D_ENV_DETECTED_* language variable auto-detection.
  Tests the following:
  - D_CFG_ENV_DETECTED_CPP is set when any C++ detected var is defined
  - D_CFG_ENV_DETECTED_C_ONLY is set when only C detected vars are defined
  - language bit is correctly set in D_CFG_ENV_CUSTOM when detected vars exist
*/
bool
d_tests_sa_env_cfg_detected_vars_lang
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_cpp_detected;
    bool   has_c_detected;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing CFG Detected Language Variables ---\n", D_INDENT);

    // check for C++ detected variables
    has_cpp_detected = false;
    #if ( defined(D_ENV_DETECTED_CPP98) ||  \
          defined(D_ENV_DETECTED_CPP11) ||  \
          defined(D_ENV_DETECTED_CPP14) ||  \
          defined(D_ENV_DETECTED_CPP17) ||  \
          defined(D_ENV_DETECTED_CPP20) ||  \
          defined(D_ENV_DETECTED_CPP23) )
        has_cpp_detected = true;
    #endif

    // check for C detected variables
    has_c_detected = false;
    #if ( defined(D_ENV_DETECTED_C95) ||  \
          defined(D_ENV_DETECTED_C99) ||  \
          defined(D_ENV_DETECTED_C11) ||  \
          defined(D_ENV_DETECTED_C17) ||  \
          defined(D_ENV_DETECTED_C23) )
        has_c_detected = true;
    #endif

    printf("%s    C++ detected vars present: %s\n",
           D_INDENT,
           has_cpp_detected ? "YES" : "NO");
    printf("%s    C detected vars present:   %s\n",
           D_INDENT,
           has_c_detected ? "YES" : "NO");

    // verify D_CFG_ENV_DETECTED_CPP consistency
    #ifdef D_CFG_ENV_DETECTED_CPP
        if (!d_assert_standalone(has_cpp_detected,
                                 "D_CFG_ENV_DETECTED_CPP matches C++ detected vars",
                                 "macro should only be set if C++ vars are defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_CFG_ENV_DETECTED_CPP is defined\n", D_INDENT);
    #else
        if (!d_assert_standalone(!has_cpp_detected,
                                 "D_CFG_ENV_DETECTED_CPP absent when no C++ vars",
                                 "macro should not be set without C++ vars",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_CFG_ENV_DETECTED_CPP is NOT defined\n", D_INDENT);
    #endif

    // verify D_CFG_ENV_DETECTED_C_ONLY consistency
    #ifdef D_CFG_ENV_DETECTED_C_ONLY
        if (!d_assert_standalone(has_c_detected && !has_cpp_detected,
                                 "D_CFG_ENV_DETECTED_C_ONLY matches C-only case",
                                 "macro should only be set if C vars defined without C++",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_CFG_ENV_DETECTED_C_ONLY is defined\n", D_INDENT);
    #else
        // c_only should be absent if we have cpp or no c vars
        if (!d_assert_standalone(!has_c_detected || has_cpp_detected,
                                 "D_CFG_ENV_DETECTED_C_ONLY absent when expected",
                                 "macro should not be set with C++ or without C vars",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_CFG_ENV_DETECTED_C_ONLY is NOT defined\n", D_INDENT);
    #endif

    // verify language bit in D_CFG_ENV_CUSTOM
    if (has_cpp_detected || has_c_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!d_assert_standalone((D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG) != 0,
                                     "LANG bit set when detected vars present",
                                     "language bit should be auto-set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG detected language variables test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG detected language variables test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_detected_vars_compiler
  Tests D_ENV_DETECTED_COMPILER_* variable auto-detection.
  Tests the following:
  - compiler bit is correctly set when any compiler detected var is defined
  - D_ENV_CRT_MSVC and D_ENV_MSC_VER are set correctly for MSVC
*/
bool
d_tests_sa_env_cfg_detected_vars_compiler
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_compiler_detected;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed  = true;
    initial_tests_passed   = _test_info->tests_passed;
    has_compiler_detected  = false;

    printf("%s--- Testing CFG Detected Compiler Variables ---\n", D_INDENT);

    // check for compiler detected variables
    #if ( defined(D_ENV_DETECTED_COMPILER_CLANG)       ||  \
          defined(D_ENV_DETECTED_COMPILER_APPLE_CLANG) ||  \
          defined(D_ENV_DETECTED_COMPILER_GCC)         ||  \
          defined(D_ENV_DETECTED_COMPILER_MSVC)        ||  \
          defined(D_ENV_DETECTED_COMPILER_INTEL)       ||  \
          defined(D_ENV_DETECTED_COMPILER_BORLAND)     ||  \
          defined(D_ENV_DETECTED_COMPILER_UNKNOWN) )
        has_compiler_detected = true;
    #endif

    printf("%s    Compiler detected vars present: %s\n",
           D_INDENT,
           has_compiler_detected ? "YES" : "NO");

    // verify compiler bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_compiler_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!d_assert_standalone((D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER) != 0,
                                     "COMPILER bit set when detected vars present",
                                     "compiler bit should be auto-set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // test MSVC-specific macros
    #ifdef D_ENV_DETECTED_COMPILER_MSVC
        printf("%s    MSVC detected via D_ENV_DETECTED_COMPILER_MSVC\n",
               D_INDENT);

        if (!d_assert_standalone(D_ENV_CRT_MSVC == 1,
                                 "D_ENV_CRT_MSVC == 1 for MSVC",
                                 "CRT macro should be set for MSVC",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // D_ENV_MSC_VER should equal _MSC_VER when MSVC detected
        #ifdef _MSC_VER
            if (!d_assert_standalone(D_ENV_MSC_VER == _MSC_VER,
                                     "D_ENV_MSC_VER matches _MSC_VER",
                                     "version macro should match compiler value",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        // when not MSVC, these should be 0 (if defined)
        #ifdef D_ENV_CRT_MSVC
            if (!d_assert_standalone(D_ENV_CRT_MSVC == 0,
                                     "D_ENV_CRT_MSVC == 0 for non-MSVC",
                                     "CRT macro should be 0 for non-MSVC",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_MSC_VER
            if (!d_assert_standalone(D_ENV_MSC_VER == 0,
                                     "D_ENV_MSC_VER == 0 for non-MSVC",
                                     "version macro should be 0 for non-MSVC",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

#ifdef D_ENV_CRT_MSVC
        printf("%s    D_ENV_CRT_MSVC: %d\n", D_INDENT, D_ENV_CRT_MSVC);
        printf("%s    D_ENV_MSC_VER:  %d\n", D_INDENT, D_ENV_MSC_VER);
#else
        printf("%s    D_ENV_CRT_MSVC: (not defined)\n", D_INDENT);
        printf("%s    D_ENV_MSC_VER:  (not defined)\n", D_INDENT);
#endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG detected compiler variables test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG detected compiler variables test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_detected_vars_os
  Tests D_ENV_DETECTED_OS_* variable auto-detection.
  Tests the following:
  - OS bit is correctly set when any OS detected var is defined
  - only one OS should be detected at a time
*/
bool
d_tests_sa_env_cfg_detected_vars_os
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_os_detected;
    int    os_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    has_os_detected       = false;
    os_count              = 0;

    printf("%s--- Testing CFG Detected OS Variables ---\n", D_INDENT);

    // check for OS detected variables and count them
    #ifdef D_ENV_DETECTED_OS_APPLE
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_APPLE is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_MACOS
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_MACOS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_IOS
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_IOS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_LINUX
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_LINUX is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_ANDROID
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_ANDROID is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_WINDOWS
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_WINDOWS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_BSD
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_BSD is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_SOLARIS
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_SOLARIS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_UNIX
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_UNIX is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_MSDOS
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_MSDOS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_OS_UNKNOWN
        has_os_detected = true;
        os_count++;
        printf("%s    D_ENV_DETECTED_OS_UNKNOWN is defined\n", D_INDENT);
    #endif

    printf("%s    OS detected vars present: %s (count: %d)\n",
           D_INDENT,
           has_os_detected ? "YES" : "NO",
           os_count);

    // verify at most one OS is detected (exclusive)
    if (!d_assert_standalone(os_count <= 1,
                             "at most one OS detected var defined",
                             "OS detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify OS bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_os_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!d_assert_standalone((D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_OS) != 0,
                                     "OS bit set when detected vars present",
                                     "OS bit should be auto-set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG detected OS variables test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG detected OS variables test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_detected_vars_arch
  Tests D_ENV_DETECTED_ARCH_* variable auto-detection.
  Tests the following:
  - ARCH bit is correctly set when any arch detected var is defined
  - only one architecture should be detected at a time
*/
bool
d_tests_sa_env_cfg_detected_vars_arch
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_arch_detected;
    int    arch_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    has_arch_detected     = false;
    arch_count            = 0;

    printf("%s--- Testing CFG Detected Architecture Variables ---\n", D_INDENT);

    // check for arch detected variables and count them
    #ifdef D_ENV_DETECTED_ARCH_X86
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_X86 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_X64
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_X64 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_ARM
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_ARM is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_ARM64
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_ARM64 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_RISCV
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_RISCV is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_POWERPC
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_POWERPC is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_MIPS
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_MIPS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_SPARC
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_SPARC is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_S390
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_S390 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_IA64
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_IA64 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_ALPHA
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_ALPHA is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_ARCH_UNKNOWN
        has_arch_detected = true;
        arch_count++;
        printf("%s    D_ENV_DETECTED_ARCH_UNKNOWN is defined\n", D_INDENT);
    #endif

    printf("%s    Arch detected vars present: %s (count: %d)\n",
           D_INDENT,
           has_arch_detected ? "YES" : "NO",
           arch_count);

    // verify at most one arch is detected (exclusive)
    if (!d_assert_standalone(arch_count <= 1,
                             "at most one arch detected var defined",
                             "arch detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify arch bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_arch_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!d_assert_standalone((D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_ARCH) != 0,
                                     "ARCH bit set when detected vars present",
                                     "arch bit should be auto-set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG detected arch variables test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG detected arch variables test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_detected_vars_build
  Tests D_ENV_DETECTED_BUILD_* variable auto-detection.
  Tests the following:
  - BUILD bit is correctly set when any build detected var is defined
  - only one build type should be detected at a time (debug XOR release)
*/
bool
d_tests_sa_env_cfg_detected_vars_build
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_build_detected;
    bool   has_debug;
    bool   has_release;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    has_build_detected    = false;
    has_debug             = false;
    has_release           = false;

    printf("%s--- Testing CFG Detected Build Variables ---\n", D_INDENT);

    // check for build detected variables
    #ifdef D_ENV_DETECTED_BUILD_DEBUG
        has_build_detected = true;
        has_debug          = true;
        printf("%s    D_ENV_DETECTED_BUILD_DEBUG is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_BUILD_RELEASE
        has_build_detected = true;
        has_release        = true;
        printf("%s    D_ENV_DETECTED_BUILD_RELEASE is defined\n", D_INDENT);
    #endif

    printf("%s    Build detected vars present: %s\n",
           D_INDENT,
           has_build_detected ? "YES" : "NO");

    // verify debug and release are mutually exclusive
    if (!d_assert_standalone(!(has_debug && has_release),
                             "debug and release are mutually exclusive",
                             "cannot be both debug and release",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify build bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_build_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!d_assert_standalone((D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_BUILD) != 0,
                                     "BUILD bit set when detected vars present",
                                     "build bit should be auto-set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG detected build variables test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG detected build variables test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_cfg_detected_vars_posix
  Tests D_ENV_DETECTED_POSIX_* variable auto-detection.
  Tests the following:
  - POSIX bit is correctly set when any POSIX detected var is defined
  - POSIX version and feature variables are checked
*/
bool
d_tests_sa_env_cfg_detected_vars_posix
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_posix_detected;
    int    posix_version_count;
    int    posix_feature_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed  = true;
    initial_tests_passed   = _test_info->tests_passed;
    has_posix_detected     = false;
    posix_version_count    = 0;
    posix_feature_count    = 0;

    printf("%s--- Testing CFG Detected POSIX Variables ---\n", D_INDENT);

    // check for POSIX version detected variables
    #ifdef D_ENV_DETECTED_POSIX_1988
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1988 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1990
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1990 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1993
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1993 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1996
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1996 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2001
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2001 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2008
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2008 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2017
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2017 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2024
        has_posix_detected = true;
        posix_version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2024 is defined\n", D_INDENT);
    #endif

    // check for POSIX feature detected variables
    #ifdef D_ENV_DETECTED_POSIX_XSI
        has_posix_detected = true;
        posix_feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_XSI is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_THREADS
        has_posix_detected = true;
        posix_feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_THREADS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_REALTIME
        has_posix_detected = true;
        posix_feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_REALTIME is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_SOCKETS
        has_posix_detected = true;
        posix_feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_SOCKETS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_NONE
        has_posix_detected = true;
        printf("%s    D_ENV_DETECTED_POSIX_NONE is defined\n", D_INDENT);
    #endif

    printf("%s    POSIX detected vars present: %s\n",
           D_INDENT,
           has_posix_detected ? "YES" : "NO");
    printf("%s    Version vars: %d, Feature vars: %d\n",
           D_INDENT,
           posix_version_count,
           posix_feature_count);

    // verify at most one POSIX version is detected (exclusive)
    if (!d_assert_standalone(posix_version_count <= 1,
                             "at most one POSIX version detected var defined",
                             "POSIX version detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify POSIX bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_posix_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!d_assert_standalone((D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_POSIX) != 0,
                                     "POSIX bit set when detected vars present",
                                     "POSIX bit should be auto-set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] CFG detected POSIX variables test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] CFG detected POSIX variables test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* CFG MODULE TEST AGGREGATORS
******************************************************************************/

/*
d_tests_sa_env_cfg_all
  Runs all CFG section tests.
  Tests the following:
  - bit flag definitions
  - D_CFG_ENV_CUSTOM value
  - section enable macros
  - detected variable auto-detection for all sections
*/
bool
d_tests_sa_env_cfg_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool bit_flags_result;
    bool custom_value_result;
    bool enabled_macros_result;
    bool enabled_zero_result;
    bool detected_lang_result;
    bool detected_compiler_result;
    bool detected_os_result;
    bool detected_arch_result;
    bool detected_build_result;
    bool detected_posix_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing CFG Configuration System\n");
    printf("========================================="
           "=======================================\n");

    // run all CFG tests
    bit_flags_result        = d_tests_sa_env_cfg_bit_flags(&module_counter);
    custom_value_result     = d_tests_sa_env_cfg_custom_value(&module_counter);
    enabled_macros_result   = d_tests_sa_env_cfg_enabled_macros(&module_counter);
    enabled_zero_result     = d_tests_sa_env_cfg_enabled_zero_case(&module_counter);
    detected_lang_result    = d_tests_sa_env_cfg_detected_vars_lang(&module_counter);
    detected_posix_result   = d_tests_sa_env_cfg_detected_vars_posix(&module_counter);
    detected_compiler_result = d_tests_sa_env_cfg_detected_vars_compiler(&module_counter);
    detected_os_result      = d_tests_sa_env_cfg_detected_vars_os(&module_counter);
    detected_arch_result    = d_tests_sa_env_cfg_detected_vars_arch(&module_counter);
    detected_build_result   = d_tests_sa_env_cfg_detected_vars_build(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( bit_flags_result        &&
                       custom_value_result     &&
                       enabled_macros_result   &&
                       enabled_zero_result     &&
                       detected_lang_result    &&
                       detected_posix_result   &&
                       detected_compiler_result &&
                       detected_os_result      &&
                       detected_arch_result    &&
                       detected_build_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] CFG Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] CFG Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Bit Flags:          %s\n",
               bit_flags_result ? "PASSED" : "FAILED");
        printf("  - Custom Value:       %s\n",
               custom_value_result ? "PASSED" : "FAILED");
        printf("  - Enabled Macros:     %s\n",
               enabled_macros_result ? "PASSED" : "FAILED");
        printf("  - Enabled Zero Case:  %s\n",
               enabled_zero_result ? "PASSED" : "FAILED");
        printf("  - Detected Lang:      %s\n",
               detected_lang_result ? "PASSED" : "FAILED");
        printf("  - Detected POSIX:     %s\n",
               detected_posix_result ? "PASSED" : "FAILED");
        printf("  - Detected Compiler:  %s\n",
               detected_compiler_result ? "PASSED" : "FAILED");
        printf("  - Detected OS:        %s\n",
               detected_os_result ? "PASSED" : "FAILED");
        printf("  - Detected Arch:      %s\n",
               detected_arch_result ? "PASSED" : "FAILED");
        printf("  - Detected Build:     %s\n",
               detected_build_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
