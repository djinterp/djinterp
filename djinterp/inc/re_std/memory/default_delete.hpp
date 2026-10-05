/*******************************************************************************
* djinterp [re_std]                                           default_delete.hpp
*
* the default deleter for unique_ptr:
*   default_delete<T>     -  calls `delete _p` on its argument.
*   default_delete<T[]>   -  calls `delete[] _p` on its argument,
*                             SFINAE-restricted to convertible types.
*
* both specialisations:
*   - are default-constructible and trivially copyable;
*   - require T to be a complete type at the point operator() is
*     instantiated (this is not optional - deleting an incomplete-type
*     pointer is undefined behaviour, and the static_assert here
*     catches it at compile time);
*   - have a templated converting constructor on C++11+, gated on
*     re_std::is_convertible. The array specialisation's converting
*     constructor uses the array-of-pointer-to-array form
*     (U(*)[] -> T(*)[]) per [unique.ptr.dltr.dflt1]/2.
*
*
* path:      /inc/re_std/memory/default_delete.hpp
*   The class itself ships, but the converting constructor is omitted
* (it requires is_convertible, which is C++11+ in re_std). This is
* sufficient for unique_ptr's basic use; covariant deleter conversions
* are a C++11+ feature.
*
*
* path:      /inc/re_std/memory/default_delete.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_DEFAULT_DELETE_HPP
#define RE_STD_MEMORY_DEFAULT_DELETE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_convertible.hpp"
#endif


namespace re_std
{

// =============================================================================
// default_delete  -  scalar specialisation
// =============================================================================

// default_delete<T>
//   class: invokes `delete _p` on its argument.
template<typename T>
struct default_delete
{
    // Default ctor.
    RE_STD_CONSTEXPR default_delete() RE_STD_NOEXCEPT
    {
    }

    #if RE_STD_LANG_IS_CPP11_OR_HIGHER

        // Converting ctor: enabled when U* is convertible to T*.
        // This is what lets you build a default_delete<Base> from a
        // default_delete<Derived>.
        template<typename U>
        default_delete
        (
            const default_delete<U>&,
            typename enable_if
            <
                is_convertible<U*, T*>::value,
                int
            >::type = 0
        ) RE_STD_NOEXCEPT
        {
        }

    #endif

    // operator()
    //   function: calls `delete _p`. T must be complete here.
    void operator()(T* _p) const
    {
        // Force a hard error if T is incomplete. The sizeof check
        // is the canonical idiom: incomplete types have no size, so
        // the array-bound expression is ill-formed.
        typedef char _T_must_be_complete_type[sizeof(T) ? 1 : -1];
        (void)sizeof(_T_must_be_complete_type);

        delete _p;
    }
};


// =============================================================================
// default_delete<T[]>  -  array specialisation
// =============================================================================

// default_delete<T[]>
//   class: invokes `delete[] _p` on its argument. The converting
//   ctor and operator() are SFINAE-restricted to types that are
//   array-of-pointer-to-array convertible to T[], not merely
//   convertible to T*. This is what makes the array form refuse
//   covariant Derived[] -> Base[] conversion (which would be a
//   violation of C-style array layout invariants).
template<typename T>
struct default_delete<T[]>
{
    RE_STD_CONSTEXPR default_delete() RE_STD_NOEXCEPT
    {
    }

    #if RE_STD_LANG_IS_CPP11_OR_HIGHER

        template<typename U>
        default_delete
        (
            const default_delete<U[]>&,
            typename enable_if
            <
                is_convertible<U(*)[], T(*)[]>::value,
                int
            >::type = 0
        ) RE_STD_NOEXCEPT
        {
        }

        template<typename U>
        typename enable_if
        <
            is_convertible<U(*)[], T(*)[]>::value,
            void
        >::type
        operator()(U* _p) const
        {
            typedef char _U_must_be_complete_type[sizeof(U) ? 1 : -1];
            (void)sizeof(_U_must_be_complete_type);

            delete[] _p;
        }

    #else

        // C++98/03 fallback: only the same-type operator() is provided.
        // No covariant array delete.
        void operator()(T* _p) const
        {
            typedef char _T_must_be_complete_type[sizeof(T) ? 1 : -1];
            (void)sizeof(_T_must_be_complete_type);

            delete[] _p;
        }

    #endif
};


}  // re_std
#endif  // RE_STD_MEMORY_DEFAULT_DELETE_HPP
