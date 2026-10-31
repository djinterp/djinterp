/*******************************************************************************
* djinterp [config]                                             cfg_value_wide.h
*
*   Build-time configuration of the wide integer as a boxed kind
* (c/math/value_wide.h): where its boxes are stored. Every box the kind makes
* is allocated through D_CFG_MATH_VALUE_WIDE_MALLOC and freed through
* D_CFG_MATH_VALUE_WIDE_FREE, so a host with an arena of its own takes all of
* it without patching the kind, as fs's allocation hooks allow
* (cfg_file_common.h).
*   Both knobs are of the value model: each expands to a call, not to 0 or 1,
* and is not validated as a flag. Each is #ifndef-guarded, so a definition
* made before the kind is first included -- with -D, in cfg_custom.h, or in
* the including file -- wins over the default here.
*
*   targets:  c/math/value_wide.h -> D_CFG_MATH_VALUE_WIDE_MALLOC,
*                                    D_CFG_MATH_VALUE_WIDE_FREE
*   requires: cfg_common.h (user overrides, the testing preset); the defaults
*             call malloc and free, which value_wide.h declares through
*             <stdlib.h>
*
*
* path:      /inc/djinterp/config/c/modules/math/cfg_value_wide.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Allocation hooks
         1.  D_CFG_MATH_VALUE_WIDE_MALLOC
         2.  D_CFG_MATH_VALUE_WIDE_FREE
*/

#ifndef DJINTERP_CONFIG_C_MATH_CFG_VALUE_WIDE_H
#define DJINTERP_CONFIG_C_MATH_CFG_VALUE_WIDE_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Allocation hooks
//------------------------------------------------------------------------------
// A box is a holder count and a wide integer: one allocation of a fixed size
// for each value the kind makes, and one release when its last holder lets
// it go.

// 1.1.1
// D_CFG_MATH_VALUE_WIDE_MALLOC
//   knob: the call that allocates a box of _size bytes; malloc's contract,
// NULL where no storage is given, which the kind reports as
// D_MATH_VALUE_ERROR_MEMORY.
#ifndef D_CFG_MATH_VALUE_WIDE_MALLOC
    #define D_CFG_MATH_VALUE_WIDE_MALLOC(_size) malloc(_size)
#endif  // D_CFG_MATH_VALUE_WIDE_MALLOC

// 1.1.2
// D_CFG_MATH_VALUE_WIDE_FREE
//   knob: the call that gives back a box D_CFG_MATH_VALUE_WIDE_MALLOC
// allocated; free's contract.
#ifndef D_CFG_MATH_VALUE_WIDE_FREE
    #define D_CFG_MATH_VALUE_WIDE_FREE(_ptr) free(_ptr)
#endif  // D_CFG_MATH_VALUE_WIDE_FREE


#endif  // DJINTERP_CONFIG_C_MATH_CFG_VALUE_WIDE_H
