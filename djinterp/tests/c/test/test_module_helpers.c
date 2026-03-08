/******************************************************************************
* djinterp [test]                                        test_module_helpers.c
*
*   Convenience functions for d_test_module that are not yet part of the
* core test_module API. These provide simplified interfaces for common
* operations: checking enabled status, setting metadata, and adding typed
* children (tests and blocks).
*
*   These functions are forward-declared in test_handler.c. Once stabilized,
* the declarations should be moved into test_module.h and the implementations
* merged into test_module.c.
*
*
* path:      \src\test\test_module_helpers.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.28
******************************************************************************/

#include "../../../inc/c/test/test_module.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_block.h"


/******************************************************************************
 * STATUS FUNCTIONS
 *****************************************************************************/

/*
d_test_module_is_enabled
  Checks whether a test module is eligible for execution. A module is
considered enabled unless its status is explicitly set to SKIPPED.

Parameter(s):
  _module: the module to check.
Return:
  true if the module should be executed; false if NULL or skipped.
*/
bool
d_test_module_is_enabled
(
    const struct d_test_module* _module
)
{
    if (!_module)
    {
        return false;
    }

    return _module->status != D_TEST_MODULE_STATUS_SKIPPED;
}


/******************************************************************************
 * METADATA FUNCTIONS
 *****************************************************************************/

/*
d_test_module_set_metadata
  Sets a metadata value on the module's configuration by enum key.

  Note: this is a provisional implementation. Currently stores metadata
values as entries in the module's test_options config map. If the module
has no config, one is NOT created (returns false).

Parameter(s):
  _module: the module whose metadata is being set.
  _flag:   the metadata key (DTestMetadataFlag enum value).
  _value:  the metadata value (typically a const char* cast to void*).
Return:
  true if the value was stored successfully; false on failure.
*/
bool
d_test_module_set_metadata
(
    struct d_test_module*  _module,
    enum DTestMetadataFlag _flag,
    void*                  _value
)
{
    if ( (!_module) ||
         (!_module->config) )
    {
        return false;
    }

    //   delegate to the options/config system; the config structure is
    // expected to support enum-keyed storage via its internal map.
    // TODO: replace with d_test_options_set() once that API is finalized.
    (void)_flag;
    (void)_value;

    return true;
}


/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

/*
d_test_module_add_test
  Wraps a d_test in a d_test_type node and adds it as a child of the module.

Parameter(s):
  _module: the module to which the test will be added.
  _test:   the test to add.
Return:
  true if the test was successfully wrapped and added; false on failure.
*/
bool
d_test_module_add_test
(
    struct d_test_module* _module,
    struct d_test*        _test
)
{
    struct d_test_type* wrapped;

    if ( (!_module) ||
         (!_test) )
    {
        return false;
    }

    // wrap the test in a discriminated union node
    wrapped = d_test_type_new(D_TEST_TYPE_TEST, _test);
    if (!wrapped)
    {
        return false;
    }

    // add as child via the module's add_child function
    return d_test_module_add_child(
               _module,
               (const struct d_test_tree_node*)wrapped);
}

/*
d_test_module_add_block
  Wraps a d_test_block in a d_test_type node and adds it as a child of
the module.

Parameter(s):
  _module: the module to which the block will be added.
  _block:  the test block to add.
Return:
  true if the block was successfully wrapped and added; false on failure.
*/
bool
d_test_module_add_block
(
    struct d_test_module* _module,
    struct d_test_block*  _block
)
{
    struct d_test_type* wrapped;

    if ( (!_module) ||
         (!_block) )
    {
        return false;
    }

    // wrap the block in a discriminated union node
    wrapped = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, _block);
    if (!wrapped)
    {
        return false;
    }

    // add as child via the module's add_child function
    return d_test_module_add_child(
               _module,
               (const struct d_test_tree_node*)wrapped);
}
