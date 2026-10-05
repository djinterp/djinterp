/*******************************************************************************
* djinterp [core]                                           container_traits.hpp
*
*  djinterp container compile-time classification traits
*   SFINAE-based detection and classification of container properties,
* including:
*   - lifetime (constexpr/immutable/mutable),
*   - iteration (constexpr_iterator/const_iterator/iterator),
*   - bounds (via interval types or legacy min/max accessors),
*   - storage (static/dynamic)
*   - capabilities
*   - ordering, sorted invariants, structural depth
*     (flat/hierarchical), element multiplicity, underlying container
*     relationships, and reverse iteration support.
*   All detection is purely structural: no tag types are required.
* Containers declare properties through their public interface and type
* aliases.  The math::interval type is used to express bounded ranges
* for size, depth, and multiplicity when available.
*   All traits operate on the `clean_t` (cv-ref stripped) form of the
* type and produce `static constexpr bool` values.  C++17 `_v` variable
* templates are provided for every public trait.
*
* CONTAINER PROTOCOL (optional members detected):
*   size bounds:         using size_interval = interval<...>;
*                     or min_size() / max_size() / size() / capacity()
*   depth bounds:        using depth_interval = interval<...>;
*                     or depth_type / max_depth() / min_depth()
*   multiplicity:        using multiplicity_interval = interval<...>;
*                     or multiplicity_min() / multiplicity_max()
*   sorted invariant:    key_compare member alias
*   hierarchical:        parent() / children() / root() / node_type
*   underlying:          underlying_container_type member alias
*
* THREAD SAFETY:
*   Thread-safe container traits are defined separately in
* `threadsafe_container_traits.hpp`.
*
*
* path:      /inc/djinterp/core/container/traits/container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.03.09
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    method detection
      ----------------

2.    constexpr detection (internal)
      ------------------------------

3.    public constexpr traits
      -----------------------

4.    array detection
      ---------------

5.    lifetime classification
      -----------------------

6.    bounds classification (interval-aware)
      --------------------------------------

7.    storage classification
      ----------------------

8.    iteration classification
      ------------------------

9.    ordering classification
      -----------------------

10.   sorted classification
      ---------------------

12.   reverse iteration classification
      --------------------------------

13.   uniqueness and multiplicity classification
      ------------------------------------------

14.   structure classification (flat / hierarchical)
      ----------------------------------------------

15.   underlying container classification
      -----------------------------------

16.   interval extraction
      -------------------

17.   combined classification
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_TRAITS_CONTAINER_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_CONTAINER_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/trait_detect.hpp"
#include "../../meta/type_traits.hpp"
#include "../../meta/member_types.hpp"
#include "../../../math/interval/interval.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./container_storage_traits.hpp"  // is_static_storage_container
#include "./bounded_container_traits.hpp"
#include "./iterable_container_traits.hpp"
#include "./mutable_container_traits.hpp"
#include "./constexpr_container_traits.hpp"
#include "./runtime_container_traits.hpp"
#include "./sorted_container_traits.hpp"
#include "./node_container_traits.hpp"
#include "./container_overlay_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Method Detection
// ===========================================================================

// --- size and capacity ---
// has_data_method
//   trait: detects a const member `data()`.
D_TYPE_TRAIT_TRUE(has_data_method,
    decltype(std::declval<const Type&>().data()))

// has_size_accessor
//   trait: detects a const member `size()`.
D_TYPE_TRAIT_TRUE(has_size_accessor,
    decltype(std::declval<const Type&>().size()))

D_TYPE_TRAIT_TRUE(has_max_size_accessor,
    decltype(std::declval<const Type&>().max_size()))

D_TYPE_TRAIT_TRUE(has_min_size_accessor,
    decltype(std::declval<const Type&>().min_size()))

D_TYPE_TRAIT_TRUE(has_capacity_accessor,
    decltype(std::declval<const Type&>().capacity()))

// --- type members ---
//   has_allocator_type, has_mapped_type (and has_value_type below, plus
// has_key_type) are owned by meta/member_types.hpp, included above, and
// re-exported through it. The local duplicates were removed to end the
// one-definition-rule conflict with that canonical detection header.

D_TYPE_TRAIT_TRUE(has_key_compare,
    typename Type::key_compare)

D_TYPE_TRAIT_TRUE(has_value_compare_type,
    typename Type::value_compare)

D_TYPE_TRAIT_TRUE(has_hasher_type,
    typename Type::hasher)

// --- interval protocol detection ---
D_TYPE_TRAIT_TRUE(has_size_interval_type,
    typename Type::size_interval)

D_TYPE_TRAIT_TRUE(has_depth_interval_type,
    typename Type::depth_interval)

D_TYPE_TRAIT_TRUE(has_multiplicity_interval_type,
    typename Type::multiplicity_interval)

// --- underlying container detection ---
// has_underlying_container_type
//   trait: detects an `underlying_container_type` member alias - the mark of a
// container defined in terms of another (a container adaptor, or a backed
// container that delegates its storage rather than owning it).
D_TYPE_TRAIT_HAS_TYPE(has_underlying_container_type, underlying_container_type)

// is_underlying_container
//   trait: true iff the container is underlying-based - it exposes an
// `underlying_container_type`, so it delegates storage instead of owning it.
// This is the complement of is_fundamental_container (below) and the signal
// container_class reports as `is_underlying`. (The underlying_container_type
// extractor lives further down this section.)
template<typename Type>
struct is_underlying_container
    : std::integral_constant<bool,
          has_underlying_container_type<clean_t<Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_underlying_container)

// --- hierarchical structure detection ---
//   `has_node_type` is owned by node_container_traits.hpp and re-exported
// through the include chain above.

D_TYPE_TRAIT_TRUE(has_depth_type,
    typename Type::depth_type)

D_TYPE_TRAIT_TRUE(has_parent_accessor,
    decltype(std::declval<const Type&>().parent()))

D_TYPE_TRAIT_TRUE(has_children_accessor,
    decltype(std::declval<const Type&>().children()))

D_TYPE_TRAIT_TRUE(has_root_accessor,
    decltype(std::declval<const Type&>().root()))

D_TYPE_TRAIT_TRUE(has_max_depth_accessor,
    decltype(std::declval<const Type&>().max_depth()))

D_TYPE_TRAIT_TRUE(has_min_depth_accessor,
    decltype(std::declval<const Type&>().min_depth()))

D_TYPE_TRAIT_TRUE(has_depth_accessor,
    decltype(std::declval<const Type&>().depth()))

// --- multiplicity detection ---

D_TYPE_TRAIT_TRUE(has_multiplicity_min_accessor,
    decltype(
        std::declval<const Type&>().multiplicity_min()))

D_TYPE_TRAIT_TRUE(has_multiplicity_max_accessor,
    decltype(
        std::declval<const Type&>().multiplicity_max()))

// --- mutability detection ---

D_TYPE_TRAIT_TRUE(has_push_back,
    decltype(std::declval<Type&>().push_back(
        std::declval<typename Type::value_type>())))

D_TYPE_TRAIT_TRUE(has_insert,
    decltype(std::declval<Type&>().insert(
        std::declval<typename Type::const_iterator>(),
        std::declval<typename Type::value_type>())))

D_TYPE_TRAIT_TRUE(has_erase,
    decltype(std::declval<Type&>().erase(
        std::declval<typename Type::const_iterator>())))

D_TYPE_TRAIT_TRUE(has_clear,
    decltype(std::declval<Type&>().clear()))

// --- iteration detection ---

D_TYPE_TRAIT_TRUE(has_begin_accessor,
    decltype(std::declval<Type&>().begin()))

D_TYPE_TRAIT_TRUE(has_end_accessor,
    decltype(std::declval<Type&>().end()))

D_TYPE_TRAIT_TRUE(has_cbegin_accessor,
    decltype(std::declval<const Type&>().cbegin()))

D_TYPE_TRAIT_TRUE(has_cend_accessor,
    decltype(std::declval<const Type&>().cend()))

// --- reverse iteration detection ---

D_TYPE_TRAIT_TRUE(has_rbegin_accessor,
    decltype(std::declval<Type&>().rbegin()))

D_TYPE_TRAIT_TRUE(has_rend_accessor,
    decltype(std::declval<Type&>().rend()))

// --- uniqueness detection ---

D_TYPE_TRAIT_TRUE(has_unique_accessor,
    decltype(std::declval<Type&>().unique()))

// --- sorted invariant detection ---

D_TYPE_TRAIT_TRUE(has_value_comp_method,
    decltype(
        std::declval<const Type&>().value_comp()))

D_TYPE_TRAIT_TRUE(has_key_comp_method,
    decltype(
        std::declval<const Type&>().key_comp()))

// --- value_type detection ---
//   has_value_type is owned by meta/member_types.hpp (included above); the
// local duplicate was removed. See the type-members note above.


// ===========================================================================
// II.  Constexpr Detection (internal)
// ===========================================================================

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct has_constexpr_size_helper : std::false_type
    {};

    template<typename Type>
    struct has_constexpr_size_helper<Type, std::enable_if_t<
        std::is_default_constructible_v<Type> &&
        has_size_accessor_v<Type>>>
    {
    private:
        template<typename U>
        static constexpr auto test(int)
            -> std::bool_constant<(U{}.size(), true)>
        {
            return {};
        }

        template<typename>
        static constexpr std::false_type test(...)
        {
            return {};
        }

    public:
        static constexpr bool value =
            decltype(test<Type>(0))::value;
    };

    template<typename Type,
             typename = void>
    struct has_constexpr_max_size_helper : std::false_type
    {};

    template<typename Type>
    struct has_constexpr_max_size_helper<Type, std::enable_if_t<
        std::is_default_constructible_v<Type> &&
        has_max_size_accessor_v<Type>>>
    {
    private:
        template<typename U>
        static constexpr auto test(int)
            -> std::bool_constant<
                   (U{}.max_size(), true)>
        {
            return {};
        }

        template<typename>
        static constexpr std::false_type test(...)
        {
            return {};
        }

    public:
        static constexpr bool value =
            decltype(test<Type>(0))::value;
    };

    template<typename Type,
             typename = void>
    struct has_constexpr_min_size_helper : std::false_type
    {};

    template<typename Type>
    struct has_constexpr_min_size_helper<Type,
        std::enable_if_t<
        ( std::is_default_constructible_v<Type> &&
          has_min_size_accessor_v<Type> )
    >>
    {
    private:
        template<typename U>
        static constexpr auto test(int)
            -> std::bool_constant<
                   (U{}.min_size(), true)>
        {
            return {};
        }

        template<typename>
        static constexpr std::false_type test(...)
        {
            return {};
        }

    public:
        static constexpr bool value =
            decltype(test<Type>(0))::value;
    };

    // fixed size: size() == max_size() always
    template<typename Type,
             bool = has_constexpr_size_helper<Type>::value &&
                    has_constexpr_max_size_helper<Type>::value>
    struct has_fixed_size_helper : std::false_type
    {};

    template<typename Type>
    struct has_fixed_size_helper<Type, true>
        : std::bool_constant<
              (Type{}.size() == Type{}.max_size())>
    {};

    // bounded capacity: max_size() < SIZE_MAX/2
    template<typename Type,
             bool = has_constexpr_max_size_helper<Type>::value>
    struct has_bounded_capacity_helper : std::false_type
    {};

    template<typename Type>
    struct has_bounded_capacity_helper<Type, true>
        : std::bool_constant<
              (Type{}.max_size() <
               (std::numeric_limits<std::size_t>::max()
                / 2))>
    {};

    // bounded minimum: has min_size() with nonzero value
    template<typename Type,
             bool = has_constexpr_min_size_helper<Type>::value>
    struct has_bounded_minimum_helper : std::false_type
    {};

    template<typename Type>
    struct has_bounded_minimum_helper<Type, true>
        : std::bool_constant<(Type{}.min_size() > 0)>
    {};

    // interval validity checks: verify exposed interval types actually satisfy
    // is_interval from interval_traits.hpp.
    template<typename Type,
             typename = void>
    struct has_valid_size_interval : std::false_type
    {};

    template<typename Type>
    struct has_valid_size_interval<Type,
        std::enable_if_t<
            has_size_interval_type_v<Type> &&
            math::is_interval<
                typename Type::size_interval>::value>>
        : std::true_type
    {};

    template<typename Type,
             typename = void>
    struct has_valid_depth_interval : std::false_type
    {};

    template<typename Type>
    struct has_valid_depth_interval<Type,
        std::enable_if_t<
            has_depth_interval_type_v<Type> &&
            math::is_interval<
                typename Type::depth_interval>::value>>
        : std::true_type
    {};

    template<typename Type,
             typename = void>
    struct has_valid_multiplicity_interval : std::false_type
    {};

    template<typename Type>
    struct has_valid_multiplicity_interval<Type,
        std::enable_if_t<
            has_multiplicity_interval_type_v<Type> &&
            math::is_interval<typename Type::multiplicity_interval>::value>
    > : std::true_type
    {};

NS_END  // internal


// ===========================================================================
// III. Public Constexpr Traits
// ===========================================================================

// --- size ---

template<typename Type>
struct has_constexpr_size
    : std::bool_constant<
          internal::has_constexpr_size_helper<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_constexpr_size_v =
    has_constexpr_size<Type>::value;

#endif
template<typename Type>
struct has_constexpr_max_size
    : std::bool_constant<
          internal::has_constexpr_max_size_helper<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_constexpr_max_size_v =
    has_constexpr_max_size<Type>::value;

#endif
template<typename Type>
struct has_constexpr_min_size
    : std::bool_constant<
          internal::has_constexpr_min_size_helper<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_constexpr_min_size_v =
    has_constexpr_min_size<Type>::value;

#endif
template<typename Type>
struct has_fixed_size
    : std::bool_constant<
          internal::has_fixed_size_helper<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_fixed_size_v =
    has_fixed_size<Type>::value;

#endif
template<typename Type>
struct has_bounded_capacity
    : std::bool_constant<
          internal::has_bounded_capacity_helper<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_bounded_capacity_v =
    has_bounded_capacity<Type>::value;

#endif
template<typename Type>
struct has_bounded_minimum
    : std::bool_constant<
          internal::has_bounded_minimum_helper<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_bounded_minimum_v =
    has_bounded_minimum<Type>::value;

#endif
// --- interval validity ---

template<typename Type>
struct has_valid_size_interval
    : std::bool_constant<
          internal::has_valid_size_interval<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_valid_size_interval_v =
    has_valid_size_interval<Type>::value;

#endif
template<typename Type>
struct has_valid_depth_interval
    : std::bool_constant<
          internal::has_valid_depth_interval<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_valid_depth_interval_v =
    has_valid_depth_interval<Type>::value;

#endif
template<typename Type>
struct has_valid_multiplicity_interval
    : std::bool_constant<
          internal::has_valid_multiplicity_interval<
              clean_t<Type>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_valid_multiplicity_interval_v =
    has_valid_multiplicity_interval<Type>::value;


#endif
// ===========================================================================
// IV.  Array Detection
// ===========================================================================

template<typename Type>
struct is_c_array : std::false_type
{};

template<typename Type, std::size_t N>
struct is_c_array<Type[N]> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_c_array_v =
    is_c_array<Type>::value;

#endif
template<typename Type>
struct is_std_array : std::false_type
{};

template<typename Type, std::size_t N>
struct is_std_array<std::array<Type, N>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_std_array_v =
    is_std_array<Type>::value;


#endif
// ===========================================================================
// V.   Lifetime Classification
// ===========================================================================

// is_compile_time_container
//   type trait: true if container exists only at compile time
// (constexpr_<container>). Detection: empty type + no runtime state +
// constexpr everything.
template<typename Type>
struct is_compile_time_container
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( std::is_empty_v<clean_type>       &&
          has_constexpr_size_v<clean_type>  &&
          !has_allocator_type_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_compile_time_container_v =
    is_compile_time_container<Type>::value;

#endif
// is_immutable_container, is_mutable_container
//   These are owned by mutable_container_traits.hpp and re-exported via the
// include chain at the top of this header. Earlier revisions defined them here
// as well; the duplicate
// definitions caused ODR conflicts when both headers were pulled into the same
// TU.

// is_runtime_container
//   type trait: true if container exists at runtime (mutable or immutable, but
// not compile-time only).
//
//   Note: a more strongly-typed version of this trait lives in
// runtime_container_traits.hpp keyed on the constexpr-container signal. That
// version is the canonical "is this a non- constexpr container shape?"
// predicate. The local version below is intentionally weaker - it answers only
// "does this type fail the empty-stateless-constexpr test of
// is_compile_time_container?" - and is kept here as a convenience for callers
// already operating in this header's vocabulary. To avoid the ODR conflict the
// two used to have, the local version is renamed to
// `is_not_compile_time_container` and `is_runtime_container` resolves through
// the include chain to the runtime_container_traits.hpp definition.
template<typename Type>
struct is_not_compile_time_container
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        !is_compile_time_container_v<clean_type>;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_not_compile_time_container_v =
    is_not_compile_time_container<Type>::value;


#endif
// ===========================================================================
// VI.  Bounds Classification (interval-aware)
// ===========================================================================
// Boundedness is determined in order of priority:
//   1. Interval protocol: if container exposes a size_interval
//      type satisfying math::is_interval, the interval's
//      lower_bound and upper_bound encode the constraints.
//   2. Legacy accessors: min_size() / max_size() / capacity().
//   3. Fixed-size arrays: C-arrays and std::array.

// is_lower_bounded
//   type trait: true if container has a minimum size constraint.
template<typename Type>
struct is_lower_bounded
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_valid_size_interval_v<clean_type> ||
          has_bounded_minimum_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_lower_bounded_v =
    is_lower_bounded<Type>::value;

#endif
// is_upper_bounded
//   type trait: true if container has a maximum size (capacity) constraint.
template<typename Type>
struct is_upper_bounded
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_valid_size_interval_v<clean_type>  ||
          is_c_array_v<clean_type>              ||
          is_std_array_v<clean_type>            ||
          has_fixed_size_v<clean_type>          ||
          has_bounded_capacity_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_upper_bounded_v =
    is_upper_bounded<Type>::value;

#endif

// is_unbounded
//   type trait: true if container has no size constraints.
template<typename Type>
struct is_unbounded
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( !is_lower_bounded_v<clean_type> &&
          !is_upper_bounded_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_unbounded_v =
    is_unbounded<Type>::value;


#endif

// is_interval_bounded
//   type trait: true if container has any size constraint (lower or upper).
// Convenience aggregate used by container_class so callers don't need to OR
// the two orientations themselves.
//
//   Distinct from `is_bounded_container_v` in bounded_container_traits.hpp:
// that umbrella checks for fixed-extent / max_size / fixed-capacity signals
// generically; this one composes the interval-aware is_lower_bounded /
// is_upper_bounded defined immediately above. Both produce the same answer for
// canonical STL containers; the local version is preferred where the
// surrounding code is already using interval-aware bounds.
//
//   NOTE: named `is_interval_bounded` (not `is_bounded`) to avoid colliding
// with the generic two-parameter meta-trait `djinterp::is_bounded<Type,
// Trait>` in meta/type_traits.hpp, which this header includes.
template<typename Type>
struct is_interval_bounded
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( is_lower_bounded_v<clean_type> ||
          is_upper_bounded_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr bool is_interval_bounded_v =
        is_interval_bounded<Type>::value;

#endif
// ===========================================================================
// VII. Storage Classification
// ===========================================================================

// is_static_storage / is_dynamic_storage
//   DELETED -- they SHADOWED the predicate they were meant to agree with.
// meta/storage.hpp declares `constexpr bool
// is_static_storage(storage_duration)` and container_storage_traits.hpp
// defines its trait BY CALLING it:
//     is_static_storage_container<T>
//         : integral_constant<bool,
//               is_static_storage(storage_duration_of<T>::value)>
// A class template of the same name here both redeclared the function and
// broke that call ('expected ( after template-argument-list'), so this header
// and container_storage_traits.hpp could not share a TU. storage.hpp already
// suffixes its type-level form _type and container_storage_traits.hpp suffixes
// its _container; this header carried neither. Use those.

// ===========================================================================
// IX.  Ordering Classification
// ===========================================================================

// is_ordered_container, is_unordered_container (with ordering_kind_of and
// sequential_layout_of)
//   These are owned by ordered_container_traits.hpp and re-exported via the
// include chain at the top of this header (sorted_container_traits.hpp pulls
// ordered_container_traits.hpp). Earlier revisions defined is_ordered_container
// and is_unordered_container here as well; the duplicate definitions caused ODR
// conflicts when both headers were pulled into the same TU.  The canonical
// definitions classify on the full Order axis (positional identity) rather than
// this header's old iterator-shape heuristic, so they agree on canonical STL
// types and are strictly more precise for user-defined ones.
// ===========================================================================
// X.   Sorted Classification
// ===========================================================================
// A sorted container maintains a sorted invariant over its
// elements.  Detection is purely structural:
//   1. Presence of key_compare member alias indicates the
//      container maintains a comparison-based ordering.
//   2. Hash-based containers (hasher type) are explicitly
//      excluded even if they also expose key_compare.

// is_sorted_container, is_unsorted_container
//   These are owned by sorted_container_traits.hpp and re-exported via the
// include chain at the top of this header. Earlier revisions defined them here
// as well; the duplicate
// definitions caused ODR conflicts when both headers were pulled into the same
// TU. Note that the canonical definitions in sorted_container_traits.hpp use a
// slightly broader signal set (key_compare alias, value_compare alias, or
// opt-in is_sorted_container tag) than this header used to,
// so callers see the same answer for canonical STL types and strictly more
// answers for user-defined ones.


// ===========================================================================
// XI.  Reverse Iteration Classification
// ===========================================================================
//
//   The canonical `has_reverse_iteration` / `has_reverse_iteration_v`
// trait lives in iterator_traits.hpp (which this header includes
// transitively).  An earlier revision of this header carried a
// duplicate definition here; it has been removed to eliminate the
// resulting one-definition-rule conflict.  Use the iterator_traits.hpp
// version directly - the snapshot in section XVII below references it
// by its public name, no aliasing or forwarding required.


// ===========================================================================
// XII. Uniqueness and Multiplicity Classification
// ===========================================================================
// Multiplicity describes how many copies of an equivalent
// element a container allows.
//
// Detection priority:
//   1. Interval protocol: multiplicity_interval type alias
//      satisfying is_interval encodes [min, max].
//   2. Legacy accessors: multiplicity_min() /
//      multiplicity_max().
//   3. Structural: key_type without mapped_type implies
//      set-like uniqueness.

// enforces_uniqueness
//   type trait: true if container enforces element uniqueness (sets, maps with
// unique keys).
template<typename Type>
struct enforces_uniqueness
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_key_type_v<clean_type> &&
          !has_mapped_type_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool enforces_uniqueness_v =
    enforces_uniqueness<Type>::value;

#endif
// allows_duplicates
//   type trait: true if container allows duplicate elements.
template<typename Type>
struct allows_duplicates
{
    static constexpr bool value =
        !enforces_uniqueness_v<Type>;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool allows_duplicates_v =
    allows_duplicates<Type>::value;

#endif
// has_bounded_multiplicity
//   type trait: true if container constrains element multiplicity, either via
// the interval protocol or legacy accessor pair.
template<typename Type>
struct has_bounded_multiplicity
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_valid_multiplicity_interval_v<clean_type> ||
          ( has_multiplicity_min_accessor_v<clean_type> &&
            has_multiplicity_max_accessor_v<clean_type> ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool has_bounded_multiplicity_v =
    has_bounded_multiplicity<Type>::value;

#endif

// is_fundamental_container
//   type trait: true if container provides its own storage. C-arrays and
// std::array are always fundamental. Any container without
// underlying_container_type is fundamental.
template<typename Type>
struct is_fundamental_container
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( is_c_array_v<clean_type>   ||
          is_std_array_v<clean_type> ||
          !is_underlying_container_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
template<typename Type>
inline constexpr bool is_fundamental_container_v =
    is_fundamental_container<Type>::value;

#endif
// underlying_container_type_of
//   type trait: extracts the underlying container type if present, otherwise
// yields void.
NS_INTERNAL
    template<typename Type,
             typename = void>
    struct underlying_container_type_of_helper
    {
        using type = void;
    };

    template<typename Type>
    struct underlying_container_type_of_helper<
        Type,
        std::void_t<typename Type::underlying_container_type>>
    {
        using type =
            typename Type::underlying_container_type;
    };

NS_END  // internal

template<typename Type>
struct underlying_container_type_of
{
    using type =
        typename internal::underlying_container_type_of_helper<
            clean_t<Type>>::type;
};

// underlying_container_type_of_t
//   type: the carrier of underlying_container_type_of -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using underlying_container_type_of_t =
    typename underlying_container_type_of<Type>::type;


// ===========================================================================
// XV.  Interval Extraction
// ===========================================================================
// Type aliases that extract the interval types from containers
// that declare them, yielding void when absent.

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct size_interval_of_helper
    {
        using type = void;
    };

    template<typename Type>
    struct size_interval_of_helper<Type,
        std::enable_if_t<
            has_valid_size_interval<Type>::value>>
    {
        using type = typename Type::size_interval;
    };

    template<typename Type,
             typename = void>
    struct depth_interval_of_helper
    {
        using type = void;
    };

    template<typename Type>
    struct depth_interval_of_helper<Type,
        std::enable_if_t<
            has_valid_depth_interval<Type>::value>>
    {
        using type = typename Type::depth_interval;
    };

    template<typename Type,
             typename = void>
    struct multiplicity_interval_of_helper
    {
        using type = void;
    };

    template<typename Type>
    struct multiplicity_interval_of_helper<Type,
        std::enable_if_t<
            has_valid_multiplicity_interval<
                Type>::value>>
    {
        using type =
            typename Type::multiplicity_interval;
    };

NS_END  // internal

// size_interval_of
//   type trait: yields the container's size_interval type, or void if none is
// declared.
template<typename Type>
struct size_interval_of
{
    using type =
        typename internal::size_interval_of_helper<
            clean_t<Type>>::type;
};

// size_interval_of_t
//   type: the carrier of size_interval_of -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using size_interval_of_t =
    typename size_interval_of<Type>::type;

// depth_interval_of
//   type trait: yields the container's depth_interval type, or void if none is
// declared.
template<typename Type>
struct depth_interval_of
{
    using type = typename internal::depth_interval_of_helper<clean_t<Type>>::type;
};

// depth_interval_of_t
//   type: the carrier of depth_interval_of -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using depth_interval_of_t =
    typename depth_interval_of<Type>::type;

// multiplicity_interval_of
//   type trait: yields the container's multiplicity_interval type, or void if
// none is declared.
template<typename Type>
struct multiplicity_interval_of
{
    using type = typename internal::multiplicity_interval_of_helper<
            clean_t<Type>
    >::type;
};

// multiplicity_interval_of_t
//   type: the carrier of multiplicity_interval_of -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using multiplicity_interval_of_t =
    typename multiplicity_interval_of<Type>::type;


// ===========================================================================
// XVI. Combined Classification
// ===========================================================================

// container_class
//   struct: complete classification of a container type. All classification is
// compile-time using static constexpr bool members.
template<typename Type>
struct container_class
{
    // lifetime classification
    static constexpr bool is_compile_time       = is_compile_time_container_v<Type>;
    static constexpr bool is_immutable          = is_immutable_container_v<Type>;
    static constexpr bool is_mutable            = is_mutable_container_v<Type>;
    static constexpr bool is_runtime            = is_runtime_container_v<Type>;
    // bounds classification
    static constexpr bool is_unbounded          = is_unbounded_v<Type>;
    static constexpr bool is_lower_bounded      = is_lower_bounded_v<Type>;
    static constexpr bool is_upper_bounded      = is_upper_bounded_v<Type>;
    static constexpr bool is_bounded            = is_interval_bounded_v<Type>;
    static constexpr bool has_size_interval     = has_valid_size_interval_v<Type>;
    // storage classification
    static constexpr bool is_static_storage     = is_static_storage_container_v<Type>;
    static constexpr bool is_dynamic_storage    = is_dynamic_storage_container_v<Type>;
    // iteration classification
    static constexpr bool is_iterable           = is_iterable_container_v<Type>;
    static constexpr bool is_const_iterable     = is_const_iterable_container_v<Type>;
    static constexpr bool is_non_iterable       = is_non_iterable_container_v<Type>;
    static constexpr bool has_reverse_iteration = has_reverse_iteration_v<Type>;
    // ordering classification
    static constexpr bool is_ordered            = is_ordered_container_v<Type>;
    static constexpr bool is_unordered          = is_unordered_container_v<Type>;
    // sorted classification
    static constexpr bool is_sorted             = is_sorted_container_v<Type>;
    static constexpr bool is_unsorted           = is_unsorted_container_v<Type>;
    // uniqueness and multiplicity
    static constexpr bool enforces_uniqueness   = enforces_uniqueness_v<Type>;
    static constexpr bool allows_duplicates     = allows_duplicates_v<Type>;
    static constexpr bool has_bounded_multiplicity  = has_bounded_multiplicity_v<Type>;
    static constexpr bool has_multiplicity_interval = has_valid_multiplicity_interval_v<Type>;
    // underlying container classification
    static constexpr bool is_underlying         = is_underlying_container_v<Type>;
    static constexpr bool is_fundamental        = is_fundamental_container_v<Type>;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_CONTAINER_TRAITS_HPP
