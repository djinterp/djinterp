/*******************************************************************************
* djinterp [dawk]                                                       dmatch.h
*
* Selector matching:
*   Tests a parsed DSS selector against a node.  Matching runs right to left:
* the rightmost compound is tried first and a failure there costs nothing
* further, which is the standard browser optimisation and the reason a sheet
* with many rules stays cheap per node.
*   The matcher knows nothing about what the nodes mean.  It reads type,
* class, attributes and tree position, and nothing else.
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


#endif  // DJINTERP_TOOLS_DAWK_DMATCH_H
