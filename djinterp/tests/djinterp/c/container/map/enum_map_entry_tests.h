/*******************************************************************************
* djinterp [test]                                         enum_map_entry_tests.h
*
*   Unit tests for the enum_map_entry module (entry structure and macros only).
*   Tests entry creation macros, comparison macros, and sentinel markers.
*   Does NOT test min_enum_map operations - that's a separate test suite.
*
*
* path:      /tests/djinterp/c/container/map/enum_map_entry_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.24
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_ENUM_MAP_ENTRY_TESTS_STANDALONE_
#define DJINTERP_ENUM_MAP_ENTRY_TESTS_STANDALONE_ 1

// djinterp
#include "../../../../../inc/djinterp/c/container/map/enum_map_entry.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST ENUMERATIONS
 *****************************************************************************/

// test_color_enum
//   enum: sample enumeration for testing.
enum dj_test_color_enum_map_entry
{
    D_TEST_COLOR_ENUM_MAP_ENTRY_RED    = 0,
    D_TEST_COLOR_ENUM_MAP_ENTRY_GREEN  = 1,
    D_TEST_COLOR_ENUM_MAP_ENTRY_BLUE   = 2,
    D_TEST_COLOR_ENUM_MAP_ENTRY_YELLOW = 3,
    D_TEST_COLOR_ENUM_MAP_ENTRY_PURPLE = 4
};


/******************************************************************************
 * ENTRY CREATION MACRO TESTS
 *****************************************************************************/

// tests D_ENUM_ENTRY and D_ENUM_KEY_ENTRY
bool d_tests_enum_map_entry_basic_macros(struct dj_test_suite* _suite);

// tests D_ENUM_ENTRY_STR, D_ENUM_ENTRY_INT, D_ENUM_ENTRY_NULL, D_ENUM_ENTRY_SELF
bool d_tests_enum_map_entry_typed_macros(struct dj_test_suite* _suite);

// entry creation aggregator
bool d_tests_enum_map_entry_creation_all(struct dj_test_suite* _suite);


/******************************************************************************
 * ENTRY COMPARISON MACRO TESTS
 *****************************************************************************/

// tests D_ENUM_ENTRY_KEY_EQ, D_ENUM_ENTRY_VAL_EQ, D_ENUM_ENTRY_EQ
bool d_tests_enum_map_entry_comparison_macros(struct dj_test_suite* _suite);


/******************************************************************************
 * SENTINEL MARKER TESTS
 *****************************************************************************/

// tests D_ENUM_ENTRY_SENTINEL and D_ENUM_ENTRY_IS_SENTINEL
bool d_tests_enum_map_entry_sentinel_macros(struct dj_test_suite* _suite);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// master test runner for all enum_map_entry tests
bool d_tests_enum_map_entry_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_ENUM_MAP_ENTRY_TESTS_STANDALONE_
