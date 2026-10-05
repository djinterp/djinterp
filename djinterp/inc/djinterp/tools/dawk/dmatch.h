/*******************************************************************************
* djinterp [djinterp]                                                   dmatch.h
*
* Selector matching:
*   Tests a parsed DSS selector against a node.  Matching runs right to left:
* the rightmost compound is tried first and a failure there costs nothing
* further, which is the standard browser optimisation and the reason a sheet
* with many rules stays cheap per node.
*   The matcher knows nothing about what the nodes mean.  It reads type,
* class, attributes and tree position, and nothing else.
*
*
* path:      /inc/djinterp/tools/dawk/dmatch.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DMATCH_H
#define DJINTERP_TOOLS_DAWK_DMATCH_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t

// djinterp
#include "./dnode.h"  // d_node_tree
#include "./dss.h"    // d_dss_sheet


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Matching
//------------------------------------------------------------------------------
bool d_match_selector(const struct d_dss_sheet* _sheet,
                      struct d_node_tree*       _tree,
                      uint32_t                  _node,
                      uint32_t                  _selector);
bool d_match_rule(const struct d_dss_sheet* _sheet,
                  struct d_node_tree*       _tree,
                  uint32_t                  _node,
                  const struct d_dss_rule*  _rule);

// 1.2    Rule index
//------------------------------------------------------------------------------
// d_match_index
//   struct: a bound sheet's rules bucketed by the node type their subject
// names, so a node meets only the rules that could match it.
struct d_match_index;
/**
 * @brief Builds the index of a sheet, which should already be bound.
 *
 * @note An unbound sheet indexes every rule as universal: correct, no faster.
 *
 * @param[in] _sheet  the sheet; it must outlive the index.
 * @return the index, or `NULL` if allocation failed.
 */
struct d_match_index* d_match_index_new(const struct d_dss_sheet* _sheet);
void                  d_match_index_free(struct d_match_index* _index);
/**
 * @brief Lists, in ascending rule order, the rules a node of one type could
 *        match.
 *
 * @param[in]  _index     the index.
 * @param[in]  _type      the node's type symbol (`node->type`).
 * @param[out] _out       receives the rule indices.
 * @param[in]  _capacity  its length; the sheet's rule count always suffices.
 * @return how many were written.
 */
size_t                d_match_index_candidates(
                          const struct d_match_index* _index,
                          uint32_t                    _type,
                          uint32_t*                   _out,
                          size_t                      _capacity);
/**
 * @brief Matches a rule and reports the specificity of its most specific
 *        matching selector, for a CSS cascade.
 *
 * @param[in]  _sheet            the sheet.
 * @param[in]  _tree             the tree.
 * @param[in]  _node             the node.
 * @param[in]  _rule             the rule.
 * @param[out] _out_specificity  receives it; may be `NULL`.
 * @return `true` if any selector of the rule matches.
 */
bool                  d_match_rule_specificity(
                          const struct d_dss_sheet* _sheet,
                          struct d_node_tree*       _tree,
                          uint32_t                  _node,
                          const struct d_dss_rule*  _rule,
                          uint32_t*                 _out_specificity);

// 1.3    Validation
//------------------------------------------------------------------------------
bool d_match_unsupported(const struct d_dss_sheet* _sheet,
                         uint32_t*                 _out_line,
                         const char**              _out_name);


#endif  // DJINTERP_TOOLS_DAWK_DMATCH_H
