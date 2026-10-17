/*******************************************************************************
* djinterp [parse]                                         arg_formatter_tests.h
*
* The suite of help (arg_formatter.c, arg_formatter_command.c,
* arg_formatter_option.c): what CLI11's default Formatter writes of a root,
* its options, positionals, subcommands and option groups; labels, widths,
* the long names' alignment, paragraphs, requirements, expanded help and a
* subcommand's, help after a parse, fallthrough, usage and footer text, the
* special options; steps of the caller's in place of CLI11's; and, C's
* alone, the line width, the terminal's, what help refuses, and the
* formatter's own settings.
*   Every expected transcript is CLI11's, from the formatter suite's
* reference program, but LINE_WIDTH's, the module's own where CLI11 has no
* counterpart; the last sections check what they hold in code.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_formatter_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_FORMATTER_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_FORMATTER_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  BASICS
bool d_tests_arg_formatter_basics(void);

// 2.  OPTION TEXT
bool d_tests_arg_formatter_option_text(void);

// 3.  LABELS
bool d_tests_arg_formatter_labels(void);

// 4.  WIDTHS
bool d_tests_arg_formatter_widths(void);

// 5.  ALIGNMENT
bool d_tests_arg_formatter_alignment(void);

// 6.  PARAGRAPHS
bool d_tests_arg_formatter_paragraphs(void);

// 7.  REQUIREMENTS
bool d_tests_arg_formatter_requirements(void);

// 8.  SUBCOMMANDS
bool d_tests_arg_formatter_subcommands(void);

// 9.  EXPANDED
bool d_tests_arg_formatter_expanded(void);

// 10. OPTION GROUPS
bool d_tests_arg_formatter_option_groups(void);

// 11. PARSED
bool d_tests_arg_formatter_parsed(void);

// 12. FALLTHROUGH
bool d_tests_arg_formatter_fallthrough(void);

// 13. USAGE AND FOOTER
bool d_tests_arg_formatter_usage_and_footer(void);

// 14. SPECIAL OPTIONS
bool d_tests_arg_formatter_special_options(void);

// 15. REPLACED STEPS
bool d_tests_arg_formatter_replaced(void);

// 16. LINE WIDTH
bool d_tests_arg_formatter_line_width(void);

// 17. FITTING
bool d_tests_arg_formatter_fitting(void);

#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)

// 18. TERMINAL
bool d_tests_arg_formatter_terminal(void);

#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH

// 19. REFUSALS
bool d_tests_arg_formatter_refusals(void);

// 20. SETTINGS
bool d_tests_arg_formatter_settings(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_formatter[];
extern const size_t                 d_tests_arg_formatter_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_FORMATTER_TESTS_H
