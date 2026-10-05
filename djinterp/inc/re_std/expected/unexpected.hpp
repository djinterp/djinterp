/*******************************************************************************
* djinterp [re_std]                                               unexpected.hpp
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
* path:      /inc/re_std/expected/unexpected.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXPECTED_UNEXPECTED_HPP
#define RE_STD_EXPECTED_UNEXPECTED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// gate: C++11+ baseline. Pre-C++11 not supported.
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <initializer_list>
// std (std::swap, for the two-step swap idiom below)
// std
#include <utility>

#include "../optional/in_place.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/decay.hpp"


namespace re_std
{


// ===========================================================================
// I.   UNEXPECTED<E>
// ===========================================================================

// unexpected<E>
//   class: wraps an error value of type E. Used by expected<T, E>
// to disambiguate error construction from value construction.
template<typename E>
class unexpected
{
public:
    // =================================================================
    // MEMBER TYPES
    // =================================================================

    typedef E error_type;

    // =================================================================
    // CTORS
    // =================================================================

    // copy / move — defaulted; transitively defaulted on E.
    unexpected(unexpected const&) = default;
    unexpected(unexpected&&)      = default;

    // (1) forwarding-from-Err ctor
    //   Selected when Err is something other than unexpected itself
    // and in_place_t, and E is constructible from Err.
    template<typename Err = E,
             typename = typename re_std::enable_if<
                 !re_std::is_same<typename re_std::decay<Err>::type, unexpected>::value &&
                 !re_std::is_same<typename re_std::decay<Err>::type, in_place_t>::value &&
                 re_std::is_constructible<E, Err>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit unexpected(Err&& _err)
        : m_error(static_cast<Err&&>(_err))
    {}

    // (2) in_place ctor — emplaces E from forwarded args.
    template<typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit unexpected(in_place_t, Args&&... _args)
        : m_error(static_cast<Args&&>(_args)...)
    {}

    // (3) in_place + initializer_list ctor — for E types built from
    // an initializer_list plus optional extra args (e.g. std::vector).
    template<typename U,
             typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, std::initializer_list<U>&, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit unexpected(
        in_place_t,
        std::initializer_list<U> _il,
        Args&&... _args
    )
        : m_error(_il, static_cast<Args&&>(_args)...)
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
    RE_STD_CONSTEXPR_CPP20 E& error() & RE_STD_NOEXCEPT
    {
        return m_error;
    }

    // error (lvalue const)
    RE_STD_CONSTEXPR E const& error() const & RE_STD_NOEXCEPT
    {
        return m_error;
    }

    // error (rvalue mutable)
    RE_STD_CONSTEXPR_CPP20 E&& error() && RE_STD_NOEXCEPT
    {
        return static_cast<E&&>(m_error);
    }

    // error (rvalue const)
    RE_STD_CONSTEXPR E const&& error() const && RE_STD_NOEXCEPT
    {
        return static_cast<E const&&>(m_error);
    }

    // =================================================================
    // SWAP
    // =================================================================

    // swap
    //   function: exchanges this->m_error with _other.m_error via
    // ADL swap (or std::swap fallback).
    RE_STD_CONSTEXPR_CPP20 void
    swap(
        unexpected& _other
    ) RE_STD_NOEXCEPT
    {
        using std::swap;
        swap(m_error, _other.m_error);

        return;
    }

private:

    E m_error;
};


// ===========================================================================
// II.  DEDUCTION GUIDE (C++17+)
// ===========================================================================

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

template<typename E>
unexpected(E) -> unexpected<E>;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_EXPECTED_UNEXPECTED_HPP
