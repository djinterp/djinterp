/******************************************************************************
* re_std [optional]                                           make_optional.hpp
*
*   optional<T> factories.
*
*   Three overloads: deduce from a value, construct in place from an argument
* pack, and construct in place from an initializer_list plus a pack.  The
* in-place forms exist because `optional<T>(args...)` cannot express them - a
* constructor call with several arguments is ambiguous with the converting
* constructor - which is why std spells them with the in_place tag.
*
*   The deducing overload strips cv and reference (decay), so
* make_optional(x) always yields optional of a value type, never
* optional<const T&>.
*
* path:      /inc/djinterp/re_std/optional/make_optional.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_OPTIONAL_MAKE_OPTIONAL_
#define DJINTERP_RE_STD_OPTIONAL_MAKE_OPTIONAL_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./optional.hpp"

NS_RESTD

// make_optional
//   function: an engaged optional holding a decayed copy of value.
template<typename _Type>
D_CONSTEXPR optional<typename decay<_Type>::type>
make_optional(_Type&& value)
{
    return optional<typename decay<_Type>::type>(
        static_cast<_Type&&>(value));
}

// make_optional
//   function: an engaged optional whose value is constructed in place.
template<typename _Type, typename... _Args>
D_CONSTEXPR optional<_Type> make_optional(_Args&&... args)
{
    return optional<_Type>(in_place, static_cast<_Args&&>(args)...);
}

NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_OPTIONAL_MAKE_OPTIONAL_
