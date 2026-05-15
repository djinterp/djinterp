/***********************************************************************
* restd                                                    unreachable.hpp
*
* unreachable():
*   Asserts to the compiler that this code point cannot be reached.
* If control flow ever does reach a call to unreachable(), behaviour
* is UNDEFINED. The compiler may use this hint for optimisation
* (eliminate dead branches, mark switch defaults, etc.).
*
*   Implementation:
*     GCC, Clang, Intel:  __builtin_unreachable()
*     MSVC:               __assume(0)
*     other:              fallback to abort() or an infinite loop
*
* added in std C++23.
*
*
* path:      /inc/restd/utility/unreachable.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_UNREACHABLE_
#define RESTD_UTILITY_UNREACHABLE_ 1

#include "djinterp.hpp"


// D_RESTD_HAS_BUILTIN_UNREACHABLE
//   Detection macro for the compiler intrinsic. Override by
//   predefining before include.
#ifndef D_RESTD_HAS_BUILTIN_UNREACHABLE
    #if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
        #define D_RESTD_HAS_BUILTIN_UNREACHABLE   1
        #define D_RESTD_UNREACHABLE_FLAVOUR_GCC   1
    #elif defined(_MSC_VER)
        #define D_RESTD_HAS_BUILTIN_UNREACHABLE   1
        #define D_RESTD_UNREACHABLE_FLAVOUR_MSVC  1
    #else
        #define D_RESTD_HAS_BUILTIN_UNREACHABLE   0
    #endif
#endif


#if !D_RESTD_HAS_BUILTIN_UNREACHABLE
    // Need <cstdlib> for the abort() fallback.
    #include <cstdlib>
#endif


namespace restd
{

// unreachable
//   Always [[noreturn]]; the compiler treats subsequent code as
//   dead. We declare it so callers can rely on the noreturn-ness
//   even on the fallback compiler (which would otherwise let
//   control flow through).
D_INLINE
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    [[noreturn]]
#endif
void unreachable() D_NOEXCEPT
{
    #if D_RESTD_HAS_BUILTIN_UNREACHABLE
      #if defined(D_RESTD_UNREACHABLE_FLAVOUR_GCC)
        __builtin_unreachable();
      #elif defined(D_RESTD_UNREACHABLE_FLAVOUR_MSVC)
        __assume(0);
      #endif
    #else
        // Fallback: abort the program. This is observable, unlike
        // the intrinsic forms, but at least it's not UB-flavoured
        // silent miscompilation.
        std::abort();
    #endif
}


}  // namespace restd

#endif  // RESTD_UTILITY_UNREACHABLE_
