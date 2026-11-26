/*******************************************************************************
* djinterp [re_std]                                      variant_alternative.hpp
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
* path:      /inc/re_std/cpp/variant/variant_alternative.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VARIANT_ALTERNATIVE — primary
    -----------------------------
2.  VARIANT<Types...> SPECIALISATION — recursive walk
    -------------------------------------------------
3.  CV-QUALIFIED PASSTHROUGH (LWG-style)
    ------------------------------------
4.  VARIANT_ALTERNATIVE_T (C++14+)
    ------------------------------
*/

#ifndef RE_STD_VARIANT_VARIANT_ALTERNATIVE_HPP
#define RE_STD_VARIANT_VARIANT_ALTERNATIVE_HPP 1

// std
#include <cstddef>
// re_std
#include "../../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


template<typename... Types> class variant;


//==============================================================================
// 1.  VARIANT_ALTERNATIVE — primary
//==============================================================================

template<std::size_t I,
         typename     V>
struct variant_alternative;


//==============================================================================
// 2.  VARIANT<Types...> SPECIALISATION — recursive walk
//==============================================================================

namespace internal
{

    // type_at<I, Types...> — picks the I-th type from a pack.
    // Recursive: peel off the head, decrement I.
    template<std::size_t I, typename Head, typename... Tail>
    struct type_at
    {
        typedef typename type_at<I - 1, Tail...>::type type;
    };

    template<typename Head, typename... Tail>
    struct type_at<0, Head, Tail...>
    {
        typedef Head type;
    };

}  // internal


template<std::size_t I,
         typename... Types>
struct variant_alternative<I, variant<Types...> >
{
    static_assert(I < sizeof...(Types),
                  "re_std::variant_alternative: index out of bounds");
    typedef typename internal::type_at<I, Types...>::type type;
};


//==============================================================================
// 3.  CV-QUALIFIED PASSTHROUGH (LWG-style)
//==============================================================================

template<std::size_t I,
         typename     V>
struct variant_alternative<I, V const>
{
    typedef typename variant_alternative<I, V>::type const type;
};

template<std::size_t I,
         typename     V>
struct variant_alternative<I, V volatile>
{
    typedef typename variant_alternative<I, V>::type volatile type;
};

template<std::size_t I,
         typename     V>
struct variant_alternative<I, V const volatile>
{
    typedef typename variant_alternative<I, V>::type const volatile type;
};


//==============================================================================
// 4.  VARIANT_ALTERNATIVE_T (C++14+)
//==============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

template<std::size_t I,
         typename     V>
using variant_alternative_t = typename variant_alternative<I, V>::type;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_ALTERNATIVE_HPP
