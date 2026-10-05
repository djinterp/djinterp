/*******************************************************************************
* djinterp [core]                                                math_common.hpp
*
* Foundational mathematical types shared by the `math` and `container`
* submodules.
*   This header sits below `math.hpp` and `interval.hpp`; it owns the
* container-agnostic vocabulary that both submodules need to spell:
* cardinality, multiplicity, directionality, ordering kinds, relation
* properties, closure of intervals, algebraic-structure tags, and a
* handful of small types (arity, degree, parity, sign).
*
*   Nothing here knows what a container is.  Nothing here knows what
* a graph is.  Everything here is a description of a property of a
* set, a relation, or a function.  Container modules consume these
* enums to label their axes; math modules consume them to classify
* expressions and morphisms.
*
* DESIGN PRINCIPLES:
*   1. No tag types are required for end users -- detection is structural.
*      The tags below are *available* for users who want explicit opt-in,
*      but every property is also detectable purely structurally.
*   2. Every enum has a `none` / `unknown` member at value 0 to allow
*      default construction and "not classified" semantics.
*   3. Every enum is `enum class` with an explicit underlying type so
*      bit-packing into trait classification structs is predictable.
*   4. C++11 baseline; feature-gated extensions for >= C++14/17/20.
*
*
* path:      /inc/djinterp/core/container/graph/math_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.27
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Numeric Vocabulary Types
      ------------------------
      1.    arity_t, degree_t, depth_t
      2.    parity, sign
      3.    handedness

II.   Cardinality
      -----------
      1.    cardinality enum
      2.    is_finite_v / is_countable_v

III.  Multiplicity
      ------------
      1.    multiplicity enum
      2.    self_loop_policy enum

IV.   Directionality
      --------------
      1.    directionality enum
      2.    orientation enum (per-edge, finer than graph-wide)

V.    Closure (interval boundary kinds)
      ---------------------------------

VI.   Ordering Kinds
      --------------
      1.    ordering_kind enum (none / partial / total / well)
      2.    strictness enum (strict / non_strict)

VII.  Relation Properties
      -------------------
      1.    relation_property bitmask
      2.    relation_kind enum (equivalence / partial_order / etc.)

VIII. Algebraic Structure Tags
      ------------------------
      1.    magma / semigroup / monoid / group / abelian / ring / field

IX.   Small Compile-Time Pair / Triple Helpers
      ----------------------------------------

X.    Sentinel / Null Helpers
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_GRAPH_MATH_COMMON_HPP
#define DJINTERP_CONTAINER_GRAPH_MATH_COMMON_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <limits>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::int8_t, uint8_t,
                                                   // uint16_t


NS_DJINTERP  // djinterp
NS_MATH      // math


// =============================================================================
// I.    NUMERIC VOCABULARY TYPES
// =============================================================================

// arity_t
//   type: number of inputs accepted by a function or relation. 0 = nullary, 1
// = unary, 2 = binary, ..., (size_t)-1 = variadic.
using arity_t = std::size_t;

// degree_t
//   type: degree of a polynomial, vertex degree in a graph, or degree of a
// recurrence. Same backing type as size_t for uniform comparison.
using degree_t = std::size_t;

// depth_t
//   type: depth of a hierarchical structure (tree level, recursion depth,
// nesting depth).
using depth_t = std::size_t;

// kVariadicArity
//   constant: sentinel value used to mark functions with unbounded arity.
// Distinct from any reasonable concrete arity.
static D_CONSTEXPR arity_t kVariadicArity =
    static_cast<arity_t>(-1);

// kUnknownDepth
//   constant: sentinel value for "depth is not known at compile time".
static D_CONSTEXPR depth_t kUnknownDepth =
    static_cast<depth_t>(-1);


// parity
//   enum: parity of an integer or a permutation.
enum class parity : re_std::uint8_t
{
    unknown = 0,
    even    = 1,
    odd     = 2
};

// sign
//   enum: tri-state sign of a real value.
enum class sign : re_std::int8_t
{
    unknown  = 0,
    negative = -1,
    zero     = 1,    // shifted so all members are distinct
    positive = 2
};

// handedness
//   enum: chirality of a coordinate system or transformation.
enum class handedness : re_std::uint8_t
{
    unknown = 0,
    left    = 1,
    right   = 2,
    neither = 3       // achiral
};


// =============================================================================
// II.   CARDINALITY
// =============================================================================

// cardinality
//   enum: the size class of a set. Used by both interval/range types in `math`
// and bound-related axes in `container`.
enum class cardinality : re_std::uint8_t
{
    unknown            = 0,

    // empty set; size = 0
    empty              = 1,

    // size is a finite natural number known at compile time
    finite_static      = 2,

    // size is a finite natural number, runtime-determined
    finite_dynamic     = 3,

    // size is countably infinite (aleph_0)
    countable_infinite = 4,

    // size is uncountably infinite (aleph_1+)
    uncountable        = 5
};

// is_finite_cardinality
//   trait: true if the cardinality is finite (any of empty, finite_static,
// finite_dynamic).
template<cardinality C>
struct is_finite_cardinality
{
    static D_CONSTEXPR bool value =
        ( (C == cardinality::empty)          ||
          (C == cardinality::finite_static)  ||
          (C == cardinality::finite_dynamic) );
};

// is_countable_cardinality
//   trait: true if the cardinality is countable (finite or countably
// infinite).
template<cardinality C>
struct is_countable_cardinality
{
    static D_CONSTEXPR bool value =
        ( is_finite_cardinality<C>::value ||
          (C == cardinality::countable_infinite) );
};


// =============================================================================
// III.  MULTIPLICITY
// =============================================================================

// multiplicity
//   enum: whether a collection allows duplicate elements, and if so, in what
// regime. Containers consume this for their `multiplicity` axis; math consumes
// it for multisets and multi-edges in graphs.
enum class multiplicity : re_std::uint8_t
{
    unknown   = 0,

    // every element appears at most once (set / simple graph)
    unique    = 1,

    // duplicates allowed, no upper bound (multiset / multigraph)
    multi     = 2,

    // duplicates allowed up to a fixed compile-time bound k (k-multiset;
    // concrete k carried by `multiplicity_interval`)
    bounded   = 3
};

// self_loop_policy
//   enum: whether a relation may relate an element to itself. Equivalent to
// "is the relation reflexive or irreflexive on the diagonal?" but spelled in
// graph-friendly terms.
enum class self_loop_policy : re_std::uint8_t
{
    unknown    = 0,

    // x ~ x is forbidden (irreflexive, simple graph)
    disallow   = 1,

    // x ~ x is allowed (reflexive-permitted, pseudograph)
    allow      = 2,

    // x ~ x is required for every x (reflexive)
    require    = 3
};


// =============================================================================
// IV.   DIRECTIONALITY
// =============================================================================

// directionality
//   enum: whether an edge / morphism / arrow has a sense of direction. Used by
// graphs at the topology level and by math for morphisms in categories.
enum class directionality : re_std::uint8_t
{
    unknown      = 0,

    // (u, v) is the same as (v, u); symmetric relation
    undirected   = 1,

    // (u, v) is distinct from (v, u); asymmetric in general
    directed     = 2,

    // both an undirected and a directed edge may exist between
    // the same pair, or the graph mixes directed and undirected
    // edges
    mixed        = 3,

    // directed AND no edge is paired with its reverse (irreflexive
    // antisymmetric)
    oriented     = 4,

    // every pair has exactly one directed edge in some direction (tournament)
    tournament   = 5,

    // both (u,v) and (v,u) always exist together (symmetric digraph;
    // equivalent to undirected for many algorithmic purposes)
    bidirected   = 6
};

// orientation
//   enum: direction of a single edge relative to a vertex. Finer-grained than
// `directionality` (graph-wide); answers the question "with respect to vertex
// v, which way does this edge go?".
enum class orientation : re_std::uint8_t
{
    unknown    = 0,
    incoming   = 1,    // edge points TO this vertex
    outgoing   = 2,    // edge points FROM this vertex
    incident   = 3,    // undirected (both)
    self_loop  = 4     // edge starts and ends at this vertex
};


// =============================================================================
// V.    CLOSURE
// =============================================================================

// closure
//   enum: which endpoints of an interval are included. Used by math::interval,
// by container size_intervals, and by graph path types (open path, closed
// walk, etc.).
enum class closure : re_std::uint8_t
{
    unknown    = 0,
    open       = 1,    // (a, b)
    closed     = 2,    // [a, b]
    left_open  = 3,    // (a, b]
    right_open = 4     // [a, b)
};

// closure_includes_left
//   trait: true if the closure includes its left endpoint. (Not named
// is_left_closed: math/interval/interval.hpp's is_left_closed classifies an
// interval TYPE, and two templates of one name cannot share djinterp::math.)
template<closure C>
struct closure_includes_left
{
    static D_CONSTEXPR bool value =
        ( (C == closure::closed) ||
          (C == closure::right_open) );
};

// closure_includes_right
//   trait: true if the closure includes its right endpoint (see
// closure_includes_left for the name).
template<closure C>
struct closure_includes_right
{
    static D_CONSTEXPR bool value =
        ( (C == closure::closed) ||
          (C == closure::left_open) );
};


// =============================================================================
// VI.   ORDERING KINDS
// =============================================================================

// ordering_kind
//   enum: the strongest ordering invariant a set / sequence satisfies. Used by
// container's `ordering` axis (sorted vs. ordered vs. unordered) and by math
// for posets, lattices, and chains.
enum class ordering_kind : re_std::uint8_t
{
    unknown          = 0,

    // no order relation
    unordered        = 1,

    // insertion order is preserved but no key relation (sequence / list)
    sequenced        = 2,

    // partial order: reflexive + antisymmetric + transitive,
    // not necessarily total
    partial          = 3,

    // total / linear order: every pair is comparable
    total            = 4,

    // total order in which every non-empty subset has a least element
    // (well-order, e.g. naturals)
    well_ordered     = 5
};

// strictness
//   enum: whether a relation is strict (irreflexive) or non-strict
// (reflexive). Pairs with ordering_kind to spell `<` vs `<=` etc.
enum class strictness : re_std::uint8_t
{
    unknown    = 0,
    strict     = 1,    // < or > ; irreflexive
    non_strict = 2     // <= or >= ; reflexive
};


// =============================================================================
// VII.  RELATION PROPERTIES
// =============================================================================

// relation_property
//   enum: bitmask of properties a binary relation R on a set X may satisfy.
// Used by graph traits to classify edge relations and by math to classify
// orderings, equivalences, equalities.
//
// Underlying type is re_std::uint16_t to leave room for future additions while
// preserving fast bitwise operations.
enum class relation_property : re_std::uint16_t
{
    none           = 0,

    // Reflexive:        for all x:        x R x
    reflexive      = 1u << 0,

    // Irreflexive:      for all x:    not x R x
    irreflexive    = 1u << 1,

    // Symmetric:        x R y => y R x
    symmetric      = 1u << 2,

    // Antisymmetric:    x R y and y R x => x == y
    antisymmetric  = 1u << 3,

    // Asymmetric:       x R y => not y R x  (irreflexive + antisymmetric)
    asymmetric     = 1u << 4,

    // Transitive:       x R y and y R z => x R z
    transitive     = 1u << 5,

    // Connex / total:   for all x, y:  x R y or y R x
    connex         = 1u << 6,

    // Trichotomous:     exactly one of  x R y, y R x, x == y holds
    trichotomous   = 1u << 7,

    // Serial / total relation:  for all x, exists y with x R y
    serial         = 1u << 8,

    // Functional:       for all x, exists at most one y with x R y
    functional     = 1u << 9,

    // Injective:        x1 R y and x2 R y => x1 == x2
    injective      = 1u << 10,

    // Dense:            x R z and x != z => exists y with x R y R z
    dense          = 1u << 11,

    // Euclidean:        x R y and x R z => y R z
    euclidean      = 1u << 12
};

// operator| / operator& / operator^ / operator~ for relation_property
//   convenience: bitwise composition of relation properties.
D_CONSTEXPR_INLINE relation_property
operator|
(
    relation_property _a,
    relation_property _b
) noexcept
{
    return static_cast<relation_property>(
        static_cast<re_std::uint16_t>(_a) |
        static_cast<re_std::uint16_t>(_b));
}

D_CONSTEXPR_INLINE relation_property
operator&
(
    relation_property _a,
    relation_property _b
) noexcept
{
    return static_cast<relation_property>(
        static_cast<re_std::uint16_t>(_a) &
        static_cast<re_std::uint16_t>(_b));
}

D_CONSTEXPR_INLINE relation_property
operator^
(
    relation_property _a,
    relation_property _b
) noexcept
{
    return static_cast<relation_property>(
        static_cast<re_std::uint16_t>(_a) ^
        static_cast<re_std::uint16_t>(_b));
}

D_CONSTEXPR_INLINE relation_property
operator~
(
    relation_property _a
) noexcept
{
    return static_cast<relation_property>(
        ~static_cast<re_std::uint16_t>(_a));
}

// has_relation_property
//   function: true if `_set` contains `_query` (bitwise AND == _query).
// Designed for use in `if constexpr` chains.
D_CONSTEXPR_INLINE bool
has_relation_property
(
    relation_property _set,
    relation_property _query
) noexcept
{
    return ( (static_cast<re_std::uint16_t>(_set) &
              static_cast<re_std::uint16_t>(_query))
                == static_cast<re_std::uint16_t>(_query) );
}


// relation_kind
//   enum: named composite of relation properties for the most
// commonly-encountered relation classes. Each kind corresponds to a fixed
// bitmask of relation_property values.
enum class relation_kind : re_std::uint8_t
{
    unknown          = 0,

    // any binary relation (no constraints)
    arbitrary        = 1,

    // reflexive + transitive
    preorder         = 2,

    // reflexive + symmetric (no transitivity required)
    tolerance        = 3,

    // reflexive + symmetric + transitive
    equivalence      = 4,

    // reflexive + antisymmetric + transitive
    partial_order    = 5,

    // partial_order + connex
    total_order      = 6,

    // irreflexive + transitive (strict partial order)
    strict_partial   = 7,

    // strict_partial + trichotomous
    strict_total     = 8,

    // partial_order + every pair has a unique join and meet
    lattice          = 9,

    // every two elements are connected by a finite chain
    well_founded     = 10,

    // total_order + well-founded (every non-empty subset has a least)
    well_order       = 11,

    // x R y => y R x (no other constraint)
    symmetric_only   = 12,

    //   function: f: X -> Y as a special relation
    function         = 13
};


// =============================================================================
// VIII. ALGEBRAIC STRUCTURE TAGS
// =============================================================================
// Tag types and an enum classifying a binary operation `op: S x S
// -> S` plus an optional identity / inverse.  These are useful to
// container modules whose elements are fed through reductions
// (fold, accumulate) and to math modules building expression
// trees.

// algebraic_structure
//   enum: classification of (S, op, ?id, ?inv). Each level strictly extends
// the one above it.
enum class algebraic_structure : re_std::uint8_t
{
    unknown        = 0,

    // op closed on S  (closure axiom only)
    magma          = 1,

    // magma + associative
    semigroup      = 2,

    // semigroup + identity element
    monoid         = 3,

    // monoid + inverses for every element
    group          = 4,

    // group + commutative
    abelian_group  = 5,

    // (S, +, *) with + abelian, * associative, distributive
    ring           = 6,

    // ring with multiplicative identity
    unital_ring    = 7,

    // unital_ring + commutative *
    commutative_ring = 8,

    // commutative_ring + every non-zero has a multiplicative
    // inverse
    field          = 9,

    // ordered field
    ordered_field  = 10
};

// has_identity_element
//   trait: true if the structure contains an identity element for its primary
// operation.
template<algebraic_structure S>
struct has_identity_element
{
    static D_CONSTEXPR bool value =
        ( static_cast<re_std::uint8_t>(S) >=
          static_cast<re_std::uint8_t>(algebraic_structure::monoid) );
};

// has_inverses
//   trait: true if every element has an inverse under the primary operation.
template<algebraic_structure S>
struct has_inverses
{
    static D_CONSTEXPR bool value =
        ( static_cast<re_std::uint8_t>(S) >=
          static_cast<re_std::uint8_t>(algebraic_structure::group) );
};

// is_commutative_structure
//   trait: true if the primary operation is commutative.
template<algebraic_structure S>
struct is_commutative_structure
{
    static D_CONSTEXPR bool value =
        ( (S == algebraic_structure::abelian_group)    ||
          (S == algebraic_structure::commutative_ring) ||
          (S == algebraic_structure::field)            ||
          (S == algebraic_structure::ordered_field) );
};


// =============================================================================
// IX.   COMPILE-TIME PAIR / TRIPLE HELPERS
// =============================================================================
// Pure-value pair / triple structs in the math namespace.  Carry
// no semantics beyond grouping; both math and container modules
// use them to express bounds, edge endpoints, and indexed values
// in constexpr contexts where std::pair was historically
// non-constexpr.

// type_pair
//   struct: compile-time pair of types. Distinct from std::pair since this
// never instantiates a runtime object.
template<typename First,
         typename Second>
struct type_pair
{
    using first  = First;
    using second = Second;
};

// type_triple
//   struct: compile-time triple of types.
template<typename First,
         typename Second,
         typename Third>
struct type_triple
{
    using first  = First;
    using second = Second;
    using third  = Third;
};

// value_pair
//   struct: compile-time pair of values of the same type. Used for vertex/edge
// endpoints, interval bounds, and similar.
template<typename Type,
         Type     First,
         Type     Second>
struct value_pair
{
    using value_type = Type;

    static D_CONSTEXPR value_type first  = First;
    static D_CONSTEXPR value_type second = Second;
};

// indexed_value
//   struct: compile-time (index, value) pair. Used by sparse vector / matrix
// types and by graph algorithms that produce indexed results (degree
// sequences, score lists, etc.).
template<std::size_t Index,
         typename    Type,
         Type        Value>
struct indexed_value
{
    using value_type = Type;

    static D_CONSTEXPR std::size_t index = Index;
    static D_CONSTEXPR value_type  value = Value;
};


// =============================================================================
// X.    SENTINEL / NULL HELPERS
// =============================================================================

// kInvalidSize
//   constant: sentinel for "no valid size" (used in size_t fields).
static D_CONSTEXPR std::size_t kInvalidSize =
    static_cast<std::size_t>(-1);

// kInvalidIndex
//   constant: sentinel for "no valid index".
static D_CONSTEXPR std::size_t kInvalidIndex =
    static_cast<std::size_t>(-1);

// invalid_id
//   function (template): returns the canonical sentinel value for any
// unsigned-integer ID type. Always (UInt)-1.
template<typename UInt>
D_CONSTEXPR_INLINE UInt invalid_id() noexcept
{
    static_assert(std::is_unsigned<UInt>::value,
                  "invalid_id<T>() requires an unsigned integer type.");
    return static_cast<UInt>(-1);
}

// is_invalid_id
//   function (template): true if `_id` equals the sentinel returned by
// `invalid_id<T>()`.
template<typename UInt>
D_CONSTEXPR_INLINE bool
is_invalid_id
(
    UInt _id
)
noexcept
{
    return (_id == invalid_id<UInt>());
}


NS_END  // math
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_GRAPH_MATH_COMMON_HPP
