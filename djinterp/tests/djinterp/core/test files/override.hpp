/******************************************************************************
* djinterp [paradigm]                                              override.hpp
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
*       template<typename _B> using on_base_only  = ...;
*       template<typename _D> using on_delta_only = ...;
*       template<typename _B, typename _D>
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
* path:      /inc/djinterp/core/paradigm/override.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    dropped sentinel
II.   OverridePolicy concept
III.  atomic primitives
IV.   combinators
V.    misc helpers (identity_t, always_drop, always_keep_left, ...)
*/

#ifndef DJINTERP_PARADIGM_OVERRIDE_
#define DJINTERP_PARADIGM_OVERRIDE_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


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
template<typename _Type>
struct is_dropped : std::is_same<_Type, dropped>
{};

template<typename _Type>
inline constexpr bool is_dropped_v = is_dropped<_Type>::value;


// ===========================================================================
// II.  OverridePolicy concept
// ===========================================================================

// OverridePolicy
//   concept: a policy is any type exposing nested template aliases
// on_base_only<_B> and on_both<_B, _D>.  on_delta_only<_D> is
// queried by the engine via SFINAE (a missing or strict definition
// is fine - the engine treats a missing alias as "drop" and invokes
// a strict alias only when an actual delta-only key occurs).
//
//   The probe types (int, int / int) are deliberate stand-ins -
// element shape is the engine's concern, not the policy contract's.
template<typename _Policy>
concept OverridePolicy = requires
{
    typename _Policy::template on_base_only<int>;
    typename _Policy::template on_both<int, int>;
};


// ===========================================================================
// III. atomic primitives
// ===========================================================================

// keep_base
//   primitive: on_both = base; on_base_only = base; on_delta_only = drop.
// Base wins everywhere; delta keys not in base are silently discarded.
struct keep_base
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _D>
    using on_delta_only = dropped;

    template<typename _B, typename _D>
    using on_both = _B;
};

// keep_delta
//   primitive: on_both = delta; on_base_only = base; on_delta_only = delta.
// Standard "delta wins" - the usual override semantic, extensions allowed.
struct keep_delta
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _D>
    using on_delta_only = _D;

    template<typename _B, typename _D>
    using on_both = _D;
};

// drop_extras
//   primitive: on_both = delta; on_base_only = base; on_delta_only = drop.
// Delta wins on overlap, but delta CANNOT introduce new keys (they're
// silently filtered out by the engine).
struct drop_extras
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _D>
    using on_delta_only = dropped;

    template<typename _B, typename _D>
    using on_both = _D;
};

// strict_subset
//   primitive: like drop_extras but a delta-only key produces a hard
// compile error.  The static_assert lives in a nested struct whose
// instantiation is gated by the engine's lazy lookup, so the concept
// probe does not fire it.
struct strict_subset
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _B, typename _D>
    using on_both = _D;

    // assert_extension
    //   helper: dependent static_assert that fires only when the
    // engine actually asks for on_delta_only<_D>.  sizeof(_D) == 0
    // is the standard "depend on the template parameter so the
    // assert isn't eager" trick.
    template<typename _D>
    struct assert_extension
    {
        static_assert(sizeof(_D) == 0,
            "strict_subset: delta carries a key that does not exist "
            "in base, but the active policy forbids extension.  "
            "Either remove the delta-only entry or switch to a "
            "non-strict policy (keep_delta, drop_extras).");
        using type = dropped;
    };

    template<typename _D>
    using on_delta_only = typename assert_extension<_D>::type;
};

// drop_unmatched_base
//   primitive: only keys present in delta survive.  A "filter to delta"
// operation - the result is value-equal to delta on those keys, but
// retains delta's full option shape.
struct drop_unmatched_base
{
    template<typename _B>
    using on_base_only = dropped;

    template<typename _D>
    using on_delta_only = _D;

    template<typename _B, typename _D>
    using on_both = _D;
};


// ===========================================================================
// IV.  combinators
// ===========================================================================

// with_on_both
//   combinator: takes a base policy and a binary metafunction _F,
// and replaces on_both with `typename _F<_B, _D>::type`.  on_base_only
// and on_delta_only are inherited unchanged.  Use for custom merges.
//
//   _F is expected to be a struct template with a nested ::type.
// Pass alias templates indirectly via a trampoline if needed.
template<typename                           _Base,
         template<typename, typename> class _F>
struct with_on_both : _Base
{
    template<typename _B, typename _D>
    using on_both = typename _F<_B, _D>::type;
};

// with_on_base_only
//   combinator: replaces on_base_only via a unary metafunction _F.
template<typename                 _Base,
         template<typename> class _F>
struct with_on_base_only : _Base
{
    template<typename _B>
    using on_base_only = typename _F<_B>::type;
};

// with_on_delta_only
//   combinator: replaces on_delta_only via a unary metafunction _F.
// Convenient for converting an extension-allowing policy into a
// stricter one, or vice versa.
template<typename                 _Base,
         template<typename> class _F>
struct with_on_delta_only : _Base
{
    template<typename _D>
    using on_delta_only = typename _F<_D>::type;
};


// ===========================================================================
// V.   misc helpers
// ===========================================================================

// identity_t
//   metafn: yields its argument unchanged.  Useful as a no-op slot
// for combinators that demand a unary metafunction.
template<typename _Type>
struct identity_t
{
    using type = _Type;
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
template<typename _L, typename>
struct always_keep_left
{
    using type = _L;
};

// always_keep_right
//   metafn: binary metafn that always yields its second argument.
template<typename, typename _R>
struct always_keep_right
{
    using type = _R;
};


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_OVERRIDE_