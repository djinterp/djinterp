/******************************************************************************
* djinterp [restd]                                          bad_variant_access.hpp
*
* bad_variant_access exception header:
*   Thrown by get<I>/get<T> when the requested alternative is not
* the active one, and by visit when called on a valueless variant.
*
*   INHERITANCE FALLBACKS (mirrors bad_any_cast / bad_expected_access):
*     <typeinfo>  available -> inherits std::bad_cast (-> std::exception)
*     <exception> available -> inherits std::exception
*     neither               -> standalone class (no base, non-virtual what())
*
*
* path:      /inc/djinterp/re_std/variant/bad_variant_access.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_BAD_VARIANT_ACCESS_
#define DJINTERP_RESTD_BAD_VARIANT_ACCESS_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// ===========================================================================
// 0.   CONDITIONAL INCLUDES
// ===========================================================================

#if D_ENV_CPP98_HAS_TYPEINFO
    #include <typeinfo>
#elif D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#endif


NS_RESTD


// ===========================================================================
// I.   BAD_VARIANT_ACCESS
// ===========================================================================

#if D_ENV_CPP98_HAS_TYPEINFO

class bad_variant_access : public std::bad_cast
{
public:
    const char* what() const D_NOEXCEPT override
    {
        return "bad variant access";
    }
};

#elif D_ENV_CPP98_HAS_EXCEPTION

class bad_variant_access : public std::exception
{
public:
    const char* what() const D_NOEXCEPT override
    {
        return "bad variant access";
    }
};

#else

class bad_variant_access
{
public:
    const char* what() const D_NOEXCEPT
    {
        return "bad variant access";
    }
};

#endif  // D_ENV_CPP98_HAS_TYPEINFO / D_ENV_CPP98_HAS_EXCEPTION


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_BAD_VARIANT_ACCESS_
