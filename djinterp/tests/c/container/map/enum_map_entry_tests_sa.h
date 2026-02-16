/******************************************************************************
* djinterp [test]                                  enum_map_entry_tests_sa.h
*
*   Unit tests for the enum_map_entry module (entry structure and macros only).
*   Tests entry creation macros, comparison macros, and sentinel markers.
*   Does NOT test min_enum_map operations - that's a separate test suite.
*
*
* path:      \test\container\map\enum_map_entry_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.24
******************************************************************************/

#ifndef DJINTERP_ENUM_MAP_ENTRY_TESTS_STANDALONE_
#define DJINTERP_ENUM_MAP_ENTRY_TESTS_STANDALONE_ 1

#include "..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\inc\c\container\map\enum_map_entry.h"


/******************************************************************************
 * TEST ENUMERATIONS
 *****************************************************************************/

// test_color_enum
//   enum: sample enumeration for testing.
enum test_color_enum
{
    TEST_COLOR_RED    = 0,
    TEST_COLOR_GREEN  = 1,
    TEST_COLOR_BLUE   = 2,
    TEST_COLOR_YELLOW = 3,
    TEST_COLOR_PURPLE = 4
};


/******************************************************************************
 * ENTRY CREATION MACRO TESTS
 *****************************************************************************/

// tests D_ENUM_ENTRY and D_ENUM_KEY_ENTRY
struct d_test_object* d_tests_enum_map_entry_basic_macros(void);

// tests D_ENUM_ENTRY_STR, D_ENUM_ENTRY_INT, D_ENUM_ENTRY_NULL, D_ENUM_ENTRY_SELF
struct d_test_object* d_tests_enum_map_entry_typed_macros(void);

// entry creation aggregator
struct d_test_object* d_tests_enum_map_entry_creation_all(void);


/******************************************************************************
 * ENTRY COMPARISON MACRO TESTS
 *****************************************************************************/

// tests D_ENUM_ENTRY_KEY_EQ, D_ENUM_ENTRY_VAL_EQ, D_ENUM_ENTRY_EQ
struct d_test_object* d_tests_enum_map_entry_comparison_macros(void);


/******************************************************************************
 * SENTINEL MARKER TESTS
 *****************************************************************************/

// tests D_ENUM_ENTRY_SENTINEL and D_ENUM_ENTRY_IS_SENTINEL
struct d_test_object* d_tests_enum_map_entry_sentinel_macros(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// master test runner for all enum_map_entry tests
struct d_test_object* d_tests_enum_map_entry_run_all(void);


#endif  // DJINTERP_ENUM_MAP_ENTRY_TESTS_STANDALONE_





