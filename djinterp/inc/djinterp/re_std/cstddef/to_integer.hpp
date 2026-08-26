/******************************************************************************
* djinterp [re_std]                                               to_integer.hpp
*
* the to_integer function template:
*   The single documented exit from byte back to the integers:
*
*       re_std::byte b = re_std::byte{0x2A};
*       int n = re_std::to_integer<int>(b);      // 42
*
*   The integer type is explicit and never deduced, which is the whole
* point -- widening or narrowing a raw byte is a decision the caller
* states rather than something that happens on the way to an overload.
*
*   THE CONSTRAINT IS SFINAE, NOT static_assert:
*   [cstddef.syn] constrains the template to integer types. Expressing
* that as enable_if (rather than a static_assert in the body) keeps
* to_integer<some_class>(b) a SUBSTITUTION FAILURE, so it drops out of
* overload resolution quietly and other candidates get their chance. A
* static_assert would be a hard error at the point of instantiation and
* would poison any surrounding detection idiom.
*
*   C++11 FLOOR: follows byte.
*
*
* path:      /inc/djinterp/re_std/cstddef/to_integer.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDDEF_TO_INTEGER_
#define DJINTERP_RE_STD_CSTDDEF_TO_INTEGER_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./byte.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_integral.hpp"


NS_RESTD

    // to_integer
    //   function: the byte's value as _IntType. Participates in overload
    // resolution only when _IntType is an integer type.
    template<typename _IntType>
    D_CONSTEXPR
    typename enable_if<is_integral<_IntType>::value, _IntType>::type
    to_integer(byte _b) D_NOEXCEPT
    {
        return static_cast<_IntType>(_b);
    }

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDDEF_TO_INTEGER_
