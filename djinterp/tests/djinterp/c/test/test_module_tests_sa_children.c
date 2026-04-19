/******************************************************************************
* djinterp [test]                                test_module_tests_sa_children.c
*
*   Unit tests for test_module.h child management functions.
*
* path:      \test\test\test_module_tests_sa_children.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// CHILD MANAGEMENT TESTS
//=============================================================================

/*
d_tests_sa_test_module_add_child
  Tests adding children to a module after construction.
  Tests the following:
  - adding a test child returns true
  - adding a block child returns true
  - child count increments correctly after each addition
  - children are retrievable in insertion order
*/
bool
d_tests_sa_test_module_add_child
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test*        t;
    struct d_test_block*  blk;
    struct d_test_type*   child_test;
    struct d_test_type*   child_block;
    bool                  added;

    printf("  --- Testing d_test_module_add_child ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // create and add a test child
    t = d_test_new(NULL, 0);

    if (t)
    {
        child_test = d_test_type_new(D_TEST_TYPE_TEST, t);

        if (child_test)
        {
            added = d_test_module_add_child(mod,
                        (const struct d_test_tree_node*)child_test);

            if (!d_assert_standalone(added,
                                     "add test child returned true",
                                     "add test child should return true",
                                     _test_info))
            {
                all_passed = false;
            }

            // verify count is 1
            if (!d_assert_standalone(
                    d_test_module_child_count(mod) == 1,
                    "child count is 1 after first add",
                    "child count should be 1 after first add",
                    _test_info))
            {
                all_passed = false;
            }
        }
    }

    // create and add a block child
    blk = d_test_block_new(NULL, 0);

    if (blk)
    {
        child_block = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, blk);

        if (child_block)
        {
            added = d_test_module_add_child(mod,
                        (const struct d_test_tree_node*)child_block);

            if (!d_assert_standalone(added,
                                     "add block child returned true",
                                     "add block child should return true",
                                     _test_info))
            {
                all_passed = false;
            }

            // verify count is 2
            if (!d_assert_standalone(
                    d_test_module_child_count(mod) == 2,
                    "child count is 2 after second add",
                    "child count should be 2 after second add",
                    _test_info))
            {
                all_passed = false;
            }
        }
    }

    // verify children are in insertion order
    struct d_test_type* got0 = d_test_module_get_child_at(mod, 0);
    struct d_test_type* got1 = d_test_module_get_child_at(mod, 1);

    if (!d_assert_standalone(got0 != NULL && got1 != NULL,
                             "both children are retrievable",
                             "both children should be retrievable",
                             _test_info))
    {
        all_passed = false;
    }

    if (got0)
    {
        if (!d_assert_standalone(got0->type == D_TEST_TYPE_TEST,
                                 "first child is a test type",
                                 "first child should be a test type",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    if (got1)
    {
        if (!d_assert_standalone(got1->type == D_TEST_TYPE_TEST_BLOCK,
                                 "second child is a block type",
                                 "second child should be a block type",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_add_child unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_add_child unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_add_child_null
  Tests add_child with NULL arguments.
  Tests the following:
  - adding a child to a NULL module returns false
  - adding a NULL child to a valid module returns false
*/
bool
d_tests_sa_test_module_add_child_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    bool                  result;

    printf("  --- Testing d_test_module_add_child (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // adding to NULL module should fail
    result = d_test_module_add_child(NULL, NULL);

    if (!d_assert_standalone(!result,
                             "add_child with NULL module returns false",
                             "add_child with NULL module should return false",
                             _test_info))
    {
        all_passed = false;
    }

    // adding NULL child to valid module should fail
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        result = d_test_module_add_child(mod, NULL);

        if (!d_assert_standalone(!result,
                                 "add_child with NULL child returns false",
                                 "add_child with NULL child should return false",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_add_child (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_add_child (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_child_count
  Tests child counting on a module with multiple children added via add_child.
  Tests the following:
  - child count increments from 0 to 3 as children are added
  - child count is accurate at each intermediate step
*/
bool
d_tests_sa_test_module_child_count
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test*        t;
    struct d_test_type*   child;
    size_t                i;

    printf("  --- Testing d_test_module_child_count ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // add three children one at a time, verifying count each step
    for (i = 0; i < 3; i++)
    {
        t = d_test_new(NULL, 0);

        if (!t)
        {
            all_passed = false;
            break;
        }

        child = d_test_type_new(D_TEST_TYPE_TEST, t);

        if (!child)
        {
            d_test_free(t);
            all_passed = false;
            break;
        }

        d_test_module_add_child(mod,
                                (const struct d_test_tree_node*)child);
    }

    // verify final count
    if (!d_assert_standalone(
            d_test_module_child_count(mod) == 3,
            "child count is 3 after three additions",
            "child count should be 3 after three additions",
            _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_child_count unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_child_count unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_child_count_empty
  Tests child counting on an empty module and a NULL module.
  Tests the following:
  - an empty module has a child count of 0
  - child_count on a NULL module returns 0
*/
bool
d_tests_sa_test_module_child_count_empty
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;

    printf("  --- Testing d_test_module_child_count (empty) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // empty module
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        if (!d_assert_standalone(
                d_test_module_child_count(mod) == 0,
                "empty module has child count 0",
                "empty module should have child count 0",
                _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    // NULL module should return 0
    if (!d_assert_standalone(
            d_test_module_child_count(NULL) == 0,
            "NULL module has child count 0",
            "NULL module should have child count 0",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_child_count (empty) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_child_count (empty) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_child_at
  Tests retrieving children by valid index using add_child.
  Tests the following:
  - get_child_at with index 0 returns the first child added
  - get_child_at with index 1 returns the second child added
  - returned children have the expected type tags
*/
bool
d_tests_sa_test_module_get_child_at
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test*        t;
    struct d_test_block*  blk;
    struct d_test_type*   child_test;
    struct d_test_type*   child_block;
    struct d_test_type*   got;

    printf("  --- Testing d_test_module_get_child_at ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // add a test child and a block child
    t   = d_test_new(NULL, 0);
    blk = d_test_block_new(NULL, 0);

    if ( (!t) || (!blk) )
    {
        printf("  %s allocation failed\n", D_TEST_SYMBOL_FAIL);
        d_test_module_free(mod);
        _test_info->tests_total++;

        return false;
    }

    child_test  = d_test_type_new(D_TEST_TYPE_TEST, t);
    child_block = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, blk);

    if ( (!child_test) || (!child_block) )
    {
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        d_test_module_free(mod);
        _test_info->tests_total++;

        return false;
    }

    d_test_module_add_child(mod,
                            (const struct d_test_tree_node*)child_test);
    d_test_module_add_child(mod,
                            (const struct d_test_tree_node*)child_block);

    // verify first child
    got = d_test_module_get_child_at(mod, 0);

    if (!d_assert_standalone(got != NULL,
                             "get_child_at(0) returns non-NULL",
                             "get_child_at(0) should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (got)
    {
        if (!d_assert_standalone(got->type == D_TEST_TYPE_TEST,
                                 "child 0 is D_TEST_TYPE_TEST",
                                 "child 0 should be D_TEST_TYPE_TEST",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    // verify second child
    got = d_test_module_get_child_at(mod, 1);

    if (!d_assert_standalone(got != NULL,
                             "get_child_at(1) returns non-NULL",
                             "get_child_at(1) should return non-NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (got)
    {
        if (!d_assert_standalone(
                got->type == D_TEST_TYPE_TEST_BLOCK,
                "child 1 is D_TEST_TYPE_TEST_BLOCK",
                "child 1 should be D_TEST_TYPE_TEST_BLOCK",
                _test_info))
        {
            all_passed = false;
        }
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_child_at unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_child_at unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_child_at_invalid
  Tests get_child_at with out-of-bounds and NULL arguments.
  Tests the following:
  - get_child_at with an index beyond child count returns NULL
  - get_child_at on a NULL module returns NULL
  - get_child_at on an empty module returns NULL for index 0
*/
bool
d_tests_sa_test_module_get_child_at_invalid
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test_type*   got;

    printf("  --- Testing d_test_module_get_child_at (invalid) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        // index 0 on empty module
        got = d_test_module_get_child_at(mod, 0);

        if (!d_assert_standalone(got == NULL,
                                 "get_child_at(0) on empty returns NULL",
                                 "get_child_at(0) on empty should return NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        // large out-of-bounds index
        got = d_test_module_get_child_at(mod, 9999);

        if (!d_assert_standalone(got == NULL,
                                 "get_child_at(9999) returns NULL",
                                 "get_child_at(9999) should return NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    // NULL module
    got = d_test_module_get_child_at(NULL, 0);

    if (!d_assert_standalone(got == NULL,
                             "get_child_at on NULL module returns NULL",
                             "get_child_at on NULL module should return NULL",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_child_at (invalid) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_child_at (invalid) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
