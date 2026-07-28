/******************************************************************************
* djinterp [restd]                                                unexpected.hpp
*
* unexpected wrapper header:
*   Provides unexpected<E> — a typed wrapper around an error value of
* type E. Two roles:
*
*     1. CONSTRUCTOR HINT for expected<T, E>:
*          expected<int, std::string> e = unexpected<std::string>("nope");
*        expected has a ctor that takes unexpected<G>; the wrapping is
*        what disambiguates "construct as an error" from "construct as
*        a value" when T and E are convertible from the same source.
*
*     2. RETURN VEHICLE from functions that produce errors:
*          expected<int, std::string> parse(...) {
*              if (bad) return unexpected<std::string>("invalid");
*              return 42;
*          }
*        Lets the function signature stay focused on expected<T, E>
*        while still being able to construct the error path cleanly.
*
*   PORTABILITY:
*   C++11+. Variadic templates, rvalue refs, default-member-init are
* all required.
*
*
* path:      /inc/djinterp/restd/expected/unexpected.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_UNEXPECTED_
#define DJINTERP_RESTD_UNEXPECTED_ 1

#include "../../core/djinterp.hpp"

// gate: C++11+ baseline. Pre-C++11 not supported.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <initializer_list>

#include "../utility/in_place.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/decay.hpp"


#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   UNEXPECTED<E>
// ===========================================================================

// unexpected<E>
//   class: wraps an error value of type _E. Used by expected<T, E>
// to disambiguate error construction from value construction.
template<typename _E>
class unexpected
{
public:
    // =================================================================
    // MEMBER TYPES
    // =================================================================

    typedef _E error_type;

    // =================================================================
    // CTORS
    // =================================================================

    // copy / move — defaulted; transitively defaulted on _E.
    unexpected(unexpected const&) = default;
    unexpected(unexpected&&)      = default;

    // (1) forwarding-from-Err ctor
    //   Selected when _Err is something other than unexpected itself
    // and in_place_t, and _E is constructible from _Err.
    template<typename _Err = _E,
             typename = typename restd::enable_if<
                 !restd::is_same<typename restd::decay<_Err>::type, unexpected>::value &&
                 !restd::is_same<typename restd::decay<_Err>::type, in_place_t>::value &&
                 restd::is_constructible<_E, _Err>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit unexpected(_Err&& _err)
        : m_error(static_cast<_Err&&>(_err))
    {}

    // (2) in_place ctor — emplaces _E from forwarded args.
    template<typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit unexpected(in_place_t, _Args&&... _args)
        : m_error(static_cast<_Args&&>(_args)...)
    {}

    // (3) in_place + initializer_list ctor — for _E types built from
    // an initializer_list plus optional extra args (e.g. std::vector).
    template<typename _U,
             typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, std::initializer_list<_U>&, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit unexpected(
        in_place_t,
        std::initializer_list<_U> _il,
        _Args&&... _args
    )
        : m_error(_il, static_cast<_Args&&>(_args)...)
    {}

    // =================================================================
    // ASSIGNMENT
    // =================================================================

    unexpected& operator=(unexpected const&) = default;
    unexpected& operator=(unexpected&&)      = default;

    // =================================================================
    // ACCESSORS
    // =================================================================

    // error (lvalue mutable)
    D_CONSTEXPR_CPP20 _E& error() & D_NOEXCEPT
    {
        return m_error;
    }

    // error (lvalue const)
    D_CONSTEXPR _E const& error() const & D_NOEXCEPT
    {
        return m_error;
    }

    // error (rvalue mutable)
    D_CONSTEXPR_CPP20 _E&& error() && D_NOEXCEPT
    {
        return static_cast<_E&&>(m_error);
    }

    // error (rvalue const)
    D_CONSTEXPR _E const&& error() const && D_NOEXCEPT
    {
        return static_cast<_E const&&>(m_error);
    }

    // =================================================================
    // SWAP
    // =================================================================

    // swap
    //   function: exchanges this->m_error with _other.m_error via
    // ADL swap (or std::swap fallback).
    D_CONSTEXPR_CPP20 void
    swap(
        unexpected& _other
    ) D_NOEXCEPT
    {
        using std::swap;
        swap(m_error, _other.m_error);

        return;
    }

private:

    _E m_error;
};


// ===========================================================================
// II.  DEDUCTION GUIDE (C++17+)
// ===========================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<typename _E>
unexpected(_E) -> unexpected<_E>;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_UNEXPECTED_
