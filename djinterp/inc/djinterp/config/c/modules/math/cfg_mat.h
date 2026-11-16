/*******************************************************************************
* djinterp [config]                                                    cfg_mat.h
*
*   Build-time configuration of the fixed-size matrices' C core
* (c/math/mat.h): how far d_mat_*_solve refines a solution. The solve takes
* each column as Gaussian elimination gives it, then corrects it from its
* residual, computed with Dot2's extra precision, until the corrections stop
* mattering or stop shrinking (S406); this knob bounds the steps, as
* LAPACK's ITHRESH bounds dgerfsx's.
*   The knob is of the value model: a count, validated below. It is
* #ifndef-guarded, so a definition made before mat.h is first included --
* with -D, in cfg_custom.h, or in the including file -- wins over the default
* here.
*
*   targets:  c/math/mat.h -> D_CFG_MATH_MAT_REFINE_STEPS
*   requires: cfg_common.h (user overrides, the testing preset)
*
*
* path:      /inc/djinterp/config/c/modules/math/cfg_mat.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Refinement
         1.  D_CFG_MATH_MAT_REFINE_STEPS
*/

#ifndef DJINTERP_CONFIG_C_MATH_CFG_MAT_H
#define DJINTERP_CONFIG_C_MATH_CFG_MAT_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Refinement
//------------------------------------------------------------------------------
// Each step costs a residual, n rows of Dot2 over 2 n products, and a
// solution by the factors: some 50 n^2 operations against the
// elimination's 2/3 n^3. A system cond(A) eps from singular gains about
// -log2(cond(A) eps) bits a step, so one or two steps converge for most;
// the bound matters only where the corrections neither converge nor stop
// shrinking quickly.

// 1.1.1
// D_CFG_MATH_MAT_REFINE_STEPS
//   knob: the most correction steps d_mat_*_solve takes on each column of
// its solution, 1 or more; 10 by default, LAPACK's ITHRESH. d_mat_*_
// solve_fast takes none.
#ifndef D_CFG_MATH_MAT_REFINE_STEPS
    #define D_CFG_MATH_MAT_REFINE_STEPS 10
#endif  // D_CFG_MATH_MAT_REFINE_STEPS

// a step at least: d_mat_*_solve_fast is the solve without any
#if (D_CFG_MATH_MAT_REFINE_STEPS < 1)
    #error "cfg_mat.h: D_CFG_MATH_MAT_REFINE_STEPS must be 1 or more"
#endif


#endif  // DJINTERP_CONFIG_C_MATH_CFG_MAT_H
