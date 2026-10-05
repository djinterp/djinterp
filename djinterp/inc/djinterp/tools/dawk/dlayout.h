/*******************************************************************************
* djinterp [djinterp]                                                  dlayout.h
*
* Layout:
*   Two things a formatter needs and a selector engine does not provide on its
* own.  The first is the computed value of a property for a node: which rule
* supplies `display` for this row?  The second is table layout, which lines up
* a column across a run of independent lines.
*   Table layout is the vertical alignment the guide keeps asking for under
* different names -- trailing include comments, function names after their
* return types, parameter names, variable names in a run of declarations.
* Each is a table: rows are lines, cells are runs within them, and a column
* starts where its widest predecessor plus its gap puts it.  One mechanism,
* four rules.
*   A uniform-slot grid, which is what text_lineup produces for a generated
* token list, is the degenerate table in which every column has one width.
*
*
* path:      /inc/djinterp/tools/dawk/dlayout.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DLAYOUT_H
#define DJINTERP_TOOLS_DAWK_DLAYOUT_H 1

// std
#include <stdbool.h>  // bool
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree
#include "./dss.h"    // d_dss_sheet


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Computed values
//------------------------------------------------------------------------------
const char* d_layout_computed(const struct d_dss_sheet* _sheet,
                              struct d_node_tree*       _tree,
                              uint32_t                  _node,
                              const char*               _property);
double      d_layout_computed_number(const struct d_dss_sheet* _sheet,
                                     struct d_node_tree*       _tree,
                                     uint32_t                  _node,
                                     const char*               _property,
                                     double                    _default);

// 1.2    Table layout
//------------------------------------------------------------------------------
bool        d_layout_table_holds(const struct d_dss_sheet* _sheet,
                                 struct d_node_tree*       _tree,
                                 uint32_t                  _table,
                                 bool*                     _out_evaluated);
bool        d_layout_table_repair(const struct d_dss_sheet* _sheet,
                                  struct d_node_tree*       _tree,
                                  uint32_t                  _table,
                                  const char*               _path,
                                  const char**              _out_refusal);


#endif  // DJINTERP_TOOLS_DAWK_DLAYOUT_H
