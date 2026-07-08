/******************************************************************************
* djinterp [meta]                                              multiplicity.hpp
*
*   Programming-agnostic vocabulary for the MULTIPLICITY axis - the bound a
* container places on mutually interchangeable occurrences (the spec,
* Multiplicity).  Multiplicity has two ingredients: a duplicate-EQUIVALENCE E
* deciding which occurrences are copies, and a numeric BOUND on copies per class.
*
*   THE BOUND.  The multiplicity is
*       m in N-bar_{>=1} = {1,2,3,...} U {inf},
*   ordered 1 < 2 < ... < inf (larger = more permissive).  An instance is valid
* only if every class occurs at most m times.  Three positions carry distinct
* meaning: m = 1 admits each class at most once (SET semantics); 1 < m = k < inf
* admits at most k copies (BOUNDED multiset); m = inf places no cap (UNBOUNDED
* multiset).  The floor is 1 - the value 0 is meaningful only for an uninhabited
* container and is excluded here; it never encodes "no comparator" or "unbounded".
*
*   THE EQUIVALENCE.  With a comparator present the canonical E is its induced
* equivalence; with none, the default is identity (distinct values are never
* copies), under which a comparator-less SEQUENCE places no equivalence-based cap
* and so reads as m = inf.  That inf is numerically equal to a multiset's inf but
* means something different - "no equivalence-based cap at all" versus "an explicit
* cap of none on a genuine equivalence" - so this vocabulary carries the
* equivalence flag alongside the bound and keeps the two infinities as distinct
* KINDS (sequence vs unbounded_multiset).
*
*   This header defines vocabulary only - the kinds, the bound's value space and
* its chain algebra, and the equivalence-aware classifier.  Detecting a concrete
* container's multiplicity is container_multiplicity_traits.hpp's concern.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/meta/multiplicity.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_META_MULTIPLICITY_
#define DJINTERP_META_MULTIPLICITY_ 1

// std
#include <cstddef>
// djinterp
#include "../djinterp.hpp"   // NS_*


NS_DJINTERP


// ===========================================================================
// I.   The bound's value space (N-bar_{>=1})
// ===========================================================================

// unbounded_multiplicity
//   constant: the sentinel for m = inf (the top of the value space - "no finite
// upper bound").  Distinct from the floor and from the excluded 0.
constexpr std::size_t unbounded_multiplicity =
    static_cast<std::size_t>(-1);

// unique_multiplicity
//   constant: the floor m = 1 (set semantics - each class at most once).
constexpr std::size_t unique_multiplicity = 1;

// is_finite_multiplicity
//   function: true iff a bound is finite (not the inf sentinel).
constexpr bool
is_finite_multiplicity(std::size_t _m) noexcept
{
    return _m != unbounded_multiplicity;
}

// multiplicity_join / multiplicity_meet
//   functions: the chain operations on N-bar_{>=1}, inf as the sentinel top.
// Join is the more-permissive bound (max), meet the less-permissive (min); since
// the inf sentinel is the largest representable value, plain max / min suffice.
constexpr std::size_t
multiplicity_join(std::size_t _a, std::size_t _b) noexcept
{
    return ( _a > _b ? _a : _b );
}

constexpr std::size_t
multiplicity_meet(std::size_t _a, std::size_t _b) noexcept
{
    return ( _a < _b ? _a : _b );
}


// ===========================================================================
// II.  The multiplicity kind
// ===========================================================================

// multiplicity_kind
//   enum: a container's qualitative multiplicity, the equivalence folded in so
// the two infinities stay distinct.
enum class multiplicity_kind
{
    unknown,             // not a container / indeterminate
    sequence,            // no equivalence (identity default); m = inf, copies by position
    unique,              // m = 1 under a genuine equivalence (set semantics)
    bounded_multiset,    // 1 < m < inf under a genuine equivalence
    unbounded_multiset   // m = inf under a genuine equivalence (multiset)
};

// multiplicity_kind_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
multiplicity_kind_name(multiplicity_kind _k) noexcept
{
    return ( _k == multiplicity_kind::unknown            ? "unknown"
           : _k == multiplicity_kind::sequence           ? "sequence"
           : _k == multiplicity_kind::unique             ? "unique"
           : _k == multiplicity_kind::bounded_multiset   ? "bounded_multiset"
           :                                               "unbounded_multiset" );
}

// make_multiplicity_kind
//   function: classify an (equivalence-present?, bound) pair into a kind.  Absent
// an equivalence the result is sequence whatever the bound (identity default);
// with one, the bound selects unique (1), unbounded_multiset (inf), or
// bounded_multiset (1 < m < inf).  A bound of 0 is below the working floor and
// maps to unknown.
constexpr multiplicity_kind
make_multiplicity_kind(bool _has_equivalence, std::size_t _m) noexcept
{
    return ( !_has_equivalence
                 ? multiplicity_kind::sequence
           : _m == 0
                 ? multiplicity_kind::unknown
           : _m == unique_multiplicity
                 ? multiplicity_kind::unique
           : _m == unbounded_multiplicity
                 ? multiplicity_kind::unbounded_multiset
           :       multiplicity_kind::bounded_multiset );
}

// multiplicity_bound_of_kind
//   function: the numeric bound a kind implies, where the kind determines it.
// unique -> 1; sequence and unbounded_multiset -> inf.  A bounded_multiset's k
// is not recoverable from the kind alone (the kind is qualitative); inf is
// returned as its permissive upper envelope, the concrete k being carried
// alongside by whoever produced the kind.
constexpr std::size_t
multiplicity_bound_of_kind(multiplicity_kind _k) noexcept
{
    return ( _k == multiplicity_kind::unique ? unique_multiplicity
                                             : unbounded_multiplicity );
}


// ===========================================================================
// III. Kind predicates + ordering
// ===========================================================================

// is_unique_kind
//   function: true for the set-semantics kind (m = 1).
constexpr bool
is_unique_kind(multiplicity_kind _k) noexcept
{
    return _k == multiplicity_kind::unique;
}

// is_multiset_kind
//   function: true for either multiset kind (a genuine equivalence with m > 1).
constexpr bool
is_multiset_kind(multiplicity_kind _k) noexcept
{
    return (    _k == multiplicity_kind::bounded_multiset
             || _k == multiplicity_kind::unbounded_multiset );
}

// is_sequence_kind
//   function: true for the comparator-less kind (identity default).
constexpr bool
is_sequence_kind(multiplicity_kind _k) noexcept
{
    return _k == multiplicity_kind::sequence;
}

// multiplicity_kind_rank
//   function: a permissiveness rank for ordering, by the underlying bound -
// unique (1) below bounded_multiset (k) below the two inf kinds (sequence and
// unbounded_multiset share the top).  unknown sits below the working floor.
// The companion COMPARISON axis consumes this; the equivalence distinction
// between the two top kinds is meaning, not magnitude, so they rank equal.
constexpr int
multiplicity_kind_rank(multiplicity_kind _k) noexcept
{
    return ( _k == multiplicity_kind::unknown            ? -1
           : _k == multiplicity_kind::unique             ?  0
           : _k == multiplicity_kind::bounded_multiset   ?  1
           :                                                2 );  // sequence / unbounded_multiset
}


NS_END  // djinterp


#endif  // DJINTERP_META_MULTIPLICITY_
