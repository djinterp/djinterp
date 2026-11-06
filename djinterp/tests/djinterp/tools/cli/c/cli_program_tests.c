/*******************************************************************************
* djinterp [tools]                                           cli_program_tests.c
*
* The suite of the command-line kit's program, declared in
* cli_program_tests.h.
*   Two fixtures make the programs the case tables run. COMMANDS is a
* program with commands, `prog 1.2`: `show`, which prints what its options
* hold; `check`, which warns at a place in a file, notes at two levels, and
* answers no; `group`, which has no work of its own and two subcommands that
* print their path; `fail`, which ends with a usage error of its own; and
* `open`, which ends with a file error; and on the root `-D NAME`, given
* once for each name, and `--level=N`. SINGLE is a program of one command:
* no version, its root's own work printing its files and whether `-n` was
* given. VERSIONED, PARTIAL and SPARSE are COMMANDS told how it was built --
* wholly, and in part -- for `--version --verbose`, whose lines are composed
* where they name the program's file and the kit's own build. REPEATED is
* COMMANDS with `pick`, whose option group holds `--kind` and which has the
* flag pair `--fast`/`--no-fast`, for the warning at `-v` (R8); DIGITS is a
* program of one command with `-1` for a short option (R6). PLAIN, LAYOUT
* and ABBREVIATED are COMMANDS made to say its errors without their word,
* to keep the argument parser's own help, and to take a unique prefix of a
* long name for the name; RESPONDING is COMMANDS made to read response
* files, which its root's help says (R14).
*
*
* path:      /tests/djinterp/tools/cli/c/cli_program_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.04
*******************************************************************************/
#include "./cli_program_tests.h"  // corresponding header
// std
#include <errno.h>    // errno, ENOENT, ENOSPC
#include <stdarg.h>   // va_list, va_start, va_end
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // snprintf
#include <stdlib.h>   // __argc and __argv on Windows
#include <string.h>   // strcmp, strerror, strstr, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/env/tools/cli/env_cli.h"  // D_ENV_CLI_HELP_SLASH
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // statuses, buffers
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // dj_cli_colour, d_cli_free
#include "../../../../../inc/djinterp/tools/cli/c/cli_plugin.h"  // d_cli_executable_path
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section
#include "./cli_tests_support.h"  // cases, checks
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_COUNT
//   macro: how many entries a table has.
#define D_TESTS_CLI_COUNT(_table)  (sizeof(_table) / sizeof((_table)[0]))

/**
 * @brief `show`'s work: prints what each of its options holds.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `show`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_work_show(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char sheet[64];
    char out[64];
    char define[128];
    char level[32];

    (void)_context;

    d_tests_cli_print(_program,
                      "show %s all=%lu out=%s define=%s level=%s\n",
                      d_tests_cli_values(_command,
                                         "SHEET",
                                         sheet,
                                         sizeof(sheet)),
                      (unsigned long)d_arg_option_count(
                          d_arg_command_option(_command,
                                               "--all")),
                      d_tests_cli_values(_command,
                                         "--out",
                                         out,
                                         sizeof(out)),
                      d_tests_cli_values(_command,
                                         "--define",
                                         define,
                                         sizeof(define)),
                      d_tests_cli_values(_command,
                                         "--level",
                                         level,
                                         sizeof(level)));

    return D_CLI_EXIT_OK;
}

/**
 * @brief `check`'s work: a warning at a place in the sheet, a note at each
 *        of two levels, and the answer no.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `check`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_NO.
 */
static
int d_tests_cli_work_check(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char sheet[64];

    (void)_context;

    d_cli_report_at(_program,
                    (int)D_CLI_SEVERITY_WARNING,
                    d_tests_cli_values(_command,
                                       "SHEET",
                                       sheet,
                                       sizeof(sheet)),
                    3L,
                    7L,
                    "row %d shares a bit",
                    2);
    d_cli_note(_program,
               1,
               "checked %s",
               sheet);
    d_cli_note(_program,
               2,
               "every row read");

    return D_CLI_EXIT_NO;
}

/**
 * @brief The work of `group`'s subcommands: prints the command's path.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_work_named(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    struct dj_arg_buffer path = D_ARG_BUFFER_INIT;

    (void)_context;
    (void)d_cli_command_path(_command,
                             &path);
    d_tests_cli_print(_program,
                      "%s\n",
                      (path.data != NULL) ? path.data : "");
    d_arg_buffer_release(&path);

    return D_CLI_EXIT_OK;
}

/**
 * @brief `fail`'s work: a usage error of its own.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `fail`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_TROUBLE.
 */
static
int d_tests_cli_work_fail(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    (void)_context;

    return d_cli_usage_error(_program,
                             _command,
                             "--x: '%s' is not a number",
                             "abc");
}

/**
 * @brief `open`'s work: the file named could not be read.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `open`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_TROUBLE.
 */
static
int d_tests_cli_work_open(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char file[64];

    (void)_context;

    return d_cli_file_error(_program,
                            "read",
                            d_tests_cli_values(_command,
                                               "FILE",
                                               file,
                                               sizeof(file)),
                            ENOENT);
}

/**
 * @brief Adds an operand to a command: required, and shown as its name.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _name         the operand's name: `SHEET`.
 * @param[in]     _description  what it is.
 * @return whether it was added.
 */
static
bool d_tests_cli_operand(
    struct dj_arg_command* _command,
    const char*            _name,
    const char*            _description
)
{
    struct dj_arg_option* const operand = (_command != NULL)
                                         ? d_arg_command_add_option(
                                               _command,
                                               _name,
                                               _description)
                                         : NULL;

    if (operand != NULL)
    {
        d_arg_option_set_required(operand,
                                  true);
    }

    return (operand != NULL);
}

/**
 * @brief The root's own options of COMMANDS: `-D NAME`, each use a name
 *        kept, and `--level=N`.
 *
 * @param[in,out] _root  the root.
 * @return whether they were made.
 */
static
bool d_tests_cli_root_options(
    struct dj_arg_command* _root
)
{
    struct dj_arg_option* const define = d_arg_command_add_option(
                                            _root,
                                            "-D,--define",
                                            "define NAME for every command");
    struct dj_arg_option* const level  = d_arg_command_add_option(
                                            _root,
                                            "--level",
                                            "how far to go");

    // made
    if ( (!define) ||
         (!level) )
    {
        return false;
    }

    // a list: any number of names, each use one of them (R8)
    d_arg_option_set_expected_range(define,
                                    1,
                                    -1);
    d_arg_option_set_multi_option_policy(define,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    return ( (d_arg_option_set_type_name(define,
                                         "NAME") == D_ARG_OK)  &&
             (d_arg_option_set_type_name(level,
                                         "N") == D_ARG_OK)     &&
             (d_arg_option_set_default_text(level,
                                            "3") == D_ARG_OK) );
}

/**
 * @brief `show`, its operand, `--all` and `-o FILE`.
 *
 * @param[in,out] _program  the program.
 * @return whether it was made.
 */
static
bool d_tests_cli_show(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const show = d_cli_program_add_command(
                                           _program,
                                           NULL,
                                           "show",
                                           "list what a sheet holds",
                                           d_tests_cli_work_show,
                                           NULL,
                                           NULL);
    struct dj_arg_option* const  out  = (show != NULL)
                                       ? d_arg_command_add_option(
                                             show,
                                             "-o,--out",
                                             "write the list to FILE")
                                       : NULL;

    return ( (d_tests_cli_operand(show,
                                  "SHEET",
                                  "the sheet to list"))                 &&
             (d_arg_command_add_flag(show,
                                     "--all",
                                     "every row, hidden ones too") != NULL) &&
             (out != NULL)                                              &&
             (d_arg_option_set_type_name(out,
                                         "FILE") == D_ARG_OK) );
}

/**
 * @brief `group`, which has no work, and its two subcommands.
 *
 * @param[in,out] _program  the program.
 * @return whether they were made.
 */
static
bool d_tests_cli_group(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const group = d_cli_program_add_command(
                                            _program,
                                            NULL,
                                            "group",
                                            "hold commands of its own",
                                            NULL,
                                            NULL,
                                            NULL);

    return ( (group != NULL)                                    &&
             (d_cli_program_add_command(_program,
                                        group,
                                        "one",
                                        "the group's first command",
                                        d_tests_cli_work_named,
                                        NULL,
                                        NULL) != NULL)          &&
             (d_cli_program_add_command(_program,
                                        group,
                                        "two",
                                        "the group's second command",
                                        d_tests_cli_work_named,
                                        NULL,
                                        NULL) != NULL) );
}

/**
 * @brief COMMANDS, made with the program flags given.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @param[in]     _flags    D_CLI_PROGRAM_* flags, D_CLI_PROGRAM_COMMANDS
 *                          among them.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_commands_with(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser,
    uint32_t               _flags
)
{
    // the program, its root's options, and its commands
    if ( (d_cli_program_init(_program,
                             _parser,
                             "prog",
                             "1.2",
                             _flags) != D_ARG_OK)                 ||
         (!d_tests_cli_root_options(&_parser->command))           ||
         (!d_tests_cli_show(_program))                            ||
         (!d_tests_cli_operand(d_cli_program_add_command(
                                   _program,
                                   NULL,
                                   "check",
                                   "report a sheet's issues",
                                   d_tests_cli_work_check,
                                   NULL,
                                   NULL),
                               "SHEET",
                               "the sheet to check"))             ||
         (!d_tests_cli_group(_program)) )
    {
        return false;
    }

    return ( (d_cli_program_add_command(_program,
                                        NULL,
                                        "fail",
                                        "end with a usage error of its own",
                                        d_tests_cli_work_fail,
                                        NULL,
                                        NULL) != NULL)  &&
             (d_tests_cli_operand(d_cli_program_add_command(
                                      _program,
                                      NULL,
                                      "open",
                                      "read a file",
                                      d_tests_cli_work_open,
                                      NULL,
                                      NULL),
                                  "FILE",
                                  "the file to read")) );
}

/**
 * @brief COMMANDS: the fixture of a program with commands.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_commands(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_commands_with(_program,
                                          _parser,
                                          D_CLI_PROGRAM_COMMANDS);
}

/**
 * @brief PLAIN: COMMANDS saying its errors without their word.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_plain(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_commands_with(_program,
                                          _parser,
                                          D_CLI_PROGRAM_COMMANDS |
                                          D_CLI_PROGRAM_PLAIN_ERRORS);
}

/**
 * @brief ABBREVIATED: COMMANDS taking a unique prefix of a long name for
 *        the name.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_abbreviated(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_commands_with(_program,
                                          _parser,
                                          D_CLI_PROGRAM_COMMANDS |
                                          D_CLI_PROGRAM_ABBREVIATIONS);
}

/**
 * @brief LAYOUT: COMMANDS with the argument parser's own help.
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
    return d_tests_cli_make_commands_with(_program,
                                          _parser,
                                          D_CLI_PROGRAM_COMMANDS |
                                          D_CLI_PROGRAM_PARSER_HELP);
}

/**
 * @brief RESPONDING: COMMANDS reading response files.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_responding(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_commands_with(_program,
                                          _parser,
                                          D_CLI_PROGRAM_COMMANDS |
                                          D_CLI_PROGRAM_RESPONSE_FILES);
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
/**
 * @brief RESPONDING_LAYOUT: RESPONDING with the argument parser's own help.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_responding_layout(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_commands_with(_program,
                                          _parser,
                                          D_CLI_PROGRAM_COMMANDS       |
                                          D_CLI_PROGRAM_PARSER_HELP    |
                                          D_CLI_PROGRAM_RESPONSE_FILES);
}
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

/**
 * @brief SINGLE's work: prints its files and how often `-n` was given.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the root.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_work_main(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char files[128];

    (void)_context;

    d_tests_cli_print(_program,
                      "files=%s n=%lu\n",
                      d_tests_cli_values(_command,
                                         "FILE",
                                         files,
                                         sizeof(files)),
                      (unsigned long)d_arg_option_count(
                          d_arg_command_option(_command,
                                               "-n")));

    return D_CLI_EXIT_OK;
}

/**
 * @brief SINGLE: the fixture of a program of one command, with no version.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_single(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    // the program
    if (d_cli_program_init(_program,
                           _parser,
                           "prog",
                           NULL,
                           0u) != D_ARG_OK)
    {
        return false;
    }

    struct dj_arg_option* const files = d_arg_command_add_option(
                                           &_parser->command,
                                           "FILE",
                                           "the files to read");

    // any number of files
    if (!files)
    {
        return false;
    }

    d_arg_option_set_expected_range(files,
                                    0,
                                    -1);

    return ( (d_arg_command_add_flag(&_parser->command,
                                     "-n,--dry-run",
                                     "say what would be done, and do "
                                     "nothing") != NULL)          &&
             (d_cli_program_set_main(_program,
                                     d_tests_cli_work_main,
                                     NULL,
                                     NULL) == D_ARG_OK) );
}

// HELP_ROOT
//   constant: COMMANDS' root help.
static const char HELP_ROOT[] =
    "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
    "A probe of the kit.\n"
    "\n"
    "Commands:\n"
    "  show                list what a sheet holds\n"
    "  check               report a sheet's issues\n"
    "  group               hold commands of its own\n"
    "  fail                end with a usage error of its own\n"
    "  open                read a file\n"
    "  help                display help for a command\n"
    "  completion          print a completion script for a shell\n"
    "\n"
    "Options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n"
    "  -h, --help          display this help and exit\n"
    "      --version       output version information and exit\n"
    "\n"
    "Exit status:\n"
    "  0  if it did what was asked, and the answer, if one was asked, is yes\n"
    "  1  if it did, and the answer is no\n"
    "  2  if there was trouble: the command line, a file, or the program\n";

// HELP_SHOW
//   constant: `show`'s help.
static const char HELP_SHOW[] =
    "Usage: prog show [OPTION]... SHEET\n"
    "List what a sheet holds.\n"
    "\n"
    "Arguments:\n"
    "  SHEET               the sheet to list\n"
    "\n"
    "Options:\n"
    "  -o, --out=FILE      write the list to FILE\n"
    "      --all           every row, hidden ones too\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Global options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n";

// HELP_ONE
//   constant: `group one`'s help.
static const char HELP_ONE[] =
    "Usage: prog group one [OPTION]...\n"
    "The group's first command.\n"
    "\n"
    "Options:\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Global options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n";

// HELP_HELP
//   constant: `help`'s help.
static const char HELP_HELP[] =
    "Usage: prog help [OPTION]... [COMMAND]...\n"
    "Display help for a command.\n"
    "\n"
    "Arguments:\n"
    "  COMMAND...          the command, and its subcommand\n"
    "\n"
    "Options:\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Global options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n";

// HELP_SINGLE
//   constant: SINGLE's help.
static const char HELP_SINGLE[] =
    "Usage: prog [OPTION]... [FILE]...\n"
    "A probe of the kit.\n"
    "\n"
    "Arguments:\n"
    "  FILE...             the files to read\n"
    "\n"
    "Options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -n, --dry-run       say what would be done, and do nothing\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Exit status:\n"
    "  0  if it did what was asked, and the answer, if one was asked, is yes\n"
    "  1  if it did, and the answer is no\n"
    "  2  if there was trouble: the command line, a file, or the program\n";

// ENDINGS
//   constant: what d_tests_cli_program_endings's command lines come to.
static const struct dj_tests_cli_case ENDINGS[] =
{
    { "E01", "prog --help", 0,
      HELP_ROOT,
      "" },
    { "E02", "prog -h", 0,
      HELP_ROOT,
      "" },
    { "E03", "prog --version", 0,
      "prog 1.2\n",
      "" },
    { "E04", "prog show --help", 0,
      HELP_SHOW,
      "" },
    { "E05", "prog show s.json --help", 0,
      HELP_SHOW,
      "" },
    { "E06", "prog --version show", 0,
      "prog 1.2\n",
      "" },
    { "E07", "prog --help -- show", 0,
      HELP_ROOT,
      "" },
    { "E08", "prog --frob", 2,
      "",
      "prog: error: the following argument was not expected: --frob\n"
      "Try 'prog --help' for more information.\n" },
    { "E09", "prog frob", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "E10", "prog frob a b", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "E11", "prog show", 2,
      "",
      "prog: error: show: SHEET is required\n"
      "Try 'prog show --help' for more information.\n" },
    { "E12", "prog show a b", 2,
      "",
      "prog: error: show: the following argument was not expected: b\n"
      "Try 'prog show --help' for more information.\n" },
    { "E13", "prog show s.json --out", 2,
      "",
      "prog: error: show: --out: 1 required FILE missing\n"
      "Try 'prog show --help' for more information.\n" },
    { "E14", "prog -- show", 2,
      "",
      "prog: error: 'show' follows '--': a command comes before it\n"
      "Try 'prog --help' for more information.\n" },
    { "E15", "prog -- show s.json", 2,
      "",
      "prog: error: 'show' follows '--': a command comes before it\n"
      "Try 'prog --help' for more information.\n" }
};

// COMMANDS
//   constant: what d_tests_cli_program_commands's command lines come to.
static const struct dj_tests_cli_case COMMANDS[] =
{
    { "C01", "prog show s.json", 0,
      "show s.json all=0 out=- define=- level=-\n",
      "" },
    { "C02", "prog show s.json --all -o x.h --level=5 -D A -D B", 0,
      "show s.json all=1 out=x.h define=A,B level=5\n",
      "" },
    { "C03", "prog -D A --level 4 show s.json", 0,
      "show s.json all=0 out=- define=A level=4\n",
      "" },
    { "C04", "prog show -D A s.json", 0,
      "show s.json all=0 out=- define=A level=-\n",
      "" },
    { "C05", "prog check s.json", 1,
      "",
      "s.json:3:7: warning: row 2 shares a bit\n" },
    { "C06", "prog -v check s.json", 1,
      "",
      "s.json:3:7: warning: row 2 shares a bit\n"
      "prog: note: checked s.json\n" },
    { "C07", "prog check s.json -vv", 1,
      "",
      "s.json:3:7: warning: row 2 shares a bit\n"
      "prog: note: checked s.json\n"
      "prog: note: every row read\n" },
    { "C08", "prog -q check s.json", 1,
      "",
      "" },
    { "C09", "prog group", 2,
      "",
      "prog: error: group: a command is required\n"
      "Usage: prog group [OPTION]... COMMAND [ARGUMENT]...\n"
      "Commands: one, two\n"
      "Try 'prog group --help' for more information.\n" },
    { "C10", "prog group one", 0,
      "prog group one\n",
      "" },
    { "C11", "prog group two", 0,
      "prog group two\n",
      "" },
    { "C12", "prog", 2,
      "",
      "prog: error: a command is required\n"
      "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
      "Commands: show, check, group, fail, open, help, completion\n"
      "Try 'prog --help' for more information.\n" },
    { "C13", "prog fail", 2,
      "",
      "prog: error: fail: --x: 'abc' is not a number\n"
      "Try 'prog fail --help' for more information.\n" },
    { "C14", "prog open nope.txt", 2,
      "",
      "prog: error: cannot read 'nope.txt': No such file or directory\n" },
    { "C15", "prog open -", 2,
      "",
      "prog: error: cannot read standard input: No such file or directory\n" },
    { "C16", "prog show s.json group", 2,
      "",
      "prog: error: show: the following argument was not expected: group\n"
      "Try 'prog show --help' for more information.\n" },
    { "C17", "prog group one two", 2,
      "",
      "prog: error: group one: the following argument was not expected: two\n"
      "Try 'prog group one --help' for more information.\n" }
};

// STANDARD
//   constant: what d_tests_cli_program_standard's command lines come to.
static const struct dj_tests_cli_case STANDARD[] =
{
    { "S01", "prog help", 0,
      HELP_ROOT,
      "" },
    { "S02", "prog help show", 0,
      HELP_SHOW,
      "" },
    { "S03", "prog help group one", 0,
      HELP_ONE,
      "" },
    { "S04", "prog help frob", 2,
      "",
      "prog: error: help: 'frob' is not a command\n"
      "Try 'prog help --help' for more information.\n" },
    { "S05", "prog help show frob", 2,
      "",
      "prog: error: help: 'frob' is not a command\n"
      "Try 'prog help --help' for more information.\n" },
    { "S06", "prog help --help", 0,
      HELP_HELP,
      "" },
    { "S07", "prog completion", 2,
      "",
      "prog: error: completion: SHELL is required\n"
      "Try 'prog completion --help' for more information.\n" },
    { "S08", "prog completion tcsh", 2,
      "",
      "prog: error: completion: 'tcsh' is not a shell: bash, zsh, fish or powershell\n"
      "Try 'prog completion --help' for more information.\n" }
};

// OPTIONS
//   constant: what d_tests_cli_program_options's command lines come to.
static const struct dj_tests_cli_case OPTIONS[] =
{
    { "O01", "prog --color=always frob", 2,
      "",
      "\033[1mprog:\033[0m \033[1;31merror:\033[0m 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "O02", "prog --colour=always frob", 2,
      "",
      "\033[1mprog:\033[0m \033[1;31merror:\033[0m 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "O03", "prog --color=never frob", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "O04", "prog --color=auto frob", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "O05", "prog --color frob", 2,
      "",
      "\033[1mprog:\033[0m \033[1;31merror:\033[0m 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "O06", "prog --color=always --color=never frob", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "O07", "prog --color=sometimes show s.json", 2,
      "",
      "prog: error: --color: 'sometimes' is not auto, always or never\n"
      "Try 'prog --help' for more information.\n" },
    { "O08", "prog --colour=bad show s.json", 2,
      "",
      "prog: error: --colour: 'bad' is not auto, always or never\n"
      "Try 'prog --help' for more information.\n" },
    { "O09", "prog --color=always check s.json", 1,
      "",
      "\033[1ms.json:3:7:\033[0m \033[1;35mwarning:\033[0m row 2 shares a bit\n" },
    { "O10", "prog --color=always -v check s.json", 1,
      "",
      "\033[1ms.json:3:7:\033[0m \033[1;35mwarning:\033[0m row 2 shares a bit\n"
      "\033[1mprog:\033[0m \033[1;36mnote:\033[0m checked s.json\n" },
    { "O11", "prog -q -v check s.json", 1,
      "",
      "" },
    { "O12", "prog show s.json --level=", 0,
      "show s.json all=0 out=- define=- level=''\n",
      "" },
    { "O13", "prog show --out= s.json", 0,
      "show s.json all=0 out='' define=- level=-\n",
      "" },
    { "O14", "prog show s.json -- --all", 2,
      "",
      "prog: error: show: the following argument was not expected: --all\n"
      "Try 'prog show --help' for more information.\n" },
    { "O15", "prog show -- -s.json", 0,
      "show -s.json all=0 out=- define=- level=-\n",
      "" },
    { "O16", "prog --verb check s.json", 2,
      "",
      "prog: error: check: the following argument was not expected: --verb\n"
      "Try 'prog check --help' for more information.\n" }
};

// FAILURES
//   constant: what d_tests_cli_program_failures's command lines come to.
static const struct dj_tests_cli_case FAILURES[] =
{
    { "F01", "prog shwo", 2,
      "",
      "prog: error: 'shwo' is not a command\n"
      "prog: note: did you mean 'show'?\n"
      "Try 'prog --help' for more information.\n" },
    { "F02", "prog shwo s.json", 2,
      "",
      "prog: error: 'shwo' is not a command\n"
      "prog: note: did you mean 'show' for 'shwo'?\n"
      "Try 'prog --help' for more information.\n" },
    { "F03", "prog show s.json --al", 2,
      "",
      "prog: error: show: the following argument was not expected: --al\n"
      "prog: note: did you mean '--all'?\n"
      "Try 'prog show --help' for more information.\n" },
    { "F04", "prog show s.json --levle=3", 2,
      "",
      "prog: error: show: the following argument was not expected: --levle=3\n"
      "prog: note: did you mean '--level'?\n"
      "Try 'prog show --help' for more information.\n" },
    { "F05", "prog --verbos show s.json", 2,
      "",
      "prog: error: show: the following argument was not expected: --verbos\n"
      "prog: note: did you mean '--verbose'?\n"
      "Try 'prog show --help' for more information.\n" },
    { "F06", "prog group oen", 2,
      "",
      "prog: error: group: 'oen' is not a command\n"
      "Try 'prog group --help' for more information.\n" },
    { "F07", "prog -x", 2,
      "",
      "prog: error: the following argument was not expected: -x\n"
      "Try 'prog --help' for more information.\n" },
    { "F08", "prog show s.json -D", 2,
      "",
      "prog: error: show: --define: 1 required NAME missing\n"
      "Try 'prog show --help' for more information.\n" },
    { "F09", "prog group one --all", 2,
      "",
      "prog: error: group one: the following argument was not expected: --all\n"
      "Try 'prog group one --help' for more information.\n" }
};

// SINGLE
//   constant: what d_tests_cli_program_single's command lines come to.
static const struct dj_tests_cli_case SINGLE[] =
{
    { "U01", "prog", 0,
      "files=- n=0\n",
      "" },
    { "U02", "prog a b c", 0,
      "files=a,b,c n=0\n",
      "" },
    { "U03", "prog -n a", 0,
      "files=a n=1\n",
      "" },
    { "U04", "prog --version", 2,
      "",
      "prog: error: the following argument was not expected: --version\n"
      "Try 'prog --help' for more information.\n" },
    { "U05", "prog --help", 0,
      HELP_SINGLE,
      "" },
    { "U06", "prog -- -n", 0,
      "files=-n n=0\n",
      "" },
    { "U07", "prog help", 0,
      "files=help n=0\n",
      "" },
    { "U08", "prog completion bash", 0,
      "files=completion,bash n=0\n",
      "" }
};

// REPORTS
//   constant: what d_tests_cli_program_reports's reports come to at the
// default verbosity: every report but the notes of a level.
static const char REPORTS[] =
    "prog: error: cannot go\n"
    "prog: warning: Mixed Case 5%\n"
    "prog: note: a note\n"
    "a.txt:3:7: error: at a column\n"
    "a.txt:3: note: at a line\n"
    "a.txt: warning: in a file\n"
    "prog: error: nowhere\n"
    "f.c:10: warning: through a list\n";

// REPORTS_QUIET
//   constant: what they come to after `-q`: the errors.
static const char REPORTS_QUIET[] =
    "prog: error: cannot go\n"
    "a.txt:3:7: error: at a column\n"
    "prog: error: nowhere\n";

// REPORTS_COLOURED
//   constant: what they come to after `-vv --color=always`: every report,
// coloured as GCC colours them.
static const char REPORTS_COLOURED[] =
    "\033[1mprog:\033[0m \033[1;31merror:\033[0m cannot go\n"
    "\033[1mprog:\033[0m \033[1;35mwarning:\033[0m Mixed Case 5%\n"
    "\033[1mprog:\033[0m \033[1;36mnote:\033[0m a note\n"
    "\033[1ma.txt:3:7:\033[0m \033[1;31merror:\033[0m at a column\n"
    "\033[1ma.txt:3:\033[0m \033[1;36mnote:\033[0m at a line\n"
    "\033[1ma.txt:\033[0m \033[1;35mwarning:\033[0m in a file\n"
    "\033[1mprog:\033[0m \033[1;31merror:\033[0m nowhere\n"
    "\033[1mf.c:10:\033[0m \033[1;35mwarning:\033[0m through a list\n"
    "\033[1mprog:\033[0m \033[1;36mnote:\033[0m at -v\n"
    "\033[1mprog:\033[0m \033[1;36mnote:\033[0m at -vv\n";

// REPORTS_ENDING
//   constant: what the reports that end a command come to.
static const char REPORTS_ENDING[] =
    "prog: error: no good\n"
    "Try 'prog --help' for more information.\n"
    "prog: error: show: --out: 'x' is a directory\n"
    "Try 'prog show --help' for more information.\n"
    "prog: error: cannot read 'a b.txt': No such file or directory\n"
    "prog: error: cannot read standard input: No such file or directory\n"
    "prog: error: cannot open standard input: No such file or directory\n"
    "prog: error: cannot write standard output: No such file or "
    "directory\n"
    "prog: error: cannot open 'c.txt': No such file or directory\n";

#if (D_ENV_CLI_HELP_SLASH == 1)

// SLASH
//   constant: what d_tests_cli_program_slash's command lines come to where
// `/?` asks for help.
static const struct dj_tests_cli_case SLASH[] =
{
    { "H01", "prog /?", 0,
      HELP_ROOT,
      "" },
    { "H02", "prog show /?", 0,
      HELP_SHOW,
      "" },
    { "H03", "prog show s.json /?", 0,
      HELP_SHOW,
      "" },
    { "H04", "prog show -- /?", 0,
      "show /? all=0 out=- define=- level=-\n",
      "" }
};

#else

// SLASH
//   constant: what d_tests_cli_program_slash's command lines come to where
// `/?` is a path.
static const struct dj_tests_cli_case SLASH[] =
{
    { "H01", "prog /?", 2,
      "",
      "prog: error: '/?' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "H02", "prog show /?", 0,
      "show /? all=0 out=- define=- level=-\n",
      "" },
    { "H03", "prog show s.json /?", 2,
      "",
      "prog: error: show: the following argument was not expected: /?\n"
      "Try 'prog show --help' for more information.\n" },
    { "H04", "prog show -- /?", 0,
      "show /? all=0 out=- define=- level=-\n",
      "" }
};

#endif  // D_ENV_CLI_HELP_SLASH

/**
 * @brief `pick`'s work: prints what its group's `--kind` and its own
 *        `--fast` hold.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `pick`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_work_pick(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char kind[32];
    char fast[32];

    (void)_context;

    d_tests_cli_print(_program,
                      "pick kind=%s fast=%s\n",
                      d_tests_cli_values(_command,
                                         "--kind",
                                         kind,
                                         sizeof(kind)),
                      d_tests_cli_values(_command,
                                         "--fast",
                                         fast,
                                         sizeof(fast)));

    return D_CLI_EXIT_OK;
}

/**
 * @brief REPEATED: COMMANDS, and `pick`, whose option group "Choices" holds
 *        `--kind=K`, and which has the flag pair `--fast`/`--no-fast`.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_repeated(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    // COMMANDS first
    if (!d_tests_cli_make_commands(_program,
                                   _parser))
    {
        return false;
    }

    struct dj_arg_command* const pick  = d_cli_program_add_command(
                                            _program,
                                            NULL,
                                            "pick",
                                            "choose a kind",
                                            d_tests_cli_work_pick,
                                            NULL,
                                            NULL);
    struct dj_arg_command* const group = (pick != NULL)
                                        ? d_arg_command_add_option_group(
                                              pick,
                                              "Choices",
                                              "what to choose")
                                        : NULL;

    return ( (group != NULL)                                      &&
             (d_arg_command_add_option(group,
                                       "--kind",
                                       "the kind to choose") != NULL) &&
             (d_arg_command_add_flag(pick,
                                     "--fast,!--no-fast",
                                     "choose quickly, or not") != NULL) );
}

// REPEATS
//   constant: what d_tests_cli_program_repeats's command lines come to: an
// option that keeps one value, given it twice or more differently, said at
// -v (R8) -- at the root, through fallthrough, in a command, in an option
// group, a flag pair, `--color` with `--colour` -- and nothing where the
// values agree, the option collects them, the parse failed, or -v was not
// given or -q was.
static const struct dj_tests_cli_case REPEATS[] =
{
    { "T01", "prog -v --level=2 --level=5 show s.json", 0,
      "show s.json all=0 out=- define=- level=5\n",
      "prog: warning: --level was given twice; the last, '5', is taken\n" },
    { "T02", "prog -v --level=2 --level=2 show s.json", 0,
      "show s.json all=0 out=- define=- level=2\n",
      "" },
    { "T03", "prog -v --level=1 show s.json --level 2 --level=3", 0,
      "show s.json all=0 out=- define=- level=3\n",
      "prog: warning: --level was given 3 times; the last, '3', is taken\n" },
    { "T04", "prog -v --color=always --colour=never show s.json", 0,
      "show s.json all=0 out=- define=- level=-\n",
      "prog: warning: --color was given twice; the last, 'never', is taken\n" },
    { "T05", "prog -v --colour=never --color=never show s.json", 0,
      "show s.json all=0 out=- define=- level=-\n",
      "" },
    { "T06", "prog -v show s.json -o a.h -o b.h", 0,
      "show s.json all=0 out=b.h define=- level=-\n",
      "prog: warning: show: --out was given twice; the last, 'b.h', is taken\n" },
    { "T07", "prog show s.json -o a.h --level=1 -o b.h -v --level=2", 0,
      "show s.json all=0 out=b.h define=- level=2\n",
      "prog: warning: --level was given twice; the last, '2', is taken\n"
      "prog: warning: show: --out was given twice; the last, 'b.h', is taken\n" },
    { "T08", "prog -v pick --kind=a --kind=b", 0,
      "pick kind=b fast=-\n",
      "prog: warning: pick: --kind was given twice; the last, 'b', is taken\n" },
    { "T09", "prog -v pick --fast --no-fast", 0,
      "pick kind=- fast=false\n",
      "prog: warning: pick: --fast was given twice; the last, 'false', is taken\n" },
    { "T10", "prog -v -D A -D B show s.json --all --all", 0,
      "show s.json all=2 out=- define=A,B level=-\n",
      "" },
    { "T11", "prog --level=2 --level=5 show s.json", 0,
      "show s.json all=0 out=- define=- level=5\n",
      "" },
    { "T12", "prog -q -v --level=2 --level=5 show s.json", 0,
      "show s.json all=0 out=- define=- level=5\n",
      "" },
    { "T13", "prog -v --level=2 --level=5", 2,
      "",
      "prog: error: a command is required\n"
      "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
      "Commands: show, check, group, fail, open, pick, help, completion\n"
      "Try 'prog --help' for more information.\n" }
};

/**
 * @brief DIGITS' work: prints how often `-1` was given, and the numbers.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the root.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_work_digits(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char numbers[64];

    (void)_context;

    d_tests_cli_print(_program,
                      "one=%lu numbers=%s\n",
                      (unsigned long)d_arg_option_count(
                          d_arg_command_option(_command,
                                               "-1")),
                      d_tests_cli_values(_command,
                                         "NUMBER",
                                         numbers,
                                         sizeof(numbers)));

    return D_CLI_EXIT_OK;
}

/**
 * @brief DIGITS: a program of one command with a digit for a short option,
 *        `-1`, which the kit allows and djinterp's programs do not use
 *        (R6), and operands that are numbers.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_digits(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    // the program
    if (d_cli_program_init(_program,
                           _parser,
                           "prog",
                           NULL,
                           0u) != D_ARG_OK)
    {
        return false;
    }

    struct dj_arg_option* const numbers = d_arg_command_add_option(
                                             &_parser->command,
                                             "NUMBER",
                                             "the numbers to print");

    // any number of numbers
    if (!numbers)
    {
        return false;
    }

    d_arg_option_set_expected_range(numbers,
                                    0,
                                    -1);

    return ( (d_arg_command_add_flag(&_parser->command,
                                     "-1,--one",
                                     "one number to a line") != NULL) &&
             (d_cli_program_set_main(_program,
                                     d_tests_cli_work_digits,
                                     NULL,
                                     NULL) == D_ARG_OK) );
}

// DIGITS
//   constant: what d_tests_cli_program_digits's command lines come to: `-1`
// the option the program defines, `-5` a number, as it defines no `-5`, and
// `-1` after `--` a number too -- what a digit option costs (R6).
static const struct dj_tests_cli_case DIGITS[] =
{
    { "G01", "prog -1 7", 0,
      "one=1 numbers=7\n",
      "" },
    { "G02", "prog -5", 0,
      "one=0 numbers=-5\n",
      "" },
    { "G03", "prog -- -1", 0,
      "one=0 numbers=-1\n",
      "" },
    { "G04", "prog -1 -5 -- -1", 0,
      "one=1 numbers=-5,-1\n",
      "" },
    { "G05", "prog -5 -1", 0,
      "one=1 numbers=-5\n",
      "" }
};

// PLAIN
//   constant: what d_tests_cli_program_plain's command lines come to on
// PLAIN: every error without its word -- the parser's failures, the kit's,
// a command's own, a file's, in colour -- and a warning and a note with
// theirs; help as it was.
static const struct dj_tests_cli_case PLAIN[] =
{
    { "Q01", "prog frob", 2,
      "",
      "prog: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "Q02", "prog show", 2,
      "",
      "prog: show: SHEET is required\n"
      "Try 'prog show --help' for more information.\n" },
    { "Q03", "prog --frob", 2,
      "",
      "prog: the following argument was not expected: --frob\n"
      "Try 'prog --help' for more information.\n" },
    { "Q04", "prog", 2,
      "",
      "prog: a command is required\n"
      "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
      "Commands: show, check, group, fail, open, help, completion\n"
      "Try 'prog --help' for more information.\n" },
    { "Q05", "prog --color=always frob", 2,
      "",
      "\033[1mprog:\033[0m 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "Q06", "prog -v check s.json", 1,
      "",
      "s.json:3:7: warning: row 2 shares a bit\n"
      "prog: note: checked s.json\n" },
    { "Q07", "prog fail", 2,
      "",
      "prog: fail: --x: 'abc' is not a number\n"
      "Try 'prog fail --help' for more information.\n" },
    { "Q08", "prog open nope.txt", 2,
      "",
      "prog: cannot read 'nope.txt': No such file or directory\n" },
    { "Q09", "prog -v --level=1 --level=2 show s.json", 0,
      "show s.json all=0 out=- define=- level=2\n",
      "prog: warning: --level was given twice; the last, '2', is taken\n" },
    { "Q10", "prog --color=sometimes show s.json", 2,
      "",
      "prog: --color: 'sometimes' is not auto, always or never\n"
      "Try 'prog --help' for more information.\n" },
    { "Q11", "prog group", 2,
      "",
      "prog: group: a command is required\n"
      "Usage: prog group [OPTION]... COMMAND [ARGUMENT]...\n"
      "Commands: one, two\n"
      "Try 'prog group --help' for more information.\n" },
    { "Q12", "prog --help", 0,
      HELP_ROOT,
      "" }
};

// REPORTS_PLAIN
//   constant: what every report comes to on PLAIN after `-vv`: an error
// without its word, a warning and a note with theirs.
static const char REPORTS_PLAIN[] =
    "prog: cannot go\n"
    "prog: warning: Mixed Case 5%\n"
    "prog: note: a note\n"
    "a.txt:3:7: at a column\n"
    "a.txt:3: note: at a line\n"
    "a.txt: warning: in a file\n"
    "prog: nowhere\n"
    "f.c:10: warning: through a list\n"
    "prog: note: at -v\n"
    "prog: note: at -vv\n";

// REPORTS_PLAIN_COLOURED
//   constant: the same after `-vv --color=always`: an error's locus bold, as
// GCC writes it, and nothing after it coloured.
static const char REPORTS_PLAIN_COLOURED[] =
    "\033[1mprog:\033[0m cannot go\n"
    "\033[1mprog:\033[0m \033[1;35mwarning:\033[0m Mixed Case 5%\n"
    "\033[1mprog:\033[0m \033[1;36mnote:\033[0m a note\n"
    "\033[1ma.txt:3:7:\033[0m at a column\n"
    "\033[1ma.txt:3:\033[0m \033[1;36mnote:\033[0m at a line\n"
    "\033[1ma.txt:\033[0m \033[1;35mwarning:\033[0m in a file\n"
    "\033[1mprog:\033[0m nowhere\n"
    "\033[1mf.c:10:\033[0m \033[1;35mwarning:\033[0m through a list\n"
    "\033[1mprog:\033[0m \033[1;36mnote:\033[0m at -v\n"
    "\033[1mprog:\033[0m \033[1;36mnote:\033[0m at -vv\n";

// REPORTS_PLAIN_ENDING
//   constant: what the reports that end a command come to on PLAIN.
static const char REPORTS_PLAIN_ENDING[] =
    "prog: no good\n"
    "Try 'prog --help' for more information.\n"
    "prog: show: --out: 'x' is a directory\n"
    "Try 'prog show --help' for more information.\n"
    "prog: cannot read 'a b.txt': No such file or directory\n"
    "prog: cannot read standard input: No such file or directory\n"
    "prog: cannot open standard input: No such file or directory\n"
    "prog: cannot write standard output: No such file or directory\n"
    "prog: cannot open 'c.txt': No such file or directory\n";

// ABBREVIATED
//   constant: what d_tests_cli_program_abbreviations's command lines come
// to on ABBREVIATED: a unique prefix of a long name for the name, the
// root's, a command's and a fallthrough parent's, before a value and with
// `=`; the hidden `--colour` making `--col` no less `--color`; a prefix
// several names begin, refused naming them; and none begun, as before.
static const struct dj_tests_cli_case ABBREVIATED[] =
{
    { "J01", "prog show s.json --al", 0,
      "show s.json all=1 out=- define=- level=-\n",
      "" },
    { "J02", "prog --lev=4 show s.json --o x.h", 0,
      "show s.json all=0 out=x.h define=- level=4\n",
      "" },
    { "J03", "prog show --def A s.json --lev 5", 0,
      "show s.json all=0 out=- define=A level=5\n",
      "" },
    { "J04", "prog --verb check s.json", 1,
      "",
      "s.json:3:7: warning: row 2 shares a bit\n"
      "prog: note: checked s.json\n" },
    { "J05", "prog --q check s.json", 1,
      "",
      "" },
    { "J06", "prog --col=never frob", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "J07", "prog --ver", 2,
      "",
      "prog: error: '--ver' is ambiguous: --version or --verbose\n"
      "Try 'prog --help' for more information.\n" },
    { "J08", "prog show s.json --a --al", 0,
      "show s.json all=2 out=- define=- level=-\n",
      "" },
    { "J09", "prog --frob", 2,
      "",
      "prog: error: the following argument was not expected: --frob\n"
      "Try 'prog --help' for more information.\n" },
    { "J10", "prog --he", 0,
      HELP_ROOT,
      "" }
};

// LAYOUT
//   constant: what d_tests_cli_program_layout's command lines come to on
// LAYOUT: help in the argument parser's own layout, wherever help is
// written; failures and the version as they were.
static const struct dj_tests_cli_case LAYOUT[] =
{
    { "Y01", "prog --help", 0,
      "a probe of the kit\n"
      "\n"
      "\n"
      "prog [OPTIONS] [SUBCOMMAND]\n"
      "\n"
      "\n"
      "OPTIONS:\n"
      "  -h,     --help              display this help and exit\n"
      "          --version           output version information and exit\n"
      "  -q,     --quiet             say nothing but errors\n"
      "  -v,     --verbose           say more; twice, more again\n"
      "          --color{always} WHEN \n"
      "                              colour diagnostics: auto (the default), always or\n"
      "                              never\n"
      "  -D,     --define NAME ...   define NAME for every command\n"
      "          --level N [3]       how far to go\n"
      "\n"
      "SUBCOMMANDS:\n"
      "  show                        list what a sheet holds\n"
      "  check                       report a sheet's issues\n"
      "  group                       hold commands of its own\n"
      "  fail                        end with a usage error of its own\n"
      "  open                        read a file\n"
      "  help                        display help for a command\n"
      "  completion                  print a completion script for a shell\n",
      "" },
    { "Y02", "prog show --help", 0,
      "list what a sheet holds\n"
      "\n"
      "\n"
      "prog show [OPTIONS] SHEET\n"
      "\n"
      "\n"
      "POSITIONALS:\n"
      "  SHEET REQUIRED              the sheet to list\n"
      "\n"
      "OPTIONS:\n"
      "  -h,     --help              display this help and exit\n"
      "  -o,     --out FILE          write the list to FILE\n"
      "          --all               every row, hidden ones too\n"
      "          --version           output version information and exit\n"
      "  -q,     --quiet             say nothing but errors\n"
      "  -v,     --verbose           say more; twice, more again\n"
      "          --color{always} WHEN \n"
      "                              colour diagnostics: auto (the default), always or\n"
      "                              never\n"
      "  -D,     --define NAME ...   define NAME for every command\n"
      "          --level N [3]       how far to go\n",
      "" },
    { "Y03", "prog help group one", 0,
      "the group's first command\n"
      "\n"
      "\n"
      "prog group one [OPTIONS]\n"
      "\n"
      "\n"
      "OPTIONS:\n"
      "  -h,     --help              display this help and exit\n"
      "          --version           output version information and exit\n"
      "  -q,     --quiet             say nothing but errors\n"
      "  -v,     --verbose           say more; twice, more again\n"
      "          --color{always} WHEN \n"
      "                              colour diagnostics: auto (the default), always or\n"
      "                              never\n"
      "  -D,     --define NAME ...   define NAME for every command\n"
      "          --level N [3]       how far to go\n",
      "" },
    { "Y04", "prog", 2,
      "",
      "prog: error: a command is required\n"
      "Usage: prog [OPTIONS] [SUBCOMMAND]\n"
      "Commands: show, check, group, fail, open, help, completion\n"
      "Try 'prog --help' for more information.\n" },
    { "Y05", "prog frob", 2,
      "",
      "prog: error: 'frob' is not a command\n"
      "Try 'prog --help' for more information.\n" },
    { "Y06", "prog --version", 0,
      "prog 1.2\n",
      "" }
};

// HELP_RESPONDING
//   constant: RESPONDING's root help: COMMANDS', and `@FILE` before help and
// version where the parser reads response files. HELP_SHOW_RESPONDING and
// HELP_ONE_RESPONDING: a command's, `@FILE` after its Global options.
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
static const char HELP_RESPONDING[] =
    "Usage: prog [OPTION]... COMMAND [ARGUMENT]...\n"
    "A probe of the kit.\n"
    "\n"
    "Commands:\n"
    "  show                list what a sheet holds\n"
    "  check               report a sheet's issues\n"
    "  group               hold commands of its own\n"
    "  fail                end with a usage error of its own\n"
    "  open                read a file\n"
    "  help                display help for a command\n"
    "  completion          print a completion script for a shell\n"
    "\n"
    "Options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n"
    "  @FILE               read more arguments from FILE, in its place\n"
    "  -h, --help          display this help and exit\n"
    "      --version       output version information and exit\n"
    "\n"
    "Exit status:\n"
    "  0  if it did what was asked, and the answer, if one was asked, is yes\n"
    "  1  if it did, and the answer is no\n"
    "  2  if there was trouble: the command line, a file, or the program\n";
static const char HELP_SHOW_RESPONDING[] =
    "Usage: prog show [OPTION]... SHEET\n"
    "List what a sheet holds.\n"
    "\n"
    "Arguments:\n"
    "  SHEET               the sheet to list\n"
    "\n"
    "Options:\n"
    "  -o, --out=FILE      write the list to FILE\n"
    "      --all           every row, hidden ones too\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Global options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always "
    "or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n"
    "  @FILE               read more arguments from FILE, in its place\n";
static const char HELP_ONE_RESPONDING[] =
    "Usage: prog group one [OPTION]...\n"
    "The group's first command.\n"
    "\n"
    "Options:\n"
    "  -h, --help          display this help and exit\n"
    "\n"
    "Global options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always "
    "or never\n"
    "  -D, --define=NAME   define NAME for every command\n"
    "      --level=N       how far to go (default: 3)\n"
    "  @FILE               read more arguments from FILE, in its place\n";
#else
    #define HELP_RESPONDING      HELP_ROOT
    #define HELP_SHOW_RESPONDING HELP_SHOW
    #define HELP_ONE_RESPONDING  HELP_ONE
#endif

// RESPONDING
//   constant: what d_tests_cli_program_responses' command lines come to on
// RESPONDING: `@FILE` on every page (ruling cli-taken 84), the root's and
// each command's, however the page is asked for.
static const struct dj_tests_cli_case RESPONDING[] =
{
    { "R01", "prog --help", 0,
      HELP_RESPONDING,
      "" },
    { "R02", "prog show --help", 0,
      HELP_SHOW_RESPONDING,
      "" },
    { "R03", "prog help", 0,
      HELP_RESPONDING,
      "" },
    { "R04", "prog help show", 0,
      HELP_SHOW_RESPONDING,
      "" },
    { "R05", "prog group one --help", 0,
      HELP_ONE_RESPONDING,
      "" }
};


// RESPONDING_LAYOUT
//   constant: what d_tests_cli_program_responses' command lines come to
// with the argument parser's own help: `@FILE` last among the OPTIONS, on
// the root's page and a command's (ruling cli-taken 84).
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
static const struct dj_tests_cli_case RESPONDING_LAYOUT[] =
{
    { "R06", "prog --help", 0,
      "a probe of the kit\n"
      "\n"
      "\n"
      "prog [OPTIONS] [SUBCOMMAND]\n"
      "\n"
      "\n"
      "OPTIONS:\n"
      "  -h,     --help              display this help and exit\n"
      "          --version           output version information and exit\n"
      "  -q,     --quiet             say nothing but errors\n"
      "  -v,     --verbose           say more; twice, more again\n"
      "          --color{always} WHEN \n"
      "                              colour diagnostics: auto (the default), always or\n"
      "                              never\n"
      "  -D,     --define NAME ...   define NAME for every command\n"
      "          --level N [3]       how far to go\n"
      "  @FILE                       read more arguments from FILE, in "
      "its place\n"
      "\n"
      "SUBCOMMANDS:\n"
      "  show                        list what a sheet holds\n"
      "  check                       report a sheet's issues\n"
      "  group                       hold commands of its own\n"
      "  fail                        end with a usage error of its own\n"
      "  open                        read a file\n"
      "  help                        display help for a command\n"
      "  completion                  print a completion script for a shell\n",
      "" },
    { "R07", "prog help group one", 0,
      "the group's first command\n"
      "\n"
      "\n"
      "prog group one [OPTIONS]\n"
      "\n"
      "\n"
      "OPTIONS:\n"
      "  -h,     --help              display this help and exit\n"
      "          --version           output version information and exit\n"
      "  -q,     --quiet             say nothing but errors\n"
      "  -v,     --verbose           say more; twice, more again\n"
      "          --color{always} WHEN \n"
      "                              colour diagnostics: auto (the default), always or\n"
      "                              never\n"
      "  -D,     --define NAME ...   define NAME for every command\n"
      "          --level N [3]       how far to go\n"
      "  @FILE                       read more arguments from FILE, in "
      "its place\n",
      "" }
};
#endif


//==============================================================================
// 1.  MAKING
//==============================================================================

/**
 * @brief Counts each release of a context it is given: a dj_arg_release_fn
 *        whose context is a counter.
 *
 * @param[in,out] _context  the counter.
 */
static
void d_tests_cli_count_release(
    void* _context
)
{
    ++*(int*)_context;

    return;
}

/**
 * @brief A command's work that does nothing: D_CLI_EXIT_OK.
 *
 * @param[in] _program  unused.
 * @param[in] _command  unused.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_work_nothing(
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
 * @brief The standard options d_cli_program_init gives the root, each the
 *        program keeps found by its names, and `--colour` hidden.
 *
 * @param[in] _program  the program.
 * @param[in] _root     its root.
 * @return whether every check held.
 */
static
bool d_tests_cli_standard_options(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _root
)
{
    const bool held = d_tests_cli_check(
                          (_program->quiet ==
                           d_arg_command_option(_root,
                                                "--quiet"))         &&
                          (_program->verbose ==
                           d_arg_command_option(_root,
                                                "-v"))              &&
                          (_program->colour_option ==
                           d_arg_command_option(_root,
                                                "--color"))         &&
                          (_program->colour_alias ==
                           d_arg_command_option(_root,
                                                "--colour"))        &&
                          (d_arg_command_option(_root,
                                                "--help") != NULL)  &&
                          (d_arg_command_option(_root,
                                                "--version") != NULL),
                          "M02",
                          "the standard options");

    return d_tests_cli_check( (_program->colour_alias != NULL)      &&
                              (strcmp(d_arg_option_group(
                                          _program->colour_alias),
                                      "") == 0),
                              "M03",
                              "--colour is hidden") && held;
}

/**
 * @brief What d_cli_program_init gives a program and its parser: the
 *        fields, the standard options, `--colour` hidden, and the standard
 *        commands not yet added.
 *
 * @return whether every check held.
 */
static
bool d_tests_cli_making_fields(void)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err  = D_ARG_BUFFER_INIT;
    bool                 held = d_tests_cli_start(&program,
                                                  &parser,
                                                  d_tests_cli_make_commands,
                                                  &out,
                                                  &err);

    // a program made, to look at
    if (!d_tests_cli_check(held,
                           "M01",
                           "the program could not be made"))
    {
        return false;
    }

    const struct dj_arg_command* const root = &parser.command;

    held = d_tests_cli_check( (strcmp(program.name,
                                      "prog") == 0)                   &&
                              (strcmp(program.version,
                                      "1.2") == 0)                    &&
                              (program.flags == D_CLI_PROGRAM_COMMANDS) &&
                              (program.verbosity == 0)                &&
                              (program.colour == (int)D_CLI_COLOUR_AUTO) &&
                              (program.parser == &parser),
                              "M01",
                              "the program's fields") && held;
    held = d_tests_cli_standard_options(&program,
                                        root) && held;
    held = d_tests_cli_check( (program.help_command == NULL)          &&
                              (program.completion_command == NULL),
                              "M04",
                              "the standard commands wait for a parse") &&
           held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief Parses a program twice: the standard commands are added at the
 *        first parse, after the program's own, and not again.
 *
 * @return whether every check held.
 */
static
bool d_tests_cli_making_standard(void)
{
    static const char* const argv[] = { "prog", "show", "s.json", NULL };
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    struct dj_arg_buffer      out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err    = D_ARG_BUFFER_INIT;
    int                      status = -1;
    bool                     held   = d_tests_cli_start(
                                          &program,
                                          &parser,
                                          d_tests_cli_make_commands,
                                          &out,
                                          &err);

    // a program made, to parse
    if (!d_tests_cli_check(held,
                           "M05",
                           "the program could not be made"))
    {
        return false;
    }

    const size_t before = parser.command.subcommands.count;
    const bool   ended  = d_cli_program_parse(&program,
                                              3,
                                              argv,
                                              &status);
    const size_t after  = parser.command.subcommands.count;

    held = d_tests_cli_check( (!ended)                                &&
                              (status == -1)                          &&
                              (after == before + 2u)                  &&
                              (program.help_command ==
                               parser.command.subcommands.items[before]) &&
                              (program.completion_command ==
                               parser.command.subcommands.items[before +
                                                                1u]),
                              "M05",
                              "help and completion, last, at the first "
                              "parse") && held;
    (void)d_cli_program_parse(&program,
                              3,
                              argv,
                              &status);
    held = d_tests_cli_check(parser.command.subcommands.count == after,
                             "M06",
                             "the standard commands added once") && held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief A program without a version has no `--version`; one without
 *        D_CLI_PROGRAM_COMMANDS gets no standard commands.
 *
 * @return whether every check held.
 */
static
bool d_tests_cli_making_single(void)
{
    static const char* const argv[] = { "prog", NULL };
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    struct dj_arg_buffer      out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err    = D_ARG_BUFFER_INIT;
    int                      status = -1;
    bool                     held   = d_tests_cli_start(
                                          &program,
                                          &parser,
                                          d_tests_cli_make_single,
                                          &out,
                                          &err);

    // a program made, to look at
    if (!d_tests_cli_check(held,
                           "M07",
                           "the program could not be made"))
    {
        return false;
    }

    held = d_tests_cli_check( (d_arg_command_option(&parser.command,
                                                    "--version") == NULL) &&
                              (strcmp(program.version,
                                      "") == 0),
                              "M07",
                              "no version, no --version") && held;
    (void)d_cli_program_parse(&program,
                              1,
                              argv,
                              &status);
    held = d_tests_cli_check( (program.help_command == NULL)          &&
                              (parser.command.subcommands.count == 0u),
                              "M08",
                              "no commands, no standard commands") && held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief A context given with a command that is not made is released at
 *        once: the program is NULL, or the parser refuses the name.
 *
 * @param[in,out] _program   the program.
 * @param[in,out] _released  the count of releases, 0 so far.
 * @return whether every check held.
 */
static
bool d_tests_cli_refused_contexts(
    struct dj_cli_program* _program,
    int*                   _released
)
{
    const bool held = d_tests_cli_check(
                          (d_cli_program_add_command(
                               NULL,
                               NULL,
                               "x",
                               "x",
                               d_tests_cli_work_nothing,
                               _released,
                               d_tests_cli_count_release) == NULL)  &&
                          (*_released == 1),
                          "M09",
                          "a NULL program releases the context");

    return d_tests_cli_check( (d_cli_program_add_command(
                                   _program,
                                   NULL,
                                   "bad name",
                                   "x",
                                   d_tests_cli_work_nothing,
                                   _released,
                                   d_tests_cli_count_release) == NULL) &&
                              (*_released == 2),
                              "M10",
                              "a refused command releases the context") &&
           held;
}

/**
 * @brief The contexts a program is given: released with the program, at
 *        once when the command could not be made or the program is NULL,
 *        and when work replaces work.
 *
 * @return whether every check held.
 */
static
bool d_tests_cli_making_contexts(void)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out      = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err      = D_ARG_BUFFER_INIT;
    int                  released = 0;
    bool                 held     = d_tests_cli_start(
                                        &program,
                                        &parser,
                                        d_tests_cli_make_single,
                                        &out,
                                        &err);

    // a program made, to give contexts to
    if (!d_tests_cli_check(held,
                           "M09",
                           "the program could not be made"))
    {
        return false;
    }

    held = d_tests_cli_refused_contexts(&program,
                                        &released);
    held = d_tests_cli_check( (d_cli_program_set_main(
                                   &program,
                                   d_tests_cli_work_nothing,
                                   &released,
                                   d_tests_cli_count_release) == D_ARG_OK) &&
                              (released == 2),
                              "M11",
                              "work replaced keeps the new context") && held;
    held = d_tests_cli_check( (d_cli_program_set_main(
                                   &program,
                                   d_tests_cli_work_nothing,
                                   &released,
                                   d_tests_cli_count_release) == D_ARG_OK) &&
                              (released == 3),
                              "M12",
                              "work replaced releases the old context") &&
           held;

    d_tests_cli_finish(&program,
                       &parser);
    held = d_tests_cli_check(released == 4,
                             "M13",
                             "the program releases the context it keeps") &&
           held;
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief A program whose making fails -- its `-v` defined before the kit's
 *        -- made with flags: its run is trouble, says why in the shape the
 *        flags ask for, and keeps why.
 *
 * @param[in] _tag       the case's tag.
 * @param[in] _flags     D_CLI_PROGRAM_* flags.
 * @param[in] _expected  what the run must write to the error output.
 * @return whether it held.
 */
static
bool d_tests_cli_failed_making(
    const char* _tag,
    uint32_t    _flags,
    const char* _expected
)
{
    static const char* const argv[] = { "prog", "--help", NULL };
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    struct dj_arg_buffer      out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err    = D_ARG_BUFFER_INIT;
    int                      made   = D_ARG_OK;
    int                      status = 0;

    memset(&program,
           0,
           sizeof(program));
    (void)d_arg_parser_init(&parser,
                            "a probe of the kit",
                            "prog");
    (void)d_arg_command_add_flag(&parser.command,
                                 "-v",
                                 "a flag of the program's own");
    made = d_cli_program_init(&program,
                              &parser,
                              "prog",
                              "1.2",
                              _flags);
    d_cli_program_set_output(&program,
                             d_arg_buffer_sink(&out),
                             d_arg_buffer_sink(&err));
    status = d_cli_program_run(&program,
                               2,
                               argv);

    bool held = d_tests_cli_check(
                    (made == (int)D_ARG_ERROR_OPTION_ALREADY_ADDED) &&
                    (program.status == made)                     &&
                    (program.name == NULL)                       &&
                    (status == D_CLI_EXIT_TROUBLE)               &&
                    (out.length == 0u),
                    _tag,
                    "a program not made keeps why, and its run is trouble");

    held = d_tests_cli_same(_tag,
                            "err",
                            _expected,
                            err.data,
                            err.length) && held;

    d_cli_program_release(&program);
    d_arg_parser_release(&parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief A program whose making fails: its run is trouble, and says why.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_making_failed(void)
{
    return d_tests_cli_failed_making("M14",
                                     D_CLI_PROGRAM_COMMANDS,
                                     "prog: error: cannot add "
                                     "'-v,--verbose': short name '-v' "
                                     "already used by '-v'\n");
}

/*
d_tests_cli_program_making
  The fields and options, the standard commands, a program of one command,
and the contexts.
*/
bool d_tests_cli_program_making(void)
{
    bool held = d_tests_cli_making_fields();

    held = d_tests_cli_making_standard() && held;
    held = d_tests_cli_making_single() && held;
    held = d_tests_cli_making_failed() && held;

    return d_tests_cli_making_contexts() && held;
}


//==============================================================================
// 2.  ENDINGS
//==============================================================================

/**
 * @brief A sink's write that takes nothing, as a full disk takes nothing,
 *        and says why as the system would: errno set to the reason its
 *        context holds, when it holds one.
 *
 * @param[in] _context  the reason, an int; NULL for none.
 * @param[in] _data     unused.
 * @param[in] _size     unused.
 * @return 0: none of it taken.
 */
static
size_t d_tests_cli_refusing_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_data;
    (void)_size;

    if (_context != NULL)
    {
        errno = *(const int*)_context;
    }

    return 0u;
}

/**
 * @brief Runs COMMANDS on a line, its output refusing what it is given --
 *        with a reason or without -- and compares the status and the error
 *        output: what was asked for, not written, ends the run in trouble
 *        and says so.
 *
 * @param[in] _tag     the case's tag.
 * @param[in] _line    the command line.
 * @param[in] _reason  the errno the output gives, or NULL for none.
 * @param[in] _err     what the error output must hold.
 * @return whether it held.
 */
static
bool d_tests_cli_unwritable(
    const char* _tag,
    const char* _line,
    const int*  _reason,
    const char* _err
)
{
    struct dj_arg_buffer      out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err    = D_ARG_BUFFER_INIT;
    struct dj_tests_cli_words words;
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    const struct dj_pack_sink refuse = { d_tests_cli_refusing_write,
                                        (void*)_reason };
    int                      status = -1;
    const bool               made   = (d_tests_cli_split(_line,
                                                         &words)) &&
                                      (d_tests_cli_start(
                                           &program,
                                           &parser,
                                           d_tests_cli_make_commands,
                                           &out,
                                           &err));

    if (made)
    {
        d_cli_program_set_output(&program,
                                 refuse,
                                 d_arg_buffer_sink(&err));
        status = d_cli_program_run(&program,
                                   words.count,
                                   words.items);
        d_tests_cli_finish(&program,
                           &parser);
    }

    const bool held = d_tests_cli_check((made) &&
                                        (status == D_CLI_EXIT_TROUBLE),
                                        _tag,
                                        "trouble, when the output refuses") &&
                      d_tests_cli_same(_tag,
                                       "err",
                                       _err,
                                       err.data,
                                       err.length);

    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief Help, a version, the help command's help and a completion script,
 *        each to an output that refuses it: with the system's reason, and
 *        without one.
 *
 * @return whether every case held.
 */
static
bool d_tests_cli_unwritable_all(void)
{
    static const int full = ENOSPC;
    char             said[256];
    bool             held = true;

    (void)snprintf(said,
                   sizeof(said),
                   "prog: error: cannot write standard output: %s\n",
                   strerror(ENOSPC));
    held = d_tests_cli_unwritable("O01",
                                  "prog --help",
                                  &full,
                                  said) && held;
    held = d_tests_cli_unwritable("O02",
                                  "prog --version",
                                  &full,
                                  said) && held;
    held = d_tests_cli_unwritable("O06",
                                  "prog --version -v",
                                  &full,
                                  said) && held;
    held = d_tests_cli_unwritable("O03",
                                  "prog help show",
                                  &full,
                                  said) && held;
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    held = d_tests_cli_unwritable("O04",
                                  "prog completion bash",
                                  &full,
                                  said) && held;
#endif

    return d_tests_cli_unwritable("O05",
                                  "prog show --help",
                                  NULL,
                                  "prog: error: cannot write standard "
                                  "output\n") && held;
}

/*
d_tests_cli_program_endings
  ENDINGS on COMMANDS; then what was asked for, to an output that refuses
it.
*/
bool d_tests_cli_program_endings(void)
{
    const bool held = d_tests_cli_cases_run(ENDINGS,
                                            D_TESTS_CLI_COUNT(ENDINGS),
                                            d_tests_cli_make_commands);

    return d_tests_cli_unwritable_all() && held;
}


//==============================================================================
// 3.  COMMANDS
//==============================================================================

/*
d_tests_cli_program_commands
  COMMANDS on COMMANDS.
*/
bool d_tests_cli_program_commands(void)
{
    return d_tests_cli_cases_run(COMMANDS,
                                 D_TESTS_CLI_COUNT(COMMANDS),
                                 d_tests_cli_make_commands);
}


//==============================================================================
// 4.  STANDARD COMMANDS
//==============================================================================

/**
 * @brief Runs `prog completion SHELL` and checks it wrote a script that
 *        names the program and its commands, and nothing on the error
 *        output.
 *
 * @param[in] _tag    the case's tag.
 * @param[in] _shell  the shell.
 * @return whether it did.
 */
static
bool d_tests_cli_completion(
    const char* _tag,
    const char* _shell
)
{
    const char* const    argv[] = { "prog", "completion", _shell, NULL };
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err    = D_ARG_BUFFER_INIT;
    bool                 held   = d_tests_cli_start(
                                      &program,
                                      &parser,
                                      d_tests_cli_make_commands,
                                      &out,
                                      &err);

    // a program made, to run
    if (!d_tests_cli_check(held,
                           _tag,
                           "the program could not be made"))
    {
        return false;
    }

    const int status = d_cli_program_run(&program,
                                         3,
                                         argv);

    d_tests_cli_finish(&program,
                       &parser);
    held = d_tests_cli_check( (status == D_CLI_EXIT_OK)               &&
                              (err.length == 0u)                      &&
                              (out.data != NULL)                      &&
                              (strstr(out.data,
                                      "prog") != NULL)                &&
                              (strstr(out.data,
                                      "show") != NULL)                &&
                              (strstr(out.data,
                                      "--level") != NULL),
                              _tag,
                              "a completion script for the shell");
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/*
d_tests_cli_program_standard
  STANDARD on COMMANDS, then a script for each shell.
*/
bool d_tests_cli_program_standard(void)
{
    bool held = d_tests_cli_cases_run(STANDARD,
                                      D_TESTS_CLI_COUNT(STANDARD),
                                      d_tests_cli_make_commands);

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    held = d_tests_cli_completion("S09",
                                  "bash") && held;
    held = d_tests_cli_completion("S10",
                                  "zsh") && held;
    held = d_tests_cli_completion("S11",
                                  "fish") && held;
    held = d_tests_cli_completion("S12",
                                  "powershell") && held;
#endif

    return held;
}


//==============================================================================
// 5.  STANDARD OPTIONS
//==============================================================================

/*
d_tests_cli_program_options
  OPTIONS on COMMANDS.
*/
bool d_tests_cli_program_options(void)
{
    return d_tests_cli_cases_run(OPTIONS,
                                 D_TESTS_CLI_COUNT(OPTIONS),
                                 d_tests_cli_make_commands);
}


//==============================================================================
// 6.  FAILURES
//==============================================================================

/*
d_tests_cli_program_failures
  FAILURES on COMMANDS.
*/
bool d_tests_cli_program_failures(void)
{
    return d_tests_cli_cases_run(FAILURES,
                                 D_TESTS_CLI_COUNT(FAILURES),
                                 d_tests_cli_make_commands);
}


//==============================================================================
// 7.  REPORTS
//==============================================================================

/**
 * @brief d_cli_vreport through a list of arguments, as a program's own
 *        report function would call it.
 *
 * @param[in] _program  the program.
 * @param[in] _format   the message's format.
 * @param[in] ...       its arguments.
 */
static
void d_tests_cli_vreport(
    const struct dj_cli_program* _program,
    const char*                  _format,
    ...
)
{
    va_list arguments;

    va_start(arguments,
             _format);
    d_cli_vreport(_program,
                  (int)D_CLI_SEVERITY_WARNING,
                  "f.c",
                  10L,
                  0L,
                  _format,
                  arguments);
    va_end(arguments);

    return;
}

/**
 * @brief Makes every report a command's work can make: each severity at no
 *        place, at a column, at a line, in a file and at no file; through a
 *        list of arguments; and a note at each of two levels.
 *
 * @param[in] _program  the program, parsed.
 */
static
void d_tests_cli_report_all(
    const struct dj_cli_program* _program
)
{
    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_ERROR,
                 "cannot %s",
                 "go");
    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_WARNING,
                 "Mixed Case %d%%",
                 5);
    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_NOTE,
                 "a note");
    d_cli_report_at(_program,
                    (int)D_CLI_SEVERITY_ERROR,
                    "a.txt",
                    3L,
                    7L,
                    "at a column");
    d_cli_report_at(_program,
                    (int)D_CLI_SEVERITY_NOTE,
                    "a.txt",
                    3L,
                    0L,
                    "at a line");
    d_cli_report_at(_program,
                    (int)D_CLI_SEVERITY_WARNING,
                    "a.txt",
                    0L,
                    7L,
                    "in a file");
    d_cli_report_at(_program,
                    (int)D_CLI_SEVERITY_ERROR,
                    NULL,
                    3L,
                    7L,
                    "nowhere");
    d_tests_cli_vreport(_program,
                        "through %s",
                        "a list");
    d_cli_note(_program,
               1,
               "at -v");
    d_cli_note(_program,
               2,
               "at -vv");

    return;
}

/**
 * @brief Makes a program, parses a command line into it, then makes every
 *        report a command's work can make, and compares the error output.
 *
 * @param[in] _tag       the case's tag.
 * @param[in] _make      the fixture: COMMANDS, or one made from it.
 * @param[in] _line      the command line parsed first, which sets the
 *                       verbosity and the colour.
 * @param[in] _expected  the error output the reports must come to.
 * @return whether it did.
 */
static
bool d_tests_cli_reports_after(
    const char*          _tag,
    dj_tests_cli_make_fn _make,
    const char*          _line,
    const char*          _expected
)
{
    struct dj_tests_cli_words words;
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    struct dj_arg_buffer      out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err    = D_ARG_BUFFER_INIT;
    int                      status = 0;

    // the words, and the program
    if ( (!d_tests_cli_split(_line,
                             &words)) ||
         (!d_tests_cli_start(&program,
                             &parser,
                             _make,
                             &out,
                             &err)) )
    {
        return d_tests_cli_check(false,
                                 _tag,
                                 "the program could not be made");
    }

    const bool parsed = (!d_cli_program_parse(&program,
                                              words.count,
                                              words.items,
                                              &status));

    if (parsed)
    {
        d_tests_cli_report_all(&program);
    }

    const bool held = d_tests_cli_check(parsed,
                                        _tag,
                                        "the command line ended the run") &&
                      d_tests_cli_same(_tag,
                                       "err",
                                       _expected,
                                       err.data,
                                       err.length);

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief Makes every report that ends a command, one after another:
 *        d_cli_usage_error for the root and for a command, and
 *        d_cli_file_error for a file and for each standard stream.
 *
 * @param[in] _program  the program.
 * @param[in] _show     its `show` command.
 * @return whether each returned D_CLI_EXIT_TROUBLE.
 */
static
bool d_tests_cli_report_endings(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _show
)
{
    bool trouble = (d_cli_usage_error(_program,
                                      NULL,
                                      "no %s",
                                      "good") == D_CLI_EXIT_TROUBLE);

    trouble = (d_cli_usage_error(_program,
                                 _show,
                                 "--out: '%s' is a directory",
                                 "x") == D_CLI_EXIT_TROUBLE) && trouble;
    trouble = (d_cli_file_error(_program,
                                "read",
                                "a b.txt",
                                ENOENT) == D_CLI_EXIT_TROUBLE) && trouble;
    trouble = (d_cli_file_error(_program,
                                "read",
                                "-",
                                ENOENT) == D_CLI_EXIT_TROUBLE) && trouble;
    trouble = (d_cli_file_error(_program,
                                "open",
                                "-",
                                ENOENT) == D_CLI_EXIT_TROUBLE) && trouble;
    trouble = (d_cli_file_error(_program,
                                "write",
                                "-",
                                ENOENT) == D_CLI_EXIT_TROUBLE) && trouble;

    return (d_cli_file_error(_program,
                             NULL,
                             "c.txt",
                             ENOENT) == D_CLI_EXIT_TROUBLE) && trouble;
}

/**
 * @brief The reports that end a command, each returning D_CLI_EXIT_TROUBLE,
 *        and what they write.
 *
 * @param[in] _tag       the case's tag.
 * @param[in] _make      the fixture: COMMANDS, or one made from it.
 * @param[in] _expected  the error output the reports must come to.
 * @return whether every check held.
 */
static
bool d_tests_cli_reports_ending(
    const char*          _tag,
    dj_tests_cli_make_fn _make,
    const char*          _expected
)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err  = D_ARG_BUFFER_INIT;
    bool                 held = d_tests_cli_start(&program,
                                                  &parser,
                                                  _make,
                                                  &out,
                                                  &err);

    // a program made, to report through
    if (!d_tests_cli_check(held,
                           _tag,
                           "the program could not be made"))
    {
        return false;
    }

    held = d_tests_cli_check(d_tests_cli_report_endings(
                                 &program,
                                 d_arg_command_subcommand(&parser.command,
                                                          "show")),
                             _tag,
                             "an ending report returns trouble");
    held = d_tests_cli_same(_tag,
                            "err",
                            _expected,
                            err.data,
                            err.length) && held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/*
d_tests_cli_program_reports
  Every report at the default verbosity, quiet, at -vv, and in colour; then
the ending reports.
*/
bool d_tests_cli_program_reports(void)
{
    bool held = d_tests_cli_reports_after("R01",
                                          d_tests_cli_make_commands,
                                          "prog show s.json",
                                          REPORTS);

    held = d_tests_cli_reports_after("R02",
                                     d_tests_cli_make_commands,
                                     "prog -q show s.json",
                                     REPORTS_QUIET) && held;
    held = d_tests_cli_reports_after("R03",
                                     d_tests_cli_make_commands,
                                     "prog show s.json -vv --color=always",
                                     REPORTS_COLOURED) && held;

    return d_tests_cli_reports_ending("R04",
                                      d_tests_cli_make_commands,
                                      REPORTS_ENDING) && held;
}


//==============================================================================
// 8.  ONE COMMAND
//==============================================================================

/*
d_tests_cli_program_single
  SINGLE on SINGLE.
*/
bool d_tests_cli_program_single(void)
{
    return d_tests_cli_cases_run(SINGLE,
                                 D_TESTS_CLI_COUNT(SINGLE),
                                 d_tests_cli_make_single);
}


//==============================================================================
// 9.  SLASH
//==============================================================================

/*
d_tests_cli_program_slash
  SLASH on COMMANDS: help where `/?` asks for it, a path elsewhere.
*/
bool d_tests_cli_program_slash(void)
{
    return d_tests_cli_cases_run(SLASH,
                                 D_TESTS_CLI_COUNT(SLASH),
                                 d_tests_cli_make_commands);
}


//==============================================================================
// 10. ARGUMENTS
//==============================================================================

/**
 * @brief Parses a list on SINGLE and says whether the program took its
 *        arguments again from the wide command line.
 *
 * @param[in]  _argc   the argument count.
 * @param[in]  _argv   the arguments.
 * @param[out] _again  receives how many arguments were taken again: 0 for
 *                     none.
 * @return whether the program was made.
 */
static
bool d_tests_cli_taken_again(
    int                _argc,
    const char* const* _argv,
    size_t*            _again
)
{
    struct dj_arg_buffer  out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err    = D_ARG_BUFFER_INIT;
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    int                  status = 0;
    const bool           made   = d_tests_cli_start(&program,
                                                    &parser,
                                                    d_tests_cli_make_single,
                                                    &out,
                                                    &err);

    *_again = 0u;

    if (made)
    {
        (void)d_cli_program_parse(&program,
                                  _argc,
                                  _argv,
                                  &status);
        *_again = parser.utf8_argv.count;
        d_tests_cli_finish(&program,
                           &parser);
    }

    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return made;
}

/*
d_tests_cli_program_arguments
  Which arguments a program parses: those it is given, unless they are
main's own -- as many as the C runtime gave main, each the same text --
which on Windows are taken again from the wide command line, in UTF-8. A
module's arguments, or a test's, are never main's: on Windows a list as
long as the runtime's, its first word another, is parsed as given, and the
runtime's own list is taken again; elsewhere nothing is.
*/
bool d_tests_cli_program_arguments(void)
{
    static const char* const argv[] = { "prog", "a", NULL };
    size_t                   again  = 0u;
    bool                     held   = d_tests_cli_check(
                                          (d_tests_cli_taken_again(
                                               2,
                                               argv,
                                               &again)) &&
                                          (again == 0u),
                                          "A01",
                                          "a list of the program's own, "
                                          "parsed as given");

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const char* runtime[D_TESTS_CLI_WORDS_MAX + 1];
    const int   count = (__argc <= D_TESTS_CLI_WORDS_MAX) ? __argc : 0;

    // the runtime's own list, its first word another
    for (int index = 0; index < count; ++index)
    {
        runtime[index] = (index == 0) ? "prog" : __argv[index];
    }

    runtime[count] = NULL;
    held = d_tests_cli_check(
               (count > 0)                                    &&
               (d_tests_cli_taken_again(count,
                                        runtime,
                                        &again))              &&
               (again == 0u),
               "A02",
               "a list as long as main's, another first word: as given") &&
           held;
    held = d_tests_cli_check(
               (d_tests_cli_taken_again(__argc,
                                        (const char* const*)__argv,
                                        &again))              &&
               (again == (size_t)__argc),
               "A03",
               "main's own arguments, taken again") && held;
#endif

    return held;
}


//==============================================================================
// 11. THE VERSION
//==============================================================================

// VERSION_BUILD
//   constant: the build VERSIONED says it was made by, every part known.
static const struct dj_cli_build VERSION_BUILD =
{
    "Probe CC",
    4,
    5,
    6,
    "C99",
    "Probe OS",
    "probe-64",
    "Release"
};

// VERSION_PARTIAL
//   constant: a build of a compiler with no version, and a machine alone.
static const struct dj_cli_build VERSION_PARTIAL =
{
    "Probe CC",
    -1,
    -1,
    -1,
    NULL,
    NULL,
    "probe-64",
    NULL
};

// VERSION_SPARSE
//   constant: a build of no compiler -- its version, 0, left unsaid -- a
// system alone, and a build type.
static const struct dj_cli_build VERSION_SPARSE =
{
    NULL,
    0,
    0,
    0,
    NULL,
    "Probe OS",
    NULL,
    "Debug"
};

/**
 * @brief VERSIONED: COMMANDS, told how it was built -- VERSION_BUILD -- and
 *        given two lines of its own.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_versioned(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    // COMMANDS, then how it was built
    if (!d_tests_cli_make_commands(_program,
                                   _parser))
    {
        return false;
    }

    d_cli_program_set_build(_program,
                            &VERSION_BUILD);

    return ( (d_cli_program_add_build(_program,
                                      "engine",
                                      "probe 7") == D_ARG_OK) &&
             (d_cli_program_add_build(_program,
                                      "data",
                                      "none") == D_ARG_OK) );
}

/**
 * @brief PARTIAL: COMMANDS, told VERSION_PARTIAL.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_partial(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    const bool made = d_tests_cli_make_commands(_program,
                                                _parser);

    d_cli_program_set_build(_program,
                            &VERSION_PARTIAL);

    return made;
}

/**
 * @brief SPARSE: COMMANDS, told VERSION_SPARSE.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_sparse(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    const bool made = d_tests_cli_make_commands(_program,
                                                _parser);

    d_cli_program_set_build(_program,
                            &VERSION_SPARSE);

    return made;
}

// dj_tests_cli_version_case
//   struct: a case of `--version`: its tag, its command line, its fixture,
// and the lines its output holds between the program's file and the kit's;
// NULL when it is the version's line alone.
struct dj_tests_cli_version_case
{
    const char*         tag;
    const char*         line;
    dj_tests_cli_make_fn make;
    const char*         build;
};

// VERSION_LINES
//   constant: what VERSIONED says of its build, and its own lines.
static const char VERSION_LINES[] =
    "compiler: Probe CC 4.5.6\n"
    "language: C99\n"
    "system: Probe OS, probe-64\n"
    "build: Release\n"
    "engine: probe 7\n"
    "data: none\n";

// VERSIONS
//   constant: `--version` alone, with `-v` before it and after, twice, and
// spelled whole; silenced by `-q` wherever it is; after a command; and for
// a program never told how it was built, and ones told only some of it.
static const struct dj_tests_cli_version_case VERSIONS[] =
{
    { "V01", "prog --version",           d_tests_cli_make_versioned, NULL },
    { "V02", "prog --version -v",        d_tests_cli_make_versioned,
      VERSION_LINES },
    { "V03", "prog -v --version",        d_tests_cli_make_versioned,
      VERSION_LINES },
    { "V04", "prog -vv --version",       d_tests_cli_make_versioned,
      VERSION_LINES },
    { "V05", "prog --version --verbose", d_tests_cli_make_versioned,
      VERSION_LINES },
    { "V06", "prog --version -q",        d_tests_cli_make_versioned, NULL },
    { "V07", "prog -v --version -q",     d_tests_cli_make_versioned, NULL },
    { "V08", "prog show --version -v",   d_tests_cli_make_versioned,
      VERSION_LINES },
    { "V09", "prog --version -v",        d_tests_cli_make_commands,  "" },
    { "V10", "prog --version -v",        d_tests_cli_make_partial,
      "compiler: Probe CC\n"
      "system: probe-64\n" },
    { "V11", "prog --version -v",        d_tests_cli_make_sparse,
      "system: Probe OS\n"
      "build: Debug\n" }
};

/**
 * @brief Runs a case of `--version`, what it must write composed: the
 *        version's line; then, with a build, the program's file as the
 *        system names it, the case's lines and the kit's.
 *
 * @param[in] _case  the case.
 * @return whether it held.
 */
static
bool d_tests_cli_version_run(
    const struct dj_tests_cli_version_case* _case
)
{
    char       out[4096];
    char       kit[256];
    char*      exe   = NULL;
    const bool named = (d_cli_executable_path("prog",
                                              &exe) == 0);
    const bool built = (_case->build != NULL);

    d_tests_cli_kit_line(kit,
                         sizeof(kit));
    (void)snprintf(out,
                   sizeof(out),
                   "prog 1.2\n%s%s%s%s%s",
                   ( (built) && (named) ) ? "program: " : "",
                   ( (built) && (named) ) ? exe : "",
                   ( (built) && (named) ) ? "\n" : "",
                   (built) ? _case->build : "",
                   (built) ? kit : "");
    d_cli_free(exe);

    const struct dj_tests_cli_case one = { _case->tag,
                                          _case->line,
                                          D_CLI_EXIT_OK,
                                          out,
                                          "" };

    return d_tests_cli_case_run(&one,
                                _case->make);
}

/**
 * @brief Whether two of a build's strings are the same: both NULL, or the
 *        same text.
 *
 * @param[in] _one    one.
 * @param[in] _other  the other.
 * @return whether they are.
 */
static
bool d_tests_cli_same_part(
    const char* _one,
    const char* _other
)
{
    return ( (_one == NULL) ||
             (_other == NULL) )
           ? (_one == _other)
           : (strcmp(_one,
                     _other) == 0);
}

/**
 * @brief The kit's own build, as this unit's compiler describes its own:
 *        the same compiler, version, system, machine and type -- the kit is
 *        compiled by the compiler that compiles the suite, and as it does --
 *        and a C level of its own, which need not be the suite's.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_version_kit(void)
{
    static const struct dj_cli_build here = D_CLI_BUILD_INIT;
    const struct dj_cli_build* const kit  = d_cli_kit_build();

    return d_tests_cli_check( (kit != NULL)                           &&
                              (d_tests_cli_same_part(kit->compiler,
                                                     here.compiler))  &&
                              (kit->major == here.major)              &&
                              (kit->minor == here.minor)              &&
                              (kit->patch == here.patch)              &&
                              (d_tests_cli_same_part(kit->system,
                                                     here.system))    &&
                              (d_tests_cli_same_part(kit->machine,
                                                     here.machine))   &&
                              (d_tests_cli_same_part(kit->type,
                                                     here.type))      &&
                              (kit->language != NULL)                 &&
                              (kit->language[0] == 'C'),
                              "V12",
                              "the kit's build, as this unit's compiler "
                              "describes its own");
}

/**
 * @brief What d_cli_program_add_build refuses, nothing kept and the program
 *        still whole: no program, a key or a value missing or empty, a key
 *        holding a colon or a line break, a value holding a line break; then
 *        two lines kept, in order, a colon in a value as it is; and a build
 *        set on nothing, or from nothing, changing nothing.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_version_refused(void)
{
    static const char* const keys[]   = { NULL, "",  "a:b", "a\nb", "a\rb",
                                          "k",  "k", "k",   "k" };
    static const char* const values[] = { "v",  "v", "v",   "v",    "v",
                                          NULL, "",  "x\ny", "x\ry" };
    struct dj_arg_buffer      out      = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err      = D_ARG_BUFFER_INIT;
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    bool                     held     = true;

    // a program to refuse them on
    if (!d_tests_cli_start(&program,
                           &parser,
                           d_tests_cli_make_commands,
                           &out,
                           &err))
    {
        return d_tests_cli_check(false,
                                 "V13",
                                 "the program could not be made");
    }

    for (size_t index = 0u; index < D_TESTS_CLI_COUNT(keys); ++index)
    {
        held = (d_cli_program_add_build(&program,
                                        keys[index],
                                        values[index]) ==
                D_ARG_ERROR_BAD_ARGUMENT) && held;
    }

    d_cli_program_set_build(NULL,
                            &VERSION_BUILD);
    d_cli_program_set_build(&program,
                            NULL);
    held = (held)                                                  &&
           (program.build_lines == NULL)                           &&
           (program.status == D_ARG_OK)                            &&
           (program.build.compiler == NULL)                        &&
           (d_cli_program_add_build(NULL,
                                    "k",
                                    "v") == D_ARG_ERROR_BAD_ARGUMENT) &&
           (d_cli_program_add_build(&program,
                                    "k",
                                    "v") == D_ARG_OK)              &&
           (d_cli_program_add_build(&program,
                                    "k2",
                                    "a: b") == D_ARG_OK)           &&
           (program.build_lines != NULL)                           &&
           (strcmp(program.build_lines,
                   "k: v\nk2: a: b\n") == 0);
    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return d_tests_cli_check(held,
                             "V13",
                             "the lines refused, and the lines kept");
}

/*
d_tests_cli_program_version
  VERSIONS, each on its fixture; the kit's own build; and the lines a
program adds, refused and kept.
*/
bool d_tests_cli_program_version(void)
{
    bool held = true;

    for (size_t index = 0u; index < D_TESTS_CLI_COUNT(VERSIONS); ++index)
    {
        held = d_tests_cli_version_run(&VERSIONS[index]) && held;
    }

    held = d_tests_cli_version_kit() && held;

    return d_tests_cli_version_refused() && held;
}


//==============================================================================
// 12. REPEATS
//==============================================================================

/*
d_tests_cli_program_repeats
  REPEATS on REPEATED.
*/
bool d_tests_cli_program_repeats(void)
{
    return d_tests_cli_cases_run(REPEATS,
                                 D_TESTS_CLI_COUNT(REPEATS),
                                 d_tests_cli_make_repeated);
}


//==============================================================================
// 13. DIGITS
//==============================================================================

/*
d_tests_cli_program_digits
  DIGITS on DIGITS.
*/
bool d_tests_cli_program_digits(void)
{
    return d_tests_cli_cases_run(DIGITS,
                                 D_TESTS_CLI_COUNT(DIGITS),
                                 d_tests_cli_make_digits);
}


//==============================================================================
// 14. PLAIN ERRORS
//==============================================================================

/*
d_tests_cli_program_plain
  PLAIN on PLAIN; every report on it at -vv, and in colour; the reports
that end a command; and a program not made, saying why without the word.
*/
bool d_tests_cli_program_plain(void)
{
    bool held = d_tests_cli_cases_run(PLAIN,
                                      D_TESTS_CLI_COUNT(PLAIN),
                                      d_tests_cli_make_plain);

    held = d_tests_cli_reports_after("Q13",
                                     d_tests_cli_make_plain,
                                     "prog show s.json -vv",
                                     REPORTS_PLAIN) && held;
    held = d_tests_cli_reports_after("Q14",
                                     d_tests_cli_make_plain,
                                     "prog show s.json -vv --color=always",
                                     REPORTS_PLAIN_COLOURED) && held;
    held = d_tests_cli_reports_ending("Q15",
                                      d_tests_cli_make_plain,
                                      REPORTS_PLAIN_ENDING) && held;

    return d_tests_cli_failed_making("Q16",
                                     D_CLI_PROGRAM_COMMANDS |
                                     D_CLI_PROGRAM_PLAIN_ERRORS,
                                     "prog: cannot add '-v,--verbose': "
                                     "short name '-v' already used by "
                                     "'-v'\n") && held;
}


//==============================================================================
// 15. THE PARSER'S HELP
//==============================================================================

/*
d_tests_cli_program_layout
  LAYOUT on LAYOUT.
*/
bool d_tests_cli_program_layout(void)
{
    return d_tests_cli_cases_run(LAYOUT,
                                 D_TESTS_CLI_COUNT(LAYOUT),
                                 d_tests_cli_make_layout);
}


//==============================================================================
// 16. ABBREVIATIONS
//==============================================================================

/*
d_tests_cli_program_abbreviations
  ABBREVIATED on ABBREVIATED.
*/
bool d_tests_cli_program_abbreviations(void)
{
    return d_tests_cli_cases_run(ABBREVIATED,
                                 D_TESTS_CLI_COUNT(ABBREVIATED),
                                 d_tests_cli_make_abbreviated);
}


//==============================================================================
// 17. NULLS
//==============================================================================

/**
 * @brief Making, parsing, running and dispatching given nothing, or a
 *        program that was never made: a status, and nothing written.
 *
 * @param[in,out] _program  a program never made, zeroed.
 * @param[in,out] _parser   a parser never made.
 * @return whether every check held.
 */
static
bool d_tests_cli_nulls_running(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    static const char* const argv[] = { "prog", NULL };
    int                      status = 0;
    bool                     held   = true;

    held = d_tests_cli_check(
               (d_cli_program_init(NULL,
                                   _parser,
                                   "prog",
                                   NULL,
                                   0u) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_program_init(_program,
                                   NULL,
                                   "prog",
                                   NULL,
                                   0u) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_program_init(_program,
                                   _parser,
                                   NULL,
                                   NULL,
                                   0u) == D_ARG_ERROR_BAD_ARGUMENT),
               "N01",
               "init without a program, a parser or a name");

    return d_tests_cli_check(
               (d_cli_program_parse(NULL,
                                    1,
                                    argv,
                                    &status))                         &&
               (status == D_CLI_EXIT_TROUBLE)                         &&
               (d_cli_program_parse(_program,
                                    1,
                                    argv,
                                    NULL))                            &&
               (d_cli_program_parse(_program,
                                    1,
                                    argv,
                                    &status))                         &&
               (d_cli_program_run(NULL,
                                  1,
                                  argv) == D_CLI_EXIT_TROUBLE)        &&
               (d_cli_program_dispatch(NULL) == D_CLI_EXIT_TROUBLE)   &&
               (d_cli_program_dispatch(_program) == D_CLI_EXIT_TROUBLE),
               "N02",
               "parsing and running nothing, or a program not made") &&
           held;
}

/*
d_tests_cli_program_nulls
  Every call given NULL where it takes a pointer: a status, NULL, false or
nothing at all, and nothing written.
*/
bool d_tests_cli_program_nulls(void)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  path = D_ARG_BUFFER_INIT;

    memset(&program,
           0,
           sizeof(program));

    const bool held = d_tests_cli_nulls_running(&program,
                                                &parser);

    // nothing to release, nothing to set, nothing to say
    d_cli_program_release(NULL);
    d_cli_program_set_output(NULL,
                             d_arg_buffer_sink(&path),
                             d_arg_buffer_sink(&path));
    d_cli_report(NULL,
                 (int)D_CLI_SEVERITY_ERROR,
                 "x");
    d_cli_report_at(NULL,
                    (int)D_CLI_SEVERITY_ERROR,
                    "f",
                    1L,
                    1L,
                    "x");
    d_cli_note(NULL,
               0,
               "x");

    return d_tests_cli_check(
               (d_cli_program_command(NULL) == NULL)                  &&
               (d_cli_program_verbosity(NULL) == 0)                   &&
               (!d_cli_program_colour(NULL,
                                      (int)D_CLI_STREAM_ERROR))        &&
               (d_cli_program_set_main(NULL,
                                       d_tests_cli_work_nothing,
                                       NULL,
                                       NULL) ==
                D_ARG_ERROR_BAD_ARGUMENT)                             &&
               (d_cli_command_path(NULL,
                                   &path) == D_ARG_ERROR_BAD_ARGUMENT) &&
               (d_cli_command_path(&parser.command,
                                   NULL) == D_ARG_ERROR_BAD_ARGUMENT) &&
               (d_cli_usage_error(NULL,
                                  NULL,
                                  "x") == D_CLI_EXIT_TROUBLE)          &&
               (path.length == 0u),
               "N03",
               "queries and settings given nothing") && held;
}


//==============================================================================
// 18. RESPONSE FILES
//==============================================================================

/*
d_tests_cli_program_responses
  RESPONDING on RESPONDING; and, where response files are read,
RESPONDING_LAYOUT on RESPONDING with the parser's own help.
*/
bool d_tests_cli_program_responses(void)
{
    bool held = d_tests_cli_cases_run(RESPONDING,
                                      D_TESTS_CLI_COUNT(RESPONDING),
                                      d_tests_cli_make_responding);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    held = d_tests_cli_cases_run(RESPONDING_LAYOUT,
                                 D_TESTS_CLI_COUNT(RESPONDING_LAYOUT),
                                 d_tests_cli_make_responding_layout) && held;
#endif

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_program
//   constant: the suite's sections, in the order they run.
const struct dj_tests_section d_tests_cli_program[] =
{
    { "cli_program_making",    d_tests_cli_program_making    },
    { "cli_program_endings",   d_tests_cli_program_endings   },
    { "cli_program_commands",  d_tests_cli_program_commands  },
    { "cli_program_standard",  d_tests_cli_program_standard  },
    { "cli_program_options",   d_tests_cli_program_options   },
    { "cli_program_failures",  d_tests_cli_program_failures  },
    { "cli_program_reports",   d_tests_cli_program_reports   },
    { "cli_program_single",    d_tests_cli_program_single    },
    { "cli_program_slash",     d_tests_cli_program_slash     },
    { "cli_program_arguments", d_tests_cli_program_arguments },
    { "cli_program_version",   d_tests_cli_program_version   },
    { "cli_program_repeats",   d_tests_cli_program_repeats   },
    { "cli_program_digits",    d_tests_cli_program_digits    },
    { "cli_program_plain",     d_tests_cli_program_plain     },
    { "cli_program_layout",    d_tests_cli_program_layout    },
    { "cli_program_abbreviations",
      d_tests_cli_program_abbreviations },
    { "cli_program_nulls",     d_tests_cli_program_nulls     },
    { "cli_program_responses", d_tests_cli_program_responses }
};

// d_tests_cli_program_count
//   constant: how many sections d_tests_cli_program has.
const size_t d_tests_cli_program_count =
    sizeof(d_tests_cli_program) / sizeof(d_tests_cli_program[0]);
