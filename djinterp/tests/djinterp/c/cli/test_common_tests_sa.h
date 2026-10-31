/******************************************************************************
* djinterp [test]                                         test_common_tests_sa.h
*
*   Unit test declarations for `test_common.h` and `cli_option.h` modules.
*   Provides comprehensive testing of all test_common types, macros, and
* structures including macro definitions, type definitions, argument structures,
* test function wrappers, lifecycle stages, type discriminators, and CLI
* option infrastructure.
*
*
* path:      /tests/djinterp/c/cli/test_common_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.03
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_COMMON_SA_
#define DJINTERP_TESTS_TEST_COMMON_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_standalone.h"
#include "../../../../inc/djinterp/c/modules/test/test_common.h"
#include "../../../../inc/djinterp/c/cli/cli_option.h"
#include "../../../../inc/djinterp/c/string_fn.h"


/******************************************************************************
 * I. MACRO DEFINITION TESTS
 *****************************************************************************/
bool d_tests_sa_test_common_keyword_macros(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_pass_fail_macros(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_symbol_macros(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_macro_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. TYPE DEFINITION TESTS
 *****************************************************************************/
bool d_tests_sa_test_common_test_id_type(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_fn_test_type(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_fn_stage_type(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_type_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. ARGUMENT STRUCTURE TESTS
 *****************************************************************************/
bool d_tests_sa_test_common_test_arg(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_test_arg_list(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_arg_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. TEST FUNCTION WRAPPER TESTS
 *****************************************************************************/
bool d_tests_sa_test_common_test_fn(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_fn_wrapper_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. LIFECYCLE STAGE TESTS
 *****************************************************************************/
bool d_tests_sa_test_common_test_stage_enum(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_test_stage_values(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_lifecycle_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. TYPE DISCRIMINATOR TESTS
 *****************************************************************************/
bool d_tests_sa_test_common_type_flag_enum(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_type_flag_values(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_type_flag_uniqueness(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_common_discriminator_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. CLI OPTION TESTS
 *****************************************************************************/
bool d_tests_sa_test_cli_string_entry(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_cli_find_by_field(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_cli_find_macro(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_cli_find_with(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_cli_parse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_cli_table_macros(
    struct dj_test_counter* _counter
);
bool d_tests_sa_test_cli_option_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_test_common_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_TEST_COMMON_SA_
