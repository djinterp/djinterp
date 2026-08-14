/******************************************************************************
* djinterp [restd]                                                   expected.hpp
*
*   restd's back-port of std::expected<T, E> — a discriminated union
* holding either a value (type _T) or an unexpected error (type _E).
* C++23 in std; restd targets C++11+ for runtime correctness, with
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
* destructible when _T and _E both are. restd's back-port always
* provides user-defined special-member functions — correctness over
* triviality. This loses some optimisation (an empty expected<int, int>
* won't be trivially copyable in restd; it will in std). Documented;
* may be addressed in a follow-up phase via conditional inheritance
* (the same trick libc++ uses).
*
*   MONADIC OPERATIONS:
*   and_then, or_else, transform, transform_error are implemented
* using direct call-syntax (static_cast<F&&>(f)(args)) rather than
* restd::invoke (which is blocked on the <functional> phase). This
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
* TABLE OF CONTENTS
* =================
* 0.    COMPATIBILITY MACROS
* I.    EXPECTED<T, E>  — primary template
* II.   EXPECTED<void, E>  — partial specialisation
*
*
* path:      /inc/djinterp/re_std/expected/expected.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_EXPECTED_
#define DJINTERP_RESTD_EXPECTED_ 1

#include "../../core/djinterp.hpp"

// gate: C++11+ baseline. Variadic templates, rvalue refs,
// default-member-init, decltype, deleted/defaulted special members
// are all required.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <new>              // placement new
#include <utility>          // std::declval (used in trailing return types)
#include <initializer_list>

#include "./unexpect.hpp"
#include "./bad_expected_access.hpp"
#include "./unexpected.hpp"

#include "../utility/in_place.hpp"

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

#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


///////////////////////////////////////////////////////////////////////////////
///                I.   EXPECTED<T, E>                                      ///
///////////////////////////////////////////////////////////////////////////////

template<typename _T,
         typename _E>
class expected
{
public:
    // =================================================================
    // MEMBER TYPES
    // =================================================================

    typedef _T              value_type;
    typedef _E              error_type;
    typedef unexpected<_E>  unexpected_type;

    template<typename _U>
    struct rebind
    {
        typedef expected<_U, _E> type;
    };

    // =================================================================
    // CTORS
    // =================================================================

    // (1) default ctor — value-initialises _T.
    //   Requires _T to be default-constructible.
    template<typename _U = _T,
             typename = typename restd::enable_if<
                 restd::is_default_constructible<_U>::value
             >::type>
    D_CONSTEXPR_CPP20 expected()
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val)) _T();
    }

    // (2) copy ctor
    D_CONSTEXPR_CPP20 expected(expected const& _other)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_val)) _T(_other.m_storage.m_val);
        }
        else
        {
            new (static_cast<void*>(&m_storage.m_err)) _E(_other.m_storage.m_err);
        }
    }

    // (3) move ctor
    D_CONSTEXPR_CPP20 expected(expected&& _other)
        D_NOEXCEPT_IF(
            restd::is_nothrow_move_constructible<_T>::value &&
            restd::is_nothrow_move_constructible<_E>::value)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_val))
                _T(static_cast<_T&&>(_other.m_storage.m_val));
        }
        else
        {
            new (static_cast<void*>(&m_storage.m_err))
                _E(static_cast<_E&&>(_other.m_storage.m_err));
        }
    }

    // (4) forwarding-from-U ctor
    //   Constructs the value from a forwarded _U; gated to avoid
    // hijacking the copy/move ctors and the unexpected/in_place ctors.
    template<typename _U = _T,
             typename = typename restd::enable_if<
                 !restd::is_same<typename restd::decay<_U>::type, expected>::value &&
                 !restd::is_same<typename restd::decay<_U>::type, in_place_t>::value &&
                 !restd::is_same<typename restd::decay<_U>::type, unexpect_t>::value &&
                 restd::is_constructible<_T, _U>::value
             >::type>
    D_CONSTEXPR_CPP20 expected(_U&& _v)
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val))
            _T(static_cast<_U&&>(_v));
    }

    // (5) unexpected copy ctor — wrap an error.
    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G const&>::value
             >::type>
    D_CONSTEXPR_CPP20 expected(unexpected<_G> const& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err)) _E(_u.error());
    }

    // (6) unexpected move ctor
    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G>::value
             >::type>
    D_CONSTEXPR_CPP20 expected(unexpected<_G>&& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            _E(static_cast<_G&&>(_u.error()));
    }

    // (7) in_place ctor — emplaces value from forwarded args.
    template<typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_T, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit expected(in_place_t, _Args&&... _args)
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val))
            _T(static_cast<_Args&&>(_args)...);
    }

    // (8) in_place + initializer_list ctor
    template<typename _U,
             typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_T, std::initializer_list<_U>&, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit expected(
        in_place_t,
        std::initializer_list<_U> _il,
        _Args&&... _args
    )
        : m_storage(), m_has_value(true)
    {
        new (static_cast<void*>(&m_storage.m_val))
            _T(_il, static_cast<_Args&&>(_args)...);
    }

    // (9) unexpect ctor — emplaces error from forwarded args.
    template<typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit expected(unexpect_t, _Args&&... _args)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            _E(static_cast<_Args&&>(_args)...);
    }

    // (10) unexpect + initializer_list ctor
    template<typename _U,
             typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, std::initializer_list<_U>&, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit expected(
        unexpect_t,
        std::initializer_list<_U> _il,
        _Args&&... _args
    )
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            _E(_il, static_cast<_Args&&>(_args)...);
    }

    // =================================================================
    // DESTRUCTOR
    // =================================================================

    // ~expected
    //   function: destroys whichever union member is active.
    D_CONSTEXPR_CPP20 ~expected()
    {
        _destroy();
    }

    // =================================================================
    // ASSIGNMENT
    // =================================================================

    // copy assignment
    D_CONSTEXPR_CPP20 expected&
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
                    _T(_other.m_storage.m_val);
            }
            else
            {
                new (static_cast<void*>(&m_storage.m_err))
                    _E(_other.m_storage.m_err);
            }
        }
        return *this;
    }

    // move assignment
    D_CONSTEXPR_CPP20 expected&
    operator=(
        expected&& _other
    )
    D_NOEXCEPT_IF(
        restd::is_nothrow_move_constructible<_T>::value &&
        restd::is_nothrow_move_constructible<_E>::value)
    {
        if (this != &_other)
        {
            _destroy();
            m_has_value = _other.m_has_value;
            if (_other.m_has_value)
            {
                new (static_cast<void*>(&m_storage.m_val))
                    _T(static_cast<_T&&>(_other.m_storage.m_val));
            }
            else
            {
                new (static_cast<void*>(&m_storage.m_err))
                    _E(static_cast<_E&&>(_other.m_storage.m_err));
            }
        }
        return *this;
    }

    // forwarding-from-U assignment
    template<typename _U = _T,
             typename = typename restd::enable_if<
                 !restd::is_same<typename restd::decay<_U>::type, expected>::value &&
                 restd::is_constructible<_T, _U>::value
             >::type>
    D_CONSTEXPR_CPP20 expected&
    operator=(
        _U&& _v
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_val))
            _T(static_cast<_U&&>(_v));
        m_has_value = true;
        return *this;
    }

    // unexpected copy assignment
    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G const&>::value
             >::type>
    D_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<_G> const& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err)) _E(_u.error());
        m_has_value = false;
        return *this;
    }

    // unexpected move assignment
    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G>::value
             >::type>
    D_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<_G>&& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err))
            _E(static_cast<_G&&>(_u.error()));
        m_has_value = false;
        return *this;
    }

    // =================================================================
    // EMPLACE
    // =================================================================

    // emplace
    //   function: destroys the current state and constructs a new
    // value in place. Returns a reference to the constructed value.
    template<typename... _Args>
    D_CONSTEXPR_CPP20 _T&
    emplace(
        _Args&&... _args
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_val))
            _T(static_cast<_Args&&>(_args)...);
        m_has_value = true;
        return m_storage.m_val;
    }

    // emplace (initializer_list)
    template<typename _U,
             typename... _Args>
    D_CONSTEXPR_CPP20 _T&
    emplace(
        std::initializer_list<_U> _il,
        _Args&&... _args
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_val))
            _T(_il, static_cast<_Args&&>(_args)...);
        m_has_value = true;
        return m_storage.m_val;
    }

    // =================================================================
    // SWAP
    // =================================================================

    D_CONSTEXPR_CPP20 void
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
            _E tmp_err = static_cast<_E&&>(_other.m_storage.m_err);
            _other.m_storage.m_err.~_E();
            new (static_cast<void*>(&_other.m_storage.m_val))
                _T(static_cast<_T&&>(m_storage.m_val));
            m_storage.m_val.~_T();
            new (static_cast<void*>(&m_storage.m_err))
                _E(static_cast<_E&&>(tmp_err));
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
    D_CONSTEXPR_CPP20 _T* operator->() D_NOEXCEPT
    {
        return &m_storage.m_val;
    }

    // operator-> (const)
    D_CONSTEXPR _T const* operator->() const D_NOEXCEPT
    {
        return &m_storage.m_val;
    }

    // operator* (lvalue mutable)
    D_CONSTEXPR_CPP20 _T& operator*() & D_NOEXCEPT
    {
        return m_storage.m_val;
    }

    // operator* (lvalue const)
    D_CONSTEXPR _T const& operator*() const & D_NOEXCEPT
    {
        return m_storage.m_val;
    }

    // operator* (rvalue mutable)
    D_CONSTEXPR_CPP20 _T&& operator*() && D_NOEXCEPT
    {
        return static_cast<_T&&>(m_storage.m_val);
    }

    // operator* (rvalue const)
    D_CONSTEXPR _T const&& operator*() const && D_NOEXCEPT
    {
        return static_cast<_T const&&>(m_storage.m_val);
    }

    // operator bool / has_value
    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
    {
        return m_has_value;
    }

    D_CONSTEXPR bool has_value() const D_NOEXCEPT
    {
        return m_has_value;
    }

    // value (lvalue mutable)
    //   throws: bad_expected_access<_E> if !has_value().
    D_CONSTEXPR_CPP20 _T& value() &
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return m_storage.m_val;
    }

    D_CONSTEXPR _T const& value() const &
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return m_storage.m_val;
    }

    D_CONSTEXPR_CPP20 _T&& value() &&
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return static_cast<_T&&>(m_storage.m_val);
    }

    D_CONSTEXPR _T const&& value() const &&
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
        return static_cast<_T const&&>(m_storage.m_val);
    }

    // error
    D_CONSTEXPR_CPP20 _E& error() & D_NOEXCEPT
    {
        return m_storage.m_err;
    }

    D_CONSTEXPR _E const& error() const & D_NOEXCEPT
    {
        return m_storage.m_err;
    }

    D_CONSTEXPR_CPP20 _E&& error() && D_NOEXCEPT
    {
        return static_cast<_E&&>(m_storage.m_err);
    }

    D_CONSTEXPR _E const&& error() const && D_NOEXCEPT
    {
        return static_cast<_E const&&>(m_storage.m_err);
    }

    // value_or
    //   function: return the value if has_value(), otherwise return
    // a _T constructed from the forwarded default.
    template<typename _U>
    D_CONSTEXPR _T
    value_or(
        _U&& _default
    ) const &
    {
        return m_has_value
            ? m_storage.m_val
            : static_cast<_T>(static_cast<_U&&>(_default));
    }

    template<typename _U>
    D_CONSTEXPR_CPP20 _T
    value_or(
        _U&& _default
    ) &&
    {
        return m_has_value
            ? static_cast<_T&&>(m_storage.m_val)
            : static_cast<_T>(static_cast<_U&&>(_default));
    }

    // =================================================================
    // MONADIC OPERATIONS
    // =================================================================

    // and_then(F)
    //   If has_value: returns f(value) which must itself be an
    // expected with the same _E. Otherwise: returns an expected of
    // f's return type carrying *this's error.
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    and_then(
        _F&& _f
    ) & -> decltype(static_cast<_F&&>(_f)(std::declval<_T&>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_T&>())) _ret_t;
        return m_has_value
            ? static_cast<_F&&>(_f)(m_storage.m_val)
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR auto
    and_then(
        _F&& _f
    ) const & -> decltype(static_cast<_F&&>(_f)(std::declval<_T const&>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_T const&>())) _ret_t;
        return m_has_value
            ? static_cast<_F&&>(_f)(m_storage.m_val)
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    and_then(
        _F&& _f
    ) && -> decltype(static_cast<_F&&>(_f)(std::declval<_T>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_T>())) _ret_t;
        return m_has_value
            ? static_cast<_F&&>(_f)(static_cast<_T&&>(m_storage.m_val))
            : _ret_t(unexpect, static_cast<_E&&>(m_storage.m_err));
    }

    // or_else(F)
    //   If has_value: returns *this packaged as the same expected
    // type as f's return. Otherwise: returns f(error).
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    or_else(
        _F&& _f
    ) & -> decltype(static_cast<_F&&>(_f)(std::declval<_E&>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_E&>())) _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : static_cast<_F&&>(_f)(m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR auto
    or_else(
        _F&& _f
    ) const & -> decltype(static_cast<_F&&>(_f)(std::declval<_E const&>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_E const&>())) _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : static_cast<_F&&>(_f)(m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    or_else(
        _F&& _f
    ) && -> decltype(static_cast<_F&&>(_f)(std::declval<_E>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_E>())) _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<_T&&>(m_storage.m_val))
            : static_cast<_F&&>(_f)(static_cast<_E&&>(m_storage.m_err));
    }

    // transform(F)
    //   If has_value: returns expected<U, E>(in_place, f(value))
    // where U = decay<decltype(f(value))>. Otherwise: returns
    // expected<U, E>(unexpect, error).
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    transform(
        _F&& _f
    ) & -> expected<typename restd::remove_cv<
                        decltype(static_cast<_F&&>(_f)(std::declval<_T&>()))
                    >::type, _E>
    {
        typedef expected<typename restd::remove_cv<
                            decltype(static_cast<_F&&>(_f)(std::declval<_T&>()))
                        >::type, _E> _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<_F&&>(_f)(m_storage.m_val))
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR auto
    transform(
        _F&& _f
    ) const & -> expected<typename restd::remove_cv<
                              decltype(static_cast<_F&&>(_f)(std::declval<_T const&>()))
                          >::type, _E>
    {
        typedef expected<typename restd::remove_cv<
                            decltype(static_cast<_F&&>(_f)(std::declval<_T const&>()))
                        >::type, _E> _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<_F&&>(_f)(m_storage.m_val))
            : _ret_t(unexpect, m_storage.m_err);
    }

    // transform_error(F)
    //   If has_value: returns expected<T, G>(in_place, value) where
    // G = decay<decltype(f(error))>. Otherwise: returns
    // expected<T, G>(unexpect, f(error)).
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    transform_error(
        _F&& _f
    ) & -> expected<_T, typename restd::remove_cv<
                            decltype(static_cast<_F&&>(_f)(std::declval<_E&>()))
                        >::type>
    {
        typedef expected<_T, typename restd::remove_cv<
                            decltype(static_cast<_F&&>(_f)(std::declval<_E&>()))
                        >::type> _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : _ret_t(unexpect, static_cast<_F&&>(_f)(m_storage.m_err));
    }

    template<typename _F>
    D_CONSTEXPR auto
    transform_error(
        _F&& _f
    ) const & -> expected<_T, typename restd::remove_cv<
                              decltype(static_cast<_F&&>(_f)(std::declval<_E const&>()))
                          >::type>
    {
        typedef expected<_T, typename restd::remove_cv<
                            decltype(static_cast<_F&&>(_f)(std::declval<_E const&>()))
                        >::type> _ret_t;
        return m_has_value
            ? _ret_t(in_place, m_storage.m_val)
            : _ret_t(unexpect, static_cast<_F&&>(_f)(m_storage.m_err));
    }

private:

    // =================================================================
    // STORAGE
    // =================================================================

    union _storage
    {
        _T   m_val;
        _E   m_err;

        // empty ctor leaves union with no active member; the
        // surrounding expected ctor immediately placement-news the
        // right one.
        _storage() {}

        // user-provided dtor — required when _T or _E is non-trivially
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
    D_CONSTEXPR_CPP20 void _destroy() D_NOEXCEPT
    {
        if (m_has_value)
        {
            m_storage.m_val.~_T();
        }
        else
        {
            m_storage.m_err.~_E();
        }
    }

    // _throw_bad_access
    //   function: throws bad_expected_access<_E> carrying a copy of
    // the error. Out-of-line to keep the constexpr value() bodies
    // happy on tiers where exception machinery isn't constexpr.
    void _throw_bad_access() const
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_expected_access<_E>(m_storage.m_err);
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
// m_val; the union becomes a single-member union of just _E.

template<typename _E>
class expected<void, _E>
{
public:
    typedef void            value_type;
    typedef _E              error_type;
    typedef unexpected<_E>  unexpected_type;

    template<typename _U>
    struct rebind
    {
        typedef expected<_U, _E> type;
    };

    // =================================================================
    // CTORS
    // =================================================================

    // (1) default ctor — no-value, in success state.
    D_CONSTEXPR expected()
        : m_storage(), m_has_value(true)
    {}

    // (2) copy ctor
    D_CONSTEXPR_CPP20 expected(expected const& _other)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (!_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_err)) _E(_other.m_storage.m_err);
        }
    }

    // (3) move ctor
    D_CONSTEXPR_CPP20 expected(expected&& _other)
        D_NOEXCEPT_IF(restd::is_nothrow_move_constructible<_E>::value)
        : m_storage(), m_has_value(_other.m_has_value)
    {
        if (!_other.m_has_value)
        {
            new (static_cast<void*>(&m_storage.m_err))
                _E(static_cast<_E&&>(_other.m_storage.m_err));
        }
    }

    // (5) unexpected copy ctor
    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G const&>::value
             >::type>
    D_CONSTEXPR_CPP20 expected(unexpected<_G> const& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err)) _E(_u.error());
    }

    // (6) unexpected move ctor
    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G>::value
             >::type>
    D_CONSTEXPR_CPP20 expected(unexpected<_G>&& _u)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            _E(static_cast<_G&&>(_u.error()));
    }

    // (7) in_place ctor — for void value type, takes no args.
    D_CONSTEXPR explicit expected(in_place_t)
        : m_storage(), m_has_value(true)
    {}

    // (9) unexpect ctor
    template<typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit expected(unexpect_t, _Args&&... _args)
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            _E(static_cast<_Args&&>(_args)...);
    }

    // (10) unexpect + initializer_list ctor
    template<typename _U,
             typename... _Args,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, std::initializer_list<_U>&, _Args...>::value
             >::type>
    D_CONSTEXPR_CPP20 explicit expected(
        unexpect_t,
        std::initializer_list<_U> _il,
        _Args&&... _args
    )
        : m_storage(), m_has_value(false)
    {
        new (static_cast<void*>(&m_storage.m_err))
            _E(_il, static_cast<_Args&&>(_args)...);
    }

    // =================================================================
    // DESTRUCTOR
    // =================================================================

    D_CONSTEXPR_CPP20 ~expected()
    {
        _destroy();
    }

    // =================================================================
    // ASSIGNMENT
    // =================================================================

    D_CONSTEXPR_CPP20 expected&
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
                    _E(_other.m_storage.m_err);
            }
        }
        return *this;
    }

    D_CONSTEXPR_CPP20 expected&
    operator=(
        expected&& _other
    )
    D_NOEXCEPT_IF(restd::is_nothrow_move_constructible<_E>::value)
    {
        if (this != &_other)
        {
            _destroy();
            m_has_value = _other.m_has_value;
            if (!_other.m_has_value)
            {
                new (static_cast<void*>(&m_storage.m_err))
                    _E(static_cast<_E&&>(_other.m_storage.m_err));
            }
        }
        return *this;
    }

    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G const&>::value
             >::type>
    D_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<_G> const& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err)) _E(_u.error());
        m_has_value = false;
        return *this;
    }

    template<typename _G,
             typename = typename restd::enable_if<
                 restd::is_constructible<_E, _G>::value
             >::type>
    D_CONSTEXPR_CPP20 expected&
    operator=(
        unexpected<_G>&& _u
    )
    {
        _destroy();
        new (static_cast<void*>(&m_storage.m_err))
            _E(static_cast<_G&&>(_u.error()));
        m_has_value = false;
        return *this;
    }

    // =================================================================
    // EMPLACE
    // =================================================================

    // emplace — for void, just resets to success state.
    D_CONSTEXPR_CPP20 void emplace() D_NOEXCEPT
    {
        _destroy();
        m_has_value = true;
    }

    // =================================================================
    // SWAP
    // =================================================================

    D_CONSTEXPR_CPP20 void
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
                _E(static_cast<_E&&>(_other.m_storage.m_err));
            _other.m_storage.m_err.~_E();
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

    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
    {
        return m_has_value;
    }

    D_CONSTEXPR bool has_value() const D_NOEXCEPT
    {
        return m_has_value;
    }

    // operator* — for void, no value to return; provided for
    // generic-code symmetry, body asserts has_value (UB otherwise).
    D_CONSTEXPR void operator*() const D_NOEXCEPT
    {
        // no-op for void; the assert that "we have a value" is
        // user-responsibility (matches std::expected<void, E>).
    }

    // value — for void; throws if !has_value().
    D_CONSTEXPR_CPP20 void value() const &
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
    }

    D_CONSTEXPR_CPP20 void value() &&
    {
        if (!m_has_value)
        {
            _throw_bad_access();
        }
    }

    D_CONSTEXPR_CPP20 _E& error() & D_NOEXCEPT
    {
        return m_storage.m_err;
    }

    D_CONSTEXPR _E const& error() const & D_NOEXCEPT
    {
        return m_storage.m_err;
    }

    D_CONSTEXPR_CPP20 _E&& error() && D_NOEXCEPT
    {
        return static_cast<_E&&>(m_storage.m_err);
    }

    D_CONSTEXPR _E const&& error() const && D_NOEXCEPT
    {
        return static_cast<_E const&&>(m_storage.m_err);
    }

    // =================================================================
    // MONADIC OPERATIONS  (void specialisation)
    // =================================================================

    // and_then(F) — callable takes no args.
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    and_then(
        _F&& _f
    ) & -> decltype(static_cast<_F&&>(_f)())
    {
        typedef decltype(static_cast<_F&&>(_f)()) _ret_t;
        return m_has_value
            ? static_cast<_F&&>(_f)()
            : _ret_t(unexpect, m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR auto
    and_then(
        _F&& _f
    ) const & -> decltype(static_cast<_F&&>(_f)())
    {
        typedef decltype(static_cast<_F&&>(_f)()) _ret_t;
        return m_has_value
            ? static_cast<_F&&>(_f)()
            : _ret_t(unexpect, m_storage.m_err);
    }

    // or_else(F)
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    or_else(
        _F&& _f
    ) & -> decltype(static_cast<_F&&>(_f)(std::declval<_E&>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_E&>())) _ret_t;
        return m_has_value
            ? _ret_t()
            : static_cast<_F&&>(_f)(m_storage.m_err);
    }

    template<typename _F>
    D_CONSTEXPR auto
    or_else(
        _F&& _f
    ) const & -> decltype(static_cast<_F&&>(_f)(std::declval<_E const&>()))
    {
        typedef decltype(static_cast<_F&&>(_f)(std::declval<_E const&>())) _ret_t;
        return m_has_value
            ? _ret_t()
            : static_cast<_F&&>(_f)(m_storage.m_err);
    }

    // transform(F) — callable takes no args.
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    transform(
        _F&& _f
    ) & -> expected<typename restd::remove_cv<
                        decltype(static_cast<_F&&>(_f)())
                    >::type, _E>
    {
        typedef expected<typename restd::remove_cv<
                            decltype(static_cast<_F&&>(_f)())
                        >::type, _E> _ret_t;
        return m_has_value
            ? _ret_t(in_place, static_cast<_F&&>(_f)())
            : _ret_t(unexpect, m_storage.m_err);
    }

    // transform_error(F)
    template<typename _F>
    D_CONSTEXPR_CPP20 auto
    transform_error(
        _F&& _f
    ) & -> expected<void, typename restd::remove_cv<
                              decltype(static_cast<_F&&>(_f)(std::declval<_E&>()))
                          >::type>
    {
        typedef expected<void, typename restd::remove_cv<
                                  decltype(static_cast<_F&&>(_f)(std::declval<_E&>()))
                              >::type> _ret_t;
        return m_has_value
            ? _ret_t()
            : _ret_t(unexpect, static_cast<_F&&>(_f)(m_storage.m_err));
    }

private:

    union _storage
    {
        char m_dummy;
        _E   m_err;

        _storage() : m_dummy() {}
        ~_storage() {}
    };

    _storage    m_storage;
    bool        m_has_value;

    D_CONSTEXPR_CPP20 void _destroy() D_NOEXCEPT
    {
        if (!m_has_value)
        {
            m_storage.m_err.~_E();
        }
    }

    void _throw_bad_access() const
    {
#if D_ENV_CPP98_HAS_EXCEPTION
        throw bad_expected_access<_E>(m_storage.m_err);
#else
        (void)m_storage.m_err;
#endif
    }
};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_EXPECTED_
