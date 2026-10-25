/*******************************************************************************
* djinterp [parse]                                                  arg_config.c
*
* Configuration formats and items, declared in arg_config.h (3, 4 and 5.1).
*   CLI11's ConfigBase settings as a plain record -- ConfigTOML's by default,
* ConfigINI's on request -- and the lookup a command makes for the format it
* reads and writes with; CLI11's ConfigItem's lifetime, building -- its parts
* copied into its storage -- full name and reading as a flag; and the
* dispatch of reading text and files and of writing a
* command's values through a format's table, whose empty entries run
* CLI11's own operations, defined in arg_config_read.c and
* arg_config_write.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_config.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "./arg_internal.h"  // copies, the system layer
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  FORMATS
//==============================================================================

// FORMAT_TOML
//   constant: CLI11's ConfigBase -- ConfigTOML -- as constructed: `#`
// comments, `[` `]` arrays separated by `,`, `=` between a name and its
// value, `"` strings and `'` literals, `.` between sections, 255 layers, every
// section at every index, CLI11's own operations.
static const struct d_arg_config_format FORMAT_TOML =
{
    NULL,  // vtable
    NULL,  // context
    NULL,  // section
    -1,    // index
    255,   // max_layers
    0u,    // flags
    '#',   // comment
    '[',   // array_start
    ']',   // array_end
    ',',   // array_separator
    '=',   // value_delimiter
    '"',   // string_quote
    '\'',  // literal_quote
    '.'    // parent_separator
};

/*
d_arg_config_format_init
  CLI11's ConfigTOML.
*/
void
d_arg_config_format_init(
    struct d_arg_config_format* _format
)
{
    // validate the parameters
    if (!_format)
    {
        return;
    }

    *_format = FORMAT_TOML;

    return;
}

/*
d_arg_config_format_init_ini
  CLI11's ConfigINI: ConfigTOML with `;` comments, no array brackets, and
values separated by spaces.
*/
void
d_arg_config_format_init_ini(
    struct d_arg_config_format* _format
)
{
    // validate the parameters
    if (!_format)
    {
        return;
    }

    *_format                 = FORMAT_TOML;
    _format->comment         = ';';
    _format->array_start     = '\0';
    _format->array_end       = '\0';
    _format->array_separator = ' ';
    _format->value_delimiter = '=';

    return;
}

/*
d_arg_config_format_release
  The section's name freed, the format made CLI11's ConfigTOML again.
*/
void
d_arg_config_format_release(
    struct d_arg_config_format* _format
)
{
    // validate the parameters
    if (!_format)
    {
        return;
    }

    free(_format->section);
    *_format = FORMAT_TOML;

    return;
}

/*
d_arg_config_format_set_section
  CLI11's section(): the name copied, replacing the one before only once the
copy is made; NULL or "" for every section.
*/
int
d_arg_config_format_set_section(
    struct d_arg_config_format* _format,
    const char*                 _section
)
{
    // validate the parameters
    if (!_format)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool  named = ( (_section != NULL) &&
                          (_section[0] != '\0') );
    char* const copy  = (named)
                        ? d_arg_internal_copy_cstr(_section)
                        : NULL;

    // check if memory allocation was successful
    if ( (named) &&
         (!copy) )
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    free(_format->section);
    _format->section = copy;

    return D_ARG_OK;
}

/*
d_arg_config_format_set_index
  CLI11's index(): which occurrence of a repeated section is read, -1 for
all of them.
*/
void
d_arg_config_format_set_index(
    struct d_arg_config_format* _format,
    int                         _index
)
{
    // validate the parameters
    if (!_format)
    {
        return;
    }

    _format->index = _index;

    return;
}

/*
d_arg_config_format_set_max_layers
  CLI11's maxLayers(), whose uint8_t holds 0 to 255; a value outside is held
at the nearer end.
*/
void
d_arg_config_format_set_max_layers(
    struct d_arg_config_format* _format,
    int                         _layers
)
{
    // validate the parameters
    if (!_format)
    {
        return;
    }

    _format->max_layers = (_layers < 0)
                          ? 0
                          : (_layers > 255) ? 255 : _layers;

    return;
}

/*
d_arg_config_format_set_flag
  CLI11's commentDefaults() and allowDuplicateFields(): one switch on or
off; any other bit is ignored.
*/
void
d_arg_config_format_set_flag(
    struct d_arg_config_format* _format,
    uint32_t                    _flag,
    bool                        _value
)
{
    // validate the parameters
    if (!_format)
    {
        return;
    }

    const uint32_t known = _flag & (D_ARG_CONFIG_COMMENT_DEFAULTS |
                                    D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS);

    _format->flags = (_value)
                     ? (_format->flags | known)
                     : (_format->flags & ~known);

    return;
}

/*
d_arg_command_config_format
  The command's own format, else the nearest ancestor's, else the parser's.
*/
const struct d_arg_config_format*
d_arg_command_config_format(
    const struct d_arg_command* _command
)
{
    const struct d_arg_command* command = _command;

    // up the tree to a format
    while (command != NULL)
    {
        if (command->config_format != NULL)
        {
            return command->config_format;
        }

        // the root falls back on its parser's
        if (command->parent == NULL)
        {
            return (command->parser != NULL)
                   ? &command->parser->config_format
                   : NULL;
        }

        command = command->parent;
    }

    return NULL;
}

//==============================================================================
// 2.  ITEMS
//==============================================================================

// ITEM_EMPTY
//   constant: an item as D_ARG_CONFIG_ITEM_INIT makes it, which release and
// adoption leave behind.
static const struct d_arg_config_item ITEM_EMPTY = D_ARG_CONFIG_ITEM_INIT;

/*
d_arg_config_item_release
  The item's lists and copies, the item left empty.
*/
void
d_arg_config_item_release(
    struct d_arg_config_item* _item
)
{
    // validate the parameters
    if (!_item)
    {
        return;
    }

    d_arg_text_list_release(&_item->parents);
    d_arg_text_list_release(&_item->inputs);
    d_arg_string_list_release(&_item->storage);
    *_item = ITEM_EMPTY;

    return;
}

/*
d_arg_config_item_list_release
  Every item, then the list's storage.
*/
void
d_arg_config_item_list_release(
    struct d_arg_config_item_list* _items
)
{
    // validate the parameters
    if (!_items)
    {
        return;
    }

    // each item in turn
    for (size_t index = 0u; index < _items->count; ++index)
    {
        d_arg_config_item_release(&_items->items[index]);
    }

    free(_items->items);
    _items->items    = NULL;
    _items->count    = 0u;
    _items->capacity = 0u;

    return;
}

/**
 * @brief Keeps a copy of text in an item's storage.
 *
 * @param[in,out] _item  the item, whose storage gains the copy.
 * @param[in]     _text  the text.
 * @param[out]    _copy  receives the copy's span.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with the item and `_copy`
 *         unchanged.
 *
 * A copy of text kept in an item's storage, its span written to `_copy`;
 * refused, the item and `_copy` stay as they were.
 */
D_STATIC int
d_arg_internal_config_keep(
    struct d_arg_config_item* _item,
    struct d_pack_text        _text,
    struct d_pack_text*       _copy
)
{
    const char* kept   = NULL;
    const int   status = d_arg_internal_string_copy(&_item->storage,
                                                    _text,
                                                    &kept);

    // the copy's span, once made
    if (status == D_ARG_OK)
    {
        *_copy = d_arg_internal_span(kept,
                                     _text.length);
    }

    return status;
}

/**
 * @brief Appends a copy of text to one of an item's lists.
 *
 * @param[in,out] _item  the item, whose storage keeps the copy.
 * @param[in,out] _list  the list, one of the item's.
 * @param[in]     _text  the text.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with the item and the list
 *         holding what they held.
 *
 * A copy of text appended to one of an item's lists, room made first so
 * that a refusal leaves no copy behind.
 */
D_STATIC int
d_arg_internal_config_append(
    struct d_arg_config_item* _item,
    struct d_arg_text_list*   _list,
    struct d_pack_text        _text
)
{
    struct d_pack_text copy   = d_arg_internal_span("",
                                                    0u);
    int                status = d_arg_internal_text_reserve(_list,
                                                            _list->count + 1u);

    status = (status == D_ARG_OK)
             ? d_arg_internal_config_keep(_item,
                                          _text,
                                          &copy)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_text_push(_list,
                                      copy)
           : status;
}

/**
 * @brief Reads a span given to a builder.
 *
 * @param[in,out] _text  the span, made the empty string when its data is NULL.
 * @return `true` for a span that can be read; `false` for NULL data with a
 *         length.
 *
 * A span given to a builder, its NULL data read as the empty string; NULL
 * data with a length is refused.
 */
D_STATIC bool
d_arg_internal_config_text(
    struct d_pack_text* _text
)
{
    // no data but a length
    if ( (!_text->data) &&
         (_text->length > 0u) )
    {
        return false;
    }

    // no data at all reads as the empty string
    if (!_text->data)
    {
        *_text = d_arg_internal_span("",
                                     0u);
    }

    return true;
}

/*
d_arg_config_item_set_name
  The name copied into the item's storage.
*/
int
d_arg_config_item_set_name(
    struct d_arg_config_item* _item,
    struct d_pack_text        _text
)
{
    struct d_pack_text text = _text;

    // validate the parameters
    if ( (!_item) ||
         (!d_arg_internal_config_text(&text)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_config_keep(_item,
                                      text,
                                      &_item->name);
}

/*
d_arg_config_item_add_parent
  A section appended, innermost last.
*/
int
d_arg_config_item_add_parent(
    struct d_arg_config_item* _item,
    struct d_pack_text        _text
)
{
    struct d_pack_text text = _text;

    // validate the parameters
    if ( (!_item) ||
         (!d_arg_internal_config_text(&text)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_config_append(_item,
                                        &_item->parents,
                                        text);
}

/*
d_arg_config_item_add_input
  A value appended.
*/
int
d_arg_config_item_add_input(
    struct d_arg_config_item* _item,
    struct d_pack_text        _text
)
{
    struct d_pack_text text = _text;

    // validate the parameters
    if ( (!_item) ||
         (!d_arg_internal_config_text(&text)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_config_append(_item,
                                        &_item->inputs,
                                        text);
}

/*
d_arg_config_item_list_adopt
  The item moved to the end of the list, which grows first; the item left
empty.
*/
int
d_arg_config_item_list_adopt(
    struct d_arg_config_item_list* _items,
    struct d_arg_config_item*      _item
)
{
    // validate the parameters
    if ( (!_items) ||
         (!_item) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // room for one more
    if (_items->count >= _items->capacity)
    {
        size_t                          capacity = 0u;
        struct d_arg_config_item* const grown    =
            (struct d_arg_config_item*)d_arg_internal_grow(
                _items->items,
                _items->capacity,
                _items->count + 1u,
                sizeof(struct d_arg_config_item),
                &capacity);

        // check if memory allocation was successful
        if (!grown)
        {
            return D_ARG_ERROR_OUT_OF_MEMORY;
        }

        _items->items    = grown;
        _items->capacity = capacity;
    }

    _items->items[_items->count] = *_item;
    _items->count++;
    *_item = ITEM_EMPTY;

    return D_ARG_OK;
}

/*
d_arg_config_item_fullname
  CLI11's fullname(): the parents and the name joined by `.`, as its join
joins them -- a `.` that ends the text dropped, as when the name is empty.
*/
int
d_arg_config_item_fullname(
    const struct d_arg_config_item* _item,
    struct d_arg_buffer*            _out
)
{
    // validate the parameters
    if ( (!_item) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t start = _out->length;

    // each parent, then the name
    for (size_t index = 0u; index < _item->parents.count; ++index)
    {
        (void)d_arg_buffer_append_text(_out,
                                       _item->parents.items[index]);
        (void)d_arg_buffer_append_char(_out,
                                       '.');
    }

    (void)d_arg_buffer_append_text(_out,
                                   _item->name);

    // a delimiter that ends the text is dropped
    if ( (_out->length > start) &&
         (_out->data[_out->length - 1u] == '.') )
    {
        d_arg_buffer_truncate(_out,
                              _out->length - 1u);
    }

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/*
d_arg_config_to_flag_default
  CLI11's Config::to_flag: the single value, `{}` for none; more than one is
the failure CLI11 reports as TooManyInputsFlag.
*/
int
d_arg_config_to_flag_default(
    const struct d_arg_config_format* _format,
    const struct d_arg_config_item*   _item,
    struct d_arg_buffer*              _out
)
{
    (void)_format;

    // validate the parameters
    if ( (!_item) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // more than one value
    if (_item->inputs.count > 1u)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    return d_arg_buffer_append_text(_out,
                                    (_item->inputs.count == 1u)
                                    ? _item->inputs.items[0]
                                    : d_arg_internal_span("{}",
                                                          2u));
}

/*
d_arg_config_to_flag
  The format's to_flag, or CLI11's when the table has none.
*/
int
d_arg_config_to_flag(
    const struct d_arg_config_format* _format,
    const struct d_arg_config_item*   _item,
    struct d_arg_buffer*              _out
)
{
    // validate the parameters
    if ( (!_format) ||
         (!_item)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_format->vtable != NULL) &&
             (_format->vtable->to_flag != NULL) )
           ? _format->vtable->to_flag(_format,
                                      _item,
                                      _out)
           : d_arg_config_to_flag_default(_format,
                                          _item,
                                          _out);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 3.  READING AND WRITING
//==============================================================================

/*
d_arg_config_from_text
  The format's from_config, or CLI11's when the table has none.
*/
int
d_arg_config_from_text(
    const struct d_arg_config_format* _format,
    struct d_pack_text                _input,
    struct d_arg_config_item_list*    _out,
    struct d_arg_buffer*              _error
)
{
    // validate the parameters
    if ( (!_format)                  ||
         (!_out)                     ||
         (!_error)                   ||
         ( (!_input.data) &&
           (_input.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_format->vtable != NULL) &&
             (_format->vtable->from_config != NULL) )
           ? _format->vtable->from_config(_format,
                                          _input,
                                          _out,
                                          _error)
           : d_arg_config_from_text_default(_format,
                                            _input,
                                            _out,
                                            _error);
}

/*
d_arg_config_from_file
  CLI11's from_file: a file read whole as text, as its std::ifstream reads
it, then read as configuration; one that cannot be read is
FileError::Missing.
*/
int
d_arg_config_from_file(
    const struct d_arg_config_format* _format,
    const char*                       _path,
    struct d_arg_config_item_list*    _out,
    struct d_arg_buffer*              _error
)
{
    // validate the parameters
    if ( (!_format) ||
         (!_path)   ||
         (!_out)    ||
         (!_error) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_read_file(_path,
                                                          true,
                                                          &text);

    // a file that cannot be read
    if (status == D_ARG_ERROR_FILE)
    {
        (void)d_arg_buffer_printf(_error,
                                  "%s was not readable (missing?)",
                                  _path);
    }

    status = (status == D_ARG_OK)
             ? d_arg_config_from_text(_format,
                                      d_arg_buffer_text(&text),
                                      _out,
                                      _error)
             : status;
    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_config_to_text
  The format's to_config, or CLI11's when the table has none.
*/
int
d_arg_config_to_text(
    const struct d_arg_config_format* _format,
    const struct d_arg_command*       _command,
    int                               _mode,
    bool                              _describe,
    struct d_pack_text                _prefix,
    struct d_arg_buffer*              _out
)
{
    // validate the parameters
    if ( (!_format)                   ||
         (!_command)                  ||
         (!_out)                      ||
         ( (!_prefix.data) &&
           (_prefix.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return ( (_format->vtable != NULL) &&
             (_format->vtable->to_config != NULL) )
           ? _format->vtable->to_config(_format,
                                        _command,
                                        _mode,
                                        _describe,
                                        _prefix,
                                        _out)
           : d_arg_config_to_text_default(_format,
                                          _command,
                                          _mode,
                                          _describe,
                                          _prefix,
                                          _out);
}

/*
d_arg_command_config_to_sink
  CLI11's config_to_str: the command written through its format with no
prefix, then handed to the sink.
*/
int
d_arg_command_config_to_sink(
    const struct d_arg_command* _command,
    int                         _mode,
    bool                        _describe,
    struct d_pack_sink          _sink
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const struct d_arg_config_format* const format =
        d_arg_command_config_format(_command);
    struct d_arg_buffer                     text   = D_ARG_BUFFER_INIT;

    // a command with no format to write with
    if (!format)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_config_to_text(format,
                                      _command,
                                      _mode,
                                      _describe,
                                      d_arg_internal_span("",
                                                          0u),
                                      &text);

    status = (status == D_ARG_OK)
             ? d_arg_sink_write(_sink,
                                d_arg_buffer_text(&text).data,
                                text.length)
             : status;
    d_arg_buffer_release(&text);

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
