/*******************************************************************************
* djinterp [tools]                                                   dmake_bsd.c
*
* Reads a makefile's statements as BSD make, as bmake reads one: see
* dmake.h.
*   bmake's parse loop, a line at a time. A line led by a tab is a command,
* its comment the shell's. Any other line loses its comment -- a `#` no
* backslash escapes and no `[` stands before, as in `:[#]` -- and a line
* with `#` in its first column, or only blanks, is passed over. A line with
* `.` in its first column is tried, blanks after the `.` skipped, as a
* conditional, as Cond_EvalLine reads one: `endif`, `else` and `elif` as
* words, `if` then `def`, `ndef`, `make` or `nmake` as prefixes, so `.ifx`
* is `.if x`; then as a loop, `.for` to the `.endfor` that closes it, nested
* ones counted; then as an include or one of the other directives, each a
* prefix of the line, as Parse_File tries them. Then a line is an include in
* System V's form, or gmake's export; an assignment, as Parse_IsVar finds
* one -- outside any bracket, the first blank followed only by an operator;
* in a rule, an indented line with no `:` or `!` is a command; and anything
* else is a dependency, its first `:`, `::` or `!` the operator, its first
* `;` after that beginning a command. An assignment ends a rule; a
* directive, an include and a conditional do not.
*   Refused here, in bmake's words, is what bmake refuses whatever the
* variables hold: an `endif`, `else` or `elif` with no `if` open, a
* conditional open at the end (at line 0, where bmake names it), an
* `endfor` with no `for`, a `for` no `endfor` closes. A loop's body is a
* scope of its own, as bmake reads one when it runs the loop: its
* conditionals must balance within it, and a line of it is numbered as
* bmake numbers the body's, from the `.for`, its empty and comment lines
* not counted. Every branch is read as one bmake evaluates: in a branch it
* skips, bmake reads no loop, so a `.for` or `.endfor` there counts for
* nothing, and its conditionals count in the scope about it.
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake_bsd.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./dmake_internal.h"  // corresponding header
// std
#include <stdio.h>             // snprintf
#include <stdlib.h>            // realloc, free
#include <string.h>            // memcmp, memset, strlen


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    The reader
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_bsd
//   struct: where bmake's parse loop stands: whether a rule is above, and
//   the last branch of each conditional open.
struct d_internal_bsd
{
    struct d_make_reader* reader;
    const char*           text;
    bool                  in_rule;
    size_t*               opens;
    size_t                open_count;
    size_t                open_capacity;
};


//==============================================================================
// 2.  SCANNING
//==============================================================================


/**
 * @brief Reports whether a byte is a letter, as isalpha reads one in the C
 *        locale.
 *
 * @param[in] _byte  the byte.
 * @return `true` for A to Z and a to z.
 */
static bool
d_internal_is_alpha(
    char _byte
)
{
    return ( ( (_byte >= 'a') &&
               (_byte <= 'z') ) ||
             ( (_byte >= 'A') &&
               (_byte <= 'Z') ) );
}


/**
 * @brief Reports whether a word begins at a place, as bmake's istoken does:
 *        no letter after it.
 *
 * @param[in] _text  the text.
 * @param[in] _at    the place.
 * @param[in] _end   where the line ends.
 * @param[in] _word  the word.
 * @return `true` if the word is there, and no letter follows it.
 */
static bool
d_internal_token(
    const char* _text,
    size_t      _at,
    size_t      _end,
    const char* _word
)
{
    const size_t length = strlen(_word);

    return ( (_at + length <= _end) &&
             (memcmp(_text + _at, _word, length) == 0) &&
             ( (_at + length == _end) ||
               (!d_internal_is_alpha(_text[_at + length])) ) );
}


/**
 * @brief Reports whether a text begins at a place, as strncmp finds it.
 *
 * @param[in] _text    the text.
 * @param[in] _at      the place.
 * @param[in] _end     where the line ends.
 * @param[in] _prefix  the text sought.
 * @return `true` if it is there.
 */
static bool
d_internal_prefix(
    const char* _text,
    size_t      _at,
    size_t      _end,
    const char* _prefix
)
{
    const size_t length = strlen(_prefix);

    return ( (_at + length <= _end) &&
             (memcmp(_text + _at, _prefix, length) == 0) );
}


/**
 * @brief Reports whether a place holds a blank, a continuation among them,
 *        or is the line's end.
 *
 * @param[in] _text  the text.
 * @param[in] _at    the place.
 * @param[in] _end   where the line ends.
 * @return `true` if a blank is there or the line ends there.
 */
static bool
d_internal_blank_or_end(
    const char* _text,
    size_t      _at,
    size_t      _end
)
{
    return ( (_at >= _end) ||
             (d_make_skip_blanks(_text, _at, _end) > _at) );
}


/**
 * @brief Finds a line's comment as bmake's ParseGetLine does.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where the line starts.
 * @param[in] _end    where it ends.
 * @return the place of its `#`, or `_end`.
 *
 * A backslash escapes the byte after it, so `\#` is no comment and `\\#`
 * is; a `#` after `[` is the `:[#]` modifier's, and no comment.
 */
static size_t
d_internal_comment_at(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    // each byte, a backslash's pair taken whole
    for (size_t at = _start; at < _end; at++)
    {
        // a table of the bytes that matter: an escape, and a `#`
        if (_text[at] == '\\')
        {
            at++;
        }
        else if ( (_text[at] == '#') &&
                  ( (at == _start) ||
                    (_text[at - 1u] != '[') ) )
        {
            return at;
        }
    }

    return _end;
}


/**
 * @brief Finds a byte of a set that no backslash escapes, as bmake's
 *        ParseIsEscaped tests one, with nothing stepped over.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where to start.
 * @param[in] _end    where to stop.
 * @param[in] _stops  the bytes sought.
 * @return its place, or `_end`.
 */
static size_t
d_internal_raw_find(
    const char* _text,
    size_t      _start,
    size_t      _end,
    const char* _stops
)
{
    // each byte, a backslash's pair taken whole
    for (size_t at = _start; at < _end; at++)
    {
        // a table of the two: an escape, and a byte sought
        if (_text[at] == '\\')
        {
            at++;
        }
        else if (memchr(_stops, _text[at], strlen(_stops)))
        {
            return at;
        }
    }

    return _end;
}


/**
 * @brief Finds an assignment's operator as bmake's Parse_IsVar does.
 *
 * @param[in]  _text   the text.
 * @param[in]  _start  where the line starts.
 * @param[in]  _end    where its code ends.
 * @param[out] _out    receives the operator: `=`, or `+=`, `:=`, `?=`,
 *                     `!=`.
 * @return `true` if the line is an assignment.
 *
 * Brackets of either kind are counted wherever they stand, not only after
 * a `$`, and a blank outside them must be followed by the operator; the
 * byte after a run of blanks is taken as the operator's or not, its
 * bracket not counted, as Parse_IsVar's loop has it.
 *
 * Long as Parse_IsVar's one loop, whose cases -- blanks, brackets, operators
 * -- stay together so the order of its tests is bmake's.
 */
static bool
d_internal_is_var(
    const char*         _text,
    size_t              _start,
    size_t              _end,
    struct d_make_span* _out
)
{
    size_t at    = d_make_skip_blanks(_text, _start, _end);
    long   level = 0;
    bool   space = false;
    bool   after = false;

    // each byte, while the line may yet be an assignment
    while (at < _end)
    {
        const size_t blanks = d_make_skip_blanks(_text, at, _end);

        // a run of blanks: outside brackets, only an operator may follow
        if (blanks > at)
        {
            space = (space) || (level == 0);
            after = (level == 0);
            at    = blanks;

            continue;
        }

        const char byte = _text[at++];

        // a table of the brackets, which are not counted after blanks
        if ( (!after) &&
             ( (byte == '(') ||
               (byte == '{') ) )
        {
            level++;

            continue;
        }
        else if ( (!after) &&
                  ( (byte == ')') ||
                    (byte == '}') ) )
        {
            level--;

            continue;
        }
        else if ( (!after) &&
                  (level != 0) )
        {
            continue;
        }

        after = false;

        // a table of the operators, and what rules them out
        if (byte == '=')
        {
            _out->start = at - 1u;
            _out->end   = at;

            return true;
        }
        else if ( (at < _end) &&
                  (_text[at] == '=') &&
                  ( (byte == '+') ||
                    (byte == ':') ||
                    (byte == '?') ||
                    (byte == '!') ) )
        {
            _out->start = at - 1u;
            _out->end   = at + 1u;

            return true;
        }
        else if (space)
        {
            return false;
        }
    }

    return false;
}


//==============================================================================
// 3.  RECORDS
//==============================================================================


/**
 * @brief Records a statement over a line with its word and text, and the
 *        line's comment.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in]     _line     the line.
 * @param[in]     _kind     the statement's kind.
 * @param[in]     _word     its word.
 * @param[in]     _comment  where the line's comment starts, or its end.
 * @return the statement's index, or D_MAKE_NONE when memory runs out.
 *
 * The text is what follows the word, blanks skipped, to the comment, its
 * trailing blanks no part of it.
 */
static size_t
d_internal_record(
    struct d_internal_bsd*    _bsd,
    const struct d_make_line* _line,
    enum d_make_kind          _kind,
    struct d_make_span        _word,
    size_t                    _comment
)
{
    const size_t index = d_make_reader_statement(_bsd->reader, _kind, _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return index;
    }

    struct d_make_statement* const statement =
        &_bsd->reader->make->statements[index];
    const size_t                   start     = d_make_skip_blanks(_bsd->text,
                                                                  _word.end,
                                                                  _comment);

    statement->word       = _word;
    statement->text.start = start;
    statement->text.end   = d_make_trim_end(_bsd->text, start, _comment);

    // the line's comment
    if ( (_comment < _line->end) &&
         (!d_make_reader_comment(_bsd->reader,
                                 _comment,
                                 _line->end,
                                 index)) )
    {
        return D_MAKE_NONE;
    }

    return index;
}


/**
 * @brief Records a command's line.
 *
 * @param[in,out] _bsd   the reader.
 * @param[in]     _line  the line.
 * @return `true`; `false` when memory runs out.
 *
 * Its word is the blanks that lead it and the run of `@`, `-` and `+`
 * after them; the rest is the command, comments and all.
 */
static bool
d_internal_command(
    struct d_internal_bsd*    _bsd,
    const struct d_make_line* _line
)
{
    const char* const text  = _bsd->text;
    size_t            flags = d_make_skip_blanks(text,
                                                 _line->start,
                                                 _line->end);

    // the flags after the blanks
    while ( (flags < _line->end) &&
            ( (text[flags] == '@') ||
              (text[flags] == '-') ||
              (text[flags] == '+') ) )
    {
        flags++;
    }

    const struct d_make_span word  = { _line->start, flags };
    const size_t             index = d_internal_record(_bsd,
                                                       _line,
                                                       D_MAKE_RECIPE,
                                                       word,
                                                       _line->end);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    _bsd->reader->make->statements[index].text.start = flags;
    _bsd->reader->make->statements[index].text.end   = _line->end;
    _bsd->reader->make->statements[index].orphan     = !_bsd->in_rule;

    return true;
}


//==============================================================================
// 4.  CONDITIONALS AND LOOPS
//==============================================================================


/**
 * @brief Finds the word of a conditional as Cond_EvalLine reads one.
 *
 * @param[in]  _text  the text.
 * @param[in]  _at    where the word starts, past the `.` and its blanks.
 * @param[in]  _end   where the line's code ends.
 * @param[out] _word  receives the word: endif, else, or an if or elif and
 *                    its form.
 * @return 0 for no conditional; 1 for an if; 2 for an elif; 3 for an
 *         else; 4 for an endif.
 */
static int
d_internal_conditional_word(
    const char*         _text,
    size_t              _at,
    size_t              _end,
    struct d_make_span* _word
)
{
    static const char* const forms[] = { "def", "ndef", "make", "nmake" };

    size_t at    = _at;
    int    which = 1;

    _word->start = _at;

    // a table of the words led by `e`: endif, else, elif
    if ( (at < _end) &&
         (_text[at] == 'e') &&
         ( (at + 1u >= _end) ||
           (_text[at + 1u] != 'l') ) )
    {
        _word->end = _at + 5u;

        return d_internal_token(_text, at + 1u, _end, "ndif") ? 4 : 0;
    }
    else if (d_internal_prefix(_text, at, _end, "el"))
    {
        // else
        if (d_internal_token(_text, at + 2u, _end, "se"))
        {
            _word->end = _at + 4u;

            return 3;
        }

        at   += 2u;
        which = 2;
    }

    // an if, or an elif: `if`, then its form, each a prefix
    if (!d_internal_prefix(_text, at, _end, "if"))
    {
        return 0;
    }

    at += 2u;

    // the first form that begins the rest
    for (size_t form = 0; form < sizeof(forms) / sizeof(forms[0]); form++)
    {
        // this form
        if (d_internal_prefix(_text, at, _end, forms[form]))
        {
            at += strlen(forms[form]);

            break;
        }
    }

    _word->end = at;

    return which;
}


/**
 * @brief Records a conditional, as Cond_EvalLine counts them.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in]     _line     the line.
 * @param[in]     _which    what it is: 1 if, 2 elif, 3 else, 4 endif.
 * @param[in]     _word     its word.
 * @param[in]     _comment  where the line's comment starts, or its end.
 * @return `true`; `false` when bmake refuses it or memory runs out.
 *
 * Long as counting's two halves, opening one and continuing one, kept
 * together as Cond_EvalLine keeps them.
 */
static bool
d_internal_conditional(
    struct d_internal_bsd*    _bsd,
    const struct d_make_line* _line,
    int                       _which,
    struct d_make_span        _word,
    size_t                    _comment
)
{
    static const char* const lacking[] =
    {
        "", "", "if-less elif", "if-less else", "if-less endif"
    };

    // an elif, an else or an endif with no if open
    if ( (_which > 1) &&
         (_bsd->open_count == 0u) )
    {
        return d_make_reader_fail(_bsd->reader, _line->line, lacking[_which]);
    }

    const size_t index = d_internal_record(_bsd,
                                           _line,
                                           D_MAKE_CONDITIONAL,
                                           _word,
                                           _comment);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_bsd->reader->make->statements[index];

    // a table of the two: an if opens one; the rest continue it
    if (_which == 1)
    {
        // room for it
        if (_bsd->open_count == _bsd->open_capacity)
        {
            const size_t wanted = (_bsd->open_capacity > 0u)
                                      ? (_bsd->open_capacity * 2u)
                                      : 8u;
            void* const  grown  = realloc(_bsd->opens,
                                          wanted * sizeof(*_bsd->opens));

            // memory that ran out
            if (!grown)
            {
                return d_make_reader_fail(_bsd->reader,
                                          _line->line,
                                          "out of memory");
            }

            _bsd->opens         = grown;
            _bsd->open_capacity = wanted;
        }

        statement->depth = _bsd->open_count;
        _bsd->opens[_bsd->open_count++] = index;
    }
    else
    {
        statement->depth = _bsd->open_count - 1u;
        _bsd->reader->make->statements[_bsd->opens[_bsd->open_count - 1u]]
            .next_branch = index;
        _bsd->opens[_bsd->open_count - 1u] = index;
        _bsd->open_count -= (_which == 4) ? 1u : 0u;
    }

    return true;
}


/**
 * @brief Reports whether a line opens or closes a loop, as For_Accum reads
 *        one: `.`, blanks, then `for` and a blank, or `endfor` and a blank
 *        or the line's end.
 *
 * @param[in] _text  the text.
 * @param[in] _line  the line.
 * @return 1 for a `.for`, -1 for an `.endfor`, 0 for neither.
 */
static int
d_internal_loop_line(
    const char*               _text,
    const struct d_make_line* _line
)
{
    // a line not led by `.`
    if ( (_line->start >= _line->end) ||
         (_text[_line->start] != '.') )
    {
        return 0;
    }

    const size_t at = d_make_skip_blanks(_text, _line->start + 1u, _line->end);

    // a table of the two
    if ( (d_internal_prefix(_text, at, _line->end, "endfor")) &&
         (d_internal_blank_or_end(_text, at + 6u, _line->end)) )
    {
        return -1;
    }
    else if ( (d_internal_prefix(_text, at, _line->end, "for")) &&
              (at + 3u < _line->end) &&
              (d_internal_blank_or_end(_text, at + 3u, _line->end)) )
    {
        return 1;
    }

    return 0;
}


/**
 * @brief Counts the conditionals of a loop's body, a scope of its own, as
 *        bmake reads the body when it runs the loop.
 *
 * @param[in,out] _bsd    the reader.
 * @param[in]     _first  the index of the `.for` line.
 * @param[in]     _close  the index of the `.endfor` line.
 * @return `true` if they balance; `false` when bmake refuses them, an
 *         `elif`, `else` or `endif` with none open at its line, or one
 *         left open at the `.for`'s.
 *
 * Long as one pass over the body, its lines numbered as bmake numbers them
 * while their conditionals are counted.
 */
static bool
d_internal_loop_scope(
    struct d_internal_bsd* _bsd,
    size_t                 _first,
    size_t                 _close
)
{
    static const char* const lacking[] =
    {
        "", "", "if-less elif", "if-less else", "if-less endif"
    };

    const struct d_make_line* const lines  = _bsd->reader->make->lines.lines;
    const char* const               text   = _bsd->text;
    size_t                          open   = 0;
    size_t                          number = lines[_first].line;

    // each line of the body led by `.`
    for (size_t index = _first + 1u; index < _close; index++)
    {
        const struct d_make_line* const line    = &lines[index];
        struct d_make_span              word;
        const size_t                    comment = d_internal_comment_at(
                                                      text,
                                                      line->start,
                                                      line->end);

        // an empty line, or one a comment begins, is not the body's
        if ( (line->start == line->end) ||
             (comment == line->start) )
        {
            continue;
        }

        number += line->end_line - line->line + 1u;

        // a line not led by `.` holds no conditional
        if (text[line->start] != '.')
        {
            continue;
        }

        const int    which   = d_internal_conditional_word(
                                   text,
                                   d_make_skip_blanks(text,
                                                      line->start + 1u,
                                                      comment),
                                   comment,
                                   &word);

        // a table of the words: an if opens; the rest need one open
        if (which == 1)
        {
            open++;
        }
        else if ( (which > 1) &&
                  (open == 0u) )
        {
            return d_make_reader_fail(_bsd->reader,
                                      number,
                                      lacking[which]);
        }
        else if (which == 4)
        {
            open--;
        }
    }

    // one left open, named at the loop's line
    if (open > 0u)
    {
        char message[64];

        (void)snprintf(message,
                       sizeof(message),
                       "%lu open conditional%s",
                       (unsigned long)open,
                       (open == 1u) ? "" : "s");

        return d_make_reader_fail(_bsd->reader,
                                  lines[_first].line,
                                  message);
    }

    return true;
}


/**
 * @brief Records a loop, from its `.for` to the `.endfor` that closes it.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in]     _index    the line of its `.for`, its index among the lines;
 *                          receives the index of its `.endfor`'s.
 * @param[in]     _word     its word, `for`.
 * @param[in]     _comment  where the `.for` line's comment starts, or its
 *                          end.
 * @return `true`; `false` when bmake refuses it or memory runs out.
 *
 * Long as a loop's parts and its body, each a few lines: the variables to
 * `in`, the values after it, and the lines to the `.endfor`.
 */
static bool
d_internal_loop(
    struct d_internal_bsd* _bsd,
    size_t*                _index,
    struct d_make_span     _word,
    size_t                 _comment
)
{
    const struct d_make_lines* const lines = &_bsd->reader->make->lines;
    const struct d_make_line* const  first = &lines->lines[*_index];
    const char* const                text  = _bsd->text;
    size_t                           depth = 1;
    size_t                           close = *_index;

    // each line after it, to the `.endfor` that closes it
    while ( (depth > 0u) &&
            (++close < lines->count) )
    {
        depth = (size_t)((long)depth + d_internal_loop_line(text,
                                                     &lines->lines[close]));
    }

    const struct d_make_line* const end     = &lines->lines[lines->count - 1u];
    const bool                      counted = ( (end->next > end->end) ||
                                                ( (end->start < end->end) &&
                                                  (text[end->start] !=
                                                   '#') ) );

    // the text's end before the `.endfor`, named one line past the last, as
    // bmake counts lines: a last line no line feed ends counts unless a
    // comment begins it, which bmake reads past uncounted
    if (close >= lines->count)
    {
        return d_make_reader_fail(_bsd->reader,
                                  end->end_line + (counted ? 1u : 0u),
                                  "Unexpected end of file in for loop.");
    }

    // the body's conditionals, which must balance within it
    if (!d_internal_loop_scope(_bsd, *_index, close))
    {
        return false;
    }

    const size_t index = d_internal_record(_bsd,
                                           first,
                                           D_MAKE_LOOP,
                                           _word,
                                           _comment);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const   statement =
        &_bsd->reader->make->statements[index];
    const struct d_make_line* const  last      = &lines->lines[close];
    const struct d_make_span         header    = statement->text;
    size_t                           at        = header.start;

    // the variables, to the word `in`
    while ( (at < header.end) &&
            (!( (d_internal_prefix(text, at, header.end, "in")) &&
                (at + 2u < header.end) &&
                (d_internal_blank_or_end(text, at + 2u, header.end)) )) )
    {
        at = d_make_skip_blanks(text,
                                d_make_word_end(text, at, header.end),
                                header.end);
    }

    statement->name.start = header.start;
    statement->name.end   = d_make_trim_end(text, header.start, at);

    // a table of the two: `in` and the values after it, or none
    if (at < header.end)
    {
        statement->separator.start = at;
        statement->separator.end   = at + 2u;
        statement->text.start      = d_make_skip_blanks(text,
                                                        at + 2u,
                                                        header.end);
    }
    else
    {
        statement->separator.start = header.end;
        statement->separator.end   = header.end;
        statement->text.start      = header.end;
    }

    statement->body.start = first->next;
    statement->body.end   = last->start;
    statement->end        = last->next;
    statement->end_line   = last->end_line;
    *_index               = close;

    const size_t comment = d_internal_comment_at(text, last->start, last->end);

    // a comment after the `.endfor`
    if (comment < last->end)
    {
        return d_make_reader_comment(_bsd->reader,
                                     comment,
                                     last->end,
                                     index);
    }

    return true;
}


//==============================================================================
// 5.  LINES
//==============================================================================


/**
 * @brief Tries a `.` line as the other directives Parse_File tries, each a
 *        prefix: the includes, undef, export, unexport, and the messages.
 *
 * @param[in]  _text   the text.
 * @param[in]  _at     where the directive's word starts, past the `.`.
 * @param[in]  _end    where the line's code ends.
 * @param[out] _word   receives its word.
 * @param[out] _kind   receives its kind.
 * @return `true` if the line is one of them.
 *
 * An include's word may be led by `d`, `s` or `-`, and needs no blank
 * after it; a message's word is the run of letters, and a blank must
 * follow it, or ParseMessage leaves the line to be a dependency.
 */
static bool
d_internal_directive_word(
    const char*         _text,
    size_t              _at,
    size_t              _end,
    struct d_make_span* _word,
    enum d_make_kind*   _kind
)
{
    const size_t lead = ( (_at < _end) &&
                          ( (_text[_at] == 'd') ||
                            (_text[_at] == 's') ||
                            (_text[_at] == '-') ) ) ? 1u : 0u;
    size_t       end  = _at;

    _word->start = _at;
    *_kind       = D_MAKE_DIRECTIVE;

    // a table of the directives, in Parse_File's order
    if (d_internal_prefix(_text, _at + lead, _end, "include"))
    {
        end   = _at + lead + 7u;
        *_kind = D_MAKE_INCLUDE;
    }
    else if (d_internal_prefix(_text, _at, _end, "undef"))
    {
        end = _at + 5u;
    }
    else if (d_internal_prefix(_text, _at, _end, "export"))
    {
        end = _at + 6u;
    }
    else if (d_internal_prefix(_text, _at, _end, "unexport"))
    {
        end = _at + 8u;
    }
    else if ( (d_internal_prefix(_text, _at, _end, "info")) ||
              (d_internal_prefix(_text, _at, _end, "error")) ||
              (d_internal_prefix(_text, _at, _end, "warning")) )
    {
        end = _at;

        // the run of letters
        while ( (end < _end) &&
                (d_internal_is_alpha(_text[end])) )
        {
            end++;
        }

        // a blank must follow it
        if (d_make_skip_blanks(_text, end, _end) == end)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    _word->end = end;

    return true;
}


/**
 * @brief Takes a line led by `.`: a conditional, a loop, or one of the
 *        other directives.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in,out] _index    the line's index; receives the last line taken.
 * @param[in]     _comment  where the line's comment starts, or its end.
 * @param[out]    _taken    receives whether the line was a directive.
 * @return `true`; `false` when bmake refuses it or memory runs out.
 */
static bool
d_internal_dot(
    struct d_internal_bsd* _bsd,
    size_t*                _index,
    size_t                 _comment,
    bool*                  _taken
)
{
    const struct d_make_line* const line =
        &_bsd->reader->make->lines.lines[*_index];
    const char* const               text = _bsd->text;
    const size_t                    at   = d_make_skip_blanks(text,
                                                              line->start + 1u,
                                                              _comment);
    struct d_make_span              word;
    enum d_make_kind                kind  = D_MAKE_DIRECTIVE;
    const int                       which = d_internal_conditional_word(
                                                text,
                                                at,
                                                _comment,
                                                &word);
    const int                       loop  = d_internal_loop_line(text, line);

    *_taken = true;

    // a table of the directives, in bmake's order: conditionals, loops,
    // the others
    if (which > 0)
    {
        return d_internal_conditional(_bsd, line, which, word, _comment);
    }
    else if (loop < 0)
    {
        return d_make_reader_fail(_bsd->reader, line->line, "for-less endfor");
    }
    else if (loop > 0)
    {
        word.start = at;
        word.end   = at + 3u;

        return d_internal_loop(_bsd, _index, word, _comment);
    }
    else if (d_internal_directive_word(text, at, _comment, &word, &kind))
    {
        return (d_internal_record(_bsd,
                                  line,
                                  kind,
                                  word,
                                  _comment) != D_MAKE_NONE);
    }

    *_taken = false;

    return true;
}


/**
 * @brief Records an assignment found.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in]     _line     the line.
 * @param[in]     _op       its operator.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when memory runs out.
 *
 * Its name runs from the line's first byte that is not a blank to the
 * first blank outside a bracket, or the operator, as Parse_DoVar has it.
 */
static bool
d_internal_assignment(
    struct d_internal_bsd*    _bsd,
    const struct d_make_line* _line,
    struct d_make_span        _op,
    size_t                    _comment
)
{
    const char* const        text  = _bsd->text;
    const size_t             name  = d_make_skip_blanks(text,
                                                        _line->start,
                                                        _op.start);
    const struct d_make_span empty = { name, name };
    const size_t             index = d_internal_record(_bsd,
                                                       _line,
                                                       D_MAKE_ASSIGNMENT,
                                                       empty,
                                                       _comment);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_bsd->reader->make->statements[index];
    size_t                         end       = name;
    long                           depth     = 0;

    // the name, to a blank outside brackets or the operator
    while ( (end < _op.start) &&
            ( (depth != 0) ||
              (d_make_skip_blanks(text, end, _op.start) == end) ) )
    {
        depth += ( (text[end] == '(') || (text[end] == '{') ) ? 1 : 0;
        depth -= ( (text[end] == ')') || (text[end] == '}') ) ? 1 : 0;
        end++;
    }

    const size_t value = d_make_skip_blanks(text, _op.end, _comment);

    statement->name.start = name;
    statement->name.end   = end;
    statement->separator  = _op;
    statement->text.start = value;
    statement->text.end   = d_make_trim_end(text, value, _comment);
    _bsd->in_rule         = false;

    return true;
}


/**
 * @brief Records a dependency, or a line with no operator, which only its
 *        expansion can give one.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in]     _line     the line.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when memory runs out.
 */
static bool
d_internal_dependency(
    struct d_internal_bsd*    _bsd,
    const struct d_make_line* _line,
    size_t                    _comment
)
{
    const char* const        text  = _bsd->text;
    const size_t             at    = d_make_skip_blanks(text,
                                                        _line->start,
                                                        _comment);
    const size_t             op    = d_make_find(text, at, _comment, ":!");
    const struct d_make_span empty = { at, at };
    const size_t             index = d_internal_record(
                                         _bsd,
                                         _line,
                                         (op < _comment) ? D_MAKE_RULE
                                                         : D_MAKE_EXPANSION,
                                         empty,
                                         _comment);

    _bsd->in_rule = true;

    // memory that ran out, or no operator: an expansion, its text the line
    if ( (index == D_MAKE_NONE) ||
         (op == _comment) )
    {
        return (index != D_MAKE_NONE);
    }

    struct d_make_statement* const statement =
        &_bsd->reader->make->statements[index];
    const size_t                   after     =
        ( (text[op] == ':') &&
          (op + 1u < _comment) &&
          (text[op + 1u] == ':') ) ? (op + 2u) : (op + 1u);
    const size_t                   semicolon = d_make_find(text,
                                                           after,
                                                           _comment,
                                                           ";");
    const size_t                   from      = d_make_skip_blanks(text,
                                                                  after,
                                                                  semicolon);

    statement->targets.start    = at;
    statement->targets.end      = d_make_trim_end(text, at, op);
    statement->colon.start      = op;
    statement->colon.end        = after;
    statement->text.start       = from;
    statement->text.end         = d_make_trim_end(text, from, semicolon);
    statement->order_only.start = statement->text.end;
    statement->order_only.end   = statement->text.end;
    statement->recipe.start     = (semicolon < _comment) ? (semicolon + 1u)
                                                         : _comment;
    statement->recipe.end       = _comment;

    return true;
}


/**
 * @brief Takes a line that is not a command's, as Parse_File takes it.
 *
 * @param[in,out] _bsd      the reader.
 * @param[in,out] _index    the line's index; receives the last line taken.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when bmake refuses it or memory runs out.
 *
 * Long as Parse_File's order of trials, each a few lines: the directives,
 * System V's include, gmake's export, an assignment, an indented command,
 * and a dependency.
 */
static bool
d_internal_line(
    struct d_internal_bsd* _bsd,
    size_t*                _index,
    size_t                 _comment
)
{
    const struct d_make_line* const line =
        &_bsd->reader->make->lines.lines[*_index];
    const char* const               text = _bsd->text;
    bool                            taken = false;

    // a directive led by `.`
    if ( (text[line->start] == '.') &&
         ( (!d_internal_dot(_bsd, _index, _comment, &taken)) ||
           (taken) ) )
    {
        return (_bsd->reader->make->error[0] == '\0');
    }

    const bool   colon = (d_internal_raw_find(text,
                                              line->start,
                                              _comment,
                                              ":") < _comment);
    const size_t start = line->start;
    size_t       word  = start;

    // a table of System V's include, its kin, and gmake's export: a word
    // and a blank, and no `:` on the line
    if ( (!colon) &&
         ( (d_internal_prefix(text, start, _comment, "include")) ||
           (d_internal_prefix(text, start, _comment, "-include")) ||
           (d_internal_prefix(text, start, _comment, "sinclude")) ||
           (d_internal_prefix(text, start, _comment, "export")) ) )
    {
        word = d_make_word_end(text, start, _comment);
    }

    const struct d_make_span words = { start, word };

    // an include or an export
    if ( (word > start) &&
         (word < _comment) &&
         ( (d_make_is_word(text, start, word, "include")) ||
           (d_make_is_word(text, start, word, "-include")) ||
           (d_make_is_word(text, start, word, "sinclude")) ||
           (d_make_is_word(text, start, word, "export")) ) )
    {
        return (d_internal_record(_bsd,
                                  line,
                                  (text[start] == 'e') ? D_MAKE_DIRECTIVE
                                                       : D_MAKE_INCLUDE,
                                  words,
                                  _comment) != D_MAKE_NONE);
    }

    struct d_make_span op;

    // an assignment
    if (d_internal_is_var(text, start, _comment, &op))
    {
        return d_internal_assignment(_bsd, line, op, _comment);
    }

    const size_t first = d_make_skip_blanks(text, start, _comment);

    // in a rule, an indented line with no operator is a command
    if ( (_bsd->in_rule) &&
         (first > start) &&
         (d_internal_raw_find(text, first, _comment, ":!") == _comment) )
    {
        return d_internal_command(_bsd, line);
    }

    return d_internal_dependency(_bsd, line, _comment);
}


/*
d_make_read_bsd
  Each line is taken by the reading bmake's parse loop gives it; a loop
takes its lines whole. At the end, a conditional still open is refused at
line 0, as bmake names it.
  Long as the parse loop's three readings of a line and the check at the
end, each a few lines.
*/
bool
d_make_read_bsd(
    struct d_make_reader* _reader
)
{
    struct d_internal_bsd bsd;

    memset(&bsd, 0, sizeof(bsd));
    bsd.reader = _reader;
    bsd.text   = _reader->make->text;

    const struct d_make_lines* const lines = &_reader->make->lines;
    bool                             read  = true;

    // each logical line, while reading goes on
    for (size_t index = 0; (read) && (index < lines->count); index++)
    {
        const struct d_make_line* const line    = &lines->lines[index];
        const size_t                    first   = d_make_skip_blanks(
                                                      bsd.text,
                                                      line->start,
                                                      line->end);
        const size_t                    comment = d_internal_comment_at(
                                                      bsd.text,
                                                      line->start,
                                                      line->end);

        // a table of the lines: a command, one bmake passes over, the rest
        if ( (line->start < line->end) &&
             (bsd.text[line->start] == '\t') )
        {
            read = ( (first == line->end) ||
                     (d_internal_command(&bsd, line)) );
        }
        else if (d_make_skip_blanks(bsd.text, line->start, comment) ==
                 comment)
        {
            read = ( (comment == line->end) ||
                     (d_make_reader_comment(_reader,
                                            comment,
                                            line->end,
                                            D_MAKE_NONE)) );
        }
        else
        {
            read = d_internal_line(&bsd, &index, comment);
        }
    }

    // a conditional still open at the end
    if ( (read) &&
         (bsd.open_count > 0u) )
    {
        char message[64];

        (void)snprintf(message,
                       sizeof(message),
                       "%lu open conditional%s",
                       (unsigned long)bsd.open_count,
                       (bsd.open_count == 1u) ? "" : "s");
        read = d_make_reader_fail(_reader, 0u, message);
    }

    free(bsd.opens);

    return read;
}
