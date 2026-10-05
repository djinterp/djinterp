/*******************************************************************************
* djinterp [re_std]                                      bad_expected_access.hpp
*
* bad_expected_access exception header:
*   Provides the exception family thrown by expected<T, E>::value() when
* the expected holds no value. Mirrors C++23 std::bad_expected_access:
*
*     bad_expected_access<void>      - abstract base, no payload
*     bad_expected_access<E>         - carries the unexpected E value
*
*   The base class follows the exception setting:
*     exceptions on  -> inherits std::exception, as [expected.bad.void]
*                       specifies (not std::bad_cast, which a catch of
*                       bad_cast would wrongly take)
*     exceptions off -> standalone (no base, non-virtual what())
*
*   bad_expected_access<E> is what user code catches when a specific
* error type is involved; catching bad_expected_access<void>& catches
* any expected access failure regardless of E.
*
*
* path:      /inc/re_std/expected/bad_expected_access.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXPECTED_BAD_EXPECTED_ACCESS_HPP
#define RE_STD_EXPECTED_BAD_EXPECTED_ACCESS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// gate: the entire <expected> module is C++11+. bad_expected_access
// uses ref-qualified accessors (C++11 feature) and is only thrown
// by expected, so we gate consistently.
#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// ===========================================================================
// 0.   CONDITIONAL INCLUDES
// ===========================================================================

#if RE_STD_HAS_EXCEPTIONS
    // std
    #include <exception>
#endif


namespace re_std
{


// ===========================================================================
// I.   BAD_EXPECTED_ACCESS<void>  (base class)
// ===========================================================================

#if RE_STD_HAS_EXCEPTIONS

template<typename E = void>
class bad_expected_access;

template<>
class bad_expected_access<void> : public std::exception
{
protected:
    bad_expected_access() {}
    bad_expected_access(bad_expected_access const&) {}
    bad_expected_access& operator=(bad_expected_access const&) { return *this; }
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    ~bad_expected_access() RE_STD_NOEXCEPT override {}
#else
    ~bad_expected_access() throw() {}
#endif
public:
    const char*
    what() const RE_STD_NOEXCEPT
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        override
#endif
    {
        return "bad expected access";
    }
};

#else

template<typename E = void>
class bad_expected_access;

template<>
class bad_expected_access<void>
{
protected:
    bad_expected_access() {}
    bad_expected_access(bad_expected_access const&) {}
    bad_expected_access& operator=(bad_expected_access const&) { return *this; }
    ~bad_expected_access() {}
public:
    const char*
    what() const RE_STD_NOEXCEPT
    {
        return "bad expected access";
    }
};

#endif  // RE_STD_HAS_EXCEPTIONS


// ===========================================================================
// II.  BAD_EXPECTED_ACCESS<E>  (carries the error payload)
// ===========================================================================

// bad_expected_access<E>
//   class: thrown by expected<T, E>::value() when *this holds no
// value. Carries a copy of the unexpected error so the catch site
// can inspect it via .error().
// inherits: bad_expected_access<void>.
template<typename E>
class bad_expected_access : public bad_expected_access<void>
{
public:
    // ctor (forwarding) — store the error.
    explicit bad_expected_access(E _e)
        : m_error(static_cast<E&&>(_e))
    {}

    // error (lvalue mutable) — access the stored error.
    E& error() & RE_STD_NOEXCEPT
    {
        return m_error;
    }

    // error (lvalue const)
    E const& error() const & RE_STD_NOEXCEPT
    {
        return m_error;
    }

    // error (rvalue) — move the stored error out.
    E&& error() && RE_STD_NOEXCEPT
    {
        return static_cast<E&&>(m_error);
    }

    // error (const rvalue) — rarely useful but standardised.
    E const&& error() const && RE_STD_NOEXCEPT
    {
        return static_cast<E const&&>(m_error);
    }

private:
    E m_error;
};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_EXPECTED_BAD_EXPECTED_ACCESS_HPP
