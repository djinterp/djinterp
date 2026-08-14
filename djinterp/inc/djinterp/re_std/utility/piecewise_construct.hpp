/******************************************************************************
* re_std [utility]                                     piecewise_construct.hpp
*
*   piecewise pair construction tag:
*   `piecewise_construct_t` is the empty tag type, and `piecewise_construct`
* its instance, that selects pair's constructor taking two tuples of
* constructor ARGUMENTS rather than two values.  Each element is then built
* in place from its own argument pack:
*
*     pair<A, B> p(re_std::piecewise_construct,
*                  re_std::forward_as_tuple(a1, a2),
*                  re_std::forward_as_tuple(b1));
*
* This is the only way to build a pair whose elements are non-copyable and
* non-movable, and the mechanism map::emplace uses internally.
*
*   STD IS C++11; re_std IS C++11.
*   The TAG TYPE alone needs nothing past C++98 - it is an empty struct.  The
* floor comes from the constructor it selects, which takes tuples and so needs
* variadic templates.  Shipping the tag below the constructor would advertise
* a selector for an overload that does not exist.
*
*   The ctor is explicit and defaulted so that `piecewise_construct_t{}` is
* well-formed but an accidental `{}` will not silently convert.
*
*
* path:      /inc/djinterp/re_std/utility/piecewise_construct.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_UTILITY_PIECEWISE_CONSTRUCT_
#define RESTD_UTILITY_PIECEWISE_CONSTRUCT_ 1

// re_std
#include "../type_traits/type_traits.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_DJINTERP
NS_RESTD

// piecewise_construct_t
//   struct: disambiguation tag for pair's piecewise constructor.
struct piecewise_construct_t
{
    //   DEFAULTED, not user-provided.  A user-provided `{}` body would make
    // the type non-literal, and then `piecewise_construct` could not be a
    // constexpr object at all.  `= default` keeps the default constructor
    // trivial and the type literal, while `explicit` still stops an
    // accidental `{}` converting silently.  std spells it the same way.
    explicit piecewise_construct_t() = default;
};

// piecewise_construct
//   constant: the piecewise_construct_t instance.
//
//   D_INLINE_VAR gives this external linkage exactly once on C++17+; below
// that it is a namespace-scope constant, which is why it is D_CONSTEXPR
// rather than an inline variable everywhere.
D_INLINE_VAR D_CONSTEXPR piecewise_construct_t piecewise_construct
    = piecewise_construct_t();

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_UTILITY_PIECEWISE_CONSTRUCT_
