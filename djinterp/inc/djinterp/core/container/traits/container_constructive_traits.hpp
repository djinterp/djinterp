/******************************************************************************
* djinterp [container]                       container_constructive_traits.hpp
*
* Tier 1 (constructive) conversion traits for the djinterp container
* framework.
*   Detects whether container type _From can be converted to _To with
* O(n) or O(n log n) work while preserving all data.  No elements are
* discarded or modified - only the container's structural properties
* change.
*
*   Constructive conversions exist on six axes:
*
*     Axis          Direction               Cost
*     ------------- ----------------------- -----------
*     Lifetime      immutable --> mutable     O(n) copy
*     Ordering      unsorted --> sorted       O(n log n)
*     Thread safety non-ts --> ts             O(n) copy + mutex wrap
*     underlying  vector --> deque (etc.)   O(n) copy
*     Element type  convertible A --> B       O(n) converting copy
*     Iterator-range any iterable --> target  O(n) range construction
*
*   Conversions that discard data (multi --> unique, unbounded -->
* bounded) are Tier 2 (lossy) and belong in
* container_conversion_traits.hpp.  Structural changes that alter
* the topology (flat ↔ hierarchical) are Tier 3.
*
* DEPENDENCIES:
*   container_traits.hpp              - container classification
*   container_compare_traits.hpp      - element compatibility
*   container_view_traits.hpp         - Tier 0 detection
*   iterator_traits.hpp               - iterator level detection
*   threadsafe_container_traits.hpp   - thread safety level
*
* TABLE OF CONTENTS
* =================
* I.      Per-Axis Constructive Detection
* II.     Iterator-Range Construction Detection
* III.    Combined Constructive Convertibility
* IV.     Constructive Classification
*
*
* path:      /inc/djinterp/core/container/traits/
*                container_constructive_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONSTRUCTIVE_TRAITS_
#define DJINTERP_CONTAINER_CONSTRUCTIVE_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./container_traits.hpp"
#include "./container_compare_traits.hpp"
#include "./container_view_traits.hpp"
#include "./threadsafe_container_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Per-Axis Constructive Detection
// ===========================================================================
// For each axis, determines whether _From can be
// converted to _To with data-preserving work.  These
// return true ONLY for conversions that are tier 1
// (not tier 0 / view, and not tier 2+ / lossy).

// --- lifetime axis ---

// needs_lifetime_copy
//   type trait: true if _From --> _To requires a copy to
// promote from immutable to mutable.  This is the only
// lifetime conversion that preserves data.
template<typename _From,
         typename _To>
struct needs_lifetime_copy
{
    static constexpr lifetime from_life =
        lifetime_of_v<_From>;
    static constexpr lifetime to_life =
        lifetime_of_v<_To>;

    static constexpr bool value =
        (    !is_lifetime_view_compatible_v<_From, _To>
          && ( to_life == lifetime::mutable_storage )
          && (    from_life == lifetime::immutable
               || from_life == lifetime::constexpr_storage ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_lifetime_copy_v =
    needs_lifetime_copy<_From, _To>::value;

#endif
// --- ordering axis ---

// needs_sort
//   type trait: true if _From is unsorted and _To
// requires sorted - needs O(n log n) sort.  Only
// possible if elements support <.
template<typename _From,
         typename _To>
struct needs_sort
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_ordering_view_compatible_v<
              _From, _To>                        &&
          !is_sorted_container_v<F>              &&
          is_sorted_container_v<T>               &&
          has_less_than_comparable_elements_v<F> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_sort_v =
    needs_sort<_From, _To>::value;

#endif
// --- thread safety axis ---

// needs_lock_wrap
//   type trait: true if _From is non-threadsafe and _To
// requires thread safety - needs mutex addition.
template<typename _From,
         typename _To>
struct needs_lock_wrap
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( !is_threadsafe_view_compatible_v<
              _From, _To>                        &&
          ( container_thread_safety_level_v<F> ==
            thread_safety_level::none )           &&
          ( container_thread_safety_level_v<T> !=
            thread_safety_level::none ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_lock_wrap_v =
    needs_lock_wrap<_From, _To>::value;

#endif
// --- element type axis ---

// needs_element_conversion
//   type trait: true if _From's value_type is implicitly
// convertible to _To's value_type but not the same type.
// Requires O(n) converting copy.
template<typename _From,
         typename _To>
struct needs_element_conversion
{
    static constexpr bool value =
        ( !elements_same_type_v<_From, _To>     &&
          elements_convertible_v<_From, _To> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool needs_element_conversion_v =
    needs_element_conversion<_From, _To>::value;


#endif
// ===========================================================================
// II.  Iterator-Range Construction Detection
// ===========================================================================
// Detects whether _To can be constructed from _From's
// iterator range.  This is the universal fall for
// constructive conversion when element types are
// compatible.

NS_INTERNAL

    // detect: To(It, It) construction from begin/end
    template<typename _To,
             typename _From,
             typename = void>
    struct is_range_constructible_check : std::false_type
    {};

    template<typename _To,
             typename _From>
    struct is_range_constructible_check<_To, _From,
        std::void_t<decltype(
            _To(std::begin(std::declval<const _From&>()),
                std::end(std::declval<const _From&>())))>>
        : std::true_type
    {};

    // detect: To has insert(It, It) for range insertion
    template<typename _To,
             typename _From,
             typename = void>
    struct has_range_insert_check : std::false_type
    {};

    template<typename _To,
             typename _From>
    struct has_range_insert_check<_To, _From,
        std::void_t<decltype(
            std::declval<_To&>().insert(
                std::begin(
                    std::declval<const _From&>()),
                std::end(
                    std::declval<const _From&>())))>>
        : std::true_type
    {};

NS_END  // internal

// is_range_constructible
//   type trait: true if _To can be constructed from
// _From's iterator range via To(begin, end).
template<typename _From,
         typename _To>
struct is_range_constructible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( is_iterable_v<F> &&
          internal::is_range_constructible_check<
              T, F>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_range_constructible_v =
    is_range_constructible<_From, _To>::value;

#endif
// is_range_insertable
//   type trait: true if _To accepts a range via
// insert(begin, end).
template<typename _From,
         typename _To>
struct is_range_insertable
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( is_iterable_v<F> &&
          internal::has_range_insert_check<
              T, F>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_range_insertable_v =
    is_range_insertable<_From, _To>::value;

#endif
// has_constructive_path
//   type trait: true if _From --> _To has any data-
// preserving construction path (range construct, range
// insert, or element-wise push_ with compatible
// types).
template<typename _From,
         typename _To>
struct has_constructive_path
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool value =
        ( is_range_constructible_v<F, T>        ||
          is_range_insertable_v<F, T>           ||
          ( is_iterable_v<F>                    &&
            ( has_push__v<T> ||
              has_insert_v<T> )                 &&
            ( elements_same_type_v<F, T> ||
              elements_convertible_v<F, T> ) ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool has_constructive_path_v =
    has_constructive_path<_From, _To>::value;


#endif
// ===========================================================================
// III. Combined Constructive Convertibility
// ===========================================================================

// is_constructive_convertible
//   type trait: true if _From can be converted to _To
// with data-preserving work (Tier 1).
// Requires:
//   1. NOT already a view (Tier 0).
//   2. Element types are compatible.
//   3. A construction path exists.
//   4. No lossy axes are triggered (multi-->unique,
//      unbounded-->bounded) - those are Tier 2.
template<typename _From,
         typename _To>
struct is_constructive_convertible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    // check that no lossy axes are triggered
    static constexpr bool no_dedup_needed =
        is_multiplicity_view_compatible_v<F, T>;
    static constexpr bool no_bound_clamp =
        is_bounds_view_compatible_v<F, T>;
    static constexpr bool no_structural_change =
        is_hierarchy_view_compatible_v<F, T>;

    static constexpr bool value =
        ( !is_view_convertible_v<F, T>          &&
          ( elements_same_type_v<F, T> ||
            elements_convertible_v<F, T> )      &&
          has_constructive_path_v<F, T>         &&
          no_dedup_needed                       &&
          no_bound_clamp                        &&
          no_structural_change );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_constructive_convertible_v =
    is_constructive_convertible<_From, _To>::value;


#endif
// ===========================================================================
// IV.  Constructive Classification
// ===========================================================================

// container_constructive_class
//   struct: complete Tier 1 classification for a
// From --> To container pair.
template<typename _From,
         typename _To>
struct container_constructive_class
{
    // per-axis needs
    static constexpr bool lifetime_copy =
        needs_lifetime_copy_v<_From, _To>;
    static constexpr bool sort_needed =
        needs_sort_v<_From, _To>;
    static constexpr bool lock_wrap =
        needs_lock_wrap_v<_From, _To>;
    static constexpr bool elem_convert =
        needs_element_conversion_v<_From, _To>;

    // construction paths
    static constexpr bool range_construct =
        is_range_constructible_v<_From, _To>;
    static constexpr bool range_insert =
        is_range_insertable_v<_From, _To>;
    static constexpr bool has_path =
        has_constructive_path_v<_From, _To>;

    // aggregate
    static constexpr bool is_constructive =
        is_constructive_convertible_v<_From, _To>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONSTRUCTIVE_TRAITS_