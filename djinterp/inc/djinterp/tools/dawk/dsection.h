/*******************************************************************************
* djinterp [tools]                                                    dsection.h
*
* Section-delimiter extension:
*   Emits one `delimiter` node for every full-line rule that separates the
* sections of a header -- a run of `=` for a section, of `-` for a
* subsection.  Two forms are in use and both are reported: the guide's, with
* the fill directly after `//`, and an older spaced form, `// ===`, which is
* the one most of the tree still carries.  A CMake file's delimiters are
* the same lines with `#` for `//`, and d_section_build_marked reads them.
*   The extension reports which form a line takes; it does not decide which
* form is right.  That belongs to the sheet, so a migration between the two
* is a change of rule rather than a change of code.
*
*
* path:      /inc/djinterp/tools/dawk/dsection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_TOOLS_DAWK_DSECTION_H
#define DJINTERP_TOOLS_DAWK_DSECTION_H 1

// std
#include <stdint.h>   // uint32_t
// djinterp
#include "./dnode.h"  // d_node_tree


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Construction
//------------------------------------------------------------------------------
uint32_t d_section_build(struct d_node_tree* _tree,
                         uint32_t            _file,
                         const char*         _path);
// d_section_build_marked reads delimiters whose comment opens with
// `_comment` -- "//" as d_section_build reads them, "#" in a CMake file
uint32_t d_section_build_marked(struct d_node_tree* _tree,
                                uint32_t            _file,
                                const char*         _path,
                                const char*         _comment);


#endif  // DJINTERP_TOOLS_DAWK_DSECTION_H
