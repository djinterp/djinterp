/*******************************************************************************
* djinterp [tools]                                                   dgit_text.c
*
* Definitions for the declarations in dgit_text.h.
*   Each kind is read as git reads it, by a reader of its own over the same
* lines: an ignore file a line to a pattern, with git's trimming of trailing
* spaces; an attribute file a line to a pattern and its states, white space
* before either skipped, a quoted pattern unquoted; a configuration byte by
* byte, as git's parser walks it, since an entry may follow a section's
* header on its line and a value continue onto the next. Every line is then
* told what it is.
*   Matching is wildmatch written again from what git's does -- the same
* answers, the same early abandoning of a search that cannot succeed -- and
* the lookups walk the scopes as git walks its stacks.
*
* path:      /src/djinterp/tools/dawk/ext/git/dgit_text.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/git/dgit_text.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, qsort
#include <string.h>  // memchr, memcmp, memcpy, memset, strlen
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, uint64_t,
                                                     // INT64_MAX


// D_INTERNAL_BOM
//   constant: UTF-8's byte-order mark, which git reads past.
#define D_INTERNAL_BOM "\xEF\xBB\xBF"

// D_INTERNAL_MACRO
//   constant: what begins an attribute file's macro.
#define D_INTERNAL_MACRO "[attr]"

// D_INTERNAL_LONGEST
//   constant: the length of an attribute line git reads no longer than:
//   one of this many bytes or more it ignores.
#define D_INTERNAL_LONGEST 2048u

// D_INTERNAL_ABORT_ALL, D_INTERNAL_ABORT_STARSTAR
//   constant: wildmatch's answers past a mismatch: that nothing further
//   along the text can match, and that nothing can before a `**` takes
//   the search back.
#define D_INTERNAL_MATCH          0
#define D_INTERNAL_NOMATCH        1
#define D_INTERNAL_ABORT_ALL      (-1)
#define D_INTERNAL_ABORT_STARSTAR (-2)

//==============================================================================
// 1.  TEXT
//==============================================================================


/*
d_internal_room
  Room in a buffer for `_more` bytes and a terminator.
*/
static bool
d_internal_room(
    struct d_git_buffer* _buffer,
    size_t               _more
)
{
    // a buffer that has failed stays failed
    if (_buffer->failed)
    {
        return false;
    }

    // the room grows by doubling
    if ((_buffer->length + _more + 1u) > _buffer->capacity)
    {
        size_t wanted = (_buffer->capacity > 0u) ? _buffer->capacity : 64u;

        // doubled until the text and its terminator fit
        while (wanted < (_buffer->length + _more + 1u))
        {
            wanted *= 2u;
        }

        char* const grown = realloc(_buffer->text, wanted);

        // memory that ran out: the buffer holds what it held, and says so
        if (!grown)
        {
            _buffer->failed = true;

            return false;
        }

        _buffer->text     = grown;
        _buffer->capacity = wanted;
    }

    return true;
}


bool
d_git_buffer_add(
    struct d_git_buffer* _buffer,
    const char*          _text,
    size_t               _length
)
{
    // there must be room for the text
    if (!d_internal_room(_buffer, _length))
    {
        return false;
    }

    // nothing to copy leaves only the terminator
    if (_length > 0u)
    {
        memcpy(&_buffer->text[_buffer->length], _text, _length);
    }

    _buffer->length               += _length;
    _buffer->text[_buffer->length] = '\0';

    return true;
}


bool
d_git_buffer_add_text(
    struct d_git_buffer* _buffer,
    const char*          _text
)
{
    return d_git_buffer_add(_buffer,
                            (_text) ? _text : "",
                            (_text) ? strlen(_text) : 0u);
}


bool
d_git_buffer_add_char(
    struct d_git_buffer* _buffer,
    char                 _byte
)
{
    return d_git_buffer_add(_buffer, &_byte, 1u);
}


char*
d_git_buffer_take(
    struct d_git_buffer* _buffer
)
{
    // a failed buffer gives nothing, and is emptied
    if (_buffer->failed)
    {
        d_git_buffer_free(_buffer);

        return NULL;
    }

    // an empty buffer still gives a string
    if (!_buffer->text)
    {
        return calloc(1u, 1u);
    }

    char* const text = _buffer->text;

    memset(_buffer, 0, sizeof(*_buffer));

    return text;
}


void
d_git_buffer_free(
    struct d_git_buffer* _buffer
)
{
    free(_buffer->text);
    memset(_buffer, 0, sizeof(*_buffer));

    return;
}


/*
d_internal_grow
  Room in an array of records for one more, its capacity doubled when it is
full; `_size` is a record's.
*/
static bool
d_internal_grow(
    void**  _items,
    size_t  _count,
    size_t* _capacity,
    size_t  _size
)
{
    // room left needs nothing
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t wanted = (*_capacity > 0u) ? (*_capacity * 2u) : 16u;
    void* const  grown  = realloc(*_items, wanted * _size);

    // memory that ran out leaves the array as it was
    if (!grown)
    {
        return false;
    }

    *_items    = grown;
    *_capacity = wanted;

    return true;
}


/*
d_internal_store
  Keeps `_length` bytes, terminated, among a file's strings, and gives
their offset; D_GIT_TEXT_NONE when memory runs out.
*/
static size_t
d_internal_store(
    struct d_git_text* _text,
    const char*        _bytes,
    size_t             _length
)
{
    // the strings grow by doubling, as a buffer does
    if ((_text->strings_length + _length + 1u) > _text->strings_capacity)
    {
        size_t wanted = (_text->strings_capacity > 0u)
                        ? _text->strings_capacity
                        : 256u;

        // doubled until the bytes and their terminator fit
        while (wanted < (_text->strings_length + _length + 1u))
        {
            wanted *= 2u;
        }

        char* const grown = realloc(_text->strings, wanted);

        // memory that ran out keeps nothing
        if (!grown)
        {
            return D_GIT_TEXT_NONE;
        }

        _text->strings          = grown;
        _text->strings_capacity = wanted;
    }

    const size_t at = _text->strings_length;

    // nothing to copy leaves only the terminator
    if (_length > 0u)
    {
        memcpy(&_text->strings[at], _bytes, _length);
    }

    _text->strings[at + _length] = '\0';
    _text->strings_length       += _length + 1u;

    return at;
}


/*
d_internal_problem
  Keeps a problem, on line `_line`.
*/
static bool
d_internal_problem(
    struct d_git_text* _text,
    size_t             _line,
    const char*        _what
)
{
    // room for one more problem
    if (!d_internal_grow((void**)&_text->problems,
                         _text->problem_count,
                         &_text->problem_capacity,
                         sizeof(*_text->problems)))
    {
        return false;
    }

    const size_t what = d_internal_store(_text, _what, strlen(_what));

    // the message must be kept too
    if (what == D_GIT_TEXT_NONE)
    {
        return false;
    }

    _text->problems[_text->problem_count].line = _line;
    _text->problems[_text->problem_count].what = what;
    ++_text->problem_count;

    return true;
}


//==============================================================================
// 2.  KINDS
//==============================================================================


// d_internal_kind_name
//   struct: a name a file of a kind has, or a program gives the kind.
struct d_internal_kind_name
{
    const char*          name;
    enum d_git_text_kind kind;
};

// d_internal_file_names
//   constant: the names a file's kind is known by.
static const struct d_internal_kind_name d_internal_file_names[] =
{
    { ".gitignore",      D_GIT_TEXT_IGNORE     },
    { "exclude",         D_GIT_TEXT_IGNORE     },
    { "ignore",          D_GIT_TEXT_IGNORE     },
    { ".gitattributes",  D_GIT_TEXT_ATTRIBUTES },
    { "attributes",      D_GIT_TEXT_ATTRIBUTES },
    { "config",          D_GIT_TEXT_CONFIG     },
    { "config.worktree", D_GIT_TEXT_CONFIG     },
    { ".gitconfig",      D_GIT_TEXT_CONFIG     },
    { "gitconfig",       D_GIT_TEXT_CONFIG     },
    { ".gitmodules",     D_GIT_TEXT_CONFIG     },
    { ".lfsconfig",      D_GIT_TEXT_CONFIG     }
};

// d_internal_kind_words
//   constant: the names a program gives a kind, besides a file's.
static const struct d_internal_kind_name d_internal_kind_words[] =
{
    { "gitignore",     D_GIT_TEXT_IGNORE     },
    { "gitattributes", D_GIT_TEXT_ATTRIBUTES },
    { "gitmodules",    D_GIT_TEXT_CONFIG     },
    { "modules",       D_GIT_TEXT_CONFIG     }
};


/*
d_internal_same_folded
  Whether two texts are alike in ASCII's letters of either case.
*/
static bool
d_internal_same_folded(
    const char* _left,
    const char* _right
)
{
    // compared byte by byte, letters folded
    for (; ( (*_left) && (*_right) ); ++_left, ++_right)
    {
        const unsigned char left  = (unsigned char)*_left;
        const unsigned char right = (unsigned char)*_right;
        const unsigned char lower_left  = ( (left >= 'A') && (left <= 'Z') )
                                          ? (unsigned char)(left + 32u)
                                          : left;
        const unsigned char lower_right = ( (right >= 'A') &&
                                            (right <= 'Z') )
                                          ? (unsigned char)(right + 32u)
                                          : right;

        // one differing byte is enough
        if (lower_left != lower_right)
        {
            return false;
        }
    }

    return (*_left == *_right);
}


/*
d_internal_kind_in
  The kind a table names a name, in any case; D_GIT_TEXT_OTHER when it
does not.
*/
static enum d_git_text_kind
d_internal_kind_in(
    const struct d_internal_kind_name* _table,
    size_t                             _count,
    const char*                        _name
)
{
    // the table is short, and read in order
    for (size_t at = 0u; at < _count; ++at)
    {
        // a name in any case
        if (d_internal_same_folded(_table[at].name, _name))
        {
            return _table[at].kind;
        }
    }

    return D_GIT_TEXT_OTHER;
}


enum d_git_text_kind
d_git_text_kind_of(
    const char* _path
)
{
    // no path, no kind
    if (!_path)
    {
        return D_GIT_TEXT_OTHER;
    }

    const char* name = _path;

    // the name is past the last separator, of either system
    for (const char* at = _path; *at; ++at)
    {
        // a separator starts the name over
        if ( (*at == '/') ||
             (*at == '\\') )
        {
            name = at + 1;
        }
    }

    return d_internal_kind_in(d_internal_file_names,
                              sizeof(d_internal_file_names)
                                  / sizeof(d_internal_file_names[0]),
                              name);
}


enum d_git_text_kind
d_git_text_kind_named(
    const char* _name
)
{
    // no name, no kind
    if (!_name)
    {
        return D_GIT_TEXT_OTHER;
    }

    const enum d_git_text_kind word =
        d_internal_kind_in(d_internal_kind_words,
                           sizeof(d_internal_kind_words)
                               / sizeof(d_internal_kind_words[0]),
                           _name);

    // a program's word for a kind, or a file's name
    if (word != D_GIT_TEXT_OTHER)
    {
        return word;
    }

    return d_git_text_kind_of(_name);
}


const char*
d_git_text_kind_name(
    enum d_git_text_kind _kind
)
{
    // each kind's own name
    if (_kind == D_GIT_TEXT_IGNORE)
    {
        return "ignore";
    }
    else if (_kind == D_GIT_TEXT_ATTRIBUTES)
    {
        return "attributes";
    }
    else if (_kind == D_GIT_TEXT_CONFIG)
    {
        return "config";
    }

    return "other";
}


//==============================================================================
// 3.  LINES
//==============================================================================


/*
d_internal_split
  Splits the text into lines at each `\n`, a `\r` before one counted with
the ending, as git counts it: a `\r` alone is text. A last line with no
ending is a line; a text that ends with an ending has no empty line after
it. Every line starts BLANK, for the reader of its kind to say otherwise.
*/
static bool
d_internal_split(
    struct d_git_text* _text
)
{
    size_t start = 0u;
    bool   seen  = false;

    // a line to each ending, and one for text after the last
    while (start < _text->length)
    {
        const char* const ending = memchr(&_text->text[start],
                                          '\n',
                                          _text->length - start);
        const size_t      next   = (ending)
                                   ? (size_t)(ending - _text->text) + 1u
                                   : _text->length;
        size_t            end    = (ending) ? (next - 1u) : next;

        // a `\r` before the `\n` is the ending's
        if ( (ending)      &&
             (end > start) &&
             (_text->text[end - 1u] == '\r') )
        {
            --end;
        }

        // the first line's ending is the one a change writes
        if ( (ending) &&
             (!seen) )
        {
            _text->crlf = (end < (next - 1u));
            seen        = true;
        }

        // room for the line
        if (!d_internal_grow((void**)&_text->lines,
                             _text->line_count,
                             &_text->line_capacity,
                             sizeof(*_text->lines)))
        {
            return false;
        }

        struct d_git_text_line* const line = &_text->lines[_text->line_count];

        line->start   = start;
        line->end     = end;
        line->next    = next;
        line->kind    = D_GIT_TEXT_BLANK;
        line->pattern = D_GIT_TEXT_NONE;
        line->section = D_GIT_TEXT_NONE;
        line->entry   = D_GIT_TEXT_NONE;
        ++_text->line_count;
        start = next;
    }

    return true;
}


/*
d_internal_line_text
  Where a line's text begins, past a byte-order mark on the first.
*/
static size_t
d_internal_line_text(
    const struct d_git_text* _text,
    size_t                   _line
)
{
    const size_t start = _text->lines[_line].start;

    // only the first line holds the mark
    if ( (_line == 0u) &&
         (_text->bom) )
    {
        return start + 3u;
    }

    return start;
}


/*
d_internal_add_pattern
  Keeps a pattern of line `_line`, written from `_start` to `_end`, that
git matches as `_length` bytes from `_bytes` -- a trailing `/` among them,
taken off here as git takes it -- and marks the line a pattern's or, for
a macro, a macro's.
*/
static bool
d_internal_add_pattern(
    struct d_git_text*               _text,
    const struct d_git_text_pattern* _pattern,
    const char*                      _bytes
)
{
    // room for one more pattern
    if (!d_internal_grow((void**)&_text->patterns,
                         _text->pattern_count,
                         &_text->pattern_capacity,
                         sizeof(*_text->patterns)))
    {
        return false;
    }

    struct d_git_text_pattern record = *_pattern;

    // a trailing slash: the pattern matches only a directory
    if ( (!record.macro)         &&
         (record.length > 0u)    &&
         (_bytes[record.length - 1u] == '/') )
    {
        record.directory = true;
        --record.length;
    }

    record.anywhere = ( (!record.macro) &&
                        (!memchr(_bytes, '/', record.length)) );
    record.text     = d_internal_store(_text, _bytes, record.length);

    // the pattern's text must be kept
    if (record.text == D_GIT_TEXT_NONE)
    {
        return false;
    }

    const size_t index = _text->pattern_count;

    _text->patterns[index] = record;
    ++_text->pattern_count;
    _text->lines[record.line].kind    = (record.macro)
                                        ? D_GIT_TEXT_MACRO
                                        : D_GIT_TEXT_PATTERN;
    _text->lines[record.line].pattern = index;

    return true;
}


/*
d_internal_trimmed
  Where an ignore pattern ends once git has trimmed it: the last run of
spaces is cut, unless a backslash escapes its first, or a backslash ends
the line, which leaves every space before it.
*/
static size_t
d_internal_trimmed(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    size_t last_space = D_GIT_TEXT_NONE;

    // a space starts a run, anything else ends it; `\` escapes the next
    for (size_t at = _start; at < _end; ++at)
    {
        // the first space of a run is where the cut would be
        if (_text[at] == ' ')
        {
            last_space = (last_space == D_GIT_TEXT_NONE) ? at : last_space;

            continue;
        }

        // an escape takes the next byte as itself, and one at the end
        // leaves the line as it is
        if (_text[at] == '\\')
        {
            ++at;

            // a backslash that ends the line: nothing is cut
            if (at >= _end)
            {
                return _end;
            }
        }

        last_space = D_GIT_TEXT_NONE;
    }

    return (last_space != D_GIT_TEXT_NONE) ? last_space : _end;
}


/*
d_internal_read_ignore_line
  One line of an ignore file, as git's add_patterns_from_buffer reads it:
`#` in the first column a comment, an empty line nothing, anything else a
pattern -- even white space other than spaces. A pattern ends at a NUL, as
git's C string does, and a `\r` that ends the file is the ending git adds.
*/
static bool
d_internal_read_ignore_line(
    struct d_git_text* _text,
    size_t             _line
)
{
    const size_t      start = d_internal_line_text(_text, _line);
    size_t            end   = _text->lines[_line].end;
    const char* const nul   = memchr(&_text->text[start], '\0', end - start);

    // git's string ends at a NUL
    if (nul)
    {
        end = (size_t)(nul - _text->text);
    }

    // the last line, with no ending of its own, ends in the one git adds
    if ( (_text->lines[_line].next == end) &&
         (end > start)                     &&
         (_text->text[end - 1u] == '\r') )
    {
        --end;
    }

    // an empty line holds nothing
    if (end == start)
    {
        return true;
    }

    // `#` in the first column
    if (_text->text[start] == '#')
    {
        _text->lines[_line].kind = D_GIT_TEXT_COMMENT;

        return true;
    }

    const size_t trimmed = d_internal_trimmed(_text->text, start, end);

    // spaces alone trim to nothing, which matches nothing
    if (trimmed == start)
    {
        return true;
    }

    const bool                negated = (_text->text[start] == '!');
    struct d_git_text_pattern pattern;

    memset(&pattern, 0, sizeof(pattern));
    pattern.line            = _line;
    pattern.start           = start;
    pattern.end             = trimmed;
    pattern.negated         = negated;
    pattern.length          = trimmed - start - ((negated) ? 1u : 0u);
    pattern.first_attribute = _text->attribute_count;

    return d_internal_add_pattern(_text,
                                  &pattern,
                                  &_text->text[start + ((negated) ? 1u : 0u)]);
}


//==============================================================================
// 4.  ATTRIBUTES
//==============================================================================


/*
d_internal_blank
  Whether a byte is one git's attribute lines are apart by.
*/
static bool
d_internal_blank(
    char _byte
)
{
    return ( (_byte == ' ')  ||
             (_byte == '\t') ||
             (_byte == '\r') ||
             (_byte == '\n') );
}


/*
d_internal_skip_blanks, d_internal_skip_token
  Past the blanks from `_at`, and past the bytes that are not.
*/
static size_t
d_internal_skip_blanks(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    // blanks, until the end
    while ( (_at < _end) &&
            (d_internal_blank(_text[_at])) )
    {
        ++_at;
    }

    return _at;
}


static size_t
d_internal_skip_token(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    // anything but a blank, until the end
    while ( (_at < _end) &&
            (!d_internal_blank(_text[_at])) )
    {
        ++_at;
    }

    return _at;
}


/*
d_internal_attribute_name
  Whether `_length` bytes are an attribute's name to git: letters, digits,
`-`, `.` and `_`, not beginning with `-`.
*/
static bool
d_internal_attribute_name(
    const char* _name,
    size_t      _length
)
{
    // a name has a byte, and does not begin with `-`
    if ( (_length == 0u) ||
         (_name[0] == '-') )
    {
        return false;
    }

    // each byte one a name may hold
    for (size_t at = 0u; at < _length; ++at)
    {
        const char byte = _name[at];

        // letters, digits, and three punctuation marks
        if ( (!( (byte >= 'a') && (byte <= 'z') )) &&
             (!( (byte >= 'A') && (byte <= 'Z') )) &&
             (!( (byte >= '0') && (byte <= '9') )) &&
             (byte != '-')                         &&
             (byte != '.')                         &&
             (byte != '_') )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_parse_state
  One state of an attribute line, the token from `_at` to `_end`: "name",
"-name", "!name", or "name=value" -- an `=` in "-name=value" leaves the
state unset, its value unread, as git leaves it. False for a name git
refuses.
*/
static bool
d_internal_parse_state(
    const char*                  _text,
    size_t                       _at,
    size_t                       _end,
    struct d_git_text_attribute* _out
)
{
    const char* const equals = memchr(&_text[_at], '=', _end - _at);
    const size_t      split  = (equals)
                               ? (size_t)(equals - _text)
                               : _end;
    size_t            name   = _at;

    _out->start       = _at;
    _out->end         = _end;
    _out->value_start = D_GIT_TEXT_NONE;
    _out->value_end   = D_GIT_TEXT_NONE;

    // `-` unsets and `!` leaves unspecified; a name alone sets
    if ( (_text[_at] == '-') ||
         (_text[_at] == '!') )
    {
        _out->state = (_text[_at] == '-')
                      ? D_GIT_TEXT_UNSET
                      : D_GIT_TEXT_UNSPECIFIED;
        ++name;
    }
    else if (equals)
    {
        _out->state       = D_GIT_TEXT_VALUE;
        _out->value_start = split + 1u;
        _out->value_end   = _end;
    }
    else
    {
        _out->state = D_GIT_TEXT_SET;
    }

    _out->name_start = name;
    _out->name_end   = split;

    return d_internal_attribute_name(&_text[name], split - name);
}


/*
d_internal_parse_states
  The states of an attribute line, from `_at` to `_end`, kept from the
file's first free attribute on; false, with nothing kept, when one has a
name git refuses -- `_refused` then says so -- or memory runs out.
*/
static bool
d_internal_parse_states(
    struct d_git_text* _text,
    size_t             _at,
    size_t             _end,
    bool*              _refused
)
{
    const size_t first = _text->attribute_count;

    *_refused = false;
    _at       = d_internal_skip_blanks(_text->text, _at, _end);

    // a token to each state
    while (_at < _end)
    {
        const size_t token_end = d_internal_skip_token(_text->text,
                                                       _at,
                                                       _end);

        // room for one more state
        if (!d_internal_grow((void**)&_text->attributes,
                             _text->attribute_count,
                             &_text->attribute_capacity,
                             sizeof(*_text->attributes)))
        {
            _text->attribute_count = first;

            return false;
        }

        // a name git refuses refuses the line
        if (!d_internal_parse_state(_text->text,
                                    _at,
                                    token_end,
                                    &_text->attributes[_text->attribute_count]))
        {
            _text->attribute_count = first;
            *_refused              = true;

            return false;
        }

        ++_text->attribute_count;
        _at = d_internal_skip_blanks(_text->text, token_end, _end);
    }

    return true;
}


/*
d_internal_refuse
  Marks a line one git reads no rule from, and says why.
*/
static bool
d_internal_refuse(
    struct d_git_text* _text,
    size_t             _line,
    const char*        _why
)
{
    _text->lines[_line].kind = D_GIT_TEXT_REFUSED;

    return d_internal_problem(_text, _line, _why);
}


// d_internal_attribute_line
//   struct: what an attribute line's first token is: `start` to `end` as
//   written, `states` where its states begin, and the pattern git
//   matches, unquoted when it is quoted, in `pattern`.
struct d_internal_attribute_line
{
    size_t              start;
    size_t              end;
    size_t              states;
    bool                quoted;
    struct d_git_buffer pattern;
};


/*
d_internal_attribute_head
  Reads an attribute line's first token, its pattern or macro: in C's
quotes when it begins with one and they are sound, otherwise up to the
first blank.
*/
static bool
d_internal_attribute_head(
    const struct d_git_text*          _text,
    size_t                            _at,
    size_t                            _end,
    struct d_internal_attribute_line* _out
)
{
    size_t used = 0u;

    _out->start  = _at;
    _out->quoted = false;

    // a quoted pattern, unquoted
    if ( (_text->text[_at] == '"') &&
         (d_git_text_unquote(&_out->pattern,
                             &_text->text[_at],
                             _end - _at,
                             &used)) )
    {
        _out->quoted = true;
        _out->end    = _at + used;
        _out->states = _out->end;

        return true;
    }

    // a quote that is not sound is the pattern's first byte
    d_git_buffer_free(&_out->pattern);
    _out->end    = d_internal_skip_token(_text->text, _at, _end);
    _out->states = _out->end;

    return d_git_buffer_add(&_out->pattern,
                            &_text->text[_at],
                            _out->end - _at);
}


/*
d_internal_attribute_macro
  Keeps an attribute line's macro, "[attr]name" and the states after it.
The name follows the prefix, past any blanks a quoted token holds.
*/
static bool
d_internal_attribute_macro(
    struct d_git_text*                      _text,
    size_t                                  _line,
    const struct d_internal_attribute_line* _head,
    size_t                                  _end
)
{
    const char* const pattern = _head->pattern.text;
    const size_t      at      = d_internal_skip_blanks(pattern,
                                                       strlen(D_INTERNAL_MACRO),
                                                       _head->pattern.length);
    const size_t      stop    = d_internal_skip_token(pattern,
                                                      at,
                                                      _head->pattern.length);

    // the macro's name must be one git allows
    if (!d_internal_attribute_name(&pattern[at], stop - at))
    {
        return d_internal_refuse(_text, _line, "invalid attribute name");
    }

    const size_t first   = _text->attribute_count;
    bool         refused = false;

    // the states, or why the line is refused
    if (!d_internal_parse_states(_text, _head->states, _end, &refused))
    {
        return (refused)
               ? d_internal_refuse(_text, _line, "invalid attribute name")
               : false;
    }

    struct d_git_text_pattern macro;

    memset(&macro, 0, sizeof(macro));
    macro.line            = _line;
    macro.start           = _head->start;
    macro.end             = _head->end;
    macro.length          = stop - at;
    macro.quoted          = _head->quoted;
    macro.macro           = true;
    macro.first_attribute = first;
    macro.attribute_count = _text->attribute_count - first;

    return d_internal_add_pattern(_text, &macro, &pattern[at]);
}


/*
d_internal_attribute_pattern
  Keeps an attribute line's pattern and its states. A negated pattern git
refuses, as it does a state of a name it does not allow.
*/
static bool
d_internal_attribute_pattern(
    struct d_git_text*                      _text,
    size_t                                  _line,
    const struct d_internal_attribute_line* _head,
    size_t                                  _end
)
{
    const size_t first   = _text->attribute_count;
    bool         refused = false;

    // the states, or why the line is refused
    if (!d_internal_parse_states(_text, _head->states, _end, &refused))
    {
        return (refused)
               ? d_internal_refuse(_text, _line, "invalid attribute name")
               : false;
    }

    // `!` negates nothing among attributes: git ignores the line
    if ( (_head->pattern.length > 0u) &&
         (_head->pattern.text[0] == '!') )
    {
        _text->attribute_count = first;

        return d_internal_refuse(_text,
                                 _line,
                                 "negative patterns are ignored in git "
                                 "attributes");
    }

    struct d_git_text_pattern pattern;

    memset(&pattern, 0, sizeof(pattern));
    pattern.line            = _line;
    pattern.start           = _head->start;
    pattern.end             = _head->end;
    pattern.length          = _head->pattern.length;
    pattern.quoted          = _head->quoted;
    pattern.first_attribute = first;
    pattern.attribute_count = _text->attribute_count - first;

    return d_internal_add_pattern(_text,
                                  &pattern,
                                  (_head->pattern.text)
                                  ? _head->pattern.text
                                  : "");
}


/*
d_internal_read_attribute_line
  One line of an attribute file, as git's parse_attr_line reads it: blanks
before anything skipped, `#` then a comment, a line of 2048 bytes or more
ignored, a first token that begins "[attr]" and is longer a macro, and
any other a pattern. A line ends at a NUL, as git's C string does.
  Long as parse_attr_line is, which it follows.
*/
static bool
d_internal_read_attribute_line(
    struct d_git_text* _text,
    size_t             _line
)
{
    const size_t      start = d_internal_line_text(_text, _line);
    size_t            end   = _text->lines[_line].end;
    const char* const nul   = memchr(&_text->text[start], '\0', end - start);

    // git's string ends at a NUL
    if (nul)
    {
        end = (size_t)(nul - _text->text);
    }

    const size_t at = d_internal_skip_blanks(_text->text, start, end);

    // nothing but blanks, or a comment
    if ( (at == end) ||
         (_text->text[at] == '#') )
    {
        _text->lines[_line].kind = (at == end)
                                   ? D_GIT_TEXT_BLANK
                                   : D_GIT_TEXT_COMMENT;

        return true;
    }

    // a line git thinks too long
    if ((end - start) >= D_INTERNAL_LONGEST)
    {
        return d_internal_refuse(_text,
                                 _line,
                                 "ignoring overly long attributes line");
    }

    struct d_internal_attribute_line head;

    memset(&head, 0, sizeof(head));

    // the first token: the pattern, or the macro
    if (!d_internal_attribute_head(_text, at, end, &head))
    {
        d_git_buffer_free(&head.pattern);

        return false;
    }

    const size_t prefix = strlen(D_INTERNAL_MACRO);
    const bool   macro  = ( (head.pattern.length > prefix) &&
                            (memcmp(head.pattern.text,
                                    D_INTERNAL_MACRO,
                                    prefix) == 0) );
    const bool   kept   = (macro)
                          ? d_internal_attribute_macro(_text,
                                                       _line,
                                                       &head,
                                                       end)
                          : d_internal_attribute_pattern(_text,
                                                         _line,
                                                         &head,
                                                         end);

    d_git_buffer_free(&head.pattern);

    return kept;
}


//==============================================================================
// 5.  CONFIGURATION
//==============================================================================


// d_internal_cursor
//   struct: a place in a configuration's text, read as git's
//   get_next_char reads it: `at` the next byte, `eof` once the end has
//   been read.
struct d_internal_cursor
{
    const char* text;
    size_t      length;
    size_t      at;
    bool        eof;
};

// d_internal_config
//   struct: a configuration being read: the section it is in, its prefix
//   as keys begin with it, where reading stopped -- D_GIT_TEXT_NONE until
//   it does -- the line's end the last value read ended at, whether git
//   counts the line past the one it stopped on, and whether memory ran
//   out.
struct d_internal_config
{
    struct d_git_text*       text;
    struct d_internal_cursor cursor;
    struct d_git_buffer      prefix;
    size_t                   section;
    size_t                   error;
    size_t                   stop;
    bool                     past_end;
    bool                     failed;
};


/*
d_internal_next
  The next character, as git's get_next_char gives it: `\r\n` as `\n`, a
`\r` alone as itself, and the end as `\n`, with `eof` set. `_where`
receives the place of its first byte -- the end's is the text's length.
*/
static int
d_internal_next(
    struct d_internal_cursor* _cursor,
    size_t*                   _where
)
{
    *_where = _cursor->at;

    // the end reads as a line's ending, again and again
    if (_cursor->at >= _cursor->length)
    {
        _cursor->eof = true;

        return '\n';
    }

    const unsigned char byte = (unsigned char)_cursor->text[_cursor->at];

    ++_cursor->at;

    // a `\r` before a `\n` is one ending
    if ( (byte == '\r')                      &&
         (_cursor->at < _cursor->length)     &&
         (_cursor->text[_cursor->at] == '\n') )
    {
        ++_cursor->at;

        return '\n';
    }

    return (int)byte;
}


/*
d_internal_space, d_internal_alpha, d_internal_keychar, d_internal_lower
  Git's own classes, which no locale moves: its white space is a space,
`\t`, `\n` and `\r`; a key's bytes are ASCII's letters and digits and `-`.
*/
static bool
d_internal_space(
    int _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\n') ||
             (_c == '\r') );
}


static bool
d_internal_alpha(
    int _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) );
}


static bool
d_internal_keychar(
    int _c
)
{
    return ( (d_internal_alpha(_c))           ||
             ( (_c >= '0') && (_c <= '9') )   ||
             (_c == '-') );
}


static char
d_internal_lower(
    int _c
)
{
    return ( (_c >= 'A') && (_c <= 'Z') ) ? (char)(_c + 32) : (char)_c;
}


/*
d_internal_stop
  Reading stops at `_where`: git refuses the line it is on.
*/
static bool
d_internal_stop(
    struct d_internal_config* _config,
    size_t                    _where
)
{
    _config->error = _where;

    return false;
}


/*
d_internal_add_text
  Adds to a buffer, noting memory that ran out.
*/
static bool
d_internal_add_text(
    struct d_internal_config* _config,
    struct d_git_buffer*      _buffer,
    char                      _byte
)
{
    // memory that ran out stops the reading
    if (!d_git_buffer_add_char(_buffer, _byte))
    {
        _config->failed = true;

        return false;
    }

    return true;
}


/*
d_internal_subsection
  A section header's subsection, as git's get_extended_base_var reads it:
blanks after the name, then a quoted text, a `\` taking the next byte as
itself, and `]` straight after the closing quote. `_c` is the blank that
ended the name, at the name's end.
  Long as get_extended_base_var is, which it follows step by step.
*/
static bool
d_internal_subsection(
    struct d_internal_config*  _config,
    int                        _c,
    struct d_git_text_section* _section
)
{
    size_t where = _section->name_end;
    int    c     = _c;

    // blanks, but not past the line's end
    do
    {
        // a header ends on its line
        if (c == '\n')
        {
            return d_internal_stop(_config, where);
        }

        c = d_internal_next(&_config->cursor, &where);
    }
    while (d_internal_space(c));

    // the subsection is quoted
    if (c != '"')
    {
        return d_internal_stop(_config, where);
    }

    _section->sub_start = where + 1u;

    // the dot that joins the subsection to the name
    if (!d_internal_add_text(_config, &_config->prefix, '.'))
    {
        return false;
    }

    // to the closing quote, a backslash escaping what follows
    for (;;)
    {
        c = d_internal_next(&_config->cursor, &where);

        // the closing quote ends it
        if (c == '"')
        {
            break;
        }

        // an escape is the byte after it
        if (c == '\\')
        {
            c = d_internal_next(&_config->cursor, &where);
        }

        // a subsection ends on its line
        if (c == '\n')
        {
            return d_internal_stop(_config, where);
        }

        // the byte, as it is
        if (!d_internal_add_text(_config, &_config->prefix, (char)c))
        {
            return false;
        }
    }

    _section->sub_end = where;
    c                 = d_internal_next(&_config->cursor, &where);

    // `]` at once; a line's end, or the text's, instead counts a line
    // past it, as git takes neither back
    if (c != ']')
    {
        _config->past_end = (c == '\n');

        return d_internal_stop(_config, where);
    }

    _section->end = where + 1u;

    return true;
}


/*
d_internal_keep_section
  Keeps a section's header once read, and makes it the one entries are in.
*/
static bool
d_internal_keep_section(
    struct d_internal_config*  _config,
    struct d_git_text_section* _section
)
{
    struct d_git_text* const text = _config->text;

    // room for one more section
    if (!d_internal_grow((void**)&text->sections,
                         text->section_count,
                         &text->section_capacity,
                         sizeof(*text->sections)))
    {
        _config->failed = true;

        return false;
    }

    _section->prefix = d_internal_store(text,
                                        (_config->prefix.text)
                                        ? _config->prefix.text
                                        : "",
                                        _config->prefix.length);

    // the prefix must be kept
    if (_section->prefix == D_GIT_TEXT_NONE)
    {
        _config->failed = true;

        return false;
    }

    _config->section                 = text->section_count;
    text->sections[_config->section] = *_section;
    ++text->section_count;

    return true;
}


/*
d_internal_config_section
  A section's header, from its `[` at `_open`, as git's get_base_var reads
it: a name of letters, digits, `-` and `.`, kept in lower case, then `]`,
or blanks and a quoted subsection. A header with no name at all is
refused.
  Long as get_base_var is, which it follows.
*/
static bool
d_internal_config_section(
    struct d_internal_config* _config,
    size_t                    _open
)
{
    struct d_git_text_section section;

    memset(&section, 0, sizeof(section));
    section.start      = _open;
    section.name_start = _open + 1u;
    section.sub_start  = D_GIT_TEXT_NONE;
    section.sub_end    = D_GIT_TEXT_NONE;
    _config->prefix.length = 0u;

    // the name, to `]` or a blank
    for (;;)
    {
        size_t    where = 0u;
        const int c     = d_internal_next(&_config->cursor, &where);

        // a header ends before the text does: git counts a line past
        // the last, as its get_base_var does not take back the end's
        if (_config->cursor.eof)
        {
            _config->past_end = true;

            return d_internal_stop(_config, where);
        }

        // `]`, or a blank and a subsection, ends the name
        if ( (c == ']') ||
             (d_internal_space(c)) )
        {
            section.name_end = where;
            section.end      = where + 1u;

            // a subsection's text, or nothing
            if ( (c != ']') &&
                 (!d_internal_subsection(_config, c, &section)) )
            {
                return false;
            }

            break;
        }

        // a name's byte, kept in lower case
        if ( (!d_internal_keychar(c)) &&
             (c != '.') )
        {
            return d_internal_stop(_config, where);
        }

        // the name, lower case
        if (!d_internal_add_text(_config,
                                 &_config->prefix,
                                 d_internal_lower(c)))
        {
            return false;
        }
    }

    // a header must name something
    if (_config->prefix.length == 0u)
    {
        return d_internal_stop(_config, _open);
    }

    section.legacy = ( (section.sub_start == D_GIT_TEXT_NONE) &&
                       (memchr(&_config->text->text[section.name_start],
                               '.',
                               section.name_end - section.name_start)) );

    return d_internal_keep_section(_config, &section);
}


/*
d_internal_escape
  What a `\` and the byte after it are in a value: `\t`, `\b`, `\n`, `\\`
and `\"`; -1 for any other, which git refuses.
*/
static int
d_internal_escape(
    int _c
)
{
    // the escapes git knows
    if (_c == 't')
    {
        return '\t';
    }
    else if (_c == 'b')
    {
        return '\b';
    }
    else if (_c == 'n')
    {
        return '\n';
    }
    else if ( (_c == '\\') ||
              (_c == '"') )
    {
        return _c;
    }

    return -1;
}


// d_internal_value
//   struct: a value being read: its text as git reads it, where it is
//   written, whether a quote is open, a comment begun, and how many
//   blanks wait to be written as spaces before the next byte.
struct d_internal_value
{
    struct d_git_buffer text;
    size_t              start;
    size_t              end;
    bool                quoted;
    bool                comment;
    bool                continued;
    size_t              spaces;
};


/*
d_internal_value_byte
  One byte of a value that is written: the blanks before it, as spaces,
then the byte -- an escape's meaning, a quote's toggling, or itself.
*/
static bool
d_internal_value_byte(
    struct d_internal_config* _config,
    struct d_internal_value*  _value,
    int                       _c,
    size_t                    _where
)
{
    _value->continued = false;

    // a value is written from its first byte that is
    if (_value->start == D_GIT_TEXT_NONE)
    {
        _value->start = _where;
    }

    // blanks between bytes are spaces
    for (; _value->spaces > 0u; --_value->spaces)
    {
        // each one a space
        if (!d_internal_add_text(_config, &_value->text, ' '))
        {
            return false;
        }
    }

    // a quote opens or closes, and is not the value's
    if (_c == '"')
    {
        _value->quoted = !_value->quoted;
        _value->end    = _where + 1u;

        return true;
    }

    // an escape, or a line continued
    if (_c == '\\')
    {
        size_t    where   = 0u;
        const int escaped = d_internal_next(&_config->cursor, &where);

        // a `\` that ends a line joins the next to it
        if (escaped == '\n')
        {
            _value->end       = _config->cursor.at;
            _value->continued = true;

            return true;
        }

        const int meant = d_internal_escape(escaped);

        // an escape git does not know
        if (meant < 0)
        {
            return d_internal_stop(_config, where);
        }

        _value->end = where + 1u;

        return d_internal_add_text(_config, &_value->text, (char)meant);
    }

    _value->end = _where + 1u;

    return d_internal_add_text(_config, &_value->text, (char)_c);
}


/*
d_internal_config_value
  An entry's value, as git's parse_value reads it: blanks before it
skipped, blanks within it -- outside quotes -- each a space, blanks after
it dropped, `;` and `#` outside quotes beginning a comment, and a line's
end inside quotes refused.
  Long as parse_value is, which it follows: quotes, comments, blanks,
escapes and continued lines.
*/
static bool
d_internal_config_value(
    struct d_internal_config* _config,
    struct d_git_text_entry*  _entry
)
{
    struct d_internal_value value;

    memset(&value, 0, sizeof(value));
    value.start = D_GIT_TEXT_NONE;
    value.end   = D_GIT_TEXT_NONE;

    // to the line's end, which a continuation moves on
    for (;;)
    {
        size_t    where = 0u;
        const int c     = d_internal_next(&_config->cursor, &where);

        // the line's end ends the value, but not inside quotes
        if (c == '\n')
        {
            // a quote left open; at the end, just past a line joined to
            // it, git counts a line past the last
            if (value.quoted)
            {
                d_git_buffer_free(&value.text);
                _config->past_end = ( (_config->cursor.eof) &&
                                      (value.continued) );

                return d_internal_stop(_config, where);
            }

            _config->stop = where;

            break;
        }

        // a comment runs to the line's end
        if (value.comment)
        {
            continue;
        }

        // a blank outside quotes waits to be a space, or is dropped
        if ( (d_internal_space(c)) &&
             (!value.quoted) )
        {
            value.spaces += (value.text.length > 0u) ? 1u : 0u;

            continue;
        }

        // `;` and `#` begin a comment outside quotes
        if ( (!value.quoted) &&
             ( (c == ';') || (c == '#') ) )
        {
            value.comment = true;

            continue;
        }

        // the byte is the value's
        if (!d_internal_value_byte(_config, &value, c, where))
        {
            d_git_buffer_free(&value.text);

            return false;
        }
    }

    // a value of nothing is written where it would begin: past the `=`
    // and the blanks after it
    if (value.start == D_GIT_TEXT_NONE)
    {
        size_t at = _entry->equals + 1u;

        // spaces and tabs on the line
        while ( (at < _config->text->length) &&
                ( (_config->text->text[at] == ' ') ||
                  (_config->text->text[at] == '\t') ) )
        {
            ++at;
        }

        value.start = at;
        value.end   = at;
    }

    _entry->value_start = value.start;
    _entry->value_end   = value.end;
    _entry->value       = d_internal_store(_config->text,
                                           (value.text.text)
                                           ? value.text.text
                                           : "",
                                           value.text.length);
    d_git_buffer_free(&value.text);
    _config->failed = ( (_config->failed) ||
                        (_entry->value == D_GIT_TEXT_NONE) );

    return !_config->failed;
}


/*
d_internal_keep_entry
  Keeps an entry once read, its key the section's prefix, a dot, and the
name -- or the name alone before any section, as git makes it -- and
counts it among its section's.
  Long as the key is made, the entry kept and its section told, each a step.
*/
static bool
d_internal_keep_entry(
    struct d_internal_config* _config,
    struct d_git_text_entry*  _entry,
    struct d_git_buffer*      _name
)
{
    struct d_git_text* const text = _config->text;
    struct d_git_buffer      key;

    memset(&key, 0, sizeof(key));

    // the section's prefix, and the dot after it
    if (_config->section != D_GIT_TEXT_NONE)
    {
        (void)d_git_buffer_add_text(&key,
                                    d_git_text_string(text,
                                        text->sections[_config->section]
                                            .prefix));
        (void)d_git_buffer_add_char(&key, '.');
    }

    (void)d_git_buffer_add(&key, _name->text, _name->length);
    _entry->key = (key.failed)
                  ? D_GIT_TEXT_NONE
                  : d_internal_store(text, key.text, key.length);
    d_git_buffer_free(&key);

    // the key, and room for the entry
    if ( (_entry->key == D_GIT_TEXT_NONE) ||
         (!d_internal_grow((void**)&text->entries,
                           text->entry_count,
                           &text->entry_capacity,
                           sizeof(*text->entries))) )
    {
        _config->failed = true;

        return false;
    }

    _entry->end       = (_entry->value_end != D_GIT_TEXT_NONE)
                        ? _entry->value_end
                        : _entry->name_end;
    _entry->section   = _config->section;
    _entry->line      = d_git_text_line_of(text, _entry->start);

    // the line whose end ended its value -- one a `\` joined to it
    // among them -- or its own
    _entry->last_line = ( (_config->stop != D_GIT_TEXT_NONE) &&
                          (_config->stop >= _entry->start) )
                        ? d_git_text_line_of(text, _config->stop)
                        : d_git_text_line_of(text,
                                             (_entry->end > _entry->start)
                                             ? (_entry->end - 1u)
                                             : _entry->start);
    _config->stop     = D_GIT_TEXT_NONE;

    // the section counts its entries, which follow one another
    if (_config->section != D_GIT_TEXT_NONE)
    {
        struct d_git_text_section* const section =
            &text->sections[_config->section];

        section->first_entry = (section->entry_count == 0u)
                               ? text->entry_count
                               : section->first_entry;
        ++section->entry_count;
    }

    text->entries[text->entry_count] = *_entry;
    ++text->entry_count;

    return true;
}


/*
d_internal_config_entry
  An entry, from its name's first byte `_first` at `_start`, as git's
get_value reads it: the name's letters, digits and `-`, kept in lower
case, then spaces and tabs, then the line's end -- a name alone -- or `=`
and a value.
*/
static bool
d_internal_config_entry(
    struct d_internal_config* _config,
    size_t                    _start,
    int                       _first
)
{
    struct d_git_text_entry entry;
    struct d_git_buffer     name;
    size_t                  where = 0u;
    int                     c     = 0;

    memset(&entry, 0, sizeof(entry));
    memset(&name, 0, sizeof(name));
    entry.start       = _start;
    entry.name_start  = _start;
    entry.equals      = D_GIT_TEXT_NONE;
    entry.value_start = D_GIT_TEXT_NONE;
    entry.value_end   = D_GIT_TEXT_NONE;
    entry.value       = D_GIT_TEXT_NONE;
    (void)d_git_buffer_add_char(&name, d_internal_lower(_first));

    // the name's bytes, until another or the end
    for (;;)
    {
        c = d_internal_next(&_config->cursor, &where);

        // the end, or a byte a name does not hold, ends it
        if ( (_config->cursor.eof) ||
             (!d_internal_keychar(c)) )
        {
            break;
        }

        (void)d_git_buffer_add_char(&name, d_internal_lower(c));
    }

    entry.name_end = where;

    // spaces and tabs before `=` or the line's end
    while ( (c == ' ') ||
            (c == '\t') )
    {
        c = d_internal_next(&_config->cursor, &where);
    }

    bool read = true;

    // `=` and a value, or the line's end and none
    if (c != '\n')
    {
        entry.equals = where;
        read         = (c == '=')
                       ? d_internal_config_value(_config, &entry)
                       : d_internal_stop(_config, where);
    }

    _config->failed = ( (_config->failed) ||
                        (name.failed) );
    read = ( (read) &&
             (!_config->failed) &&
             (d_internal_keep_entry(_config, &entry, &name)) );
    d_git_buffer_free(&name);

    return read;
}


/*
d_internal_partial_mark
  Reading stops at a byte-order mark begun and not finished, where the
first byte that is not the mark's is; git counts a line past it when that
byte is a line's end, or the text's.
*/
static void
d_internal_partial_mark(
    struct d_internal_config* _config
)
{
    const char* const mark  = D_INTERNAL_BOM;
    size_t            where = 0u;
    int               c     = 0;

    // the mark's bytes, while they are the text's
    do
    {
        c = d_internal_next(&_config->cursor, &where);
    }
    while ( (where < 3u) &&
            (c == (int)(unsigned char)mark[where]) );

    _config->past_end = (c == '\n');
    (void)d_internal_stop(_config, where);

    return;
}


/*
d_internal_config_walk
  Reads a configuration's text as git's git_parse_source walks it: blanks
and comments skipped, `[` a section's header, a letter an entry, anything
else refused. A byte-order mark is read past; part of one is refused.
  Long as git_parse_source is, which it follows: the mark, comments,
sections and entries.
*/
static void
d_internal_config_walk(
    struct d_internal_config* _config
)
{
    struct d_git_text* const text = _config->text;
    bool                     comment = false;
    const size_t             bom     = strlen(D_INTERNAL_BOM);

    // a mark, whole, is read past; a part of one is refused
    if ( (text->length >= bom) &&
         (memcmp(text->text, D_INTERNAL_BOM, bom) == 0) )
    {
        _config->cursor.at = bom;
    }
    else if ( (text->length > 0u) &&
              ((unsigned char)text->text[0] == 0xEFu) )
    {
        d_internal_partial_mark(_config);

        return;
    }

    // to the end, or to what git refuses
    for (;;)
    {
        size_t    where = 0u;
        const int c     = d_internal_next(&_config->cursor, &where);

        // a line's end ends a comment; the text's end, everything
        if (c == '\n')
        {
            // the end
            if (_config->cursor.eof)
            {
                return;
            }

            comment = false;

            continue;
        }

        // within a comment, and blanks, nothing is read
        if ( (comment) ||
             (d_internal_space(c)) )
        {
            continue;
        }

        // a comment, a section, an entry, or a byte git refuses
        if ( (c == '#') ||
             (c == ';') )
        {
            comment = true;
        }
        else if (c == '[')
        {
            // a header that is not one stops the reading
            if (!d_internal_config_section(_config, where))
            {
                return;
            }
        }
        else if ( (!d_internal_alpha(c)) ||
                  (!d_internal_config_entry(_config, where, c)) )
        {
            // what git refuses, unless memory ran out first
            if (_config->error == D_GIT_TEXT_NONE)
            {
                _config->error = where;
            }

            return;
        }
    }
}


/*
d_internal_config_lines
  Tells each line of a configuration what it is: a section's, an entry's
-- each line it spans -- a comment's, a blank; and each the section it is
in. From the line reading stopped at, every line that is not blank is
refused.
  Long as each line's kind is decided from the records over it, a step for
each.
*/
static void
d_internal_config_lines(
    struct d_git_text* _text
)
{
    // a header's line
    for (size_t at = 0u; at < _text->section_count; ++at)
    {
        const size_t line = d_git_text_line_of(_text,
                                               _text->sections[at].start);

        _text->sections[at].line   = line;
        _text->lines[line].kind    = D_GIT_TEXT_SECTION;
        _text->lines[line].section = at;
    }

    // an entry's lines, a header's line holding one among them
    for (size_t at = 0u; at < _text->entry_count; ++at)
    {
        const struct d_git_text_entry* const entry = &_text->entries[at];

        // each line it spans
        for (size_t line = entry->line; line <= entry->last_line; ++line)
        {
            _text->lines[line].entry = at;
            _text->lines[line].kind  = (_text->lines[line].kind
                                           == D_GIT_TEXT_SECTION)
                                       ? D_GIT_TEXT_SECTION
                                       : D_GIT_TEXT_ENTRY;
        }
    }

    size_t section = D_GIT_TEXT_NONE;

    // the rest are comments, blanks, or refused; each is in a section
    for (size_t line = 0u; line < _text->line_count; ++line)
    {
        struct d_git_text_line* const record = &_text->lines[line];

        section = (record->section != D_GIT_TEXT_NONE)
                  ? record->section
                  : section;
        record->section = section;

        // a line not yet told is a comment, a blank, or past the end
        if (record->kind == D_GIT_TEXT_BLANK)
        {
            size_t at = d_internal_line_text(_text, line);

            // past git's white space
            while ( (at < record->end) &&
                    (d_internal_space(_text->text[at])) )
            {
                ++at;
            }

            record->kind = (at == record->end)
                           ? D_GIT_TEXT_BLANK
                           : ( ( (_text->text[at] == '#') ||
                                 (_text->text[at] == ';') )
                               ? D_GIT_TEXT_COMMENT
                               : D_GIT_TEXT_REFUSED );
        }

        // from the line reading stopped at, nothing is read
        if ( (_text->broken != D_GIT_TEXT_NONE) &&
             (line >= _text->broken)            &&
             ( (line == _text->broken) ||
               (record->kind != D_GIT_TEXT_BLANK) ) )
        {
            record->kind = D_GIT_TEXT_REFUSED;
        }
    }

    return;
}


/*
d_internal_read_config
  Reads a configuration, then tells its lines what they are. Where git
would stop reading, so does this, and the line is a problem.
*/
static bool
d_internal_read_config(
    struct d_git_text* _text
)
{
    struct d_internal_config config;

    memset(&config, 0, sizeof(config));
    config.text          = _text;
    config.cursor.text   = _text->text;
    config.cursor.length = _text->length;
    config.section       = D_GIT_TEXT_NONE;
    config.error         = D_GIT_TEXT_NONE;
    config.stop          = D_GIT_TEXT_NONE;
    d_internal_config_walk(&config);
    d_git_buffer_free(&config.prefix);

    // memory that ran out fails the reading
    if (config.failed)
    {
        return false;
    }

    // a line git cannot read: git refuses the file from there
    if (config.error != D_GIT_TEXT_NONE)
    {
        char what[64];

        _text->broken = d_git_text_line_of(_text, config.error);
        _text->broken = (_text->broken == D_GIT_TEXT_NONE)
                        ? 0u
                        : _text->broken;
        (void)snprintf(what,
                       sizeof(what),
                       "bad config line %zu",
                       _text->broken + ((config.past_end) ? 2u : 1u));

        // the problem, if there is a line to hold it
        if ( (_text->line_count > 0u) &&
             (!d_internal_problem(_text, _text->broken, what)) )
        {
            return false;
        }
    }

    d_internal_config_lines(_text);

    return true;
}


//==============================================================================
// 6.  READING
//==============================================================================


bool
d_git_text_read(
    struct d_git_text*   _out,
    const char*          _text,
    size_t               _length,
    enum d_git_text_kind _kind,
    char*                _problem,
    size_t               _capacity
)
{
    memset(_out, 0, sizeof(*_out));
    _out->text   = (_text) ? _text : "";
    _out->length = (_text) ? _length : 0u;
    _out->kind   = _kind;
    _out->broken = D_GIT_TEXT_NONE;
    _out->bom    = ( (_out->length >= 3u) &&
                     (memcmp(_out->text, D_INTERNAL_BOM, 3u) == 0) );

    // a kind that is read
    if ( (_kind != D_GIT_TEXT_IGNORE)     &&
         (_kind != D_GIT_TEXT_ATTRIBUTES) &&
         (_kind != D_GIT_TEXT_CONFIG) )
    {
        (void)snprintf(_problem, _capacity, "not a kind of git file");

        return false;
    }

    bool read = d_internal_split(_out);

    // a configuration is read as a whole; the others a line at a time
    if ( (read) &&
         (_kind == D_GIT_TEXT_CONFIG) )
    {
        read = d_internal_read_config(_out);
    }
    else if (read)
    {
        // each line, as its kind reads it
        for (size_t line = 0u; ( (read) && (line < _out->line_count) ); ++line)
        {
            read = (_kind == D_GIT_TEXT_IGNORE)
                   ? d_internal_read_ignore_line(_out, line)
                   : d_internal_read_attribute_line(_out, line);
        }
    }

    // only memory fails a reading
    if (!read)
    {
        (void)snprintf(_problem, _capacity, "out of memory");
    }

    return read;
}


void
d_git_text_release(
    struct d_git_text* _text
)
{
    // nothing to release
    if (!_text)
    {
        return;
    }

    free(_text->lines);
    free(_text->patterns);
    free(_text->attributes);
    free(_text->sections);
    free(_text->entries);
    free(_text->problems);
    free(_text->strings);
    memset(_text, 0, sizeof(*_text));
    _text->broken = D_GIT_TEXT_NONE;

    return;
}


//==============================================================================
// 7.  RECORDS
//==============================================================================


const char*
d_git_text_string(
    const struct d_git_text* _text,
    size_t                   _offset
)
{
    // no string there
    if ( (!_text)                           ||
         (_offset == D_GIT_TEXT_NONE)       ||
         (_offset >= _text->strings_length) )
    {
        return NULL;
    }

    return &_text->strings[_offset];
}


/*
d_git_text_line_of
  A binary search: the lines are in order, and each begins where the one
before it ends. An offset at the text's end is on its last line.
*/
size_t
d_git_text_line_of(
    const struct d_git_text* _text,
    size_t                   _offset
)
{
    // no lines, no line
    if ( (!_text) ||
         (_text->line_count == 0u) )
    {
        return D_GIT_TEXT_NONE;
    }

    size_t low  = 0u;
    size_t high = _text->line_count - 1u;

    // the last line that begins at or before the offset
    while (low < high)
    {
        const size_t middle = low + ((high - low + 1u) / 2u);

        // past the offset: the line is before
        if (_text->lines[middle].start > _offset)
        {
            high = middle - 1u;
        }
        else
        {
            low = middle;
        }
    }

    return low;
}


//==============================================================================
// 8.  MATCHING
//==============================================================================


// d_internal_wild
//   struct: a wildmatch in progress: the pattern from its start to its
//   end, the text to its end, and the flags.
struct d_internal_wild
{
    const unsigned char* pattern;
    const unsigned char* pattern_end;
    const unsigned char* text_end;
    unsigned             flags;
};


/*
d_internal_at
  The byte at `_at`, or 0 at or past `_end`: what a C string's terminator
would be, where git's wildmatch reads one.
*/
static unsigned char
d_internal_at(
    const unsigned char* _at,
    const unsigned char* _end
)
{
    return (_at < _end) ? *_at : 0u;
}


/*
d_internal_fold, d_internal_upper_of, d_internal_is_upper,
d_internal_is_lower
  ASCII's letters, in the case wildmatch compares them in.
*/
static unsigned char
d_internal_fold(
    unsigned char _c
)
{
    return ( (_c >= 'A') && (_c <= 'Z') ) ? (unsigned char)(_c + 32u) : _c;
}


static unsigned char
d_internal_upper_of(
    unsigned char _c
)
{
    return ( (_c >= 'a') && (_c <= 'z') ) ? (unsigned char)(_c - 32u) : _c;
}


static bool
d_internal_is_upper(
    unsigned char _c
)
{
    return ( (_c >= 'A') && (_c <= 'Z') );
}


static bool
d_internal_is_lower(
    unsigned char _c
)
{
    return ( (_c >= 'a') && (_c <= 'z') );
}


/*
d_internal_glob_special
  Whether a byte is one git's glob treats specially: `*`, `?`, `[`, `\`.
*/
static bool
d_internal_glob_special(
    unsigned char _c
)
{
    return ( (_c == '*') ||
             (_c == '?') ||
             (_c == '[') ||
             (_c == '\\') );
}


// d_internal_class
//   struct: one of the classes wildmatch knows, [:name:], and whether a
//   byte is in it.
struct d_internal_class
{
    const char* name;
    bool      (*holds)(unsigned char _c);
};


/*
d_internal_class_alnum ... d_internal_class_xdigit
  The classes as git's own ctype has them, which no locale moves: ASCII
only, white space without `\v` and `\f`, and blanks the space and tab.
*/
static bool
d_internal_class_alpha(
    unsigned char _c
)
{
    return ( (d_internal_is_upper(_c)) ||
             (d_internal_is_lower(_c)) );
}


static bool
d_internal_class_digit(
    unsigned char _c
)
{
    return ( (_c >= '0') && (_c <= '9') );
}


static bool
d_internal_class_alnum(
    unsigned char _c
)
{
    return ( (d_internal_class_alpha(_c)) ||
             (d_internal_class_digit(_c)) );
}


static bool
d_internal_class_blank(
    unsigned char _c
)
{
    return ( (_c == ' ') ||
             (_c == '\t') );
}


static bool
d_internal_class_cntrl(
    unsigned char _c
)
{
    return ( (_c < 0x20u) ||
             (_c == 0x7Fu) );
}


static bool
d_internal_class_graph(
    unsigned char _c
)
{
    return ( (_c > 0x20u) && (_c < 0x7Fu) );
}


static bool
d_internal_class_print(
    unsigned char _c
)
{
    return ( (_c >= 0x20u) && (_c < 0x7Fu) );
}


static bool
d_internal_class_punct(
    unsigned char _c
)
{
    return ( (d_internal_class_graph(_c)) &&
             (!d_internal_class_alnum(_c)) );
}


static bool
d_internal_class_space(
    unsigned char _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\n') ||
             (_c == '\r') );
}


static bool
d_internal_class_xdigit(
    unsigned char _c
)
{
    return ( (d_internal_class_digit(_c))     ||
             ( (_c >= 'a') && (_c <= 'f') )   ||
             ( (_c >= 'A') && (_c <= 'F') ) );
}


// d_internal_classes
//   constant: the classes wildmatch knows, but upper and lower, which
//   case folding changes.
static const struct d_internal_class d_internal_classes[] =
{
    { "alnum",  d_internal_class_alnum  },
    { "alpha",  d_internal_class_alpha  },
    { "blank",  d_internal_class_blank  },
    { "cntrl",  d_internal_class_cntrl  },
    { "digit",  d_internal_class_digit  },
    { "graph",  d_internal_class_graph  },
    { "print",  d_internal_class_print  },
    { "punct",  d_internal_class_punct  },
    { "space",  d_internal_class_space  },
    { "xdigit", d_internal_class_xdigit }
};


// D_INTERNAL_GO_ON
//   constant: what a part of wildmatch answers when the match goes on
//   past it, the pattern and text moved.
#define D_INTERNAL_GO_ON 2

static int d_internal_wild_match(const struct d_internal_wild* _wild,
                                 const unsigned char*          _p,
                                 const unsigned char*          _t);


/*
d_internal_named_class
  Whether a byte is in the class [:name:], `_length` bytes from `_name`;
-1 for a name wildmatch does not know, which it abandons the match for.
Folding makes [:upper:] hold the lower case too; [:lower:] holds what
folding has already made lower.
*/
static int
d_internal_named_class(
    const unsigned char* _name,
    size_t               _length,
    unsigned char        _tc,
    bool                 _fold
)
{
    // upper is the class folding changes
    if ( (_length == 5u) &&
         (memcmp(_name, "upper", 5u) == 0) )
    {
        return ( (d_internal_is_upper(_tc)) ||
                 ( (_fold) && (d_internal_is_lower(_tc)) ) ) ? 1 : 0;
    }

    // lower is asked of the byte as it is
    if ( (_length == 5u) &&
         (memcmp(_name, "lower", 5u) == 0) )
    {
        return (d_internal_is_lower(_tc)) ? 1 : 0;
    }

    // the rest, by name
    for (size_t at = 0u;
         at < (sizeof(d_internal_classes) / sizeof(d_internal_classes[0]));
         ++at)
    {
        const char* const name = d_internal_classes[at].name;

        // the name, whole
        if ( (strlen(name) == _length) &&
             (memcmp(_name, name, _length) == 0) )
        {
            return (d_internal_classes[at].holds(_tc)) ? 1 : 0;
        }
    }

    return -1;
}


// d_internal_bracket
//   struct: a [class] being read: the pattern's place, its byte, the one
//   before it -- 0 when a range or a class has just ended -- the text's
//   byte, and whether it has matched.
struct d_internal_bracket
{
    const unsigned char* p;
    unsigned char        pc;
    unsigned char        previous;
    unsigned char        tc;
    bool                 matched;
};


/*
d_internal_bracket_range
  A range, a-z, from the byte before the `-`: the text's byte in it, or,
folded, its upper case in it.
*/
static int
d_internal_bracket_range(
    const struct d_internal_wild* _wild,
    struct d_internal_bracket*    _bracket
)
{
    _bracket->pc = d_internal_at(++_bracket->p, _wild->pattern_end);

    // an escaped end of the range
    if (_bracket->pc == '\\')
    {
        _bracket->pc = d_internal_at(++_bracket->p, _wild->pattern_end);

        // a backslash that ends the pattern
        if (_bracket->pc == 0u)
        {
            return D_INTERNAL_ABORT_ALL;
        }
    }

    const unsigned char up = d_internal_upper_of(_bracket->tc);

    // in the range, or, folded, its upper case in it
    if ( ( (_bracket->tc <= _bracket->pc) &&
           (_bracket->tc >= _bracket->previous) ) ||
         ( ((_wild->flags & D_GIT_TEXT_CASEFOLD) != 0u) &&
           (d_internal_is_lower(_bracket->tc))         &&
           (up <= _bracket->pc)                         &&
           (up >= _bracket->previous) ) )
    {
        _bracket->matched = true;
    }

    _bracket->pc = 0u;

    return D_INTERNAL_GO_ON;
}


/*
d_internal_bracket_class
  A [:name:] within brackets. Without its closing ":]" the `[` is a byte
like any other, and reading goes on from the `:` after it.
*/
static int
d_internal_bracket_class(
    const struct d_internal_wild* _wild,
    struct d_internal_bracket*    _bracket
)
{
    const unsigned char* const name = _bracket->p + 2;
    const unsigned char*       at   = name;

    // to the `]` that might close it
    while ( (d_internal_at(at, _wild->pattern_end) != 0u) &&
            (*at != ']') )
    {
        ++at;
    }

    // a class never closed
    if (d_internal_at(at, _wild->pattern_end) == 0u)
    {
        return D_INTERNAL_ABORT_ALL;
    }

    // no ":]": the `[` is itself
    if ( (at == name) ||
         (at[-1] != ':') )
    {
        _bracket->pc      = '[';
        _bracket->matched = ( (_bracket->matched) ||
                              (_bracket->tc == '[') );

        return D_INTERNAL_GO_ON;
    }

    const int holds = d_internal_named_class(name,
                                             (size_t)(at - name) - 1u,
                                             _bracket->tc,
                                             ((_wild->flags
                                               & D_GIT_TEXT_CASEFOLD) != 0u));

    // a class wildmatch does not know
    if (holds < 0)
    {
        return D_INTERNAL_ABORT_ALL;
    }

    _bracket->matched = ( (_bracket->matched) ||
                          (holds > 0) );
    _bracket->p       = at;
    _bracket->pc      = 0u;

    return D_INTERNAL_GO_ON;
}


/*
d_internal_bracket_item
  One item within brackets, at the bracket's byte: an escaped byte, a
range, a [:class:], or a byte.
*/
static int
d_internal_bracket_item(
    const struct d_internal_wild* _wild,
    struct d_internal_bracket*    _bracket
)
{
    const unsigned char next = d_internal_at(_bracket->p + 1,
                                             _wild->pattern_end);

    // brackets never closed
    if (_bracket->pc == 0u)
    {
        return D_INTERNAL_ABORT_ALL;
    }

    // an escaped byte, a range, a class, or a byte
    if (_bracket->pc == '\\')
    {
        _bracket->pc = d_internal_at(++_bracket->p, _wild->pattern_end);

        // a backslash that ends the pattern
        if (_bracket->pc == 0u)
        {
            return D_INTERNAL_ABORT_ALL;
        }

        _bracket->matched = ( (_bracket->matched) ||
                              (_bracket->tc == _bracket->pc) );
    }
    else if ( (_bracket->pc == '-')   &&
              (_bracket->previous)    &&
              (next != 0u)            &&
              (next != ']') )
    {
        return d_internal_bracket_range(_wild, _bracket);
    }
    else if ( (_bracket->pc == '[') &&
              (next == ':') )
    {
        return d_internal_bracket_class(_wild, _bracket);
    }
    else if (_bracket->tc == _bracket->pc)
    {
        _bracket->matched = true;
    }

    return D_INTERNAL_GO_ON;
}


/*
d_internal_wild_bracket
  A [class], from its `[` at `*_p`: `!` or `^` first negates it, a `]`
first is a byte of it, and the first `]` after that closes it. With
D_GIT_TEXT_PATHNAME no class matches a `/`. Going on, `*_p` is left at
the closing `]`.
*/
static int
d_internal_wild_bracket(
    const struct d_internal_wild* _wild,
    const unsigned char**         _p,
    unsigned char                 _tc
)
{
    struct d_internal_bracket bracket;

    memset(&bracket, 0, sizeof(bracket));
    bracket.p  = *_p;
    bracket.tc = _tc;
    bracket.pc = d_internal_at(++bracket.p, _wild->pattern_end);
    bracket.pc = (bracket.pc == '^') ? (unsigned char)'!' : bracket.pc;

    const bool negated = (bracket.pc == '!');

    // past the negation
    if (negated)
    {
        bracket.pc = d_internal_at(++bracket.p, _wild->pattern_end);
    }

    // each item, until the `]` after the first
    for (;;)
    {
        const int item = d_internal_bracket_item(_wild, &bracket);

        // a pattern wildmatch abandons
        if (item != D_INTERNAL_GO_ON)
        {
            return item;
        }

        bracket.previous = bracket.pc;
        bracket.pc       = d_internal_at(++bracket.p, _wild->pattern_end);

        // the closing bracket
        if (bracket.pc == ']')
        {
            break;
        }
    }

    // matched as the class says, and never a `/` of a path
    if ( (bracket.matched == negated) ||
         ( ((_wild->flags & D_GIT_TEXT_PATHNAME) != 0u) &&
           (_tc == '/') ) )
    {
        return D_INTERNAL_NOMATCH;
    }

    *_p = bracket.p;

    return D_INTERNAL_GO_ON;
}


/*
d_internal_star_slashes
  Whether stars, the second at `_second` and the pattern going on at
`_after`, are a `**` that matches directories: at the pattern's start or
after a `/`, and at its end or before a `/`.
*/
static bool
d_internal_star_slashes(
    const struct d_internal_wild* _wild,
    const unsigned char*          _start,
    const unsigned char*          _second,
    const unsigned char*          _after
)
{
    const unsigned char after = d_internal_at(_after, _wild->pattern_end);

    return ( ( ((_second - _start) < 2) ||
               (_second[-2] == '/') )                          &&
             ( (after == 0u)  ||
               (after == '/') ||
               ( (after == '\\') &&
                 (d_internal_at(_after + 1, _wild->pattern_end) == '/') ) ) );
}


/*
d_internal_star_scan
  A star's search along the text, from `_t`, for where the rest of the
pattern, from `_p`, matches. A literal byte next in the pattern lets the
search leap to where the text has it; with `_slashes` false the star
takes no `/`, and a search that reaches one hands back to the nearest
`**`.
  Long as dowild's handling of a `*` is, which it follows step by step.
*/
static int
d_internal_star_scan(
    const struct d_internal_wild* _wild,
    const unsigned char*          _p,
    const unsigned char*          _t,
    bool                          _slashes
)
{
    const bool    fold = ((_wild->flags & D_GIT_TEXT_CASEFOLD) != 0u);
    unsigned char tc   = d_internal_at(_t, _wild->text_end);

    tc = (fold) ? d_internal_fold(tc) : tc;

    // each place along the text the rest might match from
    while (tc != 0u)
    {
        const unsigned char next = d_internal_at(_p, _wild->pattern_end);

        // a literal next: leap to the text's next one
        if (!d_internal_glob_special(next))
        {
            const unsigned char pc = (fold) ? d_internal_fold(next) : next;

            // along the text, but not past a `/` the star cannot take
            while ( ((tc = d_internal_at(_t, _wild->text_end)) != 0u) &&
                    ( (_slashes) || (tc != '/') ) )
            {
                tc = (fold) ? d_internal_fold(tc) : tc;

                // the literal
                if (tc == pc)
                {
                    break;
                }

                ++_t;
            }

            // no such byte left
            if (tc != pc)
            {
                return (_slashes)
                       ? D_INTERNAL_ABORT_ALL
                       : D_INTERNAL_ABORT_STARSTAR;
            }
        }

        const int matched = d_internal_wild_match(_wild, _p, _t);

        // a match, or an answer that ends the search
        if (matched != D_INTERNAL_NOMATCH)
        {
            // a `**` goes on past what hands back to it
            if ( (!_slashes) ||
                 (matched != D_INTERNAL_ABORT_STARSTAR) )
            {
                return matched;
            }
        }
        else if ( (!_slashes) &&
                  (tc == '/') )
        {
            return D_INTERNAL_ABORT_STARSTAR;
        }

        tc = d_internal_at(++_t, _wild->text_end);
    }

    return D_INTERNAL_ABORT_ALL;
}


/*
d_internal_wild_star
  Stars, from the first at `*_p`. Without D_GIT_TEXT_PATHNAME a star takes
anything; with it, one takes no `/`, and `**` alone between slashes takes
any number of directories -- none among them, tried first. A star that
ends the pattern takes the rest of the text; one before a `/` the rest of
the name, the match going on past the slash, `*_p` and `*_t` moved.
*/
static int
d_internal_wild_star(
    const struct d_internal_wild* _wild,
    const unsigned char*          _start,
    const unsigned char**         _p,
    const unsigned char**         _t
)
{
    const bool           pathname = ((_wild->flags & D_GIT_TEXT_PATHNAME)
                                     != 0u);
    const unsigned char* p        = *_p + 1;
    bool                 slashes  = !pathname;

    // two stars or more
    if (d_internal_at(p, _wild->pattern_end) == '*')
    {
        const unsigned char* const second = p;

        // past every star
        while (d_internal_at(++p, _wild->pattern_end) == '*')
        {
        }

        slashes = ( (!pathname) ||
                    (d_internal_star_slashes(_wild, _start, second, p)) );

        // `**/` first tries matching no directory at all
        if ( (pathname)                                    &&
             (slashes)                                     &&
             (d_internal_at(p, _wild->pattern_end) == '/') &&
             (d_internal_wild_match(_wild, p + 1, *_t) == D_INTERNAL_MATCH) )
        {
            return D_INTERNAL_MATCH;
        }
    }

    const unsigned char next = d_internal_at(p, _wild->pattern_end);

    // the pattern's end: the rest, if a star may take it
    if (next == 0u)
    {
        return ( (!slashes) &&
                 (memchr(*_t, '/', (size_t)(_wild->text_end - *_t))) )
               ? D_INTERNAL_ABORT_STARSTAR
               : D_INTERNAL_MATCH;
    }

    // one star before a `/`: the rest of the name
    if ( (!slashes) &&
         (next == '/') )
    {
        const unsigned char* const slash =
            memchr(*_t, '/', (size_t)(_wild->text_end - *_t));

        // no other name to match
        if (!slash)
        {
            return D_INTERNAL_ABORT_ALL;
        }

        *_p = p;
        *_t = slash;

        return D_INTERNAL_GO_ON;
    }

    return d_internal_star_scan(_wild, p, *_t, slashes);
}


/*
d_internal_wild_match
  Git's dowild, from the pattern at `_p` and the text at `_t`: a byte at a
time, `?` any but a `/` of a path, `\` the next byte as itself -- not
folded, as git leaves it -- stars and classes by their own parts. The
text running out first abandons the whole match; the pattern running out
first fails it. `_p` is where this match's pattern begins, which a `**`
is measured from.
  Long as dowild's loop is, which it follows.
*/
static int
d_internal_wild_match(
    const struct d_internal_wild* _wild,
    const unsigned char*          _p,
    const unsigned char*          _t
)
{
    const unsigned char* const start = _p;
    const bool fold     = ((_wild->flags & D_GIT_TEXT_CASEFOLD) != 0u);
    const bool pathname = ((_wild->flags & D_GIT_TEXT_PATHNAME) != 0u);

    // a pattern byte at a time
    for (; d_internal_at(_p, _wild->pattern_end) != 0u; ++_p, ++_t)
    {
        unsigned char pc = *_p;
        unsigned char tc = d_internal_at(_t, _wild->text_end);

        // the text ran out first
        if ( (tc == 0u) &&
             (pc != '*') )
        {
            return D_INTERNAL_ABORT_ALL;
        }

        tc = (fold) ? d_internal_fold(tc) : tc;
        pc = (fold) ? d_internal_fold(pc) : pc;

        // each kind of pattern byte
        if (pc == '?')
        {
            // anything but a path's `/`
            if ( (pathname) &&
                 (tc == '/') )
            {
                return D_INTERNAL_NOMATCH;
            }

            continue;
        }
        else if ( (pc == '*') ||
                  (pc == '[') )
        {
            const int part = (pc == '*')
                             ? d_internal_wild_star(_wild, start, &_p, &_t)
                             : d_internal_wild_bracket(_wild, &_p, tc);

            // a part that answers for the whole match
            if (part != D_INTERNAL_GO_ON)
            {
                return part;
            }

            continue;
        }
        else if (pc == '\\')
        {
            pc = d_internal_at(++_p, _wild->pattern_end);
        }

        // a literal byte
        if (tc != pc)
        {
            return D_INTERNAL_NOMATCH;
        }
    }

    return (d_internal_at(_t, _wild->text_end) != 0u)
           ? D_INTERNAL_NOMATCH
           : D_INTERNAL_MATCH;
}


/*
d_internal_end_at_nul
  Where text of `_length` bytes ends as a C string would: at its first NUL.
*/
static size_t
d_internal_end_at_nul(
    const char* _text,
    size_t      _length
)
{
    const char* const nul = (_length > 0u) ? memchr(_text, '\0', _length)
                                           : NULL;

    return (nul) ? (size_t)(nul - _text) : _length;
}


bool
d_git_text_wildmatch(
    const char* _pattern,
    size_t      _pattern_length,
    const char* _text,
    size_t      _text_length,
    unsigned    _flags
)
{
    // nothing to match with
    if ( (!_pattern) ||
         (!_text) )
    {
        return false;
    }

    const unsigned char* const pattern = (const unsigned char*)_pattern;
    const unsigned char* const text    = (const unsigned char*)_text;
    struct d_internal_wild     wild;

    wild.pattern     = pattern;
    wild.pattern_end = pattern + d_internal_end_at_nul(_pattern,
                                                       _pattern_length);
    wild.text_end    = text + d_internal_end_at_nul(_text, _text_length);
    wild.flags       = _flags;

    return (d_internal_wild_match(&wild, pattern, text) == D_INTERNAL_MATCH);
}


/*
d_internal_same_bytes
  Whether `_length` bytes are alike, in either case of ASCII's letters
when `_icase` says so, as git's fspathncmp compares them.
*/
static bool
d_internal_same_bytes(
    const char* _left,
    const char* _right,
    size_t      _length,
    bool        _icase
)
{
    // exactly, or folded
    if (!_icase)
    {
        return (memcmp(_left, _right, _length) == 0);
    }

    // a byte at a time, folded
    for (size_t at = 0u; at < _length; ++at)
    {
        // one that differs, folded
        if (d_internal_fold((unsigned char)_left[at])
                != d_internal_fold((unsigned char)_right[at]))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_simple_length
  How many bytes a pattern begins with that are not glob-special, as git's
simple_length counts them, but no more than its length.
*/
static size_t
d_internal_simple_length(
    const char* _pattern,
    size_t      _length
)
{
    size_t at = 0u;

    // to the first special byte
    while ( (at < _length) &&
            (_pattern[at] != '\0') &&
            (!d_internal_glob_special((unsigned char)_pattern[at])) )
    {
        ++at;
    }

    return at;
}


/*
d_internal_below
  Whether a path is below a scope's base: always, for the top's; for
another, when it begins with the base and a `/`.
*/
static bool
d_internal_below(
    const char*                    _base,
    const struct d_git_text_query* _query
)
{
    const size_t length = (_base) ? strlen(_base) : 0u;

    return ( (length == 0u) ||
             ( (_query->length > length)                       &&
               (_query->path[length] == '/')                   &&
               (d_internal_same_bytes(_query->path,
                                      _base,
                                      length,
                                      _query->icase)) ) );
}


/*
d_internal_match_pathname
  A pattern with a slash, as git's match_pathname matches it: a leading
`/` dropped, the path's part below the base compared, the literal bytes
the pattern begins with first and the rest by wildmatch -- so a `**` the
literal part ends at counts as the pattern's beginning, as it does in git.
*/
static bool
d_internal_match_pathname(
    const char*                    _pattern,
    size_t                         _length,
    size_t                         _base,
    const struct d_git_text_query* _query
)
{
    size_t       prefix  = d_internal_simple_length(_pattern, _length);
    const char*  pattern = _pattern;
    size_t       length  = _length;

    // a leading slash anchors, and is not matched
    if ( (length > 0u) &&
         (pattern[0] == '/') )
    {
        ++pattern;
        --length;
        --prefix;
    }

    const size_t skip     = (_base > 0u) ? (_base + 1u) : 0u;
    const char*  name     = &_query->path[skip];
    size_t       name_len = _query->length - skip;

    // the literal part, compared as names compare
    if (prefix > 0u)
    {
        // the literal part must fit, and be alike
        if ( (prefix > name_len) ||
             (!d_internal_same_bytes(pattern, name, prefix, _query->icase)) )
        {
            return false;
        }

        pattern  += prefix;
        length   -= prefix;
        name     += prefix;
        name_len -= prefix;

        // a pattern that was all literal
        if ( (length == 0u) &&
             (name_len == 0u) )
        {
            return true;
        }
    }

    return d_git_text_wildmatch(pattern,
                                length,
                                name,
                                name_len,
                                D_GIT_TEXT_PATHNAME
                                    | ((_query->icase)
                                       ? D_GIT_TEXT_CASEFOLD
                                       : 0u));
}


bool
d_git_text_matches(
    const struct d_git_text_scope* _scope,
    size_t                         _pattern,
    const struct d_git_text_query* _query
)
{
    // a pattern of the scope's, and a path
    if ( (!_scope)                                 ||
         (!_scope->text)                           ||
         (!_query)                                 ||
         (!_query->path)                           ||
         (_pattern >= _scope->text->pattern_count) ||
         (_query->length == 0u) )
    {
        return false;
    }

    const struct d_git_text_pattern* const pattern =
        &_scope->text->patterns[_pattern];

    // a macro matches nothing; a directory's pattern only a directory; and
    // a scope's patterns only paths below it
    if ( (pattern->macro)                                  ||
         ( (pattern->directory) && (!_query->directory) ) ||
         (!d_internal_below(_scope->base, _query)) )
    {
        return false;
    }

    const char* const text = d_git_text_string(_scope->text, pattern->text);

    // without a slash, the path's last name
    if (pattern->anywhere)
    {
        size_t name = _query->length;

        // back to the last `/`
        while ( (name > 0u) &&
                (_query->path[name - 1u] != '/') )
        {
            --name;
        }

        return d_git_text_wildmatch(text,
                                    pattern->length,
                                    &_query->path[name],
                                    _query->length - name,
                                    (_query->icase)
                                    ? D_GIT_TEXT_CASEFOLD
                                    : 0u);
    }

    return d_internal_match_pathname(text,
                                     pattern->length,
                                     (_scope->base) ? strlen(_scope->base)
                                                    : 0u,
                                     _query);
}


//==============================================================================
// 9.  IGNORING
//==============================================================================


/*
d_internal_last_match
  The last pattern that matches a path, in the last scope that has one,
as git's last_matching_pattern_from_lists finds it.
*/
static bool
d_internal_last_match(
    const struct d_git_text_scope* _scopes,
    size_t                         _count,
    const struct d_git_text_query* _query,
    struct d_git_text_match*       _out
)
{
    // the highest scope first
    for (size_t scope = _count; scope-- > 0u;)
    {
        const struct d_git_text* const text = _scopes[scope].text;

        // a scope with nothing in it
        if (!text)
        {
            continue;
        }

        // its last pattern first
        for (size_t pattern = text->pattern_count; pattern-- > 0u;)
        {
            // the first to match decides
            if (d_git_text_matches(&_scopes[scope], pattern, _query))
            {
                _out->scope   = scope;
                _out->pattern = pattern;
                _out->ignored = !text->patterns[pattern].negated;
                _out->through = _query->length;

                return true;
            }
        }
    }

    return false;
}


/*
d_internal_normal_query
  A query whose path has no trailing `/`: one it had says it is a
directory's.
*/
static struct d_git_text_query
d_internal_normal_query(
    const struct d_git_text_query* _query
)
{
    struct d_git_text_query query = *_query;

    // trailing slashes say a directory
    while ( (query.length > 0u) &&
            (query.path[query.length - 1u] == '/') )
    {
        --query.length;
        query.directory = true;
    }

    return query;
}


/*
d_git_text_ignored
  Each directory above the path is asked first, as git's prep_exclude asks
it while pushing each directory's .gitignore: one ignored -- not by a
negated pattern -- decides for the path. A scope applies to a directory
only when the directory is below the scope's base, so a directory's own
.gitignore never decides for the directory.
*/
bool
d_git_text_ignored(
    const struct d_git_text_scope* _scopes,
    size_t                         _count,
    const struct d_git_text_query* _query,
    struct d_git_text_match*       _out
)
{
    // something to ask, and somewhere to answer
    if ( (!_query)       ||
         (!_query->path) ||
         (!_out) )
    {
        return false;
    }

    const struct d_git_text_query query = d_internal_normal_query(_query);

    memset(_out, 0, sizeof(*_out));
    _out->scope   = D_GIT_TEXT_NONE;
    _out->pattern = D_GIT_TEXT_NONE;

    // each directory above the path, from the top down
    for (size_t at = 1u; at < query.length; ++at)
    {
        // a directory ends at each `/`
        if (query.path[at] != '/')
        {
            continue;
        }

        struct d_git_text_query directory = query;

        directory.length    = at;
        directory.directory = true;

        // an ignored directory decides for everything below it
        if ( (d_internal_last_match(_scopes, _count, &directory, _out)) &&
             (_out->ignored) )
        {
            return true;
        }
    }

    // the path itself
    if (d_internal_last_match(_scopes, _count, &query, _out))
    {
        return true;
    }

    _out->scope   = D_GIT_TEXT_NONE;
    _out->pattern = D_GIT_TEXT_NONE;
    _out->ignored = false;
    _out->through = query.length;

    return false;
}


//==============================================================================
// 10. ATTRIBUTES
//==============================================================================


// D_INTERNAL_BUILTIN
//   constant: the attributes git knows before reading any file.
#define D_INTERNAL_BUILTIN "[attr]binary -diff -merge -text\n"

// d_internal_attrs
//   struct: a path's attributes being found: the scopes, git's own first,
//   and what has been decided -- unspecified among it, which lines below
//   do not override.
struct d_internal_attrs
{
    const struct d_git_text_scope* scopes;
    size_t                         count;
    const struct d_git_text_query* query;
    struct d_git_text_values       decided;
    bool                           failed;
};


/*
d_internal_decided_of
  The index of an attribute among those decided, by name -- names compare
exactly -- or D_GIT_TEXT_NONE.
*/
static size_t
d_internal_decided_of(
    const struct d_git_text_values* _values,
    const char*                     _name,
    size_t                          _length
)
{
    // each decided so far
    for (size_t at = 0u; at < _values->count; ++at)
    {
        // the same name, whole
        if ( (_values->items[at].name_length == _length) &&
             (memcmp(_values->items[at].name, _name, _length) == 0) )
        {
            return at;
        }
    }

    return D_GIT_TEXT_NONE;
}


/*
d_internal_macro_of
  The macro of a name that counts, as git's determine_macros finds it: the
last defined, in the highest scope that allows macros; false when none is.
*/
static bool
d_internal_macro_of(
    const struct d_internal_attrs* _attrs,
    const char*                    _name,
    size_t                         _length,
    size_t*                        _out_scope,
    size_t*                        _out_pattern
)
{
    // the highest scope first
    for (size_t scope = _attrs->count; scope-- > 0u;)
    {
        const struct d_git_text* const text = _attrs->scopes[scope].text;

        // a scope whose macros count
        if ( (!text) ||
             (!_attrs->scopes[scope].macros) )
        {
            continue;
        }

        // its last macro first
        for (size_t pattern = text->pattern_count; pattern-- > 0u;)
        {
            const struct d_git_text_pattern* const record =
                &text->patterns[pattern];

            // a macro of the name
            if ( (record->macro)               &&
                 (record->length == _length)   &&
                 (memcmp(d_git_text_string(text, record->text),
                         _name,
                         _length) == 0) )
            {
                *_out_scope   = scope;
                *_out_pattern = pattern;

                return true;
            }
        }
    }

    return false;
}


/*
d_internal_decide
  Decides what a line's states say of the attributes not yet decided, its
last state first, as git's fill_one does; one set that is a macro decides
what the macro says, the same way.
  Long as fill's walk over the scopes and their lines, the last first, is,
macros and all.
*/
static void
d_internal_decide(
    struct d_internal_attrs* _attrs,
    size_t                   _scope,
    size_t                   _pattern
)
{
    const struct d_git_text* const         text   = _attrs->scopes[_scope].text;
    const struct d_git_text_pattern* const record = &text->patterns[_pattern];

    // the line's states, its last first
    for (size_t at = record->attribute_count; at-- > 0u;)
    {
        const struct d_git_text_attribute* const state =
            &text->attributes[record->first_attribute + at];
        const char* const name   = &text->text[state->name_start];
        const size_t      length = state->name_end - state->name_start;

        // a higher line decided it already
        if (d_internal_decided_of(&_attrs->decided, name, length)
                != D_GIT_TEXT_NONE)
        {
            continue;
        }

        // room for one more
        if (!d_internal_grow((void**)&_attrs->decided.items,
                             _attrs->decided.count,
                             &_attrs->decided.capacity,
                             sizeof(*_attrs->decided.items)))
        {
            _attrs->failed = true;

            return;
        }

        struct d_git_text_value* const value =
            &_attrs->decided.items[_attrs->decided.count];

        value->name         = name;
        value->name_length  = length;
        value->state        = state->state;
        value->value        = (state->state == D_GIT_TEXT_VALUE)
                              ? &text->text[state->value_start]
                              : NULL;
        value->value_length = (state->state == D_GIT_TEXT_VALUE)
                              ? (state->value_end - state->value_start)
                              : 0u;
        value->scope        = (_scope == 0u) ? D_GIT_TEXT_NONE
                                             : (_scope - 1u);
        value->pattern      = _pattern;
        ++_attrs->decided.count;

        size_t macro_scope   = 0u;
        size_t macro_pattern = 0u;

        // a macro set says more
        if ( (state->state == D_GIT_TEXT_SET) &&
             (d_internal_macro_of(_attrs,
                                  name,
                                  length,
                                  &macro_scope,
                                  &macro_pattern)) )
        {
            d_internal_decide(_attrs, macro_scope, macro_pattern);
        }
    }

    return;
}


/*
d_internal_compare_values
  Orders attributes by name, as bytes, a shorter name first.
*/
static int
d_internal_compare_values(
    const void* _left,
    const void* _right
)
{
    const struct d_git_text_value* const left  = _left;
    const struct d_git_text_value* const right = _right;
    const size_t shorter = (left->name_length < right->name_length)
                           ? left->name_length
                           : right->name_length;
    const int    order   = memcmp(left->name, right->name, shorter);

    // the bytes they share, then their lengths
    if (order != 0)
    {
        return order;
    }

    return (left->name_length < right->name_length)
           ? -1
           : ( (left->name_length > right->name_length) ? 1 : 0 );
}


/*
d_internal_fill
  Walks the scopes, highest first, each pattern of a scope the path is
below, its last first, and decides from each that matches.
*/
static void
d_internal_fill(
    struct d_internal_attrs* _attrs
)
{
    // the highest scope first
    for (size_t scope = _attrs->count;
         ( (scope-- > 0u) && (!_attrs->failed) );)
    {
        const struct d_git_text* const text = _attrs->scopes[scope].text;

        // a scope with nothing in it
        if (!text)
        {
            continue;
        }

        // its last pattern first
        for (size_t pattern = text->pattern_count;
             ( (pattern-- > 0u) && (!_attrs->failed) );)
        {
            // a pattern that matches decides
            if (d_git_text_matches(&_attrs->scopes[scope],
                                   pattern,
                                   _attrs->query))
            {
                d_internal_decide(_attrs, scope, pattern);
            }
        }
    }

    return;
}


/*
d_internal_keep_specified
  Leaves among the decided only what is specified, ordered by name.
*/
static void
d_internal_keep_specified(
    struct d_git_text_values* _values
)
{
    size_t kept = 0u;

    // the unspecified are dropped
    for (size_t at = 0u; at < _values->count; ++at)
    {
        // kept in their order
        if (_values->items[at].state != D_GIT_TEXT_UNSPECIFIED)
        {
            _values->items[kept] = _values->items[at];
            ++kept;
        }
    }

    _values->count = kept;

    // ordered by name
    if (kept > 1u)
    {
        qsort(_values->items,
              kept,
              sizeof(*_values->items),
              d_internal_compare_values);
    }

    return;
}


/*
d_git_text_attributes
  Git's own macro is a scope of its own, read here, below the caller's:
the scopes are copied after it, and the scope a value names is the
caller's index.
*/
bool
d_git_text_attributes(
    const struct d_git_text_scope* _scopes,
    size_t                         _count,
    const struct d_git_text_query* _query,
    struct d_git_text_values*      _out
)
{
    // somewhere to answer, and a path
    if ( (!_out)   ||
         (!_query) ||
         (!_query->path) )
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));

    struct d_git_text_scope* const scopes =
        malloc((_count + 1u) * sizeof(*scopes));
    struct d_git_text              builtin;
    char                           problem[64];

    memset(&builtin, 0, sizeof(builtin));

    // room for the scopes, and git's own read
    if ( (!scopes) ||
         (!d_git_text_read(&builtin,
                           D_INTERNAL_BUILTIN,
                           strlen(D_INTERNAL_BUILTIN),
                           D_GIT_TEXT_ATTRIBUTES,
                           problem,
                           sizeof(problem))) )
    {
        free(scopes);
        d_git_text_release(&builtin);

        return false;
    }

    scopes[0].text   = &builtin;
    scopes[0].base   = "";
    scopes[0].macros = true;

    // the caller's above git's own
    if (_count > 0u)
    {
        memcpy(&scopes[1], _scopes, _count * sizeof(*scopes));
    }

    const struct d_git_text_query query = d_internal_normal_query(_query);
    struct d_internal_attrs       attrs;

    memset(&attrs, 0, sizeof(attrs));
    attrs.scopes = scopes;
    attrs.count  = _count + 1u;
    attrs.query  = &query;
    d_internal_fill(&attrs);
    free(scopes);
    d_git_text_release(&builtin);
    d_internal_keep_specified(&attrs.decided);
    *_out = attrs.decided;

    return !attrs.failed;
}


void
d_git_text_values_release(
    struct d_git_text_values* _values
)
{
    // nothing to release
    if (!_values)
    {
        return;
    }

    free(_values->items);
    memset(_values, 0, sizeof(*_values));

    return;
}


const struct d_git_text_value*
d_git_text_value_of(
    const struct d_git_text_values* _values,
    const char*                     _name
)
{
    // nothing to look in, or for
    if ( (!_values) ||
         (!_name) )
    {
        return NULL;
    }

    const size_t at = d_internal_decided_of(_values, _name, strlen(_name));

    return (at == D_GIT_TEXT_NONE) ? NULL : &_values->items[at];
}


//==============================================================================
// 11. CONFIGURATION
//==============================================================================


/*
d_git_text_key
  As git's git_config_parse_key: the section is up to the first dot, the
name past the last, and what is between them the subsection, left as it
is but for a newline, which git refuses.
*/
bool
d_git_text_key(
    struct d_git_buffer* _out,
    const char*          _key
)
{
    // a key to read
    if ( (!_out) ||
         (!_key) )
    {
        return false;
    }

    const char* const last_dot = strrchr(_key, '.');

    // a section and a name, apart by a dot
    if ( (!last_dot)        ||
         (last_dot == _key) ||
         (last_dot[1] == '\0') )
    {
        return false;
    }

    const size_t base = (size_t)(last_dot - _key);
    bool         dot  = false;

    // each byte, the section's and the name's in lower case
    for (size_t at = 0u; _key[at] != '\0'; ++at)
    {
        const int c = (unsigned char)_key[at];

        dot = ( (dot) || (c == '.') );

        // the section and the name hold a key's bytes; the name begins
        // with a letter; the subsection holds anything but a newline
        if ( (!dot) ||
             (at > base) )
        {
            // a byte a key does not hold
            if ( (!d_internal_keychar(c)) ||
                 ( (at == (base + 1u)) && (!d_internal_alpha(c)) ) )
            {
                return false;
            }

            (void)d_git_buffer_add_char(_out, d_internal_lower(c));
        }
        else if (c == '\n')
        {
            return false;
        }
        else
        {
            (void)d_git_buffer_add_char(_out, (char)c);
        }
    }

    return !_out->failed;
}


void
d_git_text_key_parts(
    const char* _key,
    size_t*     _out_section,
    size_t*     _out_subsection
)
{
    const char* const first = (_key) ? strchr(_key, '.') : NULL;
    const char* const last  = (_key) ? strrchr(_key, '.') : NULL;

    *_out_section    = (first) ? (size_t)(first - _key)
                               : ( (_key) ? strlen(_key) : 0u );
    *_out_subsection = ( (first) && (last != first) )
                       ? (size_t)(last - first) - 1u
                       : D_GIT_TEXT_NONE;

    return;
}


size_t
d_git_text_find_entry(
    const struct d_git_text* _text,
    const char*              _key,
    size_t                   _after
)
{
    // somewhere to look, and something to look for
    if ( (!_text) ||
         (!_key) )
    {
        return D_GIT_TEXT_NONE;
    }

    // from the entry after `_after`
    for (size_t at = (_after == D_GIT_TEXT_NONE) ? 0u : (_after + 1u);
         at < _text->entry_count;
         ++at)
    {
        // the key, exactly
        if (strcmp(d_git_text_string(_text, _text->entries[at].key),
                   _key) == 0)
        {
            return at;
        }
    }

    return D_GIT_TEXT_NONE;
}


size_t
d_git_text_find_section(
    const struct d_git_text* _text,
    const char*              _prefix,
    size_t                   _after
)
{
    // somewhere to look, and something to look for
    if ( (!_text) ||
         (!_prefix) )
    {
        return D_GIT_TEXT_NONE;
    }

    // from the section after `_after`
    for (size_t at = (_after == D_GIT_TEXT_NONE) ? 0u : (_after + 1u);
         at < _text->section_count;
         ++at)
    {
        // the prefix, exactly
        if (strcmp(d_git_text_string(_text, _text->sections[at].prefix),
                   _prefix) == 0)
        {
            return at;
        }
    }

    return D_GIT_TEXT_NONE;
}


// d_internal_number
//   struct: a number being read as strtoimax reads one: its sign, its
//   magnitude, and where reading stopped.
struct d_internal_number
{
    bool        negative;
    uint64_t    magnitude;
    const char* end;
    bool        read;
    bool        overflow;
};


/*
d_internal_digit_of
  A digit's worth in a base, or -1 for a byte that is not one of its.
*/
static int
d_internal_digit_of(
    char     _c,
    unsigned _base
)
{
    const int worth = ( (_c >= '0') && (_c <= '9') ) ? (_c - '0')
                    : ( (_c >= 'a') && (_c <= 'f') ) ? (_c - 'a' + 10)
                    : ( (_c >= 'A') && (_c <= 'F') ) ? (_c - 'A' + 10)
                    : -1;

    return ( (worth >= 0) && ((unsigned)worth < _base) ) ? worth : -1;
}


/*
d_internal_read_number
  Reads a number as strtoimax with a base of 0 does: white space, a sign,
then 0x and hex, 0 and octal, or decimal. A magnitude past 64 bits is an
overflow.
*/
static struct d_internal_number
d_internal_read_number(
    const char* _text
)
{
    struct d_internal_number number;
    const char*              at   = _text;
    unsigned                 base = 10u;

    memset(&number, 0, sizeof(number));
    number.end = _text;

    // C's white space
    while ( (*at == ' ')  ||
            (*at == '\t') ||
            (*at == '\n') ||
            (*at == '\v') ||
            (*at == '\f') ||
            (*at == '\r') )
    {
        ++at;
    }

    // a sign
    if ( (*at == '+') ||
         (*at == '-') )
    {
        number.negative = (*at == '-');
        ++at;
    }

    // the base: 0x and a hex digit, or a leading 0
    if ( (at[0] == '0')                         &&
         ( (at[1] == 'x') || (at[1] == 'X') )   &&
         (d_internal_digit_of(at[2], 16u) >= 0) )
    {
        base  = 16u;
        at   += 2;
    }
    else if (at[0] == '0')
    {
        base = 8u;
    }

    // the digits
    for (int worth = d_internal_digit_of(*at, base);
         worth >= 0;
         worth = d_internal_digit_of(*(++at), base))
    {
        // a magnitude past 64 bits
        if (number.magnitude > ((UINT64_MAX - (uint64_t)worth) / base))
        {
            number.overflow = true;
        }

        number.magnitude = (number.magnitude * base) + (uint64_t)worth;
        number.read      = true;
        number.end       = at + 1;
    }

    return number;
}


/*
d_internal_unit
  The factor git's units give: none 1, k 1024, m 1024^2, g 1024^3, in
either case; 0 for anything else.
*/
static uint64_t
d_internal_unit(
    const char* _unit
)
{
    // nothing, or one letter alone
    if (_unit[0] == '\0')
    {
        return 1u;
    }
    else if (_unit[1] != '\0')
    {
        return 0u;
    }

    const char lower = d_internal_lower((unsigned char)_unit[0]);

    return (lower == 'k') ? 1024u
         : (lower == 'm') ? (1024u * 1024u)
         : (lower == 'g') ? (1024u * 1024u * 1024u)
         : 0u;
}


/*
d_internal_scaled
  A value read as git's git_parse_signed reads it, no larger in magnitude
than `_most`: false for no number, a unit git does not know, or one too
large.
*/
static bool
d_internal_scaled(
    const char* _value,
    uint64_t    _most,
    int64_t*    _out
)
{
    // nothing is not a number
    if ( (!_value) ||
         (_value[0] == '\0') )
    {
        return false;
    }

    const struct d_internal_number number = d_internal_read_number(_value);
    const uint64_t                 factor = (number.read)
                                            ? d_internal_unit(number.end)
                                            : 0u;

    // a number, a unit, and room for both: git holds a negative one to
    // the same magnitude as a positive
    if ( (factor == 0u)       ||
         (number.overflow)    ||
         (number.magnitude > (_most / factor)) )
    {
        return false;
    }

    const int64_t magnitude = (int64_t)(number.magnitude * factor);

    *_out = (number.negative) ? -magnitude : magnitude;

    return true;
}


bool
d_git_text_int(
    const char* _value,
    int64_t*    _out
)
{
    return d_internal_scaled(_value, (uint64_t)INT64_MAX, _out);
}


bool
d_git_text_bool(
    const char* _value,
    bool*       _out
)
{
    int64_t number = 0;

    // a name alone is true; an empty value false
    if ( (!_value) ||
         (_value[0] == '\0') )
    {
        *_out = (_value == NULL);

        return true;
    }

    // the words, in any case
    if ( (d_internal_same_folded(_value, "true")) ||
         (d_internal_same_folded(_value, "yes"))  ||
         (d_internal_same_folded(_value, "on")) )
    {
        *_out = true;

        return true;
    }
    else if ( (d_internal_same_folded(_value, "false")) ||
              (d_internal_same_folded(_value, "no"))    ||
              (d_internal_same_folded(_value, "off")) )
    {
        *_out = false;

        return true;
    }

    // a number, as git's int reads it
    if (!d_internal_scaled(_value, 2147483647u, &number))
    {
        return false;
    }

    *_out = (number != 0);

    return true;
}


//==============================================================================
// 12. QUOTING
//==============================================================================


/*
d_git_text_quote_value
  As git's write_pair, with a backspace escaped and a `\r` quoted too, so
that what is written reads back as the value: a `\r` outside quotes is a
blank to git.
*/
bool
d_git_text_quote_value(
    struct d_git_buffer* _out,
    const char*          _value
)
{
    const char*  value  = (_value) ? _value : "";
    const size_t length = strlen(value);
    bool         quote  = ( (length > 0u) &&
                            ( (value[0] == ' ')            ||
                              (value[length - 1u] == ' ') ) );

    // a comment's mark, or a `\r`, is safe only in quotes
    for (size_t at = 0u; at < length; ++at)
    {
        quote = ( (quote)            ||
                  (value[at] == ';') ||
                  (value[at] == '#') ||
                  (value[at] == '\r') );
    }

    // the opening quote
    if (quote)
    {
        (void)d_git_buffer_add_char(_out, '"');
    }

    // each byte, escaped where it must be
    for (size_t at = 0u; at < length; ++at)
    {
        const char c = value[at];

        // the escapes git reads back
        if (c == '\n')
        {
            (void)d_git_buffer_add_text(_out, "\\n");
        }
        else if (c == '\t')
        {
            (void)d_git_buffer_add_text(_out, "\\t");
        }
        else if (c == '\b')
        {
            (void)d_git_buffer_add_text(_out, "\\b");
        }
        else if ( (c == '"') ||
                  (c == '\\') )
        {
            (void)d_git_buffer_add_char(_out, '\\');
            (void)d_git_buffer_add_char(_out, c);
        }
        else
        {
            (void)d_git_buffer_add_char(_out, c);
        }
    }

    // the closing quote
    if (quote)
    {
        (void)d_git_buffer_add_char(_out, '"');
    }

    return !_out->failed;
}


bool
d_git_text_quote_subsection(
    struct d_git_buffer* _out,
    const char*          _subsection
)
{
    // each byte, a quote or backslash escaped
    for (const char* at = (_subsection) ? _subsection : ""; *at; ++at)
    {
        // the two a subsection escapes
        if ( (*at == '"') ||
             (*at == '\\') )
        {
            (void)d_git_buffer_add_char(_out, '\\');
        }

        (void)d_git_buffer_add_char(_out, *at);
    }

    return !_out->failed;
}


/*
d_internal_must_quote
  Whether a byte of a path makes git quote it: a control byte, a quote, a
backslash, or a byte past ASCII.
*/
static bool
d_internal_must_quote(
    unsigned char _c
)
{
    return ( (_c < 0x20u) ||
             (_c >= 0x7Fu) ||
             (_c == '"')  ||
             (_c == '\\') );
}


/*
d_internal_quote_byte
  One byte of a quoted path: C's escape for it, or three octal digits.
*/
static void
d_internal_quote_byte(
    struct d_git_buffer* _out,
    unsigned char        _c
)
{
    static const char letters[] = "abtnvfr";

    (void)d_git_buffer_add_char(_out, '\\');

    // \a to \r, by letter; a quote and a backslash as themselves
    if ( (_c >= 7u) &&
         (_c <= 13u) )
    {
        (void)d_git_buffer_add_char(_out, letters[_c - 7u]);
    }
    else if ( (_c == '"') ||
              (_c == '\\') )
    {
        (void)d_git_buffer_add_char(_out, (char)_c);
    }
    else
    {
        (void)d_git_buffer_add_char(_out, (char)('0' + ((_c >> 6) & 3u)));
        (void)d_git_buffer_add_char(_out, (char)('0' + ((_c >> 3) & 7u)));
        (void)d_git_buffer_add_char(_out, (char)('0' + (_c & 7u)));
    }

    return;
}


bool
d_git_text_quote_path(
    struct d_git_buffer* _out,
    const char*          _path,
    size_t               _length
)
{
    bool quote = false;

    // whether any byte makes it quoted
    for (size_t at = 0u; at < _length; ++at)
    {
        quote = ( (quote) ||
                  (d_internal_must_quote((unsigned char)_path[at])) );
    }

    // a path that needs nothing is written as it is
    if (!quote)
    {
        return d_git_buffer_add(_out, _path, _length);
    }

    (void)d_git_buffer_add_char(_out, '"');

    // each byte, escaped where it must be
    for (size_t at = 0u; at < _length; ++at)
    {
        const unsigned char c = (unsigned char)_path[at];

        // escaped, or as it is
        if (d_internal_must_quote(c))
        {
            d_internal_quote_byte(_out, c);
        }
        else
        {
            (void)d_git_buffer_add_char(_out, (char)c);
        }
    }

    (void)d_git_buffer_add_char(_out, '"');

    return !_out->failed;
}


/*
d_internal_unescape
  The byte an escape in C's quotes stands for, from the byte after its
`\` at `*_at`, which it moves past; -1 for one git refuses: an unknown
letter, or octal not three digits from 0 to 3.
*/
static int
d_internal_unescape(
    const char* _text,
    size_t      _length,
    size_t*     _at
)
{
    static const char letters[] = "abfnrtv";
    static const char bytes[]   = "\a\b\f\n\r\t\v";

    // a byte to escape
    if (*_at >= _length)
    {
        return -1;
    }

    const char        c      = _text[*_at];
    const char* const letter = memchr(letters, c, 7u);

    ++*_at;

    // a letter, a quote or backslash, or octal
    if (letter)
    {
        return (unsigned char)bytes[letter - letters];
    }
    else if ( (c == '"') ||
              (c == '\\') )
    {
        return (unsigned char)c;
    }
    else if ( (c < '0') ||
              (c > '3') ||
              ((*_at + 2u) > _length) )
    {
        return -1;
    }

    const char second = _text[*_at];
    const char third  = _text[*_at + 1u];

    // two more octal digits
    if ( (second < '0') || (second > '7') ||
         (third < '0')  || (third > '7') )
    {
        return -1;
    }

    *_at += 2u;

    return ((c - '0') << 6) | ((second - '0') << 3) | (third - '0');
}


bool
d_git_text_unquote(
    struct d_git_buffer* _out,
    const char*          _text,
    size_t               _length,
    size_t*              _used
)
{
    // quoted text begins with a quote
    if ( (!_text)        ||
         (_length == 0u) ||
         (_text[0] != '"') )
    {
        return false;
    }

    const size_t kept = _out->length;
    size_t       at   = 1u;

    // to the closing quote
    while (at < _length)
    {
        const char c = _text[at];

        ++at;

        // the closing quote ends it
        if (c == '"')
        {
            *_used = at;

            return !_out->failed;
        }

        // a NUL ends git's string before its quote
        if (c == '\0')
        {
            break;
        }

        const int byte = (c == '\\')
                         ? d_internal_unescape(_text, _length, &at)
                         : (unsigned char)c;

        // an escape git refuses
        if (byte < 0)
        {
            break;
        }

        (void)d_git_buffer_add_char(_out, (char)byte);
    }

    _out->length = kept;

    // the buffer's text stays terminated where it was
    if (_out->text)
    {
        _out->text[kept] = '\0';
    }

    return false;
}
