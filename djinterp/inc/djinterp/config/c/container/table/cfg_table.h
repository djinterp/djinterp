/*******************************************************************************
* djinterp [config]                                                  cfg_table.h
*
*   Build-time configuration for the C table module (c/container/table): how
* wide a stored extent is, the greatest rank a build carries, and whether the
* stdlib allocation strategy is offered.
*
*   targets:  c/container/table/table_common.h -> D_CFG_TABLE_EXTENT_64,
*                                                 D_CFG_TABLE_MAX_RANK,
*                                                 D_INTERNAL_TABLE_EXTENT,
*                                                 D_INTERNAL_TABLE_EXTENT_MAX
*             c/container/table/table.h        -> D_CFG_TABLE_STDLIB_ALLOC
*   requires: cfg_common.h (helpers, user overrides); cfg_container.h (the
*             container aggregate these knobs inherit)
*
*   MOVED HERE 2026.10.31 from table_common.h and table.h, which defaulted
* them in the module.  The config README keeps every default in one cfg file,
* and container.h already reads its own knobs from cfg_container.h on that
* rule; the table now does the same.  Not to be confused with
* config/cpp/container/table/cfg_table.h, which configures the C++ table DSL.
*
*
* path:      /inc/djinterp/config/c/container/table/cfg_table.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.31
*                                                            revised: 2026.10.31
*******************************************************************************/

#ifndef DJINTERP_CONFIG_C_CONTAINER_TABLE_CFG_TABLE_H
#define DJINTERP_CONFIG_C_CONTAINER_TABLE_CFG_TABLE_H 1

// djinterp
#include "../../../cfg_common.h"     // D_CFG_* helpers, user overrides
#include "../cfg_container.h"        // D_CFG_CONTAINER_ALL, the aggregate


//==============================================================================
// 1.  KNOBS
//==============================================================================

// D_CFG_TABLE_EXTENT_64
//   knob: opt in to 64-bit stored extents. Off by default: 32-bit extents
// give a 24-byte dj_table, a compact wire form, and a ceiling of 4294967295
// rows and columns. It does not affect the width of any index parameter, which
// is always size_t, so turning it on changes the representation and nothing
// else. Inherits D_CFG_CONTAINER_ALL, then off.
#ifndef D_CFG_TABLE_EXTENT_64
#   if defined(D_CFG_CONTAINER_ALL)
#       define D_CFG_TABLE_EXTENT_64        D_CFG_CONTAINER_ALL
#   else
#       define D_CFG_TABLE_EXTENT_64        0
#   endif
#endif

// D_CFG_TABLE_MAX_RANK
//   knob: the greatest rank k this build carries (value model). A domain, an
// index and a region each hold their coordinates INLINE, so the cap is what
// lets them be returned by value, compared by value, and serialized without an
// allocator. Four covers the row-and-column table (k = 2) and the usual tensor
// shapes with room to spare; raise it where a build needs more and pay the
// bytes. A rank past it is refused with D_TABLE_STATUS_RANK.
#ifndef D_CFG_TABLE_MAX_RANK
#   define D_CFG_TABLE_MAX_RANK         4
#endif

// D_CFG_TABLE_STDLIB_ALLOC
//   knob: offer the convenience allocation strategy backed by realloc/free
// (1, the default), or leave it out (0). On by default because naming a
// strategy is not hiding one; off for a freestanding build, which then
// supplies its own and loses nothing else.
#ifndef D_CFG_TABLE_STDLIB_ALLOC
#   define D_CFG_TABLE_STDLIB_ALLOC     1
#endif


//==============================================================================
// 2.  VALIDATION
//==============================================================================

#if !D_CFG_IS_BOOL(D_CFG_TABLE_EXTENT_64)
#   error "D_CFG_TABLE_EXTENT_64 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TABLE_STDLIB_ALLOC)
#   error "D_CFG_TABLE_STDLIB_ALLOC must be 0 or 1"
#endif
#if (D_CFG_TABLE_MAX_RANK < 1)
#   error "D_CFG_TABLE_MAX_RANK must be at least 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================

// D_INTERNAL_TABLE_EXTENT
//   macro: the resolved spelling of a STORED extent -- a row count, a column
// count, or a cell size, as it sits in a struct and on the wire. A macro
// rather than a typedef because C must not hide what a type is, and because
// the choice must be visible to the preprocessor where table_common.h declares
// its structs. Its types come from re_std's dstdint.h, which table_common.h
// includes.
// D_INTERNAL_TABLE_EXTENT_MAX
//   macro: the largest value D_INTERNAL_TABLE_EXTENT holds.
#ifndef D_INTERNAL_TABLE_EXTENT
#   if D_CFG_IS_ON(D_CFG_TABLE_EXTENT_64)
#       define D_INTERNAL_TABLE_EXTENT      uint64_t
#       define D_INTERNAL_TABLE_EXTENT_MAX  UINT64_MAX
#   else
#       define D_INTERNAL_TABLE_EXTENT      uint32_t
#       define D_INTERNAL_TABLE_EXTENT_MAX  UINT32_MAX
#   endif
#endif


#endif  // DJINTERP_CONFIG_C_CONTAINER_TABLE_CFG_TABLE_H
