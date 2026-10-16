/******************************************************************************
* djinterp [test]                                           test_cli_unix_sa.h
*
*   Unit test declarations for `cli_unix.h` module. Tests Unix CLI
* convention parsing: long options (--key, --key=val), short flags (-v),
* combined short flags (-xvf), next-arg consumption, stop token (--),
* and edge cases.
*
*
* path:      /tests/util/test_cli_unix_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.23
******************************************************************************/

#ifndef DJINTERP_TESTS_CLI_UNIX_SA_
#define DJINTERP_TESTS_CLI_UNIX_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../../inc/c/test/test_standalone.h"
#include "../../../inc/c/util/cli_unix.h"


/******************************************************************************
 * I. LONG OPTION TESTS
 *****************************************************************************/
// --key basic matching
bool d_tests_sa_test_cli_unix_long_option(struct d_test_counter* _counter);
// --key=val embedded value
bool d_tests_sa_test_cli_unix_long_equals(struct d_test_counter* _counter);


/******************************************************************************
 * II. SHORT FLAG TESTS
 *****************************************************************************/
// -v single short flag
bool d_tests_sa_test_cli_unix_short_single(struct d_test_counter* _counter);
// -xvf combined short flags
bool d_tests_sa_test_cli_unix_short_combined(struct d_test_counter* _counter);


/******************************************************************************
 * III. NEXT-ARG CONSUMPTION TESTS
 *****************************************************************************/
// -o file, --output file, -vo file
bool d_tests_sa_test_cli_unix_consume_next(struct d_test_counter* _counter);


/******************************************************************************
 * IV. STOP TOKEN TESTS
 *****************************************************************************/
// -- stops processing
bool d_tests_sa_test_cli_unix_stop_token(struct d_test_counter* _counter);


/******************************************************************************
 * V. EDGE CASE TESTS
 *****************************************************************************/
// bare dash, positional args, handler stop, NULL handler, context, macro
bool d_tests_sa_test_cli_unix_edge_cases(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_test_cli_unix_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_CLI_UNIX_SA_
