/******************************************************************************
* djinterp [container]                              container_view_traits.hpp
*
* Tier 0 (view) conversion traits for the djinterp container framework.
*   Detects whether container type _From can be reinterpreted as
* container type _To at zero cost - no data copy, no allocation, no
* runtime work.  These are purely compile-time downgrades where the
* source has more capability or a stricter invariant than the target.
*
*   View conversions exist on seven orthogonal axes:
*
*     Axis          Free direction          Blocked direction
*     ------------- ----------------------- --------------------
*     Lifetime      mutable --> immutable     immutable --> mutable
*     Iteration     stronger --> weaker       weaker --> stronger
*     Const-iter    iterator --> const_iter   const_iter --> iterator
*     Ordering      sorted --> unsorted       unsorted --> sorted
*     Bounds        bounded --> unbounded     unbounded --> bounded
*     Multiplicity  unique --> multi          multi --> unique
*     Thread safety ts --> non-ts (opt-in)    non-ts --> ts
*
*   A view conversion between two container types is possible when
* every axis is either already equal or has a free downgrade path.
* If any axis requires work (tier >= 1), the conversion is not a
* view - it belongs to a higher tier.
*
*   This module does NOT produce views or adapters.  It only
* classifies whether a view is structurally possible.  The actual
* view types belong in the container implementation modules.
*
* DEPENDENCIES:
*   container_traits.hpp          - container classification
*   container_compare_traits.hpp  - element compatibility
*   iterator_traits.hpp           - iterator level detection
*   threadsafe_container_traits.hpp - thread safety level
*
*
* path:      /inc/djinterp/core/container/traits/container_view_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      per-axis view detection (single type queries)
II.     per-axis pair view detection (From --> To)
III.    combined view convertibility
IV.     view classification
*/

#ifndef DJINTERP_CONTAINER_VIEW_TRAITS_
#define DJINTERP_CONTAINER_VIEW_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./container_traits.hpp"
#include "./container_compare_traits.hpp"
#include "./threadsafe_container_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Per-Axis View Detection (single type queries)
// ===========================================================================
// Helpers that classify a single container's position on
// each axis.  Used by the pair detectors in Section II.

// --- lifetime axis ---

// lifetime
//   enum: container lifetime classification.
enum class lifetime
{
    constexpr_storage,    // constexpr data
    immutable,            // const / non-modifiable
    mutable_storage       // modifiable
};

NS_INTERNAL

    template<typename _Type>
    struct lifetime_of_impl
    {
        using C = clean_t<_Type>;

        static constexpr lifetime value =
            is_constexpr_container_v<C>
                ? lifetime::constexpr_storage

            : is_immutable_container_v<C>
                ? lifetime::immutable

            : lifetime::mutable_storage;
    };

NS_END  // internal

template<typename _Type>
struct lifetime_of
{
    static constexpr lifetime value =
        internal::lifetime_of_impl<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr lifetime lifetime_of_v =
        lifetime_of<_Type>::value;
#endif

// --- ordering axis ---

// is_sorted_view_source
//   type trait: true if container's sorted invariant can
// be safely ignored (sorted --> unsorted is free).
template<typename _Type>
struct is_sorted_view_source
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        is_sorted_container_v<clean_type>;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _Type>
inline constexpr bool is_sorted_view_source_v =
    is_sorted_view_source<_Type>::value;

#endif
// --- multiplicity axis ---

// is_unique_view_source
//   type trait: true if container's uniqueness invariant
// can be safely ignored (unique --> multi is free).
template<typename _Type>
struct is_unique_view_source
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        is_unique_container_v<clean_type>;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _Type>
inline constexpr bool is_unique_view_source_v =
    is_unique_view_source<_Type>::value;


#endif
// ===========================================================================
// II.  Per-Axis Pair View Detection (From --> To)
// ===========================================================================
// For each axis, determines whether _From can be viewed as
// _To at zero cost.  Returns true when the axis is either
// equal or has a free downgrade path.

// --- element type axis ---

// is_element_view_compatible
//   type trait: true if _From's value_type is the same as
// _To's, or _From's is a const-qualified version of _To's.
// No implicit conversions - must be the exact type or its
// const variant.
template<typename _From,
         typename _To>
struct is_element_view_compatible
{
    static constexpr bool value =
        elements_same_type_v<_From, _To>;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_element_view_compatible_v =
        is_element_view_compatible<
            _From, _To>::value;

#endif
// --- lifetime axis ---

// is_lifetime_view_compatible
//   type trait: true if _From's lifetime can be viewed as
// _To's lifetime at zero cost.
//   constexpr --> anything:  free
//   mutable   --> immutable: free (const& view)
//   immutable --> mutable:   NOT free (requires copy)
//   same level:            free
template<typename _From,
         typename _To>
struct is_lifetime_view_compatible
{
    static constexpr lifetime from_life =
        lifetime_of_v<_From>;
    static constexpr lifetime to_life =
        lifetime_of_v<_To>;

    // constexpr is the "highest" - can view as
    // anything below
    // mutable > immutable in capability
    static constexpr bool value =
        ( from_life == to_life )                       ||
        ( from_life == lifetime::constexpr_storage )   ||
        (    from_life == lifetime::mutable_storage
          && to_life   == lifetime::immutable          );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_lifetime_view_compatible_v =
        is_lifetime_view_compatible<
            _From, _To>::value;

#endif
// --- iteration axis ---

// is_iteration_view_compatible
//   type trait: true if _From provides at least as strong
// an iterator category as _To requires.
// Stronger --> weaker is free (just use less capability).
template<typename _From,
         typename _To>
struct is_iteration_view_compatible
{
    static constexpr iterator_level from_level =
        container_iterator_level_v<_From>;
    static constexpr iterator_level to_level =
        container_iterator_level_v<_To>;

    // from_level >= to_level means _From is at least
    // as capable
    static constexpr bool value =
        ( static_cast<int>(from_level) >=
          static_cast<int>(to_level) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_iteration_view_compatible_v =
        is_iteration_view_compatible<
            _From, _To>::value;

#endif
// --- ordering axis ---

// is_ordering_view_compatible
//   type trait: true if _From --> _To requires no sorting.
//   sorted --> sorted:      free (same invariant)
//   sorted --> unsorted:    free (ignore invariant)
//   unsorted --> unsorted:  free (no invariant)
//   unsorted --> sorted:    NOT free (requires sort)
template<typename _From,
         typename _To>
struct is_ordering_view_compatible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool from_sorted =
        is_sorted_container_v<F>;
    static constexpr bool to_sorted =
        is_sorted_container_v<T>;

    static constexpr bool value =
        // same state, or from is sorted (can
        // always relax), or target doesn't require
        // sorted
        ( from_sorted == to_sorted )      ||
        ( from_sorted && !to_sorted )     ||
        ( !to_sorted );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_ordering_view_compatible_v =
        is_ordering_view_compatible<
            _From, _To>::value;

#endif
// --- bounds axis ---

// is_bounds_view_compatible
//   type trait: true if _From --> _To requires no bound
// checking.
//   bounded --> unbounded:  free (valid subset)
//   bounded --> bounded:    free if from-bounds ⊆ to-bounds
//                         (conservative: always free)
//   unbounded --> unbounded: free
//   unbounded --> bounded:  NOT free (requires validation)
template<typename _From,
         typename _To>
struct is_bounds_view_compatible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool from_bounded =
        is_bounded_container_v<F>;
    static constexpr bool to_bounded =
        is_bounded_container_v<T>;

    static constexpr bool value =
        ( !to_bounded )                   ||
        ( from_bounded == to_bounded )    ||
        ( from_bounded && !to_bounded );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_bounds_view_compatible_v =
        is_bounds_view_compatible<
            _From, _To>::value;

#endif
// --- multiplicity axis ---

// is_multiplicity_view_compatible
//   type trait: true if _From --> _To requires no
// deduplication.
//   unique --> multi:   free (unique is valid multi)
//   unique --> unique:  free
//   multi --> multi:    free
//   multi --> unique:   NOT free (requires dedup)
template<typename _From,
         typename _To>
struct is_multiplicity_view_compatible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool from_unique =
        is_unique_container_v<F>;
    static constexpr bool to_unique =
        is_unique_container_v<T>;

    static constexpr bool value =
        ( from_unique == to_unique )     ||
        ( from_unique && !to_unique )    ||
        ( !to_unique );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_multiplicity_view_compatible_v =
        is_multiplicity_view_compatible<
            _From, _To>::value;

#endif
// --- thread safety axis ---

// is_threadsafe_view_compatible
//   type trait: true if _From --> _To requires no lock
// wrapping.
//   ts --> ts:         free
//   ts --> non-ts:     free (just don't lock - opt-in)
//   non-ts --> non-ts: free
//   non-ts --> ts:     NOT free (requires mutex addition)
template<typename _From,
         typename _To>
struct is_threadsafe_view_compatible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr thread_safety_level from_ts =
        container_thread_safety_level_v<F>;
    static constexpr thread_safety_level to_ts =
        container_thread_safety_level_v<T>;

    // free if levels are equal or if from is at
    // least as safe as to (downgrade)
    static constexpr bool value =
        ( static_cast<int>(from_ts) >=
          static_cast<int>(to_ts) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_threadsafe_view_compatible_v =
        is_threadsafe_view_compatible<
            _From, _To>::value;

#endif
// --- hierarchy axis ---

// is_hierarchy_view_compatible
//   type trait: true if _From --> _To requires no
// structural transformation.
//   flat --> flat:           free
//   hierarchical --> hier:   free
//   flat ↔ hierarchical:   NOT free (structural change)
template<typename _From,
         typename _To>
struct is_hierarchy_view_compatible
{
    using F = clean_t<_From>;
    using T = clean_t<_To>;

    static constexpr bool from_hier =
        is_hierarchical_container_v<F>;
    static constexpr bool to_hier =
        is_hierarchical_container_v<T>;

    static constexpr bool value =
        ( from_hier == to_hier );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool
    is_hierarchy_view_compatible_v =
        is_hierarchy_view_compatible<
            _From, _To>::value;


#endif
// ===========================================================================
// III. Combined View Convertibility
// ===========================================================================

// is_view_convertible
//   type trait: true if _From can be viewed as _To at
// zero cost on ALL axes simultaneously.  This is the
// Tier 0 gate: if this is true, no data movement is
// needed.
template<typename _From,
         typename _To>
struct is_view_convertible
{
    static constexpr bool value =
        ( is_element_view_compatible_v<
              _From, _To>               &&
          is_lifetime_view_compatible_v<
              _From, _To>               &&
          is_iteration_view_compatible_v<
              _From, _To>               &&
          is_ordering_view_compatible_v<
              _From, _To>               &&
          is_bounds_view_compatible_v<
              _From, _To>               &&
          is_multiplicity_view_compatible_v<
              _From, _To>               &&
          is_threadsafe_view_compatible_v<
              _From, _To>               &&
          is_hierarchy_view_compatible_v<
              _From, _To> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _From,
         typename _To>
inline constexpr bool is_view_convertible_v =
    is_view_convertible<_From, _To>::value;

#endif
// is_symmetric_view
//   type trait: true if the view conversion is free in
// both directions (types are view-equivalent).
template<typename _A,
         typename _B>
struct is_symmetric_view
{
    static constexpr bool value =
        ( is_view_convertible_v<_A, _B> &&
          is_view_convertible_v<_B, _A> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename _A,
         typename _B>
inline constexpr bool is_symmetric_view_v =
    is_symmetric_view<_A, _B>::value;


#endif
// ===========================================================================
// IV.  View Classification
// ===========================================================================

// view_axis_count
//   type trait: counts how many axes require no work
// (are view-compatible) for the From --> To pair.
template<typename _From,
         typename _To>
struct view_axis_count
{
    static constexpr std::size_t value =
        is_element_view_compatible_v<
            _From, _To>              +
        is_lifetime_view_compatible_v<
            _From, _To>              +
        is_iteration_view_compatible_v<
            _From, _To>              +
        is_ordering_view_compatible_v<
            _From, _To>              +
        is_bounds_view_compatible_v<
            _From, _To>              +
        is_multiplicity_view_compatible_v<
            _From, _To>              +
        is_threadsafe_view_compatible_v<
            _From, _To>              +
        is_hierarchy_view_compatible_v<
            _From, _To>;
};

template<typename _From,
         typename _To>
inline constexpr std::size_t view_axis_count_v =
    view_axis_count<_From, _To>::value;

static constexpr std::size_t
    D_VIEW_AXIS_TOTAL = 8;

// blocked_axis_count
//   type trait: counts how many axes BLOCK a view
// conversion (require tier >= 1 work).
template<typename _From,
         typename _To>
struct blocked_axis_count
{
    static constexpr std::size_t value =
        D_VIEW_AXIS_TOTAL -
            view_axis_count_v<_From, _To>;
};

template<typename _From,
         typename _To>
inline constexpr std::size_t blocked_axis_count_v =
    blocked_axis_count<_From, _To>::value;

// container_view_class
//   struct: complete Tier 0 classification for a From --> To
// container pair.
template<typename _From,
         typename _To>
struct container_view_class
{
    // per-axis
    static constexpr bool elem_ok =
        is_element_view_compatible_v<
            _From, _To>;
    static constexpr bool lifetime_ok =
        is_lifetime_view_compatible_v<
            _From, _To>;
    static constexpr bool iteration_ok =
        is_iteration_view_compatible_v<
            _From, _To>;
    static constexpr bool ordering_ok =
        is_ordering_view_compatible_v<
            _From, _To>;
    static constexpr bool bounds_ok =
        is_bounds_view_compatible_v<
            _From, _To>;
    static constexpr bool multiplicity_ok =
        is_multiplicity_view_compatible_v<
            _From, _To>;
    static constexpr bool threadsafe_ok =
        is_threadsafe_view_compatible_v<
            _From, _To>;
    static constexpr bool hierarchy_ok =
        is_hierarchy_view_compatible_v<
            _From, _To>;

    // aggregate
    static constexpr bool is_view =
        is_view_convertible_v<_From, _To>;
    static constexpr bool is_symmetric =
        is_symmetric_view_v<_From, _To>;
    static constexpr std::size_t free_axes =
        view_axis_count_v<_From, _To>;
    static constexpr std::size_t blocked_axes =
        blocked_axis_count_v<_From, _To>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_VIEW_TRAITS_