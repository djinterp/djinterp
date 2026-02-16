/******************************************************************************
* djinterp [test]                                           dmacro_tests_sa.h
*
*   This is a test file for `dmacro.h` unit tests.
*   For the file itself, go to `\inc\c\dmacro.h`.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest
* for unit testing. Any modules that are not dependencies of DTest should use
* DTest for unit tests.
*
* path:      \test\dmacro_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

/*
TABLE OF CONTENTS
==============================================
I.    Configuration system test functions
II.   Token manipulation test functions
III.  Array utilities test functions
IV.   Argument counting test functions
V.    Macro expansion and evaluation test functions
VI.   Boolean and conditional logic test functions
VII.  Argument selection test functions
VIII. Core iteration infrastructure test functions
IX.   For_each implementations test functions
X.    Pair and indexed iteration test functions
XI.   Member access iteration test functions
XII.  Advanced iteration patterns test functions
XIII. Pointer array initialization test functions
XIV.  Utility operators test functions
XV.   Compile-time assertions test functions
XVI.  Master test suite
*/

#ifndef DJINTERP_TESTING_DMACRO_STANDALONE
#define DJINTERP_TESTING_DMACRO_STANDALONE 1

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\dmacro.h"
#include "..\..\inc\c\string_fn.h"
#include "..\..\inc\c\test\test_standalone.h"


// I. Configuration system test functions
// configuration constant tests
bool  d_tests_sa_dmacro_cfg_constants(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_cfg_user_options(struct d_test_counter* _test_info);
// effective value tests
bool  d_tests_sa_dmacro_cfg_effective_values(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_cfg_public_aliases(struct d_test_counter* _test_info);
// query macro tests
bool  d_tests_sa_dmacro_cfg_query_macros(struct d_test_counter* _test_info);
// environment integration tests
bool  d_tests_sa_dmacro_cfg_env_integration(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_cfg_variant_consistency(struct d_test_counter* _test_info);
// configuration module aggregator
bool  d_tests_sa_dmacro_cfg_all(struct d_test_counter* _test_info);

// II.   Token manipulation test functions
// token pasting tests (D_CONCAT, D_INTERNAL_CONCAT_HELPER)
bool  d_tests_sa_dmacro_concat_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_concat_with_macros(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_concat_edge_cases(struct d_test_counter* _test_info);
// stringification tests (D_STRINGIFY, D_TOSTR)
bool  d_tests_sa_dmacro_stringify_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_stringify_vs_tostr(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_stringify_edge_cases(struct d_test_counter* _test_info);
// expansion control tests (D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT, D_UNPACK)
bool  d_tests_sa_dmacro_expand_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_empty_macro(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_defer_macro(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_obstruct_macro(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_unpack_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_unpack_nested(struct d_test_counter* _test_info);
// token manipulation module aggregator
bool  d_tests_sa_dmacro_token_all(struct d_test_counter* _test_info);

// III.  Array utilities test functions
// compile-time array sizing tests
bool  d_tests_sa_dmacro_array_count_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_array_count_initialized(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_array_count_structs(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_array_count_safe(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_array_count_t(struct d_test_counter* _test_info);
// array generation tests
bool  d_tests_sa_dmacro_make_array(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_make_string_array(struct d_test_counter* _test_info);
// array utilities module aggregator
bool  d_tests_sa_dmacro_array_all(struct d_test_counter* _test_info);

// IV.   Argument counting test functions
// D_VARG_COUNT tests
bool  d_tests_sa_dmacro_varg_count_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_varg_count_medium(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_varg_count_large(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_varg_count_types(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_varg_count_edge_cases(struct d_test_counter* _test_info);
// D_HAS_ARGS tests
bool  d_tests_sa_dmacro_has_args_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_has_args_types(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_has_args_large_counts(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_has_args_conditionals(struct d_test_counter* _test_info);
// combined and boundary tests
bool  d_tests_sa_dmacro_arg_count_combined(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_arg_count_boundary(struct d_test_counter* _test_info);
// argument counting module aggregator
bool  d_tests_sa_dmacro_arg_count_all(struct d_test_counter* _test_info);

// V.    Macro expansion and evaluation test functions
// D_INC tests
bool  d_tests_sa_dmacro_inc_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_inc_medium(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_inc_large(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_inc_chained(struct d_test_counter* _test_info);
// D_EVAL tests
bool  d_tests_sa_dmacro_eval_basic(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_eval_nested(struct d_test_counter* _test_info);
// iteration macro tests (MSVC-compatible, use D_STRINGIFY only)
bool  d_tests_sa_dmacro_for_each_stringify(struct d_test_counter* _test_info);
// practical patterns test
bool  d_tests_sa_dmacro_eval_practical(struct d_test_counter* _test_info);
// macro expansion and evaluation module aggregator
bool  d_tests_sa_dmacro_eval_all(struct d_test_counter* _test_info);

// VI.   Boolean and conditional logic test functions
// probe mechanism tests
bool  d_tests_sa_dmacro_probe_check(struct d_test_counter* _test_info);
// parentheses detection tests
bool  d_tests_sa_dmacro_is_paren(struct d_test_counter* _test_info);
// conditional expansion tests
bool  d_tests_sa_dmacro_if_macros(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_iif_macros(struct d_test_counter* _test_info);
// boolean logic tests
bool  d_tests_sa_dmacro_not_compl(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_bool_macro(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_and_or(struct d_test_counter* _test_info);
// boolean logic module aggregator
bool  d_tests_sa_dmacro_boolean_all(struct d_test_counter* _test_info);

// VII.  Argument selection test functions
// convenience alias tests
bool  d_tests_sa_dmacro_first_second_third(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_head_rest_tail(struct d_test_counter* _test_info);
// parentheses handling tests
bool  d_tests_sa_dmacro_remove_parentheses(struct d_test_counter* _test_info);
// argument selection module aggregator
bool  d_tests_sa_dmacro_selection_all(struct d_test_counter* _test_info);

// VIII. Core iteration infrastructure test functions
// map termination tests
bool  d_tests_sa_dmacro_map_end_detection(struct d_test_counter* _test_info);
// core mapping tests
bool  d_tests_sa_dmacro_map_internals(struct d_test_counter* _test_info);
// iteration infrastructure module aggregator
bool  d_tests_sa_dmacro_iteration_core_all(struct d_test_counter* _test_info);

// IX.   For_each implementations test functions
// basic iteration tests
bool  d_tests_sa_dmacro_for_each_basic(struct d_test_counter* _test_info);
// separated iteration tests
bool  d_tests_sa_dmacro_for_each_sep(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_for_each_comma(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_for_each_semicolon(struct d_test_counter* _test_info);
// data-passing iteration tests
bool  d_tests_sa_dmacro_for_each_data(struct d_test_counter* _test_info);
// for_each module aggregator
bool  d_tests_sa_dmacro_for_each_all(struct d_test_counter* _test_info);

// X.    Pair and indexed iteration test functions
// pair iteration tests
bool  d_tests_sa_dmacro_for_each_pair(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_for_each_pair_sep(struct d_test_counter* _test_info);
// triple and 4tuple iteration tests
bool  d_tests_sa_dmacro_for_each_triple(struct d_test_counter* _test_info);
bool  d_tests_sa_dmacro_for_each_4tuple(struct d_test_counter* _test_info);
// indexed iteration tests
bool  d_tests_sa_dmacro_for_each_indexed(struct d_test_counter* _test_info);
// pair/indexed module aggregator
bool  d_tests_sa_dmacro_pair_indexed_all(struct d_test_counter* _test_info);

// XI.   Member access iteration test functions
// pointer member access tests
bool  d_tests_sa_dmacro_for_each_member_ptr(struct d_test_counter* _test_info);
// direct member access tests
bool  d_tests_sa_dmacro_for_each_member_dot(struct d_test_counter* _test_info);
// generic operator access tests
bool  d_tests_sa_dmacro_for_each_op(struct d_test_counter* _test_info);
// member access module aggregator
bool  d_tests_sa_dmacro_member_access_all(struct d_test_counter* _test_info);

// XII.  Advanced iteration patterns test functions
// adjacent pair tests
bool  d_tests_sa_dmacro_adjacent_pair(struct d_test_counter* _test_info);
// advanced iteration module aggregator
bool  d_tests_sa_dmacro_advanced_iter_all(struct d_test_counter* _test_info);

// XIII. Pointer array initialization test functions
// data-comma iteration tests
bool  d_tests_sa_dmacro_for_each_data_comma(struct d_test_counter* _test_info);
// struct array init tests
bool  d_tests_sa_dmacro_struct_array_init(struct d_test_counter* _test_info);
// pointer array module aggregator
bool  d_tests_sa_dmacro_ptr_array_all(struct d_test_counter* _test_info);

// XIV.  Utility operators test functions
// debug/test operator tests
bool  d_tests_sa_dmacro_utility_ops(struct d_test_counter* _test_info);
// utility operators module aggregator
bool  d_tests_sa_dmacro_utility_all(struct d_test_counter* _test_info);

// XV.   Compile-time assertions test functions
// size/type check tests
bool  d_tests_sa_dmacro_assert_same_size(struct d_test_counter* _test_info);
// compile-time assertions module aggregator
bool  d_tests_sa_dmacro_static_assert_all(struct d_test_counter* _test_info);

// XVI.  Master test suite
struct d_test_object* d_tests_dmacro_run_all(void);
bool                  d_tests_sa_dmacro_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_DMACRO_STANDALONE




