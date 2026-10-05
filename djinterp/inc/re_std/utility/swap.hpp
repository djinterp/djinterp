/*******************************************************************************
* djinterp [re_std]                                                     swap.hpp
*
* generic swap algorithm:
*   Exchanges the values of two objects of the same type. Provides
* both the scalar overload (swap(T&, T&)) and the array overload
* (swap(T(&)[N], T(&)[N])), which performs an element-wise swap.
*
*   Tiered implementation:
*     C++14+   constexpr, move-based, conditional noexcept
*     C++11    move-based, conditional noexcept (not constexpr -- move
*              ctor + two assignments would be a multi-statement
*              constexpr body, not permitted before C++14)
*     C++98/03 copy-based, no qualifiers
*
*   This is the GENERIC swap. Type-specific overloads (such as
* re_std::swap(any&, any&) in any/any_swap.hpp) are found via ADL or
* unqualified-name lookup and take precedence per the usual two-step
* swap idiom.
*
*   noexcept on C++11+ matches the standard's
*   noexcept(is_nothrow_move_constructible<T>::value &&
*           is_nothrow_move_assignable<T>::value)
* form. Because is_nothrow_move_constructible requires variadic
* templates, the conditional clause is gated on
* RE_STD_LANG_HAS_VARIADIC_TEMPLATES; on the rare
* rvalue-references-without-variadic-templates compiler, the swap
* function is unqualified rather than falsely noexcept.
*
*
* path:      /inc/re_std/utility/swap.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_SWAP_HPP
#define RE_STD_UTILITY_SWAP_HPP 1

// std
#include <cstddef>  // std::size_t
// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "move.hpp"
#endif

#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES
    #include "../type_traits/is_nothrow_move_constructible.hpp"
    #include "../type_traits/is_nothrow_move_assignable.hpp"
#endif

namespace re_std
{

// =============================================================================
// SWAP -- SCALAR
// =============================================================================

#if RE_STD_LANG_IS_CPP14_OR_HIGHER

    #if RE_STD_LANG_HAS_VARIADIC_TEMPLATES

        // swap (C++14+: constexpr, move-based, conditional noexcept)
        //   function: exchanges _lhs and _rhs using move semantics.
        template<typename Type>
        RE_STD_CONSTEXPR void swap(Type& _lhs,
                              Type& _rhs) noexcept(
            is_nothrow_move_constructible<Type>::value &&
            is_nothrow_move_assignable<Type>::value)
        {
            Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #else  // rvalue references but no variadic templates

        // swap (C++14+ without variadic templates: constexpr, move-based)
        //   no noexcept clause -- the requisite is_nothrow_* traits
        //   are not available without variadic templates.
        template<typename Type>
        RE_STD_CONSTEXPR void swap(Type& _lhs,
                              Type& _rhs)
        {
            Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #endif

#elif RE_STD_LANG_IS_CPP11_OR_HIGHER

    #if RE_STD_LANG_HAS_VARIADIC_TEMPLATES

        // swap (C++11: move-based, conditional noexcept)
        //   function: exchanges _lhs and _rhs using move semantics.
        template<typename Type>
        void swap(Type& _lhs,
                  Type& _rhs) noexcept(
            is_nothrow_move_constructible<Type>::value &&
            is_nothrow_move_assignable<Type>::value)
        {
            Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #else  // rvalue references but no variadic templates

        // swap (C++11 without variadic templates: move-based)
        template<typename Type>
        void swap(Type& _lhs,
                  Type& _rhs)
        {
            Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #endif

#else

    // swap (C++98/03: copy-based)
    //   function: exchanges _lhs and _rhs via copy.
    template<typename Type>
    void swap(Type& _lhs,
              Type& _rhs)
    {
        Type _tmp(_lhs);
        _lhs = _rhs;
        _rhs = _tmp;
        return;
    }

#endif

// =============================================================================
// SWAP -- ARRAY
// =============================================================================

#if RE_STD_LANG_IS_CPP14_OR_HIGHER

    // swap (C++14+ array overload: constexpr)
    //   function: element-wise exchanges two arrays of the same extent.
    //   No noexcept clause: would need is_nothrow_swappable, which
    //   re_std does not yet provide.
    template<typename Type, std::size_t Size>
    RE_STD_CONSTEXPR void swap(Type (&_lhs)[Size],
                          Type (&_rhs)[Size])
    {
        for (std::size_t _i = 0; _i < Size; ++_i)
        {
            swap(_lhs[_i], _rhs[_i]);
        }
        return;
    }

#elif RE_STD_LANG_IS_CPP11_OR_HIGHER

    // swap (C++11 array overload)
    template<typename Type, std::size_t Size>
    void swap(Type (&_lhs)[Size],
              Type (&_rhs)[Size])
    {
        for (std::size_t _i = 0; _i < Size; ++_i)
        {
            swap(_lhs[_i], _rhs[_i]);
        }
        return;
    }

#else

    // swap (C++98/03 array overload)
    template<typename Type, std::size_t Size>
    void swap(Type (&_lhs)[Size],
              Type (&_rhs)[Size])
    {
        for (std::size_t _i = 0; _i < Size; ++_i)
        {
            swap(_lhs[_i], _rhs[_i]);
        }
        return;
    }

#endif

}  // re_std

#endif  // RE_STD_UTILITY_SWAP_HPP
