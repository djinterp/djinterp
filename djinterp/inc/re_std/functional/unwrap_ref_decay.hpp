/*******************************************************************************
* djinterp [re_std]                                         unwrap_ref_decay.hpp
*
* unwrap_ref_decay class header:
* trait: composition of `decay` and `unwrap_reference`.
*   First decays `Type` (strips refs / cv / array-to-pointer / function-
* to-pointer), then if the decayed result is a `reference_wrapper<U>`,
* unwraps it to `U&`. This is the canonical "what does `make_pair` /
* `make_tuple` infer for an arg of type `Type`?" computation since
* C++20.
*
*
* path:      /inc/re_std/functional/unwrap_ref_decay.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_UNWRAP_REF_DECAY_HPP
#define RE_STD_FUNCTIONAL_UNWRAP_REF_DECAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/functional/unwrap_reference.hpp"

namespace re_std
{

// unwrap_ref_decay
//   trait: yields unwrap_reference_t<decay_t<T>>.
template<typename Type>
struct unwrap_ref_decay
    : unwrap_reference<typename decay<Type>::type>
{};

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

template<typename Type>
using unwrap_ref_decay_t = typename unwrap_ref_decay<Type>::type;

#endif

}  // re_std
#endif // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_FUNCTIONAL_UNWRAP_REF_DECAY_HPP
