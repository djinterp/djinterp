/*******************************************************************************
* djinterp [test]                                               registry_tests.h
*
*   Test declarations for the registry module's suite, on DTest's C face.
*   Organises tests by header section; each section has individual test
*   functions and one *_all aggregation function.
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_REGISTRY_STANDALONE_
#define DJINTERP_TESTS_REGISTRY_STANDALONE_ 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/container/registry/registry.h"
#include "../../../../../inc/djinterp/c/container/registry/registry_common.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/dstring.h"
#include "../../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST ROW TYPE
 *
 *   Every test file (except _common.c) includes this header and uses
 *   `struct test_row` as the user-defined row type.  The first member
 *   MUST be `const char*` (the key), per the registry contract.
 *****************************************************************************/

struct test_row
{
    const char* key;
    int         value;
};


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_new(
    struct dj_test_suite* _suite
);
bool d_tests_registry_new_with_capacity(
    struct dj_test_suite* _suite
);
bool d_tests_registry_new_copy(
    struct dj_test_suite* _suite
);
bool d_tests_registry_new_from_array(
    struct dj_test_suite* _suite
);

// I. aggregation function
bool d_tests_registry_constructors_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. PRIMARY LOOKUP FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_get(
    struct dj_test_suite* _suite
);

// II. aggregation function
bool d_tests_registry_lookup_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. ROW MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_add(
    struct dj_test_suite* _suite
);
bool d_tests_registry_add_rows(
    struct dj_test_suite* _suite
);
bool d_tests_registry_set(
    struct dj_test_suite* _suite
);
bool d_tests_registry_remove(
    struct dj_test_suite* _suite
);
bool d_tests_registry_remove_at(
    struct dj_test_suite* _suite
);
bool d_tests_registry_clear(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_registry_row_manipulation_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. ALIAS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_add_alias(
    struct dj_test_suite* _suite
);
bool d_tests_registry_add_aliases(
    struct dj_test_suite* _suite
);
bool d_tests_registry_remove_alias(
    struct dj_test_suite* _suite
);
bool d_tests_registry_clear_aliases(
    struct dj_test_suite* _suite
);
bool d_tests_registry_alias_count(
    struct dj_test_suite* _suite
);

// IV. aggregation function
bool d_tests_registry_aliases_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_contains(
    struct dj_test_suite* _suite
);
bool d_tests_registry_index_of(
    struct dj_test_suite* _suite
);
bool d_tests_registry_at(
    struct dj_test_suite* _suite
);
bool d_tests_registry_count(
    struct dj_test_suite* _suite
);
bool d_tests_registry_capacity_fn(
    struct dj_test_suite* _suite
);
bool d_tests_registry_is_empty(
    struct dj_test_suite* _suite
);

// V. aggregation function
bool d_tests_registry_queries_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. LOOKUP MAINTENANCE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_rebuild_lookup(
    struct dj_test_suite* _suite
);
bool d_tests_registry_sort_lookup(
    struct dj_test_suite* _suite
);

// VI. aggregation function
bool d_tests_registry_lookup_maint_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. ITERATOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_iterator_new(
    struct dj_test_suite* _suite
);
bool d_tests_registry_iterator_filtered(
    struct dj_test_suite* _suite
);
bool d_tests_registry_iterator_has_next(
    struct dj_test_suite* _suite
);
bool d_tests_registry_iterator_next(
    struct dj_test_suite* _suite
);
bool d_tests_registry_iterator_reset(
    struct dj_test_suite* _suite
);
bool d_tests_registry_foreach(
    struct dj_test_suite* _suite
);
bool d_tests_registry_foreach_if(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_registry_iterators_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_reserve(
    struct dj_test_suite* _suite
);
bool d_tests_registry_reserve_lookup_fn(
    struct dj_test_suite* _suite
);
bool d_tests_registry_shrink_to_fit(
    struct dj_test_suite* _suite
);
bool d_tests_registry_freeze(
    struct dj_test_suite* _suite
);
bool d_tests_registry_thaw(
    struct dj_test_suite* _suite
);
bool d_tests_registry_get_all_keys(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_registry_utility_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_free(
    struct dj_test_suite* _suite
);

// IX. aggregation function
bool d_tests_registry_destructor_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * X. INTERNAL COMPARISON FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_lookup_compare(
    struct dj_test_suite* _suite
);
bool d_tests_registry_lookup_compare_nocase(
    struct dj_test_suite* _suite
);

// X. aggregation function
bool d_tests_registry_comparison_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XI. REGISTRY_COMMON FUNCTION TESTS
 *****************************************************************************/
bool d_tests_registry_strcmp(
    struct dj_test_suite* _suite
);
bool d_tests_registry_schema_max_enum_key(
    struct dj_test_suite* _suite
);

// XI. aggregation function
bool d_tests_registry_common_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_registry_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_REGISTRY_STANDALONE_
