/***********************************************************************
* re_std                                                         pair.hpp
*
* heterogeneous two-value aggregate:
*   Provides re_std::pair<T1, T2>, the canonical heterogeneous pair.
* Holds two public members `first` and `second`. The full set of
* comparison operators (==, !=, <, <=, >, >=) and a non-member swap
* are also defined here; tuple_size / tuple_element specializations
* are not provided (no <tuple> exists in re_std yet).
*
*   Tiered implementation:
*     C++11+   adds the perfect-forwarding ctor, move ctor, and
*              their templated converting variants. Constructors and
*              comparison operators are constexpr-qualified (single
*              return / member-init-list bodies -- C++11 constexpr-
*              compliant). Move ctor / move assignment carry the
*              standard's conditional-noexcept clauses where the
*              required is_nothrow_* traits are available.
*     C++98/03 default ctor + (const T1&, const T2&) ctor + templated
*              converting copy ctor. No move support.
*
*   noexcept caveats:
*     - Move ctor's conditional noexcept needs
*       is_nothrow_move_constructible (variadic-templates-gated).
*       Falls back to unqualified on rvalue-refs-without-variadic
*       compilers.
*     - Move assignment uses is_nothrow_move_assignable (C++11+).
*     - Member swap stays unqualified pending is_nothrow_swappable.
*
*   piecewise_construct: not provided (depends on tuple).
*
*
* path:      /inc/djinterp/re_std/utility/pair.hpp
* link(s):   TBA
* author(s): re_std team                                date: 2026.04.30
***********************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_PAIR_
#define DJINTERP_RE_STD_UTILITY_PAIR_ 1

#include "djinterp.hpp"
#include "../utility/swap.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/move.hpp"
    #include "../utility/forward.hpp"
    #include "../type_traits/is_nothrow_move_assignable.hpp"
#endif

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    #include "../type_traits/is_nothrow_move_constructible.hpp"
#endif

NS_RESTD

// =============================================================================
// PAIR
// =============================================================================

// pair
//   class: heterogeneous two-value aggregate.
template<typename _T1, typename _T2>
struct pair
{
    // -----------------------------------------------------------------
    //  member typedefs
    // -----------------------------------------------------------------

    typedef _T1 first_type;
    typedef _T2 second_type;

    // -----------------------------------------------------------------
    //  data members (public, by std convention)
    // -----------------------------------------------------------------

    _T1 first;
    _T2 second;

    // -----------------------------------------------------------------
    //  constructors
    // -----------------------------------------------------------------

    // pair()
    //   ctor: value-initializes first and second.
    D_CONSTEXPR pair()
        : first(),
          second()
    {}

    // pair(const T1&, const T2&)
    //   ctor: copies from explicit values.
    D_CONSTEXPR pair(const _T1& _x,
                     const _T2& _y)
        : first(_x),
          second(_y)
    {}

    // pair(const pair<U1,U2>&)
    //   ctor: implicit conversion from a compatible pair.
    template<typename _U1, typename _U2>
    D_CONSTEXPR pair(const pair<_U1, _U2>& _other)
        : first(_other.first),
          second(_other.second)
    {}

    // copy ctor -- left implicit (compiler-generated, member-wise).

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // pair(U1&&, U2&&)
    //   ctor: perfect-forwarding from arbitrary argument pair.
    template<typename _U1, typename _U2>
    D_CONSTEXPR pair(_U1&& _x,
                     _U2&& _y)
        : first(re_std::forward<_U1>(_x)),
          second(re_std::forward<_U2>(_y))
    {}

  #if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

    // pair(pair&&)
    //   ctor: move ctor with conditional noexcept.
    //
    //   Members are forwarded as _T1&& / _T2&& rather than passed through
    // re_std::move. For an ordinary object type the two are identical, but
    // when _T1 is a REFERENCE type -- pair<int&, int>, which make_pair
    // produces from a reference_wrapper argument -- move() yields int&&,
    // and an int&& cannot initialise the int& member. Reference collapsing
    // makes _T1&& collapse back to int&, so this spelling handles both
    // cases. (C++17 hid the bug behind guaranteed copy elision; C++11 and
    // C++14 still run this constructor.)
    D_CONSTEXPR pair(pair&& _other) noexcept(
        is_nothrow_move_constructible<_T1>::value &&
        is_nothrow_move_constructible<_T2>::value)
        : first(static_cast<_T1&&>(_other.first)),
          second(static_cast<_T2&&>(_other.second))
    {}

    // pair(pair<U1,U2>&&)
    //   ctor: converting move ctor with conditional noexcept.
    template<typename _U1, typename _U2>
    D_CONSTEXPR pair(pair<_U1, _U2>&& _other) noexcept(
        is_nothrow_move_constructible<_T1>::value &&
        is_nothrow_move_constructible<_T2>::value)
        : first(re_std::move(_other.first)),
          second(re_std::move(_other.second))
    {}

  #else  // rvalue refs without variadic templates -- no nothrow-ctor trait

    // pair(pair&&)
    //   ctor: move ctor (no noexcept -- is_nothrow_move_constructible
    //   unavailable without variadic templates).
    D_CONSTEXPR pair(pair&& _other)
        : first(re_std::move(_other.first)),
          second(re_std::move(_other.second))
    {}

    // pair(pair<U1,U2>&&)
    //   ctor: converting move ctor (no noexcept).
    template<typename _U1, typename _U2>
    D_CONSTEXPR pair(pair<_U1, _U2>&& _other)
        : first(re_std::move(_other.first)),
          second(re_std::move(_other.second))
    {}

  #endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

#endif  // rvalue references

    // -----------------------------------------------------------------
    //  assignment
    // -----------------------------------------------------------------

    // operator=(const pair&)
    //   assign: member-wise copy.
    pair& operator=(const pair& _other)
    {
        first = _other.first;
        second = _other.second;
        return *this;
    }

    // operator=(const pair<U1,U2>&)
    //   assign: member-wise copy from compatible pair.
    template<typename _U1, typename _U2>
    pair& operator=(const pair<_U1, _U2>& _other)
    {
        first = _other.first;
        second = _other.second;
        return *this;
    }

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // operator=(pair&&)
    //   assign: member-wise move with conditional noexcept.
    pair& operator=(pair&& _other) noexcept(
        is_nothrow_move_assignable<_T1>::value &&
        is_nothrow_move_assignable<_T2>::value)
    {
        first = re_std::move(_other.first);
        second = re_std::move(_other.second);
        return *this;
    }

    // operator=(pair<U1,U2>&&)
    //   assign: member-wise move from compatible pair (conditional noexcept).
    template<typename _U1, typename _U2>
    pair& operator=(pair<_U1, _U2>&& _other) noexcept(
        is_nothrow_move_assignable<_T1>::value &&
        is_nothrow_move_assignable<_T2>::value)
    {
        first = re_std::move(_other.first);
        second = re_std::move(_other.second);
        return *this;
    }

#endif  // rvalue references

    // -----------------------------------------------------------------
    //  swap
    // -----------------------------------------------------------------

    // swap
    //   function: member-wise swap with another pair. No noexcept --
    //   would need is_nothrow_swappable, which re_std does not yet
    //   provide.
    void swap(pair& _other)
    {
        re_std::swap(first, _other.first);
        re_std::swap(second, _other.second);
        return;
    }
};

// =============================================================================
// COMPARISON OPERATORS
// =============================================================================
//
// Lexicographic ordering: (a.first, a.second) compares as a 2-tuple.

// operator==
//   function: equal iff both members are equal.
template<typename _T1, typename _T2>
D_CONSTEXPR bool operator==(const pair<_T1, _T2>& _lhs,
                            const pair<_T1, _T2>& _rhs)
{
    return _lhs.first == _rhs.first && _lhs.second == _rhs.second;
}

// operator!=
//   function: negation of operator==.
template<typename _T1, typename _T2>
D_CONSTEXPR bool operator!=(const pair<_T1, _T2>& _lhs,
                            const pair<_T1, _T2>& _rhs)
{
    return !(_lhs == _rhs);
}

// operator<
//   function: lexicographic less-than. Equivalent to comparing
//   (first, second) as a 2-tuple. Implemented in terms of operator<
//   only, so it works for types that supply only operator<.
template<typename _T1, typename _T2>
D_CONSTEXPR bool operator<(const pair<_T1, _T2>& _lhs,
                           const pair<_T1, _T2>& _rhs)
{
    return _lhs.first < _rhs.first
        || (!(_rhs.first < _lhs.first) && _lhs.second < _rhs.second);
}

// operator<=
//   function: !(rhs < lhs).
template<typename _T1, typename _T2>
D_CONSTEXPR bool operator<=(const pair<_T1, _T2>& _lhs,
                            const pair<_T1, _T2>& _rhs)
{
    return !(_rhs < _lhs);
}

// operator>
//   function: rhs < lhs.
template<typename _T1, typename _T2>
D_CONSTEXPR bool operator>(const pair<_T1, _T2>& _lhs,
                           const pair<_T1, _T2>& _rhs)
{
    return _rhs < _lhs;
}

// operator>=
//   function: !(lhs < rhs).
template<typename _T1, typename _T2>
D_CONSTEXPR bool operator>=(const pair<_T1, _T2>& _lhs,
                            const pair<_T1, _T2>& _rhs)
{
    return !(_lhs < _rhs);
}

// =============================================================================
// NON-MEMBER SWAP
// =============================================================================

// swap (pair overload)
//   function: ADL-friendly swap. Delegates to the member swap.
template<typename _T1, typename _T2>
void swap(pair<_T1, _T2>& _lhs,
          pair<_T1, _T2>& _rhs)
{
    _lhs.swap(_rhs);
    return;
}

NS_END  // re_std

#endif  // DJINTERP_RE_STD_UTILITY_PAIR_
