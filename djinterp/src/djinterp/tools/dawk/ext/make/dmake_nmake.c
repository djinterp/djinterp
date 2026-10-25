/*******************************************************************************
* djinterp [tools]                                                 dmake_nmake.c
*
* Reads a makefile's statements as Microsoft's NMake reads one, as its
* documentation describes it: see dmake.h.
*   A line with `!` in its first column is a preprocessing directive, its
* keyword in any case after blanks: the conditionals -- IF, IFDEF, IFNDEF,
* ELSE and its ELSEIF forms, ENDIF -- counted as they nest, INCLUDE, and
* MESSAGE, ERROR, UNDEF and CMDSWITCHES. A line with `#` in its first column
* is a comment. After a dependency or an inference rule, a line led by a
* space or a tab is a command, its modifiers `@`, `-` and a number, `!`
* before it; a `#` after it is a comment; a `<<` in it begins an inline
* file, its lines to one that begins `<<` its own. An empty line straight
* after a rule ends it, as no blank line may stand between a rule and its
* commands; one among the commands does not. Any other line is a macro when
* an `=` comes before any `:`, its comment after it; a rule when a `:` or
* `::` does -- a single letter at a word's start and its colon are a
* drive -- its first `;` after the colon beginning a command that keeps any
* `#`, and a `#` before that a comment. `^` escapes the byte after it.
*   Refused here, in NMake's words, is what NMake refuses whatever its
* macros hold: an `!ELSE`, an `!ELSEIF` or an `!ENDIF` with no `!IF` open
* (U1021; the documentation gives the text for `!ELSE`, and the others are
* worded the same), and a conditional still open at the end (U1020).
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake_nmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./dmake_internal.h"  // corresponding header
// std
#include <stdio.h>             // snprintf
#include <stdlib.h>            // realloc, free
#include <string.h>            // memset, strlen


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    The reader
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_nmake
//   struct: where NMake's reading stands: whether a rule is above, how many
//   commands it has, and the last branch of each conditional open.
struct d_internal_nmake
{
    struct d_make_reader* reader;
    const char*           text;
    bool                  in_rule;
    size_t                commands;
    size_t*               opens;
    size_t                open_count;
    size_t                open_capacity;
};


//==============================================================================
// 2.  SCANNING
//==============================================================================


/**
 * @brief Reports whether a byte is a letter.
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
 * @brief Reports whether a place holds a word, compared in either case.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where the word starts.
 * @param[in] _end    where it ends.
 * @param[in] _word   the word, in upper case.
 * @return `true` if the bytes are the word's.
 */
static bool
d_internal_is_keyword(
    const char* _text,
    size_t      _start,
    size_t      _end,
    const char* _word
)
{
    const size_t length = strlen(_word);

    // a word of another length is another word
    if (_end - _start != length)
    {
        return false;
    }

    // each byte, folded to upper case
    for (size_t index = 0; index < length; index++)
    {
        const char byte  = _text[_start + index];
        const char upper = ( (byte >= 'a') &&
                             (byte <= 'z') ) ? (char)(byte - 'a' + 'A')
                                             : byte;

        // a byte that differs
        if (upper != _word[index])
        {
            return false;
        }
    }

    return true;
}


/**
 * @brief Finds a macro's `=` or a rule's colon, whichever comes first.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where the line starts.
 * @param[in] _end    where its code ends.
 * @return the place of the first `=` or `:`, or `_end`.
 *
 * References are stepped over and `^` escapes; a colon after a single
 * letter that begins a word, or follows a `{`, is a drive's, as NMake reads
 * `c:\src`, and is passed over.
 */
static size_t
d_internal_separator(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    size_t at = _start;

    // each `=` or `:`, until one is a separator
    while ( (at = d_make_find_with(_text, at, _end, "=:", '^')) < _end)
    {
        const bool drive = ( (_text[at] == ':') &&
                             (at > _start) &&
                             (d_internal_is_alpha(_text[at - 1u])) &&
                             ( (at - 1u == _start) ||
                               (_text[at - 2u] == ' ') ||
                               (_text[at - 2u] == '\t') ||
                               (_text[at - 2u] == '{') ) );

        // a separator, not a drive's colon
        if (!drive)
        {
            return at;
        }

        at++;
    }

    return _end;
}


//==============================================================================
// 3.  RECORDS
//==============================================================================


/**
 * @brief Records a statement over a line, with its word and its text to a
 *        place, and a comment from there to the line's end, if one is
 *        there.
 *
 * @param[in,out] _nmake    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _kind     the statement's kind.
 * @param[in]     _word     its word; its text starts after it, blanks
 *                          skipped.
 * @param[in]     _comment  where the line's comment starts, or its end.
 * @return the statement's index, or D_MAKE_NONE when memory runs out.
 */
static size_t
d_internal_record(
    struct d_internal_nmake*  _nmake,
    const struct d_make_line* _line,
    enum d_make_kind          _kind,
    struct d_make_span        _word,
    size_t                    _comment
)
{
    const size_t index = d_make_reader_statement(_nmake->reader,
                                                 _kind,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return index;
    }

    struct d_make_statement* const statement =
        &_nmake->reader->make->statements[index];
    const size_t                   start     = d_make_skip_blanks(
                                                   _nmake->text,
                                                   _word.end,
                                                   _comment);

    statement->word       = _word;
    statement->text.start = start;
    statement->text.end   = d_make_trim_end(_nmake->text, start, _comment);

    // the line's comment
    if ( (_comment < _line->end) &&
         (!d_make_reader_comment(_nmake->reader,
                                 _comment,
                                 _line->end,
                                 index)) )
    {
        return D_MAKE_NONE;
    }

    return index;
}


/**
 * @brief Records a conditional, as the preprocessor counts them.
 *
 * @param[in,out] _nmake    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _word     its keyword.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when NMake refuses it or memory runs out.
 *
 * Long as counting's two halves, opening one and continuing one, kept
 * together as the preprocessor keeps them.
 */
static bool
d_internal_conditional(
    struct d_internal_nmake*  _nmake,
    const struct d_make_line* _line,
    struct d_make_span        _word,
    size_t                    _comment
)
{
    const char* const text    = _nmake->text;
    const bool        opens   = ( (d_internal_is_keyword(text, _word.start,
                                                         _word.end, "IF")) ||
                                  (d_internal_is_keyword(text, _word.start,
                                                         _word.end,
                                                         "IFDEF")) ||
                                  (d_internal_is_keyword(text, _word.start,
                                                         _word.end,
                                                         "IFNDEF")) );
    const bool        closes  = d_internal_is_keyword(text,
                                                      _word.start,
                                                      _word.end,
                                                      "ENDIF");
    char              message[96];

    // an else or an endif with no if open
    if ( (!opens) &&
         (_nmake->open_count == 0u) )
    {
        (void)snprintf(message,
                       sizeof(message),
                       "fatal error U1021: syntax error : !%s unexpected",
                       closes ? "ENDIF" : "ELSE");

        return d_make_reader_fail(_nmake->reader, _line->line, message);
    }

    const size_t index = d_internal_record(_nmake,
                                           _line,
                                           D_MAKE_CONDITIONAL,
                                           _word,
                                           _comment);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    // a table of the two: an if opens one; the rest continue it
    if (opens)
    {
        // room for it
        if (_nmake->open_count == _nmake->open_capacity)
        {
            const size_t wanted = (_nmake->open_capacity > 0u)
                                      ? (_nmake->open_capacity * 2u)
                                      : 8u;
            void* const  grown  = realloc(_nmake->opens,
                                          wanted * sizeof(*_nmake->opens));

            // memory that ran out
            if (!grown)
            {
                return d_make_reader_fail(_nmake->reader,
                                          _line->line,
                                          "out of memory");
            }

            _nmake->opens         = grown;
            _nmake->open_capacity = wanted;
        }

        _nmake->reader->make->statements[index].depth = _nmake->open_count;
        _nmake->opens[_nmake->open_count++] = index;
    }
    else
    {
        const size_t last = _nmake->opens[_nmake->open_count - 1u];

        _nmake->reader->make->statements[index].depth =
            _nmake->open_count - 1u;
        _nmake->reader->make->statements[last].next_branch = index;
        _nmake->opens[_nmake->open_count - 1u]             = index;
        _nmake->open_count -= closes ? 1u : 0u;
    }

    return true;
}


/**
 * @brief Takes a preprocessing directive's line.
 *
 * @param[in,out] _nmake    the reader.
 * @param[in]     _line     the line.
 * @param[in]     _comment  where its comment starts, or its end.
 * @return `true`; `false` when NMake refuses it or memory runs out.
 *
 * `!ELSE IF`, two words, is the `!ELSE` whose text is a conditional, as
 * the documentation allows it; a keyword NMake has not is left to
 * dmake_eval as an expansion's line.
 *
 * Long as the keyword's three tables -- the conditionals, INCLUDE, the
 * others -- each a short loop.
 */
static bool
d_internal_directive(
    struct d_internal_nmake*  _nmake,
    const struct d_make_line* _line,
    size_t                    _comment
)
{
    static const char* const conditionals[] =
    {
        "IF", "IFDEF", "IFNDEF", "ELSE", "ELSEIF", "ELSEIFDEF",
        "ELSEIFNDEF", "ENDIF"
    };
    static const char* const others[] =
    {
        "MESSAGE", "ERROR", "UNDEF", "CMDSWITCHES"
    };

    const char* const  text = _nmake->text;
    const size_t       at   = d_make_skip_blanks(text,
                                                 _line->start + 1u,
                                                 _comment);
    struct d_make_span word = { at, at };

    // the keyword's letters
    while ( (word.end < _comment) &&
            (d_internal_is_alpha(text[word.end])) )
    {
        word.end++;
    }

    // each conditional's keyword
    for (size_t index = 0;
         index < sizeof(conditionals) / sizeof(conditionals[0]);
         index++)
    {
        // this one
        if (d_internal_is_keyword(text, word.start, word.end,
                                  conditionals[index]))
        {
            return d_internal_conditional(_nmake, _line, word, _comment);
        }
    }

    enum d_make_kind kind = D_MAKE_EXPANSION;

    // a table of the others: INCLUDE, then the rest
    if (d_internal_is_keyword(text, word.start, word.end, "INCLUDE"))
    {
        kind = D_MAKE_INCLUDE;
    }
    else
    {
        // each of the other keywords
        for (size_t index = 0;
             index < sizeof(others) / sizeof(others[0]);
             index++)
        {
            // this one
            if (d_internal_is_keyword(text, word.start, word.end,
                                      others[index]))
            {
                kind = D_MAKE_DIRECTIVE;
            }
        }
    }

    // a keyword NMake has not: the whole line is the text
    if (kind == D_MAKE_EXPANSION)
    {
        word.start = _line->start;
        word.end   = _line->start;
    }

    return (d_internal_record(_nmake,
                              _line,
                              kind,
                              word,
                              _comment) != D_MAKE_NONE);
}


//==============================================================================
// 4.  COMMANDS
//==============================================================================


/**
 * @brief Counts the inline files a command begins: each `<<` no `^`
 *        escapes.
 *
 * @param[in] _text   the text.
 * @param[in] _start  where the command starts.
 * @param[in] _end    where it ends.
 * @return how many.
 */
static size_t
d_internal_inline_files(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    size_t count = 0;
    size_t at    = _start;

    // each `<` that another follows
    while ( (at = d_make_find_with(_text, at, _end, "<", '^')) + 1u < _end)
    {
        // a pair
        if (_text[at + 1u] == '<')
        {
            count++;
            at += 2u;

            continue;
        }

        at++;
    }

    return count;
}


/**
 * @brief Records a command's line, and the inline files it begins.
 *
 * @param[in,out] _nmake  the reader.
 * @param[in,out] _index  the line's index; receives the last line taken.
 * @return `true`; `false` when memory runs out.
 *
 * Its word is the blanks that lead it and its modifiers -- `@`, `-` with
 * the number after it, `!` -- and the blanks among them; the rest, to a
 * `#`, is the command. Each `<<` takes the lines after it to one that
 * begins `<<`, which closes the file; they are the statement's body.
 *
 * Long as a command's two parts, its modifiers and its inline files, each a
 * short loop.
 */
static bool
d_internal_command(
    struct d_internal_nmake* _nmake,
    size_t*                  _index
)
{
    const struct d_make_lines* const lines   = &_nmake->reader->make->lines;
    const struct d_make_line* const  line    = &lines->lines[*_index];
    const char* const                text    = _nmake->text;
    const size_t                     comment = d_make_find_with(text,
                                                                line->start,
                                                                line->end,
                                                                "#",
                                                                '^');
    size_t                           flags   = line->start;

    // the blanks and the modifiers
    while ( (flags < comment) &&
            ( (text[flags] == ' ') ||
              (text[flags] == '\t') ||
              (text[flags] == '@') ||
              (text[flags] == '-') ||
              (text[flags] == '!') ||
              ( (text[flags] >= '0') &&
                (text[flags] <= '9') &&
                (flags > line->start) &&
                (text[flags - 1u] == '-') ) ) )
    {
        flags++;

        // a number after `-`, whole
        while ( (flags < comment) &&
                (text[flags - 1u] >= '0') &&
                (text[flags - 1u] <= '9') &&
                (text[flags] >= '0') &&
                (text[flags] <= '9') )
        {
            flags++;
        }
    }

    const struct d_make_span word  = { line->start, flags };
    const size_t             index = d_internal_record(_nmake,
                                                       line,
                                                       D_MAKE_RECIPE,
                                                       word,
                                                       comment);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_nmake->reader->make->statements[index];
    size_t                         files     = d_internal_inline_files(text,
                                                                       flags,
                                                                       comment);
    size_t                         last      = *_index;

    statement->text.start = flags;
    statement->orphan     = !_nmake->in_rule;
    statement->body.start = line->next;
    statement->body.end   = line->next;
    _nmake->commands++;

    // each inline file, to the line that closes it
    while ( (files > 0u) &&
            (last + 1u < lines->count) )
    {
        const struct d_make_line* const next = &lines->lines[++last];

        // a line that begins `<<` closes one
        if ( (next->end - next->start >= 2u) &&
             (text[next->start] == '<') &&
             (text[next->start + 1u] == '<') )
        {
            files--;
            statement->body.end = next->start;
        }
        else
        {
            statement->body.end = next->next;
        }

        statement->end      = next->next;
        statement->end_line = next->end_line;
    }

    *_index = last;

    return true;
}


//==============================================================================
// 5.  LINES
//==============================================================================


/**
 * @brief Records a macro, or a rule, or a line with neither separator.
 *
 * @param[in,out] _nmake  the reader.
 * @param[in]     _line   the line.
 * @return `true`; `false` when memory runs out.
 *
 * Long as the two kinds' parts, each a few lines: a macro's name, `=` and
 * value to its comment; a rule's targets, colon, dependents to its `;` or
 * comment, and its command.
 */
static bool
d_internal_definition(
    struct d_internal_nmake*  _nmake,
    const struct d_make_line* _line
)
{
    const char* const        text      = _nmake->text;
    const size_t             comment   = d_make_find_with(text,
                                                          _line->start,
                                                          _line->end,
                                                          "#",
                                                          '^');
    const size_t             separator = d_internal_separator(text,
                                                              _line->start,
                                                              comment);
    const bool               rule      = ( (separator < comment) &&
                                           (text[separator] == ':') );
    const size_t             stop      = rule ? d_make_find_with(text,
                                                                 separator,
                                                                 _line->end,
                                                                 ";#",
                                                                 '^')
                                              : comment;
    const size_t             end       = ( (stop < _line->end) &&
                                           (text[stop] == ';') ) ? _line->end
                                                                 : stop;
    const struct d_make_span empty     = { _line->start, _line->start };
    const size_t             index     = d_internal_record(
                                             _nmake,
                                             _line,
                                             (separator == comment)
                                                 ? D_MAKE_EXPANSION
                                                 : ( rule ? D_MAKE_RULE
                                                          : D_MAKE_ASSIGNMENT ),
                                             empty,
                                             end);

    _nmake->in_rule  = rule;
    _nmake->commands = 0;

    // memory that ran out, or neither separator: the line is the text
    if ( (index == D_MAKE_NONE) ||
         (separator == comment) )
    {
        return (index != D_MAKE_NONE);
    }

    struct d_make_statement* const statement =
        &_nmake->reader->make->statements[index];
    const size_t                   after     =
        ( (rule) &&
          (separator + 1u < stop) &&
          (text[separator + 1u] == ':') ) ? (separator + 2u)
                                          : (separator + 1u);
    const size_t                   from      = d_make_skip_blanks(text,
                                                                  after,
                                                                  stop);

    statement->text.start = from;
    statement->text.end   = d_make_trim_end(text, from, stop);

    // a table of the two: a rule's parts, or a macro's
    if (rule)
    {
        statement->targets.start    = _line->start;
        statement->targets.end      = d_make_trim_end(text,
                                                      _line->start,
                                                      separator);
        statement->colon.start      = separator;
        statement->colon.end        = after;
        statement->order_only.start = statement->text.end;
        statement->order_only.end   = statement->text.end;
        statement->recipe.start     = (stop < end) ? (stop + 1u) : end;
        statement->recipe.end       = end;
    }
    else
    {
        statement->name.start      = _line->start;
        statement->name.end        = d_make_trim_end(text,
                                                     _line->start,
                                                     separator);
        statement->separator.start = separator;
        statement->separator.end   = after;
    }

    return true;
}


/**
 * @brief Takes one line.
 *
 * @param[in,out] _nmake  the reader.
 * @param[in,out] _index  the line's index; receives the last line taken.
 * @return `true`; `false` when NMake refuses it or memory runs out.
 */
static bool
d_internal_line(
    struct d_internal_nmake* _nmake,
    size_t*                  _index
)
{
    const struct d_make_line* const line =
        &_nmake->reader->make->lines.lines[*_index];
    const char* const               text = _nmake->text;
    const bool                      led  = ( (line->start < line->end) &&
                                             ( (text[line->start] == ' ') ||
                                               (text[line->start] ==
                                                '\t') ) );

    // a table of the lines: a directive, a comment, an empty line, a
    // command, the rest
    if ( (line->start < line->end) &&
         (text[line->start] == '!') )
    {
        return d_internal_directive(_nmake,
                                    line,
                                    d_make_find_with(text,
                                                     line->start,
                                                     line->end,
                                                     "#",
                                                     '^'));
    }
    else if ( (line->start < line->end) &&
              (text[line->start] == '#') )
    {
        return d_make_reader_comment(_nmake->reader,
                                     line->start,
                                     line->end,
                                     D_MAKE_NONE);
    }
    else if (line->start == line->end)
    {
        _nmake->in_rule = ( (_nmake->in_rule) &&
                            (_nmake->commands > 0u) );

        return true;
    }
    else if (led)
    {
        const size_t first = d_make_skip_blanks(text, line->start, line->end);

        // blanks alone outside a rule are nothing
        if ( (first == line->end) &&
             (!_nmake->in_rule) )
        {
            return true;
        }

        return d_internal_command(_nmake, _index);
    }

    return d_internal_definition(_nmake, line);
}


/*
d_make_read_nmake
  Each line is taken in turn, a command taking its inline files' lines
with it; at the end, a conditional still open is refused, at the line past
the last, as U1020 says it.
*/
bool
d_make_read_nmake(
    struct d_make_reader* _reader
)
{
    struct d_internal_nmake nmake;

    memset(&nmake, 0, sizeof(nmake));
    nmake.reader = _reader;
    nmake.text   = _reader->make->text;

    const struct d_make_lines* const lines = &_reader->make->lines;
    bool                             read  = true;

    // each logical line, while reading goes on
    for (size_t index = 0; (read) && (index < lines->count); index++)
    {
        read = d_internal_line(&nmake, &index);
    }

    // a conditional still open at the end
    if ( (read) &&
         (nmake.open_count > 0u) )
    {
        read = d_make_reader_fail(
                   _reader,
                   lines->lines[lines->count - 1u].end_line + 1u,
                   "fatal error U1020: end-of-file found before next "
                   "directive");
    }

    free(nmake.opens);

    return read;
}
