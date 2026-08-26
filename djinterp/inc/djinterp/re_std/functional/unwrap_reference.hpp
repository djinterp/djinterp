/***********************************************************************
* re_std                                             unwrap_reference.hpp
*
* trait: yields `T&` if `_Type` is `reference_wrapper<T>`; otherwise
*   yields `_Type` unchanged.
*   Mirrors `std::unwrap_reference` (C++20). Used together with
* `decay` by `unwrap_ref_decay`, which is the canonical "auto-pluck out
* of a reference_wrapper" composition required by `make_pair`,
* `make_tuple`, and `bind_front`.
*
*
* path:      /inc/djinterp/re_std/functional/unwrap_reference.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_UNWRAP_REFERENCE_
#define DJINTERP_RE_STD_FUNCTIONAL_UNWRAP_REFERENCE_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "re_std/functional/reference_wrapper.hpp"

namespace re_std
{

// unwrap_reference
//   trait: primary template -- type is unchanged.
template<typename _Type>
struct unwrap_reference
{
    typedef _Type type;
};

// unwrap_reference<reference_wrapper<U>>
//   trait: specialization -- yields U& (the wrapped reference).
template<typename _U>
struct unwrap_reference< reference_wrapper<_U> >
{
    typedef _U& type;
};

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

template<typename _Type>
using unwrap_reference_t = typename unwrap_reference<_Type>::type;

#endif

} // namespace re_std

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // DJINTERP_RE_STD_FUNCTIONAL_UNWRAP_REFERENCE_
