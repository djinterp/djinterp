/******************************************************************************
* djinterp [stl]                                              any_cast.hpp
*
* any_cast header:
*   Provides type-safe access to the value stored in a djinterp::stl::any.
* Five overloads mirror the C++17 std::any_cast interface:
*   - any_cast<T>(any*)        → T*         (nullptr on mismatch)
*   - any_cast<T>(const any*)  → const T*   (nullptr on mismatch)
*   - any_cast<T>(const any&)  → T          (copy, checked)
*   - any_cast<T>(any&)        → T&         (reference, checked)
*   - any_cast<T>(any&&)       → T          (move, checked)
*
*   PORTABILITY:
*   Reference overloads throw bad_any_cast when D_ENV_CPP98_HAS_TYPEINFO
* or D_ENV_CPP98_HAS_EXCEPTION is available. When exceptions are disabled,
* unchecked versions are provided instead (undefined behaviour on mismatch).
*
* path:      \inc\cpp\stl\any_cast.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_ANY_CAST_
#define DJINTERP_ANY_CAST_ 1

#include ".\djinterp.hpp"
#include ".\stl\any.hpp"
#include ".\stl\bad_any_cast.hpp"


NS_DJINTERP
NS_STL


// =============================================================================
// I.   POINTER OVERLOADS (unchecked — always available)
// =============================================================================
// These return nullptr when the stored type does not match _T. No exception
// is thrown; the caller must check the return value.

// any_cast (mutable pointer)
//   function: returns a pointer to the stored value if it matches _T,
// or nullptr otherwise.
template<typename _T>
_T*
any_cast
(
    any* _a
)
noexcept
{
    // reject null or type mismatch
    if ( (!_a) ||
         (!_a->template holds<_T>()) )
    {
        return nullptr;
    }

    return &(_a->template get_ref<_T>());
}

// any_cast (const pointer)
//   function: returns a const pointer to the stored value if it
// matches _T, or nullptr otherwise.
template<typename _T>
const _T*
any_cast
(
    const any* _a
)
noexcept
{
    // reject null or type mismatch
    if ( (!_a) ||
         (!_a->template holds<_T>()) )
    {
        return nullptr;
    }

    return &(_a->template get_ref<_T>());
}


// =============================================================================
// II.  REFERENCE OVERLOADS (checked or unchecked)
// =============================================================================

#if ( D_ENV_CPP98_HAS_TYPEINFO || D_ENV_CPP98_HAS_EXCEPTION )

// any_cast (const lvalue reference — checked)
//   function: returns a copy of the stored value.
// throws: bad_any_cast if the stored type does not match _T.
template<typename _T>
_T
any_cast
(
    const any& _a
)
{
    // verify type match
    if (!_a.template holds<_T>())
    {
        throw bad_any_cast();
    }

    return _a.template get<_T>();
}

// any_cast (mutable lvalue reference — checked)
//   function: returns a mutable reference to the stored value.
// throws: bad_any_cast if the stored type does not match _T.
template<typename _T>
_T&
any_cast
(
    any& _a
)
{
    // verify type match
    if (!_a.template holds<_T>())
    {
        throw bad_any_cast();
    }

    return _a.template get_ref<_T>();
}

// any_cast (rvalue reference — checked)
//   function: moves the stored value out of the any.
// throws: bad_any_cast if the stored type does not match _T.
template<typename _T>
_T
any_cast
(
    any&& _a
)
{
    // verify type match
    if (!_a.template holds<_T>())
    {
        throw bad_any_cast();
    }

    return static_cast<_T&&>(_a.template get_ref<_T>());
}

#else  // no exceptions available

// any_cast (const lvalue reference — unchecked)
//   function: returns a copy of the stored value without type
// checking.
// note: undefined behaviour if the stored type does not match _T.
template<typename _T>
D_CONSTEXPR _T
any_cast
(
    const any& _a
)
{
    return _a.template get<_T>();
}

// any_cast (mutable lvalue reference — unchecked)
//   function: returns a mutable reference to the stored value
// without type checking.
// note: undefined behaviour if the stored type does not match _T.
template<typename _T>
_T&
any_cast
(
    any& _a
)
{
    return _a.template get_ref<_T>();
}

// any_cast (rvalue reference — unchecked)
//   function: moves the stored value out of the any without type
// checking.
// note: undefined behaviour if the stored type does not match _T.
template<typename _T>
_T
any_cast
(
    any&& _a
)
{
    return static_cast<_T&&>(_a.template get_ref<_T>());
}

#endif  // exception availability


NS_END  // stl
NS_END  // djinterp


#endif  // DJINTERP_ANY_CAST_
