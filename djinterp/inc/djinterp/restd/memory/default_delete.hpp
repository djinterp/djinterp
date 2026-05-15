/***********************************************************************
* restd                                                 default_delete.hpp
*
* the default deleter for unique_ptr:
*   default_delete<_T>     -  calls `delete _p` on its argument.
*   default_delete<_T[]>   -  calls `delete[] _p` on its argument,
*                             SFINAE-restricted to convertible types.
*
* both specialisations:
*   - are default-constructible and trivially copyable;
*   - require _T to be a complete type at the point operator() is
*     instantiated (this is not optional - deleting an incomplete-type
*     pointer is undefined behaviour, and the static_assert here
*     catches it at compile time);
*   - have a templated converting constructor on C++11+, gated on
*     restd::is_convertible. The array specialisation's converting
*     constructor uses the array-of-pointer-to-array form
*     (_U(*)[] -> _T(*)[]) per [unique.ptr.dltr.dflt1]/2.
*
* C++98/03 path:
*   The class itself ships, but the converting constructor is omitted
* (it requires is_convertible, which is C++11+ in restd). This is
* sufficient for unique_ptr's basic use; covariant deleter conversions
* are a C++11+ feature.
*
*
* path:      /inc/restd/memory/default_delete.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_DEFAULT_DELETE_
#define RESTD_MEMORY_DEFAULT_DELETE_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "restd/type_traits/enable_if.hpp"
    #include "restd/type_traits/is_convertible.hpp"
#endif


namespace restd
{

// =============================================================================
// default_delete  -  scalar specialisation
// =============================================================================

// default_delete<_T>
//   class: invokes `delete _p` on its argument.
template<typename _T>
struct default_delete
{
    // Default ctor.
    D_CONSTEXPR default_delete() D_NOEXCEPT
    {
    }

    #if D_ENV_LANG_IS_CPP11_OR_HIGHER

        // Converting ctor: enabled when _U* is convertible to _T*.
        // This is what lets you build a default_delete<Base> from a
        // default_delete<Derived>.
        template<typename _U>
        default_delete
        (
            const default_delete<_U>&,
            typename enable_if
            <
                is_convertible<_U*, _T*>::value,
                int
            >::type = 0
        ) D_NOEXCEPT
        {
        }

    #endif

    // operator()
    //   function: calls `delete _p`. _T must be complete here.
    void operator()(_T* _p) const
    {
        // Force a hard error if _T is incomplete. The sizeof check
        // is the canonical idiom: incomplete types have no size, so
        // the array-bound expression is ill-formed.
        typedef char _T_must_be_complete_type[sizeof(_T) ? 1 : -1];
        (void)sizeof(_T_must_be_complete_type);

        delete _p;
    }
};


// =============================================================================
// default_delete<_T[]>  -  array specialisation
// =============================================================================

// default_delete<_T[]>
//   class: invokes `delete[] _p` on its argument. The converting
//   ctor and operator() are SFINAE-restricted to types that are
//   array-of-pointer-to-array convertible to _T[], not merely
//   convertible to _T*. This is what makes the array form refuse
//   covariant Derived[] -> Base[] conversion (which would be a
//   violation of C-style array layout invariants).
template<typename _T>
struct default_delete<_T[]>
{
    D_CONSTEXPR default_delete() D_NOEXCEPT
    {
    }

    #if D_ENV_LANG_IS_CPP11_OR_HIGHER

        template<typename _U>
        default_delete
        (
            const default_delete<_U[]>&,
            typename enable_if
            <
                is_convertible<_U(*)[], _T(*)[]>::value,
                int
            >::type = 0
        ) D_NOEXCEPT
        {
        }

        template<typename _U>
        typename enable_if
        <
            is_convertible<_U(*)[], _T(*)[]>::value,
            void
        >::type
        operator()(_U* _p) const
        {
            typedef char _U_must_be_complete_type[sizeof(_U) ? 1 : -1];
            (void)sizeof(_U_must_be_complete_type);

            delete[] _p;
        }

    #else

        // C++98/03 fallback: only the same-type operator() is provided.
        // No covariant array delete.
        void operator()(_T* _p) const
        {
            typedef char _T_must_be_complete_type[sizeof(_T) ? 1 : -1];
            (void)sizeof(_T_must_be_complete_type);

            delete[] _p;
        }

    #endif
};


}  // namespace restd

#endif  // RESTD_MEMORY_DEFAULT_DELETE_
