/******************************************************************************
* djinterp [core]                                              type_info.h
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
*    type_info_cpp.h     — C++ modifier bits (32-47) and SET macros.
*
* path:      /inc/c/type_info.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.06
******************************************************************************/

#ifndef DJINTERP_C_TYPE_INFO_
#define DJINTERP_C_TYPE_INFO_ 1

// common definitions — always included
#include "type_info_common.h"

#if D_ENV_LANG_USING_CPP
    // C++ modifier bits (32-47), feature-gated SET macros
    #include "type_info_cpp.h"
#else
    // C storage-class bits (24-31) and C11 _Generic support
    #include "type_info_c.h"
#endif


#endif  // DJINTERP_C_TYPE_INFO_
