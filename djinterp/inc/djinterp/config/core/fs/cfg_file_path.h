/*******************************************************************************
* djinterp [config]                                              cfg_file_path.h
*
* Build-time configuration for core/fs/file_path.hpp -- djinterp::path.
*   Two knobs, both read by that one header: whether a failed allocation
* throws, and how much stack a lexical operation borrows before it allocates.
* Plus the one language-feature gate the header needs, three-way comparison.
*   The knobs lived in file_path.hpp itself until this directory existed, and
* that header said they would move here together, unchanged, when it did.
*
*   targets:  core/fs/file_path.hpp -> D_INTERNAL_PATH_THROW,
*             D_INTERNAL_PATH_STACK_BUF, D_INTERNAL_PATH_HAS_SPACESHIP
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/core/fs/cfg_file_path.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Failure reporting
         1.  D_CFG_PATH_THROW
    2.  Scratch storage
         1.  D_CFG_PATH_STACK_BUF
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_PATH_THROW
         2.  D_INTERNAL_PATH_STACK_BUF
    2.  Language features
         1.  D_INTERNAL_PATH_HAS_SPACESHIP
*/

#ifndef DJINTERP_CONFIG_CORE_FS_CFG_FILE_PATH_H
#define DJINTERP_CONFIG_CORE_FS_CFG_FILE_PATH_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins.


// 1.1    Failure reporting
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_PATH_THROW
//   knob: 1 makes the throwing constructors throw std::bad_alloc when they
// cannot allocate. Off by default: the error-code surface is the real API and
// works with -fno-exceptions.
//   ONLY A RESOURCE FAILURE THROWS, even with this on. Three things stay
// non-throwing because none of them is one: path(NULL) (a caller bug, and
// bad_alloc would be a lie about what happened); a lexical operation whose
// result does not fit (nothing was allocated -- d_path_dirname simply said
// no); and any operation on an already-invalid path (the failure already
// happened). Conflating the invalid-path model with the throwing model is what
// made `path((const char*)0)` raise bad_alloc out of a function that never
// allocated, so the two are kept apart by a private tag constructor that does
// not consult this knob at all.
#ifndef D_CFG_PATH_THROW
    #define D_CFG_PATH_THROW         0
#endif  // D_CFG_PATH_THROW

// 1.2    Scratch storage
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_PATH_STACK_BUF
//   knob: bytes of stack a lexical operation borrows before it allocates. 512
// rather than D_FILE_PATH_MAX: on Linux that is 4096, and 4 KiB per frame in a
// recursive walk is a real price for a case that essentially never happens. A
// longer path still works -- it just costs one allocation.
#ifndef D_CFG_PATH_STACK_BUF
    #define D_CFG_PATH_STACK_BUF     512
#endif  // D_CFG_PATH_STACK_BUF


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_PATH_THROW)
    #error "D_CFG_PATH_THROW must be 0 or 1"
#endif

#if ( (D_CFG_PATH_STACK_BUF + 0) <= 0 )
    #error "D_CFG_PATH_STACK_BUF must be a positive byte count"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================
// What file_path.hpp reads. Nothing outside this file resolves them.


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_PATH_THROW
//   resolved: 1 when a failed allocation throws std::bad_alloc.
#define D_INTERNAL_PATH_THROW       (D_CFG_PATH_THROW + 0)

// 3.1.2
// D_INTERNAL_PATH_STACK_BUF
//   resolved: the scratch buffer's stack capacity, in bytes.
#define D_INTERNAL_PATH_STACK_BUF   (D_CFG_PATH_STACK_BUF + 0)

// 3.2    Language features
//------------------------------------------------------------------------------
// 3.2.1
// D_INTERNAL_PATH_HAS_SPACESHIP
//   resolved: 1 when path can define operator<=>. Gated on the language
// feature-test macro -- NOT on __cplusplus, whose C++23 value varies by
// compiler; the feature-test macro is the portable signal.
#if ( (defined(__cpp_impl_three_way_comparison)) &&                            \
      (__cpp_impl_three_way_comparison >= 201907L) )
    #define D_INTERNAL_PATH_HAS_SPACESHIP 1
#else
    #define D_INTERNAL_PATH_HAS_SPACESHIP 0
#endif


#endif  // DJINTERP_CONFIG_CORE_FS_CFG_FILE_PATH_H
