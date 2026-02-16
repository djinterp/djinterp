/*******************************************************************************
* djinterp [event]                                      test_event_standalone.h
*
* This file contains tests for \inc\c\event\event.h
* For information regarding this file (and not the tests thereof), please go to
* \inc\c\event\event.h
*
*
* link:      TBA
* file:      \test\event\test_event.h
* author(s): Samuel 'teer' Neal-Blim                           date: 2023.06.27
*******************************************************************************/

#ifndef DJINTERP_TESTING_ASSERT_STANDALONE_
#define	DJINTERP_TESTING_ASSERT_STANDALONE_ 1

#include <stdlib.h>
#include <string.h>
#include "..\..\..\..\inc\c\djinterp.h"
#include "..\..\..\..\inc\c\container\map\enum_map.h"
#include "..\..\..\..\inc\c\test\test_standalone.h"


// Test enums for validation
enum TestEnum
{
    TEST_ENUM_FIRST = 1,
    TEST_ENUM_SECOND = 2,
    TEST_ENUM_THIRD = 5,
    TEST_ENUM_FOURTH = 10,
    TEST_ENUM_FIFTH = 15
};

enum TestEnumFlags
{
    TEST_FLAG_NONE = 0,
    TEST_FLAG_A = 1,
    TEST_FLAG_B = 2,
    TEST_FLAG_C = 4,
    TEST_FLAG_D = 8
};

// `d_enum_map` core function tests
bool d_tests_sa_enum_map_new(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_new_sized(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_new_copy(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_new_args(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_free(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_core_all(struct d_test_counter* _test_info);

// `d_enum_map` access function tests
bool d_tests_sa_enum_map_find(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_get(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_set(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_access_all(struct d_test_counter* _test_info);

// `d_enum_map` query function tests
bool d_tests_sa_enum_map_contains(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_contains_all(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_is_sorted(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_count_valid(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_query_all(struct d_test_counter* _test_info);

// `d_enum_map` static assertion tests
bool d_tests_sa_enum_map_static_assertions(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_compile_time_init(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_static_all(struct d_test_counter* _test_info);

// `d_enum_map` advanced tests
bool d_tests_sa_enum_map_integration(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_stress(struct d_test_counter* _test_info);
bool d_tests_sa_enum_map_advanced_all(struct d_test_counter* _test_info);

// root test function
bool d_tests_sa_enum_map_all(struct d_test_counter* _test_info);

// helper functions for testing
char* d_test_create_enum_string(const char* _prefix, int _value);
int   d_test_enum_comparator(const void* _a, const void* _b);


#endif	// DJINTERP_TESTING_ASSERT_STANDALONE_




