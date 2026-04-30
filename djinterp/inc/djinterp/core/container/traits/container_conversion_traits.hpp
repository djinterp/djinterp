/******************************************************************************
* djinterp [container]                         container_conversion_traits.hpp
*
* Master conversion traits for the djinterp container framework.
*   Aggregates the four conversion tiers into a single per-axis and
* per-pair classification:
*     Tier 0 - view:          zero-cost reinterpretation
*     Tier 1 - constructive:  O(n) data-preserving copy/sort
*     Tier 2 - lossy:         O(n) with possible data loss
*     Tier 3 - structural:    full reconstruction / topology change
*   The overall conversion tier for a (From, To) pair is the maximum
* of the per-axis tiers: the conversion is only as cheap as its most
* expensive axis.
*   Tier 2 (lossy) and Tier 3 (structural) detection is defined
* inline in this module because the per-axis logic is small and
* tightly coupled to the tier aggregation.
* 
* TIER 2 AXES (lossy - data may be discarded):
*     multi --> unique:       deduplication discards duplicates
*     unbounded --> bounded:  out-of-range elements rejected/clamped
* TIER 3 AXES (structural - topology/layout change):
*     flat ↔ hierarchical:  requires tree-building or flattening
*     underlying change:       different internal storage type
*     incompatible elements: no implicit conversion path
* 
* DEPENDENCIES:
*   container_view_traits.hpp           - Tier 0
*   container_constructive_traits.hpp   - Tier 1
*   container_compare_traits.hpp        - element compatibility
*
*
* path:      /inc/djinterp/core/container/traits/
*                container_conversion_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      Conversion Tier Enum
II.     Tier 2 (Lossy) Per-Axis Detection
III.    Tier 3 (Structural) Per-Axis Detection
IV.     Per-Axis Tier Deduction
V.      Overall Tier Deduction
VI.     Convenience Predicates
VII.    Combined Classification
*/


#ifndef DJINTERP_CONTAINER_CONVERSION_TRAITS_
#define DJINTERP_CONTAINER_CONVERSION_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "container_traits.hpp"
#include "container_compare_traits.hpp"
#include "container_view_traits.hpp"
#include "container_constructive_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Conversion Tier Enum
// ===========================================================================

// DConversionTier
//   enum: classifies the cost of a conversion.
enum class DConversionTier
{
    // zero-cost: compile-time reinterpretation
    view = 0,

    // O(n) or O(n log n): data preserved
    constructive = 1,

    // O(n): some data may be discarded
    lossy = 2,

    // full reconstruction: topology or layout change
    structural = 3,

    // no conversion path exists
    impossible = 4
};


// ===========================================================================
// II.  Tier 2 (Lossy) Per-Axis Detection
// ===========================================================================
// Lossy conversions preserve most data but may discard
// some elements to satisfy the target's invariants.

// needs_deduplication
//   type trait: true if _From allows duplicates and _To
// requires uniqueness - elements will be discarded.
template<typename _From,
         typename _To>
struct needs_deduplication
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_unique_container_v<F>             &&
          is_unique_container_v<T>              &&
          has_equality_comparable_elements_v<F> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_deduplication_v =
    needs_deduplication<_From, _To>::value;

#endif
// needs_bound_clamp
//   type trait: true if _From is unbounded and _To is
// bounded - out-of-range elements rejected.
template<typename _From,
         typename _To>
struct needs_bound_clamp
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_bounded_container_v<F>            &&
          is_bounded_container_v<T> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_bound_clamp_v =
    needs_bound_clamp<_From, _To>::value;

#endif
// is_lossy_convertible
//   type trait: true if _From --> _To requires lossy work
// (not a view, not purely constructive, but data loss is
// bounded to invariant enforcement).
template<typename _From,
         typename _To>
struct is_lossy_convertible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_view_convertible_v<F, T>          &&
          !is_constructive_convertible_v<F, T>  &&
          ( elements_same_type_v<F, T> ||
            elements_convertible_v<F, T> )      &&
          has_constructive_path_v<F, T>         &&
          is_hierarchy_view_compatible_v<F, T>  &&
          ( needs_deduplication_v<F, T> ||
            needs_bound_clamp_v<F, T> ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_lossy_convertible_v =
    is_lossy_convertible<_From, _To>::value;


#endif
// ===========================================================================
// III. Tier 3 (Structural) Per-Axis Detection
// ===========================================================================
// Structural conversions require full reconstruction with
// a fundamentally different memory layout or topology.

// needs_hierarchy_change
//   type trait: true if _From ↔ _To crosses the flat /
// hierarchical boundary.
template<typename _From,
         typename _To>
struct needs_hierarchy_change
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_hierarchy_view_compatible_v<F, T> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_hierarchy_change_v =
    needs_hierarchy_change<_From, _To>::value;

#endif
// needs_ing_change
//   type trait: true if _From and _To have different
// underlying container types (detected via
// underlying_container_type alias).
NS_INTERNAL

    template<typename _Type, typename = void>
    struct safe_ing_type
    {
        using type = void;
    };

    template<typename _Type>
    struct safe_ing_type<_Type,
        std::void_t<
            typename _Type::underlying_container_type>>
    {
        using type =
            typename _Type::underlying_container_type;
    };

    template<typename _Type>
    using safe_ing_type_t =
        typename safe_ing_type<_Type>::type;

NS_END  // internal

template<typename _From,
         typename _To>
struct needs_ing_change
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    using from_ing =
        internal::safe_ing_type_t<F>;
    using to_ing =
        internal::safe_ing_type_t<T>;

    // underlying change needed when both expose an
    // underlying type and they differ, OR when one
    // exposes it and the other doesn't
    static constexpr bool value =
        ( !std::is_void_v<from_ing>         &&
          !std::is_void_v<to_ing>           &&
          !std::is_same_v<from_ing,
                          to_ing> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_ing_change_v =
    needs_ing_change<_From, _To>::value;

#endif
// has_incompatible_elements
//   type trait: true if _From's value_type cannot be
// converted to _To's value_type at all (no implicit
// conversion path).
template<typename _From,
         typename _To>
struct has_incompatible_elements
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !elements_same_type_v<F, T>           &&
          !elements_convertible_v<F, T> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool has_incompatible_elements_v =
    has_incompatible_elements<_From, _To>::value;

#endif
// is_structural_convertible
//   type trait: true if _From --> _To requires structural
// reconstruction but is still theoretically possible
// (both are iterable and there exists some path via
// element transformation).
template<typename _From,
         typename _To>
struct is_structural_convertible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_view_convertible_v<F, T>          &&
          !is_constructive_convertible_v<F, T>  &&
          !is_lossy_convertible_v<F, T>         &&
          is_iterable_v<F>                      &&
          ( has_push__v<T> ||
            has_insert_v<T>    ||
            is_range_constructible_v<F, T> )    &&
          !has_incompatible_elements_v<F, T> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_structural_convertible_v =
    is_structural_convertible<_From, _To>::value;


#endif
// ===========================================================================
// IV.  Per-Axis Tier Deduction
// ===========================================================================
// For each axis, determines which tier the From --> To
// conversion falls into.

NS_INTERNAL

    // --- element type axis ---

    template<typename _From,
             typename _To>
    struct element_axis_tier
    {
        using F = clean_t<_From>;
        using T = clean_t<_To>;

        static constexpr DConversionTier value =
            elements_same_type_v<F, T>
                ? DConversionTier::view

            : elements_convertible_v<F, T>
                ? DConversionTier::constructive

            : DConversionTier::impossible;
    };

    // --- lifetime axis ---

    template<typename _From,
             typename _To>
    struct lifetime_axis_tier
    {
        static constexpr DConversionTier value =
            is_lifetime_view_compatible_v<
                _From, _To>
                ? DConversionTier::view

            : needs_lifetime_copy_v<_From, _To>
                ? DConversionTier::constructive

            : DConversionTier::impossible;
    };

    // --- iteration axis ---

    template<typename _From,
             typename _To>
    struct iteration_axis_tier
    {
        // iteration can only be downgraded (view)
        // or is impossible - there is no constructive
        // path to synthesize stronger iterators
        static constexpr DConversionTier value =
            is_iteration_view_compatible_v<
                _From, _To>
                ? DConversionTier::view
                : DConversionTier::impossible;
    };

    // --- ordering axis ---

    template<typename _From,
             typename _To>
    struct ordering_axis_tier
    {
        static constexpr DConversionTier value =
            is_ordering_view_compatible_v<
                _From, _To>
                ? DConversionTier::view

            : needs_sort_v<_From, _To>
                ? DConversionTier::constructive

            : DConversionTier::impossible;
    };

    // --- bounds axis ---

    template<typename _From,
             typename _To>
    struct bounds_axis_tier
    {
        static constexpr DConversionTier value =
            is_bounds_view_compatible_v<
                _From, _To>
                ? DConversionTier::view

            : needs_bound_clamp_v<_From, _To>
                ? DConversionTier::lossy

            : DConversionTier::impossible;
    };

    // --- multiplicity axis ---

    template<typename _From,
             typename _To>
    struct multiplicity_axis_tier
    {
        static constexpr DConversionTier value =
            is_multiplicity_view_compatible_v<
                _From, _To>
                ? DConversionTier::view

            : needs_deduplication_v<_From, _To>
                ? DConversionTier::lossy

            : DConversionTier::impossible;
    };

    // --- thread safety axis ---

    template<typename _From,
             typename _To>
    struct threadsafe_axis_tier
    {
        static constexpr DConversionTier value =
            is_threadsafe_view_compatible_v<
                _From, _To>
                ? DConversionTier::view

            : needs_lock_wrap_v<_From, _To>
                ? DConversionTier::constructive

            : DConversionTier::impossible;
    };

    // --- hierarchy axis ---

    template<typename _From,
             typename _To>
    struct hierarchy_axis_tier
    {
        static constexpr DConversionTier value =
            is_hierarchy_view_compatible_v<
                _From, _To>
                ? DConversionTier::view

            : needs_hierarchy_change_v<_From, _To>
                ? DConversionTier::structural

            : DConversionTier::impossible;
    };

    // --- tier max helper ---

    constexpr DConversionTier
    tier_max(DConversionTier _a,
             DConversionTier _b) noexcept
    {
        return ( static_cast<int>(_a) >=
                 static_cast<int>(_b) )
            ? _a : _b;
    }

NS_END  // internal

// element_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    element_axis_tier_v =
        internal::element_axis_tier<
            _From, _To>::value;

// lifetime_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    lifetime_axis_tier_v =
        internal::lifetime_axis_tier<
            _From, _To>::value;

// iteration_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    iteration_axis_tier_v =
        internal::iteration_axis_tier<
            _From, _To>::value;

// ordering_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    ordering_axis_tier_v =
        internal::ordering_axis_tier<
            _From, _To>::value;

// bounds_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    bounds_axis_tier_v =
        internal::bounds_axis_tier<
            _From, _To>::value;

// multiplicity_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    multiplicity_axis_tier_v =
        internal::multiplicity_axis_tier<
            _From, _To>::value;

// threadsafe_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    threadsafe_axis_tier_v =
        internal::threadsafe_axis_tier<
            _From, _To>::value;

// hierarchy_axis_tier_v
template<typename _From, typename _To>
inline constexpr DConversionTier
    hierarchy_axis_tier_v =
        internal::hierarchy_axis_tier<
            _From, _To>::value;


// ===========================================================================
// V.   Overall Tier Deduction
// ===========================================================================

NS_INTERNAL

    template<typename _From,
             typename _To>
    struct overall_tier_impl
    {
        // take the maximum across all axes - the
        // conversion is only as cheap as the most
        // expensive axis
        static constexpr DConversionTier value =
            tier_max(
            tier_max(
            tier_max(
            tier_max(
            tier_max(
            tier_max(
            tier_max(
                element_axis_tier<
                    _From, _To>::value,
                lifetime_axis_tier<
                    _From, _To>::value),
                iteration_axis_tier<
                    _From, _To>::value),
                ordering_axis_tier<
                    _From, _To>::value),
                bounds_axis_tier<
                    _From, _To>::value),
                multiplicity_axis_tier<
                    _From, _To>::value),
                threadsafe_axis_tier<
                    _From, _To>::value),
                hierarchy_axis_tier<
                    _From, _To>::value);
    };

NS_END  // internal

// conversion_tier
//   type trait: the overall conversion tier for a
// From --> To pair.
template<typename _From,
         typename _To>
struct conversion_tier
{
    static constexpr DConversionTier value =
        internal::overall_tier_impl<
            _From, _To>::value;
};

template<typename _From,
         typename _To>
inline constexpr DConversionTier
    conversion_tier_v =
        conversion_tier<_From, _To>::value;


// ===========================================================================
// VI.  Convenience Predicates
// ===========================================================================

// is_convertible_container_pair
//   type trait: true if any conversion path exists
// (tier != impossible).
template<typename _From,
         typename _To>
struct is_convertible_container_pair
{
    static constexpr bool value =
        ( conversion_tier_v<_From, _To> !=
          DConversionTier::impossible );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_convertible_container_pair_v =
        is_convertible_container_pair<
            _From, _To>::value;
#endif

// is_lossless_convertible
//   type trait: true if conversion preserves all data
// (tier <= constructive).
template<typename _From,
         typename _To>
struct is_lossless_convertible
{
    static constexpr bool value =
        ( conversion_tier_v<_From, _To> <=
          DConversionTier::constructive );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_lossless_convertible_v =
    is_lossless_convertible<_From, _To>::value;

#endif
// is_bidirectional_convertible
//   type trait: true if conversion is possible in both
// directions.
template<typename _A,
         typename _B>
struct is_bidirectional_convertible
{
    static constexpr bool value =
        ( is_convertible_container_pair_v<_A, _B> &&
          is_convertible_container_pair_v<_B, _A> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _A,
         typename _B>
inline constexpr bool
    is_bidirectional_convertible_v =
        is_bidirectional_convertible<_A, _B>::value;
#endif

// conversion_is_symmetric
//   type trait: true if both directions have the same
// tier (equally expensive).
template<typename _A,
         typename _B>
struct conversion_is_symmetric
{
    static constexpr bool value =
        ( conversion_tier_v<_A, _B> ==
          conversion_tier_v<_B, _A> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _A,
         typename _B>
inline constexpr bool conversion_is_symmetric_v =
    conversion_is_symmetric<_A, _B>::value;

#endif
// cheapest_direction
//   type trait: returns the tier of the cheaper
// direction for a bidirectional pair.
template<typename _A,
         typename _B>
struct cheapest_direction
{
    static constexpr DConversionTier a_to_b =
        conversion_tier_v<_A, _B>;
    static constexpr DConversionTier b_to_a =
        conversion_tier_v<_B, _A>;

    static constexpr DConversionTier value =
        ( static_cast<int>(a_to_b) <=
          static_cast<int>(b_to_a) )
            ? a_to_b : b_to_a;
};

template<typename _A,
         typename _B>
inline constexpr DConversionTier
    cheapest_direction_v =
        cheapest_direction<_A, _B>::value;


// ===========================================================================
// VII. Combined Classification
// ===========================================================================

// container_conversion_class
//   struct: complete conversion classification for a
// From --> To container pair.
template<typename _From,
         typename _To>
struct container_conversion_class
{
    // per-axis tiers
    static constexpr DConversionTier
        element_tier =
            element_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        lifetime_tier =
            lifetime_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        iteration_tier =
            iteration_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        ordering_tier =
            ordering_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        bounds_tier =
            bounds_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        multiplicity_tier =
            multiplicity_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        threadsafe_tier =
            threadsafe_axis_tier_v<_From, _To>;
    static constexpr DConversionTier
        hierarchy_tier =
            hierarchy_axis_tier_v<_From, _To>;

    // overall tier (max of all axes)
    static constexpr DConversionTier tier =
        conversion_tier_v<_From, _To>;

    // tier-level predicates
    static constexpr bool is_view =
        ( tier == DConversionTier::view );
    static constexpr bool is_constructive =
        ( tier == DConversionTier::constructive );
    static constexpr bool is_lossy =
        ( tier == DConversionTier::lossy );
    static constexpr bool is_structural =
        ( tier == DConversionTier::structural );
    static constexpr bool is_impossible =
        ( tier == DConversionTier::impossible );

    // aggregate
    static constexpr bool is_convertible =
        is_convertible_container_pair_v<
            _From, _To>;
    static constexpr bool is_lossless =
        is_lossless_convertible_v<_From, _To>;

    // lossy detail
    static constexpr bool dedup_needed =
        needs_deduplication_v<_From, _To>;
    static constexpr bool bound_clamp_needed =
        needs_bound_clamp_v<_From, _To>;

    // structural detail
    static constexpr bool hierarchy_change =
        needs_hierarchy_change_v<_From, _To>;
    static constexpr bool ing_change =
        needs_ing_change_v<_From, _To>;
    static constexpr bool incompatible_elems =
        has_incompatible_elements_v<_From, _To>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONVERSION_TRAITS_