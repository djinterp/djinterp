/*******************************************************************************
* djinterp [parse]                                                   arg_parse.c
*
* The parse, declared in arg_parser.h (4.1.1 and 4.1.2) and arg_internal.h
* (8.1, 8.3 and 8.4).
*   CLI11's parse of a vector of arguments, one C function for each of its
* member functions or for a step of one: the arguments wait on the parser's
* stack, the next one last, shared by every command as CLI11's commands
* share one vector; each is recognized -- the `--` mark, a subcommand, a
* long, short or Windows-style option, a positional -- and handed to its
* step, a subcommand parsing what follows until something belongs
* elsewhere, anything unclaimed kept as an unused argument. Every step
* returns D_ARG_OK where CLI11's returns, after recording its error where
* CLI11's throws, and gives CLI11's boolean result through `_go_on`.
*   Here: the stack, the entry points, the checks before a parse, the
* recognizer, the steps the parse's sources share, subcommands and the
* command's loop. The positional step is in arg_parse_positional.c, the
* option step in arg_parse_option.c, and the processing that ends a parse
* in arg_process.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_parse.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // memchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "./arg_internal.h"  // the parse, failures, lists, lookups
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// ONE_NOT_EXPECTED / MANY_NOT_EXPECTED
//   constant: how CLI11's ExtrasError opens, for one argument and for
// several.
static const char ONE_NOT_EXPECTED[]  =
    "The following argument was not expected:";
static const char MANY_NOT_EXPECTED[] =
    "The following arguments were not expected:";

//==============================================================================
// 1.  THE ARGUMENT STACK
//==============================================================================

/*
d_arg_internal_parse_stored
  A status from storage -- a list that grew, or could not -- recorded
against the command when it is a refusal.
*/
int
d_arg_internal_parse_stored(
    const struct d_arg_command* _command,
    int                         _status
)
{
    return (_status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail(_command,
                                 NULL,
                                 _status,
                                 NULL);
}

/*
d_arg_internal_pending_push
  The argument on top of the stack, borrowed.
*/
int
d_arg_internal_pending_push(
    struct d_arg_parser* _parser,
    struct d_pack_text   _text
)
{
    return d_arg_internal_text_push(&_parser->pending,
                                    _text);
}

/*
d_arg_internal_pending_push_copy
  A copy kept in the parser's pool, then its span on top of the stack; a
copy whose span cannot be pushed stays in the pool until the next parse.
*/
int
d_arg_internal_pending_push_copy(
    struct d_arg_parser* _parser,
    struct d_pack_text   _text
)
{
    const char* copy   = NULL;
    const int   status = d_arg_internal_string_copy(&_parser->pool,
                                                    _text,
                                                    &copy);

    return (status == D_ARG_OK)
           ? d_arg_internal_pending_push(_parser,
                                         d_arg_internal_span(copy,
                                                             _text.length))
           : status;
}

/*
d_arg_internal_parse_back
  The next argument: the top of the stack, which is not empty.
*/
struct d_pack_text
d_arg_internal_parse_back(
    const struct d_arg_command* _command
)
{
    const struct d_arg_text_list* const pending = &_command->parser->pending;

    return pending->items[pending->count - 1u];
}

/*
d_arg_internal_parse_pop
  The next argument taken off the stack; its bytes stay where they are, so
a span of it taken before stays valid.
*/
void
d_arg_internal_parse_pop(
    const struct d_arg_command* _command
)
{
    _command->parser->pending.count--;

    return;
}

/*
d_arg_internal_parse_left
  How many arguments wait on the stack: CLI11's args.size().
*/
size_t
d_arg_internal_parse_left(
    const struct d_arg_command* _command
)
{
    return _command->parser->pending.count;
}

/*
d_arg_internal_parse_push_built
  An argument CLI11 builds and pushes back -- "-" with what followed a
short name, or "--" with a name and, when there is a value, "=" with it --
written once into storage the pool then keeps, and its span pushed.
*/
int
d_arg_internal_parse_push_built(
    const struct d_arg_command* _command,
    const char*                 _prefix,
    struct d_pack_text          _name,
    struct d_pack_text          _value
)
{
    struct d_arg_parser* const parser = _command->parser;
    struct d_arg_buffer        text   = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&text,
                                   _prefix);
    (void)d_arg_buffer_append_text(&text,
                                   _name);

    // the value, after `=`
    if (_value.length > 0u)
    {
        (void)d_arg_buffer_append_char(&text,
                                       '=');
        (void)d_arg_buffer_append_text(&text,
                                       _value);
    }

    const size_t length = text.length;
    char* const  built  = (text.failed == 0)
                          ? d_arg_buffer_detach(&text)
                          : NULL;
    int          status = d_arg_internal_string_adopt(&parser->pool,
                                                      built);

    // the span, once the pool keeps its bytes
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_pending_push(parser,
                                             d_arg_internal_span(built,
                                                                 length));
    }

    d_arg_buffer_release(&text);

    return d_arg_internal_parse_stored(_command,
                                       status);
}

//==============================================================================
// 2.  ENTRY
//==============================================================================

/*
d_arg_internal_parse_resume
  What every call that parses starts with, whether or not it clears the
root: the last call's error and stacked arguments forgotten, then a
definition error, when there is one, reported as this call's, which ends
it.
*/
int
d_arg_internal_parse_resume(
    struct d_arg_parser* _parser
)
{
    d_arg_error_release(&_parser->error);
    _parser->pending.count = 0u;

    // a failed definition ends the parse before it starts
    if (_parser->definition.status == D_ARG_OK)
    {
        return D_ARG_OK;
    }

    return d_arg_error_set(&_parser->error,
                           _parser->definition.status,
                           _parser->definition.command,
                           _parser->definition.option,
                           _parser->definition.message);
}

/*
d_arg_internal_parse_begin
  What every parse starts with: CLI11's clear() of a root parsed before --
so values added before the first parse stay, as CLI11 keeps them -- then
the last parse's storage and program name, the values borrowing from the
storage being gone first; then the start every call that parses makes.
*/
int
d_arg_internal_parse_begin(
    struct d_arg_parser* _parser
)
{
    // a root parsed before is cleared
    if (_parser->command.parsed > 0u)
    {
        d_arg_command_clear(&_parser->command);
    }

    d_arg_string_list_release(&_parser->pool);
    free(_parser->program);
    _parser->program = NULL;

    return d_arg_internal_parse_resume(_parser);
}

/*
d_arg_internal_parse_name
  The program's name kept, then CLI11's naming of the root from `argv[0]`:
a root without a name, or one named so before, takes it again.
*/
int
d_arg_internal_parse_name(
    struct d_arg_parser* _parser,
    const char*          _name
)
{
    struct d_arg_command* const root    = &_parser->command;
    char* const                 program = d_arg_internal_copy_cstr(_name);
    const bool                  own     =
        ( (d_arg_command_name(root)[0] != '\0') &&
          (!d_arg_command_has_flag(root,
                                   D_ARG_COMMAND_AUTOMATIC_NAME)) );
    char* const                 name    = (own)
                                          ? NULL
                                          : d_arg_internal_copy_cstr(_name);

    // check if memory allocation was successful
    if ( (!program) ||
         ( (!own) &&
           (!name) ) )
    {
        free(program);
        free(name);

        return d_arg_internal_fail(root,
                                   NULL,
                                   D_ARG_ERROR_OUT_OF_MEMORY,
                                   NULL);
    }

    free(_parser->program);
    _parser->program = program;

    // a name of its own stays
    if (own)
    {
        return D_ARG_OK;
    }

    free(root->name);
    root->name   = name;
    root->flags |= D_ARG_COMMAND_AUTOMATIC_NAME;

    return D_ARG_OK;
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/*
d_arg_internal_parse_responses
  The response files, once the arguments are stacked, when the parser
reads them: each `@file` replaced by its file's words.
*/
D_STATIC int
d_arg_internal_parse_responses(
    struct d_arg_parser* _parser
)
{
    return ((_parser->flags & D_ARG_PARSER_RESPONSE_FILES) != 0u)
           ? d_arg_internal_expand_responses(_parser)
           : D_ARG_OK;
}

#else

/*
d_arg_internal_parse_responses
  Without response files built in, nothing to read.
*/
D_STATIC int
d_arg_internal_parse_responses(
    struct d_arg_parser* _parser
)
{
    (void)_parser;

    return D_ARG_OK;
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

/*
d_arg_internal_parse_stack
  The arguments after the program's name onto the stack, the last first so
that the first is on top, as CLI11 reverses them into its vector; then the
response files.
*/
D_STATIC int
d_arg_internal_parse_stack(
    struct d_arg_parser* _parser,
    int                  _argc,
    char* const*         _argv
)
{
    int status = d_arg_internal_text_reserve(&_parser->pending,
                                             (size_t)_argc);

    // the last argument first
    for (int index = _argc - 1;
         (status == D_ARG_OK) && (index > 0);
         --index)
    {
        status = d_arg_internal_pending_push(_parser,
                                             d_arg_internal_text_of(
                                                 _argv[index]));
    }

    // a refusal is the parse's failure
    if (status != D_ARG_OK)
    {
        return d_arg_internal_parse_stored(&_parser->command,
                                           status);
    }

    return d_arg_internal_parse_responses(_parser);
}

/*
d_arg_parser_parse
  CLI11's parse(argc, argv): the root named from `argv[0]` when it takes its
name from there, the rest stacked and parsed; no argument at all parses
none, as CLI11's guard against an empty `argv` does.
*/
int
d_arg_parser_parse(
    struct d_arg_parser* _parser,
    int                  _argc,
    char* const*         _argv
)
{
    // validate the parameters
    if ( (!_parser) ||
         ( (_argc > 0) &&
           (!_argv) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // every argument must be there
    for (int index = 0; index < _argc; ++index)
    {
        if (!_argv[index])
        {
            return D_ARG_ERROR_BAD_ARGUMENT;
        }
    }

    int status = d_arg_internal_parse_begin(_parser);

    // the name, then the arguments
    if ( (status == D_ARG_OK) &&
         (_argc > 0) )
    {
        status = d_arg_internal_parse_name(_parser,
                                           _argv[0]);
        status = (status == D_ARG_OK)
                 ? d_arg_internal_parse_stack(_parser,
                                              _argc,
                                              _argv)
                 : status;
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_run(_parser)
           : status;
}

/*
d_arg_internal_parse_stack_args
  Arguments given without a program name onto the stack, the first on top
whichever order they come in, borrowed or copied as asked; an empty
argument without bytes borrows the empty literal.
*/
D_STATIC int
d_arg_internal_parse_stack_args(
    struct d_arg_parser*      _parser,
    const struct d_pack_text* _args,
    size_t                    _count,
    uint32_t                  _flags
)
{
    const bool reversed = ((_flags & D_ARG_ARGS_REVERSED) != 0u);
    const bool copy     = ((_flags & D_ARG_ARGS_COPY) != 0u);
    int        status   = d_arg_internal_text_reserve(&_parser->pending,
                                                      _count);

    // the last argument first
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _count);
         ++index)
    {
        const struct d_pack_text given    = reversed
                                            ? _args[index]
                                            : _args[_count - 1u - index];
        const struct d_pack_text argument = (given.data != NULL)
                                            ? given
                                            : d_arg_internal_span("",
                                                                  0u);

        status = copy
                 ? d_arg_internal_pending_push_copy(_parser,
                                                    argument)
                 : d_arg_internal_pending_push(_parser,
                                               argument);
    }

    // a refusal is the parse's failure
    if (status != D_ARG_OK)
    {
        return d_arg_internal_parse_stored(&_parser->command,
                                           status);
    }

    return d_arg_internal_parse_responses(_parser);
}

/*
d_arg_internal_parse_words
  Words whose bytes the parser's pool keeps onto the stack, the last first,
as CLI11 reverses its vector -- from `_first` on, which no caller gives past
the words: a line stacks all of its, wmain's arguments all but the name --
then the response files.
*/
int
d_arg_internal_parse_words(
    struct d_arg_parser*          _parser,
    const struct d_arg_text_list* _words,
    size_t                        _first
)
{
    const size_t count  = _words->count;
    int          status = d_arg_internal_text_reserve(&_parser->pending,
                                                      count - _first);

    // the last word first
    for (size_t index = count;
         (status == D_ARG_OK) && (index > _first);
         --index)
    {
        status = d_arg_internal_pending_push(_parser,
                                             _words->items[index - 1u]);
    }

    // a refusal is the parse's failure
    if (status != D_ARG_OK)
    {
        return d_arg_internal_parse_stored(&_parser->command,
                                           status);
    }

    return d_arg_internal_parse_responses(_parser);
}

/*
d_arg_parser_parse_args
  CLI11's parse(vector): no program name, the arguments stacked as they come
and parsed.
*/
int
d_arg_parser_parse_args(
    struct d_arg_parser*      _parser,
    const struct d_pack_text* _args,
    size_t                    _count,
    uint32_t                  _flags
)
{
    // validate the parameters
    if ( (!_parser) ||
         ( (!_args) &&
           (_count > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // every argument with bytes must have them
    for (size_t index = 0u; index < _count; ++index)
    {
        if ( (!_args[index].data) &&
             (_args[index].length > 0u) )
        {
            return D_ARG_ERROR_BAD_ARGUMENT;
        }
    }

    int status = d_arg_internal_parse_begin(_parser);

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_stack_args(_parser,
                                               _args,
                                               _count,
                                               _flags)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_run(_parser)
           : status;
}

//==============================================================================
// 3.  SETUP
//==============================================================================

/*
d_arg_internal_parse_unlimited
  How many of a command's options taking values without end are positional
only -- all of them, or only those not required.
*/
D_STATIC size_t
d_arg_internal_parse_unlimited(
    const struct d_arg_command* _command,
    bool                        _unrequired
)
{
    size_t total = 0u;

    // each option taking values without end, by position only
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        const struct d_arg_option* const option =
            _command->options.items[index];

        if ( (d_arg_option_items_expected_max(option) >=
              (int)D_ARG_EXPECTED_UNLIMITED)               &&
             (!d_arg_option_is_nonpositional(option))      &&
             ( (!_unrequired) ||
               (!d_arg_option_has_flag(option,
                                       D_ARG_OPTION_REQUIRED)) ) )
        {
            total++;
        }
    }

    return total;
}

/*
d_arg_internal_parse_check_options
  The last checks of CLI11's _validate, on the command's requirement for
options, against its options and its nameless subcommands.
*/
D_STATIC int
d_arg_internal_parse_check_options(
    const struct d_arg_command* _command,
    size_t                      _nameless
)
{
    // no minimum, nothing to check
    if (_command->require_option_min == 0u)
    {
        return D_ARG_OK;
    }

    // a maximum below the minimum
    if ( (_command->require_option_max > 0u) &&
         (_command->require_option_max < _command->require_option_min) )
    {
        return d_arg_internal_fail(
                   _command,
                   NULL,
                   D_ARG_ERROR_INVALID,
                   "Required min options greater than required max options");
    }

    return (_command->require_option_min >
            (_command->options.count + _nameless))
           ? d_arg_internal_fail(
                 _command,
                 NULL,
                 D_ARG_ERROR_INVALID,
                 "Required min options greater than number of available "
                 "options")
           : D_ARG_OK;
}

/*
d_arg_internal_validate
  CLI11's _validate: more than one positional taking values without end,
when more than one of them is not required, cannot share the arguments;
then each subcommand in turn, counting the nameless; then the option
requirement.
*/
int
d_arg_internal_validate(
    const struct d_arg_command* _command
)
{
    size_t nameless = 0u;

    // two unlimited positionals cannot share the arguments
    if ( (d_arg_internal_parse_unlimited(_command,
                                         false) > 1u) &&
         (d_arg_internal_parse_unlimited(_command,
                                         true) > 1u) )
    {
        return d_arg_internal_fail(_command,
                                   NULL,
                                   D_ARG_ERROR_INVALID,
                                   "%s: Too many positional arguments with "
                                   "unlimited expected args",
                                   d_arg_command_name(_command));
    }

    // the subcommands, counting the nameless
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* const sub =
            _command->subcommands.items[index];
        const int                         status = d_arg_internal_validate(sub);

        if (status != D_ARG_OK)
        {
            return status;
        }

        nameless += (d_arg_command_name(sub)[0] == '\0') ? 1u : 0u;
    }

    return d_arg_internal_parse_check_options(_command,
                                              nameless);
}

/*
d_arg_internal_configure
  CLI11's _configure: each command starts disabled or enabled as its
startup says, and a nameless subcommand neither falls through nor is a
prefix command, so that nothing loops; and so on down. A subcommand never
takes its name from `argv[0]` here, so CLI11's clearing of such a name has
nothing to clear.
*/
void
d_arg_internal_configure(
    struct d_arg_command* _command
)
{
    // the startup state
    if (_command->startup == (int)D_ARG_STARTUP_ENABLED)
    {
        _command->flags &= ~D_ARG_COMMAND_DISABLED;
    }
    else if (_command->startup == (int)D_ARG_STARTUP_DISABLED)
    {
        _command->flags |= D_ARG_COMMAND_DISABLED;
    }

    // the subcommands
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const sub = _command->subcommands.items[index];

        if (d_arg_command_name(sub)[0] == '\0')
        {
            sub->flags          &= ~D_ARG_COMMAND_FALLTHROUGH;
            sub->prefix_command  = (int)D_ARG_PREFIX_OFF;
        }

        d_arg_internal_configure(sub);
    }

    return;
}

/*
d_arg_internal_parse_run
  CLI11's _parse_setup, _parse and run_callback, once the parser is
cleared: the root counts a use while the checks run -- so a failed check
still clears at the next parse, as CLI11's does -- and none once
configured; then the root parses the stack, processing what it parsed, and
the callbacks run.
*/
int
d_arg_internal_parse_run(
    struct d_arg_parser* _parser
)
{
    struct d_arg_command* const root = &_parser->command;

    root->parsed = 1u;

    int status = d_arg_internal_validate(root);

    // configured, then parsed
    if (status == D_ARG_OK)
    {
        d_arg_internal_configure(root);
        root->parsed = 0u;
        status       = d_arg_internal_parse_command(root);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_run_callback(root,
                                         false,
                                         false)
           : status;
}

//==============================================================================
// 4.  RECOGNIZING
//==============================================================================

/*
d_arg_internal_parse_at
  What follows an option's dashes or slash, cut at its first `_separator`:
the name before it, and the value after it -- empty, at the end of the
text, when there is none.
*/
void
d_arg_internal_parse_at(
    struct d_pack_text  _text,
    char                _separator,
    struct d_pack_text* _name,
    struct d_pack_text* _value
)
{
    const char* const at     = (const char*)memchr(_text.data,
                                                   _separator,
                                                   _text.length);
    const size_t      length = (at != NULL)
                               ? (size_t)(at - _text.data)
                               : _text.length;

    *_name  = d_arg_internal_span(_text.data,
                                  length);
    *_value = (at != NULL)
              ? d_arg_internal_span(at + 1,
                                    _text.length - length - 1u)
              : d_arg_internal_span(_text.data + _text.length,
                                    0u);

    return;
}

/*
d_arg_internal_parse_split_long
  CLI11's split_long: `--name` or `--name=value`, the name's first
character valid.
*/
bool
d_arg_internal_parse_split_long(
    struct d_pack_text  _argument,
    struct d_pack_text* _name,
    struct d_pack_text* _value
)
{
    // two dashes and a name
    if ( (_argument.length <= 2u)                              ||
         (_argument.data[0] != '-')                            ||
         (_argument.data[1] != '-')                            ||
         (!d_arg_internal_valid_first_char(_argument.data[2])) )
    {
        return false;
    }

    d_arg_internal_parse_at(d_arg_internal_span(_argument.data + 2,
                                                _argument.length - 2u),
                            '=',
                            _name,
                            _value);

    return true;
}

/*
d_arg_internal_parse_split_short
  CLI11's split_short: `-x` and whatever follows it, `x` a valid first
character.
*/
bool
d_arg_internal_parse_split_short(
    struct d_pack_text  _argument,
    struct d_pack_text* _name,
    struct d_pack_text* _rest
)
{
    // one dash and a name
    if ( (_argument.length <= 1u)                              ||
         (_argument.data[0] != '-')                            ||
         (!d_arg_internal_valid_first_char(_argument.data[1])) )
    {
        return false;
    }

    *_name = d_arg_internal_span(_argument.data + 1,
                                 1u);
    *_rest = d_arg_internal_span(_argument.data + 2,
                                 _argument.length - 2u);

    return true;
}

/*
d_arg_internal_parse_split_windows
  CLI11's split_windows_style: `/name` or `/name:value`.
*/
bool
d_arg_internal_parse_split_windows(
    struct d_pack_text  _argument,
    struct d_pack_text* _name,
    struct d_pack_text* _value
)
{
    // a slash and a name
    if ( (_argument.length <= 1u)                              ||
         (_argument.data[0] != '/')                            ||
         (!d_arg_internal_valid_first_char(_argument.data[1])) )
    {
        return false;
    }

    d_arg_internal_parse_at(d_arg_internal_span(_argument.data + 1,
                                                _argument.length - 1u),
                            ':',
                            _name,
                            _value);

    return true;
}

/*
d_arg_internal_parse_valid_subcommand
  CLI11's _valid_subcommand: a command that has met its maximum of
subcommands asks only its parent, and only with subcommand fallthrough;
otherwise a subcommand of its own, then, with subcommand fallthrough, its
parent's.
*/
D_STATIC bool
d_arg_internal_parse_valid_subcommand(
    const struct d_arg_command* _command,
    struct d_pack_text          _argument,
    bool                        _ignore_used
)
{
    const bool asks_parent = ( (d_arg_command_has_flag(
                                    _command,
                                    D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH)) &&
                               (_command->parent != NULL) );

    // the maximum met
    if ( (_command->require_subcommand_max != 0u) &&
         (_command->parsed_subcommands.count >=
          _command->require_subcommand_max) )
    {
        return ( (asks_parent) &&
                 (d_arg_internal_parse_valid_subcommand(_command->parent,
                                                        _argument,
                                                        _ignore_used)) );
    }

    return ( (d_arg_internal_find_subcommand(_command,
                                             _argument,
                                             true,
                                             _ignore_used) != NULL) ||
             ( (asks_parent) &&
               (d_arg_internal_parse_valid_subcommand(_command->parent,
                                                      _argument,
                                                      _ignore_used)) ) );
}

/*
d_arg_internal_parse_numeric
  CLI11's test in _recognize of a short argument that reads as a number --
a digit after the dash, or a dot and then a digit -- which is a positional
unless an option answers to the dash and that first character.
*/
D_STATIC bool
d_arg_internal_parse_numeric(
    const struct d_arg_command* _command,
    struct d_pack_text          _name,
    struct d_pack_text          _rest
)
{
    const char short_name[2] = { '-',
                                 _name.data[0] };

    // not a number
    if ( (!d_arg_internal_is_digit(_name.data[0]))   &&
         ( (_name.data[0] != '.')                  ||
           (_rest.length == 0u)                    ||
           (!d_arg_internal_is_digit(_rest.data[0])) ) )
    {
        return false;
    }

    return (d_arg_internal_find_option(_command,
                                       d_arg_internal_span(short_name,
                                                           2u)) == NULL);
}

/*
d_arg_internal_parse_option_form
  The option forms of CLI11's _recognize, in its order: long; short --
unless it reads as a number no option claims, which makes it a positional;
Windows-style where the command takes it. Whether the argument has one of
them, `_class` receiving which.
*/
D_STATIC bool
d_arg_internal_parse_option_form(
    const struct d_arg_command* _command,
    struct d_pack_text          _argument,
    int*                        _class
)
{
    struct d_pack_text name = d_arg_internal_span(_argument.data,
                                                  0u);
    struct d_pack_text tail = d_arg_internal_span(_argument.data,
                                                  0u);

    // `--name`
    if (d_arg_internal_parse_split_long(_argument,
                                        &name,
                                        &tail))
    {
        *_class = (int)D_ARG_CLASS_LONG;

        return true;
    }

    // `-x`, unless it is a number
    if (d_arg_internal_parse_split_short(_argument,
                                         &name,
                                         &tail))
    {
        *_class = d_arg_internal_parse_numeric(_command,
                                               name,
                                               tail)
                  ? (int)D_ARG_CLASS_NONE
                  : (int)D_ARG_CLASS_SHORT;

        return true;
    }

    *_class = (int)D_ARG_CLASS_WINDOWS_STYLE;

    return ( (d_arg_command_has_flag(_command,
                                     D_ARG_COMMAND_WINDOWS_STYLE)) &&
             (d_arg_internal_parse_split_windows(_argument,
                                                 &name,
                                                 &tail)) );
}

/*
d_arg_internal_parse_dotted
  The last test of CLI11's _recognize: `sub.rest`, where `sub` names a
subcommand by which `rest` is recognized as a subcommand in turn.
*/
D_STATIC bool
d_arg_internal_parse_dotted(
    const struct d_arg_command* _command,
    struct d_pack_text          _argument,
    bool                        _ignore_used
)
{
    const char* const dot = (const char*)memchr(_argument.data,
                                                '.',
                                                _argument.length);

    // no dot, nothing to try
    if (!dot)
    {
        return false;
    }

    const size_t                      at  = (size_t)(dot - _argument.data);
    const struct d_arg_command* const sub = d_arg_internal_find_subcommand(
                                                _command,
                                                d_arg_internal_span(
                                                    _argument.data,
                                                    at),
                                                true,
                                                _ignore_used);

    return ( (sub != NULL) &&
             (d_arg_internal_recognize(sub,
                                       d_arg_internal_span(
                                           dot + 1,
                                           _argument.length - at - 1u),
                                       _ignore_used) ==
              (int)D_ARG_CLASS_SUBCOMMAND) );
}

/*
d_arg_internal_recognize
  CLI11's _recognize, in its order: the `--` mark, a subcommand, an
option's form, the `++` that ends a named subcommand, a dotted subcommand;
anything else is a positional.
*/
int
d_arg_internal_recognize(
    const struct d_arg_command* _command,
    struct d_pack_text          _argument,
    bool                        _ignore_used
)
{
    int option_class = (int)D_ARG_CLASS_NONE;

    // the mark
    if (d_arg_internal_equal_cstr(_argument,
                                  "--"))
    {
        return (int)D_ARG_CLASS_POSITIONAL_MARK;
    }

    // a subcommand, here or through subcommand fallthrough
    if (d_arg_internal_parse_valid_subcommand(_command,
                                              _argument,
                                              _ignore_used))
    {
        return (int)D_ARG_CLASS_SUBCOMMAND;
    }

    // an option
    if (d_arg_internal_parse_option_form(_command,
                                         _argument,
                                         &option_class))
    {
        return option_class;
    }

    // the end of a named subcommand
    if ( (d_arg_internal_equal_cstr(_argument,
                                    "++"))         &&
         (d_arg_command_name(_command)[0] != '\0') &&
         (_command->parent != NULL) )
    {
        return (int)D_ARG_CLASS_SUBCOMMAND_TERMINATOR;
    }

    return d_arg_internal_parse_dotted(_command,
                                       _argument,
                                       _ignore_used)
           ? (int)D_ARG_CLASS_SUBCOMMAND
           : (int)D_ARG_CLASS_NONE;
}

//==============================================================================
// 5.  UNUSED ARGUMENTS
//==============================================================================

/*
d_arg_internal_parse_capture
  CLI11's capture_extras: the modes in which a command keeps what it does
not recognize for remaining().
*/
D_STATIC bool
d_arg_internal_parse_capture(
    int _mode
)
{
    return ( (_mode == (int)D_ARG_EXTRAS_CAPTURE)         ||
             (_mode == (int)D_ARG_EXTRAS_ASSUME_SINGLE)   ||
             (_mode == (int)D_ARG_EXTRAS_ASSUME_MULTIPLE) );
}

/*
d_arg_internal_parse_extras_join
  The arguments of CLI11's ExtrasError, each after a space, joined as
CLI11's join joins them -- a space that ends what it joined dropped. The
arguments are a stack's, read from its top -- its last -- down, or the
texts of unused arguments, in order.
*/
D_STATIC void
d_arg_internal_parse_extras_join(
    struct d_arg_buffer*      _text,
    const struct d_pack_text* _stack,
    const struct d_arg_extra* _unused,
    size_t                    _count
)
{
    const size_t joined = _text->length + 1u;

    // each argument, after a space
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_char(_text,
                                       ' ');
        (void)d_arg_buffer_append_text(_text,
                                       (_stack != NULL)
                                       ? _stack[_count - 1u - index]
                                       : _unused[index].text);
    }

    // CLI11's join drops a delimiter that ends what it joined
    if ( (_text->length > joined) &&
         (_text->data[_text->length - 1u] == ' ') )
    {
        d_arg_buffer_truncate(_text,
                              _text->length - 1u);
    }

    return;
}

/*
d_arg_internal_parse_extras_error
  CLI11's ExtrasError(name, args): "<name>: The following argument was not
expected: <arg>", or "arguments were" for several, joined as
d_arg_internal_parse_extras_join joins them, the name's part left out for
a command without a name; then the suggestions the parser asks for.
*/
int
d_arg_internal_parse_extras_error(
    const struct d_arg_command* _command,
    const struct d_pack_text*   _stack,
    const struct d_arg_extra*   _unused,
    size_t                      _count
)
{
    struct d_arg_buffer text = D_ARG_BUFFER_INIT;
    const char* const   name = d_arg_command_name(_command);

    // the command's name, when it has one
    if (name[0] != '\0')
    {
        (void)d_arg_buffer_append_cstr(&text,
                                       name);
        (void)d_arg_buffer_append_cstr(&text,
                                       ": ");
    }

    (void)d_arg_buffer_append_cstr(&text,
                                   (_count > 1u)
                                   ? MANY_NOT_EXPECTED
                                   : ONE_NOT_EXPECTED);
    d_arg_internal_parse_extras_join(&text,
                                     _stack,
                                     _unused,
                                     _count);
    d_arg_internal_suggest_extras(_command,
                                  _stack,
                                  _unused,
                                  _count,
                                  &text);

    const int status = (text.failed == 0)
                       ? d_arg_internal_fail_text(_command,
                                                  NULL,
                                                  D_ARG_ERROR_EXTRAS,
                                                  d_arg_buffer_text(&text))
                       : d_arg_internal_parse_stored(
                             _command,
                             D_ARG_ERROR_OUT_OF_MEMORY);

    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_internal_parse_keep
  An argument kept among a command's unused arguments, with what it was
taken to be.
*/
D_STATIC int
d_arg_internal_parse_keep(
    struct d_arg_command* _command,
    int                   _class,
    struct d_pack_text    _argument
)
{
    return d_arg_internal_parse_stored(
               _command,
               d_arg_internal_extra_push(&_command->missing,
                                         _argument,
                                         _class));
}

/*
d_arg_internal_parse_to_missing
  CLI11's _move_to_missing: an immediate error; or the command's unused
arguments, where it captures them, has no subcommands or is a prefix
command; or those of its first option group that captures them; or the
command's -- each place but the error passed over when extras are ignored.
*/
int
d_arg_internal_parse_to_missing(
    struct d_arg_command* _command,
    int                   _class,
    struct d_pack_text    _argument
)
{
    const int  mode = _command->allow_extras;
    const bool kept = (mode != (int)D_ARG_EXTRAS_IGNORE);

    // an error at once
    if (mode == (int)D_ARG_EXTRAS_ERROR_IMMEDIATELY)
    {
        const struct d_arg_extra extra =
        {
            _argument,  // text
            _class      // classifier
        };

        return d_arg_internal_parse_extras_error(_command,
                                                 NULL,
                                                 &extra,
                                                 1u);
    }

    // kept here
    if ( (d_arg_internal_parse_capture(mode))                 ||
         (_command->subcommands.count == 0u)                  ||
         (_command->prefix_command != (int)D_ARG_PREFIX_OFF) )
    {
        return (kept)
               ? d_arg_internal_parse_keep(_command,
                                           _class,
                                           _argument)
               : D_ARG_OK;
    }

    // an option group that captures them
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const group = _command->subcommands.items[index];

        if ( (d_arg_command_name(group)[0] == '\0') &&
             (d_arg_internal_parse_capture(group->allow_extras)) )
        {
            return d_arg_internal_parse_keep(group,
                                             _class,
                                             _argument);
        }
    }

    return (kept)
           ? d_arg_internal_parse_keep(_command,
                                       _class,
                                       _argument)
           : D_ARG_OK;
}

/*
d_arg_internal_parse_rest_to_missing
  Everything left on the stack kept among the command's unused arguments as
positionals, whatever its extras mode, as CLI11's prefix commands keep it.
*/
int
d_arg_internal_parse_rest_to_missing(
    struct d_arg_command* _command
)
{
    int status = D_ARG_OK;

    // each argument left, in order
    while ( (status == D_ARG_OK)                       &&
            (d_arg_internal_parse_left(_command) > 0u) )
    {
        status = d_arg_internal_parse_keep(_command,
                                           (int)D_ARG_CLASS_NONE,
                                           d_arg_internal_parse_back(
                                               _command));
        d_arg_internal_parse_pop(_command);
    }

    return status;
}

//==============================================================================
// 6.  VALUES
//==============================================================================

/*
d_arg_internal_parse_remaining
  CLI11's _count_remaining_positionals: the values the command's
positionals -- the required ones only, when asked -- still need to reach
their minimums.
*/
size_t
d_arg_internal_parse_remaining(
    const struct d_arg_command* _command,
    bool                        _required
)
{
    size_t total = 0u;

    // each positional short of its minimum
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        const struct d_arg_option* const option =
            _command->options.items[index];
        const int                        minimum =
            d_arg_option_items_expected_min(option);
        const int                        count   =
            (int)d_arg_option_count(option);

        if ( (d_arg_option_is_positional(option))               &&
             ( (!_required) ||
               (d_arg_option_has_flag(option,
                                      D_ARG_OPTION_REQUIRED)) ) &&
             (minimum > 0)                                      &&
             (count < minimum) )
        {
            total += (size_t)(minimum - count);
        }
    }

    return total;
}

/*
d_arg_internal_parse_has_remaining
  CLI11's _has_remaining_positionals: any positional short of its minimum.
*/
D_STATIC bool
d_arg_internal_parse_has_remaining(
    const struct d_arg_command* _command
)
{
    // each positional
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        const struct d_arg_option* const option =
            _command->options.items[index];

        if ( (d_arg_option_is_positional(option))           &&
             ((int)d_arg_option_count(option) <
              d_arg_option_items_expected_min(option)) )
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_parse_judge
  CLI11's opt->_validate(value, 0) as the parse asks it, of a positional or
an optional value: whether the option's validators accept the value as
given. A validator that stops with another error ends the parse with it,
as the exception escapes CLI11's.
*/
int
d_arg_internal_parse_judge(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option,
    struct d_pack_text          _value,
    bool*                       _accepted
)
{
    struct d_arg_buffer message = D_ARG_BUFFER_INIT;
    int                 status  = d_arg_internal_option_judge(_option,
                                                              _value,
                                                              0,
                                                              &message,
                                                              NULL);

    *_accepted = ( (status == D_ARG_OK) &&
                   (message.length == 0u) );

    // a validator that stopped the parse
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_fail(_command,
                                     _option,
                                     status,
                                     (message.length > 0u) ? "%s" : NULL,
                                     message.data);
    }

    d_arg_buffer_release(&message);

    return status;
}

/*
d_arg_internal_parse_restart
  CLI11's clear() of an option that triggers on parse, met again after its
callback ran: no values and PARSING -- the values first set aside in
`_set_aside`, when asked. The copies values lie in stay with the option
until it is next cleared, so a value set aside, or a variable bound to one,
outlives the restart.
*/
void
d_arg_internal_parse_restart(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _set_aside
)
{
    const struct d_arg_text_list none = D_ARG_LIST_INIT;

    // the values set aside, or simply dropped
    if (_set_aside != NULL)
    {
        *_set_aside      = _option->results;
        _option->results = none;
    }
    else
    {
        _option->results.count = 0u;
    }

    _option->processed.count = 0u;
    _option->state           = (int)D_ARG_OPTION_STATE_PARSING;

    return;
}

/*
d_arg_internal_parse_add
  CLI11's add_result(value): a value from the stack, which outlives the
option's values, added as given -- split, perhaps -- and the option
PARSING.
*/
int
d_arg_internal_parse_add(
    struct d_arg_option* _option,
    struct d_pack_text   _value,
    int*                 _added
)
{
    const int status = d_arg_internal_option_add(_option,
                                                 &_option->results,
                                                 _value,
                                                 true,
                                                 _added);

    _option->state = (int)D_ARG_OPTION_STATE_PARSING;

    return status;
}

/*
d_arg_internal_parse_separate
  CLI11's separator between the values of two occurrences: an empty value,
for an option that asks for one, after a last value that is not empty.
*/
int
d_arg_internal_parse_separate(
    struct d_arg_option* _option
)
{
    const struct d_arg_text_list* const results = &_option->results;

    // only where asked, after a value
    if ( (!d_arg_option_has_flag(_option,
                                 D_ARG_OPTION_INJECT_SEPARATOR))  ||
         (results->count == 0u)                                   ||
         (results->items[results->count - 1u].length == 0u) )
    {
        return D_ARG_OK;
    }

    return d_arg_internal_parse_add(_option,
                                    d_arg_internal_span("",
                                                        0u),
                                    NULL);
}

//==============================================================================
// 7.  SUBCOMMANDS
//==============================================================================

/*
d_arg_internal_parse_below_max
  Whether a command may take another subcommand: no maximum, or one not
met.
*/
bool
d_arg_internal_parse_below_max(
    const struct d_arg_command* _command
)
{
    return ( (_command->require_subcommand_max == 0u) ||
             (_command->require_subcommand_max >
              _command->parsed_subcommands.count) );
}

/*
d_arg_internal_parse_open_group
  Whether a subcommand is an option group that takes arguments: nameless
and enabled.
*/
bool
d_arg_internal_parse_open_group(
    const struct d_arg_command* _command
)
{
    return ( (d_arg_command_name(_command)[0] == '\0') &&
             (!d_arg_command_has_flag(_command,
                                      D_ARG_COMMAND_DISABLED)) );
}

/*
d_arg_internal_parse_entered
  CLI11's pre-parse step for an option group that took an argument: once,
with the count of arguments left.
*/
int
d_arg_internal_parse_entered(
    struct d_arg_command* _group
)
{
    return (!d_arg_command_has_flag(_group,
                                    D_ARG_COMMAND_PRE_PARSE_CALLED))
           ? d_arg_internal_trigger_pre_parse(_group,
                                              d_arg_internal_parse_left(
                                                  _group))
           : D_ARG_OK;
}

/*
d_arg_internal_parse_list
  A subcommand listed among a command's parsed subcommands, unless it is
silent.
*/
int
d_arg_internal_parse_list(
    struct d_arg_command* _command,
    struct d_arg_command* _sub
)
{
    return d_arg_command_has_flag(_sub,
                                  D_ARG_COMMAND_SILENT)
           ? D_ARG_OK
           : d_arg_internal_parse_stored(
                 _command,
                 d_arg_internal_command_push(&_command->parsed_subcommands,
                                             _sub));
}

/*
d_arg_internal_parse_take_name
  The subcommand an argument names, the name taken off the stack: CLI11's
lookup in _parse_subcommand by the whole argument, else -- the main way to
get here -- by the part before its first dot, the rest then left on the
stack in its place for the subcommand to parse. NULL, the stack as it was,
when neither names one.
*/
D_STATIC struct d_arg_command*
d_arg_internal_parse_take_name(
    struct d_arg_command* _command
)
{
    struct d_arg_text_list* const pending  = &_command->parser->pending;
    const struct d_pack_text      argument = d_arg_internal_parse_back(
                                                 _command);
    struct d_arg_command* const   whole    = d_arg_internal_find_subcommand(
                                                 _command,
                                                 argument,
                                                 true,
                                                 true);

    // the whole argument names it
    if (whole != NULL)
    {
        pending->count--;

        return whole;
    }

    const char* const dot = (const char*)memchr(argument.data,
                                                '.',
                                                argument.length);

    // no dot to name one by
    if (!dot)
    {
        return NULL;
    }

    const size_t                at     = (size_t)(dot - argument.data);
    struct d_arg_command* const dotted = d_arg_internal_find_subcommand(
                                             _command,
                                             d_arg_internal_span(
                                                 argument.data,
                                                 at),
                                             true,
                                             true);

    // `sub.rest`: the rest stays, for the subcommand
    if (dotted != NULL)
    {
        pending->items[pending->count - 1u] =
            d_arg_internal_span(dot + 1,
                                argument.length - at - 1u);
    }

    return dotted;
}

/*
d_arg_internal_parse_enter
  CLI11's parse of a subcommand named in _parse_subcommand: listed among
the command's parsed subcommands unless silent, then parsed; then each
option group between the two -- when it was found in one -- has its
pre-parse step and lists it too.
*/
D_STATIC int
d_arg_internal_parse_enter(
    struct d_arg_command* _command,
    struct d_arg_command* _sub
)
{
    int status = d_arg_internal_parse_list(_command,
                                           _sub);

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_command(_sub)
             : status;

    struct d_arg_command* group = _sub->parent;

    // each option group between, the innermost first
    while ( (status == D_ARG_OK) &&
            (group != NULL)      &&
            (group != _command) )
    {
        status = d_arg_internal_trigger_pre_parse(
                     group,
                     d_arg_internal_parse_left(_command));
        status = (status == D_ARG_OK)
                 ? d_arg_internal_parse_list(group,
                                             _sub)
                 : status;
        group  = group->parent;
    }

    return status;
}

/*
d_arg_internal_parse_subcommand
  CLI11's _parse_subcommand: while a required positional still waits for
values, a subcommand's name is a positional; otherwise the subcommand it
names is parsed. A name only a parent's subcommand answers to, through
subcommand fallthrough, ends the command -- and at the root, where none
can, is CLI11's HorribleError. Where CLI11 goes on after a positional
nothing took, meeting it again for ever, the command ends instead, leaving
it to its parent.
*/
D_STATIC int
d_arg_internal_parse_subcommand(
    struct d_arg_command* _command,
    bool*                 _go_on
)
{
    // a required positional still waits
    if (d_arg_internal_parse_remaining(_command,
                                       true) > 0u)
    {
        return d_arg_internal_parse_positional(_command,
                                               false,
                                               _go_on);
    }

    const struct d_pack_text    argument = d_arg_internal_parse_back(_command);
    struct d_arg_command* const sub      = d_arg_internal_parse_take_name(
                                               _command);

    *_go_on = (sub != NULL);

    // a subcommand of the command's
    if (sub != NULL)
    {
        return d_arg_internal_parse_enter(_command,
                                          sub);
    }

    return (_command->parent != NULL)
           ? D_ARG_OK
           : d_arg_internal_fail(_command,
                                 NULL,
                                 D_ARG_ERROR_HORRIBLE,
                                 "Subcommand %.*s missing",
                                 d_arg_internal_print_length(argument),
                                 argument.data);
}

//==============================================================================
// 8.  COMMANDS
//==============================================================================

/*
d_arg_internal_increment_parsed
  CLI11's increment_parsed: the command, and its nameless subcommands in
turn.
*/
void
d_arg_internal_increment_parsed(
    struct d_arg_command* _command
)
{
    _command->parsed++;

    // the nameless subcommands count with it
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        if (d_arg_command_name(_command->subcommands.items[index])[0] == '\0')
        {
            d_arg_internal_increment_parsed(
                _command->subcommands.items[index]);
        }
    }

    return;
}

/*
d_arg_internal_parse_preparse
  A command's pre-parse callback, watched, so that a failure it returns
without recording is recorded.
*/
D_STATIC int
d_arg_internal_parse_preparse(
    struct d_arg_command* _command,
    size_t                _remaining
)
{
    struct d_arg_internal_watch watch = { NULL,
                                          D_ARG_ERROR_INIT };

    d_arg_internal_watch_begin(&watch,
                               _command->parser);

    const int status = _command->preparse_fn(_command,
                                             _remaining,
                                             _command->preparse_context);

    return d_arg_internal_watch_close(&watch,
                                      _command,
                                      status);
}

/*
d_arg_internal_parse_fresh
  CLI11's fresh start of an immediate-callback subcommand met again: its
clear(), keeping its count and its unused arguments, with its pre-parse
step counted as done.
*/
D_STATIC void
d_arg_internal_parse_fresh(
    struct d_arg_command* _command
)
{
    const size_t                  parsed = _command->parsed;
    const struct d_arg_extra_list extras = _command->missing;
    const struct d_arg_extra_list none   = D_ARG_LIST_INIT;

    _command->missing = none;
    d_arg_command_clear(_command);
    _command->parsed   = parsed;
    _command->flags   |= D_ARG_COMMAND_PRE_PARSE_CALLED;
    _command->missing  = extras;

    return;
}

/*
d_arg_internal_trigger_pre_parse
  CLI11's _trigger_pre_parse: the first time, the pre-parse callback, if
there is one, with the count of arguments left; met again, a named
subcommand that calls back at once starts afresh.
*/
int
d_arg_internal_trigger_pre_parse(
    struct d_arg_command* _command,
    size_t                _remaining
)
{
    // the first time
    if (!d_arg_command_has_flag(_command,
                                D_ARG_COMMAND_PRE_PARSE_CALLED))
    {
        _command->flags |= D_ARG_COMMAND_PRE_PARSE_CALLED;

        return (_command->preparse_fn != NULL)
               ? d_arg_internal_parse_preparse(_command,
                                               _remaining)
               : D_ARG_OK;
    }

    // met again: a named immediate-callback subcommand's fresh start
    if ( (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_IMMEDIATE_CALLBACK)) &&
         (d_arg_command_name(_command)[0] != '\0') )
    {
        d_arg_internal_parse_fresh(_command);
    }

    return D_ARG_OK;
}

/*
d_arg_internal_parse_mark
  CLI11's `--`, taken off the stack: a prefix command keeps it and all that
follows among its unused arguments, whatever its extras mode; a subcommand
whose positionals have all they need ends at it, leaving what follows to
its parent; anything else keeps it as an unused argument.
*/
D_STATIC int
d_arg_internal_parse_mark(
    struct d_arg_command* _command,
    struct d_pack_text    _mark,
    bool*                 _go_on
)
{
    // a prefix command keeps the rest
    if (_command->prefix_command != (int)D_ARG_PREFIX_OFF)
    {
        const int status = d_arg_internal_parse_keep(
                               _command,
                               (int)D_ARG_CLASS_POSITIONAL_MARK,
                               _mark);

        return (status == D_ARG_OK)
               ? d_arg_internal_parse_rest_to_missing(_command)
               : status;
    }

    // a subcommand done with its positionals
    if ( (!d_arg_internal_parse_has_remaining(_command)) &&
         (_command->parent != NULL) )
    {
        *_go_on = false;

        return D_ARG_OK;
    }

    return d_arg_internal_parse_to_missing(_command,
                                           (int)D_ARG_CLASS_POSITIONAL_MARK,
                                           _mark);
}

/*
d_arg_internal_parse_lone
  CLI11's step for an argument recognized as nothing else: probably a
positional, or something for a parent; one taken under positionals-at-end
makes every later argument a positional.
*/
D_STATIC int
d_arg_internal_parse_lone(
    struct d_arg_command* _command,
    bool*                 _positional_only,
    bool*                 _go_on
)
{
    const int status = d_arg_internal_parse_positional(_command,
                                                       false,
                                                       _go_on);

    // positionals at the end: the rest are positionals
    if ( (status == D_ARG_OK)                                   &&
         (*_go_on)                                              &&
         (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_POSITIONALS_AT_END)) )
    {
        *_positional_only = true;
    }

    return status;
}

/*
d_arg_internal_parse_single
  CLI11's _parse_single: the next argument recognized -- or, once the
positionals stand alone, taken as one -- and handed to its step; the `++`
that ends a named subcommand is taken off the stack and ends it.
*/
D_STATIC int
d_arg_internal_parse_single(
    struct d_arg_command* _command,
    bool*                 _positional_only,
    bool*                 _go_on
)
{
    const struct d_pack_text argument   = d_arg_internal_parse_back(_command);
    const int                classifier = (*_positional_only)
                                          ? (int)D_ARG_CLASS_NONE
                                          : d_arg_internal_recognize(
                                                _command,
                                                argument,
                                                true);
    int                      status     = D_ARG_OK;

    *_go_on = true;

    switch (classifier)
    {
        case D_ARG_CLASS_POSITIONAL_MARK:
            d_arg_internal_parse_pop(_command);
            *_positional_only = true;
            status            = d_arg_internal_parse_mark(_command,
                                                          argument,
                                                          _go_on);
            break;

        case D_ARG_CLASS_SUBCOMMAND_TERMINATOR:
            d_arg_internal_parse_pop(_command);
            *_go_on = false;
            break;

        case D_ARG_CLASS_SUBCOMMAND:
            status = d_arg_internal_parse_subcommand(_command,
                                                     _go_on);
            break;

        case D_ARG_CLASS_LONG:
        case D_ARG_CLASS_SHORT:
        case D_ARG_CLASS_WINDOWS_STYLE:
            status = d_arg_internal_parse_arg(_command,
                                              classifier,
                                              false,
                                              _go_on);
            break;

        default:
            status = d_arg_internal_parse_lone(_command,
                                               _positional_only,
                                               _go_on);
            break;
    }

    return status;
}

/*
d_arg_internal_parse_command
  CLI11's _parse(args): the command counts a use and has its pre-parse
step, then takes arguments until they run out or one belongs elsewhere;
the root then processes what it parsed and reports what it did not use,
and a subcommand with a parse-complete callback runs its completion
callbacks.
*/
int
d_arg_internal_parse_command(
    struct d_arg_command* _command
)
{
    bool positional_only = false;
    bool go_on           = true;

    d_arg_internal_increment_parsed(_command);

    int status = d_arg_internal_trigger_pre_parse(
                     _command,
                     d_arg_internal_parse_left(_command));

    // one argument at a time, until they run out or one belongs elsewhere
    while ( (status == D_ARG_OK)                       &&
            (go_on)                                    &&
            (d_arg_internal_parse_left(_command) > 0u) )
    {
        status = d_arg_internal_parse_single(_command,
                                             &positional_only,
                                             &go_on);
    }

    // a failure, or a subcommand without completion callbacks
    if ( (status != D_ARG_OK)           ||
         ( (_command->parent != NULL) &&
           (!_command->parse_complete_fn) ) )
    {
        return status;
    }

    // the root's processing
    if (_command->parent == NULL)
    {
        status = d_arg_internal_process(_command);

        return (status == D_ARG_OK)
               ? d_arg_internal_process_extras(_command)
               : status;
    }

    return d_arg_internal_process_completion_callbacks(_command,
                                                       true);
}
