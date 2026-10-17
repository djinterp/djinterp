/*******************************************************************************
* djinterp [parse]                                           arg_tests_support.h
*
* What the argument parser's C suites share: a parse to test, and what it
* came to, written out as text.
*   A section begins a run, defines options and subcommands on the run's
* parser, names the commands and options it wants reported, parses, and
* compares the run's transcript with the one expected. The transcript is the
* form in which this module was checked against CLI11 -- the same
* definitions and arguments given to both, and their states printed -- so an
* expected transcript is CLI11's own, but where a section says it departs
* from CLI11:
*
*     ERR <status> <exit code>: <message>       when the parse failed
*     cmd <label> n=<uses> miss=[...] subs=[...] order=[...]
*     opt <label> n=<values> res=[...]
*
* one `cmd` line for each command named, in the order named, then one `opt`
* line for each option, a list's items separated by `|`, each argument,
* value and message written as d_tests_arg_escape writes it. A callback adds
* lines of its own as it runs, which come before the report.
*   A run's parser writes help, versions and failure messages into the run's
* buffers rather than to the standard streams, so that a section can read
* them; and a run frees everything it holds when it ends, so that a section
* built with AddressSanitizer fails on a leak.
*   A section of many parses writes them as a case table: each case a tag,
* the function that makes its definitions, its arguments, and the
* environment and the files it parses with, each run on a parser of its own
* and written after a line naming it, the whole compared at once.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_tests_support.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  Limits
         1.  D_TESTS_ARG_WATCH_MAX
         2.  D_TESTS_ARG_ARGS_MAX
         3.  D_TESTS_ARG_CASE_ARGS
         4.  D_TESTS_ARG_KEEP_MAX
    2.  The run
         1.  d_tests_arg_command_watch
         2.  d_tests_arg_option_watch
         3.  d_tests_arg_run
    3.  Case tables
         1.  d_tests_arg_define_fn
         2.  d_tests_arg_env_var
         3.  d_tests_arg_file
         4.  d_tests_arg_case
2.  RUNS
    ----
    1.  Beginning and ending
         1.  d_tests_arg_begin
         2.  d_tests_arg_end
         3.  d_tests_arg_keep
    2.  What is reported
         1.  d_tests_arg_watch_command / d_tests_arg_watch_option
         2.  d_tests_arg_watch_all
         3.  d_tests_arg_note
         4.  d_tests_arg_make_option and kin
         5.  d_tests_arg_make_subcommand / d_tests_arg_make_group
         6.  d_tests_arg_command_label / d_tests_arg_option_label
    3.  Parsing
         1.  d_tests_arg_parse / d_tests_arg_parse_quiet
         2.  d_tests_arg_argv
         3.  d_tests_arg_record and kin
         4.  d_tests_arg_matches
         5.  d_tests_arg_expect
    4.  Case tables
         1.  d_tests_arg_cases
         2.  d_tests_arg_steps
    5.  The environment
         1.  d_tests_arg_setenv
3.  TEXT
    ----
    1.  Comparing
         1.  d_tests_arg_same
    2.  Writing
         1.  d_tests_arg_render
         2.  d_tests_arg_escape
4.  VALIDATORS
    ----------
    1.  Transcripts
         1.  d_tests_arg_transcribe
         2.  d_tests_arg_holds
    2.  In a parse
         1.  d_tests_arg_checked
         2.  d_tests_arg_validates
    3.  Completion
         1.  d_tests_arg_kind_of
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TESTS_SUPPORT_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TESTS_SUPPORT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser


//==============================================================================
// 1.  RECORDS
//==============================================================================


// 1.1    Limits
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_ARG_WATCH_MAX
//   constant: how many commands, and how many options, one run reports on;
// a section naming more is a section to split.
#define D_TESTS_ARG_WATCH_MAX 24

// 1.1.2
// D_TESTS_ARG_ARGS_MAX
//   constant: how many arguments one parse of a run takes.
#define D_TESTS_ARG_ARGS_MAX  64

// 1.1.3
// D_TESTS_ARG_CASE_ARGS
//   constant: the room for the arguments of one case of a case table, which
// end at the first NULL or fill the room.
#define D_TESTS_ARG_CASE_ARGS 20

// 1.1.4
// D_TESTS_ARG_KEEP_MAX
//   constant: how many blocks of storage one run keeps until it ends
// (d_tests_arg_keep): main's arguments, which a parse borrows, a variable an
// option is bound to, a formatter a command borrows; a case keeping more is
// a case to split.
#define D_TESTS_ARG_KEEP_MAX  16

// 1.2    The run
//------------------------------------------------------------------------------
// 1.2.1
// d_tests_arg_command_watch
//   struct: a command a run reports on, and the label its line carries.
struct d_tests_arg_command_watch
{
    const char*                 label;
    const struct d_arg_command* command;
};

// 1.2.2
// d_tests_arg_option_watch
//   struct: an option a run reports on, and the label its line carries.
struct d_tests_arg_option_watch
{
    const char*                label;
    const struct d_arg_option* option;
};

// 1.2.3
// d_tests_arg_run
//   struct: one test parse: the parser; the commands and options reported,
// in the order named; the transcript; what the parser wrote to its output
// (`out`) and error output (`err`); and the storage the run keeps for what
// must outlive its parser (`kept`), freed after it. `failed` is set when a
// watch or the storage did not fit, so that the section fails rather than
// reports less.
//   The parser points into the run, so a run is never copied: a section
// declares one and passes its address.
struct d_tests_arg_run
{
    struct d_arg_parser              parser;
    struct d_tests_arg_command_watch commands[D_TESTS_ARG_WATCH_MAX];
    size_t                           command_count;
    struct d_tests_arg_option_watch  options[D_TESTS_ARG_WATCH_MAX];
    size_t                           option_count;
    struct d_arg_buffer              transcript;
    struct d_arg_buffer              out;
    struct d_arg_buffer              err;
    void*                            kept[D_TESTS_ARG_KEEP_MAX];
    size_t                           kept_count;
    bool                             failed;
};

// 1.3    Case tables
//------------------------------------------------------------------------------
// 1.3.1
// d_tests_arg_define_fn
//   type: makes one case's definitions on the root of the case's run,
// naming what the case reports with the run's watch and make calls.
typedef void (*d_tests_arg_define_fn)(struct d_tests_arg_run* _run,
                                      struct d_arg_command*   _app);

// 1.3.2
// d_tests_arg_env_var
//   struct: an environment variable a case parses with: its name and its
// value, which is not empty, since Windows cannot hold an empty one. A list
// of them ends at a NULL name.
struct d_tests_arg_env_var
{
    const char* name;
    const char* value;
};

// 1.3.3
// d_tests_arg_file
//   struct: a file a case parses with: its name, relative to the scratch
// directory the case works in, and the text it holds. A list of them ends
// at a NULL name.
struct d_tests_arg_file
{
    const char* name;
    const char* content;
};

// 1.3.4
// d_tests_arg_case
//   struct: one case of a case table: its tag; the function that makes its
// definitions; the arguments it parses, which end at the first NULL; the
// environment variables set while it parses, NULL for none; and the files
// it parses with, NULL for none, made in a scratch directory the case works
// in (arg_tests_files.h).
struct d_tests_arg_case
{
    const char*                       tag;
    d_tests_arg_define_fn             define;
    const char*                       args[D_TESTS_ARG_CASE_ARGS];
    const struct d_tests_arg_env_var* variables;
    const struct d_tests_arg_file*    files;
};


//==============================================================================
// 2.  RUNS
//==============================================================================


// 2.1    Beginning and ending
//------------------------------------------------------------------------------
// 2.1.1
// d_tests_arg_begin
/**
 * @brief Begins a run: a parser as the module was checked against CLI11.
 *
 * @note The parser is made as CLI11's App("d", "prog"), with suggestions off
 *       -- CLI11 has none -- and its output and error output going to the
 *       run's buffers. Its root is reported first, labelled "app".
 *
 * @param[out] _run  the run; whatever it held is ignored.
 * @return the root command; NULL when the parser could not be made, which
 *         fails the run.
 */
struct d_arg_command*
     d_tests_arg_begin(struct d_tests_arg_run* _run);

// 2.1.2
// d_tests_arg_end
/**
 * @brief Ends a run, freeing everything it holds.
 *
 * @param[in,out] _run  the run.
 * @post   the parser, its commands and options, the buffers and the storage
 *         the run kept are freed.
 */
void d_tests_arg_end(struct d_tests_arg_run* _run);

// 2.1.3
// d_tests_arg_keep
/**
 * @brief Storage the run keeps until it ends, after its parser: for what
 *        the parser borrows -- main's arguments, a variable an option is
 *        bound to, a formatter a command uses.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _size  how many bytes, aligned for any type.
 * @return the storage, its bytes as malloc leaves them; NULL, failing the
 *         run, when it cannot be had or the run keeps D_TESTS_ARG_KEEP_MAX
 *         blocks already.
 */
void*
     d_tests_arg_keep(struct d_tests_arg_run* _run,
                      size_t                  _size);

// 2.2    What is reported
//------------------------------------------------------------------------------
// 2.2.1
// d_tests_arg_watch_command / d_tests_arg_watch_option
//   registration: names a command, or an option, for the report, under a
// label; each returns what it was given, so a definition can be wrapped in
// it. A NULL command or option -- a definition that failed -- fails the run.
struct d_arg_command*
     d_tests_arg_watch_command(struct d_tests_arg_run* _run,
                               const char*             _label,
                               struct d_arg_command*   _command);
struct d_arg_option*
     d_tests_arg_watch_option(struct d_tests_arg_run* _run,
                              const char*             _label,
                              struct d_arg_option*    _option);

// 2.2.2
// d_tests_arg_watch_all
//   registration: d_tests_arg_watch_option for an option that is also made
// to keep every value it is given -- CLI11's MultiOptionPolicy::TakeAll --
// as the checks against CLI11 made the options they gave several values,
// so that none of them is an error.
struct d_arg_option*
     d_tests_arg_watch_all(struct d_tests_arg_run* _run,
                           const char*             _label,
                           struct d_arg_option*    _option);

// 2.2.3
// d_tests_arg_note
//   transcript: adds a line, `printf`-style, to the transcript; for
// callbacks, which are given the run as their context.
void d_tests_arg_note(struct d_tests_arg_run* _run,
                      const char*             _format,
                      ...) D_FORMAT_PRINTF(2, 3);

// 2.2.4
// d_tests_arg_make_option and kin
//   registration: makes an option on a command, with no description, and
// watches it under a label: an option of the names given (make_option), the
// same keeping every value it is given (make_all, as d_tests_arg_watch_all
// does), or a flag (make_flag). Each returns the option, or NULL, which
// fails the run, when it could not be made -- also on a NULL command.
struct d_arg_option*
     d_tests_arg_make_option(struct d_tests_arg_run* _run,
                             const char*             _label,
                             struct d_arg_command*   _command,
                             const char*             _names);
struct d_arg_option*
     d_tests_arg_make_all(struct d_tests_arg_run* _run,
                          const char*             _label,
                          struct d_arg_command*   _command,
                          const char*             _names);
struct d_arg_option*
     d_tests_arg_make_flag(struct d_tests_arg_run* _run,
                           const char*             _label,
                           struct d_arg_command*   _command,
                           const char*             _names);

// 2.2.5
// d_tests_arg_make_subcommand / d_tests_arg_make_group
//   registration: makes a subcommand, or an option group, of a command, with
// no description, and watches it under its name. Each returns the command,
// or NULL, which fails the run, when it could not be made -- also on a NULL
// parent.
struct d_arg_command*
     d_tests_arg_make_subcommand(struct d_tests_arg_run* _run,
                                 struct d_arg_command*   _parent,
                                 const char*             _name);
struct d_arg_command*
     d_tests_arg_make_group(struct d_tests_arg_run* _run,
                            struct d_arg_command*   _parent,
                            const char*             _name);

// 2.2.6
// d_tests_arg_command_label / d_tests_arg_option_label
//   query: the label a run reports a command, or an option, under -- what a
// callback's note names it by, as the reference programs' callbacks name
// theirs; "?" for one the run does not report.
const char* d_tests_arg_command_label(const struct d_tests_arg_run* _run,
                                      const struct d_arg_command*   _command);
const char* d_tests_arg_option_label(const struct d_tests_arg_run* _run,
                                     const struct d_arg_option*    _option);

// 2.3    Parsing
//------------------------------------------------------------------------------
// 2.3.1
// d_tests_arg_parse / d_tests_arg_parse_quiet
/**
 * @brief Parses arguments with the run's parser and adds the report to the
 *        transcript.
 *
 * @note The arguments follow the program's name, as `argv[1]` on; they are
 *       given as d_arg_parser_parse_args takes them.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _args   the arguments; may be NULL when `_count` is 0.
 * @param[in]     _count  how many, at most D_TESTS_ARG_ARGS_MAX.
 * @return the parse's status.
 */
int  d_tests_arg_parse(struct d_tests_arg_run* _run,
                       const char* const*      _args,
                       size_t                  _count);
/**
 * @brief Parses arguments with the run's parser as d_tests_arg_parse does,
 *        adding nothing to the transcript, for a section that writes what
 *        the parse came to in its own way.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _args   the arguments; may be NULL when `_count` is 0.
 * @param[in]     _count  how many, at most D_TESTS_ARG_ARGS_MAX.
 * @return the parse's status.
 */
int  d_tests_arg_parse_quiet(struct d_tests_arg_run* _run,
                             const char* const*      _args,
                             size_t                  _count);

// 2.3.2
// d_tests_arg_argv
/**
 * @brief Builds main's arguments in storage the run keeps until it ends, so
 *        that what a parse of them borrows outlives the parse.
 *
 * @note The words are the text split at each space, so two spaces in a row
 *       make an empty word, and an empty text has none.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _program  the program's name, `argv[0]`; NULL for none,
 *                          the words alone, as d_arg_parser_parse_args
 *                          takes them.
 * @param[in]     _words    the words after it.
 * @param[out]    _argc     receives how many arguments were built.
 * @return the arguments, a NULL after the last, as main's; NULL, failing
 *         the run, for more than D_TESTS_ARG_ARGS_MAX arguments, more than
 *         D_TESTS_ARG_KEEP_MAX blocks, or storage that cannot be had.
 */
char* const*
     d_tests_arg_argv(struct d_tests_arg_run* _run,
                      const char*             _program,
                      const char*             _words,
                      int*                    _argc);

// 2.3.3
// d_tests_arg_record and kin
//   transcript: adds the report of a parse the section made itself -- of
// main's `argv`, a line, configuration text -- that came to `_status`: its
// ERR line, when it did not come to D_ARG_OK, then the `cmd` and `opt`
// lines. record_error adds the ERR line alone and report the lines after
// it, for a section that writes more between the two, as the parser
// suite's reference writes the root's name.
void d_tests_arg_record(struct d_tests_arg_run* _run,
                        int                     _status);
void d_tests_arg_record_error(struct d_tests_arg_run* _run,
                              int                     _status);
void d_tests_arg_report(struct d_tests_arg_run* _run);

// 2.3.4
// d_tests_arg_matches
/**
 * @brief Compares the transcript with the one expected, then empties it.
 *
 * @note A mismatch is printed, expected above what was written.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _expected  the transcript expected, each line ending in a
 *                           newline.
 * @return whether the two are the same and the run has not failed.
 */
bool d_tests_arg_matches(struct d_tests_arg_run* _run,
                         const char*             _expected);

// 2.3.5
// d_tests_arg_expect
/**
 * @brief Parses, compares, and ends the run: d_tests_arg_parse,
 *        d_tests_arg_matches and d_tests_arg_end in one call, the whole of
 *        most sections after their definitions.
 *
 * @param[in,out] _run       the run, ended on return.
 * @param[in]     _args      the arguments.
 * @param[in]     _count     how many.
 * @param[in]     _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
bool d_tests_arg_expect(struct d_tests_arg_run* _run,
                        const char* const*      _args,
                        size_t                  _count,
                        const char*             _expected);

// 2.4    Case tables
//------------------------------------------------------------------------------
// 2.4.1
// d_tests_arg_cases
/**
 * @brief Runs a case table and compares what its cases came to with the
 *        transcript expected.
 *
 * @note Each case is made on a run of its own and written as a line `<tag>
 *       [<arguments>]`, its arguments separated by spaces, followed by what
 *       the run's transcript then holds: what its callbacks noted, and the
 *       report of its parse. Its environment variables are set for the parse
 *       and removed after it; its files are made, before its definitions, in
 *       a scratch directory it works in, which is removed after it.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _cases     the cases; may be NULL when `_count` is 0.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every case ran -- its definitions made, its watches,
 *         notes, variables and files kept -- and the transcript was as
 *         expected.
 */
bool d_tests_arg_cases(const char*                    _name,
                       const struct d_tests_arg_case* _cases,
                       size_t                         _count,
                       const char*                    _expected);

// 2.4.2
// d_tests_arg_steps
/**
 * @brief Runs a table of steps -- cases that each write their own lines --
 *        and compares what they wrote with the transcript expected.
 *
 * @note Each case is given a run of its own and the run's root, and writes
 *       into the run's transcript what its section's reference writes.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _steps     the cases; may be NULL when `_count` is 0.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every case ran -- its run made, its watches, notes and
 *         storage kept -- and the transcript was as expected.
 */
bool d_tests_arg_steps(const char*                  _name,
                       const d_tests_arg_define_fn* _steps,
                       size_t                       _count,
                       const char*                  _expected);

// 2.5    The environment
//------------------------------------------------------------------------------
// 2.5.1
// d_tests_arg_setenv
/**
 * @brief Sets an environment variable of the process, or removes it.
 *
 * @param[in] _name   the variable's name.
 * @param[in] _value  its value, not empty; NULL removes the variable.
 * @return whether the environment was changed; `false` also on a system
 *         whose environment the suites cannot change.
 */
bool d_tests_arg_setenv(const char* _name,
                        const char* _value);


//==============================================================================
// 3.  TEXT
//==============================================================================


// 3.1    Comparing
//------------------------------------------------------------------------------
// 3.1.1
// d_tests_arg_same
/**
 * @brief Compares text with the text expected, printing a mismatch.
 *
 * @param[in] _what      what the text is, for the printed mismatch.
 * @param[in] _got       the text.
 * @param[in] _expected  the text expected; NULL expects none at all.
 * @return whether the two are the same.
 */
bool d_tests_arg_same(const char*        _what,
                      struct d_pack_text _got,
                      const char*        _expected);

// 3.2    Writing
//------------------------------------------------------------------------------
// 3.2.1
// d_tests_arg_render
//   transcript: appends text with each control character written as its
// escape -- `\t`, `\n`, or `\x` and two hexadecimal digits -- so that a
// transcript line stays one line.
void d_tests_arg_render(struct d_arg_buffer* _out,
                        struct d_pack_text   _text);

// 3.2.2
// d_tests_arg_escape
//   transcript: appends text as d_tests_arg_render does, and each backslash
// as `\\`, so that the text written can be read back: a tab apart from a
// backslash and a `t`, a NUL byte apart from the end of the text. How a
// report writes arguments, values and messages.
void d_tests_arg_escape(struct d_arg_buffer* _out,
                        struct d_pack_text   _text);


//==============================================================================
// 4.  VALIDATORS
//==============================================================================
// A validator's transcript is the form in which the module's validators were
// checked against CLI11's: a line `desc <description>`, then a line for each
// value, `[value] -> pass <what it became>`, `[value] -> fail <why>`, or
// `[value] -> status <message>` when the validator stopped with an error, as
// CLI11's throws, the message being whatever it wrote before it stopped.
// Values are validated under CLI11's rule for leading zeros.


// 4.1    Transcripts
//------------------------------------------------------------------------------
// 4.1.1
// d_tests_arg_transcribe
//   transcript: appends a validator's transcript over values -- its
// description's line, then a line for each value -- to a buffer, without
// releasing the validator; for a section that compares several
// validators' transcripts as one.
void d_tests_arg_transcribe(struct d_arg_buffer*          _out,
                            const struct d_arg_validator* _validator,
                            const char* const*            _inputs,
                            size_t                        _count);

// 4.1.2
// d_tests_arg_holds
/**
 * @brief Runs a validator over values and compares its transcript with the
 *        one expected.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the validator, consumed: released on return.
 * @param[in] _inputs     the values.
 * @param[in] _count      how many.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
bool d_tests_arg_holds(const char*            _what,
                       struct d_arg_validator _validator,
                       const char* const*     _inputs,
                       size_t                 _count,
                       const char*            _expected);

// 4.2    In a parse
//------------------------------------------------------------------------------
// 4.2.1
// d_tests_arg_checked
//   struct: an option a validator judges in a parse, as CLI11's
// `add_option(name, variable)` followed by check() or transform(): its name,
// the built-in type of its variable (a d_arg_builtin value), and whether the
// validator transforms -- its results replacing the values -- rather than
// only checks.
struct d_tests_arg_checked
{
    const char* name;
    int         type;
    bool        transform;
};

// 4.2.2
// d_tests_arg_validates
/**
 * @brief Parses `<name> <value>` with one option validated as described,
 *        and compares the transcript with the one expected.
 *
 * @note The option is reported under its name, and the report is followed
 *       by a line `value [<variable>]`: the variable, which starts as its
 *       type's zero, written as its type writes values.
 *
 * @param[in] _option     the option.
 * @param[in] _validator  the validator, consumed: the option owns it.
 * @param[in] _value      the value given after the option's name.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
bool d_tests_arg_validates(const struct d_tests_arg_checked* _option,
                           struct d_arg_validator            _validator,
                           const char*                       _value,
                           const char*                       _expected);

// 4.3    Completion
//------------------------------------------------------------------------------
// 4.3.1
// d_tests_arg_kind_of
//   query: what a validator completes as -- its D_ARG_VALIDATOR_PATH,
// _DIRECTORY and _NUMBER flags -- after which it is released.
unsigned int d_tests_arg_kind_of(struct d_arg_validator _validator);


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TESTS_SUPPORT_H
