/*******************************************************************************
* djinterp [test]                                            registry_tests_sa.h
*
*   Test declarations for registry module standalone tests.
*   Organises tests by header section; each section has individual test
*   functions and one *_all aggregation function.
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_REGISTRY_STANDALONE_
#define DJINTERP_TESTS_REGISTRY_STANDALONE_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/container/registry/registry.h"
#include "../../../../../inc/djinterp/c/container/registry/registry_common.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/dstring.h"
#include "../../../../../inc/djinterp/c/dmemory.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


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
bool d_tests_sa_registry_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_new_with_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_new_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_new_from_array(
    struct dj_test_counter* _counter
);

// I. aggregation function
bool d_tests_sa_registry_constructors_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. PRIMARY LOOKUP FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_get(
    struct dj_test_counter* _counter
);

// II. aggregation function
bool d_tests_sa_registry_lookup_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. ROW MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_add(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_add_rows(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_set(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_remove(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_remove_at(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_clear(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_registry_row_manipulation_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. ALIAS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_add_alias(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_add_aliases(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_remove_alias(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_clear_aliases(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_alias_count(
    struct dj_test_counter* _counter
);

// IV. aggregation function
bool d_tests_sa_registry_aliases_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_contains(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_index_of(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_at(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_count(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_capacity_fn(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_is_empty(
    struct dj_test_counter* _counter
);

// V. aggregation function
bool d_tests_sa_registry_queries_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. LOOKUP MAINTENANCE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_rebuild_lookup(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_sort_lookup(
    struct dj_test_counter* _counter
);

// VI. aggregation function
bool d_tests_sa_registry_lookup_maint_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. ITERATOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_iterator_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_iterator_filtered(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_iterator_has_next(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_iterator_next(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_iterator_reset(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_foreach(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_foreach_if(
    struct dj_test_counter* _counter
);

// VII. aggregation function
bool d_tests_sa_registry_iterators_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VIII. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_reserve(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_reserve_lookup_fn(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_shrink_to_fit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_freeze(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_thaw(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_get_all_keys(
    struct dj_test_counter* _counter
);

// VIII. aggregation function
bool d_tests_sa_registry_utility_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IX. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_free(
    struct dj_test_counter* _counter
);

// IX. aggregation function
bool d_tests_sa_registry_destructor_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * X. INTERNAL COMPARISON FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_lookup_compare(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_lookup_compare_nocase(
    struct dj_test_counter* _counter
);

// X. aggregation function
bool d_tests_sa_registry_comparison_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * XI. REGISTRY_COMMON FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_registry_strcmp(
    struct dj_test_counter* _counter
);
bool d_tests_sa_registry_schema_max_enum_key(
    struct dj_test_counter* _counter
);

// XI. aggregation function
bool d_tests_sa_registry_common_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_registry_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_REGISTRY_STANDALONE_
