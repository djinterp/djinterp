/******************************************************************************
* djinterp [restd]                                                  pair_get.hpp
*
* pair get<> overloads:
*   Index-based and type-based element access for restd::pair, mirroring
* the tuple_get surface. With this header in scope, pair fully
* participates in the tuple protocol — structured bindings work
* (auto [a, b] = somepair), apply/make_from_tuple accept pair, and
* generic algorithms written against tuple_size/tuple_element/get can
* consume pair transparently.
*
*   INDEX-BASED (C++11+):
*     get<0>(p)        -> reference to p.first
*     get<1>(p)        -> reference to p.second
*   Returns the appropriate reference category (T1&, const T1&, T1&&,
*   const T1&&) for the pair's value category.
*
*   TYPE-BASED (C++14+):
*     get<T>(p)        -> reference to the (unique) element of type T.
*   Ill-formed when _T1 == _T2 (the element is not unique) — caught by
*   enable_if on is_same<_T1, _T2>::value == false.
*
*   PORTABILITY:
*   Requires C++11+ for index-based form (pair itself is C++98+ but
* the rvalue-reference categories are C++11+); the type-based form
* additionally requires C++14+ alias-template machinery. On C++98/03
* this header expands to nothing — pair is still usable directly via
* its .first / .second members.
*
*
* path:      /inc/djinterp/re_std/tuple/pair_get.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_UTILITY_PAIR_GET_
#define DJINTERP_RESTD_UTILITY_PAIR_GET_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: rvalue-ref forms below need C++11+. The whole header guards
// on rvalue-refs; pair pre-existed in C++98 but the get<I>(p&&) forms
// returning T&& are inherently C++11+.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// std
#include <cstddef>
// djinterp
#include "./pair.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"


NS_RESTD


// =============================================================================
// I.   GET BY INDEX
// =============================================================================
// The index-based form uses ordinary function overloading on the
// non-type template parameter via tag-dispatch through internal
// helpers. The dispatch picks first vs. second based on _I.

NS_INTERNAL

    // pair_get_helper
    //   helper: tag-dispatched accessor for pair. Specialised on
    // _I in {0, 1}; out-of-range _I has no specialisation and is
    // SFINAE-rejected at the call site.
    template<std::size_t _I>
    struct pair_get_helper;

    template<>
    struct pair_get_helper<0>
    {
        template<typename _T1, typename _T2>
        D_STATIC D_CONSTEXPR
        _T1&
        lref(
            pair<_T1, _T2>& _p
        ) D_NOEXCEPT
        {
            return _p.first;
        }

        template<typename _T1, typename _T2>
        D_STATIC D_CONSTEXPR
        const _T1&
        clref(
            const pair<_T1, _T2>& _p
        ) D_NOEXCEPT
        {
            return _p.first;
        }
    };

    template<>
    struct pair_get_helper<1>
    {
        template<typename _T1, typename _T2>
        D_STATIC D_CONSTEXPR
        _T2&
        lref(
            pair<_T1, _T2>& _p
        ) D_NOEXCEPT
        {
            return _p.second;
        }

        template<typename _T1, typename _T2>
        D_STATIC D_CONSTEXPR
        const _T2&
        clref(
            const pair<_T1, _T2>& _p
        ) D_NOEXCEPT
        {
            return _p.second;
        }
    };


    // pair_element_for_index
    //   trait: element type at index _I (0 or 1). Mirrors the
    // tuple_element<I, pair<T1,T2>> specialisation in
    // pair_tuple_element.hpp but is a small internal trait to avoid
    // a hard #include dependency on tuple_element here (get can be
    // used independently of tuple_element).
    template<std::size_t _I,
             typename    _T1,
             typename    _T2>
    struct pair_element_for_index;

    template<typename _T1, typename _T2>
    struct pair_element_for_index<0, _T1, _T2>
    {
        typedef _T1 type;
    };

    template<typename _T1, typename _T2>
    struct pair_element_for_index<1, _T1, _T2>
    {
        typedef _T2 type;
    };

NS_END  // internal


// get<I>(pair&)
//   function: yields lvalue reference to the I-th element.
template<std::size_t _I,
         typename    _T1,
         typename    _T2>
D_CONSTEXPR
typename internal::pair_element_for_index<_I, _T1, _T2>::type&
get(
    pair<_T1, _T2>& _p
) D_NOEXCEPT
{
    return internal::pair_get_helper<_I>::lref(_p);
}

// get<I>(const pair&)
template<std::size_t _I,
         typename    _T1,
         typename    _T2>
D_CONSTEXPR
const typename internal::pair_element_for_index<_I, _T1, _T2>::type&
get(
    const pair<_T1, _T2>& _p
) D_NOEXCEPT
{
    return internal::pair_get_helper<_I>::clref(_p);
}

// get<I>(pair&&)
template<std::size_t _I,
         typename    _T1,
         typename    _T2>
D_CONSTEXPR
typename internal::pair_element_for_index<_I, _T1, _T2>::type&&
get(
    pair<_T1, _T2>&& _p
) D_NOEXCEPT
{
    typedef typename internal::pair_element_for_index<_I, _T1, _T2>::type _E;
    return static_cast<_E&&>(internal::pair_get_helper<_I>::lref(_p));
}

// get<I>(const pair&&)
template<std::size_t _I,
         typename    _T1,
         typename    _T2>
D_CONSTEXPR
const typename internal::pair_element_for_index<_I, _T1, _T2>::type&&
get(
    const pair<_T1, _T2>&& _p
) D_NOEXCEPT
{
    typedef typename internal::pair_element_for_index<_I, _T1, _T2>::type _E;
    return static_cast<const _E&&>(internal::pair_get_helper<_I>::clref(_p));
}


// =============================================================================
// II.  GET BY TYPE  (C++14+)
// =============================================================================
// SFINAE-rejected when _T1 == _T2 (the requested type is not unique).
// For each value category, two overloads dispatch on which member
// matches _T.

#if D_ENV_LANG_IS_CPP14_OR_HIGHER


// get<T>(pair&) — _T matches _T1
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T1>::value && !is_same<_T1, _T2>::value,
    _T&
>::type
get(
    pair<_T1, _T2>& _p
) D_NOEXCEPT
{
    return _p.first;
}

// get<T>(pair&) — _T matches _T2
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T2>::value && !is_same<_T1, _T2>::value,
    _T&
>::type
get(
    pair<_T1, _T2>& _p
) D_NOEXCEPT
{
    return _p.second;
}

// get<T>(const pair&) — _T matches _T1
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T1>::value && !is_same<_T1, _T2>::value,
    const _T&
>::type
get(
    const pair<_T1, _T2>& _p
) D_NOEXCEPT
{
    return _p.first;
}

// get<T>(const pair&) — _T matches _T2
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T2>::value && !is_same<_T1, _T2>::value,
    const _T&
>::type
get(
    const pair<_T1, _T2>& _p
) D_NOEXCEPT
{
    return _p.second;
}

// get<T>(pair&&) — _T matches _T1
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T1>::value && !is_same<_T1, _T2>::value,
    _T&&
>::type
get(
    pair<_T1, _T2>&& _p
) D_NOEXCEPT
{
    return static_cast<_T&&>(_p.first);
}

// get<T>(pair&&) — _T matches _T2
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T2>::value && !is_same<_T1, _T2>::value,
    _T&&
>::type
get(
    pair<_T1, _T2>&& _p
) D_NOEXCEPT
{
    return static_cast<_T&&>(_p.second);
}

// get<T>(const pair&&) — _T matches _T1
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T1>::value && !is_same<_T1, _T2>::value,
    const _T&&
>::type
get(
    const pair<_T1, _T2>&& _p
) D_NOEXCEPT
{
    return static_cast<const _T&&>(_p.first);
}

// get<T>(const pair&&) — _T matches _T2
template<typename _T,
         typename _T1,
         typename _T2>
D_CONSTEXPR
typename enable_if<
    is_same<_T, _T2>::value && !is_same<_T1, _T2>::value,
    const _T&&
>::type
get(
    const pair<_T1, _T2>&& _p
) D_NOEXCEPT
{
    return static_cast<const _T&&>(_p.second);
}


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


#endif  // DJINTERP_RESTD_UTILITY_PAIR_GET_
