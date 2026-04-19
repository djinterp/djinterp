#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_create_module_metadata
  Tests d_test_handler_create_module_with_metadata.
  Tests the following:
  - valid metadata creates named module
  - zero metadata count creates module
*/
bool
d_tests_sa_handler_create_module_metadata
(
    struct d_test_counter* _test_info
)
{
    size_t                initial_tests_passed;
    bool                  all_assertions_passed;
    struct d_test_kv      metadata[2];
    struct d_test_module* module;

    printf("  --- Testing create_module_with_metadata ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // create module with name metadata
    metadata[0].key   = "name";
    metadata[0].value = "Test Module";
    metadata[1].key   = "author";
    metadata[1].value = "tester";

    module = d_test_handler_create_module_with_metadata(
                 "Test Module", metadata, 2);

    all_assertions_passed &= d_assert_standalone(
        module != NULL,
        "create module with metadata",
        "Module created with name metadata",
        _test_info);

    if (module)
    {
        d_test_module_free(module);
    }

    // create module with zero metadata count
    module = d_test_handler_create_module_with_metadata(
                 "Empty Meta", NULL, 0);

    all_assertions_passed &= d_assert_standalone(
        module != NULL,
        "create module with zero metadata",
        "Module created with zero metadata entries",
        _test_info);

    if (module)
    {
        d_test_module_free(module);
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
  Tests the following:
  - NULL module returns false
  - NULL key returns false
  - known keys (name, author, description, version, category) succeed
  - unknown key returns false
  - "authors" alias key is accepted
*/
bool
d_tests_sa_handler_set_metadata_str
(
    struct d_test_counter* _test_info
)
{
    size_t                initial_tests_passed;
    bool                  all_assertions_passed;
    bool                  result;
    struct d_test_module* module;

    printf("  --- Testing set_metadata_str ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // NULL module returns false
    result = d_test_module_set_metadata_str(NULL, "name", "test");

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "set_metadata_str NULL module",
        "Returns false for NULL module",
        _test_info);

    // NULL key returns false
    module = d_test_module_new("test", NULL);

    if (module)
    {
        result = d_test_module_set_metadata_str(module,
                     NULL,
                     "value");

        all_assertions_passed &= d_assert_standalone(
            result == false,
            "set_metadata_str NULL key",
            "Returns false for NULL key",
            _test_info);

        d_test_module_free(module);
    }

    // known keys succeed
    module = d_test_module_new("test", NULL);

    if (module)
    {
        result = d_test_module_set_metadata_str(module,
                     "name", "My Module")
              && d_test_module_set_metadata_str(module,
                     "author", "tester")
              && d_test_module_set_metadata_str(module,
                     "description", "A test module")
              && d_test_module_set_metadata_str(module,
                     "version", "1.0.0")
              && d_test_module_set_metadata_str(module,
                     "category", "unit");

        all_assertions_passed &= d_assert_standalone(
            result == true,
            "known keys succeed",
            "All known metadata keys accepted",
            _test_info);

        d_test_module_free(module);
    }

    // unknown key returns false
    module = d_test_module_new("test", NULL);

    if (module)
    {
        result = d_test_module_set_metadata_str(module,
                     "nonexistent_key",
                     "value");

        all_assertions_passed &= d_assert_standalone(
            result == false,
            "unknown key returns false",
            "Unknown metadata key returns false",
            _test_info);

        d_test_module_free(module);
    }

    // "authors" alias works
    module = d_test_module_new("test", NULL);

    if (module)
    {
        result = d_test_module_set_metadata_str(module,
                     "authors",
                     "tester1, tester2");

        all_assertions_passed &= d_assert_standalone(
            result == true,
            "authors alias works",
            "The 'authors' alias key is accepted",
            _test_info);

        d_test_module_free(module);
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
  Tests the following:
  - NULL nodes returns NULL
  - zero count returns NULL
  - valid nodes create block with tests
*/
bool
d_tests_sa_handler_create_block_from_nodes
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    struct d_test_dsl_node node;
    struct d_test_block*   block;
    struct d_test*         test;

    printf("  --- Testing create_block_from_nodes ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // NULL nodes returns NULL
    block = d_test_handler_create_block_from_nodes("test",
                NULL, 0);

    all_assertions_passed &= d_assert_standalone(
        block == NULL,
        "NULL nodes returns NULL",
        "create_block_from_nodes(NULL,0) returns NULL",
        _test_info);

    // zero count returns NULL
    d_memset(&node, 0, sizeof(node));

    block = d_test_handler_create_block_from_nodes("test",
                &node, 0);

    all_assertions_passed &= d_assert_standalone(
        block == NULL,
        "zero count returns NULL",
        "create_block_from_nodes with count=0 returns NULL",
        _test_info);

    // valid nodes create block
    test = d_test_new(NULL, NULL);

    if (test)
    {
        d_memset(&node, 0, sizeof(node));
        node.type = D_TEST_DSL_NODE_TEST;
        node.test = test;

        block = d_test_handler_create_block_from_nodes("my_block",
                    &node, 1);

        all_assertions_passed &= d_assert_standalone(
            block != NULL,
            "valid node creates block",
            "Block created from valid DSL node",
            _test_info);

        if (block)
        {
            d_test_block_free(block);
        }
        else
        {
            d_test_free(test);
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
  Tests the following:
  - module with name from metadata
  - module with no name defaults to "Unnamed Module"
  - module with block children
*/
bool
d_tests_sa_handler_create_module_from_decl
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    struct d_test_kv       metadata[1];
    struct d_test_module*  module;
    struct d_test_block*   block;
    struct d_test_dsl_node child;

    printf("  --- Testing create_module_from_decl ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // module with name from metadata
    metadata[0].key   = "name";
    metadata[0].value = "Declared Module";

    module = d_test_handler_create_module_from_decl(
                 metadata, 1, NULL, 0);

    all_assertions_passed &= d_assert_standalone(
        module != NULL,
        "module from decl with name",
        "Module created from declaration with name",
        _test_info);

    if (module)
    {
        d_test_module_free(module);
    }

    // module with no name defaults to "Unnamed Module"
    metadata[0].key   = "author";
    metadata[0].value = "tester";

    module = d_test_handler_create_module_from_decl(
                 metadata, 1, NULL, 0);

    all_assertions_passed &= d_assert_standalone(
        module != NULL,
        "module from decl without name",
        "Module created with default name when none provided",
        _test_info);

    if (module)
    {
        d_test_module_free(module);
    }

    // module with block children
    metadata[0].key   = "name";
    metadata[0].value = "Module With Block";

    block = d_test_block_new("child_block", NULL);

    if (block)
    {
        d_memset(&child, 0, sizeof(child));
        child.type  = D_TEST_DSL_NODE_BLOCK;
        child.block = block;

        module = d_test_handler_create_module_from_decl(
                     metadata, 1, &child, 1);

        all_assertions_passed &= d_assert_standalone(
            module != NULL,
            "module from decl with children",
            "Module created with block child",
            _test_info);

        if (module)
        {
            d_test_module_free(module);
        }
        else
        {
            d_test_block_free(block);
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
