/*******************************************************************************
* djinterp [core]                                                      maybe.hpp
*
* Maybe<T> -- a monadic optional value type (C++).
*   Represents a value that may or may not be present. Equivalent in
* purpose to std::optional<T> (C++17) but available on C++11+, integrated
* with the djinterp monad protocol, and with a richer fluent API for
* functional pipelines.
*
*   maybe<T> is "nothing" or "just(x)". All inspection methods are
* explicit (no implicit bool conversion that would defeat type safety),
* and value access on a "nothing" yields default-constructed Type (use
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
*
* path:      /inc/djinterp/core/functional/maybe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.20
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    MAYBE PRIMITIVE
      ---------------
      1.    maybe<T>                                (storage + interface)
      2.    nothing_t / nothing_v                   (empty-maybe tag)

II.   PREDICATE & STRUCTURAL TRAITS
      -----------------------------
      1.    is_maybe<T>                             (detects maybe<U>)
      2.    is_maybe_predicate<P, T>                (P callable (const T&) -> bool)
      3.    is_maybe_v / is_maybe_predicate_v       (variable-template shorthands)
      4.    maybe_type / maybe_predicate_for        (C++20 concept parallels)

III.  FACTORIES
      ---------
      1.    just(value)
      2.    nothing<T>()
      3.    from_pointer(ptr)
      4.    from_predicate(value, predicate)

IV.   COMBINATOR FACTORIES (pipeline form)
      ------------------------------------
      1.    or_else_with(default)
      2.    filter_with(predicate)
      3.    unwrap_or_with(default)                 (alias for or_else_with)
      4.    expect_with(message)

V.    MONAD TRAITS SPECIALIZATION
      ---------------------------

VI.   FREE-FUNCTION HELPERS
      ---------------------
      1.    zip_with(m1, m2, f)
      2.    flatten(m_of_m)
      3.    collect(container_of_maybe)             -> maybe<container>
*/


#ifndef DJINTERP_FUNCTIONAL_MAYBE_HPP
#define DJINTERP_FUNCTIONAL_MAYBE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

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
#include "../../djinterp.hpp"
#include "../meta/kv_pair.hpp"   // kv_pair (the unfold step's (value, next) pairing)
#include "./monad.hpp"
#include "./foldable.hpp"


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
//   class: holds either a value of type Type or nothing. Storage is
// raw bytes (aligned_storage) with a boolean discriminator; the
// value is constructed in place when needed and destroyed when
// reset or when the maybe is destroyed.
//
//   maybe is value-typed: copies and moves perform deep copy /
// move of the contained value. Comparison operators are provided
// when Type supports them.
template<typename Type>
class maybe
{
public:
    using value_type = Type;

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
    D_CONSTEXPR_CPP14
    maybe(
        const Type& _value
    )
        : m_has_value(true)
    {
        construct_value(_value);
    }

    // constructor (value, move)
    D_CONSTEXPR_CPP14
    maybe(
        Type&& _value
    )
        : m_has_value(true)
    {
        construct_value(std::move(_value));
    }

    // constructor (copy)
    D_CONSTEXPR_CPP14
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
    D_CONSTEXPR_CPP14
    maybe(
        maybe&& _other
    ) noexcept(std::is_nothrow_move_constructible<Type>::value)
        : m_has_value(_other.m_has_value)
    {
        if (m_has_value)
        {
            construct_value(std::move(*_other.pointer()));
        }
    }

    // destructor
    D_CONSTEXPR_CPP20
    ~maybe()
    {
        reset();
    }

    // assignment (copy)
    D_CONSTEXPR_CPP14
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
    D_CONSTEXPR_CPP14
    maybe& operator=(
        maybe&& _other
    ) noexcept(std::is_nothrow_move_assignable<Type>::value &&
               std::is_nothrow_move_constructible<Type>::value)
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
    D_CONSTEXPR_CPP14
    maybe& operator=(
        nothing_t
    ) noexcept
    {
        reset();

        return *this;
    }

    // assignment (value)
    D_CONSTEXPR_CPP14
    maybe& operator=(
        const Type& _value
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

    D_CONSTEXPR_CPP14
    maybe& operator=(
        Type&& _value
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
    D_NODISCARD D_CONSTEXPR bool has_value() const noexcept
    {
        return m_has_value;
    }

    // is_nothing
    //   method: the negation of has_value, for readability in
    // pattern-matching-style code.
    D_NODISCARD D_CONSTEXPR bool is_nothing() const noexcept
    {
        return !m_has_value;
    }

    // value (const)
    //   method: returns a const reference to the contained value.
    // Behavior is undefined when has_value() is false; use
    // value_or, expect, or match for safe access.
    D_NODISCARD
    D_CONSTEXPR
    const Type& value() const&
    {
        return *pointer();
    }

    // value (mutable)
    D_NODISCARD
    D_CONSTEXPR_CPP14
    Type& value() &
    {
        return *pointer();
    }

    // value (rvalue)
    D_NODISCARD
    D_CONSTEXPR_CPP14
    Type&& value() &&
    {
        return std::move(*pointer());
    }

    // value_or
    //   method: returns the contained value if present, otherwise
    // _default. _default is evaluated unconditionally; for
    // expensive defaults, use or_else with a lambda.
    template<typename U>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    Type value_or(
        U&& _default
    ) const&
    {
        if (m_has_value)
        {
            return *pointer();
        }

        return static_cast<Type>(std::forward<U>(_default));
    }

    // expect
    //   method: returns the contained value if present, otherwise
    // throws std::runtime_error with the given message.
    D_NODISCARD
    const Type& expect(
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
    D_CONSTEXPR_CPP14
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
    template<typename... Args>
    D_CONSTEXPR_CPP14
    Type& emplace(
        Args&&... _args
    )
    {
        reset();
        construct_value(std::forward<Args>(_args)...);
        m_has_value = true;

        return *pointer();
    }

    // map
    //   method: if this maybe holds a value, applies _function to
    // it and wraps the result in a new maybe. If empty, returns an
    // empty maybe of the mapped type.
    template<typename Function>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    auto map(
        Function _function
    ) const
    -> maybe<typename std::decay<decltype(
        _function(std::declval<const Type&>()))>::type>
    {
        using result_t = typename std::decay<decltype(
            _function(std::declval<const Type&>()))>::type;

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
    template<typename Function>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    auto and_then(
        Function _function
    ) const
    -> typename std::decay<decltype(
        _function(std::declval<const Type&>()))>::type
    {
        using result_t = typename std::decay<decltype(
            _function(std::declval<const Type&>()))>::type;

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
    template<typename Function>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    maybe or_else(
        Function _function
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
    template<typename Predicate>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    maybe filter(
        Predicate _predicate
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
    template<typename OnJust,
             typename OnNothing>
    D_NODISCARD
    D_CONSTEXPR_CPP14
    auto match(
        OnJust     _on_just,
        OnNothing _on_nothing
    ) const
    -> typename std::decay<decltype(
        _on_just(std::declval<const Type&>()))>::type
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
        Type                 m_value;

        // trivial ctor leaves m_empty active; maybe constructs m_value
        // on demand via construct_value().
        D_CONSTEXPR storage_t() noexcept : m_empty() {}

        // non-trivial members mean the union cannot auto-generate these;
        // maybe drives lifetime explicitly, so they are empty.
        D_CONSTEXPR ~storage_t() {}
    };

    storage_t m_union;

    // construct_value
    //   constructs the inner value in place from forwarded args using
    // std::construct_at (constexpr in C++20).
    template<typename... Args>
    D_CONSTEXPR_CPP14
    void construct_value(
        Args&&... _args
    )
    {
        std::construct_at(std::addressof(m_union.m_value),
                          std::forward<Args>(_args)...);

        return;
    }

    // pointer (const) / (mutable)
    //   typed pointer to the active union value. Behavior is undefined
    // unless m_has_value is true.
    D_CONSTEXPR
    const Type* pointer() const noexcept
    {
        return std::addressof(m_union.m_value);
    }

    D_CONSTEXPR_CPP14
    Type* pointer() noexcept
    {
        return std::addressof(m_union.m_value);
    }

    // destroy_value
    //   destroys the active value via std::destroy_at (constexpr C++20).
    D_CONSTEXPR_CPP14
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
    template<typename... Args>
    void construct_value(
        Args&&... _args
    )
    {
        new (static_cast<void*>(&m_storage))
            Type(std::forward<Args>(_args)...);

        return;
    }

    // pointer (const)
    //   typed pointer into the aligned storage. Behavior is
    // undefined unless m_has_value is true.
    const Type* pointer() const noexcept
    {
        return static_cast<const Type*>(
            static_cast<const void*>(&m_storage));
    }

    // pointer (mutable)
    Type* pointer() noexcept
    {
        return static_cast<Type*>(static_cast<void*>(&m_storage));
    }

    // destroy_value
    //   destroys the active value via an explicit destructor call.
    void destroy_value() noexcept
    {
        pointer()->~Type();

        return;
    }

    typename std::aligned_storage<sizeof(Type), alignof(Type)>::type m_storage;
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    bool m_has_value;
};


///////////////////////////////////////////////////////////////////////////////
//                            EQUALITY OPERATORS                             //
///////////////////////////////////////////////////////////////////////////////

// operator== (maybe vs maybe)
//   true if both empty, or both non-empty with equal values.
template<typename Type>
D_NODISCARD
bool operator==
(
    const maybe<Type>& _a,
    const maybe<Type>& _b
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

template<typename Type>
D_NODISCARD
bool operator!=
(
    const maybe<Type>& _a,
    const maybe<Type>& _b
)
{
    return !(_a == _b);
}

template<typename Type>
D_NODISCARD
D_CONSTEXPR
bool operator==
(
    const maybe<Type>&,
    nothing_t
)
{
    return false;
}

template<typename Type>
D_NODISCARD
D_CONSTEXPR
bool operator==
(
    nothing_t,
    const maybe<Type>& _m
)
{
    return !_m.has_value();
}


///////////////////////////////////////////////////////////////////////////////
///             II.   PREDICATE & STRUCTURAL TRAITS                         ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_maybe_helper
    //   helper: primary is std::false_type; the maybe<Type> partial
    // specialization lifts it to std::true_type. Kept internal so the
    // public is_maybe can decay its argument before matching.
    template<typename Type>
    struct is_maybe_helper
        : std::false_type
    {};

    template<typename Type>
    struct is_maybe_helper<maybe<Type>>
        : std::true_type
    {};


    // is_maybe_predicate_helper
    //   helper: SFINAE-detects whether Pred can be invoked with a
    // const Type& and whether the result is contextually convertible to
    // bool (the exact shape filter / from_predicate require). The
    // static_cast<bool> in the detected expression rejects callables
    // whose result is not bool-convertible (e.g. void-returning).
    template<typename Pred,
             typename Type>
    struct is_maybe_predicate_helper
    {
    private:
        template<typename P,
                 typename U>
        static auto test(int)
            -> decltype(
                static_cast<bool>(
                    std::declval<const P&>()(
                        std::declval<const U&>())),
                std::true_type{});

        template<typename,
                 typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Pred, Type>(0));
    };

NS_END  // internal


// is_maybe
//   trait: true if Type is a maybe<U> specialization, after
// stripping cv-qualifiers and references. False for every other
// type, including unrelated optional-like types.
template<typename Type>
struct is_maybe
    : internal::is_maybe_helper<typename std::decay<Type>::type>::type
{
};


// is_maybe_predicate
//   trait: true if Pred is callable as Pred(const Type&) and the
// result is convertible to bool -- the predicate shape accepted by
// maybe::filter, from_predicate, and filter_with. False when Pred
// is not callable with a const Type&, or its result is not
// bool-convertible.
template<typename Pred,
         typename Type>
struct is_maybe_predicate
    : internal::is_maybe_predicate_helper<Pred, Type>::type
{
};


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_maybe_v
//   variable: shorthand for is_maybe<Type>::value. Available only
// when variable templates are supported (C++14+).
template<typename Type>
static constexpr bool is_maybe_v = is_maybe<Type>::value;

// is_maybe_predicate_v
//   variable: shorthand for is_maybe_predicate<Pred, Type>::value.
template<typename Pred,
         typename Type>
static constexpr bool is_maybe_predicate_v =
    is_maybe_predicate<Pred, Type>::value;
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// maybe_type
//   concept: satisfied by any maybe<U> specialization (cv-ref
// stripped). The C++20 parallel of is_maybe. Named maybe_type to
// avoid clashing with djinterp::predicate (the std concept already
// re-exported in concepts.hpp).
template<typename Type>
concept maybe_type = is_maybe<Type>::value;

// maybe_predicate_for
//   concept: satisfied when Pred is a valid filter predicate over
// Type -- callable as Pred(const Type&) with a bool-convertible result.
// The C++20 parallel of is_maybe_predicate.
template<typename Pred,
         typename Type>
concept maybe_predicate_for = is_maybe_predicate<Pred, Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///             III.  FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

// just
//   function: builds a maybe holding _value. Equivalent to
// maybe<T>(value) but reads more clearly at call sites.
template<typename Type>
D_NODISCARD
D_CONSTEXPR
maybe<typename std::decay<Type>::type>
just
(
    Type&& _value
)
{
    return maybe<typename std::decay<Type>::type>(std::forward<Type>(_value));
}


// nothing
//   function: builds an empty maybe of the given type. The type
// must be supplied explicitly because there is no value from
// which to deduce it.
template<typename Type>
D_NODISCARD
D_CONSTEXPR
maybe<Type>
nothing()
{
    return maybe<Type>{};
}


// from_pointer
//   function: builds a maybe from a raw pointer: nothing if the
// pointer is null, otherwise just(*ptr). The pointed-to value is
// copied; the pointer itself is not stored.
template<typename Type>
D_NODISCARD
maybe<typename std::decay<Type>::type>
from_pointer
(
    const Type* _ptr
)
{
    if (_ptr == nullptr)
    {
        return maybe<typename std::decay<Type>::type>{};
    }

    return maybe<typename std::decay<Type>::type>(*_ptr);
}


// from_predicate
//   function: builds a maybe holding _value if _predicate(_value)
// is true, otherwise nothing. Useful for "validate and wrap" in a
// single expression.
template<typename Type,
         typename Predicate>
D_NODISCARD
maybe<typename std::decay<Type>::type>
from_predicate
(
    Type&&        _value,
    Predicate   _predicate
)
{
    using value_t = typename std::decay<Type>::type;

    if (_predicate(_value))
    {
        return maybe<value_t>(std::forward<Type>(_value));
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
    template<typename Default>
    class or_else_combinator
    {
    public:
        template<typename DFwd>
        D_CONSTEXPR
        explicit or_else_combinator(
            DFwd&& _default
        )
            : m_default(std::forward<DFwd>(_default))
        {}

        template<typename Type>
        D_CONSTEXPR
        Type apply(
            const maybe<Type>& _m
        ) const
        {
            return _m.value_or(m_default);
        }

    private:
        Default m_default;
    };


    // filter_combinator
    //   helper: stores a predicate; gates the LHS maybe through it.
    template<typename Predicate>
    class filter_combinator
    {
    public:
        template<typename PFwd>
        D_CONSTEXPR
        explicit filter_combinator(
            PFwd&& _predicate
        )
            : m_predicate(std::forward<PFwd>(_predicate))
        {}

        template<typename Type>
        D_CONSTEXPR
        maybe<Type> apply(
            const maybe<Type>& _m
        ) const
        {
            return _m.filter(m_predicate);
        }

    private:
        Predicate m_predicate;
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

        template<typename Type>
        Type apply(
            const maybe<Type>& _m
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
template<typename Default>
D_NODISCARD
D_CONSTEXPR
internal::or_else_combinator<typename std::decay<Default>::type>
or_else_with
(
    Default&& _default
)
{
    return internal::or_else_combinator<
        typename std::decay<Default>::type>(
            std::forward<Default>(_default));
}


// unwrap_or_with
//   function: alias for or_else_with, matching Rust-style naming.
template<typename Default>
D_NODISCARD
D_CONSTEXPR
internal::or_else_combinator<typename std::decay<Default>::type>
unwrap_or_with
(
    Default&& _default
)
{
    return or_else_with(std::forward<Default>(_default));
}


// filter_with (maybe)
//   function: builds a combinator that, when piped against a
// maybe, returns it unchanged if its value satisfies _predicate,
// otherwise nothing.
template<typename Predicate>
D_NODISCARD
D_CONSTEXPR
internal::filter_combinator<typename std::decay<Predicate>::type>
filter_with
(
    Predicate&& _predicate
)
{
    return internal::filter_combinator<
        typename std::decay<Predicate>::type>(
            std::forward<Predicate>(_predicate));
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
template<typename Type,
         typename Combinator,
         typename = decltype(
             std::declval<const Combinator&>().apply(
                 std::declval<const maybe<Type>&>()))>
D_CONSTEXPR
auto operator|
(
    const maybe<Type>& _m,
    Combinator&&    _combinator
)
-> decltype(_combinator.apply(_m))
{
    return _combinator.apply(_m);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    MONAD TRAITS SPECIALIZATION                           ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<maybe<Type>>
//   specialization: makes maybe participate in the generic monad
// protocol. Exposes value_type, rebind, unit, and bind.
template<typename Type>
struct monad_traits<maybe<Type>>
{
    using is_specialized = std::true_type;
    using value_type     = Type;

    template<typename U>
    using rebind = maybe<U>;

    // unit
    //   lifts a value into maybe. Equivalent to just().
    static
    D_CONSTEXPR
    maybe<Type>
    unit(
        Type _value
    )
    {
        return maybe<Type>(std::move(_value));
    }

    // bind
    //   monadic bind. Threads the contained value through
    // _function (which must return a maybe of some type).
    //   D_CONSTEXPR so the generic monad_bind / monad_map fold at
    // compile time over carrier-holding maybe under C++20 (runtime on
    // the C++17 floor, where maybe is not a literal type).
    template<typename Function>
    static
    D_CONSTEXPR
    auto bind(
        const maybe<Type>& _m,
        Function         _function
    )
    -> typename std::decay<decltype(
        _function(std::declval<const Type&>()))>::type
    {
        return _m.and_then(_function);
    }
};


// foldable_traits<maybe<Type>>
//   specialization: makes maybe participate in the generic foldable
// protocol. A maybe folds over its zero-or-one carried value: fold_left
// applies the reducer once when just, and is the identity when nothing.
// Keyed on is_maybe so the single instance covers every maybe<T>.
template<typename Maybe>
struct foldable_traits<
    Maybe,
    typename std::enable_if<is_maybe<Maybe>::value>::type>
{
    using is_specialized = std::true_type;
    using value_type     = typename Maybe::value_type;

    // fold_left
    //   threads _init through the (at most one) contained value.
    //   D_CONSTEXPR so the generic folds fold at compile time over a
    // carrier-holding maybe under C++20 (runtime on the C++17 floor, where
    // maybe is not a literal type).
    template<typename Acc,
             typename Function>
    static
    D_CONSTEXPR_CPP14
    Acc fold_left(
        const Maybe& _m,
        Acc           _init,
        Function      _function
    )
    {
        if (_m.has_value())
        {
            return _function(std::move(_init), _m.value());
        }

        return _init;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VI.   FREE-FUNCTION HELPERS                                 ///
///////////////////////////////////////////////////////////////////////////////

// zip_with (maybe)
//   function: combines two maybe values via a binary function.
// Returns just(f(a, b)) if both are present, nothing otherwise.
template<typename A,
         typename B,
         typename Function>
D_NODISCARD
auto zip_with
(
    const maybe<A>& _ma,
    const maybe<B>& _mb,
    Function         _function
)
-> maybe<typename std::decay<decltype(
    _function(std::declval<const A&>(),
              std::declval<const B&>()))>::type>
{
    using result_t = typename std::decay<decltype(
        _function(std::declval<const A&>(),
                  std::declval<const B&>()))>::type;

    if (_ma.has_value() && _mb.has_value())
    {
        return maybe<result_t>(_function(_ma.value(), _mb.value()));
    }

    return maybe<result_t>{};
}


// flatten (maybe)
//   function: collapses maybe<maybe<T>> to maybe<T>. Equivalent
// to monad_join for maybe.
template<typename Type>
D_NODISCARD
maybe<Type>
flatten
(
    const maybe<maybe<Type>>& _outer
)
{
    if (_outer.has_value())
    {
        return _outer.value();
    }

    return maybe<Type>{};
}


// collect (container of maybe)
//   function: turns a container of maybe<T> into maybe<container<T>>.
// Returns just(vector) if every element is just, otherwise nothing.
// Equivalent to Haskell's sequence for the maybe monad.
template<typename Container>
D_NODISCARD auto
collect(
    const Container& _container
)
-> maybe<std::vector<typename Container::value_type::value_type>>
{
    using inner_t = typename Container::value_type::value_type;

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


///////////////////////////////////////////////////////////////////////////////
///             X.    UNFOLD STEP RESULT  (some / none)                     ///
///////////////////////////////////////////////////////////////////////////////
//   The result of one pull from a pull-based source, expressed as the pure
// unfold step  State -> step_result<Value, Next>  (see the UnfoldStep concept
// in structural_traits.hpp and ROADMAP 10.4).  A step either yields a value
// together with the next state (some) or signals exhaustion (none).
//
//   Reuses the framework's existing carriers of optionality and pairing rather
// than introducing a new type: optionality is maybe<>, the (value, next_state)
// pairing is kv_pair<>.  Under C++20 a step is a constant expression - maybe's
// union storage and destructor are both constexpr there - so an unfold can be
// materialized at compile time.  On the C++17 floor maybe has a non-trivial
// destructor and is therefore NOT a literal type, so maybe (and hence some /
// none) are runtime constructs; a C++17 compile-time unfold instead uses the
// empty-type step form built in producer.hpp.  Value/Next are typically carriers
// (val_t / type_t) for a compile-time unfold and ordinary objects at runtime -
// because carriers put both a type and an NTTP in the object domain, a single
// step may even MIX them (e.g. a kv_pair<type_t<T>, val_t<N>> state).
//
//   The driver that runs a step to a fixed point - lazily/infinitely at runtime
// or to a finite materialization at compile time - lives with the source in
// producer.hpp; this header supplies only the step-result shape it pulls on.

// step_result
//   type: the result of one unfold step - an optional (value, next_state)
// pair.  Engaged means "a value plus the next state"; empty means the source
// is exhausted.
template<typename Value,
         typename Next>
using step_result = maybe<kv_pair<Value, Next>>;

// some
//   function: an unfold step that yields _value and advances to _next; builds
// an engaged step_result holding kv_pair(_value, _next).  Constexpr under C++20
// (engaged maybe); a runtime construct on the C++17 floor.
template<typename Value,
         typename Next>
D_NODISCARD
D_CONSTEXPR
step_result<typename std::decay<Value>::type,
            typename std::decay<Next>::type>
some
(
    Value&& _value,
    Next&&  _next
)
{
    return just(make_kv(std::forward<Value>(_value),
                        std::forward<Next>(_next)));
}

// none
//   function: an unfold step that signals exhaustion.  The Value/Next types
// are supplied explicitly (there is no value to deduce them from) so a step's
// two branches share one step_result type.  Constexpr under C++20; a runtime
// construct on the C++17 floor (maybe is not a literal type pre-C++20).
template<typename Value,
         typename Next>
D_NODISCARD
D_CONSTEXPR
step_result<Value, Next>
none()
{
    return nothing<kv_pair<Value, Next>>();
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_MAYBE_HPP
