/*******************************************************************************
* djinterp [re_std]                                             forward_like.hpp
*
* forwarding cast that follows another type's category:
*   forward_like<T>(x) returns x with a value category and constness
* that mirror T's. Specifically:
*
*   - constness:    if remove_reference<T> is const, the result is
*                   const-qualified. Otherwise unchanged.
*   - value cat'y:  if T is an lvalue reference, the result is an
*                   lvalue. Otherwise an rvalue.
*
*   Used in deducing-this-style code to forward "like" the qualified
* self parameter onto member values.
*
*   STANDARD STATUS:
*   Introduced in C++23 (P2445R1). re_std back-ports to C++11+.
*
*   IMPLEMENTATION:
*   The C++23 specification uses if-constexpr; we compute the same
* return type via a trait helper (forward_like_type_) that is
* SFINAE-friendly and works on the C++11 floor. The body is then a
* single static_cast, constexpr-eligible from C++11.
*
*
* path:      /inc/re_std/utility/forward_like.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_FORWARD_LIKE_HPP
#define RE_STD_UTILITY_FORWARD_LIKE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "../type_traits/conditional.hpp"
#include "../type_traits/is_const.hpp"
#include "../type_traits/is_lvalue_reference.hpp"
#include "../type_traits/remove_reference.hpp"

namespace re_std
{

namespace internal
{

// forward_like_type_
//   trait: computes the return type for forward_like<T>(U&&). The
//   computation is done in two steps:
//     1. Take U with reference stripped; call this _U_bare.
//     2. If T (with reference stripped) is const, const-qualify
//        _U_bare; otherwise leave it. Call this _U_constified.
//     3. If T is an lvalue reference, the type is _U_constified&;
//        otherwise _U_constified&&.
template<typename T, typename U>
struct forward_like_type_
{
    typedef typename remove_reference<U>::type _U_bare;
    typedef typename conditional<
        is_const<typename remove_reference<T>::type>::value,
        const _U_bare,
        _U_bare
    >::type _U_constified;
    typedef typename conditional<
        is_lvalue_reference<T>::value,
        _U_constified&,
        _U_constified&&
    >::type type;
};

}  // internal

// =============================================================================
// FORWARD_LIKE
// =============================================================================

// forward_like
//   function: forwards _value with cv- and value-category determined
//   by T. The return type is computed by internal::forward_like_type_;
//   the body is a single static_cast and is constexpr-eligible.
template<typename T, typename U>
RE_STD_CONSTEXPR
typename internal::forward_like_type_<T, U>::type
forward_like(U&& _value) noexcept
{
    return static_cast<
        typename internal::forward_like_type_<T, U>::type
    >(_value);
}

}  // re_std

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_UTILITY_FORWARD_LIKE_HPP
