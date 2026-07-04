/******************************************************************************
* djinterp [container]                            container_conversion_traits.hpp
*
*   Conversion - the carrying of a container of one type to another - is an
* operation FOUNDED on comparison: it is classified by the content level it
* preserves and by its definedness, exactly the readings of the comparison
* model.  This module reads a From -> To pair off the comparison profile and the
* element relation and says how the conversion stands.
*
*   Four tiers, coarsest work to greatest, the maximum concern the pair raises:
*
*     VIEW         a reinterpretation - no data moves.  From already offers
*                  everything To asks: the same elements, the same content level
*                  and backing shape, a discipline no weaker (dropping a
*                  restriction is free).  A stronger thing read through a weaker
*                  interface.
*
*     CONSTRUCTIVE data moves but is preserved - an O(n)/O(n log n) build.  A
*                  differing backing (vector to list), a converting element copy,
*                  or a lossless invariant added (unsorted to sorted, by the
*                  monotone enumeration of Order).  Total, and faithful at the
*                  level the two share.
*
*     LOSSY        the conversion FORGETS - it factors through a strict
*                  coarsening.  The canonical quotients: forget order (a sequence
*                  to a multiset), forget multiplicity (a multiset to a set,
*                  i.e. deduplication).  Adding a capacity bound is the guarded
*                  case - defined only where the count already fits.
*
*     STRUCTURAL   a topology change - flat <-> hierarchical - a full
*                  reconstruction, not a level of the flat content hierarchy.
*
*   The tier obeys the discipline order of the model: a move toward a WEAKER
* overlay is total and lossless (view or constructive), a move toward a STRONGER
* one is guarded (constructive, where the invariant holds) or collapsing (lossy).
*
*   RELIABILITY.  ``No data moves'' needs the backing shape, which the profile
* alone does not carry - a vector and a list share a profile yet differ in
* layout.  VIEW therefore additionally requires an identical sequential layout,
* the framework's backing-shape proxy; the distinction is a type-level
* approximation, honest at the sequence layouts and conservative elsewhere.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language, as elsewhere.
*
*
* path:      /inc/djinterp/core/container/traits/container_conversion_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONVERSION_TRAITS_
#define DJINTERP_CONTAINER_CONVERSION_TRAITS_ 1

// std
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_VOID_T, detection macros
#include "./container_comparison_traits.hpp"        // profile: native level, refinement
#include "./element_relation_traits.hpp"            // elements_same / elements_convertible
#include "./bounded_container_traits.hpp"           // is_bounded_container (capacity)
#include "./ordered_container_traits.hpp"                 // sequential_layout_of (backing proxy)


NS_DJINTERP


// ===========================================================================
// I.   Conversion tier
// ===========================================================================

// conversion_tier
//   enum: the cost/loss class of a conversion - the greatest concern the pair
// raises.  Ordered by severity; `none` is off the scale (no conversion exists).
enum class conversion_tier
{
    view,           // zero-cost reinterpretation
    constructive,   // O(n) / O(n log n), data preserved
    lossy,          // data forgotten (a coarsening, or a capacity clamp)
    structural,     // topology change (flat <-> hierarchical)
    none            // no conversion path
};

// conversion_tier_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
conversion_tier_name(conversion_tier _t) noexcept
{
    return ( _t == conversion_tier::view         ? "view"
           : _t == conversion_tier::constructive ? "constructive"
           : _t == conversion_tier::lossy        ? "lossy"
           : _t == conversion_tier::structural   ? "structural"
           :                                       "none" );
}


// ===========================================================================
// II.  Construction-path detection
// ===========================================================================

NS_INTERNAL

    // range_constructible_helper
    //   helper: To can be built from From's iterator range, To(begin, end).
    template<typename _To, typename _From, typename = void>
    struct range_constructible_helper : std::false_type {};
    template<typename _To, typename _From>
    struct range_constructible_helper<_To, _From,
        D_VOID_T<decltype(_To(
            std::begin(std::declval<const _From&>()),
            std::end  (std::declval<const _From&>())))>> : std::true_type {};

    // range_insertable_helper
    //   helper: To accepts From's range through insert(begin, end).
    template<typename _To, typename _From, typename = void>
    struct range_insertable_helper : std::false_type {};
    template<typename _To, typename _From>
    struct range_insertable_helper<_To, _From,
        D_VOID_T<decltype(std::declval<_To&>().insert(
            std::begin(std::declval<const _From&>()),
            std::end  (std::declval<const _From&>())))>> : std::true_type {};

NS_END  // internal

// is_range_constructible
//   trait: To can be constructed from From's range, To(begin, end).
template<typename _From,
         typename _To>
struct is_range_constructible
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_From>>::value
         && internal::range_constructible_helper<
                clean_t<_To>, clean_t<_From>>::value>
{};

// is_range_insertable
//   trait: To accepts From's range through insert(begin, end).
template<typename _From,
         typename _To>
struct is_range_insertable
    : std::integral_constant<bool,
            is_iterable_container<clean_t<_From>>::value
         && internal::range_insertable_helper<
                clean_t<_To>, clean_t<_From>>::value>
{};

// has_constructive_path
//   trait: some data-preserving construction of To from From exists.
template<typename _From,
         typename _To>
struct has_constructive_path
    : std::integral_constant<bool,
            is_range_constructible<_From, _To>::value
         || is_range_insertable<_From, _To>::value>
{};


// ===========================================================================
// III. Per-concern analysis
// ===========================================================================

// elements_compatible
//   trait: From's elements are the same as, or convertible to, To's - the
// prerequisite of any conversion.
template<typename _From,
         typename _To>
struct elements_compatible
    : std::integral_constant<bool,
            elements_same_type<_From, _To>::value
         || elements_convertible<_From, _To>::value>
{};

// conversion_forgets_content
//   trait: To's native content level is strictly coarser than From's - the
// conversion factors through a coarsening (forget order, forget multiplicity).
template<typename _From,
         typename _To>
struct conversion_forgets_content
    : std::integral_constant<bool,
            ( content_level_rank(
                  native_content_level_of<clean_t<_To>>::value )
            < content_level_rank(
                  native_content_level_of<clean_t<_From>>::value ) )>
{};

// conversion_clamps_capacity
//   trait: To bounds a capacity From leaves unbounded - a guarded conversion,
// defined only where the count already fits.
template<typename _From,
         typename _To>
struct conversion_clamps_capacity
    : std::integral_constant<bool,
            !is_bounded_container<clean_t<_From>>::value
         &&  is_bounded_container<clean_t<_To>>::value>
{};

// conversion_changes_topology
//   trait: From and To sit on opposite sides of the flat / hierarchical divide -
// a reconstruction, not a level of the flat content hierarchy.
template<typename _From,
         typename _To>
struct conversion_changes_topology
    : std::integral_constant<bool,
            ( is_hierarchical_container<clean_t<_From>>::value
           != is_hierarchical_container<clean_t<_To>>::value )>
{};

NS_INTERNAL

    // is_reinterpretation_helper
    //   helper: the VIEW predicate.  No data moves iff the elements are the
    // same, the content level and backing layout are identical, no topology
    // changes, each realization axis is EQUIVALENT (a differing lifetime or
    // storage betrays a materialization - a std::array is contiguous like a
    // std::vector yet compile-staged and inline, so array -> vector is a copy),
    // and From's discipline is no weaker (dropping a restriction is free).
    template<typename _From, typename _To>
    struct is_reinterpretation_helper
        : std::integral_constant<bool,
                elements_same_type<_From, _To>::value
             && ( native_content_level_of<clean_t<_From>>::value
               == native_content_level_of<clean_t<_To>>::value )
             && !conversion_changes_topology<_From, _To>::value
             && ( sequential_layout_of<clean_t<_From>>::value
               == sequential_layout_of<clean_t<_To>>::value )
             && ( lifetime_relation<_From, _To>::value
                      == order_relation::equivalent )
             && ( storage_relation<_From, _To>::value
                      == order_relation::equivalent )
             && ( iterability_relation<_From, _To>::value
                      == order_relation::equivalent )
             && (    discipline_relation<_From, _To>::value
                         == order_relation::equivalent
                  || discipline_relation<_From, _To>::value
                         == order_relation::greater )>
    {};

NS_END  // internal


// ===========================================================================
// IV.  Conversion classification
// ===========================================================================

// conversion_tier_of
//   trait: the tier of a From -> To conversion, the greatest concern the pair
// raises.  A topology change is judged first - it relates the leaves, not the
// top-level elements, so it stands apart from element compatibility; otherwise
// incompatible elements admit no conversion, and among compatible ones a
// forgetting or capacity clamp dominates, then a free reinterpretation, and
// failing those a data-preserving construction (or none, if none can build To).
template<typename _From,
         typename _To>
struct conversion_tier_of
{
private:
    using from_type = clean_t<_From>;
    using to_type   = clean_t<_To>;

public:
    static constexpr conversion_tier value =
        ( conversion_changes_topology<from_type, to_type>::value )
              ? conversion_tier::structural
      : ( !elements_compatible<from_type, to_type>::value )
              ? conversion_tier::none
      : ( conversion_forgets_content<from_type, to_type>::value
       || conversion_clamps_capacity<from_type, to_type>::value )
              ? conversion_tier::lossy
      : ( internal::is_reinterpretation_helper<from_type, to_type>::value )
              ? conversion_tier::view
      : ( has_constructive_path<from_type, to_type>::value )
              ? conversion_tier::constructive
      :         conversion_tier::none;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _From, typename _To>
    inline constexpr conversion_tier conversion_tier_of_v =
        conversion_tier_of<_From, _To>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _From, typename _To>
    constexpr conversion_tier conversion_tier_of_v =
        conversion_tier_of<_From, _To>::value;
#endif

// is_convertible_between
//   trait: any conversion path exists (the tier is not `none`).
template<typename _From,
         typename _To>
struct is_convertible_between
    : std::integral_constant<bool,
          conversion_tier_of<_From, _To>::value != conversion_tier::none>
{};

// is_view_conversion
//   trait: the conversion is a zero-cost reinterpretation.
template<typename _From,
         typename _To>
struct is_view_conversion
    : std::integral_constant<bool,
          conversion_tier_of<_From, _To>::value == conversion_tier::view>
{};

// is_lossless_conversion
//   trait: the conversion preserves all content - a view or a construction.
template<typename _From,
         typename _To>
struct is_lossless_conversion
    : std::integral_constant<bool,
            conversion_tier_of<_From, _To>::value == conversion_tier::view
         || conversion_tier_of<_From, _To>::value == conversion_tier::constructive>
{};

// is_lossy_conversion
//   trait: the conversion forgets content or clamps a capacity.
template<typename _From,
         typename _To>
struct is_lossy_conversion
    : std::integral_constant<bool,
          conversion_tier_of<_From, _To>::value == conversion_tier::lossy>
{};

// conversion_requires_restructuring
//   trait: the conversion crosses the flat / hierarchical divide.
template<typename _From,
         typename _To>
struct conversion_requires_restructuring
    : std::integral_constant<bool,
          conversion_tier_of<_From, _To>::value == conversion_tier::structural>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _From, typename _To>
    inline constexpr bool is_convertible_between_v =
        is_convertible_between<_From, _To>::value;
    template<typename _From, typename _To>
    inline constexpr bool is_view_conversion_v =
        is_view_conversion<_From, _To>::value;
    template<typename _From, typename _To>
    inline constexpr bool is_lossless_conversion_v =
        is_lossless_conversion<_From, _To>::value;
    template<typename _From, typename _To>
    inline constexpr bool is_lossy_conversion_v =
        is_lossy_conversion<_From, _To>::value;
    template<typename _From, typename _To>
    inline constexpr bool conversion_requires_restructuring_v =
        conversion_requires_restructuring<_From, _To>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _From, typename _To>
    constexpr bool is_convertible_between_v =
        is_convertible_between<_From, _To>::value;
    template<typename _From, typename _To>
    constexpr bool is_view_conversion_v =
        is_view_conversion<_From, _To>::value;
    template<typename _From, typename _To>
    constexpr bool is_lossless_conversion_v =
        is_lossless_conversion<_From, _To>::value;
    template<typename _From, typename _To>
    constexpr bool is_lossy_conversion_v =
        is_lossy_conversion<_From, _To>::value;
    template<typename _From, typename _To>
    constexpr bool conversion_requires_restructuring_v =
        conversion_requires_restructuring<_From, _To>::value;
#endif

// conversion_class
//   trait: the assembled classification of a From -> To conversion - its tier,
// whether the elements are compatible at all, whether a construction path
// exists, whether it is lossless, and the content level the two share (the
// finest a lossless conversion preserves).
template<typename _From,
         typename _To>
struct conversion_class
{
private:
    using from_type = clean_t<_From>;
    using to_type   = clean_t<_To>;

public:
    static constexpr conversion_tier tier =
        conversion_tier_of<from_type, to_type>::value;
    static constexpr bool elements_ok =
        elements_compatible<from_type, to_type>::value;
    static constexpr bool has_path =
        has_constructive_path<from_type, to_type>::value;
    static constexpr bool lossless =
        is_lossless_conversion<from_type, to_type>::value;
    static constexpr content_level shared_content_level =
        content_level_coarser(
            native_content_level_of<from_type>::value,
            native_content_level_of<to_type>::value );
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONVERSION_TRAITS_
