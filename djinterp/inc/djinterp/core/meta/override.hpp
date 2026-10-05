/*******************************************************************************
* djinterp [core]                                                   override.hpp
*
*   Foundational, container-agnostic override-policy module.  An "override
* policy" is the abstract notion of "given a BASE element and a DELTA
* element keyed at the same position, what survives in the result?".
* This module defines:
*
*     1. The `dropped` sentinel a policy returns to mean "produce nothing".
*     2. The `OverridePolicy` concept (the policy shape contract).
*     3. Atomic primitives: keep_base, keep_delta, drop_extras,
*        strict_subset, drop_unmatched_base.
*     4. Higher-order combinators: with_on_both, with_on_base_only,
*        with_on_delta_only - for composing new policies from old.
*
*   This module makes NO assumptions about the elements it operates on or
* the container that hosts them.  It is the greatest-common-subset
* foundation; downstream modules (option_override.hpp, env_override.hpp,
* attr_override.hpp, ...) provide the engine that walks their own
* container shape and the element-aware policies that look inside.
*
*   Policy contract:
*     A policy is any struct exposing three nested template aliases:
*
*       template<typename B> using on_base_only  = ...;
*       template<typename D> using on_delta_only = ...;
*       template<typename B, typename D>
*       using on_both = ...;
*
*   Each alias yields either an element-shaped result OR the `dropped`
* sentinel (meaning "filter this position out of the result").  Strict
* policies are allowed to make on_delta_only ill-formed for unwanted
* delta types - the engine is expected to detect on_delta_only via SFINAE
* and to invoke it lazily (only when a delta-only key actually appears),
* so concept probes do not trigger strict failures.
*
*
* path:      /inc/djinterp/core/meta/override.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.25
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    dropped sentinel
      ----------------

II.   OverridePolicy concept
      ----------------------

III.  atomic primitives
      -----------------

IV.   combinators
      -----------

V.    misc helpers (identity_t, always_drop, always_keep_left, ...)
      -------------------------------------------------------------
*/

#ifndef DJINTERP_META_OVERRIDE_HPP
#define DJINTERP_META_OVERRIDE_HPP 1


// THE MODULE FLOOR is C++14, and below it this header is EMPTY rather than
// an error (the brief's rule 5: a facility is absent from a level it
// cannot express). env is included first, as the floor reads it; the
// standard header below is C++11 and the body needs C++14.
//
//   C++14, for is_dropped_v below -- a variable template. It previously read
// `inline constexpr`, which would have made this a C++17 header by accident:
// the inline was redundant on a variable template (whose instantiations
// already merge under vague linkage) and bought a two-tier floor increase for
// nothing. This module does not include option.hpp and so does not inherit
// that subframework's C++17 floor.
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP14_OR_HIGHER
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./type_utility.hpp"  // void_t, clean_t


NS_DJINTERP

// ===========================================================================
// I.   dropped sentinel
// ===========================================================================

// dropped
//   type: returned by a policy hook to signal "this position does not
// appear in the result".  The engine that consumes a policy is
// responsible for filtering dropped values out of its accumulator.
struct dropped
{};

// is_dropped
//   trait: detects the dropped sentinel.
template<typename Type>
struct is_dropped : std::is_same<Type, dropped>
{};

template<typename Type>
D_CONSTEXPR bool is_dropped_v = is_dropped<Type>::value;


// ===========================================================================
// II.  OverridePolicy concept
// ===========================================================================

//   THE POLICY CONTRACT, in both faces.
//
//   Section I's traits and section III's primitives need nothing above C++14.
// The contract below was previously expressed ONLY as a bare C++20 concept,
// with no trait behind it -- the one concept in this framework that is not the
// PascalCase face of a trait (is_passthrough / Passthrough, is_functor /
// Functor, is_callable / Callable, is_predicate / Predicate). That asymmetry
// is why it could not be gated: below C++20 there was simply nothing left to
// check against, so every consumer inherited a C++20 floor from a contract
// that is expressible at C++11.
//
//   So the trait comes first and the concept is its face, per the convention.

NS_INTERNAL

    // override_policy_helper
    //   helper: SFINAE probe for the two nested template aliases a policy
    // must expose. The probe types (int, int / int) are deliberate stand-ins --
    // element shape is the engine's concern, not the policy contract's.
    template<typename Policy,
             typename Enable = void>
    struct override_policy_helper : std::false_type
    {};

    template<typename Policy>
    struct override_policy_helper<
        Policy,
        void_t<typename Policy::template on_base_only<int>,
               typename Policy::template on_both<int, int>>>
        : std::true_type
    {};

NS_END  // internal


// is_override_policy
//   trait: true iff Policy exposes nested template aliases on_base_only<B>
// and on_both<B, D>. on_delta_only<D> is queried by the engine via SFINAE
// (a missing alias is treated as "drop"), so it is deliberately not part of
// the probe.
//   Available at every tier this header supports, which is what lets the
// constraint below be gated rather than made a floor.
template<typename Policy>
struct is_override_policy
    : internal::override_policy_helper<clean_t<Policy>>
{};


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // OverridePolicy
    //   concept: the C++20 face of is_override_policy. Consumers spell it
    // through D_CONCEPT_PARAM so the constraint is checked here and elides to
    // `typename` below 20 -- additive per D1, since no overload in this
    // framework is distinguished by it.
    template<typename Policy>
    concept OverridePolicy = is_override_policy<Policy>::value;

#endif


// ===========================================================================
// III. atomic primitives
// ===========================================================================

// keep_base
//   primitive: on_both = base; on_base_only = base; on_delta_only = drop.
// Base wins everywhere; delta keys not in base are silently discarded.
struct keep_base
{
    template<typename B>
    using on_base_only = B;

    template<typename D>
    using on_delta_only = dropped;

    template<typename B, typename D>
    using on_both = B;
};

// keep_delta
//   primitive: on_both = delta; on_base_only = base; on_delta_only = delta.
// Standard "delta wins" - the usual override semantic, extensions allowed.
struct keep_delta
{
    template<typename B>
    using on_base_only = B;

    template<typename D>
    using on_delta_only = D;

    template<typename B, typename D>
    using on_both = D;
};

// drop_extras
//   primitive: on_both = delta; on_base_only = base; on_delta_only = drop.
// Delta wins on overlap, but delta CANNOT introduce new keys (they're
// silently filtered out by the engine).
struct drop_extras
{
    template<typename B>
    using on_base_only = B;

    template<typename D>
    using on_delta_only = dropped;

    template<typename B, typename D>
    using on_both = D;
};

// strict_subset
//   primitive: like drop_extras but a delta-only key produces a hard
// compile error.  The static_assert lives in a nested struct whose
// instantiation is gated by the engine's lazy lookup, so the concept
// probe does not fire it.
struct strict_subset
{
    template<typename B>
    using on_base_only = B;

    template<typename B, typename D>
    using on_both = D;

    // assert_extension
    //   helper: dependent static_assert that fires only when the
    // engine actually asks for on_delta_only<D>.  sizeof(D) == 0
    // is the standard "depend on the template parameter so the
    // assert isn't eager" trick.
    template<typename D>
    struct assert_extension
    {
        static_assert(sizeof(D) == 0,
            "strict_subset: delta carries a key that does not exist "
            "in base, but the active policy forbids extension.  "
            "Either remove the delta-only entry or switch to a "
            "non-strict policy (keep_delta, drop_extras).");
        using type = dropped;
    };

    template<typename D>
    using on_delta_only = typename assert_extension<D>::type;
};

// drop_unmatched_base
//   primitive: only keys present in delta survive.  A "filter to delta"
// operation - the result is value-equal to delta on those keys, but
// retains delta's full option shape.
struct drop_unmatched_base
{
    template<typename B>
    using on_base_only = dropped;

    template<typename D>
    using on_delta_only = D;

    template<typename B, typename D>
    using on_both = D;
};


// ===========================================================================
// IV.  combinators
// ===========================================================================

// with_on_both
//   combinator: takes a base policy and a binary metafunction F,
// and replaces on_both with `typename F<B, D>::type`.  on_base_only
// and on_delta_only are inherited unchanged.  Use for custom merges.
//
//   F is expected to be a struct template with a nested ::type.
// Pass alias templates indirectly via a trampoline if needed.
template<typename                           Base,
         template<typename, typename> class F>
struct with_on_both : Base
{
    template<typename B, typename D>
    using on_both = typename F<B, D>::type;
};

// with_on_base_only
//   combinator: replaces on_base_only via a unary metafunction F.
template<typename                 Base,
         template<typename> class F>
struct with_on_base_only : Base
{
    template<typename B>
    using on_base_only = typename F<B>::type;
};

// with_on_delta_only
//   combinator: replaces on_delta_only via a unary metafunction F.
// Convenient for converting an extension-allowing policy into a
// stricter one, or vice versa.
template<typename                 Base,
         template<typename> class F>
struct with_on_delta_only : Base
{
    template<typename D>
    using on_delta_only = typename F<D>::type;
};


// ===========================================================================
// V.   misc helpers
// ===========================================================================

// identity_t
//   metafn: yields its argument unchanged.  Useful as a no-op slot
// for combinators that demand a unary metafunction.
template<typename Type>
struct identity_t
{
    using type = Type;
};

// always_drop
//   metafn: yields dropped regardless of argument.
template<typename>
struct always_drop
{
    using type = dropped;
};

// always_keep_left
//   metafn: binary metafn that always yields its first argument.
template<typename L, typename>
struct always_keep_left
{
    using type = L;
};

// always_keep_right
//   metafn: binary metafn that always yields its second argument.
template<typename, typename R>
struct always_keep_right
{
    using type = R;
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_META_OVERRIDE_HPP
