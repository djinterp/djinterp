/*******************************************************************************
* djinterp [re_std]                                            make_optional.hpp
*
* make_optional factory header:
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
*
* path:      /inc/re_std/optional/make_optional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_OPTIONAL_MAKE_OPTIONAL_HPP
#define RE_STD_OPTIONAL_MAKE_OPTIONAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./optional.hpp"

namespace re_std
{

// make_optional
//   function: an engaged optional holding a decayed copy of value.
template<typename Type>
RE_STD_CONSTEXPR optional<typename decay<Type>::type>
make_optional(Type&& value)
{
    return optional<typename decay<Type>::type>(
        static_cast<Type&&>(value));
}

// make_optional
//   function: an engaged optional whose value is constructed in place.
template<typename Type, typename... Args>
RE_STD_CONSTEXPR optional<Type> make_optional(Args&&... args)
{
    return optional<Type>(in_place, static_cast<Args&&>(args)...);
}

}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_OPTIONAL_MAKE_OPTIONAL_HPP
