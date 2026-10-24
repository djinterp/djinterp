/*******************************************************************************
* djinterp [parse]                                                 cli_program.h
*
* A command-line program as the kit runs it: the grammar's conventions over
* the argument parser.
*   A d_cli_program holds a parser the caller made, and gives it what every
* djinterp program has (GRAMMAR.md, section 6): `-h` and `--help`,
* `--version`, `-q`, `-v` and `--color`; for a program with commands, the
* `help` and `completion` commands; the kit's layout of help; an option given
* twice taking its last value (R8); `/?` on Windows (R10) and no other `/`
* option; and failure messages in the GNU shape, one line and a pointer to
* help, on standard error. The program then adds its own commands, each with
* the function that does its work, and its own options, through the parser.
*   RUNNING. d_cli_program_run parses main's arguments -- the true command
* line on Windows, whatever the code page -- ends a parse that asked for
* help, a version or a completion script, or that failed, with what it asked
* for and the right exit status, and otherwise calls the function of the
* command the arguments reached, whose status the program exits with. A
* program of one command, or a plugin, gives the root a function of its own,
* or calls d_cli_program_parse and goes on itself.
*   EXIT STATUS (GRAMMAR.md, section 8). 0 done, 1 done and the answer is
* no, 2 trouble. Help, a version and a completion script are 0; every error
* the parser reports, whatever CLI11's code for it, is 2.
*   DIAGNOSTICS. `PROGRAM: error: MESSAGE`, `FILE:LINE:COLUMN: warning:
* MESSAGE`, coloured as GCC colours them where colour is wanted; `-q` keeps
* only errors, and `-v` lets notes of a level at or below its count through.
*
*
* path:      /inc/djinterp/parse/cli/c/cli_program.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Exit statuses
         1.  D_CLI_EXIT_OK
         2.  D_CLI_EXIT_NO
         3.  D_CLI_EXIT_TROUBLE
    2.  Severities
         1.  d_cli_severity
    3.  Program flags
         1.  D_CLI_PROGRAM_COMMANDS
         2.  D_CLI_PROGRAM_RESPONSE_FILES
    4.  Callbacks
         1.  d_cli_command_fn
2.  RECORDS
    -------
    1.  A command's work
         1.  d_cli_handler
    2.  The program
         1.  d_cli_program
3.  LIFETIME
    --------
    1.  Making and releasing
         1.  d_cli_program_init
         2.  d_cli_program_release
4.  COMMANDS
    --------
    1.  Adding
         1.  d_cli_program_add_command
         2.  d_cli_program_set_main
5.  RUNNING
    -------
    1.  Parsing
         1.  d_cli_program_parse
    2.  Dispatching
         1.  d_cli_program_dispatch
         2.  d_cli_program_run
    3.  What the parse reached
         1.  d_cli_program_command
6.  SETTINGS AND QUERIES
    --------------------
    1.  Output
         1.  d_cli_program_set_output
    2.  The standard options' values
         1.  d_cli_program_verbosity
         2.  d_cli_program_colour
    3.  Names
         1.  d_cli_command_path
7.  DIAGNOSTICS
    -----------
    1.  Reports
         1.  d_cli_report / d_cli_report_at
         2.  d_cli_vreport
         3.  d_cli_note
    2.  Reports that end a command
         1.  d_cli_usage_error
         2.  d_cli_file_error
*/

#ifndef DJINTERP_PARSE_CLI_C_CLI_PROGRAM_H
#define DJINTERP_PARSE_CLI_C_CLI_PROGRAM_H 1

// std
#include <stdarg.h>   // va_list
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../c/djinterp.h"                     // framework root
#include "../../../c/util/sink_common.h"             // d_pack_sink
#include "../../../config/parse/cli/cfg_cli.h"       // D_INTERNAL_CLI_*
#include "../../../env/c/env_vendor_attributes.h"    // D_FORMAT_PRINTF
#include "../arg_parser/c/arg_parser.h"              // d_arg_parser,
                                                     // d_arg_command
#include "./cli_io.h"                                // d_cli_stream,
                                                     // d_cli_colour
// re_std
#include "../../../../re_std/cstdint/dstdint.h"      // uint32_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Exit statuses
//------------------------------------------------------------------------------
// 1.1.1
// D_CLI_EXIT_OK
//   constant: done -- it did what was asked, and the answer, if one was
// asked, is yes.
#define D_CLI_EXIT_OK      0

// 1.1.2
// D_CLI_EXIT_NO
//   constant: done, and the answer is no -- a sheet with errors, an input a
// grammar refuses, two runs that differ. A command that answers nothing
// never returns it.
#define D_CLI_EXIT_NO      1

// 1.1.3
// D_CLI_EXIT_TROUBLE
//   constant: it could not do what was asked -- the command line, a file, a
// document that is not one, a missing plugin, the program itself.
#define D_CLI_EXIT_TROUBLE 2

// 1.2    Severities
//------------------------------------------------------------------------------
// 1.2.1
// d_cli_severity
//   enum: what a diagnostic is, as it names itself: `error`, `warning` or
// `note`. `-q` keeps only errors.
enum d_cli_severity
{
    D_CLI_SEVERITY_ERROR   = 0,
    D_CLI_SEVERITY_WARNING = 1,
    D_CLI_SEVERITY_NOTE    = 2
};

// 1.3    Program flags
//------------------------------------------------------------------------------
// 1.3.1
// D_CLI_PROGRAM_COMMANDS
//   flag: the program has commands: the kit adds `help` and `completion` to
// them at the first parse, and a run that names none is a usage error.
#define D_CLI_PROGRAM_COMMANDS       (1u << 0)

// 1.3.2
// D_CLI_PROGRAM_RESPONSE_FILES
//   flag: the program reads response files, `@FILE` (R14), split as the
// platform's tools split them. Off unless asked for.
#define D_CLI_PROGRAM_RESPONSE_FILES (1u << 1)

// 1.4    Callbacks
//------------------------------------------------------------------------------
// 1.4.1
// d_cli_command_fn
//   type: a command's work, called once the parse has reached the command:
// the program, the command, and the context the command was added with. It
// returns the status the program exits with -- D_CLI_EXIT_OK, D_CLI_EXIT_NO
// or D_CLI_EXIT_TROUBLE -- having said what went wrong, if anything, through
// the reports of section 7.
struct d_cli_program;

typedef int (*d_cli_command_fn)(struct d_cli_program* _program,
                                struct d_arg_command* _command,
                                void*                 _context);


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    A command's work
//------------------------------------------------------------------------------
// 2.1.1
// d_cli_handler
//   struct: the function a command runs, its context, and what frees the
// context with the program; NULL `release` leaves it the caller's.
struct d_cli_handler
{
    struct d_arg_command* command;
    d_cli_command_fn      run;
    void*                 context;
    d_arg_release_fn      release;
};

// 2.2    The program
//------------------------------------------------------------------------------
// 2.2.1
// d_cli_program
//   struct: a program over a parser it does not own. `name` is the program's
// own name, as diagnostics begin; `version` what `--version` prints after it.
// `verbosity` is -1 after `-q`, else how many times `-v` was given, plus
// what the program started with; `colour` the d_cli_colour mode, and
// `colour_output` and `colour_error` whether each stream takes colour, as
// decided after the last parse. `out` and `err` are where results and
// diagnostics go. `handlers` holds each command's work. `colour_option` is
// `--color`, and `colour_alias` the `--colour` help does not show;
// `help_command` and `completion_command` are NULL until the first parse
// adds them. `arguments` holds the arguments the last parse was given,
// rewritten where `/?` stood. `status` is D_ARG_OK while the program is
// made whole, and otherwise why it is not: what d_cli_program_init failed
// with, kept past the release that failure makes, or the want of memory
// for a command's work -- so that a run of the program ends in trouble and
// says why, rather than run a program short of a command.
//   Public-payload layout: read any field; change one only through the calls
// below.
struct d_cli_program
{
    struct d_arg_parser*  parser;
    char*                 name;
    char*                 version;
    uint32_t              flags;
    int                   verbosity;
    int                   verbosity_base;
    int                   colour;
    bool                  colour_output;
    bool                  colour_error;
    struct d_pack_sink    out;
    struct d_pack_sink    err;
    struct d_cli_handler* handlers;
    size_t                handler_count;
    size_t                handler_capacity;
    struct d_arg_option*  quiet;
    struct d_arg_option*  verbose;
    struct d_arg_option*  colour_option;
    struct d_arg_option*  colour_alias;
    struct d_arg_command* help_command;
    struct d_arg_command* completion_command;
    const char**          arguments;
    size_t                argument_count;
    int                   status;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 3.  LIFETIME
//==============================================================================


// 3.1    Making and releasing
//------------------------------------------------------------------------------
// 3.1.1
// d_cli_program_init
/**
 * @brief Makes a program over a parser, and gives the parser what every
 *        djinterp program has.
 *
 * @note Call it right after d_arg_parser_init and before the program's own
 *       definitions: the rule that an option given twice takes its last
 *       value is set as a default, which reaches only options and commands
 *       made afterwards. It sets the help flag (`-h, --help`), the version
 *       flag, `-q`, `-v`, and `--color` with the `--colour` help does not
 *       show; the kit's formatter and failure message; the parser's output
 *       to d_cli_sink_output and d_cli_sink_error; turns off `/` options
 *       and, unless D_CLI_PROGRAM_RESPONSE_FILES is given, response files;
 *       and makes `--name=` an empty value, as getopt_long reads it. With
 *       D_CLI_PROGRAM_COMMANDS, the first parse adds the `help` and
 *       `completion` commands, after the program's own, so that help lists
 *       them last.
 *
 * @param[out]    _program  the program to make.
 * @param[in,out] _parser   the parser, made and empty; it must outlive the
 *                          program.
 * @param[in]     _name     the program's own name, copied.
 * @param[in]     _version  its version, copied; NULL or "" for none, and
 *                          then no `--version`.
 * @param[in]     _flags    D_CLI_PROGRAM_* flags.
 * @return D_ARG_OK when made; D_ARG_ERROR_OUT_OF_MEMORY when a copy or a
 *         definition could not be made, the program then released and the
 *         failure kept in its `status`; D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         program, parser or name, the program untouched; or a
 *         definition's own status, kept as memory's is.
 */
int         d_cli_program_init(struct d_cli_program* _program,
                               struct d_arg_parser*  _parser,
                               const char*           _name,
                               const char*           _version,
                               uint32_t              _flags);

// 3.1.2
// d_cli_program_release
/**
 * @brief Releases what a program holds: its copies, the arguments it kept,
 *        and each command's context it was given to release.
 *
 * @note The parser is the caller's, and is released after the program.
 *
 * @param[in,out] _program  the program; may be NULL.
 * @post   the program holds nothing; its handlers' contexts are freed.
 */
void        d_cli_program_release(struct d_cli_program* _program);


//==============================================================================
// 4.  COMMANDS
//==============================================================================


// 4.1    Adding
//------------------------------------------------------------------------------
// 4.1.1
// d_cli_program_add_command
/**
 * @brief Adds a command, and the work it does.
 *
 * @note The command takes the root's options after its name as well as
 *       before (fallthrough, R2), and a help flag of its own. One command
 *       runs (R1): a command takes at most one subcommand, as the root does,
 *       and the parser's subcommand fallthrough is off, so a word after the
 *       command that names another is the command's operand -- or a usage
 *       error -- never a second command. A command with no function -- one
 *       that only groups subcommands -- is a usage error when the parse
 *       stops at it.
 *
 * @param[in,out] _program      the program.
 * @param[in,out] _parent       the command to add it under; NULL for the
 *                              root.
 * @param[in]     _name         its name, a lowercase verb (R1).
 * @param[in]     _description  what it does, in lower case and with no full
 *                              stop, as the command list shows it.
 * @param[in]     _run          its work; NULL for none.
 * @param[in]     _context      what `_run` is given.
 * @param[in]     _release      what frees `_context` with the program, or at
 *                              once on failure; NULL leaves it borrowed.
 * @return the command, owned by its parent; NULL on failure, recorded as the
 *         parser's definition error as d_arg_command_add_subcommand records
 *         it, or, when the work could not be kept, as the program's
 *         `status`, D_ARG_ERROR_OUT_OF_MEMORY: either way the program's run
 *         ends in trouble and says why.
 */
struct d_arg_command*
            d_cli_program_add_command(struct d_cli_program* _program,
                                      struct d_arg_command* _parent,
                                      const char*           _name,
                                      const char*           _description,
                                      d_cli_command_fn      _run,
                                      void*                 _context,
                                      d_arg_release_fn      _release);

// 4.1.2
// d_cli_program_set_main
/**
 * @brief Gives the root command work of its own: what a program of one
 *        command, or a plugin, does once its arguments are parsed.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _run      the work; NULL for none.
 * @param[in]     _context  what `_run` is given.
 * @param[in]     _release  what frees `_context`; NULL leaves it borrowed.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY, the context then released
 *         and the failure kept as the program's `status`, which its run
 *         says; or D_ARG_ERROR_BAD_ARGUMENT for a NULL program.
 */
int         d_cli_program_set_main(struct d_cli_program* _program,
                                   d_cli_command_fn      _run,
                                   void*                 _context,
                                   d_arg_release_fn      _release);


//==============================================================================
// 5.  RUNNING
//==============================================================================


// 5.1    Parsing
//------------------------------------------------------------------------------
// 5.1.1
// d_cli_program_parse
/**
 * @brief Parses main's arguments, and ends a parse that called for an end.
 *
 * @note The first parse of a program with commands adds `help` and
 *       `completion` (d_cli_program_init). Every parse makes each named
 *       option take one value each time it is given (R8), a list being the
 *       option given again: CLI11 lets one bound to a vector go on taking
 *       the arguments after it. On Windows main's own arguments -- as many
 *       as the C runtime gave main, each the same text -- are taken again
 *       from the process's wide command line, as d_arg_parser_ensure_utf8
 *       recovers them; any others, a module's among them, are parsed as
 *       given, in UTF-8; and `/?` is read as `--help`. Help, a version and
 *       a completion script are written to the program's output and end
 *       the run with D_CLI_EXIT_OK; a failure is written to its error
 *       output, in the GNU shape, and ends it with D_CLI_EXIT_TROUBLE.
 *       Either way, and on success, the standard options' values are read
 *       first, so a failure is coloured as `--color` asked.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _argc     the argument count.
 * @param[in]     _argv     the arguments, `_argv[0]` the program's path;
 *                          they must outlive the values the parse gives.
 * @param[out]    _exit     receives the status to exit with when the parse
 *                          ended the run; untouched otherwise.
 * @return `true` when the run is over and the program is to exit with
 *         `*_exit`; `false` when the parse succeeded and the program goes on.
 *         A program whose d_cli_program_init failed ends at once with
 *         D_CLI_EXIT_TROUBLE, saying why to the error output it was given,
 *         or to standard error, by its parser's name, or `argv[0]`'s:
 *         `prog: error: out of memory`. One never made, zeroed, says
 *         nothing.
 */
bool        d_cli_program_parse(struct d_cli_program* _program,
                                int                   _argc,
                                const char* const*    _argv,
                                int*                  _exit);

// 5.2    Dispatching
//------------------------------------------------------------------------------
// 5.2.1
// d_cli_program_dispatch
//   running: after a parse that succeeded, calls the work of the deepest
// command the parse reached and returns its status. A command with no work,
// and a program with commands given none, is a usage error -- its usage and
// commands to the error output -- and D_CLI_EXIT_TROUBLE.
int         d_cli_program_dispatch(struct d_cli_program* _program);

// 5.2.2
// d_cli_program_run
//   running: d_cli_program_parse, then d_cli_program_dispatch: the status
// main returns. D_CLI_EXIT_TROUBLE for a NULL program.
int         d_cli_program_run(struct d_cli_program* _program,
                              int                   _argc,
                              const char* const*    _argv);

// 5.3    What the parse reached
//------------------------------------------------------------------------------
// 5.3.1
// d_cli_program_command
//   query: the deepest command the last parse reached -- the root when it
// reached no subcommand; NULL for a NULL program.
struct d_arg_command*
            d_cli_program_command(const struct d_cli_program* _program);


//==============================================================================
// 6.  SETTINGS AND QUERIES
//==============================================================================


// 6.1    Output
//------------------------------------------------------------------------------
// 6.1.1
// d_cli_program_set_output
//   setting: where the program's results (`_out`) and diagnostics (`_err`)
// go, the parser's help and failure messages with them; each sink must
// outlive its use. A test captures a program's output this way. Does not
// fail.
void        d_cli_program_set_output(struct d_cli_program* _program,
                                     struct d_pack_sink    _out,
                                     struct d_pack_sink    _err);

// 6.2    The standard options' values
//------------------------------------------------------------------------------
// 6.2.1
// d_cli_program_verbosity
//   query: -1 after `-q`; otherwise how many times `-v` was given, plus the
// verbosity the program was started with (d_cli_program_init_plugin); 0 for
// a NULL program.
int         d_cli_program_verbosity(const struct d_cli_program* _program);

// 6.2.2
// d_cli_program_colour
//   query: whether a stream, D_CLI_STREAM_OUTPUT or D_CLI_STREAM_ERROR, takes
// colour, as decided after the last parse; false for anything else.
bool        d_cli_program_colour(const struct d_cli_program* _program,
                                 int                         _stream);

// 6.3    Names
//------------------------------------------------------------------------------
// 6.3.1
// d_cli_command_path
/**
 * @brief Writes a command's full name, as a user types it: the root's
 *        name, then each command's down to it, a space apart --
 *        `bitwisedom gen`.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _out      the buffer it is appended to.
 * @return D_ARG_OK, the buffer's status when it could not grow, or
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int         d_cli_command_path(const struct d_arg_command* _command,
                               struct d_arg_buffer*        _out);


//==============================================================================
// 7.  DIAGNOSTICS
//==============================================================================
// Each is written whole to the program's error output, ended by a newline,
// as one write. `-q` drops warnings and notes. The message is printf's format
// and arguments; it starts in lower case and has no full stop (section 8).


// 7.1    Reports
//------------------------------------------------------------------------------
// 7.1.1
// d_cli_report / d_cli_report_at
//   reports: `PROGRAM: SEVERITY: MESSAGE`; and, at a place in a file,
// `FILE:LINE:COLUMN: SEVERITY: MESSAGE`, the column left out when
// `_column` is 0 or less and the line too when `_line` is. A NULL program
// writes nothing. Neither fails: a diagnostic that cannot be written is
// dropped.
void        d_cli_report(const struct d_cli_program* _program,
                         int                         _severity,
                         const char*                 _format,
                         ...) D_FORMAT_PRINTF(3, 4);
void        d_cli_report_at(const struct d_cli_program* _program,
                            int                         _severity,
                            const char*                 _file,
                            long                        _line,
                            long                        _column,
                            const char*                 _format,
                            ...) D_FORMAT_PRINTF(6, 7);

// 7.1.2
// d_cli_vreport
//   report: d_cli_report_at with its arguments in a va_list; `_file` NULL
// for none.
void        d_cli_vreport(const struct d_cli_program* _program,
                          int                         _severity,
                          const char*                 _file,
                          long                        _line,
                          long                        _column,
                          const char*                 _format,
                          va_list                     _arguments);

// 7.1.3
// d_cli_note
//   report: a note, written only when the verbosity is at least `_level`
// (1 for `-v`, 2 for `-vv`).
void        d_cli_note(const struct d_cli_program* _program,
                       int                         _level,
                       const char*                 _format,
                       ...) D_FORMAT_PRINTF(3, 4);

// 7.2    Reports that end a command
//------------------------------------------------------------------------------
// 7.2.1
// d_cli_usage_error
//   report: a usage error found by a command's work, in the shape the parser's
// take -- `PROGRAM: error: COMMAND: MESSAGE` and `Try 'PROGRAM COMMAND --help'
// for more information.` -- the command NULL for the root. Returns
// D_CLI_EXIT_TROUBLE, so that the work may `return` it.
int         d_cli_usage_error(const struct d_cli_program* _program,
                              const struct d_arg_command* _command,
                              const char*                 _format,
                              ...) D_FORMAT_PRINTF(3, 4);

// 7.2.2
// d_cli_file_error
//   report: `PROGRAM: error: cannot DOING 'PATH': REASON`, REASON the
// system's words for `_errnum`, and `standard input` or `standard output`
// for `-`, by whether `_doing` reads. Returns D_CLI_EXIT_TROUBLE.
int         d_cli_file_error(const struct d_cli_program* _program,
                             const char*                 _doing,
                             const char*                 _path,
                             int                         _errnum);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_C_CLI_PROGRAM_H
