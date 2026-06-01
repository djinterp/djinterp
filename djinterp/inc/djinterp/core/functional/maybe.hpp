/******************************************************************************
* djinterp [functional]                                              maybe.hpp
*
* Maybe<T> -- a monadic optional value type (C++).
*   Represents a value that may or may not be present. Equivalent in
* purpose to std::optional<T> (C++17) but available on C++11+, integrated
* with the djinterp monad protocol, and with a richer fluent API for
* functional pipelines.
*
*   maybe<T> is "nothing" or "just(x)". All inspection methods are
* explicit (no implicit bool conversion that would defeat type safety),
* and value access on a "nothing" yields default-constructed _Type (use
* value_or, expect, or pattern-match via match() for safer access).
*
*   Storage uses std::aligned_storage so T need not be default-
* constructible.  Construction, copy, move, and destruction are
* properly managed.
*
* USAGE:
*   maybe<int> a = just(5);
*   maybe<int> b = nothing<int>();
*
*   if (a.has_value()) { ...use a.value()... }
*   int x = b.value_or(0);                          // 0
*
*   // monadic chain
*   auto result = a
*               | bind_with([](int v) { return safe_div(100, v); })
*               | map_with([](int v) { return v * 2; })
*               | or_else_with(-1);
*
*   // pattern matching
*   auto out = a.match(
*       [](int v) { return std::to_string(v); },
*       []      { return std::string("none"); });
*
*   // conversion from pointer / std::optional-like sources
*   maybe<int> p = from_pointer(some_int_ptr);
*
* path:      /inc/djinterp/core/functional/maybe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    MAYBE PRIMITIVE
      1.  maybe<T>                                (storage + interface)
      2.  nothing_t / nothing_v                   (empty-maybe tag)
II.   PREDICATE & STRUCTURAL TRAITS
      1.  is_maybe<T>                             (detects maybe<U>)
      2.  is_maybe_predicate<P, T>                (P callable (const T&) -> bool)
      3.  is_maybe_v / is_maybe_predicate_v       (variable-template shorthands)
      4.  maybe_type / maybe_predicate_for        (C++20 concept parallels)
III.  FACTORIES
      1.  just(value)
      2.  nothing<T>()
      3.  from_pointer(ptr)
      4.  from_predicate(value, predicate)
IV.   COMBINATOR FACTORIES (pipeline form)
      1.  or_else_with(default)
      2.  filter_with(predicate)
      3.  unwrap_or_with(default)                 (alias for or_else_with)
      4.  expect_with(message)
V.    MONAD TRAITS SPECIALIZATION
VI.   FREE-FUNCTION HELPERS
      1.  zip_with(m1, m2, f)
      2.  flatten(m_of_m)
      3.  collect(container_of_maybe)             -> maybe<container>
*/


#ifndef DJINTERP_FUNCTIONAL_MAYBE_
#define DJINTERP_FUNCTIONAL_MAYBE_ 1

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


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    MAYBE PRIMITIVE                                       ///
///////////////////////////////////////////////////////////////////////////////

// nothing_t
//   struct: tag type used to construct an empty maybe. Distinct
// from a value-constructor parameter so that maybe<T>(nothing_v)
// is unambiguously the empty case.
struct nothing_t
{
    struct construct_tag 
    {};
    
    D_CONSTEXPR explicit
    nothing_t(
        construct_tag
    ) 
    {}
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static constexpr nothing_t nothing_v{nothing_t::construct_tag{}};
#else
    static const nothing_t nothing_v = nothing_t(nothing_t::construct_tag{});
#endif


// maybe
//   class: holds either a value of type _Type or nothing. Storage is
// raw bytes (aligned_storage) with a boolean discriminator; the
// value is constructed in place when needed and destroyed when
// reset or when the maybe is destroyed.
//
//   maybe is value-typed: copies and moves perform deep copy /
// move of the contained value. Comparison operators are provided
// when _Type supports them.
template<typename _Type>
class maybe
{
public:
    using value_type = _Type;

    // constructor (empty)
    D_CONSTEXPR
    maybe() noexcept
        : m_has_value(false)
    {}

    // constructor (nothing tag)
    D_CONSTEXPR
    maybe(nothing_t) noexcept
        : m_has_value(false)
    {}

    // constructor (value, copy)
    D_CONSTEXPR20
    maybe(
        const _Type& _value
    )
        : m_has_value(true)
    {
        construct_value(_value);
    }

    // constructor (value, move)
    D_CONSTEXPR20
    maybe(
        _Type&& _value
    )
        : m_has_value(true)
    {
        construct_value(std::move(_value));
    }

    // constructor (copy)
    D_CONSTEXPR20
    maybe(
        const maybe& _other
    )
        : m_has_value(_other.m_has_value)
    {
        if (m_has_value)
        {
            construct_value(*_other.pointer());
        }
    }

    // constructor (move)
    D_CONSTEXPR20
    maybe(
        maybe&& _other
    ) noexcept(std::is_nothrow_move_constructible<_Type>::value)
        : m_has_value(_other.m_has_value)
    {
        if (m_has_value)
        {
            construct_value(std::move(*_other.pointer()));
        }
    }

    // destructor
    D_CONSTEXPR20
    ~maybe()
    {
        reset();
    }

    // assignment (copy)
    D_CONSTEXPR20
    maybe& operator=(
        const maybe& _other
    )
    {
        if (this == &_other)
        {
            return *this;
        }

        if (_other.m_has_value)
        {
            if (m_has_value)
            {
                *pointer() = *_other.pointer();
            }
            else
            {
                construct_value(*_other.pointer());
                m_has_value = true;
            }
        }
        else
        {
            reset();
        }

        return *this;
    }

    // assignment (move)
    D_CONSTEXPR20
    maybe& operator=(
        maybe&& _other
    ) noexcept(std::is_nothrow_move_assignable<_Type>::value &&
               std::is_nothrow_move_constructible<_Type>::value)
    {
        if (this == &_other)
        {
            return *this;
        }

        if (_other.m_has_value)
        {
            if (m_has_value)
            {
                *pointer() = std::move(*_other.pointer());
            }
            else
            {
                construct_value(std::move(*_other.pointer()));
                m_has_value = true;
            }
        }
        else
        {
            reset();
        }

        return *this;
    }

    // assignment (nothing)
    D_CONSTEXPR20
    maybe& operator=(
        nothing_t
    ) noexcept
    {
        reset();

        return *this;
    }

    // assignment (value)
    D_CONSTEXPR20
    maybe& operator=(
        const _Type& _value
    )
    {
        if (m_has_value)
        {
            *pointer() = _value;
        }
        else
        {
            construct_value(_value);
            m_has_value = true;
        }

        return *this;
    }

    D_CONSTEXPR20
    maybe& operator=(
        _Type&& _value
    )
    {
        if (m_has_value)
        {
            *pointer() = std::move(_value);
        }
        else
        {
            construct_value(std::move(_value));
            m_has_value = true;
        }

        return *this;
    }

    // has_value
    //   method: whether this maybe contains a value.
    D_NODISCARD
    D_CONSTEXPR
    bool has_value() const noexcept
    {
        return m_has_value;
    }

    // is_nothing
    //   method: the negation of has_value, for readability in
    // pattern-matching-style code.
    D_NODISCARD
    D_CONSTEXPR
    bool is_nothing() const noexcept
    {
        return !m_has_value;
    }

    // value (const)
    //   method: returns a const reference to the contained value.
    // Behavior is undefined when has_value() is false; use
    // value_or, expect, or match for safe access.
    D_NODISCARD
    D_CONSTEXPR20
    const _Type& value() const&
    {
        return *pointer();
    }

    // value (mutable)
    D_NODISCARD
    D_CONSTEXPR20
    _Type& value() &
    {
        return *pointer();
    }

    // value (rvalue)
    D_NODISCARD
    D_CONSTEXPR20
    _Type&& value() &&
    {
        return std::move(*pointer());
    }

    // value_or
    //   method: returns the contained value if present, otherwise
    // _default. _default is evaluated unconditionally; for
    // expensive defaults, use or_else with a lambda.
    template<typename _U>
    D_NODISCARD
    D_CONSTEXPR20
    _Type value_or(
        _U&& _default
    ) const&
    {
        if (m_has_value)
        {
            return *pointer();
        }

        return static_cast<_Type>(std::forward<_U>(_default));
    }

    // expect
    //   method: returns the contained value if present, otherwise
    // throws std::runtime_error with the given message.
    D_NODISCARD
    const _Type& expect(
        const std::string& _message
    ) const&
    {
        if (!m_has_value)
        {
            throw std::runtime_error(_message);
        }

        return *pointer();
    }

    // reset
    //   method: destroys the contained value, if any, leaving the
    // maybe in the nothing state.
    D_CONSTEXPR20
    void reset() noexcept
    {
        if (m_has_value)
        {
            destroy_value();
            m_has_value = false;
        }

        return;
    }

    // emplace
    //   method: constructs a new value in place from the given
    // arguments. Destroys any existing value first.
    template<typename... _Args>
    D_CONSTEXPR20
    _Type& emplace(
        _Args&&... _args
    )
    {
        reset();
        construct_value(std::forward<_Args>(_args)...);
        m_has_value = true;

        return *pointer();
    }

    // map
    //   method: if this maybe holds a value, applies _function to
    // it and wraps the result in a new maybe. If empty, returns an
    // empty maybe of the mapped type.
    template<typename _Function>
    D_NODISCARD
    D_CONSTEXPR20
    auto map(
        _Function _function
    ) const
    -> maybe<typename std::decay<decltype(
        _function(std::declval<const _Type&>()))>::type>
    {
        using result_t = typename std::decay<decltype(
            _function(std::declval<const _Type&>()))>::type;

        if (m_has_value)
        {
            return maybe<result_t>(_function(*pointer()));
        }

        return maybe<result_t>();
    }

    // and_then
    //   method: monadic bind. _function must return a maybe; if
    // this maybe is empty, _function is not invoked and an empty
    // maybe of the result type is returned.
    template<typename _Function>
    D_NODISCARD
    D_CONSTEXPR20
    auto and_then(
        _Function _function
    ) const
    -> typename std::decay<decltype(
        _function(std::declval<const _Type&>()))>::type
    {
        using result_t = typename std::decay<decltype(
            _function(std::declval<const _Type&>()))>::type;

        if (m_has_value)
        {
            return _function(*pointer());
        }

        return result_t();
    }

    // or_else
    //   method: if this maybe holds a value, returns it; otherwise
    // invokes _function (which must return a maybe of the same
    // value type) and returns its result. Useful for "try this,
    // fall back to that" chains.
    template<typename _Function>
    D_NODISCARD
    D_CONSTEXPR20
    maybe or_else(
        _Function _function
    ) const
    {
        if (m_has_value)
        {
            return *this;
        }

        return _function();
    }

    // filter
    //   method: if this maybe holds a value satisfying _predicate,
    // returns *this; otherwise returns nothing. Combines with map
    // and and_then for conditional pipelines.
    template<typename _Predicate>
    D_NODISCARD
    D_CONSTEXPR20
    maybe filter(
        _Predicate _predicate
    ) const
    {
        if (m_has_value && _predicate(*pointer()))
        {
            return *this;
        }

        return maybe{};
    }

    // match
    //   method: pattern-matching dispatch. Invokes _on_just(value)
    // if a value is present, otherwise _on_nothing(). Both
    // callables must return the same type.
    template<typename _OnJust,
             typename _OnNothing>
    D_NODISCARD
    D_CONSTEXPR20
    auto match(
        _OnJust    _on_just,
        _OnNothing _on_nothing
    ) const
    -> typename std::decay<decltype(
        _on_just(std::declval<const _Type&>()))>::type
    {
        if (m_has_value)
        {
            return _on_just(*pointer());
        }

        return _on_nothing();
    }

    // operator bool
    //   converts to true iff a value is present. Marked explicit
    // to avoid silent coercions in arithmetic contexts.
    explicit D_CONSTEXPR
    operator bool() const noexcept
    {
        return m_has_value;
    }

private:
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // ---- C++20 tagged-union storage ----
    //   A union with a non-trivial member needs user-provided special
    // members; maybe supplies them (ctors/dtor/assign below) and manages
    // the active member explicitly via m_has_value. std::construct_at and
    // std::destroy_at are constexpr in C++20, so the whole type is usable
    // in a constant expression. No aligned_storage, no placement-new
    // through void*, no reinterpret_cast -- all of which are barred from
    // constant evaluation.
    union storage_t
    {
        // empty-state placeholder so the union has an active trivial
        // member when m_has_value is false.
        struct empty_t {} m_empty;
        _Type                m_value;

        // trivial ctor leaves m_empty active; maybe constructs m_value
        // on demand via construct_value().
        D_CONSTEXPR20 storage_t() noexcept : m_empty() {}

        // non-trivial members mean the union cannot auto-generate these;
        // maybe drives lifetime explicitly, so they are empty.
        D_CONSTEXPR20 ~storage_t() {}
    };

    storage_t m_union;

    // construct_value
    //   constructs the inner value in place from forwarded args using
    // std::construct_at (constexpr in C++20).
    template<typename... _Args>
    D_CONSTEXPR20
    void construct_value(
        _Args&&... _args
    )
    {
        std::construct_at(std::addressof(m_union.m_value),
                          std::forward<_Args>(_args)...);

        return;
    }

    // pointer (const) / (mutable)
    //   typed pointer to the active union value. Behavior is undefined
    // unless m_has_value is true.
    D_CONSTEXPR20
    const _Type* pointer() const noexcept
    {
        return std::addressof(m_union.m_value);
    }

    D_CONSTEXPR20
    _Type* pointer() noexcept
    {
        return std::addressof(m_union.m_value);
    }

    // destroy_value
    //   destroys the active value via std::destroy_at (constexpr C++20).
    D_CONSTEXPR20
    void destroy_value() noexcept
    {
        std::destroy_at(std::addressof(m_union.m_value));

        return;
    }
#else
    // ---- pre-C++20 aligned_storage storage ----
    //   Raw aligned bytes plus placement-new / explicit destructor call.
    // Not usable in a constant expression (placement-new and the
    // void*-cast are barred from constant evaluation before C++20), but
    // identical in behavior and ABI to the original.

    // construct_value
    //   constructs the inner value in place from forwarded args.
    template<typename... _Args>
    void construct_value(
        _Args&&... _args
    )
    {
        new (static_cast<void*>(&m_storage))
            _Type(std::forward<_Args>(_args)...);

        return;
    }

    // pointer (const)
    //   typed pointer into the aligned storage. Behavior is
    // undefined unless m_has_value is true.
    const _Type* pointer() const noexcept
    {
        return static_cast<const _Type*>(
            static_cast<const void*>(&m_storage));
    }

    // pointer (mutable)
    _Type* pointer() noexcept
    {
        return static_cast<_Type*>(static_cast<void*>(&m_storage));
    }

    // destroy_value
    //   destroys the active value via an explicit destructor call.
    void destroy_value() noexcept
    {
        pointer()->~_Type();

        return;
    }

    typename std::aligned_storage<sizeof(_Type), alignof(_Type)>::type m_storage;
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    bool m_has_value;
};


///////////////////////////////////////////////////////////////////////////////
//                            EQUALITY OPERATORS                             //
///////////////////////////////////////////////////////////////////////////////

// operator== (maybe vs maybe)
//   true if both empty, or both non-empty with equal values.
template<typename _Type>
D_NODISCARD
bool operator==
(
    const maybe<_Type>& _a,
    const maybe<_Type>& _b
)
{
    if (_a.has_value() != _b.has_value())
    {
        return false;
    }

    if (!_a.has_value())
    {
        return true;
    }

    return (_a.value() == _b.value());
}

template<typename _Type>
D_NODISCARD
bool operator!=
(
    const maybe<_Type>& _a,
    const maybe<_Type>& _b
)
{
    return !(_a == _b);
}

template<typename _Type>
D_NODISCARD
D_CONSTEXPR
bool operator==
(
    const maybe<_Type>&,
    nothing_t
)
{
    return false;
}

template<typename _Type>
D_NODISCARD
D_CONSTEXPR
bool operator==
(
    nothing_t,
    const maybe<_Type>& _m
)
{
    return !_m.has_value();
}


///////////////////////////////////////////////////////////////////////////////
///             II.   PREDICATE & STRUCTURAL TRAITS                         ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_maybe_helper
    //   helper: primary is std::false_type; the maybe<_Type> partial
    // specialization lifts it to std::true_type. Kept internal so the
    // public is_maybe can decay its argument before matching.
    template<typename _Type>
    struct is_maybe_helper
        : std::false_type
    {
    };

    template<typename _Type>
    struct is_maybe_helper<maybe<_Type>>
        : std::true_type
    {
    };


    // is_maybe_predicate_helper
    //   helper: SFINAE-detects whether _Pred can be invoked with a
    // const _Type& and whether the result is contextually convertible to
    // bool (the exact shape filter / from_predicate require). The
    // static_cast<bool> in the detected expression rejects callables
    // whose result is not bool-convertible (e.g. void-returning).
    template<typename _Pred,
             typename _Type>
    struct is_maybe_predicate_helper
    {
    private:
        template<typename _P,
                 typename _U>
        static auto test(int)
            -> decltype(
                static_cast<bool>(
                    std::declval<const _P&>()(
                        std::declval<const _U&>())),
                std::true_type{});

        template<typename,
                 typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<_Pred, _Type>(0));
    };

NS_END  // internal


// is_maybe
//   trait: true if _Type is a maybe<_U> specialization, after
// stripping cv-qualifiers and references. False for every other
// type, including unrelated optional-like types.
template<typename _Type>
struct is_maybe
    : internal::is_maybe_helper<typename std::decay<_Type>::type>::type
{
};


// is_maybe_predicate
//   trait: true if _Pred is callable as _Pred(const _Type&) and the
// result is convertible to bool -- the predicate shape accepted by
// maybe::filter, from_predicate, and filter_with. False when _Pred
// is not callable with a const _Type&, or its result is not
// bool-convertible.
template<typename _Pred,
         typename _Type>
struct is_maybe_predicate
    : internal::is_maybe_predicate_helper<_Pred, _Type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_maybe_v
//   variable: shorthand for is_maybe<_Type>::value. Available only
// when variable templates are supported (C++14+).
template<typename _Type>
static constexpr bool is_maybe_v = is_maybe<_Type>::value;

// is_maybe_predicate_v
//   variable: shorthand for is_maybe_predicate<_Pred, _Type>::value.
template<typename _Pred,
         typename _Type>
static constexpr bool is_maybe_predicate_v =
    is_maybe_predicate<_Pred, _Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// maybe_type
//   concept: satisfied by any maybe<_U> specialization (cv-ref
// stripped). The C++20 parallel of is_maybe. Named maybe_type to
// avoid clashing with djinterp::predicate (the std concept already
// re-exported in concepts.hpp).
template<typename _Type>
concept maybe_type = is_maybe<_Type>::value;

// maybe_predicate_for
//   concept: satisfied when _Pred is a valid filter predicate over
// _Type -- callable as _Pred(const _Type&) with a bool-convertible result.
// The C++20 parallel of is_maybe_predicate.
template<typename _Pred,
         typename _Type>
concept maybe_predicate_for = is_maybe_predicate<_Pred, _Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             III.  FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

// just
//   function: builds a maybe holding _value. Equivalent to
// maybe<T>(value) but reads more clearly at call sites.
template<typename _Type>
D_NODISCARD
D_CONSTEXPR
maybe<typename std::decay<_Type>::type>
just
(
    _Type&& _value
)
{
    return maybe<typename std::decay<_Type>::type>(std::forward<_Type>(_value));
}


// nothing
//   function: builds an empty maybe of the given type. The type
// must be supplied explicitly because there is no value from
// which to deduce it.
template<typename _Type>
D_NODISCARD
D_CONSTEXPR
maybe<_Type>
nothing()
{
    return maybe<_Type>{};
}


// from_pointer
//   function: builds a maybe from a raw pointer: nothing if the
// pointer is null, otherwise just(*ptr). The pointed-to value is
// copied; the pointer itself is not stored.
template<typename _Type>
D_NODISCARD
maybe<typename std::decay<_Type>::type>
from_pointer
(
    const _Type* _ptr
)
{
    if (_ptr == nullptr)
    {
        return maybe<typename std::decay<_Type>::type>{};
    }

    return maybe<typename std::decay<_Type>::type>(*_ptr);
}


// from_predicate
//   function: builds a maybe holding _value if _predicate(_value)
// is true, otherwise nothing. Useful for "validate and wrap" in a
// single expression.
template<typename _Type,
         typename _Predicate>
D_NODISCARD
maybe<typename std::decay<_Type>::type>
from_predicate
(
    _Type&&        _value,
    _Predicate  _predicate
)
{
    using value_t = typename std::decay<_Type>::type;

    if (_predicate(_value))
    {
        return maybe<value_t>(std::forward<_Type>(_value));
    }

    return maybe<value_t>{};
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   COMBINATOR FACTORIES                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // or_else_combinator
    //   helper: stores a default value; when piped against a
    // maybe, extracts the value or returns the default.
    template<typename _Default>
    class or_else_combinator
    {
    public:
        template<typename _DFwd>
        D_CONSTEXPR
        explicit or_else_combinator(
            _DFwd&& _default
        )
            : m_default(std::forward<_DFwd>(_default))
        {}

        template<typename _Type>
        D_CONSTEXPR
        _Type apply(
            const maybe<_Type>& _m
        ) const
        {
            return _m.value_or(m_default);
        }

    private:
        _Default m_default;
    };


    // filter_combinator
    //   helper: stores a predicate; gates the LHS maybe through it.
    template<typename _Predicate>
    class filter_combinator
    {
    public:
        template<typename _PFwd>
        D_CONSTEXPR
        explicit filter_combinator(
            _PFwd&& _predicate
        )
            : m_predicate(std::forward<_PFwd>(_predicate))
        {}

        template<typename _Type>
        D_CONSTEXPR
        maybe<_Type> apply(
            const maybe<_Type>& _m
        ) const
        {
            return _m.filter(m_predicate);
        }

    private:
        _Predicate m_predicate;
    };


    // expect_combinator
    //   helper: stores an error message; on pipe, returns the
    // value or throws.
    class expect_combinator
    {
    public:
        explicit
        expect_combinator(
            std::string _message
        )
            : m_message(std::move(_message))
        {}

        template<typename _Type>
        _Type apply(
            const maybe<_Type>& _m
        ) const
        {
            return _m.expect(m_message);
        }

    private:
        std::string m_message;
    };

NS_END  // internal


// or_else_with
//   function: builds a combinator that, when piped against a
// maybe, returns the contained value or _default.
//   Usage:  m | or_else_with(42)
template<typename _Default>
D_NODISCARD
D_CONSTEXPR
internal::or_else_combinator<typename std::decay<_Default>::type>
or_else_with
(
    _Default&& _default
)
{
    return internal::or_else_combinator<
        typename std::decay<_Default>::type>(
            std::forward<_Default>(_default));
}


// unwrap_or_with
//   function: alias for or_else_with, matching Rust-style naming.
template<typename _Default>
D_NODISCARD
D_CONSTEXPR
internal::or_else_combinator<typename std::decay<_Default>::type>
unwrap_or_with
(
    _Default&& _default
)
{
    return or_else_with(std::forward<_Default>(_default));
}


// filter_with (maybe)
//   function: builds a combinator that, when piped against a
// maybe, returns it unchanged if its value satisfies _predicate,
// otherwise nothing.
template<typename _Predicate>
D_NODISCARD
D_CONSTEXPR
internal::filter_combinator<typename std::decay<_Predicate>::type>
filter_with
(
    _Predicate&& _predicate
)
{
    return internal::filter_combinator<
        typename std::decay<_Predicate>::type>(
            std::forward<_Predicate>(_predicate));
}


// expect_with
//   function: builds a combinator that returns the contained
// value or throws std::runtime_error(_message).
inline
internal::expect_combinator
expect_with
(
    std::string _message
)
{
    return internal::expect_combinator(std::move(_message));
}


// operator| (maybe | combinator)
//   pipeline operator for maybe combinators. SFINAE-constrained
// to those defined in this module (matched by their .apply
// method's signature accepting a maybe).
template<typename _Type,
         typename _Combinator,
         typename = decltype(
             std::declval<const _Combinator&>().apply(
                 std::declval<const maybe<_Type>&>()))>
D_CONSTEXPR
auto operator|
(
    const maybe<_Type>& _m,
    _Combinator&&    _combinator
)
-> decltype(_combinator.apply(_m))
{
    return _combinator.apply(_m);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    MONAD TRAITS SPECIALIZATION                           ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<maybe<_Type>>
//   specialization: makes maybe participate in the generic monad
// protocol. Exposes value_type, rebind, unit, and bind.
template<typename _Type>
struct monad_traits<maybe<_Type>>
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _U>
    using rebind = maybe<_U>;

    // unit
    //   lifts a value into maybe. Equivalent to just().
    static
    D_CONSTEXPR
    maybe<_Type>
    unit
    (
        _Type _value
    )
    {
        return maybe<_Type>(std::move(_value));
    }

    // bind
    //   monadic bind. Threads the contained value through
    // _function (which must return a maybe of some type).
    template<typename _Function>
    static
    auto bind(
        const maybe<_Type>& _m,
        _Function        _function
    )
    -> typename std::decay<decltype(
        _function(std::declval<const _Type&>()))>::type
    {
        return _m.and_then(_function);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VI.   FREE-FUNCTION HELPERS                                 ///
///////////////////////////////////////////////////////////////////////////////

// zip_with (maybe)
//   function: combines two maybe values via a binary function.
// Returns just(f(a, b)) if both are present, nothing otherwise.
template<typename _A,
         typename _B,
         typename _Function>
D_NODISCARD
auto zip_with
(
    const maybe<_A>& _ma,
    const maybe<_B>& _mb,
    _Function        _function
)
-> maybe<typename std::decay<decltype(
    _function(std::declval<const _A&>(),
              std::declval<const _B&>()))>::type>
{
    using result_t = typename std::decay<decltype(
        _function(std::declval<const _A&>(),
                  std::declval<const _B&>()))>::type;

    if (_ma.has_value() && _mb.has_value())
    {
        return maybe<result_t>(_function(_ma.value(), _mb.value()));
    }

    return maybe<result_t>{};
}


// flatten (maybe)
//   function: collapses maybe<maybe<T>> to maybe<T>. Equivalent
// to monad_join for maybe.
template<typename _Type>
D_NODISCARD
maybe<_Type>
flatten
(
    const maybe<maybe<_Type>>& _outer
)
{
    if (_outer.has_value())
    {
        return _outer.value();
    }

    return maybe<_Type>{};
}


// collect (container of maybe)
//   function: turns a container of maybe<T> into maybe<container<T>>.
// Returns just(vector) if every element is just, otherwise nothing.
// Equivalent to Haskell's sequence for the maybe monad.
template<typename _Container>
D_NODISCARD auto
collect(
    const _Container& _container
)
-> maybe<std::vector<typename _Container::value_type::value_type>>
{
    using inner_t = typename _Container::value_type::value_type;

    std::vector<inner_t> result;

    for (const auto& element : _container)
    {
        if (!element.has_value())
        {
            return maybe<std::vector<inner_t>>{};
        }

        result.push_back(element.value());
    }

    return maybe<std::vector<inner_t>>(std::move(result));
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_MAYBE_