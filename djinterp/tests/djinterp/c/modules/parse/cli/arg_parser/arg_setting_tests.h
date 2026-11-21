/*******************************************************************************
* djinterp [c/parse]                                         arg_setting_tests.h
*
* The suite of the parser's setting hook (d_arg_parser_set_setting_hook):
* which long options it is offered, and which never; the arguments it sees
* and where the offered one sits among them; what it may take, by count or
* by moving `*_at`; and a refusal, as a status, an exit code and the
* messages written for it.
*   The hook is a stand-in for the settings kit's: it takes `--NAME=VALUE`
* where NAME is a dotted setting it knows, and `--set NAME=VALUE` as two
* arguments; refuses a dotted name it does not know; and leaves everything
* else to the parse. Each call it gets is written as a line of the
* transcript, before the report of the parse. The first section is the ten
* cases the kit's probe gave: four its own walk got wrong, five it got
* right, and one where the hook comes before an abbreviation. CLI11 has no
* such hook, so every transcript here is the module's own.
*
*
* path:      /tests/djinterp/c/modules/parse/cli/arg_parser/arg_setting_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SETTING_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SETTING_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../tests_section.h"  // dj_tests_section


// 1.  THE PROBE'S CASES
bool d_tests_arg_setting_probe(void);

// 2.  WHEN IT IS OFFERED
bool d_tests_arg_setting_offered(void);
bool d_tests_arg_setting_offered_place(void);

// 3.  WHAT IT SEES
bool d_tests_arg_setting_sees(void);

// 4.  WHAT IT TAKES
bool d_tests_arg_setting_takes(void);

// 5.  REFUSALS
bool d_tests_arg_setting_refusals(void);

// the suite's sections, in the order they run
extern const struct dj_tests_section d_tests_arg_setting[];
extern const size_t                 d_tests_arg_setting_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SETTING_TESTS_H
