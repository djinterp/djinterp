/******************************************************************************
* djinterp [test]                                      env_tests_sa_pp_limits.c
*
* Unit tests for `env.h` preprocessor limits (section IV-A).
* Tests the D_ENV_PP_* macros that define standard and compiler-specific limits.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_pp_limits.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.24
******************************************************************************/

#include "env_tests_sa.h"


/******************************************************************************
* STANDARD LIMIT CONSTANTS TESTS
******************************************************************************/

/*
d_tests_sa_env_pp_limit_constants_c89
  Tests C89/C90 standard limit constant definitions.
  Tests the following:
  - D_ENV_PP_LIMIT_C89_MACRO_ARGS equals 31
  - D_ENV_PP_LIMIT_C89_NESTING_DEPTH equals 8
  - D_ENV_PP_LIMIT_C89_MACRO_IDS equals 1024
  - D_ENV_PP_LIMIT_C89_PARAMS equals 31
  - D_ENV_PP_LIMIT_C89_STRING_LENGTH equals 509
*/
bool
d_tests_sa_env_pp_limit_constants_c89
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

    printf("%s--- Testing C89 Standard Limit Constants ---\n", D_INDENT);

    // verify C89 macro args limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_MACRO_ARGS == 31,
                             "D_ENV_PP_LIMIT_C89_MACRO_ARGS == 31",
                             "C89 requires minimum 31 macro arguments",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C89 nesting depth limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_NESTING_DEPTH == 8,
                             "D_ENV_PP_LIMIT_C89_NESTING_DEPTH == 8",
                             "C89 requires minimum 8 nesting levels",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C89 macro IDs limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_MACRO_IDS == 1024,
                             "D_ENV_PP_LIMIT_C89_MACRO_IDS == 1024",
                             "C89 requires minimum 1024 macro identifiers",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C89 params limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_PARAMS == 31,
                             "D_ENV_PP_LIMIT_C89_PARAMS == 31",
                             "C89 requires minimum 31 function parameters",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C89 string length limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C89_STRING_LENGTH == 509,
                             "D_ENV_PP_LIMIT_C89_STRING_LENGTH == 509",
                             "C89 requires minimum 509 character strings",
                             _test_info))
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

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C89 limit constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C89 limit constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_limit_constants_c99
  Tests C99/C11/C17/C23 standard limit constant definitions.
  Tests the following:
  - D_ENV_PP_LIMIT_C99_MACRO_ARGS equals 127
  - D_ENV_PP_LIMIT_C99_NESTING_DEPTH equals 15
  - D_ENV_PP_LIMIT_C99_MACRO_IDS equals 4095
  - D_ENV_PP_LIMIT_C99_PARAMS equals 127
  - D_ENV_PP_LIMIT_C99_STRING_LENGTH equals 4095
*/
bool
d_tests_sa_env_pp_limit_constants_c99
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

    printf("%s--- Testing C99 Standard Limit Constants ---\n", D_INDENT);

    // verify C99 macro args limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_MACRO_ARGS == 127,
                             "D_ENV_PP_LIMIT_C99_MACRO_ARGS == 127",
                             "C99 requires minimum 127 macro arguments",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C99 nesting depth limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_NESTING_DEPTH == 15,
                             "D_ENV_PP_LIMIT_C99_NESTING_DEPTH == 15",
                             "C99 requires minimum 15 nesting levels",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C99 macro IDs limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_MACRO_IDS == 4095,
                             "D_ENV_PP_LIMIT_C99_MACRO_IDS == 4095",
                             "C99 requires minimum 4095 macro identifiers",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C99 params limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_PARAMS == 127,
                             "D_ENV_PP_LIMIT_C99_PARAMS == 127",
                             "C99 requires minimum 127 function parameters",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C99 string length limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_C99_STRING_LENGTH == 4095,
                             "D_ENV_PP_LIMIT_C99_STRING_LENGTH == 4095",
                             "C99 requires minimum 4095 character strings",
                             _test_info))
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

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C99 limit constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C99 limit constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_limit_constants_cpp
  Tests C++ standard limit constant definitions.
  Tests the following:
  - D_ENV_PP_LIMIT_CPP_MACRO_ARGS equals 256
  - D_ENV_PP_LIMIT_CPP_NESTING_DEPTH equals 256
  - D_ENV_PP_LIMIT_CPP_MACRO_IDS equals 65536
  - D_ENV_PP_LIMIT_CPP_PARAMS equals 256
  - D_ENV_PP_LIMIT_CPP_STRING_LENGTH equals 65536
*/
bool
d_tests_sa_env_pp_limit_constants_cpp
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

    printf("%s--- Testing C++ Standard Limit Constants ---\n", D_INDENT);

    // verify C++ macro args limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_MACRO_ARGS == 256,
                             "D_ENV_PP_LIMIT_CPP_MACRO_ARGS == 256",
                             "C++ requires minimum 256 macro arguments",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C++ nesting depth limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_NESTING_DEPTH == 256,
                             "D_ENV_PP_LIMIT_CPP_NESTING_DEPTH == 256",
                             "C++ requires minimum 256 nesting levels",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C++ macro IDs limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_MACRO_IDS == 65536,
                             "D_ENV_PP_LIMIT_CPP_MACRO_IDS == 65536",
                             "C++ requires minimum 65536 macro identifiers",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C++ params limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_PARAMS == 256,
                             "D_ENV_PP_LIMIT_CPP_PARAMS == 256",
                             "C++ requires minimum 256 function parameters",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify C++ string length limit
    if (!d_assert_standalone(D_ENV_PP_LIMIT_CPP_STRING_LENGTH == 65536,
                             "D_ENV_PP_LIMIT_CPP_STRING_LENGTH == 65536",
                             "C++ requires minimum 65536 character strings",
                             _test_info))
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

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C++ limit constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C++ limit constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_limit_constants_all
  Runs all standard limit constant tests.
  Tests the following:
  - C89 standard limits
  - C99 standard limits
  - C++ standard limits
*/
bool
d_tests_sa_env_pp_limit_constants_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool c89_result;
    bool c99_result;
    bool cpp_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing Standard Limit Constants\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    c89_result = d_tests_sa_env_pp_limit_constants_c89(&subsection_counter);
    c99_result = d_tests_sa_env_pp_limit_constants_c99(&subsection_counter);
    cpp_result = d_tests_sa_env_pp_limit_constants_cpp(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = (c89_result && c99_result && cpp_result);

    if (overall_result)
    {
        printf("[PASS] Standard Limit Constants: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Standard Limit Constants: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* STANDARD-BASED MINIMUM LIMITS TESTS
******************************************************************************/

/*
d_tests_sa_env_pp_min_limits_definition
  Tests that standard-based minimum limit macros are defined.
  Tests the following:
  - D_ENV_PP_MIN_MACRO_ARGS is defined
  - D_ENV_PP_MIN_NESTING_DEPTH is defined
  - D_ENV_PP_MIN_MACRO_IDS is defined
  - D_ENV_PP_MIN_PARAMS is defined
  - D_ENV_PP_MIN_STRING_LENGTH is defined
*/
bool
d_tests_sa_env_pp_min_limits_definition
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

    printf("%s--- Testing Minimum Limits Definition ---\n", D_INDENT);

    // verify all minimum limit macros are defined
    #ifdef D_ENV_PP_MIN_MACRO_ARGS
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_MACRO_ARGS is defined",
                                 "minimum macro args limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_MACRO_ARGS should be defined",
                                 "minimum macro args limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_NESTING_DEPTH
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_NESTING_DEPTH is defined",
                                 "minimum nesting depth limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_NESTING_DEPTH should be defined",
                                 "minimum nesting depth limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_MACRO_IDS
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_MACRO_IDS is defined",
                                 "minimum macro IDs limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_MACRO_IDS should be defined",
                                 "minimum macro IDs limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_PARAMS
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_PARAMS is defined",
                                 "minimum params limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_PARAMS should be defined",
                                 "minimum params limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MIN_STRING_LENGTH
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MIN_STRING_LENGTH is defined",
                                 "minimum string length limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MIN_STRING_LENGTH should be defined",
                                 "minimum string length limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Minimum limits definition test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Minimum limits definition test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_min_limits_values
  Tests that minimum limit values are correct for detected language standard.
  Tests the following:
  - minimum limits match C++ limits when compiling as C++
  - minimum limits match C99 limits when compiling as C99 or higher
  - minimum limits match C89 limits when compiling as C89/C95
  - minimum limits are positive values
*/
bool
d_tests_sa_env_pp_min_limits_values
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

    printf("%s--- Testing Minimum Limits Values ---\n", D_INDENT);

    // verify all minimum limits are positive
    if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_ARGS > 0,
                             "D_ENV_PP_MIN_MACRO_ARGS > 0",
                             "minimum macro args must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MIN_NESTING_DEPTH > 0,
                             "D_ENV_PP_MIN_NESTING_DEPTH > 0",
                             "minimum nesting depth must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_IDS > 0,
                             "D_ENV_PP_MIN_MACRO_IDS > 0",
                             "minimum macro IDs must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MIN_PARAMS > 0,
                             "D_ENV_PP_MIN_PARAMS > 0",
                             "minimum params must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MIN_STRING_LENGTH > 0,
                             "D_ENV_PP_MIN_STRING_LENGTH > 0",
                             "minimum string length must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test language standard-specific limits
    #ifdef D_ENV_LANG_CPP_STANDARD
        printf("%s    Detected C++ standard\n", D_INDENT);

        if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_ARGS == D_ENV_PP_LIMIT_CPP_MACRO_ARGS,
                                 "C++ min macro args == C++ standard limit",
                                 "C++ should use C++ standard limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MIN_NESTING_DEPTH == D_ENV_PP_LIMIT_CPP_NESTING_DEPTH,
                                 "C++ min nesting depth == C++ standard limit",
                                 "C++ should use C++ standard limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_IDS == D_ENV_PP_LIMIT_CPP_MACRO_IDS,
                                 "C++ min macro IDs == C++ standard limit",
                                 "C++ should use C++ standard limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MIN_PARAMS == D_ENV_PP_LIMIT_CPP_PARAMS,
                                 "C++ min params == C++ standard limit",
                                 "C++ should use C++ standard limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MIN_STRING_LENGTH == D_ENV_PP_LIMIT_CPP_STRING_LENGTH,
                                 "C++ min string length == C++ standard limit",
                                 "C++ should use C++ standard limits",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_LANG_IS_C99_OR_HIGHER)
        #if D_ENV_LANG_IS_C99_OR_HIGHER
            printf("%s    Detected C99 or higher standard\n", D_INDENT);

            if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_ARGS == D_ENV_PP_LIMIT_C99_MACRO_ARGS,
                                     "C99+ min macro args == C99 standard limit",
                                     "C99+ should use C99 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_NESTING_DEPTH == D_ENV_PP_LIMIT_C99_NESTING_DEPTH,
                                     "C99+ min nesting depth == C99 standard limit",
                                     "C99+ should use C99 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_IDS == D_ENV_PP_LIMIT_C99_MACRO_IDS,
                                     "C99+ min macro IDs == C99 standard limit",
                                     "C99+ should use C99 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_PARAMS == D_ENV_PP_LIMIT_C99_PARAMS,
                                     "C99+ min params == C99 standard limit",
                                     "C99+ should use C99 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_STRING_LENGTH == D_ENV_PP_LIMIT_C99_STRING_LENGTH,
                                     "C99+ min string length == C99 standard limit",
                                     "C99+ should use C99 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            printf("%s    Detected C89/C95 standard\n", D_INDENT);

            if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_ARGS == D_ENV_PP_LIMIT_C89_MACRO_ARGS,
                                     "C89 min macro args == C89 standard limit",
                                     "C89 should use C89 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_NESTING_DEPTH == D_ENV_PP_LIMIT_C89_NESTING_DEPTH,
                                     "C89 min nesting depth == C89 standard limit",
                                     "C89 should use C89 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_MACRO_IDS == D_ENV_PP_LIMIT_C89_MACRO_IDS,
                                     "C89 min macro IDs == C89 standard limit",
                                     "C89 should use C89 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_PARAMS == D_ENV_PP_LIMIT_C89_PARAMS,
                                     "C89 min params == C89 standard limit",
                                     "C89 should use C89 standard limits",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(D_ENV_PP_MIN_STRING_LENGTH == D_ENV_PP_LIMIT_C89_STRING_LENGTH,
                                     "C89 min string length == C89 standard limit",
                                     "C89 should use C89 standard limits",
                                     _test_info))
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

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Minimum limits values test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Minimum limits values test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_min_limits_all
  Runs all standard-based minimum limit tests.
  Tests the following:
  - minimum limits are defined
  - minimum limits have correct values for detected language standard
*/
bool
d_tests_sa_env_pp_min_limits_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool definition_result;
    bool values_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing Standard-Based Minimum Limits\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    definition_result = d_tests_sa_env_pp_min_limits_definition(&subsection_counter);
    values_result     = d_tests_sa_env_pp_min_limits_values(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = (definition_result && values_result);

    if (overall_result)
    {
        printf("[PASS] Minimum Limits: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Minimum Limits: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* COMPILER-SPECIFIC MAXIMUM LIMITS TESTS
******************************************************************************/

/*
d_tests_sa_env_pp_max_limits_definition
  Tests that compiler-specific maximum limit macros are defined.
  Tests the following:
  - D_ENV_PP_MAX_MACRO_ARGS is defined
  - D_ENV_PP_MAX_NESTING_DEPTH is defined
  - D_ENV_PP_MAX_MACRO_IDS is defined
  - D_ENV_PP_MAX_STRING_LENGTH is defined
  - D_ENV_PP_LIMIT_SOURCE is defined
*/
bool
d_tests_sa_env_pp_max_limits_definition
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

    printf("%s--- Testing Maximum Limits Definition ---\n", D_INDENT);

    // verify all maximum limit macros are defined
    #ifdef D_ENV_PP_MAX_MACRO_ARGS
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MAX_MACRO_ARGS is defined",
                                 "maximum macro args limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MAX_MACRO_ARGS should be defined",
                                 "maximum macro args limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MAX_NESTING_DEPTH
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MAX_NESTING_DEPTH is defined",
                                 "maximum nesting depth limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MAX_NESTING_DEPTH should be defined",
                                 "maximum nesting depth limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MAX_MACRO_IDS
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MAX_MACRO_IDS is defined",
                                 "maximum macro IDs limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MAX_MACRO_IDS should be defined",
                                 "maximum macro IDs limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_MAX_STRING_LENGTH
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_MAX_STRING_LENGTH is defined",
                                 "maximum string length limit should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_MAX_STRING_LENGTH should be defined",
                                 "maximum string length limit should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_LIMIT_SOURCE
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_LIMIT_SOURCE is defined",
                                 "limit source string should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_LIMIT_SOURCE should be defined",
                                 "limit source string should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Maximum limits definition test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Maximum limits definition test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_max_limits_values
  Tests that maximum limit values are reasonable and exceed minimums.
  Tests the following:
  - maximum limits are non-negative (0 = unlimited)
  - maximum limits are greater than or equal to minimum limits
  - known compiler limits match expected values
  - limit source string is appropriate
*/
bool
d_tests_sa_env_pp_max_limits_values
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

    printf("%s--- Testing Maximum Limits Values ---\n", D_INDENT);

    // verify all maximum limits are non-negative (0 is valid for unlimited)
    if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS >= 0,
                             "D_ENV_PP_MAX_MACRO_ARGS >= 0",
                             "maximum macro args must be non-negative",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MAX_NESTING_DEPTH >= 0,
                             "D_ENV_PP_MAX_NESTING_DEPTH >= 0",
                             "maximum nesting depth must be non-negative",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_IDS >= 0,
                             "D_ENV_PP_MAX_MACRO_IDS >= 0",
                             "maximum macro IDs must be non-negative",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_MAX_STRING_LENGTH >= 0,
                             "D_ENV_PP_MAX_STRING_LENGTH >= 0",
                             "maximum string length must be non-negative",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify maximum limits meet or exceed minimum limits (0 = unlimited always passes)
    if (!d_assert_standalone((D_ENV_PP_MAX_MACRO_ARGS == 0) ||
                             (D_ENV_PP_MAX_MACRO_ARGS >= D_ENV_PP_MIN_MACRO_ARGS),
                             "max macro args >= min macro args (or unlimited)",
                             "maximum should meet or exceed standard minimum",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_ENV_PP_MAX_NESTING_DEPTH == 0) ||
                             (D_ENV_PP_MAX_NESTING_DEPTH >= D_ENV_PP_MIN_NESTING_DEPTH),
                             "max nesting depth >= min nesting depth (or unlimited)",
                             "maximum should meet or exceed standard minimum",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_ENV_PP_MAX_MACRO_IDS == 0) ||
                             (D_ENV_PP_MAX_MACRO_IDS >= D_ENV_PP_MIN_MACRO_IDS),
                             "max macro IDs >= min macro IDs (or unlimited)",
                             "maximum should meet or exceed standard minimum",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_ENV_PP_MAX_STRING_LENGTH == 0) ||
                             (D_ENV_PP_MAX_STRING_LENGTH >= D_ENV_PP_MIN_STRING_LENGTH),
                             "max string length >= min string length (or unlimited)",
                             "maximum should meet or exceed standard minimum",
                             _test_info))
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

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS == 10000,
                                 "GCC max macro args == 10000",
                                 "GCC should support 10000 macro arguments",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_NESTING_DEPTH == 200,
                                 "GCC max nesting depth == 200",
                                 "GCC should support 200 nesting levels",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_IDS == 0,
                                 "GCC max macro IDs == 0 (unlimited)",
                                 "GCC should have unlimited macro identifiers",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_STRING_LENGTH == 0,
                                 "GCC max string length == 0 (unlimited)",
                                 "GCC should have unlimited string length",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_COMPILER_CLANG)
        printf("%s    Detected Clang compiler\n", D_INDENT);

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS == 10000,
                                 "Clang max macro args == 10000",
                                 "Clang should support 10000 macro arguments",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_NESTING_DEPTH == 256,
                                 "Clang max nesting depth == 256",
                                 "Clang should support 256 nesting levels",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_IDS == 0,
                                 "Clang max macro IDs == 0 (unlimited)",
                                 "Clang should have unlimited macro identifiers",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_STRING_LENGTH == 0,
                                 "Clang max string length == 0 (unlimited)",
                                 "Clang should have unlimited string length",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_COMPILER_MSVC)
        printf("%s    Detected MSVC compiler\n", D_INDENT);

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS == 127,
                                 "MSVC max macro args == 127",
                                 "MSVC should support 127 macro arguments",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_NESTING_DEPTH == 256,
                                 "MSVC max nesting depth == 256",
                                 "MSVC should support 256 nesting levels",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_IDS == 16380,
                                 "MSVC max macro IDs == 16380",
                                 "MSVC should support 16380 macro identifiers",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_STRING_LENGTH == 16380,
                                 "MSVC max string length == 16380",
                                 "MSVC should support 16380 character strings",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

    #elif defined(D_ENV_COMPILER_BORLAND)
        printf("%s    Detected Borland compiler\n", D_INDENT);

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS == 32,
                                 "Borland max macro args == 32",
                                 "Borland should support 32 macro arguments",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_NESTING_DEPTH == 32,
                                 "Borland max nesting depth == 32",
                                 "Borland should support 32 nesting levels",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_IDS == 1024,
                                 "Borland max macro IDs == 1024",
                                 "Borland should support 1024 macro identifiers",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_PP_MAX_STRING_LENGTH == 4096,
                                 "Borland max string length == 4096",
                                 "Borland should support 4096 character strings",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        printf("%s    Unknown or undetected compiler\n", D_INDENT);

        // for unknown compilers, max should equal min (fallback to standard minimums)
        if (!d_assert_standalone(D_ENV_PP_MAX_MACRO_ARGS == D_ENV_PP_MIN_MACRO_ARGS,
                                 "unknown compiler: max == min for macro args",
                                 "unknown compilers should use standard minimums",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Maximum limits values test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Maximum limits values test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_max_limits_all
  Runs all compiler-specific maximum limit tests.
  Tests the following:
  - maximum limits are defined
  - maximum limits have reasonable values
  - compiler-specific limits match expected values
*/
bool
d_tests_sa_env_pp_max_limits_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool definition_result;
    bool values_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing Compiler-Specific Maximum Limits\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    definition_result = d_tests_sa_env_pp_max_limits_definition(&subsection_counter);
    values_result     = d_tests_sa_env_pp_max_limits_values(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = (definition_result && values_result);

    if (overall_result)
    {
        printf("[PASS] Maximum Limits: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Maximum Limits: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* UTILITY MACROS TESTS
******************************************************************************/

/*
d_tests_sa_env_pp_utility_macros_definition
  Tests that preprocessor utility macros are defined.
  Tests the following:
  - D_ENV_PP_ARGS_WITHIN_LIMIT is defined
  - D_ENV_PP_ARGS_WITHIN_STANDARD is defined
  - D_ENV_PP_IS_UNLIMITED is defined
  - D_ENV_PP_EFFECTIVE_LIMIT is defined
*/
bool
d_tests_sa_env_pp_utility_macros_definition
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

    printf("%s--- Testing Utility Macros Definition ---\n", D_INDENT);

    // verify all utility macros are defined by testing their expansion
    #ifdef D_ENV_PP_ARGS_WITHIN_LIMIT
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_ARGS_WITHIN_LIMIT is defined",
                                 "utility macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_ARGS_WITHIN_LIMIT should be defined",
                                 "utility macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_ARGS_WITHIN_STANDARD
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_ARGS_WITHIN_STANDARD is defined",
                                 "utility macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_ARGS_WITHIN_STANDARD should be defined",
                                 "utility macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_IS_UNLIMITED
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_IS_UNLIMITED is defined",
                                 "utility macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_IS_UNLIMITED should be defined",
                                 "utility macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_PP_EFFECTIVE_LIMIT
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_EFFECTIVE_LIMIT is defined",
                                 "utility macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_EFFECTIVE_LIMIT should be defined",
                                 "utility macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Utility macros definition test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Utility macros definition test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_utility_macros_functionality
  Tests that preprocessor utility macros work correctly.
  Tests the following:
  - D_ENV_PP_ARGS_WITHIN_LIMIT correctly evaluates bounds
  - D_ENV_PP_ARGS_WITHIN_STANDARD correctly evaluates bounds
  - D_ENV_PP_IS_UNLIMITED correctly identifies unlimited (0) values
  - D_ENV_PP_EFFECTIVE_LIMIT returns correct effective limits
*/
bool
d_tests_sa_env_pp_utility_macros_functionality
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

    printf("%s--- Testing Utility Macros Functionality ---\n", D_INDENT);

    // test D_ENV_PP_ARGS_WITHIN_LIMIT
    if (!d_assert_standalone(D_ENV_PP_ARGS_WITHIN_LIMIT(1),
                             "D_ENV_PP_ARGS_WITHIN_LIMIT(1) == true",
                             "1 argument should always be within limit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (D_ENV_PP_MAX_MACRO_ARGS > 0)
    {
        if (!d_assert_standalone(D_ENV_PP_ARGS_WITHIN_LIMIT(D_ENV_PP_MAX_MACRO_ARGS),
                                 "count at max limit should be within limit",
                                 "exact maximum should be acceptable",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(!D_ENV_PP_ARGS_WITHIN_LIMIT(D_ENV_PP_MAX_MACRO_ARGS + 1),
                                 "count above max limit should be outside limit",
                                 "exceeding maximum should fail",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test D_ENV_PP_ARGS_WITHIN_STANDARD
    if (!d_assert_standalone(D_ENV_PP_ARGS_WITHIN_STANDARD(1),
                             "D_ENV_PP_ARGS_WITHIN_STANDARD(1) == true",
                             "1 argument should always be within standard",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_ARGS_WITHIN_STANDARD(D_ENV_PP_MIN_MACRO_ARGS),
                             "count at min standard should be within standard",
                             "exact minimum should be acceptable",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_PP_ARGS_WITHIN_STANDARD(D_ENV_PP_MIN_MACRO_ARGS + 1),
                             "count above min standard should be outside standard",
                             "exceeding minimum should fail",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_ENV_PP_IS_UNLIMITED
    if (!d_assert_standalone(D_ENV_PP_IS_UNLIMITED(0),
                             "D_ENV_PP_IS_UNLIMITED(0) == true",
                             "0 should be recognized as unlimited",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_PP_IS_UNLIMITED(1),
                             "D_ENV_PP_IS_UNLIMITED(1) == false",
                             "positive values should not be unlimited",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_PP_IS_UNLIMITED(127),
                             "D_ENV_PP_IS_UNLIMITED(127) == false",
                             "specific limits should not be unlimited",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_ENV_PP_EFFECTIVE_LIMIT
    if (!d_assert_standalone(D_ENV_PP_EFFECTIVE_LIMIT(0) == 2147483647L,
                             "D_ENV_PP_EFFECTIVE_LIMIT(0) == INT_MAX",
                             "unlimited should map to practical maximum",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_EFFECTIVE_LIMIT(127) == 127,
                             "D_ENV_PP_EFFECTIVE_LIMIT(127) == 127",
                             "specific limit should return itself",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_PP_EFFECTIVE_LIMIT(256) == 256,
                             "D_ENV_PP_EFFECTIVE_LIMIT(256) == 256",
                             "specific limit should return itself",
                             _test_info))
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

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Utility macros functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Utility macros functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_pp_utility_macros_all
  Runs all preprocessor utility macro tests.
  Tests the following:
  - utility macros are defined
  - utility macros function correctly
*/
bool
d_tests_sa_env_pp_utility_macros_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool definition_result;
    bool functionality_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing Preprocessor Utility Macros\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    definition_result    = d_tests_sa_env_pp_utility_macros_definition(&subsection_counter);
    functionality_result = d_tests_sa_env_pp_utility_macros_functionality(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = (definition_result && functionality_result);

    if (overall_result)
    {
        printf("[PASS] Utility Macros: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Utility Macros: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* VA_OPT DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_pp_va_opt_detection
  Tests __VA_OPT__ feature detection.
  Tests the following:
  - D_ENV_PP_HAS_VA_OPT is defined
  - D_ENV_PP_HAS_VA_OPT_ENABLED is defined
  - detection is consistent with language standard
*/
bool
d_tests_sa_env_pp_va_opt_detection
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

    printf("%s--- Testing __VA_OPT__ Detection ---\n", D_INDENT);

    // verify D_ENV_PP_HAS_VA_OPT is defined
    #ifdef D_ENV_PP_HAS_VA_OPT
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_HAS_VA_OPT is defined",
                                 "__VA_OPT__ detection macro should exist",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_PP_HAS_VA_OPT = %d\n",
               D_INDENT,
               D_ENV_PP_HAS_VA_OPT);

        // verify value is either 0 or 1
        if (!d_assert_standalone((D_ENV_PP_HAS_VA_OPT == 0) ||
                                 (D_ENV_PP_HAS_VA_OPT == 1),
                                 "D_ENV_PP_HAS_VA_OPT is 0 or 1",
                                 "__VA_OPT__ detection should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_HAS_VA_OPT should be defined",
                                 "__VA_OPT__ detection macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_PP_HAS_VA_OPT_ENABLED is defined
    #ifdef D_ENV_PP_HAS_VA_OPT_ENABLED
        if (!d_assert_standalone(true,
                                 "D_ENV_PP_HAS_VA_OPT_ENABLED is defined",
                                 "__VA_OPT__ enabled macro should exist",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it evaluates to same as D_ENV_PP_HAS_VA_OPT
        if (!d_assert_standalone(D_ENV_PP_HAS_VA_OPT_ENABLED == D_ENV_PP_HAS_VA_OPT,
                                 "D_ENV_PP_HAS_VA_OPT_ENABLED == D_ENV_PP_HAS_VA_OPT",
                                 "enabled macro should match detection macro",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_PP_HAS_VA_OPT_ENABLED should be defined",
                                 "__VA_OPT__ enabled macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // test consistency with language standard for C++
    #ifdef D_ENV_LANG_CPP_STANDARD
        #if ( defined(__cpp_va_opt) &&  \
              (__cpp_va_opt >= 201803L) )
            // C++20 or later should support __VA_OPT__
            if (!d_assert_standalone(D_ENV_PP_HAS_VA_OPT == 1,
                                     "C++20+ should detect __VA_OPT__ support",
                                     "__cpp_va_opt feature test macro indicates support",
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
        printf("%s[PASS] __VA_OPT__ detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] __VA_OPT__ detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_pp_limits_all
  Runs all preprocessor limits tests.
  Tests the following:
  - standard limit constants (C89, C99, C++)
  - standard-based minimum limits
  - compiler-specific maximum limits
  - utility macros
  - __VA_OPT__ detection
*/
bool
d_tests_sa_env_pp_limits_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool constants_result;
    bool min_limits_result;
    bool max_limits_result;
    bool utility_result;
    bool va_opt_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing Preprocessor Limits\n");
    printf("========================================="
           "=======================================\n");

    constants_result   = d_tests_sa_env_pp_limit_constants_all(&module_counter);
    min_limits_result  = d_tests_sa_env_pp_min_limits_all(&module_counter);
    max_limits_result  = d_tests_sa_env_pp_max_limits_all(&module_counter);
    utility_result     = d_tests_sa_env_pp_utility_macros_all(&module_counter);
    va_opt_result      = d_tests_sa_env_pp_va_opt_detection(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( constants_result   &&
                       min_limits_result  &&
                       max_limits_result  &&
                       utility_result     &&
                       va_opt_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Preprocessor Limits Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Preprocessor Limits Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Standard Constants:  %s\n",
               constants_result ? "PASSED" : "FAILED");
        printf("  - Minimum Limits:      %s\n",
               min_limits_result ? "PASSED" : "FAILED");
        printf("  - Maximum Limits:      %s\n",
               max_limits_result ? "PASSED" : "FAILED");
        printf("  - Utility Macros:      %s\n",
               utility_result ? "PASSED" : "FAILED");
        printf("  - __VA_OPT__ Detection:%s\n",
               va_opt_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
