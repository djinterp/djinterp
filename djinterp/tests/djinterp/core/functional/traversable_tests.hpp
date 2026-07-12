/******************************************************************************
* djinterp [test]                                       traversable_tests.hpp
*
*   Unit-test declarations for core/functional/traversable.hpp -- the capstone of
* the functional layer: walk a structure left-to-right, run an applicative effect
* at each element, and collect the results back into the same shape.  One
* declaration group per section of the module under test:
*
*     traversable_tests_protocol.cpp    -- I.  traversable_traits, is_traversable
*     traversable_tests_structural.cpp  -- 0.  traversable_value_type, the
*                                                internal helpers, the concept
*     traversable_tests_vector.cpp      --     the std::vector instance (the one
*                                                concrete instance shipped here)
*     traversable_tests_operations.cpp  -- II. traverse and sequence
*
*   FIXTURES.  Traversal needs two things that must not be confused: a STRUCTURE
* T to walk, and an EFFECT F to thread.  The suite supplies both, and keeps them
* separable so the machinery can be checked from either side.
*
*     tmaybe<T>   is BOTH.  As an EFFECT it is an applicative that can fail (via
*                 the monad bridge), giving the all-or-nothing semantics that make
*                 a failed traversal observable.  As a STRUCTURE it is a second
*                 traversable instance -- zero-or-one element -- so the generic
*                 traverse can be shown to dispatch through traversable_traits
*                 rather than being wired to std::vector.
*
*     tbox<T>     is an EFFECT ONLY: an identity monad that always succeeds.  It
*                 is a Functor and an Applicative but deliberately NOT a
*                 Traversable, which makes it the sharpest negative in the suite:
*                 traversability is a SEPARATE obligation, not a consequence of
*                 the other two.
*
*   IMPORTANT.  tmaybe stores its payload in a std::optional rather than as a
* bare member, and its empty state constructs no payload at all.  That is not
* incidental: lift_a2 (which the vector instance uses) threads an internal
* applicative_a2_binder THROUGH the effect, and that binder has no default
* constructor.  An effect monad whose empty state requires a T{} therefore will
* not compile against the vector traversable -- a real constraint on effects, and
* one worth knowing before writing maybe.hpp's instance.
*
*   logging_fn records every call it receives.  It proves two claims at once: that
* the walk is LEFT-TO-RIGHT, and -- traversing an EMPTY structure -- that f is
* never invoked at all, even though F is still recovered correctly from the TYPE
* of f's result.
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/functional/traversable_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    FIXTURES  (two effects, two traversable structures, negatives, probes)
I.    TRAVERSABLE PROTOCOL   (traversable_traits, is_traversable)
0'.   STRUCTURAL TRAITS      (traversable_value_type, helpers, the concept)
      THE std::vector INSTANCE
II.   GENERIC OPERATIONS     (traverse, sequence)
*/


#ifndef DJINTERP_TEST_TRAVERSABLE_TESTS_
#define DJINTERP_TEST_TRAVERSABLE_TESTS_ 1

// std
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp (module under test)
#include "../../core/functional/traversable.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    FIXTURES                                              ///
///////////////////////////////////////////////////////////////////////////////

// -- EFFECT #1 / STRUCTURE #2:  tmaybe<T> --------------------------------

// tmaybe
//   fixture: a maybe-shaped monad -- hence an applicative and a functor, via the
// bridges. It serves as the failing EFFECT (all-or-nothing) and, through its own
// traversable_traits below, as a second traversable STRUCTURE.
//
//   The payload lives in a std::optional and the empty state constructs NO
// payload. That is load-bearing: lift_a2 threads a non-default-constructible
// applicative_a2_binder through the effect, so an effect whose empty state needs
// a T{} cannot be used with the vector traversable at all.
template<typename _Type>
struct tmaybe
{
    using value_type = _Type;

    std::optional<_Type> slot;

    D_CONSTEXPR bool         has() const { return slot.has_value(); }
    D_CONSTEXPR const _Type& get() const { return *slot; }
};

template<typename _Type>
D_CONSTEXPR tmaybe<_Type>
just(
    _Type _x
)
{
    return tmaybe<_Type>{ std::optional<_Type>(_x) };
}

template<typename _Type>
D_CONSTEXPR tmaybe<_Type>
nothing()
{
    return tmaybe<_Type>{ std::optional<_Type>() };
}


// -- EFFECT #2:  tbox<T>  (a Functor + Applicative, but NOT a Traversable) -

// tbox
//   fixture: the identity monad -- always holds a value, never fails. Used as a
// second, non-failing EFFECT, and as the suite's sharpest negative: it is a
// Functor and an Applicative, yet NOT a Traversable, because traversability is a
// separate obligation that nothing else implies.
template<typename _Type>
struct tbox
{
    using value_type = _Type;

    _Type value;
};

template<typename _Type>
D_CONSTEXPR tbox<_Type>
boxed(
    _Type _x
)
{
    return tbox<_Type>{ _x };
}


// -- the effect-producing functions f : A -> F<B> -------------------------

// pos_or_nothing : int -> tmaybe<int>   (fails on a non-positive element)
struct pos_or_nothing
{
    D_CONSTEXPR tmaybe<int> operator()(int _x) const
    {
        return _x > 0 ? just(_x * 2) : nothing<int>();
    }
};

// to_box : int -> tbox<int>             (never fails)
struct to_box
{
    D_CONSTEXPR tbox<int> operator()(int _x) const { return boxed(_x * 10); }
};

// to_box_str : int -> tbox<std::string>  (a type change, B != A)
struct to_box_str
{
    tbox<std::string> operator()(int _x) const
    {
        return boxed(std::string(static_cast<std::size_t>(_x), 'x'));
    }
};

// to_maybe_str : int -> tmaybe<std::string>
struct to_maybe_str
{
    tmaybe<std::string> operator()(int _x) const
    {
        return just(std::string(static_cast<std::size_t>(_x), 'y'));
    }
};

// logging_fn
//   fixture function: records every element it is called with, so the walk order
// is observable -- and so is the fact that an EMPTY structure never calls it.
struct logging_fn
{
    std::vector<int>* log;

    tbox<int> operator()(int _x) const
    {
        log->push_back(_x);
        return boxed(_x);
    }
};


// -- negatives ------------------------------------------------------------

// not_traversable: no traversable_traits specialization at all.
struct not_traversable
{
};

// no_marker: a traversable_traits WITH a traverse but WITHOUT the
// is_specialized marker -- must read as not-a-traversable.
struct no_marker
{
};


// -- probes ---------------------------------------------------------------

// is_complete
//   probe: true when _Type is a COMPLETE type. The primary traversable_traits is
// declared but left UNDEFINED, so this reads that contract SFINAE-safely.
template<typename _Type,
         typename = void>
struct is_complete : std::false_type
{
};

template<typename _Type>
struct is_complete<_Type, decltype(void(sizeof(_Type)))> : std::true_type
{
};

// has_type
//   probe: true when _Type exposes a nested ::type. Applied to the INTERNAL
// value-type helper, whose primary soft-fails by having none.
template<typename _Type,
         typename = void>
struct has_type : std::false_type
{
};

template<typename _Type>
struct has_type<_Type, void_t<typename _Type::type> > : std::true_type
{
};

// vt_helper: shorthand for the internal value-type helper under test.
template<typename _Traversable>
using vt_helper = internal::traversable_value_type_helper<void, _Traversable>;


///////////////////////////////////////////////////////////////////////////////
///             CONCEPT-FACING HELPERS  (C++20)                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// which_traversable: a constrained overload + an unconstrained fallback, proving
// the concept GATES resolution rather than merely evaluating to a bool.
template<typename _Type>
    requires Traversable<_Type>
int which_traversable(const _Type&) { return 1; }

template<typename _Type>
int which_traversable(const _Type&) { return 0; }

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///             I.    TRAVERSABLE PROTOCOL                                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_is_traversable_positive();
bool tests_is_traversable_negative();
bool tests_is_traversable_is_a_separate_obligation();
bool tests_is_traversable_decay();
bool tests_is_traversable_requires_marker();
bool tests_traversable_traits_primary_is_undefined();
bool tests_traversable_traits_surface();
bool tests_is_traversable_v_agrees();


///////////////////////////////////////////////////////////////////////////////
///             0'.   STRUCTURAL TRAITS & CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

bool tests_traversable_value_type();
bool tests_traversable_value_type_t_alias();
bool tests_traversable_value_type_helper_sfinae();
bool tests_traversable_value_type_mirrors_siblings();
bool tests_identity_helper();
bool tests_append_helper();
bool tests_append_helper_threads_by_value();
bool tests_traversable_concept();
bool tests_traversable_concept_gating();


///////////////////////////////////////////////////////////////////////////////
///             THE std::vector INSTANCE                                    ///
///////////////////////////////////////////////////////////////////////////////

bool tests_vector_traverse_all_succeed();
bool tests_vector_traverse_one_failure_sinks_it();
bool tests_vector_traverse_empty_uses_pure();
bool tests_vector_traverse_is_left_to_right();
bool tests_vector_traverse_single_element();
bool tests_vector_traverse_changes_inner_type();
bool tests_vector_traverse_result_type();
bool tests_vector_traverse_second_effect();
bool tests_vector_traverse_source_untouched();
bool tests_vector_is_the_foldable_companion();


///////////////////////////////////////////////////////////////////////////////
///             II.   GENERIC OPERATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

bool tests_traverse_delegates_to_the_instance();
bool tests_traverse_over_the_maybe_structure();
bool tests_traverse_empty_recovers_the_effect_from_the_type();
bool tests_traverse_hoists_the_effect();
bool tests_traverse_result_type_is_deduced();
bool tests_traverse_forwarding();
bool tests_traverse_constexpr();
bool tests_sequence_is_traverse_with_identity();
bool tests_sequence_inverts_the_nesting();
bool tests_sequence_is_all_or_nothing();
bool tests_sequence_empty();
bool tests_law_traverse_is_sequence_after_map();


NS_END  // testing


///////////////////////////////////////////////////////////////////////////////
///             FIXTURE INSTANCES  (djinterp scope)                         ///
///////////////////////////////////////////////////////////////////////////////

// monad_traits<tmaybe<T>>  -- makes tmaybe a monad, hence (via the bridges) an
// applicative and a functor. Its empty state constructs NO payload.
template<typename _Type>
struct monad_traits<testing::tmaybe<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = testing::tmaybe<_To>;

    static D_CONSTEXPR testing::tmaybe<_Type>
    unit(
        _Type _x
    )
    {
        return testing::just(_x);
    }

    template<typename _Function>
    static D_CONSTEXPR auto
    bind(
        const testing::tmaybe<_Type>& _m,
        _Function                     _f
    )
    -> decltype(_f(_m.get()))
    {
        using result_t = decltype(_f(_m.get()));

        // the empty case builds no payload -- the inner type need not be
        // default-constructible, and lift_a2's binder is not.
        return _m.has() ? _f(_m.get()) : result_t{};
    }
};

// monad_traits<tbox<T>>  -- the identity monad: an effect that always succeeds.
template<typename _Type>
struct monad_traits<testing::tbox<_Type> >
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _To>
    using rebind = testing::tbox<_To>;

    static D_CONSTEXPR testing::tbox<_Type>
    unit(
        _Type _x
    )
    {
        return testing::boxed(_x);
    }

    template<typename _Function>
    static D_CONSTEXPR auto
    bind(
        const testing::tbox<_Type>& _m,
        _Function                   _f
    )
    -> decltype(_f(_m.value))
    {
        return _f(_m.value);
    }
};

// traversable_traits<tmaybe<T>>  -- a SECOND traversable structure, over its
// zero-or-one element. The shape is preserved: T<A> -> F<T<B>>.
//
//   The empty case never calls f, so the effect F cannot be deduced from a
// value; it is recovered from the TYPE of f's result and injected with pure.
template<typename _Type>
struct traversable_traits<testing::tmaybe<_Type>, void>
{
    using is_specialized = std::true_type;
    using value_type     = _Type;

    template<typename _Function>
    static
    D_CONSTEXPR
    typename monad_rebind<
        decltype(std::declval<_Function&>()(std::declval<const _Type&>())),
        testing::tmaybe<applicative_value_type_t<decltype(
            std::declval<_Function&>()(std::declval<const _Type&>()))> > >::type
    traverse(
        const testing::tmaybe<_Type>& _ta,
        _Function                     _function
    )
    {
        using effect_t = decltype(
            _function(std::declval<const _Type&>()));            // F<B>
        using inner_t  = applicative_value_type_t<effect_t>;     // B
        using shape_t  = testing::tmaybe<inner_t>;               // T<B>
        using result_t = typename monad_rebind<effect_t, shape_t>::type;

        if (!_ta.has())
        {
            // f is NOT called; F comes from its declared result type.
            return ::djinterp::pure<result_t>(testing::nothing<inner_t>());
        }

        return ::djinterp::functor_map(
            _function(_ta.get()),
            [](inner_t _b){ return testing::just(_b); });
    }
};

// traversable_traits<no_marker>  -- a traverse, but NO is_specialized marker, so
// detection must refuse it.
template<>
struct traversable_traits<testing::no_marker, void>
{
    using value_type = int;

    template<typename _Function>
    static int traverse(const testing::no_marker&, _Function) { return 0; }
};


NS_END  // djinterp


#endif  // DJINTERP_TEST_TRAVERSABLE_TESTS_
