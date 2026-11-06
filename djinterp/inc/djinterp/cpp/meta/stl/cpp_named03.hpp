/*******************************************************************************
* djinterp [cpp]                                                 cpp_named03.hpp
*
* C++03 named requirement type traits:
*   C++03 introduced no new metaprogramming capabilities relevant to named
* requirement detection. This header includes cpp_named98.hpp unmodified and
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
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named03.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED03_HPP
#define DJINTERP_META_STL_CPP_NAMED03_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named03.hpp can only be used in C++ compilation mode"
#endif

// C++03 adds no relevant metaprogramming features over C++98
#include "cpp_named98.hpp"


#endif  // DJINTERP_META_STL_CPP_NAMED03_HPP
