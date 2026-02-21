/******************************************************************************
* djinterp [container]                                          array_config.h
*
* Feature configuration for the array sub-module.
*   Resolves per-array-type filter toggles from the category defaults in
* container_config.h.  The toggle hierarchy within this file is:
*
*   container_config.h
*     D_CFG_CONTAINER_FILTER_CONTIGUOUS           (category default)
*       |
*       +-- array_config.h
*             |
*             |   SUB-GROUPS (inherit from CONTIGUOUS)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
*             |      (min_array, min_array_sorted, min_circular_array,
*             |       min_ptr_array)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
*             |      (circular_array, min_circular_array)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_POINTER
*             |      (ptr_array, min_ptr_array)
*             |
*             |   PER-TYPE (inherit from most specific sub-group)
*             |-- D_CFG_CONTAINER_FILTER_ARRAY               <- CONTIGUOUS
*             |-- D_CFG_CONTAINER_FILTER_ARRAY_SORTED        <- CONTIGUOUS
*             |-- D_CFG_CONTAINER_FILTER_BYTE_ARRAY          <- CONTIGUOUS
*             |-- D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY      <- ARRAY_CIRCULAR
*             |-- D_CFG_CONTAINER_FILTER_MIN_ARRAY           <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_MIN_ARRAY_SORTED    <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_MIN_CIRCULAR_ARRAY  <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_MIN_PTR_ARRAY       <- ARRAY_MINIMAL
*             |-- D_CFG_CONTAINER_FILTER_PTR_ARRAY           <- ARRAY_POINTER
*             |-- D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY     <- CONTIGUOUS
*             +-- D_CFG_CONTAINER_FILTER_STATIC_ARRAY        <- CONTIGUOUS
*
*   When a type belongs to multiple sub-groups (e.g. min_circular_array is
* both MINIMAL and CIRCULAR), the primary parent is ARRAY_MINIMAL.  Users
* who need the cross-cut behaviour can override the per-type toggle directly.
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
*       #define D_CFG_CONTAINER_FILTER_CONTIGUOUS 0   // in container_config.h
*
*
* path:      \inc\container\array\array_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.19
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ARRAY_CONFIG_
#define DJINTERP_C_CONTAINER_ARRAY_CONFIG_ 1

#include "..\container_config.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    SUB-GROUP TOGGLES                                     ///
///////////////////////////////////////////////////////////////////////////////
// Each sub-group inherits from D_CFG_CONTAINER_FILTER_CONTIGUOUS unless
// the user has already defined it.

// D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
//   configuration: enable filter for all minimal ("min_*") array variants.
// Minimal arrays are stripped-down implementations optimised for footprint.
// Applies to: min_array, min_array_sorted, min_circular_array,
//             min_ptr_array.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL
    #define D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL                            \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_MINIMAL

// D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
//   configuration: enable filter for circular (ring-buffer) array variants.
// Applies to: circular_array, min_circular_array.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR
    #define D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR                           \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_CIRCULAR

// D_CFG_CONTAINER_FILTER_ARRAY_POINTER
//   configuration: enable filter for pointer-element array variants.
// Applies to: ptr_array, min_ptr_array.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_POINTER
    #define D_CFG_CONTAINER_FILTER_ARRAY_POINTER                            \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_POINTER


///////////////////////////////////////////////////////////////////////////////
///             II.   PER-TYPE TOGGLES                                      ///
///////////////////////////////////////////////////////////////////////////////
// Each per-type toggle inherits from its most specific sub-group, or from
// D_CFG_CONTAINER_FILTER_CONTIGUOUS when no sub-group applies.

// D_CFG_CONTAINER_FILTER_ARRAY
//   configuration: enable filter for d_array (the base dynamic array).
#ifndef D_CFG_CONTAINER_FILTER_ARRAY
    #define D_CFG_CONTAINER_FILTER_ARRAY                                    \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_ARRAY

// D_CFG_CONTAINER_FILTER_ARRAY_SORTED
//   configuration: enable filter for d_array_sorted.
#ifndef D_CFG_CONTAINER_FILTER_ARRAY_SORTED
    #define D_CFG_CONTAINER_FILTER_ARRAY_SORTED                             \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_ARRAY_SORTED

// D_CFG_CONTAINER_FILTER_BYTE_ARRAY
//   configuration: enable filter for d_byte_array.
#ifndef D_CFG_CONTAINER_FILTER_BYTE_ARRAY
    #define D_CFG_CONTAINER_FILTER_BYTE_ARRAY                               \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_BYTE_ARRAY

// D_CFG_CONTAINER_FILTER_CIRCULAR_ARRAY
//   configuration: enable filter for d_circular_array.
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
//   configuration: enable filter for d_ptr_array.
#ifndef D_CFG_CONTAINER_FILTER_PTR_ARRAY
    #define D_CFG_CONTAINER_FILTER_PTR_ARRAY                                \
        D_CFG_CONTAINER_FILTER_ARRAY_POINTER
#endif  // D_CFG_CONTAINER_FILTER_PTR_ARRAY

// D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY
//   configuration: enable filter for d_segmented_array.
#ifndef D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY
    #define D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY                          \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_SEGMENTED_ARRAY

// D_CFG_CONTAINER_FILTER_STATIC_ARRAY
//   configuration: enable filter for d_static_array (C++ only).
#ifndef D_CFG_CONTAINER_FILTER_STATIC_ARRAY
    #define D_CFG_CONTAINER_FILTER_STATIC_ARRAY                             \
        D_CFG_CONTAINER_FILTER_CONTIGUOUS
#endif  // D_CFG_CONTAINER_FILTER_STATIC_ARRAY


#endif  // DJINTERP_C_CONTAINER_ARRAY_CONFIG_
