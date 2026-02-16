/******************************************************************************
* djinterp [test]                                         test_common_tests_sa.h
*
*   Unit test declarations for `test_common.h` module.
*   Provides comprehensive testing of all test_common types, macros, and
* structures including macro definitions, type definitions, argument structures,
* test function wrappers, lifecycle stages, and type discriminators.
*
*
* path:      \tests\test\test_common_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.03
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_COMMON_SA_
#define DJINTERP_TESTS_TEST_COMMON_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\inc\c\test\test_common.h"
#include "..\..\..\inc\c\string_fn.h"


/******************************************************************************
 * I. MACRO DEFINITION TESTS
 *****************************************************************************/
// keyword macros
bool d_tests_sa_test_common_keyword_macros(struct d_test_counter* _counter);
// pass/fail macros
bool d_tests_sa_test_common_pass_fail_macros(struct d_test_counter* _counter);
// emoji/symbol macros
bool d_tests_sa_test_common_symbol_macros(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_test_common_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. TYPE DEFINITION TESTS
 *****************************************************************************/
// d_test_id type
bool d_tests_sa_test_common_test_id_type(struct d_test_counter* _counter);
// fn_test function pointer type
bool d_tests_sa_test_common_fn_test_type(struct d_test_counter* _counter);
// fn_stage function pointer type
bool d_tests_sa_test_common_fn_stage_type(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_test_common_type_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ARGUMENT STRUCTURE TESTS
 *****************************************************************************/
// d_test_arg structure
bool d_tests_sa_test_common_test_arg(struct d_test_counter* _counter);
// d_test_arg_list structure
bool d_tests_sa_test_common_test_arg_list(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_test_common_arg_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. TEST FUNCTION WRAPPER TESTS
 *****************************************************************************/
// d_test_fn structure
bool d_tests_sa_test_common_test_fn(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_test_common_fn_wrapper_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. LIFECYCLE STAGE TESTS
 *****************************************************************************/
// DTestStage enumeration
bool d_tests_sa_test_common_test_stage_enum(struct d_test_counter* _counter);
// DTestStage value validation
bool d_tests_sa_test_common_test_stage_values(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_test_common_lifecycle_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. TYPE DISCRIMINATOR TESTS
 *****************************************************************************/
// DTestTypeFlag enumeration
bool d_tests_sa_test_common_type_flag_enum(struct d_test_counter* _counter);
// DTestTypeFlag value validation
bool d_tests_sa_test_common_type_flag_values(struct d_test_counter* _counter);
// DTestTypeFlag uniqueness
bool d_tests_sa_test_common_type_flag_uniqueness(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_test_common_discriminator_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_test_common_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_TEST_COMMON_SA_





