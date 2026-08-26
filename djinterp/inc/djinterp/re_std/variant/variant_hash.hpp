/******************************************************************************
* re_std [variant]                                            variant_hash.hpp
*
*   hash<variant<Ts...>>.
*
*   THE INDEX MUST BE MIXED IN, not just the alternative's hash.
*   variant<int, int> can hold the value 1 in either alternative, and those two
* variants are NOT equal - operator== compares the index first. Hashing only
* the contained value would make them collide, which is legal but defeats the
* hash. Mixing the index in costs one multiply and keeps hash consistent with
* equality for duplicate alternative types.
*
*   A VALUELESS VARIANT HASHES TO A FIXED SENTINEL. std leaves the value
* unspecified; what it must not do is hash the same as any engaged variant, and
* it must not try to read an alternative that is not there.
*
*   DISPATCH IS AN INDEX IF-CHAIN, NOT visit(). Going through visit would work
* but would give the alternative's TYPE rather than its index, and with
* duplicate alternative types that is not enough to pick the right hash<T> -
* the whole reason the index matters here.
*
*   Enabled only when every alternative's hash is; std requires the disabled
* case to be a disabled specialisation rather than a hard error.
*
*   STD IS C++17; re_std IS C++11 - inherits variant's own floor.
*
* path:      /inc/djinterp/re_std/variant/variant_hash.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_VARIANT_VARIANT_HASH_
#define DJINTERP_RE_STD_VARIANT_VARIANT_HASH_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../functional/hash.hpp"
#include "./variant.hpp"

NS_RESTD
NS_INTERNAL

    // variant_hash_dispatch
    //   struct: linear index dispatch, so duplicate alternative types are
    // still hashed by the alternative actually held.
    template<size_t _Index, size_t _Size>
    struct variant_hash_dispatch
    {
        template<typename _Variant>
        static size_t apply(const _Variant& value, size_t index)
        {
            if (index == _Index)
            {
                typedef typename remove_cv<
                    typename remove_reference<
                        decltype(re_std::get<_Index>(value))>::type>::type _Alt;
                return hash<_Alt>()(re_std::get<_Index>(value));
            }
            return variant_hash_dispatch<_Index + 1, _Size>::apply(value, index);
        }
    };

    template<size_t _Size>
    struct variant_hash_dispatch<_Size, _Size>
    {
        template<typename _Variant>
        static size_t apply(const _Variant&, size_t) { return 0; }
    };

NS_END  // internal

// hash<variant<_Types...>>
//   struct: hashes the index combined with the held alternative.
template<typename... _Types>
struct hash<variant<_Types...> >
{
    typedef variant<_Types...> argument_type;
    typedef size_t             result_type;

    //   Arbitrary; chosen only to be unlikely to collide with a small hash.
    static const size_t k_valueless_hash = static_cast<size_t>(0x9E3779B9u);

    size_t operator()(const variant<_Types...>& value) const
    {
        if (value.valueless_by_exception())
        {
            return k_valueless_hash;
        }
        const size_t index = value.index();
        const size_t inner =
            internal::variant_hash_dispatch<0, sizeof...(_Types)>::apply(
                value, index);
        //   Mix the index in - see the header note on variant<int,int>.
        return inner ^ (index * static_cast<size_t>(0x9E3779B9u)
                        + (inner << 6) + (inner >> 2));
    }
};

NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_VARIANT_VARIANT_HASH_
