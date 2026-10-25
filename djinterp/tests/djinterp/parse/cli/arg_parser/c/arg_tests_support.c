/*******************************************************************************
* djinterp [parse]                                           arg_tests_support.c
*
* The runs the argument parser's C suites share, declared in
* arg_tests_support.h: a parser made as the module was checked against
* CLI11, the report of a parse in the probes' transcript form, the
* comparison that prints what differed, and the case tables written over
* them, whose files are made in the scratch directories of
* arg_tests_files.h.
*   Setting an environment variable is the one thing here the system is
* asked: POSIX's setenv where the environment layer finds POSIX's file
* status, which goes with it, and the Microsoft runtime's _putenv_s where it
* finds Windows. Needs POSIX.1-2008 where the C library hides setenv in a
* strict ISO mode, which the ladders' _XOPEN_SOURCE=700 gives.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_tests_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_tests_support.h"  // corresponding header
// std
#include <stdarg.h>   // va_list, va_start, va_end
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf, vsnprintf
#include <stdlib.h>   // malloc, free, setenv, unsetenv, _putenv_s
#include <string.h>   // memchr, memcmp, memcpy, strcspn, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_PARSER_STAT
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
#include "./arg_tests_files.h"  // scratch directories, for a case's files
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uintmax_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// NOTE_MAX
//   constant: the room for one line d_tests_arg_note adds, its terminator
// included; a line that does not fit fails the run rather than being cut
// short.
enum
{
    NOTE_MAX = 256
};

// d_tests_arg_variable
//   union: room for a variable of any built-in type, for
// d_tests_arg_validates.
union d_tests_arg_variable
{
    long double        real;
    uintmax_t          whole;
    const char*        text;
    struct d_pack_text span;
};

//==============================================================================
// 1.  REPORTING
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

/**
 * @brief Keeps a parser's messages CLI11's.
 *
 * @param[in,out] _parser  the parser.
 *
 * Turns the parser's suggestions off, so that its messages are CLI11's.
 */
static void
d_tests_arg_quiet(
    struct d_arg_parser* _parser
)
{
    d_arg_parser_set_suggestions(_parser,
                                 false);

    return;
}

#else

/**
 * @brief Keeps a parser's messages CLI11's.
 *
 * @param[in,out] _parser  the parser.
 *
 * Nothing to turn off: a build without suggestions has CLI11's messages.
 */
static void
d_tests_arg_quiet(
    struct d_arg_parser* _parser
)
{
    (void)_parser;

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

/**
 * @brief Writes a watched command's `cmd` line to a run's transcript.
 *
 * @param[in,out] _run    the run, whose transcript gains the line.
 * @param[in]     _watch  the watch: the command and its label.
 *
 * One `cmd` line: the uses, the arguments the command did not use, the
 * subcommands it parsed, and its options in the order met.
 */
static void
d_tests_arg_report_command(
    struct d_tests_arg_run*                 _run,
    const struct d_tests_arg_command_watch* _watch
)
{
    const struct d_arg_command* const command = _watch->command;
    struct d_arg_buffer* const        out     = &_run->transcript;

    (void)d_arg_buffer_printf(out,
                              "cmd %s n=%lu miss=[",
                              _watch->label,
                              (unsigned long)command->parsed);

    // the arguments not used
    for (size_t index = 0u; index < command->missing.count; ++index)
    {
        (void)d_arg_buffer_append_cstr(out,
                                       (index > 0u) ? "|" : "");
        d_tests_arg_escape(out,
                           command->missing.items[index].text);
    }

    (void)d_arg_buffer_append_cstr(out,
                                   "] subs=[");

    // the subcommands parsed, groups by their display names
    for (size_t index = 0u;
         index < command->parsed_subcommands.count;
         ++index)
    {
        (void)d_arg_buffer_append_cstr(out,
                                       (index > 0u) ? "|" : "");
        (void)d_arg_command_display_name(
                  command->parsed_subcommands.items[index],
                  false,
                  out);
    }

    (void)d_arg_buffer_append_cstr(out,
                                   "] order=[");

    // the options in the order met
    for (size_t index = 0u; index < command->parse_order.count; ++index)
    {
        (void)d_arg_buffer_append_cstr(out,
                                       (index > 0u) ? "|" : "");
        (void)d_arg_option_name(command->parse_order.items[index],
                                0u,
                                out);
    }

    (void)d_arg_buffer_append_cstr(out,
                                   "]\n");

    return;
}

/**
 * @brief Writes a watched option's `opt` line to a run's transcript.
 *
 * @param[in,out] _run    the run, whose transcript gains the line.
 * @param[in]     _watch  the watch: the option and its label.
 *
 * One `opt` line: how many values the option was given, and the values as
 * given, escaped.
 */
static void
d_tests_arg_report_option(
    struct d_tests_arg_run*                _run,
    const struct d_tests_arg_option_watch* _watch
)
{
    const unsigned long             given   =
        (unsigned long)d_arg_option_count(_watch->option);
    size_t                          count   = 0u;
    const struct d_pack_text* const results =
        d_arg_option_results(_watch->option,
                             &count);

    (void)d_arg_buffer_printf(&_run->transcript,
                              "opt %s n=%lu res=[",
                              _watch->label,
                              given);

    // each value as given
    for (size_t index = 0u; index < count; ++index)
    {
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       (index > 0u) ? "|" : "");
        d_tests_arg_escape(&_run->transcript,
                           results[index]);
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");

    return;
}

/**
 * @brief Prints text under a heading, line by line.
 *
 * @param[in] _heading  the heading.
 * @param[in] _text     the text.
 *
 * Prints text under a heading, each line indented and marked, so that an
 * empty line and a line's trailing spaces can be seen.
 */
static void
d_tests_arg_show(
    const char*        _heading,
    struct d_pack_text _text
)
{
    size_t start = 0u;

    printf("    %s\n",
           _heading);

    // each line, the last whether or not a newline ends it
    while (start < _text.length)
    {
        const char* const line = _text.data + start;
        const char* const end  = memchr(line,
                                        '\n',
                                        _text.length - start);
        const size_t      size = (end != NULL)
                                 ? (size_t)(end - line)
                                 : (_text.length - start);

        printf("      |%.*s|\n",
               (int)size,
               line);
        start += size + ((end != NULL) ? 1u : 0u);
    }

    return;
}

//==============================================================================
// 2.  RUNS
//==============================================================================

/*
d_tests_arg_begin
  The parser is made first, so that a failure to make it -- which leaves it
releasable -- can fail the run; the root is reported as "app".
*/
struct d_arg_command*
d_tests_arg_begin(
    struct d_tests_arg_run* _run
)
{
    // validate the parameters
    if (!_run)
    {
        return NULL;
    }

    const int status = d_arg_parser_init(&_run->parser,
                                         "d",
                                         "prog");

    _run->command_count = 0u;
    _run->option_count  = 0u;
    _run->kept_count    = 0u;
    _run->failed        = (status != (int)D_ARG_OK);
    d_arg_buffer_init(&_run->transcript);
    d_arg_buffer_init(&_run->out);
    d_arg_buffer_init(&_run->err);
    d_tests_arg_quiet(&_run->parser);
    d_arg_parser_set_output(&_run->parser,
                            d_arg_buffer_sink(&_run->out),
                            d_arg_buffer_sink(&_run->err));

    // the root, reported first; none when the parser could not be made
    return (status == (int)D_ARG_OK)
           ? d_tests_arg_watch_command(_run,
                                       "app",
                                       &_run->parser.command)
           : NULL;
}

/*
d_tests_arg_end
  The parser first, since its sinks write into the buffers and its values
borrow the storage kept.
*/
void
d_tests_arg_end(
    struct d_tests_arg_run* _run
)
{
    // validate the parameters
    if (!_run)
    {
        return;
    }

    d_arg_parser_release(&_run->parser);
    d_arg_buffer_release(&_run->transcript);
    d_arg_buffer_release(&_run->out);
    d_arg_buffer_release(&_run->err);

    // each block kept for what the parses borrowed
    for (size_t index = 0u; index < _run->kept_count; ++index)
    {
        free(_run->kept[index]);
        _run->kept[index] = NULL;
    }

    _run->command_count = 0u;
    _run->option_count  = 0u;
    _run->kept_count    = 0u;

    return;
}

/*
d_tests_arg_keep
  A block of the run's, freed by d_tests_arg_end after the parser.
*/
void*
d_tests_arg_keep(
    struct d_tests_arg_run* _run,
    size_t                  _size
)
{
    // validate the parameters
    if (!_run)
    {
        return NULL;
    }

    void* const block = (_run->kept_count < (size_t)D_TESTS_ARG_KEEP_MAX)
                        ? malloc(_size)
                        : NULL;

    // a block that could not be had, or kept, fails the run
    if (!block)
    {
        _run->failed = true;

        return NULL;
    }

    _run->kept[_run->kept_count] = block;
    _run->kept_count            += 1u;

    return block;
}

/*
d_tests_arg_watch_command
  A command that is not there, or that does not fit, fails the run.
*/
struct d_arg_command*
d_tests_arg_watch_command(
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_command*   _command
)
{
    // validate the parameters
    if (!_run)
    {
        return _command;
    }

    // a failed definition, or one past the room, fails the run
    if ( (!_command) ||
         (_run->command_count >= (size_t)D_TESTS_ARG_WATCH_MAX) )
    {
        _run->failed = true;

        return _command;
    }

    _run->commands[_run->command_count].label   = _label;
    _run->commands[_run->command_count].command = _command;
    ++_run->command_count;

    return _command;
}

/*
d_tests_arg_watch_option
  An option that is not there, or that does not fit, fails the run.
*/
struct d_arg_option*
d_tests_arg_watch_option(
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_option*    _option
)
{
    // validate the parameters
    if (!_run)
    {
        return _option;
    }

    // a failed definition, or one past the room, fails the run
    if ( (!_option) ||
         (_run->option_count >= (size_t)D_TESTS_ARG_WATCH_MAX) )
    {
        _run->failed = true;

        return _option;
    }

    _run->options[_run->option_count].label  = _label;
    _run->options[_run->option_count].option = _option;
    ++_run->option_count;

    return _option;
}

/*
d_tests_arg_watch_all
  The policy is set on whatever the watch hands back; a NULL option ignores
it, having failed the run already.
*/
struct d_arg_option*
d_tests_arg_watch_all(
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_option*    _option
)
{
    struct d_arg_option* const option = d_tests_arg_watch_option(_run,
                                                                 _label,
                                                                 _option);

    d_arg_option_set_multi_option_policy(option,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    return option;
}

/*
d_tests_arg_make_option
  d_arg_command_add_option without a description, then watched.
*/
struct d_arg_option*
d_tests_arg_make_option(
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_command*   _command,
    const char*             _names
)
{
    return d_tests_arg_watch_option(_run,
                                    _label,
                                    d_arg_command_add_option(_command,
                                                             _names,
                                                             NULL));
}

/*
d_tests_arg_make_all
  d_arg_command_add_option without a description, then watched keeping
every value.
*/
struct d_arg_option*
d_tests_arg_make_all(
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_command*   _command,
    const char*             _names
)
{
    return d_tests_arg_watch_all(_run,
                                 _label,
                                 d_arg_command_add_option(_command,
                                                          _names,
                                                          NULL));
}

/*
d_tests_arg_make_flag
  d_arg_command_add_flag without a description, then watched.
*/
struct d_arg_option*
d_tests_arg_make_flag(
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_command*   _command,
    const char*             _names
)
{
    return d_tests_arg_watch_option(_run,
                                    _label,
                                    d_arg_command_add_flag(_command,
                                                           _names,
                                                           NULL));
}

/*
d_tests_arg_make_subcommand
  d_arg_command_add_subcommand without a description, then watched under
its name.
*/
struct d_arg_command*
d_tests_arg_make_subcommand(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _parent,
    const char*             _name
)
{
    return d_tests_arg_watch_command(_run,
                                     _name,
                                     d_arg_command_add_subcommand(_parent,
                                                                  _name,
                                                                  NULL));
}

/*
d_tests_arg_make_group
  d_arg_command_add_option_group without a description, then watched under
its name.
*/
struct d_arg_command*
d_tests_arg_make_group(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _parent,
    const char*             _name
)
{
    return d_tests_arg_watch_command(_run,
                                     _name,
                                     d_arg_command_add_option_group(_parent,
                                                                    _name,
                                                                    NULL));
}

/*
d_tests_arg_command_label
  The first watch of the command, in the order named.
*/
const char*
d_tests_arg_command_label(
    const struct d_tests_arg_run* _run,
    const struct d_arg_command*   _command
)
{
    // validate the parameters
    if (!_run)
    {
        return "?";
    }

    // each command watched
    for (size_t index = 0u; index < _run->command_count; ++index)
    {
        // the command's watch
        if (_run->commands[index].command == _command)
        {
            return _run->commands[index].label;
        }
    }

    return "?";
}

/*
d_tests_arg_option_label
  The first watch of the option, in the order named.
*/
const char*
d_tests_arg_option_label(
    const struct d_tests_arg_run* _run,
    const struct d_arg_option*    _option
)
{
    // validate the parameters
    if (!_run)
    {
        return "?";
    }

    // each option watched
    for (size_t index = 0u; index < _run->option_count; ++index)
    {
        // the option's watch
        if (_run->options[index].option == _option)
        {
            return _run->options[index].label;
        }
    }

    return "?";
}

/*
d_tests_arg_note
  Formatted into a line of NOTE_MAX bytes at most, then added with its
newline.
*/
void
d_tests_arg_note(
    struct d_tests_arg_run* _run,
    const char*             _format,
    ...
)
{
    // validate the parameters
    if ( (!_run) ||
         (!_format) )
    {
        return;
    }

    char    line[NOTE_MAX] = { 0 };
    va_list arguments;

    va_start(arguments,
             _format);

    const int length = vsnprintf(line,
                                 sizeof(line),
                                 _format,
                                 arguments);

    va_end(arguments);

    // a line that did not fit fails the run
    if ( (length < 0) ||
         (length >= (int)NOTE_MAX) )
    {
        _run->failed = true;

        return;
    }

    (void)d_arg_buffer_append(&_run->transcript,
                              line,
                              (size_t)length);
    (void)d_arg_buffer_append_char(&_run->transcript,
                                   '\n');

    return;
}

/*
d_tests_arg_parse_quiet
  The arguments as spans over the caller's strings, which outlive the
parse's values, parsed without the program's name.
*/
int
d_tests_arg_parse_quiet(
    struct d_tests_arg_run* _run,
    const char* const*      _args,
    size_t                  _count
)
{
    // validate the parameters
    if (!_run)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // more arguments than there is room for, or none to read, fail the run
    if ( (_count > (size_t)D_TESTS_ARG_ARGS_MAX) ||
         ( (!_args) &&
           (_count > 0u) ) )
    {
        _run->failed = true;

        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_pack_text texts[D_TESTS_ARG_ARGS_MAX] = { { NULL, 0u } };

    // each argument as a span
    for (size_t index = 0u; index < _count; ++index)
    {
        texts[index] = d_pack_text_from_cstr(_args[index]);
    }

    return d_arg_parser_parse_args(&_run->parser,
                                   texts,
                                   _count,
                                   0u);
}

/*
d_tests_arg_parse
  The quiet parse, then its report; d_tests_arg_record ignores a NULL run.
*/
int
d_tests_arg_parse(
    struct d_tests_arg_run* _run,
    const char* const*      _args,
    size_t                  _count
)
{
    const int status = d_tests_arg_parse_quiet(_run,
                                               _args,
                                               _count);

    d_tests_arg_record(_run,
                       status);

    return status;
}

/**
 * @brief Counts the words of a text split at each space, as d_tests_arg_argv
 *        splits it: one more than its spaces, none for an empty text.
 *
 * @param[in] _words  the text.
 * @return how many words it holds.
 */
static size_t
d_tests_arg_word_count(
    const char* _words
)
{
    size_t count = (_words[0] != '\0') ? 1u : 0u;

    // one more after each space
    for (const char* cursor = _words; *cursor != '\0'; ++cursor)
    {
        count += (*cursor == ' ') ? 1u : 0u;
    }

    return count;
}

/**
 * @brief Splits a text where it lies, at each space, into its words: each
 *        word ended where its space was.
 *
 * @param[out]    _words  receives a pointer to each word.
 * @param[in,out] _text   the text, its spaces overwritten.
 * @param[in]     _count  how many words it holds, as d_tests_arg_word_count
 *                        counts them.
 */
static void
d_tests_arg_split(
    char** _words,
    char*  _text,
    size_t _count
)
{
    char* cursor = _text;

    // each word, ended where its space was
    for (size_t index = 0u; index < _count; ++index)
    {
        char* const end = cursor + strcspn(cursor,
                                           " ");

        _words[index] = cursor;
        cursor        = end + 1;
        *end          = '\0';
    }

    return;
}

/*
d_tests_arg_argv
  One block the run keeps: the pointers, a NULL after the last, then the
program's name and the words.
*/
char* const*
d_tests_arg_argv(
    struct d_tests_arg_run* _run,
    const char*             _program,
    const char*             _words,
    int*                    _argc
)
{
    // validate the parameters
    if ( (!_run)   ||
         (!_words) ||
         (!_argc) )
    {
        return NULL;
    }

    const size_t named   = (_program != NULL) ? 1u : 0u;
    const size_t program = (_program != NULL) ? (strlen(_program) + 1u) : 0u;
    const size_t text    = strlen(_words) + 1u;
    const size_t count   = named + d_tests_arg_word_count(_words);
    const size_t bytes   = ((count + 1u) * sizeof(char*)) + program + text;
    char** const argv    = (count <= (size_t)D_TESTS_ARG_ARGS_MAX)
                           ? d_tests_arg_keep(_run,
                                              bytes)
                           : NULL;

    // more arguments than a parse takes, or storage that could not be had,
    // fail the run
    if (!argv)
    {
        _run->failed = true;

        return NULL;
    }

    char* const storage = (char*)(argv + count + 1u);

    // the program's name, first
    if (_program != NULL)
    {
        memcpy(storage,
               _program,
               program);
        argv[0] = storage;
    }

    memcpy(storage + program,
           _words,
           text);
    d_tests_arg_split(argv + named,
                      storage + program,
                      count - named);
    argv[count] = NULL;
    *_argc      = (int)count;

    return argv;
}

/*
d_tests_arg_record
  The ERR line, then the lines of the report.
*/
void
d_tests_arg_record(
    struct d_tests_arg_run* _run,
    int                     _status
)
{
    d_tests_arg_record_error(_run,
                             _status);
    d_tests_arg_report(_run);

    return;
}

/*
d_tests_arg_record_error
  The ERR line of a parse that did not come to D_ARG_OK: the status's name,
the exit code and the message the parser recorded, escaped.
*/
void
d_tests_arg_record_error(
    struct d_tests_arg_run* _run,
    int                     _status
)
{
    // validate the parameters; a parse that went on has no line
    if ( (!_run) ||
         (_status == (int)D_ARG_OK) )
    {
        return;
    }

    const struct d_arg_error* const error = d_arg_parser_error(&_run->parser);

    (void)d_arg_buffer_printf(&_run->transcript,
                              "ERR %s %d: ",
                              d_arg_status_name(_status),
                              d_arg_error_exit_code(error));
    d_tests_arg_escape(&_run->transcript,
                       d_pack_text_from_cstr(d_arg_error_message(error)));
    (void)d_arg_buffer_append_char(&_run->transcript,
                                   '\n');

    return;
}

/*
d_tests_arg_report
  Each command named, then each option, in the order named.
*/
void
d_tests_arg_report(
    struct d_tests_arg_run* _run
)
{
    // validate the parameters
    if (!_run)
    {
        return;
    }

    // each command
    for (size_t index = 0u; index < _run->command_count; ++index)
    {
        d_tests_arg_report_command(_run,
                                   &_run->commands[index]);
    }

    // each option
    for (size_t index = 0u; index < _run->option_count; ++index)
    {
        d_tests_arg_report_option(_run,
                                  &_run->options[index]);
    }

    return;
}

/*
d_tests_arg_matches
  A transcript that could not be written fails, as does a run that failed
before it.
*/
bool
d_tests_arg_matches(
    struct d_tests_arg_run* _run,
    const char*             _expected
)
{
    // validate the parameters
    if (!_run)
    {
        return false;
    }

    const bool same = d_tests_arg_same("transcript",
                                       d_arg_buffer_text(&_run->transcript),
                                       _expected);

    // a run that failed says so
    if ( (_run->failed) ||
         (_run->transcript.failed != 0) )
    {
        printf("    the run failed: a definition, a watch or a note\n");
    }

    d_arg_buffer_clear(&_run->transcript);

    return ( (same)          &&
             (!_run->failed) );
}

/*
d_tests_arg_expect
  The run ends whatever the comparison found.
*/
bool
d_tests_arg_expect(
    struct d_tests_arg_run* _run,
    const char* const*      _args,
    size_t                  _count,
    const char*             _expected
)
{
    (void)d_tests_arg_parse(_run,
                            _args,
                            _count);

    const bool same = d_tests_arg_matches(_run,
                                          _expected);

    d_tests_arg_end(_run);

    return same;
}

/**
 * @brief How many arguments a case gives: those before its first NULL.
 *
 * @param[in] _case  the case.
 * @return the count, at most D_TESTS_ARG_CASE_ARGS.
 */
static size_t
d_tests_arg_case_count(
    const struct d_tests_arg_case* _case
)
{
    size_t count = 0u;

    // the arguments before the first NULL
    while ( (count < (size_t)D_TESTS_ARG_CASE_ARGS) &&
            (_case->args[count] != NULL) )
    {
        ++count;
    }

    return count;
}

/**
 * @brief Appends a case's line: its tag, then its arguments in brackets,
 *        separated by spaces, each escaped.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _case   the case.
 * @param[in]     _count  how many arguments it gives.
 */
static void
d_tests_arg_case_line(
    struct d_arg_buffer*           _out,
    const struct d_tests_arg_case* _case,
    size_t                         _count
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s [",
                              _case->tag);

    // each argument, after a space when one comes before it
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? " " : "");
        d_tests_arg_escape(_out,
                           d_pack_text_from_cstr(_case->args[index]));
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    return;
}

/**
 * @brief Sets a case's environment variables, or removes them.
 *
 * @param[in] _variables  the variables, ending at a NULL name; NULL for
 *                        none.
 * @param[in] _set        whether they are set, rather than removed.
 * @return whether each was set or removed.
 */
static bool
d_tests_arg_case_variables(
    const struct d_tests_arg_env_var* _variables,
    bool                              _set
)
{
    bool changed = true;

    // each variable, until the NULL name
    for (size_t index = 0u;
         (_variables != NULL) && (_variables[index].name != NULL);
         ++index)
    {
        changed = d_tests_arg_setenv(_variables[index].name,
                                     (_set)
                                     ? _variables[index].value
                                     : NULL) && changed;
    }

    return changed;
}

/**
 * @brief Makes the files a case parses with in a scratch directory, which
 *        is the working directory until the case ends.
 *
 * @param[out] _scratch  receives the scratch directory; holds nothing for a
 *                       case without files.
 * @param[in]  _files    the files, ending at a NULL name; NULL for none.
 * @return whether the directory and every file were made; `true` for no
 *         files.
 */
static bool
d_tests_arg_case_files(
    struct d_tests_arg_files*      _scratch,
    const struct d_tests_arg_file* _files
)
{
    // no files: the working directory stays
    if (!_files)
    {
        return true;
    }

    bool made = d_tests_arg_files_enter(_scratch);

    // each file, until the NULL name
    for (size_t index = 0u;
         (made) && (_files[index].name != NULL);
         ++index)
    {
        made = d_tests_arg_files_file(_scratch,
                                      _files[index].name,
                                      _files[index].content);
    }

    return made;
}

/**
 * @brief Runs one case of a table on a run of its own and appends its line
 *        and what its run's transcript came to.
 *
 * @param[in,out] _out   the table's transcript.
 * @param[in]     _case  the case.
 * @return whether the case ran: its root made, its watches, notes,
 *         variables and files kept.
 *
 * The files are made first, then the definitions, before the line is
 * written, as the reference programs make them before they print it; the
 * variables are set only for the parse, so that a definition reads none of
 * them.
 */
static bool
d_tests_arg_case_run(
    struct d_arg_buffer*           _out,
    const struct d_tests_arg_case* _case
)
{
    struct d_tests_arg_files    scratch = { 0 };
    const bool                  made    = d_tests_arg_case_files(&scratch,
                                                                 _case->files);
    struct d_tests_arg_run      run     = { 0 };
    struct d_arg_command* const app     = d_tests_arg_begin(&run);
    const size_t                count   = d_tests_arg_case_count(_case);

    // the definitions, on a root that was made
    if (app != NULL)
    {
        _case->define(&run,
                      app);
    }

    d_tests_arg_case_line(_out,
                          _case,
                          count);

    const bool set = d_tests_arg_case_variables(_case->variables,
                                                true);

    (void)d_tests_arg_parse(&run,
                            _case->args,
                            count);

    const bool unset = d_tests_arg_case_variables(_case->variables,
                                                  false);
    const bool held  = ( (made)                          &&
                         (set)                           &&
                         (unset)                         &&
                         (!run.failed)                   &&
                         (run.transcript.failed == 0) );

    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&run.transcript));

    // a case that did not run says so
    if (!held)
    {
        printf("    %s: the case failed: a definition, a watch, a note, a "
               "variable or a file\n",
               _case->tag);
    }

    d_tests_arg_end(&run);
    d_tests_arg_files_leave(&scratch);

    return held;
}

/*
d_tests_arg_cases
  Every case runs, whatever the ones before it came to, so that a mismatch
shows the whole transcript.
*/
bool
d_tests_arg_cases(
    const char*                    _name,
    const struct d_tests_arg_case* _cases,
    size_t                         _count,
    const char*                    _expected
)
{
    // validate the parameters
    if ( (!_cases) &&
         (_count > 0u) )
    {
        return false;
    }

    struct d_arg_buffer written = D_ARG_BUFFER_INIT;
    bool                ran     = true;

    // each case, on a run of its own
    for (size_t index = 0u; index < _count; ++index)
    {
        ran = d_tests_arg_case_run(&written,
                                   &_cases[index]) && ran;
    }

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (ran)  &&
             (kept) );
}

/**
 * @brief Runs one case of a table of steps on a run of its own, and
 *        appends what it wrote.
 *
 * @param[in,out] _out    the table's transcript.
 * @param[in]     _step   the case.
 * @param[in]     _index  its place in the table, from 0.
 * @return whether the case ran: its run made, its watches, notes and
 *         storage kept.
 */
static bool
d_tests_arg_step_run(
    struct d_arg_buffer*  _out,
    d_tests_arg_define_fn _step,
    size_t                _index
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    // the case, on a root that was made
    if (app != NULL)
    {
        _step(&run,
              app);
    }

    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&run.transcript));

    const bool held = ( (app != NULL)                &&
                        (!run.failed)                &&
                        (run.transcript.failed == 0) );

    // a case that did not run says so
    if (!held)
    {
        printf("    case %lu failed: its run, a definition, a watch, a note "
               "or its storage\n",
               (unsigned long)(_index + 1u));
    }

    d_tests_arg_end(&run);

    return held;
}

/*
d_tests_arg_steps
  Every case runs, whatever the ones before it came to, so that a mismatch
shows the whole transcript.
*/
bool
d_tests_arg_steps(
    const char*                  _name,
    const d_tests_arg_define_fn* _steps,
    size_t                       _count,
    const char*                  _expected
)
{
    // validate the parameters
    if ( (!_steps) &&
         (_count > 0u) )
    {
        return false;
    }

    struct d_arg_buffer written = D_ARG_BUFFER_INIT;
    bool                ran     = true;

    // each case, on a run of its own
    for (size_t index = 0u; index < _count; ++index)
    {
        ran = d_tests_arg_step_run(&written,
                                   _steps[index],
                                   index) && ran;
    }

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (ran)  &&
             (kept) );
}

#if (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_POSIX)

/*
d_tests_arg_setenv
  POSIX's setenv, overwriting, and unsetenv.
*/
bool
d_tests_arg_setenv(
    const char* _name,
    const char* _value
)
{
    // validate the parameters
    if (!_name)
    {
        return false;
    }

    return (_value != NULL)
           ? (setenv(_name,
                     _value,
                     1) == 0)
           : (unsetenv(_name) == 0);
}

#elif (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_WINDOWS)

/*
d_tests_arg_setenv
  The Microsoft runtime's _putenv_s, which the module's _dupenv_s reads
back; an empty value is how it removes a variable, which is why a set value
may not be empty.
*/
bool
d_tests_arg_setenv(
    const char* _name,
    const char* _value
)
{
    // validate the parameters
    if (!_name)
    {
        return false;
    }

    return (_putenv_s(_name,
                      (_value != NULL) ? _value : "") == 0);
}

#else

/*
d_tests_arg_setenv
  A system whose environment the suites cannot change: nothing is set, and
a case that needs a variable fails rather than parses without it.
*/
bool
d_tests_arg_setenv(
    const char* _name,
    const char* _value
)
{
    (void)_name;
    (void)_value;

    return false;
}

#endif  // D_ENV_ARG_PARSER_STAT

//==============================================================================
// 3.  TEXT
//==============================================================================

/*
d_tests_arg_same
  A NULL expectation is the empty text. A mismatch is printed expected first.
*/
bool
d_tests_arg_same(
    const char*        _what,
    struct d_pack_text _got,
    const char*        _expected
)
{
    const char* const expected = (_expected != NULL)
                                 ? _expected
                                 : "";
    const size_t      length   = strlen(expected);
    const bool        same     = ( (_got.length == length) &&
                                   ( (length == 0u) ||
                                     (memcmp(_got.data,
                                             expected,
                                             length) == 0) ) );

    // a mismatch is shown
    if (!same)
    {
        printf("    %s differs\n",
               (_what != NULL) ? _what : "text");
        d_tests_arg_show("expected:",
                         d_pack_text_from_cstr(expected));
        d_tests_arg_show("written:",
                         _got);
    }

    return same;
}

/**
 * @brief Appends text with each control character as its escape -- `\\t`,
 *        `\\n`, or `\\x` and two hexadecimal digits -- and, when asked, each
 *        backslash as two.
 *
 * @param[in,out] _out        the transcript.
 * @param[in]     _text       the text.
 * @param[in]     _backslash  whether a backslash is doubled.
 */
static void
d_tests_arg_write_escaped(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text,
    bool                 _backslash
)
{
    // each byte
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const unsigned char byte = (unsigned char)_text.data[index];

        // a control character as its escape, a backslash doubled when
        // asked, anything else as itself
        if (byte == '\t')
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "\\t");
        }
        else if (byte == '\n')
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "\\n");
        }
        else if ( (byte < 0x20u) ||
                  (byte == 0x7fu) )
        {
            (void)d_arg_buffer_printf(_out,
                                      "\\x%02x",
                                      (unsigned int)byte);
        }
        else if ( (_backslash) &&
                  (byte == '\\') )
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "\\\\");
        }
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           (char)byte);
        }
    }

    return;
}

/*
d_tests_arg_render
  Control characters escaped, so that a transcript line stays one line.
*/
void
d_tests_arg_render(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text
)
{
    d_tests_arg_write_escaped(_out,
                              _text,
                              false);

    return;
}

/*
d_tests_arg_escape
  Control characters escaped and backslashes doubled, so that the text can
be read back.
*/
void
d_tests_arg_escape(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text
)
{
    d_tests_arg_write_escaped(_out,
                              _text,
                              true);

    return;
}

//==============================================================================
// 4.  VALIDATORS
//==============================================================================

/**
 * @brief Writes what a validator made of one value, as one line.
 *
 * @param[in]     _validator  the validator.
 * @param[in]     _input      the value.
 * @param[in,out] _out        the buffer that gains the line.
 *
 * One value's line: the value, then what came of it -- `pass` and the value
 * it became, `fail` and the reason, or `status` and whatever the validator
 * wrote before it stopped with an error.
 */
static void
d_tests_arg_validation_line(
    const struct d_arg_validator* _validator,
    const char*                   _input,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr(_input));
    validation.integer_rule = D_ARG_INTEGER_C;

    const int  status = d_arg_validator_run(_validator,
                                            &validation);
    const bool passed = ( (status == D_ARG_OK) &&
                          (d_arg_validation_passed(&validation)) );

    (void)d_arg_buffer_append_char(_out,
                                   '[');
    d_tests_arg_render(_out,
                       d_pack_text_from_cstr(_input));
    (void)d_arg_buffer_printf(_out,
                              "] -> %s <",
                              (passed)
                              ? "pass"
                              : ( (status == D_ARG_OK)
                                  ? "fail"
                                  : "status" ));
    d_tests_arg_render(_out,
                       (passed)
                       ? validation.value
                       : d_arg_buffer_text(&validation.message));
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");
    d_arg_validation_release(&validation);

    return;
}

/*
d_tests_arg_transcribe
  The description's line, then a line for each value, each value validated
afresh from the text given.
*/
void
d_tests_arg_transcribe(
    struct d_arg_buffer*          _out,
    const struct d_arg_validator* _validator,
    const char* const*            _inputs,
    size_t                        _count
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "desc <");
    (void)d_arg_validator_describe(_validator,
                                   _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");

    // a line for each value
    for (size_t index = 0u; index < _count; ++index)
    {
        d_tests_arg_validation_line(_validator,
                                    _inputs[index],
                                    _out);
    }

    return;
}

/*
d_tests_arg_holds
  Transcribes a validator over the values, compares the transcript with the
one expected, and releases the validator.
*/
bool
d_tests_arg_holds(
    const char*            _what,
    struct d_arg_validator _validator,
    const char* const*     _inputs,
    size_t                 _count,
    const char*            _expected
)
{
    struct d_arg_buffer transcript = D_ARG_BUFFER_INIT;

    d_tests_arg_transcribe(&transcript,
                           &_validator,
                           _inputs,
                           _count);

    const bool same = d_tests_arg_same(_what,
                                       d_arg_buffer_text(&transcript),
                                       _expected);

    d_arg_buffer_release(&transcript);
    d_arg_validator_release(&_validator);

    return same;
}

/**
 * @brief Defines a checked option, bound to a variable, and hands it its
 *        validator.
 *
 * @param[in,out] _run        the run, which watches the option, and fails when
 *                            it could not be made or its validator was refused.
 * @param[in,out] _app        the root command.
 * @param[in]     _option     the option's name and type, and whether its
 *                            validator transforms.
 * @param[in]     _validator  the validator, consumed either way.
 * @param[in]     _variable   the variable bound, which the parse writes.
 * @return the option; NULL when it could not be defined.
 *
 * Defines the option a d_tests_arg_validates parse validates on the run's
 * root, bound to the variable, and hands it the validator, which is consumed
 * whether or not the option could be defined; the option, or NULL -- also for
 * a root that could not be made -- which fails the run.
 */
static struct d_arg_option*
d_tests_arg_define_checked(
    struct d_tests_arg_run*           _run,
    struct d_arg_command*             _app,
    const struct d_tests_arg_checked* _option,
    struct d_arg_validator            _validator,
    union d_tests_arg_variable*       _variable
)
{
    struct d_arg_option* const option =
        d_tests_arg_watch_option(
            _run,
            _option->name,
            d_arg_command_add_option_bound(_app,
                                           _option->name,
                                           _variable,
                                           d_arg_type_builtin(_option->type),
                                           NULL));
    const int                  added  = (_option->transform)
                                        ? d_arg_option_transform(option,
                                                                 _validator,
                                                                 NULL,
                                                                 NULL)
                                        : d_arg_option_check(option,
                                                             _validator,
                                                             NULL);

    // a validator the option did not take fails the run
    if (added != D_ARG_OK)
    {
        _run->failed = true;
    }

    return option;
}

/*
d_tests_arg_validates
  The variable starts as its type's zero -- the conversion of no value -- and
is read before the run ends, which frees the text a string variable points
at.
*/
bool
d_tests_arg_validates(
    const struct d_tests_arg_checked* _option,
    struct d_arg_validator            _validator,
    const char*                       _value,
    const char*                       _expected
)
{
    const char* const              args[]   =
    {
        _option->name,
        _value
    };
    const struct d_arg_type* const type     =
        d_arg_type_builtin(_option->type);
    union d_tests_arg_variable     variable = { 0.0L };
    struct d_arg_buffer            written  = D_ARG_BUFFER_INIT;
    struct d_tests_arg_run         run      = { 0 };
    struct d_arg_command* const    app      = d_tests_arg_begin(&run);

    (void)d_arg_convert(type,
                        NULL,
                        0u,
                        &variable,
                        NULL);
    (void)d_tests_arg_define_checked(&run,
                                     app,
                                     _option,
                                     _validator,
                                     &variable);
    (void)d_tests_arg_parse(&run,
                            args,
                            2u);
    (void)d_arg_format(type,
                       &variable,
                       d_arg_buffer_sink(&written));
    d_tests_arg_note(&run,
                     "value [%.*s]",
                     (int)written.length,
                     (written.data != NULL) ? written.data : "");

    const bool same = d_tests_arg_matches(&run,
                                          _expected);

    d_arg_buffer_release(&written);
    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 5.  COMPLETION
//==============================================================================

/*
d_tests_arg_kind_of
  The flags shell completion reads, of a validator then released.
*/
unsigned int
d_tests_arg_kind_of(
    struct d_arg_validator _validator
)
{
    const unsigned int kind = (unsigned int)(_validator.flags &
                                             (D_ARG_VALIDATOR_PATH      |
                                              D_ARG_VALIDATOR_DIRECTORY |
                                              D_ARG_VALIDATOR_NUMBER));

    d_arg_validator_release(&_validator);

    return kind;
}
