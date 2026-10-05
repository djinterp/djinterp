/*******************************************************************************
* djinterp [re_std]                                         unwrap_reference.hpp
*
* unwrap_reference class header:
* trait: yields `T&` if `Type` is `reference_wrapper<T>`; otherwise
*   yields `Type` unchanged.
*   Mirrors `std::unwrap_reference` (C++20). Used together with
* `decay` by `unwrap_ref_decay`, which is the canonical "auto-pluck out
* of a reference_wrapper" composition required by `make_pair`,
* `make_tuple`, and `bind_front`.
*
*
* path:      /inc/re_std/functional/unwrap_reference.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_UNWRAP_REFERENCE_HPP
#define RE_STD_FUNCTIONAL_UNWRAP_REFERENCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "re_std/functional/reference_wrapper.hpp"

namespace re_std
{

// unwrap_reference
//   trait: primary template -- type is unchanged.
template<typename Type>
struct unwrap_reference
{
    typedef Type type;
};

// unwrap_reference<reference_wrapper<U>>
//   trait: specialization -- yields U& (the wrapped reference).
template<typename U>
struct unwrap_reference< reference_wrapper<U> >
{
    typedef U& type;
};

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

template<typename Type>
using unwrap_reference_t = typename unwrap_reference<Type>::type;

#endif

}  // re_std
#endif // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_FUNCTIONAL_UNWRAP_REFERENCE_HPP
