/*******************************************************************************
* djinterp [parse]                                                   lex_token.h
*
* The token record, its flag vocabulary, and the token-kind value space.
*   Kinds live in one enumeration split into three disjoint ranges: the
* kinds both languages share, beginning at zero; the kinds only C has,
* beginning at D_TOKEN_C_FIRST; and the kinds only C++ has, beginning at
* D_TOKEN_CPP_FIRST.  Because the ranges do not overlap, a value identifies a
* kind without reference to the language that produced it, which is what lets
* one consumer read tokens from both scanners.
*   A shared kind is reachable under three spellings -- D_TOKEN_IDENTIFIER,
* D_TOKEN_C_IDENTIFIER and D_TOKEN_CPP_IDENTIFIER -- which name one value.
* The language-tagged aliases exist so that code written against one language
* reads consistently, not because the values differ.  Only the shared list has
* all three; a C-only kind has the neutral and the C spelling, and a C++-only
* kind the neutral and the C++ spelling.
*   A token is an offset and a length into the source the scanner was given,
* so scanning allocates nothing per token and the source is never copied.  A
* token whose spelling was rewritten in an earlier phase -- a line splice, a
* trigraph -- covers the rewriting in its byte range and carries a flag saying
* so; the byte range is then its extent, not its spelling.
*
*
* path:      /inc/djinterp/parse/lex/lex_token.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  Standard levels
         2.  Range bases
         3.  Token flags
    2.  Types
         1.  d_token_kind
         2.  d_token
2.  OPERATIONS
    ----------
    1.  Kind inspection
    2.  Class predicates
*/

#ifndef DJINTERP_PARSE_LEX_LEX_TOKEN_H
#define DJINTERP_PARSE_LEX_LEX_TOKEN_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../c/diagnostic.h"  // d_parse_span
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================
// The standard-level scale both languages are measured on, the bases of the
// three kind ranges, the token flags, the kind enumeration, and the token.


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// Standard levels
//   constant: the publication year of each standard, used as an ordered scale.
// A year is chosen over an enumeration so that one scale serves both
// languages: a table row says a keyword arrived in 2011 without having to say
// which committee's 2011, because the row already sits in a per-language
// table.  D_LEX_NEVER is zero, which orders below every real level and so is
// rejected by the same comparison that rejects a keyword from a later
// standard.
#define D_LEX_NEVER   0u
#define D_LEX_C89     1989u
#define D_LEX_C99     1999u
#define D_LEX_C11     2011u
#define D_LEX_C17     2017u
#define D_LEX_C23     2023u
#define D_LEX_CPP98   1998u
#define D_LEX_CPP11   2011u
#define D_LEX_CPP14   2014u
#define D_LEX_CPP17   2017u
#define D_LEX_CPP20   2020u
#define D_LEX_CPP23   2023u

// 1.1.2
// Range bases
//   constant: where each language's private kinds begin.  The shared list is
// under a hundred entries and the bases are far above it, so the gap absorbs
// every shared kind a future standard adds without renumbering anything a
// consumer may have stored.  D_TOKEN_SHARED_COUNT is checked against
// D_TOKEN_C_FIRST at compile time in lex_token.c.
#define D_TOKEN_C_FIRST    256
#define D_TOKEN_CPP_FIRST  512

// 1.1.3
// Token flags
//   constant: bits in the `flags` member of d_token.  LINE_START and
// LEADING_SPACE are what a preprocessor needs in order to recognize a
// directive and to requote a macro argument.  SPLICED, TRIGRAPH and UCN mark a
// token whose byte range is wider than its spelling, so a consumer that wants
// the spelling must call d_token_spell rather than read the range; a token
// with none of them set is spelled by its range alone, which is the common
// case and costs nothing to check.
//     LINE_START means no token other than a comment precedes this one on its
// line.  A comment is whitespace to the grammar, so a comment and the `#`
// after it may both carry the flag; that is what makes `/* x */ #define`
// a directive whether or not comments are being reported.
//     The directive flags answer questions phase 7 cannot.  A directive line
// is consumed in phase 4 and never reaches phase 7, so its name is not a
// keyword even where it is spelled like one -- `if` in `#if` -- and the
// identifier a `#define`, `#undef`, `#ifdef`, `#ifndef` or `defined` names is
// an identifier by definition, even `private`.  Neither is keyword-resolved,
// and each is flagged so that a rule about macro names can find them.
#define D_TOKEN_FLAG_NONE           0x0000u
#define D_TOKEN_FLAG_LINE_START     0x0001u  // no non-comment token before
#define D_TOKEN_FLAG_LEADING_SPACE  0x0002u  // whitespace or comment before it
#define D_TOKEN_FLAG_SPLICED        0x0004u  // spans a backslash-newline
#define D_TOKEN_FLAG_TRIGRAPH       0x0008u  // contains a trigraph
#define D_TOKEN_FLAG_DIGRAPH        0x0010u  // spelled as an alternative token
#define D_TOKEN_FLAG_UCN            0x0020u  // contains a `\u` or `\U` escape
#define D_TOKEN_FLAG_EXTENDED       0x0040u  // contains non-ASCII UTF-8
#define D_TOKEN_FLAG_USER_SUFFIX    0x0080u  // literal carries a ud-suffix
#define D_TOKEN_FLAG_UNTERMINATED   0x0100u  // literal or comment ran to EOF
#define D_TOKEN_FLAG_IN_DIRECTIVE   0x0200u  // on a `#` line, the `#` included
#define D_TOKEN_FLAG_DIRECTIVE_NAME 0x0400u  // `if` in `#if`, `define`, ...
#define D_TOKEN_FLAG_MACRO_NAME     0x0800u  // defined, undefined or tested
#define D_TOKEN_FLAG_COMMENT_LINE   0x1000u  // `//`; a comment without is `/*`
#define D_TOKEN_FLAG_COMMENT_DOC    0x2000u  // `/**`, `/*!`, `///`, `//!`

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_token_kind
//   enum: what a token is.  Generated from the three list files, so a kind
//   cannot exist in the enumeration without existing in the tables that give
//   it a spelling.  The neutral names are declared first and the tagged
//   aliases after, which is why every alias is an assignment rather than an
//   ordinary member: the aliases add names, never values.
enum d_token_kind
{
    // ---- shared, from token_shared.def --------------------------------------
#define D_TOKEN_CLASS(_name, _label)                 D_TOKEN_##_name,
#define D_TOKEN_PUNCT(_name, _spelling)              D_TOKEN_##_name,
#define D_TOKEN_KEYWORD(_name, _spelling, _c, _cpp)  D_TOKEN_##_name,
#include "./token_shared.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD

    D_TOKEN_SHARED_COUNT,

    // ---- C only, from token_c.def -------------------------------------------
    D_TOKEN_C_RANGE_BASE_ = D_TOKEN_C_FIRST - 1,
#define D_TOKEN_CLASS(_name, _label)                 D_TOKEN_##_name,
#define D_TOKEN_PUNCT(_name, _spelling)              D_TOKEN_##_name,
#define D_TOKEN_KEYWORD(_name, _spelling, _c)        D_TOKEN_##_name,
#include "./token_c.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD

    D_TOKEN_C_LAST_,

    // ---- C++ only, from token_cpp.def ---------------------------------------
    D_TOKEN_CPP_RANGE_BASE_ = D_TOKEN_CPP_FIRST - 1,
#define D_TOKEN_CLASS(_name, _label)                 D_TOKEN_##_name,
#define D_TOKEN_PUNCT(_name, _spelling)              D_TOKEN_##_name,
#define D_TOKEN_KEYWORD(_name, _spelling, _cpp)      D_TOKEN_##_name,
#include "./token_cpp.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD

    D_TOKEN_CPP_LAST_
};

// The language-tagged spellings.  Every shared kind gains both tags; a kind
// private to one language gains only its own.  These are further names for
// values that already exist, which is why they are declared apart from
// d_token_kind: as members of it they would be duplicate enumerators.
//     All of them share one enumeration rather than one per language.  C
// permits two enumerators of the same type to carry the same value and
// forbids only a repeated name, so this is well formed -- and it is what lets
// D_TOKEN_C_IDENTIFIER be compared with D_TOKEN_CPP_IDENTIFIER, which is a
// reasonable thing to write and which two enumerations would warn about.
#define D_TOKEN_ALIAS_(_tag, _name)  D_TOKEN_##_tag##_##_name = D_TOKEN_##_name,

enum d_token_kind_tagged
{
    // ---- the C spellings ----------------------------------------------------
#define D_TOKEN_CLASS(_name, _label)                 D_TOKEN_ALIAS_(C, _name)
#define D_TOKEN_PUNCT(_name, _spelling)              D_TOKEN_ALIAS_(C, _name)
#define D_TOKEN_KEYWORD(_name, _spelling, _c, _cpp)  D_TOKEN_ALIAS_(C, _name)
#include "./token_shared.def"
#undef D_TOKEN_KEYWORD
#define D_TOKEN_KEYWORD(_name, _spelling, _c)        D_TOKEN_ALIAS_(C, _name)
#include "./token_c.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD

    // ---- the C++ spellings --------------------------------------------------
#define D_TOKEN_CLASS(_name, _label)                 D_TOKEN_ALIAS_(CPP, _name)
#define D_TOKEN_PUNCT(_name, _spelling)              D_TOKEN_ALIAS_(CPP, _name)
#define D_TOKEN_KEYWORD(_name, _spelling, _c, _cpp)  D_TOKEN_ALIAS_(CPP, _name)
#include "./token_shared.def"
#undef D_TOKEN_KEYWORD
#define D_TOKEN_KEYWORD(_name, _spelling, _cpp)      D_TOKEN_ALIAS_(CPP, _name)
#include "./token_cpp.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD

    D_TOKEN_TAGGED_COUNT_
};

// 1.2.2
// d_token
//   struct: one located token.  `span` is the substrate's position type, so a
//   token and a diagnostic about it name a place the same way: `offset` and
//   `length` delimit the token in bytes of the physical source, `line` and
//   `column` address its first character, and `source` is the d_source's
//   `id`.  `end_line` and `end_column` address the position just past its
//   last character -- half-open, like the byte range -- so a block comment or
//   a raw string that crosses lines has its full extent without a rescan.
//   Columns are one-based and count code points, as the style guide measures
//   a line.  `kind` is an int rather than the enumeration so that the three
//   tagged spellings assign to it without a cast and without the
//   implementation-defined choice of underlying type entering the ABI.
struct d_token
{
    int                  kind;        // a d_token_kind value
    uint32_t             flags;       // D_TOKEN_FLAG_* bits
    struct d_parse_span  span;        // where the token stands
    uint32_t             end_line;    // line just past the last character
    uint32_t             end_column;  // column just past the last character
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Kind inspection is language-neutral: it reads the generated tables and so
// answers for a kind from either scanner.


// 2.1    Kind inspection
//------------------------------------------------------------------------------
// d_token_spelling returns the primary written form of a punctuator or
// keyword, or NULL for a class kind, which has no fixed spelling.
// d_token_kind_name returns the enumerator's own name, for diagnostics and for
// any consumer needing a stable spelling of a kind.
const char*  d_token_spelling(int _kind);
const char*  d_token_kind_name(int _kind);
const char*  d_token_class_label(int _kind);

// 2.2    Class predicates
//------------------------------------------------------------------------------
// Each of these is a range test or a table read, not a switch, so adding a
// kind to a list file does not require editing a predicate.
bool         d_token_is_shared(int _kind);
bool         d_token_is_c_only(int _kind);
bool         d_token_is_cpp_only(int _kind);
bool         d_token_is_keyword(int _kind);
bool         d_token_is_punctuator(int _kind);
bool         d_token_is_literal(int _kind);


#endif  // DJINTERP_PARSE_LEX_LEX_TOKEN_H
