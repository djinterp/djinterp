/*******************************************************************************
* djinterp [parse]                                        arg_formatter_option.c
*
* The default steps for an option's help, declared in arg_formatter.h (4.2).
*   CLI11's Formatter, one C function for each of its make_* functions that
* lays out options: a group of options under its heading; an option's line,
* positional -- its name and what follows padded to the left column -- or
* named, its short names and its long names in two columns whose boundary
* the long-name alignment ratio sets; the names; what follows them -- the
* option text, or the type, default, count, requirement, environment
* variable and links; the description; and an option's form in the usage
* line. Each calls the others through the dispatchers in arg_formatter.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_formatter_option.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr, memcmp
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "./arg_internal.h"  // spans, lists, help text
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  GROUPS AND NAMES
//==============================================================================

/*
d_arg_format_group_default
  CLI11's make_group: a blank line, the heading and a colon, then each
option's line.
*/
int
d_arg_format_group_default(
    const struct d_arg_formatter*   _formatter,
    struct d_pack_text              _group,
    bool                            _positional,
    const struct d_arg_option_list* _options,
    struct d_arg_buffer*            _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_options)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_buffer_append_char(_out,
                                          '\n');

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_text(_out,
                                        _group)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        ":\n")
             : status;

    // each option's line
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _options->count);
         ++index)
    {
        status = d_arg_format_option(_formatter,
                                     _options->items[index],
                                     _positional,
                                     _out);
    }

    return status;
}

/*
d_arg_format_option_name_default
  CLI11's make_option_name: the positional name for a positional's line;
otherwise every name, with the flag values unless they are switched off.
*/
int
d_arg_format_option_name_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    bool                          _positional,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_option)    ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const uint32_t values = ((_formatter->flags &
                              D_ARG_FORMAT_DEFAULT_FLAG_VALUES) != 0u)
                            ? 0u
                            : D_ARG_NAME_NO_FLAG_VALUES;

    return d_arg_option_name(_option,
                             (_positional)
                             ? D_ARG_NAME_POSITIONAL
                             : (D_ARG_NAME_ALL | values),
                             _out);
}

/*
d_arg_format_option_desc_default
  CLI11's make_option_desc: the description.
*/
int
d_arg_format_option_desc_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_option)    ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_buffer_append_cstr(_out,
                                    d_arg_option_description(_option));
}

/*
d_arg_format_option_usage_default
  CLI11's make_option_usage: the positional name, "..." for any number of
values or "(Nx)" for more than one, in brackets unless required.
*/
int
d_arg_format_option_usage_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_option)    ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool required = d_arg_option_has_flag(_option,
                                                D_ARG_OPTION_REQUIRED);
    int        status   = (required)
                          ? D_ARG_OK
                          : d_arg_buffer_append_char(_out,
                                                     '[');

    status = (status == D_ARG_OK)
             ? d_arg_format_option_name(_formatter,
                                        _option,
                                        true,
                                        _out)
             : status;

    // how many values
    if (_option->expected_max >= (int)D_ARG_EXPECTED_UNLIMITED)
    {
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_cstr(_out,
                                            "...")
                 : status;
    }
    else if (_option->expected_max > 1)
    {
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_out,
                                            '(')
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_signed(_out,
                                              _option->expected_min)
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_cstr(_out,
                                            "x)")
                 : status;
    }

    return ( (status == D_ARG_OK) &&
             (!required) )
           ? d_arg_buffer_append_char(_out,
                                      ']')
           : status;
}

//==============================================================================
// 2.  WHAT FOLLOWS THE NAMES
//==============================================================================

/*
d_arg_internal_help_before
  Whether one option's name sorts before another's, as std::string's `<`
compares them: byte by byte, unsigned, a prefix first. The names are
written into the two scratch buffers given.
*/
D_STATIC bool
d_arg_internal_help_before(
    const struct d_arg_option* _first,
    const struct d_arg_option* _second,
    struct d_arg_buffer*       _scratch_first,
    struct d_arg_buffer*       _scratch_second
)
{
    d_arg_buffer_clear(_scratch_first);
    d_arg_buffer_clear(_scratch_second);
    (void)d_arg_option_name(_first,
                            0u,
                            _scratch_first);
    (void)d_arg_option_name(_second,
                            0u,
                            _scratch_second);

    const size_t shorter = (_scratch_first->length < _scratch_second->length)
                           ? _scratch_first->length
                           : _scratch_second->length;
    const int    order   = (shorter > 0u)
                           ? memcmp(_scratch_first->data,
                                    _scratch_second->data,
                                    shorter)
                           : 0;

    return ( (order < 0) ||
             ( (order == 0) &&
               (_scratch_first->length < _scratch_second->length) ) );
}

/*
d_arg_internal_help_sorted
  The options of a list in the order of their names, by insertion: each
slid back past the names that sort after its own, so that options of the
same name keep the order they were linked in, where CLI11's std::sort
leaves it unspecified.
*/
D_STATIC int
d_arg_internal_help_sorted(
    const struct d_arg_option_list* _options,
    struct d_arg_option_list*       _sorted
)
{
    struct d_arg_buffer first  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer second = D_ARG_BUFFER_INIT;
    int                 status = D_ARG_OK;

    // each option slid back into place
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _options->count);
         ++index)
    {
        status = d_arg_internal_option_push(_sorted,
                                            _options->items[index]);

        // back past each name that sorts after its own
        for (size_t at = _sorted->count - 1u;
             (status == D_ARG_OK) &&
             (at > 0u)            &&
             (d_arg_internal_help_before(_sorted->items[at],
                                         _sorted->items[at - 1u],
                                         &first,
                                         &second));
             --at)
        {
            struct d_arg_option* const moved = _sorted->items[at];

            _sorted->items[at]      = _sorted->items[at - 1u];
            _sorted->items[at - 1u] = moved;
        }
    }

    // a name that could not be written spoiled the order
    status = ( (first.failed != 0) ||
               (second.failed != 0) )
             ? D_ARG_ERROR_OUT_OF_MEMORY
             : status;
    d_arg_buffer_release(&second);
    d_arg_buffer_release(&first);

    return status;
}

/*
d_arg_internal_help_linked
  The options another needs or excludes, each after a space by its name, in
the order of their names, as CLI11 sorts its pointer-ordered set for stable
help.
*/
D_STATIC int
d_arg_internal_help_linked(
    const struct d_arg_option_list* _options,
    struct d_arg_buffer*            _out
)
{
    struct d_arg_option_list sorted = D_ARG_LIST_INIT;
    int                      status = d_arg_internal_help_sorted(_options,
                                                                 &sorted);

    // each name after a space
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < sorted.count);
         ++index)
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
        status = (status == D_ARG_OK)
                 ? d_arg_option_name(sorted.items[index],
                                     0u,
                                     _out)
                 : status;
    }

    d_arg_internal_option_list_release(&sorted);

    return status;
}

/*
d_arg_internal_help_typed
  The type name, labelled, with the delimiter and "..." after it when values
may be joined by one; nothing when there is no type name.
*/
D_STATIC int
d_arg_internal_help_typed(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer type   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_option_type_name(_option,
                                                        &type);

    // a type name, labelled
    if ( (status == D_ARG_OK) &&
         (type.length > 0u) )
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_text(
                       _out,
                       d_arg_formatter_label(_formatter,
                                             d_arg_buffer_text(&type)))
                 : status;

        // the delimiter the values may be joined by
        if ( (status == D_ARG_OK) &&
             (_option->delimiter != '\0') )
        {
            status = d_arg_buffer_append_char(_out,
                                              _option->delimiter);
            status = (status == D_ARG_OK)
                     ? d_arg_buffer_append_cstr(_out,
                                                "...")
                     : status;
        }
    }

    d_arg_buffer_release(&type);

    return status;
}

/*
d_arg_internal_help_shown_default
  An option's default in brackets, with the space after them CLI11 leaves;
nothing when it has none.
*/
D_STATIC int
d_arg_internal_help_shown_default(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    const char* text = d_arg_option_default_text(_option);

    // no default, nothing shown
    if (text[0] == '\0')
    {
        return D_ARG_OK;
    }

    int status = d_arg_buffer_append_cstr(_out,
                                          " [");

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        text)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_out,
                                      "] ")
           : status;
}

/*
d_arg_internal_help_shown_count
  How many values an option takes, as make_option_opts shows it: " ..." for
any number, " x N" for a fixed count above one.
*/
D_STATIC int
d_arg_internal_help_shown_count(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    int status = D_ARG_OK;

    // any number of values
    if (_option->expected_max == (int)D_ARG_EXPECTED_UNLIMITED)
    {
        return d_arg_buffer_append_cstr(_out,
                                        " ...");
    }

    // more than one
    if (_option->expected_min > 1)
    {
        status = d_arg_buffer_append_cstr(_out,
                                          " x ");
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_signed(_out,
                                              _option->expected_min)
                 : status;
    }

    return status;
}

/*
d_arg_internal_help_valued
  What an option that takes values shows: its type, its default, its count,
and the REQUIRED label.
*/
D_STATIC int
d_arg_internal_help_valued(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    int status = ((_formatter->flags & D_ARG_FORMAT_OPTION_TYPE_NAMES) != 0u)
                 ? d_arg_internal_help_typed(_formatter,
                                             _option,
                                             _out)
                 : D_ARG_OK;

    status = ( (status == D_ARG_OK) &&
               ((_formatter->flags & D_ARG_FORMAT_OPTION_DEFAULTS) != 0u) )
             ? d_arg_internal_help_shown_default(_option,
                                                 _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_shown_count(_option,
                                               _out)
             : status;

    // a required option says so
    if ( (status == D_ARG_OK) &&
         (d_arg_option_has_flag(_option,
                                D_ARG_OPTION_REQUIRED)) )
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_label(_formatter,
                                             "REQUIRED",
                                             _out)
                 : status;
    }

    return status;
}

/*
d_arg_internal_help_links
  One kind of link: a space, the label and a colon, then the names.
*/
D_STATIC int
d_arg_internal_help_links(
    const struct d_arg_formatter*   _formatter,
    const char*                     _key,
    const struct d_arg_option_list* _options,
    struct d_arg_buffer*            _out
)
{
    int status = d_arg_buffer_append_char(_out,
                                          ' ');

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_label(_formatter,
                                         _key,
                                         _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(_out,
                                        ':')
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_help_linked(_options,
                                        _out)
           : status;
}

/*
d_arg_internal_help_environment
  The environment variable an option reads, in parentheses after the Env
label; nothing when it reads none.
*/
D_STATIC int
d_arg_internal_help_environment(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    const char* name = d_arg_option_env_name(_option);

    // no variable, nothing shown
    if (name[0] == '\0')
    {
        return D_ARG_OK;
    }

    int status = d_arg_buffer_append_cstr(_out,
                                          " (");

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_label(_formatter,
                                         "Env",
                                         _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_printf(_out,
                                 ":%s)",
                                 name)
           : status;
}

/*
d_arg_format_option_opts_default
  CLI11's make_option_opts: the option text alone, when there is one;
otherwise, for an option whose type takes values, what d_arg_internal_help_
valued writes, then for any option its environment variable and the
options it needs and excludes.
*/
int
d_arg_format_option_opts_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_option)    ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const char* text = d_arg_option_option_text(_option);

    // the option's own text stands for the rest
    if (text[0] != '\0')
    {
        return d_arg_buffer_printf(_out,
                                   " %s",
                                   text);
    }

    int status = (_option->type_size_min != 0)
                 ? d_arg_internal_help_valued(_formatter,
                                              _option,
                                              _out)
                 : D_ARG_OK;

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_environment(_formatter,
                                               _option,
                                               _out)
             : status;
    status = ( (status == D_ARG_OK) &&
               (_option->needs.count > 0u) )
             ? d_arg_internal_help_links(_formatter,
                                         "Needs",
                                         &_option->needs,
                                         _out)
             : status;

    return ( (status == D_ARG_OK) &&
             (_option->excludes.count > 0u) )
           ? d_arg_internal_help_links(_formatter,
                                       "Excludes",
                                       &_option->excludes,
                                       _out)
           : status;
}

//==============================================================================
// 3.  AN OPTION'S LINE
//==============================================================================

// d_arg_internal_help_names
//   struct: a named option's names, split as make_option splits them: the
// short ones and the long ones, each joined by ", ", with how many each
// holds, since an empty name still counts in the join.
struct d_arg_internal_help_names
{
    struct d_arg_buffer shorts;
    struct d_arg_buffer longs;
    size_t              short_count;
    size_t              long_count;
};

/*
d_arg_internal_help_sort_name
  One name into the short or the long list: long when it holds `--`
anywhere, as CLI11's find("--") decides.
*/
D_STATIC int
d_arg_internal_help_sort_name(
    struct d_arg_internal_help_names* _names,
    struct d_pack_text                _name
)
{
    bool long_name = false;

    // a `--` anywhere makes it long
    for (size_t at = 0u; (at + 1u) < _name.length; ++at)
    {
        long_name = ( (long_name) ||
                      ( (_name.data[at] == '-') &&
                        (_name.data[at + 1u] == '-') ) );
    }

    struct d_arg_buffer* const list  = (long_name)
                                       ? &_names->longs
                                       : &_names->shorts;
    size_t* const              count = (long_name)
                                       ? &_names->long_count
                                       : &_names->short_count;
    const int                  joint = (*count > 0u)
                                       ? d_arg_buffer_append_cstr(list,
                                                                  ", ")
                                       : D_ARG_OK;

    *count += 1u;

    return (joint == D_ARG_OK)
           ? d_arg_buffer_append_text(list,
                                      _name)
           : joint;
}

/*
d_arg_internal_help_split_names
  CLI11's split(names, ','): an empty text is one empty name; otherwise
each piece up to a comma, the empty piece after a final comma not counted,
as getline gives them.
*/
D_STATIC int
d_arg_internal_help_split_names(
    struct d_arg_internal_help_names* _names,
    struct d_pack_text                _all
)
{
    size_t at     = 0u;
    int    status = (_all.length == 0u)
                    ? d_arg_internal_help_sort_name(_names,
                                                    _all)
                    : D_ARG_OK;

    // each piece up to a comma
    while ( (status == D_ARG_OK) &&
            (at < _all.length) )
    {
        const char*  comma = (const char*)memchr(_all.data + at,
                                                 ',',
                                                 _all.length - at);
        const size_t end   = (comma != NULL)
                             ? (size_t)(comma - _all.data)
                             : _all.length;

        status = d_arg_internal_help_sort_name(_names,
                                               d_arg_internal_span(
                                                   _all.data + at,
                                                   end - at));
        at     = (comma != NULL) ? end + 1u : _all.length;
    }

    return status;
}

/*
d_arg_internal_help_scaled
  CLI11's static_cast<int>(static_cast<float>(column_width_) * ratio): the
product rounded to a float -- the cast forces it where the platform would
keep more precision, so that 30 * 0.7f is 21 everywhere -- and truncated,
held at INT_MAX where the int could not hold it.
*/
D_STATIC int
d_arg_internal_help_scaled(
    size_t _width,
    float  _ratio
)
{
    const float scaled = (float)((float)_width * _ratio);

    return (scaled >= (float)INT_MAX)
           ? INT_MAX
           : (scaled > 0.0f)
             ? (int)scaled
             : 0;
}

/*
d_arg_internal_help_wide
  Whether a field written so far reaches a column width, as CLI11 compares
them: a width below 1 is reached by anything.
*/
D_STATIC bool
d_arg_internal_help_wide(
    size_t _written,
    int    _width
)
{
    return ( (_width <= 0) ||
             (_written >= (size_t)_width) );
}

/*
d_arg_internal_help_short_column
  The short names' column: indented two, with the option's text when there
are no long names, or a comma when there are, and a space when it reaches
the column's width, padded to that width. Returns through `_over` how far
it ran past.
*/
D_STATIC int
d_arg_internal_help_short_column(
    const struct d_arg_internal_help_names* _names,
    struct d_pack_text                      _opts,
    int                                     _width,
    int*                                    _over,
    struct d_arg_buffer*                    _out
)
{
    const size_t start  = _out->length;
    int          status = D_ARG_OK;

    // short names, and what goes with them
    if (_names->shorts.length > 0u)
    {
        status = d_arg_buffer_append_cstr(_out,
                                          "  ");
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_text(_out,
                                            d_arg_buffer_text(&_names->shorts))
                 : status;
        status = ( (status == D_ARG_OK)         &&
                   (_names->longs.length == 0u) )
                 ? d_arg_buffer_append_text(_out,
                                            _opts)
                 : status;
        status = ( (status == D_ARG_OK)        &&
                   (_names->longs.length > 0u) )
                 ? d_arg_buffer_append_char(_out,
                                            ',')
                 : status;

        // reaching the long names' column, a space and the overrun
        if ( (status == D_ARG_OK) &&
             (d_arg_internal_help_wide(_out->length - start,
                                       _width)) )
        {
            status = d_arg_buffer_append_char(_out,
                                              ' ');
            *_over = d_arg_internal_help_int(_out->length - start) - _width;
        }
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_help_pad(_out,
                                     start,
                                     _width)
           : status;
}

/*
d_arg_internal_help_long_column
  The long names' column: the long names with the option's text, a space
when they reach the column's width, padded to that width.
*/
D_STATIC int
d_arg_internal_help_long_column(
    const struct d_arg_internal_help_names* _names,
    struct d_pack_text                      _opts,
    int                                     _width,
    struct d_arg_buffer*                    _out
)
{
    const size_t start  = _out->length;
    int          status = D_ARG_OK;

    // long names, and the option's text
    if (_names->longs.length > 0u)
    {
        status = d_arg_buffer_append_text(_out,
                                          d_arg_buffer_text(&_names->longs));
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_text(_out,
                                            _opts)
                 : status;

        // reaching the right column, a space
        if ( (status == D_ARG_OK) &&
             (d_arg_internal_help_wide(_out->length - start,
                                       _width)) )
        {
            status = d_arg_buffer_append_char(_out,
                                              ' ');
        }
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_help_pad(_out,
                                     start,
                                     _width)
           : status;
}

/*
d_arg_internal_help_named_line
  The non-positional half of CLI11's make_option: the names split into short
and long, the short ones in a column the alignment ratio sizes, the long
ones in the rest of the left column less however far the short ones ran
past theirs, then the description -- on the next line when the two columns
ran past the left one.
*/
D_STATIC int
d_arg_internal_help_named_line(
    const struct d_arg_formatter*     _formatter,
    const struct d_arg_option*        _option,
    struct d_arg_buffer*              _parts,
    struct d_arg_internal_help_names* _names,
    struct d_arg_buffer*              _out
)
{
    const float  ratio  = d_arg_internal_help_ratio(
                              _formatter->long_option_alignment_ratio);
    const int    shorts = d_arg_internal_help_scaled(_formatter->column_width,
                                                     ratio);
    const int    longs  = d_arg_internal_help_int(_formatter->column_width) -
                          shorts;
    const size_t start  = _out->length;
    int          over   = 0;
    int          status = d_arg_format_option_name(_formatter,
                                                   _option,
                                                   false,
                                                   &_parts[0]);

    status = (status == D_ARG_OK)
             ? d_arg_format_option_opts(_formatter,
                                        _option,
                                        &_parts[1])
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_format_option_desc(_formatter,
                                        _option,
                                        &_parts[2])
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_split_names(_names,
                                               d_arg_buffer_text(&_parts[0]))
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_short_column(_names,
                                                d_arg_buffer_text(&_parts[1]),
                                                shorts,
                                                &over,
                                                _out)
             : status;
    over   = (over < longs) ? over : longs;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_long_column(_names,
                                               d_arg_buffer_text(&_parts[1]),
                                               longs - over,
                                               _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_help_column(_formatter,
                                        d_arg_buffer_text(&_parts[2]),
                                        ((_out->length - start) >
                                         _formatter->column_width),
                                        _out)
           : status;
}

/*
d_arg_internal_help_positional_line
  The positional half of CLI11's make_option: two spaces, the positional
name and what follows it, padded to the left column, then the description
-- on the next line when the left part fills the column.
*/
D_STATIC int
d_arg_internal_help_positional_line(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _description,
    struct d_arg_buffer*          _out
)
{
    const size_t start  = _out->length;
    int          status = d_arg_buffer_append_cstr(_out,
                                                   "  ");

    status = (status == D_ARG_OK)
             ? d_arg_format_option_name(_formatter,
                                        _option,
                                        true,
                                        _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_format_option_opts(_formatter,
                                        _option,
                                        _out)
             : status;

    const size_t left = _out->length - start;

    status = (status == D_ARG_OK)
             ? d_arg_format_option_desc(_formatter,
                                        _option,
                                        _description)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_pad(_out,
                                       start,
                                       d_arg_internal_help_int(
                                           _formatter->column_width))
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_help_column(_formatter,
                                        d_arg_buffer_text(_description),
                                        (left >= _formatter->column_width),
                                        _out)
           : status;
}

/*
d_arg_format_option_default
  CLI11's make_option: a positional's line or a named option's, then a line
feed. The scratch buffers the line is built from are this function's, so
the halves need not free them.
*/
int
d_arg_format_option_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    bool                          _positional,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_option)    ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer              parts[3] =
    {
        D_ARG_BUFFER_INIT,  // names
        D_ARG_BUFFER_INIT,  // opts
        D_ARG_BUFFER_INIT   // description
    };
    struct d_arg_internal_help_names names    =
    {
        D_ARG_BUFFER_INIT,  // shorts
        D_ARG_BUFFER_INIT,  // longs
        0u,                 // short_count
        0u                  // long_count
    };
    int                              status   = D_ARG_OK;

    // a positional's line, or a named option's
    if (_positional)
    {
        status = d_arg_internal_help_positional_line(_formatter,
                                                     _option,
                                                     &parts[2],
                                                     _out);
    }
    else
    {
        status = d_arg_internal_help_named_line(_formatter,
                                                _option,
                                                parts,
                                                &names,
                                                _out);
    }

    d_arg_buffer_release(&names.longs);
    d_arg_buffer_release(&names.shorts);
    d_arg_buffer_release(&parts[2]);
    d_arg_buffer_release(&parts[1]);
    d_arg_buffer_release(&parts[0]);

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}
