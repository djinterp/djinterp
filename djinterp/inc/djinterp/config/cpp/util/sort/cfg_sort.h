/*******************************************************************************
* djinterp [config]                                                   cfg_sort.h
*
*   Build-time tuning for the C++ sort subsystem.  One knob so far: the size
* below which quicksort stops partitioning and finishes a sub-range with
* insertion sort.  It changes how fast a sort runs, never what it produces --
* every value sorts correctly -- so it is a tuning knob rather than a policy.
*
*   targets:  cpp/util/sort/quick_sort_common.hpp
*             -> D_INTERNAL_SORT_QUICK_INSERTION_THRESHOLD
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/cpp/util/sort/cfg_sort.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CPP_UTIL_SORT_CFG_SORT_H
#define DJINTERP_CONFIG_CPP_UTIL_SORT_CFG_SORT_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides

/*
TABLE OF CONTENTS
=================
0.    SORT CONFIGURATION
      ------------------
      1.    Defaults                     (D_CFG_SORT_QUICK_INSERTION_THRESHOLD)
      2.    Validation of the knobs
      3.    Derived values               (D_INTERNAL_SORT_*)
*/


// ===========================================================================
// 0.   SORT CONFIGURATION
// ===========================================================================

// --- 0.1  Defaults ---

// D_CFG_SORT_QUICK_INSERTION_THRESHOLD
//   brief: quicksort hands a sub-range of this many elements or fewer to
// insertion sort instead of partitioning it.  16 is the default: below it,
// insertion sort's low constant factor beats another round of partitioning.
// 0 makes quicksort partition every sub-range down to single elements.
#ifndef D_CFG_SORT_QUICK_INSERTION_THRESHOLD
#   define D_CFG_SORT_QUICK_INSERTION_THRESHOLD 16
#endif


// --- 0.2  Validation of the knobs ---

//   The ceiling keeps the value inside an int on every platform, 16-bit ones
// included; insertion sort on sub-ranges anywhere near it would be quadratic
// long before the limit mattered.
#if ( (D_CFG_NORM(D_CFG_SORT_QUICK_INSERTION_THRESHOLD) < 0) ||               \
      (D_CFG_NORM(D_CFG_SORT_QUICK_INSERTION_THRESHOLD) > 32767) )
#   error "D_CFG_SORT_QUICK_INSERTION_THRESHOLD must be 0 to 32767"
#endif


// --- 0.3  Derived values (D_INTERNAL_*) ---

// D_INTERNAL_SORT_QUICK_INSERTION_THRESHOLD
//   brief: the knob as the integer quick_sort_traits::insertion_threshold
// holds.
#define D_INTERNAL_SORT_QUICK_INSERTION_THRESHOLD                             \
    D_CFG_NORM(D_CFG_SORT_QUICK_INSERTION_THRESHOLD)


#endif  // DJINTERP_CONFIG_CPP_UTIL_SORT_CFG_SORT_H
