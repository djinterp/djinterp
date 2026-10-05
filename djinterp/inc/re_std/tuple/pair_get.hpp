/*******************************************************************************
* djinterp [re_std]                                                 pair_get.hpp
*
* pair get<> overloads:
*   Index-based and type-based element access for re_std::pair, mirroring
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
*   Ill-formed when T1 == T2 (the element is not unique) — caught by
*   enable_if on is_same<T1, T2>::value == false.
*
*   PORTABILITY:
*   Requires C++11+ for index-based form (pair itself is C++98+ but
* the rvalue-reference categories are C++11+); the type-based form
* additionally requires C++14+ alias-template machinery. On C++98/03
* this header expands to nothing — pair is still usable directly via
* its .first / .second members.
*
*
* path:      /inc/re_std/tuple/pair_get.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_PAIR_GET_HPP
#define RE_STD_TUPLE_PAIR_GET_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: rvalue-ref forms below need C++11+. The whole header guards
// on rvalue-refs; pair pre-existed in C++98 but the get<I>(p&&) forms
// returning T&& are inherently C++11+.
#if RE_STD_LANG_HAS_RVALUE_REFERENCES


// std
#include <cstddef>
// re_std
#include "../utility/pair.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"


namespace re_std
{


// =============================================================================
// I.   GET BY INDEX
// =============================================================================
// The index-based form uses ordinary function overloading on the
// non-type template parameter via tag-dispatch through internal
// helpers. The dispatch picks first vs. second based on I.

namespace internal
{

    // pair_get_helper
    //   trait: tag-dispatched accessor for pair. Specialised on
    // I in {0, 1}; out-of-range I has no specialisation and is
    // SFINAE-rejected at the call site.
    template<std::size_t I>
    struct pair_get_helper;

    template<>
    struct pair_get_helper<0>
    {
        template<typename T1, typename T2>
        static RE_STD_CONSTEXPR
        T1&
        lref(
            pair<T1, T2>& _p
        ) RE_STD_NOEXCEPT
        {
            return _p.first;
        }

        template<typename T1, typename T2>
        static RE_STD_CONSTEXPR
        const T1&
        clref(
            const pair<T1, T2>& _p
        ) RE_STD_NOEXCEPT
        {
            return _p.first;
        }
    };

    template<>
    struct pair_get_helper<1>
    {
        template<typename T1, typename T2>
        static RE_STD_CONSTEXPR
        T2&
        lref(
            pair<T1, T2>& _p
        ) RE_STD_NOEXCEPT
        {
            return _p.second;
        }

        template<typename T1, typename T2>
        static RE_STD_CONSTEXPR
        const T2&
        clref(
            const pair<T1, T2>& _p
        ) RE_STD_NOEXCEPT
        {
            return _p.second;
        }
    };


    // pair_element_for_index
    //   trait: element type at index I (0 or 1). Mirrors the
    // tuple_element<I, pair<T1,T2>> specialisation in
    // pair_tuple_element.hpp but is a small internal trait to avoid
    // a hard #include dependency on tuple_element here (get can be
    // used independently of tuple_element).
    template<std::size_t I,
             typename    T1,
             typename    T2>
    struct pair_element_for_index;

    template<typename T1, typename T2>
    struct pair_element_for_index<0, T1, T2>
    {
        typedef T1 type;
    };

    template<typename T1, typename T2>
    struct pair_element_for_index<1, T1, T2>
    {
        typedef T2 type;
    };

}  // internal


// get<I>(pair&)
//   function: yields lvalue reference to the I-th element.
template<std::size_t I,
         typename    T1,
         typename    T2>
RE_STD_CONSTEXPR
typename internal::pair_element_for_index<I, T1, T2>::type&
get(
    pair<T1, T2>& _p
) RE_STD_NOEXCEPT
{
    return internal::pair_get_helper<I>::lref(_p);
}

// get<I>(const pair&)
template<std::size_t I,
         typename    T1,
         typename    T2>
RE_STD_CONSTEXPR
const typename internal::pair_element_for_index<I, T1, T2>::type&
get(
    const pair<T1, T2>& _p
) RE_STD_NOEXCEPT
{
    return internal::pair_get_helper<I>::clref(_p);
}

// get<I>(pair&&)
template<std::size_t I,
         typename    T1,
         typename    T2>
RE_STD_CONSTEXPR
typename internal::pair_element_for_index<I, T1, T2>::type&&
get(
    pair<T1, T2>&& _p
) RE_STD_NOEXCEPT
{
    typedef typename internal::pair_element_for_index<I, T1, T2>::type _E;
    return static_cast<_E&&>(internal::pair_get_helper<I>::lref(_p));
}

// get<I>(const pair&&)
template<std::size_t I,
         typename    T1,
         typename    T2>
RE_STD_CONSTEXPR
const typename internal::pair_element_for_index<I, T1, T2>::type&&
get(
    const pair<T1, T2>&& _p
) RE_STD_NOEXCEPT
{
    typedef typename internal::pair_element_for_index<I, T1, T2>::type _E;
    return static_cast<const _E&&>(internal::pair_get_helper<I>::clref(_p));
}


// =============================================================================
// II.  GET BY TYPE  (C++14+)
// =============================================================================
// SFINAE-rejected when T1 == T2 (the requested type is not unique).
// For each value category, two overloads dispatch on which member
// matches T.

#if RE_STD_LANG_IS_CPP14_OR_HIGHER


// get<T>(pair&) — T matches T1
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T1>::value && !is_same<T1, T2>::value,
    T&
>::type
get(
    pair<T1, T2>& _p
) RE_STD_NOEXCEPT
{
    return _p.first;
}

// get<T>(pair&) — T matches T2
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T2>::value && !is_same<T1, T2>::value,
    T&
>::type
get(
    pair<T1, T2>& _p
) RE_STD_NOEXCEPT
{
    return _p.second;
}

// get<T>(const pair&) — T matches T1
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T1>::value && !is_same<T1, T2>::value,
    const T&
>::type
get(
    const pair<T1, T2>& _p
) RE_STD_NOEXCEPT
{
    return _p.first;
}

// get<T>(const pair&) — T matches T2
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T2>::value && !is_same<T1, T2>::value,
    const T&
>::type
get(
    const pair<T1, T2>& _p
) RE_STD_NOEXCEPT
{
    return _p.second;
}

// get<T>(pair&&) — T matches T1
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T1>::value && !is_same<T1, T2>::value,
    T&&
>::type
get(
    pair<T1, T2>&& _p
) RE_STD_NOEXCEPT
{
    return static_cast<T&&>(_p.first);
}

// get<T>(pair&&) — T matches T2
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T2>::value && !is_same<T1, T2>::value,
    T&&
>::type
get(
    pair<T1, T2>&& _p
) RE_STD_NOEXCEPT
{
    return static_cast<T&&>(_p.second);
}

// get<T>(const pair&&) — T matches T1
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T1>::value && !is_same<T1, T2>::value,
    const T&&
>::type
get(
    const pair<T1, T2>&& _p
) RE_STD_NOEXCEPT
{
    return static_cast<const T&&>(_p.first);
}

// get<T>(const pair&&) — T matches T2
template<typename T,
         typename T1,
         typename T2>
RE_STD_CONSTEXPR
typename enable_if<
    is_same<T, T2>::value && !is_same<T1, T2>::value,
    const T&&
>::type
get(
    const pair<T1, T2>&& _p
) RE_STD_NOEXCEPT
{
    return static_cast<const T&&>(_p.second);
}


#endif  // RE_STD_LANG_IS_CPP14_OR_HIGHER


}  // re_std


#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


#endif  // RE_STD_TUPLE_PAIR_GET_HPP
