/***********************************************************************
* restd                                                          cref.hpp
*
* function: factory producing `reference_wrapper<const _Type>`.
*   The const counterpart to `restd::ref`. The overload set is
* identical: lvalue-accepting, rvalue-deleted, and reference_wrapper-
* idempotent.
*
*
* path:      /inc/restd/functional/cref.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_CREF_
#define RESTD_FUNCTIONAL_CREF_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "restd/functional/reference_wrapper.hpp"

namespace restd
{

// cref
//   function: build a reference_wrapper<const T> from an lvalue.
template<typename _Type>
D_CONSTEXPR reference_wrapper<const _Type>
cref(
    const _Type& _v
) noexcept
{
    return reference_wrapper<const _Type>(_v);
}

// cref (rvalue overload)
//   function: deleted -- would dangle.
template<typename _Type>
void cref(const _Type&&) = delete;

// cref (idempotent overload)
//   function: cref(reference_wrapper<T>) returns reference_wrapper<const T>.
template<typename _Type>
D_CONSTEXPR reference_wrapper<const _Type>
cref(
    reference_wrapper<_Type> _v
) noexcept
{
    return reference_wrapper<const _Type>(_v.get());
}

} // namespace restd

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif // RESTD_FUNCTIONAL_CREF_
