/******************************************************************************
* djinterp [test]                                          predicate_tests_sa.h
*
*   Unit test declarations for `predicate.h` module.
*   Provides comprehensive testing of all d_predicate combinator functions
* including constructor functions (_new variants), evaluation functions (_eval
* variants), utility predicates, and compound literal macros. Tests cover AND,
* OR, XOR, and NOT combinators with both context and non-context variants.
*
*
* path:      \tests\functional\predicate_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.09
******************************************************************************/

#ifndef DJINTERP_TESTS_PREDICATE_STANDALONE_
#define DJINTERP_TESTS_PREDICATE_STANDALONE_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\inc\djinterp.h"
#include "..\..\inc\string_fn.h"
#include "..\..\inc\test\test_standalone.h"
#include "..\..\inc\functional\predicate.h"


// constructor function tests
bool d_tests_sa_predicate_and_new(struct d_test_counter* _counter);
bool d_tests_sa_predicate_or_new(struct d_test_counter* _counter);
bool d_tests_sa_predicate_xor_new(struct d_test_counter* _counter);
bool d_tests_sa_predicate_not_new(struct d_test_counter* _counter);
bool d_tests_sa_predicate_constructor_all(struct d_test_counter* _counter);

// evaulation function tests
bool d_tests_sa_predicate_and_eval(struct d_test_counter* _counter);
bool d_tests_sa_predicate_or_eval(struct d_test_counter* _counter);
bool d_tests_sa_predicate_xor_eval(struct d_test_counter* _counter);
bool d_tests_sa_predicate_not_eval(struct d_test_counter* _counter);
bool d_tests_sa_predicate_eval_all(struct d_test_counter* _counter);

// macro tests
bool d_tests_sa_predicate_macro_and(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_or(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_xor(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_not(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_and_simple(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_or_simple(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_xor_simple(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_not_simple(struct d_test_counter* _counter);
bool d_tests_sa_predicate_macro_all(struct d_test_counter* _counter);

// module-level aggregation
bool d_tests_sa_predicate_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_PREDICATE_STANDALONE
