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
* line for each option, a list's items separated by `|`. A callback adds
* lines of its own as it runs, which come before the report.
*   A run's parser writes help, versions and failure messages into the run's
* buffers rather than to the standard streams, so that a section can read
* them; and a run frees everything it holds when it ends, so that a section
* built with AddressSanitizer fails on a leak.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_tests_support.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  Limits
         1.  D_TESTS_ARG_WATCH_MAX
         2.  D_TESTS_ARG_ARGS_MAX
    2.  The run
         1.  d_tests_arg_command_watch
         2.  d_tests_arg_option_watch
         3.  d_tests_arg_run
2.  RUNS
    ----
    1.  Beginning and ending
         1.  d_tests_arg_begin
         2.  d_tests_arg_end
    2.  What is reported
         1.  d_tests_arg_watch_command / d_tests_arg_watch_option
         2.  d_tests_arg_watch_all
         3.  d_tests_arg_note
    3.  Parsing
         1.  d_tests_arg_parse
         2.  d_tests_arg_record
         3.  d_tests_arg_matches
         4.  d_tests_arg_expect
3.  COMPARING
    ---------
    1.  Text
         1.  d_tests_arg_same
4.  VALIDATORS
    ----------
    1.  Transcripts
         1.  d_tests_arg_render
         2.  d_tests_arg_transcribe
         3.  d_tests_arg_holds
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
#define D_TESTS_ARG_ARGS_MAX  32

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
// in the order named; the transcript; and what the parser wrote to its
// output (`out`) and error output (`err`). `failed` is set when a watch did
// not fit, so that the section fails rather than reports less.
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
    bool                             failed;
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
 * @post   the parser, its commands and options, and the buffers are freed.
 */
void d_tests_arg_end(struct d_tests_arg_run* _run);

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

// 2.3    Parsing
//------------------------------------------------------------------------------
// 2.3.1
// d_tests_arg_parse
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

// 2.3.2
// d_tests_arg_record
//   transcript: adds the report of a parse the section made itself -- of
// main's `argv`, a line, configuration text -- that came to `_status`.
void d_tests_arg_record(struct d_tests_arg_run* _run,
                        int                     _status);

// 2.3.3
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

// 2.3.4
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


//==============================================================================
// 3.  COMPARING
//==============================================================================


// 3.1    Text
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
// d_tests_arg_render
//   transcript: appends text with each control character written as its
// escape -- `\t`, `\n`, or `\x` and two hexadecimal digits -- so that a
// transcript line stays one line.
void d_tests_arg_render(struct d_arg_buffer* _out,
                        struct d_pack_text   _text);

// 4.1.2
// d_tests_arg_transcribe
//   transcript: appends a validator's transcript over values -- its
// description's line, then a line for each value -- to a buffer, without
// releasing the validator; for a section that compares several
// validators' transcripts as one.
void d_tests_arg_transcribe(struct d_arg_buffer*          _out,
                            const struct d_arg_validator* _validator,
                            const char* const*            _inputs,
                            size_t                        _count);

// 4.1.3
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
