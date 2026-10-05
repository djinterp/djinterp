/*******************************************************************************
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
* path:      /inc/re_std/cstddef/to_integer.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDDEF_TO_INTEGER_HPP
#define RE_STD_CSTDDEF_TO_INTEGER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./byte.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_integral.hpp"


namespace re_std
{

    // to_integer
    //   function: the byte's value as IntType. Participates in overload
    // resolution only when IntType is an integer type.
    template<typename IntType>
    RE_STD_CONSTEXPR
    typename enable_if<is_integral<IntType>::value, IntType>::type
    to_integer(byte _b) RE_STD_NOEXCEPT
    {
        return static_cast<IntType>(_b);
    }

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CSTDDEF_TO_INTEGER_HPP
