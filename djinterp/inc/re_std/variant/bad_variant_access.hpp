/*******************************************************************************
* djinterp [re_std]                                       bad_variant_access.hpp
*
* bad_variant_access exception header:
*   Thrown by get<I>/get<T> when the requested alternative is not
* the active one, and by visit when called on a valueless variant.
*
*   INHERITANCE FALLBACKS:
*     exceptions on  -> inherits std::exception, as [variant.bad.access]
*                       specifies (not std::bad_cast: a catch of bad_cast
*                       must not catch it)
*     neither               -> standalone class (no base, non-virtual what())
*
*
* path:      /inc/re_std/variant/bad_variant_access.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_BAD_VARIANT_ACCESS_HPP
#define RE_STD_VARIANT_BAD_VARIANT_ACCESS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// ===========================================================================
// 0.   CONDITIONAL INCLUDES
// ===========================================================================

#if RE_STD_HAS_EXCEPTIONS
    // std
    #include <exception>
#endif

#if !RE_STD_HAS_EXCEPTIONS
    // std
    #include <cstdlib>  // std::abort, a failed access's only outcome
#endif


namespace re_std
{


// ===========================================================================
// I.   BAD_VARIANT_ACCESS
// ===========================================================================

#if RE_STD_HAS_EXCEPTIONS

class bad_variant_access : public std::exception
{
public:
    const char* what() const RE_STD_NOEXCEPT override
    {
        return "bad variant access";
    }
};

#else

class bad_variant_access
{
public:
    const char* what() const RE_STD_NOEXCEPT
    {
        return "bad variant access";
    }
};

#endif  // RE_STD_HAS_EXCEPTIONS


namespace internal
{

    // throw_bad_variant_access
    //   function: raise bad_variant_access, for the visit paths that reach a
    // valueless variant. variant_visit.hpp called it and nothing defined it.
    // With exceptions off there is no alternative to hand back, so it aborts,
    // as std's variant does in that mode. [[noreturn]] lets a caller's
    // unreachable tail end without a dummy return or throw.
    [[noreturn]] RE_STD_INLINE void throw_bad_variant_access()
    {
    #if RE_STD_HAS_EXCEPTIONS
        throw bad_variant_access();
    #else
        ::std::abort();
    #endif
    }

}  // internal


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_BAD_VARIANT_ACCESS_HPP
