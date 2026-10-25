/*******************************************************************************
* djinterp [tools]                                                 dmake_posix.c
*
* Reads a makefile's statements as POSIX make, as pdpmake reads one in its
* POSIX mode: see dmake.h.
*   pdpmake's input loop, a line at a time. A line of blanks, or with `#` in
* its first column, is passed over, and a rule's commands go on past it.
* While a rule is above, a line led by a tab is a command. Any other line
* loses its comment -- the first `#` on it, which nothing escapes, in a
* command too -- and is an include when `include` or `-include` and a blank
* begin it; a macro when it holds an `=` outside every reference, its
* operator `=`, `::=`, `:::=`, `+=`, `?=` or `!=` as the bytes before the
* `=` make it; otherwise a rule, its first `:` separating targets from
* prerequisites and its first `;` after that beginning a command. POSIX
* mode reads no conditional, no `::` and no `:=`.
*   With no conditional to hide a line, what pdpmake refuses of a line
* whatever its macros hold is refused here, in its words: a command where no
* rule is above, a line with no separator and no reference -- a last line
* no line feed ends among them, which pdpmake takes for neither blank nor
* comment -- a macro whose
* left side is not one word, an include that names nothing -- each at the
* line pdpmake names, a continued line's last, and the line before for a
* last line no line feed ends.
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake_posix.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./dmake_internal.h"  // corresponding header
// std
#include <string.h>            // memchr, memcmp, memset


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    The reader
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_posix
//   struct: where pdpmake's input loop stands: the text, and whether a rule
//   is above, whose commands a tab-led line would be.
struct d_internal_posix
{
    struct d_make_reader* reader;
    const char*           text;
    bool                  in_rule;
};


//==============================================================================
// 2.  SCANNING
//==============================================================================


/**
 * @brief Steps over a reference as pdpmake's skip_macro does.
 *
 * @param[in] _text  the text.
 * @param[in] _at    the place of the reference's `$`.
 * @param[in] _end   where the line ends.
 * @return the place past the reference.
 *
 * Unlike GNU's, the count is of references, not brackets: within `$(` the
 * first `)` that no inner reference holds closes it, so `$(a(b)c)` ends at
 * its first `)`.
 */
static size_t
d_internal_skip(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    // a `$` that ends the line
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
    size_t     at    = _at + 2u;

    // to the bracket that closes it, inner references stepped over
    while ( (at < _end) &&
            (_text[at] != close) )
    {
        at = (_text[at] == '$') ? d_internal_skip(_text, at, _end)
                                : (at + 1u);
    }

    return (at < _end) ? (at + 1u) : _end;
}


/**
 * @brief Finds a byte outside every reference, as pdpmake's find_char does.
 *
 * @param[in] _text  the text.
 * @param[in] _at    where to start.
 * @param[in] _end   where to stop.
 * @param[in] _byte  the byte.
 * @return its place, or `_end`.
 */
static size_t
d_internal_find(
    const char* _text,
    size_t      _at,
    size_t      _end,
    char        _byte
)
{
    size_t at = _at;

    // each byte, references stepped over
    while (at < _end)
    {
        // a table of the three: a reference, the byte, another
        if (_text[at] == '$')
        {
            at = d_internal_skip(_text, at, _end);
        }
        else if (_text[at] == _byte)
        {
            return at;
        }
        else
        {
            at++;
        }
    }

    return _end;
}


/**
 * @brief Reports whether a part of a line holds a reference.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where the part starts.
 * @param[in] _end    where it ends.
 * @return `true` if a `$` is in it.
 */
static bool
d_internal_refers(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    return ( (_end > _start) &&
             (memchr(_text + _start, '$', _end - _start) != NULL) );
}


/**
 * @brief Finds a byte as strchr does, references and all.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where to start.
 * @param[in] _end    where to stop.
 * @param[in] _byte   the byte.
 * @return its place, or `_end`.
 */
static size_t
d_internal_first(
    const char* _text,
    size_t      _start,
    size_t      _end,
    char        _byte
)
{
    const char* const found = (_end > _start)
                                  ? memchr(_text + _start,
                                           _byte,
                                           _end - _start)
                                  : NULL;

    return found ? (size_t)(found - _text) : _end;
}


//==============================================================================
// 3.  STATEMENTS
//==============================================================================


/**
 * @brief Refuses a line, at the line pdpmake names.
 *
 * @param[in,out] _posix    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _message  why, in pdpmake's words.
 * @return `false`.
 *
 * pdpmake counts a physical line as it reads the line feed that ends it,
 * and names a logical line by its last: so a continued line is named by its
 * last physical line, and a last line no line feed ends by the line before.
 */
static bool
d_internal_refuse(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line,
    const char*               _message
)
{
    const bool unended = (_line->next == _line->end);

    return d_make_reader_fail(_posix->reader,
                              _line->end_line - (unended ? 1u : 0u),
                              _message);
}


/**
 * @brief Records a comment, the first `#` of a line to its end, when there
 *        is one.
 *
 * @param[in,out] _posix    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _comment  where its `#` stands, or its end.
 * @param[in]     _within   the statement the line holds, or D_MAKE_NONE.
 * @return `true`; `false` when memory runs out.
 */
static bool
d_internal_comment(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line,
    size_t                    _comment,
    size_t                    _within
)
{
    // no comment
    if (_comment == _line->end)
    {
        return true;
    }

    return d_make_reader_comment(_posix->reader,
                                 _comment,
                                 _line->end,
                                 _within);
}


/**
 * @brief Records a rule's command line.
 *
 * @param[in,out] _posix    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when memory runs out.
 */
static bool
d_internal_command(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line,
    size_t                    _comment
)
{
    const size_t index = d_make_reader_statement(_posix->reader,
                                                 D_MAKE_RECIPE,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    const char* const text  = _posix->text;
    size_t            flags = _line->start + 1u;

    // the flags after the tab
    while ( (flags < _comment) &&
            ( (text[flags] == '@') ||
              (text[flags] == '-') ||
              (text[flags] == '+') ||
              (text[flags] == ' ') ||
              (text[flags] == '\t') ) )
    {
        flags++;
    }

    struct d_make_statement* const statement =
        &_posix->reader->make->statements[index];

    statement->word.start = _line->start;
    statement->word.end   = flags;
    statement->text.start = flags;
    statement->text.end   = _comment;

    return d_internal_comment(_posix, _line, _comment, index);
}


/**
 * @brief Records an include, or refuses one that names nothing.
 *
 * @param[in,out] _posix    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _word     where its word ends: past `include`.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when pdpmake refuses it or memory runs out.
 */
static bool
d_internal_include(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line,
    size_t                    _word,
    size_t                    _comment
)
{
    const char* const text  = _posix->text;
    const size_t      start = d_make_skip_blanks(text, _word, _comment);
    const size_t      end   = d_make_trim_end(text, start, _comment);

    // nothing named, and no reference that could name it
    if (start == end)
    {
        return d_internal_refuse(_posix, _line, "no include file");
    }

    const size_t index = d_make_reader_statement(_posix->reader,
                                                 D_MAKE_INCLUDE,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_posix->reader->make->statements[index];

    statement->word.start = _line->start;
    statement->word.end   = _word;
    statement->text.start = start;
    statement->text.end   = end;

    return d_internal_comment(_posix, _line, _comment, index);
}


/**
 * @brief Finds a macro's operator: the `=` and what before it makes it
 *        another, as pdpmake reads them.
 *
 * @param[in]  _text   the text.
 * @param[in]  _start  where the line starts.
 * @param[in]  _equal  the `=`.
 * @param[out] _out    receives the operator.
 *
 * Each byte before the `=` counts only when another stands before it, as
 * pdpmake's `q - 1 > copy2` has it; `:=` is no operator in POSIX mode, so
 * its colon is the name's.
 */
static void
d_internal_operator(
    const char*         _text,
    size_t              _start,
    size_t              _equal,
    struct d_make_span* _out
)
{
    _out->start = _equal;
    _out->end   = _equal + 1u;

    // nothing before the `=` that could join it
    if (_equal < _start + 2u)
    {
        return;
    }

    const char before = _text[_equal - 1u];

    // a table of the operators: `::=` and `:::=`, then `+=`, `?=`, `!=`
    if ( (before == ':') &&
         (_equal >= _start + 3u) &&
         (_text[_equal - 2u] == ':') )
    {
        const bool three = ( (_equal >= _start + 4u) &&
                             (_text[_equal - 3u] == ':') );

        _out->start = _equal - (three ? 3u : 2u);
    }
    else if ( (before == '+') ||
              (before == '?') ||
              (before == '!') )
    {
        _out->start = _equal - 1u;
    }

    return;
}


/**
 * @brief Records a macro, refusing one whose left side is not one word, or
 *        reports that the line is a rule's after all.
 *
 * @param[in,out] _posix    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _equal    its `=`.
 * @param[in]     _comment  where its comment starts, or its end.
 * @param[out]    _rule     receives whether the line is a rule's: its
 *                          left side's first word holds `:` and a `;` after
 *                          it, as pdpmake's try_target has it.
 * @return `true`; `false` when pdpmake refuses it or memory runs out.
 */
static bool
d_internal_macro(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line,
    size_t                    _equal,
    size_t                    _comment,
    bool*                     _rule
)
{
    const char* const  text = _posix->text;
    struct d_make_span op;

    d_internal_operator(text, _line->start, _equal, &op);

    const size_t name  = d_make_skip_blanks(text, _line->start, op.start);
    const size_t end   = d_make_trim_end(text, name, op.start);
    const size_t first = d_make_word_end(text, name, end);
    const size_t colon = d_internal_first(text, name, first, ':');

    *_rule = false;

    // a first word that holds `:` and then `;` is a rule's
    if ( (colon < first) &&
         (d_internal_first(text, colon, first, ';') < first) )
    {
        *_rule = true;

        return true;
    }

    // a left side that is not one word, and no reference that could make it
    if ( ( (name == end) ||
           (first < end) ) &&
         (!d_internal_refers(text, name, end)) )
    {
        return d_internal_refuse(_posix, _line, "invalid macro assignment");
    }

    const size_t index = d_make_reader_statement(_posix->reader,
                                                 D_MAKE_ASSIGNMENT,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_posix->reader->make->statements[index];

    statement->name.start = name;
    statement->name.end   = end;
    statement->separator  = op;
    statement->text.start = d_make_skip_blanks(text, _equal + 1u, _comment);
    statement->text.end   = d_make_trim_end(text, statement->text.start,
                                            _comment);

    return d_internal_comment(_posix, _line, _comment, index);
}


/**
 * @brief Records a rule, or an expansion, refusing a line pdpmake refuses
 *        whatever its macros hold.
 *
 * @param[in,out] _posix    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when pdpmake refuses it or memory runs out.
 *
 * Long as a rule's parts, each found after the one before: the colon, the
 * `;`, the prerequisites between.
 */
static bool
d_internal_rule(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line,
    size_t                    _comment
)
{
    const char* const text  = _posix->text;
    const size_t      at    = d_make_skip_blanks(text, _line->start, _comment);
    const size_t      colon = d_internal_find(text, at, _comment, ':');
    const bool        refer = d_internal_refers(text, at, _comment);

    // a tab-led line with no rule above
    if (text[_line->start] == '\t')
    {
        return d_internal_refuse(_posix, _line, "command not allowed here");
    }

    // no separator, and no reference that could make one
    if ( (colon == _comment) &&
         (!refer) )
    {
        return d_internal_refuse(_posix, _line, "expected separator");
    }

    const size_t index = d_make_reader_statement(
                             _posix->reader,
                             (colon == _comment) ? D_MAKE_EXPANSION
                                                 : D_MAKE_RULE,
                             _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_posix->reader->make->statements[index];

    // a table of the two: an expansion, or a rule and its parts
    if (colon == _comment)
    {
        statement->text.start = at;
        statement->text.end   = d_make_trim_end(text, at, _comment);
    }
    else
    {
        const size_t semicolon = d_internal_find(text,
                                                 colon + 1u,
                                                 _comment,
                                                 ';');
        const size_t from      = d_make_skip_blanks(text,
                                                    colon + 1u,
                                                    semicolon);

        statement->targets.start    = at;
        statement->targets.end      = d_make_trim_end(text, at, colon);
        statement->colon.start      = colon;
        statement->colon.end        = colon + 1u;
        statement->text.start       = from;
        statement->text.end         = d_make_trim_end(text, from, semicolon);
        statement->order_only.start = statement->text.end;
        statement->order_only.end   = statement->text.end;
        statement->recipe.start     = (semicolon < _comment)
                                          ? (semicolon + 1u)
                                          : _comment;
        statement->recipe.end       = _comment;
        _posix->in_rule             = true;
    }

    return d_internal_comment(_posix, _line, _comment, index);
}


/**
 * @brief Takes one line, as pdpmake's input loop takes it.
 *
 * @param[in,out] _posix  the reader.
 * @param[in]     _line   the line.
 * @return `true`; `false` when pdpmake refuses it or memory runs out.
 *
 * Long as the input loop's order of trials, each a few lines: what is
 * passed over, a command, an include, a macro, a rule.
 */
static bool
d_internal_line(
    struct d_internal_posix*  _posix,
    const struct d_make_line* _line
)
{
    const char* const text    = _posix->text;
    const size_t      comment = d_internal_first(text,
                                                 _line->start,
                                                 _line->end,
                                                 '#');
    const size_t      first   = d_make_skip_blanks(text,
                                                   _line->start,
                                                   _line->end);
    const bool        unended = (_line->next == _line->end);

    // a table of what pdpmake passes over or keeps for a rule: a line of
    // blanks, a comment in the first column -- but a last line no line feed
    // ends, which its readline returns unexamined -- and a command
    if ( (first == _line->end) &&
         (!unended) )
    {
        return true;
    }
    else if ( (text[_line->start] == '#') &&
              (!unended) )
    {
        return d_internal_comment(_posix, _line, comment, D_MAKE_NONE);
    }
    else if ( (_posix->in_rule) &&
              (text[_line->start] == '\t') )
    {
        return d_internal_command(_posix, _line, comment);
    }

    _posix->in_rule = false;

    const size_t minus = (text[_line->start] == '-') ? 1u : 0u;
    const size_t word  = _line->start + minus + 7u;
    bool         rule  = false;

    // an include: its word, then a blank
    if ( (word < comment) &&
         (memcmp(text + _line->start + minus, "include", 7u) == 0) &&
         (d_make_skip_blanks(text, word, comment) > word) )
    {
        return d_internal_include(_posix, _line, word, comment);
    }

    const size_t equal = d_internal_find(text, _line->start, comment, '=');

    // a macro, unless its left side makes it a rule's
    if (equal < comment)
    {
        // refused, or memory that ran out
        if (!d_internal_macro(_posix, _line, equal, comment, &rule))
        {
            return false;
        }

        // a macro recorded
        if (!rule)
        {
            return true;
        }
    }

    return d_internal_rule(_posix, _line, comment);
}


/*
d_make_read_posix
  Each line is taken as pdpmake's input loop takes it; nothing is left open
at the end, as POSIX mode reads no conditional.
*/
bool
d_make_read_posix(
    struct d_make_reader* _reader
)
{
    struct d_internal_posix posix;

    memset(&posix, 0, sizeof(posix));
    posix.reader = _reader;
    posix.text   = _reader->make->text;

    const struct d_make_lines* const lines = &_reader->make->lines;

    // each logical line, while reading goes on
    for (size_t index = 0; index < lines->count; index++)
    {
        // a line refused, or memory that ran out
        if (!d_internal_line(&posix, &lines->lines[index]))
        {
            return false;
        }
    }

    return true;
}
