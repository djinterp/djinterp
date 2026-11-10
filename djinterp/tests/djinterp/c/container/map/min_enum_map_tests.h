/*******************************************************************************
* djinterp [test]                                           min_enum_map_tests.h
*
*   Unit tests for the min_enum_map module (minimal enum-keyed map).
*   Tests cover map creation, insertion, retrieval, removal, merging,
* and comprehensive edge cases.
*
*
* path:      /tests/djinterp/c/container/map/min_enum_map_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.24
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_MIN_ENUM_MAP_TESTS_STANDALONE_
#define DJINTERP_MIN_ENUM_MAP_TESTS_STANDALONE_ 1

// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/container/map/enum_map_entry.h"
#include "../../../../../inc/djinterp/c/container/map/min_enum_map.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


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
enum dj_test_color_min_enum_map
{
    D_TEST_COLOR_MIN_ENUM_MAP_RED    = 0,
    D_TEST_COLOR_MIN_ENUM_MAP_GREEN  = 1,
    D_TEST_COLOR_MIN_ENUM_MAP_BLUE   = 2,
    D_TEST_COLOR_MIN_ENUM_MAP_YELLOW = 3,
    D_TEST_COLOR_MIN_ENUM_MAP_PURPLE = 4,
    D_TEST_COLOR_MIN_ENUM_MAP_ORANGE = 5,
    D_TEST_COLOR_MIN_ENUM_MAP_BLACK  = 6,
    D_TEST_COLOR_MIN_ENUM_MAP_WHITE  = 7
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
int* d_test_min_enum_map_create_int(
    int _value
);

// d_test_min_enum_map_create_string
//   function: helper to create heap-allocated string copy for testing.
char* d_test_min_enum_map_create_string(
    const char* _str
);


/******************************************************************************
 * CORE OPERATION TESTS
 *****************************************************************************/

// creation tests
bool d_tests_min_enum_map_new(struct dj_test_suite* _suite);
bool d_tests_min_enum_map_new_copy(struct dj_test_suite* _suite);

// insertion tests
bool d_tests_min_enum_map_put(struct dj_test_suite* _suite);

// retrieval tests
bool d_tests_min_enum_map_get(struct dj_test_suite* _suite);
bool d_tests_min_enum_map_contains(struct dj_test_suite* _suite);
bool d_tests_min_enum_map_count(struct dj_test_suite* _suite);

// removal tests
bool d_tests_min_enum_map_remove(struct dj_test_suite* _suite);

// core operations aggregator
bool d_tests_min_enum_map_core_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MEMORY MANAGEMENT TESTS
 *****************************************************************************/

// clear operation tests
bool d_tests_min_enum_map_clear(struct dj_test_suite* _suite);

// destruction tests
bool d_tests_min_enum_map_free(struct dj_test_suite* _suite);

// memory management aggregator
bool d_tests_min_enum_map_memory_all(struct dj_test_suite* _suite);


/******************************************************************************
 * ADVANCED OPERATION TESTS
 *****************************************************************************/

// merge operation tests
bool d_tests_min_enum_map_merge(struct dj_test_suite* _suite);

// entry macro tests
bool d_tests_min_enum_map_entry_macros(struct dj_test_suite* _suite);

// sorting and ordering tests
bool d_tests_min_enum_map_ordering(struct dj_test_suite* _suite);

// advanced operations aggregator
bool d_tests_min_enum_map_advanced_all(struct dj_test_suite* _suite);


// EDGE CASE AND STRESS TESTS

// edge case tests
bool d_tests_min_enum_map_edge_cases(struct dj_test_suite* _suite);

// stress tests
bool d_tests_min_enum_map_stress(struct dj_test_suite* _suite);

// edge case and stress aggregator
bool d_tests_min_enum_map_edge_stress_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// master test runner for all min_enum_map tests
bool d_tests_min_enum_map_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_MIN_ENUM_MAP_TESTS_STANDALONE_
