/*******************************************************************************
* djinterp [parse]                                                 lex_literal.h
*
* Phase 7 for literals: classify a numeric, character or string literal, check
* it against its dialect, and compute its value.
*   The scanner stops at phase 3, where a pp-number is any run of number
* characters and a character literal is anything between quotes.  Whether
* `09` is an octal literal with a bad digit, or `'\q'` an escape nobody
* defined, is only decided for a token that reaches phase 7, since a token in
* a skipped group or a stringized argument never does.  This decides it, on
* request, and reports what it finds into the same sink and domain the
* scanner uses, D_PARSE_DIAG_DOMAIN_LEX.
*   Some facts are not the language's but the target's: how wide `char`,
* `wchar_t` and `long long` are.  A d_lex_target carries them, beside the
* dialect, so a wide literal is checked as the target will store it -- 16-bit
* UTF-16 on Windows, 32-bit UTF-32 elsewhere.  The execution character set is
* taken to be UTF-8, the one both major compilers default to.
*
*
* path:      /inc/djinterp/parse/lex/lex_literal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  Categories
         2.  Encodings
         3.  Suffixes
         4.  Literal flags
         5.  Target presets
    2.  Types
         1.  d_lex_target
         2.  d_lex_literal_context
         3.  d_lex_literal
2.  OPERATIONS
    ----------
    1.  Targets
    2.  Decoding
*/

#ifndef DJINTERP_PARSE_LEX_LEX_LITERAL_H
#define DJINTERP_PARSE_LEX_LEX_LITERAL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../c/diagnostic.h"  // d_parse_diag_sink
#include "./lex_dialect.h"  // d_lex_dialect
#include "./lex_scan.h"     // d_lexer
#include "./lex_token.h"    // d_token
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// Categories
//   constant: what a literal is.  NONE is the answer for a token that is not
// a literal at all, and for an error token, which the scanner has reported.
#define D_LEX_LITERAL_NONE       0
#define D_LEX_LITERAL_INTEGER    1
#define D_LEX_LITERAL_FLOATING   2
#define D_LEX_LITERAL_CHARACTER  3
#define D_LEX_LITERAL_STRING     4

// 1.1.2
// Encodings
//   constant: a character or string literal's encoding prefix.
#define D_LEX_ENCODING_ORDINARY  0  // no prefix
#define D_LEX_ENCODING_WIDE      1  // `L`
#define D_LEX_ENCODING_UTF8      2  // `u8`
#define D_LEX_ENCODING_UTF16     3  // `u`
#define D_LEX_ENCODING_UTF32     4  // `U`

// 1.1.3
// Suffixes
//   constant: the standard suffixes, as bits.  A suffix the standard does not
// define is either a user-defined suffix, where the dialect has them, or an
// error.  EXTENDED carries its width in `float_width`; BFLOAT is `bf16`.
#define D_LEX_SUFFIX_NONE         0x0000u
#define D_LEX_SUFFIX_UNSIGNED     0x0001u  // `u`
#define D_LEX_SUFFIX_LONG         0x0002u  // `l`
#define D_LEX_SUFFIX_LONG_LONG    0x0004u  // `ll`
#define D_LEX_SUFFIX_SIZE         0x0008u  // `z`
#define D_LEX_SUFFIX_BIT_PRECISE  0x0010u  // `wb`
#define D_LEX_SUFFIX_FLOAT        0x0020u  // `f`
#define D_LEX_SUFFIX_LONG_DOUBLE  0x0040u  // `l` on a floating literal
#define D_LEX_SUFFIX_DECIMAL      0x0080u  // `df`, `dd`, `dl`, `d32`, ...
#define D_LEX_SUFFIX_EXTENDED     0x0100u  // `f16`, `f32x`, ...
#define D_LEX_SUFFIX_BFLOAT       0x0200u  // `bf16`
#define D_LEX_SUFFIX_USER         0x0400u  // a ud-suffix

// 1.1.4
// Literal flags
//   constant: facts about how a literal was written, for rules that care
// about spelling rather than value -- an octal literal, a separator, an escape.
#define D_LEX_LITERAL_FLAG_NONE       0x0000u
#define D_LEX_LITERAL_FLAG_SEPARATOR  0x0001u  // contains a digit separator
#define D_LEX_LITERAL_FLAG_OVERFLOW   0x0002u  // integer exceeds 64 bits
#define D_LEX_LITERAL_FLAG_RAW        0x0004u  // a raw string
#define D_LEX_LITERAL_FLAG_OCTAL_ESC  0x0008u  // an octal escape
#define D_LEX_LITERAL_FLAG_HEX_ESC    0x0010u  // a hexadecimal escape
#define D_LEX_LITERAL_FLAG_UCN        0x0020u  // a universal character name
#define D_LEX_LITERAL_FLAG_MULTICHAR  0x0040u  // more than one character
#define D_LEX_LITERAL_FLAG_NO_VALUE   0x0080u  // value not computed

// 1.1.5
// Target presets
//   constant: the arguments d_lex_target_preset accepts.  HOST is the machine
// the decoder was compiled for, read from <limits.h> and <wchar.h>; UNIX is
// LP64 with a 32-bit `wchar_t`; WINDOWS is LLP64 with a 16-bit one.
#define D_LEX_TARGET_HOST     0
#define D_LEX_TARGET_UNIX     1
#define D_LEX_TARGET_WINDOWS  2

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_lex_target
//   struct: what the target stores a literal in.  Widths are in bits: a code
//   unit of an ordinary or `u8` literal is `char_bits` wide, of an `L`
//   literal `wchar_bits` wide -- 16 means UTF-16, with surrogate pairs, and
//   32 means UTF-32 -- and "too large for any signed type" is measured
//   against `long_long_bits`.
struct d_lex_target
{
    unsigned  char_bits;       // 8 on every hosted target
    unsigned  wchar_bits;      // 16 or 32
    unsigned  long_long_bits;  // 64 on every current target
};

// 1.2.2
// d_lex_literal_context
//   struct: what a decode reads by and reports to.  `dialect` is required;
//   `target` NULL means the host's; `sink` NULL means diagnostics are only
//   counted, in d_lex_literal's `errors`.
struct d_lex_literal_context
{
    const struct d_lex_dialect* dialect;
    const struct d_lex_target*  target;
    struct d_parse_diag_sink*   sink;
};

// 1.2.3
// d_lex_literal
//   struct: a decoded literal.  `integer` holds an integer literal's value,
//   or a character literal's: the code unit or code point of a single
//   character, or for an ordinary multicharacter literal the value gcc and
//   clang compute, each code unit shifted in from the right.  An ordinary
//   character's value is the unsigned code unit; the int it converts to
//   depends on the target's `char` signedness.  `floating` is the nearest
//   double to a floating literal, whatever its suffix.  `characters` counts
//   the c-chars or s-chars of a character or string literal, an escape being
//   one.  Offsets are into the spelling that was decoded.
struct d_lex_literal
{
    int       category;       // a D_LEX_LITERAL_* category
    int       encoding;       // a D_LEX_ENCODING_*, for the quoted kinds
    unsigned  base;           // 2, 8, 10 or 16, for the numeric kinds
    unsigned  suffix;         // D_LEX_SUFFIX_* bits
    unsigned  float_width;    // 16, 32, 64 or 128 with an extended suffix
    uint32_t  flags;          // D_LEX_LITERAL_FLAG_* bits
    uint32_t  suffix_offset;  // where the suffix begins in the spelling
    uint32_t  suffix_length;  // zero when there is none
    uint64_t  integer;
    double    floating;
    uint32_t  characters;
    uint32_t  errors;         // error-severity diagnostics raised
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Targets
//------------------------------------------------------------------------------
// one of the D_LEX_TARGET_* presets, statically allocated; NULL for an
// unknown one
const struct d_lex_target*
             d_lex_target_preset(int _which);

// 2.2    Decoding
//------------------------------------------------------------------------------
/**
 * @brief Classifies a literal's spelling, checks it, and computes its value.
 *
 * @note The spelling is the phase-3 one: splices and trigraphs resolved, as
 *       d_token_spell writes it.  When the token carries neither rewriting
 *       flag its byte range is its spelling, and every diagnostic is placed
 *       on the offending characters; otherwise diagnostics cover the token.
 *
 * @param[in]  _context   the dialect, target and sink; see
 *                        d_lex_literal_context.
 * @param[in]  _token     the token, for its kind, flags and span.
 * @param[in]  _spelling  the token's phase-3 spelling.
 * @param[in]  _length    the spelling's length in bytes.
 * @param[out] _out       receives the decoded literal.
 * @return `true` if the literal is well-formed, `false` if an error-severity
 *         diagnostic was raised, the token is not a literal, or the context
 *         names no dialect.
 */
bool         d_lex_literal_decode(const struct d_lex_literal_context* _context,
                                  const struct d_token*               _token,
                                  const char*                         _spelling,
                                  size_t                              _length,
                                  struct d_lex_literal*               _out);
/**
 * @brief Decodes a literal token as a scanner produced it, in that scanner's
 *        dialect.
 *
 * @note Obtains the spelling itself: the source bytes when the token was not
 *       rewritten, otherwise a heap copy from d_token_spell, released before
 *       returning.
 *
 * @param[in]     _lexer   the scanner the token came from.
 * @param[in]     _target  the target; `NULL` for the host's.
 * @param[in]     _token   the token.
 * @param[in,out] _sink    receives the diagnostics; may be `NULL`.
 * @param[out]    _out     receives the decoded literal.
 * @return as d_lex_literal_decode, and `false` if the spelling could not be
 *         allocated.
 */
bool         d_lex_literal_decode_token(const struct d_lexer*      _lexer,
                                        const struct d_lex_target* _target,
                                        const struct d_token*      _token,
                                        struct d_parse_diag_sink*  _sink,
                                        struct d_lex_literal*      _out);


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_PARSE_LEX_LEX_LITERAL_H
