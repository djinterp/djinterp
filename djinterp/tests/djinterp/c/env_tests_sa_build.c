/******************************************************************************
* djinterp [test]                                          env_tests_sa_build.c
*
* Unit tests for `env.h` build configuration detection (section VIII).
* Tests the D_ENV_BUILD_* macros that detect debug/release build configuration.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_build.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.24
******************************************************************************/

#include "env_tests_sa.h"


/******************************************************************************
* BUILD CONFIGURATION DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_build_config_defined
  Tests that build configuration macros are defined.
  Tests the following:
  - exactly one of D_ENV_BUILD_DEBUG or D_ENV_BUILD_RELEASE is defined
  - D_ENV_BUILD_TYPE is defined
  - build configuration is consistent
*/
bool
d_tests_sa_env_build_config_defined
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    build_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Build Configuration Definition ---\n", D_INDENT);

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
    if (!d_assert_standalone(build_count == 1,
                             "exactly one build config defined",
                             "must be either debug or release, not both",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: %d build configs defined (expected 1)\n",
               D_INDENT,
               build_count);
    }

    // verify D_ENV_BUILD_TYPE is defined
    #ifdef D_ENV_BUILD_TYPE
        if (!d_assert_standalone(true,
                                 "D_ENV_BUILD_TYPE is defined",
                                 "build type string should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_BUILD_TYPE = \"%s\"\n", D_INDENT, D_ENV_BUILD_TYPE);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_BUILD_TYPE should be defined",
                                 "build type string should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Build configuration definition test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Build configuration definition test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_build_config_values
  Tests that build configuration values are correct.
  Tests the following:
  - D_ENV_BUILD_DEBUG equals 1 when defined
  - D_ENV_BUILD_RELEASE equals 1 when defined
  - D_ENV_BUILD_TYPE matches detected build configuration
*/
bool
d_tests_sa_env_build_config_values
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

    printf("%s--- Testing Build Configuration Values ---\n", D_INDENT);

    // test debug build configuration
    #ifdef D_ENV_BUILD_DEBUG
        if (!d_assert_standalone(D_ENV_BUILD_DEBUG == 1,
                                 "D_ENV_BUILD_DEBUG == 1",
                                 "debug build flag should equal 1",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify build type matches
        #ifdef D_ENV_BUILD_TYPE
            if (!d_assert_standalone((D_ENV_BUILD_TYPE[0] == 'D') &&
                                     (D_ENV_BUILD_TYPE[1] == 'e') &&
                                     (D_ENV_BUILD_TYPE[2] == 'b') &&
                                     (D_ENV_BUILD_TYPE[3] == 'u') &&
                                     (D_ENV_BUILD_TYPE[4] == 'g'),
                                     "D_ENV_BUILD_TYPE == \"Debug\"",
                                     "debug build should have Debug type string",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        printf("%s    Debug build detected\n", D_INDENT);
    #endif

    // test release build configuration
    #ifdef D_ENV_BUILD_RELEASE
        if (!d_assert_standalone(D_ENV_BUILD_RELEASE == 1,
                                 "D_ENV_BUILD_RELEASE == 1",
                                 "release build flag should equal 1",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify build type matches
        #ifdef D_ENV_BUILD_TYPE
            if (!d_assert_standalone((D_ENV_BUILD_TYPE[0] == 'R') &&
                                     (D_ENV_BUILD_TYPE[1] == 'e') &&
                                     (D_ENV_BUILD_TYPE[2] == 'l') &&
                                     (D_ENV_BUILD_TYPE[3] == 'e') &&
                                     (D_ENV_BUILD_TYPE[4] == 'a') &&
                                     (D_ENV_BUILD_TYPE[5] == 's') &&
                                     (D_ENV_BUILD_TYPE[6] == 'e'),
                                     "D_ENV_BUILD_TYPE == \"Release\"",
                                     "release build should have Release type string",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        printf("%s    Release build detected\n", D_INDENT);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Build configuration values test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Build configuration values test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_build_config_consistency
  Tests build configuration consistency with standard debug macros.
  Tests the following:
  - if DEBUG/_DEBUG defined, should be debug build
  - if NDEBUG defined, should be release build
  - build configuration is logically consistent
*/
bool
d_tests_sa_env_build_config_consistency
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

    printf("%s--- Testing Build Configuration Consistency ---\n", D_INDENT);

    // check consistency with standard debug macros
    #if defined(DEBUG) || defined(_DEBUG)
        printf("%s    DEBUG or _DEBUG is defined\n", D_INDENT);

        #ifdef D_ENV_BUILD_DEBUG
            if (!d_assert_standalone(true,
                                     "debug macros match debug build",
                                     "DEBUG/_DEBUG should indicate debug build",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            // this could be valid if NDEBUG is also defined
            #ifdef NDEBUG
                printf("%s    Note: both DEBUG and NDEBUG defined (unusual)\n",
                       D_INDENT);
            #else
                if (!d_assert_standalone(false,
                                         "DEBUG/_DEBUG should indicate debug build",
                                         "debug build should be detected",
                                         _test_info))
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
            if (!d_assert_standalone(true,
                                     "NDEBUG matches release build",
                                     "NDEBUG should indicate release build",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            // this could be valid if DEBUG/_DEBUG also defined
            #if defined(DEBUG) || defined(_DEBUG)
                printf("%s    Note: both NDEBUG and DEBUG defined (unusual)\n",
                       D_INDENT);
            #else
                if (!d_assert_standalone(false,
                                         "NDEBUG should indicate release build",
                                         "release build should be detected",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #endif

    // verify logical consistency: not both debug and release
    #if defined(D_ENV_BUILD_DEBUG) && defined(D_ENV_BUILD_RELEASE)
        if (!d_assert_standalone(false,
                                 "cannot be both debug and release",
                                 "build configuration is contradictory",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(true,
                                 "build configuration is consistent",
                                 "exactly one build type is defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    Build configuration is consistent\n", D_INDENT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Build configuration consistency test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Build configuration consistency test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* BUILD DETECTION LOGIC TESTS
******************************************************************************/

/*
d_tests_sa_env_build_detection_logic
  Tests build configuration detection logic.
  Tests the following:
  - detection logic prioritizes debug indicators
  - absence of debug indicators results in release build
  - multiple debug indicators are handled correctly
*/
bool
d_tests_sa_env_build_detection_logic
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    debug_indicator_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Build Detection Logic ---\n", D_INDENT);

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
            if (!d_assert_standalone(true,
                                     "debug indicators result in debug build",
                                     "debug build should be detected",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(false,
                                     "debug indicators should result in debug build",
                                     "debug build should be detected",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    }
    else
    {
        // no debug indicators present
        #ifdef D_ENV_BUILD_RELEASE
            if (!d_assert_standalone(true,
                                     "no debug indicators result in release build",
                                     "release build should be detected",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(false,
                                     "no debug indicators should result in release build",
                                     "release build should be detected",
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
        printf("%s[PASS] Build detection logic test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Build detection logic test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* BUILD TYPE STRING TESTS
******************************************************************************/

/*
d_tests_sa_env_build_type_string
  Tests D_ENV_BUILD_TYPE string properties.
  Tests the following:
  - string is either "Debug" or "Release"
  - string is null-terminated
  - string length is correct
*/
bool
d_tests_sa_env_build_type_string
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    size_t str_len;
    int    i;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Build Type String ---\n", D_INDENT);

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
            if (!d_assert_standalone(str_len == 5,
                                     "Debug string has length 5",
                                     "\"Debug\" should be 5 characters",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_BUILD_RELEASE
            if (!d_assert_standalone(str_len == 7,
                                     "Release string has length 7",
                                     "\"Release\" should be 7 characters",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify string is null-terminated
        if (!d_assert_standalone(D_ENV_BUILD_TYPE[str_len] == '\0',
                                 "build type string is null-terminated",
                                 "string should end with null terminator",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify no unexpected characters
        for (i = 0; i < (int)str_len; i++)
        {
            if (!d_assert_standalone((D_ENV_BUILD_TYPE[i] >= 'A' &&
                                      D_ENV_BUILD_TYPE[i] <= 'Z') ||
                                     (D_ENV_BUILD_TYPE[i] >= 'a' &&
                                      D_ENV_BUILD_TYPE[i] <= 'z'),
                                     "build type contains only letters",
                                     "string should contain only alphabetic characters",
                                     _test_info))
            {
                all_assertions_passed = false;
                break;
            }
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_BUILD_TYPE should be defined",
                                 "build type string should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Build type string test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Build type string test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* CONDITIONAL COMPILATION TESTS
******************************************************************************/

/*
d_tests_sa_env_build_conditional_compilation
  Tests conditional compilation based on build configuration.
  Tests the following:
  - code can conditionally compile based on D_ENV_BUILD_DEBUG
  - code can conditionally compile based on D_ENV_BUILD_RELEASE
  - mutually exclusive compilation works correctly
*/
bool
d_tests_sa_env_build_conditional_compilation
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    debug_code_compiled;
    int    release_code_compiled;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Conditional Compilation ---\n", D_INDENT);

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
    if (!d_assert_standalone((debug_code_compiled + release_code_compiled) == 1,
                             "exactly one build path compiled",
                             "debug and release should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify build type matches compiled code
    #ifdef D_ENV_BUILD_DEBUG
        if (!d_assert_standalone(debug_code_compiled == 1,
                                 "debug build compiles debug code",
                                 "debug-specific code should compile in debug build",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(release_code_compiled == 0,
                                 "debug build does not compile release code",
                                 "release-specific code should not compile in debug build",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        if (!d_assert_standalone(release_code_compiled == 1,
                                 "release build compiles release code",
                                 "release-specific code should compile in release build",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(debug_code_compiled == 0,
                                 "release build does not compile debug code",
                                 "debug-specific code should not compile in release build",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Conditional compilation test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Conditional compilation test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* BUILD CONFIGURATION USAGE TESTS
******************************************************************************/

/*
d_tests_sa_env_build_usage_examples
  Tests practical usage of build configuration macros.
  Tests the following:
  - build configuration can be used in preprocessor conditions
  - build type string can be used in runtime code
  - macros integrate well with existing code patterns
*/
bool
d_tests_sa_env_build_usage_examples
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    const char* build_type_ptr;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Build Configuration Usage ---\n", D_INDENT);

    // test runtime usage of build type string
    #ifdef D_ENV_BUILD_TYPE
        build_type_ptr = D_ENV_BUILD_TYPE;

        if (!d_assert_standalone(build_type_ptr != NULL,
                                 "build type string is valid pointer",
                                 "string should be usable at runtime",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Build type accessible at runtime: \"%s\"\n",
               D_INDENT,
               build_type_ptr);
    #endif

    // test compile-time conditional based on build
    #ifdef D_ENV_BUILD_DEBUG
        if (!d_assert_standalone(true,
                                 "compile-time debug checks work",
                                 "can use D_ENV_BUILD_DEBUG in conditions",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Compile-time debug checks enabled\n", D_INDENT);
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        if (!d_assert_standalone(true,
                                 "compile-time release optimizations work",
                                 "can use D_ENV_BUILD_RELEASE in conditions",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Compile-time release optimizations enabled\n", D_INDENT);
    #endif

    // test that macros evaluate to 1 (not just defined)
    #ifdef D_ENV_BUILD_DEBUG
        #if D_ENV_BUILD_DEBUG
            if (!d_assert_standalone(true,
                                     "D_ENV_BUILD_DEBUG evaluates to true",
                                     "macro should have value 1",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(false,
                                     "D_ENV_BUILD_DEBUG should evaluate to true",
                                     "macro should have value 1, not 0",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    #ifdef D_ENV_BUILD_RELEASE
        #if D_ENV_BUILD_RELEASE
            if (!d_assert_standalone(true,
                                     "D_ENV_BUILD_RELEASE evaluates to true",
                                     "macro should have value 1",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(false,
                                     "D_ENV_BUILD_RELEASE should evaluate to true",
                                     "macro should have value 1, not 0",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Build configuration usage test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Build configuration usage test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_build_all
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
bool
d_tests_sa_env_build_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool defined_result;
    bool values_result;
    bool consistency_result;
    bool detection_result;
    bool string_result;
    bool conditional_result;
    bool usage_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing Build Configuration\n");
    printf("========================================="
           "=======================================\n");

    defined_result      = d_tests_sa_env_build_config_defined(&module_counter);
    values_result       = d_tests_sa_env_build_config_values(&module_counter);
    consistency_result  = d_tests_sa_env_build_config_consistency(&module_counter);
    detection_result    = d_tests_sa_env_build_detection_logic(&module_counter);
    string_result       = d_tests_sa_env_build_type_string(&module_counter);
    conditional_result  = d_tests_sa_env_build_conditional_compilation(&module_counter);
    usage_result        = d_tests_sa_env_build_usage_examples(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( defined_result      &&
                       values_result       &&
                       consistency_result  &&
                       detection_result    &&
                       string_result       &&
                       conditional_result  &&
                       usage_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Build Configuration Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Build Configuration Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Configuration Definition:    %s\n",
               defined_result ? "PASSED" : "FAILED");
        printf("  - Configuration Values:        %s\n",
               values_result ? "PASSED" : "FAILED");
        printf("  - Configuration Consistency:   %s\n",
               consistency_result ? "PASSED" : "FAILED");
        printf("  - Detection Logic:             %s\n",
               detection_result ? "PASSED" : "FAILED");
        printf("  - Build Type String:           %s\n",
               string_result ? "PASSED" : "FAILED");
        printf("  - Conditional Compilation:     %s\n",
               conditional_result ? "PASSED" : "FAILED");
        printf("  - Usage Examples:              %s\n",
               usage_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
