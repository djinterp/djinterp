/*******************************************************************************
* djinterp [test]                                                env_tests_cfg.c
*
* Unit tests for `env.h` configuration system (CFG section).
* Tests the D_CFG_ENV_* macros that control environment detection.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_cfg.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.26
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "env_tests.h"


/******************************************************************************
* CFG BIT FLAG TESTS
******************************************************************************/

/*
d_tests_env_cfg_bit_flags
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
bool d_tests_env_cfg_bit_flags(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    combined_flags;
    int    expected_combined;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_bit_flags");

    all_assertions_passed = true;

    // verify each bit flag has the correct value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_LANG == 0x01",
                          D_CFG_ENV_BIT_LANG == 0x01,
                          "language bit should be bit 0"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_POSIX == 0x02",
                          D_CFG_ENV_BIT_POSIX == 0x02,
                          "POSIX bit should be bit 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_COMPILER == 0x04",
                          D_CFG_ENV_BIT_COMPILER == 0x04,
                          "compiler bit should be bit 2"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_OS == 0x08",
                          D_CFG_ENV_BIT_OS == 0x08,
                          "OS bit should be bit 3"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_ARCH == 0x10",
                          D_CFG_ENV_BIT_ARCH == 0x10,
                          "architecture bit should be bit 4"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_BUILD == 0x20",
                          D_CFG_ENV_BIT_BUILD == 0x20,
                          "build bit should be bit 5"))
    {
        all_assertions_passed = false;
    }

    // verify all flags are powers of 2 (single bit set)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_LANG is power of 2",
                          (D_CFG_ENV_BIT_LANG & (D_CFG_ENV_BIT_LANG - 1)) == 0,
                          "bit flag should have exactly one bit set"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_POSIX is power of 2",
                          (D_CFG_ENV_BIT_POSIX & (D_CFG_ENV_BIT_POSIX - 1)) == 0,
                          "bit flag should have exactly one bit set"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_COMPILER is power of 2",
                          (D_CFG_ENV_BIT_COMPILER & (D_CFG_ENV_BIT_COMPILER - 1)) == 0,
                          "bit flag should have exactly one bit set"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_OS is power of 2",
                          (D_CFG_ENV_BIT_OS & (D_CFG_ENV_BIT_OS - 1)) == 0,
                          "bit flag should have exactly one bit set"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_ARCH is power of 2",
                          (D_CFG_ENV_BIT_ARCH & (D_CFG_ENV_BIT_ARCH - 1)) == 0,
                          "bit flag should have exactly one bit set"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BIT_BUILD is power of 2",
                          (D_CFG_ENV_BIT_BUILD & (D_CFG_ENV_BIT_BUILD - 1)) == 0,
                          "bit flag should have exactly one bit set"))
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "all bit flags are unique (OR equals SUM)",
                          combined_flags == expected_combined,
                          "no two flags should share any bits"))
    {
        all_assertions_passed = false;
    }

    // verify combined flags equals 0x3F (all 6 bits set)
    if (!D_TEST_CHECK_MSG(_suite,
                          "combined flags == 0x3F",
                          combined_flags == 0x3F,
                          "all 6 section bits should be representable"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_env_cfg_custom_value
  Tests the D_CFG_ENV_CUSTOM master control value.
  Tests the following:
  - D_CFG_ENV_CUSTOM is defined
  - D_CFG_ENV_CUSTOM has a valid value (0, 1, or bitfield)
  - current configuration mode is correctly identified
*/
bool d_tests_env_cfg_custom_value(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    cfg_value;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_custom_value");

    all_assertions_passed = true;
    cfg_value             = D_CFG_ENV_CUSTOM;

    // verify D_CFG_ENV_CUSTOM is defined (always should be)
    #ifdef D_CFG_ENV_CUSTOM
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_CFG_ENV_CUSTOM is defined",
                              true,
                              "master control macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_CFG_ENV_CUSTOM should be defined",
                              false,
                              "master control macro must always be defined"))
        {
            all_assertions_passed = false;
        }

        return false;
    #endif

    // verify value is non-negative
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_CUSTOM >= 0",
                          cfg_value >= 0,
                          "configuration value should be non-negative"))
    {
        all_assertions_passed = false;
    }

    // verify value fits in valid range (0 to 0x3F for all bits)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_CUSTOM <= 0x3F",
                          cfg_value <= 0x3F,
                          "configuration value should not exceed valid bit range"))
    {
        all_assertions_passed = false;
    }

    // identify and report current mode
    if (cfg_value == 0)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "automatic detection mode active",
                              true,
                              "all sections should be auto-detected"))
        {
            all_assertions_passed = false;
        }
    }
    else if (cfg_value == 1)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "skip-all mode active",
                              true,
                              "all sections should use pre-defined values"))
        {
            all_assertions_passed = false;
        }
    }
    else
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "selective detection mode active",
                              true,
                              "specific sections use pre-defined values"))
        {
            all_assertions_passed = false;
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
* CFG SECTION ENABLE MACRO TESTS
******************************************************************************/

/*
d_tests_env_cfg_enabled_macros
  Tests the D_CFG_ENV_*_ENABLED section control macros.
  Tests the following:
  - D_CFG_ENV_LANG_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_POSIX_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_COMPILER_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_OS_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_ARCH_ENABLED is defined and evaluates correctly
  - D_CFG_ENV_BUILD_ENABLED is defined and evaluates correctly
  - enabled macros are consistent with D_CFG_ENV_CUSTOM value
*/
bool d_tests_env_cfg_enabled_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    cfg_value;
    bool   lang_enabled_expected;
    bool   posix_enabled_expected;
    bool   compiler_enabled_expected;
    bool   os_enabled_expected;
    bool   arch_enabled_expected;
    bool   build_enabled_expected;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_enabled_macros");

    all_assertions_passed = true;
    cfg_value             = D_CFG_ENV_CUSTOM;

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
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_LANG_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                          (D_CFG_ENV_LANG_ENABLED != 0) == lang_enabled_expected,
                          "language enable macro should match expected value"))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_POSIX_ENABLED
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_POSIX_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                          (D_CFG_ENV_POSIX_ENABLED != 0) == posix_enabled_expected,
                          "POSIX enable macro should match expected value"))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_COMPILER_ENABLED
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_COMPILER_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                          (D_CFG_ENV_COMPILER_ENABLED != 0) == compiler_enabled_expected,
                          "compiler enable macro should match expected value"))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_OS_ENABLED
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_OS_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                          (D_CFG_ENV_OS_ENABLED != 0) == os_enabled_expected,
                          "OS enable macro should match expected value"))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_ARCH_ENABLED
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_ARCH_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                          (D_CFG_ENV_ARCH_ENABLED != 0) == arch_enabled_expected,
                          "architecture enable macro should match expected value"))
    {
        all_assertions_passed = false;
    }

    // test D_CFG_ENV_BUILD_ENABLED
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CFG_ENV_BUILD_ENABLED is consistent with D_CFG_ENV_CUSTOM",
                          (D_CFG_ENV_BUILD_ENABLED != 0) == build_enabled_expected,
                          "build enable macro should match expected value"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_env_cfg_enabled_zero_case
  Tests the _ENABLED macros when D_CFG_ENV_CUSTOM is 0 (full auto).
  Tests the following:
  - when D_CFG_ENV_CUSTOM == 0, all sections should be enabled
  - the (D_CFG_ENV_CUSTOM == 0) branch of each _ENABLED macro works
*/
bool d_tests_env_cfg_enabled_zero_case(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_enabled_zero_case");

    all_assertions_passed = true;

    // this test validates logic, not current state
    // when custom == 0, all should be enabled

    #if (D_CFG_ENV_CUSTOM == 0)

        if (!D_TEST_CHECK_MSG(_suite,
                              "LANG enabled when CUSTOM == 0",
                              D_CFG_ENV_LANG_ENABLED,
                              "language detection should be enabled"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "POSIX enabled when CUSTOM == 0",
                              D_CFG_ENV_POSIX_ENABLED,
                              "POSIX detection should be enabled"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "COMPILER enabled when CUSTOM == 0",
                              D_CFG_ENV_COMPILER_ENABLED,
                              "compiler detection should be enabled"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "OS enabled when CUSTOM == 0",
                              D_CFG_ENV_OS_ENABLED,
                              "OS detection should be enabled"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "ARCH enabled when CUSTOM == 0",
                              D_CFG_ENV_ARCH_ENABLED,
                              "architecture detection should be enabled"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "BUILD enabled when CUSTOM == 0",
                              D_CFG_ENV_BUILD_ENABLED,
                              "build detection should be enabled"))
        {
            all_assertions_passed = false;
        }
    #else

        // still count as pass since we can't test this case in current config
        if (!D_TEST_CHECK_MSG(_suite,
                              "zero case test skipped (not applicable)",
                              true,
                              "D_CFG_ENV_CUSTOM is not 0 in this build"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* CFG DETECTED VARIABLE TESTS
******************************************************************************/

/*
d_tests_env_cfg_detected_vars_lang
  Tests D_ENV_DETECTED_* language variable auto-detection.
  Tests the following:
  - D_CFG_ENV_DETECTED_CPP is set when any C++ detected var is defined
  - D_CFG_ENV_DETECTED_C_ONLY is set when only C detected vars are defined
  - language bit is correctly set in D_CFG_ENV_CUSTOM when detected vars exist
*/
bool d_tests_env_cfg_detected_vars_lang(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_cpp_detected;
    bool   has_c_detected;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_detected_vars_lang");

    all_assertions_passed = true;

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


    // verify D_CFG_ENV_DETECTED_CPP consistency
    #ifdef D_CFG_ENV_DETECTED_CPP
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_CFG_ENV_DETECTED_CPP matches C++ detected vars",
                              has_cpp_detected,
                              "macro should only be set if C++ vars are defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_CFG_ENV_DETECTED_CPP absent when no C++ vars",
                              !has_cpp_detected,
                              "macro should not be set without C++ vars"))
        {
            all_assertions_passed = false;
        }

    #endif

    // verify D_CFG_ENV_DETECTED_C_ONLY consistency
    #ifdef D_CFG_ENV_DETECTED_C_ONLY
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_CFG_ENV_DETECTED_C_ONLY matches C-only case",
                              has_c_detected && !has_cpp_detected,
                              "macro should only be set if C vars defined without C++"))
        {
            all_assertions_passed = false;
        }

    #else
        // c_only should be absent if we have cpp or no c vars
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_CFG_ENV_DETECTED_C_ONLY absent when expected",
                              !has_c_detected || has_cpp_detected,
                              "macro should not be set with C++ or without C vars"))
        {
            all_assertions_passed = false;
        }

    #endif

    // verify language bit in D_CFG_ENV_CUSTOM
    if (has_cpp_detected || has_c_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "LANG bit set when detected vars present",
                                  (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG) != 0,
                                  "language bit should be auto-set"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    return all_assertions_passed;
}


/*
d_tests_env_cfg_detected_vars_compiler
  Tests D_ENV_DETECTED_COMPILER_* variable auto-detection.
  Tests the following:
  - compiler bit is correctly set when any compiler detected var is defined
  - D_ENV_CRT_MSVC and D_ENV_MSC_VER are set correctly for MSVC
*/
bool d_tests_env_cfg_detected_vars_compiler(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_compiler_detected;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_detected_vars_compiler");

    all_assertions_passed  = true;
    has_compiler_detected  = false;

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


    // simulating nothing, the section runs its own detection
    if (!D_TEST_CHECK_MSG(_suite,
                          "detection runs unless D_CFG_ENV_CUSTOM simulates",
                          (D_CFG_ENV_CUSTOM != 0) ||
                              (D_CFG_ENV_COMPILER_ENABLED == 1),
                          "D_CFG_ENV_COMPILER_ENABLED should be 1"))
    {
        all_assertions_passed = false;
    }

    // verify compiler bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_compiler_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "COMPILER bit set when detected vars present",
                                  (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER) != 0,
                                  "compiler bit should be auto-set"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    // test MSVC-specific macros
    #ifdef D_ENV_DETECTED_COMPILER_MSVC

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_CRT_MSVC == 1 for MSVC",
                              D_ENV_CRT_MSVC == 1,
                              "CRT macro should be set for MSVC"))
        {
            all_assertions_passed = false;
        }

        // D_ENV_MSC_VER should equal _MSC_VER when MSVC detected
        #ifdef _MSC_VER
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_MSC_VER matches _MSC_VER",
                                  D_ENV_MSC_VER == _MSC_VER,
                                  "version macro should match compiler value"))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        // when not MSVC, these should be 0 (if defined)
        #ifdef D_ENV_CRT_MSVC
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_CRT_MSVC == 0 for non-MSVC",
                                  D_ENV_CRT_MSVC == 0,
                                  "CRT macro should be 0 for non-MSVC"))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_MSC_VER
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_MSC_VER == 0 for non-MSVC",
                                  D_ENV_MSC_VER == 0,
                                  "version macro should be 0 for non-MSVC"))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_cfg_detected_vars_os
  Tests D_ENV_DETECTED_OS_* variable auto-detection.
  Tests the following:
  - OS bit is correctly set when any OS detected var is defined
  - only one OS should be detected at a time
*/
bool d_tests_env_cfg_detected_vars_os(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_os_detected;
    int    os_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_detected_vars_os");

    all_assertions_passed = true;
    has_os_detected       = false;
    os_count              = 0;

    // check for OS detected variables and count them
    #ifdef D_ENV_DETECTED_OS_APPLE
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_MACOS
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_IOS
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_LINUX
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_ANDROID
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_WINDOWS
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_BSD
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_SOLARIS
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_UNIX
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_MSDOS
        has_os_detected = true;
        os_count++;
    #endif

    #ifdef D_ENV_DETECTED_OS_UNKNOWN
        has_os_detected = true;
        os_count++;
    #endif


    // verify at most one OS is detected (exclusive)
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one OS detected var defined",
                          os_count <= 1,
                          "OS detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }

    // verify OS bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_os_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "OS bit set when detected vars present",
                                  (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_OS) != 0,
                                  "OS bit should be auto-set"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    return all_assertions_passed;
}


/*
d_tests_env_cfg_detected_vars_arch
  Tests D_ENV_DETECTED_ARCH_* variable auto-detection.
  Tests the following:
  - ARCH bit is correctly set when any arch detected var is defined
  - only one architecture should be detected at a time
*/
bool d_tests_env_cfg_detected_vars_arch(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_arch_detected;
    int    arch_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_detected_vars_arch");

    all_assertions_passed = true;
    has_arch_detected     = false;
    arch_count            = 0;

    // check for arch detected variables and count them
    #ifdef D_ENV_DETECTED_ARCH_X86
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_X64
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_ARM
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_ARM64
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_RISCV
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_POWERPC
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_MIPS
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_SPARC
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_S390
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_IA64
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_ALPHA
        has_arch_detected = true;
        arch_count++;
    #endif

    #ifdef D_ENV_DETECTED_ARCH_UNKNOWN
        has_arch_detected = true;
        arch_count++;
    #endif


    // verify at most one arch is detected (exclusive)
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one arch detected var defined",
                          arch_count <= 1,
                          "arch detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }

    // verify arch bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_arch_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "ARCH bit set when detected vars present",
                                  (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_ARCH) != 0,
                                  "arch bit should be auto-set"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    return all_assertions_passed;
}


/*
d_tests_env_cfg_detected_vars_build
  Tests D_ENV_DETECTED_BUILD_* variable auto-detection.
  Tests the following:
  - BUILD bit is correctly set when any build detected var is defined
  - only one build type should be detected at a time (debug XOR release)
*/
bool d_tests_env_cfg_detected_vars_build(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_build_detected;
    bool   has_debug;
    bool   has_release;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_detected_vars_build");

    all_assertions_passed = true;
    has_build_detected    = false;
    has_debug             = false;
    has_release           = false;

    // check for build detected variables
    #ifdef D_ENV_DETECTED_BUILD_DEBUG
        has_build_detected = true;
        has_debug          = true;
    #endif

    #ifdef D_ENV_DETECTED_BUILD_RELEASE
        has_build_detected = true;
        has_release        = true;
    #endif


    // verify debug and release are mutually exclusive
    if (!D_TEST_CHECK_MSG(_suite,
                          "debug and release are mutually exclusive",
                          !(has_debug && has_release),
                          "cannot be both debug and release"))
    {
        all_assertions_passed = false;
    }

    // verify build bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_build_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "BUILD bit set when detected vars present",
                                  (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_BUILD) != 0,
                                  "build bit should be auto-set"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    return all_assertions_passed;
}


/*
d_tests_env_cfg_detected_vars_posix
  Tests D_ENV_DETECTED_POSIX_* variable auto-detection.
  Tests the following:
  - POSIX bit is correctly set when any POSIX detected var is defined
  - POSIX version and feature variables are checked
*/
bool d_tests_env_cfg_detected_vars_posix(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_posix_detected;
    int    posix_version_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_cfg_detected_vars_posix");

    all_assertions_passed  = true;
    has_posix_detected     = false;
    posix_version_count    = 0;

    // check for POSIX version detected variables
    #ifdef D_ENV_DETECTED_POSIX_1988
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1990
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1993
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1996
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2001
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2008
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2017
        has_posix_detected = true;
        posix_version_count++;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2024
        has_posix_detected = true;
        posix_version_count++;
    #endif

    // check for POSIX feature detected variables
    #ifdef D_ENV_DETECTED_POSIX_XSI
        has_posix_detected = true;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_THREADS
        has_posix_detected = true;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_REALTIME
        has_posix_detected = true;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_SOCKETS
        has_posix_detected = true;
    #endif

    #ifdef D_ENV_DETECTED_POSIX_NONE
        has_posix_detected = true;
    #endif


    // verify at most one POSIX version is detected (exclusive)
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one POSIX version detected var defined",
                          posix_version_count <= 1,
                          "POSIX version detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }

    // verify POSIX bit in D_CFG_ENV_CUSTOM if detected vars present
    if (has_posix_detected)
    {
        #if (D_CFG_ENV_CUSTOM > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "POSIX bit set when detected vars present",
                                  (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_POSIX) != 0,
                                  "POSIX bit should be auto-set"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    return all_assertions_passed;
}


/******************************************************************************
* CFG MODULE TEST AGGREGATORS
******************************************************************************/

/*
d_tests_env_cfg_all
  Runs all CFG section tests.
  Tests the following:
  - bit flag definitions
  - D_CFG_ENV_CUSTOM value
  - section enable macros
  - detected variable auto-detection for all sections
*/
bool d_tests_env_cfg_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_cfg_all");

    d_tests_env_cfg_bit_flags(_suite);
    d_tests_env_cfg_custom_value(_suite);
    d_tests_env_cfg_enabled_macros(_suite);
    d_tests_env_cfg_enabled_zero_case(_suite);
    d_tests_env_cfg_detected_vars_lang(_suite);
    d_tests_env_cfg_detected_vars_posix(_suite);
    d_tests_env_cfg_detected_vars_compiler(_suite);
    d_tests_env_cfg_detected_vars_os(_suite);
    d_tests_env_cfg_detected_vars_arch(_suite);
    d_tests_env_cfg_detected_vars_build(_suite);

    return true;
}
