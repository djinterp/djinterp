/***********************************************************************
* restd                                                           ref.hpp
*
* function: factory producing `reference_wrapper<_Type>`.
*   Two overloads: one accepting an lvalue (returns a wrapper to it)
* and one explicitly deleted for rvalues (mirrors `reference_wrapper`'s
* own deleted rvalue ctor). The reference_wrapper-of-reference_wrapper
* overload unwraps one level so `ref(ref(x))` is just `ref(x)`.
*
*
* path:      /inc/djinterp/re_std/functional/ref.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_REF_
#define RESTD_FUNCTIONAL_REF_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "restd/functional/reference_wrapper.hpp"

namespace restd
{

// ref
//   function: build a reference_wrapper from an lvalue.
template<typename _Type>
D_CONSTEXPR reference_wrapper<_Type>
ref(
    _Type& _v
) noexcept
{
    return reference_wrapper<_Type>(_v);
}

// ref (rvalue overload)
//   function: deleted -- forbidden, would dangle.
template<typename _Type>
void ref(const _Type&&) = delete;

// ref (idempotent overload)
//   function: ref(reference_wrapper<T>) returns a copy unchanged.
template<typename _Type>
D_CONSTEXPR reference_wrapper<_Type>
ref(
    reference_wrapper<_Type> _v
) noexcept
{
    return _v;
}

} // namespace restd

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif // RESTD_FUNCTIONAL_REF_
