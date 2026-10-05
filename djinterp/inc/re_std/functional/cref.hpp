/*******************************************************************************
* djinterp [re_std]                                                     cref.hpp
*
* cref function header:
* function: factory producing `reference_wrapper<const Type>`.
*   The const counterpart to `re_std::ref`. The overload set is
* identical: lvalue-accepting, rvalue-deleted, and reference_wrapper-
* idempotent.
*
*
* path:      /inc/re_std/functional/cref.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_CREF_HPP
#define RE_STD_FUNCTIONAL_CREF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "re_std/functional/reference_wrapper.hpp"

namespace re_std
{

// cref
//   function: build a reference_wrapper<const T> from an lvalue.
template<typename Type>
RE_STD_CONSTEXPR reference_wrapper<const Type>
cref(
    const Type& _v
) noexcept
{
    return reference_wrapper<const Type>(_v);
}

// cref (rvalue overload)
//   function: deleted -- would dangle.
template<typename Type>
void cref(const Type&&) = delete;

// cref (idempotent overload)
//   function: cref(reference_wrapper<T>) returns reference_wrapper<const T>.
template<typename Type>
RE_STD_CONSTEXPR reference_wrapper<const Type>
cref(
    reference_wrapper<Type> _v
) noexcept
{
    return reference_wrapper<const Type>(_v.get());
}

}  // re_std
#endif // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_FUNCTIONAL_CREF_HPP
