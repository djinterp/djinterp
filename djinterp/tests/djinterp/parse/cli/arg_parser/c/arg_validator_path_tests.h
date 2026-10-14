/*******************************************************************************
* djinterp [parse]                                    arg_validator_path_tests.h
*
* The suite of the validators that ask the file system
* (arg_validator_path.h): CLI11's existing-file, -directory and -path checks,
* the nonexistent-path check, the file found on a default path, the
* permission and size checks, the names whose answer is the system's own --
* symbolic links, FIFOs and modes on POSIX, separators and case on Windows
* -- a name that is UTF-8, what cannot be made, what each completes as, and
* the checks on options inside a parse.
*   Each section makes the files it asks about in a scratch directory
* (arg_tests_files.h), which it removes when it is done.
*   Everything here is compiled only with the path validators
* (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS); without them there is no suite,
* and the runner runs none.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_path_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_PATH_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_PATH_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 1.  EXISTENCE AND SEARCH
bool d_tests_arg_path_existence(void);
bool d_tests_arg_path_search(void);

// 2.  PERMISSIONS AND SIZE
bool d_tests_arg_path_permissions(void);
bool d_tests_arg_path_size(void);

// 3.  NAMES
bool d_tests_arg_path_system(void);
bool d_tests_arg_path_utf8(void);

// 4.  CONSTRUCTION AND COMPLETION
bool d_tests_arg_path_construction(void);

// 5.  IN A PARSE
bool d_tests_arg_path_in_a_parse(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_path[];
extern const size_t                 d_tests_arg_path_count;

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_PATH_TESTS_H
