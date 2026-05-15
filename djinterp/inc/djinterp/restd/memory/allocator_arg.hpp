/***********************************************************************
* restd                                                  allocator_arg.hpp
*
* tag type and constant marking allocator-extended ctors:
*   allocator_arg_t  -  empty tag struct.
*   allocator_arg    -  constexpr instance of allocator_arg_t.
*
* both are used to disambiguate constructors that take an allocator as
* their first argument (uses-allocator construction). pair, tuple,
* function, optional, etc. all have ctors of the form
*
*     T(allocator_arg_t, const Allocator&, ...)
*
* and the tag is what makes the overload resolvable at the call site:
*
*     restd::tuple<int, std::string> t(allocator_arg, my_alloc, 1, "x");
*                                       ^^^^^^^^^^^^^
*                                       this disambiguator
*
* tier behaviour for the allocator_arg constant (matches restd::nullopt):
*   C++17+   inline constexpr  -  one address across all TUs.
*   C++11+   constexpr         -  internal linkage (per-TU address).
*                                 Fine: tag is used by value, not address.
*   C++98/03 static const      -  internal linkage. Same rationale.
*
*
* path:      /inc/restd/memory/allocator_arg.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_ALLOCATOR_ARG_
#define RESTD_MEMORY_ALLOCATOR_ARG_ 1

#include "djinterp.hpp"


namespace restd
{

// =============================================================================
// allocator_arg_t
// =============================================================================

// allocator_arg_t
//   struct: empty disambiguator type. The explicit default ctor on
//           C++11+ matches the standard and prevents accidental
//           construction from `{}` in some contexts.
struct allocator_arg_t
{
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        D_CONSTEXPR explicit allocator_arg_t() D_NOEXCEPT
        {
        }
    #else
        // C++98/03: no `explicit` on default ctor (C++11 refinement).
        // Code value-initialises this with `allocator_arg_t()`, which
        // works on every tier.
        allocator_arg_t()
        {
        }
    #endif
};


// =============================================================================
// allocator_arg
// =============================================================================

// allocator_arg
//   constant: the canonical instance.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    inline constexpr allocator_arg_t allocator_arg{};

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

    // constexpr at namespace scope is internal linkage in C++11/14.
    // That is fine here: the tag is used by value at call sites, never
    // by address-comparison across TUs.
    constexpr allocator_arg_t allocator_arg = allocator_arg_t();

#else

    static const allocator_arg_t allocator_arg = allocator_arg_t();

#endif


}  // namespace restd

#endif  // RESTD_MEMORY_ALLOCATOR_ARG_
