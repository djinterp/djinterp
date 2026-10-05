/*******************************************************************************
* djinterp [re_std]                                                 any_cast.hpp
*
* any_cast header:
*   Provides type-safe access to the value stored in a re_std::any.
* Five overloads mirror the C++17 std::any_cast interface:
*   - any_cast<T>(any*)        -> T*         (nullptr on mismatch)
*   - any_cast<T>(const any*)  -> const T*   (nullptr on mismatch)
*   - any_cast<T>(const any&)  -> T          (copy, checked)
*   - any_cast<T>(any&)        -> T&         (reference, checked)
*   - any_cast<T>(any&&)       -> T          (move, checked)
*
*   PORTABILITY:
*   - C++98/03: pointer overloads always available. Reference overloads
*     throw bad_any_cast when RE_STD_HAS_RTTI or
*     RE_STD_HAS_EXCEPTIONS is available; otherwise unchecked
*     (undefined behaviour on mismatch).
*   - C++11+: rvalue reference overload (any&&) additionally available.
*   - Pointer overloads only support heap-stored types (require
*     RE_STD_HAS_HEADER_NEW).
*
*
* path:      /inc/re_std/any/any_cast.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.10
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ANY_ANY_CAST_HPP
#define RE_STD_ANY_ANY_CAST_HPP 1

// any.hpp exists wherever `long long` does (decision 4.6), so this header
// does too: under ISO strict C++98 it is empty, as any.hpp is.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_HAS_LONG_LONG

// re_std
                                      // _EXCEPTION
#include "./any.hpp"                             // any, internal::is_sbo_type
#include "./bad_any_cast.hpp"                    // bad_any_cast
#include "../type_traits/false_type.hpp"         // false_type
#include "../type_traits/integral_constant.hpp"  // integral_constant
#include "../type_traits/is_reference.hpp"       // is_reference
#include "../type_traits/remove_cv.hpp"          // remove_cv
#include "../type_traits/remove_reference.hpp"   // remove_reference
#include "../type_traits/true_type.hpp"          // true_type


namespace re_std
{


// ===========================================================================
// 0.   WHAT EACH FORM CAN RETURN
// ===========================================================================
//   any keeps heap-stored types as objects and scalars (bool, integers,
// floating point, enums, pointers) converted into a wider small-buffer
// member. So any_cast can return a VALUE of any stored type -- get<Type>()
// reads every category -- but a reference or a pointer only to a
// heap-stored object: for a scalar there is no Type object to point at.
// Asking for one is a compile error, never an address read through the
// null heap pointer. (These forms read the heap pointer for every type,
// so any_cast<int>(a) on a non-const any dereferenced null.)

namespace internal
{

    // any_cast_value
    //   type: Type with references and cv-qualifiers removed -- the type
    // the any must hold.
    template<typename Type>
    struct any_cast_value
    {
        typedef typename remove_cv<
            typename remove_reference<Type>::type>::type type;
    };

    // any_cast_object
    //   function: the stored object itself; heap-stored types only.
    template<typename Value>
    Value&
    any_cast_object(
        any& _a
    )
    {
        RE_STD_STATIC_ASSERT(!is_sbo_type<Value>::value,
                        "re_std::any keeps this type converted in its small "
                        "buffer: any_cast can return its value, not a "
                        "reference or a pointer to it");

        return _a.template get_ref<Value>();
    }

    template<typename Value>
    const Value&
    any_cast_object(
        const any& _a
    )
    {
        RE_STD_STATIC_ASSERT(!is_sbo_type<Value>::value,
                        "re_std::any keeps this type converted in its small "
                        "buffer: any_cast can return its value, not a "
                        "reference or a pointer to it");

        return _a.template get_ref<Value>();
    }

    // any_cast_out
    //   function: Type from a matching any: a value (false_type) through
    // get, which reads every category; a reference (true_type) to the
    // stored object.
    template<typename Type,
             typename Any>
    Type
    any_cast_out(
        Any&      _a,
        false_type
    )
    {
        return _a.template get<typename any_cast_value<Type>::type>();
    }

    template<typename Type,
             typename Any>
    Type
    any_cast_out(
        Any&     _a,
        true_type
    )
    {
        return any_cast_object<typename any_cast_value<Type>::type>(_a);
    }

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    // any_cast_moved
    //   function: Type from a matching rvalue any. A heap-stored object
    // is moved out (true_type); a small-buffer value is read (false_type).
    template<typename Type>
    Type
    any_cast_moved(
        any&       _a,
        false_type
    )
    {
        return _a.template get<typename any_cast_value<Type>::type>();
    }

    template<typename Type>
    Type
    any_cast_moved(
        any&      _a,
        true_type
    )
    {
        typedef typename any_cast_value<Type>::type Value;

        return static_cast<Type>(
            static_cast<Value&&>(any_cast_object<Value>(_a)));
    }
#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

}  // internal


// ===========================================================================
// 1.   POINTER OVERLOADS (unchecked - always available)
// ===========================================================================
// These return RE_STD_NULLPTR when the stored type does not match Type. No
// exception is thrown; the caller must check the return value. Heap-stored
// types only (section 0).

#if RE_STD_HAS_HEADER_NEW

// any_cast (mutable pointer)
//   function: returns a pointer to the stored value if it matches
// Type, or RE_STD_NULLPTR otherwise.
template<typename Type>
Type*
any_cast(
    any* _a
)
RE_STD_NOEXCEPT
{
    // reject null or type mismatch
    if ( (!_a) ||
         (!_a->template holds<Type>()) )
    {
        return RE_STD_NULLPTR;
    }

    return &internal::any_cast_object<Type>(*_a);
}

// any_cast (const pointer)
//   function: returns a const pointer to the stored value if it
// matches Type, or RE_STD_NULLPTR otherwise.
template<typename Type>
const Type*
any_cast(
    const any* _a
)
RE_STD_NOEXCEPT
{
    // reject null or type mismatch
    if ( (!_a) ||
         (!_a->template holds<Type>()) )
    {
        return RE_STD_NULLPTR;
    }

    return &internal::any_cast_object<Type>(*_a);
}

#endif  // RE_STD_HAS_HEADER_NEW


// ===========================================================================
// 2.   VALUE AND REFERENCE OVERLOADS (checked or unchecked)
// ===========================================================================
//   As std's: any_cast<Type> returns Type, a value or -- when Type is a
// reference -- the stored object. The any must hold Type with references
// and cv-qualifiers removed. With exceptions on, a mismatch throws
// bad_any_cast; without them it is undefined, as documented below.
//   The checked overloads throw, so they need exceptions; RTTI decides only
// which base bad_any_cast has, not whether it can be thrown.

// any_cast (const lvalue)
//   function: Type from a const any.
// throws: bad_any_cast if the stored type does not match (exceptions on);
// otherwise unchecked, and a mismatch is undefined behaviour.
template<typename Type>
Type
any_cast(
    const any& _a
)
{
    typedef typename internal::any_cast_value<Type>::type Value;

#if RE_STD_HAS_EXCEPTIONS
    // verify type match
    if (!_a.template holds<Value>())
    {
        throw bad_any_cast();
    }
#endif  // RE_STD_HAS_EXCEPTIONS

    return internal::any_cast_out<Type>(_a,
                                         typename is_reference<Type>::type());
}

// any_cast (mutable lvalue)
//   function: Type from an any; a reference Type refers to the stored
// object, which only heap-stored types have (section 0).
// throws: as the const lvalue form.
template<typename Type>
Type
any_cast(
    any& _a
)
{
    typedef typename internal::any_cast_value<Type>::type Value;

#if RE_STD_HAS_EXCEPTIONS
    // verify type match
    if (!_a.template holds<Value>())
    {
        throw bad_any_cast();
    }
#endif  // RE_STD_HAS_EXCEPTIONS

    return internal::any_cast_out<Type>(_a,
                                         typename is_reference<Type>::type());
}

// any_cast (rvalue)
//   function: Type from an expiring any; a heap-stored object is moved
// out, a small-buffer value is read.
// throws: as the const lvalue form.
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
template<typename Type>
Type
any_cast(
    any&& _a
)
{
    typedef typename internal::any_cast_value<Type>::type Value;

#if RE_STD_HAS_EXCEPTIONS
    // verify type match
    if (!_a.template holds<Value>())
    {
        throw bad_any_cast();
    }
#endif  // RE_STD_HAS_EXCEPTIONS

    return internal::any_cast_moved<Type>(
        _a,
        integral_constant<bool,
                          ( is_reference<Type>::value ||
                            !internal::is_sbo_type<Value>::value )>());
}
#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES


}  // re_std

#endif  // RE_STD_HAS_LONG_LONG


#endif  // RE_STD_ANY_ANY_CAST_HPP
