/*******************************************************************************
* djinterp [parse]                                            arg_parse_option.c
*
* The option step of the parse, declared in arg_internal.h (8.1.7).
*   CLI11's _parse_arg: an argument recognized as an option is split into
* its name, its value and what follows a short name, and taken by the
* command's option that answers to the name -- or one an option group, a
* non-standard short name, CLI11's dot notation or the fallthrough parent
* finds -- which takes its values from the argument and those after it;
* otherwise it is an unused argument, with what follows it as the extras
* mode keeps that.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_parse_option.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser


//==============================================================================
// 1.  SPLITTING
//==============================================================================

// d_arg_internal_parse_split
//   struct: an option argument as CLI11's _parse_arg splits it: the whole
// argument; the name, without its dashes or slash; the value after `=` or
// `:`; what follows a short name; and the class it was recognized as. Each
// part is a span of the argument, or of the parser's pool.
struct d_arg_internal_parse_split
{
    struct d_pack_text argument;
    struct d_pack_text name;
    struct d_pack_text value;
    struct d_pack_text rest;
    int                classifier;
};

// SPLIT_NONE
//   constant: a split of nothing, from which an argument's is made.
static const struct d_arg_internal_parse_split SPLIT_NONE;

/*
d_arg_internal_parse_split_arg
  The argument on top of the stack split as its class splits it, the value
and the rest starting empty at its end; CLI11's HorribleError when it does
not split, which recognizing never lets happen.
*/
D_STATIC int
d_arg_internal_parse_split_arg(
    const struct d_arg_command*        _command,
    int                                _class,
    struct d_arg_internal_parse_split* _split
)
{
    const struct d_pack_text argument = d_arg_internal_parse_back(_command);

    _split->argument   = argument;
    _split->value      = d_arg_internal_span(argument.data + argument.length,
                                             0u);
    _split->rest       = _split->value;
    _split->classifier = _class;

    // `-x` and what follows
    if (_class == (int)D_ARG_CLASS_SHORT)
    {
        return d_arg_internal_parse_split_short(argument,
                                                &_split->name,
                                                &_split->rest)
               ? D_ARG_OK
               : d_arg_internal_fail(_command,
                                     NULL,
                                     D_ARG_ERROR_HORRIBLE,
                                     "Short parsed but missing! You should "
                                     "not see this");
    }

    // `--name` and its value
    if (_class == (int)D_ARG_CLASS_LONG)
    {
        return d_arg_internal_parse_split_long(argument,
                                               &_split->name,
                                               &_split->value)
               ? D_ARG_OK
               : d_arg_internal_fail(_command,
                                     NULL,
                                     D_ARG_ERROR_HORRIBLE,
                                     "Long parsed but missing (you should "
                                     "not see this):%.*s",
                                     d_arg_internal_print_length(argument),
                                     argument.data);
    }

    return d_arg_internal_parse_split_windows(argument,
                                              &_split->name,
                                              &_split->value)
           ? D_ARG_OK
           : d_arg_internal_fail(_command,
                                 NULL,
                                 D_ARG_ERROR_HORRIBLE,
                                 "windows option parsed but missing! You "
                                 "should not see this");
}

/*
d_arg_internal_parse_answers
  CLI11's match of an option to a split name: by its long names for a long
argument, its short names for a short one, either for a Windows-style one.
*/
D_STATIC bool
d_arg_internal_parse_answers(
    const struct d_arg_option* _option,
    int                        _class,
    struct d_pack_text         _name
)
{
    // a long argument
    if (_class == (int)D_ARG_CLASS_LONG)
    {
        return d_arg_option_check_long_name(_option,
                                            _name);
    }

    // a short one
    if (_class == (int)D_ARG_CLASS_SHORT)
    {
        return d_arg_option_check_short_name(_option,
                                             _name);
    }

    return ( (d_arg_option_check_long_name(_option,
                                           _name))  ||
             (d_arg_option_check_short_name(_option,
                                            _name)) );
}

/*
d_arg_internal_parse_own_option
  The command's own option, the first that answers to the split name.
*/
D_STATIC struct d_arg_option*
d_arg_internal_parse_own_option(
    const struct d_arg_command*              _command,
    const struct d_arg_internal_parse_split* _split
)
{
    // each option in turn
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        if (d_arg_internal_parse_answers(_command->options.items[index],
                                         _split->classifier,
                                         _split->name))
        {
            return _command->options.items[index];
        }
    }

    return NULL;
}

//==============================================================================
// 2.  SEARCHING
//==============================================================================

/*
d_arg_internal_parse_group_arg
  CLI11's offer of an option argument to the command's option groups, each
enabled one in turn, until one takes it; the one that does has its
pre-parse step.
*/
D_STATIC int
d_arg_internal_parse_group_arg(
    struct d_arg_command* _command,
    int                   _class,
    bool                  _local,
    bool*                 _taken
)
{
    *_taken = false;

    // each enabled option group
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const group = _command->subcommands.items[index];

        if (d_arg_internal_parse_open_group(group))
        {
            const int status = d_arg_internal_parse_arg(group,
                                                        _class,
                                                        _local,
                                                        _taken);

            // taken, or the parse stopped
            if ( (status != D_ARG_OK) ||
                 (*_taken) )
            {
                return (status == D_ARG_OK)
                       ? d_arg_internal_parse_entered(group)
                       : status;
            }
        }
    }

    return D_ARG_OK;
}

/*
d_arg_internal_parse_non_standard
  CLI11's second look at a short argument longer than its name, where
non-standard option names are allowed: read as `-name=value`, an option
with `name` among its short names takes it, the split made over to match.
*/
D_STATIC struct d_arg_option*
d_arg_internal_parse_non_standard(
    const struct d_arg_command*        _command,
    struct d_arg_internal_parse_split* _split
)
{
    const struct d_pack_text argument = _split->argument;
    struct d_pack_text       name     = _split->name;
    struct d_pack_text       value    = _split->value;

    // only a short argument longer than its name, where allowed
    if ( (_split->classifier != (int)D_ARG_CLASS_SHORT)            ||
         (argument.length <= 2u)                                   ||
         (!d_arg_command_has_flag(_command,
                                  D_ARG_COMMAND_NON_STANDARD_NAMES)) )
    {
        return NULL;
    }

    d_arg_internal_parse_at(d_arg_internal_span(argument.data + 1,
                                                argument.length - 1u),
                            '=',
                            &name,
                            &value);

    // the first option with the name among its short names
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        if (d_arg_option_check_short_name(_command->options.items[index],
                                          name))
        {
            _split->name        = name;
            _split->value       = value;
            _split->rest.length = 0u;

            return _command->options.items[index];
        }
    }

    return NULL;
}

/*
d_arg_internal_parse_dotted_sub
  The subcommand CLI11's dot notation names: in `sub.name`, cut at the first
dot past the first character and before the last, `sub` among the
command's subcommands, used ones included -- unless the command has met its
maximum of subcommands and `sub` is not among them. `_name` receives what
follows the dot.
*/
D_STATIC struct d_arg_command*
d_arg_internal_parse_dotted_sub(
    const struct d_arg_command* _command,
    struct d_pack_text          _whole,
    struct d_pack_text*         _name
)
{
    const char* const dot = (_whole.length > 1u)
                            ? (const char*)memchr(_whole.data + 1,
                                                  '.',
                                                  _whole.length - 1u)
                            : NULL;

    // a dot, with something after it
    if ( (!dot)                                              ||
         ((size_t)(dot - _whole.data) >= _whole.length - 1u) )
    {
        return NULL;
    }

    const size_t                at  = (size_t)(dot - _whole.data);
    struct d_arg_command* const sub = d_arg_internal_find_subcommand(
                                          _command,
                                          d_arg_internal_span(_whole.data,
                                                              at),
                                          true,
                                          false);

    *_name = d_arg_internal_span(dot + 1,
                                 _whole.length - at - 1u);

    // no new subcommand past the maximum
    if ( (sub != NULL)                                         &&
         (!d_arg_internal_parse_below_max(_command))           &&
         (!d_arg_internal_command_has(&_command->parsed_subcommands,
                                      sub)) )
    {
        return NULL;
    }

    return sub;
}

/*
d_arg_internal_parse_dotted_push
  The option of CLI11's dot notation rebuilt from the split for the
subcommand, so that `/` works as `--` does: `--name=value`, or, for a
one-character name, `-n` above its value as an argument of its own, as
`=` cannot follow a short name. `_class` receives the rebuilt one's class.
*/
D_STATIC int
d_arg_internal_parse_dotted_push(
    const struct d_arg_command* _command,
    struct d_pack_text          _name,
    struct d_pack_text          _value,
    int*                        _class
)
{
    const struct d_pack_text none   = d_arg_internal_span(_name.data,
                                                          0u);
    int                      status = D_ARG_OK;

    // a long name, with its value
    if (_name.length > 1u)
    {
        *_class = (int)D_ARG_CLASS_LONG;

        return d_arg_internal_parse_push_built(_command,
                                               "--",
                                               _name,
                                               _value);
    }

    *_class = (int)D_ARG_CLASS_SHORT;

    // the value below the short name
    if (_value.length > 0u)
    {
        status = d_arg_internal_parse_stored(
                     _command,
                     d_arg_internal_pending_push(_command->parser,
                                                 _value));
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_push_built(_command,
                                             "-",
                                             _name,
                                             none)
           : status;
}

/*
d_arg_internal_parse_dotted_taken
  What follows CLI11's dot notation once the subcommand took the option:
the subcommand listed as parsed unless silent -- again, should it be
listed already; the command, not the subcommand, counted once more with
its pre-parse step, as CLI11 has it; then the subcommand's parse-complete
callback.
*/
D_STATIC int
d_arg_internal_parse_dotted_taken(
    struct d_arg_command* _command,
    struct d_arg_command* _sub
)
{
    int status = d_arg_internal_parse_list(_command,
                                           _sub);

    // the command's count and pre-parse step, as CLI11 takes them
    if (status == D_ARG_OK)
    {
        d_arg_internal_increment_parsed(_command);
        status = d_arg_internal_trigger_pre_parse(
                     _command,
                     d_arg_internal_parse_left(_command));
    }

    return ( (status == D_ARG_OK) &&
             (_sub->parse_complete_fn != NULL) )
           ? d_arg_internal_process_completion_callbacks(_sub,
                                                         true)
           : status;
}

/*
d_arg_internal_parse_dotted_arg
  CLI11's dot notation for an option, `--sub.name[=value]`: the option,
rebuilt, offered to `sub` alone, as a local search; not taken, the argument
is put back as it was. A name no option form could take is never offered,
as CLI11's check before its offer refuses it.
*/
D_STATIC int
d_arg_internal_parse_dotted_arg(
    struct d_arg_command*                    _command,
    const struct d_arg_internal_parse_split* _split,
    bool*                                    _taken
)
{
    struct d_pack_text          name   = _split->name;
    struct d_arg_command* const sub    = d_arg_internal_parse_dotted_sub(
                                             _command,
                                             _split->name,
                                             &name);
    const size_t                before = d_arg_internal_parse_left(_command);
    int                         type   = (int)D_ARG_CLASS_NONE;

    *_taken = false;

    // no subcommand, or no option form for the name
    if ( (!sub)                                            ||
         (!d_arg_internal_valid_first_char(name.data[0])) )
    {
        return D_ARG_OK;
    }

    d_arg_internal_parse_pop(_command);

    int status = d_arg_internal_parse_dotted_push(_command,
                                                  name,
                                                  _split->value,
                                                  &type);

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_arg(sub,
                                        type,
                                        true,
                                        _taken)
             : status;

    // not taken: the argument back as it was
    if ( (status == D_ARG_OK) &&
         (!(*_taken)) )
    {
        _command->parser->pending.count              = before;
        _command->parser->pending.items[before - 1u] = _split->argument;
    }

    return ( (status == D_ARG_OK) &&
             (*_taken) )
           ? d_arg_internal_parse_dotted_taken(_command,
                                               sub)
           : status;
}

/*
d_arg_internal_parse_allowance
  How many of the positionals following an unused option CLI11 keeps with
it: the next one under assume-single, every one under assume-multiple,
none otherwise.
*/
D_STATIC size_t
d_arg_internal_parse_allowance(
    const struct d_arg_command* _command
)
{
    // the next one
    if (_command->allow_extras == (int)D_ARG_EXTRAS_ASSUME_SINGLE)
    {
        return 1u;
    }

    return (_command->allow_extras == (int)D_ARG_EXTRAS_ASSUME_MULTIPLE)
           ? d_arg_internal_parse_left(_command)
           : 0u;
}

/*
d_arg_internal_parse_followers
  What CLI11 keeps after an unused option: everything that follows, for a
prefix command that stops at anything unrecognized; else as many of the
positionals that follow as the extras mode keeps, each an unused argument.
*/
D_STATIC int
d_arg_internal_parse_followers(
    struct d_arg_command* _command
)
{
    // a prefix command keeps the rest
    if (_command->prefix_command == (int)D_ARG_PREFIX_ON)
    {
        return d_arg_internal_parse_rest_to_missing(_command);
    }

    size_t allowance = d_arg_internal_parse_allowance(_command);
    int    status    = D_ARG_OK;

    // each positional that follows, while the mode keeps them
    while ( (status == D_ARG_OK)                                     &&
            (allowance > 0u)                                         &&
            (d_arg_internal_parse_left(_command) > 0u)               &&
            (d_arg_internal_recognize(
                 _command,
                 d_arg_internal_parse_back(_command),
                 false) == (int)D_ARG_CLASS_NONE) )
    {
        status = d_arg_internal_parse_to_missing(_command,
                                                 (int)D_ARG_CLASS_NONE,
                                                 d_arg_internal_parse_back(
                                                     _command));
        d_arg_internal_parse_pop(_command);
        allowance--;
    }

    return status;
}

/*
d_arg_internal_parse_unclaimed
  The end of CLI11's search for an option: a local search gives up; a
command that falls through hands the argument to its fallthrough parent's
search; anywhere else it is unused, with what follows it as the command
keeps that.
*/
D_STATIC int
d_arg_internal_parse_unclaimed(
    struct d_arg_command*                    _command,
    const struct d_arg_internal_parse_split* _split,
    bool                                     _local,
    bool*                                    _go_on
)
{
    // a local search, which CLI11's dot notation makes, gives up
    if (_local)
    {
        *_go_on = false;

        return D_ARG_OK;
    }

    // the fallthrough parent's search
    if ( (_command->parent != NULL)                    &&
         (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_FALLTHROUGH)) )
    {
        return d_arg_internal_parse_arg(
                   d_arg_internal_fallthrough_parent(_command),
                   _split->classifier,
                   false,
                   _go_on);
    }

    d_arg_internal_parse_pop(_command);

    const int status = d_arg_internal_parse_to_missing(_command,
                                                       _split->classifier,
                                                       _split->argument);

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_followers(_command)
           : status;
}

/*
d_arg_internal_parse_unknown
  CLI11's search for an option the command does not have, in its order:
the command's option groups; a non-standard short name, which finds an
option for the caller to go on with; an option group stops there, leaving
the argument to its parent; then CLI11's dot notation; then the parents,
or the unused arguments.
*/
D_STATIC int
d_arg_internal_parse_unknown(
    struct d_arg_command*              _command,
    struct d_arg_internal_parse_split* _split,
    bool                               _local,
    struct d_arg_option**              _option,
    bool*                              _go_on
)
{
    bool taken  = false;
    int  status = d_arg_internal_parse_group_arg(_command,
                                                 _split->classifier,
                                                 _local,
                                                 &taken);

    // an option group's
    if ( (status != D_ARG_OK) ||
         (taken) )
    {
        return status;
    }

    *_option = d_arg_internal_parse_non_standard(_command,
                                                 _split);

    // found after all
    if (*_option != NULL)
    {
        return D_ARG_OK;
    }

    // an option group leaves it to its parent
    if ( (_command->parent != NULL)                &&
         (d_arg_command_name(_command)[0] == '\0') )
    {
        *_go_on = false;

        return D_ARG_OK;
    }

    status = d_arg_internal_parse_dotted_arg(_command,
                                             _split,
                                             &taken);

    return ( (status != D_ARG_OK) ||
             (taken) )
           ? status
           : d_arg_internal_parse_unclaimed(_command,
                                            _split,
                                            _local,
                                            _go_on);
}

//==============================================================================
// 3.  TAKING VALUES
//==============================================================================

// d_arg_internal_parse_take
//   struct: CLI11's working state in _parse_arg once an option is found:
// the command whose search found it, the option, the split argument, the
// fewest and most values the option takes from here, and how many it has
// taken.
struct d_arg_internal_parse_take
{
    struct d_arg_command*              command;
    struct d_arg_option*               option;
    struct d_arg_internal_parse_split* split;
    int                                min;
    int                                max;
    int                                collected;
};

/*
d_arg_internal_parse_min
  CLI11's min_num: the fewest values an option takes from the arguments --
its type's size, or fewer when it expects fewer items.
*/
D_STATIC int
d_arg_internal_parse_min(
    const struct d_arg_option* _option
)
{
    const int items = d_arg_option_items_expected_min(_option);

    return (_option->type_size_min < items)
           ? _option->type_size_min
           : items;
}

/*
d_arg_internal_parse_max
  CLI11's max_num: the most values an option takes; for one that could take
very many without taking extra arguments, its type's largest size for each
value it expects at least -- the unlimited count should that overflow.
*/
D_STATIC int
d_arg_internal_parse_max(
    const struct d_arg_option* _option
)
{
    const int maximum = d_arg_option_items_expected_max(_option);
    int       product = _option->type_size_max;

    // a count within reason, or extra arguments taken
    if ( (maximum < (int)D_ARG_EXPECTED_UNLIMITED / 16)        ||
         (d_arg_option_has_flag(_option,
                                D_ARG_OPTION_ALLOW_EXTRA_ARGS)) )
    {
        return maximum;
    }

    return d_arg_internal_checked_multiply(&product,
                                           _option->expected_min)
           ? product
           : (int)D_ARG_EXPECTED_UNLIMITED;
}

/*
d_arg_internal_parse_order
  The option in the command's parse order once more, as CLI11 lists it for
each value it takes.
*/
D_STATIC int
d_arg_internal_parse_order(
    const struct d_arg_internal_parse_take* _take
)
{
    return d_arg_internal_parse_stored(
               _take->command,
               d_arg_internal_option_push(&_take->command->parse_order,
                                          _take->option));
}

/*
d_arg_internal_parse_collect
  A value the option takes from the arguments, as CLI11's add_result(value,
count) adds it -- split, perhaps -- the values it made counted, and the
option in the parse order.
*/
D_STATIC int
d_arg_internal_parse_collect(
    struct d_arg_internal_parse_take* _take,
    struct d_pack_text                _value
)
{
    int       added  = 0;
    const int status = d_arg_internal_parse_add(_take->option,
                                                _value,
                                                &added);

    _take->collected += added;

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_order(_take)
           : status;
}

/*
d_arg_internal_parse_flag
  CLI11's get_flag_value for the name given with an input, added as the
option's value as its add_result(value) adds it -- the value's storage
becoming the option's own -- and the option in the parse order.
*/
D_STATIC int
d_arg_internal_parse_flag(
    struct d_arg_internal_parse_take* _take,
    struct d_pack_text                _input
)
{
    struct d_arg_option* const option = _take->option;
    struct d_arg_buffer        value  = D_ARG_BUFFER_INIT;
    int                        status = d_arg_option_flag_value(
                                            option,
                                            _take->split->name,
                                            _input,
                                            &value);

    // the value, or the buffer's refusal, which flag_value leaves unrecorded
    if (status == D_ARG_OK)
    {
        status        = d_arg_internal_option_add_built(option,
                                                        &option->results,
                                                        &value,
                                                        NULL);
        option->state = (int)D_ARG_OPTION_STATE_PARSING;
    }
    else if (status == D_ARG_ERROR_OUT_OF_MEMORY)
    {
        status = d_arg_internal_fail(_take->command,
                                     option,
                                     status,
                                     NULL);
    }

    d_arg_buffer_release(&value);

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_order(_take)
           : status;
}

/*
d_arg_internal_parse_first
  CLI11's first value of an option argument: a flag's value for an option
that takes none; else the value after `=`; else what follows a short name,
which is then used up.
*/
D_STATIC int
d_arg_internal_parse_first(
    struct d_arg_internal_parse_take* _take
)
{
    struct d_arg_internal_parse_split* const split = _take->split;

    // a flag
    if (_take->max == 0)
    {
        return d_arg_internal_parse_flag(_take,
                                         split->value);
    }

    // `--name=value`
    if (split->value.length > 0u)
    {
        return d_arg_internal_parse_collect(_take,
                                            split->value);
    }

    const struct d_pack_text rest = split->rest;

    split->rest.length = 0u;

    // `-xrest`, or nothing yet
    return (rest.length > 0u)
           ? d_arg_internal_parse_collect(_take,
                                          rest)
           : D_ARG_OK;
}

/*
d_arg_internal_parse_mismatch
  CLI11's ArgumentMismatch for an option's values: TypedAtLeast, "<name>:
<count> required <type> missing", or PartialType, "<name>: <type> only
partially specified: <count> required for each element".
*/
D_STATIC int
d_arg_internal_parse_mismatch(
    const struct d_arg_internal_parse_take* _take,
    int                                     _count,
    bool                                    _partial
)
{
    struct d_arg_buffer name   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer type   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_option_name(_take->option,
                                                   0u,
                                                   &name);

    status = (status == D_ARG_OK)
             ? d_arg_option_type_name(_take->option,
                                      &type)
             : status;

    // the message, once both are written
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_fail(_take->command,
                                     _take->option,
                                     status,
                                     NULL);
    }
    else if (_partial)
    {
        status = d_arg_internal_fail(_take->command,
                                     _take->option,
                                     D_ARG_ERROR_ARGUMENT_MISMATCH,
                                     "%s: %s only partially specified: %d "
                                     "required for each element",
                                     d_arg_buffer_text(&name).data,
                                     d_arg_buffer_text(&type).data,
                                     _count);
    }
    else
    {
        status = d_arg_internal_fail(_take->command,
                                     _take->option,
                                     D_ARG_ERROR_ARGUMENT_MISMATCH,
                                     "%s: %d required %s missing",
                                     d_arg_buffer_text(&name).data,
                                     _count,
                                     d_arg_buffer_text(&type).data);
    }

    d_arg_buffer_release(&name);
    d_arg_buffer_release(&type);

    return status;
}

/*
d_arg_internal_parse_minimum
  CLI11's gathering of the fewest values an option takes, whatever the
arguments look like; too few left is its TypedAtLeast mismatch.
*/
D_STATIC int
d_arg_internal_parse_minimum(
    struct d_arg_internal_parse_take* _take
)
{
    int status = D_ARG_OK;

    // the next argument, while more are needed and any are left
    while ( (status == D_ARG_OK)                             &&
            (_take->min > _take->collected)                  &&
            (d_arg_internal_parse_left(_take->command) > 0u) )
    {
        const struct d_pack_text value = d_arg_internal_parse_back(
                                             _take->command);

        d_arg_internal_parse_pop(_take->command);
        status = d_arg_internal_parse_collect(_take,
                                              value);
    }

    return ( (status == D_ARG_OK) &&
             (_take->min > _take->collected) )
           ? d_arg_internal_parse_mismatch(_take,
                                           _take->min,
                                           false)
           : status;
}

/*
d_arg_internal_parse_wants
  Whether CLI11's optional gathering takes the next argument: room left or
extra arguments taken, an argument that is a positional, more arguments
left than the required positionals still need, and -- where the command
validates optional arguments -- validators that accept it.
*/
D_STATIC int
d_arg_internal_parse_wants(
    const struct d_arg_internal_parse_take* _take,
    size_t                                  _waiting,
    bool*                                   _wants
)
{
    const struct d_arg_command* const command = _take->command;
    const size_t                      left    = d_arg_internal_parse_left(
                                                    command);

    *_wants = false;

    // room, an argument, a positional, and enough for the required ones
    if ( ( (_take->collected >= _take->max) &&
           (!d_arg_option_has_flag(_take->option,
                                   D_ARG_OPTION_ALLOW_EXTRA_ARGS)) )   ||
         (left == 0u)                                                  ||
         (d_arg_internal_recognize(command,
                                   d_arg_internal_parse_back(command),
                                   false) != (int)D_ARG_CLASS_NONE)    ||
         (_waiting >= left) )
    {
        return D_ARG_OK;
    }

    // the validators, where optional arguments are validated
    if (d_arg_command_has_flag(command,
                               D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS))
    {
        return d_arg_internal_parse_judge(command,
                                          _take->option,
                                          d_arg_internal_parse_back(command),
                                          _wants);
    }

    *_wants = true;

    return D_ARG_OK;
}

/*
d_arg_internal_parse_optional
  CLI11's gathering of the values an option takes past its fewest, while it
wants them; then a `--` that ends them is eaten, and an option that takes a
value only when given one, given none, takes its flag value.
*/
D_STATIC int
d_arg_internal_parse_optional(
    struct d_arg_internal_parse_take* _take
)
{
    // nothing more to take
    if ( (_take->max <= _take->collected)                         &&
         (!d_arg_option_has_flag(_take->option,
                                 D_ARG_OPTION_ALLOW_EXTRA_ARGS)) )
    {
        return D_ARG_OK;
    }

    const size_t waiting = d_arg_internal_parse_remaining(_take->command,
                                                          true);
    bool         wants   = true;
    int          status  = D_ARG_OK;

    // each argument it wants
    while ( (status == D_ARG_OK) &&
            (wants) )
    {
        status = d_arg_internal_parse_wants(_take,
                                            waiting,
                                            &wants);

        if ( (status == D_ARG_OK) &&
             (wants) )
        {
            status = d_arg_internal_parse_collect(
                         _take,
                         d_arg_internal_parse_back(_take->command));
            d_arg_internal_parse_pop(_take->command);
        }
    }

    // `--` ends the values, and is eaten
    if ( (status == D_ARG_OK)                                     &&
         (d_arg_internal_parse_left(_take->command) > 0u)         &&
         (d_arg_internal_equal_cstr(
              d_arg_internal_parse_back(_take->command),
              "--")) )
    {
        d_arg_internal_parse_pop(_take->command);
    }

    return ( (status == D_ARG_OK) &&
             (_take->min == 0)    &&
             (_take->max > 0)     &&
             (_take->collected == 0) )
           ? d_arg_internal_parse_flag(_take,
                                       d_arg_internal_span("",
                                                           0u))
           : status;
}

/*
d_arg_internal_parse_partial
  CLI11's check that each item was given whole: a count of values that is
no multiple of the type's largest size is marked with an empty value where
the size may vary, and refused -- its PartialType mismatch -- where it may
not.
*/
D_STATIC int
d_arg_internal_parse_partial(
    struct d_arg_internal_parse_take* _take
)
{
    const struct d_arg_option* const option = _take->option;

    // nothing required, or every item whole
    if ( (_take->min <= 0)                                     ||
         (option->type_size_max <= 0)                          ||
         ((_take->collected % option->type_size_max) == 0) )
    {
        return D_ARG_OK;
    }

    return (option->type_size_max != option->type_size_min)
           ? d_arg_internal_parse_add(_take->option,
                                      d_arg_internal_span("",
                                                          0u),
                                      NULL)
           : d_arg_internal_parse_mismatch(_take,
                                           option->type_size_min,
                                           true);
}

/*
d_arg_internal_parse_gather
  CLI11's gathering of an option's values: the first from the argument
itself, the fewest it needs whatever follows, as many more as it takes,
then an item given in part marked or refused.
*/
D_STATIC int
d_arg_internal_parse_gather(
    struct d_arg_internal_parse_take* _take
)
{
    int status = d_arg_internal_parse_first(_take);

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_minimum(_take)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_optional(_take)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_parse_partial(_take)
           : status;
}

/*
d_arg_internal_parse_values
  The rest of CLI11's _parse_arg once an option takes the argument: a
separator before its values where asked, a trigger whose callback ran
started afresh, the values gathered, the trigger's callback, and what
followed a short name back on the stack as an argument of its own.
*/
D_STATIC int
d_arg_internal_parse_values(
    struct d_arg_command*              _command,
    struct d_arg_option*               _option,
    struct d_arg_internal_parse_split* _split
)
{
    const bool trigger = d_arg_option_has_flag(_option,
                                               D_ARG_OPTION_TRIGGER_ON_PARSE);
    int        status  = d_arg_internal_parse_separate(_option);

    // a trigger met again after its callback ran
    if ( (trigger) &&
         (_option->state == (int)D_ARG_OPTION_STATE_CALLBACK_RUN) )
    {
        d_arg_internal_parse_restart(_option,
                                     NULL);
    }

    struct d_arg_internal_parse_take take = { _command,
                                              _option,
                                              _split,
                                              d_arg_internal_parse_min(_option),
                                              d_arg_internal_parse_max(_option),
                                              0 };

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_gather(&take)
             : status;
    status = ( (status == D_ARG_OK) &&
               (trigger) )
             ? d_arg_option_run_callback(_option)
             : status;

    return ( (status == D_ARG_OK) &&
             (_split->rest.length > 0u) )
           ? d_arg_internal_parse_push_built(_command,
                                             "-",
                                             _split->rest,
                                             d_arg_internal_span(
                                                 _split->rest.data,
                                                 0u))
           : status;
}

//==============================================================================
// 4.  THE OPTION STEP
//==============================================================================

/*
d_arg_internal_parse_arg
  CLI11's _parse_arg: the argument split as recognized; an option of the
command's that answers to the name -- or one the further search finds --
takes it, with its values; otherwise the search has dealt with it.
*/
int
d_arg_internal_parse_arg(
    struct d_arg_command* _command,
    int                   _class,
    bool                  _local,
    bool*                 _go_on
)
{
    struct d_arg_internal_parse_split split  = SPLIT_NONE;
    int                               status = d_arg_internal_parse_split_arg(
                                                   _command,
                                                   _class,
                                                   &split);

    *_go_on = true;

    // an argument that does not split, which recognizing never lets happen
    if (status != D_ARG_OK)
    {
        return status;
    }

    struct d_arg_option* option = d_arg_internal_parse_own_option(_command,
                                                                  &split);

    // not the command's own: the further search
    if (!option)
    {
        status = d_arg_internal_parse_unknown(_command,
                                              &split,
                                              _local,
                                              &option,
                                              _go_on);
    }

    // found: the option takes it, with its values
    if ( (status != D_ARG_OK) ||
         (!option) )
    {
        return status;
    }

    d_arg_internal_parse_pop(_command);

    return d_arg_internal_parse_values(_command,
                                       option,
                                       &split);
}
