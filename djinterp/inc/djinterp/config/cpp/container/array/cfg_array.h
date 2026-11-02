/*******************************************************************************
* djinterp [config]                                                  cfg_array.h
*
* Feature configuration for the array sub-module.
*   Resolves per-array-type filter toggles from the category defaults in
* cfg_container.h.  The toggle hierarchy within this file is:
*
*   cfg_container.h
*     D_CFG_CONTAINER_ARRAY_FILTER                (category default)
*       |
*       +-- cfg_array.h
*             |
*             |   SUB-GROUPS (inherit from ARRAY_FILTER)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
*             |      (min_array, min_array_sorted, min_circular_array,
*             |       min_ptr_array)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
*             |      (circular_array, min_circular_array)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_POINTER
*             |      (ptr_array, min_ptr_array)
*             |
*             |   PER-TYPE (inherit from most specific sub-group)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY               <- ARRAY_FILTER
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_SORTED        <- ARRAY_FILTER
*             |-- D_CFG_CONTAINER_FILTER_BYTE_ARRAY          <- ARRAY_FILTER
*             |-- D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY      <- ARRAY_CIRCULAR
*             |-- D_CFG_CONTAINER_FILTER_MIN_ARRAY           <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED    <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY  <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY       <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_PTR_ARRAY           <- ARRAY_POINTER
*             |-- D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY     <- ARRAY_FILTER
*             +-- D_CFG_CONTAINER_FILTER_STATIC_ARRAY        <- ARRAY_FILTER
*
*   When a type belongs to multiple sub-groups (e.g. min_circular_array is
* both MINIMAL and CIRCULAR), the primary parent is ARRAY_MINIMAL.  Users
* who need the cross-cut behaviour can override the per-type toggle directly.
*
*   NOTHING READS THESE GATES YET.  The one filter implemented, the
* d_contiguous_filter_* family in c/container/array/array_filter.h, serves
* every contiguous container at once and is gated by the category,
* D_CFG_CONTAINER_ARRAY_FILTER.  The sub-group and per-type gates here wait for
* per-type filters.  Until 2026.10.31 their defaults named
* D_CFG_CONTAINER_FILTER_CONTIGUOUS, which nothing defines, so each of them
* read as 0 whatever the category said; they now inherit the category.
*
* ZERO-OVERHEAD GUARANTEE:
*   All toggles resolve at compile time.  When a toggle is 0, the
* corresponding container's filter header expands to nothing.
*
* USAGE EXAMPLES:
*   Disable filter for every minimal array variant:
*       #define D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL 0
*
*   Disable filter for only circular_array (keep min_circular_array):
*       #define D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY 0
*
*   Disable filter for all arrays (entire sub-module):
*       #define D_CFG_CONTAINER_ARRAY_FILTER 0   // in cfg_container.h
*
*   targets:  the C++ arrays under cpp/container/array, none of which
*             includes it yet: its gates are reserved for filters not
*             written (D_CFG_CONTAINER_FILTER_*)
*   requires: cfg_common.h (user overrides); c/container/cfg_container.h
*             (D_CFG_CONTAINER_ARRAY_FILTER, which every gate follows)
*
*
* path:      /inc/djinterp/config/cpp/container/array/cfg_array.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.19
*                                                            revised: 2026.10.31
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CPP_CONTAINER_ARRAY_CFG_ARRAY_H
#define DJINTERP_CONFIG_CPP_CONTAINER_ARRAY_CFG_ARRAY_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides
#include "../../../c/container/cfg_container.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    SUB-GROUP TOGGLES                                     ///
///////////////////////////////////////////////////////////////////////////////
// Each sub-group inherits from D_CFG_CONTAINER_ARRAY_FILTER unless
// the user has already defined it.

// D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
//   configuration: enable filter for all minimal ("min_*") array variants.
// Minimal arrays are stripped-down implementations optimised for footprint.
// Applies to: min_array, min_array_sorted, min_circular_array,
//             min_ptr_array.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
    #define D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL                            \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL

// D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
//   configuration: enable filter for circular (ring-buffer) array variants.
// Applies to: circular_array, min_circular_array.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
    #define D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR                           \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR

// D_CFG_CONTAINER_FILTER_ARRAY_POINTER
//   configuration: enable filter for pointer-element array variants.
// Applies to: ptr_array, min_ptr_array.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_POINTER
    #define D_CFG_CONTAINER_FILTER_ARRAY_POINTER                            \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_POINTER


///////////////////////////////////////////////////////////////////////////////
///             II.   PER-TYPE TOGGLES                                      ///
///////////////////////////////////////////////////////////////////////////////
// Each per-type toggle inherits from its most specific sub-group, or from
// D_CFG_CONTAINER_ARRAY_FILTER when no sub-group applies.

// D_CFG_CONTAINER_FILTER_ARRAY
//   configuration: enable filter for dj_array (the base dynamic array).
#ifndef D_CFG_CONTAINER_FILTER_ARRAY
    #define D_CFG_CONTAINER_FILTER_ARRAY                                    \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ARRAY

// D_CFG_CONTAINER_FILTER_ARRAY_SORTED
//   configuration: enable filter for d_array_sorted.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_SORTED
    #define D_CFG_CONTAINER_FILTER_ARRAY_SORTED                             \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_SORTED

// D_CFG_CONTAINER_FILTER_BYTE_ARRAY
//   configuration: enable filter for d_byte_array.
#ifndef D_CFG_CONTAINER_FILTER_BYTE_ARRAY
    #define D_CFG_CONTAINER_FILTER_BYTE_ARRAY                               \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_BYTE_ARRAY

// D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY
//   configuration: enable filter for dj_circular_array.
#ifndef D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY
    #define D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY                           \
        D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
#endif  // D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY

// D_CFG_CONTAINER_FILTER_MIN_ARRAY
//   configuration: enable filter for d_min_array.
#ifndef D_CFG_CONTAINER_FILTER_MIN_ARRAY
    #define D_CFG_CONTAINER_FILTER_MIN_ARRAY                                \
        D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
#endif  // D_CFG_CONTAINER_FILTER_MIN_ARRAY

// D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED
//   configuration: enable filter for d_min_array_sorted.
#ifndef D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED
    #define D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED                         \
        D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
#endif  // D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED

// D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY
//   configuration: enable filter for d_min_circular_array.
// Primary parent: ARRAY_MINIMAL.  Also a member of ARRAY_CIRCULAR.
#ifndef D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY
    #define D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY                       \
        D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
#endif  // D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY

// D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY
//   configuration: enable filter for d_min_ptr_array.
// Primary parent: ARRAY_MINIMAL.  Also a member of ARRAY_POINTER.
#ifndef D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY
    #define D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY                            \
        D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
#endif  // D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY

// D_CFG_CONTAINER_FILTER_PTR_ARRAY
//   configuration: enable filter for dj_ptr_array.
#ifndef D_CFG_CONTAINER_FILTER_PTR_ARRAY
    #define D_CFG_CONTAINER_FILTER_PTR_ARRAY                                \
        D_CFG_CONTAINER_FILTER_ARRAY_POINTER
#endif  // D_CFG_CONTAINER_FILTER_PTR_ARRAY

// D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY
//   configuration: enable filter for d_segmented_array.
#ifndef D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY
    #define D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY                          \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY

// D_CFG_CONTAINER_FILTER_STATIC_ARRAY
//   configuration: enable filter for d_static_array (C++ only).
#ifndef D_CFG_CONTAINER_FILTER_STATIC_ARRAY
    #define D_CFG_CONTAINER_FILTER_STATIC_ARRAY                             \
        D_CFG_CONTAINER_ARRAY_FILTER
#endif  // D_CFG_CONTAINER_FILTER_STATIC_ARRAY

///////////////////////////////////////////////////////////////////////////////
///             III.  VALIDATION                                          ///
///////////////////////////////////////////////////////////////////////////////
// Every gate is a boolean; a misspelled value would read as 0.

#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL)
    #error "D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR)
    #error "D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_ARRAY_POINTER)
    #error "D_CFG_CONTAINER_FILTER_ARRAY_POINTER must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_ARRAY_SORTED)
    #error "D_CFG_CONTAINER_FILTER_ARRAY_SORTED must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_BYTE_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_BYTE_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_MIN_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_MIN_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED)
    #error "D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_PTR_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_PTR_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CONTAINER_FILTER_STATIC_ARRAY)
    #error "D_CFG_CONTAINER_FILTER_STATIC_ARRAY must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CPP_CONTAINER_ARRAY_CFG_ARRAY_H
