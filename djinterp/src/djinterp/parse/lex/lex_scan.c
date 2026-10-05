/*******************************************************************************
* djinterp [parse]                                                    lex_scan.c
*
* Definitions for the non-inline declarations in lex_scan.h.
*   One scanner, driven by a descriptor.  Sections 2 through 5 consult the
* dialect for every question that has a language-dependent answer and answer
* the rest from the standard's common lexical grammar, which is why a second
* language costs a table and not a file.
*   Section 6 is where a token's kind is decided.  The order of the tests
* there is the grammar's: a literal prefix is recognized before the
* identifier it looks like, and a pp-number before the `.` that may open it,
* because the alternative in each case is a correct token of the wrong kind.
*   Lookahead is a copy of the structure and a rescan.  The scanner holds
* no buffer and owns nothing but itself, so a copy is a position and costs no
* allocation; the price is that a deep peek repeated often is quadratic, and
* a caller that needs one should buffer.
*
*
* path:      /src/djinterp/parse/lex/lex_scan.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/djinterp/parse/lex/lex_scan.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // calloc, free
#include <string.h>   // memcmp, strlen
// djinterp
#include "../../../../inc/djinterp/parse/lex/lex_unicode.h"  // identifier sets
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uint16_t,
                                                     // UINT32_MAX


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Scanner state
//------------------------------------------------------------------------------
// 1.1.1
// D_INTERNAL_OPERATOR_WINDOW
//   constant: the widest operator any dialect may spell, and so the number of
// logical characters the punctuator matcher looks at.  `%:%:` is four; the
// validator rejects a table that needs more.
#define D_INTERNAL_OPERATOR_WINDOW 4

// 1.1.2
// D_INTERNAL_PREFIX_MAX
//   constant: the widest literal prefix, `u8R`.  A longer identifier abutting
// a quote is an identifier followed by a literal, not a prefixed one.
#define D_INTERNAL_PREFIX_MAX 3

// 1.1.3
// D_INTERNAL_SPELLING_MAX
//   constant: the widest spelling reconstructed on the stack when a token's
// byte range is not its spelling.  The longest keyword in either language is
// `reinterpret_cast` at sixteen, so anything past this bound is an identifier
// and the reconstruction is abandoned rather than grown.
#define D_INTERNAL_SPELLING_MAX 64

// 1.1.4
// D_INTERNAL_NAME_MAX
//   constant: how many code points of an identifier are kept for the NFC
// check.  A name longer than this that also needs the full check is judged
// by the quick check alone, which can only miss a violation, never invent one.
#define D_INTERNAL_NAME_MAX 128

// 1.1.5
// d_lexer
//   struct: the reader, the borrowed descriptor, and the directive state.
//   Lookahead copies the whole structure, so nothing here may own memory that
//   a copy would free; the only allocation is the structure itself.  The
//   sink is borrowed, and a copy made for lookahead has it cleared, so a
//   peeked token is never reported twice.
struct d_lexer
{
    struct d_reader             reader;
    const struct d_lex_dialect* dialect;
    unsigned                    options;
    struct d_parse_diag_sink*   sink;

    bool                        line_start;      // no token yet on this line
    bool                        leading_space;   // whitespace since the last
    bool                        in_directive;    // inside a `#` line
    bool                        after_hash;      // the `#` was the last token
    bool                        expect_macro;    // next identifier is a macro
    bool                        header_context;  // `<` opens a header name

    uint32_t                    comment_line;    // where a reported comment
    uint32_t                    comment_column;  // began, since the reader
                                                 // has moved past it
};

// 1.1.6
// d_internal_mark
//   struct: a position the scanner has passed and must report later: where a
//   token, a comment or a UCN began.
struct d_internal_mark
{
    size_t   offset;
    uint32_t line;
    uint32_t column;
};

// 1.1.7
// d_internal_prefix
//   struct: a literal prefix standing at the position, as
//   d_internal_prefix_length measured it.
struct d_internal_prefix
{
    size_t length;
    bool   raw;
    int    quote;
};

// 1.1.8
// d_internal_name
//   struct: the code points of the identifier being scanned, kept only while
//   it is scanned and only for the NFC check.  `unusual` is set once any code
//   point's quick check is not YES, so an ordinary name costs no check.
struct d_internal_name
{
    uint32_t  code_points[D_INTERNAL_NAME_MAX];
    size_t    count;
    bool      unusual;
    bool      overflowed;
};


//==============================================================================
// 2.  CHARACTER CLASSIFICATION
//==============================================================================
// The tests the scanner makes on a single character.  Each takes an int so
// that D_SOURCE_END may be passed without a cast and answers false.  Only
// the basic characters are classified here; an extended character is
// decoded and looked up by d_internal_extended_width.


/**
 * @brief Reports whether a character is a decimal digit.
 *
 * @param[in] _character  the character to test.
 * @return `true` if the character is a digit, `false` otherwise.
 */
static bool
d_internal_is_digit(
    int _character
)
{
    return ( (_character >= '0') &&
             (_character <= '9') );
}


/**
 * @brief Reports whether a character is a hexadecimal digit.
 *
 * @param[in] _character  the character to test.
 * @return `true` if the character is a hex digit, `false` otherwise.
 */
static bool
d_internal_is_hex(
    int _character
)
{
    return ( d_internal_is_digit(_character)                  ||
             ( (_character >= 'a') && (_character <= 'f') ) ||
             ( (_character >= 'A') && (_character <= 'F') ) );
}


/**
 * @brief Reports whether a basic character may begin an identifier.
 *
 * @param[in] _dialect    the descriptor to consult, for `$`.
 * @param[in] _character  the character to test.
 * @return `true` for a letter, `_`, or `$` where the dialect allows it.
 */
static bool
d_internal_is_basic_start(
    const struct d_lex_dialect* _dialect,
    int                         _character
)
{
    if ( ( (_character >= 'a') && (_character <= 'z') ) ||
         ( (_character >= 'A') && (_character <= 'Z') ) ||
         (_character == '_') )
    {
        return true;
    }

    return ( (_character == '$') &&
             d_lex_dialect_has(_dialect,
                               D_LEX_FEATURE_DOLLAR) );
}


/**
 * @brief Reports whether a basic character may continue an identifier.
 *
 * @param[in] _dialect    the descriptor to consult.
 * @param[in] _character  the character to test.
 * @return `true` for an identifier start or a digit, `false` otherwise.
 */
static bool
d_internal_is_basic_part(
    const struct d_lex_dialect* _dialect,
    int                         _character
)
{
    return ( d_internal_is_basic_start(_dialect,
                                       _character) ||
             d_internal_is_digit(_character) );
}


/**
 * @brief Reports whether a character is whitespace that does not end a line.
 *
 * @param[in] _character  the character to test.
 * @return `true` if the character is horizontal whitespace, `false` otherwise.
 */
static bool
d_internal_is_horizontal_space(
    int _character
)
{
    return ( (_character == ' ')  ||
             (_character == '\t') ||
             (_character == '\v') ||
             (_character == '\f') );
}


/**
 * @brief Finds the physical offset of the next logical character.
 *
 * @note The reader's offset may stand before a splice, and the logical
 *       character is then past it; UTF-8 is decoded from the physical bytes,
 *       so the bytes must be found where the character actually is.
 *
 * @param[in] _lexer  the scanner to inspect; not advanced.
 * @return the physical offset of the character d_reader_peek(0) reports.
 */
static size_t
d_internal_physical_here(
    const struct d_lexer* _lexer
)
{
    struct d_reader probe = _lexer->reader;

    probe.options &= ~(unsigned)D_READER_OPT_UTF8;

    // reading one character crosses any splices before it; a byte of 0x80 or
    // above is never a trigraph or a line ending, so it is one byte wide
    (void)d_reader_next(&probe);

    return (probe.offset > 0u) ? (probe.offset - 1u) : 0u;
}


/**
 * @brief Measures an extended character standing at the position, if the
 *        dialect admits it in an identifier there.
 *
 * @param[in]  _lexer       the scanner to inspect; not advanced.
 * @param[in]  _initial     whether the character would begin the name.
 * @param[out] _out_code    receives the code point when one is admitted.
 * @return the character's width in bytes, or zero if it is not well-formed
 *         UTF-8 or the dialect's identifier set excludes it.
 */
static size_t
d_internal_extended_width(
    const struct d_lexer* _lexer,
    bool                  _initial,
    uint32_t*             _out_code
)
{
    // the character must be extended, and extended names allowed at all
    if ( (d_reader_peek(&_lexer->reader, 0u) < 0x80) ||
         (!d_lex_dialect_has(_lexer->dialect,
                             D_LEX_FEATURE_UTF8_IDENTIFIER)) )
    {
        return 0u;
    }

    const struct d_source* const source = _lexer->reader.source;
    const size_t                 at     = d_internal_physical_here(_lexer);
    uint32_t                     code   = 0u;
    const size_t                 width  =
        d_source_utf8_decode(source->text + at,
                             source->length - at,
                             &code);

    // ill-formed bytes end the name; the reader reports them
    if (width == 0u)
    {
        return 0u;
    }

    const bool admitted = _initial
        ? d_lex_unicode_is_id_start(_lexer->dialect->identifiers,
                                    code)
        : d_lex_unicode_is_id_continue(_lexer->dialect->identifiers,
                                       code);

    // a character the set excludes ends the name and is then stray
    if (!admitted)
    {
        return 0u;
    }

    *_out_code = code;

    return width;
}


/**
 * @brief Measures the body of `\N{NAME}` whose brace stands at a position.
 *
 * @param[in] _reader  the reader to inspect; not advanced.
 * @param[in] _brace   where the opening brace stands, in logical characters.
 * @return the characters from the brace through the closing one, or zero
 *         when the name is empty or holds anything but letters, digits,
 *         spaces and hyphens.
 */
static size_t
d_internal_named_length(
    const struct d_reader* _reader,
    size_t                 _brace
)
{
    size_t at = _brace + 1u;

    for (;;)
    {
        const int c = d_reader_peek(_reader, at);

        // the characters a Unicode name, or a name alias, is spelled with
        if ( ( (c >= 'A') && (c <= 'Z') ) ||
             d_internal_is_digit(c)       ||
             (c == ' ')                   ||
             (c == '-') )
        {
            ++at;
            continue;
        }

        return ( (c == '}') &&
                 (at > (_brace + 1u)) )
               ? ((at + 1u) - _brace)
               : 0u;
    }
}


/**
 * @brief Measures the hexadecimal digits of `\u`, `\U` or `\u{}`.
 *
 * @param[in]  _reader     the reader to inspect; not advanced.
 * @param[in]  _first      where the first digit would stand.
 * @param[in]  _fixed      how many digits a fixed form needs, or zero for
 *                         the delimited form, which needs one and a brace.
 * @param[out] _out_code   receives the value, capped at 0x110000.
 * @return the position just past the form, or zero if it is not well-formed.
 */
static size_t
d_internal_hex_ucn_end(
    const struct d_reader* _reader,
    size_t                 _first,
    size_t                 _fixed,
    uint32_t*              _out_code
)
{
    uint32_t value = 0u;
    size_t   at    = _first;

    // accumulate hex digits, saturating past the Unicode range
    while ( d_internal_is_hex(d_reader_peek(_reader, at)) &&
            ( (_fixed == 0u) ||
              ((at - _first) < _fixed) ) )
    {
        const int c     = d_reader_peek(_reader, at);
        const int digit = d_internal_is_digit(c)
                        ? (c - '0')
                        : ((c | 0x20) - 'a' + 10);

        value = (value > 0x10FFFFu)
              ? 0x110000u
              : ((value << 4) | (uint32_t)digit);
        ++at;
    }

    *_out_code = value;

    // a fixed form needs exactly its digits; a delimited one, one or more
    // and its closing brace
    if (_fixed != 0u)
    {
        return ((at - _first) == _fixed) ? at : 0u;
    }

    return ( (at > _first) &&
             (d_reader_peek(_reader, at) == '}') )
           ? (at + 1u)
           : 0u;
}


/**
 * @brief Measures a well-formed universal character name at the position.
 *
 * @note Reads `\uXXXX`, `\UXXXXXXXX`, and where the dialect has them the
 *       C++23 forms `\u{X...}` and `\N{NAME}`.  A named character's value is
 *       not resolved, since that needs the Unicode name list; it is reported
 *       as 0xFFFFFFFF and treated as admissible.  Only the shape is checked
 *       here -- whether the value may stand where it does is the caller's
 *       question.
 *
 * @param[in]  _lexer     the scanner to inspect; not advanced.
 * @param[in]  _ahead     where the backslash stands, in logical characters.
 * @param[out] _out_code  receives the value, capped at 0x110000 when larger.
 * @return the length in logical characters, or zero if no well-formed UCN
 *         begins there.
 */
static size_t
d_internal_ucn_length(
    const struct d_lexer* _lexer,
    size_t                _ahead,
    uint32_t*             _out_code
)
{
    const struct d_reader* const reader = &_lexer->reader;
    const int                    marker = d_reader_peek(reader,
                                                        _ahead + 1u);
    const bool                   braced =
        (d_reader_peek(reader, _ahead + 2u) == '{');

    // only a backslash, in a dialect with UCNs, can open one
    if ( (d_reader_peek(reader, _ahead) != '\\') ||
         (!d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_UCN)) )
    {
        return 0u;
    }

    // `\N{NAME}`, whose value is not resolved
    if ( (marker == 'N') &&
         braced &&
         d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_NAMED_ESCAPE) )
    {
        const size_t body = d_internal_named_length(reader,
                                                    _ahead + 2u);

        *_out_code = 0xFFFFFFFFu;

        return (body > 0u) ? (body + 2u) : 0u;
    }

    // anything else must be `\u` or `\U`, the first possibly delimited
    if ( (marker != 'u') &&
         (marker != 'U') )
    {
        return 0u;
    }

    const bool   delimited = ( (marker == 'u') &&
                               braced          &&
                               d_lex_dialect_has(
                                   _lexer->dialect,
                                   D_LEX_FEATURE_DELIMITED_ESCAPE) );
    const size_t end       =
        d_internal_hex_ucn_end(reader,
                               _ahead + (delimited ? 3u : 2u),
                               delimited ? 0u
                                         : ((marker == 'u') ? 4u : 8u),
                               _out_code);

    return (end > 0u) ? (end - _ahead) : 0u;
}


//==============================================================================
// 3.  DIAGNOSTICS
//==============================================================================


/**
 * @brief Emits one diagnostic about a range of the source into the attached
 *        sink.
 *
 * @param[in,out] _lexer  the scanner; nothing happens without a sink.
 * @param[in]     _code   a d_lex_error.
 * @param[in]     _from   where the range begins.
 * @param[in]     _end    the byte just past it.
 */
static void
d_internal_report(
    struct d_lexer*               _lexer,
    int                           _code,
    const struct d_internal_mark* _from,
    size_t                        _end
)
{
    // a scanner with no sink, or a lookahead copy, reports nothing
    if (!_lexer->sink)
    {
        return;
    }

    const struct d_parse_span span =
        d_parse_span_located(_lexer->reader.source->id,
                             (uint32_t)_from->offset,
                             (uint32_t)(_end - _from->offset),
                             _from->line,
                             _from->column);

    (void)d_parse_diag_emit(_lexer->sink,
                            d_lex_error_severity(_code),
                            (uint16_t)D_PARSE_DIAG_DOMAIN_LEX,
                            (uint16_t)_code,
                            span,
                            d_lex_error_text(_code));

    return;
}


/**
 * @brief Records the scan position, for a report made after moving past it.
 *
 * @param[in] _lexer  the scanner.
 * @return the position.
 */
static struct d_internal_mark
d_internal_here(
    const struct d_lexer* _lexer
)
{
    const struct d_internal_mark mark =
    {
        d_reader_offset(&_lexer->reader),
        d_reader_line(&_lexer->reader),
        d_reader_column(&_lexer->reader)
    };

    return mark;
}


/**
 * @brief Reports and clears an ill-formed UTF-8 byte the reader met.
 *
 * @note Called after trivia and after each token, so a bad byte is
 *       reported where it was read, once, whether it lay in a comment, a
 *       literal or nowhere at all.
 *
 * @param[in,out] _lexer  the scanner to check.
 */
static void
d_internal_check_encoding(
    struct d_lexer* _lexer
)
{
    // nothing was met since the last check
    if (!_lexer->reader.malformed)
    {
        return;
    }

    const struct d_internal_mark at =
    {
        _lexer->reader.malformed_offset,
        _lexer->reader.malformed_line,
        _lexer->reader.malformed_column
    };

    d_internal_report(_lexer,
                      D_LEX_ERR_ENCODING,
                      &at,
                      at.offset + 1u);

    _lexer->reader.malformed = false;

    return;
}


//==============================================================================
// 4.  WHITESPACE AND COMMENTS
//==============================================================================
// Skipping is where the directive state is maintained, because a directive is
// ended by a newline and a newline is whitespace.


/**
 * @brief Consumes whitespace and comments up to the next token, recording
 *        whether any was seen and whether a line was crossed.
 *
 * A comment counts as whitespace, as phase 3 requires.
 *
 * @param[in,out] _lexer            the scanner to advance.
 * @param[out]    _out_comment      receives the offset of a comment to report,
 *                                  or (size_t)-1.
 * @param[out]    _out_comment_end  receives the offset just past that comment.
 * @param[out]    _out_flags        receives the comment's
 *                                  D_TOKEN_FLAG_COMMENT_* bits.
 * @return `true` if a comment was met and the caller asked for comments,
 *         `false` otherwise.
 */
static bool
d_internal_skip_trivia(
    struct d_lexer* _lexer,
    size_t*         _out_comment,
    size_t*         _out_comment_end,
    uint32_t*       _out_flags
)
{
    const bool emit = ((_lexer->options & D_LEX_OPT_EMIT_COMMENTS) != 0u);

    for (;;)
    {
        const int character = d_reader_peek(&_lexer->reader, 0u);

        // horizontal whitespace separates tokens and nothing more
        if (d_internal_is_horizontal_space(character))
        {
            (void)d_reader_next(&_lexer->reader);
            _lexer->leading_space = true;
            continue;
        }

        // a newline ends any directive and opens a new line
        if (character == '\n')
        {
            (void)d_reader_next(&_lexer->reader);

            _lexer->line_start     = true;
            _lexer->leading_space  = false;
            _lexer->in_directive   = false;
            _lexer->after_hash     = false;
            _lexer->expect_macro   = false;
            _lexer->header_context = false;
            continue;
        }

        // a block comment, which may span lines
        if ((character == '/') && (d_reader_peek(&_lexer->reader, 1u) == '*'))
        {
            const size_t start = d_reader_offset(&_lexer->reader);
            const int    third = d_reader_peek(&_lexer->reader, 2u);

            _lexer->comment_line   = d_reader_line(&_lexer->reader);
            _lexer->comment_column = d_reader_column(&_lexer->reader);
            const int    fourth = d_reader_peek(&_lexer->reader, 3u);

            // `/**` opens a doc comment unless it is `/**/` or a rule of
            // asterisks, which is a banner and not documentation
            *_out_flags = ( ((third == '*') &&
                             (fourth != '*') && (fourth != '/')) ||
                            (third == '!') )
                        ? D_TOKEN_FLAG_COMMENT_DOC
                        : D_TOKEN_FLAG_NONE;

            (void)d_reader_next(&_lexer->reader);
            (void)d_reader_next(&_lexer->reader);

            bool closed = false;

            while (!d_reader_at_end(&_lexer->reader))
            {
                const int inner = d_reader_next(&_lexer->reader);

                if ((inner == '*') &&
                    (d_reader_peek(&_lexer->reader, 0u) == '/'))
                {
                    (void)d_reader_next(&_lexer->reader);
                    closed = true;
                    break;
                }
            }

            // an unclosed comment swallows the rest of the file
            if (!closed)
            {
                const struct d_internal_mark from =
                {
                    start,
                    _lexer->comment_line,
                    _lexer->comment_column
                };

                d_internal_report(_lexer,
                                  D_LEX_ERR_UNTERMINATED_COMMENT,
                                  &from,
                                  d_reader_offset(&_lexer->reader));
            }

            _lexer->leading_space = true;

            if (emit)
            {
                *_out_comment     = start;
                *_out_comment_end = d_reader_offset(&_lexer->reader);
                return true;
            }

            continue;
        }

        // a line comment, where the dialect has them
        if ( (character == '/') &&
             (d_reader_peek(&_lexer->reader, 1u) == '/') &&
             d_lex_dialect_has(_lexer->dialect,
                               D_LEX_FEATURE_LINE_COMMENT) )
        {
            const size_t start = d_reader_offset(&_lexer->reader);
            const int    third = d_reader_peek(&_lexer->reader, 2u);
            const int    fourth = d_reader_peek(&_lexer->reader, 3u);

            _lexer->comment_line   = d_reader_line(&_lexer->reader);
            _lexer->comment_column = d_reader_column(&_lexer->reader);

            // `///` is documentation and `////` is a rule; `//!` is either way
            *_out_flags = D_TOKEN_FLAG_COMMENT_LINE |
                          ( ( ((third == '/') && (fourth != '/')) ||
                              (third == '!') )
                          ? D_TOKEN_FLAG_COMMENT_DOC
                          : D_TOKEN_FLAG_NONE );

            // the newline is left for the loop above, so a directive ends
            while ( (!d_reader_at_end(&_lexer->reader)) &&
                    (d_reader_peek(&_lexer->reader, 0u) != '\n') )
            {
                (void)d_reader_next(&_lexer->reader);
            }

            _lexer->leading_space = true;

            if (emit)
            {
                *_out_comment     = start;
                *_out_comment_end = d_reader_offset(&_lexer->reader);
                return true;
            }

            continue;
        }

        return false;
    }
}


//==============================================================================
// 5.  TOKEN SCANNING
//==============================================================================


/**
 * @brief Records one code point of the identifier being scanned.
 *
 * @param[in,out] _name  the record; may be `NULL` when no check is wanted.
 * @param[in]     _code  the code point, or 0xFFFFFFFF for an unresolved
 *                       named character, which makes the name uncheckable.
 */
static void
d_internal_name_add(
    struct d_internal_name* _name,
    uint32_t                _code
)
{
    if (!_name)
    {
        return;
    }

    // an unresolved or overlong name is judged by the quick check alone
    if ( (_code == 0xFFFFFFFFu) ||
         (_name->count == (size_t)D_INTERNAL_NAME_MAX) )
    {
        _name->overflowed = true;

        return;
    }

    _name->code_points[_name->count] = _code;
    ++_name->count;

    // ASCII is always YES, so only an extended character can make it unusual
    if ( (_code >= 0x80u) &&
         (d_lex_unicode_nfc_check(_code) != D_LEX_NFC_YES) )
    {
        _name->unusual = true;
    }

    return;
}


/**
 * @brief Reports whether a UCN's value may stand in an identifier there.
 *
 * @note Outside a literal no standard lets a UCN name a control character
 *       or a basic character, `$` excepted where `$` is an identifier
 *       character; and none lets it name a surrogate or anything past
 *       U+10FFFF.  Beyond that the dialect's identifier set decides.
 *
 * @param[in] _lexer    the scanner, for its dialect.
 * @param[in] _code     the value.
 * @param[in] _initial  whether the UCN begins the name.
 * @return `true` if the value is admissible, `false` otherwise.
 */
static bool
d_internal_ucn_admissible(
    const struct d_lexer* _lexer,
    uint32_t              _code,
    bool                  _initial
)
{
    // a named character is not resolved, so it cannot be refused
    if (_code == 0xFFFFFFFFu)
    {
        return true;
    }

    // `$` is the one basic character a UCN may name in a name
    if (_code < 0xA0u)
    {
        return ( (_code == '$') &&
                 d_lex_dialect_has(_lexer->dialect,
                                   D_LEX_FEATURE_DOLLAR) );
    }

    if (!d_lex_unicode_is_scalar(_code))
    {
        return false;
    }

    return _initial
        ? d_lex_unicode_is_id_start(_lexer->dialect->identifiers,
                                    _code)
        : d_lex_unicode_is_id_continue(_lexer->dialect->identifiers,
                                       _code);
}


/**
 * @brief Consumes a well-formed UCN inside an identifier, reporting it when
 *        its value may not stand there.
 *
 * @note The UCN stays in the identifier either way, which is where its
 *       writer put it; only a UCN that is not well-formed ends the name.
 *
 * @param[in,out] _lexer    the scanner to advance.
 * @param[in]     _length   the UCN's length in logical characters.
 * @param[in]     _code     its value.
 * @param[in]     _initial  whether it begins the name.
 */
static void
d_internal_take_ucn(
    struct d_lexer* _lexer,
    size_t          _length,
    uint32_t        _code,
    bool            _initial
)
{
    const struct d_internal_mark from = d_internal_here(_lexer);

    for (size_t at = 0u; at < _length; ++at)
    {
        (void)d_reader_next(&_lexer->reader);
    }

    // the value is checked only once the extent is known, so the report
    // covers exactly the UCN
    if (!d_internal_ucn_admissible(_lexer,
                                   _code,
                                   _initial))
    {
        d_internal_report(_lexer,
                          D_LEX_ERR_UNIVERSAL_NAME,
                          &from,
                          d_reader_offset(&_lexer->reader));
    }

    return;
}


/**
 * @brief Consumes an extended character if the dialect admits it in a name.
 *
 * @param[in,out] _lexer     the scanner to advance.
 * @param[in]     _initial   whether the character would begin the name.
 * @param[out]    _out_code  receives its code point.
 * @return `true` if it was consumed, `false` if it ends the name.
 */
static bool
d_internal_take_extended(
    struct d_lexer* _lexer,
    bool            _initial,
    uint32_t*       _out_code
)
{
    const size_t width = d_internal_extended_width(_lexer,
                                                   _initial,
                                                   _out_code);

    for (size_t at = 0u; at < width; ++at)
    {
        (void)d_reader_next(&_lexer->reader);
    }

    return (width > 0u);
}


/**
 * @brief Consumes an identifier and reports its extent.
 *
 * @note The name ends at the first character that cannot continue it: a
 *       basic character outside the grammar, an extended character outside
 *       the dialect's identifier set, a backslash that begins no well-formed
 *       UCN, or ill-formed UTF-8.  The first character must already be known
 *       to begin one.
 *
 * @param[in,out] _lexer  the scanner to advance.
 * @param[in,out] _flags  receives D_TOKEN_FLAG_EXTENDED or D_TOKEN_FLAG_UCN
 *                        when met.
 * @param[in,out] _name   receives the code points; may be `NULL`.
 */
static void
d_internal_scan_identifier(
    struct d_lexer*         _lexer,
    uint32_t*               _flags,
    struct d_internal_name* _name
)
{
    bool initial = true;

    for (;;)
    {
        const int character = d_reader_peek(&_lexer->reader, 0u);
        uint32_t  code      = (uint32_t)character;

        // a universal character name is part of the identifier that holds it
        if (character == '\\')
        {
            const size_t length = d_internal_ucn_length(_lexer,
                                                        0u,
                                                        &code);

            if (length == 0u)
            {
                return;
            }

            d_internal_take_ucn(_lexer,
                                length,
                                code,
                                initial);
            *_flags |= D_TOKEN_FLAG_UCN;
        }
        // an extended character, admitted by the dialect's identifier set
        else if (character >= 0x80)
        {
            if (!d_internal_take_extended(_lexer,
                                          initial,
                                          &code))
            {
                return;
            }

            *_flags |= D_TOKEN_FLAG_EXTENDED;
        }
        // a basic character; the dispatcher has already vetted the first
        else if (d_internal_is_basic_part(_lexer->dialect, character))
        {
            (void)d_reader_next(&_lexer->reader);
        }
        else
        {
            return;
        }

        d_internal_name_add(_name,
                            code);
        initial = false;
    }
}


/**
 * @brief Consumes whatever may continue a pp-number that is not a digit,
 *        letter or `.`: an extended identifier character or a UCN.
 *
 * @param[in,out] _lexer  the scanner to advance.
 * @return `true` if something was consumed, `false` otherwise.
 */
static bool
d_internal_number_extended(
    struct d_lexer* _lexer
)
{
    const int    first = d_reader_peek(&_lexer->reader, 0u);
    uint32_t     code  = 0u;

    // only an extended byte or a backslash can begin either
    if ( (first < 0x80) &&
         (first != '\\') )
    {
        return false;
    }

    size_t       width = d_internal_extended_width(_lexer,
                                                   false,
                                                   &code);

    // a UCN continues a pp-number exactly as it continues a name
    if (width == 0u)
    {
        width = d_internal_ucn_length(_lexer,
                                      0u,
                                      &code);
    }

    for (size_t at = 0u; at < width; ++at)
    {
        (void)d_reader_next(&_lexer->reader);
    }

    return (width > 0u);
}


/**
 * @brief Consumes a preprocessing number.
 *
 * @note A pp-number is not a numeric literal. `0x1e+2` is one token here and is
 *       ill-formed only later, which is what the standard requires and what
 *       keeps the scanner from needing to know a literal's grammar.
 *
 * @param[in,out] _lexer  the scanner to advance.
 */
static void
d_internal_scan_number(
    struct d_lexer* _lexer
)
{
    const bool separators =
        d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_DIGIT_SEPARATOR);

    (void)d_reader_next(&_lexer->reader);

    for (;;)
    {
        const int character = d_reader_peek(&_lexer->reader, 0u);

        // an exponent marker carries its sign into the same token
        if ( (character == 'e') || (character == 'E') ||
             (character == 'p') || (character == 'P') )
        {
            const int sign = d_reader_peek(&_lexer->reader, 1u);

            if ((sign == '+') || (sign == '-'))
            {
                (void)d_reader_next(&_lexer->reader);
                (void)d_reader_next(&_lexer->reader);
                continue;
            }
        }

        // a digit separator binds only between two number characters
        if (separators && (character == '\''))
        {
            const int following = d_reader_peek(&_lexer->reader, 1u);

            if (d_internal_is_basic_part(_lexer->dialect, following))
            {
                (void)d_reader_next(&_lexer->reader);
                (void)d_reader_next(&_lexer->reader);
                continue;
            }

            return;
        }

        if ( d_internal_is_basic_part(_lexer->dialect, character) ||
             (character == '.') )
        {
            (void)d_reader_next(&_lexer->reader);
            continue;
        }

        // an extended character or a UCN may continue it, as in a name
        if (!d_internal_number_extended(_lexer))
        {
            return;
        }
    }
}


/**
 * @brief Consumes a character or string literal, the opening quote included.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[in]     _quote      the closing quote to look for.
 * @param[in,out] _flags      receives D_TOKEN_FLAG_UNTERMINATED when the
 *                            literal did not close.
 * @param[out]    _out_empty  receives whether it closed with nothing inside.
 * @return `true` if the literal closed, `false` otherwise.
 */
static bool
d_internal_scan_quoted(
    struct d_lexer* _lexer,
    int             _quote,
    uint32_t*       _flags,
    bool*           _out_empty
)
{
    (void)d_reader_next(&_lexer->reader);

    *_out_empty = true;

    for (;;)
    {
        const int character = d_reader_peek(&_lexer->reader, 0u);

        // a literal may not cross a line, so the newline is left in place
        if ( (character == D_SOURCE_END) ||
             (character == '\n') )
        {
            *_flags |= D_TOKEN_FLAG_UNTERMINATED;

            return false;
        }

        if (character == _quote)
        {
            (void)d_reader_next(&_lexer->reader);

            return true;
        }

        *_out_empty = false;

        // a backslash takes the next character with it, whatever it is
        if (character == '\\')
        {
            (void)d_reader_next(&_lexer->reader);

            if (d_reader_peek(&_lexer->reader, 0u) != D_SOURCE_END)
            {
                (void)d_reader_next(&_lexer->reader);
            }

            continue;
        }

        (void)d_reader_next(&_lexer->reader);
    }
}


/**
 * @brief Reports whether an identifier begins at the position.
 *
 * @param[in] _lexer      the scanner to inspect; not advanced.
 * @param[in] _character  the character at the position, already peeked.
 * @return `true` if a basic start character, an admitted extended character
 *         or a well-formed UCN stands there.
 */
static bool
d_internal_identifier_begins(
    const struct d_lexer* _lexer,
    int                   _character
)
{
    const int character = _character;
    uint32_t  code      = 0u;

    if (d_internal_is_basic_start(_lexer->dialect, character))
    {
        return true;
    }

    // a UCN begins a name even with a value that may not, which is then
    // reported against the name rather than splitting it
    if (character == '\\')
    {
        return (d_internal_ucn_length(_lexer, 0u, &code) > 0u);
    }

    return ( (character >= 0x80) &&
             (d_internal_extended_width(_lexer, true, &code) > 0u) );
}


/**
 * @brief Consumes a user-defined literal suffix abutting the literal just read.
 *
 * @note The suffix is part of the literal token, not a separate identifier,
 *       which is why `"s"sv` is one token and `"s" sv` is two. A pp-number
 *       already swallows its suffix under the pp-number grammar, so only the
 *       quoted forms need this.
 *
 * @param[in,out] _lexer  the scanner to advance.
 * @param[in,out] _flags  receives D_TOKEN_FLAG_USER_SUFFIX when a suffix was
 *                        consumed.
 */
static void
d_internal_scan_user_suffix(
    struct d_lexer* _lexer,
    uint32_t*       _flags
)
{
    // the dialect must have the feature, and it arrived with C++11
    if (!d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_USER_SUFFIX))
    {
        return;
    }

    // only an identifier abutting the closing quote is a suffix
    if (!d_internal_identifier_begins(_lexer,
                                      d_reader_peek(&_lexer->reader, 0u)))
    {
        return;
    }

    d_internal_scan_identifier(_lexer,
                               _flags,
                               NULL);

    *_flags |= D_TOKEN_FLAG_USER_SUFFIX;

    return;
}


/**
 * @brief Consumes a header name in an inclusion directive.
 *
 * @param[in,out] _lexer  the scanner to advance.
 * @param[in]     _close  the character that ends the name.
 * @param[in,out] _flags  receives D_TOKEN_FLAG_UNTERMINATED when the name did
 *                        not close.
 * @return `true` if the name closed, `false` otherwise.
 */
static bool
d_internal_scan_header_name(
    struct d_lexer* _lexer,
    int             _close,
    uint32_t*       _flags
)
{
    (void)d_reader_next(&_lexer->reader);

    for (;;)
    {
        const int character = d_reader_peek(&_lexer->reader, 0u);

        // a header name may not cross the line its directive stands on
        if ((character == D_SOURCE_END) || (character == '\n'))
        {
            *_flags |= D_TOKEN_FLAG_UNTERMINATED;
            return false;
        }

        (void)d_reader_next(&_lexer->reader);

        if (character == _close)
        {
            return true;
        }
    }
}


/**
 * @brief Consumes a raw string literal, the opening quote included.
 *
 * @note This is the one construct no table can describe, because the closing
 *       sequence is built from text read at the opening. Its contents are
 *       physical, so the body is read through the reader's physical entry point
 *       and a splice inside it is text rather than a splice.
 *
 * @param[in,out] _lexer  the scanner to advance.
 * @param[in,out] _flags  receives D_TOKEN_FLAG_UNTERMINATED when the literal
 *                        did not close.
 * @return zero when the literal closed, otherwise the d_lex_error saying why
 *         it did not.
 */
static int
d_internal_scan_raw_string(
    struct d_lexer* _lexer,
    uint32_t*       _flags
)
{
    char   delimiter[D_LEX_RAW_DELIMITER_MAX + 1];
    size_t length = 0u;

    (void)d_reader_next(&_lexer->reader);

    // the delimiter runs from the quote to the opening parenthesis
    for (;;)
    {
        const int character = d_reader_peek_physical(&_lexer->reader, 0u);

        if (character == '(')
        {
            (void)d_reader_next_physical(&_lexer->reader);
            break;
        }

        // the delimiter is bounded and excludes these characters by rule
        if ( (character == D_SOURCE_END) || (character == '\\') ||
             (character == ')')          || (character == '\n') ||
             d_internal_is_horizontal_space(character)          ||
             (length == D_LEX_RAW_DELIMITER_MAX) )
        {
            *_flags |= D_TOKEN_FLAG_UNTERMINATED;
            return (int)D_LEX_ERR_RAW_DELIMITER;
        }

        delimiter[length] = (char)character;
        ++length;

        (void)d_reader_next_physical(&_lexer->reader);
    }

    delimiter[length] = '\0';

    // the body ends at the first parenthesis followed by the delimiter and a
    // quote, and at no other parenthesis
    for (;;)
    {
        const int character = d_reader_peek_physical(&_lexer->reader, 0u);

        if (character == D_SOURCE_END)
        {
            *_flags |= D_TOKEN_FLAG_UNTERMINATED;
            return (int)D_LEX_ERR_UNTERMINATED_RAW;
        }

        if (character == ')')
        {
            bool matched = true;

            for (size_t at = 0u; at < length; ++at)
            {
                if (d_reader_peek_physical(&_lexer->reader, at + 1u) !=
                    (int)(unsigned char)delimiter[at])
                {
                    matched = false;
                    break;
                }
            }

            if ( matched &&
                 (d_reader_peek_physical(&_lexer->reader, length + 1u) ==
                  '"') )
            {
                for (size_t at = 0u; at < (length + 2u); ++at)
                {
                    (void)d_reader_next_physical(&_lexer->reader);
                }

                return 0;
            }
        }

        (void)d_reader_next_physical(&_lexer->reader);
    }
}


/**
 * @brief Consumes the longest operator spelled at the position.
 *
 * @note Maximal munch, with the one exception the standard names: `<::` is `<`
 *       followed by `::` unless a `:` or `>` follows, so that `A<::B>` means
 *       what it appears to mean rather than opening a digraph bracket.
 *
 * @param[in,out] _lexer     the scanner to advance.
 * @param[out]    _out_kind  receives the operator's kind.
 * @param[in,out] _flags     receives D_TOKEN_FLAG_DIGRAPH for an alternative
 *                           spelling.
 * @return `true` if an operator was consumed, `false` with the position
 *         unchanged.
 */
static bool
d_internal_scan_operator(
    struct d_lexer* _lexer,
    int*            _out_kind,
    uint32_t*       _flags
)
{
    char   window[D_INTERNAL_OPERATOR_WINDOW];
    size_t available = 0u;

    while (available < (size_t)D_INTERNAL_OPERATOR_WINDOW)
    {
        const int character = d_reader_peek(&_lexer->reader, available);

        if ((character == D_SOURCE_END) || (character >= 0x80))
        {
            break;
        }

        window[available] = (char)character;
        ++available;
    }

    // nothing to match
    if (available == 0u)
    {
        return false;
    }

    // the `<::` rule, which only arises where digraphs exist
    if ( (available >= 3u) &&
         (window[0] == '<') && (window[1] == ':') && (window[2] == ':') &&
         d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_DIGRAPH) )
    {
        const int fourth = (available >= 4u) ? (int)window[3] : D_SOURCE_END;

        if ((fourth != ':') && (fourth != '>'))
        {
            (void)d_reader_next(&_lexer->reader);
            *_out_kind = D_TOKEN_LESS;
            return true;
        }
    }

    // the longest row that matches wins, so the table needs no ordering
    const struct d_lex_operator* best   = NULL;
    size_t                       chosen = 0u;

    for (size_t length = available; length > 0u; --length)
    {
        const struct d_lex_operator* const row =
            d_lex_dialect_operator(_lexer->dialect, window, length);

        if (row)
        {
            best   = row;
            chosen = length;
            break;
        }
    }

    // the character begins no operator in this dialect
    if (!best)
    {
        return false;
    }

    for (size_t at = 0u; at < chosen; ++at)
    {
        (void)d_reader_next(&_lexer->reader);
    }

    if ((best->flags & D_LEX_ROW_DIGRAPH) != 0u)
    {
        *_flags |= D_TOKEN_FLAG_DIGRAPH;
    }

    *_out_kind = best->kind;
    return true;
}


//==============================================================================
// 6.  DISPATCH
//==============================================================================
// The one place a token's kind is decided.  The order of the tests is the
// grammar's order, not a convenience: a prefix is tried before the identifier
// it resembles, and a number before the `.` that may open one.


/**
 * @brief Measures a literal prefix standing at the position, if one does.
 *
 * @note A prefix is only a prefix when a quote follows it. `u8` alone is an
 *       identifier, and so is `Rx`; only `u8"` and `R"` are literals.
 *
 * @param[in,out] _lexer      the scanner to inspect; not advanced.
 * @param[out]    _out_raw    receives whether the prefix opens a raw string.
 * @param[out]    _out_quote  receives the quote that follows the prefix.
 * @return the prefix length in characters, or zero when none stands here.
 */
static size_t
d_internal_prefix_length(
    struct d_lexer* _lexer,
    bool*           _out_raw,
    int*            _out_quote
)
{
    char window[D_INTERNAL_PREFIX_MAX];

    *_out_raw   = false;
    *_out_quote = D_SOURCE_END;

    // the longest prefix that is followed by a quote wins
    for (size_t length = (size_t)D_INTERNAL_PREFIX_MAX; length > 0u; --length)
    {
        bool usable = true;

        for (size_t at = 0u; at < length; ++at)
        {
            const int character = d_reader_peek(&_lexer->reader, at);

            if ((character == D_SOURCE_END) || (character >= 0x80))
            {
                usable = false;
                break;
            }

            window[at] = (char)character;
        }

        if (!usable)
        {
            continue;
        }

        const int following = d_reader_peek(&_lexer->reader, length);

        // only a quote turns the run into a prefix
        if ((following != '"') && (following != '\''))
        {
            continue;
        }

        const struct d_lex_prefix* const row =
            d_lex_dialect_prefix(_lexer->dialect, window, length);

        if (!row)
        {
            continue;
        }

        // the role must match the quote that follows
        const unsigned needed = (following == '"')
                              ? D_LEX_ROW_STRING
                              : D_LEX_ROW_CHARACTER;

        if ((row->flags & needed) == 0u)
        {
            continue;
        }

        *_out_raw   = ((row->flags & D_LEX_ROW_RAW) != 0u);
        *_out_quote = following;

        return length;
    }

    return 0u;
}


/**
 * @brief Resolves an identifier's extent to a keyword kind.
 *
 * @note An identifier whose byte range was rewritten by an earlier phase --
 *       `in`, a splice, then `t` -- is not spelled by its bytes, so the range
 *       cannot be offered to the table directly. The rewritten case re-reads
 *       the range through the reader, which is what resolved it in the first
 *       place; the common case compares the bytes where they lie and copies
 *       nothing.
 *
 * @param[in,out] _lexer   the scanner the extent was read from.
 * @param[in]     _start   the extent's offset.
 * @param[in]     _length  the extent's length in bytes.
 * @return the keyword kind, or D_TOKEN_IDENTIFIER when the spelling is not one.
 */
static int
d_internal_resolve_keyword(
    struct d_lexer* _lexer,
    size_t          _start,
    size_t          _length
)
{
    // the bytes are the spelling, which is the case for nearly every token
    if (!_lexer->reader.spliced)
    {
        return d_lex_dialect_keyword(_lexer->dialect,
                                     _lexer->reader.source->text + _start,
                                     _length);
    }

    char   spelled[D_INTERNAL_SPELLING_MAX];
    size_t length = 0u;

    struct d_reader reader = _lexer->reader;

    d_reader_seek(&reader, _start);

    // re-read the range, which drops the splices the flag warned about
    while (d_reader_offset(&reader) < (_start + _length))
    {
        const int character = d_reader_next(&reader);

        if (character == D_SOURCE_END)
        {
            break;
        }

        // a keyword longer than the buffer does not exist, so give up early
        if (length == (size_t)D_INTERNAL_SPELLING_MAX)
        {
            return D_TOKEN_IDENTIFIER;
        }

        spelled[length] = (char)character;
        ++length;
    }

    return d_lex_dialect_keyword(_lexer->dialect, spelled, length);
}


/**
 * @brief Reports whether an extent spells one of a list of words.
 *
 * @param[in] _text    the extent's first byte.
 * @param[in] _length  its length.
 * @param[in] _words   the words, NULL-terminated.
 * @return `true` if the extent spells one of them, `false` otherwise.
 */
static bool
d_internal_is_one_of(
    const char*        _text,
    size_t             _length,
    const char* const* _words
)
{
    for (size_t at = 0u; _words[at]; ++at)
    {
        if ( (strlen(_words[at]) == _length) &&
             (memcmp(_words[at], _text, _length) == 0) )
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Maintains the directive state after a token has been classified, so
 *        that the next token is read as a header name, a macro name, or
 *        neither, exactly where the directive grammar says.
 *
 * @note D_LEX_OPT_MANUAL_HEADER hands only the header context to the caller.
 *       Directive tracking itself continues, since d_lex_in_directive and the
 *       directive flags depend on it and neither is the caller's to drive.
 *
 * @param[in,out] _lexer   the scanner to update.
 * @param[in]     _kind    the kind just produced.
 * @param[in]     _start   the token's offset.
 * @param[in]     _length  the token's length.
 */
static void
d_internal_note_directive(
    struct d_lexer* _lexer,
    int             _kind,
    size_t          _start,
    size_t          _length
)
{
    static const char* const defining[] =
    {
        "define", "undef", "ifdef", "ifndef", "elifdef", "elifndef", NULL
    };
    static const char* const including[] =
    {
        "include", "include_next", "import", "embed", NULL
    };
    static const char* const testing[] =
    {
        "__has_include", "__has_include_next", "__has_embed", NULL
    };

    const char* const text      = _lexer->reader.source->text + _start;
    const bool        automatic =
        ( ((_lexer->options & D_LEX_OPT_MANUAL_HEADER) == 0u) &&
          d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_HEADER_NAME) );

    // a header name is expected by one token only, or by the one after an
    // opening parenthesis that follows it
    if (automatic && _lexer->header_context && (_kind != D_TOKEN_PAREN_OPEN))
    {
        _lexer->header_context = false;
    }

    // a hash first on its line opens a directive
    if ((_kind == D_TOKEN_HASH) && _lexer->line_start)
    {
        _lexer->in_directive = true;
        _lexer->after_hash   = true;
        _lexer->expect_macro = false;
        return;
    }

    // nothing below applies outside a directive
    if (!_lexer->in_directive)
    {
        return;
    }

    // the directive's name decides what follows it
    if (_lexer->after_hash)
    {
        _lexer->after_hash = false;

        // `# 12` is a line marker and `#` alone the null directive
        if (_kind != D_TOKEN_IDENTIFIER)
        {
            return;
        }

        if (d_internal_is_one_of(text, _length, defining))
        {
            _lexer->expect_macro = true;
        }

        if (automatic && d_internal_is_one_of(text, _length, including))
        {
            _lexer->header_context = true;
        }

        return;
    }

    // `defined` tests a macro, with or without parentheses
    if ( (_kind == D_TOKEN_IDENTIFIER) &&
         (_length == 7u) && (memcmp(text, "defined", 7u) == 0) )
    {
        _lexer->expect_macro = true;
        return;
    }

    // `__has_include` takes a header name after its parenthesis
    if ( automatic && (_kind == D_TOKEN_IDENTIFIER) &&
         d_internal_is_one_of(text, _length, testing) )
    {
        _lexer->header_context = true;
        return;
    }

    // an opening parenthesis passes an expected macro name through
    if (_kind != D_TOKEN_PAREN_OPEN)
    {
        _lexer->expect_macro = false;
    }

    return;
}


/**
 * @brief Scans a character or string literal whose prefix, if any, stands at
 *        the position.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[in]     _prefix     the prefix: its length, possibly zero, whether
 *                            it opens a raw string, and the quote after it.
 * @param[in,out] _flags      receives the token's literal flags.
 * @param[out]    _out_error  receives a d_lex_error, or zero.
 * @return the token's kind: a literal kind, or D_TOKEN_ERROR.
 */
static int
d_internal_scan_literal(
    struct d_lexer*                 _lexer,
    const struct d_internal_prefix* _prefix,
    uint32_t*                       _flags,
    int*                            _out_error
)
{
    const int quote = _prefix->quote;
    bool      empty = false;

    for (size_t at = 0u; at < _prefix->length; ++at)
    {
        (void)d_reader_next(&_lexer->reader);
    }

    // a raw string reports its own failure, which has two causes
    if (_prefix->raw)
    {
        *_out_error = (int)d_internal_scan_raw_string(_lexer,
                                                      _flags);

        if (*_out_error != 0)
        {
            return D_TOKEN_ERROR;
        }

        d_internal_scan_user_suffix(_lexer,
                                    _flags);

        return D_TOKEN_RAW_STRING;
    }

    // an unclosed literal and an empty character literal are both errors,
    // and neither takes a suffix
    if (!d_internal_scan_quoted(_lexer,
                                quote,
                                _flags,
                                &empty))
    {
        *_out_error = D_LEX_ERR_UNTERMINATED_LITERAL;

        return D_TOKEN_ERROR;
    }

    if ( empty &&
         (quote == '\'') )
    {
        *_out_error = D_LEX_ERR_EMPTY_CHARACTER;

        return D_TOKEN_ERROR;
    }

    d_internal_scan_user_suffix(_lexer,
                                _flags);

    return (quote == '"') ? D_TOKEN_STRING : D_TOKEN_CHARACTER;
}


/**
 * @brief Scans an identifier and decides what it is: a directive name, a
 *        macro name, a keyword, an alternative token, or an identifier.
 *
 * @param[in,out] _lexer  the scanner to advance.
 * @param[in]     _start  the identifier's first byte.
 * @param[in,out] _flags  receives the identifier's flags.
 * @param[out]    _name   receives its code points, for the NFC check.
 * @return the token's kind.
 */
static int
d_internal_scan_word(
    struct d_lexer*         _lexer,
    size_t                  _start,
    uint32_t*               _flags,
    struct d_internal_name* _name
)
{
    d_internal_scan_identifier(_lexer,
                               _flags,
                               _name);

    // a directive line never reaches phase 7, so its name is not a keyword
    // even where it is spelled like one
    if ( _lexer->in_directive &&
         _lexer->after_hash )
    {
        *_flags |= D_TOKEN_FLAG_DIRECTIVE_NAME;

        return D_TOKEN_IDENTIFIER;
    }

    // the identifier a directive defines or tests is one by definition
    if ( _lexer->in_directive &&
         _lexer->expect_macro )
    {
        *_flags              |= D_TOKEN_FLAG_MACRO_NAME;
        _lexer->expect_macro  = false;

        return D_TOKEN_IDENTIFIER;
    }

    // keywords are resolved only when the caller asked for phase 7
    if ((_lexer->options & D_LEX_OPT_KEYWORDS) == 0u)
    {
        return D_TOKEN_IDENTIFIER;
    }

    const int kind =
        d_internal_resolve_keyword(_lexer,
                                   _start,
                                   d_reader_offset(&_lexer->reader) - _start);

    // an alternative token is a punctuator spelled as a word
    if (d_token_is_punctuator(kind))
    {
        *_flags |= D_TOKEN_FLAG_DIGRAPH;
    }

    return kind;
}


/**
 * @brief Consumes a character that begins no token.
 *
 * @note An extended character is consumed whole, so one stray code point is
 *       one token and one report; ill-formed UTF-8 is consumed a byte at a
 *       time and reported as an encoding error by the reader instead.  A
 *       backslash that opens a UCN which is not well-formed is reported as
 *       that, which says more than calling it stray.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[in]     _character  the character at the position.
 * @param[out]    _out_error  receives a d_lex_error, or zero.
 * @return D_TOKEN_OTHER.
 */
static int
d_internal_scan_stray(
    struct d_lexer* _lexer,
    int             _character,
    int*            _out_error
)
{
    const int  marker = d_reader_peek(&_lexer->reader, 1u);
    size_t     width  = 1u;

    *_out_error = D_LEX_ERR_STRAY_CHARACTER;

    // `\u12` and the like: a UCN that is not well-formed
    if ( (_character == '\\') &&
         d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_UCN) &&
         ( (marker == 'u') ||
           (marker == 'U') ||
           ( (marker == 'N') &&
             d_lex_dialect_has(_lexer->dialect,
                               D_LEX_FEATURE_NAMED_ESCAPE) ) ) )
    {
        *_out_error = D_LEX_ERR_UNIVERSAL_NAME;
    }

    // an extended character is taken whole; a malformed byte, alone
    if (_character >= 0x80)
    {
        const struct d_source* const source = _lexer->reader.source;
        const size_t                 at     = d_internal_physical_here(_lexer);
        uint32_t                     code   = 0u;

        width = d_source_utf8_decode(source->text + at,
                                     source->length - at,
                                     &code);

        if (width == 0u)
        {
            width       = 1u;
            *_out_error = 0;
        }
    }

    for (size_t at = 0u; at < width; ++at)
    {
        (void)d_reader_next(&_lexer->reader);
    }

    return D_TOKEN_OTHER;
}


/**
 * @brief Scans a header name, where the directive says one stands.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[in]     _character  the opening `<` or `"`.
 * @param[in,out] _flags      receives the token's flags.
 * @param[out]    _out_error  receives a d_lex_error, or zero.
 * @return D_TOKEN_HEADER_NAME, or D_TOKEN_ERROR when it did not close.
 */
static int
d_internal_scan_header(
    struct d_lexer* _lexer,
    int             _character,
    uint32_t*       _flags,
    int*            _out_error
)
{
    _lexer->header_context = false;

    if (!d_internal_scan_header_name(_lexer,
                                     (_character == '<') ? '>' : '"',
                                     _flags))
    {
        *_out_error = D_LEX_ERR_UNTERMINATED_LITERAL;

        return D_TOKEN_ERROR;
    }

    return D_TOKEN_HEADER_NAME;
}


/**
 * @brief Decides the kind of the token at the position and consumes it.
 *
 * @note The order of the tests is the grammar's: a header name where a
 *       directive expects one, then a literal with or without a prefix, a
 *       pp-number before the `.` that may open it, an identifier, an
 *       operator, and last a character that is none of these.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[in]     _start      the token's first byte.
 * @param[in,out] _flags      receives the token's flags.
 * @param[out]    _out_error  receives a d_lex_error, or zero.
 * @param[out]    _name       receives an identifier's code points.
 * @return the token's kind.
 */
static int
d_internal_classify(
    struct d_lexer*         _lexer,
    size_t                  _start,
    uint32_t*               _flags,
    int*                    _out_error,
    struct d_internal_name* _name
)
{
    const int                character = d_reader_peek(&_lexer->reader, 0u);
    int                      kind      = D_TOKEN_END;
    struct d_internal_prefix prefix    = { 0u, false, D_SOURCE_END };

    prefix.length = d_internal_prefix_length(_lexer,
                                             &prefix.raw,
                                             &prefix.quote);

    // a header name, where the directive says one stands
    if ( _lexer->header_context &&
         ( (character == '<') ||
           (character == '"') ) )
    {
        return d_internal_scan_header(_lexer,
                                      character,
                                      _flags,
                                      _out_error);
    }

    // a literal, prefixed or not; unprefixed, the quote is the character
    if ( (prefix.length > 0u) ||
         (character == '"')   ||
         (character == '\'') )
    {
        prefix.quote = (prefix.length > 0u) ? prefix.quote : character;

        return d_internal_scan_literal(_lexer,
                                       &prefix,
                                       _flags,
                                       _out_error);
    }

    // a pp-number, which `.` may open when a digit follows
    if ( d_internal_is_digit(character) ||
         ( (character == '.') &&
           d_internal_is_digit(d_reader_peek(&_lexer->reader, 1u)) ) )
    {
        d_internal_scan_number(_lexer);

        return D_TOKEN_NUMBER;
    }

    if (d_internal_identifier_begins(_lexer,
                                     character))
    {
        return d_internal_scan_word(_lexer,
                                    _start,
                                    _flags,
                                    _name);
    }

    if (d_internal_scan_operator(_lexer,
                                 &kind,
                                 _flags))
    {
        return kind;
    }

    return d_internal_scan_stray(_lexer,
                                 character,
                                 _out_error);
}


/**
 * @brief Computes the flags a token carries from its position alone.
 *
 * @param[in] _lexer          the scanner, before the token is consumed.
 * @param[in] _at_line_start  whether no token precedes it on its line.
 * @param[in] _had_space      whether whitespace or a comment precedes it.
 * @return the D_TOKEN_FLAG_* bits for line start, space and directive.
 */
static uint32_t
d_internal_position_flags(
    const struct d_lexer* _lexer,
    bool                  _at_line_start,
    bool                  _had_space
)
{
    const int first  = d_reader_peek(&_lexer->reader, 0u);
    uint32_t  flags  = D_TOKEN_FLAG_NONE;

    if (_at_line_start)
    {
        flags |= D_TOKEN_FLAG_LINE_START;
    }

    if (_had_space)
    {
        flags |= D_TOKEN_FLAG_LEADING_SPACE;
    }

    // every token on a directive line says so, the opening `#` included,
    // which is decided before the `#` itself opens the directive
    if ( _lexer->in_directive ||
         ( _at_line_start &&
           (first == '#') ) ||
         ( _at_line_start &&
           (first == '%') &&
           (d_reader_peek(&_lexer->reader, 1u) == ':') &&
           d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_DIGRAPH) ) )
    {
        flags |= D_TOKEN_FLAG_IN_DIRECTIVE;
    }

    return flags;
}


/**
 * @brief Writes a token's record.
 *
 * @param[in]  _lexer  the scanner, positioned just past the token.
 * @param[out] _token  the record to fill.
 * @param[in]  _kind   the token's kind.
 * @param[in]  _flags  its flags.
 * @param[in]  _from   where it began.
 */
static void
d_internal_fill(
    const struct d_lexer*         _lexer,
    struct d_token*               _token,
    int                           _kind,
    uint32_t                      _flags,
    const struct d_internal_mark* _from
)
{
    const size_t end = d_reader_offset(&_lexer->reader);

    _token->kind       = _kind;
    _token->flags      = _flags;
    _token->span       = d_parse_span_located(_lexer->reader.source->id,
                                              (uint32_t)_from->offset,
                                              (uint32_t)(end - _from->offset),
                                              _from->line,
                                              _from->column);
    _token->end_line   = d_reader_line(&_lexer->reader);
    _token->end_column = d_reader_column(&_lexer->reader);

    return;
}


/**
 * @brief Reports a token's error and, for an extended name in a dialect that
 *        requires it, a name that is not in NFC.
 *
 * @param[in,out] _lexer  the scanner.
 * @param[in]     _token  the token just produced.
 * @param[in]     _error  its d_lex_error, or zero.
 * @param[in]     _name   its code points, when it was an identifier.
 */
static void
d_internal_report_token(
    struct d_lexer*               _lexer,
    const struct d_token*         _token,
    int                           _error,
    const struct d_internal_name* _name
)
{
    const struct d_internal_mark from =
    {
        _token->span.offset,
        _token->span.line,
        _token->span.column
    };
    const size_t end = from.offset + _token->span.length;

    if (_error != 0)
    {
        d_internal_report(_lexer,
                          _error,
                          &from,
                          end);
    }

    // only a name holding a character whose quick check was not YES can
    // fail, which spares every ordinary name the check
    if ( _name->unusual &&
         d_lex_dialect_has(_lexer->dialect, D_LEX_FEATURE_IDENTIFIER_NFC) &&
         (!d_lex_unicode_is_nfc(_name->code_points,
                                _name->count,
                                !_name->overflowed)) )
    {
        d_internal_report(_lexer,
                          D_LEX_ERR_NOT_NFC,
                          &from,
                          end);
    }

    return;
}


/**
 * @brief Produces a token that is not scanned: a reported comment, which
 *        trivia has already consumed, or the end of the source.
 *
 * @param[in,out] _lexer    the scanner.
 * @param[out]    _token    receives the token.
 * @param[in]     _comment  whether a comment was met; otherwise the end.
 * @param[in]     _flags    the position flags, the comment's own included.
 * @param[in]     _from     where it began.
 * @return `true` for a comment, `false` for the end.
 */
static bool
d_internal_trivia_token(
    struct d_lexer*               _lexer,
    struct d_token*               _token,
    bool                          _comment,
    uint32_t                      _flags,
    const struct d_internal_mark* _from
)
{
    // a comment is whitespace to the grammar, so it does not end the line's
    // start, and it stands in a directive only if the directive is open
    const uint32_t flags = _comment
        ? ( (_flags & ~(uint32_t)D_TOKEN_FLAG_IN_DIRECTIVE) |
            (_lexer->in_directive ? D_TOKEN_FLAG_IN_DIRECTIVE
                                  : D_TOKEN_FLAG_NONE) )
        : _flags;

    d_internal_fill(_lexer,
                    _token,
                    _comment ? D_TOKEN_COMMENT : D_TOKEN_END,
                    flags,
                    _from);

    return _comment;
}


/**
 * @brief Produces the token at the scan position.
 *
 * @note The scanner's state machine: trivia, then position flags, then the
 *       kind, then the record, the reports and the directive state, in that
 *       order because each reads what the one before it established.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[out]    _out_token  receives the token.
 * @return `true` if a token was produced, `false` at the end of the source,
 *         with the kind set to D_TOKEN_END.
 */
static bool
d_internal_scan(
    struct d_lexer* _lexer,
    struct d_token* _out_token
)
{
    size_t                 comment_start = (size_t)-1;
    size_t                 comment_end   = (size_t)-1;
    uint32_t               comment_flags = D_TOKEN_FLAG_NONE;
    struct d_internal_name name;
    int                    error         = 0;
    const bool             comment       =
        d_internal_skip_trivia(_lexer,
                               &comment_start,
                               &comment_end,
                               &comment_flags);
    struct d_internal_mark from          = d_internal_here(_lexer);
    uint32_t               flags         =
        d_internal_position_flags(_lexer,
                                  _lexer->line_start,
                                  _lexer->leading_space);

    d_internal_check_encoding(_lexer);

    // the code points are written as they are read; only the tallies need a
    // start, so the array is not cleared for every token
    name.count      = 0u;
    name.unusual    = false;
    name.overflowed = false;

    // the flag accumulates over the token only, so trivia does not set it
    _lexer->reader.spliced = false;

    // a reported comment began where trivia found it, not here
    if (comment)
    {
        from.offset = comment_start;
        from.line   = _lexer->comment_line;
        from.column = _lexer->comment_column;
    }

    // a comment was asked for and met, or nothing remains
    if ( comment ||
         (d_reader_peek(&_lexer->reader, 0u) == D_SOURCE_END) )
    {
        return d_internal_trivia_token(_lexer,
                                       _out_token,
                                       comment,
                                       flags | comment_flags,
                                       &from);
    }

    const int kind = d_internal_classify(_lexer,
                                         from.offset,
                                         &flags,
                                         &error,
                                         &name);

    // the byte range is wider than the spelling when a splice lay inside it
    flags |= _lexer->reader.spliced ? D_TOKEN_FLAG_SPLICED : D_TOKEN_FLAG_NONE;

    d_internal_fill(_lexer,
                    _out_token,
                    kind,
                    flags,
                    &from);
    d_internal_report_token(_lexer,
                            _out_token,
                            error,
                            &name);
    d_internal_check_encoding(_lexer);
    d_internal_note_directive(_lexer,
                              kind,
                              from.offset,
                              _out_token->span.length);

    _lexer->line_start    = false;
    _lexer->leading_space = false;

    return true;
}


//==============================================================================
// 7.  PUBLIC INTERFACE
//==============================================================================


/*
d_lex_create
  Builds a scanner over a source using a dialect descriptor.
*/
struct d_lexer*
d_lex_create(
    const struct d_source*      _source,
    const struct d_lex_dialect* _dialect,
    unsigned                    _options
)
{
    // parameter validation first; a span addresses 32 bits of offset
    if ( (!_source)                          ||
         (!_source->text)                    ||
         (!_dialect)                         ||
         (_source->length > (size_t)UINT32_MAX) )
    {
        return NULL;
    }

    struct d_lexer* const lexer = calloc(1u, sizeof(*lexer));

    // the handle could not be held
    if (!lexer)
    {
        return NULL;
    }

    unsigned reader_options = D_READER_OPT_NONE;

    // trigraphs are a property of the language, not a choice of the caller
    if (d_lex_dialect_has(_dialect, D_LEX_FEATURE_TRIGRAPH))
    {
        reader_options |= D_READER_OPT_TRIGRAPH;
    }

    // the source is UTF-8 unless the caller says otherwise
    if ((_options & D_LEX_OPT_RAW_BYTES) == 0u)
    {
        reader_options |= D_READER_OPT_UTF8;
    }

    d_reader_init(&lexer->reader, _source, reader_options);

    lexer->dialect      = _dialect;
    lexer->options      = _options;
    lexer->line_start   = true;
    lexer->sink         = NULL;

    return lexer;
}


/*
d_lex_destroy
  Releases a scanner.  The source and the descriptor are borrowed and are not
touched.
*/
void
d_lex_destroy(
    struct d_lexer* _lexer
)
{
    free(_lexer);

    return;
}


/*
d_lex_reset
  Moves the scanner to an offset and clears the directive state, since an
arbitrary offset stands in no known directive.
*/
void
d_lex_reset(
    struct d_lexer* _lexer,
    size_t          _offset
)
{
    // parameter validation first
    if (!_lexer)
    {
        return;
    }

    d_reader_seek(&_lexer->reader, _offset);

    _lexer->line_start     = d_reader_at_line_start(&_lexer->reader);
    _lexer->leading_space  = false;
    _lexer->in_directive   = false;
    _lexer->after_hash     = false;
    _lexer->expect_macro   = false;
    _lexer->header_context = false;

    return;
}


/*
d_lex_set_diagnostics
  Attaches the sink every later diagnostic goes to.
*/
void
d_lex_set_diagnostics(
    struct d_lexer*           _lexer,
    struct d_parse_diag_sink* _sink
)
{
    // parameter validation first
    if (!_lexer)
    {
        return;
    }

    _lexer->sink = _sink;

    return;
}


/*
d_lex_next
  Reports the token at the scan position and advances past it.
*/
bool
d_lex_next(
    struct d_lexer* _lexer,
    struct d_token* _out_token
)
{
    // parameter validation first
    if ((!_lexer) || (!_out_token))
    {
        return false;
    }

    return d_internal_scan(_lexer, _out_token);
}


/*
d_lex_peek
  Reports the token `_ahead` positions forward without moving.
  The scanner owns nothing but itself, so a copy is a complete saved position
and the rescan needs no undo.
*/
bool
d_lex_peek(
    struct d_lexer* _lexer,
    size_t          _ahead,
    struct d_token* _out_token
)
{
    // parameter validation first
    if ((!_lexer) || (!_out_token))
    {
        return false;
    }

    struct d_lexer scratch = *_lexer;
    bool           found   = false;

    // a peeked token is reported when it is consumed, not now
    scratch.sink = NULL;

    for (size_t at = 0u; at <= _ahead; ++at)
    {
        found = d_internal_scan(&scratch, _out_token);

        // the source ended before the position did
        if (!found)
        {
            return false;
        }
    }

    return found;
}


/*
d_lex_tokenize
  Fills a caller-supplied array with as many tokens as it holds.
*/
size_t
d_lex_tokenize(
    struct d_lexer* _lexer,
    struct d_token* _out_tokens,
    size_t          _capacity
)
{
    // parameter validation first
    if ((!_lexer) || (!_out_tokens) || (_capacity == 0u))
    {
        return 0u;
    }

    size_t count = 0u;

    while (count < _capacity)
    {
        if (!d_internal_scan(_lexer, &_out_tokens[count]))
        {
            break;
        }

        ++count;
    }

    return count;
}


/*
d_lex_offset
  Reports the scan position as a physical byte offset.
*/
size_t
d_lex_offset(
    const struct d_lexer* _lexer
)
{
    return _lexer ? d_reader_offset(&_lexer->reader) : 0u;
}


/*
d_lex_dialect_of
  Reports the descriptor a scanner reads by.
*/
const struct d_lex_dialect*
d_lex_dialect_of(
    const struct d_lexer* _lexer
)
{
    return _lexer ? _lexer->dialect : NULL;
}


/*
d_lex_source_of
  Reports the source a scanner reads.
*/
const struct d_source*
d_lex_source_of(
    const struct d_lexer* _lexer
)
{
    return _lexer ? _lexer->reader.source : NULL;
}


/*
d_lex_in_directive
  Reports whether the scanner stands inside a preprocessing directive.
*/
bool
d_lex_in_directive(
    const struct d_lexer* _lexer
)
{
    return (_lexer && _lexer->in_directive);
}


/*
d_lex_header_context
  Reports whether the next `<` would open a header name.
*/
bool
d_lex_header_context(
    const struct d_lexer* _lexer
)
{
    return (_lexer && _lexer->header_context);
}


/*
d_lex_set_header_context
  Declares that the next `<` does or does not open a header name.
*/
void
d_lex_set_header_context(
    struct d_lexer* _lexer,
    bool            _enabled
)
{
    // parameter validation first
    if (!_lexer)
    {
        return;
    }

    _lexer->header_context = _enabled;

    return;
}


/*
d_token_spell
  Writes the phase-3 spelling of a token, splices and trigraphs resolved.
  Reported in the manner of snprintf: the return is what the spelling needs,
so a return of `_size` or more means the result was truncated.
A raw string is exempt from the re-reading.  Its contents are physical text
by rule, so resolving a splice inside one would produce a spelling the
source does not have and that would not lex back to the same token.
*/
size_t
d_token_spell(
    const struct d_lexer* _lexer,
    const struct d_token* _token,
    char*                 _out_buffer,
    size_t                _size
)
{
    // parameter validation first
    if ((!_lexer) || (!_token) || (!_lexer->reader.source))
    {
        return 0u;
    }

    struct d_reader reader = _lexer->reader;
    size_t          needed = 0u;

    d_reader_seek(&reader, _token->span.offset);

    const size_t end = (size_t)_token->span.offset + _token->span.length;
    const bool   raw = (_token->kind == D_TOKEN_RAW_STRING);

    // read the token again, which resolves what the flags warned about
    while (d_reader_offset(&reader) < end)
    {
        const int character = raw ? d_reader_next_physical(&reader)
                                  : d_reader_next(&reader);

        if (character == D_SOURCE_END)
        {
            break;
        }

        if ((_out_buffer) && ((needed + 1u) < _size))
        {
            _out_buffer[needed] = (char)character;
        }

        ++needed;
    }

    if ((_out_buffer) && (_size > 0u))
    {
        _out_buffer[(needed < _size) ? needed : (_size - 1u)] = '\0';
    }

    return needed;
}


/*
d_lex_error_text
  One phrase per code, for the diagnostic's message.  The phrase describes the
source, not the scanner, so it reads the same in any consumer.
*/
const char*
d_lex_error_text(
    int _code
)
{
    static const char* const texts[D_LEX_ERR_COUNT_] =
    {
        NULL,
        "comment not closed before end of source",
        "literal not closed before end of line",
        "raw string not closed before end of source",
        "raw string delimiter is ill-formed",
        "character literal is empty",
        "universal character name is ill-formed or not allowed here",
        "source is not well-formed UTF-8",
        "character belongs to no token",
        "identifier is not in Normalization Form C",
        "numeric literal is malformed",
        "digit is not valid in this base",
        "suffix is not valid on this literal",
        "integer literal is too large for any integer type",
        "escape sequence is malformed",
        "escape sequence value is out of range for its code unit",
        "escape sequence is not recognized",
        "character does not fit one code unit of the literal's encoding",
        "character literal holds more than one character",
        "literal suffix without a leading underscore is reserved",
        "character does not fit one code unit; its value is "
        "implementation-defined"
    };

    // zero and anything past the table are not codes
    if ( (_code <= 0) ||
         (_code >= (int)D_LEX_ERR_COUNT_) )
    {
        return "unrecognized code";
    }

    return texts[_code];
}


/*
d_lex_error_severity
  The three codes the standards leave implementation-defined or conditionally
supported are warnings; everything else makes the source ill-formed.
*/
int
d_lex_error_severity(
    int _code
)
{
    switch (_code)
    {
        case D_LEX_ERR_UNKNOWN_ESCAPE:
        case D_LEX_ERR_MULTICHARACTER:
        case D_LEX_ERR_RESERVED_SUFFIX:
        case D_LEX_ERR_WIDE_CHARACTER:
            return D_PARSE_SEVERITY_WARNING;
        default:
            return D_PARSE_SEVERITY_ERROR;
    }
}
