/******************************************************************************
* djinterp [core]                                              cpp_named03.h
*
* C++03 named requirement type traits:
*   C++03 introduced no new metaprogramming capabilities relevant to named
* requirement detection. This header includes cpp_named98.h unmodified and
* serves as a version-consistent entry point for C++03 compilation targets.
*
* SUPPORTED REQUIREMENTS:
*   - DefaultConstructible  (is_default_constructible)
*   - CopyConstructible     (is_copy_constructible)
*   - CopyAssignable        (is_copy_assignable)
*   - Destructible          (is_destructible)
*
* UNSUPPORTED (no rvalue references in C++03):
*   - MoveConstructible
*   - MoveAssignable
*
* path:      \inc\cpp_named03.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED03_
#define DJINTERP_CPP_NAMED03_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named03.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named03.h can only be used in C++ compilation mode"
#endif

// C++03 adds no relevant metaprogramming features over C++98
#include "cpp_named98.h"


#endif  // DJINTERP_CPP_NAMED03_
