/*******************************************************************************
* djinterp [parse]                                                  arg_parser.c
*
* The parser's own parts, declared in arg_parser.h (sections 3, 5, 6 and 7)
* and arg_internal.h (7.1).
*   Failures recorded in the parser's two reports -- the first failed
* definition kept, the parse's error replaced -- and set aside while a
* caller's function runs; the parser made and released around its root;
* CLI11's exit(), with its two failure messages and the calls a callback
* fails through; and the parser's settings. The parse itself is in
* arg_parse.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_parser.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text,
                                                                // d_pack_sink
#include "../../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // errors, buffers, sinks
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // d_arg_config_format
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // d_arg_formatter, d_arg_help
#include "./arg_internal.h"  // failures, commands, copies


//==============================================================================
// 1.  FAILURES
//==============================================================================

// d_arg_internal_parser_message / d_arg_internal_parser_vfail
//   declarations: the format attribute their definitions cannot carry, so
// that every format passed through them is checked as a printf format.
D_STATIC char*
     d_arg_internal_parser_message(const char* _format,
                                   va_list     _arguments)
    D_FORMAT_PRINTF(1, 0);
D_STATIC int
     d_arg_internal_parser_vfail(const struct d_arg_command* _command,
                                 const struct d_arg_option*  _option,
                                 int                         _status,
                                 const char*                 _format,
                                 va_list                     _arguments)
    D_FORMAT_PRINTF(4, 0);

/*
d_arg_internal_parser_for
  The parser a failure belongs to: the command's, or else the option's.
*/
D_STATIC struct d_arg_parser*
d_arg_internal_parser_for(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option
)
{
    return (_command != NULL)
           ? _command->parser
           : d_arg_internal_parser_of(_option);
}

/*
d_arg_internal_parser_message
  A failure's message, formatted onto the heap: NULL for no format -- the
status then speaks for itself -- or for one that could not be expanded.
*/
D_STATIC char*
d_arg_internal_parser_message(
    const char* _format,
    va_list     _arguments
)
{
    struct d_arg_buffer text = D_ARG_BUFFER_INIT;

    // no format, no message
    if (!_format)
    {
        return NULL;
    }

    // a message that cannot be formatted is none
    if (d_arg_internal_vformat(&text,
                               _format,
                               _arguments) != D_ARG_OK)
    {
        d_arg_buffer_release(&text);

        return NULL;
    }

    char* const message = d_arg_buffer_detach(&text);

    // an empty message holds no storage of its own
    return (message != NULL)
           ? message
           : d_arg_internal_copy_cstr("");
}

/*
d_arg_internal_parser_store
  A report replaced: its message taken over, the exit code the status's,
and the record it arose in -- the option's command when no command is
named.
*/
D_STATIC void
d_arg_internal_parser_store(
    struct d_arg_error*         _report,
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    int                         _status,
    char*                       _message
)
{
    d_arg_error_release(_report);
    _report->status    = _status;
    _report->exit_code = d_arg_status_exit_code(_status);
    _report->message   = _message;
    _report->command   = (_command != NULL)
                         ? _command
                         : ( (_option != NULL)
                             ? _option->parent
                             : NULL );
    _report->option    = _option;

    return;
}

/*
d_arg_internal_parser_vfail
  A failure of the parse, recorded in place of whatever the parser's error
held; with no parser to record in, only the status stands.
*/
D_STATIC int
d_arg_internal_parser_vfail(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    int                         _status,
    const char*                 _format,
    va_list                     _arguments
)
{
    struct d_arg_parser* const parser = d_arg_internal_parser_for(_command,
                                                                  _option);

    // nowhere to record it
    if (!parser)
    {
        return _status;
    }

    d_arg_internal_parser_store(&parser->error,
                                _command,
                                _option,
                                _status,
                                d_arg_internal_parser_message(_format,
                                                              _arguments));

    return _status;
}

/*
d_arg_internal_fail_definition
  A failed definition, recorded unless one is kept already.
*/
int
d_arg_internal_fail_definition(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    int                         _status,
    const char*                 _format,
    ...
)
{
    struct d_arg_parser* const parser = d_arg_internal_parser_for(_command,
                                                                  _option);
    va_list                    arguments;

    // nowhere to record it, or the first is kept
    if ( (!parser) ||
         (parser->definition.status != D_ARG_OK) )
    {
        return _status;
    }

    va_start(arguments,
             _format);
    d_arg_internal_parser_store(&parser->definition,
                                _command,
                                _option,
                                _status,
                                d_arg_internal_parser_message(_format,
                                                              arguments));
    va_end(arguments);

    return _status;
}

/*
d_arg_internal_fail
  A failure of the parse.
*/
int
d_arg_internal_fail(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    int                         _status,
    const char*                 _format,
    ...
)
{
    va_list arguments;

    va_start(arguments,
             _format);

    const int status = d_arg_internal_parser_vfail(_command,
                                                   _option,
                                                   _status,
                                                   _format,
                                                   arguments);

    va_end(arguments);

    return status;
}

/*
d_arg_internal_fail_adopt
  A failure of the parse whose message, on the heap, the report takes over:
a completed request whose message is what it asked for -- a version, a
completion script -- and has none is a refused allocation instead.
*/
int
d_arg_internal_fail_adopt(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    int                         _status,
    char*                       _message
)
{
    struct d_arg_parser* const parser = d_arg_internal_parser_for(_command,
                                                                  _option);
    const bool                 lost   =
        ( (!_message) &&
          ( (_status == (int)D_ARG_CALL_FOR_VERSION) ||
            (_status == (int)D_ARG_CALL_FOR_COMPLETION) ) );
    const int                  status = (lost)
                                        ? (int)D_ARG_ERROR_OUT_OF_MEMORY
                                        : _status;

    // nowhere to record it
    if (!parser)
    {
        free(_message);

        return status;
    }

    d_arg_internal_parser_store(&parser->error,
                                _command,
                                _option,
                                status,
                                _message);

    return status;
}

/*
d_arg_internal_fail_text
  A failure of the parse whose message is a span, copied whole -- adopted,
so that a request whose copy was refused is reported as the refusal.
*/
int
d_arg_internal_fail_text(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    int                         _status,
    struct d_pack_text          _message
)
{
    return d_arg_internal_fail_adopt(_command,
                                     _option,
                                     _status,
                                     d_arg_internal_copy_text(_message));
}

/*
d_arg_internal_watch_begin
  The parser's error moved into the watch, the parser left a clean one.
*/
void
d_arg_internal_watch_begin(
    struct d_arg_internal_watch* _watch,
    struct d_arg_parser*         _parser
)
{
    _watch->parser = _parser;
    d_arg_error_init(&_watch->saved);

    // only a parser has an error to set aside
    if (_parser != NULL)
    {
        _watch->saved = _parser->error;
        d_arg_error_init(&_parser->error);
    }

    return;
}

/*
d_arg_internal_watch_end
  A failure the function recorded stays, and what was set aside is
dropped; otherwise what was set aside comes back over whatever the
function recorded.
*/
bool
d_arg_internal_watch_end(
    struct d_arg_internal_watch* _watch,
    int                          _status
)
{
    struct d_arg_parser* const parser = _watch->parser;

    // nothing was watched
    if (!parser)
    {
        return false;
    }

    // the function's own failure
    if ( (_status != D_ARG_OK) &&
         (parser->error.status == _status) )
    {
        d_arg_error_release(&_watch->saved);

        return true;
    }

    d_arg_error_release(&parser->error);
    parser->error = _watch->saved;
    d_arg_error_init(&_watch->saved);

    return false;
}

/*
d_arg_internal_watch_close
  The watch ended; a failure the callback returned without recording it is
recorded with nothing but its status, the report then reading the status's
own text.
*/
int
d_arg_internal_watch_close(
    struct d_arg_internal_watch* _watch,
    const struct d_arg_command*  _command,
    int                          _status
)
{
    const bool recorded = d_arg_internal_watch_end(_watch,
                                                   _status);

    return ( (_status == D_ARG_OK) ||
             (recorded) )
           ? _status
           : d_arg_internal_fail(_command,
                                 NULL,
                                 _status,
                                 NULL);
}

/*
d_arg_internal_unreport
  Each of the two reports forgets the command and the option given.
*/
void
d_arg_internal_unreport(
    struct d_arg_parser*        _parser,
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option
)
{
    // no parser, no reports
    if (!_parser)
    {
        return;
    }

    struct d_arg_error* const reports[2] = { &_parser->error,
                                             &_parser->definition };

    // each report, by pointer
    for (size_t index = 0u; index < 2u; ++index)
    {
        if ( (_command != NULL) &&
             (reports[index]->command == _command) )
        {
            reports[index]->command = NULL;
        }

        if ( (_option != NULL) &&
             (reports[index]->option == _option) )
        {
            reports[index]->option = NULL;
        }
    }

    return;
}

//==============================================================================
// 2.  LIFETIME
//==============================================================================

// PARSER_EMPTY
//   constant: a parser holding nothing, every list empty and every pointer
// NULL, from which a parser is made.
static const struct d_arg_parser PARSER_EMPTY;

/*
d_arg_parser_init
  CLI11's App(description, name): the parser's own parts first -- reports,
formatter, configuration format, standard output and error, suggestions on
-- then the root, then CLI11's help flag; a failure is the definition
error, the parser left whole.
*/
int
d_arg_parser_init(
    struct d_arg_parser* _parser,
    const char*          _description,
    const char*          _name
)
{
    // validate the parameters
    if (!_parser)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    *_parser = PARSER_EMPTY;
    d_arg_error_init(&_parser->error);
    d_arg_error_init(&_parser->definition);
    d_arg_formatter_init(&_parser->formatter);
    d_arg_config_format_init(&_parser->config_format);
    _parser->out              = d_arg_sink_stdout();
    _parser->err              = d_arg_sink_stderr();
    _parser->response_quoting = (int)D_ARG_QUOTING_NATIVE_RESPONSE;
    _parser->flags            = D_ARG_PARSER_SUGGESTIONS;

    const int status = d_arg_internal_command_init(&_parser->command,
                                                   _parser,
                                                   NULL,
                                                   _name,
                                                   _description);

    // the help flag, as CLI11's App constructor adds it
    if ( (status == D_ARG_OK) &&
         (!d_arg_command_set_help_flag(&_parser->command,
                                       "-h,--help",
                                       "Print this help message and exit")) )
    {
        return (_parser->definition.status != D_ARG_OK)
               ? _parser->definition.status
               : D_ARG_ERROR_OUT_OF_MEMORY;
    }

    return status;
}

/*
d_arg_parser_release
  The tree first -- its records leave the reports as they go -- then the
reports, the formatter, the configuration format and the storage; the
parser is left as d_arg_parser_init finds any.
*/
void
d_arg_parser_release(
    struct d_arg_parser* _parser
)
{
    // nothing to release
    if (!_parser)
    {
        return;
    }

    d_arg_internal_command_release(&_parser->command);
    d_arg_error_release(&_parser->error);
    d_arg_error_release(&_parser->definition);
    d_arg_formatter_release(&_parser->formatter);
    d_arg_config_format_release(&_parser->config_format);
    d_arg_text_list_release(&_parser->pending);
    d_arg_string_list_release(&_parser->pool);
    free(_parser->program);
    d_arg_string_list_release(&_parser->utf8_argv);
    *_parser = PARSER_EMPTY;

    return;
}

/*
d_arg_parser_clear
  CLI11's clear() on the root, then the last parse's error and storage --
the program's name among it: the values that borrowed from the storage are
gone first.
*/
void
d_arg_parser_clear(
    struct d_arg_parser* _parser
)
{
    // nothing to clear
    if (!_parser)
    {
        return;
    }

    d_arg_command_clear(&_parser->command);
    d_arg_error_release(&_parser->error);
    d_arg_text_list_release(&_parser->pending);
    d_arg_string_list_release(&_parser->pool);
    free(_parser->program);
    _parser->program = NULL;

    return;
}

//==============================================================================
// 3.  ENDING
//==============================================================================

/*
d_arg_internal_parser_request
  What CLI11's exit() writes for a completed request: the help, or help with
every subcommand expanded; the version, and a newline; a completion script
as it was made.
*/
D_STATIC void
d_arg_internal_parser_request(
    const struct d_arg_parser* _parser,
    const struct d_arg_error*  _error,
    struct d_pack_sink         _out
)
{
    // help, plain or expanded
    if ( (_error->status == (int)D_ARG_CALL_FOR_HELP) ||
         (_error->status == (int)D_ARG_CALL_FOR_ALL_HELP) )
    {
        (void)d_arg_help(&_parser->command,
                         d_arg_internal_span("",
                                             0u),
                         (_error->status == (int)D_ARG_CALL_FOR_HELP)
                         ? (int)D_ARG_HELP_NORMAL
                         : (int)D_ARG_HELP_ALL,
                         _out);

        return;
    }

    (void)d_arg_sink_cstr(_out,
                          d_arg_error_message(_error));

    // the version ends its line
    if (_error->status == (int)D_ARG_CALL_FOR_VERSION)
    {
        (void)d_arg_sink_write(_out,
                               "\n",
                               1u);
    }

    return;
}

/*
d_arg_parser_exit_to
  CLI11's exit(): a runtime error writes nothing; a completed request what
it asked for; an error that does not exit with success the root's failure
message -- its own, or CLI11's simple one. The status is described by the
parser's error when it is that error's, and by its own text otherwise.
*/
int
d_arg_parser_exit_to(
    const struct d_arg_parser* _parser,
    int                        _status,
    struct d_pack_sink         _out,
    struct d_pack_sink         _err
)
{
    struct d_arg_error own = D_ARG_ERROR_INIT;

    // validate the parameters
    if (!_parser)
    {
        return (int)D_ARG_EXIT_BASE_CLASS;
    }

    own.status    = _status;
    own.exit_code = d_arg_status_exit_code(_status);

    const struct d_arg_error* const error = (_parser->error.status == _status)
                                            ? &_parser->error
                                            : &own;
    const struct d_arg_command* const root = &_parser->command;

    // the completed requests write what they asked for
    if ( (error->status >= (int)D_ARG_CALL_FOR_HELP) &&
         (error->status <= (int)D_ARG_CALL_FOR_COMPLETION) )
    {
        d_arg_internal_parser_request(_parser,
                                      error,
                                      _out);
    }
    else if ( (error->status != (int)D_ARG_RUNTIME_ERROR) &&
              (error->exit_code != (int)D_ARG_EXIT_SUCCESS) )
    {
        const d_arg_failure_fn write = (root->failure_fn != NULL)
                                       ? root->failure_fn
                                       : d_arg_failure_simple;

        (void)write(root,
                    error,
                    _err,
                    root->failure_context);
    }

    return error->exit_code;
}

/*
d_arg_parser_exit
  d_arg_parser_exit_to with the parser's own sinks.
*/
int
d_arg_parser_exit(
    const struct d_arg_parser* _parser,
    int                        _status
)
{
    return (_parser != NULL)
           ? d_arg_parser_exit_to(_parser,
                                  _status,
                                  _parser->out,
                                  _parser->err)
           : (int)D_ARG_EXIT_BASE_CLASS;
}

/*
d_arg_internal_parser_run_with
  The line CLI11's simple message ends with: "Run with <help> for more
information." -- the help-all flag joined with " or " -- when the command
has either flag.
*/
D_STATIC int
d_arg_internal_parser_run_with(
    const struct d_arg_command* _command,
    struct d_arg_buffer*        _text
)
{
    const struct d_arg_option* const first = (_command->help != NULL)
                                             ? _command->help
                                             : _command->help_all;
    int                              status = D_ARG_OK;

    // no help flag, nothing to suggest
    if (!first)
    {
        return D_ARG_OK;
    }

    status = d_arg_buffer_append_cstr(_text,
                                      "Run with ");
    status = (status == D_ARG_OK)
             ? d_arg_option_name(first,
                                 0u,
                                 _text)
             : status;

    // both flags, joined
    if ( (status == D_ARG_OK)          &&
         (_command->help != NULL)      &&
         (_command->help_all != NULL) )
    {
        status = d_arg_buffer_append_cstr(_text,
                                          " or ");
        status = (status == D_ARG_OK)
                 ? d_arg_option_name(_command->help_all,
                                     0u,
                                     _text)
                 : status;
    }

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_text,
                                      " for more information.\n")
           : status;
}

/*
d_arg_internal_parser_emit
  A finished text written to a sink in one piece; a text that could not be
built is reported as such.
*/
D_STATIC int
d_arg_internal_parser_emit(
    struct d_arg_buffer* _text,
    int                  _status,
    struct d_pack_sink   _sink
)
{
    const int status = (_status == D_ARG_OK)
                       ? d_arg_sink_write(_sink,
                                          d_arg_buffer_text(_text).data,
                                          d_arg_buffer_text(_text).length)
                       : _status;

    d_arg_buffer_release(_text);

    return status;
}

/*
d_arg_failure_simple
  CLI11's FailureMessage::simple: the message and a newline, then the line
naming the help flags.
*/
int
d_arg_failure_simple(
    const struct d_arg_command* _command,
    const struct d_arg_error*   _error,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_buffer_printf(
                                     &text,
                                     "%s\n",
                                     d_arg_error_message(_error));

    (void)_context;

    // the help flags, for a command
    if ( (status == D_ARG_OK) &&
         (_command != NULL) )
    {
        status = d_arg_internal_parser_run_with(_command,
                                                &text);
    }

    return d_arg_internal_parser_emit(&text,
                                      status,
                                      _sink);
}

/*
d_arg_failure_help
  CLI11's FailureMessage::help: "ERROR: <error name>: <message>" and a
newline, then the command's help.
*/
int
d_arg_failure_help(
    const struct d_arg_command* _command,
    const struct d_arg_error*   _error,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    const int           status = d_arg_buffer_printf(
                                     &text,
                                     "ERROR: %s: %s\n",
                                     d_arg_status_name(
                                         (_error != NULL)
                                         ? _error->status
                                         : (int)D_ARG_OK),
                                     d_arg_error_message(_error));
    const int           written = d_arg_internal_parser_emit(&text,
                                                             status,
                                                             _sink);

    (void)_context;

    return ( (written == D_ARG_OK) &&
             (_command != NULL) )
           ? d_arg_help(_command,
                        d_arg_internal_span("",
                                            0u),
                        (int)D_ARG_HELP_NORMAL,
                        _sink)
           : written;
}

/*
d_arg_command_fail
  A callback's failure, recorded as the parse's error.
*/
int
d_arg_command_fail(
    const struct d_arg_command* _command,
    int                         _status,
    const char*                 _format,
    ...
)
{
    va_list arguments;

    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    va_start(arguments,
             _format);

    const int status = d_arg_internal_parser_vfail(_command,
                                                   NULL,
                                                   _status,
                                                   _format,
                                                   arguments);

    va_end(arguments);

    return status;
}

/*
d_arg_option_fail
  An option's callback's failure, naming the option.
*/
int
d_arg_option_fail(
    const struct d_arg_option* _option,
    int                        _status,
    const char*                _format,
    ...
)
{
    va_list arguments;

    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    va_start(arguments,
             _format);

    const int status = d_arg_internal_parser_vfail(NULL,
                                                   _option,
                                                   _status,
                                                   _format,
                                                   arguments);

    va_end(arguments);

    return status;
}

/*
d_arg_command_fail_runtime
  CLI11's RuntimeError(exit_code): no message, and the code chosen.
*/
int
d_arg_command_fail_runtime(
    const struct d_arg_command* _command,
    int                         _exit_code
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // recorded where there is a parser to record in
    if (_command->parser != NULL)
    {
        d_arg_internal_parser_store(&_command->parser->error,
                                    _command,
                                    NULL,
                                    (int)D_ARG_RUNTIME_ERROR,
                                    NULL);
        _command->parser->error.exit_code = _exit_code;
    }

    return D_ARG_RUNTIME_ERROR;
}

//==============================================================================
// 4.  SETTINGS
//==============================================================================

/*
d_arg_parser_set_output
  The two sinks, as given.
*/
void
d_arg_parser_set_output(
    struct d_arg_parser* _parser,
    struct d_pack_sink   _out,
    struct d_pack_sink   _err
)
{
    // nothing to set without a parser
    if (_parser != NULL)
    {
        _parser->out = _out;
        _parser->err = _err;
    }

    return;
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/*
d_arg_parser_set_response_files
  The switch, and the quoting rules when they are a d_arg_quoting value.
*/
void
d_arg_parser_set_response_files(
    struct d_arg_parser* _parser,
    bool                 _enabled,
    int                  _quoting
)
{
    // nothing to set without a parser
    if (!_parser)
    {
        return;
    }

    _parser->flags = _enabled
                     ? (_parser->flags | D_ARG_PARSER_RESPONSE_FILES)
                     : (_parser->flags & ~D_ARG_PARSER_RESPONSE_FILES);

    // the rules, when they are rules
    if (d_arg_internal_in_range(_quoting,
                                (int)D_ARG_QUOTING_POSIX,
                                (int)D_ARG_QUOTING_GNU))
    {
        _parser->response_quoting = _quoting;
    }

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

/*
d_arg_parser_set_suggestions
  The switch.
*/
void
d_arg_parser_set_suggestions(
    struct d_arg_parser* _parser,
    bool                 _enabled
)
{
    // nothing to set without a parser
    if (_parser != NULL)
    {
        _parser->flags = _enabled
                         ? (_parser->flags | D_ARG_PARSER_SUGGESTIONS)
                         : (_parser->flags & ~D_ARG_PARSER_SUGGESTIONS);
    }

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

//==============================================================================
// 5.  ERRORS
//==============================================================================

/*
d_arg_parser_error
  The last parse's report.
*/
const struct d_arg_error*
d_arg_parser_error(
    const struct d_arg_parser* _parser
)
{
    return (_parser != NULL)
           ? &_parser->error
           : NULL;
}

/*
d_arg_parser_definition_error
  The first failed definition's report.
*/
const struct d_arg_error*
d_arg_parser_definition_error(
    const struct d_arg_parser* _parser
)
{
    return (_parser != NULL)
           ? &_parser->definition
           : NULL;
}

/*
d_arg_parser_clear_definition_error
  The definition report emptied, so that parsing goes ahead.
*/
void
d_arg_parser_clear_definition_error(
    struct d_arg_parser* _parser
)
{
    // nothing to clear without a parser
    if (_parser != NULL)
    {
        d_arg_error_release(&_parser->definition);
    }

    return;
}
