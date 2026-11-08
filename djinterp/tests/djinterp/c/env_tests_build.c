/*******************************************************************************
* djinterp [test]                                              env_tests_build.c
*
* Unit tests for `env.h` build configuration detection (section VIII).
* Tests the D_ENV_BUILD_* macros that detect debug/release build configuration.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_build.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.24
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "env_tests.h"


/******************************************************************************
* BUILD CONFIGURATION DETECTION TESTS
******************************************************************************/

/*
d_tests_env_build_config_defined
  Tests that build configuration macros are defined.
  Tests the following:
  - exactly one of D_ENV_BUILD_DEBUG or D_ENV_BUILD_RELEASE is defined
  - D_ENV_BUILD_TYPE is defined
  - build configuration is consistent
*/
bool d_tests_env_build_config_defined(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    build_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_config_defined");

    all_assertions_passed = true;

    // count how many build configuration macros are defined
    build_count = 0;

    #ifdef D_ENV_BUILD_DEBUG
        build_count++;
        printf("%s    D_ENV_BUILD_DEBUG is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        build_count++;
        printf("%s    D_ENV_BUILD_RELEASE is defined\n", D_INDENT);
    #endif

    // verify exactly one build configuration is defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one build config defined",
                          build_count == 1,
                          "must be either debug or release, not both"))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: %d build configs defined (expected 1)\n",
               D_INDENT,
               build_count);
    }

    // verify D_ENV_BUILD_TYPE is defined
    #ifdef D_ENV_BUILD_TYPE
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_BUILD_TYPE is defined",
                              true,
                              "build type string should be defined"))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_BUILD_TYPE = \"%s\"\n", D_INDENT, D_ENV_BUILD_TYPE);
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_BUILD_TYPE should be defined",
                              false,
                              "build type string should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_build_config_values
  Tests that build configuration values are correct.
  Tests the following:
  - D_ENV_BUILD_DEBUG equals 1 when defined
  - D_ENV_BUILD_RELEASE equals 1 when defined
  - D_ENV_BUILD_TYPE matches detected build configuration
*/
bool d_tests_env_build_config_values(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_config_values");

    all_assertions_passed = true;

    // test debug build configuration
    #ifdef D_ENV_BUILD_DEBUG
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_BUILD_DEBUG == 1",
                              D_ENV_BUILD_DEBUG == 1,
                              "debug build flag should equal 1"))
        {
            all_assertions_passed = false;
        }

        // verify build type matches
        #ifdef D_ENV_BUILD_TYPE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_BUILD_TYPE == \"Debug\"",
                                  (D_ENV_BUILD_TYPE[0] == 'D') && (D_ENV_BUILD_TYPE[1] == 'e') && (D_ENV_BUILD_TYPE[2] == 'b') && (D_ENV_BUILD_TYPE[3] == 'u') && (D_ENV_BUILD_TYPE[4] == 'g'),
                                  "debug build should have Debug type string"))
            {
                all_assertions_passed = false;
            }
        #endif

        printf("%s    Debug build detected\n", D_INDENT);
    #endif

    // test release build configuration
    #ifdef D_ENV_BUILD_RELEASE
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_BUILD_RELEASE == 1",
                              D_ENV_BUILD_RELEASE == 1,
                              "release build flag should equal 1"))
        {
            all_assertions_passed = false;
        }

        // verify build type matches
        #ifdef D_ENV_BUILD_TYPE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_BUILD_TYPE == \"Release\"",
                                  (D_ENV_BUILD_TYPE[0] == 'R') && (D_ENV_BUILD_TYPE[1] == 'e') && (D_ENV_BUILD_TYPE[2] == 'l') && (D_ENV_BUILD_TYPE[3] == 'e') && (D_ENV_BUILD_TYPE[4] == 'a') && (D_ENV_BUILD_TYPE[5] == 's') && (D_ENV_BUILD_TYPE[6] == 'e'),
                                  "release build should have Release type string"))
            {
                all_assertions_passed = false;
            }
        #endif

        printf("%s    Release build detected\n", D_INDENT);
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_build_config_consistency
  Tests build configuration consistency with standard debug macros.
  Tests the following:
  - if DEBUG/_DEBUG defined, should be debug build
  - if NDEBUG defined, should be release build
  - build configuration is logically consistent
*/
bool d_tests_env_build_config_consistency(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_config_consistency");

    all_assertions_passed = true;

    // check consistency with standard debug macros
    #if defined(DEBUG) || defined(_DEBUG)
        printf("%s    DEBUG or _DEBUG is defined\n", D_INDENT);

        #ifdef D_ENV_BUILD_DEBUG
            if (!D_TEST_CHECK_MSG(_suite,
                                  "debug macros match debug build",
                                  true,
                                  "DEBUG/_DEBUG should indicate debug build"))
            {
                all_assertions_passed = false;
            }
        #else
            // this could be valid if NDEBUG is also defined
            #ifdef NDEBUG
                printf("%s    Note: both DEBUG and NDEBUG defined (unusual)\n",
                       D_INDENT);
            #else
                if (!D_TEST_CHECK_MSG(_suite,
                                      "DEBUG/_DEBUG should indicate debug build",
                                      false,
                                      "debug build should be detected"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #endif

    // check NDEBUG consistency
    #ifdef NDEBUG
        printf("%s    NDEBUG is defined\n", D_INDENT);

        #ifdef D_ENV_BUILD_RELEASE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "NDEBUG matches release build",
                                  true,
                                  "NDEBUG should indicate release build"))
            {
                all_assertions_passed = false;
            }
        #else
            // this could be valid if DEBUG/_DEBUG also defined
            #if defined(DEBUG) || defined(_DEBUG)
                printf("%s    Note: both NDEBUG and DEBUG defined (unusual)\n",
                       D_INDENT);
            #else
                if (!D_TEST_CHECK_MSG(_suite,
                                      "NDEBUG should indicate release build",
                                      false,
                                      "release build should be detected"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #endif

    // verify logical consistency: not both debug and release
    #if defined(D_ENV_BUILD_DEBUG) && defined(D_ENV_BUILD_RELEASE)
        if (!D_TEST_CHECK_MSG(_suite,
                              "cannot be both debug and release",
                              false,
                              "build configuration is contradictory"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "build configuration is consistent",
                              true,
                              "exactly one build type is defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    Build configuration is consistent\n", D_INDENT);

    return all_assertions_passed;
}


/******************************************************************************
* BUILD DETECTION LOGIC TESTS
******************************************************************************/

/*
d_tests_env_build_detection_logic
  Tests build configuration detection logic.
  Tests the following:
  - detection logic prioritizes debug indicators
  - absence of debug indicators results in release build
  - multiple debug indicators are handled correctly
*/
bool d_tests_env_build_detection_logic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    debug_indicator_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_detection_logic");

    all_assertions_passed = true;

    // count debug indicators
    debug_indicator_count = 0;

    #ifdef DEBUG
        debug_indicator_count++;
        printf("%s    DEBUG is defined\n", D_INDENT);
    #endif

    #ifdef _DEBUG
        debug_indicator_count++;
        printf("%s    _DEBUG is defined\n", D_INDENT);
    #endif

    #ifndef NDEBUG
        debug_indicator_count++;
        printf("%s    NDEBUG is not defined (debug indicator)\n", D_INDENT);
    #endif

    printf("%s    Debug indicators present: %d\n",
           D_INDENT,
           debug_indicator_count);

    // test detection logic
    if (debug_indicator_count > 0)
    {
        // at least one debug indicator present
        #ifdef D_ENV_BUILD_DEBUG
            if (!D_TEST_CHECK_MSG(_suite,
                                  "debug indicators result in debug build",
                                  true,
                                  "debug build should be detected"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "debug indicators should result in debug build",
                                  false,
                                  "debug build should be detected"))
            {
                all_assertions_passed = false;
            }
        #endif
    }
    else
    {
        // no debug indicators present
        #ifdef D_ENV_BUILD_RELEASE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "no debug indicators result in release build",
                                  true,
                                  "release build should be detected"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "no debug indicators should result in release build",
                                  false,
                                  "release build should be detected"))
            {
                all_assertions_passed = false;
            }
        #endif
    }

    return all_assertions_passed;
}


/******************************************************************************
* BUILD TYPE STRING TESTS
******************************************************************************/

/*
d_tests_env_build_type_string
  Tests D_ENV_BUILD_TYPE string properties.
  Tests the following:
  - string is either "Debug" or "Release"
  - string is null-terminated
  - string length is correct
*/
bool d_tests_env_build_type_string(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    size_t str_len;
    int    i;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_type_string");

    all_assertions_passed = true;

    #ifdef D_ENV_BUILD_TYPE
        // calculate string length
        str_len = 0;
        for (i = 0; D_ENV_BUILD_TYPE[i] != '\0'; i++)
        {
            str_len++;
        }

        printf("%s    D_ENV_BUILD_TYPE = \"%s\" (length %zu)\n",
               D_INDENT,
               D_ENV_BUILD_TYPE,
               str_len);

        // verify string is either "Debug" or "Release"
        #ifdef D_ENV_BUILD_DEBUG
            if (!D_TEST_CHECK_MSG(_suite,
                                  "Debug string has length 5",
                                  str_len == 5,
                                  "\"Debug\" should be 5 characters"))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_BUILD_RELEASE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "Release string has length 7",
                                  str_len == 7,
                                  "\"Release\" should be 7 characters"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify string is null-terminated
        if (!D_TEST_CHECK_MSG(_suite,
                              "build type string is null-terminated",
                              D_ENV_BUILD_TYPE[str_len] == '\0',
                              "string should end with null terminator"))
        {
            all_assertions_passed = false;
        }

        // verify no unexpected characters
        for (i = 0; i < (int)str_len; i++)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "build type contains only letters",
                                  (D_ENV_BUILD_TYPE[i] >= 'A' && D_ENV_BUILD_TYPE[i] <= 'Z') || (D_ENV_BUILD_TYPE[i] >= 'a' && D_ENV_BUILD_TYPE[i] <= 'z'),
                                  "string should contain only alphabetic characters"))
            {
                all_assertions_passed = false;
                break;
            }
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_BUILD_TYPE should be defined",
                              false,
                              "build type string should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* CONDITIONAL COMPILATION TESTS
******************************************************************************/

/*
d_tests_env_build_conditional_compilation
  Tests conditional compilation based on build configuration.
  Tests the following:
  - code can conditionally compile based on D_ENV_BUILD_DEBUG
  - code can conditionally compile based on D_ENV_BUILD_RELEASE
  - mutually exclusive compilation works correctly
*/
bool d_tests_env_build_conditional_compilation(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    debug_code_compiled;
    int    release_code_compiled;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_conditional_compilation");

    all_assertions_passed = true;

    // test debug-specific compilation
    debug_code_compiled = 0;
    #ifdef D_ENV_BUILD_DEBUG
        debug_code_compiled = 1;
    #endif

    // test release-specific compilation
    release_code_compiled = 0;
    #ifdef D_ENV_BUILD_RELEASE
        release_code_compiled = 1;
    #endif

    printf("%s    Debug code compiled: %s\n",
           D_INDENT,
           debug_code_compiled ? "yes" : "no");

    printf("%s    Release code compiled: %s\n",
           D_INDENT,
           release_code_compiled ? "yes" : "no");

    // verify exactly one is compiled
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one build path compiled",
                          (debug_code_compiled + release_code_compiled) == 1,
                          "debug and release should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }

    // verify build type matches compiled code
    #ifdef D_ENV_BUILD_DEBUG
        if (!D_TEST_CHECK_MSG(_suite,
                              "debug build compiles debug code",
                              debug_code_compiled == 1,
                              "debug-specific code should compile in debug build"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "debug build does not compile release code",
                              release_code_compiled == 0,
                              "release-specific code should not compile in debug build"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        if (!D_TEST_CHECK_MSG(_suite,
                              "release build compiles release code",
                              release_code_compiled == 1,
                              "release-specific code should compile in release build"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "release build does not compile debug code",
                              debug_code_compiled == 0,
                              "debug-specific code should not compile in release build"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* BUILD CONFIGURATION USAGE TESTS
******************************************************************************/

/*
d_tests_env_build_usage_examples
  Tests practical usage of build configuration macros.
  Tests the following:
  - build configuration can be used in preprocessor conditions
  - build type string can be used in runtime code
  - macros integrate well with existing code patterns
*/
bool d_tests_env_build_usage_examples(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    const char* build_type_ptr;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_build_usage_examples");

    all_assertions_passed = true;

    // test runtime usage of build type string
    #ifdef D_ENV_BUILD_TYPE
        build_type_ptr = D_ENV_BUILD_TYPE;

        if (!D_TEST_CHECK_MSG(_suite,
                              "build type string is valid pointer",
                              build_type_ptr != NULL,
                              "string should be usable at runtime"))
        {
            all_assertions_passed = false;
        }

        printf("%s    Build type accessible at runtime: \"%s\"\n",
               D_INDENT,
               build_type_ptr);
    #endif

    // test compile-time conditional based on build
    #ifdef D_ENV_BUILD_DEBUG
        if (!D_TEST_CHECK_MSG(_suite,
                              "compile-time debug checks work",
                              true,
                              "can use D_ENV_BUILD_DEBUG in conditions"))
        {
            all_assertions_passed = false;
        }

        printf("%s    Compile-time debug checks enabled\n", D_INDENT);
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        if (!D_TEST_CHECK_MSG(_suite,
                              "compile-time release optimizations work",
                              true,
                              "can use D_ENV_BUILD_RELEASE in conditions"))
        {
            all_assertions_passed = false;
        }

        printf("%s    Compile-time release optimizations enabled\n", D_INDENT);
    #endif

    // test that macros evaluate to 1 (not just defined)
    #ifdef D_ENV_BUILD_DEBUG
        #if D_ENV_BUILD_DEBUG
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_BUILD_DEBUG evaluates to true",
                                  true,
                                  "macro should have value 1"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_BUILD_DEBUG should evaluate to true",
                                  false,
                                  "macro should have value 1, not 0"))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        #if D_ENV_BUILD_RELEASE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_BUILD_RELEASE evaluates to true",
                                  true,
                                  "macro should have value 1"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_BUILD_RELEASE should evaluate to true",
                                  false,
                                  "macro should have value 1, not 0"))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_env_build_all
  Runs all build configuration tests.
  Tests the following:
  - build configuration definition
  - build configuration values
  - build configuration consistency
  - build detection logic
  - build type string
  - conditional compilation
  - build configuration usage
*/
bool d_tests_env_build_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_build_all");

    d_tests_env_build_config_defined(_suite);
    d_tests_env_build_config_values(_suite);
    d_tests_env_build_config_consistency(_suite);
    d_tests_env_build_detection_logic(_suite);
    d_tests_env_build_type_string(_suite);
    d_tests_env_build_conditional_compilation(_suite);
    d_tests_env_build_usage_examples(_suite);

    return true;
}
