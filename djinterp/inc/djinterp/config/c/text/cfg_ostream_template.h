/*******************************************************************************
* djinterp [config]                                       cfg_ostream_template.h
*
*   Configuration for the template engine that writes onto an output stream:
* how deep its conditional blocks may nest. The engine is otherwise
* configuration-free: it allocates nothing, and its operations go into the
* caller's array.
*
*   targets:  c/text/ostream_template.c -> D_CFG_OSTREAM_TEMPLATE_DEPTH
*   requires: cfg_common.h (helpers, D_CFG_TESTING)
*
*   A KNOB, NOT AN OPTION. The depth sizes the compiler's stack of open
* blocks, an array on the C stack, so it is fixed when the engine is built.
* A program that wants a shallower limit for one template need not rebuild:
* nesting is checked as the template compiles, and a template that nests too
* deeply is refused with its offset (D_OSTREAM_TEMPLATE_NESTING).
*
*
* path:      /inc/djinterp/config/c/text/cfg_ostream_template.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Templates
         1.  D_CFG_OSTREAM_TEMPLATE_DEPTH
2.  VALIDATION
    ----------
    1.  Ranges
*/

#ifndef DJINTERP_CONFIG_C_TEXT_CFG_OSTREAM_TEMPLATE_H
#define DJINTERP_CONFIG_C_TEXT_CFG_OSTREAM_TEMPLATE_H 1

// djinterp
#include "../../cfg_common.h"  // the root, first: overrides, D_CFG_TESTING


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Templates
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_OSTREAM_TEMPLATE_DEPTH
//   knob: how deep a template's {?...} and {^...} blocks may nest. The
// compiler keeps one slot per open block on its stack, so this bounds that
// stack; a deeper template is refused as D_OSTREAM_TEMPLATE_NESTING. 1 to 255.
#ifndef D_CFG_OSTREAM_TEMPLATE_DEPTH
    #define D_CFG_OSTREAM_TEMPLATE_DEPTH 16
#endif  // D_CFG_OSTREAM_TEMPLATE_DEPTH


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Ranges
//------------------------------------------------------------------------------
#if ( ((D_CFG_OSTREAM_TEMPLATE_DEPTH + 0) < 1) ||                              \
      ((D_CFG_OSTREAM_TEMPLATE_DEPTH + 0) > 255) )
    #error "D_CFG_OSTREAM_TEMPLATE_DEPTH must be from 1 to 255"
#endif


#endif  // DJINTERP_CONFIG_C_TEXT_CFG_OSTREAM_TEMPLATE_H
