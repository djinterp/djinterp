/******************************************************************************
* djinterp [restd]                                                any_cast.hpp
*
* any_cast header:
*   Provides type-safe access to the value stored in a restd::any.
* Five overloads mirror the C++17 std::any_cast interface:
*   - any_cast<T>(any*)        -> T*         (nullptr on mismatch)
*   - any_cast<T>(const any*)  -> const T*   (nullptr on mismatch)
*   - any_cast<T>(const any&)  -> T          (copy, checked)
*   - any_cast<T>(any&)        -> T&         (reference, checked)
*   - any_cast<T>(any&&)       -> T          (move, checked)
*
*   PORTABILITY:
*   - C++98/03: pointer overloads always available. Reference overloads
*     throw bad_any_cast when D_ENV_CPP98_HAS_TYPEINFO or
*     D_ENV_CPP98_HAS_EXCEPTION is available; otherwise unchecked
*     (undefined behaviour on mismatch).
*   - C++11+: rvalue reference overload (any&&) additionally available.
*   - Pointer overloads only support heap-stored types (require
*     D_ENV_CPP98_HAS_NEW).
*
*
* path:      /inc/djinterp/restd/any/any_cast.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_RESTD_ANY_CAST_
#define DJINTERP_RESTD_ANY_CAST_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./any.hpp"
#include "./bad_any_cast.hpp"


NS_RESTD


// ===========================================================================
// I.   POINTER OVERLOADS (unchecked - always available)
// ===========================================================================
// These return D_NULLPTR when the stored type does not match _Type.
// No exception is thrown; the caller must check the return value.
// note: only valid for heap-stored types (SBO types cannot
// return a pointer to their stored representation because the
// union member is a wider type).

#if D_ENV_CPP98_HAS_NEW

// any_cast (mutable pointer)
//   function: returns a pointer to the stored value if it matches
// _Type, or D_NULLPTR otherwise.
template<typename _Type>
_Type*
any_cast(
    any* _a
)
D_NOEXCEPT
{
    // reject null or type mismatch
    if ( (!_a) ||
         (!_a->template holds<_Type>()) )
    {
        return D_NULLPTR;
    }

    return &(_a->template get_ref<_Type>());
}

// any_cast (const pointer)
//   function: returns a const pointer to the stored value if it
// matches _Type, or D_NULLPTR otherwise.
template<typename _Type>
const _Type*
any_cast(
    const any* _a
)
D_NOEXCEPT
{
    // reject null or type mismatch
    if ( (!_a) ||
         (!_a->template holds<_Type>()) )
    {
        return D_NULLPTR;
    }

    return &(_a->template get_ref<_Type>());
}

#endif  // D_ENV_CPP98_HAS_NEW


// ===========================================================================
// II.  REFERENCE OVERLOADS (checked or unchecked)
// ===========================================================================

#if ( D_ENV_CPP98_HAS_TYPEINFO ||                                             \
      D_ENV_CPP98_HAS_EXCEPTION )

// any_cast (const lvalue reference - checked)
//   function: returns a copy of the stored value.
// throws: bad_any_cast if the stored type does not match _Type.
template<typename _Type>
_Type
any_cast(
    const any& _a
)
{
    // verify type match
    if (!_a.template holds<_Type>())
    {
        throw bad_any_cast();
    }

    return _a.template get<_Type>();
}

// any_cast (mutable lvalue reference - checked)
//   function: returns a mutable reference to the stored value.
// throws: bad_any_cast if the stored type does not match _Type.
// note: only valid for heap-stored types.
#if D_ENV_CPP98_HAS_NEW
template<typename _Type>
_Type&
any_cast(
    any& _a
)
{
    // verify type match
    if (!_a.template holds<_Type>())
    {
        throw bad_any_cast();
    }

    return _a.template get_ref<_Type>();
}
#endif  // D_ENV_CPP98_HAS_NEW

// any_cast (rvalue reference - checked)
//   function: moves the stored value out of the any.
// throws: bad_any_cast if the stored type does not match _Type.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES && D_ENV_CPP98_HAS_NEW
template<typename _Type>
_Type
any_cast(
    any&& _a
)
{
    // verify type match
    if (!_a.template holds<_Type>())
    {
        throw bad_any_cast();
    }

    return static_cast<_Type&&>(_a.template get_ref<_Type>());
}
#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES && D_ENV_CPP98_HAS_NEW

#else  // no exceptions available

// any_cast (const lvalue reference - unchecked)
//   function: returns a copy of the stored value without type
// checking.
// note: undefined behaviour if the stored type does not match _Type.
template<typename _Type>
_Type
any_cast(
    const any& _a
)
{
    return _a.template get<_Type>();
}

// any_cast (mutable lvalue reference - unchecked)
//   function: returns a mutable reference to the stored value
// without type checking.
// note: undefined behaviour if the stored type does not match _Type.
#if D_ENV_CPP98_HAS_NEW
template<typename _Type>
_Type&
any_cast(
    any& _a
)
{
    return _a.template get_ref<_Type>();
}
#endif  // D_ENV_CPP98_HAS_NEW

// any_cast (rvalue reference - unchecked)
//   function: moves the stored value out of the any without type
// checking.
// note: undefined behaviour if the stored type does not match _Type.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES && D_ENV_CPP98_HAS_NEW
template<typename _Type>
_Type
any_cast(
    any&& _a
)
{
    return static_cast<_Type&&>(_a.template get_ref<_Type>());
}
#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES && D_ENV_CPP98_HAS_NEW

#endif  // exception availability


NS_END  // restd


#endif  // DJINTERP_RESTD_ANY_CAST_
