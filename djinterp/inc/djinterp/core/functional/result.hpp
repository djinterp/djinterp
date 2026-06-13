/******************************************************************************
* djinterp [functional]                                             result.hpp
*
* Result<T, E> -- a monadic success-or-error type (C++).
*   Represents the outcome of a fallible computation: either a value of
* type _Type (success / ok) or an error of type _Error (err). Equivalent in
* purpose to Rust's Result, Haskell's Either, std::expected (C++23). The
* error type is explicit, unlike maybe<T>, so the caller knows what kind
* of failure to handle.
*   result<T, E> participates in the djinterp monad protocol on its
* success type _Type: bind, map, and the operator| combinators all propagate
* the err case unchanged. To transform or inspect the error side, use
* map_err, or_else, or pattern-match via match().
*
*   Storage uses a union (via aligned_storage) with a discriminator. _Type
* and _Error need not be related; either may be void-like (use a unit type
* such as struct{}). Both must be at least move-constructible.
*
* USAGE:
*   result<int, std::string> parse(std::string s)
*   {
*       try { return ok<int, std::string>(std::stoi(s)); }
*       catch (...) { return err<int, std::string>("not a number"); }
*   }
*
*   auto r = parse("42")
*          | bind_with([](int n) { return safe_div(100, n); })
*          | map_with([](int n) { return n + 1; });
*
*   if (r.is_ok())  { use(r.value()); }
*   else            { log(r.error()); }
*
*   int v = parse("oops").value_or(0);                 // 0
*
*   auto out = r.match(
*       [](int v)              { return std::to_string(v); },
*       [](const std::string& e) { return std::string("ERR: ") + e; });
*
* 
* path:      /inc/djinterp/core/functional/result.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RESULT PRIMITIVE
      1.  result<T, E>                           (tagged union + interface)
II.   RESULT SFINAE STRUCTURAL TRAITS & CONCEPTS       
      1.  is_result<T>                           (detects result<U, F>)
      2.  result_value_type<R> /                 (T / E extractors)
          result_error_type<R>                
      3.  is_result_value_mapper<F, T>           (F callable (const T&))
      4.  is_result_error_mapper<F, E>           (F callable (const E&))
      5.  is_result_v / ..._mapper_v             (variable-template shorthands)
      6.  result_type /                          (C++20 concept parallels)
          result_value_mapper_for /
          result_error_mapper_for
III.  FACTORIES
      1.  ok<T, E>(value)
      2.  err<T, E>(error)
IV.   COMBINATOR FACTORIES (pipeline form)
      1.  or_value_with(default)                  (extract value or default)
      2.  map_err_with(f)                         (transform error side)
      3.  unwrap_with(message)                    (extract value or throw)
V.    MONAD TRAITS SPECIALIZATION
VI.   FREE-FUNCTION HELPERS
      1.  collect(container_of_result)            -> result<container, E>
      2.  combine(r1, r2, f)                      (binary, both must be ok)
      3.  to_maybe(result)                        (lossy: drops error)
*/


#ifndef DJINTERP_FUNCTIONAL_RESULT_
#define DJINTERP_FUNCTIONAL_RESULT_ 1

// std
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./monad.hpp"
#include "./maybe.hpp"
#include "./foldable.hpp"
#include "./traversable.hpp"
#include "./bifunctor.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    RESULT PRIMITIVE                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // ok_tag / err_tag
    //   helpers: disambiguators for result's tagged constructors.
    // Used so that ok<T, E>(x) and err<T, E>(x) can be value-
    // constructed even when _Type and _Error are the same type.
    struct ok_tag
    {
        struct construct_tag {};
        D_CONSTEXPR explicit ok_tag(construct_tag) {}
    };

    struct err_tag
    {
        struct construct_tag {};
        D_CONSTEXPR explicit err_tag(construct_tag) {}
    };

NS_END  // internal


// result
//   class: holds either a value of type _Type (ok) or an error of
// type _Error (err). The active branch is tracked by a boolean
// discriminator; storage for the inactive branch is unused.
//
//   result is value-typed and supports copy, move, assignment,
// equality (when both _Type and _Error support ==), and pattern-matching
// access via match(). The default constructor is intentionally
// deleted: every result must be explicitly an ok or an err.
template<typename _Type,
         typename _Error>
class result
{
public:
    using value_type = _Type;
    using error_type = _Error;

    // constructor (deleted default)
    //   no implicit "empty" state; constructors below are the
    // only way to instantiate.
    result() = delete;

    // constructor (ok, copy)
    D_CONSTEXPR20 result(
        internal::ok_tag,
        const _Type& _value
    )
        : m_is_ok(true)
    {
        construct_value(_value);
    }

    // constructor (ok, move)
    D_CONSTEXPR20 result(
        internal::ok_tag,
        _Type&& _value
    )
        : m_is_ok(true)
    {
        construct_value(std::move(_value));
    }

    // constructor (err, copy)
    D_CONSTEXPR20 result(
        internal::err_tag,
        const _Error& _error
    )
        : m_is_ok(false)
    {
        construct_error(_error);
    }

    // constructor (err, move)
    D_CONSTEXPR20 result(
        internal::err_tag,
        _Error&& _error
    )
        : m_is_ok(false)
    {
        construct_error(std::move(_error));
    }

    // constructor (copy)
    D_CONSTEXPR20 result(
        const result& _other
    )
        : m_is_ok(_other.m_is_ok)
    {
        if (m_is_ok)
        {
            construct_value(*_other.value_pointer());
        }
        else
        {
            construct_error(*_other.error_pointer());
        }
    }

    // constructor (move)
    D_CONSTEXPR20 result(
        result&& _other
    ) noexcept(std::is_nothrow_move_constructible<_Type>::value &&
               std::is_nothrow_move_constructible<_Error>::value)
        : m_is_ok(_other.m_is_ok)
    {
        if (m_is_ok)
        {
            construct_value(std::move(*_other.value_pointer()));
        }
        else
        {
            construct_error(std::move(*_other.error_pointer()));
        }
    }

    // destructor
    D_CONSTEXPR20
    ~result()
    {
        destroy_active();
    }

    // assignment (copy)
    D_CONSTEXPR20 result& 
    operator=(
        const result& _other
    )
    {
        if (this == &_other)
        {
            return *this;
        }

        if (m_is_ok == _other.m_is_ok)
        {
            // same branch: assign in place
            if (m_is_ok)
            {
                *value_pointer() = *_other.value_pointer();
            }
            else
            {
                *error_pointer() = *_other.error_pointer();
            }
        }
        else
        {
            // different branch: destroy then reconstruct
            destroy_active();

            m_is_ok = _other.m_is_ok;

            if (m_is_ok)
            {
                construct_value(*_other.value_pointer());
            }
            else
            {
                construct_error(*_other.error_pointer());
            }
        }

        return *this;
    }

    // assignment (move)
    D_CONSTEXPR20 result& 
    operator=(
        result&& _other
    ) noexcept(std::is_nothrow_move_assignable<_Type>::value     &&
               std::is_nothrow_move_assignable<_Error>::value    &&
               std::is_nothrow_move_constructible<_Type>::value  &&
               std::is_nothrow_move_constructible<_Error>::value)
    {
        if (this == &_other)
        {
            return *this;
        }

        if (m_is_ok == _other.m_is_ok)
        {
            if (m_is_ok)
            {
                *value_pointer() = std::move(*_other.value_pointer());
            }
            else
            {
                *error_pointer() = std::move(*_other.error_pointer());
            }
        }
        else
        {
            destroy_active();

            m_is_ok = _other.m_is_ok;

            if (m_is_ok)
            {
                construct_value(std::move(*_other.value_pointer()));
            }
            else
            {
                construct_error(std::move(*_other.error_pointer()));
            }
        }

        return *this;
    }

    // is_ok
    //   method: whether this result holds a value.
    D_NODISCARD D_CONSTEXPR bool 
    is_ok() const noexcept
    {
        return m_is_ok;
    }

    // is_err
    //   method: whether this result holds an error.
    D_CONSTEXPR bool 
    is_err() const noexcept
    {
        return !m_is_ok;
    }

    // value (const)
    //   method: returns the contained value. Behavior is
    // undefined when is_err(); use value_or or unwrap for safe
    // access.
    D_NODISCARD D_CONSTEXPR20 const 
    _Type& value() const&
    {
        return *value_pointer();
    }

    // value (mutable)
    D_NODISCARD D_CONSTEXPR20 _Type& 
    value() &
    {
        return *value_pointer();
    }

    // value (rvalue)
    D_NODISCARD D_CONSTEXPR20 _Type&& 
    value() &&
    {
        return std::move(*value_pointer());
    }

    // error (const)
    //   method: returns the contained error. Behavior is
    // undefined when is_ok().
    D_NODISCARD D_CONSTEXPR20 const _Error& 
    error() const&
    {
        return *error_pointer();
    }

    // error (mutable)
    D_NODISCARD D_CONSTEXPR20 _Error& 
    error() &
    {
        return *error_pointer();
    }

    // value_or
    //   method: returns the contained value if ok, otherwise
    // _default. _default is evaluated unconditionally; for
    // expensive defaults use or_else with a lambda.
    template<typename _U>
    D_NODISCARD D_CONSTEXPR20 _Type
    value_or(
        _U&& _default
    ) const&
    {
        if (m_is_ok)
        {
            return *value_pointer();
        }

        return static_cast<_Type>(std::forward<_U>(_default));
    }

    // unwrap
    //   method: returns the contained value, or throws
    // std::runtime_error with the given message if err.
    D_NODISCARD const _Type& 
    unwrap(
        const std::string& _message
    ) const&
    {
        if (!m_is_ok)
        {
            throw std::runtime_error(_message);
        }

        return *value_pointer();
    }

    // map
    //   method: if ok, applies _function to the value and wraps
    // the result; if err, propagates the error unchanged into the
    // new result type.
    template<typename _Function>
    D_NODISCARD D_CONSTEXPR20 auto
    map(
        _Function _function
    ) const
    -> result<typename std::decay<decltype(
        _function(std::declval<const _Type&>()))>::type, _Error>
    {
        using mapped_t = typename std::decay<decltype(
            _function(std::declval<const _Type&>()))>::type;
        using out_t    = result<mapped_t, _Error>;

        if (m_is_ok)
        {
            return out_t(internal::ok_tag(
                internal::ok_tag::construct_tag{}),
                _function(*value_pointer()));
        }

        return out_t(internal::err_tag(
            internal::err_tag::construct_tag{}),
            *error_pointer());
    }

    // map_err
    //   method: transforms the error side via _function; the ok
    // case is propagated unchanged. Useful for converting between
    // error type hierarchies.
    template<typename _Function>
    D_NODISCARD D_CONSTEXPR20 auto 
    map_err(
        _Function _function
    ) const
    -> result<_Type, typename std::decay<decltype(
        _function(std::declval<const _Error&>()))>::type>
    {
        using mapped_e = typename std::decay<decltype(
            _function(std::declval<const _Error&>()))>::type;
        using out_t    = result<_Type, mapped_e>;

        if (m_is_ok)
        {
            return out_t(internal::ok_tag(
                internal::ok_tag::construct_tag{}),
                *value_pointer());
        }

        return out_t(internal::err_tag(
            internal::err_tag::construct_tag{}),
            _function(*error_pointer()));
    }

    // and_then
    //   method: monadic bind on the ok side. _function must
    // return a result whose error type matches this one; on err,
    // _function is not invoked and the error propagates.
    template<typename _Function>
    D_NODISCARD D_CONSTEXPR20 auto 
    and_then(
        _Function _function
    ) const
    -> typename std::decay<decltype(
        _function(std::declval<const _Type&>()))>::type
    {
        using out_t = typename std::decay<decltype(
            _function(std::declval<const _Type&>()))>::type;

        if (m_is_ok)
        {
            return _function(*value_pointer());
        }

        return out_t(internal::err_tag(
            internal::err_tag::construct_tag{}),
            *error_pointer());
    }

    // or_else
    //   method: if ok, returns *this unchanged; if err, invokes
    // _function on the error to produce a recovery result. The
    // recovery function may return a result of the same shape
    // (offering an alternative ok value, or a different err).
    template<typename _Function>
    D_NODISCARD D_CONSTEXPR20 auto 
    or_else(
        _Function _function
    ) const
    -> typename std::decay<decltype(
        _function(std::declval<const _Error&>()))>::type
    {
        using out_t = typename std::decay<decltype(
            _function(std::declval<const _Error&>()))>::type;

        if (m_is_ok)
        {
            return out_t(internal::ok_tag(
                internal::ok_tag::construct_tag{}),
                *value_pointer());
        }

        return _function(*error_pointer());
    }

    // match
    //   method: pattern-match dispatch. _on_ok is invoked with the
    // value if ok; _on_err is invoked with the error if err. Both
    // callables must return the same type.
    template<typename _OnOk,
             typename _OnErr>
    D_NODISCARD D_CONSTEXPR20 auto 
    match(
        _OnOk  _on_ok,
        _OnErr _on_err
    ) const
    -> typename std::decay<decltype(
        _on_ok(std::declval<const _Type&>()))>::type
    {
        if (m_is_ok)
        {
            return _on_ok(*value_pointer());
        }

        return _on_err(*error_pointer());
    }

    // operator bool
    //   converts to true iff is_ok(). Explicit to avoid silent
    // coercions in arithmetic contexts.
    explicit D_CONSTEXPR
    operator bool() const noexcept
    {
        return m_is_ok;
    }

    // ok (conversion to maybe)
    //   method: returns just(value) if ok, nothing if err. Lossy:
    // drops the error information.
    D_NODISCARD maybe<_Type>
    ok() const
    {
        if (m_is_ok)
        {
            return maybe<_Type>(*value_pointer());
        }

        return maybe<_Type>{};
    }

    // err (conversion to maybe)
    //   method: returns just(error) if err, nothing if ok.
    D_NODISCARD maybe<_Error> 
    err() const
    {
        if (!m_is_ok)
        {
            return maybe<_Error>(*error_pointer());
        }

        return maybe<_Error>{};
    }

private:
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // ---- C++20 tagged-union storage ----
    //   Direct union of _Type and _Error with explicit lifetime management via
    // std::construct_at / std::destroy_at (both constexpr in C++20). No
    // aligned_storage, placement-new, or void* casts -- so result<T,E>
    // is usable in a constant expression.
    union storage_t
    {
        struct empty_t {} m_empty;
        _Type             m_value;
        _Error            m_error;

        D_CONSTEXPR20 storage_t() noexcept : m_empty() {}
        D_CONSTEXPR20 ~storage_t() {}
    };

    storage_t m_union;

    // construct_value / construct_error
    //   construct the active branch in place. Caller is responsible for
    // setting m_is_ok consistently.
    template<typename... _Args>
    D_CONSTEXPR20 void 
    construct_value(
        _Args&&... _args
    )
    {
        std::construct_at(std::addressof(m_union.m_value),
                          std::forward<_Args>(_args)...);

        return;
    }

    template<typename... _Args>
    D_CONSTEXPR20 void 
    construct_error(
        _Args&&... _args
    )
    {
        std::construct_at(std::addressof(m_union.m_error),
                          std::forward<_Args>(_args)...);

        return;
    }

    // destroy_active
    //   destroys whichever branch is currently live.
    D_CONSTEXPR20 void 
    destroy_active() noexcept
    {
        if (m_is_ok)
        {
            std::destroy_at(std::addressof(m_union.m_value));
        }
        else
        {
            std::destroy_at(std::addressof(m_union.m_error));
        }

        return;
    }

    // typed pointers to the active union members
    D_CONSTEXPR20 const _Type* 
    value_pointer() const noexcept
    {
        return std::addressof(m_union.m_value);
    }

    D_CONSTEXPR20 _Type* 
    value_pointer() noexcept
    {
        return std::addressof(m_union.m_value);
    }

    D_CONSTEXPR20 const _Error* 
    error_pointer() const noexcept
    {
        return std::addressof(m_union.m_error);
    }

    D_CONSTEXPR20 _Error*
    error_pointer() noexcept
    {
        return std::addressof(m_union.m_error);
    }
#else
    // ---- pre-C++20 aligned_storage storage ----
    //   Two aligned buffers in a union; placement-new + explicit
    // destructor. Identical behavior/ABI to the original; not usable in
    // a constant expression before C++20.

    // construct_value / construct_error
    template<typename... _Args>
    void 
    construct_value(
        _Args&&... _args
    )
    {
        new (static_cast<void*>(&m_value_storage))
            _Type(std::forward<_Args>(_args)...);

        return;
    }

    template<typename... _Args>
    void
    construct_error(
        _Args&&... _args
    )
    {
        new (static_cast<void*>(&m_error_storage))
            _Error(std::forward<_Args>(_args)...);

        return;
    }

    // destroy_active
    void 
    destroy_active() noexcept
    {
        if (m_is_ok)
        {
            value_pointer()->~_Type();
        }
        else
        {
            error_pointer()->~_Error();
        }

        return;
    }

    // typed pointers into the aligned storage
    const _Type* 
    value_pointer() const noexcept
    {
        return static_cast<const _Type*>(
            static_cast<const void*>(&m_value_storage));
    }

    _Type* 
    value_pointer() noexcept
    {
        return static_cast<_Type*>(
            static_cast<void*>(&m_value_storage));
    }

    const _Error*
    error_pointer() const noexcept
    {
        return static_cast<const _Error*>(
            static_cast<const void*>(&m_error_storage));
    }

    _Error* 
    error_pointer() noexcept
    {
        return static_cast<_Error*>(
            static_cast<void*>(&m_error_storage));
    }

    // Storage: two separate aligned buffers (one for each branch).
    // Only the buffer corresponding to m_is_ok is active.
    union
    {
        typename std::aligned_storage<sizeof(_Type), alignof(_Type)>::type m_value_storage;
        typename std::aligned_storage<sizeof(_Error), alignof(_Error)>::type m_error_storage;
    };
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    bool m_is_ok;
};


///////////////////////////////////////////////////////////////////////////////
//                            EQUALITY OPERATORS                             //
///////////////////////////////////////////////////////////////////////////////

// operator== (result vs result)
//   true if both are ok with equal values, or both are err with
// equal errors.
template<typename _Type,
         typename _Error>
D_NODISCARD bool 
operator==(
    const result<_Type, _Error>& _a,
    const result<_Type, _Error>& _b
)
{
    if (_a.is_ok() != _b.is_ok())
    {
        return false;
    }

    if (_a.is_ok())
    {
        return (_a.value() == _b.value());
    }

    return (_a.error() == _b.error());
}

template<typename _Type,
         typename _Error>
D_NODISCARD bool 
operator!=(
    const result<_Type, _Error>& _a,
    const result<_Type, _Error>& _b
)
{
    return !(_a == _b);
}


///////////////////////////////////////////////////////////////////////////////
///             II.   RESULT SFINAE STRUCTURAL TRAITS & CONCEPTS            ///
///////////////////////////////////////////////////////////////////////////////
//   Self-contained detection vocabulary for result<T, E>: is a type a result,
// what are its value / error types, and is a callable a valid value-side or
// error-side handler for it.  These complement the generic monad traits in
// monad.hpp (is_monad, monad_value_type, is_mappable, ...) with result-
// specific, error-type-aware introspection that generic code can constrain on
// without naming concrete T / E.  Each predicate reduces to a `static
// constexpr bool value`; the extractors yield a `::type`.  The C++20 concepts
// close the section.

NS_INTERNAL

    // is_result_helper
    //   helper: primary is std::false_type; the result<_Type, _Error> partial
    // specialization lifts it to std::true_type. Kept internal so the
    // public is_result can decay its argument before matching.
    template<typename _Type>
    struct is_result_helper
        : std::false_type
    {};

    template<typename _Type,
             typename _Error>
    struct is_result_helper<result<_Type, _Error>>
        : std::true_type
    {};

    // result_decompose_helper
    //   helper: primary exposes no members (soft failure for non-result
    // types); the result<_Type, _Error> specialization exposes the value and
    // error types. Used by the SFINAE-friendly extractors below.
    template<typename _Type>
    struct result_decompose_helper
    {};

    template<typename _Type,
             typename _Error>
    struct result_decompose_helper<result<_Type, _Error>>
    {
        using value_type = _Type;
        using error_type = _Error;
    };


    // is_callable_with_helper
    //   helper: SFINAE-detects whether _Fn can be invoked with a single
    // const _Arg&. Return type is unconstrained -- result's value / error
    // handlers (map, map_err, match arms) accept any return type.
    template<typename _Fn,
             typename _Arg>
    struct is_callable_with_helper
    {
    private:
        template<typename _F,
                 typename _A>
        static auto test(int)
            -> decltype(
                static_cast<void>(
                    std::declval<const _F&>()(
                        std::declval<const _A&>())),
                std::true_type{});

        template<typename,
                 typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Fn, _Arg>(0));
    };

NS_END  // internal


// is_result
//   trait: true if _Type is a result<_U, _F> specialization, after
// stripping cv-qualifiers and references. False for every other type.
template<typename _Type>
struct is_result
    : internal::is_result_helper<typename std::decay<_Type>::type>::type
{};

// result_value_type
//   trait: the success type _Type of a result<_Type, _Error>. SFINAE-friendly:
// has a `::type` only when _Result is (a cv/ref-qualified) result.
template<typename _Result>
struct result_value_type
{
    using type = typename internal::result_decompose_helper<
        typename std::decay<_Result>::type>::value_type;
};

// result_value_type_t
//   alias: shorthand for result_value_type<_Result>::type.
template<typename _Result>
using result_value_type_t = typename result_value_type<_Result>::type;


// result_error_type
//   trait: the error type _Error of a result<_Type, _Error>. SFINAE-friendly.
template<typename _Result>
struct result_error_type
{
    using type = typename internal::result_decompose_helper<
        typename std::decay<_Result>::type>::error_type;
};

// result_error_type_t
//   alias: shorthand for result_error_type<_Result>::type.
template<typename _Result>
using result_error_type_t = typename result_error_type<_Result>::type;


// is_result_value_mapper
//   trait: true if _Fn is callable as _Fn(const _Type&) -- the value-side
// handler shape accepted by result::map, result::and_then, and the ok
// arm of result::match. The return type is unconstrained.
template<typename _Fn,
         typename _Type>
struct is_result_value_mapper
    : internal::is_callable_with_helper<_Fn, _Type>::type
{};

// is_result_error_mapper
//   trait: true if _Fn is callable as _Fn(const _Error&) -- the error-side
// handler shape accepted by result::map_err, result::or_else, and the
// err arm of result::match. The return type is unconstrained.
template<typename _Fn,
         typename _Error>
struct is_result_error_mapper
    : internal::is_callable_with_helper<_Fn, _Error>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_result_v
    //   variable: shorthand for is_result<_Type>::value. Available only
    // when variable templates are supported (C++14+).
    template<typename _Type>
    static constexpr bool is_result_v = is_result<_Type>::value;

    // is_result_value_mapper_v
    //   variable: shorthand for is_result_value_mapper<_Fn, _Type>::value.
    template<typename _Fn,
             typename _Type>
    static constexpr bool is_result_value_mapper_v =
        is_result_value_mapper<_Fn, _Type>::value;

    // is_result_error_mapper_v
    //   variable: shorthand for is_result_error_mapper<_Fn, _Error>::value.
    template<typename _Fn,
             typename _Error>
    static constexpr bool is_result_error_mapper_v =
        is_result_error_mapper<_Fn, _Error>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // result_type
    //   concept: satisfied by any result<_U, _F> specialization (cv-ref
    // stripped). The C++20 parallel of is_result.
    template<typename _Type>
    concept result_type = is_result<_Type>::value;

    // result_value_mapper_for
    //   concept: satisfied when _Fn is a valid value-side handler for a
    // result whose success type is _Type. The C++20 parallel of
    // is_result_value_mapper.
    template<typename _Fn,
             typename _Type>
    concept result_value_mapper_for = is_result_value_mapper<_Fn, _Type>::value;

    // result_error_mapper_for
    //   concept: satisfied when _Fn is a valid error-side handler for a
    // result whose error type is _Error. The C++20 parallel of
    // is_result_error_mapper.
    template<typename _Fn,
             typename _Error>
    concept result_error_mapper_for = is_result_error_mapper<_Fn, _Error>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             III.  FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////
//   DUAL DOMAIN.  Like maybe, result lifts uniformly: ok / err and the
// map / map_err / and_then / or_else combinators carry carrier leaves (val_t /
// type_t) as readily as ordinary values, and propagate the err case unchanged.
// Under C++20 - where result's union storage and destructor are constexpr - a
// carrier-holding result is a constant expression, so map / bind fold at compile
// time (and monad_bind / monad_map over result do too; see monad.hpp).  On the
// C++17 floor result has a non-trivial destructor and is therefore NOT a literal
// type, so result is a runtime construct there.

// ok
//   function: builds an ok result holding _value. Both _Type and _Error
// must be specified explicitly because there is no error to
// deduce from.
template<typename _Type,
         typename _Error,
         typename _Value>
D_NODISCARD D_CONSTEXPR result<_Type, _Error>
ok(
    _Value&& _value
)
{
    return result<_Type, _Error>(
        internal::ok_tag(internal::ok_tag::construct_tag{}),
        std::forward<_Value>(_value));
}


// err
//   function: builds an err result holding _error. Both _Type and
// _Error must be specified explicitly.
template<typename _Type,
         typename _Error,
         typename _Value>
D_NODISCARD D_CONSTEXPR result<_Type, _Error>
err(
    _Value&& _error
)
{
    return result<_Type, _Error>(
        internal::err_tag(internal::err_tag::construct_tag{}),
        std::forward<_Value>(_error));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   COMBINATOR FACTORIES                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // or_value_combinator
    //   helper: when piped against a result, extracts the value
    // or returns the stored default. (Renamed from or_else to
    // avoid colliding with the method on the class, since the
    // method takes a recovery function rather than a default
    // value.)
    template<typename _Default>
    class or_value_combinator
    {
    public:
        template<typename _DFwd>
        D_CONSTEXPR explicit 
        or_value_combinator(
            _DFwd&& _default
        )
            : m_default(std::forward<_DFwd>(_default))
        {}

        template<typename _Type,
                 typename _Error>
        D_CONSTEXPR _Type 
        apply(
            const result<_Type, _Error>& _r
        ) const
        {
            return _r.value_or(m_default);
        }

    private:
        _Default m_default;
    };


    // map_err_combinator
    //   helper: stores an error-transform function; when piped,
    // forwards to result::map_err.
    template<typename _Function>
    class map_err_combinator
    {
    public:
        template<typename _FFwd>
        D_CONSTEXPR explicit
        map_err_combinator(
            _FFwd&& _function
        )
            : m_function(std::forward<_FFwd>(_function))
        {}

        template<typename _Type,
                 typename _Error>
        D_CONSTEXPR auto 
        apply(
            const result<_Type, _Error>& _r
        ) const
        -> decltype(_r.map_err(std::declval<const _Function&>()))
        {
            return _r.map_err(m_function);
        }

    private:
        _Function m_function;
    };


    // unwrap_combinator
    //   helper: stores an error message; on pipe, returns the
    // value or throws std::runtime_error.
    class unwrap_combinator
    {
    public:
        explicit unwrap_combinator(
            std::string _message
        )
            : m_message(std::move(_message))
        {}

        template<typename _Type,
                 typename _Error>
        _Type apply(
            const result<_Type, _Error>& _r
        ) const
        {
            return _r.unwrap(m_message);
        }

    private:
        std::string m_message;
    };

NS_END  // internal


// or_value_with
//   function: combinator that, when piped against a result,
// returns the contained value if ok, otherwise _default.
//   Usage:  r | or_value_with(0)
template<typename _Default>
D_NODISCARD D_CONSTEXPR internal::or_value_combinator<typename std::decay<_Default>::type>
or_value_with(
    _Default&& _default
)
{
    return internal::or_value_combinator<
        typename std::decay<_Default>::type>(
            std::forward<_Default>(_default));
}


// map_err_with
//   function: combinator that, when piped against a result,
// transforms the error side via _function. Ok values pass
// through unchanged.
//   Usage:  r | map_err_with([](int code) { return error_msg(code); })
template<typename _Function>
D_NODISCARD D_CONSTEXPR internal::map_err_combinator<typename std::decay<_Function>::type>
map_err_with(
    _Function&& _function
)
{
    return internal::map_err_combinator<
        typename std::decay<_Function>::type>(
            std::forward<_Function>(_function));
}


// unwrap_with
//   function: combinator that, when piped against a result,
// returns the value or throws std::runtime_error(_message).
inline internal::unwrap_combinator
unwrap_with(
    std::string _message
)
{
    return internal::unwrap_combinator(std::move(_message));
}


// operator| (result | combinator)
//   pipeline operator for result combinators. SFINAE-constrained
// to those defined in this module.
template<typename _Type,
         typename _Error,
         typename _Combinator,
         typename = decltype(
             std::declval<const _Combinator&>().apply(
                 std::declval<const result<_Type, _Error>&>()))>
D_CONSTEXPR auto 
operator|(
    const result<_Type, _Error>& _r,
    _Combinator&&                _combinator
)
-> decltype(_combinator.apply(_r))
{
    return _combinator.apply(_r);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    MONAD TRAITS SPECIALIZATION                           ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<result<_Type, _Error>>
//   specialization: makes result participate in the generic
// monad protocol over the success type _Type. The error type _Error is
// preserved across map/bind (errors propagate unchanged).
template<typename _Type,
         typename _Error>
struct monad_traits<result<_Type, _Error>>
{
    using is_specialized = std::true_type;
    using value_type     = _Type;
    using error_type     = _Error;

    template<typename _U>
    using rebind = result<_U, _Error>;

    // unit
    //   lifts a value into result as an ok. The error type is
    // preserved by rebind; no error can be produced by unit
    // alone.
    static D_CONSTEXPR20 result<_Type, _Error>
    unit(
        _Type _value
    )
    {
        return result<_Type, _Error>(
            internal::ok_tag(internal::ok_tag::construct_tag{}),
            std::move(_value));
    }

    // bind
    //   monadic bind on the success side. Threads the value
    // through _function (which must return a result with the
    // same _Error); err propagates unchanged.
    //   D_CONSTEXPR20 so the generic monad_bind / monad_map fold at
    // compile time over a carrier-holding result under C++20 (runtime
    // on the C++17 floor, where result is not a literal type).
    template<typename _Function>
    static D_CONSTEXPR20 auto 
    bind(
        const result<_Type, _Error>& _r,
        _Function             _function
    )
    -> typename std::decay<decltype(
        _function(std::declval<const _Type&>()))>::type
    {
        return _r.and_then(_function);
    }
};


// foldable_traits<result<_Type, _Error>>
//   specialization: makes result participate in the generic foldable
// protocol over its success type _Type. An ok folds its one value; an err is
// treated as empty (the error is not an element), so fold_left is the
// identity on err -- consistent with map / bind, which propagate err
// untouched. Keyed on is_result so the single instance covers every
// result<T, E>.
template<typename _Result>
struct foldable_traits<
    _Result,
    typename std::enable_if<is_result<_Result>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename _Result::value_type;

    // fold_left
    //   threads _init through the success value when ok; identity on err.
    //   D_CONSTEXPR20 so the generic folds fold at compile time over a
    // carrier-holding result under C++20 (runtime on the C++17 floor, where
    // result is not a literal type).
    template<typename _Acc,
             typename _Function>
    static
    D_CONSTEXPR20
    _Acc fold_left(
        const _Result& _r,
        _Acc           _init,
        _Function      _function
    )
    {
        if (_r.is_ok())
        {
            return _function(std::move(_init), _r.value());
        }

        return _init;
    }
};


NS_INTERNAL

    // traversable_ok_helper
    //   helper: wraps a bare value into an ok, used by result's traverse to
    // turn F<B> into F<result<B,Error>> via functor_map. A named functor so it
    // can appear in trailing return types on every floor.
    template<typename _Value,
             typename _Error>
    struct traversable_ok_helper
    {
        D_CONSTEXPR20
        result<_Value, _Error> operator()(
            const _Value& _value
        ) const
        {
            return ::djinterp::ok<_Value, _Error>(_value);
        }
    };

NS_END  // internal


// traversable_traits<result<_Type, _Error>>
//   specialization: result is Traversable over its success side. Traversing an
// ok runs the effect on its value and re-wraps the result in an ok inside the
// effect (F<B> -> F<result<B,Error>>); traversing an err injects that same err
// into the effect with pure -- the effect F being recovered from the type of
// f's result, so the err branch is well-typed even though f is never called.
// Keyed on is_result.
template<typename _Result>
struct traversable_traits<
    _Result,
    typename std::enable_if<is_result<_Result>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename _Result::value_type;
    using error_type     = typename _Result::error_type;

    // traverse
    //   F<result<B,Error>> from a result<A,Error> and f : A -> F<B>.
    template<typename _Function>
    static
    D_CONSTEXPR20
    auto traverse(
        const _Result& _r,
        _Function      _function
    )
    -> decltype(::djinterp::functor_map(
           _function(std::declval<const value_type&>()),
           internal::traversable_ok_helper<
               applicative_value_type_t<decltype(
                   _function(std::declval<const value_type&>()))>,
               error_type>()))
    {
        using effect_t = decltype(
            _function(std::declval<const value_type&>()));      // F<B>
        using inner_t  = applicative_value_type_t<effect_t>;     // B
        using wrap_t   = internal::traversable_ok_helper<inner_t, error_type>;
        using result_t = decltype(::djinterp::functor_map(
            std::declval<effect_t>(), std::declval<wrap_t>()));  // F<result<B,Error>>

        if (_r.is_ok())
        {
            return ::djinterp::functor_map(_function(_r.value()), wrap_t());
        }

        return ::djinterp::pure<result_t>(
            ::djinterp::err<inner_t, error_type>(_r.error()));
    }
};


// bifunctor_traits<result<_Type, _Error>>
//   specialization: result is a Bifunctor over (success, error). bimap maps
// the success side with f and the error side with g -- exactly map followed by
// map_err -- so the one-sided map_first / map_second recover result's own map
// and map_err. Keyed on is_result.
template<typename _Result>
struct bifunctor_traits<
    _Result,
    typename std::enable_if<is_result<_Result>::value>::type>
{
    using is_specialized = std::true_type;
    using first_type     = typename _Result::value_type;
    using second_type    = typename _Result::error_type;

    // bimap
    //   result<f(T), g(E)> from result<T, E>.
    template<typename _First,
             typename _Second>
    static
    D_CONSTEXPR20
    auto bimap(
        const _Result& _r,
        _First         _f,
        _Second        _g
    )
    -> decltype(std::declval<const _Result&>().map(std::declval<_First&>())
                    .map_err(std::declval<_Second&>()))
    {
        return _r.map(_f).map_err(_g);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VI.   FREE-FUNCTION HELPERS                                 ///
///////////////////////////////////////////////////////////////////////////////

// collect (container of result)
//   function: turns a container of result<T, E> into
// result<vector<T>, E>. Returns ok(vector) if every element is
// ok, otherwise the first err encountered (short-circuit).
//   SFINAE-constrained (via is_result on the element type) so this
// overload is viable only for containers of result. Without the
// constraint it would collide with the equally-unconstrained
// collect in maybe.hpp -- a result<T, E> element exposes a nested
// value_type, which is all maybe's collect requires, so both
// overloads would otherwise be ambiguous for a container of result.
template<typename _Container,
         typename std::enable_if<
             is_result<typename _Container::value_type>::value,
             int>::type = 0>
D_NODISCARD auto 
collect(
    const _Container& _container
)
-> result<std::vector<typename _Container::value_type::value_type>,
          typename _Container::value_type::error_type>
{
    using element_t = typename _Container::value_type;
    using inner_t   = typename element_t::value_type;
    using error_t   = typename element_t::error_type;
    using out_t     = result<std::vector<inner_t>, error_t>;

    std::vector<inner_t> result_vec;

    for (const auto& element : _container)
    {
        if (element.is_err())
        {
            return out_t(internal::err_tag(
                internal::err_tag::construct_tag{}),
                element.error());
        }

        result_vec.push_back(element.value());
    }

    return out_t(internal::ok_tag(
        internal::ok_tag::construct_tag{}),
        std::move(result_vec));
}


// combine (binary)
//   function: combines two results via a binary function. Returns
// ok(f(a, b)) if both are ok. Otherwise returns the first err.
// Useful for "two-input" operations where either input may fail.
template<typename _A,
         typename _B,
         typename _Error,
         typename _Function>
D_NODISCARD auto 
combine(
    const result<_A, _Error>& _ra,
    const result<_B, _Error>& _rb,
    _Function             _function
)
-> result<typename std::decay<decltype(
    _function(std::declval<const _A&>(),
              std::declval<const _B&>()))>::type, _Error>
{
    using out_value_t = typename std::decay<decltype(
        _function(std::declval<const _A&>(),
                  std::declval<const _B&>()))>::type;
    using out_t = result<out_value_t, _Error>;

    if (_ra.is_err())
    {
        return out_t(internal::err_tag(
            internal::err_tag::construct_tag{}),
            _ra.error());
    }

    if (_rb.is_err())
    {
        return out_t(internal::err_tag(
            internal::err_tag::construct_tag{}),
            _rb.error());
    }

    return out_t(internal::ok_tag(
        internal::ok_tag::construct_tag{}),
        _function(_ra.value(), _rb.value()));
}


// to_maybe
//   function: converts a result into a maybe by discarding the
// error. Lossy; use only when the caller does not care about
// the failure reason.
template<typename _Type,
         typename _Error>
D_NODISCARD maybe<_Type>
to_maybe(
    const result<_Type, _Error>& _r
)
{
    return _r.ok();
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_RESULT_