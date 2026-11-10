/*******************************************************************************
* djinterp [test]                                              predicate_tests.h
*
*   Unit test declarations for `predicate.h` module.
*   Provides comprehensive testing of all d_predicate combinator functions
* including constructor functions (_new variants), evaluation functions (_eval
* variants), utility predicates, and compound literal macros. Tests cover AND,
* OR, XOR, and NOT combinators with both context and non-context variants.
*
*
* path:      /tests/djinterp/c/functional/predicate_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.09
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_PREDICATE_STANDALONE_
#define DJINTERP_TESTS_PREDICATE_STANDALONE_ 1

// std
#include <stddef.h>
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/c/functional/predicate.h"
#include "./predicate_tests_helpers.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// constructor function tests
bool d_tests_predicate_and_new(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_or_new(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_xor_new(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_not_new(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_constructor_all(
    struct dj_test_suite* _suite
);

// evaulation function tests
bool d_tests_predicate_and_eval(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_or_eval(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_xor_eval(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_not_eval(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_eval_all(
    struct dj_test_suite* _suite
);

// macro tests
bool d_tests_predicate_macro_and(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_or(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_xor(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_not(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_and_simple(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_or_simple(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_xor_simple(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_not_simple(
    struct dj_test_suite* _suite
);
bool d_tests_predicate_macro_all(
    struct dj_test_suite* _suite
);

// module-level aggregation
bool d_tests_predicate_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_PREDICATE_STANDALONE
