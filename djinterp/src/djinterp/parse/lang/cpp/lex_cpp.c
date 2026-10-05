/*******************************************************************************
* djinterp [parse]                                                     lex_cpp.c
*
* Definitions for the non-inline declarations in lex_cpp.h.
*   The keyword table is token_shared.def read with its C++ column, plus
* token_cpp.def, plus the eleven alternative tokens.  Those eleven are the
* only rows in either language written out rather than generated, because
* they are keyword spellings of punctuator kinds and so belong to neither
* list on its own.
*   Trigraphs are the one feature C++ removed.  C++17's mask is therefore
* not a superset of C++14's, which is why the masks are written out and not
* derived from the level before them -- a derivation would keep trigraphs
* alive in C++17 and nothing would notice until a `??/` turned up in a
* string.
*
*
* path:      /src/djinterp/parse/lang/cpp/lex_cpp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../../inc/djinterp/parse/lang/cpp/lex_cpp.h"  // corresponding header
// std
#include <stddef.h>  // NULL, size_t


//==============================================================================
// 1.  TABLES
//==============================================================================


// 1.1    Keywords
//------------------------------------------------------------------------------
// The shared list read with its C++ column, the C++-only list, and then the
// alternative tokens, whose kinds are punctuators.  A row whose C++ column is
// zero is emitted and never matched, which is how a spelling C has and C++
// does not costs a row rather than a second list.
static const struct d_lex_keyword d_internal_cpp_keywords[] =
{
#define D_TOKEN_KEYWORD(_name, _spelling, _c, _cpp) \
    { _spelling, D_TOKEN_##_name, _cpp },
#include "../../../../../inc/djinterp/parse/lex/token_shared.def"
#undef D_TOKEN_KEYWORD
#define D_TOKEN_KEYWORD(_name, _spelling, _cpp) \
    { _spelling, D_TOKEN_##_name, _cpp },
#include "../../../../../inc/djinterp/parse/lex/token_cpp.def"
#undef D_TOKEN_KEYWORD

    // the alternative tokens: keyword spellings of punctuator kinds
    { "and",    D_TOKEN_AND,           D_LEX_CPP98 },
    { "and_eq", D_TOKEN_AMP_ASSIGN,    D_LEX_CPP98 },
    { "bitand", D_TOKEN_AMP,           D_LEX_CPP98 },
    { "bitor",  D_TOKEN_PIPE,          D_LEX_CPP98 },
    { "compl",  D_TOKEN_TILDE,         D_LEX_CPP98 },
    { "not",    D_TOKEN_NOT,           D_LEX_CPP98 },
    { "not_eq", D_TOKEN_NOT_EQUAL,     D_LEX_CPP98 },
    { "or",     D_TOKEN_OR,            D_LEX_CPP98 },
    { "or_eq",  D_TOKEN_PIPE_ASSIGN,   D_LEX_CPP98 },
    { "xor",    D_TOKEN_CARET,         D_LEX_CPP98 },
    { "xor_eq", D_TOKEN_CARET_ASSIGN,  D_LEX_CPP98 }
};

// 1.2    Operators
//------------------------------------------------------------------------------
static const struct d_lex_operator d_internal_cpp_operators[] =
{
#define D_TOKEN_PUNCT(_name, _spelling) \
    { _spelling, D_TOKEN_##_name, D_LEX_CPP98, D_LEX_ROW_NONE },
#include "../../../../../inc/djinterp/parse/lex/token_shared.def"
#undef D_TOKEN_PUNCT

    // the four C++ has and C does not; `<=>` arrived last
    { "::",   D_TOKEN_SCOPE,          D_LEX_CPP98, D_LEX_ROW_NONE },
    { ".*",   D_TOKEN_DOT_STAR,       D_LEX_CPP98, D_LEX_ROW_NONE },
    { "->*",  D_TOKEN_ARROW_STAR,     D_LEX_CPP98, D_LEX_ROW_NONE },
    { "<=>",  D_TOKEN_COMPARE,        D_LEX_CPP20, D_LEX_ROW_NONE },

    // the digraphs, which are further spellings of kinds already above
    { "<%",   D_TOKEN_BRACE_OPEN,     D_LEX_CPP98, D_LEX_ROW_DIGRAPH },
    { "%>",   D_TOKEN_BRACE_CLOSE,    D_LEX_CPP98, D_LEX_ROW_DIGRAPH },
    { "<:",   D_TOKEN_BRACKET_OPEN,   D_LEX_CPP98, D_LEX_ROW_DIGRAPH },
    { ":>",   D_TOKEN_BRACKET_CLOSE,  D_LEX_CPP98, D_LEX_ROW_DIGRAPH },
    { "%:",   D_TOKEN_HASH,           D_LEX_CPP98, D_LEX_ROW_DIGRAPH },
    { "%:%:", D_TOKEN_HASH_HASH,      D_LEX_CPP98, D_LEX_ROW_DIGRAPH }
};

// 1.3    Literal prefixes
//------------------------------------------------------------------------------
// `u8` appears twice because it prefixes a string from C++11 and a character
// only from C++17, and the lookup takes the later row at or below the level.
// The `R` forms carry D_LEX_ROW_RAW, which is what sends the scanner down the
// one branch in the engine that a table cannot describe.
static const struct d_lex_prefix d_internal_cpp_prefixes[] =
{
    { "",    D_LEX_CPP98, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "L",   D_LEX_CPP98, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "u",   D_LEX_CPP11, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "U",   D_LEX_CPP11, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "u8",  D_LEX_CPP11, D_LEX_ROW_STRING },
    { "u8",  D_LEX_CPP17, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "R",   D_LEX_CPP11, D_LEX_ROW_STRING | D_LEX_ROW_RAW },
    { "LR",  D_LEX_CPP11, D_LEX_ROW_STRING | D_LEX_ROW_RAW },
    { "uR",  D_LEX_CPP11, D_LEX_ROW_STRING | D_LEX_ROW_RAW },
    { "UR",  D_LEX_CPP11, D_LEX_ROW_STRING | D_LEX_ROW_RAW },
    { "u8R", D_LEX_CPP11, D_LEX_ROW_STRING | D_LEX_ROW_RAW }
};


//==============================================================================
// 2.  DESCRIPTORS
//==============================================================================


#define D_INTERNAL_CPP_COUNTS                                              \
    d_internal_cpp_keywords,                                               \
    sizeof(d_internal_cpp_keywords) / sizeof(d_internal_cpp_keywords[0]),  \
    d_internal_cpp_operators,                                              \
    sizeof(d_internal_cpp_operators) / sizeof(d_internal_cpp_operators[0]),\
    d_internal_cpp_prefixes,                                               \
    sizeof(d_internal_cpp_prefixes) / sizeof(d_internal_cpp_prefixes[0])

// C++98 already had universal character names.  Every C++ level reads names
// by UAX #31, NFC included: P1949 was adopted for C++23 as a defect report
// against the earlier standards, and both major compilers apply it to every
// level.
#define D_INTERNAL_CPP98_FEATURES                                  \
    (D_LEX_FEATURE_TRIGRAPH        | D_LEX_FEATURE_DIGRAPH       | \
     D_LEX_FEATURE_LINE_COMMENT    | D_LEX_FEATURE_ALT_TOKEN     | \
     D_LEX_FEATURE_HEADER_NAME     | D_LEX_FEATURE_UCN           | \
     D_LEX_FEATURE_UTF8_IDENTIFIER | D_LEX_FEATURE_IDENTIFIER_NFC| \
     D_LEX_FEATURE_UCN_ANY_LITERAL)

#define D_INTERNAL_CPP11_FEATURES                                  \
    (D_INTERNAL_CPP98_FEATURES                                     | \
     D_LEX_FEATURE_RAW_STRING      | D_LEX_FEATURE_USER_SUFFIX     | \
     D_LEX_FEATURE_LONG_LONG)

#define D_INTERNAL_CPP14_FEATURES                                  \
    (D_INTERNAL_CPP11_FEATURES                                     | \
     D_LEX_FEATURE_DIGIT_SEPARATOR | D_LEX_FEATURE_BINARY_LITERAL)

// C++17 removed trigraphs, so this mask is written by subtraction and is the
// only one that is not a superset of the mask before it.  It added
// hexadecimal floating literals.
#define D_INTERNAL_CPP17_FEATURES                                  \
    ((D_INTERNAL_CPP14_FEATURES & ~((unsigned)D_LEX_FEATURE_TRIGRAPH)) | \
     D_LEX_FEATURE_HEX_FLOAT)

#define D_INTERNAL_CPP23_FEATURES                                  \
    (D_INTERNAL_CPP17_FEATURES                                     | \
     D_LEX_FEATURE_DELIMITED_ESCAPE | D_LEX_FEATURE_NAMED_ESCAPE   | \
     D_LEX_FEATURE_SIZE_SUFFIX      | D_LEX_FEATURE_EXTENDED_FLOAT)

static const struct d_lex_dialect d_internal_cpp98 =
{
    "c++", "c++98", D_LEX_CPP98,
    D_INTERNAL_CPP98_FEATURES,
    D_LEX_IDSET_XID,
    D_INTERNAL_CPP_COUNTS
};

static const struct d_lex_dialect d_internal_cpp11 =
{
    "c++", "c++11", D_LEX_CPP11,
    D_INTERNAL_CPP11_FEATURES,
    D_LEX_IDSET_XID,
    D_INTERNAL_CPP_COUNTS
};

static const struct d_lex_dialect d_internal_cpp14 =
{
    "c++", "c++14", D_LEX_CPP14,
    D_INTERNAL_CPP14_FEATURES,
    D_LEX_IDSET_XID,
    D_INTERNAL_CPP_COUNTS
};

static const struct d_lex_dialect d_internal_cpp17 =
{
    "c++", "c++17", D_LEX_CPP17,
    D_INTERNAL_CPP17_FEATURES,
    D_LEX_IDSET_XID,
    D_INTERNAL_CPP_COUNTS
};

static const struct d_lex_dialect d_internal_cpp20 =
{
    "c++", "c++20", D_LEX_CPP20,
    D_INTERNAL_CPP17_FEATURES,
    D_LEX_IDSET_XID,
    D_INTERNAL_CPP_COUNTS
};

static const struct d_lex_dialect d_internal_cpp23 =
{
    "c++", "c++23", D_LEX_CPP23,
    D_INTERNAL_CPP23_FEATURES,
    D_LEX_IDSET_XID,
    D_INTERNAL_CPP_COUNTS
};


/*
d_lex_cpp_default_level
  Reports the level used when none is named.
*/
unsigned
d_lex_cpp_default_level(
    void
)
{
    return D_LEX_CPP23;
}


/*
d_lex_cpp_dialect
  Selects the descriptor for a standard level.
*/
const struct d_lex_dialect*
d_lex_cpp_dialect(
    unsigned _level
)
{
    switch (_level)
    {
        case D_LEX_CPP98: return &d_internal_cpp98;
        case D_LEX_CPP11: return &d_internal_cpp11;
        case D_LEX_CPP14: return &d_internal_cpp14;
        case D_LEX_CPP17: return &d_internal_cpp17;
        case D_LEX_CPP20: return &d_internal_cpp20;
        default:          return &d_internal_cpp23;
    }
}


/*
d_lex_cpp_create
  Builds a scanner over a source using the C++ descriptor for a level.
*/
struct d_lexer*
d_lex_cpp_create(
    const struct d_source* _source,
    unsigned               _level,
    unsigned               _options
)
{
    const unsigned level = (_level == 0u) ? d_lex_cpp_default_level() : _level;

    return d_lex_create(_source, d_lex_cpp_dialect(level), _options);
}
