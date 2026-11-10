/*******************************************************************************
* djinterp [test]                                        registry_common_tests.h
*
*   Unit test declarations for `registry_common.h` module.
*   Provides comprehensive testing of all registry_common functions including
* string comparison with case sensitivity options and schema enumeration key
* utilities.
*
*
* path:      /tests/djinterp/c/container/registry/registry_common_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.05
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_REGISTRY_COMMON_
#define DJINTERP_TESTS_REGISTRY_COMMON_ 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/registry/registry_common.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. STRING COMPARISON FUNCTION TESTS (d_registry_strcmp)
 *****************************************************************************/

// basic comparison tests
bool d_tests_registry_strcmp_equal_strings(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_less_than(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_greater_than(
    struct dj_test_suite* _suite
);

// null handling tests
bool d_tests_registry_strcmp_both_null(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_first_null(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_second_null(
    struct dj_test_suite* _suite
);

// case sensitivity tests
bool d_tests_registry_strcmp_case_sensitive_diff(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_case_insensitive_equal(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_case_insensitive_diff(
    struct dj_test_suite* _suite
);

// edge case tests
bool d_tests_registry_strcmp_empty_strings(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_empty_vs_nonempty(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_prefix_strings(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_special_chars(
    struct dj_test_suite* _suite
);
bool d_tests_registry_strcmp_numeric_chars(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_registry_strcmp_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. SCHEMA MAX ENUM KEY FUNCTION TESTS (d_registry_schema_max_enum_key)
 *****************************************************************************/

// basic functionality tests
bool d_tests_registry_schema_max_enum_key_single(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_multiple(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_first_is_max(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_last_is_max(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_middle_is_max(
    struct dj_test_suite* _suite
);

// null and empty handling tests
bool d_tests_registry_schema_max_enum_key_null_schema(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_zero_count(
    struct dj_test_suite* _suite
);

// edge case tests
bool d_tests_registry_schema_max_enum_key_all_same(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_max_uint16(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_zero_keys(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key_sparse(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_registry_schema_max_enum_key_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_registry_common_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_REGISTRY_COMMON_
