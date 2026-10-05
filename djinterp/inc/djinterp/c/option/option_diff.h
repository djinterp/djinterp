/*******************************************************************************
* djinterp [c]                                                     option_diff.h
*
* The diff algebra's front door. Include THIS. See option.h for the umbrella
* pattern and for the note on the provisional dispatch spelling.
*
* NO C FACE. Same reasoning as option_override.h: the diff functions take a
* destination and a capacity, and there is no C sugar over that shape worth a
* header. The C++ face keeps its compile-time half, which computes at
* translation time what the core computes at run time.
*
*
* path:      /inc/djinterp/c/option/option_diff.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_DIFF_H
#define DJINTERP_C_OPTION_OPTION_DIFF_H 1

// djinterp
#include "./option_override.h"
#include "./option_diff_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)

// the C++ face, from C++17, decision 3.6's floor for option; below it a C++
// caller has the C core
#if ( (D_ENV_LANG_USING_CPP) &&                                           \
      (D_ENV_LANG_IS_CPP17_OR_HIGHER) )
#   include "../../core/option/option_diff.hpp"
#endif  // C++17 and up


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_DIFF_H
