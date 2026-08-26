/******************************************************************************
* djinterp [re_std]                                         variant_alternative.hpp
*
* variant_alternative trait header:
*   variant_alternative<I, V>::type yields the I-th alternative
* type of variant V.
*
*     variant_alternative<1, variant<int, double, string>>::type == double
*
*   Out-of-range I produces a compile error (no specialisation matches).
*
*
* path:      /inc/djinterp/re_std/variant/variant_alternative.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RE_STD_VARIANT_ALTERNATIVE_
#define DJINTERP_RE_STD_VARIANT_ALTERNATIVE_ 1

#include <cstddef>
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


template<typename... _Types> class variant;


// ===========================================================================
// I.   VARIANT_ALTERNATIVE — primary
// ===========================================================================

template<std::size_t _I,
         typename     _V>
struct variant_alternative;


// ===========================================================================
// II.  VARIANT<Types...> SPECIALISATION — recursive walk
// ===========================================================================

NS_INTERNAL

    // type_at<I, Types...> — picks the I-th type from a pack.
    // Recursive: peel off the head, decrement I.
    template<std::size_t _I, typename _Head, typename... _Tail>
    struct type_at
    {
        typedef typename type_at<_I - 1, _Tail...>::type type;
    };

    template<typename _Head, typename... _Tail>
    struct type_at<0, _Head, _Tail...>
    {
        typedef _Head type;
    };

NS_END  // internal


template<std::size_t _I,
         typename... _Types>
struct variant_alternative<_I, variant<_Types...> >
{
    static_assert(_I < sizeof...(_Types),
                  "re_std::variant_alternative: index out of bounds");
    typedef typename internal::type_at<_I, _Types...>::type type;
};


// ===========================================================================
// III. CV-QUALIFIED PASSTHROUGH (LWG-style)
// ===========================================================================

template<std::size_t _I,
         typename     _V>
struct variant_alternative<_I, _V const>
{
    typedef typename variant_alternative<_I, _V>::type const type;
};

template<std::size_t _I,
         typename     _V>
struct variant_alternative<_I, _V volatile>
{
    typedef typename variant_alternative<_I, _V>::type volatile type;
};

template<std::size_t _I,
         typename     _V>
struct variant_alternative<_I, _V const volatile>
{
    typedef typename variant_alternative<_I, _V>::type const volatile type;
};


// ===========================================================================
// IV.  VARIANT_ALTERNATIVE_T (C++14+)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

template<std::size_t _I,
         typename     _V>
using variant_alternative_t = typename variant_alternative<_I, _V>::type;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_VARIANT_ALTERNATIVE_
