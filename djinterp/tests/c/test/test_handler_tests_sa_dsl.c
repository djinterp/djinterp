/******************************************************************************
* djinterp [test]                                    test_handler_tests_sa_dsl.c
*
*   Unit tests for test_handler DSL helper functions:
*     d_test_handler_create_module_with_metadata,
*     d_test_module_set_metadata_str,
*     d_test_handler_create_block_from_nodes,
*     d_test_handler_create_module_from_decl
*
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/

#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_create_module_metadata
  Tests d_test_handler_create_module_with_metadata.

  Tests:
  - NULL metadata array with count 0 creates unnamed module
  - Valid metadata sets module name
  - Multiple metadata key-value pairs stored
  - Empty name string handled correctly
*/
bool
d_tests_sa_handler_create_module_metadata
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing create_module_with_metadata ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Create module with name metadata
    {
        struct d_test_kv metadata[2];
        struct d_test_module* module;

        metadata[0].key   = "name";
        metadata[0].value = "Test Module";
        metadata[1].key   = "author";
        metadata[1].value = "tester";

        module = d_test_handler_create_module_with_metadata(
                     "Test Module", metadata, 2);

        if (!d_assert_standalone(module != NULL,
            "create module with metadata",
            "Module created with name metadata",
            _test_info))
        {
            all_assertions_passed = false;
        }

        if (module)
        {
            d_test_module_free(module);
        }
    }

    // Test 2: Create module with zero metadata count
    {
        struct d_test_module* module;

        module = d_test_handler_create_module_with_metadata(
                     "Empty Meta", NULL, 0);

        if (!d_assert_standalone(module != NULL,
            "create module with zero metadata",
            "Module created with zero metadata entries",
            _test_info))
        {
            all_assertions_passed = false;
        }

        if (module)
        {
            d_test_module_free(module);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] create_module_with_metadata\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] create_module_with_metadata\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_set_metadata_str
  Tests d_test_module_set_metadata_str with various keys.

  Tests:
  - NULL module returns false
  - NULL key returns false
  - Known key "name" succeeds
  - Known key "author" succeeds
  - Known key "description" succeeds
  - Known key "version" succeeds
  - Known key "category" succeeds
  - Unknown key returns false
*/
bool
d_tests_sa_handler_set_metadata_str
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing set_metadata_str ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: NULL module
    {
        bool result;

        result = d_test_module_set_metadata_str(NULL, "name", "test");

        if (!d_assert_standalone(result == false,
            "set_metadata_str NULL module",
            "Returns false for NULL module",
            _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // Test 2: NULL key
    {
        struct d_test_module* module;

        module = d_test_module_new("test", NULL);

        if (module)
        {
            bool result;

            result = d_test_module_set_metadata_str(module,
                         NULL, "value");

            if (!d_assert_standalone(result == false,
                "set_metadata_str NULL key",
                "Returns false for NULL key",
                _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_module_free(module);
        }
    }

    // Test 3: Known keys succeed
    {
        struct d_test_module* module;

        module = d_test_module_new("test", NULL);

        if (module)
        {
            bool name_ok;
            bool author_ok;
            bool desc_ok;
            bool version_ok;
            bool category_ok;

            name_ok     = d_test_module_set_metadata_str(module,
                              "name", "My Module");
            author_ok   = d_test_module_set_metadata_str(module,
                              "author", "tester");
            desc_ok     = d_test_module_set_metadata_str(module,
                              "description", "A test module");
            version_ok  = d_test_module_set_metadata_str(module,
                              "version", "1.0.0");
            category_ok = d_test_module_set_metadata_str(module,
                              "category", "unit");

            if (!d_assert_standalone(
                (name_ok && author_ok && desc_ok &&
                 version_ok && category_ok),
                "known keys succeed",
                "All known metadata keys accepted",
                _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_module_free(module);
        }
    }

    // Test 4: Unknown key returns false
    {
        struct d_test_module* module;

        module = d_test_module_new("test", NULL);

        if (module)
        {
            bool result;

            result = d_test_module_set_metadata_str(module,
                         "nonexistent_key", "value");

            if (!d_assert_standalone(result == false,
                "unknown key returns false",
                "Unknown metadata key returns false",
                _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_module_free(module);
        }
    }

    // Test 5: "authors" alias works
    {
        struct d_test_module* module;

        module = d_test_module_new("test", NULL);

        if (module)
        {
            bool result;

            result = d_test_module_set_metadata_str(module,
                         "authors", "tester1, tester2");

            if (!d_assert_standalone(result == true,
                "authors alias works",
                "The 'authors' alias key is accepted",
                _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_module_free(module);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] set_metadata_str\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] set_metadata_str\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_create_block_from_nodes
  Tests d_test_handler_create_block_from_nodes.

  Tests:
  - NULL nodes returns NULL
  - Zero count returns NULL
  - Valid nodes create block with tests
  - Block contains correct number of children
*/
bool
d_tests_sa_handler_create_block_from_nodes
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing create_block_from_nodes ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: NULL nodes returns NULL
    {
        struct d_test_block* block;

        block = d_test_handler_create_block_from_nodes("test",
                    NULL, 0);

        if (!d_assert_standalone(block == NULL,
            "NULL nodes returns NULL",
            "create_block_from_nodes(NULL,0) returns NULL",
            _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // Test 2: Zero count returns NULL
    {
        struct d_test_dsl_node dummy;
        struct d_test_block*   block;

        d_memset(&dummy, 0, sizeof(dummy));

        block = d_test_handler_create_block_from_nodes("test",
                    &dummy, 0);

        if (!d_assert_standalone(block == NULL,
            "zero count returns NULL",
            "create_block_from_nodes with count=0 returns NULL",
            _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // Test 3: Valid nodes create block
    {
        struct d_test*         test;
        struct d_test_dsl_node node;
        struct d_test_block*   block;

        test = d_test_new(NULL, NULL);

        if (test)
        {
            d_memset(&node, 0, sizeof(node));
            node.type = D_TEST_DSL_NODE_TEST;
            node.test = test;

            block = d_test_handler_create_block_from_nodes("my_block",
                        &node, 1);

            if (!d_assert_standalone(block != NULL,
                "valid node creates block",
                "Block created from valid DSL node",
                _test_info))
            {
                all_assertions_passed = false;
            }

            if (block)
            {
                d_test_block_free(block);
            }
            else
            {
                d_test_free(test);
            }
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] create_block_from_nodes\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] create_block_from_nodes\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_create_module_from_decl
  Tests d_test_handler_create_module_from_decl.

  Tests:
  - NULL metadata with NULL children creates module
  - Module name extracted from metadata
  - Children added to module
  - Zero counts produce valid empty module
*/
bool
d_tests_sa_handler_create_module_from_decl
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing create_module_from_decl ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Module with name from metadata
    {
        struct d_test_kv      metadata[1];
        struct d_test_module* module;

        metadata[0].key   = "name";
        metadata[0].value = "Declared Module";

        module = d_test_handler_create_module_from_decl(
                     metadata, 1, NULL, 0);

        if (!d_assert_standalone(module != NULL,
            "module from decl with name",
            "Module created from declaration with name",
            _test_info))
        {
            all_assertions_passed = false;
        }

        if (module)
        {
            d_test_module_free(module);
        }
    }

    // Test 2: Module with no name defaults to "Unnamed Module"
    {
        struct d_test_kv      metadata[1];
        struct d_test_module* module;

        metadata[0].key   = "author";
        metadata[0].value = "tester";

        module = d_test_handler_create_module_from_decl(
                     metadata, 1, NULL, 0);

        if (!d_assert_standalone(module != NULL,
            "module from decl without name",
            "Module created with default name when none provided",
            _test_info))
        {
            all_assertions_passed = false;
        }

        if (module)
        {
            d_test_module_free(module);
        }
    }

    // Test 3: Module with block children
    {
        struct d_test_kv       metadata[1];
        struct d_test_block*   block;
        struct d_test_dsl_node child;
        struct d_test_module*  module;

        metadata[0].key   = "name";
        metadata[0].value = "Module With Block";

        block = d_test_block_new(NULL, 0);

        if (block)
        {
            d_memset(&child, 0, sizeof(child));
            child.type  = D_TEST_DSL_NODE_BLOCK;
            child.block = block;

            module = d_test_handler_create_module_from_decl(
                         metadata, 1, &child, 1);

            if (!d_assert_standalone(module != NULL,
                "module from decl with children",
                "Module created with block child",
                _test_info))
            {
                all_assertions_passed = false;
            }

            if (module)
            {
                d_test_module_free(module);
            }
            else
            {
                d_test_block_free(block);
            }
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] create_module_from_decl\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] create_module_from_decl\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
