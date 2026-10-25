/*******************************************************************************
* djinterp [tools]                                                       dmake.c
*
* Reads a makefile into records, as its make reads it: see dmake.h.
*   The logical lines first, in one pass over the text: each physical line
* found by its line feed, the CR before it dropped from the line, and the
* line joined to the next while the backslashes that end it say it
* continues, by its make's rule. Then each make's reader takes the lines in
* turn into statements and comments, through the records and the scanning
* dmake_internal.h shares. The guess walks the same lines without keeping
* them, and stops at the first that only one make reads.
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/make/dmake.h"  // corresponding header
// std
#include <stdio.h>             // snprintf
#include <stdlib.h>            // realloc, free
#include <string.h>            // memchr, memcmp, memset, strchr, strcmp,
                               // strlen
// djinterp
#include "./dmake_internal.h"  // d_make_reader


//==============================================================================
// 1.  DIALECTS
//==============================================================================


// the names of the makes, in the order of d_make_dialect
static const char* const g_dialect_names[] =
{
    "gnu",
    "posix",
    "bsd",
    "nmake"
};

// BSD's directives, each read after a `.` that begins a line
static const char* const g_bsd_directives[] =
{
    "include",  "-include", "sinclude", "dinclude", "if",       "ifdef",
    "ifndef",   "ifmake",   "ifnmake",  "elif",     "elifdef",  "elifndef",
    "elifmake", "elifnmake", "else",    "endif",    "for",      "endfor",
    "undef",    "export",   "export-env", "export-literal", "unexport",
    "unexport-env", "info", "warning",  "error",    NULL
};

// NMake's directives, each read after a `!` that begins a line, in any case
static const char* const g_nmake_directives[] =
{
    "if",        "ifdef",       "ifndef",  "else",    "elseif",
    "elseifdef", "elseifndef",  "endif",   "include", "message",
    "error",     "undef",       "cmdswitches", NULL
};

// GNU's directives that no other make reads, each a line's first word
static const char* const g_gnu_directives[] =
{
    "define", "endef",    "ifeq",     "ifneq", "ifdef", "ifndef",
    "undefine", "override", "vpath",  NULL
};

// GNU's functions, each called as `$(name ...` or `${name ...`
static const char* const g_gnu_functions[] =
{
    "abspath",  "addprefix",  "addsuffix", "and",      "basename",
    "call",     "dir",        "error",     "eval",     "file",
    "filter",   "filter-out", "findstring", "firstword", "flavor",
    "foreach",  "guile",      "if",        "info",     "join",
    "lastword", "notdir",     "or",        "origin",   "patsubst",
    "realpath", "shell",      "sort",      "strip",    "subst",
    "suffix",   "value",      "warning",   "wildcard", "word",
    "wordlist", "words",      NULL
};


/*
d_make_dialect_name
  The table above, read by the enum's value.
*/
const char*
d_make_dialect_name(
    enum d_make_dialect _dialect
)
{
    // a value past the table names nothing
    if ((size_t)_dialect >= (sizeof(g_dialect_names) /
                             sizeof(g_dialect_names[0])))
    {
        return "";
    }

    return g_dialect_names[_dialect];
}


/*
d_make_dialect_of
  The table above, searched by name.
*/
bool
d_make_dialect_of(
    const char*          _name,
    enum d_make_dialect* _out
)
{
    // nothing to find, or nowhere to put it
    if ( (!_name) ||
         (!_out) )
    {
        return false;
    }

    const size_t count = sizeof(g_dialect_names) / sizeof(g_dialect_names[0]);

    // each make's name, in turn
    for (size_t index = 0; index < count; index++)
    {
        // the name found
        if (strcmp(_name,
                   g_dialect_names[index]) == 0)
        {
            *_out = (enum d_make_dialect)index;

            return true;
        }
    }

    return false;
}


//==============================================================================
// 2.  LINES
//==============================================================================


/**
 * @brief Finds the end of the physical line that starts at a place.
 *
 * @param[in]  _text      the text.
 * @param[in]  _length    its length.
 * @param[in]  _at        where the line starts.
 * @param[out] _out_next  receives the place past the line's ending, or the
 *                        text's length when no line feed ends it.
 * @return the place of the line's ending: its CR when CR LF ends it, its
 *         line feed when LF alone does, or the text's length.
 */
static size_t
d_internal_physical_end(
    const char* _text,
    size_t      _length,
    size_t      _at,
    size_t*     _out_next
)
{
    const char* const feed = memchr(_text + _at,
                                    '\n',
                                    _length - _at);

    // the last line, which no line feed ends
    if (!feed)
    {
        *_out_next = _length;

        return _length;
    }

    const size_t end = (size_t)(feed - _text);

    *_out_next = end + 1u;

    // a CR before the line feed belongs to the ending
    if ( (end > _at) &&
         (_text[end - 1u] == '\r') )
    {
        return end - 1u;
    }

    return end;
}


/*
d_make_continues
  Counts the run of backslashes that ends the line, and for NMake the run of
carets before it, since `^\` is a backslash and no continuation, while `^^\`
is a caret and a continuation.
*/
bool
d_make_continues(
    const char*         _text,
    size_t              _start,
    size_t              _end,
    enum d_make_dialect _dialect
)
{
    // a line that does not end in a backslash ends
    if ( (!_text)        ||
         (_end <= _start) ||
         (_text[_end - 1u] != '\\') )
    {
        return false;
    }

    size_t backslashes = 0;

    // the run of backslashes that ends the line
    while ( (_end - backslashes > _start) &&
            (_text[_end - backslashes - 1u] == '\\') )
    {
        backslashes++;
    }

    // a table of the makes' rules: POSIX joins at any backslash, NMake
    // unless an odd run of carets escapes the last, the others at an odd run
    if (_dialect == D_MAKE_POSIX)
    {
        return true;
    }
    else if (_dialect == D_MAKE_NMAKE)
    {
        size_t       carets = 0;
        const size_t before = _end - 1u;

        // the run of carets before the last backslash
        while ( (before - carets > _start) &&
                (_text[before - carets - 1u] == '^') )
        {
            carets++;
        }

        return (carets % 2u) == 0u;
    }

    return (backslashes % 2u) == 1u;
}


/**
 * @brief Reads the logical line that starts at a place.
 *
 * @param[in]  _lines   the text and its make; `text` and `length` are read.
 * @param[in]  _at      where the line starts, before the text's end.
 * @param[in]  _number  the physical line it starts on, from 1.
 * @param[out] _out     receives the line.
 *
 * A continuation joins a line to the next only when there is a next: a
 * backslash and a line feed that end the text end its last line.
 */
static void
d_internal_logical_line(
    const struct d_make_lines* _lines,
    size_t                     _at,
    size_t                     _number,
    struct d_make_line*        _out
)
{
    size_t next = 0;
    size_t end  = d_internal_physical_end(_lines->text,
                                          _lines->length,
                                          _at,
                                          &next);

    _out->start    = _at;
    _out->line     = _number;
    _out->end_line = _number;

    // each physical line that continues on one after it
    while ( (next < _lines->length) &&
            (d_make_continues(_lines->text,
                              _out->start,
                              end,
                              _lines->dialect)) )
    {
        const size_t from = next;

        end = d_internal_physical_end(_lines->text,
                                      _lines->length,
                                      from,
                                      &next);
        _out->end_line++;
    }

    _out->end  = end;
    _out->next = next;

    return;
}


/*
d_make_lines_read
  The lines are kept in an array that doubles as it fills.
*/
bool
d_make_lines_read(
    struct d_make_lines* _out,
    const char*          _text,
    size_t               _length,
    enum d_make_dialect  _dialect
)
{
    // nowhere to put the lines
    if (!_out)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    _out->text    = _text ? _text : "";
    _out->length  = _text ? _length : 0u;
    _out->dialect = _dialect;

    size_t capacity = 0;
    size_t at       = 0;
    size_t number   = 1;

    // each logical line, to the text's end
    while (at < _out->length)
    {
        // room for one more line
        if (_out->count == capacity)
        {
            const size_t wanted = (capacity > 0u) ? (capacity * 2u) : 64u;
            void* const  grown  = realloc(_out->lines,
                                          wanted * sizeof(*_out->lines));

            // memory that ran out
            if (!grown)
            {
                return false;
            }

            _out->lines = grown;
            capacity    = wanted;
        }

        struct d_make_line* const line = &_out->lines[_out->count];

        d_internal_logical_line(_out,
                                at,
                                number,
                                line);
        _out->count++;
        number = line->end_line + 1u;
        at     = line->next;
    }

    const char* const feed = (_out->length > 0u)
                                 ? memchr(_out->text, '\n', _out->length)
                                 : NULL;

    _out->crlf = ( (feed) &&
                   (feed > _out->text) &&
                   (feed[-1] == '\r') );

    return true;
}


/*
d_make_lines_release
  Leaves the record empty, so a second release does nothing.
*/
void
d_make_lines_release(
    struct d_make_lines* _lines
)
{
    // nothing to release
    if (!_lines)
    {
        return;
    }

    free(_lines->lines);
    memset(_lines, 0, sizeof(*_lines));

    return;
}


//==============================================================================
// 3.  GUESSING
//==============================================================================


/**
 * @brief Reports whether a word is one of a table's.
 *
 * @param[in] _table   the words, ending in `NULL`.
 * @param[in] _word    the word.
 * @param[in] _length  its length.
 * @param[in] _fold    whether letters compare in either case.
 * @return `true` if the word is the table's.
 */
static bool
d_internal_word_in(
    const char* const* _table,
    const char*        _word,
    size_t             _length,
    bool               _fold
)
{
    // each word of the table, in turn
    for (size_t index = 0; _table[index]; index++)
    {
        const char* const entry = _table[index];

        // a word of another length is another word
        if (strlen(entry) != _length)
        {
            continue;
        }

        size_t same = 0;

        // the bytes that agree, letters folded when asked
        while ( (same < _length) &&
                ( (entry[same] == _word[same]) ||
                  ( (_fold) &&
                    (_word[same] >= 'A') &&
                    (_word[same] <= 'Z') &&
                    (entry[same] == (char)(_word[same] - 'A' + 'a')) ) ) )
        {
            same++;
        }

        // every byte agreed
        if (same == _length)
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Reads a directive's name: the run of letters and dashes at a place.
 *
 * @param[in] _text  the text.
 * @param[in] _at    where the name starts.
 * @param[in] _end   where the line ends.
 * @return the place past the name.
 */
static size_t
d_internal_name_end(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    size_t at = _at;

    // the letters and dashes of the name
    while ( (at < _end) &&
            ( ( (_text[at] >= 'a') &&
                (_text[at] <= 'z') ) ||
              ( (_text[at] >= 'A') &&
                (_text[at] <= 'Z') ) ||
              (_text[at] == '-') ) )
    {
        at++;
    }

    return at;
}


/**
 * @brief Reports whether a name ends where a directive's may: at the line's
 *        end or at a byte no name holds.
 *
 * @param[in] _text  the text.
 * @param[in] _at    the place past the name.
 * @param[in] _end   where the line ends.
 * @return `true` if the name ends there.
 */
static bool
d_internal_name_ends(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    // the line's end ends the name
    if (_at >= _end)
    {
        return true;
    }

    const char next = _text[_at];

    return !( ( (next >= '0') &&
                (next <= '9') ) ||
              (next == '_') ||
              (next == '.') ||
              (next == '-') );
}


/**
 * @brief Reports whether a line calls one of GNU's functions.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where the line starts.
 * @param[in] _end    where it ends.
 * @return `true` if a `$(` or a `${` opens a call of one of them, its name
 *         followed by a blank.
 */
static bool
d_internal_calls_gnu(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    // each `$` that a parenthesis or a brace follows
    for (size_t at = _start; at + 2u < _end; at++)
    {
        // a reference opened here
        if ( (_text[at] == '$') &&
             ( (_text[at + 1u] == '(') ||
               (_text[at + 1u] == '{') ) )
        {
            const size_t name  = at + 2u;
            const size_t after = d_internal_name_end(_text, name, _end);

            // a function's name, then a blank
            if ( (after < _end) &&
                 ( (_text[after] == ' ') ||
                   (_text[after] == '\t') ) &&
                 (d_internal_word_in(g_gnu_functions,
                                     _text + name,
                                     after - name,
                                     false)) )
            {
                return true;
            }
        }
    }

    return false;
}


/**
 * @brief Reads which make a line belongs to alone.
 *
 * @param[in]  _text   the text.
 * @param[in]  _line   the line.
 * @param[out] _out    receives the make, when there is one.
 * @return `true` if the line is one only that make reads.
 *
 * A line led by a tab is a recipe's, or may be, and says nothing.
 *
 * Long as the three makes' leads, each a few lines: BSD's and NMake's
 * directives after their lead byte, GNU's words, and GNU's calls.
 */
static bool
d_internal_line_dialect(
    const char*               _text,
    const struct d_make_line* _line,
    enum d_make_dialect*      _out
)
{
    const size_t end = _line->end;
    size_t       at  = _line->start;

    // a recipe's line, or an empty one
    if ( (at >= end) ||
         (_text[at] == '\t') )
    {
        return false;
    }

    const char lead = _text[at];

    // a table of the leads: BSD's `.`, NMake's `!`, and GNU's words
    if ( (lead == '.') ||
         (lead == '!') )
    {
        at++;

        // the blanks a directive's name may follow
        while ( (at < end) &&
                ( (_text[at] == ' ') ||
                  (_text[at] == '\t') ) )
        {
            at++;
        }

        const size_t after = d_internal_name_end(_text, at, end);
        const bool   bsd   = (lead == '.');

        // a directive of the make the lead is
        if ( (d_internal_name_ends(_text, after, end)) &&
             (d_internal_word_in(bsd ? g_bsd_directives : g_nmake_directives,
                                 _text + at,
                                 after - at,
                                 !bsd)) )
        {
            *_out = bsd ? D_MAKE_BSD : D_MAKE_NMAKE;

            return true;
        }
    }
    else
    {
        // the spaces GNU's directives may follow
        while ( (at < end) &&
                (_text[at] == ' ') )
        {
            at++;
        }

        const size_t after = d_internal_name_end(_text, at, end);

        // one of GNU's directives, then a blank, a parenthesis or the end
        if ( (d_internal_word_in(g_gnu_directives,
                                 _text + at,
                                 after - at,
                                 false)) &&
             ( (after == end) ||
               (_text[after] == ' ') ||
               (_text[after] == '\t') ||
               (_text[after] == '(') ) )
        {
            *_out = D_MAKE_GNU;

            return true;
        }
    }

    // a call of one of GNU's functions
    if (d_internal_calls_gnu(_text, _line->start, end))
    {
        *_out = D_MAKE_GNU;

        return true;
    }

    return false;
}


/**
 * @brief Reports whether a line suggests GNU make without deciding it: an
 *        assignment by `:=`, which POSIX mode refuses, or a pattern rule.
 *
 * @param[in] _text  the text.
 * @param[in] _line  the line.
 * @return `true` if the line suggests GNU make.
 *
 * BSD make reads `:=` too, and a BSD makefile's `.include` often comes
 * after it, so the suggestion stands only when no line decides.
 */
static bool
d_internal_suggests_gnu(
    const char*               _text,
    const struct d_make_line* _line
)
{
    // a recipe's line, or an empty one
    if ( (_line->start >= _line->end) ||
         (_text[_line->start] == '\t') )
    {
        return false;
    }

    const size_t end   = d_make_find(_text, _line->start, _line->end, "#");
    const size_t equal = d_make_find(_text, _line->start, end, "=");
    const size_t colon = d_make_find(_text, _line->start, end, ":");

    // a table of the two: `:=` but not `::=`, and a `%` before a rule's
    // colon
    if ( (equal < end) &&
         (equal > _line->start) &&
         (_text[equal - 1u] == ':') &&
         ( (equal - 1u == _line->start) ||
           (_text[equal - 2u] != ':') ) )
    {
        return true;
    }
    else if ( (colon < end) &&
              (colon < equal) &&
              (memchr(_text + _line->start, '%', colon - _line->start)) )
    {
        return true;
    }

    return false;
}


/*
d_make_guess
  The lines are joined by GNU's rule while guessing: the makes differ only
in a run of backslashes ending a line, which no line that decides holds.
*/
enum d_make_dialect
d_make_guess(
    const char* _text,
    size_t      _length
)
{
    struct d_make_lines lines;

    memset(&lines, 0, sizeof(lines));
    lines.text    = _text ? _text : "";
    lines.length  = _text ? _length : 0u;
    lines.dialect = D_MAKE_GNU;

    size_t              at       = 0;
    size_t              number   = 1;
    enum d_make_dialect fallback = D_MAKE_POSIX;

    // each logical line until one decides
    while (at < lines.length)
    {
        struct d_make_line  line;
        enum d_make_dialect found = D_MAKE_POSIX;

        d_internal_logical_line(&lines, at, number, &line);

        // the first line only one make reads decides
        if (d_internal_line_dialect(lines.text, &line, &found))
        {
            return found;
        }

        // a line that suggests GNU make, should none decide
        if (d_internal_suggests_gnu(lines.text, &line))
        {
            fallback = D_MAKE_GNU;
        }

        number = line.end_line + 1u;
        at     = line.next;
    }

    return fallback;
}


//==============================================================================
// 4.  RECORDS
//==============================================================================


/**
 * @brief Makes room for one more record in an array that doubles as it
 *        fills.
 *
 * @param[in,out] _array     the array.
 * @param[in,out] _capacity  the records it has room for.
 * @param[in]     _count     the records it holds.
 * @param[in]     _size      the size of one.
 * @return `true` if there is room; `false` when memory runs out.
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

    const size_t wanted = (*_capacity > 0u) ? (*_capacity * 2u) : 32u;
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
d_make_reader_fail
  The message is kept bare in the records and given with its line to the
caller, as d_make_read's contract says.
*/
bool
d_make_reader_fail(
    struct d_make_reader* _reader,
    size_t                _line,
    const char*           _message
)
{
    struct d_make* const make = _reader->make;

    make->error_line = _line;
    (void)snprintf(make->error, sizeof(make->error), "%s", _message);

    // the caller asked to be told why
    if (_reader->problem)
    {
        (void)snprintf(_reader->problem->text,
                       sizeof(_reader->problem->text),
                       "line %lu: %s",
                       (unsigned long)_line,
                       _message);
    }

    return false;
}


/*
d_make_reader_statement
  Every part starts empty at the line's start, so a reader sets only the
parts its statement has.
*/
size_t
d_make_reader_statement(
    struct d_make_reader*     _reader,
    enum d_make_kind          _kind,
    const struct d_make_line* _line
)
{
    struct d_make* const make = _reader->make;

    // room for the statement
    if (!d_internal_grow((void**)&make->statements,
                         &_reader->statement_capacity,
                         make->statement_count,
                         sizeof(*make->statements)))
    {
        (void)d_make_reader_fail(_reader, _line->line, "out of memory");

        return D_MAKE_NONE;
    }

    struct d_make_statement* const statement =
        &make->statements[make->statement_count];
    const struct d_make_span       empty = { _line->start, _line->start };

    memset(statement, 0, sizeof(*statement));
    statement->kind        = _kind;
    statement->start       = _line->start;
    statement->end         = _line->next;
    statement->line        = _line->line;
    statement->end_line    = _line->end_line;
    statement->modifiers   = empty;
    statement->word        = empty;
    statement->targets     = empty;
    statement->colon       = empty;
    statement->pattern     = empty;
    statement->name        = empty;
    statement->separator   = empty;
    statement->text        = empty;
    statement->order_only  = empty;
    statement->recipe      = empty;
    statement->body        = empty;
    statement->next_branch = D_MAKE_NONE;

    return make->statement_count++;
}


/*
d_make_reader_comment
  A comment's line is found from its start, so a reader gives only places.
*/
bool
d_make_reader_comment(
    struct d_make_reader* _reader,
    size_t                _start,
    size_t                _end,
    size_t                _within
)
{
    struct d_make* const make = _reader->make;

    // room for the comment
    if (!d_internal_grow((void**)&make->comments,
                         &_reader->comment_capacity,
                         make->comment_count,
                         sizeof(*make->comments)))
    {
        return d_make_reader_fail(_reader,
                                  d_make_line_of(make, _start),
                                  "out of memory");
    }

    struct d_make_comment* const comment =
        &make->comments[make->comment_count];

    comment->start  = _start;
    comment->end    = _end;
    comment->line   = d_make_line_of(make, _start);
    comment->within = _within;
    make->comment_count++;

    return true;
}


//==============================================================================
// 5.  SCANNING
//==============================================================================


/**
 * @brief Gives the length of the continuation at a place: a backslash and
 *        the line ending after it, or 0 for none.
 *
 * @param[in] _text  the text.
 * @param[in] _at    the place.
 * @param[in] _end   where the logical line ends.
 * @return 2 for a backslash and LF, 3 for a backslash and CR LF, or 0.
 *
 * Within a logical line every line feed is a continuation's, so a backslash
 * just before one is the continuation's own.
 */
static size_t
d_internal_continuation(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    // a backslash, then a line ending
    if ( (_at + 1u < _end) &&
         (_text[_at] == '\\') )
    {
        // a table of the two endings
        if (_text[_at + 1u] == '\n')
        {
            return 2u;
        }
        else if ( (_at + 2u < _end) &&
                  (_text[_at + 1u] == '\r') &&
                  (_text[_at + 2u] == '\n') )
        {
            return 3u;
        }
    }

    return 0u;
}


/**
 * @brief Reports whether a byte is one make reads as a blank.
 *
 * @param[in] _byte  the byte.
 * @return `true` for a space, a tab, a vertical tab, a form feed, a CR or a
 *         line feed.
 */
static bool
d_internal_is_blank(
    char _byte
)
{
    return ( (_byte == ' ')  ||
             (_byte == '\t') ||
             (_byte == '\v') ||
             (_byte == '\f') ||
             (_byte == '\r') ||
             (_byte == '\n') );
}


/*
d_make_skip_blanks
  A continuation is taken whole, so its backslash is never left as a byte of
the word after it.
*/
size_t
d_make_skip_blanks(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    size_t at = _at;

    // each blank, or continuation, in turn
    while (at < _end)
    {
        const size_t continuation = d_internal_continuation(_text, at, _end);

        // a table of what stops the run, and what it skips
        if (continuation > 0u)
        {
            at += continuation;
        }
        else if (d_internal_is_blank(_text[at]))
        {
            at++;
        }
        else
        {
            break;
        }
    }

    return at;
}


/*
d_make_trim_end
  A line feed met from behind is taken back with its CR and the backslash of
its continuation.
*/
size_t
d_make_trim_end(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    size_t end = _end;

    // each blank before the end, in turn
    while ( (end > _at) &&
            (d_internal_is_blank(_text[end - 1u])) )
    {
        end--;

        // a line feed takes its continuation's CR and backslash with it
        if (_text[end] == '\n')
        {
            // the CR of a CR LF
            if ( (end > _at) &&
                 (_text[end - 1u] == '\r') )
            {
                end--;
            }

            // the continuation's backslash
            if ( (end > _at) &&
                 (_text[end - 1u] == '\\') )
            {
                end--;
            }
        }
    }

    return end;
}


/*
d_make_word_end
  A word ends at a blank, as make's end_of_token finds it: a reference's
brackets do not hold a word together.
*/
size_t
d_make_word_end(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    size_t at = _at;

    // the bytes of the word
    while ( (at < _end) &&
            (!d_internal_is_blank(_text[at])) &&
            (d_internal_continuation(_text, at, _end) == 0u) )
    {
        at++;
    }

    return at;
}


/*
d_make_find
  Backslash is the escape, as GNU make's find_map_unquote has it.
*/
size_t
d_make_find(
    const char* _text,
    size_t      _at,
    size_t      _end,
    const char* _stops
)
{
    return d_make_find_with(_text,
                            _at,
                            _end,
                            _stops,
                            '\\');
}


/*
d_make_find_with
  As make's find_map_unquote looks: a reference is stepped over whole, and a
stop counts only when the escapes before it are even in number.
*/
size_t
d_make_find_with(
    const char* _text,
    size_t      _at,
    size_t      _end,
    const char* _stops,
    char        _escape
)
{
    size_t at = _at;

    // each byte, references stepped over
    while (at < _end)
    {
        const char byte = _text[at];

        // a reference is skipped whole
        if (byte == '$')
        {
            at = d_make_reference_end(_text, at, _end);

            continue;
        }

        // a stop, unless an odd run of backslashes escapes it
        if ( (byte != '\0') &&
             (strchr(_stops, byte)) )
        {
            size_t escapes = 0;

            // the escapes before it
            while ( (at - escapes > _at) &&
                    (_text[at - escapes - 1u] == _escape) )
            {
                escapes++;
            }

            // an even run leaves it a stop
            if ((escapes % 2u) == 0u)
            {
                return at;
            }
        }

        at++;
    }

    return _end;
}


/*
d_make_is_word
  Compared by length first, so a word that only begins with another is not
it.
*/
bool
d_make_is_word(
    const char* _text,
    size_t      _start,
    size_t      _end,
    const char* _word
)
{
    const size_t length = strlen(_word);

    return ( (_end - _start == length) &&
             (memcmp(_text + _start, _word, length) == 0) );
}


/*
d_make_reference_end
  Only brackets of the reference's own kind are counted, as make counts
them, so `$(a{)` closes at its `)`.
*/
size_t
d_make_reference_end(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    // a `$` that ends the line is a byte alone
    if (_at + 1u >= _end)
    {
        return _end;
    }

    const char open = _text[_at + 1u];

    // `$x` and `$$`
    if ( (open != '(') &&
         (open != '{') )
    {
        return _at + 2u;
    }

    const char close = (open == '(') ? ')' : '}';
    size_t     depth = 1;

    // each byte, to the bracket that matches
    for (size_t at = _at + 2u; at < _end; at++)
    {
        // a table of the brackets that count
        if (_text[at] == open)
        {
            depth++;
        }
        else if ( (_text[at] == close) &&
                  (--depth == 0u) )
        {
            return at + 1u;
        }
    }

    return _end;
}


//==============================================================================
// 6.  READING
//==============================================================================


// the kinds' names, in the order of d_make_kind
static const char* const g_kind_names[] =
{
    "assignment",
    "define",
    "rule",
    "recipe",
    "conditional",
    "include",
    "directive",
    "expansion",
    "loop"
};


/*
d_make_kind_name
  The table above, read by the enum's value.
*/
const char*
d_make_kind_name(
    enum d_make_kind _kind
)
{
    // a value past the table names nothing
    if ((size_t)_kind >= (sizeof(g_kind_names) / sizeof(g_kind_names[0])))
    {
        return "";
    }

    return g_kind_names[_kind];
}


/*
d_make_read
  The lines are read first, by the make's rule for joining them; its reader
then takes them into statements.
*/
bool
d_make_read(
    struct d_make*         _out,
    const char*            _text,
    size_t                 _length,
    enum d_make_dialect    _dialect,
    struct d_make_problem* _problem
)
{
    // nowhere to put the records
    if (!_out)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    _out->text    = _text ? _text : "";
    _out->length  = _text ? _length : 0u;
    _out->dialect = _dialect;

    struct d_make_reader reader;

    memset(&reader, 0, sizeof(reader));
    reader.make    = _out;
    reader.problem = _problem;

    // nothing yet to say why reading stopped
    if (_problem)
    {
        _problem->text[0] = '\0';
    }

    // the logical lines
    if (!d_make_lines_read(&_out->lines,
                           _out->text,
                           _out->length,
                           _dialect))
    {
        return d_make_reader_fail(&reader, 1u, "out of memory");
    }

    // a table of the makes
    if (_dialect == D_MAKE_POSIX)
    {
        return d_make_read_posix(&reader);
    }
    else if (_dialect == D_MAKE_BSD)
    {
        return d_make_read_bsd(&reader);
    }
    else if (_dialect == D_MAKE_NMAKE)
    {
        return d_make_read_nmake(&reader);
    }

    return d_make_read_gnu(&reader);
}


/*
d_make_release
  Leaves the record empty, so a second release does nothing.
*/
void
d_make_release(
    struct d_make* _make
)
{
    // nothing to release
    if (!_make)
    {
        return;
    }

    d_make_lines_release(&_make->lines);
    free(_make->statements);
    free(_make->comments);
    memset(_make, 0, sizeof(*_make));

    return;
}


/*
d_make_line_of
  The logical line that holds the place is found by halving, and the line
feeds before the place within it are counted.
  Long as two searches, each a short loop: the logical line by halving, then
the line feeds within it.
*/
size_t
d_make_line_of(
    const struct d_make* _make,
    size_t               _at
)
{
    // no lines, the first
    if ( (!_make) ||
         (_make->lines.count == 0u) )
    {
        return 1u;
    }

    const struct d_make_line* const lines = _make->lines.lines;
    size_t                          low   = 0;
    size_t                          high  = _make->lines.count;

    // the last line that starts at or before the place
    while (high - low > 1u)
    {
        const size_t middle = low + ((high - low) / 2u);

        // a table of the halves
        if (lines[middle].start <= _at)
        {
            low = middle;
        }
        else
        {
            high = middle;
        }
    }

    size_t line = lines[low].line;

    // the line feeds between its start and the place
    for (size_t at = lines[low].start;
         (at < _at) && (at < _make->length);
         at++)
    {
        // a line feed begins a line
        if (_make->text[at] == '\n')
        {
            line++;
        }
    }

    return line;
}


/*
d_make_words
  A word ends at a blank, a reference stepped over whole, so the blanks
within `$(...)` do not split it.
*/
size_t
d_make_words(
    const struct d_make* _make,
    struct d_make_span   _span,
    struct d_make_span*  _out,
    size_t               _room
)
{
    const char* const text  = _make->text;
    size_t            count = 0;
    size_t            at    = d_make_skip_blanks(text, _span.start, _span.end);

    // each word
    while (at < _span.end)
    {
        size_t end = at;

        // its bytes, references whole, to a blank
        while ( (end < _span.end) &&
                (d_make_skip_blanks(text, end, _span.end) == end) )
        {
            end = (text[end] == '$') ? d_make_reference_end(text,
                                                            end,
                                                            _span.end)
                                     : (end + 1u);
        }

        // room for its place
        if ( (_out) &&
             (count < _room) )
        {
            _out[count].start = at;
            _out[count].end   = end;
        }

        count++;
        at = d_make_skip_blanks(text, end, _span.end);
    }

    return count;
}
