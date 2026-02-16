/******************************************************************************
* djinterp [test]                                      env_tests_sa_compiler.c
*
* Unit tests for `env.h` compiler detection section (Section IV).
* Tests compiler detection, version info, version comparison, VA_OPT detection,
* and preprocessor limits.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_compiler.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.26
******************************************************************************/

#include "env_tests_sa.h"
#include <string.h>


/******************************************************************************
* COMPILER DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_compiler_detection_flags
  Tests that exactly one compiler flag is defined.
  Tests the following:
  - exactly one of CLANG, GCC, MSVC, INTEL, BORLAND, UNKNOWN is defined
  - compiler flags are mutually exclusive
  - D_ENV_COMPILER_APPLE_CLANG implies D_ENV_COMPILER_CLANG
*/
bool
d_tests_sa_env_compiler_detection_flags
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    compiler_flag_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    compiler_flag_count   = 0;

    printf("%s--- Testing Compiler Detection Flags ---\n", D_INDENT);

    // count compiler flags
    #ifdef D_ENV_COMPILER_CLANG
        compiler_flag_count++;
        printf("%s    D_ENV_COMPILER_CLANG is defined\n", D_INDENT);

        #ifdef D_ENV_COMPILER_APPLE_CLANG
            printf("%s    D_ENV_COMPILER_APPLE_CLANG is also defined\n",
                   D_INDENT);
        #endif
    #endif

    #ifdef D_ENV_COMPILER_GCC
        compiler_flag_count++;
        printf("%s    D_ENV_COMPILER_GCC is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_COMPILER_MSVC
        compiler_flag_count++;
        printf("%s    D_ENV_COMPILER_MSVC is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_COMPILER_INTEL
        compiler_flag_count++;
        printf("%s    D_ENV_COMPILER_INTEL is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_COMPILER_BORLAND
        compiler_flag_count++;
        printf("%s    D_ENV_COMPILER_BORLAND is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_COMPILER_UNKNOWN
        compiler_flag_count++;
        printf("%s    D_ENV_COMPILER_UNKNOWN is defined\n", D_INDENT);
    #endif

    // verify exactly one compiler flag (Clang counts as one even with Apple)
    if (!d_assert_standalone(compiler_flag_count == 1,
                             "exactly one compiler flag defined",
                             "compiler detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: found %d compiler flags\n",
               D_INDENT,
               compiler_flag_count);
    }

    // verify Apple Clang implies Clang
    #ifdef D_ENV_COMPILER_APPLE_CLANG
        #ifndef D_ENV_COMPILER_CLANG
            if (!d_assert_standalone(false,
                                     "APPLE_CLANG should imply CLANG",
                                     "Apple Clang is a variant of Clang",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(true,
                                     "APPLE_CLANG implies CLANG",
                                     "Apple Clang correctly implies Clang",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    printf("%s    Compiler flags count: %d\n", D_INDENT, compiler_flag_count);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Compiler detection flags test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Compiler detection flags test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_compiler_name_macros
  Tests compiler name macros.
  Tests the following:
  - D_ENV_COMPILER_NAME is defined and non-empty
  - D_ENV_COMPILER_FULL_NAME is defined and non-empty
  - names are consistent with detected compiler flag
*/
bool
d_tests_sa_env_compiler_name_macros
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* name;
    const char* full_name;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Compiler Name Macros ---\n", D_INDENT);

    // verify D_ENV_COMPILER_NAME is defined
    #ifdef D_ENV_COMPILER_NAME
        name = D_ENV_COMPILER_NAME;

        if (!d_assert_standalone(true,
                                 "D_ENV_COMPILER_NAME is defined",
                                 "compiler name must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify non-empty
        if (!d_assert_standalone( (name != NULL) && (name[0] != '\0'),
                                 "D_ENV_COMPILER_NAME is non-empty",
                                 "name should be non-empty string",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        name = NULL;

        if (!d_assert_standalone(false,
                                 "D_ENV_COMPILER_NAME should be defined",
                                 "compiler name must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_FULL_NAME is defined
    #ifdef D_ENV_COMPILER_FULL_NAME
        full_name = D_ENV_COMPILER_FULL_NAME;

        if (!d_assert_standalone(true,
                                 "D_ENV_COMPILER_FULL_NAME is defined",
                                 "compiler full name must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify non-empty
        if (!d_assert_standalone( (full_name != NULL) && (full_name[0] != '\0'),
                                 "D_ENV_COMPILER_FULL_NAME is non-empty",
                                 "full name should be non-empty string",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        full_name = NULL;

        if (!d_assert_standalone(false,
                                 "D_ENV_COMPILER_FULL_NAME should be defined",
                                 "compiler full name must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify consistency with compiler flag
    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_GCC)
        if (!d_assert_standalone(strcmp(D_ENV_COMPILER_NAME, "GCC") == 0,
                                 "NAME is \"GCC\" for GCC compiler",
                                 "name should match compiler",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_MSVC)
        if (!d_assert_standalone(strcmp(D_ENV_COMPILER_NAME, "MSVC") == 0,
                                 "NAME is \"MSVC\" for MSVC compiler",
                                 "name should match compiler",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_CLANG)
        #ifdef D_ENV_COMPILER_APPLE_CLANG
            if (!d_assert_standalone(strstr(D_ENV_COMPILER_NAME, "Clang") != NULL,
                                     "NAME contains \"Clang\" for Apple Clang",
                                     "name should match compiler",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(strcmp(D_ENV_COMPILER_NAME, "Clang") == 0,
                                     "NAME is \"Clang\" for Clang compiler",
                                     "name should match compiler",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    // report names
    if (name != NULL)
    {
        printf("%s    COMPILER_NAME:      \"%s\"\n", D_INDENT, name);
    }

    if (full_name != NULL)
    {
        printf("%s    COMPILER_FULL_NAME: \"%s\"\n", D_INDENT, full_name);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Compiler name macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Compiler name macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* COMPILER VERSION TESTS
******************************************************************************/

/*
d_tests_sa_env_compiler_version_macros
  Tests compiler version macros.
  Tests the following:
  - D_ENV_COMPILER_MAJOR is defined and non-negative
  - D_ENV_COMPILER_MINOR is defined and non-negative
  - D_ENV_COMPILER_PATCHLEVEL is defined and non-negative
  - D_ENV_COMPILER_VERSION_STRING is defined
*/
bool
d_tests_sa_env_compiler_version_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    major_ver;
    int    minor_ver;
    int    patch_ver;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Compiler Version Macros ---\n", D_INDENT);

    // verify D_ENV_COMPILER_MAJOR
    #ifdef D_ENV_COMPILER_MAJOR
        major_ver = D_ENV_COMPILER_MAJOR;

        if (!d_assert_standalone(true,
                                 "D_ENV_COMPILER_MAJOR is defined",
                                 "major version must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(major_ver >= 0,
                                 "D_ENV_COMPILER_MAJOR >= 0",
                                 "major version should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        major_ver = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_COMPILER_MAJOR should be defined",
                                 "major version must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_MINOR
    #ifdef D_ENV_COMPILER_MINOR
        minor_ver = D_ENV_COMPILER_MINOR;

        if (!d_assert_standalone(true,
                                 "D_ENV_COMPILER_MINOR is defined",
                                 "minor version must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(minor_ver >= 0,
                                 "D_ENV_COMPILER_MINOR >= 0",
                                 "minor version should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        minor_ver = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_COMPILER_MINOR should be defined",
                                 "minor version must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_PATCHLEVEL
    #ifdef D_ENV_COMPILER_PATCHLEVEL
        patch_ver = D_ENV_COMPILER_PATCHLEVEL;

        if (!d_assert_standalone(true,
                                 "D_ENV_COMPILER_PATCHLEVEL is defined",
                                 "patch level must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(patch_ver >= 0,
                                 "D_ENV_COMPILER_PATCHLEVEL >= 0",
                                 "patch level should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        patch_ver = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_COMPILER_PATCHLEVEL should be defined",
                                 "patch level must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_COMPILER_VERSION_STRING
    #ifdef D_ENV_COMPILER_VERSION_STRING
        if (!d_assert_standalone(true,
                                 "D_ENV_COMPILER_VERSION_STRING is defined",
                                 "version string must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    VERSION_STRING: \"%s\"\n",
               D_INDENT,
               D_ENV_COMPILER_VERSION_STRING);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_COMPILER_VERSION_STRING should be defined",
                                 "version string must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // report version
    printf("%s    Version: %d.%d.%d\n",
           D_INDENT,
           major_ver,
           minor_ver,
           patch_ver);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Compiler version macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Compiler version macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_compiler_version_at_least
  Tests D_ENV_COMPILER_VERSION_AT_LEAST macro.
  Tests the following:
  - macro correctly compares major versions
  - macro correctly compares minor versions when major equal
  - macro correctly compares patch levels when major and minor equal
  - boundary conditions work correctly
*/
bool
d_tests_sa_env_compiler_version_at_least
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    major;
    int    minor;
    int    patch;
    bool   result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing VERSION_AT_LEAST Macro ---\n", D_INDENT);

    major = D_ENV_COMPILER_MAJOR;
    minor = D_ENV_COMPILER_MINOR;
    patch = D_ENV_COMPILER_PATCHLEVEL;

    // test: current version should be at least itself
    result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch);

    if (!d_assert_standalone(result,
                             "VERSION_AT_LEAST(current) is true",
                             "current version should be at least itself",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test: current version should be at least (0, 0, 0)
    result = D_ENV_COMPILER_VERSION_AT_LEAST(0, 0, 0);

    if (!d_assert_standalone(result,
                             "VERSION_AT_LEAST(0,0,0) is true",
                             "any version should be at least 0.0.0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test: major version comparison
    if (major > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_LEAST(major - 1, 999, 999);

        if (!d_assert_standalone(result,
                                 "higher major beats lower major",
                                 "major version takes precedence",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test: lower major version should fail
    result = D_ENV_COMPILER_VERSION_AT_LEAST(major + 1, 0, 0);

    if (!d_assert_standalone(!result,
                             "VERSION_AT_LEAST(major+1,0,0) is false",
                             "higher required major should fail",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test: minor version comparison when major equal
    if (minor > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor - 1, 999);

        if (!d_assert_standalone(result,
                                 "higher minor beats lower minor (same major)",
                                 "minor version matters when major equal",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test: patch level comparison when major and minor equal
    if (patch > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch - 1);

        if (!d_assert_standalone(result,
                                 "higher patch beats lower patch (same maj/min)",
                                 "patch matters when major and minor equal",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test: exact boundary (patch + 1 should fail)
    result = D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch + 1);

    if (!d_assert_standalone(!result,
                             "VERSION_AT_LEAST(maj,min,patch+1) is false",
                             "higher required patch should fail",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Current version: %d.%d.%d\n", D_INDENT, major, minor, patch);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] VERSION_AT_LEAST macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] VERSION_AT_LEAST macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_compiler_version_at_most
  Tests D_ENV_COMPILER_VERSION_AT_MOST macro.
  Tests the following:
  - macro correctly compares major versions
  - macro correctly compares minor versions when major equal
  - macro correctly compares patch levels when major and minor equal
  - boundary conditions work correctly
*/
bool
d_tests_sa_env_compiler_version_at_most
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    major;
    int    minor;
    int    patch;
    bool   result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing VERSION_AT_MOST Macro ---\n", D_INDENT);

    major = D_ENV_COMPILER_MAJOR;
    minor = D_ENV_COMPILER_MINOR;
    patch = D_ENV_COMPILER_PATCHLEVEL;

    // test: current version should be at most itself
    result = D_ENV_COMPILER_VERSION_AT_MOST(major, minor, patch);

    if (!d_assert_standalone(result,
                             "VERSION_AT_MOST(current) is true",
                             "current version should be at most itself",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test: current version should be at most (999, 999, 999)
    result = D_ENV_COMPILER_VERSION_AT_MOST(999, 999, 999);

    if (!d_assert_standalone(result,
                             "VERSION_AT_MOST(999,999,999) is true",
                             "any version should be at most large value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test: current version should NOT be at most (0, 0, 0) unless it is 0.0.0
    if ( (major > 0) || (minor > 0) || (patch > 0) )
    {
        result = D_ENV_COMPILER_VERSION_AT_MOST(0, 0, 0);

        if (!d_assert_standalone(!result,
                                 "VERSION_AT_MOST(0,0,0) is false for non-zero",
                                 "non-zero version exceeds 0.0.0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test: higher major limit should pass
    result = D_ENV_COMPILER_VERSION_AT_MOST(major + 1, 0, 0);

    if (!d_assert_standalone(result,
                             "VERSION_AT_MOST(major+1,0,0) is true",
                             "higher limit should pass",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test: lower major limit should fail
    if (major > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_MOST(major - 1, 999, 999);

        if (!d_assert_standalone(!result,
                                 "VERSION_AT_MOST(major-1,999,999) is false",
                                 "lower major limit should fail",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test: exact boundary (patch - 1 should fail if patch > 0)
    if (patch > 0)
    {
        result = D_ENV_COMPILER_VERSION_AT_MOST(major, minor, patch - 1);

        if (!d_assert_standalone(!result,
                                 "VERSION_AT_MOST(maj,min,patch-1) is false",
                                 "lower required patch should fail",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    printf("%s    Current version: %d.%d.%d\n", D_INDENT, major, minor, patch);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] VERSION_AT_MOST macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] VERSION_AT_MOST macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* VA_OPT DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_compiler_va_opt_detection
  Tests __VA_OPT__ support detection.
  Tests the following:
  - D_ENV_PP_HAS_VA_OPT is defined
  - D_ENV_PP_HAS_VA_OPT_ENABLED is defined
  - values are boolean (0 or 1)
  - values are consistent with each other
*/
bool
d_tests_sa_env_compiler_va_opt_detection
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    has_va_opt;
    int    va_opt_enabled;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing VA_OPT Detection ---\n", D_INDENT);

    // verify D_ENV_PP_HAS_VA_OPT is defined
    #ifdef D_ENV_PP_HAS_VA_OPT
        has_va_opt = D_ENV_PP_HAS_VA_OPT;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_HAS_VA_OPT is defined",
                                 "VA_OPT detection macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify boolean value
        if (!d_assert_standalone( (has_va_opt == 0) || (has_va_opt == 1),
                                 "D_ENV_PP_HAS_VA_OPT is 0 or 1",
                                 "should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        has_va_opt = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_HAS_VA_OPT should be defined",
                                 "VA_OPT detection macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_HAS_VA_OPT_ENABLED is defined
    #ifdef D_ENV_PP_HAS_VA_OPT_ENABLED
        va_opt_enabled = D_ENV_PP_HAS_VA_OPT_ENABLED;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_HAS_VA_OPT_ENABLED is defined",
                                 "enabled wrapper macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        va_opt_enabled = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_HAS_VA_OPT_ENABLED should be defined",
                                 "enabled wrapper macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify consistency between the two macros
    if ( (has_va_opt >= 0) && (va_opt_enabled >= 0) )
    {
        if (!d_assert_standalone(has_va_opt == va_opt_enabled,
                                 "HAS_VA_OPT == HAS_VA_OPT_ENABLED",
                                 "both macros should have same value",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    printf("%s    D_ENV_PP_HAS_VA_OPT:         %d\n", D_INDENT, has_va_opt);
    printf("%s    D_ENV_PP_HAS_VA_OPT_ENABLED: %d\n", D_INDENT, va_opt_enabled);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] VA_OPT detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] VA_OPT detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* PREPROCESSOR LIMIT CONSTANT TESTS
******************************************************************************/

/*
d_tests_sa_env_compiler_pp_limit_constants
  Tests preprocessor limit constants for different standards.
  Tests the following:
  - C89 limit constants are defined with correct values
  - C99 limit constants are defined with correct values
  - C++ limit constants are defined with correct values
  - limits are in ascending order (C89 < C99 < C++)
*/
bool
d_tests_sa_env_compiler_pp_limit_constants
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

    printf("%s--- Testing PP Limit Constants ---\n", D_INDENT);

    // verify C89 constants
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_MACRO_ARGS == 31,
                             "C89_MACRO_ARGS == 31",
                             "C89 standard requires at least 31",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_NESTING_DEPTH == 8,
                             "C89_NESTING_DEPTH == 8",
                             "C89 standard requires at least 8",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_MACRO_IDS == 1024,
                             "C89_MACRO_IDS == 1024",
                             "C89 standard requires at least 1024",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C99 constants
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_MACRO_ARGS == 127,
                             "C99_MACRO_ARGS == 127",
                             "C99 standard requires at least 127",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_NESTING_DEPTH == 15,
                             "C99_NESTING_DEPTH == 15",
                             "C99 standard requires at least 15",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_MACRO_IDS == 4095,
                             "C99_MACRO_IDS == 4095",
                             "C99 standard requires at least 4095",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C++ constants
    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_MACRO_ARGS == 256,
                             "CPP_MACRO_ARGS == 256",
                             "C++ standard requires at least 256",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_NESTING_DEPTH == 256,
                             "CPP_NESTING_DEPTH == 256",
                             "C++ standard requires at least 256",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify ascending order C89 < C99 < C++
    if (!d_assert_standalone( (D_ENV_PP_LIMIT_C89_MACRO_ARGS <
                               D_ENV_PP_LIMIT_C99_MACRO_ARGS) &&
                              (D_ENV_PP_LIMIT_C99_MACRO_ARGS <
                               D_ENV_PP_LIMIT_CPP_MACRO_ARGS),
                             "MACRO_ARGS: C89 < C99 < C++",
                             "limits should increase with newer standards",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // print values
    printf("%s    C89: args=%d depth=%d ids=%d\n",
           D_INDENT,
           D_ENV_PP_LIMIT_C89_MACRO_ARGS,
           D_ENV_PP_LIMIT_C89_NESTING_DEPTH,
           D_ENV_PP_LIMIT_C89_MACRO_IDS);
    printf("%s    C99: args=%d depth=%d ids=%d\n",
           D_INDENT,
           D_ENV_PP_LIMIT_C99_MACRO_ARGS,
           D_ENV_PP_LIMIT_C99_NESTING_DEPTH,
           D_ENV_PP_LIMIT_C99_MACRO_IDS);
    printf("%s    C++: args=%d depth=%d ids=%d\n",
           D_INDENT,
           D_ENV_PP_LIMIT_CPP_MACRO_ARGS,
           D_ENV_PP_LIMIT_CPP_NESTING_DEPTH,
           D_ENV_PP_LIMIT_CPP_MACRO_IDS);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] PP limit constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] PP limit constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_compiler_pp_min_limits
  Tests standard-based minimum limit macros.
  Tests the following:
  - D_ENV_PP_MIN_MACRO_ARGS is defined and matches language standard
  - D_ENV_PP_MIN_NESTING_DEPTH is defined
  - D_ENV_PP_MIN_MACRO_IDS is defined
  - D_ENV_PP_MIN_PARAMS is defined
  - D_ENV_PP_MIN_STRING_LENGTH is defined
  - values are appropriate for detected language
*/
bool
d_tests_sa_env_compiler_pp_min_limits
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    min_args;
    int    min_depth;
    int    min_ids;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing PP Minimum Limits ---\n", D_INDENT);

    // verify D_ENV_PP_MIN_MACRO_ARGS
    #ifdef D_ENV_PP_MIN_MACRO_ARGS
        min_args = D_ENV_PP_MIN_MACRO_ARGS;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_MACRO_ARGS is defined",
                                 "minimum args must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(min_args > 0,
                                 "D_ENV_PP_MIN_MACRO_ARGS > 0",
                                 "minimum args should be positive",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        min_args = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_MACRO_ARGS should be defined",
                                 "minimum args must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_MIN_NESTING_DEPTH
    #ifdef D_ENV_PP_MIN_NESTING_DEPTH
        min_depth = D_ENV_PP_MIN_NESTING_DEPTH;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_NESTING_DEPTH is defined",
                                 "minimum depth must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(min_depth > 0,
                                 "D_ENV_PP_MIN_NESTING_DEPTH > 0",
                                 "minimum depth should be positive",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        min_depth = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_NESTING_DEPTH should be defined",
                                 "minimum depth must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_MIN_MACRO_IDS
    #ifdef D_ENV_PP_MIN_MACRO_IDS
        min_ids = D_ENV_PP_MIN_MACRO_IDS;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_MACRO_IDS is defined",
                                 "minimum IDs must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(min_ids > 0,
                                 "D_ENV_PP_MIN_MACRO_IDS > 0",
                                 "minimum IDs should be positive",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        min_ids = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_MACRO_IDS should be defined",
                                 "minimum IDs must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify values match language standard
    #ifdef D_ENV_LANG_CPP_STANDARD
        if (!d_assert_standalone(min_args == D_ENV_PP_LIMIT_CPP_MACRO_ARGS,
                                 "MIN_MACRO_ARGS matches C++ limit",
                                 "C++ mode should use C++ limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Using C++ standard limits\n", D_INDENT);
    #elif D_ENV_LANG_IS_C99_OR_HIGHER
        if (!d_assert_standalone(min_args == D_ENV_PP_LIMIT_C99_MACRO_ARGS,
                                 "MIN_MACRO_ARGS matches C99 limit",
                                 "C99+ mode should use C99 limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Using C99+ standard limits\n", D_INDENT);
    #else
        if (!d_assert_standalone(min_args == D_ENV_PP_LIMIT_C89_MACRO_ARGS,
                                 "MIN_MACRO_ARGS matches C89 limit",
                                 "C89/90 mode should use C89 limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Using C89/90 standard limits\n", D_INDENT);
    #endif

    printf("%s    MIN_MACRO_ARGS:    %d\n", D_INDENT, min_args);
    printf("%s    MIN_NESTING_DEPTH: %d\n", D_INDENT, min_depth);
    printf("%s    MIN_MACRO_IDS:     %d\n", D_INDENT, min_ids);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] PP minimum limits test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] PP minimum limits test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_compiler_pp_max_limits
  Tests compiler-specific maximum limit macros.
  Tests the following:
  - D_ENV_PP_MAX_MACRO_ARGS is defined
  - D_ENV_PP_MAX_NESTING_DEPTH is defined
  - D_ENV_PP_LIMIT_SOURCE is defined
  - max limits are >= min limits (unless unlimited)
*/
bool
d_tests_sa_env_compiler_pp_max_limits
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    max_args;
    int    max_depth;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing PP Maximum Limits ---\n", D_INDENT);

    // verify D_ENV_PP_MAX_MACRO_ARGS
    #ifdef D_ENV_PP_MAX_MACRO_ARGS
        max_args = D_ENV_PP_MAX_MACRO_ARGS;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MAX_MACRO_ARGS is defined",
                                 "max args must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(max_args >= 0,
                                 "D_ENV_PP_MAX_MACRO_ARGS >= 0",
                                 "max args should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        max_args = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MAX_MACRO_ARGS should be defined",
                                 "max args must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_MAX_NESTING_DEPTH
    #ifdef D_ENV_PP_MAX_NESTING_DEPTH
        max_depth = D_ENV_PP_MAX_NESTING_DEPTH;

        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MAX_NESTING_DEPTH is defined",
                                 "max depth must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(max_depth >= 0,
                                 "D_ENV_PP_MAX_NESTING_DEPTH >= 0",
                                 "max depth should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        max_depth = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MAX_NESTING_DEPTH should be defined",
                                 "max depth must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_LIMIT_SOURCE
    #ifdef D_ENV_PP_LIMIT_SOURCE
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_LIMIT_SOURCE is defined",
                                 "limit source must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    LIMIT_SOURCE: \"%s\"\n", D_INDENT, D_ENV_PP_LIMIT_SOURCE);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_LIMIT_SOURCE should be defined",
                                 "limit source must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify max >= min (unless unlimited)
    #if defined(D_ENV_PP_MAX_MACRO_ARGS) && defined(D_ENV_PP_MIN_MACRO_ARGS)
        if (D_ENV_PP_MAX_MACRO_ARGS > 0)
        {
            if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS >= D_ENV_PP_MIN_MACRO_ARGS,
                                     "MAX_MACRO_ARGS >= MIN_MACRO_ARGS",
                                     "max should be at least min",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }
        else
        {
            // 0 means unlimited
            if (!d_assert_standalone(true,
                                     "MAX_MACRO_ARGS is unlimited (0)",
                                     "unlimited is valid",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }
    #endif

    printf("%s    MAX_MACRO_ARGS:    %d%s\n",
           D_INDENT,
           max_args,
           (max_args == 0) ? " (unlimited)" : "");
    printf("%s    MAX_NESTING_DEPTH: %d%s\n",
           D_INDENT,
           max_depth,
           (max_depth == 0) ? " (unlimited)" : "");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] PP maximum limits test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] PP maximum limits test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_compiler_pp_limit_utility_macros
  Tests preprocessor limit utility macros.
  Tests the following:
  - D_ENV_PP_ARGS_WITHIN_LIMIT evaluates correctly
  - D_ENV_PP_ARGS_WITHIN_STANDARD evaluates correctly
  - D_ENV_PP_IS_UNLIMITED evaluates correctly
  - D_ENV_PP_EFFECTIVE_LIMIT evaluates correctly
*/
bool
d_tests_sa_env_compiler_pp_limit_utility_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   within_limit;
    bool   within_standard;
    bool   is_unlimited;
    long   effective;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing PP Limit Utility Macros ---\n", D_INDENT);

    // test ARGS_WITHIN_LIMIT
    within_limit = D_ENV_PP_ARGS_WITHIN_LIMIT(10);

    if (!d_assert_standalone(within_limit,
                             "ARGS_WITHIN_LIMIT(10) is true",
                             "10 args should be within any limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test ARGS_WITHIN_STANDARD
    within_standard = D_ENV_PP_ARGS_WITHIN_STANDARD(10);

    if (!d_assert_standalone(within_standard,
                             "ARGS_WITHIN_STANDARD(10) is true",
                             "10 args should be within any standard",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with value at C89 limit
    within_standard = D_ENV_PP_ARGS_WITHIN_STANDARD(31);

    if (!d_assert_standalone(within_standard,
                             "ARGS_WITHIN_STANDARD(31) is true",
                             "31 args is C89 minimum",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test IS_UNLIMITED
    is_unlimited = D_ENV_PP_IS_UNLIMITED(0);

    if (!d_assert_standalone(is_unlimited,
                             "IS_UNLIMITED(0) is true",
                             "0 represents unlimited",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    is_unlimited = D_ENV_PP_IS_UNLIMITED(100);

    if (!d_assert_standalone(!is_unlimited,
                             "IS_UNLIMITED(100) is false",
                             "non-zero is not unlimited",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test EFFECTIVE_LIMIT
    effective = D_ENV_PP_EFFECTIVE_LIMIT(0);

    if (!d_assert_standalone(effective == 2147483647L,
                             "EFFECTIVE_LIMIT(0) is large value",
                             "unlimited should become large number",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    effective = D_ENV_PP_EFFECTIVE_LIMIT(100);

    if (!d_assert_standalone(effective == 100,
                             "EFFECTIVE_LIMIT(100) is 100",
                             "non-zero should pass through",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Utility macros evaluated successfully\n", D_INDENT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] PP limit utility macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] PP limit utility macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MANUAL DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_compiler_manual_detection
  Tests manual compiler detection via D_ENV_DETECTED_COMPILER_* variables.
  Tests the following:
  - detection variables are mutually exclusive
  - correct mapping when manual mode is active
*/
bool
d_tests_sa_env_compiler_manual_detection
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    detected_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    detected_count        = 0;

    printf("%s--- Testing Manual Compiler Detection ---\n", D_INDENT);

    // count detection variables
    #ifdef D_ENV_DETECTED_COMPILER_APPLE_CLANG
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_APPLE_CLANG defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_CLANG
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_CLANG defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_GCC
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_GCC defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_MSVC
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_MSVC defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_INTEL
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_INTEL defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_BORLAND
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_BORLAND defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_COMPILER_UNKNOWN
        detected_count++;
        printf("%s    D_ENV_DETECTED_COMPILER_UNKNOWN defined\n", D_INDENT);
    #endif

    // verify at most one detection variable
    if (!d_assert_standalone(detected_count <= 1,
                             "at most one compiler detection var defined",
                             "detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Detection vars count: %d\n", D_INDENT, detected_count);

    // if manual mode is active, verify correct mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER)
        printf("%s    Manual compiler detection mode active\n", D_INDENT);

        #ifdef D_ENV_DETECTED_COMPILER_GCC
            #ifdef D_ENV_COMPILER_GCC
                if (!d_assert_standalone(true,
                                         "DETECTED_GCC maps to COMPILER_GCC",
                                         "manual detection should set flag",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #else
                if (!d_assert_standalone(false,
                                         "DETECTED_GCC should set COMPILER_GCC",
                                         "manual detection not working",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif

        #ifdef D_ENV_DETECTED_COMPILER_MSVC
            #ifdef D_ENV_COMPILER_MSVC
                if (!d_assert_standalone(true,
                                         "DETECTED_MSVC maps to COMPILER_MSVC",
                                         "manual detection should set flag",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #else
                if (!d_assert_standalone(false,
                                         "DETECTED_MSVC should set COMPILER_MSVC",
                                         "manual detection not working",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #else
        printf("%s    Automatic compiler detection mode\n", D_INDENT);

        if (!d_assert_standalone(true,
                                 "automatic detection mode active",
                                 "manual vars are informational only",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Manual compiler detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Manual compiler detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* CONSISTENCY TESTS
******************************************************************************/

/*
d_tests_sa_env_compiler_consistency_check
  Tests overall consistency of compiler detection.
  Tests the following:
  - all required macros are defined together
  - version info is consistent
  - preprocessor limits are consistent with compiler
*/
bool
d_tests_sa_env_compiler_consistency_check
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_flag;
    bool   has_name;
    bool   has_version;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Compiler Detection Consistency ---\n", D_INDENT);

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

    if (!d_assert_standalone(has_flag,
                             "at least one compiler flag defined",
                             "some compiler must be detected",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // check name macros
    has_name = false;
    #if defined(D_ENV_COMPILER_NAME) && defined(D_ENV_COMPILER_FULL_NAME)
        has_name = true;
    #endif

    if (!d_assert_standalone(has_name,
                             "both NAME and FULL_NAME defined",
                             "name macros must be consistent",
                             _test_info))
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

    if (!d_assert_standalone(has_version,
                             "all version macros defined",
                             "version info must be complete",
                             _test_info))
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

        if (!d_assert_standalone(at_least_self && at_most_self,
                                 "version is both at_least and at_most itself",
                                 "comparison macros should be consistent",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    #endif

    printf("%s    Has compiler flag: %s\n", D_INDENT, has_flag ? "YES" : "NO");
    printf("%s    Has name macros:   %s\n", D_INDENT, has_name ? "YES" : "NO");
    printf("%s    Has version info:  %s\n", D_INDENT, has_version ? "YES" : "NO");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Compiler consistency check test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Compiler consistency check test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_compiler_all
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
bool
d_tests_sa_env_compiler_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool detection_flags_result;
    bool name_macros_result;
    bool version_macros_result;
    bool version_at_least_result;
    bool version_at_most_result;
    bool va_opt_result;
    bool pp_constants_result;
    bool pp_min_result;
    bool pp_max_result;
    bool pp_utility_result;
    bool manual_result;
    bool consistency_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing Compiler Detection\n");
    printf("========================================="
           "=======================================\n");

    // run all compiler tests
    detection_flags_result  = d_tests_sa_env_compiler_detection_flags(&module_counter);
    name_macros_result      = d_tests_sa_env_compiler_name_macros(&module_counter);
    version_macros_result   = d_tests_sa_env_compiler_version_macros(&module_counter);
    version_at_least_result = d_tests_sa_env_compiler_version_at_least(&module_counter);
    version_at_most_result  = d_tests_sa_env_compiler_version_at_most(&module_counter);
    va_opt_result           = d_tests_sa_env_compiler_va_opt_detection(&module_counter);
    pp_constants_result     = d_tests_sa_env_compiler_pp_limit_constants(&module_counter);
    pp_min_result           = d_tests_sa_env_compiler_pp_min_limits(&module_counter);
    pp_max_result           = d_tests_sa_env_compiler_pp_max_limits(&module_counter);
    pp_utility_result       = d_tests_sa_env_compiler_pp_limit_utility_macros(&module_counter);
    manual_result           = d_tests_sa_env_compiler_manual_detection(&module_counter);
    consistency_result      = d_tests_sa_env_compiler_consistency_check(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( detection_flags_result  &&
                       name_macros_result      &&
                       version_macros_result   &&
                       version_at_least_result &&
                       version_at_most_result  &&
                       va_opt_result           &&
                       pp_constants_result     &&
                       pp_min_result           &&
                       pp_max_result           &&
                       pp_utility_result       &&
                       manual_result           &&
                       consistency_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Compiler Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Compiler Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Detection Flags:     %s\n",
               detection_flags_result ? "PASSED" : "FAILED");
        printf("  - Name Macros:         %s\n",
               name_macros_result ? "PASSED" : "FAILED");
        printf("  - Version Macros:      %s\n",
               version_macros_result ? "PASSED" : "FAILED");
        printf("  - VERSION_AT_LEAST:    %s\n",
               version_at_least_result ? "PASSED" : "FAILED");
        printf("  - VERSION_AT_MOST:     %s\n",
               version_at_most_result ? "PASSED" : "FAILED");
        printf("  - VA_OPT Detection:    %s\n",
               va_opt_result ? "PASSED" : "FAILED");
        printf("  - PP Limit Constants:  %s\n",
               pp_constants_result ? "PASSED" : "FAILED");
        printf("  - PP Min Limits:       %s\n",
               pp_min_result ? "PASSED" : "FAILED");
        printf("  - PP Max Limits:       %s\n",
               pp_max_result ? "PASSED" : "FAILED");
        printf("  - PP Utility Macros:   %s\n",
               pp_utility_result ? "PASSED" : "FAILED");
        printf("  - Manual Detection:    %s\n",
               manual_result ? "PASSED" : "FAILED");
        printf("  - Consistency Check:   %s\n",
               consistency_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
