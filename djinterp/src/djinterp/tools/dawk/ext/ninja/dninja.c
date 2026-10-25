/*******************************************************************************
* djinterp [tools]                                                      dninja.c
*
* Reads the Ninja build language into records: see dninja.h.
*   One pass over the text, by hand, as ninja's re2c lexer and its manifest
* parser make it: a token at a time -- a keyword, a name, `=`, `:`, the three
* pipes, an indent, a line ending -- with comments skipped wherever a token
* may start, and blanks and `$` line continuations eaten after every token
* but a line ending. A value is read as ninja's ReadEvalString reads one,
* in a path's sense or a value's. Where ninja's parser reports a fault, the
* reader reports it at the token ninja's lexer last read, which is the line
* ninja names.
*
* path:      /src/djinterp/tools/dawk/ext/ninja/dninja.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset, memchr, memcmp, strlen


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Tokens
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_token
//   enum: the tokens of ninja's lexer, in its order.
enum d_internal_token
{
    D_INTERNAL_ERROR = 0,
    D_INTERNAL_BUILD,
    D_INTERNAL_COLON,
    D_INTERNAL_DEFAULT,
    D_INTERNAL_EQUALS,
    D_INTERNAL_IDENT,
    D_INTERNAL_INCLUDE,
    D_INTERNAL_INDENT,
    D_INTERNAL_NEWLINE,
    D_INTERNAL_PIPE2,
    D_INTERNAL_PIPE,
    D_INTERNAL_PIPEAT,
    D_INTERNAL_POOL,
    D_INTERNAL_RULE,
    D_INTERNAL_SUBNINJA,
    D_INTERNAL_TEOF
};

// 1.2    The reader
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_reader
//   struct: the lexer's place -- `at`, ninja's ofs_, and `last`, its
//   last_token_ -- and the records it grows.
struct d_internal_reader
{
    struct d_ninja* ninja;
    const char*     text;
    size_t          length;
    size_t          at;
    size_t          last;
    size_t          statement_capacity;
    size_t          path_capacity;
    size_t          binding_capacity;
    size_t          value_capacity;
    size_t          piece_capacity;
    size_t          comment_capacity;
    char*           problem;
    size_t          capacity;
    bool            failed;
    bool            begun;
};


//==============================================================================
// 2.  RECORDS
//==============================================================================


/*
d_internal_grow
  Room for one more record in an array that doubles as it fills.
*/
static bool
d_internal_grow(
    void**  _array,
    size_t* _capacity,
    size_t  _count,
    size_t  _size
)
{
    // room already
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t wanted = (*_capacity > 0u) ? (*_capacity * 2u) : 16u;
    void* const  grown  = realloc(*_array, wanted * _size);

    // memory that ran out
    if (!grown)
    {
        return false;
    }

    *_array    = grown;
    *_capacity = wanted;

    return true;
}


/*
d_ninja_line_of
  The count of line starts at or before the place: a binary search of the
table made when reading began.
*/
size_t
d_ninja_line_of(
    const struct d_ninja* _ninja,
    size_t                _at
)
{
    size_t low  = 0;
    size_t high = 0;

    // no table, no lines
    if ( (!_ninja) ||
         (!_ninja->line_starts) )
    {
        return 1u;
    }

    high = _ninja->line_count + 1u;

    // the last start at or before the place
    while ((low + 1u) < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        // a table of the halves: the place at or past this start, or before
        if (_ninja->line_starts[middle] <= _at)
        {
            low = middle;
        }
        else
        {
            high = middle;
        }
    }

    return low + 1u;
}


/*
d_internal_lines
  The table of line starts: one at 0, and one past each line feed.
*/
static bool
d_internal_lines(
    struct d_ninja* _ninja
)
{
    size_t feeds = 0;

    // each line feed, counted
    for (size_t at = 0; at < _ninja->length; ++at)
    {
        feeds += (_ninja->text[at] == '\n') ? 1u : 0u;
    }

    _ninja->line_starts = malloc((feeds + 2u) * sizeof(size_t));

    // memory that ran out
    if (!_ninja->line_starts)
    {
        return false;
    }

    size_t count = 0;

    _ninja->line_starts[count++] = 0u;

    // a start past each line feed
    for (size_t at = 0; at < _ninja->length; ++at)
    {
        // a line feed ends a line
        if (_ninja->text[at] == '\n')
        {
            _ninja->line_starts[count++] = at + 1u;
        }
    }

    // a start at the end stands for no line; the table is closed past it,
    // as it is searched over one more entry than there are lines
    _ninja->line_count = ( (_ninja->length == 0u) ||
                           (_ninja->text[_ninja->length - 1u] == '\n') )
                             ? (count - 1u) : count;
    _ninja->line_starts[count] = (size_t)-1;

    return true;
}


/*
d_internal_fail
  Says why reading stopped, at the place ninja's lexer last read, as ninja
words it; and gives false.
*/
static bool
d_internal_fail(
    struct d_internal_reader* _reader,
    size_t                    _at,
    const char*               _message
)
{
    // only the first failure is the reader's to report
    if (_reader->failed)
    {
        return false;
    }

    _reader->failed            = true;
    _reader->ninja->error_line = d_ninja_line_of(_reader->ninja, _at);
    (void)snprintf(_reader->ninja->error, sizeof(_reader->ninja->error),
                   "%s", _message);

    // the reason, where there is room for it
    if ( (_reader->problem) &&
         (_reader->capacity > 0u) )
    {
        (void)snprintf(_reader->problem, _reader->capacity, "line %zu: %s",
                       _reader->ninja->error_line, _message);
    }

    return false;
}


/*
d_internal_no_memory
  Fails for memory that ran out.
*/
static bool
d_internal_no_memory(
    struct d_internal_reader* _reader
)
{
    return d_internal_fail(_reader, _reader->at, "out of memory");
}


/*
d_internal_add_comment
  Records a comment line, `#` to its line ending.
*/
static bool
d_internal_add_comment(
    struct d_internal_reader* _reader,
    size_t                    _start,
    size_t                    _end
)
{
    struct d_ninja* const ninja = _reader->ninja;

    // room for one more
    if (!d_internal_grow((void**)&ninja->comments,
                         &_reader->comment_capacity, ninja->comment_count,
                         sizeof(*ninja->comments)))
    {
        return d_internal_no_memory(_reader);
    }

    struct d_ninja_comment* const comment =
        &ninja->comments[ninja->comment_count++];

    comment->start  = _start;
    comment->end    = ( (_end > _start) &&
                        (_reader->text[_end - 1u] == '\r') ) ? (_end - 1u)
                                                             : _end;
    comment->line   = d_ninja_line_of(ninja, _start);
    comment->within = D_NINJA_NONE;

    return true;
}


//==============================================================================
// 3.  THE LEXER
//==============================================================================


/*
d_internal_is_name_byte, d_internal_is_simple_byte
  A byte of ninja's varname, `[a-zA-Z0-9_.-]`, and of its simple_varname,
the same without `.`.
*/
static bool
d_internal_is_simple_byte(
    char _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) ||
             ( (_c >= '0') && (_c <= '9') ) ||
             (_c == '_') ||
             (_c == '-') );
}


static bool
d_internal_is_name_byte(
    char _c
)
{
    return ( (d_internal_is_simple_byte(_c)) ||
             (_c == '.') );
}


/*
d_internal_name_run
  How many bytes of names stand from a place.
*/
static size_t
d_internal_name_run(
    const struct d_internal_reader* _reader,
    size_t                          _at,
    bool                            _simple
)
{
    size_t at = _at;

    // each byte a name may hold
    while ( (at < _reader->length) &&
            ( (_simple) ? d_internal_is_simple_byte(_reader->text[at])
                        : d_internal_is_name_byte(_reader->text[at]) ) )
    {
        at++;
    }

    return at - _at;
}


/*
d_internal_byte
  The byte at a place, or NUL past the end, as ninja's lexer reads its
terminated input.
*/
static char
d_internal_byte(
    const struct d_internal_reader* _reader,
    size_t                          _at
)
{
    return (_at < _reader->length) ? _reader->text[_at] : '\0';
}


/*
d_internal_eat_whitespace
  Ninja's EatWhitespace: blanks, and `$` line continuations.
*/
static void
d_internal_eat_whitespace(
    struct d_internal_reader* _reader
)
{
    // each run of spaces, and each continuation
    for (;;)
    {
        const char c = d_internal_byte(_reader, _reader->at);

        // a table of what is eaten: a space, `$` LF, `$` CR LF
        if (c == ' ')
        {
            _reader->at++;
        }
        else if ( (c == '$') &&
                  (d_internal_byte(_reader, _reader->at + 1u) == '\n') )
        {
            _reader->at += 2u;
        }
        else if ( (c == '$') &&
                  (d_internal_byte(_reader, _reader->at + 1u) == '\r') &&
                  (d_internal_byte(_reader, _reader->at + 2u) == '\n') )
        {
            _reader->at += 3u;
        }
        else
        {
            break;
        }
    }

    return;
}


/*
d_internal_keyword
  The keyword a name of `_length` bytes is, or D_INTERNAL_IDENT: a
keyword's rule wins over the name's only at the same length.
*/
static enum d_internal_token
d_internal_keyword(
    const char* _text,
    size_t      _length
)
{
    static const char* const        words[]  =
    {
        "build", "pool", "rule", "default", "include", "subninja"
    };
    static const enum d_internal_token tokens[] =
    {
        D_INTERNAL_BUILD, D_INTERNAL_POOL, D_INTERNAL_RULE,
        D_INTERNAL_DEFAULT, D_INTERNAL_INCLUDE, D_INTERNAL_SUBNINJA
    };

    // each keyword, whole
    for (size_t at = 0; at < (sizeof(words) / sizeof(words[0])); ++at)
    {
        // the same bytes, and as many
        if ( (strlen(words[at]) == _length) &&
             (memcmp(words[at], _text, _length) == 0) )
        {
            return tokens[at];
        }
    }

    return D_INTERNAL_IDENT;
}


/*
d_internal_comment_at
  The end of a comment line from a place -- blanks, `#`, the rest, and its
line feed -- or the place itself when none stands there.
*/
static size_t
d_internal_comment_at(
    struct d_internal_reader* _reader,
    size_t                    _at,
    size_t*                   _out_hash
)
{
    size_t p = _at;

    // the blanks before it
    while (d_internal_byte(_reader, p) == ' ')
    {
        p++;
    }

    // a `#` opens it
    if (d_internal_byte(_reader, p) != '#')
    {
        return _at;
    }

    const char* const feed = memchr(&_reader->text[p], '\n',
                                    _reader->length - p);
    const char* const nul  = memchr(&_reader->text[p], '\0',
                                    _reader->length - p);

    // a comment needs its line feed, before any NUL
    if ( (!feed) ||
         ( (nul) && (nul < feed) ) )
    {
        return _at;
    }

    *_out_hash = p;

    return (size_t)(feed - _reader->text) + 1u;
}


/*
d_internal_read_token
  Ninja's ReadToken: comment lines skipped and recorded, then the token at
`at`, `last` left at its start, and the blanks after it eaten unless it
ends a line or the file.
  Long as it is a table of the tokens, each a line or two.
*/
static enum d_internal_token
d_internal_read_token(
    struct d_internal_reader* _reader
)
{
    enum d_internal_token token = D_INTERNAL_ERROR;
    size_t                start = _reader->at;

    // past each comment line
    for (;;)
    {
        size_t       hash = 0;
        const size_t end  = d_internal_comment_at(_reader, start, &hash);

        // no comment here: the token starts here
        if (end == start)
        {
            break;
        }

        // a comment that could not be recorded stops the reading
        if (!d_internal_add_comment(_reader, hash, end - 1u))
        {
            return D_INTERNAL_ERROR;
        }

        start = end;
    }

    size_t p = start;

    // the blanks a newline or an indent opens with
    while (d_internal_byte(_reader, p) == ' ')
    {
        p++;
    }

    const char   c    = d_internal_byte(_reader, p);
    const size_t name = (p == start) ? d_internal_name_run(_reader, p, false)
                                     : 0u;

    // a table of the tokens, as ninja's lexer orders them
    if (c == '\n')
    {
        token = D_INTERNAL_NEWLINE;
        p++;
    }
    else if ( (c == '\r') &&
              (d_internal_byte(_reader, p + 1u) == '\n') )
    {
        token = D_INTERNAL_NEWLINE;
        p += 2u;
    }
    else if (p > start)
    {
        token = D_INTERNAL_INDENT;
    }
    else if (name > 0u)
    {
        token = d_internal_keyword(&_reader->text[p], name);
        p    += name;
    }
    else if ( (c == '\0') &&
              (p >= _reader->length) )
    {
        token = D_INTERNAL_TEOF;
    }
    else if (c == '=')
    {
        token = D_INTERNAL_EQUALS;
        p++;
    }
    else if (c == ':')
    {
        token = D_INTERNAL_COLON;
        p++;
    }
    else if ( (c == '|') &&
              (d_internal_byte(_reader, p + 1u) == '@') )
    {
        token = D_INTERNAL_PIPEAT;
        p += 2u;
    }
    else if ( (c == '|') &&
              (d_internal_byte(_reader, p + 1u) == '|') )
    {
        token = D_INTERNAL_PIPE2;
        p += 2u;
    }
    else if (c == '|')
    {
        token = D_INTERNAL_PIPE;
        p++;
    }
    else
    {
        token = D_INTERNAL_ERROR;
        p++;
    }

    _reader->last = start;
    _reader->at   = p;

    // the blanks after any token but the end of a line or of the file
    if ( (token != D_INTERNAL_NEWLINE) &&
         (token != D_INTERNAL_TEOF) )
    {
        d_internal_eat_whitespace(_reader);
    }

    return token;
}


/*
d_internal_unread, d_internal_peek
  Ninja's UnreadToken, back to the last token's start, and PeekToken.
*/
static void
d_internal_unread(
    struct d_internal_reader* _reader
)
{
    _reader->at = _reader->last;

    return;
}


static bool
d_internal_peek(
    struct d_internal_reader* _reader,
    enum d_internal_token     _token
)
{
    // the token wanted is taken; any other is put back
    if (d_internal_read_token(_reader) == _token)
    {
        return true;
    }

    d_internal_unread(_reader);

    return false;
}


/*
d_internal_token_name
  A token as ninja's messages name it.
*/
static const char*
d_internal_token_name(
    enum d_internal_token _token
)
{
    static const char* const names[] =
    {
        "lexing error", "'build'", "':'", "'default'", "'='", "identifier",
        "'include'", "indent", "newline", "'||'", "'|'", "'|@'", "'pool'",
        "'rule'", "'subninja'", "eof"
    };

    return names[_token];
}


/*
d_internal_describe
  Ninja's DescribeLastError: a tab says so; anything else is a lexing
error.
*/
static const char*
d_internal_describe(
    const struct d_internal_reader* _reader
)
{
    return (d_internal_byte(_reader, _reader->last) == '\t')
               ? "tabs are not allowed, use spaces" : "lexing error";
}


/*
d_internal_expect
  Ninja's ExpectToken: the token wanted, or a failure that names the one
found.
*/
static bool
d_internal_expect(
    struct d_internal_reader* _reader,
    enum d_internal_token     _token
)
{
    const enum d_internal_token found = d_internal_read_token(_reader);
    char                        message[128];

    // the token wanted
    if (found == _token)
    {
        return true;
    }

    (void)snprintf(message, sizeof(message), "expected %s, got %s%s",
                   d_internal_token_name(_token),
                   d_internal_token_name(found),
                   (_token == D_INTERNAL_COLON) ? " ($ also escapes ':')"
                                                : "");

    return d_internal_fail(_reader, _reader->last, message);
}


/*
d_internal_read_ident
  Ninja's ReadIdent: a name at `at`, its span given, and the blanks after
it eaten; false, `last` at the place, when no name stands there.
*/
static bool
d_internal_read_ident(
    struct d_internal_reader* _reader,
    size_t*                   _out_start,
    size_t*                   _out_end
)
{
    const size_t run = d_internal_name_run(_reader, _reader->at, false);

    _reader->last = _reader->at;

    // no name here
    if (run == 0u)
    {
        return false;
    }

    *_out_start  = _reader->at;
    *_out_end    = _reader->at + run;
    _reader->at += run;
    d_internal_eat_whitespace(_reader);

    return true;
}


//==============================================================================
// 4.  VALUES
//==============================================================================


/*
d_internal_add_piece
  Adds a piece to the value being read: text runs into the text before it,
as ninja's EvalString joins its text.
*/
static bool
d_internal_add_piece(
    struct d_internal_reader* _reader,
    struct d_ninja_value*     _value,
    enum d_ninja_piece_kind   _kind,
    size_t                    _start,
    size_t                    _end,
    size_t                    _name_start,
    size_t                    _name_end
)
{
    struct d_ninja* const ninja = _reader->ninja;

    // text after text is the same piece, longer
    if ( (_kind == D_NINJA_TEXT) &&
         (_value->piece_count > 0u) &&
         (ninja->pieces[ninja->piece_count - 1u].kind == D_NINJA_TEXT) &&
         (ninja->pieces[ninja->piece_count - 1u].end == _start) )
    {
        ninja->pieces[ninja->piece_count - 1u].end = _end;

        return true;
    }

    // room for one more
    if (!d_internal_grow((void**)&ninja->pieces, &_reader->piece_capacity,
                         ninja->piece_count, sizeof(*ninja->pieces)))
    {
        return d_internal_no_memory(_reader);
    }

    struct d_ninja_piece* const piece = &ninja->pieces[ninja->piece_count++];

    piece->kind       = _kind;
    piece->start      = _start;
    piece->end        = _end;
    piece->name_start = _name_start;
    piece->name_end   = _name_end;
    _value->piece_count++;

    return true;
}


/*
d_internal_dollar
  One `$` escape at `_at`: its length, and whether it is a reference, whose
name is given; 0 for a bad escape, after ninja's message is made.
  Long as it is a table of the escapes, each a line or two.
*/
static size_t
d_internal_dollar(
    struct d_internal_reader* _reader,
    size_t                    _at,
    bool*                     _out_reference,
    size_t*                   _out_name_start,
    size_t*                   _out_name_end
)
{
    const char c = d_internal_byte(_reader, _at + 1u);
    size_t     p = _at + 2u;

    *_out_reference = false;

    // a table of the escapes: `$$`, `$ `, `$:`, `$^`, the continuations,
    // `${name}` and `$name`
    if ( (c == '$') ||
         (c == ' ') ||
         (c == ':') )
    {
        return 2u;
    }
    else if (c == '^')
    {
        // a newline, for a file that requires ninja 1.14
        if ( (_reader->ninja->major < 1) ||
             ( (_reader->ninja->major == 1) && (_reader->ninja->minor < 14) ) )
        {
            (void)d_internal_fail(_reader, _at,
                                  "using $^ escape requires specifying "
                                  "'ninja_required_version' with version "
                                  "greater or equal 1.14");

            return 0u;
        }

        return 2u;
    }
    else if ( (c == '\n') ||
              ( (c == '\r') &&
                (d_internal_byte(_reader, _at + 2u) == '\n') ) )
    {
        p = (c == '\n') ? (_at + 2u) : (_at + 3u);

        // the blanks that open the next line go with it
        while (d_internal_byte(_reader, p) == ' ')
        {
            p++;
        }

        return p - _at;
    }
    else if (c == '{')
    {
        const size_t run = d_internal_name_run(_reader, _at + 2u, false);

        // a name, and its `}`
        if ( (run > 0u) &&
             (d_internal_byte(_reader, _at + 2u + run) == '}') )
        {
            *_out_reference  = true;
            *_out_name_start = _at + 2u;
            *_out_name_end   = _at + 2u + run;

            return run + 3u;
        }
    }
    else if (d_internal_is_simple_byte(c))
    {
        const size_t run = d_internal_name_run(_reader, _at + 1u, true);

        *_out_reference  = true;
        *_out_name_start = _at + 1u;
        *_out_name_end   = _at + 1u + run;

        return run + 1u;
    }

    (void)d_internal_fail(_reader, _at, "bad $-escape (literal $ must be "
                                        "written as $$)");

    return 0u;
}


/*
d_internal_plain
  Whether a byte is text to ReadEvalString, which ends a run only at `$`,
space, `:`, a line ending, `|` or NUL.
*/
static bool
d_internal_plain(
    char _c
)
{
    return ( (_c != '$')  &&
             (_c != ' ')  &&
             (_c != ':')  &&
             (_c != '\r') &&
             (_c != '\n') &&
             (_c != '|')  &&
             (_c != '\0') );
}


/*
d_internal_value_step
  One step of ReadEvalString at `p`: the text, escape or reference there
added to the value, `p` moved past it; 1 to go on, 0 at the value's end,
`p` then at its delimiter, and -1 for a failure.
  Long as it is a table of what may stand at the place.
*/
static int
d_internal_value_step(
    struct d_internal_reader* _reader,
    struct d_ninja_value*     _value,
    bool                      _path,
    size_t*                   _p
)
{
    const size_t start = *_p;
    const char   c     = d_internal_byte(_reader, start);
    size_t       p     = start;

    // a table of what stands here: text, a line ending, a delimiter, an
    // escape, or the end of the file
    if (d_internal_plain(c))
    {
        // the run of text
        while (d_internal_plain(d_internal_byte(_reader, p)))
        {
            p++;
        }

        *_p = p;

        return (d_internal_add_piece(_reader, _value, D_NINJA_TEXT, start, p,
                                     0u, 0u)) ? 1 : -1;
    }
    else if ( (c == '\r') &&
              (d_internal_byte(_reader, start + 1u) == '\n') )
    {
        _reader->last = start;
        *_p           = (_path) ? start : (start + 2u);

        return 0;
    }
    else if ( (c == ' ') ||
              (c == ':') ||
              (c == '|') ||
              (c == '\n') )
    {
        // a path ends at each; a value only at its line ending
        if ( (_path) ||
             (c == '\n') )
        {
            _reader->last = start;
            *_p           = (_path) ? start : (start + 1u);

            return 0;
        }

        *_p = start + 1u;

        return (d_internal_add_piece(_reader, _value, D_NINJA_TEXT, start,
                                     start + 1u, 0u, 0u)) ? 1 : -1;
    }
    else if (c == '$')
    {
        bool         reference = false;
        size_t       from      = 0;
        size_t       to        = 0;
        const size_t length    = d_internal_dollar(_reader, start, &reference,
                                                   &from, &to);

        // a bad escape fails the reading
        if (length == 0u)
        {
            return -1;
        }

        *_p = start + length;

        return (d_internal_add_piece(_reader, _value,
                                     (reference) ? D_NINJA_REFERENCE
                                                 : D_NINJA_TEXT,
                                     start, start + length, from, to))
                   ? 1 : -1;
    }
    else if ( (c == '\0') &&
              (start >= _reader->length) )
    {
        _reader->last = start;
        (void)d_internal_fail(_reader, start, "unexpected EOF");

        return -1;
    }

    _reader->last = start;
    (void)d_internal_fail(_reader, start, d_internal_describe(_reader));

    return -1;
}


/*
d_internal_read_value
  Ninja's ReadEvalString: a value, or a path, at `at`; its index is given.
A path ends before a space, `:`, `|` or a line ending, and the blanks after
it are eaten; a value ends at its line ending, which is read with it but is
no part of it.
*/
static bool
d_internal_read_value(
    struct d_internal_reader* _reader,
    bool                      _path,
    size_t*                   _out_value
)
{
    struct d_ninja* const ninja = _reader->ninja;

    // room for one more
    if (!d_internal_grow((void**)&ninja->values, &_reader->value_capacity,
                         ninja->value_count, sizeof(*ninja->values)))
    {
        return d_internal_no_memory(_reader);
    }

    struct d_ninja_value value;
    size_t               p    = _reader->at;
    int                  step = 1;

    value.start       = p;
    value.end         = p;
    value.first_piece = ninja->piece_count;
    value.piece_count = 0;

    // each piece, to the value's end
    while (step > 0)
    {
        step = d_internal_value_step(_reader, &value, _path, &p);
    }

    // a failure is the step's to have said
    if (step < 0)
    {
        return false;
    }

    value.end   = _reader->last;
    _reader->at = p;

    // a path eats the blanks after it
    if (_path)
    {
        d_internal_eat_whitespace(_reader);
    }

    ninja->values[ninja->value_count] = value;
    *_out_value                        = ninja->value_count++;

    return true;
}


//==============================================================================
// 5.  STATEMENTS
//==============================================================================


/*
d_internal_begin
  A statement begun at `_start`, of a kind; NULL when memory runs out.
*/
static struct d_ninja_statement*
d_internal_begin(
    struct d_internal_reader* _reader,
    enum d_ninja_kind         _kind,
    size_t                    _start
)
{
    struct d_ninja* const ninja = _reader->ninja;

    // room for one more
    if (!d_internal_grow((void**)&ninja->statements,
                         &_reader->statement_capacity,
                         ninja->statement_count, sizeof(*ninja->statements)))
    {
        (void)d_internal_no_memory(_reader);

        return NULL;
    }

    struct d_ninja_statement* const statement =
        &ninja->statements[ninja->statement_count];

    memset(statement, 0, sizeof(*statement));
    statement->kind          = _kind;
    statement->start         = _start;
    statement->line          = d_ninja_line_of(ninja, _start);
    statement->name_start    = _start;
    statement->name_end      = _start;
    statement->value         = D_NINJA_NONE;
    statement->first_path    = ninja->path_count;
    statement->first_binding = ninja->binding_count;
    _reader->begun           = true;

    return statement;
}


/*
d_internal_finish
  Keeps the statement begun: it ends at `_end`, and ninja reads next at
`at`, past any comment.
*/
static void
d_internal_finish(
    struct d_internal_reader* _reader,
    size_t                    _end
)
{
    struct d_ninja* const           ninja     = _reader->ninja;
    struct d_ninja_statement* const statement =
        &ninja->statements[ninja->statement_count++];
    size_t                          next      = _reader->at;
    size_t                          hash      = 0;

    statement->end      = _end;
    statement->end_line = d_ninja_line_of(ninja, (_end > statement->start)
                                                     ? (_end - 1u) : _end);
    _reader->begun      = false;

    // past the comment lines after it, as ninja's next token is
    for (;;)
    {
        const size_t end = d_internal_comment_at(_reader, next, &hash);

        // the token after the comments
        if (end == next)
        {
            break;
        }

        next = end;
    }

    statement->next_line = d_ninja_line_of(ninja, next);

    return;
}


/*
d_internal_add_path
  A path of the statement begun, in a section.
*/
static bool
d_internal_add_path(
    struct d_internal_reader* _reader,
    size_t                    _value,
    enum d_ninja_section      _section
)
{
    struct d_ninja* const ninja = _reader->ninja;

    // room for one more
    if (!d_internal_grow((void**)&ninja->paths, &_reader->path_capacity,
                         ninja->path_count, sizeof(*ninja->paths)))
    {
        return d_internal_no_memory(_reader);
    }

    ninja->paths[ninja->path_count].value   = _value;
    ninja->paths[ninja->path_count].section = _section;
    ninja->path_count++;
    ninja->statements[ninja->statement_count].path_count++;

    return true;
}


/*
d_internal_paths
  The paths of a section, read until one is empty: true when at least one
was read, which `_out_any` says.
*/
static bool
d_internal_paths(
    struct d_internal_reader* _reader,
    enum d_ninja_section      _section,
    bool*                     _out_any
)
{
    struct d_ninja* const ninja = _reader->ninja;

    *_out_any = false;

    // each path, until one comes to nothing
    for (;;)
    {
        size_t value = 0;

        // a path that could not be read
        if (!d_internal_read_value(_reader, true, &value))
        {
            return false;
        }

        // an empty one ends the section, and is no path
        if (ninja->values[value].start == ninja->values[value].end)
        {
            ninja->value_count--;

            return true;
        }

        *_out_any = true;

        // the path kept
        if (!d_internal_add_path(_reader, value, _section))
        {
            return false;
        }
    }
}


/*
d_internal_let
  Ninja's ParseLet at `at`: a name, `=`, and a value to its line's end,
whose index is given.
*/
static bool
d_internal_let(
    struct d_internal_reader* _reader,
    size_t*                   _out_name_start,
    size_t*                   _out_name_end,
    size_t*                   _out_value
)
{
    // a name
    if (!d_internal_read_ident(_reader, _out_name_start, _out_name_end))
    {
        return d_internal_fail(_reader, _reader->last,
                               "expected variable name");
    }

    return ( (d_internal_expect(_reader, D_INTERNAL_EQUALS)) &&
             (d_internal_read_value(_reader, false, _out_value)) );
}


/*
d_internal_bindings
  The bindings indented beneath the statement begun, each line a binding,
the comment lines among them skipped; the end of the last, or `_end`, is
given.
*/
static bool
d_internal_bindings(
    struct d_internal_reader* _reader,
    size_t*                   _io_end
)
{
    struct d_ninja* const ninja = _reader->ninja;

    // each indented line
    while (d_internal_peek(_reader, D_INTERNAL_INDENT))
    {
        const size_t start = _reader->last;
        size_t       from  = 0;
        size_t       to    = 0;
        size_t       value = 0;

        // the binding, and room for it
        if ( (!d_internal_let(_reader, &from, &to, &value)) ||
             (!d_internal_grow((void**)&ninja->bindings,
                               &_reader->binding_capacity,
                               ninja->binding_count,
                               sizeof(*ninja->bindings))) )
        {
            return (_reader->failed) ? false : d_internal_no_memory(_reader);
        }

        struct d_ninja_binding* const binding =
            &ninja->bindings[ninja->binding_count++];

        binding->start      = start;
        binding->end        = _reader->at;
        binding->line       = d_ninja_line_of(ninja, start);
        binding->end_line   = d_ninja_line_of(ninja, _reader->last);
        binding->name_start = from;
        binding->name_end   = to;
        binding->value      = value;
        ninja->statements[ninja->statement_count].binding_count++;
        *_io_end            = _reader->at;
    }

    return true;
}


/*
d_internal_binding_is
  Whether a binding of the statement begun has a name.
*/
static bool
d_internal_binding_is(
    const struct d_internal_reader* _reader,
    const struct d_ninja_binding*   _binding,
    const char*                     _name
)
{
    const size_t span = strlen(_name);

    return ( ((_binding->name_end - _binding->name_start) == span) &&
             (memcmp(&_reader->text[_binding->name_start], _name,
                     span) == 0) );
}


/*
d_internal_has_value
  Whether the statement begun has a binding of a name whose value is not
empty, as ninja's `bindings_[name].empty()` asks.
*/
static bool
d_internal_has_value(
    const struct d_internal_reader* _reader,
    const char*                     _name
)
{
    const struct d_ninja* const           ninja     = _reader->ninja;
    const struct d_ninja_statement* const statement =
        &ninja->statements[ninja->statement_count];
    bool                                  has       = false;

    // the last binding of the name is the one the rule keeps
    for (size_t at = 0; at < statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &ninja->bindings[statement->first_binding + at];

        // the name's
        if (d_internal_binding_is(_reader, binding, _name))
        {
            has = (ninja->values[binding->value].piece_count > 0u);
        }
    }

    return has;
}


/*
d_internal_check_bindings
  Ninja's checks of a rule's or a pool's bindings, which it makes as it
reads them: each may have only its own; a rule needs its command, and
both or neither of rspfile and rspfile_content; a pool needs its depth.
*/
static bool
d_internal_check_bindings(
    struct d_internal_reader* _reader,
    enum d_ninja_kind         _kind,
    size_t                    _next
)
{
    const struct d_ninja* const           ninja     = _reader->ninja;
    const struct d_ninja_statement* const statement =
        &ninja->statements[ninja->statement_count];
    bool                                  depth     = false;
    char                                  message[192];

    // each binding, one the statement may have
    for (size_t at = 0; at < statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &ninja->bindings[statement->first_binding + at];
        const char* const name = &_reader->text[binding->name_start];
        const size_t      span = binding->name_end - binding->name_start;
        const bool        own  = (_kind == D_NINJA_RULE)
                                     ? d_ninja_is_reserved(name, span)
                                     : d_internal_binding_is(_reader, binding,
                                                             "depth");

        depth = (depth) || (own);

        // one it may not have, where ninja reads it
        if (!own)
        {
            (void)snprintf(message, sizeof(message),
                           "unexpected variable '%.*s'", (int)span, name);

            return d_internal_fail(_reader,
                                   ninja->line_starts[binding->end_line - 1u],
                                   message);
        }
    }

    // a table of what each needs: a pool its depth, a rule its two
    if (_kind == D_NINJA_POOL)
    {
        return (depth) ? true
                       : d_internal_fail(_reader, _next,
                                         "expected 'depth =' line");
    }
    else if (d_internal_has_value(_reader, "rspfile") !=
             d_internal_has_value(_reader, "rspfile_content"))
    {
        return d_internal_fail(_reader, _next,
                               "rspfile and rspfile_content need to be both "
                               "specified");
    }
    else if (!d_internal_has_value(_reader, "command"))
    {
        return d_internal_fail(_reader, _next,
                               "expected 'command =' line");
    }

    return true;
}


/*
d_internal_named
  A rule or a pool: its name, its line's end, and its bindings, checked
as ninja checks them.
*/
static bool
d_internal_named(
    struct d_internal_reader* _reader,
    enum d_ninja_kind         _kind
)
{
    struct d_ninja_statement* const statement =
        d_internal_begin(_reader, _kind, _reader->last);
    size_t                          end       = 0;

    // room for it
    if (!statement)
    {
        return false;
    }

    // its name
    if (!d_internal_read_ident(_reader, &statement->name_start,
                               &statement->name_end))
    {
        return d_internal_fail(_reader, _reader->last,
                               (_kind == D_NINJA_RULE) ? "expected rule name"
                                                       : "expected pool "
                                                         "name");
    }

    // its line's end
    if (!d_internal_expect(_reader, D_INTERNAL_NEWLINE))
    {
        return false;
    }

    statement->header_end = _reader->at;
    end                   = _reader->at;

    // its bindings
    if (!d_internal_bindings(_reader, &end))
    {
        return false;
    }

    // what ninja reads next is past the bindings, where it reports their
    // faults
    if (!d_internal_check_bindings(_reader, _kind, _reader->at))
    {
        return false;
    }

    d_internal_finish(_reader, end);

    return true;
}


/*
d_internal_variable
  A variable at the top level: its name, `=` and its value; the version a
ninja_required_version asks for is kept, for `$^`.
*/
static bool
d_internal_variable(
    struct d_internal_reader* _reader
)
{
    struct d_ninja_statement* const statement =
        d_internal_begin(_reader, D_NINJA_VARIABLE, _reader->at);

    // room for it, and the let
    if ( (!statement) ||
         (!d_internal_let(_reader, &statement->name_start,
                          &statement->name_end, &statement->value)) )
    {
        return false;
    }

    statement->header_end = _reader->at;

    const size_t span = statement->name_end - statement->name_start;

    // the version a file requires, read as ninja's ParseVersion reads it
    if ( (span == 22u) &&
         (memcmp(&_reader->text[statement->name_start],
                 "ninja_required_version", 22u) == 0) )
    {
        const struct d_ninja_value* const value =
            &_reader->ninja->values[statement->value];
        char                              text[32];
        size_t                            used = 0;

        // its text, its references read as nothing
        for (size_t at = 0; at < value->piece_count; ++at)
        {
            const struct d_ninja_piece* const piece =
                &_reader->ninja->pieces[value->first_piece + at];
            char                              part[64];

            // text only, as much as is room for
            if ( (piece->kind == D_NINJA_TEXT) &&
                 ((piece->end - piece->start) < sizeof(part)) )
            {
                const size_t got = d_ninja_text_of(_reader->ninja, piece,
                                                   part);

                used += (size_t)snprintf(&text[used], sizeof(text) - used,
                                         "%.*s", (int)got, part);
                used  = (used < sizeof(text)) ? used : (sizeof(text) - 1u);
            }
        }

        text[used] = '\0';

        const char* const dot = strchr(text, '.');

        _reader->ninja->major = atoi(text);
        _reader->ninja->minor = (dot) ? atoi(&dot[1]) : 0;
    }

    d_internal_finish(_reader, _reader->at);

    return true;
}


/*
d_internal_default
  A default: its targets, at least one, and its line's end.
*/
static bool
d_internal_default(
    struct d_internal_reader* _reader
)
{
    struct d_ninja_statement* const statement =
        d_internal_begin(_reader, D_NINJA_DEFAULT, _reader->last);
    bool                            any       = false;

    // room for it, and its targets
    if ( (!statement) ||
         (!d_internal_paths(_reader, D_NINJA_TARGET, &any)) )
    {
        return false;
    }

    // a default needs a target
    if (!any)
    {
        return d_internal_fail(_reader, _reader->last,
                               "expected target name");
    }

    // its line's end
    if (!d_internal_expect(_reader, D_INTERNAL_NEWLINE))
    {
        return false;
    }

    statement->header_end = _reader->at;
    d_internal_finish(_reader, _reader->at);

    return true;
}


/*
d_internal_file
  An include or a subninja: its file, and its line's end.
*/
static bool
d_internal_file(
    struct d_internal_reader* _reader,
    enum d_ninja_kind         _kind
)
{
    struct d_ninja_statement* const statement =
        d_internal_begin(_reader, _kind, _reader->last);
    size_t                          value     = 0;

    // room for it, and its file, which may come to nothing
    if ( (!statement) ||
         (!d_internal_read_value(_reader, true, &value)) ||
         (!d_internal_add_path(_reader, value, D_NINJA_FILE)) )
    {
        return false;
    }

    statement->value = value;

    // its line's end
    if (!d_internal_expect(_reader, D_INTERNAL_NEWLINE))
    {
        return false;
    }

    statement->header_end = _reader->at;
    d_internal_finish(_reader, _reader->at);

    return true;
}


/*
d_internal_sections
  A build's sections after its outputs: `:`, its rule, its inputs, and the
three that follow a pipe, each when its pipe stands there.
*/
static bool
d_internal_sections(
    struct d_internal_reader*  _reader,
    struct d_ninja_statement* _statement
)
{
    static const enum d_internal_token pipes[] =
    {
        D_INTERNAL_PIPE, D_INTERNAL_PIPE2, D_INTERNAL_PIPEAT
    };
    static const enum d_ninja_section  sections[] =
    {
        D_NINJA_IMPLICIT, D_NINJA_ORDER_ONLY, D_NINJA_VALIDATION
    };
    bool                               any = false;

    // the `:` and the rule's name
    if (!d_internal_expect(_reader, D_INTERNAL_COLON))
    {
        return false;
    }

    // the rule, by its name
    if (!d_internal_read_ident(_reader, &_statement->name_start,
                               &_statement->name_end))
    {
        return d_internal_fail(_reader, _reader->last,
                               "expected build command name");
    }

    // the inputs
    if (!d_internal_paths(_reader, D_NINJA_INPUT, &any))
    {
        return false;
    }

    // each section a pipe opens, in its order
    for (size_t at = 0; at < (sizeof(pipes) / sizeof(pipes[0])); ++at)
    {
        // the pipe, and its paths
        if ( (d_internal_peek(_reader, pipes[at])) &&
             (!d_internal_paths(_reader, sections[at], &any)) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_build
  A build: its outputs, its implicit outputs after a `|`, the rest of its
line, its line's end, and its bindings.
*/
static bool
d_internal_build(
    struct d_internal_reader* _reader
)
{
    struct d_ninja_statement* const statement =
        d_internal_begin(_reader, D_NINJA_BUILD, _reader->last);
    bool                            outputs   = false;
    bool                            implicit  = false;
    size_t                          end       = 0;

    // room for it, and its outputs
    if ( (!statement) ||
         (!d_internal_paths(_reader, D_NINJA_OUTPUT, &outputs)) )
    {
        return false;
    }

    // its implicit outputs, after a pipe
    if ( (d_internal_peek(_reader, D_INTERNAL_PIPE)) &&
         (!d_internal_paths(_reader, D_NINJA_IMPLICIT_OUTPUT, &implicit)) )
    {
        return false;
    }

    // a build makes something
    if ( (!outputs) &&
         (!implicit) )
    {
        return d_internal_fail(_reader, _reader->last, "expected path");
    }

    // the rest of its line, its line's end, and its bindings
    if ( (!d_internal_sections(_reader, statement)) ||
         (!d_internal_expect(_reader, D_INTERNAL_NEWLINE)) )
    {
        return false;
    }

    statement->header_end = _reader->at;
    end                   = _reader->at;

    // its bindings
    if (!d_internal_bindings(_reader, &end))
    {
        return false;
    }

    d_internal_finish(_reader, end);

    return true;
}


/*
d_internal_statement
  One statement, by the token that opens it; a line ending alone is none.
1 for a statement read, 0 at the file's end, -1 for a failure.
*/
static int
d_internal_statement(
    struct d_internal_reader* _reader
)
{
    const enum d_internal_token token = d_internal_read_token(_reader);
    char                        message[64];

    // a table of what opens a statement
    switch (token)
    {
        case D_INTERNAL_POOL:
            return d_internal_named(_reader, D_NINJA_POOL) ? 1 : -1;

        case D_INTERNAL_BUILD:
            return d_internal_build(_reader) ? 1 : -1;

        case D_INTERNAL_RULE:
            return d_internal_named(_reader, D_NINJA_RULE) ? 1 : -1;

        case D_INTERNAL_DEFAULT:
            return d_internal_default(_reader) ? 1 : -1;

        case D_INTERNAL_IDENT:
            d_internal_unread(_reader);

            return d_internal_variable(_reader) ? 1 : -1;

        case D_INTERNAL_INCLUDE:
            return d_internal_file(_reader, D_NINJA_INCLUDE) ? 1 : -1;

        case D_INTERNAL_SUBNINJA:
            return d_internal_file(_reader, D_NINJA_SUBNINJA) ? 1 : -1;

        case D_INTERNAL_ERROR:
            (void)d_internal_fail(_reader, _reader->last,
                                  d_internal_describe(_reader));

            return -1;

        case D_INTERNAL_TEOF:
            return 0;

        case D_INTERNAL_NEWLINE:
            return 1;

        default:
            break;
    }

    (void)snprintf(message, sizeof(message), "unexpected %s",
                   d_internal_token_name(token));
    (void)d_internal_fail(_reader, _reader->last, message);

    return -1;
}


/*
d_internal_place_comments
  Each comment among a statement's bindings is that statement's.
*/
static void
d_internal_place_comments(
    struct d_ninja* _ninja
)
{
    size_t statement = 0;

    // each comment, against the statements in order
    for (size_t at = 0; at < _ninja->comment_count; ++at)
    {
        struct d_ninja_comment* const comment = &_ninja->comments[at];

        // the statements that end before it are passed
        while ( (statement < _ninja->statement_count) &&
                (_ninja->statements[statement].end <= comment->start) )
        {
            statement++;
        }

        // one that holds it is its own
        if ( (statement < _ninja->statement_count) &&
             (_ninja->statements[statement].start < comment->start) )
        {
            comment->within = statement;
        }
    }

    return;
}


bool
d_ninja_read(
    struct d_ninja* _out,
    const char*     _text,
    size_t          _length,
    char*           _problem,
    size_t          _capacity
)
{
    struct d_internal_reader reader;

    // parameter validation first
    if (!_out)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    memset(&reader, 0, sizeof(reader));
    _out->text       = (_text) ? _text : "";
    _out->length     = (_text) ? _length : 0u;
    reader.ninja     = _out;
    reader.text      = _out->text;
    reader.length    = _out->length;
    reader.problem   = _problem;
    reader.capacity  = _capacity;

    // the line table, and the first line ending's kind
    if (!d_internal_lines(_out))
    {
        return d_internal_no_memory(&reader);
    }

    const char* const feed = memchr(_out->text, '\n', _out->length);

    _out->crlf = ( (feed) &&
                   (feed > _out->text) &&
                   (feed[-1] == '\r') );

    // a NUL is ninja's end of input, and no byte past it would be read
    if (memchr(_out->text, '\0', _out->length))
    {
        return d_internal_fail(&reader,
                               (size_t)((const char*)memchr(_out->text, '\0',
                                                            _out->length) -
                                        _out->text),
                               "a NUL byte, which ninja reads as the end of "
                               "the file");
    }

    int step = 1;

    // each statement, to the file's end
    while (step > 0)
    {
        step = d_internal_statement(&reader);
    }

    d_internal_place_comments(_out);
    _out->partial = (step < 0) && (reader.begun);

    return (step == 0);
}


void
d_ninja_release(
    struct d_ninja* _ninja
)
{
    // releasing nothing is allowed
    if (!_ninja)
    {
        return;
    }

    free(_ninja->statements);
    free(_ninja->paths);
    free(_ninja->bindings);
    free(_ninja->values);
    free(_ninja->pieces);
    free(_ninja->comments);
    free(_ninja->line_starts);
    memset(_ninja, 0, sizeof(*_ninja));

    return;
}


//==============================================================================
// 6.  NAMES AND VALUES
//==============================================================================


const char*
d_ninja_kind_name(
    enum d_ninja_kind _kind
)
{
    static const char* const names[] =
    {
        "variable", "rule", "build", "pool", "default", "include", "subninja"
    };

    return ( ((size_t)_kind) < (sizeof(names) / sizeof(names[0])) )
               ? names[_kind] : "";
}


const char*
d_ninja_section_name(
    enum d_ninja_section _section
)
{
    static const char* const names[] =
    {
        "output", "implicit_output", "input", "implicit", "order_only",
        "validation", "target", "file"
    };

    return ( ((size_t)_section) < (sizeof(names) / sizeof(names[0])) )
               ? names[_section] : "";
}


bool
d_ninja_is_name(
    const char* _text,
    size_t      _length
)
{
    // a name has a byte
    if ( (!_text) ||
         (_length == 0u) )
    {
        return false;
    }

    // each byte, one a name may hold
    for (size_t at = 0; at < _length; ++at)
    {
        // one it may not
        if (!d_internal_is_name_byte(_text[at]))
        {
            return false;
        }
    }

    return true;
}


bool
d_ninja_is_reserved(
    const char* _name,
    size_t      _length
)
{
    static const char* const names[] =
    {
        "command", "depfile", "dyndep", "description", "deps", "generator",
        "pool", "restat", "rspfile", "rspfile_content", "msvc_deps_prefix"
    };

    // each name a rule may bind
    for (size_t at = 0; at < (sizeof(names) / sizeof(names[0])); ++at)
    {
        // the same bytes, and as many
        if ( (strlen(names[at]) == _length) &&
             (memcmp(names[at], _name, _length) == 0) )
        {
            return true;
        }
    }

    return false;
}


/*
d_ninja_text_of
  Each escape decoded: `$$`, `$ `, `$:` the byte after the `$`, `$^` a
newline, and a continuation, `$` and its line ending and the blanks after
it, nothing.
*/
size_t
d_ninja_text_of(
    const struct d_ninja*       _ninja,
    const struct d_ninja_piece* _piece,
    char*                       _out
)
{
    size_t used = 0;
    size_t at   = _piece->start;

    // each byte, or escape
    while (at < _piece->end)
    {
        const char c    = _ninja->text[at];
        const char next = ((at + 1u) < _piece->end) ? _ninja->text[at + 1u]
                                                     : '\0';

        // a table of what stands here: a byte, an escape that writes one, a
        // newline, or a continuation
        if (c != '$')
        {
            _out[used++] = c;
            at++;
        }
        else if ( (next == '$') ||
                  (next == ' ') ||
                  (next == ':') )
        {
            _out[used++] = next;
            at          += 2u;
        }
        else if (next == '^')
        {
            _out[used++] = '\n';
            at          += 2u;
        }
        else
        {
            at += (next == '\r') ? 3u : 2u;

            // the blanks after the line ending
            while ( (at < _piece->end) &&
                    (_ninja->text[at] == ' ') )
            {
                at++;
            }
        }
    }

    _out[used] = '\0';

    return used;
}


/*
d_ninja_unparse
  Text decoded, a reference written as `${name}`; a reference's own form
is never longer than that, but `$x` is shorter, hence the room asked for.
*/
size_t
d_ninja_unparse(
    const struct d_ninja*       _ninja,
    const struct d_ninja_value* _value,
    char*                       _out
)
{
    size_t used = 0;

    // each piece
    for (size_t at = 0; at < _value->piece_count; ++at)
    {
        const struct d_ninja_piece* const piece =
            &_ninja->pieces[_value->first_piece + at];

        // a table of the two kinds
        if (piece->kind == D_NINJA_TEXT)
        {
            used += d_ninja_text_of(_ninja, piece, &_out[used]);
        }
        else
        {
            const size_t span = piece->name_end - piece->name_start;

            _out[used++] = '$';
            _out[used++] = '{';
            memcpy(&_out[used], &_ninja->text[piece->name_start], span);
            used        += span;
            _out[used++] = '}';
        }
    }

    _out[used] = '\0';

    return used;
}


//==============================================================================
// 7.  WRITING
//==============================================================================


/*
d_internal_escape_byte
  How one byte is written, into `_out`: its length, or 0 when it cannot be.
*/
static size_t
d_internal_escape_byte(
    char  _c,
    bool  _path,
    bool  _newline,
    bool  _first,
    char* _out
)
{
    // a table of the bytes: those with an escape, those with none, and the
    // rest, as they are
    if ( (_c == '$') ||
         ( (_path) && ( (_c == ' ') || (_c == ':') ) ) ||
         ( (!_path) && (_c == ' ') && (_first) ) )
    {
        _out[0] = '$';
        _out[1] = _c;

        return 2u;
    }
    else if ( (_c == '\n') &&
              (!_path) &&
              (_newline) )
    {
        _out[0] = '$';
        _out[1] = '^';

        return 2u;
    }
    else if ( (_c == '\n') ||
              (_c == '\r') ||
              (_c == '\0') ||
              ( (_path) && (_c == '|') ) )
    {
        return 0u;
    }

    _out[0] = _c;

    return 1u;
}


char*
d_ninja_escape(
    const char* _text,
    size_t      _length,
    bool        _path,
    bool        _newline,
    size_t*     _out_length
)
{
    const char* const text = (_text) ? _text : "";
    const size_t      span = (_text) ? _length : 0u;
    char* const       out  = malloc((span * 2u) + 1u);
    size_t            used = 0;

    // room for every byte escaped
    if (!out)
    {
        return NULL;
    }

    // each byte, a value's leading space escaped too
    for (size_t at = 0; at < span; ++at)
    {
        const size_t wrote = d_internal_escape_byte(text[at], _path, _newline,
                                                    (at == 0u), &out[used]);

        // a byte ninja has no way to write
        if (wrote == 0u)
        {
            free(out);

            return NULL;
        }

        used += wrote;
    }

    out[used] = '\0';

    // the length, when it is asked for
    if (_out_length)
    {
        *_out_length = used;
    }

    return out;
}


//==============================================================================
// 8.  PATHS
//==============================================================================


/*
d_internal_separator
  Whether a byte separates a path's components: `/`, and `\` on Windows.
*/
static bool
d_internal_separator(
    char _c,
    bool _win32
)
{
    return ( (_c == '/') ||
             ( (_win32) && (_c == '\\') ) );
}


/*
d_internal_back_up
  Takes the destination back to the start of the component before it, as
CanonicalizePath backs up over one for a `..`.
*/
static size_t
d_internal_back_up(
    const char* _out,
    size_t      _dst,
    size_t      _dst0,
    bool        _win32
)
{
    size_t dst = _dst;

    // back past the component, to the separator before it or the start
    while ( (--dst > _dst0) &&
            (!d_internal_separator(_out[dst - 1u], _win32)) )
    {
        // the decrement is the condition's
    }

    return dst;
}


/*
d_internal_component
  One component of a path being made canonical, `_src` to `_next` with its
separator, or to the end without one: dropped, backed over, or copied.
*/
static size_t
d_internal_component(
    const char* _path,
    size_t      _src,
    size_t      _next,
    bool        _last,
    char*       _out,
    size_t      _dst,
    size_t      _dst0,
    int*        _io_count,
    bool        _win32
)
{
    const size_t length = (_last) ? (_next - _src) : (_next - _src - 1u);

    // a table of the components: empty, `.`, `..`, and any other
    if ( (length == 0u) ||
         ( (length == 1u) && (_path[_src] == '.') ) )
    {
        return _dst;
    }
    else if ( (length == 2u) &&
              (_path[_src] == '.') &&
              (_path[_src + 1u] == '.') )
    {
        // back over the one before, or keep it when there is none
        if (*_io_count > 0)
        {
            *_io_count -= (_last) ? 0 : 1;

            return d_internal_back_up(_out, _dst, _dst0, _win32);
        }

        _out[_dst]      = '.';
        _out[_dst + 1u] = '.';

        // its separator, but for the last
        if (!_last)
        {
            _out[_dst + 2u] = _path[_src + 2u];
        }

        return _dst + ( (_last) ? 2u : 3u );
    }

    *_io_count += 1;
    memmove(&_out[_dst], &_path[_src], _next - _src);

    return _dst + (_next - _src);
}


/*
d_ninja_canonical
  Ninja's CanonicalizePath, component by component: a leading separator, or
two on Windows, kept; leading `../` kept; then each component, the last
without a separator after it; a trailing separator dropped, and "." for
nothing left.  Windows' `\` become `/`.
  Long as the leading part, the components, and the end are each a step.
*/
size_t
d_ninja_canonical(
    const char* _path,
    size_t      _length,
    bool        _win32,
    char*       _out
)
{
    size_t src       = 0;
    size_t dst       = 0;
    size_t dst_start = 0;
    int    count     = 0;

    // nothing is nothing
    if ( (!_path) ||
         (_length == 0u) )
    {
        _out[0] = '\0';

        return 0u;
    }

    // a table of the leading parts: a root, kept; leading `../`, kept
    if (d_internal_separator(_path[0], _win32))
    {
        const size_t lead = ( (_win32) &&
                              (_length >= 2u) &&
                              (d_internal_separator(_path[1], true)) )
                                ? 2u : 1u;

        memcpy(_out, _path, lead);
        src       = lead;
        dst       = lead;
        dst_start = lead;
    }
    else
    {
        // each `../` at the start
        while ( ((src + 3u) <= _length) &&
                (_path[src] == '.') &&
                (_path[src + 1u] == '.') &&
                (d_internal_separator(_path[src + 2u], _win32)) )
        {
            memcpy(&_out[dst], &_path[src], 3u);
            src += 3u;
            dst += 3u;
        }
    }

    const size_t dst0 = dst;

    // each component but the last, with its separator
    for (;;)
    {
        size_t next = src;

        // to its separator
        while ( (next < _length) &&
                (!d_internal_separator(_path[next], _win32)) )
        {
            next++;
        }

        // the last has none
        if (next >= _length)
        {
            break;
        }

        dst = d_internal_component(_path, src, next + 1u, false, _out, dst,
                                   dst0, &count, _win32);
        src = next + 1u;
    }

    dst = d_internal_component(_path, src, _length, true, _out, dst, dst0,
                               &count, _win32);

    // a trailing separator goes, but not the root's
    if ( (dst > dst_start) &&
         (d_internal_separator(_out[dst - 1u], _win32)) )
    {
        dst--;
    }

    // nothing left is "."
    if (dst == 0u)
    {
        _out[dst++] = '.';
    }

    // Windows' backslashes, as ninja keeps them apart and writes `/`
    for (size_t at = 0; (_win32) && (at < dst); ++at)
    {
        _out[at] = (_out[at] == '\\') ? '/' : _out[at];
    }

    _out[dst] = '\0';

    return dst;
}


/*
d_internal_shell_safe
  A byte a shell takes as itself: a letter, a digit, `_`, `+`, `-`, `.`,
`/`.
*/
static bool
d_internal_shell_safe(
    char _c
)
{
    return ( ( (_c >= 'A') && (_c <= 'Z') ) ||
             ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= '0') && (_c <= '9') ) ||
             (_c == '_') ||
             (_c == '+') ||
             (_c == '-') ||
             (_c == '.') ||
             (_c == '/') );
}


/*
d_internal_needs_quotes
  Whether a path needs quoting: a byte a shell would not take as itself,
or, on Windows, a space or a quote.
*/
static bool
d_internal_needs_quotes(
    const char* _path,
    size_t      _length,
    bool        _win32
)
{
    // each byte
    for (size_t at = 0; at < _length; ++at)
    {
        // one the shell would read otherwise
        if ( ( (_win32) && ( (_path[at] == ' ') || (_path[at] == '"') ) ) ||
             ( (!_win32) && (!d_internal_shell_safe(_path[at])) ) )
        {
            return true;
        }
    }

    return false;
}


char*
d_ninja_shell_escape(
    const char* _path,
    size_t      _length,
    bool        _win32,
    size_t*     _out_length
)
{
    const char* const path = (_path) ? _path : "";
    const size_t      span = (_path) ? _length : 0u;
    char* const       out  = malloc((span * 4u) + 3u);
    size_t            used = 0;
    size_t            run  = 0;

    // room for the path, every byte escaped
    if (!out)
    {
        return NULL;
    }

    const bool quote = d_internal_needs_quotes(path, span, _win32);

    out[used] = '"';
    used     += (quote) ? 1u : 0u;
    out[0]    = ( (quote) && (!_win32) ) ? '\'' : out[0];

    // each byte, a quote of the kind written escaped
    for (size_t at = 0; at < span; ++at)
    {
        const char c = path[at];

        // a table of the escapes: POSIX's `'`, and Windows' `"` after its
        // backslashes, each doubled
        if ( (quote) && (!_win32) && (c == '\'') )
        {
            memcpy(&out[used], "'\\'", 3u);
            used += 3u;
        }
        else if ( (quote) && (_win32) && (c == '"') )
        {
            memset(&out[used], '\\', run + 1u);
            used += run + 1u;
        }

        run         = ( (_win32) && (c == '\\') ) ? (run + 1u) : 0u;
        out[used++] = c;
    }

    // the backslashes before the closing quote doubled, and the quote
    if (quote)
    {
        memset(&out[used], '\\', (_win32) ? run : 0u);
        used        += (_win32) ? run : 0u;
        out[used++]  = (_win32) ? '"' : '\'';
    }

    out[used] = '\0';

    // the length, when it is asked for
    if (_out_length)
    {
        *_out_length = used;
    }

    return out;
}
