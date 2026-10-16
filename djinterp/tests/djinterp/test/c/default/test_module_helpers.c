/*******************************************************************************
* djinterp [test]                                          test_module_helpers.c
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
* path:      /tests/djinterp/test/c/default/test_module_helpers.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.28
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "../../../../../inc/djinterp/test/c/default/test_module.h"
#include "../../../../../inc/djinterp/test/c/default/test.h"
#include "../../../../../inc/djinterp/test/c/default/test_block.h"


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
