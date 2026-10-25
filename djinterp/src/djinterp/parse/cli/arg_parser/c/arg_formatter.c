/*******************************************************************************
* djinterp [parse]                                               arg_formatter.c
*
* Help formatters, declared in arg_formatter.h (3, 4.1 and 5.1).
*   A formatter's lifetime, settings and labels -- CLI11's FormatterBase --
* and the formatter a command's help uses; the dispatch of each step to the
* formatter's table, or to the default step when its entry is empty; and help
* itself, CLI11's App::help, built whole into a buffer before a byte of it is
* written. Also the text CLI11's steps share: a paragraph wrapped to a width
* (streamOutAsParagraph), a field padded (std::setw), a label, and the widths
* a line width leaves. The default steps are in arg_formatter_command.c and
* arg_formatter_option.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_formatter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // memchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "./arg_internal.h"  // spans, copies, string lists, stored floats
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  FORMATTERS
//==============================================================================

// FORMATTER_DEFAULT
//   constant: CLI11's Formatter as constructed: no step replaced, no labels,
// the configured widths, no line width, long names a third of the way into
// the left column, CLI11's five switches on.
static const struct d_arg_formatter FORMATTER_DEFAULT =
{
    NULL,                                      // vtable
    NULL,                                      // context
    D_ARG_LIST_INIT,                           // label_keys
    D_ARG_LIST_INIT,                           // label_values
    D_INTERNAL_ARG_PARSER_COLUMN_WIDTH,        // column_width
    D_INTERNAL_ARG_PARSER_RIGHT_COLUMN_WIDTH,  // right_column_width
    D_INTERNAL_ARG_PARSER_PARAGRAPH_WIDTH,     // description_paragraph_width
    D_INTERNAL_ARG_PARSER_PARAGRAPH_WIDTH,     // footer_paragraph_width
    0u,                                        // line_width
    1.0f / 3.0f,                               // long_option_alignment_ratio
    D_ARG_FORMAT_DEFAULTS                      // flags
};

// LABELS_EMPTY
//   constant: a label list that owns nothing.
static const struct d_arg_string_list LABELS_EMPTY = D_ARG_LIST_INIT;

/*
d_arg_formatter_init
  CLI11's default Formatter.
*/
void
d_arg_formatter_init(
    struct d_arg_formatter* _formatter
)
{
    // validate the parameters
    if (!_formatter)
    {
        return;
    }

    *_formatter = FORMATTER_DEFAULT;

    return;
}

/*
d_arg_formatter_release
  The labels freed, and every setting back to CLI11's default.
*/
void
d_arg_formatter_release(
    struct d_arg_formatter* _formatter
)
{
    // validate the parameters
    if (!_formatter)
    {
        return;
    }

    d_arg_string_list_release(&_formatter->label_keys);
    d_arg_string_list_release(&_formatter->label_values);
    *_formatter = FORMATTER_DEFAULT;

    return;
}

/**
 * @brief Copies a formatter's labels into one that has none.
 *
 * @param[in,out] _destination  the formatter that gains the labels.
 * @param[in]     _source       the formatter whose labels are copied.
 * @return D_ARG_OK, or the failure of a pair's copy, the destination then left
 *         with none.
 *
 * The source's labels copied, pair by pair, into a destination that has
 * none; on failure the pairs copied so far are freed again, so the
 * destination is left with none.
 */
D_STATIC int
d_arg_internal_formatter_copy_labels(
    struct d_arg_formatter*       _destination,
    const struct d_arg_formatter* _source
)
{
    int status = D_ARG_OK;

    // each key with its value
    for (size_t index = 0u;
         (status == D_ARG_OK)                &&
         (index < _source->label_keys.count) &&
         (index < _source->label_values.count);
         ++index)
    {
        status = d_arg_formatter_set_label(_destination,
                                           _source->label_keys.items[index],
                                           _source->label_values.items[index]);
    }

    // a pair refused: none kept
    if (status != D_ARG_OK)
    {
        d_arg_string_list_release(&_destination->label_keys);
        d_arg_string_list_release(&_destination->label_values);
    }

    return status;
}

/*
d_arg_formatter_copy
  The destination's labels are freed before its settings are overwritten, so
that the copy's lists, which at first alias the source's, are replaced by
empty ones before any label is copied into them.
*/
int
d_arg_formatter_copy(
    struct d_arg_formatter*       _destination,
    const struct d_arg_formatter* _source
)
{
    // validate the parameters
    if ( (!_destination) ||
         (!_source) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a copy onto itself has nothing to do
    if (_destination == _source)
    {
        return D_ARG_OK;
    }

    d_arg_string_list_release(&_destination->label_keys);
    d_arg_string_list_release(&_destination->label_values);
    *_destination              = *_source;
    _destination->label_keys   = LABELS_EMPTY;
    _destination->label_values = LABELS_EMPTY;

    return d_arg_internal_formatter_copy_labels(_destination,
                                                _source);
}

/*
d_arg_formatter_set_label
  CLI11's label(): the value copied first, so that a refused copy leaves the
labels as they were; a new key is copied next, and taken back out when its
value cannot be added beside it.
*/
int
d_arg_formatter_set_label(
    struct d_arg_formatter* _formatter,
    const char*             _key,
    const char*             _value
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_key)       ||
         (!_value) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const long index = d_arg_internal_find_member(d_arg_internal_text_of(_key),
                                                  &_formatter->label_keys,
                                                  false,
                                                  false);
    char*      value = d_arg_internal_copy_cstr(_value);

    // the value could not be copied
    if (!value)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // a key already labelled takes the new value
    if (index >= 0)
    {
        free(_formatter->label_values.items[index]);
        _formatter->label_values.items[index] = value;

        return D_ARG_OK;
    }

    // a new key, then its value
    if (d_arg_internal_string_copy(&_formatter->label_keys,
                                   d_arg_internal_text_of(_key),
                                   NULL) != D_ARG_OK)
    {
        free(value);

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // the value refused: the key goes again
    if (d_arg_internal_string_adopt(&_formatter->label_values,
                                    value) != D_ARG_OK)
    {
        d_arg_internal_string_truncate(&_formatter->label_keys,
                                       _formatter->label_keys.count - 1u);

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    return D_ARG_OK;
}

/*
d_arg_formatter_label
  CLI11's get_label(): a map lookup, here a search of the parallel lists.
*/
struct d_pack_text
d_arg_formatter_label(
    const struct d_arg_formatter* _formatter,
    struct d_pack_text            _key
)
{
    // no formatter: the word itself
    if (!_formatter)
    {
        return _key;
    }

    const long index = d_arg_internal_find_member(_key,
                                                  &_formatter->label_keys,
                                                  false,
                                                  false);

    return ( (index >= 0) &&
             ((size_t)index < _formatter->label_values.count) )
           ? d_arg_internal_text_of(_formatter->label_values.items[index])
           : _key;
}

/*
d_arg_formatter_set_column_width
  CLI11's column_width().
*/
void
d_arg_formatter_set_column_width(
    struct d_arg_formatter* _formatter,
    size_t                  _width
)
{
    // nothing to set without a formatter
    if (_formatter != NULL)
    {
        _formatter->column_width = _width;
    }

    return;
}

/*
d_arg_formatter_set_right_column_width
  CLI11's right_column_width().
*/
void
d_arg_formatter_set_right_column_width(
    struct d_arg_formatter* _formatter,
    size_t                  _width
)
{
    // nothing to set without a formatter
    if (_formatter != NULL)
    {
        _formatter->right_column_width = _width;
    }

    return;
}

/*
d_arg_formatter_set_description_paragraph_width
  CLI11's description_paragraph_width().
*/
void
d_arg_formatter_set_description_paragraph_width(
    struct d_arg_formatter* _formatter,
    size_t                  _width
)
{
    // nothing to set without a formatter
    if (_formatter != NULL)
    {
        _formatter->description_paragraph_width = _width;
    }

    return;
}

/*
d_arg_formatter_set_footer_paragraph_width
  CLI11's footer_paragraph_width().
*/
void
d_arg_formatter_set_footer_paragraph_width(
    struct d_arg_formatter* _formatter,
    size_t                  _width
)
{
    // nothing to set without a formatter
    if (_formatter != NULL)
    {
        _formatter->footer_paragraph_width = _width;
    }

    return;
}

/*
d_arg_formatter_set_line_width
  The longest line help is to write; 0 for no limit.
*/
void
d_arg_formatter_set_line_width(
    struct d_arg_formatter* _formatter,
    size_t                  _width
)
{
    // nothing to set without a formatter
    if (_formatter != NULL)
    {
        _formatter->line_width = _width;
    }

    return;
}

/*
d_arg_formatter_set_long_option_alignment_ratio
  CLI11's long_option_alignment_ratio(), through the rule help reads the
field by.
*/
void
d_arg_formatter_set_long_option_alignment_ratio(
    struct d_arg_formatter* _formatter,
    float                   _ratio
)
{
    // nothing to set without a formatter
    if (_formatter != NULL)
    {
        _formatter->long_option_alignment_ratio =
            d_arg_internal_help_ratio(_ratio);
    }

    return;
}

/*
d_arg_formatter_set_flag
  One switch set or cleared.
*/
void
d_arg_formatter_set_flag(
    struct d_arg_formatter* _formatter,
    uint32_t                _flag,
    bool                    _value
)
{
    // validate the parameters
    if (!_formatter)
    {
        return;
    }

    _formatter->flags = (_value)
                        ? (_formatter->flags | _flag)
                        : (_formatter->flags & ~_flag);

    return;
}

/*
d_arg_command_formatter
  The nearest formatter up the tree, the parser's last: a command that has
none of its own follows whatever its parent uses at the time help is made.
*/
const struct d_arg_formatter*
d_arg_command_formatter(
    const struct d_arg_command* _command
)
{
    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    // the command's own, or the nearest ancestor's
    for (const struct d_arg_command* current = _command;
         current != NULL;
         current = current->parent)
    {
        // the first one set
        if (current->formatter != NULL)
        {
            return current->formatter;
        }
    }

    return (_command->parser != NULL)
           ? &_command->parser->formatter
           : NULL;
}

//==============================================================================
// 2.  DISPATCH
//==============================================================================

/*
d_arg_format_help
  The table's make_help, or CLI11's.
*/
int
d_arg_format_help(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_help != NULL) )
           ? _formatter->vtable->make_help(_formatter,
                                           _command,
                                           _name,
                                           _mode,
                                           _out)
           : d_arg_format_help_default(_formatter,
                                       _command,
                                       _name,
                                       _mode,
                                       _out);
}

/*
d_arg_format_group
  The table's make_group, or CLI11's.
*/
int
d_arg_format_group(
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

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_group != NULL) )
           ? _formatter->vtable->make_group(_formatter,
                                            _group,
                                            _positional,
                                            _options,
                                            _out)
           : d_arg_format_group_default(_formatter,
                                        _group,
                                        _positional,
                                        _options,
                                        _out);
}

/*
d_arg_format_positionals
  The table's make_positionals, or CLI11's.
*/
int
d_arg_format_positionals(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_positionals != NULL) )
           ? _formatter->vtable->make_positionals(_formatter,
                                                  _command,
                                                  _out)
           : d_arg_format_positionals_default(_formatter,
                                              _command,
                                              _out);
}

/*
d_arg_format_subcommands
  The table's make_subcommands, or CLI11's.
*/
int
d_arg_format_subcommands(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_subcommands != NULL) )
           ? _formatter->vtable->make_subcommands(_formatter,
                                                  _command,
                                                  _mode,
                                                  _out)
           : d_arg_format_subcommands_default(_formatter,
                                              _command,
                                              _mode,
                                              _out);
}

/*
d_arg_format_subcommand
  The table's make_subcommand, or CLI11's.
*/
int
d_arg_format_subcommand(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_subcommand != NULL) )
           ? _formatter->vtable->make_subcommand(_formatter,
                                                 _command,
                                                 _out)
           : d_arg_format_subcommand_default(_formatter,
                                             _command,
                                             _out);
}

/*
d_arg_format_expanded
  The table's make_expanded, or CLI11's.
*/
int
d_arg_format_expanded(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_expanded != NULL) )
           ? _formatter->vtable->make_expanded(_formatter,
                                               _command,
                                               _mode,
                                               _out)
           : d_arg_format_expanded_default(_formatter,
                                           _command,
                                           _mode,
                                           _out);
}

/*
d_arg_format_footer
  The table's make_footer, or CLI11's.
*/
int
d_arg_format_footer(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_footer != NULL) )
           ? _formatter->vtable->make_footer(_formatter,
                                             _command,
                                             _out)
           : d_arg_format_footer_default(_formatter,
                                         _command,
                                         _out);
}

/*
d_arg_format_description
  The table's make_description, or CLI11's.
*/
int
d_arg_format_description(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_description != NULL) )
           ? _formatter->vtable->make_description(_formatter,
                                                  _command,
                                                  _out)
           : d_arg_format_description_default(_formatter,
                                              _command,
                                              _out);
}

/*
d_arg_format_usage
  The table's make_usage, or CLI11's.
*/
int
d_arg_format_usage(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_usage != NULL) )
           ? _formatter->vtable->make_usage(_formatter,
                                            _command,
                                            _name,
                                            _out)
           : d_arg_format_usage_default(_formatter,
                                        _command,
                                        _name,
                                        _out);
}

/*
d_arg_format_option
  The table's make_option, or CLI11's.
*/
int
d_arg_format_option(
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

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_option != NULL) )
           ? _formatter->vtable->make_option(_formatter,
                                             _option,
                                             _positional,
                                             _out)
           : d_arg_format_option_default(_formatter,
                                         _option,
                                         _positional,
                                         _out);
}

/*
d_arg_format_option_name
  The table's make_option_name, or CLI11's.
*/
int
d_arg_format_option_name(
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

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_option_name != NULL) )
           ? _formatter->vtable->make_option_name(_formatter,
                                                  _option,
                                                  _positional,
                                                  _out)
           : d_arg_format_option_name_default(_formatter,
                                              _option,
                                              _positional,
                                              _out);
}

/*
d_arg_format_option_opts
  The table's make_option_opts, or CLI11's.
*/
int
d_arg_format_option_opts(
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

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_option_opts != NULL) )
           ? _formatter->vtable->make_option_opts(_formatter,
                                                  _option,
                                                  _out)
           : d_arg_format_option_opts_default(_formatter,
                                              _option,
                                              _out);
}

/*
d_arg_format_option_desc
  The table's make_option_desc, or CLI11's.
*/
int
d_arg_format_option_desc(
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

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_option_desc != NULL) )
           ? _formatter->vtable->make_option_desc(_formatter,
                                                  _option,
                                                  _out)
           : d_arg_format_option_desc_default(_formatter,
                                              _option,
                                              _out);
}

/*
d_arg_format_option_usage
  The table's make_option_usage, or CLI11's.
*/
int
d_arg_format_option_usage(
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

    return ( (_formatter->vtable != NULL) &&
             (_formatter->vtable->make_option_usage != NULL) )
           ? _formatter->vtable->make_option_usage(_formatter,
                                                   _option,
                                                   _out)
           : d_arg_format_option_usage_default(_formatter,
                                               _option,
                                               _out);
}

//==============================================================================
// 3.  TEXT
//==============================================================================

// RATIO_DEFAULT
//   constant: CLI11's long_option_alignment_ratio_ as constructed.
static const float RATIO_DEFAULT = 1.0f / 3.0f;

/*
d_arg_internal_help_mode
  Whether a value is one of d_arg_help_mode's.
*/
bool
d_arg_internal_help_mode(
    int _mode
)
{
    return ( (_mode == (int)D_ARG_HELP_NORMAL) ||
             (_mode == (int)D_ARG_HELP_ALL)    ||
             (_mode == (int)D_ARG_HELP_SUB) );
}

/*
d_arg_internal_help_ratio
  CLI11's long_option_alignment_ratio() setter: a ratio from 0 to 1 kept, one
above inverted, a negative one taken by its magnitude (and inverted past
-1). A NaN, which CLI11 would keep and later convert to an int as undefined
behaviour, compares false with everything and is caught first. An inverse
is the float CLI11's member holds (d_arg_internal_float_stored), not the
quotient a unit with more precision computed.
*/
float
d_arg_internal_help_ratio(
    float _ratio
)
{
    // not a number: the default
    if (_ratio != _ratio)
    {
        return RATIO_DEFAULT;
    }

    // from 0 up
    if (_ratio >= 0.0f)
    {
        return (_ratio <= 1.0f)
               ? _ratio
               : d_arg_internal_float_stored(1.0f / _ratio);
    }

    return (_ratio < -1.0f)
           ? d_arg_internal_float_stored(1.0f / (-_ratio))
           : -_ratio;
}

/*
d_arg_internal_help_int
  CLI11's static_cast<int> of a width, held at INT_MAX where the cast would
wrap: a padding that large fails as memory rather than vanishing.
*/
int
d_arg_internal_help_int(
    size_t _value
)
{
    return (_value <= (size_t)INT_MAX)
           ? (int)_value
           : INT_MAX;
}

/*
d_arg_internal_help_spaces
  `_count` spaces appended, a block at a time.
*/
int
d_arg_internal_help_spaces(
    struct d_arg_buffer* _out,
    size_t               _count
)
{
    static const char SPACES[] = "                                ";
    size_t            left     = _count;
    int               status   = D_ARG_OK;

    // a block at a time
    while ( (status == D_ARG_OK) &&
            (left > 0u) )
    {
        const size_t block = (left < (sizeof(SPACES) - 1u))
                             ? left
                             : (sizeof(SPACES) - 1u);

        status = d_arg_buffer_append(_out,
                                     SPACES,
                                     block);
        left  -= block;
    }

    return status;
}

/*
d_arg_internal_help_pad
  std::setw with std::left: the text appended since `_start` padded with
spaces to `_width` characters; a width it already fills, or one below 1,
adds nothing.
*/
int
d_arg_internal_help_pad(
    struct d_arg_buffer* _out,
    size_t               _start,
    int                  _width
)
{
    const size_t written = (_out->length >= _start)
                           ? (_out->length - _start)
                           : 0u;

    return ( (_width > 0) &&
             (written < (size_t)_width) )
           ? d_arg_internal_help_spaces(_out,
                                        (size_t)_width - written)
           : D_ARG_OK;
}

/**
 * @brief Takes the next word of a line.
 *
 * @param[in]     _line  the line.
 * @param[in,out] _at    the place to read from, moved past the word.
 * @return the word; an empty span when none is left.
 *
 * The next word of a line from `*_at` on -- the run of characters up to a
 * space, the spaces before it skipped -- with `*_at` moved past it; an empty
 * span when none is left. CLI11 reads the words with `>>`, whose spaces are
 * the "C" locale's six.
 */
D_STATIC struct d_pack_text
d_arg_internal_help_word(
    struct d_pack_text _line,
    size_t*            _at
)
{
    // the spaces before the word
    while ( (*_at < _line.length) &&
            (d_arg_internal_is_space(_line.data[*_at])) )
    {
        (*_at)++;
    }

    const size_t start = *_at;

    // the word itself
    while ( (*_at < _line.length) &&
            (!d_arg_internal_is_space(_line.data[*_at])) )
    {
        (*_at)++;
    }

    return d_arg_internal_span(_line.data + start,
                               *_at - start);
}

/**
 * @brief Writes one line of a paragraph, wrapped to a width.
 *
 * @param[in,out] _out     the buffer that gains the line.
 * @param[in]     _line    the line.
 * @param[in]     _width   the width to keep within.
 * @param[in]     _prefix  how many spaces begin each line the wrapping adds.
 * @return D_ARG_OK, or the buffer's status.
 *
 * One line of a paragraph: its words rejoined by single spaces, a line feed
 * and the prefix going before a word that would carry the line past the
 * width.
 */
D_STATIC int
d_arg_internal_help_words(
    struct d_arg_buffer* _out,
    struct d_pack_text   _line,
    size_t               _width,
    size_t               _prefix
)
{
    size_t             written = 0u;
    size_t             at      = 0u;
    struct d_pack_text word    = d_arg_internal_help_word(_line,
                                                          &at);
    int                status  = D_ARG_OK;

    // each word in turn
    while ( (status == D_ARG_OK) &&
            (word.length > 0u) )
    {
        // a word that does not fit after what the line holds starts another
        if ( (written > 0u) &&
             (word.length + 1u + written > _width) )
        {
            status  = d_arg_buffer_append_char(_out,
                                               '\n');
            status  = (status == D_ARG_OK)
                      ? d_arg_internal_help_spaces(_out,
                                                   _prefix)
                      : status;
            written = 0u;
        }

        status   = ( (status == D_ARG_OK) &&
                     (written > 0u) )
                   ? d_arg_buffer_append_char(_out,
                                              ' ')
                   : status;
        written += (written > 0u) ? (word.length + 1u) : word.length;
        status   = (status == D_ARG_OK)
                   ? d_arg_buffer_append_text(_out,
                                              word)
                   : status;
        word     = d_arg_internal_help_word(_line,
                                            &at);
    }

    return status;
}

/*
d_arg_internal_help_paragraph
  CLI11's streamOutAsParagraph. The text is read a line at a time as getline
reads it: a line feed ends a line, and only a line that ended in one is
followed by one, so text without a final line feed gets none, and an empty
text writes nothing. Each line starts with the prefix but, when asked, the
first.
*/
int
d_arg_internal_help_paragraph(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text,
    size_t               _width,
    size_t               _prefix,
    bool                 _skip_first
)
{
    size_t at     = 0u;
    bool   skip   = _skip_first;
    int    status = D_ARG_OK;

    // each line in turn
    while ( (status == D_ARG_OK) &&
            (at < _text.length) )
    {
        const char*  feed = (const char*)memchr(_text.data + at,
                                                '\n',
                                                _text.length - at);
        const size_t end  = (feed != NULL)
                            ? (size_t)(feed - _text.data)
                            : _text.length;

        status = (skip)
                 ? D_ARG_OK
                 : d_arg_internal_help_spaces(_out,
                                              _prefix);
        skip   = false;
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_words(_out,
                                             d_arg_internal_span(
                                                 _text.data + at,
                                                 end - at),
                                             _width,
                                             _prefix)
                 : status;

        // a line that ended in a line feed keeps it
        if ( (status == D_ARG_OK) &&
             (feed != NULL) )
        {
            status = d_arg_buffer_append_char(_out,
                                              '\n');
        }

        at = (feed != NULL) ? end + 1u : _text.length;
    }

    return status;
}

/*
d_arg_internal_help_signed
  A count in decimal, as std::to_string or `<<` writes an int.
*/
int
d_arg_internal_help_signed(
    struct d_arg_buffer* _out,
    intmax_t             _value
)
{
    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    d_arg_internal_text_signed(&text,
                               _value);

    return d_arg_buffer_append(_out,
                               text.data,
                               text.length);
}

/*
d_arg_internal_help_unsigned
  A count in decimal, as std::to_string writes a size_t.
*/
int
d_arg_internal_help_unsigned(
    struct d_arg_buffer* _out,
    uintmax_t            _value
)
{
    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    d_arg_internal_text_unsigned(&text,
                                 _value);

    return d_arg_buffer_append(_out,
                               text.data,
                               text.length);
}

/*
d_arg_internal_help_column
  A description in the right column, as make_option and make_subcommand
write it: nothing for an empty one; otherwise wrapped behind the column's
indent, starting where the left part ends or, when that reached the
column, on a line of its own.
*/
int
d_arg_internal_help_column(
    const struct d_arg_formatter* _formatter,
    struct d_pack_text            _text,
    bool                          _next_line,
    struct d_arg_buffer*          _out
)
{
    // no description, nothing written
    if (_text.length == 0u)
    {
        return D_ARG_OK;
    }

    const int status = (_next_line)
                       ? d_arg_buffer_append_char(_out,
                                                  '\n')
                       : D_ARG_OK;

    return (status == D_ARG_OK)
           ? d_arg_internal_help_paragraph(
                 _out,
                 _text,
                 d_arg_internal_help_fitted(_formatter,
                                            _formatter->right_column_width,
                                            _formatter->column_width),
                 _formatter->column_width,
                 !_next_line)
           : status;
}

/*
d_arg_internal_help_label
  CLI11's get_label(key), appended.
*/
int
d_arg_internal_help_label(
    const struct d_arg_formatter* _formatter,
    const char*                   _key,
    struct d_arg_buffer*          _out
)
{
    return d_arg_buffer_append_text(_out,
                                    d_arg_formatter_label(
                                        _formatter,
                                        d_arg_internal_text_of(_key)));
}

/*
d_arg_internal_help_fitted
  A width narrowed so that a line of it, after `_indent` characters, keeps
within the line width: unchanged without a line width; otherwise no more
than the room the indent leaves, and one character at least, so that each
word then stands on its own line.
*/
size_t
d_arg_internal_help_fitted(
    const struct d_arg_formatter* _formatter,
    size_t                        _width,
    size_t                        _indent
)
{
    const size_t room = (_formatter->line_width > _indent)
                        ? (_formatter->line_width - _indent)
                        : 1u;

    return ( (_formatter->line_width > 0u) &&
             (room < _width) )
           ? room
           : _width;
}

//==============================================================================
// 4.  HELP
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)

/**
 * @brief Measures the terminal's width.
 *
 * @return the width in columns; 0 when it is unknown, or in a build that
 *         measures none.
 *
 * The terminal's width, as arg_system.c measures it; 0 when unknown.
 */
D_STATIC size_t
d_arg_internal_help_terminal(void)
{
    return d_arg_terminal_width();
}

#else

/**
 * @brief Measures the terminal's width.
 *
 * @return the width in columns; 0 when it is unknown, or in a build that
 *         measures none.
 *
 * No terminal is measured in this build.
 */
D_STATIC size_t
d_arg_internal_help_terminal(void)
{
    return 0u;
}

#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH

/**
 * @brief Gives the line width a formatter lays help out within.
 *
 * @param[in] _formatter  the formatter.
 * @param[in] _limit      the room the help it is part of leaves it; 0 for no
 *                        limit.
 * @return the width; 0 for none.
 *
 * The line width a formatter lays help out within: its own; one less than
 * the terminal's, measured now, when it asks for that and the terminal is
 * narrower; and, once it keeps to a width at all, no more than the room the
 * help it is part of leaves it (`_limit`, 0 for no such limit).
 */
D_STATIC size_t
d_arg_internal_help_fit(
    const struct d_arg_formatter* _formatter,
    size_t                        _limit
)
{
    size_t width = _formatter->line_width;

    // the terminal, its last column left free
    if ((_formatter->flags & D_ARG_FORMAT_TERMINAL_WIDTH) != 0u)
    {
        const size_t columns = d_arg_internal_help_terminal();

        // a terminal measured, narrower than the width set
        if ( (columns > 1u) &&
             ( (width == 0u) ||
               (width > (columns - 1u)) ) )
        {
            width = columns - 1u;
        }
    }

    return ( (width > 0u)  &&
             (_limit > 0u) &&
             (_limit < width) )
           ? _limit
           : width;
}

/**
 * @brief Finds the command whose help App::help gives, building its name.
 *
 * @param[in]     _command  the command help was asked of.
 * @param[in,out] _name     the buffer that gains the name.
 * @param[in,out] _status   the status so far, the walk ending when it is not
 *                          D_ARG_OK; receives the buffer's failure.
 * @return the command reached.
 *
 * The command whose help App::help gives: the last subcommand the last
 * parse used, followed down the tree. The name is built on the way, the words
 * before and each command's own joined by a space unless there are none
 * before -- so a nameless command after words leaves a trailing space, as in
 * CLI11.
 */
D_STATIC const struct d_arg_command*
d_arg_internal_help_reached(
    const struct d_arg_command* _command,
    struct d_arg_buffer*        _name,
    int*                        _status
)
{
    const struct d_arg_command* current = _command;

    // each command on the way down
    while (*_status == D_ARG_OK)
    {
        *_status = (_name->length > 0u)
                   ? d_arg_buffer_append_char(_name,
                                              ' ')
                   : D_ARG_OK;
        *_status = (*_status == D_ARG_OK)
                   ? d_arg_buffer_append_cstr(_name,
                                              d_arg_command_name(current))
                   : *_status;

        // no subcommand used: this command's help
        if (current->parsed_subcommands.count == 0u)
        {
            break;
        }

        current = current->parsed_subcommands.items[
                      current->parsed_subcommands.count - 1u];
    }

    return current;
}

/*
d_arg_internal_help_make
  CLI11's App::help(prev, mode): the command reached builds its help with its
formatter, copied so that the line width can be fitted without touching the
original; the copy shares the original's labels and is never released.
*/
int
d_arg_internal_help_make(
    const struct d_arg_command* _command,
    struct d_pack_text          _prefix,
    int                         _mode,
    size_t                      _limit,
    struct d_arg_buffer*        _out
)
{
    // validate the parameters
    if ( (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer           name      = D_ARG_BUFFER_INIT;
    int                           status    = d_arg_buffer_append_text(
                                                  &name,
                                                  _prefix);
    const struct d_arg_command*   current   = d_arg_internal_help_reached(
                                                  _command,
                                                  &name,
                                                  &status);
    const struct d_arg_formatter* formatter = d_arg_command_formatter(current);

    // built with a fitted copy of the formatter
    if ( (status == D_ARG_OK) &&
         (formatter != NULL) )
    {
        struct d_arg_formatter fitted = *formatter;

        fitted.line_width = d_arg_internal_help_fit(formatter,
                                                    _limit);
        status            = d_arg_format_help(&fitted,
                                              current,
                                              d_arg_buffer_text(&name),
                                              _mode,
                                              _out);
    }

    d_arg_buffer_release(&name);

    return ( (status == D_ARG_OK) &&
             (formatter == NULL) )
           ? D_ARG_ERROR_BAD_ARGUMENT
           : status;
}

/*
d_arg_help
  Built whole, then written: a failure leaves the sink untouched.
*/
int
d_arg_help(
    const struct d_arg_command* _command,
    struct d_pack_text          _prefix,
    int                         _mode,
    struct d_pack_sink          _sink
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_help_make(_command,
                                                          _prefix,
                                                          _mode,
                                                          0u,
                                                          &text);

    // written whole
    if (status == D_ARG_OK)
    {
        status = d_arg_sink_write(_sink,
                                  text.data,
                                  text.length);
    }

    d_arg_buffer_release(&text);

    return status;
}
