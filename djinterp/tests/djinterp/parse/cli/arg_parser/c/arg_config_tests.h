/*******************************************************************************
* djinterp [parse]                                            arg_config_tests.h
*
* The suite of configuration files (arg_config.c, arg_config_read.c,
* arg_config_write.c, arg_process_config.c): CLI11's ConfigBase reading TOML
* and INI under the settings each character, layer, section and switch
* takes; items made by hand, their full names and the text a flag takes;
* files read in the parse, for options, flags, subcommands and groups, from
* the command line, a default, the environment; text parsed as a file's;
* files read as text; configuration written in each mode, of values, names,
* subcommands, groups and formats; and, C's alone, the formats themselves,
* their tables of operations, files and sinks, what is refused, and text
* parsed as a file's given nothing to parse.
*   Every expected transcript is CLI11's, from the configuration suite's
* reference program, but the departures' -- DEPARTED_READS, DEPARTED_MOVED --
* the module's own where it departs from CLI11, and the last sections',
* which check what they hold in code.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_config_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_CONFIG_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_CONFIG_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 1.  VALUES
bool d_tests_arg_config_values(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 2.  SECTIONS
bool d_tests_arg_config_sections(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 3.  HEADERS
bool d_tests_arg_config_headers(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 4.  A SECTION CHOSEN
bool d_tests_arg_config_chosen(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 5.  LAYERS
bool d_tests_arg_config_layers(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 6.  NAMES
bool d_tests_arg_config_names(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 7.  MULTILINE STRINGS
bool d_tests_arg_config_multiline(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 8.  COMMENTS
bool d_tests_arg_config_comments(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 9.  ARRAYS
bool d_tests_arg_config_arrays(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 10. ESCAPES
bool d_tests_arg_config_escapes(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 11. DUPLICATES
bool d_tests_arg_config_duplicates(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 12. ODD LINES
bool d_tests_arg_config_odd_lines(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 13. DELIMITERS
bool d_tests_arg_config_delimiters(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 14. BYTES
bool d_tests_arg_config_bytes(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 15. A SECTION ENTERED
bool d_tests_arg_config_entered(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 16. SECTIONS AFTER SECTIONS
bool d_tests_arg_config_transitions(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 17. HEADERS AT THE EDGES
bool d_tests_arg_config_header_edges(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 18. MULTILINE STRINGS AT THE EDGES
bool d_tests_arg_config_multiline_edges(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 19. QUOTES AT THE EDGES
bool d_tests_arg_config_quote_edges(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 20. ARRAYS AND LISTS AT THE EDGES
bool d_tests_arg_config_list_edges(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 21. NAMES MERGED
bool d_tests_arg_config_merges(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 22. DEPARTED READS
bool d_tests_arg_config_departed_reads(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 23. ITEMS
bool d_tests_arg_config_items(void);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 24. FILES FOR OPTIONS
bool d_tests_arg_config_file_options(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 25. FILE VALUES
bool d_tests_arg_config_file_values(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 26. FILES FOR COMMANDS
bool d_tests_arg_config_file_commands(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 27. NAMES IN FILES
bool d_tests_arg_config_file_names(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 28. CONFIGURABLE SUBCOMMANDS
bool d_tests_arg_config_file_subcommands(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 29. EXTRAS IN FILES
bool d_tests_arg_config_file_extras(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 30. WHERE FILES COME FROM
bool d_tests_arg_config_file_sources(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

// 31. FILES FROM THE ENVIRONMENT
bool d_tests_arg_config_file_environment(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 32. FILES FOR FLAG-LIKE OPTIONS
bool d_tests_arg_config_file_flags(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 33. OPTIONS LOOKED FOR
bool d_tests_arg_config_file_lookups(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 34. SECTION MARKS AND CALLBACKS
bool d_tests_arg_config_file_marks(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 35. FILES NAMED
bool d_tests_arg_config_file_named(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

// 36. VARIABLES AND FILES
bool d_tests_arg_config_file_variables(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 37. FILES THAT CANNOT BE READ
bool d_tests_arg_config_file_permissions(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 38. TEXT PARSED AS A FILE
bool d_tests_arg_config_streams(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 39. FILES READ AS TEXT
bool d_tests_arg_config_text_mode(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 40. WRITING VALUES
bool d_tests_arg_config_write_basics(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 41. WRITING TEXT
bool d_tests_arg_config_write_text(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 42. LONG VALUES
bool d_tests_arg_config_write_long(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 43. LONG VALUES WITHOUT NEWLINES
bool d_tests_arg_config_write_longer(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 44. WRITING POLICIES
bool d_tests_arg_config_write_policies(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 45. WRITING SUBCOMMANDS
bool d_tests_arg_config_write_commands(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 46. WRITING GROUPS
bool d_tests_arg_config_write_groups(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 47. WRITING IN OTHER FORMATS
bool d_tests_arg_config_write_formats(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 48. A PREFIX
bool d_tests_arg_config_write_prefix(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 49. VALUES AT THE EDGES
bool d_tests_arg_config_write_values(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 50. QUOTING AT THE EDGES
bool d_tests_arg_config_write_quoting(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 51. THE TRIPLE QUOTE IN LONG VALUES
bool d_tests_arg_config_write_triples(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 52. NAMES AT THE EDGES
bool d_tests_arg_config_write_names(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 53. FLAGS READ BACK
bool d_tests_arg_config_write_flags(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 54. VALUES REVERSED
bool d_tests_arg_config_write_orders(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 55. SUMS CHECKED AGAIN
bool d_tests_arg_config_write_sums(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 56. JOINS AT THE EDGES
bool d_tests_arg_config_write_joins(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 57. DEFAULTS OF `{}`
bool d_tests_arg_config_write_defaults(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 58. GROUPS AND PREFIXES AT THE EDGES
bool d_tests_arg_config_write_layout(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 59. DEPARTED: MOVED OPTIONS
bool d_tests_arg_config_departed_moved(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 60. FORMATS
bool d_tests_arg_config_formats(void);

// 61. BUILDING ITEMS
bool d_tests_arg_config_building(void);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 62. TABLES OF OPERATIONS
bool d_tests_arg_config_tables(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 63. READING FILES
bool d_tests_arg_config_files(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 64. SINKS
bool d_tests_arg_config_sinks(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 65. CLI11'S OWN, CALLED DIRECTLY
bool d_tests_arg_config_defaults(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 66. TEXT PARSED AS A FILE, GIVEN NOTHING
bool d_tests_arg_config_stream_nulls(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 0)

// 67. WITHOUT CONFIGURATION FILES
bool d_tests_arg_config_unbuilt(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_config[];
extern const size_t                 d_tests_arg_config_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_CONFIG_TESTS_H
