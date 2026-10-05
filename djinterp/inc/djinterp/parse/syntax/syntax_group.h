/*******************************************************************************
* djinterp [parse]                                                syntax_group.h
*
* Bracket and directive structure over a token stream: the first layer
* above lexing, and the one every other structural question rests on.
*   Grouping needs no language.  `{`, `[` and `(` pair the same way in C and
* C++, and a digraph arrives already carrying its bracket's kind, so this
* module consults no dialect and is written once.
*   A directive line is a group of its own.  A macro body may leave a
* bracket open -- `#define BEGIN {` is legal and common -- so a bracket
* opened on a directive line pairs only within that line, and whatever it
* left open closes when the line does.  Without that rule one such macro
* would swallow the rest of the file.
*   The result is two parallel arrays indexed by token, which is the
* layout measured faster than interleaved records in the tree work.  The
* nesting stack is threaded through the parent array itself, so grouping
* allocates the two arrays and nothing else, and does not recurse.
*
*
* path:      /inc/djinterp/parse/syntax/syntax_group.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_SYNTAX_NONE
    2.  Types
         1.  d_syntax_groups
2.  OPERATIONS
    ----------
    1.  Grouping
    2.  Classification
*/

#ifndef DJINTERP_PARSE_SYNTAX_SYNTAX_GROUP_H
#define DJINTERP_PARSE_SYNTAX_SYNTAX_GROUP_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

// djinterp
#include "../lex/lex_token.h"  // d_token
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_SYNTAX_NONE
//   constant: the index reported for no token -- no parent at the top level,
// no partner for an unpaired bracket.
#define D_SYNTAX_NONE ((uint32_t)-1)

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_syntax_groups
//   struct: the structure of one token stream.  `parent[i]` is the opening
//   bracket or directive `#` that encloses token i; `match[i]` is the partner
//   of a bracket, or for a directive `#` the last token of its line.  A
//   closing bracket's parent is its opener's parent, so the pair are siblings
//   and everything between them is beneath the opener.
//     The three counts describe the stream's balance outside directives,
//   where imbalance is a defect.  Inside a directive it is not counted, since
//   a macro body may be unbalanced by design.
struct d_syntax_groups
{
    size_t     count;       // tokens described
    uint32_t*  parent;      // enclosing opener, or D_SYNTAX_NONE
    uint32_t*  match;       // partner, or D_SYNTAX_NONE

    size_t     unclosed;    // openers never closed
    size_t     unopened;    // closers with no opener in scope
    size_t     mismatched;  // closers that closed past an unclosed opener
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Grouping
//------------------------------------------------------------------------------
// d_syntax_group describes a token array.  Imbalance is recovered from rather
// than rejected: a closer pairs with the nearest opener of its kind, leaving
// any opener it passes unclosed, and a closer with no such opener is a leaf.
// The return is false only when the arrays could not be allocated.
/**
 * @brief Describes the bracket and directive structure of a token array.
 *
 * @note Imbalance is recovered from, not rejected; see the counts.
 *
 * @param[in]  _tokens      the tokens, in source order.
 * @param[in]  _count       how many.
 * @param[out] _out_groups  receives the arrays and the balance counts.
 * @post The arrays are released with d_syntax_groups_free.
 * @return `true` on success, `false` if the arrays could not be allocated.
 */
bool         d_syntax_group(const struct d_token*   _tokens,
                            size_t                  _count,
                            struct d_syntax_groups* _out_groups);
/**
 * @brief Releases a description's arrays.
 *
 * @param[in,out] _groups  the description; may be `NULL`.
 * @post The arrays are invalid and the count is zero.  The structure itself
 *       is the caller's.
 */
void         d_syntax_groups_free(struct d_syntax_groups* _groups);

// 2.2    Classification
//------------------------------------------------------------------------------
bool         d_syntax_is_opener(int _kind);
bool         d_syntax_is_closer(int _kind);
bool         d_syntax_is_directive_start(const struct d_token* _token);
int          d_syntax_closer_for(int _opener_kind);


#endif  // DJINTERP_PARSE_SYNTAX_SYNTAX_GROUP_H
