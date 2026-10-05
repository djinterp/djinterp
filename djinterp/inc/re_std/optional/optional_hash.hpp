/*******************************************************************************
* djinterp [re_std]                                            optional_hash.hpp
*
* optional_hash support header:
*   hash<optional<T>> specialisation.
*
*   ENABLED ONLY WHEN hash<remove_const_t<T>> IS.
*   std requires that hash<optional<T>> be a DISABLED specialisation whenever
* hash<T> is disabled, rather than a hard error.  That is what lets generic
* code ask `is_default_constructible<hash<optional<T>>>` and get a useful
* answer instead of a compile failure, so the enable_if on the call operator is
* load-bearing rather than decorative.
*
*   A DISENGAGED OPTIONAL HASHES TO A FIXED VALUE, and deliberately not to
* hash<T>() of anything: there is no value to hash, and reusing hash<T>{}(T())
* would collide every disengaged optional with the one holding a default-
* constructed T.
*
*
* path:      /inc/re_std/optional/optional_hash.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_OPTIONAL_OPTIONAL_HASH_HPP
#define RE_STD_OPTIONAL_OPTIONAL_HASH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../functional/hash.hpp"
#include "./optional.hpp"

namespace re_std
{

// hash<optional<Type>>
//   struct: hash support, enabled iff hash<Type> is.
template<typename Type>
struct hash<optional<Type> >
{
    typedef optional<Type> argument_type;
    typedef size_t          result_type;

    //   The disengaged sentinel. Any fixed value works; this one is simply
    // unlikely to be produced by hashing a small integer.
    static const size_t k_disengaged_hash = static_cast<size_t>(0x9E3779B9u);

    size_t operator()(const optional<Type>& value) const
    {
        return value.has_value()
                   ? hash<typename remove_const<Type>::type>()(*value)
                   : k_disengaged_hash;
    }
};

}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_OPTIONAL_OPTIONAL_HASH_HPP
