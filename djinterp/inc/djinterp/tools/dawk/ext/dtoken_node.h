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
*   Every attribute is a fact about the source, present when it holds:
* an identifier's `case`, a `#define` that is `guarded`.  A naming rule is a
* negation -- a macro *not* spelled `D_` -- and is written as one, with
* `:not([text^="D_"])`, so no attribute exists only to make a negation
* expressible; the judgement stays in the sheet.
*
*
* path:      /inc/djinterp/tools/dawk/ext/dtoken_node.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Types
         1.  d_token_node_input
         2.  d_token_node_stats
2.  OPERATIONS
    ----------
    1.  Building
*/

#ifndef DJINTERP_TOOLS_DAWK_EXT_DTOKEN_NODE_H
#define DJINTERP_TOOLS_DAWK_EXT_DTOKEN_NODE_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t

// djinterp
#include "../dnode.h"                              // d_node_tree
#include "../../../parse/lex/lex_dialect.h"          // d_lex_dialect
#include "../../../parse/c/diagnostic.h"             // d_parse_diag_sink
#include "../../../parse/lex/lex_scan.h"             // d_lexer
#include "../../../parse/source/source_reader.h"     // d_source


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_token_node_input
//   struct: what a build reads and where it reports -- everything but the
//   language, which each entry point takes in its own terms.  `source` is
//   borrowed for the call and required; `sink` receives the scanner's
//   diagnostics and may be NULL.  Grouped so that the next input (a target
//   descriptor, a preprocessor context) is a field, not a parameter.
struct d_token_node_input
{
    const struct d_source*     source;
    struct d_parse_diag_sink*  sink;
};

// d_token_node_stats
//   struct: what a build produced and what it found wrong.  `lex_errors`
//   counts the error diagnostics the scanner emitted during this build, into
//   the caller's sink or, with none, a counting one; `unbalanced` totals the
//   three bracket counts from syntax_group.h, outside directives only.
struct d_token_node_stats
{
    size_t             tokens;
    size_t             nodes;
    size_t             groups;
    size_t             directives;
    size_t             comments;
    size_t             unbalanced;
    size_t             items;
    uint32_t           lex_errors;
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
 * @param[in]     _dialect    the language to read it as.
 * @param[in]     _input      the source and the sink; see d_token_node_input.
 * @param[out]    _out_stats  receives what was built; may be `NULL`.
 * @return the node the tree hangs from, or `D_DSS_NO_INDEX` on failure.
 */
uint32_t     d_token_node_build(struct d_node_tree*              _tree,
                                uint32_t                         _parent,
                                const struct d_lex_dialect*      _dialect,
                                const struct d_token_node_input* _input,
                                struct d_token_node_stats*       _out_stats);


#endif  // DJINTERP_TOOLS_DAWK_EXT_DTOKEN_NODE_H
