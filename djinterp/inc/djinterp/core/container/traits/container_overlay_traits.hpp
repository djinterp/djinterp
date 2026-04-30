/******************************************************************************
* djinterp [container]                            container_overlay_traits.hpp
*
* Overlay container compile-time classification:
*   Detects whether a type is an overlay (delegates storage to an underlying
* container type), classifies the overlay's structural kind, and provides
* utilities for constructing, inspecting, and traversing overlay type
* chains.
*
*   An overlay container is any type that exposes
* `using underlying_container_type = ...;`.  The overlay borrows storage and
* memory layout from that underlying type; its own role is to layer
* additional semantics on top - sort invariants (sorted adapter over a
* vector), bounds checking, recursive multi-axis structure (table over
* nested containers), or remote storage (database table over a connection).
*
*   This is the traits-only counterpart to the overlay container types
* themselves (table.hpp, database_table.hpp, sequential adapters).
* Include this header when you need to detect or classify overlays
* without pulling in the concrete classes.
*
*   This file is the canonical source for axis-9 (`underlying`) traits.
* The basic predicate `is_overlay_container_v` and its counterpart
* `is_fundamental_container_v` may also be referenced from
* `container_traits.hpp` as part of the twelve-axis classification - both
* refer back to the definitions in this file.
*
*
* path:      /inc/djinterp/core/container/traits/container_overlay_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    internal detection helpers
        - has_underlying_container_type_check
        - has_rank_check
II.   public overlay detection
        - has_underlying_container_type
        - is_overlay_container
III.  underlying type extraction
        - underlying_container_type_of
IV.   nesting utilities (construction)
        - nest_container
V.    peeling utilities (deconstruction)
        - peel_overlay              single-layer unwrap
        - overlay_leaf_type         recursive unwrap to leaf
        - overlay_depth             compile-time depth count
VI.   overlay kind classification
        - DOverlayKind              enum
        - overlay_kind              classifier
VII.  combined overlay classification
        - container_overlay_class
*/

#ifndef DJINTERP_CONTAINER_OVERLAY_TRAITS_
#define DJINTERP_CONTAINER_OVERLAY_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// =========================================================================
// I.   INTERNAL DETECTION HELPERS
// =========================================================================

NS_INTERNAL

    // has_underlying_container_type_check
    //   trait: detects whether _Type exposes a member alias
    // `underlying_container_type`.  Primary template (failure case).
    template<typename _Type,
                typename = void>
    struct has_underlying_container_type_check : std::false_type
    {};

    // has_underlying_container_type_check (success case)
    //   trait: partial specialization when the alias is well-formed.
    template<typename _Type>
    struct has_underlying_container_type_check<
        _Type,
        void_t<typename _Type::underlying_container_type>
    > : std::true_type
    {};

    // has_rank_check
    //   trait: detects whether _Type exposes a static `rank` member
    // (used to distinguish multi-axis overlays from flat ones).
    // Primary template (failure case).
    template<typename _Type,
                typename = void>
    struct has_rank_check : std::false_type
    {};

    // has_rank_check (success case)
    //   trait: partial specialization when _Type::rank is a constant
    // expression.
    template<typename _Type>
    struct has_rank_check<
        _Type,
        void_t<decltype(_Type::rank)>
    > : std::true_type
    {};

NS_END  // internal


// =========================================================================
// II.  PUBLIC OVERLAY DETECTION
// =========================================================================

// has_underlying_container_type
//   trait: true if _Type exposes the alias
// `underlying_container_type`, indicating that it delegates storage
// to a separate container.
template<typename _Type>
struct has_underlying_container_type
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_underlying_container_type_check<
            clean_type>::value;
};

// has_underlying_container_type_v
//   value: convenience variable template for
// has_underlying_container_type.
template<typename _Type>
inline constexpr bool has_underlying_container_type_v =
    has_underlying_container_type<_Type>::value;

// is_overlay_container
//   trait: true if _Type is an overlay (delegates storage via
// `underlying_container_type`).  Synonym for
// `has_underlying_container_type` - exists for symmetry with the
// axis-9 `fundamental` / `overlay` classification vocabulary.
template<typename _Type>
struct is_overlay_container
    : has_underlying_container_type<_Type>
{};

// is_overlay_container_v
//   value: convenience variable template for is_overlay_container.
template<typename _Type>
inline constexpr bool is_overlay_container_v =
    is_overlay_container<_Type>::value;


// =========================================================================
// III. UNDERLYING TYPE EXTRACTION
// =========================================================================

NS_INTERNAL

    // underlying_container_type_of_helper
    //   trait: extracts the underlying container type when present,
    // yields void otherwise.  Primary template (failure case).
    template<typename _Type,
                typename = void>
    struct underlying_container_type_of_helper
    {
        using type = void;
    };

    // underlying_container_type_of_helper (success case)
    //   trait: partial specialization when the alias is well-formed.
    template<typename _Type>
    struct underlying_container_type_of_helper<
        _Type,
        void_t<typename _Type::underlying_container_type>
    >
    {
        using type = typename _Type::underlying_container_type;
    };

NS_END  // internal

// underlying_container_type_of
//   trait: yields _Type::underlying_container_type if present,
// otherwise yields void.  Use this when generic code needs the
// underlying type without committing to a SFINAE error on
// non-overlay inputs.
template<typename _Type>
struct underlying_container_type_of
{
    using type = typename internal::
        underlying_container_type_of_helper<
            clean_t<_Type>>::type;
};

// underlying_container_type_of_t
//   type: convenience alias for
// underlying_container_type_of<...>::type.
template<typename _Type>
using underlying_container_type_of_t =
    typename underlying_container_type_of<_Type>::type;


// =========================================================================
// IV.  NESTING UTILITIES
// =========================================================================
//
// nest_container builds a recursively-nested container type by
// applying a single template-template parameter _Rank times around
// a base cell type.  This is the construction direction; the peeling
// utilities in section V invert it.
//

NS_INTERNAL

    // nest_container_helper
    //   trait: recursive case - wraps the (_Rank - 1) result in
    // another _Container layer.
    template<template<typename...> typename _Container,
                typename                       _Cell,
                std::size_t                    _Rank>
    struct nest_container_helper
    {
        using type = _Container<typename nest_container_helper<
                                    _Container, _Cell, _Rank - 1>::type>;
    };

    // nest_container_helper (base case)
    //   trait: rank-0 specialization yielding _Cell unchanged.
    template<template<typename...> typename _Container,
                typename                       _Cell>
    struct nest_container_helper<_Container, _Cell, 0>
    {
        using type = _Cell;
    };

NS_END  // internal

// nest_container
//   trait: builds the nested container type
//     _Container<_Container<...<_Container<_Cell>>...>>
// by applying _Container _Rank times.  Rank 0 yields _Cell.
//
// Example:
//   nest_container<std::vector, int, 3>::type
//     == std::vector<std::vector<std::vector<int>>>
template<template<typename...> typename _Container,
            typename                       _Cell,
            std::size_t                    _Rank>
struct nest_container
{
    using type = typename internal::nest_container_helper<
                        _Container, _Cell, _Rank>::type;
};

// nest_container_t
//   type: convenience alias for nest_container<...>::type.
template<template<typename...> typename _Container,
            typename                       _Cell,
            std::size_t                    _Rank>
using nest_container_t =
    typename nest_container<_Container, _Cell, _Rank>::type;


// =========================================================================
// V.   PEELING UTILITIES
// =========================================================================
//
// The inverse of nesting.  peel_overlay strips one underlying layer.
// overlay_leaf_type peels recursively until reaching a non-overlay.
// overlay_depth counts how many layers were peeled.
//

// peel_overlay
//   trait: yields the underlying container type (one layer down) if
// _Type is an overlay, otherwise yields _Type unchanged.  Useful for
// generic algorithms that may or may not be looking at an overlay.
template<typename _Type,
            bool     _IsOverlay = is_overlay_container_v<clean_t<_Type>>>
struct peel_overlay
{
    using type = clean_t<_Type>;
};

// peel_overlay (overlay specialization)
//   trait: success case - yields the underlying container type.
template<typename _Type>
struct peel_overlay<_Type, true>
{
    using type = underlying_container_type_of_t<clean_t<_Type>>;
};

// peel_overlay_t
//   type: convenience alias for peel_overlay<...>::type.
template<typename _Type>
using peel_overlay_t = typename peel_overlay<_Type>::type;


NS_INTERNAL

    // overlay_leaf_type_helper
    //   trait: recursively peels overlays until a non-overlay type
    // remains.  Generic case: not an overlay, stop.
    template<typename _Type,
                bool     _IsOverlay =
                    is_overlay_container_v<clean_t<_Type>>>
    struct overlay_leaf_type_helper
    {
        using type = clean_t<_Type>;
    };

    // overlay_leaf_type_helper (overlay case)
    //   trait: peel one layer and recurse.
    template<typename _Type>
    struct overlay_leaf_type_helper<_Type, true>
    {
        using type = typename overlay_leaf_type_helper<
                            underlying_container_type_of_t<
                                clean_t<_Type>>>::type;
    };

NS_END  // internal

// overlay_leaf_type
//   trait: recursively peels overlay layers, yielding the first type
// in the chain that is NOT an overlay.  For a non-overlay input,
// yields the input itself.
//
// Example:
//   table<int, std::vector, 3>           overlay --> vector<vector<vector<int>>> ...
//   sorted_view<vector<int>>             overlay --> vector<int> (not overlay)
//                                                  --> vector<int>
//   vector<int>                          not overlay --> vector<int>
template<typename _Type>
struct overlay_leaf_type
{
    using type = typename internal::
        overlay_leaf_type_helper<_Type>::type;
};

// overlay_leaf_type_t
//   type: convenience alias for overlay_leaf_type<...>::type.
template<typename _Type>
using overlay_leaf_type_t = typename overlay_leaf_type<_Type>::type;


NS_INTERNAL

    // overlay_depth_helper
    //   trait: counts overlay nesting depth by recursive peeling.
    // Generic case: not an overlay, depth 0.
    template<typename _Type,
                bool     _IsOverlay =
                    is_overlay_container_v<clean_t<_Type>>>
    struct overlay_depth_helper
    {
        static constexpr std::size_t value = 0;
    };

    // overlay_depth_helper (overlay case)
    //   trait: increment and recurse.
    template<typename _Type>
    struct overlay_depth_helper<_Type, true>
    {
        static constexpr std::size_t value =
            ( 1 +
                overlay_depth_helper<
                    underlying_container_type_of_t<
                        clean_t<_Type>>>::value );
    };

NS_END  // internal

// overlay_depth
//   trait: counts how many overlay layers wrap the leaf type.
// Yields 0 for non-overlay types, 1 for a single-layer overlay,
// N for an overlay whose underlying is itself an N-1 overlay.
//
// Note: this counts *overlay* nesting (containers exposing
// underlying_container_type), not raw value_type nesting.
// A vector<vector<int>> reports depth 0 because vector is not an
// overlay; a table<int, vector, 3> reports depth 1 because table
// is the only overlay in the chain (its underlying is the raw
// nested vector chain).
template<typename _Type>
struct overlay_depth
{
    static constexpr std::size_t value =
        internal::overlay_depth_helper<_Type>::value;
};

// overlay_depth_v
//   value: convenience variable template for overlay_depth.
template<typename _Type>
inline constexpr std::size_t overlay_depth_v =
    overlay_depth<_Type>::value;


// =========================================================================
// VI.  OVERLAY KIND CLASSIFICATION
// =========================================================================

// DOverlayKind
//   enum: classifies the structural kind of an overlay container.
//
//   none    - not an overlay (no underlying_container_type)
//   flat    - single-layer overlay (e.g. sorted adapter over vector,
//             ring buffer over array, bounded view over deque)
//   nested  - multi-axis overlay declaring `rank >= 2` (e.g. table
//             over a nested container chain)
//   axis    - overlay declaring `rank == 1`; structurally flat but
//             part of the multi-axis family (a degenerate table)
enum class DOverlayKind
{
    none,
    flat,
    nested,
    axis
};

NS_INTERNAL

    // overlay_kind_helper
    //   trait: resolves DOverlayKind for _Type by inspecting whether
    // it is an overlay and whether it declares a `rank` member.
    // Uses two boolean template parameters to gate `::rank` access
    // behind has_rank_check, avoiding ill-formed name lookup on
    // types that do not declare it.
    // Primary template - covers the non-overlay case.
    template<typename _Type,
                bool     _IsOverlay =
                    is_overlay_container_v<clean_t<_Type>>,
                bool     _HasRank =
                    has_rank_check<clean_t<_Type>>::value>
    struct overlay_kind_helper
    {
        static constexpr DOverlayKind value = DOverlayKind::none;
    };

    // overlay_kind_helper (overlay, no rank) --> flat
    template<typename _Type>
    struct overlay_kind_helper<_Type, true, false>
    {
        static constexpr DOverlayKind value = DOverlayKind::flat;
    };

    // overlay_kind_helper (overlay, has rank) --> nested or axis
    //   `::rank` access is well-formed here because _HasRank is
    // true via has_rank_check.
    template<typename _Type>
    struct overlay_kind_helper<_Type, true, true>
    {
        static constexpr DOverlayKind value =
            ( clean_t<_Type>::rank >= 2 )
                ? DOverlayKind::nested
                : DOverlayKind::axis;
    };

NS_END  // internal

// overlay_kind
//   trait: resolves the DOverlayKind for a given type.
template<typename _Type>
struct overlay_kind
{
    static constexpr DOverlayKind value =
        internal::overlay_kind_helper<_Type>::value;
};

// overlay_kind_v
//   value: convenience variable template for overlay_kind.
template<typename _Type>
inline constexpr DOverlayKind overlay_kind_v =
    overlay_kind<_Type>::value;


// =========================================================================
// VII. COMBINED OVERLAY CLASSIFICATION
// =========================================================================
//
// Aggregates all overlay-axis detections into a single classification
// struct.  Mirrors the sequential_kind / table_class pattern from the
// rest of the trait system.
//

// container_overlay_class
//   struct: compile-time overlay classification for _Type.  Default
// branch (non-overlay) reports the "no overlay" answers.
template<typename _Type,
            bool     _IsOverlay = is_overlay_container_v<clean_t<_Type>>>
struct container_overlay_class
{
    // identity
    static constexpr bool         is_overlay   = false;
    static constexpr bool         is_fundamental = true;
    static constexpr DOverlayKind kind         = DOverlayKind::none;

    // structure
    static constexpr std::size_t  depth = 0;

    // type chain
    using underlying_type = void;
    using leaf_type       = clean_t<_Type>;
};

// container_overlay_class (overlay specialization)
//   struct: classification for types that satisfy the overlay
// structural interface.
template<typename _Type>
struct container_overlay_class<_Type, true>
{
    // identity
    static constexpr bool         is_overlay     = true;
    static constexpr bool         is_fundamental = false;
    static constexpr DOverlayKind kind           = overlay_kind_v<_Type>;

    // structure
    static constexpr std::size_t  depth = overlay_depth_v<_Type>;

    // type chain
    using underlying_type = underlying_container_type_of_t<clean_t<_Type>>;
    using leaf_type       = overlay_leaf_type_t<_Type>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OVERLAY_TRAITS_