/*******************************************************************************
* djinterp [parse]                                            cli_format_tests.c
*
* The suite of the command-line kit's help, declared in cli_format_tests.h.
*   LAYOUT is a program whose definitions reach every part of the page: on
* the root, an option with a default, one whose names reach past the right
* column, a short name alone with its value, a flag whose value may be left
* out and one whose value is a negation, a hidden option, options in groups
* -- "Output", and "OPTIONS", CLI11's name for the default group, which the
* page lists with the options in none -- an option group, a description
* with a line break of its own and one in UTF-8, and a footer long enough
* to wrap; and commands in a group, in none, with an alias, with a required
* operand and an optional one, with a usage of its own, with work and
* subcommands, with subcommands and no work, hidden, and disabled. Its pages
* are compared whole, at 80 columns and at 40 and 100.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_format_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_format_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcmp, memset
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // d_arg_format_help
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/parse/cli/c/cli_format.h"  // the kit's formatter
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // d_cli_is_terminal
#include "../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // the program
#include "../../c/tests_section.h"  // d_tests_section
#include "./cli_tests_support.h"  // cases, checks


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_COUNT
//   macro: how many entries a table has.
#define D_TESTS_CLI_COUNT(_table)  (sizeof(_table) / sizeof((_table)[0]))

/**
 * @brief A command's work that does nothing: D_CLI_EXIT_OK.
 *
 * @param[in] _program  unused.
 * @param[in] _command  unused.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static int
d_tests_cli_work_none(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_program;
    (void)_command;
    (void)_context;

    return D_CLI_EXIT_OK;
}

/**
 * @brief Adds an option and sets its type name, where it is given one.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        its names.
 * @param[in]     _description  what it does.
 * @param[in]     _type         its type name, or NULL for none.
 * @return the option, or NULL when it could not be made.
 */
static struct d_arg_option*
d_tests_cli_option(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description,
    const char*           _type
)
{
    struct d_arg_option* const option = d_arg_command_add_option(
                                            _command,
                                            _names,
                                            _description);

    // the type name, where there is one to set
    if ( (option != NULL) &&
         (_type != NULL)  &&
         (d_arg_option_set_type_name(option,
                                     _type) != D_ARG_OK) )
    {
        return NULL;
    }

    return option;
}

/**
 * @brief Adds a flag and sets its type name.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        its names, with the value it takes alone in
 *                              braces.
 * @param[in]     _description  what it does.
 * @param[in]     _type         its type name.
 * @return whether it was made.
 */
static bool
d_tests_cli_flag(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description,
    const char*           _type
)
{
    struct d_arg_option* const flag = d_arg_command_add_flag(_command,
                                                             _names,
                                                             _description);

    return ( (flag != NULL) &&
             (d_arg_option_set_type_name(flag,
                                         _type) == D_ARG_OK) );
}

/**
 * @brief LAYOUT's root options that sit in "Options".
 *
 * @param[in,out] _root  the root.
 * @return whether they were made.
 */
static bool
d_tests_cli_layout_plain(
    struct d_arg_command* _root
)
{
    struct d_arg_option* const out = d_tests_cli_option(
                                         _root,
                                         "-o,--out",
                                         "write the result to FILE",
                                         "FILE");

    return ( (out != NULL)                                              &&
             (d_arg_option_set_default_text(out,
                                            "standard output") ==
              D_ARG_OK)                                                 &&
             (d_tests_cli_option(_root,
                                 "--very-long-option-name",
                                 "an option whose names reach past the "
                                 "column, so its description starts on "
                                 "the line under them",
                                 NULL) != NULL)                         &&
             (d_tests_cli_option(_root,
                                 "-x",
                                 "a short name alone, and its value",
                                 NULL) != NULL)                         &&
             (d_tests_cli_flag(_root,
                               "--format{json}",
                               "the format: json alone, or FMT",
                               "FMT"))                                  &&
             (d_tests_cli_flag(_root,
                               "--cache{false}",
                               "a flag whose value is a negation",
                               "WHEN"))                                 &&
             (d_tests_cli_option(_root,
                                 "--mode",
                                 "the mode:\nfast, or slow",
                                 "MODE") != NULL)                       &&
             (d_tests_cli_option(_root,
                                 "--name",
                                 "a name such as café, naïve, ünïcödé or "
                                 "Øresund, in any script the terminal "
                                 "shows",
                                 "NAME") != NULL) );
}

/**
 * @brief LAYOUT's root options that sit elsewhere: hidden, in groups, and
 *        in an option group.
 *
 * @param[in,out] _root  the root.
 * @return whether they were made.
 */
static bool
d_tests_cli_layout_grouped(
    struct d_arg_command* _root
)
{
    struct d_arg_option* const  secret = d_arg_command_add_flag(
                                             _root,
                                             "--secret",
                                             "never shown");
    struct d_arg_option* const  width  = d_tests_cli_option(
                                             _root,
                                             "--width",
                                             "the page's width",
                                             "N");
    struct d_arg_option* const  depth  = d_tests_cli_option(
                                             _root,
                                             "--depth",
                                             "how deep to go",
                                             "N");
    struct d_arg_command* const tuning = d_arg_command_add_option_group(
                                             _root,
                                             "Tuning",
                                             "how hard to try");

    return ( (secret != NULL)                                    &&
             (d_arg_option_set_group(secret,
                                     "") == D_ARG_OK)            &&
             (width != NULL)                                     &&
             (d_arg_option_set_group(width,
                                     "Output") == D_ARG_OK)      &&
             (depth != NULL)                                     &&
             (d_arg_option_set_group(depth,
                                     "OPTIONS") == D_ARG_OK)     &&
             (tuning != NULL)                                    &&
             (d_tests_cli_option(tuning,
                                 "--effort",
                                 "how hard",
                                 "N") != NULL) );
}

/**
 * @brief Adds a command of LAYOUT with work that does nothing, and puts it
 *        in a group.
 *
 * @param[in,out] _program      the program.
 * @param[in,out] _parent       its parent; NULL for the root.
 * @param[in]     _name         its name.
 * @param[in]     _description  what it does.
 * @param[in]     _group        its group; NULL for none.
 * @return the command, or NULL when it could not be made.
 */
static struct d_arg_command*
d_tests_cli_layout_command(
    struct d_cli_program* _program,
    struct d_arg_command* _parent,
    const char*           _name,
    const char*           _description,
    const char*           _group
)
{
    struct d_arg_command* const command = d_cli_program_add_command(
                                              _program,
                                              _parent,
                                              _name,
                                              _description,
                                              d_tests_cli_work_none,
                                              NULL,
                                              NULL);

    // the group, where there is one
    if ( (command != NULL) &&
         (_group != NULL)  &&
         (d_arg_command_set_group(command,
                                  _group) != D_ARG_OK) )
    {
        return NULL;
    }

    return command;
}

/**
 * @brief LAYOUT's commands in a group, in none, with an alias, and with
 *        operands.
 *
 * @param[in,out] _program  the program.
 * @return whether they were made.
 */
static bool
d_tests_cli_layout_listed(
    struct d_cli_program* _program
)
{
    struct d_arg_command* const list = d_tests_cli_layout_command(
                                           _program,
                                           NULL,
                                           "list",
                                           "list the rows, or those a "
                                           "PATTERN matches",
                                           NULL);
    struct d_arg_command* const copy = d_tests_cli_layout_command(
                                           _program,
                                           NULL,
                                           "copy",
                                           "copy FROM to TO, or to "
                                           "standard output",
                                           NULL);
    struct d_arg_option* const  any  = (list != NULL)
                                       ? d_arg_command_add_option(
                                             list,
                                             "PATTERN",
                                             "a pattern rows must match")
                                       : NULL;
    struct d_arg_option* const  from = (copy != NULL)
                                       ? d_arg_command_add_option(
                                             copy,
                                             "FROM",
                                             "what to copy")
                                       : NULL;

    // made
    if ( (!any) ||
         (!from) )
    {
        return false;
    }

    d_arg_option_set_expected_range(any,
                                    0,
                                    -1);
    d_arg_option_set_required(from,
                              true);

    return ( (d_arg_command_add_alias(list,
                                      "ls") == D_ARG_OK)            &&
             (d_arg_command_add_option(copy,
                                       "TO",
                                       "where to copy it") != NULL) &&
             (d_tests_cli_layout_command(_program,
                                         NULL,
                                         "build",
                                         "make everything the sheet "
                                         "describes",
                                         "Making") != NULL)         &&
             (d_tests_cli_layout_command(_program,
                                         NULL,
                                         "clean",
                                         "remove what build made",
                                         "Making") != NULL) );
}

/**
 * @brief LAYOUT's commands that test the edges: a usage of its own, work
 *        and subcommands, subcommands alone, and disabled.
 *
 * @param[in,out] _program  the program.
 * @return whether they were made.
 */
static bool
d_tests_cli_layout_edges(
    struct d_cli_program* _program
)
{
    struct d_arg_command* const custom = d_tests_cli_layout_command(
                                             _program,
                                             NULL,
                                             "custom",
                                             "write a usage of its own",
                                             NULL);
    struct d_arg_command* const both   = d_tests_cli_layout_command(
                                             _program,
                                             NULL,
                                             "both",
                                             "work, and subcommands",
                                             NULL);
    struct d_arg_command* const tree   = d_cli_program_add_command(
                                             _program,
                                             NULL,
                                             "tree",
                                             "subcommands, and no work",
                                             NULL,
                                             NULL,
                                             NULL);
    struct d_arg_command* const old    = d_tests_cli_layout_command(
                                             _program,
                                             NULL,
                                             "old",
                                             "disabled, and not shown",
                                             NULL);

    // made
    if ( (!custom) ||
         (!both)   ||
         (!tree)   ||
         (!old) )
    {
        return false;
    }

    d_arg_command_set_disabled(old,
                               true);

    return ( (d_arg_command_set_usage(custom,
                                      "Usage: prog custom [MAGIC]") ==
              D_ARG_OK)                                               &&
             (d_tests_cli_layout_command(_program,
                                         both,
                                         "leaf",
                                         "a leaf",
                                         NULL) != NULL)               &&
             (d_tests_cli_layout_command(_program,
                                         tree,
                                         "branch",
                                         "a branch",
                                         NULL) != NULL) );
}

/**
 * @brief LAYOUT's last commands: one hidden, and one whose description
 *        wraps.
 *
 * @param[in,out] _program  the program.
 * @return whether they were made.
 */
static bool
d_tests_cli_layout_last(
    struct d_cli_program* _program
)
{
    return ( (d_tests_cli_layout_command(_program,
                                         NULL,
                                         "secret",
                                         "hidden, and not shown",
                                         "") != NULL)                 &&
             (d_tests_cli_layout_command(_program,
                                         NULL,
                                         "long",
                                         "a command whose description is "
                                         "long enough that one line of the "
                                         "page cannot hold it, so it is "
                                         "wrapped under its own column",
                                         NULL) != NULL) );
}

/**
 * @brief LAYOUT: the fixture whose pages reach every part of the layout.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static bool
d_tests_cli_make_layout(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    return ( (d_cli_program_init(_program,
                                 _parser,
                                 "prog",
                                 "1.2",
                                 D_CLI_PROGRAM_COMMANDS) == D_ARG_OK) &&
             (d_tests_cli_layout_plain(&_parser->command))            &&
             (d_tests_cli_layout_grouped(&_parser->command))          &&
             (d_tests_cli_layout_listed(_program))                    &&
             (d_tests_cli_layout_edges(_program))                     &&
             (d_tests_cli_layout_last(_program))                      &&
             (d_arg_command_set_footer(&_parser->command,
                                       "Report a bug with the sheet and "
                                       "the command line that show it, so "
                                       "that each can be run again; the "
                                       "issue tracker takes them.") ==
              D_ARG_OK) );
}

/**
 * @brief LAYOUT at 40 columns.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static bool
d_tests_cli_make_narrow(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    d_arg_formatter_set_line_width(&_parser->formatter,
                                   40u);

    return d_tests_cli_make_layout(_program,
                                   _parser);
}

/**
 * @brief LAYOUT at 100 columns.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static bool
d_tests_cli_make_wide(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    d_arg_formatter_set_line_width(&_parser->formatter,
                                   100u);

    return d_tests_cli_make_layout(_program,
                                   _parser);
}

// HELP_LIST
//   constant: `list`'s help, by its name or its alias.
static const char HELP_LIST[] =
    "Usage: prog list [OPTION]... [PATTERN]...\n"
    "List the rows, or those a PATTERN matches.\n"
    "\n"
    "Arguments:\n"
    "  PATTERN...          a pattern rows must match\n"
    "\n"
    "Options:\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Global options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
    "      --very-long-option-name=VALUE\n"
    "                      an option whose names reach past the column, so its\n"
    "                      description starts on the line under them\n"
    "  -x VALUE            a short name alone, and its value\n"
    "      --format[=FMT]  the format: json alone, or FMT\n"
    "      --cache         a flag whose value is a negation\n"
    "      --mode=MODE     the mode:\n"
    "                      fast, or slow\n"
    "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
    "                      script the terminal shows\n"
    "      --width=N       the page's width\n"
    "      --depth=N       how deep to go\n"
    "      --effort=N      how hard\n";

// BARE_OPTIONAL
//   constant: the page of a parser with no program and a subcommand it
// does not require: no exit statuses, and the parser's own help flag.
static const char BARE_OPTIONAL[] =
    "Usage: bare [OPTION]... [COMMAND [ARGUMENT]...]\n"
    "A parser without a program.\n"
    "\n"
    "Commands:\n"
    "  sub         a subcommand\n"
    "\n"
    "Options:\n"
    "  -h, --help  Print this help message and exit\n";

// BARE_REQUIRED
//   constant: the same parser's page when it requires a subcommand.
static const char BARE_REQUIRED[] =
    "Usage: bare [OPTION]... COMMAND [ARGUMENT]...\n"
    "A parser without a program.\n"
    "\n"
    "Commands:\n"
    "  sub         a subcommand\n"
    "\n"
    "Options:\n"
    "  -h, --help  Print this help message and exit\n";

// PAGES
//   constant: LAYOUT's pages at 80 columns.
static const struct d_tests_cli_case PAGES[] =
{
    { "P01", "prog --help", 0,
      "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
      "A probe of the kit.\n"
      "\n"
      "Commands:\n"
      "  list, ls            list the rows, or those a PATTERN matches\n"
      "  copy                copy FROM to TO, or to standard output\n"
      "  custom              write a usage of its own\n"
      "  both                work, and subcommands\n"
      "  tree                subcommands, and no work\n"
      "  long                a command whose description is long enough that one line\n"
      "                      of the page cannot hold it, so it is wrapped under its own\n"
      "                      column\n"
      "  help                display help for a command\n"
      "  completion          print a completion script for a shell\n"
      "\n"
      "Making:\n"
      "  build               make everything the sheet describes\n"
      "  clean               remove what build made\n"
      "\n"
      "Options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its\n"
      "                      description starts on the line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
      "                      script the terminal shows\n"
      "      --depth=N       how deep to go\n"
      "  -h, --help          display this help and exit\n"
      "      --version       output version information and exit\n"
      "\n"
      "Output:\n"
      "      --width=N       the page's width\n"
      "\n"
      "Tuning:\n"
      "      --effort=N      how hard\n"
      "\n"
      "Report a bug with the sheet and the command line that show it, so that each can\n"
      "be run again; the issue tracker takes them.\n"
      "\n"
      "Exit status:\n"
      "  0  if it did what was asked, and the answer, if one was asked, is yes\n"
      "  1  if it did, and the answer is no\n"
      "  2  if there was trouble: the command line, a file, or the program\n",
      "" },
    { "P02", "prog list --help", 0,
      HELP_LIST,
      "" },
    { "P03", "prog ls --help", 0,
      HELP_LIST,
      "" },
    { "P04", "prog copy --help", 0,
      "Usage: prog copy [OPTION]... FROM [TO]\n"
      "Copy FROM to TO, or to standard output.\n"
      "\n"
      "Arguments:\n"
      "  FROM                what to copy\n"
      "  TO                  where to copy it\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its\n"
      "                      description starts on the line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
      "                      script the terminal shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" },
    { "P05", "prog custom --help", 0,
      "Usage: prog custom [MAGIC]\n"
      "Write a usage of its own.\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its\n"
      "                      description starts on the line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
      "                      script the terminal shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" },
    { "P06", "prog both --help", 0,
      "Usage: prog both [OPTION]... [COMMAND [ARGUMENT]...]\n"
      "Work, and subcommands.\n"
      "\n"
      "Commands:\n"
      "  leaf                a leaf\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its\n"
      "                      description starts on the line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
      "                      script the terminal shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" },
    { "P07", "prog tree --help", 0,
      "Usage: prog tree [OPTION]... COMMAND [ARGUMENT]...\n"
      "Subcommands, and no work.\n"
      "\n"
      "Commands:\n"
      "  branch              a branch\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its\n"
      "                      description starts on the line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
      "                      script the terminal shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" },
    { "P08", "prog build --help", 0,
      "Usage: prog build [OPTION]...\n"
      "Make everything the sheet describes.\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its\n"
      "                      description starts on the line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any\n"
      "                      script the terminal shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" },
    { "P09", "prog tree", 2,
      "",
      "prog: error: tree: a command is required\n"
      "Usage: prog tree [OPTION]... COMMAND [ARGUMENT]...\n"
      "Commands: branch\n"
      "Try 'prog tree --help' for more information.\n" }
};

// NARROW
//   constant: LAYOUT's pages at 40 columns.
static const struct d_tests_cli_case NARROW[] =
{
    { "W01", "prog --help", 0,
      "Usage: prog [OPTION]... COMMAND\n"
      "            [ARGUMENT]...\n"
      "A probe of the kit.\n"
      "\n"
      "Commands:\n"
      "  list, ls            list the rows, or\n"
      "                      those a PATTERN\n"
      "                      matches\n"
      "  copy                copy FROM to TO,\n"
      "                      or to standard\n"
      "                      output\n"
      "  custom              write a usage of\n"
      "                      its own\n"
      "  both                work, and\n"
      "                      subcommands\n"
      "  tree                subcommands, and\n"
      "                      no work\n"
      "  long                a command whose\n"
      "                      description is\n"
      "                      long enough that\n"
      "                      one line of the\n"
      "                      page cannot hold\n"
      "                      it, so it is\n"
      "                      wrapped under its\n"
      "                      own column\n"
      "  help                display help for a\n"
      "                      command\n"
      "  completion          print a completion\n"
      "                      script for a shell\n"
      "\n"
      "Making:\n"
      "  build               make everything\n"
      "                      the sheet\n"
      "                      describes\n"
      "  clean               remove what build\n"
      "                      made\n"
      "\n"
      "Options:\n"
      "  -q, --quiet         say nothing but\n"
      "                      errors\n"
      "  -v, --verbose       say more; twice,\n"
      "                      more again\n"
      "      --color[=WHEN]  colour\n"
      "                      diagnostics: auto\n"
      "                      (the default),\n"
      "                      always or never\n"
      "  -o, --out=FILE      write the result\n"
      "                      to FILE (default:\n"
      "                      standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose\n"
      "                      names reach past\n"
      "                      the column, so its\n"
      "                      description starts\n"
      "                      on the line under\n"
      "                      them\n"
      "  -x VALUE            a short name\n"
      "                      alone, and its\n"
      "                      value\n"
      "      --format[=FMT]  the format: json\n"
      "                      alone, or FMT\n"
      "      --cache         a flag whose value\n"
      "                      is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as\n"
      "                      café, naïve,\n"
      "                      ünïcödé or\n"
      "                      Øresund, in any\n"
      "                      script the\n"
      "                      terminal shows\n"
      "      --depth=N       how deep to go\n"
      "  -h, --help          display this help\n"
      "                      and exit\n"
      "      --version       output version\n"
      "                      information and\n"
      "                      exit\n"
      "\n"
      "Output:\n"
      "      --width=N       the page's width\n"
      "\n"
      "Tuning:\n"
      "      --effort=N      how hard\n"
      "\n"
      "Report a bug with the sheet and the\n"
      "command line that show it, so that each\n"
      "can be run again; the issue tracker\n"
      "takes them.\n"
      "\n"
      "Exit status:\n"
      "  0  if it did what was asked, and the\n"
      "     answer, if one was asked, is yes\n"
      "  1  if it did, and the answer is no\n"
      "  2  if there was trouble: the command\n"
      "     line, a file, or the program\n",
      "" },
    { "W02", "prog copy --help", 0,
      "Usage: prog copy [OPTION]... FROM [TO]\n"
      "Copy FROM to TO, or to standard output.\n"
      "\n"
      "Arguments:\n"
      "  FROM                what to copy\n"
      "  TO                  where to copy it\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help\n"
      "                      and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but\n"
      "                      errors\n"
      "  -v, --verbose       say more; twice,\n"
      "                      more again\n"
      "      --color[=WHEN]  colour\n"
      "                      diagnostics: auto\n"
      "                      (the default),\n"
      "                      always or never\n"
      "  -o, --out=FILE      write the result\n"
      "                      to FILE (default:\n"
      "                      standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose\n"
      "                      names reach past\n"
      "                      the column, so its\n"
      "                      description starts\n"
      "                      on the line under\n"
      "                      them\n"
      "  -x VALUE            a short name\n"
      "                      alone, and its\n"
      "                      value\n"
      "      --format[=FMT]  the format: json\n"
      "                      alone, or FMT\n"
      "      --cache         a flag whose value\n"
      "                      is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as\n"
      "                      café, naïve,\n"
      "                      ünïcödé or\n"
      "                      Øresund, in any\n"
      "                      script the\n"
      "                      terminal shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" }
};

// WIDE
//   constant: LAYOUT's pages at 100 columns.
static const struct d_tests_cli_case WIDE[] =
{
    { "W03", "prog --help", 0,
      "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
      "A probe of the kit.\n"
      "\n"
      "Commands:\n"
      "  list, ls            list the rows, or those a PATTERN matches\n"
      "  copy                copy FROM to TO, or to standard output\n"
      "  custom              write a usage of its own\n"
      "  both                work, and subcommands\n"
      "  tree                subcommands, and no work\n"
      "  long                a command whose description is long enough that one line of the page cannot\n"
      "                      hold it, so it is wrapped under its own column\n"
      "  help                display help for a command\n"
      "  completion          print a completion script for a shell\n"
      "\n"
      "Making:\n"
      "  build               make everything the sheet describes\n"
      "  clean               remove what build made\n"
      "\n"
      "Options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its description starts on the\n"
      "                      line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any script the terminal\n"
      "                      shows\n"
      "      --depth=N       how deep to go\n"
      "  -h, --help          display this help and exit\n"
      "      --version       output version information and exit\n"
      "\n"
      "Output:\n"
      "      --width=N       the page's width\n"
      "\n"
      "Tuning:\n"
      "      --effort=N      how hard\n"
      "\n"
      "Report a bug with the sheet and the command line that show it, so that each can be run again; the\n"
      "issue tracker takes them.\n"
      "\n"
      "Exit status:\n"
      "  0  if it did what was asked, and the answer, if one was asked, is yes\n"
      "  1  if it did, and the answer is no\n"
      "  2  if there was trouble: the command line, a file, or the program\n",
      "" },
    { "W04", "prog copy --help", 0,
      "Usage: prog copy [OPTION]... FROM [TO]\n"
      "Copy FROM to TO, or to standard output.\n"
      "\n"
      "Arguments:\n"
      "  FROM                what to copy\n"
      "  TO                  where to copy it\n"
      "\n"
      "Options:\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
      "  -o, --out=FILE      write the result to FILE (default: standard output)\n"
      "      --very-long-option-name=VALUE\n"
      "                      an option whose names reach past the column, so its description starts on the\n"
      "                      line under them\n"
      "  -x VALUE            a short name alone, and its value\n"
      "      --format[=FMT]  the format: json alone, or FMT\n"
      "      --cache         a flag whose value is a negation\n"
      "      --mode=MODE     the mode:\n"
      "                      fast, or slow\n"
      "      --name=NAME     a name such as café, naïve, ünïcödé or Øresund, in any script the terminal\n"
      "                      shows\n"
      "      --width=N       the page's width\n"
      "      --depth=N       how deep to go\n"
      "      --effort=N      how hard\n",
      "" }
};


//==============================================================================
// 1.  PAGES
//==============================================================================

/*
d_tests_cli_format_pages
  PAGES on LAYOUT.
*/
bool
d_tests_cli_format_pages(void)
{
    return d_tests_cli_cases_run(PAGES,
                                 D_TESTS_CLI_COUNT(PAGES),
                                 d_tests_cli_make_layout);
}


//==============================================================================
// 2.  WIDTHS
//==============================================================================

/*
d_tests_cli_format_widths
  NARROW and WIDE.
*/
bool
d_tests_cli_format_widths(void)
{
    const bool held = d_tests_cli_cases_run(NARROW,
                                            D_TESTS_CLI_COUNT(NARROW),
                                            d_tests_cli_make_narrow);

    return d_tests_cli_cases_run(WIDE,
                                 D_TESTS_CLI_COUNT(WIDE),
                                 d_tests_cli_make_wide) && held;
}

//==============================================================================
// 3.  WITHOUT A PROGRAM
//==============================================================================

/**
 * @brief Lays out the help of a parser that uses the kit's formatter with
 *        no program behind it: no exit statuses, and a subcommand required
 *        only as the parser requires it.
 *
 * @param[in] _tag       the case's tag.
 * @param[in] _require   how many subcommands the parser requires.
 * @param[in] _expected  the page.
 * @return whether it was the page.
 */
static bool
d_tests_cli_bare_page(
    const char* _tag,
    size_t      _require,
    const char* _expected
)
{
    struct d_arg_parser parser;
    struct d_arg_buffer out  = D_ARG_BUFFER_INIT;
    bool                held = false;

    // a parser, the kit's formatter, a subcommand
    if ( (d_arg_parser_init(&parser,
                            "a parser without a program",
                            "bare") != D_ARG_OK) )
    {
        return d_tests_cli_check(false,
                                 _tag,
                                 "the parser could not be made");
    }

    d_cli_formatter_use(&parser.formatter);
    d_arg_formatter_set_line_width(&parser.formatter,
                                   80u);
    d_arg_command_require_subcommand(&parser.command,
                                     _require,
                                     0u);
    held = ( (d_arg_command_add_subcommand(&parser.command,
                                           "sub",
                                           "a subcommand") != NULL) &&
             (d_arg_format_help(&parser.formatter,
                                &parser.command,
                                d_pack_text_from_cstr("bare"),
                                (int)D_ARG_HELP_NORMAL,
                                &out) == D_ARG_OK) );
    held = d_tests_cli_check(held,
                             _tag,
                             "the page could not be made") &&
           d_tests_cli_same(_tag,
                            "page",
                            _expected,
                            out.data,
                            out.length);

    d_arg_buffer_release(&out);
    d_arg_parser_release(&parser);

    return held;
}

/*
d_tests_cli_format_bare
  A parser with no program: a subcommand optional, then required.
*/
bool
d_tests_cli_format_bare(void)
{
    bool held = d_tests_cli_bare_page("B01",
                                      0u,
                                      BARE_OPTIONAL);

    held = d_tests_cli_bare_page("B02",
                                 1u,
                                 BARE_REQUIRED) && held;

    // nothing to set
    d_cli_formatter_use(NULL);

    return held;
}


//==============================================================================
// 4.  MODES
//==============================================================================

/**
 * @brief The usage line alone, as the parser gives it to whoever asks: the
 *        kit's.
 *
 * @param[in] _parser  LAYOUT's parser.
 * @return whether it was the kit's line.
 */
static bool
d_tests_cli_usage_alone(
    const struct d_arg_parser* _parser
)
{
    struct d_arg_buffer usage  = D_ARG_BUFFER_INIT;
    const int           status = d_arg_format_usage(
                                     &_parser->formatter,
                                     &_parser->command,
                                     d_pack_text_from_cstr("prog"),
                                     &usage);
    const bool          held   = d_tests_cli_check(status == D_ARG_OK,
                                                   "D02",
                                                   "the usage line alone") &&
                                 d_tests_cli_same("D02",
                                                  "usage",
                                                  "Usage: prog [OPTION]... "
                                                  "COMMAND [ARGUMENT]...\n",
                                                  usage.data,
                                                  usage.length);

    d_arg_buffer_release(&usage);

    return held;
}

/*
d_tests_cli_format_modes
  Help of every subcommand, and of a subcommand alone, is the parser's own:
the same page the parser's default step makes. The usage line alone is the
kit's.
*/
bool
d_tests_cli_format_modes(void)
{
    struct d_cli_program program;
    struct d_arg_parser  parser;
    struct d_arg_buffer  out   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer  err   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer  own   = D_ARG_BUFFER_INIT;
    bool                 held  = d_tests_cli_start(&program,
                                                   &parser,
                                                   d_tests_cli_make_layout,
                                                   &out,
                                                   &err);

    // a program, to ask for help of
    if (!d_tests_cli_check(held,
                           "D01",
                           "the program could not be made"))
    {
        return false;
    }

    const struct d_pack_text name = d_pack_text_from_cstr("prog");

    for (int mode = (int)D_ARG_HELP_ALL; mode <= (int)D_ARG_HELP_SUB; ++mode)
    {
        out.length = 0u;
        own.length = 0u;
        held = d_tests_cli_check(
                   (d_arg_format_help(&parser.formatter,
                                      &parser.command,
                                      name,
                                      mode,
                                      &out) == D_ARG_OK)                 &&
                   (d_arg_format_help_default(&parser.formatter,
                                              &parser.command,
                                              name,
                                              mode,
                                              &own) == D_ARG_OK)         &&
                   (out.length == own.length)                            &&
                   (out.length > 0u)                                     &&
                   (memcmp(out.data,
                           own.data,
                           out.length) == 0),
                   "D01",
                   "a mode the kit leaves to the parser") && held;
    }

    held = d_tests_cli_usage_alone(&parser) && held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);
    d_arg_buffer_release(&own);

    return held;
}


//==============================================================================
// 5.  THE WIDTH HELP TAKES
//==============================================================================

/*
d_tests_cli_format_help_width
  The fixed width off a terminal; the terminal's, held in its bounds, on
one.
*/
bool
d_tests_cli_format_help_width(void)
{
    const size_t width = d_cli_help_width();

    return d_tests_cli_check(
               (d_cli_is_terminal((int)D_CLI_STREAM_OUTPUT))
               ? ( (width >= 40u) &&
                   (width <= (size_t)D_INTERNAL_CLI_HELP_WIDTH_MAX) )
               : (width == (size_t)D_INTERNAL_CLI_HELP_WIDTH),
               "W05",
               "d_cli_help_width");
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_format
//   constant: the suite's sections, in the order they run; with the kit's
// layout compiled out, help is the parser's own, which the parser's suites
// test, and there is nothing of the kit's here to run.
const struct d_tests_section d_tests_cli_format[] =
{
#if (D_INTERNAL_CLI_FORMATTER == 1)
    { "cli_format_pages",      d_tests_cli_format_pages      },
    { "cli_format_widths",     d_tests_cli_format_widths     },
    { "cli_format_bare",       d_tests_cli_format_bare       },
    { "cli_format_modes",      d_tests_cli_format_modes      },
#endif
    { "cli_format_help_width", d_tests_cli_format_help_width }
};

// d_tests_cli_format_count
//   constant: how many sections d_tests_cli_format has.
const size_t d_tests_cli_format_count =
    sizeof(d_tests_cli_format) / sizeof(d_tests_cli_format[0]);
