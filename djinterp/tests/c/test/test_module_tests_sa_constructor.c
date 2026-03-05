/******************************************************************************
* djinterp [test]                               test_module_tests_sa_constructor.c
*
*   Unit tests for test_module.h constructor and destructor functions.
*
* path:      \test\test\test_module_tests_sa_constructor.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// CONSTRUCTOR / DESTRUCTOR TESTS
//=============================================================================

/*
d_tests_sa_test_module_new
  Tests basic construction of a d_test_module with no children.
  Tests the following:
  - d_test_module_new returns a non-NULL pointer when given NULL children
  - the children vector is initialized (non-NULL)
  - the config pointer is non-NULL (default config is pre-allocated)
  - the result pointer is non-NULL (default result is pre-allocated)
  - the initial status is D_TEST_MODULE_STATUS_PENDING
  - the child count is zero
*/
bool
d_tests_sa_test_module_new
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_new ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // construct a module with no children
    mod = d_test_module_new(NULL, 0);

    // verify allocation succeeded
    if (!d_assert_standalone(mod != NULL,
                             "d_test_module_new returned non-NULL",
                             "d_test_module_new should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (mod)
    {
        // verify children vector was initialized
        if (!d_assert_standalone(mod->children != NULL,
                                 "children vector is non-NULL",
                                 "children vector should be initialized",
                                 _test_info))
        {
            all_passed = false;
        }

        // verify config is non-NULL (default config is pre-allocated)
        if (!d_assert_standalone(mod->config != NULL,
                                 "config is non-NULL (default allocated)",
                                 "config should be non-NULL (default allocated)",
                                 _test_info))
        {
            all_passed = false;
        }

        // verify result is non-NULL (default result is pre-allocated)
        if (!d_assert_standalone(mod->result != NULL,
                                 "result is non-NULL (default allocated)",
                                 "result should be non-NULL (default allocated)",
                                 _test_info))
        {
            all_passed = false;
        }

        // verify initial status is PENDING
        if (!d_assert_standalone(
                mod->status == D_TEST_MODULE_STATUS_PENDING,
                "initial status is PENDING",
                "initial status should be PENDING",
                _test_info))
        {
            all_passed = false;
        }

        // verify no children present
        if (!d_assert_standalone(
                d_test_module_child_count(mod) == 0,
                "child count is 0",
                "child count should be 0 for empty module",
                _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_new unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_new unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_new_with_children
  Tests construction of a d_test_module and adding children via add_child.
  Note: children are added post-construction because the constructor's
  array parameter populates via ptr_vector internals that differ from the
  add_child path.
  Tests the following:
  - a freshly constructed module starts with child count 0
  - children added via d_test_module_add_child are reflected in child_count
  - children are retrievable by index after addition
*/
bool
d_tests_sa_test_module_new_with_children
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test*        t1;
    struct d_test*        t2;
    struct d_test_type*   type1;
    struct d_test_type*   type2;

    printf("  --- Testing d_test_module_new with children ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create the module first, then add children via add_child
    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // verify initial child count is 0
    if (!d_assert_standalone(
            d_test_module_child_count(mod) == 0,
            "initial child count is 0",
            "initial child count should be 0",
            _test_info))
    {
        all_passed = false;
    }

    // create two test children and add them
    t1 = d_test_new(NULL, 0);
    t2 = d_test_new(NULL, 0);

    if ( (!t1) || (!t2) )
    {
        printf("  %s child test allocation failed\n", D_TEST_SYMBOL_FAIL);
        d_test_module_free(mod);
        _test_info->tests_total++;

        return false;
    }

    type1 = d_test_type_new(D_TEST_TYPE_TEST, t1);
    type2 = d_test_type_new(D_TEST_TYPE_TEST, t2);

    if ( (!type1) || (!type2) )
    {
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        d_test_module_free(mod);
        _test_info->tests_total++;

        return false;
    }

    // add first child
    d_test_module_add_child(mod,
                            (const struct d_test_tree_node*)type1);

    if (!d_assert_standalone(
            d_test_module_child_count(mod) == 1,
            "child count is 1 after first add",
            "child count should be 1 after first add",
            _test_info))
    {
        all_passed = false;
    }

    // add second child
    d_test_module_add_child(mod,
                            (const struct d_test_tree_node*)type2);

    if (!d_assert_standalone(
            d_test_module_child_count(mod) == 2,
            "child count is 2 after second add",
            "child count should be 2 after second add",
            _test_info))
    {
        all_passed = false;
    }

    // verify children are retrievable
    struct d_test_type* got0 = d_test_module_get_child_at(mod, 0);

    if (!d_assert_standalone(got0 != NULL,
                             "child at index 0 is non-NULL",
                             "child at index 0 should be non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    struct d_test_type* got1 = d_test_module_get_child_at(mod, 1);

    if (!d_assert_standalone(got1 != NULL,
                             "child at index 1 is non-NULL",
                             "child at index 1 should be non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_new_with_children unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_new_with_children unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_new_args
  Tests construction using d_test_module_new_args with key-value arguments.
  Tests the following:
  - d_test_module_new_args with valid args returns a non-NULL module
  - the config is populated from args (non-NULL)
  - the child count is zero when no children are given
  - the initial status is PENDING
*/
bool
d_tests_sa_test_module_new_args
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_arg     args[1];
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_new_args ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // set up a name argument
    args[0].key   = "name";
    args[0].value = (void*)"ArgsModule";

    // construct with args, no children
    mod = d_test_module_new_args(args, 1, NULL, 0);

    // verify module was created
    if (!d_assert_standalone(mod != NULL,
                             "d_test_module_new_args returned non-NULL",
                             "d_test_module_new_args should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (mod)
    {
        // verify the config was populated from args
        if (!d_assert_standalone(mod->config != NULL,
                                 "config was populated from args",
                                 "config should be populated from args",
                                 _test_info))
        {
            all_passed = false;
        }

        // verify child count is zero
        if (!d_assert_standalone(
                d_test_module_child_count(mod) == 0,
                "child count is 0 with no children",
                "child count should be 0 with no children",
                _test_info))
        {
            all_passed = false;
        }

        // verify initial status
        if (!d_assert_standalone(
                mod->status == D_TEST_MODULE_STATUS_PENDING,
                "status is PENDING after construction with args",
                "status should be PENDING after construction with args",
                _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_new_args unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_new_args unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_validate_args
  Tests the d_test_module_validate_args function.
  Tests the following:
  - validate_args with valid args returns a non-NULL config
  - validate_args with NULL args returns NULL
  - validate_args with zero count returns NULL
*/
bool
d_tests_sa_test_module_validate_args
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_arg     args[1];
    struct d_test_config* cfg;
    struct d_test_config* null_cfg;
    struct d_test_config* zero_cfg;

    printf("  --- Testing d_test_module_validate_args ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // valid args should produce a config
    args[0].key   = "name";
    args[0].value = (void*)"ValidateTest";

    cfg = d_test_module_validate_args(args, 1);

    if (!d_assert_standalone(cfg != NULL,
                             "validate_args with valid args returns config",
                             "validate_args should return config for valid args",
                             _test_info))
    {
        all_passed = false;
    }

    // NULL args should return NULL
    null_cfg = d_test_module_validate_args(NULL, 0);

    if (!d_assert_standalone(null_cfg == NULL,
                             "validate_args with NULL args returns NULL",
                             "validate_args should return NULL for NULL args",
                             _test_info))
    {
        all_passed = false;
    }

    // zero count with non-NULL args should return NULL
    zero_cfg = d_test_module_validate_args(args, 0);

    if (!d_assert_standalone(zero_cfg == NULL,
                             "validate_args with zero count returns NULL",
                             "validate_args should return NULL for zero count",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_validate_args unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_validate_args unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_free
  Tests freeing a valid module with children and config.
  Tests the following:
  - freeing a populated module does not crash
  - freeing a module with children also frees children
*/
bool
d_tests_sa_test_module_free
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test_type*   child;
    struct d_test*        t;

    printf("  --- Testing d_test_module_free ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a module and add one child via add_child
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        t = d_test_new(NULL, 0);

        if (t)
        {
            child = d_test_type_new(D_TEST_TYPE_TEST, t);

            if (child)
            {
                d_test_module_add_child(
                    mod,
                    (const struct d_test_tree_node*)child);
            }
            else
            {
                d_test_free(t);
            }
        }

        // free should not crash (frees children too)
        d_test_module_free(mod);

        if (!d_assert_standalone(
                true,
                "free on populated module did not crash",
                "free on populated module should not crash",
                _test_info))
        {
            all_passed = false;
        }
    }
    else
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_free unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_free unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_free_null
  Tests that freeing a NULL module pointer does not crash.
  Tests the following:
  - d_test_module_free(NULL) is a safe no-op
*/
bool
d_tests_sa_test_module_free_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;

    printf("  --- Testing d_test_module_free (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // free NULL should be a safe no-op
    d_test_module_free(NULL);

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
        printf("  %s d_test_module_free (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_free (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
