/*******************************************************************************
* djinterp [test]                                          env_tests_pp_limits.c
*
* Unit tests for `env.h` preprocessor limits (section IV-A).
* Tests the D_ENV_PP_* macros that define standard and compiler-specific limits.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_pp_limits.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.24
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "env_tests.h"


/******************************************************************************
* STANDARD LIMIT CONSTANTS TESTS
******************************************************************************/

/*
d_tests_env_pp_limit_constants_c89
  Tests C89/C90 standard limit constant definitions.
  Tests the following:
  - D_ENV_PP_LIMIT_C89_MACRO_ARGS equals 31
  - D_ENV_PP_LIMIT_C89_NESTING_DEPTH equals 8
  - D_ENV_PP_LIMIT_C89_MACRO_IDS equals 1024
  - D_ENV_PP_LIMIT_C89_PARAMS equals 31
  - D_ENV_PP_LIMIT_C89_STRING_LENGTH equals 509
*/
bool d_tests_env_pp_limit_constants_c89(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_limit_constants_c89");

    all_assertions_passed = true;

    // verify C89 macro args limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C89_MACRO_ARGS == 31",
                          D_ENV_PP_LIMIT_C89_MACRO_ARGS == 31,
                          "C89 requires minimum 31 macro arguments"))
    {
        all_assertions_passed = false;
    }

    // verify C89 nesting depth limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C89_NESTING_DEPTH == 8",
                          D_ENV_PP_LIMIT_C89_NESTING_DEPTH == 8,
                          "C89 requires minimum 8 nesting levels"))
    {
        all_assertions_passed = false;
    }

    // verify C89 macro IDs limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C89_MACRO_IDS == 1024",
                          D_ENV_PP_LIMIT_C89_MACRO_IDS == 1024,
                          "C89 requires minimum 1024 macro identifiers"))
    {
        all_assertions_passed = false;
    }

    // verify C89 params limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C89_PARAMS == 31",
                          D_ENV_PP_LIMIT_C89_PARAMS == 31,
                          "C89 requires minimum 31 function parameters"))
    {
        all_assertions_passed = false;
    }

    // verify C89 string length limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C89_STRING_LENGTH == 509",
                          D_ENV_PP_LIMIT_C89_STRING_LENGTH == 509,
                          "C89 requires minimum 509 character strings"))
    {
        all_assertions_passed = false;
    }

    printf("%s    C89 limits: args=%d nest=%d ids=%d params=%d str=%d\n",
           D_INDENT,
           D_ENV_PP_LIMIT_C89_MACRO_ARGS,
           D_ENV_PP_LIMIT_C89_NESTING_DEPTH,
           D_ENV_PP_LIMIT_C89_MACRO_IDS,
           D_ENV_PP_LIMIT_C89_PARAMS,
           D_ENV_PP_LIMIT_C89_STRING_LENGTH);

    return all_assertions_passed;
}

/*
d_tests_env_pp_limit_constants_c99
  Tests C99/C11/C17/C23 standard limit constant definitions.
  Tests the following:
  - D_ENV_PP_LIMIT_C99_MACRO_ARGS equals 127
  - D_ENV_PP_LIMIT_C99_NESTING_DEPTH equals 15
  - D_ENV_PP_LIMIT_C99_MACRO_IDS equals 4095
  - D_ENV_PP_LIMIT_C99_PARAMS equals 127
  - D_ENV_PP_LIMIT_C99_STRING_LENGTH equals 4095
*/
bool d_tests_env_pp_limit_constants_c99(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_limit_constants_c99");

    all_assertions_passed = true;

    // verify C99 macro args limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C99_MACRO_ARGS == 127",
                          D_ENV_PP_LIMIT_C99_MACRO_ARGS == 127,
                          "C99 requires minimum 127 macro arguments"))
    {
        all_assertions_passed = false;
    }

    // verify C99 nesting depth limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C99_NESTING_DEPTH == 15",
                          D_ENV_PP_LIMIT_C99_NESTING_DEPTH == 15,
                          "C99 requires minimum 15 nesting levels"))
    {
        all_assertions_passed = false;
    }

    // verify C99 macro IDs limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C99_MACRO_IDS == 4095",
                          D_ENV_PP_LIMIT_C99_MACRO_IDS == 4095,
                          "C99 requires minimum 4095 macro identifiers"))
    {
        all_assertions_passed = false;
    }

    // verify C99 params limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C99_PARAMS == 127",
                          D_ENV_PP_LIMIT_C99_PARAMS == 127,
                          "C99 requires minimum 127 function parameters"))
    {
        all_assertions_passed = false;
    }

    // verify C99 string length limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_C99_STRING_LENGTH == 4095",
                          D_ENV_PP_LIMIT_C99_STRING_LENGTH == 4095,
                          "C99 requires minimum 4095 character strings"))
    {
        all_assertions_passed = false;
    }

    printf("%s    C99 limits: args=%d nest=%d ids=%d params=%d str=%d\n",
           D_INDENT,
           D_ENV_PP_LIMIT_C99_MACRO_ARGS,
           D_ENV_PP_LIMIT_C99_NESTING_DEPTH,
           D_ENV_PP_LIMIT_C99_MACRO_IDS,
           D_ENV_PP_LIMIT_C99_PARAMS,
           D_ENV_PP_LIMIT_C99_STRING_LENGTH);

    return all_assertions_passed;
}

/*
d_tests_env_pp_limit_constants_cpp
  Tests C++ standard limit constant definitions.
  Tests the following:
  - D_ENV_PP_LIMIT_CPP_MACRO_ARGS equals 256
  - D_ENV_PP_LIMIT_CPP_NESTING_DEPTH equals 256
  - D_ENV_PP_LIMIT_CPP_MACRO_IDS equals 65536
  - D_ENV_PP_LIMIT_CPP_PARAMS equals 256
  - D_ENV_PP_LIMIT_CPP_STRING_LENGTH equals 65536
*/
bool d_tests_env_pp_limit_constants_cpp(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_limit_constants_cpp");

    all_assertions_passed = true;

    // verify C++ macro args limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_CPP_MACRO_ARGS == 256",
                          D_ENV_PP_LIMIT_CPP_MACRO_ARGS == 256,
                          "C++ requires minimum 256 macro arguments"))
    {
        all_assertions_passed = false;
    }

    // verify C++ nesting depth limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_CPP_NESTING_DEPTH == 256",
                          D_ENV_PP_LIMIT_CPP_NESTING_DEPTH == 256,
                          "C++ requires minimum 256 nesting levels"))
    {
        all_assertions_passed = false;
    }

    // verify C++ macro IDs limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_CPP_MACRO_IDS == 65536",
                          D_ENV_PP_LIMIT_CPP_MACRO_IDS == 65536,
                          "C++ requires minimum 65536 macro identifiers"))
    {
        all_assertions_passed = false;
    }

    // verify C++ params limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_CPP_PARAMS == 256",
                          D_ENV_PP_LIMIT_CPP_PARAMS == 256,
                          "C++ requires minimum 256 function parameters"))
    {
        all_assertions_passed = false;
    }

    // verify C++ string length limit
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_LIMIT_CPP_STRING_LENGTH == 65536",
                          D_ENV_PP_LIMIT_CPP_STRING_LENGTH == 65536,
                          "C++ requires minimum 65536 character strings"))
    {
        all_assertions_passed = false;
    }

    printf("%s    C++ limits: args=%d nest=%d ids=%d params=%d str=%d\n",
           D_INDENT,
           D_ENV_PP_LIMIT_CPP_MACRO_ARGS,
           D_ENV_PP_LIMIT_CPP_NESTING_DEPTH,
           D_ENV_PP_LIMIT_CPP_MACRO_IDS,
           D_ENV_PP_LIMIT_CPP_PARAMS,
           D_ENV_PP_LIMIT_CPP_STRING_LENGTH);

    return all_assertions_passed;
}

/*
d_tests_env_pp_limit_constants_all
  Runs all standard limit constant tests.
  Tests the following:
  - C89 standard limits
  - C99 standard limits
  - C++ standard limits
*/
bool d_tests_env_pp_limit_constants_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_pp_limit_constants_all");

    d_tests_env_pp_limit_constants_c89(_suite);
    d_tests_env_pp_limit_constants_c99(_suite);
    d_tests_env_pp_limit_constants_cpp(_suite);

    return true;
}


/******************************************************************************
* STANDARD-BASED MINIMUM LIMITS TESTS
******************************************************************************/

/*
d_tests_env_pp_min_limits_definition
  Tests that standard-based minimum limit macros are defined.
  Tests the following:
  - D_ENV_PP_MIN_MACRO_ARGS is defined
  - D_ENV_PP_MIN_NESTING_DEPTH is defined
  - D_ENV_PP_MIN_MACRO_IDS is defined
  - D_ENV_PP_MIN_PARAMS is defined
  - D_ENV_PP_MIN_STRING_LENGTH is defined
*/
bool d_tests_env_pp_min_limits_definition(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_min_limits_definition");

    all_assertions_passed = true;

    // verify all minimum limit macros are defined
    #ifdef D_ENV_PP_MIN_MACRO_ARGS
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_ARGS is defined",
                              true,
                              "minimum macro args limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_ARGS should be defined",
                              false,
                              "minimum macro args limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_NESTING_DEPTH
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_NESTING_DEPTH is defined",
                              true,
                              "minimum nesting depth limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_NESTING_DEPTH should be defined",
                              false,
                              "minimum nesting depth limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_MACRO_IDS
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_IDS is defined",
                              true,
                              "minimum macro IDs limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_MACRO_IDS should be defined",
                              false,
                              "minimum macro IDs limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_PARAMS
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_PARAMS is defined",
                              true,
                              "minimum params limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_PARAMS should be defined",
                              false,
                              "minimum params limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_STRING_LENGTH
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_STRING_LENGTH is defined",
                              true,
                              "minimum string length limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MIN_STRING_LENGTH should be defined",
                              false,
                              "minimum string length limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_pp_min_limits_values
  Tests that minimum limit values are correct for detected language standard.
  Tests the following:
  - minimum limits match C++ limits when compiling as C++
  - minimum limits match C99 limits when compiling as C99 or higher
  - minimum limits match C89 limits when compiling as C89/C95
  - minimum limits are positive values
*/
bool d_tests_env_pp_min_limits_values(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_min_limits_values");

    all_assertions_passed = true;

    // verify all minimum limits are positive
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MIN_MACRO_ARGS > 0",
                          D_ENV_PP_MIN_MACRO_ARGS > 0,
                          "minimum macro args must be positive"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MIN_NESTING_DEPTH > 0",
                          D_ENV_PP_MIN_NESTING_DEPTH > 0,
                          "minimum nesting depth must be positive"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MIN_MACRO_IDS > 0",
                          D_ENV_PP_MIN_MACRO_IDS > 0,
                          "minimum macro IDs must be positive"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MIN_PARAMS > 0",
                          D_ENV_PP_MIN_PARAMS > 0,
                          "minimum params must be positive"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MIN_STRING_LENGTH > 0",
                          D_ENV_PP_MIN_STRING_LENGTH > 0,
                          "minimum string length must be positive"))
    {
        all_assertions_passed = false;
    }

    // test language standard-specific limits
    #ifdef D_ENV_LANG_CPP_STANDARD
        printf("%s    Detected C++ standard\n", D_INDENT);

        if (!D_TEST_CHECK_MSG(_suite,
                              "C++ min macro args == C++ standard limit",
                              D_ENV_PP_MIN_MACRO_ARGS == D_ENV_PP_LIMIT_CPP_MACRO_ARGS,
                              "C++ should use C++ standard limits"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "C++ min nesting depth == C++ standard limit",
                              D_ENV_PP_MIN_NESTING_DEPTH == D_ENV_PP_LIMIT_CPP_NESTING_DEPTH,
                              "C++ should use C++ standard limits"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "C++ min macro IDs == C++ standard limit",
                              D_ENV_PP_MIN_MACRO_IDS == D_ENV_PP_LIMIT_CPP_MACRO_IDS,
                              "C++ should use C++ standard limits"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "C++ min params == C++ standard limit",
                              D_ENV_PP_MIN_PARAMS == D_ENV_PP_LIMIT_CPP_PARAMS,
                              "C++ should use C++ standard limits"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "C++ min string length == C++ standard limit",
                              D_ENV_PP_MIN_STRING_LENGTH == D_ENV_PP_LIMIT_CPP_STRING_LENGTH,
                              "C++ should use C++ standard limits"))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_LANG_IS_C99_OR_HIGHER)
        #if D_ENV_LANG_IS_C99_OR_HIGHER
            printf("%s    Detected C99 or higher standard\n", D_INDENT);

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C99+ min macro args == C99 standard limit",
                                  D_ENV_PP_MIN_MACRO_ARGS == D_ENV_PP_LIMIT_C99_MACRO_ARGS,
                                  "C99+ should use C99 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C99+ min nesting depth == C99 standard limit",
                                  D_ENV_PP_MIN_NESTING_DEPTH == D_ENV_PP_LIMIT_C99_NESTING_DEPTH,
                                  "C99+ should use C99 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C99+ min macro IDs == C99 standard limit",
                                  D_ENV_PP_MIN_MACRO_IDS == D_ENV_PP_LIMIT_C99_MACRO_IDS,
                                  "C99+ should use C99 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C99+ min params == C99 standard limit",
                                  D_ENV_PP_MIN_PARAMS == D_ENV_PP_LIMIT_C99_PARAMS,
                                  "C99+ should use C99 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C99+ min string length == C99 standard limit",
                                  D_ENV_PP_MIN_STRING_LENGTH == D_ENV_PP_LIMIT_C99_STRING_LENGTH,
                                  "C99+ should use C99 standard limits"))
            {
                all_assertions_passed = false;
            }
        #else
            printf("%s    Detected C89/C95 standard\n", D_INDENT);

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C89 min macro args == C89 standard limit",
                                  D_ENV_PP_MIN_MACRO_ARGS == D_ENV_PP_LIMIT_C89_MACRO_ARGS,
                                  "C89 should use C89 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C89 min nesting depth == C89 standard limit",
                                  D_ENV_PP_MIN_NESTING_DEPTH == D_ENV_PP_LIMIT_C89_NESTING_DEPTH,
                                  "C89 should use C89 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C89 min macro IDs == C89 standard limit",
                                  D_ENV_PP_MIN_MACRO_IDS == D_ENV_PP_LIMIT_C89_MACRO_IDS,
                                  "C89 should use C89 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C89 min params == C89 standard limit",
                                  D_ENV_PP_MIN_PARAMS == D_ENV_PP_LIMIT_C89_PARAMS,
                                  "C89 should use C89 standard limits"))
            {
                all_assertions_passed = false;
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C89 min string length == C89 standard limit",
                                  D_ENV_PP_MIN_STRING_LENGTH == D_ENV_PP_LIMIT_C89_STRING_LENGTH,
                                  "C89 should use C89 standard limits"))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    printf("%s    Minimum limits: args=%d nest=%d ids=%d params=%d str=%d\n",
           D_INDENT,
           D_ENV_PP_MIN_MACRO_ARGS,
           D_ENV_PP_MIN_NESTING_DEPTH,
           D_ENV_PP_MIN_MACRO_IDS,
           D_ENV_PP_MIN_PARAMS,
           D_ENV_PP_MIN_STRING_LENGTH);

    return all_assertions_passed;
}

/*
d_tests_env_pp_min_limits_all
  Runs all standard-based minimum limit tests.
  Tests the following:
  - minimum limits are defined
  - minimum limits have correct values for detected language standard
*/
bool d_tests_env_pp_min_limits_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_pp_min_limits_all");

    d_tests_env_pp_min_limits_definition(_suite);
    d_tests_env_pp_min_limits_values(_suite);

    return true;
}


/******************************************************************************
* COMPILER-SPECIFIC MAXIMUM LIMITS TESTS
******************************************************************************/

/*
d_tests_env_pp_max_limits_definition
  Tests that compiler-specific maximum limit macros are defined.
  Tests the following:
  - D_ENV_PP_MAX_MACRO_ARGS is defined
  - D_ENV_PP_MAX_NESTING_DEPTH is defined
  - D_ENV_PP_MAX_MACRO_IDS is defined
  - D_ENV_PP_MAX_STRING_LENGTH is defined
  - D_ENV_PP_LIMIT_SOURCE is defined
*/
bool d_tests_env_pp_max_limits_definition(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_max_limits_definition");

    all_assertions_passed = true;

    // verify all maximum limit macros are defined
    #ifdef D_ENV_PP_MAX_MACRO_ARGS
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_ARGS is defined",
                              true,
                              "maximum macro args limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_ARGS should be defined",
                              false,
                              "maximum macro args limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MAX_NESTING_DEPTH
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_NESTING_DEPTH is defined",
                              true,
                              "maximum nesting depth limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_NESTING_DEPTH should be defined",
                              false,
                              "maximum nesting depth limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MAX_MACRO_IDS
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_IDS is defined",
                              true,
                              "maximum macro IDs limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_MACRO_IDS should be defined",
                              false,
                              "maximum macro IDs limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MAX_STRING_LENGTH
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_STRING_LENGTH is defined",
                              true,
                              "maximum string length limit should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_MAX_STRING_LENGTH should be defined",
                              false,
                              "maximum string length limit should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_LIMIT_SOURCE
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_LIMIT_SOURCE is defined",
                              true,
                              "limit source string should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_LIMIT_SOURCE should be defined",
                              false,
                              "limit source string should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_pp_max_limits_values
  Tests that maximum limit values are reasonable and exceed minimums.
  Tests the following:
  - maximum limits are non-negative (0 = unlimited)
  - maximum limits are greater than or equal to minimum limits
  - known compiler limits match expected values
  - limit source string is appropriate
*/
bool d_tests_env_pp_max_limits_values(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_max_limits_values");

    all_assertions_passed = true;

    // verify all maximum limits are non-negative (0 is valid for unlimited)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MAX_MACRO_ARGS >= 0",
                          D_ENV_PP_MAX_MACRO_ARGS >= 0,
                          "maximum macro args must be non-negative"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MAX_NESTING_DEPTH >= 0",
                          D_ENV_PP_MAX_NESTING_DEPTH >= 0,
                          "maximum nesting depth must be non-negative"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MAX_MACRO_IDS >= 0",
                          D_ENV_PP_MAX_MACRO_IDS >= 0,
                          "maximum macro IDs must be non-negative"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_MAX_STRING_LENGTH >= 0",
                          D_ENV_PP_MAX_STRING_LENGTH >= 0,
                          "maximum string length must be non-negative"))
    {
        all_assertions_passed = false;
    }

    // verify maximum limits meet or exceed minimum limits (0 = unlimited always passes)
    if (!D_TEST_CHECK_MSG(_suite,
                          "max macro args >= min macro args (or unlimited)",
                          (D_ENV_PP_MAX_MACRO_ARGS == 0) || (D_ENV_PP_MAX_MACRO_ARGS >= D_ENV_PP_MIN_MACRO_ARGS),
                          "maximum should meet or exceed standard minimum"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "max nesting depth >= min nesting depth (or unlimited)",
                          (D_ENV_PP_MAX_NESTING_DEPTH == 0) || (D_ENV_PP_MAX_NESTING_DEPTH >= D_ENV_PP_MIN_NESTING_DEPTH),
                          "maximum should meet or exceed standard minimum"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "max macro IDs >= min macro IDs (or unlimited)",
                          (D_ENV_PP_MAX_MACRO_IDS == 0) || (D_ENV_PP_MAX_MACRO_IDS >= D_ENV_PP_MIN_MACRO_IDS),
                          "maximum should meet or exceed standard minimum"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "max string length >= min string length (or unlimited)",
                          (D_ENV_PP_MAX_STRING_LENGTH == 0) || (D_ENV_PP_MAX_STRING_LENGTH >= D_ENV_PP_MIN_STRING_LENGTH),
                          "maximum should meet or exceed standard minimum"))
    {
        all_assertions_passed = false;
    }

    printf("%s    Maximum limits: args=%d nest=%d ids=%d str=%d\n",
           D_INDENT,
           D_ENV_PP_MAX_MACRO_ARGS,
           D_ENV_PP_MAX_NESTING_DEPTH,
           D_ENV_PP_MAX_MACRO_IDS,
           D_ENV_PP_MAX_STRING_LENGTH);

    #ifdef D_ENV_PP_LIMIT_SOURCE
        printf("%s    Limit source: %s\n", D_INDENT, D_ENV_PP_LIMIT_SOURCE);
    #endif

    // test specific known compiler limits
    #if defined(D_ENV_COMPILER_GCC)
        printf("%s    Detected GCC compiler\n", D_INDENT);

        if (!D_TEST_CHECK_MSG(_suite,
                              "GCC max macro args == 10000",
                              D_ENV_PP_MAX_MACRO_ARGS == 10000,
                              "GCC should support 10000 macro arguments"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "GCC max nesting depth == 200",
                              D_ENV_PP_MAX_NESTING_DEPTH == 200,
                              "GCC should support 200 nesting levels"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "GCC max macro IDs == 0 (unlimited)",
                              D_ENV_PP_MAX_MACRO_IDS == 0,
                              "GCC should have unlimited macro identifiers"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "GCC max string length == 0 (unlimited)",
                              D_ENV_PP_MAX_STRING_LENGTH == 0,
                              "GCC should have unlimited string length"))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_COMPILER_CLANG)
        printf("%s    Detected Clang compiler\n", D_INDENT);

        if (!D_TEST_CHECK_MSG(_suite,
                              "Clang max macro args == 10000",
                              D_ENV_PP_MAX_MACRO_ARGS == 10000,
                              "Clang should support 10000 macro arguments"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "Clang max nesting depth == 256",
                              D_ENV_PP_MAX_NESTING_DEPTH == 256,
                              "Clang should support 256 nesting levels"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "Clang max macro IDs == 0 (unlimited)",
                              D_ENV_PP_MAX_MACRO_IDS == 0,
                              "Clang should have unlimited macro identifiers"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "Clang max string length == 0 (unlimited)",
                              D_ENV_PP_MAX_STRING_LENGTH == 0,
                              "Clang should have unlimited string length"))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_COMPILER_MSVC)
        printf("%s    Detected MSVC compiler\n", D_INDENT);

        if (!D_TEST_CHECK_MSG(_suite,
                              "MSVC max macro args == 127",
                              D_ENV_PP_MAX_MACRO_ARGS == 127,
                              "MSVC should support 127 macro arguments"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "MSVC max nesting depth == 256",
                              D_ENV_PP_MAX_NESTING_DEPTH == 256,
                              "MSVC should support 256 nesting levels"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "MSVC max macro IDs == 16380",
                              D_ENV_PP_MAX_MACRO_IDS == 16380,
                              "MSVC should support 16380 macro identifiers"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "MSVC max string length == 16380",
                              D_ENV_PP_MAX_STRING_LENGTH == 16380,
                              "MSVC should support 16380 character strings"))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_COMPILER_BORLAND)
        printf("%s    Detected Borland compiler\n", D_INDENT);

        if (!D_TEST_CHECK_MSG(_suite,
                              "Borland max macro args == 32",
                              D_ENV_PP_MAX_MACRO_ARGS == 32,
                              "Borland should support 32 macro arguments"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "Borland max nesting depth == 32",
                              D_ENV_PP_MAX_NESTING_DEPTH == 32,
                              "Borland should support 32 nesting levels"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "Borland max macro IDs == 1024",
                              D_ENV_PP_MAX_MACRO_IDS == 1024,
                              "Borland should support 1024 macro identifiers"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "Borland max string length == 4096",
                              D_ENV_PP_MAX_STRING_LENGTH == 4096,
                              "Borland should support 4096 character strings"))
        {
            all_assertions_passed = false;
        }
    #else
        printf("%s    Unknown or undetected compiler\n", D_INDENT);

        // for unknown compilers, max should equal min (fallback to standard minimums)
        if (!D_TEST_CHECK_MSG(_suite,
                              "unknown compiler: max == min for macro args",
                              D_ENV_PP_MAX_MACRO_ARGS == D_ENV_PP_MIN_MACRO_ARGS,
                              "unknown compilers should use standard minimums"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_pp_max_limits_all
  Runs all compiler-specific maximum limit tests.
  Tests the following:
  - maximum limits are defined
  - maximum limits have reasonable values
  - compiler-specific limits match expected values
*/
bool d_tests_env_pp_max_limits_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_pp_max_limits_all");

    d_tests_env_pp_max_limits_definition(_suite);
    d_tests_env_pp_max_limits_values(_suite);

    return true;
}


/******************************************************************************
* UTILITY MACROS TESTS
******************************************************************************/

/*
d_tests_env_pp_utility_macros_definition
  Tests that preprocessor utility macros are defined.
  Tests the following:
  - D_ENV_PP_ARGS_WITHIN_LIMIT is defined
  - D_ENV_PP_ARGS_WITHIN_STANDARD is defined
  - D_ENV_PP_IS_UNLIMITED is defined
  - D_ENV_PP_EFFECTIVE_LIMIT is defined
*/
bool d_tests_env_pp_utility_macros_definition(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_utility_macros_definition");

    all_assertions_passed = true;

    // verify all utility macros are defined by testing their expansion
    #ifdef D_ENV_PP_ARGS_WITHIN_LIMIT
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_ARGS_WITHIN_LIMIT is defined",
                              true,
                              "utility macro should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_ARGS_WITHIN_LIMIT should be defined",
                              false,
                              "utility macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_ARGS_WITHIN_STANDARD
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_ARGS_WITHIN_STANDARD is defined",
                              true,
                              "utility macro should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_ARGS_WITHIN_STANDARD should be defined",
                              false,
                              "utility macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_IS_UNLIMITED
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_IS_UNLIMITED is defined",
                              true,
                              "utility macro should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_IS_UNLIMITED should be defined",
                              false,
                              "utility macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_EFFECTIVE_LIMIT
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_EFFECTIVE_LIMIT is defined",
                              true,
                              "utility macro should be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_EFFECTIVE_LIMIT should be defined",
                              false,
                              "utility macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_pp_utility_macros_functionality
  Tests that preprocessor utility macros work correctly.
  Tests the following:
  - D_ENV_PP_ARGS_WITHIN_LIMIT correctly evaluates bounds
  - D_ENV_PP_ARGS_WITHIN_STANDARD correctly evaluates bounds
  - D_ENV_PP_IS_UNLIMITED correctly identifies unlimited (0) values
  - D_ENV_PP_EFFECTIVE_LIMIT returns correct effective limits
*/
bool d_tests_env_pp_utility_macros_functionality(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_utility_macros_functionality");

    all_assertions_passed = true;

    // test D_ENV_PP_ARGS_WITHIN_LIMIT
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_ARGS_WITHIN_LIMIT(1) == true",
                          D_ENV_PP_ARGS_WITHIN_LIMIT(1),
                          "1 argument should always be within limit"))
    {
        all_assertions_passed = false;
    }

    if (D_ENV_PP_MAX_MACRO_ARGS > 0)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "count at max limit should be within limit",
                              D_ENV_PP_ARGS_WITHIN_LIMIT(D_ENV_PP_MAX_MACRO_ARGS),
                              "exact maximum should be acceptable"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "count above max limit should be outside limit",
                              !D_ENV_PP_ARGS_WITHIN_LIMIT(D_ENV_PP_MAX_MACRO_ARGS + 1),
                              "exceeding maximum should fail"))
        {
            all_assertions_passed = false;
        }
    }

    // test D_ENV_PP_ARGS_WITHIN_STANDARD
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_ARGS_WITHIN_STANDARD(1) == true",
                          D_ENV_PP_ARGS_WITHIN_STANDARD(1),
                          "1 argument should always be within standard"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "count at min standard should be within standard",
                          D_ENV_PP_ARGS_WITHIN_STANDARD(D_ENV_PP_MIN_MACRO_ARGS),
                          "exact minimum should be acceptable"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "count above min standard should be outside standard",
                          !D_ENV_PP_ARGS_WITHIN_STANDARD(D_ENV_PP_MIN_MACRO_ARGS + 1),
                          "exceeding minimum should fail"))
    {
        all_assertions_passed = false;
    }

    // test D_ENV_PP_IS_UNLIMITED
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_IS_UNLIMITED(0) == true",
                          D_ENV_PP_IS_UNLIMITED(0),
                          "0 should be recognized as unlimited"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_IS_UNLIMITED(1) == false",
                          !D_ENV_PP_IS_UNLIMITED(1),
                          "positive values should not be unlimited"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_IS_UNLIMITED(127) == false",
                          !D_ENV_PP_IS_UNLIMITED(127),
                          "specific limits should not be unlimited"))
    {
        all_assertions_passed = false;
    }

    // test D_ENV_PP_EFFECTIVE_LIMIT
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_EFFECTIVE_LIMIT(0) == INT_MAX",
                          D_ENV_PP_EFFECTIVE_LIMIT(0) == 2147483647L,
                          "unlimited should map to practical maximum"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_EFFECTIVE_LIMIT(127) == 127",
                          D_ENV_PP_EFFECTIVE_LIMIT(127) == 127,
                          "specific limit should return itself"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_PP_EFFECTIVE_LIMIT(256) == 256",
                          D_ENV_PP_EFFECTIVE_LIMIT(256) == 256,
                          "specific limit should return itself"))
    {
        all_assertions_passed = false;
    }

    printf("%s    D_ENV_PP_ARGS_WITHIN_LIMIT(64) = %d\n",
           D_INDENT,
           D_ENV_PP_ARGS_WITHIN_LIMIT(64));

    printf("%s    D_ENV_PP_ARGS_WITHIN_STANDARD(30) = %d\n",
           D_INDENT,
           D_ENV_PP_ARGS_WITHIN_STANDARD(30));

    printf("%s    D_ENV_PP_IS_UNLIMITED(D_ENV_PP_MAX_MACRO_IDS) = %d\n",
           D_INDENT,
           D_ENV_PP_IS_UNLIMITED(D_ENV_PP_MAX_MACRO_IDS));

    printf("%s    D_ENV_PP_EFFECTIVE_LIMIT(D_ENV_PP_MAX_MACRO_IDS) = %ld\n",
           D_INDENT,
           D_ENV_PP_EFFECTIVE_LIMIT(D_ENV_PP_MAX_MACRO_IDS));

    return all_assertions_passed;
}

/*
d_tests_env_pp_utility_macros_all
  Runs all preprocessor utility macro tests.
  Tests the following:
  - utility macros are defined
  - utility macros function correctly
*/
bool d_tests_env_pp_utility_macros_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_pp_utility_macros_all");

    d_tests_env_pp_utility_macros_definition(_suite);
    d_tests_env_pp_utility_macros_functionality(_suite);

    return true;
}


/******************************************************************************
* VA_OPT DETECTION TESTS
******************************************************************************/

/*
d_tests_env_pp_va_opt_detection
  Tests __VA_OPT__ feature detection.
  Tests the following:
  - D_ENV_PP_HAS_VA_OPT is defined
  - D_ENV_PP_HAS_VA_OPT_ENABLED is defined
  - detection is consistent with language standard
*/
bool d_tests_env_pp_va_opt_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_pp_va_opt_detection");

    all_assertions_passed = true;

    // verify D_ENV_PP_HAS_VA_OPT is defined
    #ifdef D_ENV_PP_HAS_VA_OPT
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT is defined",
                              true,
                              "__VA_OPT__ detection macro should exist"))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_PP_HAS_VA_OPT = %d\n",
               D_INDENT,
               D_ENV_PP_HAS_VA_OPT);

        // verify value is either 0 or 1
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT is 0 or 1",
                              (D_ENV_PP_HAS_VA_OPT == 0) || (D_ENV_PP_HAS_VA_OPT == 1),
                              "__VA_OPT__ detection should be boolean"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT should be defined",
                              false,
                              "__VA_OPT__ detection macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_HAS_VA_OPT_ENABLED is defined
    #ifdef D_ENV_PP_HAS_VA_OPT_ENABLED
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT_ENABLED is defined",
                              true,
                              "__VA_OPT__ enabled macro should exist"))
        {
            all_assertions_passed = false;
        }

        // verify it evaluates to same as D_ENV_PP_HAS_VA_OPT
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT_ENABLED == D_ENV_PP_HAS_VA_OPT",
                              D_ENV_PP_HAS_VA_OPT_ENABLED == D_ENV_PP_HAS_VA_OPT,
                              "enabled macro should match detection macro"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_PP_HAS_VA_OPT_ENABLED should be defined",
                              false,
                              "__VA_OPT__ enabled macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // test consistency with language standard for C++
    #ifdef D_ENV_LANG_CPP_STANDARD
        #if ( defined(__cpp_va_opt) &&  \
              (__cpp_va_opt >= 201803L) )
            // C++20 or later should support __VA_OPT__
            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++20+ should detect __VA_OPT__ support",
                                  D_ENV_PP_HAS_VA_OPT == 1,
                                  "__cpp_va_opt feature test macro indicates support"))
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
d_tests_env_pp_limits_all
  Runs all preprocessor limits tests.
  Tests the following:
  - standard limit constants (C89, C99, C++)
  - standard-based minimum limits
  - compiler-specific maximum limits
  - utility macros
  - __VA_OPT__ detection
*/
bool d_tests_env_pp_limits_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_pp_limits_all");

    d_tests_env_pp_limit_constants_all(_suite);
    d_tests_env_pp_min_limits_all(_suite);
    d_tests_env_pp_max_limits_all(_suite);
    d_tests_env_pp_utility_macros_all(_suite);
    d_tests_env_pp_va_opt_detection(_suite);

    return true;
}
