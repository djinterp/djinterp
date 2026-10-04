/***********************************************************************
* re_std                                                         swap.hpp
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
* D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES; on the rare
* rvalue-references-without-variadic-templates compiler, the swap
* function is unqualified rather than falsely noexcept.
*
*
* path:      /inc/djinterp/re_std/utility/swap.hpp
* link(s):   TBA
* author(s): re_std team                                date: 2026.04.30
***********************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_SWAP_
#define DJINTERP_RE_STD_UTILITY_SWAP_ 1

#include "djinterp.hpp"

#include <cstddef>  // std::size_t

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/move.hpp"
#endif

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    #include "../type_traits/is_nothrow_move_constructible.hpp"
    #include "../type_traits/is_nothrow_move_assignable.hpp"
#endif

NS_RESTD

// =============================================================================
// SWAP -- SCALAR
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    #if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

        // swap (C++14+: constexpr, move-based, conditional noexcept)
        //   function: exchanges _lhs and _rhs using move semantics.
        template<typename _Type>
        D_CONSTEXPR void swap(_Type& _lhs,
                              _Type& _rhs) noexcept(
            is_nothrow_move_constructible<_Type>::value &&
            is_nothrow_move_assignable<_Type>::value)
        {
            _Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #else  // rvalue references but no variadic templates

        // swap (C++14+ without variadic templates: constexpr, move-based)
        //   no noexcept clause -- the requisite is_nothrow_* traits
        //   are not available without variadic templates.
        template<typename _Type>
        D_CONSTEXPR void swap(_Type& _lhs,
                              _Type& _rhs)
        {
            _Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #endif

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

    #if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

        // swap (C++11: move-based, conditional noexcept)
        //   function: exchanges _lhs and _rhs using move semantics.
        template<typename _Type>
        void swap(_Type& _lhs,
                  _Type& _rhs) noexcept(
            is_nothrow_move_constructible<_Type>::value &&
            is_nothrow_move_assignable<_Type>::value)
        {
            _Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #else  // rvalue references but no variadic templates

        // swap (C++11 without variadic templates: move-based)
        template<typename _Type>
        void swap(_Type& _lhs,
                  _Type& _rhs)
        {
            _Type _tmp(re_std::move(_lhs));
            _lhs = re_std::move(_rhs);
            _rhs = re_std::move(_tmp);
            return;
        }

    #endif

#else

    // swap (C++98/03: copy-based)
    //   function: exchanges _lhs and _rhs via copy.
    template<typename _Type>
    void swap(_Type& _lhs,
              _Type& _rhs)
    {
        _Type _tmp(_lhs);
        _lhs = _rhs;
        _rhs = _tmp;
        return;
    }

#endif

// =============================================================================
// SWAP -- ARRAY
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // swap (C++14+ array overload: constexpr)
    //   function: element-wise exchanges two arrays of the same extent.
    //   No noexcept clause: would need is_nothrow_swappable, which
    //   re_std does not yet provide.
    template<typename _Type, std::size_t _Size>
    D_CONSTEXPR void swap(_Type (&_lhs)[_Size],
                          _Type (&_rhs)[_Size])
    {
        for (std::size_t _i = 0; _i < _Size; ++_i)
        {
            swap(_lhs[_i], _rhs[_i]);
        }
        return;
    }

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

    // swap (C++11 array overload)
    template<typename _Type, std::size_t _Size>
    void swap(_Type (&_lhs)[_Size],
              _Type (&_rhs)[_Size])
    {
        for (std::size_t _i = 0; _i < _Size; ++_i)
        {
            swap(_lhs[_i], _rhs[_i]);
        }
        return;
    }

#else

    // swap (C++98/03 array overload)
    template<typename _Type, std::size_t _Size>
    void swap(_Type (&_lhs)[_Size],
              _Type (&_rhs)[_Size])
    {
        for (std::size_t _i = 0; _i < _Size; ++_i)
        {
            swap(_lhs[_i], _rhs[_i]);
        }
        return;
    }

#endif

NS_END  // re_std

#endif  // DJINTERP_RE_STD_UTILITY_SWAP_
