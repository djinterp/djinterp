/*******************************************************************************
* djinterp [re_std]                                                      ref.hpp
*
* ref function header:
* function: factory producing `reference_wrapper<Type>`.
*   Two overloads: one accepting an lvalue (returns a wrapper to it)
* and one explicitly deleted for rvalues (mirrors `reference_wrapper`'s
* own deleted rvalue ctor). The reference_wrapper-of-reference_wrapper
* overload unwraps one level so `ref(ref(x))` is just `ref(x)`.
*
*
* path:      /inc/re_std/functional/ref.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_REF_HPP
#define RE_STD_FUNCTIONAL_REF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "re_std/functional/reference_wrapper.hpp"

namespace re_std
{

// ref
//   function: build a reference_wrapper from an lvalue.
template<typename Type>
RE_STD_CONSTEXPR reference_wrapper<Type>
ref(
    Type& _v
) noexcept
{
    return reference_wrapper<Type>(_v);
}

// ref (rvalue overload)
//   function: deleted -- forbidden, would dangle.
template<typename Type>
void ref(const Type&&) = delete;

// ref (idempotent overload)
//   function: ref(reference_wrapper<T>) returns a copy unchanged.
template<typename Type>
RE_STD_CONSTEXPR reference_wrapper<Type>
ref(
    reference_wrapper<Type> _v
) noexcept
{
    return _v;
}

}  // re_std
#endif // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_FUNCTIONAL_REF_HPP
