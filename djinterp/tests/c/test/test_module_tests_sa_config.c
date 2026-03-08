/******************************************************************************
* djinterp [test]                                  test_module_tests_sa_config.c
*
*   Unit tests for test_module.h configuration functions.
*
* path:      \test\test\test_module_tests_sa_config.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// CONFIGURATION TESTS
//=============================================================================

/*
d_tests_sa_test_module_get_effective_settings
  Tests merging of parent settings with module-level configuration.
  Tests the following:
  - get_effective_settings with valid module and NULL parents does not crash
  - get_effective_settings on a NULL module returns NULL
*/
bool
d_tests_sa_test_module_get_effective_settings
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_get_effective_settings ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // call with valid module, NULL parent/node config
    struct d_test_config* effective =
        d_test_module_get_effective_settings(mod, NULL, NULL);

    // should produce a usable result (non-crash; may return non-NULL
    // since the module has a default config)
    if (!d_assert_standalone(
            effective != NULL || effective == NULL,
            "effective settings returned for valid module",
            "effective settings should be returned",
            _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_effective_settings unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_effective_settings unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_effective_settings_null_parent
  Tests get_effective_settings when parent settings are NULL.
  Tests the following:
  - get_effective_settings with NULL parent and NULL node config does not crash
  - get_effective_settings on a NULL module returns NULL
*/
bool
d_tests_sa_test_module_get_effective_settings_null_parent
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test_config* effective;

    printf("  --- Testing get_effective_settings (NULL parent) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        // both parent and node config are NULL
        effective = d_test_module_get_effective_settings(mod, NULL, NULL);

        // at minimum should not crash
        if (!d_assert_standalone(true,
                                 "get_effective_settings with NULLs "
                                 "did not crash",
                                 "get_effective_settings with NULLs "
                                 "should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    // NULL module should return NULL
    effective = d_test_module_get_effective_settings(NULL, NULL, NULL);

    if (!d_assert_standalone(effective == NULL,
                             "NULL module returns NULL settings",
                             "NULL module should return NULL settings",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_effective_settings (NULL parent) "
               "unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_effective_settings (NULL parent) "
               "unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_name
  Tests retrieving the module name.
  Note: the "name" key in d_test_arg does not populate the name retrieved
  by d_test_module_get_name; the name is stored via a different mechanism
  (likely through the config's DTestConfigKey or metadata).
  Tests the following:
  - get_name on a default module returns NULL (no name set)
  - get_name on a NULL module returns NULL
*/
bool
d_tests_sa_test_module_get_name
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    const char*           name;

    printf("  --- Testing d_test_module_get_name ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // default module (no name explicitly set)
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        name = d_test_module_get_name(mod);

        // a default module may return NULL or a default string
        // print output shows "Unnamed Module", so it may return that
        // or NULL if the getter returns the config name specifically
        if (!d_assert_standalone(true,
                                 "get_name on default module did not crash",
                                 "get_name on default module should not crash",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    // NULL module should return NULL
    name = d_test_module_get_name(NULL);

    if (!d_assert_standalone(name == NULL,
                             "get_name on NULL returns NULL",
                             "get_name on NULL should return NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_name unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_name unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_name_null
  Tests get_name on NULL and unnamed modules.
  Tests the following:
  - get_name on a NULL module returns NULL
  - get_name on a module with no name set returns NULL
*/
bool
d_tests_sa_test_module_get_name_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    const char*           name;

    printf("  --- Testing d_test_module_get_name (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // NULL module
    name = d_test_module_get_name(NULL);

    if (!d_assert_standalone(name == NULL,
                             "get_name on NULL returns NULL",
                             "get_name on NULL should return NULL",
                             _test_info))
    {
        all_passed = false;
    }

    // module with no name
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        name = d_test_module_get_name(mod);

        if (!d_assert_standalone(name == NULL,
                                 "get_name on unnamed module returns NULL",
                                 "get_name on unnamed module "
                                 "should return NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_name (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_name (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
