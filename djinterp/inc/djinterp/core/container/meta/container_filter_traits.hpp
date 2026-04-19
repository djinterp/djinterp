/******************************************************************************
* djinterp [container]                             container_filter_traits.hpp
*
* Container-aware filter traits for the djinterp framework.
*   Bridges the container classification system (container_traits.hpp) with
* the functional filtering infrastructure (filterable_traits.hpp,
* filter.hpp) to provide compile-time detection of which filter operations
* a container can support and what invariants filtering preserves.
*
*   All detection is structural SFINAE.  No tag types are required.
*
* DEPENDENCIES:
*   container_traits.hpp      - container classification
*   filterable_traits.hpp     - filterable detection primitives
*   functional_traits.hpp     - callable/predicate detection
*
* TABLE OF CONTENTS
* =================
* I.      Filterability Detection
* II.     Native Filter Detection
* III.    Filter Strategy Classification
* IV.     Invariant Preservation
* V.      Result Type Deduction
* VI.     Combined Classification
*
*
* path:      /inc/container/container_filter_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_FILTER_TRAITS_
#define DJINTERP_CONTAINER_FILTER_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include <vector>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "container_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Filterability Detection
// =============================================================================
// A container is filterable when it satisfies three structural
// requirements:
//   1. Iterable: begin()/end() are well-formed.
//   2. Typed: exposes a value_type alias.
//   3. Output-capable: supports push_back() or iterator-based
//      insert(), enabling construction of a filtered result.
//
// A container may be filter-input-only when it is iterable and
// typed but lacks output methods.  Such containers can be
// filtered into a different output container (e.g.
// std::vector).

// is_container_filterable
//   type trait: true if container supports complete filter
// round-trip (read elements, apply predicate, build result of
// the same type).
template<typename _Type>
struct is_container_filterable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type> &&
          has_value_type_v<clean_type>        &&
          ( has_push_back_v<clean_type>       ||
            has_insert_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_container_filterable_v =
    is_container_filterable<_Type>::value;

// is_filter_input_only
//   type trait: true if container can serve as filter input
// but cannot receive filtered output (no push_back/insert).
// Filtering such containers requires an explicit output
// container type.
template<typename _Type>
struct is_filter_input_only
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type>  &&
          has_value_type_v<clean_type>         &&
          !has_push_back_v<clean_type>         &&
          !has_insert_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_filter_input_only_v =
    is_filter_input_only<_Type>::value;

// is_filter_source
//   type trait: true if container can supply elements to a
// filter operation (either filterable or input-only).
template<typename _Type>
struct is_filter_source
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type> &&
          has_value_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_filter_source_v =
    is_filter_source<_Type>::value;


// =============================================================================
// II.  Native Filter Detection
// =============================================================================
// A container may provide its own .filter() member that is
// more efficient than the generic iterator-based path.
// Detection checks for a .filter(predicate) signature where
// predicate is bool(const value_type&).

NS_INTERNAL

    // native_filter_expr
    //   helper: expression alias detecting a .filter() member
    // accepting a function pointer predicate.
    template<typename _Type>
    using native_filter_expr = decltype(
        std::declval<const _Type&>().filter(
            std::declval<
                bool(*)(const typename _Type::value_type&)
            >()));

    // native_filter_check
    //   helper: SFINAE check for native .filter().
    template<typename _Type, typename = void>
    struct native_filter_check : std::false_type
    {};

    template<typename _Type>
    struct native_filter_check<_Type,
        std::void_t<native_filter_expr<_Type>>>
        : std::true_type
    {};

NS_END  // internal

// has_native_filter
//   type trait: true if container has a .filter(predicate)
// member function.
template<typename _Type>
struct has_native_filter
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::native_filter_check<clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_native_filter_v =
    has_native_filter<_Type>::value;


// =============================================================================
// III. Filter Strategy Classification
// =============================================================================
// Determines the most efficient filtering approach based on
// container capabilities:
//
//   native:        container has .filter() — delegate to it.
//   random_access: container supports operator[] — use
//                  index-based filtering.
//   bidirectional: container supports bidirectional iteration
//                  — use iterator-based filtering.
//   forward:       container supports only forward iteration
//                  — use single-pass filtering.
//   external:      container is input-only — filter into an
//                  external output container.

// DFilterStrategy
//   enum: compile-time filter strategy tags.
enum class DFilterStrategy
{
    native,
    random_access,
    bidirectional,
    forward_only,
    external,
    unsupported
};

NS_INTERNAL

    template<typename _Type, typename = void>
    struct filter_strategy_impl
    {
        static constexpr DFilterStrategy value =
            DFilterStrategy::unsupported;
    };

    // native: highest priority
    template<typename _Type>
    struct filter_strategy_impl<_Type,
        std::enable_if_t<
            has_native_filter<_Type>::value>>
    {
        static constexpr DFilterStrategy value =
            DFilterStrategy::native;
    };

    // random_access: operator[] + size + output capable
    template<typename _Type>
    struct filter_strategy_impl<_Type,
        std::enable_if_t<
            !has_native_filter<_Type>::value            &&
            is_container_filterable<_Type>::value        &&
            has_data_accessor_v<_Type>>>
    {
        static constexpr DFilterStrategy value =
            DFilterStrategy::random_access;
    };

    // bidirectional: rbegin/rend available
    template<typename _Type>
    struct filter_strategy_impl<_Type,
        std::enable_if_t<
            !has_native_filter<_Type>::value             &&
            is_container_filterable<_Type>::value         &&
            !has_data_accessor_v<_Type>                  &&
            has_reverse_iteration_v<_Type>>>
    {
        static constexpr DFilterStrategy value =
            DFilterStrategy::bidirectional;
    };

    // forward only: iterable + output capable, no reverse
    template<typename _Type>
    struct filter_strategy_impl<_Type,
        std::enable_if_t<
            !has_native_filter<_Type>::value              &&
            is_container_filterable<_Type>::value          &&
            !has_data_accessor_v<_Type>                   &&
            !has_reverse_iteration_v<_Type>>>
    {
        static constexpr DFilterStrategy value =
            DFilterStrategy::forward_only;
    };

    // external: input-only, no output methods
    template<typename _Type>
    struct filter_strategy_impl<_Type,
        std::enable_if_t<
            !has_native_filter<_Type>::value              &&
            is_filter_input_only<_Type>::value>>
    {
        static constexpr DFilterStrategy value =
            DFilterStrategy::external;
    };

NS_END  // internal

// container_filter_strategy
//   type trait: determines the most efficient filter strategy
// for the given container type.
template<typename _Type>
struct container_filter_strategy
{
    using clean_type = clean_t<_Type>;

    static constexpr DFilterStrategy value =
        internal::filter_strategy_impl<
            clean_type>::value;
};

template<typename _Type>
inline constexpr DFilterStrategy
    container_filter_strategy_v =
        container_filter_strategy<_Type>::value;

// convenience predicates on strategy

template<typename _Type>
inline constexpr bool is_natively_filterable_v =
    ( container_filter_strategy_v<_Type> ==
      DFilterStrategy::native );

template<typename _Type>
inline constexpr bool is_random_access_filterable_v =
    ( container_filter_strategy_v<_Type> ==
      DFilterStrategy::random_access );


// =============================================================================
// IV.  Invariant Preservation
// =============================================================================
// These traits describe which container invariants survive a
// filter operation.  Filtering selects a subset of elements
// without reordering, so:
//   - Ordered containers remain ordered.
//   - Sorted containers remain sorted.
//   - Unique containers remain unique.
//   - Bounded containers lose their lower bound (the result
//     may have fewer elements).

// filter_preserves_order
//   type trait: true if filtering this container produces a
// result that maintains the original element ordering.
template<typename _Type>
struct filter_preserves_order
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        is_ordered_container_v<clean_type>;
};

template<typename _Type>
inline constexpr bool filter_preserves_order_v =
    filter_preserves_order<_Type>::value;

// filter_preserves_sortedness
//   type trait: true if filtering this container produces a
// result that maintains the sorted invariant.
// A subset of a sorted sequence is still sorted.
template<typename _Type>
struct filter_preserves_sortedness
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        is_sorted_container_v<clean_type>;
};

template<typename _Type>
inline constexpr bool filter_preserves_sortedness_v =
    filter_preserves_sortedness<_Type>::value;

// filter_preserves_uniqueness
//   type trait: true if filtering this container produces a
// result that maintains element uniqueness.
// A subset of a unique set is still unique.
template<typename _Type>
struct filter_preserves_uniqueness
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        enforces_uniqueness_v<clean_type>;
};

template<typename _Type>
inline constexpr bool filter_preserves_uniqueness_v =
    filter_preserves_uniqueness<_Type>::value;

// filter_preserves_upper_bound
//   type trait: always true — the result size cannot exceed
// the source size.
template<typename _Type>
struct filter_preserves_upper_bound : std::true_type
{};

template<typename _Type>
inline constexpr bool filter_preserves_upper_bound_v = true;

// filter_preserves_lower_bound
//   type trait: always false — filtering can remove elements
// below any minimum size constraint.
template<typename _Type>
struct filter_preserves_lower_bound : std::false_type
{};

template<typename _Type>
inline constexpr bool filter_preserves_lower_bound_v = false;


// =============================================================================
// V.   Result Type Deduction
// =============================================================================
// Determines the output type of a filter operation.
//   - For containers with push_back: result is same type.
//   - For set-like containers with insert: result is same
//     type.
//   - For input-only containers: result is
//     std::vector<value_type>.

NS_INTERNAL

    // filter_result_type_impl
    //   helper: deduces filter result container type.
    template<typename _Type,
             bool = is_container_filterable<_Type>::value,
             bool = is_filter_input_only<_Type>::value>
    struct filter_result_type_impl
    {
        using type = void;
    };

    // full round-trip: same container type
    template<typename _Type>
    struct filter_result_type_impl<_Type, true, false>
    {
        using type = _Type;
    };

    // input-only: fall back to std::vector<value_type>
    template<typename _Type>
    struct filter_result_type_impl<_Type, false, true>
    {
        using type = std::vector<
            typename _Type::value_type>;
    };

NS_END  // internal

// filter_result_type
//   type trait: deduces the output container type for a
// filter operation on _Type.
template<typename _Type>
struct filter_result_type
{
    using type =
        typename internal::filter_result_type_impl<
            clean_t<_Type>>::type;
};

template<typename _Type>
using filter_result_type_t =
    typename filter_result_type<_Type>::type;


// =============================================================================
// VI.  Combined Classification
// =============================================================================

// container_filter_class
//   struct: complete filter classification of a container
// type.  All members are static constexpr.
template<typename _Type>
struct container_filter_class
{
    // filterability
    static constexpr bool is_filterable =
        is_container_filterable_v<_Type>;
    static constexpr bool is_input_only =
        is_filter_input_only_v<_Type>;
    static constexpr bool is_source =
        is_filter_source_v<_Type>;
    static constexpr bool has_native =
        has_native_filter_v<_Type>;

    // strategy
    static constexpr DFilterStrategy strategy =
        container_filter_strategy_v<_Type>;

    // invariant preservation
    static constexpr bool preserves_order =
        filter_preserves_order_v<_Type>;
    static constexpr bool preserves_sortedness =
        filter_preserves_sortedness_v<_Type>;
    static constexpr bool preserves_uniqueness =
        filter_preserves_uniqueness_v<_Type>;
    static constexpr bool preserves_upper_bound =
        filter_preserves_upper_bound_v<_Type>;
    static constexpr bool preserves_lower_bound =
        filter_preserves_lower_bound_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_FILTER_TRAITS_
