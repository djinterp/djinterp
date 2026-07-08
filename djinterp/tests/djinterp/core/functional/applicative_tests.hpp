/******************************************************************************
* djinterp [test]                                        applicative_tests.hpp
*
*   Declarations and shared fixtures for the applicative.hpp unit suite.  The
* individual tests_* predicates and their block-providers are defined per
* translation unit (one .cpp per like-group semantic section of the header);
* this head carries only what those files and the runner share.
*
*   applicative.hpp ships one concrete instance of its own -- the monad bridge
* (every is_monad<F> is automatically an applicative, pure via monad_unit, ap
* via monad_bind + monad_map) -- and derives lift_a2 generically.  A suite must
* therefore supply BOTH a monad (to drive the bridge, the path maybe / result
* take) and a directly-specialized non-monad applicative (the view / producer
* path), so the generic ops are proven not to secretly assume the bridge.  Two
* fixtures are provided:
*
*     - box<T> : a maybe-like MONAD (specializes monad_traits: unit / bind,
*       value_type, rebind).  Via the monad bridges it is automatically a
*       functor and an applicative, so pure / ap / lift_a2 all run through the
*       bridge over it, and its empty state gives ap / lift_a2 their maybe-style
*       short-circuit.  Storage is a 0-or-1-element vector so the empty state
*       never default-constructs the value type -- essential, because lift_a2's
*       internal applicative_a2_binder is not default-constructible.
*     - ident<T> : a total Identity applicative specialized DIRECTLY
*       (functor_traits + applicative_traits, NOT monad_traits), so is_monad is
*       false and the bridge does not apply.  It exercises pure / ap / lift_a2
*       over a non-monad applicative.
*
*   has_app_value_type<T> is the SFINAE-safe detector (probing the undefined
* primary applicative_traits, as is_applicative does); applicative_value_type<T>
* declares its member unconditionally, so a direct probe of it would be a hard
* error for a non-applicative.
*
*   Every predicate lives flat in djinterp::testing and returns true iff all of
* its checks pass.  Each section .cpp keeps its predicates file-local (internal
* linkage) and exposes exactly one block-provider, declared below.
*
*   SECTIONS (mirroring applicative.hpp's table of contents):
*     0 + I.  protocol & traits ......... applicative_tests_protocol.cpp
*     0.      is_applicable / concept ... applicative_tests_applicable.cpp
*     II.1    pure ..................... applicative_tests_pure.cpp
*     II.2    ap ....................... applicative_tests_ap.cpp
*     II.3    lift_a2 .................. applicative_tests_lift_a2.cpp
*
*
* path:      /tests/djinterp/core/functional/applicative_tests.hpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_APPLICATIVE_TESTS_
#define DJINTERP_FUNCTIONAL_APPLICATIVE_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp -- the header under test (which pulls in monad.hpp + functor.hpp),
//   plus the DTest authoring + runner surface.  NOTE: these two include paths
//   are rooted at the djinterp include directory (e.g. -I.../inc); adjust them
//   to match your build tree.
#include "djinterp/core/functional/applicative.hpp"
#include "djinterp/test/test_defaults.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   MONAD FIXTURE:  box<T>  (maybe-like)                 ///
///////////////////////////////////////////////////////////////////////////////

// box
//   fixture: a maybe-like monad -- engaged (holds a value) or empty.  Storage
// is a 0-or-1-element std::vector so the empty state never default-constructs T
// (lift_a2's internal binder is not default-constructible, and its empty branch
// materializes an empty box over that binder type).  Via the monad bridges box
// is a functor and an applicative for free; its empty state is what makes ap /
// lift_a2 short-circuit.  Equality is by engagement and, when engaged, by value.
template<typename _Type>
struct box
{
    std::vector<_Type> slot;

    // box
    //   constructor: the empty value (nothing).
    box()
        : slot()
    {
    }

    // box
    //   constructor: an engaged value carrying _v (just).
    box(
        const _Type& _v
    )
        : slot(1, _v)
    {
    }

    // has
    //   true iff engaged.
    bool has() const
    {
        return (!slot.empty());
    }

    // get
    //   the carried value (defined only when engaged).
    const _Type& get() const
    {
        return slot[0];
    }

    // operator==
    //   equal iff both empty, or both engaged with equal values.
    friend bool
    operator==(
        const box& _a,
        const box& _b
    )
    {
        if (_a.has() != _b.has())
        {
            return false;
        }

        return ( (!_a.has()) || (_a.get() == _b.get()) );
    }

    // operator!=
    //   the negation of operator==.
    friend bool
    operator!=(
        const box& _a,
        const box& _b
    )
    {
        return (!(_a == _b));
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  DIRECT APPLICATIVE FIXTURE:  ident<T>               ///
///////////////////////////////////////////////////////////////////////////////

// ident
//   fixture: a total Identity applicative, specialized directly (functor +
// applicative, NOT a monad), modelling the view / producer path that carries
// its own pure / ap without the monad bridge.  It always holds exactly one
// value; there is no empty state and thus no short-circuit.
template<typename _Type>
struct ident
{
    _Type value;

    // ident
    //   constructor: wraps a value.
    ident(
        const _Type& _v
    )
        : value(_v)
    {
    }

    // operator==
    //   equal iff the carried values are equal.
    friend bool
    operator==(
        const ident& _a,
        const ident& _b
    )
    {
        return (_a.value == _b.value);
    }

    // operator!=
    //   the negation of operator==.
    friend bool
    operator!=(
        const ident& _a,
        const ident& _b
    )
    {
        return (!(_a == _b));
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. WRAPPED-FUNCTION FIXTURES  (named callables)         ///
///////////////////////////////////////////////////////////////////////////////
//   ap needs the wrapped function to have a nameable type (box<times2>, ...),
// so these are named function objects rather than lambdas.

// times2
//   callable: doubles an int (int -> int).
struct times2
{
    int operator()(int _x) const
    {
        return (_x * 2);
    }
};

// square
//   callable: squares an int (int -> int).
struct square
{
    int operator()(int _x) const
    {
        return (_x * _x);
    }
};

// to_text
//   callable: renders an int as its decimal string (int -> std::string),
// used to prove ap / the wrapped function may change the value type.
struct to_text
{
    std::string operator()(int _x) const
    {
        return std::to_string(_x);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  COMPILE-TIME DETECTOR:  has_app_value_type<T>        ///
///////////////////////////////////////////////////////////////////////////////

// has_app_value_type
//   detector: true iff applicative_traits<decay<_Type>>::value_type is
// well-formed -- i.e. iff _Type carries the Applicative protocol's inner value
// type (whether via the monad bridge or a direct specialization).  SFINAE-safe:
// the primary applicative_traits is declared but undefined, so for a
// non-applicative the member access soft-fails in the immediate context (false)
// rather than a hard error.  Deliberately NOT written in terms of
// applicative_value_type<T>::type, which declares `type` unconditionally.
template<typename _Type,
         typename _Enable = void>
struct has_app_value_type
    : std::false_type
{
};

// has_app_value_type (well-formed specialization)
//   detector: the arm chosen when the trait's value_type exists.
template<typename _Type>
struct has_app_value_type<
    _Type,
    ::djinterp::void_t<
        typename ::djinterp::applicative_traits<
            typename std::decay<_Type>::type >::value_type> >
    : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///                V.   SECTION BLOCK-PROVIDERS  (the runner's surface)      ///
///////////////////////////////////////////////////////////////////////////////

::djinterp::test::block_spec applicative_protocol_block();
::djinterp::test::block_spec applicative_applicable_block();
::djinterp::test::block_spec applicative_pure_block();
::djinterp::test::block_spec applicative_ap_block();
::djinterp::test::block_spec applicative_lift_a2_block();


NS_END  // testing
NS_END  // djinterp


///////////////////////////////////////////////////////////////////////////////
///                VI.  FIXTURE PROTOCOL SPECIALIZATIONS                     ///
///////////////////////////////////////////////////////////////////////////////
//   box registers monad_traits (single-parameter primary) and rides the monad
// bridges for functor / applicative.  ident registers functor_traits and
// applicative_traits directly (two-parameter primaries, explicit <F, void>).

NS_DJINTERP

// monad_traits< ::djinterp::testing::box<_Type> >
//   instance: a maybe-like monad.  unit is just; bind threads an engaged value
// through the Kleisli arrow and propagates empty otherwise (the empty branch
// returns a default-constructed result box -- an empty vector -- so it never
// default-constructs the result's value type).
template<typename _Type>
struct monad_traits< ::djinterp::testing::box<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = ::djinterp::testing::box<_To>;

    // unit
    //   lift a bare value into an engaged box (just).
    static
    ::djinterp::testing::box<_Type> unit(
        _Type _value
    )
    {
        return ::djinterp::testing::box<_Type>(std::move(_value));
    }

    // bind
    //   thread an engaged value through _function : T -> box<U>; propagate
    // empty (an empty box<U>) when this box is empty.
    template<typename _Function>
    static
    auto bind(
        const ::djinterp::testing::box<_Type>& _m,
        _Function                              _function
    )
    -> decltype(_function(_m.get()))
    {
        using result_t = decltype(_function(_m.get()));

        return _m.has() ? _function(_m.get()) : result_t();
    }
};


// functor_traits< ::djinterp::testing::ident<_Type> >
//   instance: the Identity functor's direct map (no monad bridge).
template<typename _Type>
struct functor_traits< ::djinterp::testing::ident<_Type>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // map
    //   apply _function to the carried value, yielding ident<U>.
    template<typename _Function>
    static
    auto map(
        const ::djinterp::testing::ident<_Type>& _fa,
        _Function                                _function
    )
    -> ::djinterp::testing::ident<
           typename std::decay<decltype(_function(_fa.value))>::type>
    {
        using mapped_t =
            typename std::decay<decltype(_function(_fa.value))>::type;

        return ::djinterp::testing::ident<mapped_t>(_function(_fa.value));
    }
};


// applicative_traits< ::djinterp::testing::ident<_Type> >
//   instance: the Identity applicative's direct pure / ap (no monad bridge).
// When used by ap, _Type is the wrapped-function type; when used by pure, it is
// the value type -- each instantiation is self-consistent.
template<typename _Type>
struct applicative_traits< ::djinterp::testing::ident<_Type>, void >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    // pure
    //   wrap a bare value: ident<T>(value).
    template<typename _Value>
    static
    ::djinterp::testing::ident<_Type> pure(
        _Value&& _value
    )
    {
        return ::djinterp::testing::ident<_Type>(std::forward<_Value>(_value));
    }

    // ap
    //   apply the wrapped function to the wrapped argument: ident<U>(f(a)).
    template<typename _ArgApplicative>
    static
    auto ap(
        const ::djinterp::testing::ident<_Type>& _ff,
        const _ArgApplicative&                   _fa
    )
    -> ::djinterp::testing::ident<
           typename std::decay<decltype(_ff.value(_fa.value))>::type>
    {
        using applied_t =
            typename std::decay<decltype(_ff.value(_fa.value))>::type;

        return ::djinterp::testing::ident<applied_t>(_ff.value(_fa.value));
    }
};

NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_APPLICATIVE_TESTS_
