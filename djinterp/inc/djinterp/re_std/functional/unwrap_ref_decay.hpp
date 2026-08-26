/***********************************************************************
* re_std                                             unwrap_ref_decay.hpp
*
* trait: composition of `decay` and `unwrap_reference`.
*   First decays `_Type` (strips refs / cv / array-to-pointer / function-
* to-pointer), then if the decayed result is a `reference_wrapper<U>`,
* unwraps it to `U&`. This is the canonical "what does `make_pair` /
* `make_tuple` infer for an arg of type `_Type`?" computation since
* C++20.
*
*
* path:      /inc/djinterp/re_std/functional/unwrap_ref_decay.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_UNWRAP_REF_DECAY_
#define DJINTERP_RE_STD_FUNCTIONAL_UNWRAP_REF_DECAY_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/functional/unwrap_reference.hpp"

namespace re_std
{

// unwrap_ref_decay
//   trait: yields unwrap_reference_t<decay_t<T>>.
template<typename _Type>
struct unwrap_ref_decay
    : unwrap_reference<typename decay<_Type>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

template<typename _Type>
using unwrap_ref_decay_t = typename unwrap_ref_decay<_Type>::type;

#endif

} // namespace re_std

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // DJINTERP_RE_STD_FUNCTIONAL_UNWRAP_REF_DECAY_
