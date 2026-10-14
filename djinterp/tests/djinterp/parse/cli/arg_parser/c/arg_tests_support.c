/*******************************************************************************
* djinterp [parse]                                           arg_tests_support.c
*
* The runs the argument parser's C suites share, declared in
* arg_tests_support.h: a parser made as the module was checked against
* CLI11, the report of a parse in the probes' transcript form, and the
* comparison that prints what differed.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_tests_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_tests_support.h"  // corresponding header
// std
#include <stdarg.h>   // va_list, va_start, va_end
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf, vsnprintf
#include <string.h>   // memchr, memcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
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

/*
d_tests_arg_quiet
  Turns the parser's suggestions off, so that its messages are CLI11's.
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

/*
d_tests_arg_quiet
  Nothing to turn off: a build without suggestions has CLI11's messages.
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

/*
d_tests_arg_report_status
  The ERR line of a parse that did not come to D_ARG_OK: the status's name,
the exit code and the message the parser recorded.
*/
static void
d_tests_arg_report_status(
    struct d_tests_arg_run* _run,
    int                     _status
)
{
    const struct d_arg_error* const error = d_arg_parser_error(&_run->parser);

    // a parse that went on has no line
    if (_status == (int)D_ARG_OK)
    {
        return;
    }

    (void)d_arg_buffer_printf(&_run->transcript,
                              "ERR %s %d: %s\n",
                              d_arg_status_name(_status),
                              d_arg_error_exit_code(error),
                              d_arg_error_message(error));

    return;
}

/*
d_tests_arg_report_command
  One `cmd` line: the uses, the arguments the command did not use, the
subcommands it parsed, and its options in the order met.
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
        (void)d_arg_buffer_append_text(out,
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

/*
d_tests_arg_report_option
  One `opt` line: how many values the option was given, and the values as
given.
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
        (void)d_arg_buffer_append_text(&_run->transcript,
                                       results[index]);
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");

    return;
}

/*
d_tests_arg_show
  Prints text under a heading, each line indented and marked, so that an
empty line and a line's trailing spaces can be seen.
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
  The parser first, since its sinks write into the buffers.
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
    _run->command_count = 0u;
    _run->option_count  = 0u;

    return;
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
d_tests_arg_parse
  The arguments as spans over the caller's strings, which outlive the
parse's values; then the report.
*/
int
d_tests_arg_parse(
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

    const int status = d_arg_parser_parse_args(&_run->parser,
                                               texts,
                                               _count,
                                               0u);

    d_tests_arg_record(_run,
                       status);

    return status;
}

/*
d_tests_arg_record
  The status, then each command named, then each option, in the order named.
*/
void
d_tests_arg_record(
    struct d_tests_arg_run* _run,
    int                     _status
)
{
    // validate the parameters
    if (!_run)
    {
        return;
    }

    d_tests_arg_report_status(_run,
                              _status);

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

//==============================================================================
// 3.  COMPARING
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

//==============================================================================
// 4.  VALIDATORS
//==============================================================================

/*
d_tests_arg_render
  Appends text with each control character as its escape, so that a
transcript line stays one line.
*/
void
d_tests_arg_render(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text
)
{
    // each byte
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const unsigned char byte = (unsigned char)_text.data[index];

        // a control character as its escape, anything else as itself
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
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           (char)byte);
        }
    }

    return;
}

/*
d_tests_arg_validation_line
  One value's line: the value, then what came of it -- `pass` and the value
it became, `fail` and the reason, or `status` and whatever the validator
wrote before it stopped with an error.
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

/*
d_tests_arg_define_checked
  Defines the option a d_tests_arg_validates parse validates on the run's
root, bound to the variable, and hands it the validator, which is consumed
whether or not the option could be defined; the option, or NULL -- also for
a root that could not be made -- which fails the run.
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
