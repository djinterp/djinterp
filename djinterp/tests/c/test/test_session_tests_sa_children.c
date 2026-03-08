/******************************************************************************
* djinterp [test]                            test_session_tests_sa_children.c
*
*   Unit tests for test_session.h child management functions.
*
* path:      \test\test\test_session_tests_sa_children.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/
#include "./test_session_tests_sa.h"


//=============================================================================
// HELPER: create a MODULE-typed d_test_type wrapping a fresh d_test_module
//=============================================================================

static struct d_test_type*
create_module_child(void)
{
    struct d_test_module* mod;
    struct d_test_type*   type_mod;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        return NULL;
    }

    type_mod = d_test_type_new(D_TEST_TYPE_MODULE, mod);

    if (!type_mod)
    {
        d_test_module_free(mod);

        return NULL;
    }

    return type_mod;
}


//=============================================================================
// CHILD MANAGEMENT TESTS
//=============================================================================

/*
d_tests_sa_test_session_add_child
  Tests adding a module child to a session.
  Tests the following:
  - add_child with a MODULE-typed child returns true
  - child count increments
  - child is retrievable at index 0
*/
bool
d_tests_sa_test_session_add_child
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_type*    child;

    printf("  --- Testing d_test_session_add_child ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();
    child   = create_module_child();

    if ( (!session) || 
         (!child) )
    {
        printf("  %s allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    if (!d_assert_standalone(
            d_test_session_add_child(session, child),
            "add_child returned true",
            "add_child should return true for module",
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

    if (!d_assert_standalone(
            d_test_session_get_child_at(session, 0) != NULL,
            "child at 0 is non-NULL",
            "child at 0 should be non-NULL",
            _test_info))
    {
        all_passed = false;
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s add_child unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s add_child unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_add_child_non_module
  Tests that add_child rejects non-MODULE typed children.
  Tests the following:
  - add_child with a TEST-typed child returns false
  - child count remains 0
*/
bool
d_tests_sa_test_session_add_child_non_module
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test*         t;
    struct d_test_type*    child;

    printf("  --- Testing add_child (non-module) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();
    t       = d_test_new(NULL, 0);

    if ( (!session) || (!t) )
    {
        printf("  %s allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    child = d_test_type_new(D_TEST_TYPE_TEST, t);

    if (!child)
    {
        d_test_free(t);
        d_test_session_free(session);
        _test_info->tests_total++;

        return false;
    }

    // session only accepts MODULE type
    if (!d_assert_standalone(
            !d_test_session_add_child(session, child),
            "add_child rejects non-MODULE child",
            "add_child should reject non-MODULE child",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            d_test_session_child_count(session) == 0,
            "child count still 0 after rejection",
            "child count should still be 0",
            _test_info))
    {
        all_passed = false;
    }

    d_test_type_free(child);
    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s add_child (non-module) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s add_child (non-module) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_add_child_null
  Tests add_child with NULL arguments.
  Tests the following:
  - add_child(NULL, child) returns false
  - add_child(session, NULL) returns false
*/
bool
d_tests_sa_test_session_add_child_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing add_child (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    if (!d_assert_standalone(
            !d_test_session_add_child(NULL, NULL),
            "add_child(NULL,NULL) returns false",
            "add_child(NULL,NULL) should return false",
            _test_info))
    {
        all_passed = false;
    }

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                !d_test_session_add_child(session, NULL),
                "add_child(session,NULL) returns false",
                "add_child(session,NULL) should return false",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s add_child (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s add_child (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_add_children
  Tests adding multiple children at once.
  Tests the following:
  - add_children with valid array returns true
  - child count reflects all added children
*/
bool
d_tests_sa_test_session_add_children
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_type*    children[2];

    printf("  --- Testing add_children ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session     = d_test_session_new();
    children[0] = create_module_child();
    children[1] = create_module_child();

    if ( (!session) || (!children[0]) || (!children[1]) )
    {
        printf("  %s allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    if (!d_assert_standalone(
            d_test_session_add_children(session, children, 2),
            "add_children returned true",
            "add_children should return true",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            d_test_session_child_count(session) == 2,
            "child count is 2",
            "child count should be 2",
            _test_info))
    {
        all_passed = false;
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s add_children unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s add_children unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_child_count
  Tests child_count on various states.
  Tests the following:
  - empty session has 0 children
  - NULL session returns 0
*/
bool
d_tests_sa_test_session_child_count
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing child_count ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                d_test_session_child_count(session) == 0,
                "empty session has 0 children",
                "empty session should have 0 children",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            d_test_session_child_count(NULL) == 0,
            "NULL session has 0 children",
            "NULL session should have 0 children",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s child_count unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s child_count unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_get_child_at
  Tests retrieving a child by valid index.
  Tests the following:
  - get_child_at(0) returns the added child
  - the child has MODULE type
*/
bool
d_tests_sa_test_session_get_child_at
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_type*    child;
    struct d_test_type*    got;

    printf("  --- Testing get_child_at ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();
    child   = create_module_child();

    if ( (!session) || (!child) )
    {
        _test_info->tests_total++;

        return false;
    }

    d_test_session_add_child(session, child);
    got = d_test_session_get_child_at(session, 0);

    if (!d_assert_standalone(got != NULL,
                             "get_child_at(0) is non-NULL",
                             "get_child_at(0) should be non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (got)
    {
        if (!d_assert_standalone(got->type == D_TEST_TYPE_MODULE,
                                 "child type is MODULE",
                                 "child type should be MODULE",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_child_at unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_child_at unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_get_child_at_invalid
  Tests get_child_at with invalid arguments.
  Tests the following:
  - get_child_at on empty session returns NULL
  - get_child_at on NULL session returns NULL
  - get_child_at with out-of-bounds index returns NULL
*/
bool
d_tests_sa_test_session_get_child_at_invalid
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing get_child_at (invalid) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                d_test_session_get_child_at(session, 0) == NULL,
                "empty session index 0 returns NULL",
                "empty session index 0 should return NULL",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                d_test_session_get_child_at(session, 9999) == NULL,
                "out-of-bounds returns NULL",
                "out-of-bounds should return NULL",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            d_test_session_get_child_at(NULL, 0) == NULL,
            "NULL session returns NULL",
            "NULL session should return NULL",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_child_at (invalid) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_child_at (invalid) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_clear_children
  Tests clearing all children from a session.
  Tests the following:
  - clear_children returns true
  - child count is 0 after clear
  - clear_children on NULL returns false
*/
bool
d_tests_sa_test_session_clear_children
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    struct d_test_type*    child;

    printf("  --- Testing clear_children ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();
    child   = create_module_child();

    if ( (!session) || (!child) )
    {
        _test_info->tests_total++;

        return false;
    }

    d_test_session_add_child(session, child);

    if (!d_assert_standalone(
            d_test_session_clear_children(session),
            "clear_children returned true",
            "clear_children should return true",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            d_test_session_child_count(session) == 0,
            "child count is 0 after clear",
            "child count should be 0 after clear",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            !d_test_session_clear_children(NULL),
            "clear_children(NULL) returns false",
            "clear_children(NULL) should return false",
            _test_info))
    {
        all_passed = false;
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s clear_children unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s clear_children unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
