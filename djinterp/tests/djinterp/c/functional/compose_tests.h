/*******************************************************************************
* djinterp [test]                                                compose_tests.h
*
*   Unit test declarations for `compose.h` module.
*   Provides comprehensive testing of all function composition and partial
* application functions including transformer composition (_compose_* variants),
* partial consumer functions, composition application operations, and
* convenience/template macros.
*
*
* path:      /tests/djinterp/c/functional/compose_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_COMPOSE_
#define DJINTERP_TESTS_COMPOSE_ 1

// std
#include <stddef.h>
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/compose.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. TRANSFORMER COMPOSITION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_compose_new(
    struct dj_test_suite* _suite
);
bool d_tests_compose_apply(
    struct dj_test_suite* _suite
);
bool d_tests_compose_free(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_compose_transformer_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. PARTIAL APPLICATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_partial_consumer_new(
    struct dj_test_suite* _suite
);
bool d_tests_partial_consumer_apply(
    struct dj_test_suite* _suite
);
bool d_tests_partial_consumer_free(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_compose_partial_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. MACRO TESTS
 *****************************************************************************/
bool d_tests_compose_macro_compose_fn(
    struct dj_test_suite* _suite
);
bool d_tests_compose_macro_make_composable(
    struct dj_test_suite* _suite
);
bool d_tests_compose_macro_make_predicate_from(
    struct dj_test_suite* _suite
);
bool d_tests_compose_macro_map_then_filter(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_compose_macro_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_compose_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_COMPOSE_
