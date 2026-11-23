/*******************************************************************************
* djinterp [c][tools]                                         cli_format_tests.c
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
* are compared whole, at 80 columns and at 40 and 100. JSONED is a smaller
* program -- an option with a default and a variable, an option group, a
* command with an alias, an option of two choices and any number of
* operands, and a hidden command -- whose description is compared in
* parts: its head and first command whole, its settings by the kit's own,
* and its tail whole.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_format_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.22
*******************************************************************************/
#include "./cli_format_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <math.h>     // HUGE_VAL
#include <string.h>   // memcmp, memset, strstr
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // dj_pack_text
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // buffers
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_formatter.h"  // d_arg_format_help
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_validator.h"  // choices
#include "../../../../../inc/djinterp/tools/cli/c/cli_format.h"  // the kit's formatter
#include "../../../../../inc/djinterp/tools/cli/c/cli_help.h"  // the description
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // d_cli_is_terminal
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // specs
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
static
int d_tests_cli_work_none(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
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
static
struct dj_arg_option* d_tests_cli_option(
    struct dj_arg_command* _command,
    const char*            _names,
    const char*            _description,
    const char*            _type
)
{
    struct dj_arg_option* const option = d_arg_command_add_option(
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
static
bool d_tests_cli_flag(
    struct dj_arg_command* _command,
    const char*            _names,
    const char*            _description,
    const char*            _type
)
{
    struct dj_arg_option* const flag = d_arg_command_add_flag(_command,
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
static
bool d_tests_cli_layout_plain(
    struct dj_arg_command* _root
)
{
    struct dj_arg_option* const out = d_tests_cli_option(
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
static
bool d_tests_cli_layout_grouped(
    struct dj_arg_command* _root
)
{
    struct dj_arg_option* const  secret = d_arg_command_add_flag(
                                             _root,
                                             "--secret",
                                             "never shown");
    struct dj_arg_option* const  width  = d_tests_cli_option(
                                             _root,
                                             "--width",
                                             "the page's width",
                                             "N");
    struct dj_arg_option* const  depth  = d_tests_cli_option(
                                             _root,
                                             "--depth",
                                             "how deep to go",
                                             "N");
    struct dj_arg_command* const tuning = d_arg_command_add_option_group(
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
static
struct dj_arg_command* d_tests_cli_layout_command(
    struct dj_cli_program* _program,
    struct dj_arg_command* _parent,
    const char*            _name,
    const char*            _description,
    const char*            _group
)
{
    struct dj_arg_command* const command = d_cli_program_add_command(
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
static
bool d_tests_cli_layout_listed(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const list = d_tests_cli_layout_command(
                                           _program,
                                           NULL,
                                           "list",
                                           "list the rows, or those a "
                                           "PATTERN matches",
                                           NULL);
    struct dj_arg_command* const copy = d_tests_cli_layout_command(
                                           _program,
                                           NULL,
                                           "copy",
                                           "copy FROM to TO, or to "
                                           "standard output",
                                           NULL);
    struct dj_arg_option* const  any  = (list != NULL)
                                       ? d_arg_command_add_option(
                                             list,
                                             "PATTERN",
                                             "a pattern rows must match")
                                       : NULL;
    struct dj_arg_option* const  from = (copy != NULL)
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
static
bool d_tests_cli_layout_edges(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const custom = d_tests_cli_layout_command(
                                             _program,
                                             NULL,
                                             "custom",
                                             "write a usage of its own",
                                             NULL);
    struct dj_arg_command* const both   = d_tests_cli_layout_command(
                                             _program,
                                             NULL,
                                             "both",
                                             "work, and subcommands",
                                             NULL);
    struct dj_arg_command* const tree   = d_cli_program_add_command(
                                             _program,
                                             NULL,
                                             "tree",
                                             "subcommands, and no work",
                                             NULL,
                                             NULL,
                                             NULL);
    struct dj_arg_command* const old    = d_tests_cli_layout_command(
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
static
bool d_tests_cli_layout_last(
    struct dj_cli_program* _program
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
static
bool d_tests_cli_make_layout(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
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
static
bool d_tests_cli_make_narrow(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
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
static
bool d_tests_cli_make_wide(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
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
static const struct dj_tests_cli_case PAGES[] =
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
static const struct dj_tests_cli_case NARROW[] =
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
static const struct dj_tests_cli_case WIDE[] =
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
bool d_tests_cli_format_pages(void)
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
bool d_tests_cli_format_widths(void)
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
static
bool d_tests_cli_bare_page(
    const char* _tag,
    size_t      _require,
    const char* _expected
)
{
    struct dj_arg_parser parser;
    struct dj_arg_buffer out  = D_ARG_BUFFER_INIT;
    bool                held = false;

    // a parser, the kit's formatter, a subcommand
    if ( (d_arg_parser_init(&parser,
                            "a parser without a program",
                            "bare") != D_ARG_OK) )
    {
        return D_TESTS_CLI_CHECK(false,
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
    held = D_TESTS_CLI_CHECK(held,
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
bool d_tests_cli_format_bare(void)
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
static
bool d_tests_cli_usage_alone(
    const struct dj_arg_parser* _parser
)
{
    struct dj_arg_buffer usage  = D_ARG_BUFFER_INIT;
    const int           status = d_arg_format_usage(
                                     &_parser->formatter,
                                     &_parser->command,
                                     d_pack_text_from_cstr("prog"),
                                     &usage);
    const bool          held   = D_TESTS_CLI_CHECK(status == D_ARG_OK,
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
bool d_tests_cli_format_modes(void)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out   = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err   = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  own   = D_ARG_BUFFER_INIT;
    bool                 held  = d_tests_cli_start(&program,
                                                   &parser,
                                                   d_tests_cli_make_layout,
                                                   &out,
                                                   &err);

    // a program, to ask for help of
    if (!D_TESTS_CLI_CHECK(held,
                           "D01",
                           "the program could not be made"))
    {
        return false;
    }

    const struct dj_pack_text name = d_pack_text_from_cstr("prog");

    for (int mode = (int)D_ARG_HELP_ALL; mode <= (int)D_ARG_HELP_SUB; ++mode)
    {
        out.length = 0u;
        own.length = 0u;
        held = D_TESTS_CLI_CHECK(
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
bool d_tests_cli_format_help_width(void)
{
    const size_t width = d_cli_help_width();

    return D_TESTS_CLI_CHECK(
               (d_cli_is_terminal((int)D_CLI_STREAM_OUTPUT))
               ? ( (width >= 40u) &&
                   (width <= (size_t)D_INTERNAL_CLI_HELP_WIDTH_MAX) )
               : (width == (size_t)D_INTERNAL_CLI_HELP_WIDTH),
               "W05",
               "d_cli_help_width");
}


//==============================================================================
// 6.  AS JSON
//==============================================================================

/**
 * @brief JSONED's work: nothing.
 */
static
int d_tests_cli_json_work(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    (void)_program;
    (void)_command;
    (void)_context;

    return D_CLI_EXIT_OK;
}

/**
 * @brief A validator that takes anything, and says it takes `fast` and
 *        `slow`: choices for the description to list.
 */
static
int d_tests_cli_json_any(
    const struct dj_arg_validator* _validator,
    struct dj_arg_validation*      _validation
)
{
    (void)_validator;
    (void)_validation;

    return D_ARG_OK;
}

static
int d_tests_cli_json_modes(
    const struct dj_arg_validator* _validator,
    dj_arg_choice_fn               _each,
    void*                          _context
)
{
    int status = _each(d_pack_text_from_cstr("fast"), _context);

    (void)_validator;

    return (status == D_ARG_OK)
           ? _each(d_pack_text_from_cstr("slow"), _context)
           : status;
}

/**
 * @brief JSONED: `prog 1.2`, with `-o, --output=FILE` on its root -- a
 *        default, a variable -- and `--wide` in an option group, `Output`;
 *        `show`, alias `sh`, with `--mode=MODE` of two choices and
 *        `[WORD]...`; and `secret`, hidden.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_json(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    struct dj_arg_command* const root = &_parser->command;
    struct dj_arg_command*       show;
    struct dj_arg_command*       secret;
    struct dj_arg_command*       group;
    struct dj_arg_option*        option;
    struct dj_arg_validator      modes;

    if (d_cli_program_init(_program, _parser, "prog", "1.2",
                           D_CLI_PROGRAM_COMMANDS) != D_ARG_OK)
    {
        return false;
    }

    option = d_arg_command_add_option(root, "-o,--output",
                                      "the file to write");
    (void)d_arg_option_set_type_name(option, "FILE");
    (void)d_arg_option_set_default_text(option, "-");
    (void)d_arg_option_set_env_name(option, "PROG_OUTPUT");
    group  = d_arg_command_add_option_group(root, "Output", "the output");
    show   = d_cli_program_add_command(_program, NULL, "show",
                                       "say what is \"shown\"",
                                       d_tests_cli_json_work, NULL, NULL);
    secret = d_cli_program_add_command(_program, NULL, "secret",
                                       "not in help", d_tests_cli_json_work,
                                       NULL, NULL);

    if ( (option == NULL) ||
         (group == NULL) ||
         (show == NULL) ||
         (secret == NULL) ||
         (d_arg_command_add_flag(group, "--wide", "wide lines") == NULL) ||
         (d_arg_command_add_alias(show, "sh") != D_ARG_OK) ||
         (d_arg_command_set_group(secret, "") != D_ARG_OK) )
    {
        return false;
    }

    option = d_arg_command_add_option(show, "--mode", "how it is shown");
    modes  = d_arg_validator_custom(d_tests_cli_json_any, NULL,
                                    "fast or slow");
    d_arg_validator_set_choices(&modes, d_tests_cli_json_modes);
    (void)d_arg_option_set_type_name(option, "MODE");
    (void)d_arg_option_check(option, modes, NULL);
    option = d_arg_command_add_option(show, "WORD", "the words to show");
    d_arg_option_set_expected_range(option, 0, (int)D_ARG_EXPECTED_UNLIMITED);

    return (option != NULL);
}

// JSON_HEAD
//   constant: JSONED's description from its start to its root's first
// subcommand.
static const char JSON_HEAD[] =
    "{\n"
    "  \"format\": \"djinterp.help/1.0\",\n"
    "  \"program\": \"prog\",\n"
    "  \"version\": \"1.2\",\n"
    "  \"command\": {\n"
    "    \"name\": \"prog\",\n"
    "    \"path\": \"prog\",\n"
    "    \"summary\": \"a probe of the kit\",\n"
    "    \"options\": [\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"-h\",\n"
    "          \"--help\"\n"
    "        ],\n"
    "        \"summary\": \"display this help and exit\",\n"
    "        \"required\": false,\n"
    "        \"standard\": true\n"
    "      },\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"--version\"\n"
    "        ],\n"
    "        \"summary\": \"output version information and exit\",\n"
    "        \"required\": false,\n"
    "        \"standard\": true\n"
    "      },\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"-q\",\n"
    "          \"--quiet\"\n"
    "        ],\n"
    "        \"summary\": \"say nothing but errors\",\n"
    "        \"required\": false,\n"
    "        \"standard\": true\n"
    "      },\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"-v\",\n"
    "          \"--verbose\"\n"
    "        ],\n"
    "        \"summary\": \"say more; twice, more again\",\n"
    "        \"required\": false,\n"
    "        \"standard\": true\n"
    "      },\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"--color\"\n"
    "        ],\n"
    "        \"value\": \"WHEN\",\n"
    "        \"optional\": true,\n"
    "        \"summary\": \"colour diagnostics: auto (the default), always or "
    "never\",\n"
    "        \"required\": false,\n"
    "        \"standard\": true\n"
    "      },\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"-o\",\n"
    "          \"--output\"\n"
    "        ],\n"
    "        \"value\": \"FILE\",\n"
    "        \"summary\": \"the file to write\",\n"
    "        \"default\": \"-\",\n"
    "        \"env\": \"PROG_OUTPUT\",\n"
    "        \"required\": false,\n"
    "        \"standard\": false\n"
    "      },\n"
    "      {\n"
    "        \"names\": [\n"
    "          \"--wide\"\n"
    "        ],\n"
    "        \"summary\": \"wide lines\",\n"
    "        \"group\": \"Output\",\n"
    "        \"required\": false,\n"
    "        \"standard\": false\n"
    "      }\n"
    "    ],\n"
    "    \"arguments\": [],\n"
    "    \"commands\": [\n";

// JSON_SHOW
//   constant: `show`, the root's first subcommand.
static const char JSON_SHOW[] =
    "      {\n"
    "        \"name\": \"show\",\n"
    "        \"path\": \"prog show\",\n"
    "        \"summary\": \"say what is \\\"shown\\\"\",\n"
    "        \"aliases\": [\n"
    "          \"sh\"\n"
    "        ],\n"
    "        \"options\": [\n"
    "          {\n"
    "            \"names\": [\n"
    "              \"-h\",\n"
    "              \"--help\"\n"
    "            ],\n"
    "            \"summary\": \"display this help and exit\",\n"
    "            \"required\": false,\n"
    "            \"standard\": true\n"
    "          },\n"
    "          {\n"
    "            \"names\": [\n"
    "              \"--mode\"\n"
    "            ],\n"
    "            \"value\": \"MODE\",\n"
    "            \"summary\": \"how it is shown\",\n"
    "            \"choices\": [\n"
    "              \"fast\",\n"
    "              \"slow\"\n"
    "            ],\n"
    "            \"required\": false,\n"
    "            \"standard\": false\n"
    "          }\n"
    "        ],\n"
    "        \"arguments\": [\n"
    "          {\n"
    "            \"name\": \"WORD\",\n"
    "            \"summary\": \"the words to show\",\n"
    "            \"min\": 0,\n"
    "            \"max\": null,\n"
    "            \"required\": false\n"
    "          }\n"
    "        ],\n"
    "        \"commands\": []\n"
    "      },\n";

// JSON_TAIL
//   constant: JSONED's description from its variables to its end; the
// settings before them are the build's knobs'.
static const char JSON_TAIL[] =
    "  \"variables\": [\n"
    "    {\n"
    "      \"name\": \"NO_COLOR\",\n"
    "      \"summary\": \"set and not empty: no colour, unless --color=always "
    "asks for it\"\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"TERM\",\n"
    "      \"summary\": \"dumb: no colour, unless --color=always asks for "
    "it\"\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"COLUMNS\",\n"
    "      \"summary\": \"the width help is wrapped to where the terminal does "
    "not say, from 1 to 10000\"\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"XDG_CONFIG_HOME\",\n"
    "      \"system\": \"posix\",\n"
    "      \"summary\": \"an absolute path: the settings file is "
    "djinterp/settings under it\"\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"HOME\",\n"
    "      \"system\": \"posix\",\n"
    "      \"summary\": \"where XDG_CONFIG_HOME is not one: the settings file "
    "is .config/djinterp/settings under it\"\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"APPDATA\",\n"
    "      \"system\": \"windows\",\n"
    "      \"summary\": \"the settings file is djinterp\\\\settings under "
    "it\"\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"PATH\",\n"
    "      \"system\": \"posix\",\n"
    "      \"summary\": \"where the program's own file is looked for when it "
    "was run by name, so that its modules are found beside it\"\n"
    "    }\n"
    "  ],\n"
    "  \"prefixes\": [\n"
    "    \"DJINTERP_\",\n"
    "    \"PROG_\"\n"
    "  ],\n"
    "  \"exit_statuses\": [\n"
    "    {\n"
    "      \"status\": 0,\n"
    "      \"meaning\": \"if it did what was asked, and the answer, if one was "
    "asked, is yes\"\n"
    "    },\n"
    "    {\n"
    "      \"status\": 1,\n"
    "      \"meaning\": \"if it did, and the answer is no\"\n"
    "    },\n"
    "    {\n"
    "      \"status\": 2,\n"
    "      \"meaning\": \"if there was trouble: the command line, a file, or "
    "the program\"\n"
    "    }\n"
    "  ]\n"
    "}\n";

// JSON_CASES
//   constant: help's formats refused, and the description of a subcommand.
static const struct dj_tests_cli_case JSON_CASES[] =
{
    { "J03", "prog help --format=yaml", 2, "",
      "prog: error: help: 'yaml' is not a format: text or json\n"
      "Try 'prog help --help' for more information.\n" }
};

/**
 * @brief Runs JSONED with a command line, and checks its description: its
 *        head, the root's first subcommand and its tail, as JSONED's are,
 *        and the kit's own setting among its settings.
 *
 * @param[in] _tag   the case's tag.
 * @param[in] _argc  the arguments' count.
 * @param[in] _argv  the arguments.
 * @param[in] _root  whether the description is the root's; a subcommand's
 *                   is held to its start and its tail.
 * @return whether it held.
 */
static
bool d_tests_cli_json_run(
    const char*        _tag,
    int                _argc,
    const char* const* _argv,
    bool               _root
)
{
    static const char SETTING[] =
        "      \"name\": \"cli.plugin_strict_version\",\n"
        "      \"djin\": \"cli\",\n"
        "      \"kind\": \"bool\",\n";
    static const char VARIABLES[] =
        "        \"DJINTERP_CLI_PLUGIN_STRICT_VERSION\",\n"
        "        \"PROG_CLI_PLUGIN_STRICT_VERSION\"\n";
    static const char SHOWN[] =
        "  \"command\": {\n"
        "    \"name\": \"show\",\n"
        "    \"path\": \"prog show\",\n";
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err  = D_ARG_BUFFER_INIT;
    const size_t          head = sizeof(JSON_HEAD) - 1u;
    const size_t          show = sizeof(JSON_SHOW) - 1u;
    const size_t          tail = sizeof(JSON_TAIL) - 1u;
    bool                  held = d_tests_cli_start(&program, &parser,
                                                   d_tests_cli_make_json,
                                                   &out, &err);

    held = (held) &&
           (d_cli_program_run(&program, _argc, _argv) == D_CLI_EXIT_OK) &&
           (err.length == 0u) &&
           (out.length > ((_root) ? head + show : 0u) + tail);
    held = D_TESTS_CLI_CHECK(held, _tag, "the description is written") &&
           ( (!_root) ||
             ( (d_tests_cli_same(_tag, "head", JSON_HEAD, out.data,
                                 head)) &&
               (d_tests_cli_same(_tag, "show", JSON_SHOW, out.data + head,
                                 show)) ) ) &&
           ( (_root) ||
             (D_TESTS_CLI_CHECK(strstr(out.data, SHOWN) != NULL, _tag,
                                "the subcommand's description")) ) &&
           (d_tests_cli_same(_tag, "tail", JSON_TAIL,
                             out.data + out.length - tail, tail)) &&
           (D_TESTS_CLI_CHECK( (strstr(out.data, SETTING) != NULL) &&
                               (strstr(out.data, VARIABLES) != NULL),
                               _tag, "the kit's setting, and its variables"));

    d_tests_cli_finish(&program, &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief The writer alone: each kind of value; text escaped, UTF-8 kept,
 *        a byte that begins none replaced; empty containers; a number for
 *        no JSON number; then each misuse, which fails the writer.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_json_writer(void)
{
    static const char WRITTEN[] =
        "{\n"
        "  \"text\": \"q\\\"b\\\\s\\u0001\\n\xc3\xa9\\ufffd\",\n"
        "  \"none\": null,\n"
        "  \"list\": [],\n"
        "  \"map\": {},\n"
        "  \"tenth\": 0.1,\n"
        "  \"large\": 1e+300,\n"
        "  \"huge\": null,\n"
        "  \"integer\": -9007199254740993,\n"
        "  \"truth\": true,\n"
        "  \"nothing\": null\n"
        "}\n";
    struct dj_arg_buffer out = D_ARG_BUFFER_INIT;
    struct dj_cli_json   json;
    bool                 held;

    d_cli_json_init(&json, &out);
    d_cli_json_object(&json);
    d_cli_json_member(&json, "text");
    d_cli_json_string(&json, "q\"b\\s\x01\n\xc3\xa9\xff");
    d_cli_json_member(&json, "none");
    d_cli_json_string(&json, NULL);
    d_cli_json_member(&json, "list");
    d_cli_json_array(&json);
    d_cli_json_end(&json);
    d_cli_json_member(&json, "map");
    d_cli_json_object(&json);
    d_cli_json_end(&json);
    d_cli_json_member(&json, "tenth");
    d_cli_json_real(&json, 0.1);
    d_cli_json_member(&json, "large");
    d_cli_json_real(&json, 1e300);
    d_cli_json_member(&json, "huge");
    d_cli_json_real(&json, HUGE_VAL);
    d_cli_json_member(&json, "integer");
    d_cli_json_integer(&json, -9007199254740993LL);
    d_cli_json_member(&json, "truth");
    d_cli_json_bool(&json, true);
    d_cli_json_member(&json, "nothing");
    d_cli_json_null(&json);
    d_cli_json_end(&json);
    held = D_TESTS_CLI_CHECK(d_cli_json_finish(&json) == D_ARG_OK, "J04",
                             "each kind of value written") &&
           (d_tests_cli_same("J04", "text", WRITTEN, out.data, out.length));
    d_arg_buffer_release(&out);

    return held;
}

/**
 * @brief Whether a writer fails as a misuse should: `_how` 0, a value
 *        without a name in an object; 1, an array left open; 2, nesting
 *        past D_CLI_JSON_DEPTH; 3, no buffer; 4, a name outside an object.
 */
static
bool d_tests_cli_json_misused(
    int _how
)
{
    struct dj_arg_buffer out = D_ARG_BUFFER_INIT;
    struct dj_cli_json   json;
    int                  status;

    d_cli_json_init(&json, (_how == 3) ? NULL : &out);

    if (_how == 0)
    {
        d_cli_json_object(&json);
        d_cli_json_string(&json, "x");
        d_cli_json_end(&json);
    }
    else if (_how == 1)
    {
        d_cli_json_array(&json);
    }
    else if (_how == 2)
    {
        for (int level = 0; level <= (int)D_CLI_JSON_DEPTH; ++level)
        {
            d_cli_json_array(&json);
        }
    }
    else if (_how == 4)
    {
        d_cli_json_array(&json);
        d_cli_json_member(&json, "x");
        d_cli_json_end(&json);
    }
    else
    {
        d_cli_json_null(&json);
    }

    status = d_cli_json_finish(&json);
    d_arg_buffer_release(&out);

    return (status == D_ARG_ERROR_BAD_ARGUMENT);
}

/**
 * @brief JSONED's hook: a member of the program's own; `_context` the
 *        status it returns.
 */
static
int d_tests_cli_json_hook(
    void*                        _context,
    const struct dj_cli_program* _program,
    struct dj_cli_json*          _json
)
{
    (void)_program;

    d_cli_json_member(_json, "reserved");
    d_cli_json_array(_json);
    d_cli_json_string(_json, "list");
    d_cli_json_end(_json);

    return *(const int*)_context;
}

/**
 * @brief A program's hook: its member written after the kit's, before the
 *        object closes; a failure of its ending the description with it.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_json_hooked(void)
{
    static const char END[] =
        "  ],\n"
        "  \"reserved\": [\n"
        "    \"list\"\n"
        "  ]\n"
        "}\n";
    static const int      WELL = D_ARG_OK;
    static const int      ILL  = D_ARG_ERROR_OUT_OF_MEMORY;
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  text = D_ARG_BUFFER_INIT;
    bool                  held = d_tests_cli_start(&program, &parser,
                                                   d_tests_cli_make_json,
                                                   &out, &err);

    d_cli_program_set_help_json(&program, d_tests_cli_json_hook,
                                (void*)&WELL);
    held = (held) &&
           (d_cli_help_json(&program, NULL, &text) == D_ARG_OK) &&
           (text.length > sizeof(END) - 1u);
    held = (D_TESTS_CLI_CHECK(held, "J05", "a hook's member written")) &&
           (d_tests_cli_same("J05", "end", END,
                             text.data + text.length - (sizeof(END) - 1u),
                             sizeof(END) - 1u));
    d_cli_program_set_help_json(&program, d_tests_cli_json_hook,
                                (void*)&ILL);
    text.length = 0u;
    held = D_TESTS_CLI_CHECK(
               (d_cli_help_json(&program, NULL, &text) == ILL) &&
               (d_cli_help_json(NULL, NULL, &text) ==
                D_ARG_ERROR_BAD_ARGUMENT) &&
               (d_cli_help_json(&program, NULL, NULL) ==
                D_ARG_ERROR_BAD_ARGUMENT),
               "J06", "a hook's failure, and no program or buffer") &&
           (held);

    d_tests_cli_finish(&program, &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);
    d_arg_buffer_release(&text);

    return held;
}

/*
d_tests_cli_format_json
  Tests the following:
  - `help --format=json` describes the program: its commands down, each
    option's names, value, default, variable, heading and choices, the
    arguments' counts, aliases; the hidden left out; the settings, each
    with its variables; the kit's variables, the prefixes, the statuses
  - `help COMMAND --format=json` describes from the command down
  - a format there is none of is a usage error
  - the writer: every kind of value, escapes, UTF-8, and each misuse
  - a program's hook adds its members, and its failure ends the text
*/
bool d_tests_cli_format_json(void)
{
    static const char* const ROOT[] =
        { "prog", "help", "--format=json", NULL };
    static const char* const SHOW[] =
        { "prog", "help", "show", "--format", "json", NULL };
    bool held;

    held = d_tests_cli_json_run("J01", 3, ROOT, true);
    held = (d_tests_cli_json_run("J02", 5, SHOW, false)) && (held);
    held = (d_tests_cli_cases_run(JSON_CASES,
                                  D_TESTS_CLI_COUNT(JSON_CASES),
                                  d_tests_cli_make_json)) &&
           (held);
    held = (d_tests_cli_json_writer()) && (held);

    for (int how = 0; how <= 4; ++how)
    {
        held = (D_TESTS_CLI_CHECK(d_tests_cli_json_misused(how), "J07",
                                  "a misuse fails the writer")) &&
               (held);
    }

    return (d_tests_cli_json_hooked()) && (held);
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_format
//   constant: the suite's sections, in the order they run; with the kit's
// layout compiled out, help is the parser's own, which the parser's suites
// test, and there is nothing of the kit's here to run.
const struct dj_test_case_spec d_tests_cli_format[] =
{
#if (D_INTERNAL_CLI_FORMATTER == 1)
    { "cli_format_pages",
      "each help page, laid out",
      d_tests_cli_format_pages },
    { "cli_format_widths",
      "help at a narrow and a wide width",
      d_tests_cli_format_widths },
    { "cli_format_bare",
      "a parser with no program: subcommand optional, required",
      d_tests_cli_format_bare },
    { "cli_format_modes",
      "a subcommand's help the parser's; the usage line the kit's",
      d_tests_cli_format_modes },
#endif
    { "cli_format_help_width",
      "help's width off a terminal and on one",
      d_tests_cli_format_help_width },
    { "cli_format_json",
      "the description as JSON: help --format=json, its writer",
      d_tests_cli_format_json }
};

// d_tests_cli_format_block
//   constant: d_tests_cli_format as a block of the C face's spec form.
const struct dj_test_block_spec d_tests_cli_format_block =
{
    "format",
    "the help the kit lays out (cli_format.c)",
    d_tests_cli_format,
    sizeof(d_tests_cli_format) / sizeof(d_tests_cli_format[0])
};

