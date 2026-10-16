/*******************************************************************************
* djinterp [test]                                             compose_tests_sa.h
*
*   Unit test declarations for `compose.h` module.
*   Provides comprehensive testing of all function composition and partial
* application functions including transformer composition (_compose_* variants),
* partial consumer functions, composition application operations, and
* convenience/template macros.
*
*
* path:      /tests/djinterp/c/functional/compose_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.09
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COMPOSE_SA_
#define DJINTERP_TESTS_COMPOSE_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/compose.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * I. TRANSFORMER COMPOSITION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_compose_new(struct d_test_counter* _counter);
bool d_tests_sa_compose_apply(struct d_test_counter* _counter);
bool d_tests_sa_compose_free(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_compose_transformer_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. PARTIAL APPLICATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_partial_consumer_new(struct d_test_counter* _counter);
bool d_tests_sa_partial_consumer_apply(struct d_test_counter* _counter);
bool d_tests_sa_partial_consumer_free(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_compose_partial_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. MACRO TESTS
 *****************************************************************************/
bool d_tests_sa_compose_macro_compose_fn(struct d_test_counter* _counter);
bool d_tests_sa_compose_macro_make_composable(struct d_test_counter* _counter);
bool d_tests_sa_compose_macro_make_predicate_from(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_compose_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_compose_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_COMPOSE_SA_
