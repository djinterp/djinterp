/*******************************************************************************
* djinterp [parse]                                                    arg_argv.c
*
* Splitting text into arguments and quoting words to survive a split,
* declared in arg_argv.h (2.1), and CLI11's split_up, close_sequence and
* remove_quotes, declared in arg_internal.h (8.3.3) with the check of a set
* of rules, which configuration files share.
*   Four sets of rules turn a line into words, each a loop over the line
* that builds one word at a time. A POSIX shell's and GNU's read quotes and
* backslashes anywhere in a word; the C runtime's reads backslashes only
* before a double quote, and a program's name by a rule of its own; CLI11's
* first cuts the line as split_up cuts it -- a word that opens with a quote
* or a bracket running to its match -- then strips each word's outer quotes.
* d_arg_quote_word writes a word that each set gives back unchanged.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_argv.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_argv.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr, strchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "./arg_internal.h"  // spans, string lists, unescaping
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  CLI11'S SPLIT_UP
//==============================================================================

// OPENERS / CLOSERS
//   constant: CLI11's bracketChars and matchBracketChars, position for
// position: the double quote, whose escapes count; the single and back
// quotes, which close at the next one; and the four brackets, which nest.
static const char OPENERS[] = "\"'`[(<{";
static const char CLOSERS[] = "\"'`])>}";

/*
d_arg_internal_bracket_index
  The position of a character among the openers or the closers, or -1; NUL
is neither, though strchr would find it.
*/
D_STATIC int
d_arg_internal_bracket_index(
    const char* _table,
    char        _c
)
{
    const char* const found = (_c != '\0')
                              ? strchr(_table,
                                       _c)
                              : NULL;

    return (found != NULL)
           ? (int)(found - _table)
           : -1;
}

/*
d_arg_internal_close_quote
  CLI11's close_string_quote: the position of the quote closing the one
opened at `_start`, each character after a backslash skipped -- past the
end, by one, when a backslash is last -- or the length.
*/
D_STATIC size_t
d_arg_internal_close_quote(
    struct d_pack_text _text,
    size_t             _start,
    char               _closure
)
{
    size_t at = _start + 1u;

    // up to the closing quote, escaped characters skipped
    while ( (at < _text.length) &&
            (_text.data[at] != _closure) )
    {
        if (_text.data[at] == '\\')
        {
            at++;
        }

        at++;
    }

    return at;
}

/*
d_arg_internal_close_literal
  CLI11's close_literal_quote: the next `_closure` after `_start`, or the
length.
*/
D_STATIC size_t
d_arg_internal_close_literal(
    struct d_pack_text _text,
    size_t             _start,
    char               _closure
)
{
    const char* const found = ((_start + 1u) < _text.length)
                              ? (const char*)memchr(
                                    _text.data + _start + 1u,
                                    (unsigned char)_closure,
                                    _text.length - _start - 1u)
                              : NULL;

    return (found != NULL)
           ? (size_t)(found - _text.data)
           : _text.length;
}

/*
d_arg_internal_skip_inner
  One character inside a bracketed sequence, as CLI11's close_sequence
treats it: a quote skips to where it closes, and a bracket pushes its
closer onto the closers awaited. Returns the position of the last character
taken.
*/
D_STATIC size_t
d_arg_internal_skip_inner(
    struct d_pack_text   _text,
    size_t               _at,
    struct d_arg_buffer* _awaited
)
{
    const int kind = d_arg_internal_bracket_index(OPENERS,
                                                  _text.data[_at]);

    // a double quote, whose escapes count
    if (kind == 0)
    {
        return d_arg_internal_close_quote(_text,
                                          _at,
                                          _text.data[_at]);
    }

    // a single or back quote
    if ( (kind == 1) ||
         (kind == 2) )
    {
        return d_arg_internal_close_literal(_text,
                                            _at,
                                            _text.data[_at]);
    }

    // a bracket, whose closer is awaited too
    if (kind > 2)
    {
        (void)d_arg_buffer_append_char(_awaited,
                                       CLOSERS[kind]);
    }

    return _at;
}

/*
d_arg_internal_close_brackets
  CLI11's close_sequence for a bracket: the closers awaited kept on a
stack, innermost last, the sequence closing where the stack empties; the
length when it never does.
*/
D_STATIC int
d_arg_internal_close_brackets(
    struct d_pack_text _text,
    size_t             _start,
    char               _closure,
    size_t*            _end
)
{
    struct d_arg_buffer awaited = D_ARG_BUFFER_INIT;
    size_t              at      = _start + 1u;

    (void)d_arg_buffer_append_char(&awaited,
                                   _closure);
    *_end = _text.length;

    // each character, until the stack empties
    while ( (at < _text.length) &&
            (!awaited.failed) )
    {
        // the closer awaited last
        if (_text.data[at] == awaited.data[awaited.length - 1u])
        {
            d_arg_buffer_truncate(&awaited,
                                  awaited.length - 1u);

            if (awaited.length == 0u)
            {
                *_end = at;

                break;
            }
        }

        at = d_arg_internal_skip_inner(_text,
                                       at,
                                       &awaited) + 1u;
    }

    const int status = (awaited.failed)
                       ? D_ARG_ERROR_OUT_OF_MEMORY
                       : D_ARG_OK;

    d_arg_buffer_release(&awaited);

    return status;
}

/*
d_arg_internal_close_sequence
  CLI11's close_sequence: a double quote closes as close_quote finds, a
single or back quote as close_literal finds, and a bracket as
close_brackets finds.
*/
int
d_arg_internal_close_sequence(
    struct d_pack_text _text,
    size_t             _start,
    char               _closure,
    size_t*            _end
)
{
    const int kind = d_arg_internal_bracket_index(CLOSERS,
                                                  _closure);

    // a double quote
    if (kind == 0)
    {
        *_end = d_arg_internal_close_quote(_text,
                                           _start,
                                           _closure);

        return D_ARG_OK;
    }

    // a single or back quote
    if (kind < 3)
    {
        *_end = d_arg_internal_close_literal(_text,
                                             _start,
                                             _closure);

        return D_ARG_OK;
    }

    return d_arg_internal_close_brackets(_text,
                                         _start,
                                         _closure,
                                         _end);
}

/*
d_arg_internal_is_split_point
  CLI11's find_ws in split_up: whitespace when the delimiter is NUL, the
delimiter otherwise.
*/
D_STATIC bool
d_arg_internal_is_split_point(
    char _c,
    char _delimiter
)
{
    return (_delimiter == '\0')
           ? d_arg_internal_is_space(_c)
           : (_c == _delimiter);
}

/*
d_arg_internal_split_sequence
  A word that opens with a quote or a bracket: up to and with its closer --
the rest of the text when it never closes -- and the character after the
closer taken too when it is a split point or an opener, which would start a
sequence of its own.
*/
D_STATIC int
d_arg_internal_split_sequence(
    struct d_pack_text  _text,
    char                _delimiter,
    struct d_pack_text* _word,
    size_t*             _taken
)
{
    const int kind   = d_arg_internal_bracket_index(OPENERS,
                                                    _text.data[0]);
    size_t    end    = 0u;
    const int status = d_arg_internal_close_sequence(
                           _text,
                           0u,
                           CLOSERS[kind],
                           &end);

    // a sequence that never closes is the rest of the text
    if (end >= _text.length)
    {
        *_word  = _text;
        *_taken = _text.length;

        return status;
    }

    const char   follow = ((end + 1u) < _text.length)
                          ? _text.data[end + 1u]
                          : '\0';
    const bool   skip   = ( (d_arg_internal_is_split_point(follow,
                                                           _delimiter)) ||
                            (d_arg_internal_bracket_index(OPENERS,
                                                          follow) >= 0) );
    const size_t next   = end + (skip ? 2u : 1u);

    *_word  = d_arg_internal_span(_text.data,
                                  end + 1u);
    *_taken = (next < _text.length) ? next : _text.length;

    return status;
}

/*
d_arg_internal_split_plain
  Any other word: up to the next split point, which is taken with it.
*/
D_STATIC void
d_arg_internal_split_plain(
    struct d_pack_text  _text,
    char                _delimiter,
    struct d_pack_text* _word,
    size_t*             _taken
)
{
    size_t at = 0u;

    // up to the next split point
    while ( (at < _text.length) &&
            (!d_arg_internal_is_split_point(_text.data[at],
                                            _delimiter)) )
    {
        at++;
    }

    *_word  = d_arg_internal_span(_text.data,
                                  at);
    *_taken = (at < _text.length) ? (at + 1u) : _text.length;

    return;
}

/*
d_arg_internal_split_step
  One turn of CLI11's split_up over trimmed text that is not empty: the next
word, a sequence or a plain word, a span of the text; the rest after it
trimmed again.
*/
int
d_arg_internal_split_step(
    struct d_pack_text* _rest,
    char                _delimiter,
    struct d_pack_text* _word
)
{
    size_t taken  = 0u;
    int    status = D_ARG_OK;

    // a sequence, or a plain word
    if (d_arg_internal_bracket_index(OPENERS,
                                     _rest->data[0]) >= 0)
    {
        status = d_arg_internal_split_sequence(*_rest,
                                               _delimiter,
                                               _word,
                                               &taken);
    }
    else
    {
        d_arg_internal_split_plain(*_rest,
                                   _delimiter,
                                   _word,
                                   &taken);
    }

    *_rest = d_arg_internal_trim(d_arg_internal_span(_rest->data + taken,
                                                     _rest->length - taken));

    return status;
}

/*
d_arg_internal_split_up
  CLI11's split_up over the trimmed text: one word at a time, each copied
into `_out`.
*/
int
d_arg_internal_split_up(
    struct d_pack_text        _text,
    char                      _delimiter,
    struct d_arg_string_list* _out
)
{
    struct d_pack_text rest   = d_arg_internal_trim(_text);
    int                status = D_ARG_OK;

    // a word at a time
    while ( (status == D_ARG_OK) &&
            (rest.length > 0u) )
    {
        struct d_pack_text word = d_arg_internal_span(rest.data,
                                                      0u);

        status = d_arg_internal_split_step(&rest,
                                           _delimiter,
                                           &word);
        status = (status == D_ARG_OK)
                 ? d_arg_internal_string_copy(_out,
                                              word,
                                              NULL)
                 : status;
    }

    return status;
}

//==============================================================================
// 2.  THE WORD UNDER WAY
//==============================================================================

// d_arg_internal_split_word
//   struct: what a split builds: the list the words go to, the bytes of the
// one under way, and whether it has begun -- quotes with nothing between
// them begin a word that holds no bytes.
struct d_arg_internal_split_word
{
    struct d_arg_string_list* out;
    struct d_arg_buffer       bytes;
    bool                      begun;
};

/*
d_arg_internal_split_take
  One byte of the word under way, which begins it. A refused byte leaves the
bytes failed, which ending the word reports.
*/
D_STATIC void
d_arg_internal_split_take(
    struct d_arg_internal_split_word* _word,
    char                              _c
)
{
    (void)d_arg_buffer_append_char(&_word->bytes,
                                   _c);
    _word->begun = true;

    return;
}

/*
d_arg_internal_split_take_text
  Bytes of the word under way, which begin it, as d_arg_internal_split_take.
*/
D_STATIC void
d_arg_internal_split_take_text(
    struct d_arg_internal_split_word* _word,
    struct d_pack_text                _text
)
{
    (void)d_arg_buffer_append_text(&_word->bytes,
                                   _text);
    _word->begun = true;

    return;
}

/*
d_arg_internal_split_repeat
  A byte taken `_count` times.
*/
D_STATIC void
d_arg_internal_split_repeat(
    struct d_arg_internal_split_word* _word,
    char                              _c,
    size_t                            _count
)
{
    // one at a time
    for (size_t index = 0u; index < _count; ++index)
    {
        d_arg_internal_split_take(_word,
                                  _c);
    }

    return;
}

/*
d_arg_internal_split_end
  The word under way, once it has begun, copied into the list; the bytes
then emptied for the next. A byte refused on the way is the failure.
*/
D_STATIC int
d_arg_internal_split_end(
    struct d_arg_internal_split_word* _word
)
{
    int status = (_word->bytes.failed)
                 ? D_ARG_ERROR_OUT_OF_MEMORY
                 : D_ARG_OK;

    // a word that has begun is kept
    if ( (status == D_ARG_OK) &&
         (_word->begun) )
    {
        status = d_arg_internal_string_copy(_word->out,
                                            d_arg_buffer_text(&_word->bytes),
                                            NULL);
    }

    d_arg_buffer_clear(&_word->bytes);
    _word->begun = false;

    return status;
}

/*
d_arg_internal_split_refuse
  Text that breaks the rules: described, where there is a buffer for it.
*/
D_STATIC int
d_arg_internal_split_refuse(
    struct d_arg_buffer* _error,
    const char*          _description
)
{
    // described where asked
    if (_error != NULL)
    {
        (void)d_arg_buffer_append_cstr(_error,
                                       _description);
    }

    return D_ARG_ERROR_INVALID;
}

/*
d_arg_internal_split_find
  The position of the first `_c` at or after `_from`, or the length.
*/
D_STATIC size_t
d_arg_internal_split_find(
    struct d_pack_text _line,
    size_t             _from,
    char               _c
)
{
    const char* const found = (_from < _line.length)
                              ? (const char*)memchr(_line.data + _from,
                                                    (unsigned char)_c,
                                                    _line.length - _from)
                              : NULL;

    return (found != NULL)
           ? (size_t)(found - _line.data)
           : _line.length;
}

/*
d_arg_internal_split_run
  How many `_c` in a row start at `_from`.
*/
D_STATIC size_t
d_arg_internal_split_run(
    struct d_pack_text _line,
    size_t             _from,
    char               _c
)
{
    size_t at = _from;

    // to the end of the run
    while ( (at < _line.length) &&
            (_line.data[at] == _c) )
    {
        at++;
    }

    return at - _from;
}

//==============================================================================
// 3.  A POSIX SHELL'S RULES
//==============================================================================

/*
d_arg_internal_split_posix_escape
  A backslash outside quotes: the byte after it taken as it is, or, a line
break, dropped with it; with nothing after it, the backslash itself, as dash
and bash take one.
*/
D_STATIC void
d_arg_internal_split_posix_escape(
    struct d_pack_text                _line,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word
)
{
    const size_t next = *_at + 1u;

    // nothing to escape
    if (next >= _line.length)
    {
        d_arg_internal_split_take(_word,
                                  '\\');
        *_at = next;

        return;
    }

    // a line break joins the lines
    if (_line.data[next] != '\n')
    {
        d_arg_internal_split_take(_word,
                                  _line.data[next]);
    }

    *_at = next + 1u;

    return;
}

/*
d_arg_internal_split_posix_single
  Single quotes: every byte up to the next one, as it is.
*/
D_STATIC int
d_arg_internal_split_posix_single(
    struct d_pack_text                _line,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word,
    struct d_arg_buffer*              _error
)
{
    const size_t start = *_at + 1u;
    const size_t close = d_arg_internal_split_find(_line,
                                                   start,
                                                   '\'');

    // the quote never closes
    if (close >= _line.length)
    {
        return d_arg_internal_split_refuse(_error,
                                           "a single quote is not closed");
    }

    d_arg_internal_split_take_text(_word,
                                   d_arg_internal_span(_line.data + start,
                                                       close - start));
    *_at = close + 1u;

    return D_ARG_OK;
}

/*
d_arg_internal_split_posix_special
  Whether a backslash within double quotes escapes the byte after it: `$`, a
backquote, `"`, a backslash or a line break.
*/
D_STATIC bool
d_arg_internal_split_posix_special(
    char _c
)
{
    return ( (_c == '$')  ||
             (_c == '`')  ||
             (_c == '"')  ||
             (_c == '\\') ||
             (_c == '\n') );
}

/*
d_arg_internal_split_posix_double
  Double quotes: every byte up to the next unescaped one, a backslash before
`$`, a backquote, `"` or a backslash dropped and the byte kept, one before a
line break dropped with it, and any other kept.
*/
D_STATIC int
d_arg_internal_split_posix_double(
    struct d_pack_text                _line,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word,
    struct d_arg_buffer*              _error
)
{
    size_t at = *_at + 1u;

    _word->begun = true;

    // up to the closing quote
    while ( (at < _line.length) &&
            (_line.data[at] != '"') )
    {
        const char next = ((at + 1u) < _line.length)
                          ? _line.data[at + 1u]
                          : '\0';

        // an escape the quotes honour, or any other byte
        if ( (_line.data[at] == '\\') &&
             (d_arg_internal_split_posix_special(next)) )
        {
            // a line break is dropped with its backslash
            if (next != '\n')
            {
                d_arg_internal_split_take(_word,
                                          next);
            }

            at += 2u;
        }
        else
        {
            d_arg_internal_split_take(_word,
                                      _line.data[at]);
            at++;
        }
    }

    // the quote never closes
    if (at >= _line.length)
    {
        return d_arg_internal_split_refuse(_error,
                                           "a double quote is not closed");
    }

    *_at = at + 1u;

    return D_ARG_OK;
}

/*
d_arg_internal_split_posix_blank
  Whether a byte separates a POSIX shell's words: a space, a tab or a line
feed; a carriage return, a vertical tab or a form feed is an ordinary byte,
as the shells read one.
*/
D_STATIC bool
d_arg_internal_split_posix_blank(
    char _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\n') );
}

/*
d_arg_internal_split_posix_step
  One step of a POSIX shell's split at `*_at`: a blank ends the word under
way; a backslash, a single quote or a double quote reads what it governs;
any other byte is taken.
*/
D_STATIC int
d_arg_internal_split_posix_step(
    struct d_pack_text                _line,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word,
    struct d_arg_buffer*              _error
)
{
    const char c      = _line.data[*_at];
    int        status = D_ARG_OK;

    switch (c)
    {
        case '\\':
            d_arg_internal_split_posix_escape(_line,
                                              _at,
                                              _word);
            break;

        case '\'':
            status = d_arg_internal_split_posix_single(_line,
                                                       _at,
                                                       _word,
                                                       _error);
            break;

        case '"':
            status = d_arg_internal_split_posix_double(_line,
                                                       _at,
                                                       _word,
                                                       _error);
            break;

        default:
            // a blank ends the word under way; any other byte is taken
            if (d_arg_internal_split_posix_blank(c))
            {
                status = d_arg_internal_split_end(_word);
            }
            else
            {
                d_arg_internal_split_take(_word,
                                          c);
            }

            (*_at)++;
            break;
    }

    return status;
}

/*
d_arg_internal_split_posix
  A POSIX shell's words, without expansions: a step at a time, then the last
word.
*/
D_STATIC int
d_arg_internal_split_posix(
    struct d_pack_text                _line,
    struct d_arg_internal_split_word* _word,
    struct d_arg_buffer*              _error
)
{
    size_t at     = 0u;
    int    status = D_ARG_OK;

    // a step at a time
    while ( (status == D_ARG_OK) &&
            (at < _line.length) )
    {
        status = d_arg_internal_split_posix_step(_line,
                                                 &at,
                                                 _word,
                                                 _error);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_split_end(_word)
           : status;
}

//==============================================================================
// 4.  THE C RUNTIME'S RULES
//==============================================================================

/*
d_arg_internal_split_windows_separates
  Whether a byte outside quotes separates arguments: a space or a tab, and,
with line breaks asked for, `\n` and `\r`.
*/
D_STATIC bool
d_arg_internal_split_windows_separates(
    char _c,
    bool _line_breaks
)
{
    return ( (_c == ' ')         ||
             (_c == '\t')        ||
             ( (_line_breaks) &&
               ( (_c == '\n') ||
                 (_c == '\r') ) ) );
}

/*
d_arg_internal_split_windows_name
  The runtime's rule for a program's name: quotes open and close and are
dropped, nothing is escaped, and a separator outside quotes ends the name
and is taken with it. The name is a word even when empty.
*/
D_STATIC int
d_arg_internal_split_windows_name(
    struct d_pack_text                _line,
    bool                              _line_breaks,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word
)
{
    size_t at        = 0u;
    bool   in_quotes = false;

    _word->begun = true;

    // up to a separator outside quotes
    while (at < _line.length)
    {
        const char c = _line.data[at];

        at++;

        // a quote opens or closes; a separator ends the name
        if (c == '"')
        {
            in_quotes = !in_quotes;
        }
        else if ( (!in_quotes) &&
                  (d_arg_internal_split_windows_separates(c,
                                                          _line_breaks)) )
        {
            break;
        }
        else
        {
            d_arg_internal_split_take(_word,
                                      c);
        }
    }

    *_at = at;

    return d_arg_internal_split_end(_word);
}

/*
d_arg_internal_split_windows_quote
  A double quote after an even run of backslashes, within an argument:
within quotes, a second quote right after it makes the two one literal
quote; otherwise it opens or closes a quote and is dropped.
*/
D_STATIC void
d_arg_internal_split_windows_quote(
    struct d_pack_text                _line,
    size_t*                           _at,
    bool*                             _in_quotes,
    struct d_arg_internal_split_word* _word
)
{
    const size_t next = *_at + 1u;

    // two quotes within quotes are one
    if ( (*_in_quotes)               &&
         (next < _line.length)       &&
         (_line.data[next] == '"') )
    {
        d_arg_internal_split_take(_word,
                                  '"');
        *_at = next + 1u;

        return;
    }

    *_in_quotes = !(*_in_quotes);
    *_at        = next;

    return;
}

/*
d_arg_internal_split_windows_argument
  One argument by the runtime's rules: a run of backslashes gives itself,
but half of it before a double quote, where an even run leaves the quote to
open or close and an odd one makes it literal; a separator outside quotes
ends the argument. An argument is a word even when empty.
*/
D_STATIC int
d_arg_internal_split_windows_argument(
    struct d_pack_text                _line,
    bool                              _line_breaks,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word
)
{
    size_t at        = *_at;
    bool   in_quotes = false;

    _word->begun = true;

    // a run of backslashes, then the byte after it
    while (at < _line.length)
    {
        const size_t run   = d_arg_internal_split_run(_line,
                                                      at,
                                                      '\\');
        const bool   quote = ( ((at + run) < _line.length) &&
                               (_line.data[at + run] == '"') );

        at += run;
        d_arg_internal_split_repeat(_word,
                                    '\\',
                                    (quote) ? (run / 2u) : run);

        // an even run's quote, the end, a separator, or a byte to take
        if ( (quote) &&
             ((run % 2u) == 0u) )
        {
            d_arg_internal_split_windows_quote(_line,
                                               &at,
                                               &in_quotes,
                                               _word);
        }
        else if ( (at >= _line.length) ||
                  ( (!in_quotes) &&
                    (d_arg_internal_split_windows_separates(
                         _line.data[at],
                         _line_breaks)) ) )
        {
            break;
        }
        else
        {
            d_arg_internal_split_take(_word,
                                      _line.data[at]);
            at++;
        }
    }

    *_at = at;

    return d_arg_internal_split_end(_word);
}

/*
d_arg_internal_split_windows
  The C runtime's arguments: the program's name first when the line holds
one, then each argument after the separators before it.
*/
D_STATIC int
d_arg_internal_split_windows(
    struct d_pack_text                _line,
    uint32_t                          _flags,
    struct d_arg_internal_split_word* _word
)
{
    const bool line_breaks = ((_flags & D_ARG_SPLIT_LINE_BREAKS) != 0u);
    size_t     at          = 0u;
    int        status      = ((_flags & D_ARG_SPLIT_PROGRAM_NAME) != 0u)
                             ? d_arg_internal_split_windows_name(_line,
                                                                 line_breaks,
                                                                 &at,
                                                                 _word)
                             : D_ARG_OK;

    // each argument, after the separators before it
    while ( (status == D_ARG_OK) &&
            (at < _line.length) )
    {
        if (d_arg_internal_split_windows_separates(_line.data[at],
                                                   line_breaks))
        {
            at++;
        }
        else
        {
            status = d_arg_internal_split_windows_argument(_line,
                                                           line_breaks,
                                                           &at,
                                                           _word);
        }
    }

    return status;
}

//==============================================================================
// 5.  GNU'S RULES
//==============================================================================

/*
d_arg_internal_split_gnu_byte
  One byte of an argument by buildargv's rules, but a backslash: a quote
opens when none is open, and closes the one it matches; any other byte is
taken. Returns the quote open after it, or NUL.
*/
D_STATIC char
d_arg_internal_split_gnu_byte(
    struct d_arg_internal_split_word* _word,
    char                              _c,
    char                              _quote
)
{
    // a quote opens
    if ( (_quote == '\0') &&
         ( (_c == '\'') ||
           (_c == '"') ) )
    {
        return _c;
    }

    // the quote closes
    if (_c == _quote)
    {
        return '\0';
    }

    d_arg_internal_split_take(_word,
                              _c);

    return _quote;
}

/*
d_arg_internal_split_gnu_argument
  One argument by libiberty's buildargv, which GCC reads response files
with: a backslash takes the byte after it as it is, within quotes as
outside, and is dropped when nothing follows; single and double quotes keep
everything to their match, an unclosed one running to the end; whitespace
outside quotes ends the argument. An argument is a word even when empty.
*/
D_STATIC int
d_arg_internal_split_gnu_argument(
    struct d_pack_text                _line,
    size_t*                           _at,
    struct d_arg_internal_split_word* _word
)
{
    size_t at    = *_at;
    char   quote = '\0';

    _word->begun = true;

    // up to whitespace outside quotes
    while ( (at < _line.length)                            &&
            ( (quote != '\0') ||
              (!d_arg_internal_is_space(_line.data[at])) ) )
    {
        // a backslash takes the next byte, if there is one
        if (_line.data[at] == '\\')
        {
            if ((at + 1u) < _line.length)
            {
                d_arg_internal_split_take(_word,
                                          _line.data[at + 1u]);
            }

            at += 2u;
        }
        else
        {
            quote = d_arg_internal_split_gnu_byte(_word,
                                                  _line.data[at],
                                                  quote);
            at++;
        }
    }

    *_at = (at < _line.length) ? at : _line.length;

    return d_arg_internal_split_end(_word);
}

/*
d_arg_internal_split_gnu
  GNU's arguments, each after the whitespace before it; text of whitespace
alone holds none, as expandargv reads a response file.
*/
D_STATIC int
d_arg_internal_split_gnu(
    struct d_pack_text                _line,
    struct d_arg_internal_split_word* _word
)
{
    size_t at     = 0u;
    int    status = D_ARG_OK;

    // each argument, after the whitespace before it
    while ( (status == D_ARG_OK) &&
            (at < _line.length) )
    {
        if (d_arg_internal_is_space(_line.data[at]))
        {
            at++;
        }
        else
        {
            status = d_arg_internal_split_gnu_argument(_line,
                                                       &at,
                                                       _word);
        }
    }

    return status;
}

//==============================================================================
// 6.  CLI11'S RULES
//==============================================================================

/*
d_arg_internal_split_cli11_quote
  Whether a byte is one of the quotes CLI11's escape_detect looks for.
*/
D_STATIC bool
d_arg_internal_split_cli11_quote(
    char _c
)
{
    return ( (_c == '"')  ||
             (_c == '\'') ||
             (_c == '`') );
}

/*
d_arg_internal_split_cli11_lead
  CLI11's find_last_of("-/ \"'`", offset - 1) in escape_detect: the last of
those bytes before `_at`, or NUL when there is none.
*/
D_STATIC char
d_arg_internal_split_cli11_lead(
    const struct d_arg_buffer* _text,
    size_t                     _at
)
{
    // back from the byte before
    for (size_t at = _at; at > 0u; --at)
    {
        const char c = _text->data[at - 1u];

        if ( (c == '-') ||
             (c == '/') ||
             (c == ' ') ||
             (d_arg_internal_split_cli11_quote(c)) )
        {
            return c;
        }
    }

    return '\0';
}

/*
d_arg_internal_split_cli11_detect
  CLI11's find_and_modify with escape_detect, for one trigger, `=` or `:`:
in order, each trigger after the first byte that a quote follows becomes a
space when the last dash, slash, space or quote before it is the option's
lead -- `-` for `=`, `/` for `:` -- so that the quoted value is a word of
its own.
*/
D_STATIC void
d_arg_internal_split_cli11_detect(
    struct d_arg_buffer* _text,
    char                 _trigger
)
{
    const char lead = (_trigger == '=') ? '-' : '/';

    // each trigger after the first byte
    for (size_t at = 1u; at < _text->length; ++at)
    {
        const char next = ((at + 1u) < _text->length)
                          ? _text->data[at + 1u]
                          : '\0';

        if ( (_text->data[at] == _trigger)                   &&
             (d_arg_internal_split_cli11_quote(next))        &&
             (d_arg_internal_split_cli11_lead(_text,
                                              at) == lead) )
        {
            _text->data[at] = ' ';
        }
    }

    return;
}

/*
d_arg_internal_quotes_inner
  What CLI11's remove_quotes keeps of a word: the inside of a pair of the
same quote around it, or the word itself.
*/
D_STATIC struct d_pack_text
d_arg_internal_quotes_inner(
    struct d_pack_text _word
)
{
    const char front = _word.data[0];
    const bool outer = ( (_word.length > 1u)                       &&
                         (front == _word.data[_word.length - 1u])  &&
                         (d_arg_internal_split_cli11_quote(front)) );

    return (outer)
           ? d_arg_internal_span(_word.data + 1u,
                                 _word.length - 2u)
           : _word;
}

/*
d_arg_internal_quotes_double
  Whether a word starts and ends with a double quote, so that CLI11's
remove_quotes resolves its escapes once the quotes are gone.
*/
D_STATIC bool
d_arg_internal_quotes_double(
    struct d_pack_text _word
)
{
    return ( (_word.data[0] == '"') &&
             (_word.data[_word.length - 1u] == '"') );
}

/*
d_arg_internal_remove_quotes_text
  CLI11's remove_quotes for one word of a vector, into a buffer, whatever
bytes the word resolves to: a word in double quotes loses them and then its
escapes, a malformed one breaking the rules; one in single quotes or
backquotes loses them alone; any other is kept as it is.
*/
int
d_arg_internal_remove_quotes_text(
    struct d_pack_text   _word,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error
)
{
    const struct d_pack_text inner = d_arg_internal_quotes_inner(_word);

    // a single-quoted or plain word
    if (!d_arg_internal_quotes_double(_word))
    {
        return d_arg_buffer_append_text(_out,
                                        inner);
    }

    struct d_arg_buffer message = D_ARG_BUFFER_INIT;
    int                 status  = d_arg_internal_unescape(inner,
                                                          _out,
                                                          &message);

    status = (status == D_ARG_ERROR_VALIDATION)
             ? d_arg_internal_split_refuse(_error,
                                           (message.data != NULL)
                                           ? message.data
                                           : "")
             : status;
    d_arg_buffer_release(&message);

    return status;
}

/*
d_arg_internal_remove_quotes
  d_arg_internal_remove_quotes_text into a string list: a word that keeps
its bytes copied straight in, a double-quoted one resolved first.
*/
int
d_arg_internal_remove_quotes(
    struct d_pack_text        _word,
    struct d_arg_string_list* _out,
    struct d_arg_buffer*      _error
)
{
    // a single-quoted or plain word
    if (!d_arg_internal_quotes_double(_word))
    {
        return d_arg_internal_string_copy(_out,
                                          d_arg_internal_quotes_inner(_word),
                                          NULL);
    }

    struct d_arg_buffer resolved = D_ARG_BUFFER_INIT;
    int                 status   = d_arg_internal_remove_quotes_text(
                                       _word,
                                       &resolved,
                                       _error);

    status = (status == D_ARG_OK)
             ? d_arg_internal_string_copy(_out,
                                          d_arg_buffer_text(&resolved),
                                          NULL)
             : status;
    d_arg_buffer_release(&resolved);

    return status;
}

/*
d_arg_internal_split_cli11
  CLI11's split of a command line in parse(string): the trimmed line's
triggers rewritten -- `=`, and `:` for Windows-style options -- the line
split up, empty pieces dropped, and each piece's quotes removed.
*/
D_STATIC int
d_arg_internal_split_cli11(
    struct d_pack_text        _line,
    uint32_t                  _flags,
    struct d_arg_string_list* _out,
    struct d_arg_buffer*      _error
)
{
    struct d_arg_buffer      text   = D_ARG_BUFFER_INIT;
    struct d_arg_string_list pieces = D_ARG_LIST_INIT;
    int                      status = d_arg_buffer_append_text(
                                          &text,
                                          d_arg_internal_trim(_line));

    // the triggers, then the pieces
    if (status == D_ARG_OK)
    {
        d_arg_internal_split_cli11_detect(&text,
                                          '=');

        if ((_flags & D_ARG_SPLIT_WINDOWS_STYLE) != 0u)
        {
            d_arg_internal_split_cli11_detect(&text,
                                              ':');
        }

        status = d_arg_internal_split_up(d_arg_buffer_text(&text),
                                         '\0',
                                         &pieces);
    }

    // each piece but the empty, its quotes removed
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < pieces.count);
         ++index)
    {
        status = (pieces.items[index][0] != '\0')
                 ? d_arg_internal_remove_quotes(
                       d_arg_internal_text_of(pieces.items[index]),
                       _out,
                       _error)
                 : D_ARG_OK;
    }

    d_arg_string_list_release(&pieces);
    d_arg_buffer_release(&text);

    return status;
}

//==============================================================================
// 7.  SPLITTING
//==============================================================================

/*
d_arg_internal_quoting_known
  Whether a value is one of the four sets of rules.
*/
bool
d_arg_internal_quoting_known(
    int _quoting
)
{
    return ( (_quoting == (int)D_ARG_QUOTING_POSIX)   ||
             (_quoting == (int)D_ARG_QUOTING_WINDOWS) ||
             (_quoting == (int)D_ARG_QUOTING_CLI11)   ||
             (_quoting == (int)D_ARG_QUOTING_GNU) );
}

/*
d_arg_internal_split_by
  The split by one set of rules.
*/
D_STATIC int
d_arg_internal_split_by(
    struct d_pack_text                _line,
    int                               _quoting,
    uint32_t                          _flags,
    struct d_arg_internal_split_word* _word,
    struct d_arg_buffer*              _error
)
{
    int status = D_ARG_OK;

    switch (_quoting)
    {
        case D_ARG_QUOTING_POSIX:
            status = d_arg_internal_split_posix(_line,
                                                _word,
                                                _error);
            break;

        case D_ARG_QUOTING_WINDOWS:
            status = d_arg_internal_split_windows(_line,
                                                  _flags,
                                                  _word);
            break;

        case D_ARG_QUOTING_GNU:
            status = d_arg_internal_split_gnu(_line,
                                              _word);
            break;

        default:
            status = d_arg_internal_split_cli11(_line,
                                                _flags,
                                                _word->out,
                                                _error);
            break;
    }

    return status;
}

/*
d_arg_split_line
  The words of a line by a set of rules, appended; on failure the list is cut
back to what it held.
*/
int
d_arg_split_line(
    struct d_pack_text        _line,
    int                       _quoting,
    uint32_t                  _flags,
    struct d_arg_string_list* _out,
    struct d_arg_buffer*      _error
)
{
    // validate the parameters
    if ( (!_out)                              ||
         ( (!_line.data) &&
           (_line.length > 0u) )              ||
         (!d_arg_internal_quoting_known(_quoting)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t                     before = _out->count;
    const struct d_pack_text         line   = (_line.data != NULL)
                                              ? _line
                                              : d_arg_internal_span("",
                                                                    0u);
    struct d_arg_internal_split_word word   = { _out,
                                                D_ARG_BUFFER_INIT,
                                                false };
    const int                        status = d_arg_internal_split_by(
                                                  line,
                                                  _quoting,
                                                  _flags,
                                                  &word,
                                                  _error);

    // a failure leaves the list as it was
    if (status != D_ARG_OK)
    {
        d_arg_internal_string_truncate(_out,
                                       before);
    }

    d_arg_buffer_release(&word.bytes);

    return status;
}

//==============================================================================
// 8.  QUOTING
//==============================================================================

// CLI11_ESCAPED / CLI11_ESCAPE_CODES
//   constant: CLI11's escapedChars and escapedCharsCode, position for
// position: the bytes add_escaped_characters escapes, and the letter each is
// written with after a backslash.
static const char CLI11_ESCAPED[]      = "\b\t\n\f\r\"\\";
static const char CLI11_ESCAPE_CODES[] = "btnfr\"\\";

// POSIX_SAFE
//   constant: the bytes besides ASCII letters and digits that no POSIX shell
// reads specially anywhere in a word -- Python's shlex.quote leaves a word of
// these, letters and digits as it is.
static const char POSIX_SAFE[] = "@%+=:,./-_";

/*
d_arg_internal_quote_special
  Whether a byte is one a set of rules reads specially anywhere in a word:
for POSIX's, any byte but a letter, a digit or a safe one, so that a real
shell, with its expansions and operators, also gives the word back; for
GNU's, whitespace, a quote or a backslash; for the C runtime's, a space, a
tab, a line break or a double quote; for CLI11's, whitespace.
*/
D_STATIC bool
d_arg_internal_quote_special(
    char _c,
    int  _quoting
)
{
    // anything a shell might read specially
    if (_quoting == (int)D_ARG_QUOTING_POSIX)
    {
        return ( (!d_arg_internal_is_alpha(_c)) &&
                 (!d_arg_internal_is_digit(_c)) &&
                 ( (_c == '\0') ||
                   (strchr(POSIX_SAFE,
                           _c) == NULL) ) );
    }

    // the C runtime's separators and its quote
    if (_quoting == (int)D_ARG_QUOTING_WINDOWS)
    {
        return ( (d_arg_internal_split_windows_separates(_c,
                                                         true)) ||
                 (_c == '"') );
    }

    // whitespace alone matters within a word for CLI11's
    if (_quoting == (int)D_ARG_QUOTING_CLI11)
    {
        return d_arg_internal_is_space(_c);
    }

    return ( (d_arg_internal_is_space(_c)) ||
             (_c == '\'')                  ||
             (_c == '"')                   ||
             (_c == '\\') );
}

/*
d_arg_internal_quote_cli11_trigger
  Whether the byte at `_at` is a `=` or `:` that a quote follows, or that
ends the word -- where a closing quote will follow it -- which CLI11's
escape_detect would turn into a space.
*/
D_STATIC bool
d_arg_internal_quote_cli11_trigger(
    struct d_pack_text _word,
    size_t             _at
)
{
    const size_t next = _at + 1u;

    return ( ( (_word.data[_at] == '=') ||
               (_word.data[_at] == ':') )                          &&
             ( (next >= _word.length) ||
               (d_arg_internal_split_cli11_quote(_word.data[next])) ) );
}

/*
d_arg_internal_quote_needed
  Whether a word must be quoted to come back whole: an empty word always; a
word holding a byte its rules read specially; and, for CLI11's, a word
opening with a quote or a bracket, or holding a `=` or `:` before a quote.
*/
D_STATIC bool
d_arg_internal_quote_needed(
    struct d_pack_text _word,
    int                _quoting
)
{
    const bool cli11 = (_quoting == (int)D_ARG_QUOTING_CLI11);

    // empty, or opening as a CLI11 sequence
    if ( (_word.length == 0u) ||
         ( (cli11) &&
           (d_arg_internal_bracket_index(OPENERS,
                                         _word.data[0]) >= 0) ) )
    {
        return true;
    }

    // a special byte, or a CLI11 trigger before a quote
    for (size_t at = 0u; at < _word.length; ++at)
    {
        if ( (d_arg_internal_quote_special(_word.data[at],
                                           _quoting)) ||
             ( (cli11)                                    &&
               ((at + 1u) < _word.length)                 &&
               (d_arg_internal_quote_cli11_trigger(_word,
                                                   at)) ) )
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_quote_repeat
  A byte written `_count` times.
*/
D_STATIC void
d_arg_internal_quote_repeat(
    struct d_arg_buffer* _out,
    char                 _c,
    size_t               _count
)
{
    // one at a time
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_char(_out,
                                       _c);
    }

    return;
}

/*
d_arg_internal_quote_posix
  POSIX's quoting: the word in single quotes, each single quote in it
written `'\''`, closing the quotes around an escaped one.
*/
D_STATIC void
d_arg_internal_quote_posix(
    struct d_pack_text   _word,
    struct d_arg_buffer* _out
)
{
    (void)d_arg_buffer_append_char(_out,
                                   '\'');

    // each byte, a single quote closed around
    for (size_t at = 0u; at < _word.length; ++at)
    {
        if (_word.data[at] == '\'')
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "'\\''");
        }
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           _word.data[at]);
        }
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\'');

    return;
}

/*
d_arg_internal_quote_windows
  The C runtime's quoting: the word in double quotes, a run of backslashes
doubled before a double quote, which is then escaped, and before the
closing quote; any other run left as it is.
*/
D_STATIC void
d_arg_internal_quote_windows(
    struct d_pack_text   _word,
    struct d_arg_buffer* _out
)
{
    size_t at = 0u;

    (void)d_arg_buffer_append_char(_out,
                                   '"');

    // a run of backslashes, then the byte after it
    while (at < _word.length)
    {
        const size_t run   = d_arg_internal_split_run(_word,
                                                      at,
                                                      '\\');
        const bool   end   = ((at + run) >= _word.length);
        const bool   quote = ( (!end) &&
                               (_word.data[at + run] == '"') );

        // the run, doubled where a quote follows it
        d_arg_internal_quote_repeat(_out,
                                    '\\',
                                    ( (end) ||
                                      (quote) )
                                    ? (2u * run)
                                    : run);
        at += run;

        // a quote escaped, or any other byte as it is
        if (quote)
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "\\\"");
        }
        else if (!end)
        {
            (void)d_arg_buffer_append_char(_out,
                                           _word.data[at]);
        }

        at += (end) ? 0u : 1u;
    }

    (void)d_arg_buffer_append_char(_out,
                                   '"');

    return;
}

/*
d_arg_internal_quote_cli11_byte
  One byte of a word in CLI11's double quotes: one of add_escaped_characters'
-- backspace, tab, line feed, form feed, carriage return, `"` and the
backslash -- after a backslash, by its letter; a `=` or `:` before a quote,
or last, as a `\u` escape; any other as it is.
*/
D_STATIC void
d_arg_internal_quote_cli11_byte(
    struct d_pack_text   _word,
    size_t               _at,
    struct d_arg_buffer* _out
)
{
    const char        c     = _word.data[_at];
    const char* const found = (c != '\0')
                              ? strchr(CLI11_ESCAPED,
                                       c)
                              : NULL;

    // an escaped character, a trigger, or the byte
    if (found != NULL)
    {
        (void)d_arg_buffer_append_char(_out,
                                       '\\');
        (void)d_arg_buffer_append_char(_out,
                                       CLI11_ESCAPE_CODES[found -
                                                          CLI11_ESCAPED]);
    }
    else if (d_arg_internal_quote_cli11_trigger(_word,
                                                _at))
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (c == '=') ? "\\u003d" : "\\u003a");
    }
    else
    {
        (void)d_arg_buffer_append_char(_out,
                                       c);
    }

    return;
}

/*
d_arg_internal_quote_cli11
  CLI11's quoting: the word in double quotes, each byte as
d_arg_internal_quote_cli11_byte writes it.
*/
D_STATIC void
d_arg_internal_quote_cli11(
    struct d_pack_text   _word,
    struct d_arg_buffer* _out
)
{
    (void)d_arg_buffer_append_char(_out,
                                   '"');

    // each byte, escaped as it needs
    for (size_t at = 0u; at < _word.length; ++at)
    {
        d_arg_internal_quote_cli11_byte(_word,
                                        at,
                                        _out);
    }

    (void)d_arg_buffer_append_char(_out,
                                   '"');

    return;
}

/*
d_arg_internal_quote_gnu
  GNU's quoting: each special byte after a backslash, and an empty word as
two single quotes.
*/
D_STATIC void
d_arg_internal_quote_gnu(
    struct d_pack_text   _word,
    struct d_arg_buffer* _out
)
{
    // an empty word
    if (_word.length == 0u)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "''");

        return;
    }

    // each byte, a special one escaped
    for (size_t at = 0u; at < _word.length; ++at)
    {
        if (d_arg_internal_quote_special(_word.data[at],
                                         (int)D_ARG_QUOTING_GNU))
        {
            (void)d_arg_buffer_append_char(_out,
                                           '\\');
        }

        (void)d_arg_buffer_append_char(_out,
                                       _word.data[at]);
    }

    return;
}

/*
d_arg_quote_word
  A word written so that its rules give it back: as it is where they would
not change it, quoted by the rules otherwise.
*/
int
d_arg_quote_word(
    struct d_pack_text   _word,
    int                  _quoting,
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if ( (!_out)                              ||
         ( (!_word.data) &&
           (_word.length > 0u) )              ||
         (!d_arg_internal_quoting_known(_quoting)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const struct d_pack_text word = (_word.data != NULL)
                                    ? _word
                                    : d_arg_internal_span("",
                                                          0u);

    // a word the rules leave alone
    if (!d_arg_internal_quote_needed(word,
                                     _quoting))
    {
        return d_arg_buffer_append_text(_out,
                                        word);
    }

    switch (_quoting)
    {
        case D_ARG_QUOTING_POSIX:
            d_arg_internal_quote_posix(word,
                                       _out);
            break;

        case D_ARG_QUOTING_WINDOWS:
            d_arg_internal_quote_windows(word,
                                         _out);
            break;

        case D_ARG_QUOTING_GNU:
            d_arg_internal_quote_gnu(word,
                                     _out);
            break;

        default:
            d_arg_internal_quote_cli11(word,
                                       _out);
            break;
    }

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}
