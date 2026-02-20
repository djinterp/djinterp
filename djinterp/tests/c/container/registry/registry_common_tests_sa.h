/******************************************************************************
* djinterp [test]                                    registry_common_tests_sa.h
*
*   Unit test declarations for `registry_common.h` module.
*   Provides comprehensive testing of all registry_common functions including
* string comparison with case sensitivity options and schema enumeration key
* utilities.
*
*
* path:      \tests\container\registry\registry_common_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.05
******************************************************************************/

#ifndef DJINTERP_TESTS_REGISTRY_COMMON_SA_
#define DJINTERP_TESTS_REGISTRY_COMMON_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\..\inc\test\test_standalone.h"
#include "..\..\..\inc\container\registry\registry_common.h"
#include "..\..\..\inc\string_fn.h"


/******************************************************************************
 * I. STRING COMPARISON FUNCTION TESTS (d_registry_strcmp)
 *****************************************************************************/

// basic comparison tests
bool d_tests_sa_registry_strcmp_equal_strings(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_less_than(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_greater_than(struct d_test_counter* _counter);

// null handling tests
bool d_tests_sa_registry_strcmp_both_null(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_first_null(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_second_null(struct d_test_counter* _counter);

// case sensitivity tests
bool d_tests_sa_registry_strcmp_case_sensitive_diff(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_case_insensitive_equal(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_case_insensitive_diff(struct d_test_counter* _counter);

// edge case tests
bool d_tests_sa_registry_strcmp_empty_strings(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_empty_vs_nonempty(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_prefix_strings(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_special_chars(struct d_test_counter* _counter);
bool d_tests_sa_registry_strcmp_numeric_chars(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_registry_strcmp_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. SCHEMA MAX ENUM KEY FUNCTION TESTS (d_registry_schema_max_enum_key)
 *****************************************************************************/

// basic functionality tests
bool d_tests_sa_registry_schema_max_enum_key_single(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_multiple(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_first_is_max(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_last_is_max(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_middle_is_max(struct d_test_counter* _counter);

// null and empty handling tests
bool d_tests_sa_registry_schema_max_enum_key_null_schema(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_zero_count(struct d_test_counter* _counter);

// edge case tests
bool d_tests_sa_registry_schema_max_enum_key_all_same(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_max_uint16(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_zero_keys(struct d_test_counter* _counter);
bool d_tests_sa_registry_schema_max_enum_key_sparse(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_registry_schema_max_enum_key_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_registry_common_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_REGISTRY_COMMON_SA_
