/*******************************************************************************
* djinterp [test]                                           test_cli_option_sa.h
*
*   Unit test declarations for `cli_option.h` module. Tests convention-
* agnostic string-to-void* lookup and CLI argument parsing including
* d_string_cli_entry lookup, generic field-based search, custom lookup
* delegation, argv parsing, and table macros.
*
*
* path:      /tests/djinterp/c/cli/test_cli_option_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CLI_OPTION_STANDALONE_
#define DJINTERP_TESTS_CLI_OPTION_STANDALONE_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../inc/djinterp/c/cli/cli_option.h"
#include "../../../../inc/djinterp/c/string_fn.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


bool d_tests_sa_test_cli_string_entry(struct d_test_counter* _counter);
bool d_tests_sa_test_cli_find_by_field(struct d_test_counter* _counter);
bool d_tests_sa_test_cli_find_macro(struct d_test_counter* _counter);
bool d_tests_sa_test_cli_find_with(struct d_test_counter* _counter);
bool d_tests_sa_test_cli_parse(struct d_test_counter* _counter);
bool d_tests_sa_test_cli_table_macros(struct d_test_counter* _counter);

bool d_tests_sa_test_cli_option_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_CLI_OPTION_STANDALONE_
