/*******************************************************************************
* djinterp [re_std]                                                 expected.hpp
*
* expected class header:
*   re_std's back-port of std::expected<T, E> — a discriminated union
* holding either a value (type T) or an unexpected error (type E).
* C++23 in std; re_std targets C++11+ for runtime correctness, with
* constexpr promoted to C++20 (where placement-new becomes constexpr-
* accessible via std::construct_at).
*
*   STORAGE MODEL:
*   A private anonymous union holds either m_val (the value) or
* m_err (the error); a separate bool m_has_value discriminates.
* The union has user-provided ctor/dtor (required when its members
* are non-trivially-destructible) so the active member's lifetime
* is managed manually by the expected class via placement-new and
* explicit destructor calls. Same approach used by libc++ and Microsoft
* STL.
*
*   TRIVIALITY:
*   std::expected is conditionally trivially copyable / movable /
* destructible when T and E both are. re_std's back-port always
* provides user-defined special-member functions — correctness over
* triviality. This loses some optimisation (an empty expected<int, int>
* won't be trivially copyable in re_std; it will in std). Documented;
* may be addressed in a follow-up phase via conditional inheritance
* (the same trick libc++ uses).
*
*   MONADIC OPERATIONS:
*   and_then, or_else, transform, transform_error are implemented
* using direct call-syntax (static_cast<F&&>(f)(args)) rather than
* re_std::invoke (which is blocked on the <functional> phase). This
* supports function objects, lambdas, and free function pointers
* but NOT pointer-to-member-functions. PMF callers must wrap their
* callable; std::expected behaves the same way without invoke.
*
*   REFERENCE SPECIALISATION:
*   expected<T&, E> (added in C++23 via P2655R3) is NOT shipped in
* this initial phase. Its rebinding semantics interact with assignment
* in non-obvious ways and warrant their own dedicated header.
*
*   Uses:
*     unexpect.hpp                 - unexpect_t tag
*     bad_expected_access.hpp      - thrown by value()
*     unexpected.hpp               - the unexpected<E> wrapper
*     in_place.hpp                 - in_place_t tag
*     plus assorted type_traits granular headers (see includes)
*
*
* path:      /inc/re_std/expected/expected.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    COMPATIBILITY MACROS
      --------------------

I.    EXPECTED<T, E>  — primary template
      ----------------------------------

II.   EXPECTED<void, E>  — partial specialisation
      -------------------------------------------
*/

#ifndef RE_STD_EXPECTED_EXPECTED_HPP
#define RE_STD_EXPECTED_EXPECTED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// gate: C++11+ baseline. Variadic templates, rvalue refs,
// default-member-init, decltype, deleted/defaulted special members
// are all required.
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <initializer_list>
#include <new>              // placement new
#include <utility>          // std::declval (used in trailing return types)

#include "./unexpect.hpp"
#include "./bad_expected_access.hpp"
#include "./unexpected.hpp"

#include "../optional/in_place.hpp"

#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/is_void.hpp"
#include "../type_traits/is_constructible.hpp"
#include "../type_traits/is_convertible.hpp"
#include "../type_traits/is_nothrow_move_constructible.hpp"
#include "../type_traits/is_nothrow_copy_constructible.hpp"
#include "../type_traits/is_default_constructible.hpp"
#include "../type_traits/decay.hpp"
#include "../type_traits/remove_cv.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


///////////////////////////////////////////////////////////////////////////////
///                I.   EXPECTED<T, E>                                      ///
///////////////////////////////////////////////////////////////////////////////

template<typename T,
         typename E>
class expected
{
public:
    // =================================================================
    // MEMBER TYPES
    // =================================================================

    typedef T              value_type;
    typedef E              error_type;
    typedef unexpected<E>  unexpected_type;

    template<typename U>
    struct rebind
    {
        typedef expected<U, E> type;
    };

    // =================================================================
    // CTORS
    // =================================================================

    // (1) default ctor — value-initialises T.
    //   Requires T to be default-constructible.
    template<typename U = T,
             typename = typename re_std::enable_if<
                 re_std::is_default_constructible<U>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected()
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val)) T();
    }

    // (2) copy ctor
    RE_STD_CONSTEXPR_CPP20 expected(expected const& _other)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_val)) T(_other.m_storage.m_val);
        }
        else
        {
            new (static_cast<void*>(&m_storage.m_err)) E(_other.m_storage.m_err);
        }
    }

    // (3) move ctor
    RE_STD_CONSTEXPR_CPP20 expected(expected&& _other)
        RE_STD_NOEXCEPT_IF(
            re_std::is_nothrow_move_constructible<T>::value &&
            re_std::is_nothrow_move_constructible<E>::value)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_val))
                T(static_cast<T&&>(_other.m_storage.m_val));
        }
        else
        {
            new (static_cast<void*>(&m_storage.m_err))
                E(static_cast<E&&>(_other.m_storage.m_err));
        }
    }

    // (4) forwarding-from-U ctor
    //   Constructs the value from a forwarded U; gated to avoid
    // hijacking the copy/move ctors and the unexpected/in_place ctors.
    template<typename U = T,
             typename = typename re_std::enable_if<
                 !re_std::is_same<typename re_std::decay<U>::type, expected>::value &&
                 !re_std::is_same<typename re_std::decay<U>::type, in_place_t>::value &&
                 !re_std::is_same<typename re_std::decay<U>::type, unexpect_t>::value &&
                 re_std::is_constructible<T, U>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected(U&& _v)
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val))
            T(static_cast<U&&>(_v));
    }

    // (5) unexpected copy ctor — wrap an error.
    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G const&>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected(unexpected<G> const& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err)) E(_u.error());
    }

    // (6) unexpected move ctor
    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected(unexpected<G>&& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            E(static_cast<G&&>(_u.error()));
    }

    // (7) in_place ctor — emplaces value from forwarded args.
    template<typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<T, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit expected(in_place_t, Args&&... _args)
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val))
            T(static_cast<Args&&>(_args)...);
    }

    // (8) in_place + initializer_list ctor
    template<typename U,
             typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<T, std::initializer_list<U>&, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit expected(
        in_place_t,
        std::initializer_list<U> _il,
        Args&&... _args
    )
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val))
            T(_il, static_cast<Args&&>(_args)...);
    }

    // (9) unexpect ctor — emplaces error from forwarded args.
    template<typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit expected(unexpect_t, Args&&... _args)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            E(static_cast<Args&&>(_args)...);
    }

    // (10) unexpect + initializer_list ctor
    template<typename U,
             typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, std::initializer_list<U>&, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit expected(
        unexpect_t,
        std::initializer_list<U> _il,
        Args&&... _args
    )
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            E(_il, static_cast<Args&&>(_args)...);
    }

    // =================================================================
    // DESTRUCTOR
    // =================================================================

    // ~expected
    //   function: destroys whichever union member is active.
    RE_STD_CONSTEXPR_CPP20 ~expected()
    {
        _destroy();
    }

    // =================================================================
    // ASSIGNMENT
    // =================================================================

    // copy assignment
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        expected const& _other
    )
    {
        if (this != &_other)
        {
            _destroy();
            m_has_value = _other.m_has_value;
            if (_other.m_has_value)
            {
                new (static_cast<void*>(&m_storage.m_val))
                    T(_other.m_storage.m_val);
            }
            else
            {
                new (static_cast<void*>(&m_storage.m_err))
                    E(_other.m_storage.m_err);
            }
        }
        return *this;
    }

    // move assignment
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        expected&& _other
    )
    RE_STD_NOEXCEPT_IF(
        re_std::is_nothrow_move_constructible<T>::value &&
        re_std::is_nothrow_move_constructible<E>::value)
    {
        if (this != &_other)
        {
            _destroy();
            m_has_value = _other.m_has_value;
            if (_other.m_has_value)
            {
                new (static_cast<void*>(&m_storage.m_val))
                    T(static_cast<T&&>(_other.m_storage.m_val));
            }
            else
            {
                new (static_cast<void*>(&m_storage.m_err))
                    E(static_cast<E&&>(_other.m_storage.m_err));
            }
        }
        return *this;
    }

    // forwarding-from-U assignment
    template<typename U = T,
             typename = typename re_std::enable_if<
                 !re_std::is_same<typename re_std::decay<U>::type, expected>::value &&
                 re_std::is_constructible<T, U>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        U&& _v
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_val))
            T(static_cast<U&&>(_v));
        m_has_value = true;
        return *this;
    }

    // unexpected copy assignment
    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G const&>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<G> const& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err)) E(_u.error());
        m_has_value = false;
        return *this;
    }

    // unexpected move assignment
    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<G>&& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err))
            E(static_cast<G&&>(_u.error()));
        m_has_value = false;
        return *this;
    }

    // =================================================================
    // EMPLACE
    // =================================================================

    // emplace
    //   function: destroys the current state and constructs a new
    // value in place. Returns a reference to the constructed value.
    template<typename... Args>
    RE_STD_CONSTEXPR_CPP20 T&
    emplace(
        Args&&... _args
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_val))
            T(static_cast<Args&&>(_args)...);
        m_has_value = true;
        return m_storage.m_val;
    }

    // emplace (initializer_list)
    template<typename U,
             typename... Args>
    RE_STD_CONSTEXPR_CPP20 T&
    emplace(
        std::initializer_list<U> _il,
        Args&&... _args
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_val))
            T(_il, static_cast<Args&&>(_args)...);
        m_has_value = true;
        return m_storage.m_val;
    }

    // =================================================================
    // SWAP
    // =================================================================

    RE_STD_CONSTEXPR_CPP20 void
    swap(
        expected& _other
    )
    {
        if (m_has_value && _other.m_has_value)
        {
            using std::swap;
            swap(m_storage.m_val, _other.m_storage.m_val);
        }
        else if (!m_has_value && !_other.m_has_value)
        {
            using std::swap;
            swap(m_storage.m_err, _other.m_storage.m_err);
        }
        else if (m_has_value)  // && !_other.m_has_value
        {
            E tmp_err = static_cast<E&&>(_other.m_storage.m_err);
            _other.m_storage.m_err.~E();
            new (static_cast<void*>(&_other.m_storage.m_val))
                T(static_cast<T&&>(m_storage.m_val));
            m_storage.m_val.~T();
            new (static_cast<void*>(&m_storage.m_err))
                E(static_cast<E&&>(tmp_err));
            m_has_value = false;
            _other.m_has_value = true;
        }
        else  // !m_has_value && _other.m_has_value
        {
            _other.swap(*this);
        }

        return;
    }

    // =================================================================
    // OBSERVERS
    // =================================================================

    // operator-> (mutable)
    RE_STD_CONSTEXPR_CPP20 T* operator->() RE_STD_NOEXCEPT
    {
        return &m_storage.m_val;
    }

    // operator-> (const)
    RE_STD_CONSTEXPR T const* operator->() const RE_STD_NOEXCEPT
    {
        return &m_storage.m_val;
    }

    // operator* (lvalue mutable)
    RE_STD_CONSTEXPR_CPP20 T& operator*() & RE_STD_NOEXCEPT
    {
        return m_storage.m_val;
    }

    // operator* (lvalue const)
    RE_STD_CONSTEXPR T const& operator*() const & RE_STD_NOEXCEPT
    {
        return m_storage.m_val;
    }

    // operator* (rvalue mutable)
    RE_STD_CONSTEXPR_CPP20 T&& operator*() && RE_STD_NOEXCEPT
    {
        return static_cast<T&&>(m_storage.m_val);
    }

    // operator* (rvalue const)
    RE_STD_CONSTEXPR T const&& operator*() const && RE_STD_NOEXCEPT
    {
        return static_cast<T const&&>(m_storage.m_val);
    }

    // operator bool / has_value
    RE_STD_CONSTEXPR explicit operator bool() const RE_STD_NOEXCEPT
    {
        return m_has_value;
    }

    RE_STD_CONSTEXPR bool has_value() const RE_STD_NOEXCEPT
    {
        return m_has_value;
    }

    // value (lvalue mutable)
    //   throws: bad_expected_access<E> if !has_value().
    RE_STD_CONSTEXPR_CPP20 T& value() &
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return m_storage.m_val;
    }

    RE_STD_CONSTEXPR_CPP14 T const& value() const &
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return m_storage.m_val;
    }

    RE_STD_CONSTEXPR_CPP20 T&& value() &&
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return static_cast<T&&>(m_storage.m_val);
    }

    RE_STD_CONSTEXPR_CPP14 T const&& value() const &&
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return static_cast<T const&&>(m_storage.m_val);
    }

    // error
    RE_STD_CONSTEXPR_CPP20 E& error() & RE_STD_NOEXCEPT
    {
        return m_storage.m_err;
    }

    RE_STD_CONSTEXPR E const& error() const & RE_STD_NOEXCEPT
    {
        return m_storage.m_err;
    }

    RE_STD_CONSTEXPR_CPP20 E&& error() && RE_STD_NOEXCEPT
    {
        return static_cast<E&&>(m_storage.m_err);
    }

    RE_STD_CONSTEXPR E const&& error() const && RE_STD_NOEXCEPT
    {
        return static_cast<E const&&>(m_storage.m_err);
    }

    // value_or
    //   function: return the value if has_value(), otherwise return
    // a T constructed from the forwarded default.
    template<typename U>
    RE_STD_CONSTEXPR T
    value_or(
        U&& _default
    ) const &
    {
        return m_has_value
            ? m_storage.m_val
            : static_cast<T>(static_cast<U&&>(_default));
    }

    template<typename U>
    RE_STD_CONSTEXPR_CPP20 T
    value_or(
        U&& _default
    ) &&
    {
        return m_has_value
            ? static_cast<T&&>(m_storage.m_val)
            : static_cast<T>(static_cast<U&&>(_default));
    }

    // =================================================================
    // MONADIC OPERATIONS
    // =================================================================

    // and_then(F)
    //   If has_value: returns f(value) which must itself be an
    // expected with the same E. Otherwise: returns an expected of
    // f's return type carrying *this's error.
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    and_then(
        F&& _f
    ) & -> decltype(static_cast<F&&>(_f)(std::declval<T&>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<T&>())) _ret_t;
        return m_has_value
            ? static_cast<F&&>(_f)(m_storage.m_val)
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR auto
    and_then(
        F&& _f
    ) const & -> decltype(static_cast<F&&>(_f)(std::declval<T const&>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<T const&>())) _ret_t;
        return m_has_value
            ? static_cast<F&&>(_f)(m_storage.m_val)
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    and_then(
        F&& _f
    ) && -> decltype(static_cast<F&&>(_f)(std::declval<T>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<T>())) _ret_t;
        return m_has_value
            ? static_cast<F&&>(_f)(static_cast<T&&>(m_storage.m_val))
            : _ret_t(unexpect, static_cast<E&&>(m_storage.m_err));
    }

    // or_else(F)
    //   If has_value: returns *this packaged as the same expected
    // type as f's return. Otherwise: returns f(error).
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    or_else(
        F&& _f
    ) & -> decltype(static_cast<F&&>(_f)(std::declval<E&>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<E&>())) _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : static_cast<F&&>(_f)(m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR auto
    or_else(
        F&& _f
    ) const & -> decltype(static_cast<F&&>(_f)(std::declval<E const&>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<E const&>())) _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : static_cast<F&&>(_f)(m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    or_else(
        F&& _f
    ) && -> decltype(static_cast<F&&>(_f)(std::declval<E>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<E>())) _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<T&&>(m_storage.m_val))
            : static_cast<F&&>(_f)(static_cast<E&&>(m_storage.m_err));
    }

    // transform(F)
    //   If has_value: returns expected<U, E>(in_place, f(value))
    // where U = decay<decltype(f(value))>. Otherwise: returns
    // expected<U, E>(unexpect, error).
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    transform(
        F&& _f
    ) & -> expected<typename re_std::remove_cv<
                        decltype(static_cast<F&&>(_f)(std::declval<T&>()))
                    >::type, E>
    {
        typedef expected<typename re_std::remove_cv<
                            decltype(static_cast<F&&>(_f)(std::declval<T&>()))
                        >::type, E> _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<F&&>(_f)(m_storage.m_val))
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR auto
    transform(
        F&& _f
    ) const & -> expected<typename re_std::remove_cv<
                              decltype(static_cast<F&&>(_f)(std::declval<T const&>()))
                          >::type, E>
    {
        typedef expected<typename re_std::remove_cv<
                            decltype(static_cast<F&&>(_f)(std::declval<T const&>()))
                        >::type, E> _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<F&&>(_f)(m_storage.m_val))
            : _ret_t(unexpect, m_storage.m_err);
    }

    // transform_error(F)
    //   If has_value: returns expected<T, G>(in_place, value) where
    // G = decay<decltype(f(error))>. Otherwise: returns
    // expected<T, G>(unexpect, f(error)).
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    transform_error(
        F&& _f
    ) & -> expected<T, typename re_std::remove_cv<
                            decltype(static_cast<F&&>(_f)(std::declval<E&>()))
                        >::type>
    {
        typedef expected<T, typename re_std::remove_cv<
                            decltype(static_cast<F&&>(_f)(std::declval<E&>()))
                        >::type> _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : _ret_t(unexpect, static_cast<F&&>(_f)(m_storage.m_err));
    }

    template<typename F>
    RE_STD_CONSTEXPR auto
    transform_error(
        F&& _f
    ) const & -> expected<T, typename re_std::remove_cv<
                              decltype(static_cast<F&&>(_f)(std::declval<E const&>()))
                          >::type>
    {
        typedef expected<T, typename re_std::remove_cv<
                            decltype(static_cast<F&&>(_f)(std::declval<E const&>()))
                        >::type> _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : _ret_t(unexpect, static_cast<F&&>(_f)(m_storage.m_err));
    }

private:

    // =================================================================
    // STORAGE
    // =================================================================

    union _storage
    {
        T   m_val;
        E   m_err;

        // empty ctor leaves union with no active member; the
        // surrounding expected ctor immediately placement-news the
        // right one.
        _storage() {}

        // user-provided dtor — required when T or E is non-trivially
        // destructible. Body is empty; expected's dtor destroys the
        // active member manually before the union's dtor runs.
        ~_storage() {}
    };

    _storage    m_storage;
    bool        m_has_value;

    // =================================================================
    // INTERNAL HELPERS
    // =================================================================

    // _destroy
    //   function: destroys whichever union member is active.
    // Called by dtor, op=, and emplace.
    RE_STD_CONSTEXPR_CPP20 void _destroy() RE_STD_NOEXCEPT
    {
        if (m_has_value)
        {
            m_storage.m_val.~T();
        }
        else
        {
            m_storage.m_err.~E();
        }
    }

    // _throw_bad_access
    //   function: throws bad_expected_access<E> carrying a copy of
    // the error. Out-of-line to keep the constexpr value() bodies
    // happy on tiers where exception machinery isn't constexpr.
    void _throw_bad_access() const
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_expected_access<E>(m_storage.m_err);
#else
        // exceptions disabled: undefined behaviour on value() with
        // no value. Matches libstdc++ -fno-exceptions.
        // Touch the storage to silence unused warnings.
        (void)m_storage.m_err;
#endif
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  EXPECTED<void, E>                                   ///
///////////////////////////////////////////////////////////////////////////////
// Partial specialisation for the no-value-payload case. There is no
// m_val; the union becomes a single-member union of just E.

template<typename E>
class expected<void, E>
{
public:
    typedef void            value_type;
    typedef E              error_type;
    typedef unexpected<E>  unexpected_type;

    template<typename U>
    struct rebind
    {
        typedef expected<U, E> type;
    };

    // =================================================================
    // CTORS
    // =================================================================

    // (1) default ctor — no-value, in success state.
    RE_STD_CONSTEXPR expected()
        : m_storage(), m_has_value(true)
    {}

    // (2) copy ctor
    RE_STD_CONSTEXPR_CPP20 expected(expected const& _other)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (!_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_err)) E(_other.m_storage.m_err);
        }
    }

    // (3) move ctor
    RE_STD_CONSTEXPR_CPP20 expected(expected&& _other)
        RE_STD_NOEXCEPT_IF(re_std::is_nothrow_move_constructible<E>::value)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (!_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_err))
                E(static_cast<E&&>(_other.m_storage.m_err));
        }
    }

    // (5) unexpected copy ctor
    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G const&>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected(unexpected<G> const& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err)) E(_u.error());
    }

    // (6) unexpected move ctor
    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected(unexpected<G>&& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            E(static_cast<G&&>(_u.error()));
    }

    // (7) in_place ctor — for void value type, takes no args.
    RE_STD_CONSTEXPR explicit expected(in_place_t)
        : m_storage(), m_has_value(true)
    {}

    // (9) unexpect ctor
    template<typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit expected(unexpect_t, Args&&... _args)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            E(static_cast<Args&&>(_args)...);
    }

    // (10) unexpect + initializer_list ctor
    template<typename U,
             typename... Args,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, std::initializer_list<U>&, Args...>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 explicit expected(
        unexpect_t,
        std::initializer_list<U> _il,
        Args&&... _args
    )
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            E(_il, static_cast<Args&&>(_args)...);
    }

    // =================================================================
    // DESTRUCTOR
    // =================================================================

    RE_STD_CONSTEXPR_CPP20 ~expected()
    {
        _destroy();
    }

    // =================================================================
    // ASSIGNMENT
    // =================================================================

    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        expected const& _other
    )
    {
        if (this != &_other)
        {
            _destroy();
            m_has_value = _other.m_has_value;
            if (!_other.m_has_value)
            {
                new (static_cast<void*>(&m_storage.m_err))
                    E(_other.m_storage.m_err);
            }
        }
        return *this;
    }

    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        expected&& _other
    )
    RE_STD_NOEXCEPT_IF(re_std::is_nothrow_move_constructible<E>::value)
    {
        if (this != &_other)
        {
            _destroy();
            m_has_value = _other.m_has_value;
            if (!_other.m_has_value)
            {
                new (static_cast<void*>(&m_storage.m_err))
                    E(static_cast<E&&>(_other.m_storage.m_err));
            }
        }
        return *this;
    }

    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G const&>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<G> const& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err)) E(_u.error());
        m_has_value = false;
        return *this;
    }

    template<typename G,
             typename = typename re_std::enable_if<
                 re_std::is_constructible<E, G>::value
             >::type>
    RE_STD_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<G>&& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err))
            E(static_cast<G&&>(_u.error()));
        m_has_value = false;
        return *this;
    }

    // =================================================================
    // EMPLACE
    // =================================================================

    // emplace — for void, just resets to success state.
    RE_STD_CONSTEXPR_CPP20 void emplace() RE_STD_NOEXCEPT
    {
        _destroy();
        m_has_value = true;
    }

    // =================================================================
    // SWAP
    // =================================================================

    RE_STD_CONSTEXPR_CPP20 void
    swap(
        expected& _other
    )
    {
        if (m_has_value && _other.m_has_value)
        {
            // both success — nothing to swap.
        }
        else if (!m_has_value && !_other.m_has_value)
        {
            using std::swap;
            swap(m_storage.m_err, _other.m_storage.m_err);
        }
        else if (m_has_value)  // && !_other.m_has_value
        {
            new (static_cast<void*>(&m_storage.m_err))
                E(static_cast<E&&>(_other.m_storage.m_err));
            _other.m_storage.m_err.~E();
            m_has_value = false;
            _other.m_has_value = true;
        }
        else
        {
            _other.swap(*this);
        }

        return;
    }

    // =================================================================
    // OBSERVERS
    // =================================================================

    RE_STD_CONSTEXPR explicit operator bool() const RE_STD_NOEXCEPT
    {
        return m_has_value;
    }

    RE_STD_CONSTEXPR bool has_value() const RE_STD_NOEXCEPT
    {
        return m_has_value;
    }

    // operator* — for void, no value to return; provided for
    // generic-code symmetry, body asserts has_value (UB otherwise).
    RE_STD_CONSTEXPR_CPP14 void operator*() const RE_STD_NOEXCEPT
    {
        // no-op for void; the assert that "we have a value" is
        // user-responsibility (matches std::expected<void, E>).
    }

    // value — for void; throws if !has_value().
    RE_STD_CONSTEXPR_CPP20 void value() const &
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
    }

    RE_STD_CONSTEXPR_CPP20 void value() &&
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
    }

    RE_STD_CONSTEXPR_CPP20 E& error() & RE_STD_NOEXCEPT
    {
        return m_storage.m_err;
    }

    RE_STD_CONSTEXPR E const& error() const & RE_STD_NOEXCEPT
    {
        return m_storage.m_err;
    }

    RE_STD_CONSTEXPR_CPP20 E&& error() && RE_STD_NOEXCEPT
    {
        return static_cast<E&&>(m_storage.m_err);
    }

    RE_STD_CONSTEXPR E const&& error() const && RE_STD_NOEXCEPT
    {
        return static_cast<E const&&>(m_storage.m_err);
    }

    // =================================================================
    // MONADIC OPERATIONS  (void specialisation)
    // =================================================================

    // and_then(F) — callable takes no args.
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    and_then(
        F&& _f
    ) & -> decltype(static_cast<F&&>(_f)())
    {
        typedef decltype(static_cast<F&&>(_f)()) _ret_t;
        return m_has_value
            ? static_cast<F&&>(_f)()
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR auto
    and_then(
        F&& _f
    ) const & -> decltype(static_cast<F&&>(_f)())
    {
        typedef decltype(static_cast<F&&>(_f)()) _ret_t;
        return m_has_value
            ? static_cast<F&&>(_f)()
            : _ret_t(unexpect, m_storage.m_err);
    }

    // or_else(F)
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    or_else(
        F&& _f
    ) & -> decltype(static_cast<F&&>(_f)(std::declval<E&>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<E&>())) _ret_t;
        return m_has_value
            ? _ret_t()
            : static_cast<F&&>(_f)(m_storage.m_err);
    }

    template<typename F>
    RE_STD_CONSTEXPR auto
    or_else(
        F&& _f
    ) const & -> decltype(static_cast<F&&>(_f)(std::declval<E const&>()))
    {
        typedef decltype(static_cast<F&&>(_f)(std::declval<E const&>())) _ret_t;
        return m_has_value
            ? _ret_t()
            : static_cast<F&&>(_f)(m_storage.m_err);
    }

    // transform(F) — callable takes no args.
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    transform(
        F&& _f
    ) & -> expected<typename re_std::remove_cv<
                        decltype(static_cast<F&&>(_f)())
                    >::type, E>
    {
        typedef expected<typename re_std::remove_cv<
                            decltype(static_cast<F&&>(_f)())
                        >::type, E> _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<F&&>(_f)())
            : _ret_t(unexpect, m_storage.m_err);
    }

    // transform_error(F)
    template<typename F>
    RE_STD_CONSTEXPR_CPP20 auto
    transform_error(
        F&& _f
    ) & -> expected<void, typename re_std::remove_cv<
                              decltype(static_cast<F&&>(_f)(std::declval<E&>()))
                          >::type>
    {
        typedef expected<void, typename re_std::remove_cv<
                                  decltype(static_cast<F&&>(_f)(std::declval<E&>()))
                              >::type> _ret_t;
        return m_has_value
            ? _ret_t()
            : _ret_t(unexpect, static_cast<F&&>(_f)(m_storage.m_err));
    }

private:

    union _storage
    {
        char m_dummy;
        E   m_err;

        _storage() : m_dummy() {}
        ~_storage() {}
    };

    _storage    m_storage;
    bool        m_has_value;

    RE_STD_CONSTEXPR_CPP20 void _destroy() RE_STD_NOEXCEPT
    {
        if (!m_has_value)
        {
            m_storage.m_err.~E();
        }
    }

    void _throw_bad_access() const
    {
#if RE_STD_HAS_EXCEPTIONS
        throw bad_expected_access<E>(m_storage.m_err);
#else
        (void)m_storage.m_err;
#endif
    }
};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_EXPECTED_EXPECTED_HPP
