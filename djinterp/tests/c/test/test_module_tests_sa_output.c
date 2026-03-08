/******************************************************************************
* djinterp [test]                                  test_module_tests_sa_output.c
*
*   Unit tests for test_module.h output and utility functions.
*
* path:      \test\test\test_module_tests_sa_output.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// OUTPUT TESTS
//=============================================================================

/*
d_tests_sa_test_module_print
  Tests printing a module tree to stdout.
  Tests the following:
  - d_test_module_print on a valid module does not crash
  - d_test_module_print with various prefix lengths does not crash
*/
bool
d_tests_sa_test_module_print
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_print ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        // print with standard indent
        d_test_module_print(mod, D_INDENT, d_strnlen(D_INDENT, 16));

        if (!d_assert_standalone(true,
                                 "print with standard indent did not crash",
                                 "print with standard indent should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        // print with empty prefix
        d_test_module_print(mod, "", 0);

        if (!d_assert_standalone(true,
                                 "print with empty prefix did not crash",
                                 "print with empty prefix should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        // print with deeper indent
        d_test_module_print(mod, "    ", 4);

        if (!d_assert_standalone(true,
                                 "print with deep indent did not crash",
                                 "print with deep indent should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_print unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_print unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_print_null
  Tests printing with NULL arguments.
  Tests the following:
  - d_test_module_print(NULL, ...) does not crash
  - d_test_module_print(mod, NULL, ...) does not crash
*/
bool
d_tests_sa_test_module_print_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_print (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // NULL module
    d_test_module_print(NULL, D_INDENT, 2);

    if (!d_assert_standalone(true,
                             "print on NULL module did not crash",
                             "print on NULL module should not crash",
                             _test_info))
    {
        all_passed = false;
    }

    // NULL prefix
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        d_test_module_print(mod, NULL, 0);

        if (!d_assert_standalone(true,
                                 "print with NULL prefix did not crash",
                                 "print with NULL prefix should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_print (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_print (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_print_result
  Tests printing module results after a run.
  Tests the following:
  - d_test_module_print_result on a run module does not crash
  - d_test_module_print_result on an unrun module does not crash
  - d_test_module_print_result on NULL does not crash
*/
bool
d_tests_sa_test_module_print_result
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_print_result ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        // print result before running (should handle gracefully)
        d_test_module_print_result(mod);

        if (!d_assert_standalone(true,
                                 "print_result on unrun module did not crash",
                                 "print_result on unrun module "
                                 "should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        // run and then print
        d_test_module_run(mod, NULL);
        d_test_module_print_result(mod);

        if (!d_assert_standalone(true,
                                 "print_result on run module did not crash",
                                 "print_result on run module should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    // NULL module
    d_test_module_print_result(NULL);

    if (!d_assert_standalone(true,
                             "print_result on NULL did not crash",
                             "print_result on NULL should not crash",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_print_result unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_print_result unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_status_to_string
  Tests converting each DTestModuleStatus enum value to its string form.
  Tests the following:
  - D_TEST_MODULE_STATUS_PENDING  converts to "PENDING"
  - D_TEST_MODULE_STATUS_PASSED   converts to "PASSED"
  - D_TEST_MODULE_STATUS_FAILED   converts to "FAILED"
*/
bool
d_tests_sa_test_module_status_to_string
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    const char* str;

    printf("  --- Testing d_test_module_status_to_string ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // PENDING
    str = d_test_module_status_to_string(D_TEST_MODULE_STATUS_PENDING);

    if (!d_assert_standalone(str != NULL,
                             "PENDING converts to non-NULL string",
                             "PENDING should convert to non-NULL string",
                             _test_info))
    {
        all_passed = false;
    }

    if (str)
    {
        size_t str_len      = d_strnlen(str, 64);
        size_t expected_len = d_strnlen("PENDING", 64);

        if (!d_assert_standalone(
                d_strequals(str, str_len, "PENDING", expected_len),
                "PENDING status string is 'PENDING'",
                "PENDING status string should be 'PENDING'",
                _test_info))
        {
            all_passed = false;
        }
    }

    // PASSED
    str = d_test_module_status_to_string(D_TEST_MODULE_STATUS_PASSED);

    if (!d_assert_standalone(str != NULL,
                             "PASSED converts to non-NULL string",
                             "PASSED should convert to non-NULL string",
                             _test_info))
    {
        all_passed = false;
    }

    if (str)
    {
        size_t str_len      = d_strnlen(str, 64);
        size_t expected_len = d_strnlen("PASSED", 64);

        if (!d_assert_standalone(
                d_strequals(str, str_len, "PASSED", expected_len),
                "PASSED status string is 'PASSED'",
                "PASSED status string should be 'PASSED'",
                _test_info))
        {
            all_passed = false;
        }
    }

    // FAILED
    str = d_test_module_status_to_string(D_TEST_MODULE_STATUS_FAILED);

    if (!d_assert_standalone(str != NULL,
                             "FAILED converts to non-NULL string",
                             "FAILED should convert to non-NULL string",
                             _test_info))
    {
        all_passed = false;
    }

    if (str)
    {
        size_t str_len      = d_strnlen(str, 64);
        size_t expected_len = d_strnlen("FAILED", 64);

        if (!d_assert_standalone(
                d_strequals(str, str_len, "FAILED", expected_len),
                "FAILED status string is 'FAILED'",
                "FAILED status string should be 'FAILED'",
                _test_info))
        {
            all_passed = false;
        }
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_status_to_string unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_status_to_string unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_status_to_string_all
  Tests all DTestModuleStatus enum values exhaustively.
  Tests the following:
  - UNKNOWN, PENDING, RUNNING, PASSED, FAILED, SKIPPED, and ERROR all
    convert to non-NULL, non-empty strings
  - each string is distinct from the others
*/
bool
d_tests_sa_test_module_status_to_string_all
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    enum DTestModuleStatus statuses[7];
    const char*            strings[7];
    size_t                 i;
    size_t                 j;

    printf("  --- Testing status_to_string (all values) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    statuses[0] = D_TEST_MODULE_STATUS_UNKNOWN;
    statuses[1] = D_TEST_MODULE_STATUS_PENDING;
    statuses[2] = D_TEST_MODULE_STATUS_RUNNING;
    statuses[3] = D_TEST_MODULE_STATUS_PASSED;
    statuses[4] = D_TEST_MODULE_STATUS_FAILED;
    statuses[5] = D_TEST_MODULE_STATUS_SKIPPED;
    statuses[6] = D_TEST_MODULE_STATUS_ERROR;

    // convert all statuses
    for (i = 0; i < 7; i++)
    {
        strings[i] = d_test_module_status_to_string(statuses[i]);

        if (!d_assert_standalone(strings[i] != NULL,
                                 "status converts to non-NULL string",
                                 "every status should convert to "
                                 "non-NULL string",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    // verify each known status string is non-empty
    for (i = 0; i < 7; i++)
    {
        if (strings[i])
        {
            size_t len = d_strnlen(strings[i], 64);

            if (!d_assert_standalone(len > 0,
                                     "status string is non-empty",
                                     "every status string should be non-empty",
                                     _test_info))
            {
                all_passed = false;
            }
        }
    }

    // verify all distinct (no two strings are equal)
    for (i = 0; i < 7; i++)
    {
        for (j = i + 1; j < 7; j++)
        {
            if ( (strings[i]) && (strings[j]) )
            {
                size_t len_i = d_strnlen(strings[i], 64);
                size_t len_j = d_strnlen(strings[j], 64);

                if (!d_assert_standalone(
                        !d_strequals(strings[i], len_i,
                                     strings[j], len_j),
                        "status strings are distinct",
                        "each status string should be distinct",
                        _test_info))
                {
                    all_passed = false;
                }
            }
        }
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s status_to_string (all values) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s status_to_string (all values) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
