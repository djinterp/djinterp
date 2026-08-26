/******************************************************************************
* djinterp [re_std]                                      bad_expected_access.hpp
*
* bad_expected_access exception header:
*   Provides the exception family thrown by expected<T, E>::value() when
* the expected holds no value. Mirrors C++23 std::bad_expected_access:
*
*     bad_expected_access<void>      - abstract base, no payload
*     bad_expected_access<E>         - carries the unexpected E value
*
*   The base class is selected based on available headers (same pattern
* as bad_any_cast):
*     <typeinfo>  available -> inherits std::bad_cast (-> std::exception)
*     <exception> available -> inherits std::exception
*     neither               -> standalone (no base, non-virtual what())
*
*   bad_expected_access<E> is what user code catches when a specific
* error type is involved; catching bad_expected_access<void>& catches
* any expected access failure regardless of E.
*
*
* path:      /inc/djinterp/re_std/expected/bad_expected_access.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RE_STD_BAD_EXPECTED_ACCESS_
#define DJINTERP_RE_STD_BAD_EXPECTED_ACCESS_ 1

#include "../../core/djinterp.hpp"

// gate: the entire <expected> module is C++11+. bad_expected_access
// uses ref-qualified accessors (C++11 feature) and is only thrown
// by expected, so we gate consistently.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// ===========================================================================
// 0.   CONDITIONAL INCLUDES
// ===========================================================================

#if D_ENV_CPP98_HAS_TYPEINFO
    #include <typeinfo>
#elif D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#endif


NS_RESTD


// ===========================================================================
// I.   BAD_EXPECTED_ACCESS<void>  (base class)
// ===========================================================================

#if D_ENV_CPP98_HAS_TYPEINFO

// bad_expected_access<void>
//   exception: base class for the expected-access exception family.
// inherits: std::bad_cast -> std::exception.
template<typename _E = void>
class bad_expected_access;

template<>
class bad_expected_access<void> : public std::bad_cast
{
protected:
    bad_expected_access() {}
    bad_expected_access(bad_expected_access const&) {}
    bad_expected_access& operator=(bad_expected_access const&) { return *this; }
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    ~bad_expected_access() D_NOEXCEPT override {}
#else
    ~bad_expected_access() throw() {}
#endif
public:
    const char*
    what() const D_NOEXCEPT
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        override
#endif
    {
        return "bad expected access";
    }
};

#elif D_ENV_CPP98_HAS_EXCEPTION

template<typename _E = void>
class bad_expected_access;

template<>
class bad_expected_access<void> : public std::exception
{
protected:
    bad_expected_access() {}
    bad_expected_access(bad_expected_access const&) {}
    bad_expected_access& operator=(bad_expected_access const&) { return *this; }
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    ~bad_expected_access() D_NOEXCEPT override {}
#else
    ~bad_expected_access() throw() {}
#endif
public:
    const char*
    what() const D_NOEXCEPT
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        override
#endif
    {
        return "bad expected access";
    }
};

#else

template<typename _E = void>
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
    what() const D_NOEXCEPT
    {
        return "bad expected access";
    }
};

#endif  // D_ENV_CPP98_HAS_TYPEINFO / D_ENV_CPP98_HAS_EXCEPTION


// ===========================================================================
// II.  BAD_EXPECTED_ACCESS<E>  (carries the error payload)
// ===========================================================================

// bad_expected_access<E>
//   exception: thrown by expected<T, E>::value() when *this holds no
// value. Carries a copy of the unexpected error so the catch site
// can inspect it via .error().
// inherits: bad_expected_access<void>.
template<typename _E>
class bad_expected_access : public bad_expected_access<void>
{
public:
    // ctor (forwarding) — store the error.
    explicit bad_expected_access(_E _e)
        : m_error(static_cast<_E&&>(_e))
    {}

    // error (lvalue mutable) — access the stored error.
    _E& error() & D_NOEXCEPT
    {
        return m_error;
    }

    // error (lvalue const)
    _E const& error() const & D_NOEXCEPT
    {
        return m_error;
    }

    // error (rvalue) — move the stored error out.
    _E&& error() && D_NOEXCEPT
    {
        return static_cast<_E&&>(m_error);
    }

    // error (const rvalue) — rarely useful but standardised.
    _E const&& error() const && D_NOEXCEPT
    {
        return static_cast<_E const&&>(m_error);
    }

private:
    _E m_error;
};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BAD_EXPECTED_ACCESS_
