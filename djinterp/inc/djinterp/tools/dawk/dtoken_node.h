/*******************************************************************************
* djinterp [djinterp]                                              dtoken_node.h
*
* Builds a DSS node tree from source, through the /parse/ layers.
*   This is the one place tokens become nodes, shared by the C and C++
* extensions so that neither carries a copy.  An extension chooses a dialect
* and a file; everything after that choice is here, and the two languages
* produce trees of one shape that one sheet can select over.
*   The tree follows the grouping in syntax_group.h: a bracket pair becomes
* a `group` holding what lies between, a directive line a `directive`
* holding its tokens, and everything else a leaf named for its class.  The
* closing bracket is not a node of its own; its position is an attribute of
* its group, since a selector wants the group and a layout rule wants where
* it ends.
*   Identifiers carry two policy-free shape facts, `prefix` and `case`.
* A naming rule is a negation -- a macro *not* spelled `D_` -- and the
* matcher has no `:not()`, so `[prefix!=D_]` is how such a rule is written.
* The shape is a fact about the spelling, not a judgement about it; the
* judgement stays in the sheet.  Both are present on every identifier, empty
* or `none` where they do not apply, because the matcher fails a comparison
* against an absent attribute and an absent prefix would hide a name with no
* segment from exactly the rule meant to find it.
*
*
* path:      /inc/djinterp/tools/dawk/dtoken_node.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Types
         1.  d_token_node_stats
2.  OPERATIONS
    ----------
    1.  Building
*/

#ifndef DJINTERP_TOOLS_DAWK_DTOKEN_NODE_H
#define DJINTERP_TOOLS_DAWK_DTOKEN_NODE_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t

// djinterp
#include "./dnode.h"                              // d_node_tree
#include "../../parse/lex/lex_dialect.h"          // d_lex_dialect
#include "../../parse/lex/lex_scan.h"             // d_lex_status
#include "../../parse/source/source_reader.h"     // d_source


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_token_node_stats
//   struct: what a build produced and what it found wrong.  `lex_status` is
//   the scanner's first latched error; `unbalanced` totals the three bracket
//   counts from syntax_group.h, outside directives only.
struct d_token_node_stats
{
    size_t             tokens;
    size_t             nodes;
    size_t             groups;
    size_t             directives;
    size_t             comments;
    size_t             unbalanced;
    size_t             items;
    enum d_lex_status  lex_status;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Building
//------------------------------------------------------------------------------
// d_token_node_build lexes, groups and emits a source beneath `_parent`.
// When `_parent` is D_DSS_NO_INDEX a `file` node is created to hold the tree,
// carrying `path`, `lang` -- the language, `c` or `c++`, as the element model
// uses the word -- and `std`, the standard it was read as, so a host with no
// file node of its own still gets the element model's shape.  Returns the
// node the tree hangs from, or D_DSS_NO_INDEX when the tree could not be
// built.
/**
 * @brief Lexes, groups and emits a source as a DSS node tree.
 *
 * @param[in,out] _tree       the tree to add to.
 * @param[in]     _parent     the node to build beneath, or `D_DSS_NO_INDEX` for
 *                            a new `file` node carrying `path`, `lang` and
 *                            `std`.
 * @param[in]     _source     the source; borrowed for the call.
 * @param[in]     _dialect    the language to read it as.
 * @param[out]    _out_stats  receives what was built; may be `NULL`.
 * @return the node the tree hangs from, or `D_DSS_NO_INDEX` on failure.
 */
uint32_t     d_token_node_build(struct d_node_tree*         _tree,
                                uint32_t                    _parent,
                                const struct d_source*      _source,
                                const struct d_lex_dialect* _dialect,
                                struct d_token_node_stats*  _out_stats);


#endif  // DJINTERP_TOOLS_DAWK_DTOKEN_NODE_H
