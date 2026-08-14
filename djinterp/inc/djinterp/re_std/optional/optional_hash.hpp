/******************************************************************************
* re_std [optional]                                          optional_hash.hpp
*
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
* path:      /inc/djinterp/re_std/optional/optional_hash.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_OPTIONAL_HASH_
#define RESTD_OPTIONAL_HASH_ 1

#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../functional/hash.hpp"
#include "./optional.hpp"

NS_DJINTERP
NS_RESTD

// hash<optional<_Type>>
//   struct: hash support, enabled iff hash<_Type> is.
template<typename _Type>
struct hash<optional<_Type> >
{
    typedef optional<_Type> argument_type;
    typedef size_t          result_type;

    //   The disengaged sentinel. Any fixed value works; this one is simply
    // unlikely to be produced by hashing a small integer.
    static const size_t k_disengaged_hash = static_cast<size_t>(0x9E3779B9u);

    size_t operator()(const optional<_Type>& value) const
    {
        return value.has_value()
                   ? hash<typename remove_const<_Type>::type>()(*value)
                   : k_disengaged_hash;
    }
};

NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_OPTIONAL_HASH_
