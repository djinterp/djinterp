/*******************************************************************************
* djinterp [parse]                                              arg_argv_tests.h
*
* The suite of arguments from text (arg_argv.c, arg_argv_source.c): lines
* split by each set of rules -- a POSIX shell's, the C runtime's, CLI11's,
* GCC's -- under each switch; words quoted to survive a split; program names
* found on disk; response files read, and expanded in a parse; and wide
* text converted both ways, and parsed.
*   Every expected transcript is a reference's -- dash and bash, the
* Universal C Runtime's rules and LLVM's, CLI11's, libiberty's -- from the
* argv suite's reference program, but the module's own: the lines no
* reference reads so, the quoted words, which the references split back,
* and the wide text, which Python's codecs convert.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_argv_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.17
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_ARGV_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_ARGV_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  SPLITTING: POSIX
bool d_tests_arg_argv_split_posix(void);

// 2.  SPLITTING: WINDOWS
bool d_tests_arg_argv_split_windows(void);

// 3.  SPLITTING: WINDOWS, LINE BREAKS
bool d_tests_arg_argv_split_windows_breaks(void);

// 4.  SPLITTING: WINDOWS, PROGRAM NAMES
bool d_tests_arg_argv_split_windows_names(void);

// 5.  SPLITTING: WINDOWS, PROGRAM NAMES AND LINE BREAKS
bool d_tests_arg_argv_split_windows_names_breaks(void);

// 6.  SPLITTING: CLI11
bool d_tests_arg_argv_split_cli11(void);

// 7.  SPLITTING: CLI11, WINDOWS STYLE
bool d_tests_arg_argv_split_cli11_windows(void);

// 8.  SPLITTING: GNU
bool d_tests_arg_argv_split_gnu(void);

// 9.  SPLITTING: SWITCHES IGNORED
bool d_tests_arg_argv_split_flags(void);

// 10. SPLITTING: THE MODULE'S OWN
bool d_tests_arg_argv_split_own(void);

// 11. SPLITTING: CALLS REFUSED AND KEPT
bool d_tests_arg_argv_split_refusals(void);

// 12. QUOTING: POSIX
bool d_tests_arg_argv_quote_posix(void);

// 13. QUOTING: WINDOWS
bool d_tests_arg_argv_quote_windows(void);

// 14. QUOTING: CLI11
bool d_tests_arg_argv_quote_cli11(void);

// 15. QUOTING: GNU
bool d_tests_arg_argv_quote_gnu(void);

// 16. QUOTING: CALLS REFUSED AND KEPT
bool d_tests_arg_argv_quote_refusals(void);

// 17. PROGRAM NAMES
bool d_tests_arg_argv_program_names(void);

// 18. PROGRAM NAMES: CALLS REFUSED AND KEPT
bool d_tests_arg_argv_program_refusals(void);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 19. RESPONSE FILES: READING
bool d_tests_arg_argv_response_read(void);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 20. RESPONSE FILES: GCC'S
bool d_tests_arg_argv_response_expand_gnu(void);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 21. RESPONSE FILES: WINDOWS'
bool d_tests_arg_argv_response_expand_windows(void);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 22. RESPONSE FILES: THE MODULE'S OWN
bool d_tests_arg_argv_response_expand_own(void);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 23. RESPONSE FILES: THE OTHER PARSES
bool d_tests_arg_argv_response_expand_entries(void);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 24. RESPONSE FILES: CALLS REFUSED AND KEPT
bool d_tests_arg_argv_response_refusals(void);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 25. WIDE TEXT
bool d_tests_arg_argv_wide_text(void);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 26. WIDE TEXT FROM UTF-8
bool d_tests_arg_argv_wide_back(void);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 27. WIDE ARGUMENTS
bool d_tests_arg_argv_wide_parse(void);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_text[];
extern const size_t                 d_tests_arg_text_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_ARGV_TESTS_H
