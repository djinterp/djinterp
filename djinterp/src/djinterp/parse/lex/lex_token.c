/*******************************************************************************
* djinterp [parse]                                                   lex_token.c
*
* Definitions for the non-inline declarations in lex_token.h.
*   Every table here is generated from the three list files, so a kind that
* exists in the enumeration necessarily has a row, and a row necessarily has a
* kind.  The static assertion below is the one place the arrangement could
* fail silently: it fails to build instead if the shared list ever grows past
* the base of the C range.
*
*
* path:      /src/djinterp/parse/lex/lex_token.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/parse/lex/lex_token.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL, size_t


//==============================================================================
// 1.  GENERATED TABLES
//==============================================================================
// One row per kind, in three arrays matching the three ranges.  `spelling` is
// the written form of a punctuator or keyword and is NULL for a class;
// `label` is the diagnostic name of a class and NULL otherwise.


// 1.1    Row shape
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_row
//   struct: one kind's names and its class.  `group` is one of the three
//   values below and is what every predicate in section 3 tests.
enum d_internal_group
{
    D_INTERNAL_GROUP_CLASS = 0,
    D_INTERNAL_GROUP_PUNCT,
    D_INTERNAL_GROUP_KEYWORD
};

struct d_internal_row
{
    const char*           name;
    const char*           spelling;
    const char*           label;
    enum d_internal_group group;
};

// 1.2    The three ranges
//------------------------------------------------------------------------------
static const struct d_internal_row d_internal_shared[] =
{
#define D_TOKEN_CLASS(_name, _label) \
    { "D_TOKEN_" #_name, NULL, _label, D_INTERNAL_GROUP_CLASS },
#define D_TOKEN_PUNCT(_name, _spelling) \
    { "D_TOKEN_" #_name, _spelling, NULL, D_INTERNAL_GROUP_PUNCT },
#define D_TOKEN_KEYWORD(_name, _spelling, _c, _cpp) \
    { "D_TOKEN_" #_name, _spelling, NULL, D_INTERNAL_GROUP_KEYWORD },
#include "../../../../inc/djinterp/parse/lex/token_shared.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD
};

static const struct d_internal_row d_internal_c_only[] =
{
#define D_TOKEN_CLASS(_name, _label) \
    { "D_TOKEN_" #_name, NULL, _label, D_INTERNAL_GROUP_CLASS },
#define D_TOKEN_PUNCT(_name, _spelling) \
    { "D_TOKEN_" #_name, _spelling, NULL, D_INTERNAL_GROUP_PUNCT },
#define D_TOKEN_KEYWORD(_name, _spelling, _c) \
    { "D_TOKEN_" #_name, _spelling, NULL, D_INTERNAL_GROUP_KEYWORD },
#include "../../../../inc/djinterp/parse/lex/token_c.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD
};

static const struct d_internal_row d_internal_cpp_only[] =
{
#define D_TOKEN_CLASS(_name, _label) \
    { "D_TOKEN_" #_name, NULL, _label, D_INTERNAL_GROUP_CLASS },
#define D_TOKEN_PUNCT(_name, _spelling) \
    { "D_TOKEN_" #_name, _spelling, NULL, D_INTERNAL_GROUP_PUNCT },
#define D_TOKEN_KEYWORD(_name, _spelling, _cpp) \
    { "D_TOKEN_" #_name, _spelling, NULL, D_INTERNAL_GROUP_KEYWORD },
#include "../../../../inc/djinterp/parse/lex/token_cpp.def"
#undef D_TOKEN_CLASS
#undef D_TOKEN_PUNCT
#undef D_TOKEN_KEYWORD
};

// The arrangement rests on each range fitting below the next.  A violation
// renumbers every stored kind, so it is caught at compile time.
D_STATIC_ASSERT(D_TOKEN_SHARED_COUNT <= D_TOKEN_C_FIRST,
                "the shared token list has grown into the C range; "
                "raise D_TOKEN_C_FIRST");
D_STATIC_ASSERT(D_TOKEN_C_LAST_ <= D_TOKEN_CPP_FIRST,
                "the C token list has grown into the C++ range; "
                "raise D_TOKEN_CPP_FIRST");


//==============================================================================
// 2.  KIND INSPECTION
//==============================================================================


/**
 * @brief Locates the row describing a kind.
 *
 * @param[in] _kind  the kind to describe.
 * @return the row, or NULL when the value names no kind.
 */
static const struct d_internal_row*
d_internal_row_for(
    int _kind
)
{
    const size_t shared_count = sizeof(d_internal_shared) /
                                sizeof(d_internal_shared[0]);
    const size_t c_count      = sizeof(d_internal_c_only) /
                                sizeof(d_internal_c_only[0]);
    const size_t cpp_count    = sizeof(d_internal_cpp_only) /
                                sizeof(d_internal_cpp_only[0]);

    // the shared range begins at zero
    if ((_kind >= 0) && ((size_t)_kind < shared_count))
    {
        return &d_internal_shared[_kind];
    }

    // the C range
    if ((_kind >= D_TOKEN_C_FIRST) &&
        ((size_t)(_kind - D_TOKEN_C_FIRST) < c_count))
    {
        return &d_internal_c_only[_kind - D_TOKEN_C_FIRST];
    }

    // the C++ range
    if ((_kind >= D_TOKEN_CPP_FIRST) &&
        ((size_t)(_kind - D_TOKEN_CPP_FIRST) < cpp_count))
    {
        return &d_internal_cpp_only[_kind - D_TOKEN_CPP_FIRST];
    }

    return NULL;
}


/*
d_token_spelling
  Reports the primary written form of a punctuator or keyword.
*/
const char*
d_token_spelling(
    int _kind
)
{
    const struct d_internal_row* const row = d_internal_row_for(_kind);

    return row ? row->spelling : NULL;
}


/*
d_token_kind_name
  Reports the enumerator's own name, for diagnostics and for any consumer that
needs a stable spelling of a kind, such as a node type in a tree built from
the tokens.
*/
const char*
d_token_kind_name(
    int _kind
)
{
    const struct d_internal_row* const row = d_internal_row_for(_kind);

    return row ? row->name : NULL;
}


/*
d_token_class_label
  Reports the diagnostic label of a class kind.
*/
const char*
d_token_class_label(
    int _kind
)
{
    const struct d_internal_row* const row = d_internal_row_for(_kind);

    return row ? row->label : NULL;
}


//==============================================================================
// 3.  CLASS PREDICATES
//==============================================================================


/*
d_token_is_shared
  Reports whether a kind is one both languages have.
*/
bool
d_token_is_shared(
    int _kind
)
{
    return ((_kind >= 0) && (_kind < D_TOKEN_SHARED_COUNT));
}


/*
d_token_is_c_only
  Reports whether a kind is one only C has.
*/
bool
d_token_is_c_only(
    int _kind
)
{
    return ((_kind >= D_TOKEN_C_FIRST) && (_kind < D_TOKEN_CPP_FIRST) &&
            (d_internal_row_for(_kind) != NULL));
}


/*
d_token_is_cpp_only
  Reports whether a kind is one only C++ has.
*/
bool
d_token_is_cpp_only(
    int _kind
)
{
    return ((_kind >= D_TOKEN_CPP_FIRST) &&
            (d_internal_row_for(_kind) != NULL));
}


/*
d_token_is_keyword
  Reports whether a kind is a keyword in the language that produced it.
*/
bool
d_token_is_keyword(
    int _kind
)
{
    const struct d_internal_row* const row = d_internal_row_for(_kind);

    return (row && (row->group == D_INTERNAL_GROUP_KEYWORD));
}


/*
d_token_is_punctuator
  Reports whether a kind is a punctuator.
*/
bool
d_token_is_punctuator(
    int _kind
)
{
    const struct d_internal_row* const row = d_internal_row_for(_kind);

    return (row && (row->group == D_INTERNAL_GROUP_PUNCT));
}


/*
d_token_is_literal
  Reports whether a kind is one of the literal classes.
  The raw string class lives in the C++ range, so the test is two comparisons
rather than one, and a shared-range bound would be wrong.
*/
bool
d_token_is_literal(
    int _kind
)
{
    return ( (_kind == D_TOKEN_NUMBER)    ||
             (_kind == D_TOKEN_CHARACTER) ||
             (_kind == D_TOKEN_STRING)    ||
             (_kind == D_TOKEN_RAW_STRING) );
}
