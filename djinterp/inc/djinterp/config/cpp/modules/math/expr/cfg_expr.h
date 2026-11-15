/*******************************************************************************
* djinterp [config]                                                   cfg_expr.h
*
*   Build-time configuration of the run-time expression language
* (cpp/modules/math/expr/): how many operands a compiled term keeps in the
* caller's frame before it asks the heap for them (expr/program.hpp, S395).
*   The knob is of the value model: it expands to a number, not to 0 or 1, and
* is validated as one. It is #ifndef-guarded, so a definition made before the
* language is first included -- with -D, in cfg_custom.h, or in the including
* file -- wins over the default here.
*
*   targets:  cpp/modules/math/expr/program.hpp -> D_CFG_MATH_EXPR_PROGRAM_STACK
*   requires: cfg_common.h (user overrides, the testing preset)
*
*
* path:      /inc/djinterp/config/cpp/modules/math/expr/cfg_expr.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  A compiled term
         1.  D_CFG_MATH_EXPR_PROGRAM_STACK
*/

#ifndef DJINTERP_CONFIG_CPP_MATH_EXPR_CFG_EXPR_H
#define DJINTERP_CONFIG_CPP_MATH_EXPR_CFG_EXPR_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_* helpers, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    A compiled term
//------------------------------------------------------------------------------
// A compiled term runs on a stack of operands whose greatest depth is known
// when it is compiled. A depth within the knob runs in an array of the
// caller's frame, with no allocation; a deeper one in a vector, one
// allocation a run.

// 1.1.1
// D_CFG_MATH_EXPR_PROGRAM_STACK
//   knob: the operands a run keeps in the caller's frame, from 1; each is one
// number of the program's type, or in a complex type a complex number and a
// flag. At the default that is 512 bytes of long doubles on x86-64, and
// 1,536 of complex long doubles (384 and 896 on 32-bit x86). A term of n
// nested operations of two operands is n + 1 deep at most; 32 holds every
// term a person writes.
#ifndef D_CFG_MATH_EXPR_PROGRAM_STACK
    #define D_CFG_MATH_EXPR_PROGRAM_STACK 32
#endif  // D_CFG_MATH_EXPR_PROGRAM_STACK

// validated: a number from 1
#if ( (D_CFG_MATH_EXPR_PROGRAM_STACK) < 1 )
    #error "D_CFG_MATH_EXPR_PROGRAM_STACK must be a number from 1"
#endif


#endif  // DJINTERP_CONFIG_CPP_MATH_EXPR_CFG_EXPR_H
