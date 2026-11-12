/*******************************************************************************
* djinterp [c][env]                                         env_tests_compiler.c
*
* Unit tests for `env.h` compiler detection section (Section IV).
* Tests compiler detection, version info, version comparison, VA_OPT detection,
* and preprocessor limits.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_compiler.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.26
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <string.h>
// djinterp
#include "env_tests.h"


/******************************************************************************
* COMPILER DETECTION TESTS
******************************************************************************/

/*
d_tests_env_compiler_detection_flags
  Tests that exactly one compiler flag is defined.
  Tests the following:
  - exactly one of CLANG, GCC, MSVC, INTEL, BORLAND, UNKNOWN is defined
  - compiler flags are mutually exclusive
  - D_ENV_COMPILER_APPLE_CLANG implies D_ENV_COMPILER_CLANG
*/
bool d_tests_env_compiler_detection_flags(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    compiler_flag_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_detection_flags");

    all_assertions_passed = true;
    compiler_flag_count   = 0;

    // count compiler flags
    #ifdef D_ENV_COMPILER_CLANG
        compiler_flag_count++;

    #endif

    #ifdef D_ENV_COMPILER_GCC
        compiler_flag_count++;
    #endif

    #ifdef D_ENV_COMPILER_MSVC
        compiler_flag_count++;
    #endif

    #ifdef D_ENV_COMPILER_INTEL
        compiler_flag_count++;
    #endif

    #ifdef D_ENV_COMPILER_BORLAND
        compiler_flag_count++;
    #endif

    #ifdef D_ENV_COMPILER_UNKNOWN
        compiler_flag_count++;
    #endif

    // verify exactly one compiler flag (Clang counts as one even with Apple)
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one compiler flag defined",
                          compiler_flag_count == 1,
                          "compiler detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "found %d compiler flags",
                                compiler_flag_count);
    }

    // verify Apple Clang implies Clang
    #ifdef D_ENV_COMPILER_APPLE_CLANG
        #ifndef D_ENV_COMPILER_CLANG
            if (!D_TEST_CHECK_MSG(_suite,
                                  "APPLE_CLANG should imply CLANG",
                                  false,
                                  "Apple Clang is a variant of Clang"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "APPLE_CLANG implies CLANG",
                                  true,
                                  "Apple Clang correctly implies Clang"))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_compiler_name_macros
  Tests compiler name macros.
  Tests the following:
  - D_ENV_COMPILER_NAME is defined and non-empty
  - D_ENV_COMPILER_FULL_NAME is defined and non-empty
  - names are consistent with detected compiler flag
*/
bool d_tests_env_compiler_name_macros(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* name;
    const char* full_name;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_name_macros");

    all_assertions_passed = true;

    // verify D_ENV_COMPILER_NAME is defined
    #ifdef D_ENV_COMPILER_NAME
        name = D_ENV_COMPILER_NAME;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_NAME is defined",
                              true,
                              "compiler name must be defined"))
        {
            all_assertions_passed = false;
        }

        // verify non-empty
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_NAME is non-empty",
                              (name != NULL) && (name[0] != '\0'),
                              "name should be non-empty string"))
        {
            all_assertions_passed = false;
        }
    #else
        name = NULL;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_NAME should be defined",
                              false,
                              "compiler name must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_FULL_NAME is defined
    #ifdef D_ENV_COMPILER_FULL_NAME
        full_name = D_ENV_COMPILER_FULL_NAME;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_FULL_NAME is defined",
                              true,
                              "compiler full name must be defined"))
        {
            all_assertions_passed = false;
        }

        // verify non-empty
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_FULL_NAME is non-empty",
                              (full_name != NULL) && (full_name[0] != '\0'),
                              "full name should be non-empty string"))
        {
            all_assertions_passed = false;
        }
    #else
        full_name = NULL;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_FULL_NAME should be defined",
                              false,
                              "compiler full name must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify consistency with compiler flag
    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_GCC)
        if (!D_TEST_CHECK_MSG(_suite,
                              "NAME is \"GCC\" for GCC compiler",
                              strcmp(D_ENV_COMPILER_NAME, "GCC") == 0,
                              "name should match compiler"))
        {
            all_assertions_passed = false;
        }
    #endif

    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_MSVC)
        if (!D_TEST_CHECK_MSG(_suite,
                              "NAME is \"MSVC\" for MSVC compiler",
                              strcmp(D_ENV_COMPILER_NAME, "MSVC") == 0,
                              "name should match compiler"))
        {
            all_assertions_passed = false;
        }
    #endif

    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_CLANG)
        #ifdef D_ENV_COMPILER_APPLE_CLANG
            if (!D_TEST_CHECK_MSG(_suite,
                                  "NAME contains \"Clang\" for Apple Clang",
                                  strstr(D_ENV_COMPILER_NAME, "Clang") != NULL,
                                  "name should match compiler"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "NAME is \"Clang\" for Clang compiler",
                                  strcmp(D_ENV_COMPILER_NAME, "Clang") == 0,
                                  "name should match compiler"))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* COMPILER VERSION TESTS
******************************************************************************/

/*
d_tests_env_compiler_version_macros
  Tests compiler version macros.
  Tests the following:
  - D_ENV_COMPILER_MAJOR is defined and non-negative
  - D_ENV_COMPILER_MINOR is defined and non-negative
  - D_ENV_COMPILER_PATCHLEVEL is defined and non-negative
  - D_ENV_COMPILER_VERSION_STRING is defined
*/
bool d_tests_env_compiler_version_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    major_ver;
    int    minor_ver;
    int    patch_ver;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_version_macros");

    all_assertions_passed = true;

    // verify D_ENV_COMPILER_MAJOR
    #ifdef D_ENV_COMPILER_MAJOR
        major_ver = D_ENV_COMPILER_MAJOR;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_MAJOR is defined",
                              true,
                              "major version must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_MAJOR >= 0",
                              major_ver >= 0,
                              "major version should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        major_ver = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_MAJOR should be defined",
                              false,
                              "major version must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_MINOR
    #ifdef D_ENV_COMPILER_MINOR
        minor_ver = D_ENV_COMPILER_MINOR;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_MINOR is defined",
                              true,
                              "minor version must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_MINOR >= 0",
                              minor_ver >= 0,
                              "minor version should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        minor_ver = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_MINOR should be defined",
                              false,
                              "minor version must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_PATCHLEVEL
    #ifdef D_ENV_COMPILER_PATCHLEVEL
        patch_ver = D_ENV_COMPILER_PATCHLEVEL;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_PATCHLEVEL is defined",
                              true,
                              "patch level must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_PATCHLEVEL >= 0",
                              patch_ver >= 0,
                              "patch level should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        patch_ver = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_PATCHLEVEL should be defined",
                              false,
                              "patch level must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_VERSION_STRING
    #ifdef D_ENV_COMPILER_VERSION_STRING
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_VERSION_STRING is defined",
                              true,
                              "version string must be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_COMPILER_VERSION_STRING should be defined",
                              false,
                              "version string must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_compiler_version_at_least
  Tests D_ENV_COMPILER_VERSION_AT_LEAST macro.
  Tests the following:
  - macro correctly compares major versions
  - macro correctly compares minor versions when major equal
  - macro correctly compares patch levels when major and minor equal
  - boundary conditions work correctly
*/
bool d_tests_env_compiler_version_at_least(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    major;
    int    minor;
    int    patch;
    bool   result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_version_at_least");

    all_assertions_passed = true;

    major = D_ENV_COMPILER_MAJOR;
    minor = D_ENV_COMPILER_MINOR;
    patch = D_ENV_COMPILER_PATCHLEVEL;

    // test: current version should be at least itself
    result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(current) is true",
                          result,
                          "current version should be at least itself"))
    {
        all_assertions_passed = false;
    }

    // test: current version should be at least (0, 0, 0)
    result = D_ENV_COMPILER_VERSION_AT_LEAST(0, 0, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(0,0,0) is true",
                          result,
                          "any version should be at least 0.0.0"))
    {
        all_assertions_passed = false;
    }

    // test: major version comparison
    if (major > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_LEAST(major - 1, 999, 999);

        if (!D_TEST_CHECK_MSG(_suite,
                              "higher major beats lower major",
                              result,
                              "major version takes precedence"))
        {
            all_assertions_passed = false;
        }
    }

    // test: lower major version should fail
    result = D_ENV_COMPILER_VERSION_AT_LEAST(major + 1, 0, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(major+1,0,0) is false",
                          !result,
                          "higher required major should fail"))
    {
        all_assertions_passed = false;
    }

    // test: minor version comparison when major equal
    if (minor > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor - 1, 999);

        if (!D_TEST_CHECK_MSG(_suite,
                              "higher minor beats lower minor (same major)",
                              result,
                              "minor version matters when major equal"))
        {
            all_assertions_passed = false;
        }
    }

    // test: patch level comparison when major and minor equal
    if (patch > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch - 1);

        if (!D_TEST_CHECK_MSG(_suite,
                              "higher patch beats lower patch (same maj/min)",
                              result,
                              "patch matters when major and minor equal"))
        {
            all_assertions_passed = false;
        }
    }

    // test: exact boundary (patch + 1 should fail)
    result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch + 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(maj,min,patch+1) is false",
                          !result,
                          "higher required patch should fail"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_env_compiler_version_at_most
  Tests D_ENV_COMPILER_VERSION_AT_MOST macro.
  Tests the following:
  - macro correctly compares major versions
  - macro correctly compares minor versions when major equal
  - macro correctly compares patch levels when major and minor equal
  - boundary conditions work correctly
*/
bool d_tests_env_compiler_version_at_most(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    major;
    int    minor;
    int    patch;
    bool   result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_version_at_most");

    all_assertions_passed = true;

    major = D_ENV_COMPILER_MAJOR;
    minor = D_ENV_COMPILER_MINOR;
    patch = D_ENV_COMPILER_PATCHLEVEL;

    // test: current version should be at most itself
    result = D_ENV_COMPILER_VERSION_AT_MOST(major, minor, patch);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_MOST(current) is true",
                          result,
                          "current version should be at most itself"))
    {
        all_assertions_passed = false;
    }

    // test: current version should be at most (999, 999, 999)
    result = D_ENV_COMPILER_VERSION_AT_MOST(999, 999, 999);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_MOST(999,999,999) is true",
                          result,
                          "any version should be at most large value"))
    {
        all_assertions_passed = false;
    }

    // test: current version should NOT be at most (0, 0, 0) unless it is 0.0.0
    if ( (major > 0) || (minor > 0) || (patch > 0) )
    {
        result = D_ENV_COMPILER_VERSION_AT_MOST(0, 0, 0);

        if (!D_TEST_CHECK_MSG(_suite,
                              "VERSION_AT_MOST(0,0,0) is false for non-zero",
                              !result,
                              "non-zero version exceeds 0.0.0"))
        {
            all_assertions_passed = false;
        }
    }

    // test: higher major limit should pass
    result = D_ENV_COMPILER_VERSION_AT_MOST(major + 1, 0, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_MOST(major+1,0,0) is true",
                          result,
                          "higher limit should pass"))
    {
        all_assertions_passed = false;
    }

    // test: lower major limit should fail
    if (major > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_MOST(major - 1, 999, 999);

        if (!D_TEST_CHECK_MSG(_suite,
                              "VERSION_AT_MOST(major-1,999,999) is false",
                              !result,
                              "lower major limit should fail"))
        {
            all_assertions_passed = false;
        }
    }

    // test: exact boundary (patch - 1 should fail if patch > 0)
    if (patch > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_MOST(major, minor, patch - 1);

        if (!D_TEST_CHECK_MSG(_suite,
                              "VERSION_AT_MOST(maj,min,patch-1) is false",
                              !result,
                              "lower required patch should fail"))
        {
            all_assertions_passed = false;
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
* VA_OPT DETECTION TESTS
******************************************************************************/

/*
d_tests_env_compiler_va_opt_detection
  Tests __VA_OPT__ support detection.
  Tests the following:
  - D_ENV_PP_HAS_VA_OPT is defined
  - D_ENV_PP_HAS_VA_OPT_ENABLED is defined
  - values are boolean (0 or 1)
  - values are consistent with each other
*/
bool d_tests_env_compiler_va_opt_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_va_opt;
    int    va_opt_enabled;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_va_opt_detection");

    all_assertions_passed = true;

    // verify D_ENV_PP_HAS_VA_OPT is defined
    #ifdef D_ENV_PP_HAS_VA_OPT
        has_va_opt = D_ENV_PP_HAS_VA_OPT;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT is defined",
                              true,
                              "VA_OPT detection macro must be defined"))
        {
            all_assertions_passed = false;
        }

        // verify boolean value
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT is 0 or 1",
                              (has_va_opt == 0) || (has_va_opt == 1),
                              "should be boolean"))
        {
            all_assertions_passed = false;
        }
    #else
        has_va_opt = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT should be defined",
                              false,
                              "VA_OPT detection macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_HAS_VA_OPT_ENABLED is defined
    #ifdef D_ENV_PP_HAS_VA_OPT_ENABLED
        va_opt_enabled = D_ENV_PP_HAS_VA_OPT_ENABLED;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT_ENABLED is defined",
                              true,
                              "enabled wrapper macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        va_opt_enabled = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT_ENABLED should be defined",
                              false,
                              "enabled wrapper macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify consistency between the two macros
    if ( (has_va_opt >= 0) && (va_opt_enabled >= 0) )
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "HAS_VA_OPT == HAS_VA_OPT_ENABLED",
                              has_va_opt == va_opt_enabled,
                              "both macros should have same value"))
        {
            all_assertions_passed = false;
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
* PREPROCESSOR LIMIT CONSTANT TESTS
******************************************************************************/

/*
d_tests_env_compiler_pp_limit_constants
  Tests preprocessor limit constants for different standards.
  Tests the following:
  - C89 limit constants are defined with correct values
  - C99 limit constants are defined with correct values
  - C++ limit constants are defined with correct values
  - limits are in ascending order (C89 < C99 < C++)
*/
bool d_tests_env_compiler_pp_limit_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_pp_limit_constants");

    all_assertions_passed = true;

    // verify C89 constants
    if (!D_TEST_CHECK_MSG(_suite,
                          "C89_MACRO_ARGS == 31",
                          D_ENV_PP_LIMIT_C89_MACRO_ARGS == 31,
                          "C89 standard requires at least 31"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C89_NESTING_DEPTH == 8",
                          D_ENV_PP_LIMIT_C89_NESTING_DEPTH == 8,
                          "C89 standard requires at least 8"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C89_MACRO_IDS == 1024",
                          D_ENV_PP_LIMIT_C89_MACRO_IDS == 1024,
                          "C89 standard requires at least 1024"))
    {
        all_assertions_passed = false;
    }

    // verify C99 constants
    if (!D_TEST_CHECK_MSG(_suite,
                          "C99_MACRO_ARGS == 127",
                          D_ENV_PP_LIMIT_C99_MACRO_ARGS == 127,
                          "C99 standard requires at least 127"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C99_NESTING_DEPTH == 15",
                          D_ENV_PP_LIMIT_C99_NESTING_DEPTH == 15,
                          "C99 standard requires at least 15"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C99_MACRO_IDS == 4095",
                          D_ENV_PP_LIMIT_C99_MACRO_IDS == 4095,
                          "C99 standard requires at least 4095"))
    {
        all_assertions_passed = false;
    }

    // verify C++ constants
    if (!D_TEST_CHECK_MSG(_suite,
                          "CPP_MACRO_ARGS == 256",
                          D_ENV_PP_LIMIT_CPP_MACRO_ARGS == 256,
                          "C++ standard requires at least 256"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "CPP_NESTING_DEPTH == 256",
                          D_ENV_PP_LIMIT_CPP_NESTING_DEPTH == 256,
                          "C++ standard requires at least 256"))
    {
        all_assertions_passed = false;
    }

    // verify ascending order C89 < C99 < C++
    if (!D_TEST_CHECK_MSG(_suite,
                          "MACRO_ARGS: C89 < C99 < C++",
                          (D_ENV_PP_LIMIT_C89_MACRO_ARGS < D_ENV_PP_LIMIT_C99_MACRO_ARGS) && (D_ENV_PP_LIMIT_C99_MACRO_ARGS < D_ENV_PP_LIMIT_CPP_MACRO_ARGS),
                          "limits should increase with newer standards"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_env_compiler_pp_min_limits
  Tests standard-based minimum limit macros.
  Tests the following:
  - D_ENV_PP_MIN_MACRO_ARGS is defined and matches language standard
  - D_ENV_PP_MIN_NESTING_DEPTH is defined
  - D_ENV_PP_MIN_MACRO_IDS is defined
  - D_ENV_PP_MIN_PARAMS is defined
  - D_ENV_PP_MIN_STRING_LENGTH is defined
  - values are appropriate for detected language
*/
bool d_tests_env_compiler_pp_min_limits(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    min_args;
    int    min_depth;
    int    min_ids;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_pp_min_limits");

    all_assertions_passed = true;

    // verify D_ENV_PP_MIN_MACRO_ARGS
    #ifdef D_ENV_PP_MIN_MACRO_ARGS
        min_args = D_ENV_PP_MIN_MACRO_ARGS;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_ARGS is defined",
                              true,
                              "minimum args must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_ARGS > 0",
                              min_args > 0,
                              "minimum args should be positive"))
        {
            all_assertions_passed = false;
        }
    #else
        min_args = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_ARGS should be defined",
                              false,
                              "minimum args must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_MIN_NESTING_DEPTH
    #ifdef D_ENV_PP_MIN_NESTING_DEPTH
        min_depth = D_ENV_PP_MIN_NESTING_DEPTH;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_NESTING_DEPTH is defined",
                              true,
                              "minimum depth must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_NESTING_DEPTH > 0",
                              min_depth > 0,
                              "minimum depth should be positive"))
        {
            all_assertions_passed = false;
        }
    #else
        min_depth = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_NESTING_DEPTH should be defined",
                              false,
                              "minimum depth must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_MIN_MACRO_IDS
    #ifdef D_ENV_PP_MIN_MACRO_IDS
        min_ids = D_ENV_PP_MIN_MACRO_IDS;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_IDS is defined",
                              true,
                              "minimum IDs must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_IDS > 0",
                              min_ids > 0,
                              "minimum IDs should be positive"))
        {
            all_assertions_passed = false;
        }
    #else
        min_ids = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_IDS should be defined",
                              false,
                              "minimum IDs must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify values match language standard
    #ifdef D_ENV_LANG_CPP_STANDARD
        if (!D_TEST_CHECK_MSG(_suite,
                              "MIN_MACRO_ARGS matches C++ limit",
                              min_args == D_ENV_PP_LIMIT_CPP_MACRO_ARGS,
                              "C++ mode should use C++ limits"))
        {
            all_assertions_passed = false;
        }

    #elif D_ENV_LANG_IS_C99_OR_HIGHER
        if (!D_TEST_CHECK_MSG(_suite,
                              "MIN_MACRO_ARGS matches C99 limit",
                              min_args == D_ENV_PP_LIMIT_C99_MACRO_ARGS,
                              "C99+ mode should use C99 limits"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "MIN_MACRO_ARGS matches C89 limit",
                              min_args == D_ENV_PP_LIMIT_C89_MACRO_ARGS,
                              "C89/90 mode should use C89 limits"))
        {
            all_assertions_passed = false;
        }

    #endif

    return all_assertions_passed;
}


/*
d_tests_env_compiler_pp_max_limits
  Tests compiler-specific maximum limit macros.
  Tests the following:
  - D_ENV_PP_MAX_MACRO_ARGS is defined
  - D_ENV_PP_MAX_NESTING_DEPTH is defined
  - D_ENV_PP_LIMIT_SOURCE is defined
  - max limits are >= min limits (unless unlimited)
*/
bool d_tests_env_compiler_pp_max_limits(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    max_args;
    int    max_depth;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_pp_max_limits");

    all_assertions_passed = true;

    // verify D_ENV_PP_MAX_MACRO_ARGS
    #ifdef D_ENV_PP_MAX_MACRO_ARGS
        max_args = D_ENV_PP_MAX_MACRO_ARGS;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_ARGS is defined",
                              true,
                              "max args must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_ARGS >= 0",
                              max_args >= 0,
                              "max args should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        max_args = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_ARGS should be defined",
                              false,
                              "max args must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_MAX_NESTING_DEPTH
    #ifdef D_ENV_PP_MAX_NESTING_DEPTH
        max_depth = D_ENV_PP_MAX_NESTING_DEPTH;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_NESTING_DEPTH is defined",
                              true,
                              "max depth must be defined"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_NESTING_DEPTH >= 0",
                              max_depth >= 0,
                              "max depth should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        max_depth = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_NESTING_DEPTH should be defined",
                              false,
                              "max depth must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_LIMIT_SOURCE
    #ifdef D_ENV_PP_LIMIT_SOURCE
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_LIMIT_SOURCE is defined",
                              true,
                              "limit source must be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_LIMIT_SOURCE should be defined",
                              false,
                              "limit source must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify max >= min (unless unlimited)
    #if defined(D_ENV_PP_MAX_MACRO_ARGS) && defined(D_ENV_PP_MIN_MACRO_ARGS)
        if (D_ENV_PP_MAX_MACRO_ARGS > 0)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "MAX_MACRO_ARGS >= MIN_MACRO_ARGS",
                                  D_ENV_PP_MAX_MACRO_ARGS >= D_ENV_PP_MIN_MACRO_ARGS,
                                  "max should be at least min"))
            {
                all_assertions_passed = false;
            }
        }
        else
        {
            // 0 means unlimited
            if (!D_TEST_CHECK_MSG(_suite,
                                  "MAX_MACRO_ARGS is unlimited (0)",
                                  true,
                                  "unlimited is valid"))
            {
                all_assertions_passed = false;
            }
        }
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_compiler_pp_limit_utility_macros
  Tests preprocessor limit utility macros.
  Tests the following:
  - D_ENV_PP_ARGS_WITHIN_LIMIT evaluates correctly
  - D_ENV_PP_ARGS_WITHIN_STANDARD evaluates correctly
  - D_ENV_PP_IS_UNLIMITED evaluates correctly
  - D_ENV_PP_EFFECTIVE_LIMIT evaluates correctly
*/
bool d_tests_env_compiler_pp_limit_utility_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   within_limit;
    bool   within_standard;
    bool   is_unlimited;
    long   effective;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_pp_limit_utility_macros");

    all_assertions_passed = true;

    // test ARGS_WITHIN_LIMIT
    within_limit = D_ENV_PP_ARGS_WITHIN_LIMIT(10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "ARGS_WITHIN_LIMIT(10) is true",
                          within_limit,
                          "10 args should be within any limit"))
    {
        all_assertions_passed = false;
    }

    // test ARGS_WITHIN_STANDARD
    within_standard = D_ENV_PP_ARGS_WITHIN_STANDARD(10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "ARGS_WITHIN_STANDARD(10) is true",
                          within_standard,
                          "10 args should be within any standard"))
    {
        all_assertions_passed = false;
    }

    // test with value at C89 limit
    within_standard = D_ENV_PP_ARGS_WITHIN_STANDARD(31);

    if (!D_TEST_CHECK_MSG(_suite,
                          "ARGS_WITHIN_STANDARD(31) is true",
                          within_standard,
                          "31 args is C89 minimum"))
    {
        all_assertions_passed = false;
    }

    // test IS_UNLIMITED
    is_unlimited = D_ENV_PP_IS_UNLIMITED(0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "IS_UNLIMITED(0) is true",
                          is_unlimited,
                          "0 represents unlimited"))
    {
        all_assertions_passed = false;
    }

    is_unlimited = D_ENV_PP_IS_UNLIMITED(100);

    if (!D_TEST_CHECK_MSG(_suite,
                          "IS_UNLIMITED(100) is false",
                          !is_unlimited,
                          "non-zero is not unlimited"))
    {
        all_assertions_passed = false;
    }

    // test EFFECTIVE_LIMIT
    effective = D_ENV_PP_EFFECTIVE_LIMIT(0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "EFFECTIVE_LIMIT(0) is large value",
                          effective == 2147483647L,
                          "unlimited should become large number"))
    {
        all_assertions_passed = false;
    }

    effective = D_ENV_PP_EFFECTIVE_LIMIT(100);

    if (!D_TEST_CHECK_MSG(_suite,
                          "EFFECTIVE_LIMIT(100) is 100",
                          effective == 100,
                          "non-zero should pass through"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
* MANUAL DETECTION TESTS
******************************************************************************/

/*
d_tests_env_compiler_manual_detection
  Tests manual compiler detection via D_ENV_DETECTED_COMPILER_* variables.
  Tests the following:
  - detection variables are mutually exclusive
  - correct mapping when manual mode is active
*/
bool d_tests_env_compiler_manual_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    detected_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_manual_detection");

    all_assertions_passed = true;
    detected_count        = 0;

    // count detection variables
    #ifdef D_ENV_DETECTED_COMPILER_APPLE_CLANG
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_CLANG
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_GCC
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_MSVC
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_INTEL
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_BORLAND
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_UNKNOWN
        detected_count++;
    #endif

    // verify at most one detection variable
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one compiler detection var defined",
                          detected_count <= 1,
                          "detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }


    // if manual mode is active, verify correct mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER)

        #ifdef D_ENV_DETECTED_COMPILER_GCC
            #ifdef D_ENV_COMPILER_GCC
                if (!D_TEST_CHECK_MSG(_suite,
                                      "DETECTED_GCC maps to COMPILER_GCC",
                                      true,
                                      "manual detection should set flag"))
                {
                    all_assertions_passed = false;
                }
            #else
                if (!D_TEST_CHECK_MSG(_suite,
                                      "DETECTED_GCC should set COMPILER_GCC",
                                      false,
                                      "manual detection not working"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif

        #ifdef D_ENV_DETECTED_COMPILER_MSVC
            #ifdef D_ENV_COMPILER_MSVC
                if (!D_TEST_CHECK_MSG(_suite,
                                      "DETECTED_MSVC maps to COMPILER_MSVC",
                                      true,
                                      "manual detection should set flag"))
                {
                    all_assertions_passed = false;
                }
            #else
                if (!D_TEST_CHECK_MSG(_suite,
                                      "DETECTED_MSVC should set COMPILER_MSVC",
                                      false,
                                      "manual detection not working"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #else

        if (!D_TEST_CHECK_MSG(_suite,
                              "automatic detection mode active",
                              true,
                              "manual vars are informational only"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* CONSISTENCY TESTS
******************************************************************************/

/*
d_tests_env_compiler_consistency_check
  Tests overall consistency of compiler detection.
  Tests the following:
  - all required macros are defined together
  - version info is consistent
  - preprocessor limits are consistent with compiler
*/
bool d_tests_env_compiler_consistency_check(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_flag;
    bool   has_name;
    bool   has_version;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_compiler_consistency_check");

    all_assertions_passed = true;

    // check that compiler flag is defined
    has_flag = false;
    #if ( defined(D_ENV_COMPILER_CLANG)   ||  \
          defined(D_ENV_COMPILER_GCC)     ||  \
          defined(D_ENV_COMPILER_MSVC)    ||  \
          defined(D_ENV_COMPILER_INTEL)   ||  \
          defined(D_ENV_COMPILER_BORLAND) ||  \
          defined(D_ENV_COMPILER_UNKNOWN) )
        has_flag = true;
    #endif

    if (!D_TEST_CHECK_MSG(_suite,
                          "at least one compiler flag defined",
                          has_flag,
                          "some compiler must be detected"))
    {
        all_assertions_passed = false;
    }

    // check name macros
    has_name = false;
    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_FULL_NAME)
        has_name = true;
    #endif

    if (!D_TEST_CHECK_MSG(_suite,
                          "both NAME and FULL_NAME defined",
                          has_name,
                          "name macros must be consistent"))
    {
        all_assertions_passed = false;
    }

    // check version macros
    has_version = false;
    #if ( defined(D_ENV_COMPILER_MAJOR)          &&  \
          defined(D_ENV_COMPILER_MINOR)          &&  \
          defined(D_ENV_COMPILER_PATCHLEVEL)     &&  \
          defined(D_ENV_COMPILER_VERSION_STRING) )
        has_version = true;
    #endif

    if (!D_TEST_CHECK_MSG(_suite,
                          "all version macros defined",
                          has_version,
                          "version info must be complete"))
    {
        all_assertions_passed = false;
    }

    // verify version comparison macros work with current version
    #if ( defined(D_ENV_COMPILER_MAJOR) &&  \
          defined(D_ENV_COMPILER_MINOR) &&  \
          defined(D_ENV_COMPILER_PATCHLEVEL) )
    {
        bool at_least_self = D_ENV_COMPILER_VERSION_AT_LEAST(
            D_ENV_COMPILER_MAJOR,
            D_ENV_COMPILER_MINOR,
            D_ENV_COMPILER_PATCHLEVEL);

        bool at_most_self = D_ENV_COMPILER_VERSION_AT_MOST(
            D_ENV_COMPILER_MAJOR,
            D_ENV_COMPILER_MINOR,
            D_ENV_COMPILER_PATCHLEVEL);

        if (!D_TEST_CHECK_MSG(_suite,
                              "version is both at_least and at_most itself",
                              at_least_self && at_most_self,
                              "comparison macros should be consistent"))
        {
            all_assertions_passed = false;
        }
    }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_env_compiler_all
  Runs all compiler environment tests.
  Tests the following:
  - detection flags
  - name macros
  - version macros
  - version comparison
  - VA_OPT detection
  - preprocessor limits
  - manual detection
  - consistency
*/
bool d_tests_env_compiler_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_compiler_all");

    d_tests_env_compiler_detection_flags(_suite);
    d_tests_env_compiler_name_macros(_suite);
    d_tests_env_compiler_version_macros(_suite);
    d_tests_env_compiler_version_at_least(_suite);
    d_tests_env_compiler_version_at_most(_suite);
    d_tests_env_compiler_va_opt_detection(_suite);
    d_tests_env_compiler_pp_limit_constants(_suite);
    d_tests_env_compiler_pp_min_limits(_suite);
    d_tests_env_compiler_pp_max_limits(_suite);
    d_tests_env_compiler_pp_limit_utility_macros(_suite);
    d_tests_env_compiler_manual_detection(_suite);
    d_tests_env_compiler_consistency_check(_suite);

    return true;
}
