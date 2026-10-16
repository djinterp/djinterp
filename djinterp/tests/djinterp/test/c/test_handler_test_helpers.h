/*******************************************************************************
* djinterp [test]                                    test_handler_test_helpers.h
*
*   Shared helper functions used by the test_handler standalone test suites.
*   Provides factory functions for creating pre-configured passing and failing
* tests, and utility functions for populating test blocks with children.
*
*
* path:      /tests/djinterp/test/c/test_handler_test_helpers.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_HANDLER_TEST_HELPERS_
#define DJINTERP_TEST_HANDLER_TEST_HELPERS_ 1

// djinterp
#include "../../../../inc/djinterp/test/c/test_handler.h"


/******************************************************************************
 * TEST FACTORY FUNCTIONS
 *****************************************************************************/

// helper_make_passing_test
//   function: creates a d_test containing a single test function that
// unconditionally passes (returns D_TEST_PASS).
struct d_test* helper_make_passing_test(void);

// helper_make_failing_test
//   function: creates a d_test containing a single test function that
// unconditionally fails (returns D_TEST_FAIL).
struct d_test* helper_make_failing_test(void);


/******************************************************************************
 * BLOCK POPULATION FUNCTIONS
 *****************************************************************************/

// helper_add_passing_test_to_block
//   function: creates a passing test and adds it as a child of the given
// block. returns true if the test was successfully created and added.
bool helper_add_passing_test_to_block(struct d_test_block* _block);

// helper_add_failing_test_to_block
//   function: creates a failing test and adds it as a child of the given
// block. returns true if the test was successfully created and added.
bool helper_add_failing_test_to_block(struct d_test_block* _block);

// helper_add_block_child_to_block
//   function: adds a pre-created child block as a child of the given
// parent block. returns true if the block was successfully added.
bool helper_add_block_child_to_block(struct d_test_block* _parent,
                                     struct d_test_block* _child);


#endif  // DJINTERP_TEST_HANDLER_TEST_HELPERS_
