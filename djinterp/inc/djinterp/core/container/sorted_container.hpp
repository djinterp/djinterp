/******************************************************************************
* djinterp [container]                                     sorted_container.hpp
*
*   The SORTEDNESS axis (the spec, Sortedness), layered on the Order axis.
* Sortedness presupposes positions: a container is SORTED with respect to a
* comparator iff it is ordered and, along its positions, comp(e_i, e_{i+1}) is
* non-descending; it is UNSORTED iff ordered but not in the comparator's order
* (it still has an order, just not that one).  For an UNORDERED container
* positional sortedness is NOT APPLICABLE - it has no positions to be sorted -
* yet a comparator-equipped one (a set, a map) admits a unique MONOTONE
* ENUMERATION, "sorted by construction" as an invariant of how it enumerates
* rather than a checkable property of stored positions.
*
*   At the type level the axis reads:
*     non_container    - not an (iterable) container;
*     unordered        - unordered, no comparator: positional sortedness N/A, no
*                        monotone enumeration (a hash-ordered set / map);
*     monotone         - unordered but comparator-equipped: sorted-by-construction
*                        enumeration (an ordered set / map / multiset / multimap);
*     order_dependent  - ordered: sortedness is a property of the INSTANCE, not of
*                        the type (a plain sequence may or may not be sorted); and
*     sorted           - ordered AND guaranteed in comparator order - a closed
*                        interval (arithmetic, monotone by construction) or a
*                        sequence that asserts a sorted invariant (opt-in).
*
*   For an order_dependent container the property is checkable at runtime, which
* is what is_sorted_range performs; for a monotone or sorted type, enumeration
* yields comparator order with no check needed.  A comparator is detected as a
* key_compare alias; a sorted sequence opts in through a static `sorted_invariant`
* constant; and interval bounds mark the arithmetic sorted case.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/sorted_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_SORTED_
#define DJINTERP_CONTAINER_SORTED_ 1

// std
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"              // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"     // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "./ordered_container.hpp"      // is_ordered_container, is_unordered_container (Order axis)


NS_DJINTERP


// ===========================================================================
// I.   Sortedness signals
// ===========================================================================

NS_INTERNAL

    // has_key_compare_helper
    //   helper: detects a `key_compare` alias - the comparator of an ordered
    // associative container.  Its presence is what lets an (unordered) associative
    // enumerate monotonically; a hash-ordered container has no such comparator.
    template<typename _Type,
             typename = void>
    struct has_key_compare_helper : std::false_type
    {};

    template<typename _Type>
    struct has_key_compare_helper<_Type,
        D_VOID_T<typename clean_t<_Type>::key_compare>>
        : std::true_type
    {};

    // has_interval_bounds_helper
    //   helper: detects static `lower_bound` AND `upper_bound` - a closed-interval
    // carrier, whose arithmetic enumeration is monotone by construction.
    template<typename _Type,
             typename = void>
    struct has_interval_bounds_helper : std::false_type
    {};

    template<typename _Type>
    struct has_interval_bounds_helper<_Type,
        D_VOID_T<decltype(clean_t<_Type>::lower_bound),
                 decltype(clean_t<_Type>::upper_bound)>>
        : std::true_type
    {};

    // sorted_invariant_helper
    //   helper: reads the opt-in static `sorted_invariant` constant, by which an
    // ordered sequence asserts it is maintained in sorted order.
    template<typename _Type,
             typename = void>
    struct sorted_invariant_helper
    {
        static constexpr bool value = false;
    };

    template<typename _Type>
    struct sorted_invariant_helper<_Type,
        D_VOID_T<decltype(clean_t<_Type>::sorted_invariant)>>
    {
        static constexpr bool value =
            static_cast<bool>(clean_t<_Type>::sorted_invariant);
    };

NS_END  // internal


// ===========================================================================
// II.  Sortedness classification
// ===========================================================================

// sortedness
//   enum: a container's position on the sortedness axis.
enum class sortedness
{
    non_container,    // not an (iterable) container
    unordered,        // unordered, no comparator: sortedness not applicable
    monotone,         // unordered but comparator-equipped: sorted-by-construction enumeration
    order_dependent,  // ordered: sortedness is a property of the instance
    sorted            // ordered AND guaranteed in comparator order (interval / opt-in)
};

// sortedness_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
sortedness_name(sortedness _s) noexcept
{
    return ( _s == sortedness::non_container   ? "non_container"
           : _s == sortedness::unordered       ? "unordered"
           : _s == sortedness::monotone        ? "monotone"
           : _s == sortedness::order_dependent ? "order_dependent"
           :                                     "sorted" );
}

// sortedness_of
//   trait: classifies a type.  An unordered container is monotone when it carries
// a comparator, else unordered; an ordered container is sorted when it guarantees
// comparator order (interval bounds or an opt-in invariant), else order_dependent.
template<typename _Type>
struct sortedness_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr sortedness value =
        ( is_unordered_container<clean_type>::value )
              ? ( internal::has_key_compare_helper<clean_type>::value
                      ? sortedness::monotone
                      : sortedness::unordered )
      : ( is_ordered_container<clean_type>::value )
              ? ( (    internal::has_interval_bounds_helper<clean_type>::value
                    || internal::sorted_invariant_helper<clean_type>::value )
                      ? sortedness::sorted
                      : sortedness::order_dependent )
      :         sortedness::non_container;

    using type = std::integral_constant<sortedness, value>;
};

template<typename _Type>
using sortedness_of_t = typename sortedness_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr sortedness sortedness_of_v =
        sortedness_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr sortedness sortedness_of_v =
        sortedness_of<_Type>::value;
#endif


// ===========================================================================
// III. Classification predicates
// ===========================================================================

// is_sorted_container
//   trait: true iff the type guarantees comparator order along its positions -
// the ordered, sorted-by-construction case (the sorted restriction at type level).
template<typename _Type>
struct is_sorted_container
    : std::integral_constant<bool,
          sortedness_of<clean_t<_Type>>::value == sortedness::sorted>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_sorted_container)

// is_monotone_container
//   trait: true iff the type is a comparator-equipped unordered container - it
// has no positions, but its enumeration is sorted by construction.
template<typename _Type>
struct is_monotone_container
    : std::integral_constant<bool,
          sortedness_of<clean_t<_Type>>::value == sortedness::monotone>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_monotone_container)

// admits_sorted_enumeration
//   trait: true iff enumerating the container is guaranteed to yield comparator
// order - either a monotone (associative) or a sorted (ordered) type.  This is
// the property that lets such a container be PRESENTED in sorted order.
template<typename _Type>
struct admits_sorted_enumeration
    : std::integral_constant<bool,
            is_sorted_container<clean_t<_Type>>::value
         || is_monotone_container<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(admits_sorted_enumeration)


// ===========================================================================
// IV.  Instance-level sortedness (the checkable property)
// ===========================================================================

// is_sorted_range
//   function: for an ORDERED container - whose sortedness is a property of the
// instance - reports whether THIS container's elements are in non-descending
// order along their positions.  (For a monotone or sorted type no check is
// needed; enumeration is already in comparator order.)
template<typename _Container>
typename std::enable_if<
    is_ordered_container<_Container>::value,
    bool
>::type
is_sorted_range(
    const _Container& _container
)
{
    return std::is_sorted(std::begin(_container), std::end(_container));
}

// is_sorted_range (custom comparator)
//   function: as above, against a supplied comparator.
template<typename _Container,
         typename _Compare>
typename std::enable_if<
    is_ordered_container<_Container>::value,
    bool
>::type
is_sorted_range(
    const _Container& _container,
    _Compare          _cmp
)
{
    return std::is_sorted(std::begin(_container), std::end(_container), _cmp);
}


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct sorted_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_comparator =
        internal::has_key_compare_helper<clean_type>::value;
    static constexpr bool interval_domain =
        internal::has_interval_bounds_helper<clean_type>::value;
    static constexpr bool sorted_invariant =
        internal::sorted_invariant_helper<clean_type>::value;

    static constexpr sortedness kind =
        sortedness_of<clean_type>::value;
    static constexpr const char* kind_name =
        sortedness_name(kind);

    static constexpr bool is_sorted_type =
        is_sorted_container<clean_type>::value;
    static constexpr bool is_monotone =
        is_monotone_container<clean_type>::value;
    static constexpr bool sorted_enumeration =
        admits_sorted_enumeration<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_SORTED_
