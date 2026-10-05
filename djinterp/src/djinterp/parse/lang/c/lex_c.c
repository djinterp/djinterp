/*******************************************************************************
* djinterp [parse]                                                       lex_c.c
*
* Definitions for the non-inline declarations in lex_c.h.
*   The keyword table is token_shared.def read with its C column plus
* token_c.def, and nothing else.  A shared row whose C column is zero is
* emitted and never matched, which is how a spelling C++ has and C does not
* costs a row rather than a second list.
*   The operator table is the shared punctuators plus the six digraphs.  A
* digraph is not a kind: `<%` is another way to write `{`, so its row carries
* the brace's kind and a flag, and a consumer counting braces finds it
* without knowing digraphs exist.
*   Six descriptors differ only in a level and a feature mask.  Trigraphs
* are the one feature C removed rather than added, so C23's mask is the only
* one that is not a superset of the mask before it.
*
*
* path:      /src/djinterp/parse/lang/c/lex_c.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../../inc/djinterp/parse/lang/c/lex_c.h"  // corresponding header
// std
#include <stddef.h>  // NULL, size_t


//==============================================================================
// 1.  TABLES
//==============================================================================
// Generated from the list files.  Nothing in this section is written out; a
// spelling added to a list file appears here without an edit.


// 1.1    Keywords
//------------------------------------------------------------------------------
static const struct d_lex_keyword d_internal_c_keywords[] =
{
#define D_TOKEN_KEYWORD(_name, _spelling, _c, _cpp) \
    { _spelling, D_TOKEN_##_name, _c },
#include "../../../../../inc/djinterp/parse/lex/token_shared.def"
#undef D_TOKEN_KEYWORD
#define D_TOKEN_KEYWORD(_name, _spelling, _c) \
    { _spelling, D_TOKEN_##_name, _c },
#include "../../../../../inc/djinterp/parse/lex/token_c.def"
#undef D_TOKEN_KEYWORD
};

// 1.2    Operators
//------------------------------------------------------------------------------
static const struct d_lex_operator d_internal_c_operators[] =
{
#define D_TOKEN_PUNCT(_name, _spelling) \
    { _spelling, D_TOKEN_##_name, D_LEX_C89, D_LEX_ROW_NONE },
#include "../../../../../inc/djinterp/parse/lex/token_shared.def"
#undef D_TOKEN_PUNCT

    // the digraphs, which are further spellings of kinds already above
    { "<%",   D_TOKEN_BRACE_OPEN,     D_LEX_C99, D_LEX_ROW_DIGRAPH },
    { "%>",   D_TOKEN_BRACE_CLOSE,    D_LEX_C99, D_LEX_ROW_DIGRAPH },
    { "<:",   D_TOKEN_BRACKET_OPEN,   D_LEX_C99, D_LEX_ROW_DIGRAPH },
    { ":>",   D_TOKEN_BRACKET_CLOSE,  D_LEX_C99, D_LEX_ROW_DIGRAPH },
    { "%:",   D_TOKEN_HASH,           D_LEX_C99, D_LEX_ROW_DIGRAPH },
    { "%:%:", D_TOKEN_HASH_HASH,      D_LEX_C99, D_LEX_ROW_DIGRAPH }
};

// 1.3    Literal prefixes
//------------------------------------------------------------------------------
// `u8` appears twice on purpose: it prefixes a string from C11 and a
// character only from C23, and the lookup takes the later row at or below the
// level, so the character role is added without the string role being lost.
static const struct d_lex_prefix d_internal_c_prefixes[] =
{
    { "",   D_LEX_C89, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "L",  D_LEX_C89, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "u",  D_LEX_C11, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "U",  D_LEX_C11, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING },
    { "u8", D_LEX_C11, D_LEX_ROW_STRING },
    { "u8", D_LEX_C23, D_LEX_ROW_CHARACTER | D_LEX_ROW_STRING }
};


//==============================================================================
// 2.  DESCRIPTORS
//==============================================================================
// One per standard level.  The masks are written out rather than derived,
// because C23 removed trigraphs and a derivation that assumed each level
// extends the one before it would silently keep them.


#define D_INTERNAL_C_COUNTS                                            \
    d_internal_c_keywords,                                             \
    sizeof(d_internal_c_keywords) / sizeof(d_internal_c_keywords[0]),  \
    d_internal_c_operators,                                            \
    sizeof(d_internal_c_operators) / sizeof(d_internal_c_operators[0]),\
    d_internal_c_prefixes,                                             \
    sizeof(d_internal_c_prefixes) / sizeof(d_internal_c_prefixes[0])

static const struct d_lex_dialect d_internal_c89 =
{
    "c", "c89", D_LEX_C89,
    D_LEX_FEATURE_TRIGRAPH | D_LEX_FEATURE_HEADER_NAME,
    D_LEX_IDSET_NONE,
    D_INTERNAL_C_COUNTS
};

// C99 added universal character names and, with them, the Annex D table of
// characters a name may hold.  A UTF-8 spelling of such a character is the
// same character, which is how both major compilers read it.
static const struct d_lex_dialect d_internal_c99 =
{
    "c", "c99", D_LEX_C99,
    D_LEX_FEATURE_TRIGRAPH        | D_LEX_FEATURE_DIGRAPH       |
    D_LEX_FEATURE_LINE_COMMENT    | D_LEX_FEATURE_UCN           |
    D_LEX_FEATURE_UTF8_IDENTIFIER | D_LEX_FEATURE_HEADER_NAME   |
    D_LEX_FEATURE_HEX_FLOAT       | D_LEX_FEATURE_LONG_LONG,
    D_LEX_IDSET_C99,
    D_INTERNAL_C_COUNTS
};

static const struct d_lex_dialect d_internal_c11 =
{
    "c", "c11", D_LEX_C11,
    D_LEX_FEATURE_TRIGRAPH        | D_LEX_FEATURE_DIGRAPH       |
    D_LEX_FEATURE_LINE_COMMENT    | D_LEX_FEATURE_UCN           |
    D_LEX_FEATURE_UTF8_IDENTIFIER | D_LEX_FEATURE_HEADER_NAME   |
    D_LEX_FEATURE_HEX_FLOAT       | D_LEX_FEATURE_LONG_LONG,
    D_LEX_IDSET_C11,
    D_INTERNAL_C_COUNTS
};

static const struct d_lex_dialect d_internal_c17 =
{
    "c", "c17", D_LEX_C17,
    D_LEX_FEATURE_TRIGRAPH        | D_LEX_FEATURE_DIGRAPH       |
    D_LEX_FEATURE_LINE_COMMENT    | D_LEX_FEATURE_UCN           |
    D_LEX_FEATURE_UTF8_IDENTIFIER | D_LEX_FEATURE_HEADER_NAME   |
    D_LEX_FEATURE_HEX_FLOAT       | D_LEX_FEATURE_LONG_LONG,
    D_LEX_IDSET_C11,
    D_INTERNAL_C_COUNTS
};

// C23 is the only level whose mask is not a superset of the one before it:
// trigraphs were removed, not added to.  It also replaced the Annex D tables
// with UAX #31 and its NFC requirement.
static const struct d_lex_dialect d_internal_c23 =
{
    "c", "c23", D_LEX_C23,
    D_LEX_FEATURE_DIGRAPH         | D_LEX_FEATURE_LINE_COMMENT    |
    D_LEX_FEATURE_UCN             | D_LEX_FEATURE_HEADER_NAME     |
    D_LEX_FEATURE_BINARY_LITERAL  | D_LEX_FEATURE_DIGIT_SEPARATOR |
    D_LEX_FEATURE_UTF8_IDENTIFIER | D_LEX_FEATURE_HEX_FLOAT       |
    D_LEX_FEATURE_LONG_LONG       | D_LEX_FEATURE_BIT_PRECISE     |
    D_LEX_FEATURE_DECIMAL_FLOAT   | D_LEX_FEATURE_EXTENDED_FLOAT  |
    D_LEX_FEATURE_IDENTIFIER_NFC,
    D_LEX_IDSET_XID,
    D_INTERNAL_C_COUNTS
};


/*
d_lex_c_default_level
  Reports the level used when none is named.
*/
unsigned
d_lex_c_default_level(
    void
)
{
    return D_LEX_C23;
}


/*
d_lex_c_dialect
  Selects the descriptor for a standard level.
  An unrecognized level is answered with the newest supported rather than
with NULL, because a scanner that refuses to start is a worse diagnostic
than one that reports the keyword it actually found.
*/
const struct d_lex_dialect*
d_lex_c_dialect(
    unsigned _level
)
{
    switch (_level)
    {
        case D_LEX_C89: return &d_internal_c89;
        case D_LEX_C99: return &d_internal_c99;
        case D_LEX_C11: return &d_internal_c11;
        case D_LEX_C17: return &d_internal_c17;
        default:        return &d_internal_c23;
    }
}


/*
d_lex_c_create
  Builds a scanner over a source using the C descriptor for a level.
*/
struct d_lexer*
d_lex_c_create(
    const struct d_source* _source,
    unsigned               _level,
    unsigned               _options
)
{
    const unsigned level = (_level == 0u) ? d_lex_c_default_level() : _level;

    return d_lex_create(_source, d_lex_c_dialect(level), _options);
}
