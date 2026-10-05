/*******************************************************************************
* djinterp [parse]                                                 syntax_item.h
*
* Declarations, statements, labels and parameters: the items a scope is
* made of, recognized over tokens and their grouping.
*   This layer answers where each declaration and statement begins and
* ends, what it is, what it declares, and how deeply it is nested -- and no
* more.  It does not decide what an identifier means.  `T * x;` in a block
* is a statement of kind `simple`, because telling a declaration from a
* multiplication there needs to know whether `T` names a type, which is a
* symbol table's question and not a recognizer's.  Every fact reported here
* is decided by tokens alone.
*   Scopes are walked in the order of their openers, so the item that owns
* a scope always exists before the scope is walked.  Within a scope, the
* chain of open braceless bodies -- `if (a) for (;;) x;` -- is threaded
* through each item's `parent`, as the grouping threads its stack through
* `parent`, so there is no recursion and no work stack.
*   `level` is the indentation level the structure implies, counting a
* body, a member list, a parameter list, a braceless body and the statements
* under a `case` label, and not counting an `extern "C"` block or a
* namespace.  `pp_depth` counts the preprocessor conditionals opened inside
* the item's own scope.  Both are facts; what column they imply is a style
* sheet's decision.
*
*
* path:      /inc/djinterp/parse/syntax/syntax_item.h
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
         1.  Item flags
    2.  Types
         1.  d_syntax_item_kind
         2.  d_syntax_statement
         3.  d_syntax_item
         4.  d_syntax_items
2.  OPERATIONS
    ----------
    1.  Recognition
    2.  Naming
*/

#ifndef DJINTERP_PARSE_SYNTAX_SYNTAX_ITEM_H
#define DJINTERP_PARSE_SYNTAX_SYNTAX_ITEM_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

// djinterp
#include "../lex/lex_token.h"   // d_token
#include "./syntax_group.h"     // d_syntax_groups
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint16_t, uint8_t


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// Item flags
//   constant: facts about one item.  RETURNS_VOID, ENDS_RETURN and EMPTY_BODY
// describe a function; FN_POINTER and FN_TYPE a typedef's declarator; the
// parameter flags a single parameter; MACRO_HEAD an item whose head is a
// macro invocation -- `D_TEST(name) { ... }` -- whose parentheses are the
// macro's arguments and not a parameter list.
//     ALIGNED marks a parameter in a list whose first parameter shares the
// line of its `(`.  Such a list is laid out by alignment under the first
// parameter rather than by indentation, so its `level` says nothing about its
// column; MISALIGNED marks a parameter that begins a line elsewhere than
// beneath the first.  Both are geometry, not judgement.
//     HIDES_TAG marks a typedef whose type is spelled with a `struct`,
// `union` or `enum` keyword at its own level -- the one kind the guide's
// Typedefs section forbids -- whatever the declarator's form.  DETACHED marks
// every parameter of a declaration whose first parameter begins its own line:
// the guide lays a declaration's parameters out after its `(`, while a
// definition's begin their own lines by rule, so only a declaration has it.
#define D_SYNTAX_ITEM_FLAG_NONE          0x0000u
#define D_SYNTAX_ITEM_FLAG_RETURNS_VOID  0x0001u  // returns `void`, not `void*`
#define D_SYNTAX_ITEM_FLAG_ENDS_RETURN   0x0002u  // body ends with `return`
#define D_SYNTAX_ITEM_FLAG_EMPTY_BODY    0x0004u  // body holds no item
#define D_SYNTAX_ITEM_FLAG_MACRO_HEAD    0x0008u  // head is a macro call
#define D_SYNTAX_ITEM_FLAG_FN_POINTER    0x0010u  // declares a function pointer
#define D_SYNTAX_ITEM_FLAG_FN_TYPE       0x0020u  // declares a function type
#define D_SYNTAX_ITEM_FLAG_VARIADIC      0x0040u  // parameter is `...`
#define D_SYNTAX_ITEM_FLAG_VOID          0x0080u  // parameter list is `(void)`
#define D_SYNTAX_ITEM_FLAG_UNNAMED       0x0100u  // declares nothing by name
#define D_SYNTAX_ITEM_FLAG_UNDER_LABEL   0x0200u  // under a `case`/`default`
#define D_SYNTAX_ITEM_FLAG_BRACELESS     0x0400u  // an unbraced body
#define D_SYNTAX_ITEM_FLAG_UNTERMINATED  0x0800u  // its scope ended first
#define D_SYNTAX_ITEM_FLAG_ALIGNED       0x1000u  // parameters aligned, not
                                                  // indented
#define D_SYNTAX_ITEM_FLAG_MISALIGNED    0x2000u  // not under the first one
#define D_SYNTAX_ITEM_FLAG_HIDES_TAG     0x4000u  // typedef names a struct,
                                                  // union or enum type
#define D_SYNTAX_ITEM_FLAG_DETACHED      0x8000u  // a declaration's first
                                                  // parameter begins a line

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_syntax_item_kind
//   enum: what an item is.  BLOCK is an `extern "C" { }` or a namespace,
//   whose contents belong to the scope around it for every purpose but
//   grouping.
enum d_syntax_item_kind
{
    D_SYNTAX_ITEM_DECLARATION = 0,
    D_SYNTAX_ITEM_FN_DEF,
    D_SYNTAX_ITEM_FN_DECL,
    D_SYNTAX_ITEM_TYPEDEF,
    D_SYNTAX_ITEM_STATEMENT,
    D_SYNTAX_ITEM_LABEL,
    D_SYNTAX_ITEM_PARAM,
    D_SYNTAX_ITEM_ENUMERATOR,
    D_SYNTAX_ITEM_BLOCK
};

// 1.2.2
// d_syntax_statement
//   enum: what a statement is, by its leading token -- which is as far as
//   tokens can tell.  SIMPLE is an expression statement or a declaration
//   whose type is a typedef name; the two are not told apart.  MACRO is a
//   macro invocation followed by a braced body, `D_FOREACH(x) { }`.
enum d_syntax_statement
{
    D_SYNTAX_STMT_NONE = 0,
    D_SYNTAX_STMT_SIMPLE,
    D_SYNTAX_STMT_COMPOUND,
    D_SYNTAX_STMT_EMPTY,
    D_SYNTAX_STMT_IF,
    D_SYNTAX_STMT_FOR,
    D_SYNTAX_STMT_WHILE,
    D_SYNTAX_STMT_DO,
    D_SYNTAX_STMT_SWITCH,
    D_SYNTAX_STMT_RETURN,
    D_SYNTAX_STMT_BREAK,
    D_SYNTAX_STMT_CONTINUE,
    D_SYNTAX_STMT_GOTO,
    D_SYNTAX_STMT_TRY,
    D_SYNTAX_STMT_MACRO,
    D_SYNTAX_STMT_CASE,
    D_SYNTAX_STMT_DEFAULT,
    D_SYNTAX_STMT_NAMED,
    D_SYNTAX_STMT_ACCESS
};

// 1.2.3
// d_syntax_item
//   struct: one item, as an inclusive range of token indices.  `scope` is
//   the opener whose interior holds the item; `parent` is the item holding
//   it within that scope, which is set only for a braceless body.  `name`,
//   `params` and `body` are token indices of the declared identifier, the
//   parameter group's opener and the body or member group's opener.
struct d_syntax_item
{
    uint32_t  first;
    uint32_t  last;
    uint32_t  parent;
    uint32_t  scope;
    uint32_t  name;
    uint32_t  params;
    uint32_t  body;
    uint16_t  flags;       // D_SYNTAX_ITEM_FLAG_* bits
    uint8_t   kind;        // d_syntax_item_kind
    uint8_t   statement;   // d_syntax_statement; LABEL items use it too
    uint8_t   level;       // indentation level the structure implies
    uint8_t   pp_depth;    // conditionals opened within the item's scope
    uint8_t   pp_total;    // conditionals around it, include guard excluded
};

// 1.2.4
// d_syntax_items
//   struct: every item of a token stream.  `item_of[i]` is the innermost item
//   containing token i; `level_of[o]` is, for an opener that is some item's
//   body, the level of the item that owns it.  Both are parallel to the
//   tokens, as the grouping's arrays are.
struct d_syntax_items
{
    struct d_syntax_item*  items;
    size_t                 count;
    size_t                 capacity;
    uint32_t*              item_of;
    uint8_t*               level_of;
    size_t                 token_count;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Recognition
//------------------------------------------------------------------------------
// d_syntax_items_build recognizes the items of a grouped token stream.
// `_text` is the source the tokens index, read only to spell the names of
// directives.  Returns false only when memory ran out; a construct the
// recognizer does not understand still becomes an item, of the most general
// kind that fits.
/**
 * @brief Recognizes the declarations, statements, labels and parameters of a
 *        grouped token stream.
 *
 * @param[in]  _tokens     the tokens, in source order.
 * @param[in]  _count      how many.
 * @param[in]  _groups     their grouping, from d_syntax_group.
 * @param[in]  _text       the source the tokens index; read only to spell
 *                         directive names.
 * @param[out] _out_items  receives the items.
 * @pre  `_groups` describes exactly `_tokens`.
 * @post The items are released with d_syntax_items_free.
 * @return `true` on success, `false` only if memory ran out.
 */
bool         d_syntax_items_build(const struct d_token*         _tokens,
                                  size_t                        _count,
                                  const struct d_syntax_groups* _groups,
                                  const char*                   _text,
                                  struct d_syntax_items*        _out_items);
/**
 * @brief Releases a recognition's arrays.
 *
 * @param[in,out] _items  the recognition; may be `NULL`.
 * @post Every array is invalid and every count zero.
 */
void         d_syntax_items_free(struct d_syntax_items* _items);

// 2.2    Naming
//------------------------------------------------------------------------------
// The node-type and attribute spellings a consumer shows, `fn-def` and `if`.
const char*  d_syntax_item_kind_name(unsigned _kind);
const char*  d_syntax_statement_name(unsigned _statement);


#endif  // DJINTERP_PARSE_SYNTAX_SYNTAX_ITEM_H
