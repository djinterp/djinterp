/*******************************************************************************
* djinterp [tools]                                                    dcascade.h
*
* The cascade: which declaration wins, per node and property, computed once.
*   A host records each (node, rule) match; d_cascade_rank orders every node's
* matches best first under a policy; after that, asking which rule wins a
* property is a walk over one node's few matches instead of every rule in
* the sheet.  Three policies: FIRST, the linter's (a higher layer, then the
* earlier rule, wins); ALL, where every declaration counts; and CSS (layer,
* then specificity, then the later rule, and the last declaration of a
* property within its rule).  Computed values follow CSS inheritance: an
* inherited property nobody declares takes its parent's.
*
*
* path:      /inc/djinterp/tools/dawk/dcascade.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Enumerations
    2.  Opaque types
2.  OPERATIONS
    ----------
    1.  Lifecycle
    2.  Recording
    3.  Queries
*/

#ifndef DJINTERP_TOOLS_DAWK_DCASCADE_H
#define DJINTERP_TOOLS_DAWK_DCASCADE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree
#include "./dss.h"    // d_dss_sheet


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// d_cascade_policy
//   enum: how competing declarations resolve.
enum d_cascade_policy
{
    D_CASCADE_FIRST = 0,  // higher layer, then earlier rule; the linter's
    D_CASCADE_ALL,        // every declaration counts
    D_CASCADE_CSS         // layer, specificity, later rule; CSS's own
};

// 1.2    Opaque types
//------------------------------------------------------------------------------
// 1.2.1
// d_cascade
//   struct: one tree's matches, ranked per node.
struct d_cascade;


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Allocates a cascade for one sheet.
 *
 * @param[in] _sheet   the sheet; bound (d_dss_bind) so properties compare as
 *                     symbols.  It must outlive the cascade.
 * @param[in] _policy  a d_cascade_policy.
 * @return the cascade, or `NULL` if allocation failed.
 */
struct d_cascade* d_cascade_new(const struct d_dss_sheet* _sheet,
                                enum d_cascade_policy     _policy);
void              d_cascade_free(struct d_cascade* _cascade);

// 2.2    Recording
//------------------------------------------------------------------------------
/**
 * @brief Forgets every match, ready for a tree of `_node_count` nodes.
 *
 * @param[in,out] _cascade     the cascade.
 * @param[in]     _node_count  the tree's node count.
 * @return `true` on success, `false` if allocation failed.
 */
bool              d_cascade_reset(struct d_cascade* _cascade,
                                  size_t            _node_count);
/**
 * @brief Records that a rule matches a node, in any order.
 *
 * @param[in,out] _cascade      the cascade.
 * @param[in]     _node         the node.
 * @param[in]     _rule         the rule.
 * @param[in]     _specificity  the rule's specificity at this node; read by
 *                              the CSS policy only.
 * @return `true` on success, `false` if allocation failed.
 */
bool              d_cascade_add(struct d_cascade* _cascade,
                                uint32_t          _node,
                                uint32_t          _rule,
                                uint32_t          _specificity);
/**
 * @brief Orders every node's matches best first; the queries need it.
 *
 * @param[in,out] _cascade  the cascade.
 * @return `true` on success, `false` if allocation failed.
 */
bool              d_cascade_rank(struct d_cascade* _cascade);

// 2.3    Queries
//------------------------------------------------------------------------------
// a node's matching rules, best first; the count, with *_out_rules set
size_t            d_cascade_matched(const struct d_cascade* _cascade,
                                    uint32_t                _node,
                                    const uint32_t**        _out_rules);
// the best-ranked matching rule declaring the property, or D_DSS_NO_INDEX
uint32_t          d_cascade_winning_rule(const struct d_cascade* _cascade,
                                         uint32_t                _node,
                                         uint32_t                _property);
// whether a rule's declarations of the property count; always under ALL
bool              d_cascade_wins(const struct d_cascade* _cascade,
                                 uint32_t                _node,
                                 uint32_t                _rule,
                                 uint32_t                _property);
/**
 * @brief The declaration that decides a property at a node.
 *
 * @param[in] _cascade    the cascade.
 * @param[in] _tree       the tree, for walking to ancestors.
 * @param[in] _node       the node.
 * @param[in] _property   the property's symbol.
 * @param[in] _inherited  whether an undeclared value comes from the parent.
 * @return the declaration's index, or `D_DSS_NO_INDEX` when the property
 *         takes its initial value.
 */
uint32_t          d_cascade_computed(const struct d_cascade* _cascade,
                                     struct d_node_tree*     _tree,
                                     uint32_t                _node,
                                     uint32_t                _property,
                                     bool                    _inherited);


#endif  // DJINTERP_TOOLS_DAWK_DCASCADE_H
