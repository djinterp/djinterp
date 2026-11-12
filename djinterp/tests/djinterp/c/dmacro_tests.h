/*******************************************************************************
* djinterp [c]                                                    dmacro_tests.h
*
*   This is a test file for `dmacro.h` unit tests.
*   For the file itself, go to `\inc\c\dmacro.h`.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Configuration system test functions
      -----------------------------------

II.   Token manipulation test functions
      ---------------------------------

III.  Array utilities test functions
      ------------------------------

IV.   Argument counting test functions
      --------------------------------

V.    Macro expansion and evaluation test functions
      ---------------------------------------------

VI.   Boolean and conditional logic test functions
      --------------------------------------------

VII.  Argument selection test functions
      ---------------------------------

VIII. Core iteration infrastructure test functions
      --------------------------------------------

IX.   For_each implementations test functions
      ---------------------------------------

X.    Pair and indexed iteration test functions
      -----------------------------------------

XI.   Member access iteration test functions
      --------------------------------------

XII.  Advanced iteration patterns test functions
      ------------------------------------------

XIII. Pointer array initialization test functions
      -------------------------------------------

XIV.  Utility operators test functions
      --------------------------------

XV.   Compile-time assertions test functions
      --------------------------------------

XVI.  Master test suite
      -----------------
*/

#ifndef DJINTERP_TESTS_C_DMACRO_TESTS_H
#define DJINTERP_TESTS_C_DMACRO_TESTS_H 1

// std
#include "../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"
#include "../../../inc/djinterp/c/dmacro.h"
#include "../../../inc/djinterp/c/string_fn.h"
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// I. Configuration system test functions
// configuration constant tests
bool d_tests_dmacro_cfg_constants(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_cfg_user_options(
    struct dj_test_suite* _suite
);

// effective value tests
bool d_tests_dmacro_cfg_effective_values(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_cfg_public_aliases(
    struct dj_test_suite* _suite
);

// query macro tests
bool d_tests_dmacro_cfg_query_macros(
    struct dj_test_suite* _suite
);

// environment integration tests
bool d_tests_dmacro_cfg_env_integration(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_cfg_variant_consistency(
    struct dj_test_suite* _suite
);

// configuration module aggregator
bool d_tests_dmacro_cfg_all(
    struct dj_test_suite* _suite
);

// II.   Token manipulation test functions
// token pasting tests (D_CONCAT, D_INTERNAL_CONCAT_HELPER)
bool d_tests_dmacro_concat_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_concat_with_macros(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_concat_edge_cases(
    struct dj_test_suite* _suite
);

// stringification tests (D_STRINGIFY, D_TOSTR)
bool d_tests_dmacro_stringify_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_stringify_vs_tostr(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_stringify_edge_cases(
    struct dj_test_suite* _suite
);

// expansion control tests (D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT, D_UNPACK)
bool d_tests_dmacro_expand_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_empty_macro(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_defer_macro(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_obstruct_macro(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_unpack_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_unpack_nested(
    struct dj_test_suite* _suite
);

// token manipulation module aggregator
bool d_tests_dmacro_token_all(
    struct dj_test_suite* _suite
);

// III.  Array utilities test functions
// compile-time array sizing tests
bool d_tests_dmacro_array_count_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_array_count_initialized(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_array_count_structs(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_array_count_safe(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_array_count_t(
    struct dj_test_suite* _suite
);

// array generation tests
bool d_tests_dmacro_make_array(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_make_string_array(
    struct dj_test_suite* _suite
);

// array utilities module aggregator
bool d_tests_dmacro_array_all(
    struct dj_test_suite* _suite
);

// IV.   Argument counting test functions
// D_VARG_COUNT tests
bool d_tests_dmacro_varg_count_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_varg_count_medium(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_varg_count_large(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_varg_count_types(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_varg_count_edge_cases(
    struct dj_test_suite* _suite
);

// D_HAS_ARGS tests
bool d_tests_dmacro_has_args_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_has_args_types(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_has_args_large_counts(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_has_args_conditionals(
    struct dj_test_suite* _suite
);

// combined and boundary tests
bool d_tests_dmacro_arg_count_combined(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_arg_count_boundary(
    struct dj_test_suite* _suite
);

// argument counting module aggregator
bool d_tests_dmacro_arg_count_all(
    struct dj_test_suite* _suite
);

// V.    Macro expansion and evaluation test functions
// D_INC tests
bool d_tests_dmacro_inc_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_inc_medium(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_inc_large(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_inc_chained(
    struct dj_test_suite* _suite
);

// D_EVAL tests
bool d_tests_dmacro_eval_basic(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_eval_nested(
    struct dj_test_suite* _suite
);

// iteration macro tests (MSVC-compatible, use D_STRINGIFY only)
bool d_tests_dmacro_for_each_stringify(
    struct dj_test_suite* _suite
);

// practical patterns test
bool d_tests_dmacro_eval_practical(
    struct dj_test_suite* _suite
);

// macro expansion and evaluation module aggregator
bool d_tests_dmacro_eval_all(
    struct dj_test_suite* _suite
);

// VI.   Boolean and conditional logic test functions
// probe mechanism tests
bool d_tests_dmacro_probe_check(
    struct dj_test_suite* _suite
);

// parentheses detection tests
bool d_tests_dmacro_is_paren(
    struct dj_test_suite* _suite
);

// conditional expansion tests
bool d_tests_dmacro_if_macros(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_iif_macros(
    struct dj_test_suite* _suite
);

// boolean logic tests
bool d_tests_dmacro_not_compl(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_bool_macro(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_and_or(
    struct dj_test_suite* _suite
);

// boolean logic module aggregator
bool d_tests_dmacro_boolean_all(
    struct dj_test_suite* _suite
);

// VII.  Argument selection test functions
// convenience alias tests
bool d_tests_dmacro_first_second_third(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_head_rest_tail(
    struct dj_test_suite* _suite
);

// parentheses handling tests
bool d_tests_dmacro_remove_parentheses(
    struct dj_test_suite* _suite
);

// argument selection module aggregator
bool d_tests_dmacro_selection_all(
    struct dj_test_suite* _suite
);

// VIII. Core iteration infrastructure test functions
// map termination tests
bool d_tests_dmacro_map_end_detection(
    struct dj_test_suite* _suite
);

// core mapping tests
bool d_tests_dmacro_map_internals(
    struct dj_test_suite* _suite
);

// iteration infrastructure module aggregator
bool d_tests_dmacro_iteration_core_all(
    struct dj_test_suite* _suite
);

// IX.   For_each implementations test functions
// basic iteration tests
bool d_tests_dmacro_for_each_basic(
    struct dj_test_suite* _suite
);

// separated iteration tests
bool d_tests_dmacro_for_each_sep(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_for_each_comma(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_for_each_semicolon(
    struct dj_test_suite* _suite
);

// data-passing iteration tests
bool d_tests_dmacro_for_each_data(
    struct dj_test_suite* _suite
);

// for_each module aggregator
bool d_tests_dmacro_for_each_all(
    struct dj_test_suite* _suite
);

// X.    Pair and indexed iteration test functions
// pair iteration tests
bool d_tests_dmacro_for_each_pair(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_for_each_pair_sep(
    struct dj_test_suite* _suite
);

// triple and 4tuple iteration tests
bool d_tests_dmacro_for_each_triple(
    struct dj_test_suite* _suite
);

bool d_tests_dmacro_for_each_4tuple(
    struct dj_test_suite* _suite
);

// indexed iteration tests
bool d_tests_dmacro_for_each_indexed(
    struct dj_test_suite* _suite
);

// pair/indexed module aggregator
bool d_tests_dmacro_pair_indexed_all(
    struct dj_test_suite* _suite
);

// XI.   Member access iteration test functions
// pointer member access tests
bool d_tests_dmacro_for_each_member_ptr(
    struct dj_test_suite* _suite
);

// direct member access tests
bool d_tests_dmacro_for_each_member_dot(
    struct dj_test_suite* _suite
);

// generic operator access tests
bool d_tests_dmacro_for_each_op(
    struct dj_test_suite* _suite
);

// member access module aggregator
bool d_tests_dmacro_member_access_all(
    struct dj_test_suite* _suite
);

// XII.  Advanced iteration patterns test functions
// adjacent pair tests
bool d_tests_dmacro_adjacent_pair(
    struct dj_test_suite* _suite
);

// advanced iteration module aggregator
bool d_tests_dmacro_advanced_iter_all(
    struct dj_test_suite* _suite
);

// XIII. Pointer array initialization test functions
// data-comma iteration tests
bool d_tests_dmacro_for_each_data_comma(
    struct dj_test_suite* _suite
);

// struct array init tests
bool d_tests_dmacro_struct_array_init(
    struct dj_test_suite* _suite
);

// pointer array module aggregator
bool d_tests_dmacro_ptr_array_all(
    struct dj_test_suite* _suite
);

// XIV.  Utility operators test functions
// debug/test operator tests
bool d_tests_dmacro_utility_ops(
    struct dj_test_suite* _suite
);

// utility operators module aggregator
bool d_tests_dmacro_utility_all(
    struct dj_test_suite* _suite
);

// XV.   Compile-time assertions test functions
// size/type check tests
bool d_tests_dmacro_assert_same_size(
    struct dj_test_suite* _suite
);

// compile-time assertions module aggregator
bool d_tests_dmacro_static_assert_all(
    struct dj_test_suite* _suite
);

// XVI.  Master test suite
bool d_tests_dmacro_run_all(struct dj_test_suite* _suite);

bool d_tests_dmacro_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_DMACRO_TESTS_H
