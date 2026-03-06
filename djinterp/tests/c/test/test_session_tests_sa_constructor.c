/******************************************************************************
* djinterp [test]                          test_session_tests_sa_constructor.c
*
*   Unit tests for test_session.h constructor and destructor functions.
*
* path:      \test\test\test_session_tests_sa_constructor.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"


//=============================================================================
// CONSTRUCTOR / DESTRUCTOR TESTS
//=============================================================================

/*
d_tests_sa_test_session_new
  Tests basic construction of a d_test_session with no arguments.
  Tests the following:
  - d_test_session_new returns non-NULL
  - children vector is initialized (non-NULL)
  - config is non-NULL (default allocated with D_TEST_MODE_NORMAL)
  - status is D_TEST_SESSION_STATUS_CREATED
  - output stream defaults to stdout
  - output format defaults to D_TEST_OUTPUT_CONSOLE
  - output verbosity defaults to D_TEST_VERBOSITY_NORMAL
  - output use_color defaults to true
  - child count is 0
  - failure_count is 0
*/
bool
d_tests_sa_test_session_new
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing d_test_session_new ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (!d_assert_standalone(session != NULL,
                             "d_test_session_new returned non-NULL",
                             "d_test_session_new should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (session)
    {
        if (!d_assert_standalone(session->children != NULL,
                                 "children vector is non-NULL",
                                 "children vector should be initialized",
                                 _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(session->config != NULL,
                                 "config is non-NULL (default allocated)",
                                 "config should be non-NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->status == D_TEST_SESSION_STATUS_CREATED,
                "status is CREATED",
                "status should be CREATED",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->output.stream == stdout,
                "output stream is stdout",
                "output stream should default to stdout",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->output.format == D_TEST_OUTPUT_CONSOLE,
                "output format is CONSOLE",
                "output format should default to CONSOLE",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->output.verbosity == D_TEST_VERBOSITY_NORMAL,
                "verbosity is NORMAL",
                "verbosity should default to NORMAL",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(session->output.use_color == true,
                                 "use_color defaults to true",
                                 "use_color should default to true",
                                 _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                d_test_session_child_count(session) == 0,
                "child count is 0",
                "child count should be 0",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(session->failure_count == 0,
                                 "failure_count is 0",
                                 "failure_count should be 0",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_session_new unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_session_new unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_new_with_config
  Tests construction with a provided config.
  Tests the following:
  - new_with_config with a valid config returns non-NULL
  - the session's config pointer matches the provided config
  - new_with_config with NULL config returns non-NULL (uses default)
*/
bool
d_tests_sa_test_session_new_with_config
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_options* options;

    printf("  --- Testing d_test_session_new_with_config ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a config and pass it
    options = d_test_options_new(D_TEST_MODE_VERBOSE);

    if (options)
    {
        session = d_test_session_new_with_config(options);

        if (!d_assert_standalone(session != NULL,
                                 "new_with_config returned non-NULL",
                                 "new_with_config should return non-NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        if (session)
        {
            // session takes ownership of config
            if (!d_assert_standalone(session->config == options,
                                     "session config is the provided config",
                                     "session config should be the "
                                     "provided config",
                                     _test_info))
            {
                all_passed = false;
            }

            if (!d_assert_standalone(
                    session->status == D_TEST_SESSION_STATUS_CREATED,
                    "status is CREATED with config",
                    "status should be CREATED with config",
                    _test_info))
            {
                all_passed = false;
            }

            // session free also frees the config
            d_test_session_free(session);
        }
        else
        {
            // config not consumed, free it
            d_test_options_free(options);
        }
    }

    // with NULL config, should still create a session with defaults
    session = d_test_session_new_with_config(NULL);

    if (!d_assert_standalone(session != NULL,
                             "new_with_config(NULL) returned non-NULL",
                             "new_with_config(NULL) should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (session)
    {
        if (!d_assert_standalone(session->config != NULL,
                                 "NULL config session has default config",
                                 "NULL config session should have "
                                 "default config",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_session_new_with_config unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_session_new_with_config unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_new_with_modules
  Tests construction with pre-supplied module children.
  Tests the following:
  - new_with_modules with a valid module array returns non-NULL
  - child count reflects the modules supplied
  - new_with_modules with NULL/0 returns a session with 0 children
*/
bool
d_tests_sa_test_session_new_with_modules
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_module*  mod;
    struct d_test_type*    type_mod;
    struct d_test_type*    modules[1];

    printf("  --- Testing d_test_session_new_with_modules ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a module wrapped in d_test_type with MODULE type
    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    type_mod = d_test_type_new(D_TEST_TYPE_MODULE, mod);

    if (!type_mod)
    {
        d_test_module_free(mod);
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    modules[0] = type_mod;
    session = d_test_session_new_with_modules(modules, 1);

    if (!d_assert_standalone(session != NULL,
                             "new_with_modules returned non-NULL",
                             "new_with_modules should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (session)
    {
        if (!d_assert_standalone(
                d_test_session_child_count(session) == 1,
                "child count is 1",
                "child count should be 1",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }
    else
    {
        d_test_type_free(type_mod);
    }

    // with NULL/0, should create empty session
    session = d_test_session_new_with_modules(NULL, 0);

    if (!d_assert_standalone(session != NULL,
                             "new_with_modules(NULL,0) returned non-NULL",
                             "new_with_modules(NULL,0) should "
                             "return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (session)
    {
        if (!d_assert_standalone(
                d_test_session_child_count(session) == 0,
                "empty modules child count is 0",
                "empty modules child count should be 0",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_session_new_with_modules unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_session_new_with_modules unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_new_with_modules_and_config
  Tests construction with both config and modules.
  Tests the following:
  - returns non-NULL with valid config and modules
  - session config is the provided config
  - child count reflects the modules
*/
bool
d_tests_sa_test_session_new_with_modules_and_config
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_options*  cfg;
    struct d_test_module*  mod;
    struct d_test_type*    type_mod;
    struct d_test_type*    modules[1];

    printf("  --- Testing d_test_session_new_with_modules_and_config ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    cfg = d_test_options_new(D_TEST_MODE_SILENT);
    mod = d_test_module_new(NULL, 0);

    if ( (!cfg) || (!mod) )
    {
        printf("  %s allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    type_mod = d_test_type_new(D_TEST_TYPE_MODULE, mod);

    if (!type_mod)
    {
        d_test_module_free(mod);
        d_test_options_free(cfg);
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    modules[0] = type_mod;
    session = d_test_session_new_with_modules_and_config(cfg,
                                                          modules, 1);

    if (!d_assert_standalone(session != NULL,
                             "new_with_modules_and_config returned non-NULL",
                             "new_with_modules_and_config should "
                             "return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (session)
    {
        if (!d_assert_standalone(session->config == cfg,
                                 "config is the provided config",
                                 "config should be the provided config",
                                 _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                d_test_session_child_count(session) == 1,
                "child count is 1",
                "child count should be 1",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }
    else
    {
        d_test_type_free(type_mod);
        d_test_options_free(cfg);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s new_with_modules_and_config unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s new_with_modules_and_config unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_validate_args
  Tests the validate_args function.
  Tests the following:
  - validate_args with valid args returns non-NULL config
  - validate_args with NULL/0 returns NULL
*/
bool
d_tests_sa_test_session_validate_args
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_arg     args[1];
    struct d_test_options* cfg;
    struct d_test_options* null_cfg;

    printf("  --- Testing d_test_session_validate_args ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    args[0].key   = "name";
    args[0].value = (void*)"ValidateSession";

    cfg = d_test_session_validate_args(args, 1);

    if (!d_assert_standalone(cfg != NULL,
                             "validate_args with valid args returns config",
                             "validate_args should return config",
                             _test_info))
    {
        all_passed = false;
    }

    null_cfg = d_test_session_validate_args(NULL, 0);

    if (!d_assert_standalone(null_cfg == NULL,
                             "validate_args with NULL returns NULL",
                             "validate_args should return NULL for NULL args",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_session_validate_args unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_session_validate_args unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_free
  Tests freeing a valid session.
  Tests the following:
  - freeing a populated session does not crash
*/
bool
d_tests_sa_test_session_free
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing d_test_session_free ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        d_test_session_free(session);

        if (!d_assert_standalone(true,
                                 "free on valid session did not crash",
                                 "free on valid session should not crash",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_session_free unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_session_free unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_free_null
  Tests freeing NULL session.
  Tests the following:
  - d_test_session_free(NULL) is a safe no-op
*/
bool
d_tests_sa_test_session_free_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;

    printf("  --- Testing d_test_session_free (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    d_test_session_free(NULL);

    if (!d_assert_standalone(true,
                             "free on NULL did not crash",
                             "free on NULL should not crash",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_session_free (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_session_free (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
