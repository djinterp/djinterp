/******************************************************************************
* djinterp [test]                                      min_enum_map_tests_sa.h
*
*   Unit tests for the min_enum_map module (minimal enum-keyed map).
*   Tests cover map creation, insertion, retrieval, removal, merging,
* and comprehensive edge cases.
*
*
* path:      \test\container\map\min_enum_map_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.24
******************************************************************************/

#ifndef DJINTERP_MIN_ENUM_MAP_TESTS_STANDALONE_
#define DJINTERP_MIN_ENUM_MAP_TESTS_STANDALONE_ 1

#include "..\..\..\..\inc\c\djinterp.h"
#include "..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\inc\c\container\map\min_enum_map.h"
#include "..\..\..\..\inc\c\container\map\enum_map_entry.h"
#include "..\..\..\..\inc\c\string_fn.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_MIN_ENUM_MAP_SMALL_SIZE
//   constant: small number of entries for basic tests.
#define D_TEST_MIN_ENUM_MAP_SMALL_SIZE      5

// D_TEST_MIN_ENUM_MAP_MEDIUM_SIZE
//   constant: medium number of entries for standard tests.
#define D_TEST_MIN_ENUM_MAP_MEDIUM_SIZE     20

// D_TEST_MIN_ENUM_MAP_LARGE_SIZE
//   constant: large number of entries for stress tests.
#define D_TEST_MIN_ENUM_MAP_LARGE_SIZE      100

// D_TEST_MIN_ENUM_MAP_STRESS_SIZE
//   constant: very large number for stress testing.
#define D_TEST_MIN_ENUM_MAP_STRESS_SIZE     1000


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
    TEST_COLOR_PURPLE = 4,
    TEST_COLOR_ORANGE = 5,
    TEST_COLOR_BLACK  = 6,
    TEST_COLOR_WHITE  = 7
};

// test_status_enum
//   enum: another sample enumeration for testing.
enum test_status_enum
{
    TEST_STATUS_PENDING   = 10,
    TEST_STATUS_ACTIVE    = 20,
    TEST_STATUS_COMPLETED = 30,
    TEST_STATUS_FAILED    = 40,
    TEST_STATUS_CANCELLED = 50
};

/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

// d_test_min_enum_map_create_int
//   function: helper to create heap-allocated integer for testing.
int* d_test_min_enum_map_create_int(int _value);

// d_test_min_enum_map_create_string
//   function: helper to create heap-allocated string copy for testing.
char* d_test_min_enum_map_create_string(const char* _str);


/******************************************************************************
 * CORE OPERATION TESTS
 *****************************************************************************/

// creation tests
struct d_test_object* d_tests_min_enum_map_new(void);

// insertion tests
struct d_test_object* d_tests_min_enum_map_put(void);

// retrieval tests
struct d_test_object* d_tests_min_enum_map_get(void);
struct d_test_object* d_tests_min_enum_map_contains(void);
struct d_test_object* d_tests_min_enum_map_count(void);

// removal tests
struct d_test_object* d_tests_min_enum_map_remove(void);

// core operations aggregator
struct d_test_object* d_tests_min_enum_map_core_all(void);


/******************************************************************************
 * MEMORY MANAGEMENT TESTS
 *****************************************************************************/

// clear operation tests
struct d_test_object* d_tests_min_enum_map_clear(void);

// destruction tests
struct d_test_object* d_tests_min_enum_map_free(void);

// memory management aggregator
struct d_test_object* d_tests_min_enum_map_memory_all(void);


/******************************************************************************
 * ADVANCED OPERATION TESTS
 *****************************************************************************/

// merge operation tests
struct d_test_object* d_tests_min_enum_map_merge(void);

// entry macro tests
struct d_test_object* d_tests_min_enum_map_entry_macros(void);

// sorting and ordering tests
struct d_test_object* d_tests_min_enum_map_ordering(void);

// advanced operations aggregator
struct d_test_object* d_tests_min_enum_map_advanced_all(void);


// EDGE CASE AND STRESS TESTS

// edge case tests
struct d_test_object* d_tests_min_enum_map_edge_cases(void);

// stress tests
struct d_test_object* d_tests_min_enum_map_stress(void);

// edge case and stress aggregator
struct d_test_object* d_tests_min_enum_map_edge_stress_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// master test runner for all min_enum_map tests
struct d_test_object* d_tests_min_enum_map_run_all(void);


#endif  // DJINTERP_MIN_ENUM_MAP_TESTS_STANDALONE_



