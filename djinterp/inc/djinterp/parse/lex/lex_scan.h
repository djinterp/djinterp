/*******************************************************************************
* djinterp [parse]                                                    lex_scan.h
*
* The scanner: translation phase 3, driven by a dialect descriptor.
*   There is one engine.  C and C++ differ to it only by the descriptor they
* hand it, so a defect fixed in a literal or a comment is fixed for both
* languages at once and cannot be fixed for one and missed in the other.
*   Options here are the caller's choices -- emit comments, resolve
* keywords -- and are kept apart from the dialect's features, which are facts
* about the language.  Asking for comments is a decision; having `//` at all
* is not.
*   Errors do not stop the scan.  An ill-formed construct becomes a token of
* kind D_TOKEN_ERROR spanning the offending text, a diagnostic is emitted into
* the sink the caller attached -- the parse subframework's one channel,
* domain D_PARSE_DIAG_DOMAIN_LEX -- and scanning resumes, so one pass reports
* every defect in a file rather than the first.  With no sink attached the
* tokens still say what went wrong; a caller wanting only pass or fail attaches
* a sink with no storage, which counts and allocates nothing.
*   Phase 3 is checked here, and only phase 3: whether a pp-number is a valid
* number, or an escape a valid escape, is decided when the token reaches
* phase 7, since a token in a skipped group or a stringized argument never
* does.  lex_literal.h makes that decision on request.
*
*
* path:      /inc/djinterp/parse/lex/lex_scan.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  Scanner options
    2.  Types
         1.  d_lex_error
         2.  d_lexer
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Scanning
    3.  Directive context
    4.  Spelling
    5.  Diagnostics
*/

#ifndef DJINTERP_PARSE_LEX_LEX_SCAN_H
#define DJINTERP_PARSE_LEX_LEX_SCAN_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../c/diagnostic.h"          // d_parse_diag_sink
#include "../source/source_reader.h"  // d_source
#include "./lex_dialect.h"            // d_lex_dialect
#include "./lex_token.h"              // d_token


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// Scanner options
//   constant: what the caller wants, as opposed to what the language is.  A
// preprocessor wants neither keywords resolved nor comments discarded; a
// compiler front end wants the first and not the second; a formatter wants
// both comments and keywords.  None of the three is a property of C or C++.
#define D_LEX_OPT_NONE            0x0000u
#define D_LEX_OPT_EMIT_COMMENTS   0x0001u  // report comments as tokens
#define D_LEX_OPT_KEYWORDS        0x0002u  // resolve identifiers (phase 7)
#define D_LEX_OPT_MANUAL_HEADER   0x0004u  // caller drives header context
#define D_LEX_OPT_RAW_BYTES       0x0008u  // not UTF-8: no encoding check,
                                           // columns count bytes

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_lex_error
//   enum: the codes of domain D_PARSE_DIAG_DOMAIN_LEX, raised by the scanner
//   (phase 3) and by the literal decoder in lex_literal.h (phase 7).  Zero is
//   not a code.  Values are stable: a stored diagnostic outlives a rebuild,
//   so a code is appended, never renumbered.  The severity each carries is
//   given where it is raised; UNKNOWN_ESCAPE, MULTICHARACTER,
//   RESERVED_SUFFIX and WIDE_CHARACTER are warnings, since the standards
//   make them implementation-defined or conditionally supported rather than
//   ill-formed, and everything else is an error.
enum d_lex_error
{
    D_LEX_ERR_UNTERMINATED_COMMENT = 1,   // `/*` with no `*/`
    D_LEX_ERR_UNTERMINATED_LITERAL = 2,   // quote or header name unclosed
    D_LEX_ERR_UNTERMINATED_RAW     = 3,   // raw string unclosed
    D_LEX_ERR_RAW_DELIMITER        = 4,   // raw delimiter ill-formed
    D_LEX_ERR_EMPTY_CHARACTER      = 5,   // `''`
    D_LEX_ERR_UNIVERSAL_NAME       = 6,   // UCN ill-formed or not allowed
    D_LEX_ERR_ENCODING             = 7,   // ill-formed UTF-8
    D_LEX_ERR_STRAY_CHARACTER      = 8,   // belongs to no token
    D_LEX_ERR_NOT_NFC              = 9,   // a name not in NFC
    D_LEX_ERR_NUMBER_FORM          = 10,  // `0x`, `1e`, misplaced `'`
    D_LEX_ERR_NUMBER_DIGIT         = 11,  // `09`, `0b2`
    D_LEX_ERR_NUMBER_SUFFIX        = 12,  // `1lL`, `1.0u`
    D_LEX_ERR_NUMBER_RANGE         = 13,  // exceeds every integer type
    D_LEX_ERR_ESCAPE_FORM          = 14,  // `\x` with no digit, `\u{` open
    D_LEX_ERR_ESCAPE_RANGE         = 15,  // value wider than a code unit
    D_LEX_ERR_UNKNOWN_ESCAPE       = 16,  // `\q` (warning)
    D_LEX_ERR_CHARACTER_WIDTH      = 17,  // needs more than one code unit
    D_LEX_ERR_MULTICHARACTER       = 18,  // `'ab'` (warning)
    D_LEX_ERR_RESERVED_SUFFIX      = 19,  // ud-suffix without `_` (warning)
    D_LEX_ERR_WIDE_CHARACTER       = 20,  // C `u'\U0001F600'` (warning)
    D_LEX_ERR_COUNT_               = 21
};

// 1.2.2
// d_lexer
//   struct: a scanner positioned in a source.  Holds the reader and the
//   directive context, so one instance must not be used from two threads at
//   once.  The source and the dialect are borrowed and must outlive it.
struct d_lexer;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifetime
//------------------------------------------------------------------------------
/**
 * @brief Creates a scanner over a source, driven by a dialect.
 *
 * @param[in] _source   the source to scan; borrowed, not copied.
 * @param[in] _dialect  the language to scan it as; borrowed.
 * @param[in] _options  `D_LEX_OPT_*` bits.
 * @pre  `_source` and `_dialect` outlive the scanner.
 * @post The scanner is released with d_lex_destroy.
 * @return the scanner, or `NULL` if a parameter is `NULL`, the source is longer
 *         than a d_parse_span can address (4 GiB), or allocation failed.
 */
struct d_lexer*      d_lex_create(const struct d_source*      _source,
                                  const struct d_lex_dialect* _dialect,
                                  unsigned                    _options);
/**
 * @brief Releases a scanner.
 *
 * @param[in] _lexer  the scanner to release; may be `NULL`.
 * @post `_lexer` is invalid.  The source and dialect it borrowed are
 *       untouched.
 */
void                 d_lex_destroy(struct d_lexer* _lexer);
void                 d_lex_reset(struct d_lexer* _lexer,
                                 size_t          _offset);
/**
 * @brief Attaches the sink every later diagnostic is emitted into.
 *
 * @note d_lex_peek never emits: a token seen twice is reported once, when
 *       d_lex_next or d_lex_tokenize consumes it.
 *
 * @param[in,out] _lexer  the scanner.
 * @param[in]     _sink   the sink, borrowed; `NULL` detaches.
 * @pre  `_sink` outlives the scanner or is detached first.
 */
void                 d_lex_set_diagnostics(struct d_lexer*           _lexer,
                                           struct d_parse_diag_sink* _sink);

// 2.2    Scanning
//------------------------------------------------------------------------------
// d_lex_next reports the token at the scan position and advances past it,
// returning false once the source is exhausted with the kind set to
// D_TOKEN_END.  An ill-formed construct yields D_TOKEN_ERROR and a true
// return.  d_lex_peek reports the token `_ahead` positions forward without
// moving; it rescans, so a caller needing many should buffer instead.
/**
 * @brief Reports the token at the scan position and advances past it.
 *
 * @param[in,out] _lexer      the scanner to advance.
 * @param[out]    _out_token  receives the token.
 * @return `true` if a token was produced, `D_TOKEN_ERROR` included; `false` at
 *         the end of the source, with the kind set to `D_TOKEN_END`.
 */
bool                 d_lex_next(struct d_lexer* _lexer,
                                struct d_token* _out_token);
/**
 * @brief Reports the token `_ahead` positions forward, without moving.
 *
 * @note Rescans from the position, so repeated deep peeks are quadratic.
 *
 * @param[in]  _lexer      the scanner to read through; not advanced.
 * @param[in]  _ahead      how many tokens forward; zero is the next.
 * @param[out] _out_token  receives the token.
 * @return `true` if a token was produced, `false` if the source ended first.
 */
bool                 d_lex_peek(struct d_lexer* _lexer,
                                size_t          _ahead,
                                struct d_token* _out_token);
/**
 * @brief Fills an array with as many tokens as it holds.
 *
 * @param[in,out] _lexer       the scanner to advance.
 * @param[out]    _out_tokens  the array to fill.
 * @param[in]     _capacity    how many tokens it holds.
 * @return the number written; fewer than `_capacity` means the source ended.
 */
size_t               d_lex_tokenize(struct d_lexer* _lexer,
                                    struct d_token* _out_tokens,
                                    size_t          _capacity);
size_t               d_lex_offset(const struct d_lexer* _lexer);
// what a scanner was built over: both borrowed, both NULL for a NULL scanner
const struct d_lex_dialect*
                     d_lex_dialect_of(const struct d_lexer* _lexer);
const struct d_source*
                     d_lex_source_of(const struct d_lexer* _lexer);

// 2.3    Directive context
//------------------------------------------------------------------------------
// `<` opens a header name in an inclusion directive and is a comparison
// everywhere else, so the scanner tracks whether it stands in a directive and
// whether that directive takes a header name.  Tracking is automatic unless
// D_LEX_OPT_MANUAL_HEADER was given, which hands the caller the header
// context and nothing else: directive tracking, d_lex_in_directive and the
// directive token flags continue regardless.  Either way the setting clears
// at the newline that ends the directive.
bool                 d_lex_in_directive(const struct d_lexer* _lexer);
bool                 d_lex_header_context(const struct d_lexer* _lexer);
void                 d_lex_set_header_context(struct d_lexer* _lexer,
                                              bool            _enabled);

// 2.4    Spelling
//------------------------------------------------------------------------------
// d_token_spell writes the phase-3 spelling of a token, splices and trigraphs
// resolved, and returns the byte count the spelling needs excluding the
// terminator, in the manner of snprintf.  A token carrying none of the
// rewriting flags is spelled by its byte range alone, which is the common case
// and is worth checking before calling.
/**
 * @brief Writes a token's phase-3 spelling, splices and trigraphs resolved.
 *
 * @note A raw string is spelled physically, since its contents are.
 *
 * @param[in]  _lexer       the scanner the token came from.
 * @param[in]  _token       the token to spell.
 * @param[out] _out_buffer  receives the spelling; may be `NULL` when `_size` is
 *                          zero.
 * @param[in]  _size        the buffer's size in bytes.
 * @return the byte count the spelling needs, terminator excluded; a return of
 *         `_size` or more means the result was truncated.
 */
size_t               d_token_spell(const struct d_lexer* _lexer,
                                   const struct d_token* _token,
                                   char*                 _out_buffer,
                                   size_t                _size);

// 2.5    Diagnostics
//------------------------------------------------------------------------------
// the message a d_lex_error is emitted with, and its severity, a
// d_parse_severity value; unknown codes answer "unrecognized code" and error
const char*          d_lex_error_text(int _code);
int                  d_lex_error_severity(int _code);


#endif  // DJINTERP_PARSE_LEX_LEX_SCAN_H
