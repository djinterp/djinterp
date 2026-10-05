/*******************************************************************************
* djinterp [parse]                                                 lex_dialect.h
*
* The description of a language that the scanner is driven by.
*   A dialect is data.  It names the standard level to scan at, a feature
* mask, and three tables -- keywords, operators, literal prefixes -- and the
* scanner in lex_scan.h consults nothing else about the language.  This is
* what keeps C and C++ from duplicating a scanner: there is one engine and two
* descriptors, and the descriptors are generated from the list files in this
* directory rather than written out.
*   Every table row carries the standard that introduced it, compared
* against the dialect's own level, so one table serves every level of a
* language.  Scanning C99 and C23 is the same table with a different integer,
* not two tables.
*   One construct resists description by table: the raw string literal,
* whose closing sequence depends on text read at its opening.  It is reached
* through a feature bit rather than a table row and is the only place in the
* engine where a branch is taken on a language-shaped fact.  A second branch
* would be the point at which to introduce a hook instead.
*
*
* path:      /inc/djinterp/parse/lex/lex_dialect.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  Feature flags
         2.  Table row flags
    2.  Types
         1.  d_lex_keyword
         2.  d_lex_operator
         3.  d_lex_prefix
         4.  d_lex_dialect
2.  OPERATIONS
    ----------
    1.  Validation
    2.  Lookup
*/

#ifndef DJINTERP_PARSE_LEX_LEX_DIALECT_H
#define DJINTERP_PARSE_LEX_LEX_DIALECT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

// djinterp
#include "./lex_token.h"    // d_token_kind, standard levels
#include "./lex_unicode.h"  // D_LEX_IDSET_*


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// Feature flags
//   constant: lexical behaviour a dialect either has or does not.  These are
// facts about the language, fixed when the descriptor is built; behaviour the
// caller chooses instead -- whether to emit comments, whether to resolve
// keywords -- belongs to the option mask in lex_scan.h and is not here.
#define D_LEX_FEATURE_NONE             0x000000u
#define D_LEX_FEATURE_LINE_COMMENT     0x000001u  // `//` ends at the newline
#define D_LEX_FEATURE_TRIGRAPH         0x000002u  // `??=` and the rest
#define D_LEX_FEATURE_DIGRAPH          0x000004u  // `<%` `%>` `<:` `:>` `%:`
#define D_LEX_FEATURE_ALT_TOKEN        0x000008u  // `and`, `bitor`, `not_eq`
#define D_LEX_FEATURE_RAW_STRING       0x000010u  // `R"delim( ... )delim"`
#define D_LEX_FEATURE_DIGIT_SEPARATOR  0x000020u  // `1'000` in a pp-number
#define D_LEX_FEATURE_USER_SUFFIX      0x000040u  // `1_km`, `"s"sv`
#define D_LEX_FEATURE_UCN              0x000080u  // `\u` and `\U` in a name
#define D_LEX_FEATURE_UTF8_IDENTIFIER  0x000100u  // non-ASCII UTF-8 in a name
#define D_LEX_FEATURE_BINARY_LITERAL   0x000200u  // `0b1010`
#define D_LEX_FEATURE_DOLLAR           0x000400u  // `$` is an identifier byte
#define D_LEX_FEATURE_HEADER_NAME      0x000800u  // `<...>` after an inclusion
#define D_LEX_FEATURE_HEX_FLOAT        0x001000u  // `0x1.8p3`
#define D_LEX_FEATURE_LONG_LONG        0x002000u  // `ll` and `ull` suffixes
#define D_LEX_FEATURE_DELIMITED_ESCAPE 0x004000u  // `\u{}` `\x{}` `\o{}`
#define D_LEX_FEATURE_NAMED_ESCAPE     0x008000u  // `\N{LATIN SMALL LETTER A}`
#define D_LEX_FEATURE_SIZE_SUFFIX      0x010000u  // `z` and `uz` suffixes
#define D_LEX_FEATURE_BIT_PRECISE      0x020000u  // `wb` and `uwb` suffixes
#define D_LEX_FEATURE_DECIMAL_FLOAT    0x040000u  // `df`, `dd`, `dl` suffixes
#define D_LEX_FEATURE_EXTENDED_FLOAT   0x080000u  // `f16`, `f32`, `bf16`, ...
#define D_LEX_FEATURE_UCN_ANY_LITERAL  0x100000u  // `"\u0041"` is allowed
#define D_LEX_FEATURE_IDENTIFIER_NFC   0x200000u  // a name must be in NFC

// 1.1.2
// Table row flags
//   constant: what is unusual about one row.  DIGRAPH and ALT_TOKEN mark an
// operator row that is a second spelling of a kind some other row already
// carries, so a consumer that wants each kind once filters on them.  RAW marks
// a prefix row that opens a raw string.
// 1.1.3
// D_LEX_RAW_DELIMITER_MAX
//   constant: the longest raw string delimiter the standard permits between
// `R"` and the opening parenthesis.  The scanner must know the bound in order
// to recognize a closing sequence without unbounded lookahead.
#define D_LEX_RAW_DELIMITER_MAX 16

#define D_LEX_ROW_NONE        0x0000u
#define D_LEX_ROW_DIGRAPH     0x0001u  // `<%` for `{`, and the rest
#define D_LEX_ROW_ALT_TOKEN   0x0002u  // `and` for `&&`, and the rest
#define D_LEX_ROW_RAW         0x0004u  // prefix opens a raw string
#define D_LEX_ROW_CHARACTER   0x0008u  // prefix is valid on a character
#define D_LEX_ROW_STRING      0x0010u  // prefix is valid on a string

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_lex_keyword
//   struct: one row of a keyword table.  `since` is the standard level that
//   introduced the spelling; a row whose level exceeds the dialect's is not
//   matched, and the spelling lexes as an ordinary identifier, which is the
//   correct behaviour rather than an approximation of it.
struct d_lex_keyword
{
    const char*  text;
    int          kind;
    unsigned     since;
};

// 1.2.2
// d_lex_operator
//   struct: one row of an operator table.  The matcher makes one pass and
//   keeps the longest row that matches, so the table needs no ordering and a
//   row may be appended anywhere.  Requiring descending length would be
//   faster by a little and would fail silently when the order slipped, which
//   is the wrong trade for a table that is generated rather than written.
struct d_lex_operator
{
    const char*  text;
    int          kind;
    unsigned     since;
    unsigned     flags;
};

// 1.2.3
// d_lex_prefix
//   struct: one row of a literal-prefix table -- the empty prefix, `L`, `u`,
//   `U`, `u8`, and in C++ the `R` forms.  A prefix is not a token; it is part
//   of the literal it introduces, and the row exists so that the scanner knows
//   which identifiers may abut a quote without becoming one.
struct d_lex_prefix
{
    const char*  text;
    unsigned     since;
    unsigned     flags;
};

// 1.2.4
// d_lex_dialect
//   struct: a complete language description.  A descriptor is constant data
//   with static storage duration, shared by every scanner using that language,
//   so it carries no state and needs no lifetime management.
//     `language` and `name` are kept apart because a consumer selecting by
//   language -- a style sheet scoped to C -- must not have to parse a
//   standard's name to find it.
struct d_lex_dialect
{
    const char*                   language;        // "c", "c++"
    const char*                   name;            // "c23", "c++17"
    unsigned                      level;           // a D_LEX_C* or D_LEX_CPP*
    unsigned                      features;        // D_LEX_FEATURE_* bits
    unsigned                      identifiers;     // a D_LEX_IDSET_* value

    const struct d_lex_keyword*   keywords;
    size_t                        keyword_count;
    const struct d_lex_operator*  operators;
    size_t                        operator_count;
    const struct d_lex_prefix*    prefixes;
    size_t                        prefix_count;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Validation
//------------------------------------------------------------------------------
// d_lex_dialect_validate checks the invariants the scanner relies on and
// cannot check at every step: every table is present and non-empty, no
// operator row is empty or longer than the matcher's window, and every kind
// names a row in lex_token.h.  A descriptor built by the generators in lang/
// always passes; the call exists for one assembled by hand.
/**
 * @brief Checks the invariants a scanner relies on and cannot check itself.
 *
 * @param[in]  _dialect     the descriptor to check.
 * @param[out] _out_reason  receives the first failure; may be `NULL`.
 * @return `true` if the descriptor is usable, `false` otherwise.
 */
bool                          d_lex_dialect_validate(
                                  const struct d_lex_dialect* _dialect,
                                  const char**                _out_reason);

// 2.2    Lookup
//------------------------------------------------------------------------------
// d_lex_dialect_keyword returns the kind for a spelling available at the
// dialect's level, or D_TOKEN_IDENTIFIER when the spelling is not a keyword
// there.  A row is rejected on its length and then its first byte before any
// comparison of the text, so a miss -- which is what almost every identifier
// is -- costs one or two byte comparisons per row and no allocation.
int                           d_lex_dialect_keyword(
                                  const struct d_lex_dialect* _dialect,
                                  const char*                 _text,
                                  size_t                      _length);
const struct d_lex_operator*  d_lex_dialect_operator(
                                  const struct d_lex_dialect* _dialect,
                                  const char*                 _text,
                                  size_t                      _length);
const struct d_lex_prefix*    d_lex_dialect_prefix(
                                  const struct d_lex_dialect* _dialect,
                                  const char*                 _text,
                                  size_t                      _length);
bool                          d_lex_dialect_has(
                                  const struct d_lex_dialect* _dialect,
                                  unsigned                    _feature);


#endif  // DJINTERP_PARSE_LEX_LEX_DIALECT_H
