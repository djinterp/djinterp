/*******************************************************************************
* djinterp [parse]                                             arg_config_read.c
*
* Reading configuration text, declared in arg_config.h (5.2.1).
*   CLI11's ConfigBase::from_config, a C function for each of its helpers and
* steps, over the text a line at a time as getline reads it: lines shorter
* than three characters, comment lines and `"""` comment blocks skipped; a
* `[section]` header -- `[[...]]` too, and with a comment after it --
* closing the section before it and opening the sections it names, as `--`
* and `++` items; then each `name = value` line split where CLI11 splits it,
* quotes and comments considered, its value a `'''` or `"""` string that
* may run over lines, an array in brackets that may too, a list, or a single
* value; a name alone being `true`. Each item's sections come from the
* section and the name's dotted parts, quotes removed; items outside the
* section asked for, or nested too deeply, are left out; and a name met
* again merges into the item before it, `%%` between groups.
*   Every part stays a span of the text, or of a value built over lines,
* until it is copied into its item, so that the bytes an escape or a binary
* string makes, NUL among them, are kept as CLI11's std::string keeps them.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_config_read.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr, memmove
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "./arg_internal.h"  // spans, splitting, escapes, lists


#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 1.  TEXT
//==============================================================================

// NPOS
//   constant: std::string::npos, the position of what is not there.
static const size_t NPOS = (size_t)-1;

// DEFAULT_SECTION
//   constant: the name CLI11 gives the top level, which a header naming it,
// in any case, returns to.
static const char DEFAULT_SECTION[] = "default";

/*
d_arg_internal_config_find
  std::string::find_first_of(_c, _from): the first `_c` at or after
`_from`, or NPOS; a `_from` past the end finds nothing.
*/
D_STATIC size_t
d_arg_internal_config_find(
    struct d_pack_text _text,
    char               _c,
    size_t             _from
)
{
    const char* const found = (_from < _text.length)
                              ? (const char*)memchr(_text.data + _from,
                                                    (unsigned char)_c,
                                                    _text.length - _from)
                              : NULL;

    return (found != NULL)
           ? (size_t)(found - _text.data)
           : NPOS;
}

/*
d_arg_internal_config_find_any
  find_first_of over a set of three bytes -- NUL among them when one is NUL,
as CLI11's std::string of them holds it -- from `_from` on.
*/
D_STATIC size_t
d_arg_internal_config_find_any(
    struct d_pack_text _text,
    const char*        _set,
    size_t             _from
)
{
    // each byte from `_from` on
    for (size_t at = _from; at < _text.length; ++at)
    {
        if ( (_text.data[at] == _set[0]) ||
             (_text.data[at] == _set[1]) ||
             (_text.data[at] == _set[2]) )
        {
            return at;
        }
    }

    return NPOS;
}

/*
d_arg_internal_config_skip
  find_first_not_of(_c, _from): the first byte at or after `_from` that is
not `_c`, or NPOS.
*/
D_STATIC size_t
d_arg_internal_config_skip(
    struct d_pack_text _text,
    char               _c,
    size_t             _from
)
{
    // past each `_c`
    for (size_t at = _from; at < _text.length; ++at)
    {
        if (_text.data[at] != _c)
        {
            return at;
        }
    }

    return NPOS;
}

/*
d_arg_internal_config_unblank
  find_first_not_of(" \t"): the first byte that is neither a space nor a
tab, or NPOS.
*/
D_STATIC size_t
d_arg_internal_config_unblank(
    struct d_pack_text _text
)
{
    // past the spaces and tabs
    for (size_t at = 0u; at < _text.length; ++at)
    {
        if ( (_text.data[at] != ' ') &&
             (_text.data[at] != '\t') )
        {
            return at;
        }
    }

    return NPOS;
}

/*
d_arg_internal_config_quote
  Whether a byte is one of the three quotes CLI11's reader passes over
whole.
*/
D_STATIC bool
d_arg_internal_config_quote(
    char _c
)
{
    return ( (_c == '"')  ||
             (_c == '\'') ||
             (_c == '`') );
}

/*
d_arg_internal_config_has_quote
  find_first_of("\"'`") != npos: whether the text holds any of the three.
*/
D_STATIC bool
d_arg_internal_config_has_quote(
    struct d_pack_text _text
)
{
    // each byte in turn
    for (size_t at = 0u; at < _text.length; ++at)
    {
        if (d_arg_internal_config_quote(_text.data[at]))
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_config_tail
  substr(_from): the text from `_from` on, empty past the end.
*/
D_STATIC struct d_pack_text
d_arg_internal_config_tail(
    struct d_pack_text _text,
    size_t             _from
)
{
    return (_from < _text.length)
           ? d_arg_internal_span(_text.data + _from,
                                 _text.length - _from)
           : d_arg_internal_span(_text.data + _text.length,
                                 0u);
}

/*
d_arg_internal_config_head
  substr(0, _count): the first `_count` bytes, all of them for NPOS.
*/
D_STATIC struct d_pack_text
d_arg_internal_config_head(
    struct d_pack_text _text,
    size_t             _count
)
{
    return d_arg_internal_span(_text.data,
                               (_count < _text.length)
                               ? _count
                               : _text.length);
}

/*
d_arg_internal_config_ltrim
  CLI11's ltrim: the text less its leading whitespace.
*/
D_STATIC struct d_pack_text
d_arg_internal_config_ltrim(
    struct d_pack_text _text
)
{
    size_t at = 0u;

    // past the whitespace
    while ( (at < _text.length) &&
            (d_arg_internal_is_space(_text.data[at])) )
    {
        at++;
    }

    return d_arg_internal_config_tail(_text,
                                      at);
}

/*
d_arg_internal_config_triple
  Whether text has three of a byte from `_at`: a `"""` or a `'''`.
*/
D_STATIC bool
d_arg_internal_config_triple(
    struct d_pack_text _text,
    size_t             _at,
    char               _c
)
{
    return ( (_at < _text.length)          &&
             ((_text.length - _at) >= 3u)  &&
             (_text.data[_at] == _c)       &&
             (_text.data[_at + 1u] == _c)  &&
             (_text.data[_at + 2u] == _c) );
}

/*
d_arg_internal_config_ml_opens
  Whether text starts with `"""` or `'''`, CLI11's multiline quotes.
*/
D_STATIC bool
d_arg_internal_config_ml_opens(
    struct d_pack_text _text
)
{
    return ( (d_arg_internal_config_triple(_text,
                                           0u,
                                           '"')) ||
             (d_arg_internal_config_triple(_text,
                                           0u,
                                           '\'')) );
}

/*
d_arg_internal_config_ml_string
  CLI11's hasMLString: whether text ends with three of a byte.
*/
D_STATIC bool
d_arg_internal_config_ml_string(
    struct d_pack_text _text,
    char               _check
)
{
    return ( (_text.length >= 3u) &&
             (d_arg_internal_config_triple(_text,
                                           _text.length - 3u,
                                           _check)) );
}

/*
d_arg_internal_config_ml_escaped
  Whether an odd run of backslashes comes right before `_at`, escaping the
quote there in a `"""` value.
*/
D_STATIC bool
d_arg_internal_config_ml_escaped(
    struct d_pack_text _text,
    size_t             _at
)
{
    size_t slashes = 0u;

    // back over the backslashes
    while ( (slashes < _at) &&
            (_text.data[_at - slashes - 1u] == '\\') )
    {
        slashes++;
    }

    return ((slashes % 2u) == 1u);
}

/*
d_arg_internal_config_ml_close
  CLI11's find_ml_close: the length of a multiline value's content before
the three quotes that close it -- unescaped for `"`, with up to two more
quotes belonging to the content, and only blanks or a comment after them --
or NPOS when the line does not close it.
*/
D_STATIC size_t
d_arg_internal_config_ml_close(
    struct d_pack_text _text,
    char               _quote,
    char               _comment
)
{
    size_t pos = 0u;

    // each run of three quotes in turn
    while (pos < _text.length)
    {
        // not three quotes here, or escaped ones
        if ( (!d_arg_internal_config_triple(_text,
                                            pos,
                                            _quote))      ||
             ( (_quote == '"') &&
               (d_arg_internal_config_ml_escaped(_text,
                                                 pos)) ) )
        {
            pos++;

            continue;
        }

        const size_t found = d_arg_internal_config_skip(_text,
                                                        _quote,
                                                        pos);
        const size_t end   = (found != NPOS) ? found : _text.length;
        size_t       rest  = end;

        // past the blanks after them
        while ( (rest < _text.length)          &&
                ( (_text.data[rest] == ' ')  ||
                  (_text.data[rest] == '\t') ||
                  (_text.data[rest] == '\r') ) )
        {
            rest++;
        }

        if ( (rest >= _text.length) ||
             (_text.data[rest] == _comment) )
        {
            return end - 3u;
        }

        pos = end;
    }

    return NPOS;
}

// d_arg_internal_config_lines
//   struct: std::getline over text: the text, and where the next line
// starts.
struct d_arg_internal_config_lines
{
    struct d_pack_text text;
    size_t             at;
};

/*
d_arg_internal_config_getline
  std::getline: the next line, without its `\n`; false once no byte is
left, so that text ending in `\n` ends there.
*/
D_STATIC bool
d_arg_internal_config_getline(
    struct d_arg_internal_config_lines* _lines,
    struct d_pack_text*                 _line
)
{
    // nothing left to read
    if (_lines->at >= _lines->text.length)
    {
        return false;
    }

    const size_t end  = d_arg_internal_config_find(_lines->text,
                                                   '\n',
                                                   _lines->at);
    const size_t stop = (end != NPOS) ? end : _lines->text.length;

    *_line     = d_arg_internal_span(_lines->text.data + _lines->at,
                                     stop - _lines->at);
    _lines->at = (end != NPOS) ? (end + 1u) : _lines->text.length;

    return true;
}

//==============================================================================
// 2.  ITEMS
//==============================================================================

/*
d_arg_internal_config_copy_parents
  The first `_count` spans of a list appended to an item's sections.
*/
D_STATIC int
d_arg_internal_config_copy_parents(
    struct d_arg_config_item*     _item,
    const struct d_arg_text_list* _parents,
    size_t                        _count
)
{
    int status = D_ARG_OK;

    // each in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _count);
         ++index)
    {
        status = d_arg_config_item_add_parent(_item,
                                              _parents->items[index]);
    }

    return status;
}

/*
d_arg_internal_config_adopt
  An item built for the list adopted by it, or, when building it failed,
released; the item is left empty either way.
*/
D_STATIC int
d_arg_internal_config_adopt(
    struct d_arg_config_item_list* _items,
    struct d_arg_config_item*      _item,
    int                            _status
)
{
    const int status = (_status == D_ARG_OK)
                       ? d_arg_config_item_list_adopt(_items,
                                                      _item)
                       : _status;

    d_arg_config_item_release(_item);

    return status;
}

/*
d_arg_internal_config_marker
  An item marking a section's start or end -- `++` or `--` -- whose
sections are the first `_depth` of `_parents`, appended.
*/
D_STATIC int
d_arg_internal_config_marker(
    struct d_arg_config_item_list* _items,
    const struct d_arg_text_list*  _parents,
    size_t                         _depth,
    const char*                    _name
)
{
    struct d_arg_config_item marker = D_ARG_CONFIG_ITEM_INIT;
    int                      status = d_arg_config_item_set_name(
                                          &marker,
                                          d_arg_internal_text_of(_name));

    status = (status == D_ARG_OK)
             ? d_arg_internal_config_copy_parents(&marker,
                                                  _parents,
                                                  _depth)
             : status;

    return d_arg_internal_config_adopt(_items,
                                       &marker,
                                       status);
}

/*
d_arg_internal_config_repeat_last
  CLI11's output.push_back(output.back()), then output.back().parents.
pop_back(): the last item copied, less its innermost section.
*/
D_STATIC int
d_arg_internal_config_repeat_last(
    struct d_arg_config_item_list* _items
)
{
    const struct d_arg_config_item* const last   =
        &_items->items[_items->count - 1u];
    struct d_arg_config_item              copy   = D_ARG_CONFIG_ITEM_INIT;
    int                                   status = d_arg_config_item_set_name(
                                                       &copy,
                                                       last->name);

    status = (status == D_ARG_OK)
             ? d_arg_internal_config_copy_parents(&copy,
                                                  &last->parents,
                                                  last->parents.count - 1u)
             : status;

    // the values, as they are
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < last->inputs.count);
         ++index)
    {
        status = d_arg_config_item_add_input(&copy,
                                             last->inputs.items[index]);
    }

    copy.multiline = last->multiline;

    return d_arg_internal_config_adopt(_items,
                                       &copy,
                                       status);
}

/*
d_arg_internal_config_peel
  CLI11's loops that repeat the last item, a section fewer each time, while
it has more than `_keep` sections.
*/
D_STATIC int
d_arg_internal_config_peel(
    struct d_arg_config_item_list* _items,
    size_t                         _keep
)
{
    int status = D_ARG_OK;

    // a section fewer each time
    while ( (status == D_ARG_OK) &&
            (_items->items[_items->count - 1u].parents.count > _keep) )
    {
        status = d_arg_internal_config_repeat_last(_items);
    }

    return status;
}

/*
d_arg_internal_config_pop_last
  CLI11's output.pop_back(): the last item released and taken off.
*/
D_STATIC void
d_arg_internal_config_pop_last(
    struct d_arg_config_item_list* _items
)
{
    d_arg_config_item_release(&_items->items[_items->count - 1u]);
    _items->count--;

    return;
}

/*
d_arg_internal_config_drop_first
  An item's outermost section taken off its sections, its copy staying in
the item's storage.
*/
D_STATIC void
d_arg_internal_config_drop_first(
    struct d_arg_config_item* _item
)
{
    memmove(_item->parents.items,
            _item->parents.items + 1u,
            (_item->parents.count - 1u) * sizeof(struct d_pack_text));
    _item->parents.count--;

    return;
}

/*
d_arg_internal_config_same_parents
  Whether an item's sections are exactly those of a list, byte for byte.
*/
D_STATIC bool
d_arg_internal_config_same_parents(
    const struct d_arg_config_item* _item,
    const struct d_arg_text_list*   _parents
)
{
    // as many sections
    if (_item->parents.count != _parents->count)
    {
        return false;
    }

    // and each the same
    for (size_t index = 0u; index < _parents->count; ++index)
    {
        if (!d_arg_internal_equal(_item->parents.items[index],
                                  _parents->items[index]))
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_config_common
  How many of the first `_count` sections an item shares with a section's
path, counted from the outermost until one differs.
*/
D_STATIC size_t
d_arg_internal_config_common(
    const struct d_arg_config_item* _item,
    const struct d_arg_text_list*   _parents,
    size_t                          _count
)
{
    size_t common = 0u;

    // the shared sections, from the outermost
    while ( (common < _count) &&
            (d_arg_internal_equal(_item->parents.items[common],
                                  _parents->items[common])) )
    {
        common++;
    }

    return common;
}

/*
d_arg_internal_config_matching
  CLI11's find_matching_config: from the last item back -- only the last,
unless searching fully -- the first with an entry's sections and name; NULL
for none.
*/
D_STATIC struct d_arg_config_item*
d_arg_internal_config_matching(
    struct d_arg_config_item_list*  _items,
    const struct d_arg_config_item* _entry,
    bool                            _full_search
)
{
    // from the last item back
    for (size_t index = _items->count; index > 0u; --index)
    {
        struct d_arg_config_item* const item = &_items->items[index - 1u];

        if ( (d_arg_internal_config_same_parents(item,
                                                 &_entry->parents)) &&
             (d_arg_internal_equal(item->name,
                                   _entry->name)) )
        {
            return item;
        }

        // only the last, unless searching fully
        if (!_full_search)
        {
            break;
        }
    }

    return NULL;
}

/*
d_arg_internal_config_separated
  Whether CLI11 puts `%%` between an item's values and those merged into
it: not when either side, at the join, has an empty value or a `%%`. A
side with no values at all, past which CLI11 would read, counts as empty.
*/
D_STATIC bool
d_arg_internal_config_separated(
    const struct d_arg_config_item* _match,
    const struct d_arg_config_item* _entry
)
{
    const struct d_arg_text_list* const before = &_match->inputs;
    const struct d_arg_text_list* const after  = &_entry->inputs;

    // a side with no values, or an empty one at the join
    if ( (before->count == 0u)                             ||
         (after->count == 0u)                              ||
         (before->items[before->count - 1u].length == 0u) ||
         (after->items[0].length == 0u) )
    {
        return false;
    }

    return ( (!d_arg_internal_equal_cstr(before->items[before->count - 1u],
                                         "%%")) &&
             (!d_arg_internal_equal_cstr(after->items[0],
                                         "%%")) );
}

/*
d_arg_internal_config_merge
  CLI11's merge of an entry into the item it repeats: `%%` between the
groups -- when both hold several values, or whenever duplicates are
allowed -- making the item multiline; then the entry's values appended.
*/
D_STATIC int
d_arg_internal_config_merge(
    struct d_arg_config_item*       _match,
    const struct d_arg_config_item* _entry,
    bool                            _duplicates
)
{
    const bool several = ( (_match->inputs.count > 1u) &&
                           (_entry->inputs.count > 1u) );
    int        status  = D_ARG_OK;

    // a separator between the groups
    if ( ( (several) ||
           (_duplicates) )                                  &&
         (d_arg_internal_config_separated(_match,
                                          _entry)) )
    {
        status            = d_arg_config_item_add_input(
                                _match,
                                d_arg_internal_span("%%",
                                                    2u));
        _match->multiline = ( (status == D_ARG_OK) ||
                              (_match->multiline) );
    }

    // the values after it
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _entry->inputs.count);
         ++index)
    {
        status = d_arg_config_item_add_input(_match,
                                             _entry->inputs.items[index]);
    }

    return status;
}

//==============================================================================
// 3.  QUOTES
//==============================================================================

// d_arg_internal_config_quoting
//   struct: what CLI11's process_quoted_string is told: the string quote,
// the literal quote, and whether a result in brackets is kept as it is.
struct d_arg_internal_config_quoting
{
    char string_quote;
    char literal_quote;
    bool keep_brackets;
};

// NAME_QUOTING
//   constant: how CLI11 unquotes an item's name: `"` strings, `'` literals,
// brackets kept.
static const struct d_arg_internal_config_quoting NAME_QUOTING =
{
    '"',   // string_quote
    '\'',  // literal_quote
    true   // keep_brackets
};

/*
d_arg_internal_config_secondary
  CLI11's handle_secondary_array: a text in brackets becomes `[[` followed
by each byte after the first, twice, so that a quoted value is not read as
an array.
*/
D_STATIC int
d_arg_internal_config_secondary(
    struct d_arg_buffer* _text
)
{
    struct d_arg_buffer doubled = D_ARG_BUFFER_INIT;
    int                 status  = D_ARG_OK;

    // only a text in brackets
    if ( (_text->length < 2u)                      ||
         (_text->data[0] != '[')                   ||
         (_text->data[_text->length - 1u] != ']') )
    {
        return D_ARG_OK;
    }

    (void)d_arg_buffer_append(&doubled,
                              "[[",
                              2u);

    // each byte after the first, twice
    for (size_t index = 1u; index < _text->length; ++index)
    {
        (void)d_arg_buffer_append_char(&doubled,
                                       _text->data[index]);
        (void)d_arg_buffer_append_char(&doubled,
                                       _text->data[index]);
    }

    // the doubled text replaces the text
    if (!doubled.failed)
    {
        d_arg_buffer_clear(_text);
        status = d_arg_buffer_append_text(_text,
                                          d_arg_buffer_text(&doubled));
    }

    status = (doubled.failed)
             ? D_ARG_ERROR_OUT_OF_MEMORY
             : status;
    d_arg_buffer_release(&doubled);

    return status;
}

/*
d_arg_internal_config_unescape
  CLI11's remove_escaped_characters as the reader calls it: a malformed
escape is its ParseError, in the escape's own words.
*/
D_STATIC int
d_arg_internal_config_unescape(
    struct d_pack_text   _text,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error
)
{
    const int status = d_arg_internal_unescape(_text,
                                               _out,
                                               _error);

    return (status == D_ARG_ERROR_VALIDATION)
           ? D_ARG_ERROR_PARSE
           : status;
}

/*
d_arg_internal_config_string
  The inside of a string in string quotes: unescaped when it holds a
backslash, kept as it is otherwise.
*/
D_STATIC int
d_arg_internal_config_string(
    struct d_pack_text   _inner,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error
)
{
    return (memchr(_inner.data,
                   '\\',
                   _inner.length) != NULL)
           ? d_arg_internal_config_unescape(_inner,
                                            _out,
                                            _error)
           : d_arg_buffer_append_text(_out,
                                      _inner);
}

/*
d_arg_internal_config_unquote
  CLI11's process_quoted_string into `_out`, which it replaces: a binary
string decoded; a string in the string quotes stripped, and unescaped when
it holds a backslash; one in the literal quotes or backquotes stripped
alone; each of these then, unless brackets are kept, doubled when in
brackets. A text of one byte, and any other, is written as it is.
*/
D_STATIC int
d_arg_internal_config_unquote(
    struct d_pack_text                          _text,
    const struct d_arg_internal_config_quoting* _quoting,
    struct d_arg_buffer*                        _out,
    struct d_arg_buffer*                        _error
)
{
    d_arg_buffer_clear(_out);

    // too short to be quoted
    if (_text.length <= 1u)
    {
        return d_arg_buffer_append_text(_out,
                                        _text);
    }

    const char               front  = _text.data[0];
    const char               back   = _text.data[_text.length - 1u];
    const struct d_pack_text inner  = d_arg_internal_span(_text.data + 1u,
                                                          _text.length - 2u);
    int                      status = D_ARG_OK;

    // a binary string, a string, a literal, or the text itself
    if (d_arg_internal_is_binary_string(_text))
    {
        status = d_arg_internal_extract_binary(_text,
                                               _out);
    }
    else if ( (front == _quoting->string_quote) &&
              (back == _quoting->string_quote) )
    {
        status = d_arg_internal_config_string(inner,
                                              _out,
                                              _error);
    }
    else if ( ( (front == _quoting->literal_quote) ||
                (front == '`') )                     &&
              (back == front) )
    {
        status = d_arg_buffer_append_text(_out,
                                          inner);
    }
    else
    {
        return d_arg_buffer_append_text(_out,
                                        _text);
    }

    return ( (status == D_ARG_OK) &&
             (!_quoting->keep_brackets) )
           ? d_arg_internal_config_secondary(_out)
           : status;
}

//==============================================================================
// 4.  SECTIONS
//==============================================================================

// d_arg_internal_config_reader
//   struct: from_config's state across lines: the format; the array
// characters the read uses -- an INI format's brackets and comma, where it
// has none of its own -- and whether values split as the default format's
// or an INI format's do; the lines, the items and the error; the section
// under way and the one before it, each a span of the text or "default",
// the occurrences of a section met again in a row, and whether an item has
// entered the section asked for; and what is reused from line to line --
// the value built over lines, a buffer for whatever a step transforms, and
// the values found on a line.
struct d_arg_internal_config_reader
{
    const struct d_arg_config_format*  format;
    char                               array_start;
    char                               array_end;
    char                               array_separator;
    bool                               default_array;
    bool                               ini_array;
    struct d_arg_internal_config_lines lines;
    struct d_arg_config_item_list*     out;
    struct d_arg_buffer*               error;
    struct d_pack_text                 section;
    struct d_pack_text                 previous;
    int                                section_index;
    bool                               in_section;
    struct d_arg_buffer                value;
    struct d_arg_buffer                scratch;
    struct d_arg_text_list             raw;
};

/*
d_arg_internal_config_parent
  One part of a path, its quotes removed as CLI11's remove_quotes(vector)
removes them -- an empty part kept as it is -- added to an item's sections;
a malformed escape, which d_arg_internal_remove_quotes_text calls text that
breaks the rules, is CLI11's ParseError.
*/
D_STATIC int
d_arg_internal_config_parent(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _part,
    struct d_arg_config_item*            _item
)
{
    // an empty part, kept
    if (_part.length == 0u)
    {
        return d_arg_config_item_add_parent(_item,
                                            _part);
    }

    d_arg_buffer_clear(&_reader->scratch);

    const int status = d_arg_internal_remove_quotes_text(_part,
                                                         &_reader->scratch,
                                                         _reader->error);

    // a malformed escape is CLI11's ParseError
    if (status == D_ARG_ERROR_INVALID)
    {
        return D_ARG_ERROR_PARSE;
    }

    return (status == D_ARG_OK)
           ? d_arg_config_item_add_parent(
                 _item,
                 d_arg_buffer_text(&_reader->scratch))
           : status;
}

/*
d_arg_internal_config_parts
  The parts of a dotted path, cut as CLI11's split_up cuts them, added to
an item's sections: all of them, or, given `_last`, all but the last, which
`_last` receives.
*/
D_STATIC int
d_arg_internal_config_parts(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _path,
    struct d_arg_config_item*            _item,
    struct d_pack_text*                  _last
)
{
    struct d_pack_text rest    = d_arg_internal_trim(_path);
    struct d_pack_text pending = d_arg_internal_span(rest.data,
                                                     0u);
    bool               held    = false;
    int                status  = D_ARG_OK;

    // a part at a time, each added once the next is found
    while ( (status == D_ARG_OK) &&
            (rest.length > 0u) )
    {
        struct d_pack_text part = d_arg_internal_span(rest.data,
                                                      0u);

        status  = d_arg_internal_split_step(
                      &rest,
                      _reader->format->parent_separator,
                      &part);
        status  = ( (status == D_ARG_OK) &&
                    (held) )
                  ? d_arg_internal_config_parent(_reader,
                                                 pending,
                                                 _item)
                  : status;
        pending = part;
        held    = true;
    }

    // the last part, the name's
    if ( (status == D_ARG_OK) &&
         (held)               &&
         (_last != NULL) )
    {
        *_last = pending;

        return D_ARG_OK;
    }

    // or a section of its own
    return ( (status == D_ARG_OK) &&
             (held) )
           ? d_arg_internal_config_parent(_reader,
                                          pending,
                                          _item)
           : status;
}

/*
d_arg_internal_config_parents
  CLI11's generate_parents: the parts of the section under way, unless it
is the default one, then, given a name, its dotted parts but the last,
which becomes the name -- each part added to the item's sections without
its quotes.
*/
D_STATIC int
d_arg_internal_config_parents(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text*                  _name,
    struct d_arg_config_item*            _item
)
{
    const char separator = _reader->format->parent_separator;
    int        status    = D_ARG_OK;

    // the section's parts, but the default's
    if (!d_arg_internal_equal_nocase_cstr(_reader->section,
                                          DEFAULT_SECTION))
    {
        status = (d_arg_internal_config_find(_reader->section,
                                             separator,
                                             0u) != NPOS)
                 ? d_arg_internal_config_parts(_reader,
                                               _reader->section,
                                               _item,
                                               NULL)
                 : d_arg_internal_config_parent(_reader,
                                                _reader->section,
                                                _item);
    }

    // the name's parts before its last
    if ( (status == D_ARG_OK)                       &&
         (_name != NULL)                            &&
         (d_arg_internal_config_find(*_name,
                                     separator,
                                     0u) != NPOS) )
    {
        status = d_arg_internal_config_parts(_reader,
                                             *_name,
                                             _item,
                                             _name);
    }

    return status;
}

/*
d_arg_internal_config_openers
  CLI11's `++` items for the sections enclosing the one opened, from the
`_first`-th: one for each depth short of the section's own.
*/
D_STATIC int
d_arg_internal_config_openers(
    struct d_arg_config_item_list* _items,
    const struct d_arg_text_list*  _parents,
    size_t                         _first
)
{
    int status = D_ARG_OK;

    // one for each enclosing depth
    for (size_t depth = _first;
         (status == D_ARG_OK) && ((depth + 1u) < _parents->count);
         ++depth)
    {
        status = d_arg_internal_config_marker(_items,
                                              _parents,
                                              depth + 1u,
                                              "++");
    }

    return status;
}

/*
d_arg_internal_config_reopen
  CLI11's checkParentSegments after a section's end: the end repeated a
level up while it is as deep as the section opened -- or, for one at the
top, deeper than one level; then, for a nested section, the end dropped
when it lies along the new section's path, or else repeated up to the
sections they share, and the enclosing sections not shared opened.
*/
D_STATIC int
d_arg_internal_config_reopen(
    struct d_arg_config_item_list* _items,
    const struct d_arg_text_list*  _parents
)
{
    const size_t depth  = (_parents->count > 1u) ? _parents->count : 2u;
    int          status = d_arg_internal_config_peel(_items,
                                                     depth - 1u);

    // a section at the top opens nothing more
    if ( (status != D_ARG_OK) ||
         (_parents->count <= 1u) )
    {
        return status;
    }

    const struct d_arg_config_item* const last   =
        &_items->items[_items->count - 1u];
    const size_t                          pair   =
        (last->parents.count < (_parents->count - 1u))
        ? last->parents.count
        : (_parents->count - 1u);
    const size_t                          common =
        d_arg_internal_config_common(last,
                                     _parents,
                                     pair);

    // the end dropped, or repeated up to the shared sections
    if (common == pair)
    {
        d_arg_internal_config_pop_last(_items);
    }
    else
    {
        status = d_arg_internal_config_peel(_items,
                                            common + 1u);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_config_openers(_items,
                                           _parents,
                                           common)
           : status;
}

/*
d_arg_internal_config_open
  CLI11's checkParentSegments: the section under way opened -- after the
end of the one before, as d_arg_internal_config_reopen has it, or with its
enclosing sections opened first -- and then its own `++` item.
*/
D_STATIC int
d_arg_internal_config_open(
    struct d_arg_internal_config_reader* _reader
)
{
    struct d_arg_config_item_list* const items  = _reader->out;
    struct d_arg_config_item             opened = D_ARG_CONFIG_ITEM_INIT;
    int                                  status =
        d_arg_internal_config_parents(_reader,
                                      NULL,
                                      &opened);

    // after a section's end, or not
    if (status == D_ARG_OK)
    {
        status = ( (items->count > 0u)                         &&
                   (d_arg_internal_equal_cstr(
                        items->items[items->count - 1u].name,
                        "--")) )
                 ? d_arg_internal_config_reopen(items,
                                                &opened.parents)
                 : d_arg_internal_config_openers(items,
                                                 &opened.parents,
                                                 0u);
    }

    status = (status == D_ARG_OK)
             ? d_arg_config_item_set_name(&opened,
                                          d_arg_internal_span("++",
                                                              2u))
             : status;

    return d_arg_internal_config_adopt(items,
                                       &opened,
                                       status);
}

/*
d_arg_internal_config_close
  The `--` item ending the section under way, its sections the section's
parts.
*/
D_STATIC int
d_arg_internal_config_close(
    struct d_arg_internal_config_reader* _reader
)
{
    struct d_arg_config_item closed = D_ARG_CONFIG_ITEM_INIT;
    int                      status = d_arg_internal_config_parents(_reader,
                                                                    NULL,
                                                                    &closed);

    status = (status == D_ARG_OK)
             ? d_arg_config_item_set_name(&closed,
                                          d_arg_internal_span("--",
                                                              2u))
             : status;

    return d_arg_internal_config_adopt(_reader->out,
                                       &closed,
                                       status);
}

/*
d_arg_internal_config_count
  CLI11's currentSectionIndex: one more for a header naming the section the
header before named, 0 for any other, which becomes the section before.
*/
D_STATIC void
d_arg_internal_config_count(
    struct d_arg_internal_config_reader* _reader
)
{
    // the same section again
    if (d_arg_internal_equal(_reader->section,
                             _reader->previous))
    {
        _reader->section_index += (_reader->section_index < INT_MAX) ? 1 : 0;

        return;
    }

    _reader->section_index = 0;
    _reader->previous      = _reader->section;

    return;
}

/*
d_arg_internal_config_header
  A `[section]` line: the section under way ended, unless it is the default
one; the new one named -- without a second pair of brackets, and "default"
in any case being the default -- and opened; the section entered by no item
yet, and its occurrence counted.
*/
D_STATIC int
d_arg_internal_config_header(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _line
)
{
    struct d_pack_text section = d_arg_internal_span(_line.data + 1u,
                                                     _line.length - 2u);
    int                status  =
        (!d_arg_internal_equal_cstr(_reader->section,
                                    DEFAULT_SECTION))
        ? d_arg_internal_config_close(_reader)
        : D_ARG_OK;

    // a second pair of brackets, TOML's
    if ( (section.length > 1u)                      &&
         (section.data[0] == '[')                   &&
         (section.data[section.length - 1u] == ']') )
    {
        section = d_arg_internal_span(section.data + 1u,
                                      section.length - 2u);
    }

    // the default section, or another, opened
    if (d_arg_internal_equal_nocase_cstr(section,
                                         DEFAULT_SECTION))
    {
        _reader->section = d_arg_internal_span(DEFAULT_SECTION,
                                               sizeof DEFAULT_SECTION - 1u);
    }
    else
    {
        _reader->section = section;
        status           = (status == D_ARG_OK)
                           ? d_arg_internal_config_open(_reader)
                           : status;
    }

    _reader->in_section = false;
    d_arg_internal_config_count(_reader);

    return status;
}

/*
d_arg_internal_config_ml_comment
  A line opening with `"""` or `'''`: a comment, closed on the line itself
when it is long enough to hold both ends and ends with the same quotes,
else by the first later line that, trimmed, does -- or by the end of the
text.
*/
D_STATIC void
d_arg_internal_config_ml_comment(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _line
)
{
    const char         quote = _line.data[0];
    struct d_pack_text next  = d_arg_internal_span(_line.data,
                                                   0u);

    // closed on its own line
    if ( (_line.length >= 6u) &&
         (d_arg_internal_config_ml_string(_line,
                                          quote)) )
    {
        return;
    }

    // the lines up to the closing one
    while (d_arg_internal_config_getline(&_reader->lines,
                                         &next))
    {
        if (d_arg_internal_config_ml_string(d_arg_internal_trim(next),
                                            quote))
        {
            break;
        }
    }

    return;
}

/*
d_arg_internal_config_header_comment
  CLI11's cut of a comment after a section header, `[section] # note`: a
line that opens with `[`, does not end with `]` and holds the comment
character is cut at the first comment character outside quotes, if any,
and trimmed.
*/
D_STATIC void
d_arg_internal_config_header_comment(
    const struct d_arg_internal_config_reader* _reader,
    struct d_pack_text*                        _line
)
{
    const char comment = _reader->format->comment;
    size_t     at      = 0u;

    // not a header with a comment after it
    if ( (_line->data[0] != '[')                   ||
         (_line->data[_line->length - 1u] == ']')  ||
         (d_arg_internal_config_find(*_line,
                                     comment,
                                     0u) == NPOS) )
    {
        return;
    }

    // to the first comment character outside quotes
    while (at < _line->length)
    {
        const char c = _line->data[at];

        if (d_arg_internal_config_quote(c))
        {
            // a quote's closing never allocates, so it cannot fail
            (void)d_arg_internal_close_sequence(*_line,
                                                at,
                                                c,
                                                &at);
            at++;
        }
        else if (c == comment)
        {
            break;
        }
        else
        {
            at++;
        }
    }

    // a comment found: the line before it
    if (at < _line->length)
    {
        *_line = d_arg_internal_trim(d_arg_internal_config_head(*_line,
                                                                at));
    }

    return;
}

//==============================================================================
// 5.  VALUES
//==============================================================================

// d_arg_internal_config_entry
//   struct: a name-value line as from_config splits it: the line as read and
// as trimmed, the name, the values as found -- spans of the line, of the
// reader's value, or "true" -- and whether the value was a multiline
// string, which is final as read.
struct d_arg_internal_config_entry
{
    struct d_pack_text      buffer;
    struct d_pack_text      line;
    struct d_pack_text      name;
    struct d_arg_text_list* raw;
    bool                    mlquote;
};

// d_arg_internal_config_ml
//   struct: a multiline value under way: its quote, whether it is still
// open, whether the line before ended in a backslash that joins the next,
// and whether no line after the first has been added.
struct d_arg_internal_config_ml
{
    char quote;
    bool open;
    bool extension;
    bool first;
};

/*
d_arg_internal_config_search_start
  CLI11's search_start for a line holding quotes: past quoted runs, blanks
and section separators to just before the first value delimiter or comment
character -- wrapping round, as CLI11's size_t does, when that is the first
character -- or past the end when neither follows. 0 for a line without
quotes.
*/
D_STATIC size_t
d_arg_internal_config_search_start(
    const struct d_arg_config_format* _format,
    struct d_pack_text                _line
)
{
    const char set[3] = { _format->parent_separator,
                          _format->comment,
                          _format->value_delimiter };
    size_t     at     = 0u;

    // nothing to pass over without quotes
    if (!d_arg_internal_config_has_quote(_line))
    {
        return 0u;
    }

    // a run, a blank or a word at a time
    while (at < _line.length)
    {
        const char c = _line.data[at];

        if (d_arg_internal_config_quote(c))
        {
            // a quote's closing never allocates, so it cannot fail
            (void)d_arg_internal_close_sequence(_line,
                                                at,
                                                c,
                                                &at);
            at++;
        }
        else if ( (c == _format->value_delimiter) ||
                  (c == _format->comment) )
        {
            return at - 1u;
        }
        else if ( (c == ' ')                       ||
                  (c == '\t')                      ||
                  (c == _format->parent_separator) )
        {
            at++;
        }
        else
        {
            at = d_arg_internal_config_find_any(_line,
                                                set,
                                                at);
        }
    }

    return at;
}

/*
d_arg_internal_config_uncomment
  CLI11's cut of a comment off a value: the first piece split_up makes of it
at the comment character, trimmed; where CLI11 would read a piece that is
not there, the value is empty.
*/
D_STATIC int
d_arg_internal_config_uncomment(
    char                _comment,
    struct d_pack_text* _value
)
{
    struct d_pack_text rest   = d_arg_internal_trim(*_value);
    struct d_pack_text first  = d_arg_internal_span(rest.data,
                                                    0u);
    int                status = D_ARG_OK;

    // the first piece, if any
    if (rest.length > 0u)
    {
        status = d_arg_internal_split_step(&rest,
                                           _comment,
                                           &first);
    }

    *_value = d_arg_internal_trim(first);

    return status;
}

/*
d_arg_internal_config_split
  CLI11's split_up of a value into the line's values, each a span of it.
*/
D_STATIC int
d_arg_internal_config_split(
    struct d_pack_text      _value,
    char                    _delimiter,
    struct d_arg_text_list* _raw
)
{
    struct d_pack_text rest   = d_arg_internal_trim(_value);
    int                status = D_ARG_OK;

    // a piece at a time
    while ( (status == D_ARG_OK) &&
            (rest.length > 0u) )
    {
        struct d_pack_text piece = d_arg_internal_span(rest.data,
                                                       0u);

        status = d_arg_internal_split_step(&rest,
                                           _delimiter,
                                           &piece);
        status = (status == D_ARG_OK)
                 ? d_arg_internal_text_push(_raw,
                                            piece)
                 : status;
    }

    return status;
}

/*
d_arg_internal_config_array
  CLI11's array value: from its opening character, later lines added,
trimmed, until it ends with the closing one or the text ends; then split at
the separator, inside the brackets -- or after the opening one, when it
never closed.
*/
D_STATIC int
d_arg_internal_config_array(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _value,
    struct d_arg_text_list*              _raw
)
{
    struct d_arg_buffer* const value = &_reader->value;
    struct d_pack_text         next  = d_arg_internal_span(_value.data,
                                                           0u);

    d_arg_buffer_clear(value);
    (void)d_arg_buffer_append_text(value,
                                   _value);

    // lines added until it closes
    while ( (!value->failed)                                         &&
            (value->data[value->length - 1u] != _reader->array_end)  &&
            (d_arg_internal_config_getline(&_reader->lines,
                                           &next)) )
    {
        (void)d_arg_buffer_append_text(value,
                                       d_arg_internal_trim(next));
    }

    // check if memory allocation was successful
    if (value->failed)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const bool closed = (value->data[value->length - 1u] == _reader->array_end);

    return d_arg_internal_config_split(
               (closed)
               ? d_arg_internal_span(value->data + 1u,
                                     (value->length >= 2u)
                                     ? (value->length - 2u)
                                     : 0u)
               : d_arg_internal_config_tail(d_arg_buffer_text(value),
                                            1u),
               _reader->array_separator,
               _raw);
}

/*
d_arg_internal_config_values
  The values of a value that is not a multiline string: an array, when it
opens with the array's opening character; else, for the default or an INI
format, a list split at the separator when it holds one, or at whitespace
when it holds a space; else the value itself.
*/
D_STATIC int
d_arg_internal_config_values(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _value,
    struct d_arg_text_list*              _raw
)
{
    const bool lists = ( (_reader->default_array) ||
                         (_reader->ini_array) );

    // an array
    if ( (_value.length > 0u) &&
         (_value.data[0] == _reader->array_start) )
    {
        return d_arg_internal_config_array(_reader,
                                           _value,
                                           _raw);
    }

    // a list, split at the separator
    if ( (lists)                                         &&
         (d_arg_internal_config_find(_value,
                                     _reader->array_separator,
                                     0u) != NPOS) )
    {
        return d_arg_internal_config_split(_value,
                                           _reader->array_separator,
                                           _raw);
    }

    // a list, split at whitespace
    if ( (lists)                            &&
         (d_arg_internal_config_find(_value,
                                     ' ',
                                     0u) != NPOS) )
    {
        return d_arg_internal_config_split(_value,
                                           '\0',
                                           _raw);
    }

    return d_arg_internal_text_push(_raw,
                                    _value);
}

/*
d_arg_internal_config_replace
  The reader's value replaced by what a step wrote into its scratch buffer.
*/
D_STATIC int
d_arg_internal_config_replace(
    struct d_arg_internal_config_reader* _reader
)
{
    d_arg_buffer_clear(&_reader->value);

    return d_arg_buffer_append_text(&_reader->value,
                                    d_arg_buffer_text(&_reader->scratch));
}

/*
d_arg_internal_config_ml_unescape
  A `"""` value's escapes resolved once it closes: CLI11's
remove_escaped_characters over the whole value, a malformed escape being
its ParseError.
*/
D_STATIC int
d_arg_internal_config_ml_unescape(
    struct d_arg_internal_config_reader* _reader
)
{
    d_arg_buffer_clear(&_reader->scratch);

    const int status = d_arg_internal_config_unescape(
                           d_arg_buffer_text(&_reader->value),
                           &_reader->scratch,
                           _reader->error);

    return (status == D_ARG_OK)
           ? d_arg_internal_config_replace(_reader)
           : status;
}

/*
d_arg_internal_config_ml_binary
  A multiline value as CLI11 leaves it once read: refused when it could not
be stored, decoded when it is a binary string.
*/
D_STATIC int
d_arg_internal_config_ml_binary(
    struct d_arg_internal_config_reader* _reader
)
{
    // check if memory allocation was successful
    if (_reader->value.failed)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // not a binary string
    if (!d_arg_internal_is_binary_string(d_arg_buffer_text(&_reader->value)))
    {
        return D_ARG_OK;
    }

    d_arg_buffer_clear(&_reader->scratch);

    const int status = d_arg_internal_extract_binary(
                           d_arg_buffer_text(&_reader->value),
                           &_reader->scratch);

    return (status == D_ARG_OK)
           ? d_arg_internal_config_replace(_reader)
           : status;
}

/*
d_arg_internal_config_ml_first
  The first line of a multiline value, from after its opening quotes:
closed on the line, its content before the closing quotes, unescaped for
`"""`; or left open, a backslash ending a `"""` value's line dropped to
join the next.
*/
D_STATIC int
d_arg_internal_config_ml_first(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_internal_config_ml*     _ml,
    struct d_pack_text                   _text
)
{
    const size_t       close = d_arg_internal_config_ml_close(
                                   _text,
                                   _ml->quote,
                                   _reader->format->comment);
    struct d_pack_text text  = _text;

    d_arg_buffer_clear(&_reader->value);

    // closed on its first line
    if (close != NPOS)
    {
        _ml->open = false;
        (void)d_arg_buffer_append_text(&_reader->value,
                                       d_arg_internal_config_head(_text,
                                                                  close));

        return (_ml->quote == '"')
               ? d_arg_internal_config_ml_unescape(_reader)
               : D_ARG_OK;
    }

    _ml->extension = ( (text.length > 0u)                     &&
                       (text.data[text.length - 1u] == '\\')  &&
                       (_ml->quote == '"') );

    // a joining backslash dropped
    if (_ml->extension)
    {
        text.length--;
    }

    (void)d_arg_buffer_append_text(&_reader->value,
                                   text);

    return D_ARG_OK;
}

/*
d_arg_internal_config_ml_more
  A line inside a multiline value, joined to it: trimmed after a line that
ended in a backslash, else after a newline -- unless the value is still
empty on the first line after its own -- a backslash ending it, in a `"""`
value, dropped to join the next line.
*/
D_STATIC void
d_arg_internal_config_ml_more(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_internal_config_ml*     _ml,
    struct d_pack_text                   _line
)
{
    struct d_arg_buffer* const value = &_reader->value;
    struct d_pack_text         text  = _line;

    // joined to what came before
    if (_ml->extension)
    {
        text = d_arg_internal_trim(text);
    }
    else if ( (!_ml->first) ||
              (value->length > 0u) )
    {
        (void)d_arg_buffer_append_char(value,
                                       '\n');
    }

    _ml->extension = ( (text.length > 0u)                     &&
                       (text.data[text.length - 1u] == '\\')  &&
                       (_ml->quote == '"') );
    _ml->first     = false;

    // a joining backslash dropped
    if (_ml->extension)
    {
        text.length--;
    }

    (void)d_arg_buffer_append_text(value,
                                   text);

    return;
}

/*
d_arg_internal_config_ml_last
  The line that closes a multiline value: its content before the closing
quotes joined as d_arg_internal_config_ml_more joins a line -- but with
only its leading whitespace dropped after a backslash -- a newline ending
the value dropped, and the value unescaped for `"""`.
*/
D_STATIC int
d_arg_internal_config_ml_last(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_internal_config_ml*     _ml,
    struct d_pack_text                   _line,
    size_t                               _close
)
{
    struct d_arg_buffer* const value = &_reader->value;
    struct d_pack_text         text  = d_arg_internal_config_head(_line,
                                                                  _close);

    // joined to what came before
    if (_ml->extension)
    {
        text = d_arg_internal_config_ltrim(text);
    }
    else if ( (!_ml->first) ||
              (value->length > 0u) )
    {
        (void)d_arg_buffer_append_char(value,
                                       '\n');
    }

    _ml->first = false;
    _ml->open  = false;
    (void)d_arg_buffer_append_text(value,
                                   text);

    // a newline that ends the value dropped
    if ( (value->length > 0u) &&
         (value->data[value->length - 1u] == '\n') )
    {
        d_arg_buffer_truncate(value,
                              value->length - 1u);
    }

    return (_ml->quote == '"')
           ? d_arg_internal_config_ml_unescape(_reader)
           : D_ARG_OK;
}

/*
d_arg_internal_config_ml_value
  CLI11's multiline string value, `'''` or `"""`: taken from the line as
read, past its leading spaces and tabs and the delimiter -- whose position
is the trimmed line's, so that other leading whitespace shifts it -- its
own leading whitespace and three characters dropped; lines then added until
one closes it or the text ends; a binary string decoded.
*/
D_STATIC int
d_arg_internal_config_ml_value(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_internal_config_entry*  _entry,
    size_t                               _delimiter,
    char                                 _quote
)
{
    struct d_arg_internal_config_ml ml     = { _quote, true, false, true };
    const size_t                    blank  = d_arg_internal_config_unblank(
                                                 _entry->buffer);
    const size_t                    offset = (blank != NPOS) ? blank : 0u;
    const struct d_pack_text        first  = d_arg_internal_config_tail(
        d_arg_internal_config_ltrim(
            d_arg_internal_config_tail(_entry->buffer,
                                       offset + _delimiter + 1u)),
        3u);
    struct d_pack_text              line   = d_arg_internal_span(first.data,
                                                                 0u);
    int                             status = d_arg_internal_config_ml_first(
                                                 _reader,
                                                 &ml,
                                                 first);

    // the lines up to the closing one
    while ( (status == D_ARG_OK)                        &&
            (ml.open)                                   &&
            (d_arg_internal_config_getline(&_reader->lines,
                                           &line)) )
    {
        const size_t close = d_arg_internal_config_ml_close(
                                 line,
                                 _quote,
                                 _reader->format->comment);

        if (close != NPOS)
        {
            status = d_arg_internal_config_ml_last(_reader,
                                                   &ml,
                                                   line,
                                                   close);
        }
        else
        {
            d_arg_internal_config_ml_more(_reader,
                                          &ml,
                                          line);
        }
    }

    status          = (status == D_ARG_OK)
                      ? d_arg_internal_config_ml_binary(_reader)
                      : status;
    _entry->mlquote = true;

    return (status == D_ARG_OK)
           ? d_arg_internal_text_push(_entry->raw,
                                      d_arg_buffer_text(&_reader->value))
           : status;
}

/*
d_arg_internal_config_name_value
  The two sides of a name-value line's delimiter, trimmed: the name before
it, and after it the value -- a multiline string, or, cut at a comment when
the line holds one, an array, a list or a single value.
*/
D_STATIC int
d_arg_internal_config_name_value(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_internal_config_entry*  _entry,
    size_t                               _delimiter,
    size_t                               _comment
)
{
    struct d_pack_text value  = d_arg_internal_trim(
                                    d_arg_internal_config_tail(
                                        _entry->line,
                                        _delimiter + 1u));
    int                status = D_ARG_OK;

    _entry->name = d_arg_internal_trim(
                       d_arg_internal_config_head(_entry->line,
                                                  _delimiter));

    // a multiline string
    if (d_arg_internal_config_ml_opens(value))
    {
        return d_arg_internal_config_ml_value(_reader,
                                              _entry,
                                              _delimiter,
                                              value.data[0]);
    }

    // a comment cut off
    if (_comment != NPOS)
    {
        status = d_arg_internal_config_uncomment(_reader->format->comment,
                                                 &value);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_config_values(_reader,
                                          value,
                                          _entry->raw)
           : status;
}

/*
d_arg_internal_config_split_line
  CLI11's split of a name-value line: at the first value delimiter after the
search start -- none when a comment character comes first -- or, without
one, a name alone, before any comment, being `true`.
*/
D_STATIC int
d_arg_internal_config_split_line(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_internal_config_entry*  _entry
)
{
    const struct d_arg_config_format* const format  = _reader->format;
    const size_t                            start   =
        d_arg_internal_config_search_start(format,
                                           _entry->line);
    const size_t                            comment =
        d_arg_internal_config_find(_entry->line,
                                   format->comment,
                                   start);
    const size_t                            found   =
        d_arg_internal_config_find(_entry->line,
                                   format->value_delimiter,
                                   start + 1u);

    // a name and a value
    if ( (found != NPOS) &&
         (comment >= found) )
    {
        return d_arg_internal_config_name_value(_reader,
                                                _entry,
                                                found,
                                                comment);
    }

    _entry->name = d_arg_internal_trim(
                       d_arg_internal_config_head(_entry->line,
                                                  comment));

    return d_arg_internal_text_push(_entry->raw,
                                    d_arg_internal_span("true",
                                                        4u));
}

//==============================================================================
// 6.  ENTRIES
//==============================================================================

/*
d_arg_internal_config_name
  CLI11's process_quoted_string of an item's name -- `"` strings and `'`
literals, brackets kept -- naming the item.
*/
D_STATIC int
d_arg_internal_config_name(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _name,
    struct d_arg_config_item*            _item
)
{
    const int status = d_arg_internal_config_unquote(_name,
                                                     &NAME_QUOTING,
                                                     &_reader->scratch,
                                                     _reader->error);

    return (status == D_ARG_OK)
           ? d_arg_config_item_set_name(_item,
                                        d_arg_buffer_text(&_reader->scratch))
           : status;
}

/*
d_arg_internal_config_inputs
  An item's values: a multiline string's as read; any other's each through
CLI11's process_quoted_string with the format's quotes, a result in
brackets doubled.
*/
D_STATIC int
d_arg_internal_config_inputs(
    struct d_arg_internal_config_reader*      _reader,
    const struct d_arg_internal_config_entry* _entry,
    struct d_arg_config_item*                 _item
)
{
    const struct d_arg_internal_config_quoting quoting =
    {
        _reader->format->string_quote,   // string_quote
        _reader->format->literal_quote,  // literal_quote
        false                            // keep_brackets
    };
    int                                        status  = D_ARG_OK;

    // each value in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _entry->raw->count);
         ++index)
    {
        // a multiline string's value is final
        if (_entry->mlquote)
        {
            status = d_arg_config_item_add_input(_item,
                                                 _entry->raw->items[index]);

            continue;
        }

        status = d_arg_internal_config_unquote(_entry->raw->items[index],
                                               &quoting,
                                               &_reader->scratch,
                                               _reader->error);
        status = (status == D_ARG_OK)
                 ? d_arg_config_item_add_input(
                       _item,
                       d_arg_buffer_text(&_reader->scratch))
                 : status;
    }

    return status;
}

/*
d_arg_internal_config_enter
  CLI11's section filter: with a section asked for and not yet entered, an
item belongs only when its outermost section is that one -- in the
occurrence asked for, when one is -- and it then loses that section, and
the section is entered, so that later items until the next header pass
whatever their sections.
*/
D_STATIC bool
d_arg_internal_config_enter(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_config_item*            _item
)
{
    const struct d_arg_config_format* const format = _reader->format;

    // no section asked for, or one entered
    if ( (!format->section)            ||
         (format->section[0] == '\0')  ||
         (_reader->in_section) )
    {
        return true;
    }

    // another section, or another occurrence of it
    if ( (_item->parents.count == 0u)                        ||
         (!d_arg_internal_equal_cstr(_item->parents.items[0],
                                     format->section))       ||
         ( (format->index >= 0) &&
           (_reader->section_index != format->index) ) )
    {
        return false;
    }

    d_arg_internal_config_drop_first(_item);
    _reader->in_section = true;

    return true;
}

/*
d_arg_internal_config_place
  Where CLI11 puts an item read: nowhere when nested deeper than the
format's layers, or outside the section asked for; else merged into the
item it repeats -- the last, or any, when duplicates are allowed -- or
added.
*/
D_STATIC int
d_arg_internal_config_place(
    struct d_arg_internal_config_reader* _reader,
    struct d_arg_config_item*            _item
)
{
    const struct d_arg_config_format* const format     = _reader->format;
    const bool                              duplicates =
        ((format->flags & D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS) != 0u);
    const size_t                            layers     =
        (format->max_layers > 0) ? (size_t)format->max_layers : 0u;

    // nested too deeply, or outside the section asked for
    if ( (_item->parents.count > layers)          ||
         (!d_arg_internal_config_enter(_reader,
                                       _item)) )
    {
        return D_ARG_OK;
    }

    struct d_arg_config_item* const match = d_arg_internal_config_matching(
                                                _reader->out,
                                                _item,
                                                duplicates);

    return (match != NULL)
           ? d_arg_internal_config_merge(match,
                                         _item,
                                         duplicates)
           : d_arg_config_item_list_adopt(_reader->out,
                                          _item);
}

/*
d_arg_internal_config_entry
  A name-value line: split; its sections found and its name and values
unquoted, in CLI11's order, which decides which malformed escape is
reported; then placed.
*/
D_STATIC int
d_arg_internal_config_entry(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _buffer,
    struct d_pack_text                   _line
)
{
    struct d_arg_internal_config_entry entry =
    {
        _buffer,               // buffer
        _line,                 // line
        { _line.data, 0u },    // name
        &_reader->raw,         // raw
        false                  // mlquote
    };
    struct d_arg_config_item           item  = D_ARG_CONFIG_ITEM_INIT;

    _reader->raw.count = 0u;

    int status = d_arg_internal_config_split_line(_reader,
                                                  &entry);

    status = (status == D_ARG_OK)
             ? d_arg_internal_config_parents(_reader,
                                             &entry.name,
                                             &item)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_config_name(_reader,
                                          entry.name,
                                          &item)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_config_inputs(_reader,
                                            &entry,
                                            &item)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_config_place(_reader,
                                           &item)
             : status;
    d_arg_config_item_release(&item);

    return status;
}

/*
d_arg_internal_config_line
  One line as from_config takes it: trimmed; skipped when shorter than
three characters; a multiline comment; a section header, a comment after
it cut first; a comment line, skipped; else a name and its value.
*/
D_STATIC int
d_arg_internal_config_line(
    struct d_arg_internal_config_reader* _reader,
    struct d_pack_text                   _buffer
)
{
    const char         comment = _reader->format->comment;
    struct d_pack_text line    = d_arg_internal_trim(_buffer);

    // too short to mean anything
    if (line.length < 3u)
    {
        return D_ARG_OK;
    }

    // a multiline comment
    if (d_arg_internal_config_ml_opens(line))
    {
        d_arg_internal_config_ml_comment(_reader,
                                         line);

        return D_ARG_OK;
    }

    d_arg_internal_config_header_comment(_reader,
                                         &line);

    // too short once a comment is cut
    if (line.length < 3u)
    {
        return D_ARG_OK;
    }

    // a section header
    if ( (line.data[0] == '[') &&
         (line.data[line.length - 1u] == ']') )
    {
        return d_arg_internal_config_header(_reader,
                                            line);
    }

    // a comment line
    if ( (line.data[0] == ';')     ||
         (line.data[0] == '#')     ||
         (line.data[0] == comment) )
    {
        return D_ARG_OK;
    }

    return d_arg_internal_config_entry(_reader,
                                       _buffer,
                                       line);
}

/*
d_arg_internal_config_finish
  The end of the text: the section under way, unless it is the default one,
ended with a `--` item, repeated a level up until it has one section.
*/
D_STATIC int
d_arg_internal_config_finish(
    struct d_arg_internal_config_reader* _reader
)
{
    // the default section needs no end
    if (d_arg_internal_equal_cstr(_reader->section,
                                  DEFAULT_SECTION))
    {
        return D_ARG_OK;
    }

    const int status = d_arg_internal_config_close(_reader);

    return (status == D_ARG_OK)
           ? d_arg_internal_config_peel(_reader->out,
                                        1u)
           : status;
}

/*
d_arg_internal_config_start
  A reader at the start of the text: the array characters as CLI11's
from_config derives them -- an INI format, whose start and end are alike
and blank, reading brackets, and a comma for a space -- with the default
section under way, and the section before it the same.
*/
D_STATIC struct d_arg_internal_config_reader
d_arg_internal_config_start(
    const struct d_arg_config_format* _format,
    struct d_pack_text                _input,
    struct d_arg_config_item_list*    _out,
    struct d_arg_buffer*              _error
)
{
    const bool                          ini    =
        ( ( (_format->array_start == '\0') ||
            (_format->array_start == ' ') )                &&
          (_format->array_start == _format->array_end) );
    const struct d_pack_text            top    =
        d_arg_internal_span(DEFAULT_SECTION,
                            sizeof DEFAULT_SECTION - 1u);
    struct d_arg_internal_config_reader reader =
    {
        _format,                                         // format
        (ini) ? '[' : _format->array_start,              // array_start
        (ini) ? ']' : _format->array_end,                // array_end
        ( (ini) &&
          (_format->array_separator == ' ') )
        ? ','
        : _format->array_separator,                      // array_separator
        ( (_format->array_start == '[')     &&
          (_format->array_end == ']')       &&
          (_format->array_separator == ',') ),           // default_array
        ini,                                             // ini_array
        { _input, 0u },                                  // lines
        _out,                                            // out
        _error,                                          // error
        top,                                             // section
        top,                                             // previous
        0,                                               // section_index
        false,                                           // in_section
        D_ARG_BUFFER_INIT,                               // value
        D_ARG_BUFFER_INIT,                               // scratch
        D_ARG_LIST_INIT                                  // raw
    };

    return reader;
}

/*
d_arg_config_from_text_default
  CLI11's ConfigBase::from_config: every line read in turn, then the section
under way ended.
*/
int
d_arg_config_from_text_default(
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

    struct d_arg_internal_config_reader reader =
        d_arg_internal_config_start(_format,
                                    (_input.data != NULL)
                                    ? _input
                                    : d_arg_internal_span("",
                                                          0u),
                                    _out,
                                    _error);
    struct d_pack_text                  line   = d_arg_internal_span("",
                                                                     0u);
    int                                 status = D_ARG_OK;

    // each line in turn
    while ( (status == D_ARG_OK)                         &&
            (d_arg_internal_config_getline(&reader.lines,
                                           &line)) )
    {
        status = d_arg_internal_config_line(&reader,
                                            line);
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_config_finish(&reader)
             : status;
    d_arg_buffer_release(&reader.value);
    d_arg_buffer_release(&reader.scratch);
    d_arg_text_list_release(&reader.raw);

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
