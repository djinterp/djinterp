/*******************************************************************************
* djinterp [c]                                                       type_info.h
*
*  Umbrella header for the bit-efficient type information system.
*  Includes the common definitions unconditionally, then pulls in either
*  the C-specific or C++-specific extensions depending on the compiler.
*
*  Module breakdown:
*    type_info_common.h  — shared types, bit layout (0-23, 48-63),
*                          X-macros, constants, builders, accessors,
*                          predefined type constants, user type ID
*                          support, extended-info structures, and
*                          utility helpers.
*    type_info_c.h       — C storage-class bits (24-31), SET macros,
*                          predefined _Generic constants, C11 _Generic
*                          type detection.
*    type_info_cpp.hpp   — C++ modifier bits (32-47) and SET macros.
*
*
* path:      /inc/djinterp/c/meta/type_info.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.06
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_C_META_TYPE_INFO_H
#define DJINTERP_C_META_TYPE_INFO_H 1

// djinterp
// common definitions — always included
#include "type_info_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)

#if D_ENV_LANG_USING_CPP
    // C++ modifier bits (32-47), feature-gated SET macros
    #include "../../core/meta/type_info_cpp.hpp"
#else
    // C storage-class bits (24-31) and C11 _Generic support
    #include "type_info_c.h"
#endif


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_META_TYPE_INFO_H
