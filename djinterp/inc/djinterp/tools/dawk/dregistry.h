/*******************************************************************************
* djinterp [dawk]                                                    dregistry.h
*
* Property registry:
*   Evaluates one declaration against one matched node, and computes the
* repair when the declaration failed and its expected value is derivable.
*   The registry is the whole of what a sheet can say.  A property that is
* parsed but has no evaluator reports as unevaluated rather than passing,
* because a rule that silently holds is worse than one that visibly cannot
* run yet.
*   A repair exists exactly where the expected value is computable without
* asking a human.  That is a property of the property, not a judgement made
* per rule, and it is what partitions a defect list into the part a tool can
* close and the part it cannot.
*
* path:      /inc/djinterp/tools/dawk/dregistry.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DREGISTRY_H
#define DJINTERP_TOOLS_DAWK_DREGISTRY_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree
#include "./dss.h"    // d_dss_sheet, d_dss_declaration


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Evaluation
//------------------------------------------------------------------------------
bool d_registry_evaluate(const struct d_dss_sheet*       _sheet,
                         struct d_node_tree*             _tree,
                         uint32_t                        _node,
                         const struct d_dss_declaration* _declaration,
                         bool*                           _out_evaluated);

// 1.2    Derivation
//------------------------------------------------------------------------------
bool d_registry_derive(const struct d_dss_sheet*       _sheet,
                       struct d_node_tree*             _tree,
                       uint32_t                        _node,
                       const struct d_dss_declaration* _declaration,
                       char*                           _out,
                       size_t                          _out_size);
double d_registry_number(const struct d_dss_sheet*       _sheet,
                         const struct d_dss_declaration* _declaration);

// 1.3    Repair
//------------------------------------------------------------------------------
bool d_registry_repair(struct d_node_tree* _tree,
                       uint32_t            _node,
                       const char*         _property,
                       double              _want,
                       const char*         _line_text,
                       const char*         _derived,
                       char*               _out,
                       size_t              _out_size);


#endif  // DJINTERP_TOOLS_DAWK_DREGISTRY_H
