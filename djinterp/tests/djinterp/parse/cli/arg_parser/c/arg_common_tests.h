/*******************************************************************************
* djinterp [parse]                                            arg_common_tests.h
*
* The suite of the argument parser's shared vocabulary (arg_common.h).
*   Statuses, their names and the exit codes CLI11 gives them; the byte
* buffer every message and help text is built in; the sinks output goes
* through; the error report a parser keeps; and the lists the records keep
* their parts in. Every section works on storage it declares, so each holds
* in every configuration.
*   The suite is a table of sections, defined in arg_common_tests.c beside
* the sections themselves, so its length is the number of sections this
* build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_common_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMON_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMON_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  STATUS
bool d_tests_arg_common_status_names(void);
bool d_tests_arg_common_status_exit_codes(void);
bool d_tests_arg_common_status_is_error(void);

// 2.  VOCABULARY
bool d_tests_arg_common_vocabulary(void);

// 4.1  Buffers
bool d_tests_arg_common_buffer_empty(void);
bool d_tests_arg_common_buffer_append(void);
bool d_tests_arg_common_buffer_append_refused(void);
bool d_tests_arg_common_buffer_printf(void);
bool d_tests_arg_common_buffer_truncate_clear(void);
bool d_tests_arg_common_buffer_detach(void);
bool d_tests_arg_common_buffer_sink(void);

// 4.2  Sinks
bool d_tests_arg_common_sink_no_stream(void);
bool d_tests_arg_common_sink_file(void);
bool d_tests_arg_common_sink_printf(void);

// 4.3  Errors
bool d_tests_arg_common_error_empty(void);
bool d_tests_arg_common_error_set(void);
bool d_tests_arg_common_error_setf(void);
bool d_tests_arg_common_error_defaults(void);

// 4.4  Lists
bool d_tests_arg_common_string_list(void);
bool d_tests_arg_common_lists_zeroed(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_common[];
extern const size_t                 d_tests_arg_common_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMON_TESTS_H
