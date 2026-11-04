/*******************************************************************************
* djinterp [config]                                                  cfg_value.h
*
*   Build-time configuration of the generic value's boxed kinds
* (c/math/value.h): how a box counts the values that hold it. Every boxed
* kind counts its holders through value.h's d_math_boxed_hold and
* d_math_boxed_unhold, so that one setting serves the wide integer
* (value_wide.h), the matrix and the complex number (math/expr/) and every
* kind to come (S134).
*   A count that two threads change at once must be changed atomically, or
* a box shared between them, as the language's constant im is, is freed
* early or never. So the count is atomic by default wherever the compiler
* gives atomic operations on a plain integer: GCC's __atomic builtins, which
* Clang and the GCC-compatible compilers implement at every language level,
* C89 and C++98 among them, or MSVC's interlocked functions. Neither C99
* nor C++98 has atomics of its own, and C11's _Atomic and C++11's
* std::atomic are types of their own, which no C struct both languages
* share can hold; the builtins work on the size_t the boxes already have.
* A compiler with neither counts plainly, as D_MATH_BOXED_HOLDERS_ATOMIC
* then says; a program that shares values between threads there must
* retain and release them under a lock of its own.
*   The knob is a flag: each #ifndef-guarded, so a definition made before
* value.h is first included -- with -D, in cfg_custom.h, or in the including
* file -- wins over the default here.
*
*   targets:  c/math/value.h -> D_INTERNAL_MATH_VALUE_HOLDERS
*   requires: cfg_common.h (user overrides, the testing preset, the flag
*             helpers); env/env_compiler.h (D_ENV_COMPILER_HAS_ATOMIC_BUILTINS,
*             D_ENV_COMPILER_MSVC_FAMILY)
*
*
* path:      /inc/djinterp/config/c/modules/math/cfg_value.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.31
*                                                            revised: 2026.10.31
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Holders
         1.  D_CFG_MATH_VALUE_ATOMIC_HOLDERS
2.  RESOLVED
    --------
    1.  Holders
         1.  D_INTERNAL_MATH_VALUE_HOLDERS
*/

#ifndef DJINTERP_CONFIG_C_MATH_CFG_VALUE_H
#define DJINTERP_CONFIG_C_MATH_CFG_VALUE_H 1

// djinterp
#include "../../../cfg_common.h"           // D_CFG_* helpers, user overrides
#include "../../../../env/env_compiler.h"  // D_ENV_COMPILER_HAS_ATOMIC_BUILTINS,
                                           // D_ENV_COMPILER_MSVC_FAMILY


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Holders
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_MATH_VALUE_ATOMIC_HOLDERS
//   knob: 1 to count a box's holders atomically wherever the compiler can,
// so that values may be copied and given back in several threads at once;
// 0 to count them plainly everywhere, a little faster, for a program that
// never shares a value between threads. On by default, so that sharing is
// safe unless a program says it needs none.
#ifndef D_CFG_MATH_VALUE_ATOMIC_HOLDERS
    #define D_CFG_MATH_VALUE_ATOMIC_HOLDERS 1
#endif  // D_CFG_MATH_VALUE_ATOMIC_HOLDERS

// validation
#if !D_CFG_IS_BOOL(D_CFG_MATH_VALUE_ATOMIC_HOLDERS)
    #error "D_CFG_MATH_VALUE_ATOMIC_HOLDERS must be literally 0 or 1"
#endif


//==============================================================================
// 2.  RESOLVED
//==============================================================================


// 2.1    Holders
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_MATH_VALUE_HOLDERS
//   resolved: how value.h counts a box's holders: 0 plainly; 1 by the
// __atomic builtins; 2 by MSVC's _InterlockedIncrement and
// _InterlockedDecrement, where size_t is 32 bits wide; 3 by their 64-bit
// forms, where it is 64 (_WIN64). The builtins come first, as clang-cl has
// both. Not settable: set D_CFG_MATH_VALUE_ATOMIC_HOLDERS.
#if D_CFG_IS_OFF(D_CFG_MATH_VALUE_ATOMIC_HOLDERS)
    #define D_INTERNAL_MATH_VALUE_HOLDERS 0
#elif D_CFG_IS_ON(D_ENV_COMPILER_HAS_ATOMIC_BUILTINS)
    #define D_INTERNAL_MATH_VALUE_HOLDERS 1
#elif ( (D_CFG_IS_ON(D_ENV_COMPILER_MSVC_FAMILY)) &&                           \
        (defined(_WIN64)) )
    #define D_INTERNAL_MATH_VALUE_HOLDERS 3
#elif D_CFG_IS_ON(D_ENV_COMPILER_MSVC_FAMILY)
    #define D_INTERNAL_MATH_VALUE_HOLDERS 2
#else
    #define D_INTERNAL_MATH_VALUE_HOLDERS 0
#endif


#endif  // DJINTERP_CONFIG_C_MATH_CFG_VALUE_H
