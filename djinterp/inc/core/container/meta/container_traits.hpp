/******************************************************************************
* djinterp [container]                                    container_traits.hpp
*
* Compile-time container classification traits for the djinterp framework.
*   Provides SFINAE-based detection and classification of container
* properties including lifetime (constexpr/immutable/mutable), iteration
* capabilities, bounds (via interval types or legacy min/max accessors),
* storage (static/dynamic), ordering, sorted invariants, structural depth
* (flat/hierarchical), element multiplicity, backing container
* relationships, and reverse iteration support.
*
*   All detection is purely structural: no tag types are required.
* Containers declare properties through their public interface and type
* aliases.  The maths::interval type is used to express bounded ranges
* for size, depth, and multiplicity when available.
*
*   All traits operate on the `clean_t` (cv-ref stripped) form of the
* type and produce `static constexpr bool` values.  C++17 `_v` variable
* templates are provided for every public trait.
*
* CONTAINER PROTOCOL (optional members detected):
*   Size bounds:         using size_interval = interval<...>;
*                     or min_size() / max_size() / size() / capacity()
*   Depth bounds:        using depth_interval = interval<...>;
*                     or depth_type / max_depth() / min_depth()
*   Multiplicity:        using multiplicity_interval = interval<...>;
*                     or multiplicity_min() / multiplicity_max()
*   Sorted invariant:    key_compare member alias
*   Hierarchical:        parent() / children() / root() / node_type
*   Backing:             backing_container_type member alias
*
* TABLE OF CONTENTS
* =================
* I.      Method Detection
* II.     Constexpr Detection (internal)
* III.    Public Constexpr Traits
* IV.     Array Detection
* V.      Lifetime Classification
* VI.     Bounds Classification (interval-aware)
* VII.    Storage Classification
* VIII.   Iteration Classification
* IX.     Ordering Classification
* X.      Sorted Classification
* XI.     Reverse Iteration Classification
* XII.    Uniqueness and Multiplicity Classification
* XIII.   Structure Classification (flat / hierarchical)
* XIV.    Backing Container Classification
* XV.     Interval Extraction
* XVI.    Combined Classification
*
* THREAD SAFETY:
*   Thread-safe container traits are defined separately in
* threadsafe_container_traits.hpp.
*
* path:      \inc\container\meta\container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2024.03.09
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_
#define	DJINTERP_CONTAINER_TRAITS_ 1

#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "..\maths\interval_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Method Detection
// =============================================================================

// --- size and capacity ---

D_TYPE_TRAIT_TRUE(has_size_accessor,
    decltype(std::declval<const _Type&>().size()))

D_TYPE_TRAIT_TRUE(has_max_size_accessor,
    decltype(std::declval<const _Type&>().max_size()))

D_TYPE_TRAIT_TRUE(has_min_size_accessor,
    decltype(std::declval<const _Type&>().min_size()))

D_TYPE_TRAIT_TRUE(has_capacity_accessor,
    decltype(std::declval<const _Type&>().capacity()))

D_TYPE_TRAIT_TRUE(has_data_accessor,
    decltype(std::declval<const _Type&>().data()))

// --- type members ---

D_TYPE_TRAIT_TRUE(has_allocator_type,
    typename _Type::allocator_type)

D_TYPE_TRAIT_TRUE(has_key_type,
    typename _Type::key_type)

D_TYPE_TRAIT_TRUE(has_mapped_type,
    typename _Type::mapped_type)

D_TYPE_TRAIT_TRUE(has_key_compare,
    typename _Type::key_compare)

D_TYPE_TRAIT_TRUE(has_value_compare_type,
    typename _Type::value_compare)

D_TYPE_TRAIT_TRUE(has_hasher_type,
    typename _Type::hasher)

// --- interval protocol detection ---

D_TYPE_TRAIT_TRUE(has_size_interval_type,
    typename _Type::size_interval)

D_TYPE_TRAIT_TRUE(has_depth_interval_type,
    typename _Type::depth_interval)

D_TYPE_TRAIT_TRUE(has_multiplicity_interval_type,
    typename _Type::multiplicity_interval)

// --- backing container detection ---

D_TYPE_TRAIT_TRUE(has_backing_container_type,
    typename _Type::backing_container_type)

// --- hierarchical structure detection ---

D_TYPE_TRAIT_TRUE(has_node_type,
    typename _Type::node_type)

D_TYPE_TRAIT_TRUE(has_depth_type,
    typename _Type::depth_type)

D_TYPE_TRAIT_TRUE(has_parent_accessor,
    decltype(std::declval<const _Type&>().parent()))

D_TYPE_TRAIT_TRUE(has_children_accessor,
    decltype(std::declval<const _Type&>().children()))

D_TYPE_TRAIT_TRUE(has_root_accessor,
    decltype(std::declval<const _Type&>().root()))

D_TYPE_TRAIT_TRUE(has_max_depth_accessor,
    decltype(std::declval<const _Type&>().max_depth()))

D_TYPE_TRAIT_TRUE(has_min_depth_accessor,
    decltype(std::declval<const _Type&>().min_depth()))

D_TYPE_TRAIT_TRUE(has_depth_accessor,
    decltype(std::declval<const _Type&>().depth()))

// --- multiplicity detection ---

D_TYPE_TRAIT_TRUE(has_multiplicity_min_accessor,
    decltype(
        std::declval<const _Type&>().multiplicity_min()))

D_TYPE_TRAIT_TRUE(has_multiplicity_max_accessor,
    decltype(
        std::declval<const _Type&>().multiplicity_max()))

// --- mutability detection ---

D_TYPE_TRAIT_TRUE(has_push_back,
    decltype(std::declval<_Type&>().push_back(
        std::declval<typename _Type::value_type>())))

D_TYPE_TRAIT_TRUE(has_insert,
    decltype(std::declval<_Type&>().insert(
        std::declval<typename _Type::const_iterator>(),
        std::declval<typename _Type::value_type>())))

D_TYPE_TRAIT_TRUE(has_erase,
    decltype(std::declval<_Type&>().erase(
        std::declval<typename _Type::const_iterator>())))

D_TYPE_TRAIT_TRUE(has_clear,
    decltype(std::declval<_Type&>().clear()))

// --- iteration detection ---

D_TYPE_TRAIT_TRUE(has_begin_accessor,
    decltype(std::declval<_Type&>().begin()))

D_TYPE_TRAIT_TRUE(has_end_accessor,
    decltype(std::declval<_Type&>().end()))

D_TYPE_TRAIT_TRUE(has_cbegin_accessor,
    decltype(std::declval<const _Type&>().cbegin()))

D_TYPE_TRAIT_TRUE(has_cend_accessor,
    decltype(std::declval<const _Type&>().cend()))

// --- reverse iteration detection ---

D_TYPE_TRAIT_TRUE(has_rbegin_accessor,
    decltype(std::declval<_Type&>().rbegin()))

D_TYPE_TRAIT_TRUE(has_rend_accessor,
    decltype(std::declval<_Type&>().rend()))

// --- uniqueness detection ---

D_TYPE_TRAIT_TRUE(has_unique_accessor,
    decltype(std::declval<_Type&>().unique()))

// --- sorted invariant detection ---

D_TYPE_TRAIT_TRUE(has_value_comp_method,
    decltype(
        std::declval<const _Type&>().value_comp()))

D_TYPE_TRAIT_TRUE(has_key_comp_method,
    decltype(
        std::declval<const _Type&>().key_comp()))

// --- value_type detection ---

D_TYPE_TRAIT_TRUE(has_value_type,
    typename _Type::value_type)


// =============================================================================
// II.  Constexpr Detection (internal)
// =============================================================================

NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_constexpr_size_impl : std::false_type
    {};

    template<typename _Type>
    struct has_constexpr_size_impl<_Type, std::enable_if_t<
        std::is_default_constructible_v<_Type> &&
        has_size_accessor_v<_Type>>>
    {
    private:
        template<typename _U>
        static constexpr auto test(int)
            -> std::bool_constant<(_U{}.size(), true)>
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
            decltype(test<_Type>(0))::value;
    };

    template<typename _Type, typename = void>
    struct has_constexpr_max_size_impl : std::false_type
    {};

    template<typename _Type>
    struct has_constexpr_max_size_impl<_Type, std::enable_if_t<
        std::is_default_constructible_v<_Type> &&
        has_max_size_accessor_v<_Type>>>
    {
    private:
        template<typename _U>
        static constexpr auto test(int)
            -> std::bool_constant<
                   (_U{}.max_size(), true)>
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
            decltype(test<_Type>(0))::value;
    };

    template<typename _Type, typename = void>
    struct has_constexpr_min_size_impl : std::false_type
    {};

    template<typename _Type>
    struct has_constexpr_min_size_impl<_Type, std::enable_if_t<
        std::is_default_constructible_v<_Type> &&
        has_min_size_accessor_v<_Type>>>
    {
    private:
        template<typename _U>
        static constexpr auto test(int)
            -> std::bool_constant<
                   (_U{}.min_size(), true)>
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
            decltype(test<_Type>(0))::value;
    };

    // fixed size: size() == max_size() always
    template<typename _Type,
             bool = has_constexpr_size_impl<_Type>::value &&
                    has_constexpr_max_size_impl<_Type>::value>
    struct has_fixed_size_impl : std::false_type
    {};

    template<typename _Type>
    struct has_fixed_size_impl<_Type, true>
        : std::bool_constant<
              (_Type{}.size() == _Type{}.max_size())>
    {};

    // bounded capacity: max_size() < SIZE_MAX/2
    template<typename _Type,
             bool = has_constexpr_max_size_impl<_Type>::value>
    struct has_bounded_capacity_impl : std::false_type
    {};

    template<typename _Type>
    struct has_bounded_capacity_impl<_Type, true>
        : std::bool_constant<
              (_Type{}.max_size() <
               (std::numeric_limits<std::size_t>::max()
                / 2))>
    {};

    // bounded minimum: has min_size() with nonzero value
    template<typename _Type,
             bool = has_constexpr_min_size_impl<_Type>::value>
    struct has_bounded_minimum_impl : std::false_type
    {};

    template<typename _Type>
    struct has_bounded_minimum_impl<_Type, true>
        : std::bool_constant<(_Type{}.min_size() > 0)>
    {};

    // interval validity checks: verify exposed interval
    // types actually satisfy is_interval from
    // interval_traits.hpp.
    template<typename _Type, typename = void>
    struct has_valid_size_interval : std::false_type
    {};

    template<typename _Type>
    struct has_valid_size_interval<_Type,
        std::enable_if_t<
            has_size_interval_type_v<_Type> &&
            maths::is_interval<
                typename _Type::size_interval>::value>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_valid_depth_interval : std::false_type
    {};

    template<typename _Type>
    struct has_valid_depth_interval<_Type,
        std::enable_if_t<
            has_depth_interval_type_v<_Type> &&
            maths::is_interval<
                typename _Type::depth_interval>::value>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_valid_multiplicity_interval : std::false_type
    {};

    template<typename _Type>
    struct has_valid_multiplicity_interval<_Type,
        std::enable_if_t<
            has_multiplicity_interval_type_v<_Type> &&
            maths::is_interval<
                typename _Type::multiplicity_interval
            >::value>>
        : std::true_type
    {};

NS_END  // internal


// =============================================================================
// III. Public Constexpr Traits
// =============================================================================

// --- size ---

template<typename _Type>
struct has_constexpr_size
    : std::bool_constant<
          internal::has_constexpr_size_impl<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_constexpr_size_v =
    has_constexpr_size<_Type>::value;

template<typename _Type>
struct has_constexpr_max_size
    : std::bool_constant<
          internal::has_constexpr_max_size_impl<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_constexpr_max_size_v =
    has_constexpr_max_size<_Type>::value;

template<typename _Type>
struct has_constexpr_min_size
    : std::bool_constant<
          internal::has_constexpr_min_size_impl<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_constexpr_min_size_v =
    has_constexpr_min_size<_Type>::value;

template<typename _Type>
struct has_fixed_size
    : std::bool_constant<
          internal::has_fixed_size_impl<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_fixed_size_v =
    has_fixed_size<_Type>::value;

template<typename _Type>
struct has_bounded_capacity
    : std::bool_constant<
          internal::has_bounded_capacity_impl<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_bounded_capacity_v =
    has_bounded_capacity<_Type>::value;

template<typename _Type>
struct has_bounded_minimum
    : std::bool_constant<
          internal::has_bounded_minimum_impl<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_bounded_minimum_v =
    has_bounded_minimum<_Type>::value;

// --- interval validity ---

template<typename _Type>
struct has_valid_size_interval
    : std::bool_constant<
          internal::has_valid_size_interval<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_valid_size_interval_v =
    has_valid_size_interval<_Type>::value;

template<typename _Type>
struct has_valid_depth_interval
    : std::bool_constant<
          internal::has_valid_depth_interval<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_valid_depth_interval_v =
    has_valid_depth_interval<_Type>::value;

template<typename _Type>
struct has_valid_multiplicity_interval
    : std::bool_constant<
          internal::has_valid_multiplicity_interval<
              clean_t<_Type>>::value>
{};

template<typename _Type>
inline constexpr bool has_valid_multiplicity_interval_v =
    has_valid_multiplicity_interval<_Type>::value;


// =============================================================================
// IV.  Array Detection
// =============================================================================

template<typename _Type>
struct is_c_array : std::false_type
{};

template<typename _Type, std::size_t _N>
struct is_c_array<_Type[_N]> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_c_array_v =
    is_c_array<_Type>::value;

template<typename _Type>
struct is_std_array : std::false_type
{};

template<typename _Type, std::size_t _N>
struct is_std_array<std::array<_Type, _N>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_std_array_v =
    is_std_array<_Type>::value;


// =============================================================================
// V.   Lifetime Classification
// =============================================================================

// is_compile_time_container
//   type trait: true if container exists only at compile time
// (constexpr_<container>).
// Detection: empty type + no runtime state + constexpr
// everything.
template<typename _Type>
struct is_compile_time_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( std::is_empty_v<clean_type>       &&
          has_constexpr_size_v<clean_type>  &&
          !has_allocator_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_compile_time_container_v =
    is_compile_time_container<_Type>::value;

// is_immutable_container
//   type trait: true if container is read-only after
// initialization (const_<container>).
// Detection: no mutating methods (push_back, insert, erase,
// clear).
template<typename _Type>
struct is_immutable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_size_accessor_v<clean_type> &&
          !has_push_back_v<clean_type>    &&
          !has_insert_v<clean_type>       &&
          !has_erase_v<clean_type>        &&
          !has_clear_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_immutable_container_v =
    is_immutable_container<_Type>::value;

// is_mutable_container
//   type trait: true if container supports mutation at runtime
// (mutable_<container>).
template<typename _Type>
struct is_mutable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_size_accessor_v<clean_type> &&
          ( has_push_back_v<clean_type>   ||
            has_insert_v<clean_type>      ||
            has_erase_v<clean_type>       ||
            has_clear_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_mutable_container_v =
    is_mutable_container<_Type>::value;

// is_runtime_container
//   type trait: true if container exists at runtime (mutable or
// immutable, but not compile-time only).
template<typename _Type>
struct is_runtime_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        !is_compile_time_container_v<clean_type>;
};

template<typename _Type>
inline constexpr bool is_runtime_container_v =
    is_runtime_container<_Type>::value;


// =============================================================================
// VI.  Bounds Classification (interval-aware)
// =============================================================================
// Boundedness is determined in order of priority:
//   1. Interval protocol: if container exposes a size_interval
//      type satisfying maths::is_interval, the interval's
//      lower_bound and upper_bound encode the constraints.
//   2. Legacy accessors: min_size() / max_size() / capacity().
//   3. Fixed-size arrays: C-arrays and std::array.

// is_lower_bounded
//   type trait: true if container has a minimum size constraint.
template<typename _Type>
struct is_lower_bounded
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_valid_size_interval_v<clean_type> ||
          has_bounded_minimum_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_lower_bounded_v =
    is_lower_bounded<_Type>::value;

// is_upper_bounded
//   type trait: true if container has a maximum size
// (capacity) constraint.
template<typename _Type>
struct is_upper_bounded
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_valid_size_interval_v<clean_type>  ||
          is_c_array_v<clean_type>              ||
          is_std_array_v<clean_type>            ||
          has_fixed_size_v<clean_type>          ||
          has_bounded_capacity_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_upper_bounded_v =
    is_upper_bounded<_Type>::value;

// is_bounded
//   type trait: true if container has both lower and upper
// bounds.
template<typename _Type>
struct is_bounded
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_lower_bounded_v<clean_type> &&
          is_upper_bounded_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_bounded_v =
    is_bounded<_Type>::value;

// is_unbounded
//   type trait: true if container has no size constraints.
template<typename _Type>
struct is_unbounded
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( !is_lower_bounded_v<clean_type> &&
          !is_upper_bounded_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_unbounded_v =
    is_unbounded<_Type>::value;


// =============================================================================
// VII. Storage Classification
// =============================================================================

// is_static_storage
//   type trait: true if container has inline/fixed storage.
template<typename _Type>
struct is_static_storage
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_c_array_v<clean_type>             ||
          is_std_array_v<clean_type>           ||
          has_fixed_size_v<clean_type>         ||
          ( has_bounded_capacity_v<clean_type> &&
            !has_allocator_type_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_static_storage_v =
    is_static_storage<_Type>::value;

// is_dynamic_storage
//   type trait: true if container uses heap allocation.
template<typename _Type>
struct is_dynamic_storage
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_size_accessor_v<clean_type>    &&
          !is_static_storage_v<clean_type>   &&
          ( has_allocator_type_v<clean_type>  ||
            !has_bounded_capacity_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_dynamic_storage_v =
    is_dynamic_storage<_Type>::value;


// =============================================================================
// VIII. Iteration Classification
// =============================================================================

// is_iterable_container
//   type trait: true if container supports forward iteration
// via begin()/end().
template<typename _Type>
struct is_iterable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_begin_accessor_v<clean_type> &&
          has_end_accessor_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_iterable_container_v =
    is_iterable_container<_Type>::value;

// is_const_iterable_container
//   type trait: true if container supports const iteration
// via cbegin()/cend().
template<typename _Type>
struct is_const_iterable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_cbegin_accessor_v<clean_type> &&
          has_cend_accessor_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_const_iterable_container_v =
    is_const_iterable_container<_Type>::value;

// is_non_iterable_container
//   type trait: true if container has size() but no iteration
// support (e.g. stack, queue adaptors).
template<typename _Type>
struct is_non_iterable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_size_accessor_v<clean_type> &&
          !is_iterable_container_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_non_iterable_container_v =
    is_non_iterable_container<_Type>::value;


// =============================================================================
// IX.  Ordering Classification
// =============================================================================

// is_ordered_container
//   type trait: true if container maintains element ordering.
// Detection:
//   1. C-arrays and std::array are always ordered
//   2. Containers with random_access or bidirectional
//      iterators are ordered
template<typename _Type>
struct is_ordered_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_c_array_v<clean_type>                ||
          is_std_array_v<clean_type>              ||
          is_random_access_iterable_v<clean_type> ||
          is_bidirectional_iterable_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_ordered_container_v =
    is_ordered_container<_Type>::value;

// is_unordered_container
//   type trait: true if container does not maintain element
// ordering.
// Detection: iterable but not ordered.
template<typename _Type>
struct is_unordered_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type> &&
          !is_ordered_container_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_unordered_container_v =
    is_unordered_container<_Type>::value;


// =============================================================================
// X.   Sorted Classification
// =============================================================================
// A sorted container maintains a sorted invariant over its
// elements.  Detection is purely structural:
//   1. Presence of key_compare member alias indicates the
//      container maintains a comparison-based ordering.
//   2. Hash-based containers (hasher type) are explicitly
//      excluded even if they also expose key_compare.

// is_sorted_container
//   type trait: true if container maintains a sorted invariant.
template<typename _Type>
struct is_sorted_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_key_compare_v<clean_type> &&
          !has_hasher_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_sorted_container_v =
    is_sorted_container<_Type>::value;

// is_unsorted_container
//   type trait: true if container does not maintain a sorted
// invariant.
template<typename _Type>
struct is_unsorted_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_size_accessor_v<clean_type> &&
          !is_sorted_container_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_unsorted_container_v =
    is_unsorted_container<_Type>::value;


// =============================================================================
// XI.  Reverse Iteration Classification
// =============================================================================

// has_reverse_iteration
//   type trait: true if container supports reverse iteration
// (rbegin/rend).
template<typename _Type>
struct has_reverse_iteration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_rbegin_accessor_v<clean_type> &&
          has_rend_accessor_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_reverse_iteration_v =
    has_reverse_iteration<_Type>::value;


// =============================================================================
// XII. Uniqueness and Multiplicity Classification
// =============================================================================
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
//   type trait: true if container enforces element uniqueness
// (sets, maps with unique keys).
template<typename _Type>
struct enforces_uniqueness
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_key_type_v<clean_type> &&
          !has_mapped_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool enforces_uniqueness_v =
    enforces_uniqueness<_Type>::value;

// allows_duplicates
//   type trait: true if container allows duplicate elements.
template<typename _Type>
struct allows_duplicates
{
    static constexpr bool value =
        !enforces_uniqueness_v<clean_t<_Type>>;
};

template<typename _Type>
inline constexpr bool allows_duplicates_v =
    allows_duplicates<_Type>::value;

// has_bounded_multiplicity
//   type trait: true if container constrains element
// multiplicity, either via the interval protocol or legacy
// accessor pair.
template<typename _Type>
struct has_bounded_multiplicity
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_valid_multiplicity_interval_v<clean_type> ||
          ( has_multiplicity_min_accessor_v<clean_type> &&
            has_multiplicity_max_accessor_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool has_bounded_multiplicity_v =
    has_bounded_multiplicity<_Type>::value;

// is_unique_container
//   type trait: true if container enforces element uniqueness
// via structural detection.
template<typename _Type>
struct is_unique_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        enforces_uniqueness_v<clean_type>;
};

template<typename _Type>
inline constexpr bool is_unique_container_v =
    is_unique_container<_Type>::value;


// =============================================================================
// XIII. Structure Classification (flat / hierarchical)
// =============================================================================
// A flat container stores all elements at a single level.
// A hierarchical container organizes elements into levels.
//
// Detection is purely structural (no tags):
//   1. parent(), children(), or root() member functions
//      indicate a tree-like traversal API.
//   2. node_type alias indicates node-based hierarchy.
//   3. depth_type alias or depth()/max_depth() accessors.
//   4. depth_interval type alias (interval protocol).
//   5. Absence of all hierarchy indicators => flat.

// is_hierarchical_container
//   type trait: true if container organizes elements in a
// hierarchical (multi-level) structure.
template<typename _Type>
struct is_hierarchical_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_parent_accessor_v<clean_type>    ||
          has_children_accessor_v<clean_type>  ||
          has_root_accessor_v<clean_type>      ||
          has_node_type_v<clean_type>          ||
          has_depth_type_v<clean_type>         ||
          has_depth_accessor_v<clean_type>     ||
          has_valid_depth_interval_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_hierarchical_container_v =
    is_hierarchical_container<_Type>::value;

// is_flat_container
//   type trait: true if container stores elements at a single
// level.  Inferred as the negation of hierarchical.
template<typename _Type>
struct is_flat_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        !is_hierarchical_container_v<clean_type>;
};

template<typename _Type>
inline constexpr bool is_flat_container_v =
    is_flat_container<_Type>::value;

// is_depth_bounded_container
//   type trait: true if hierarchical depth is constrained.
// Detection priority:
//   1. depth_interval satisfying is_interval.
//   2. Legacy max_depth() accessor.
template<typename _Type>
struct is_depth_bounded_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_hierarchical_container_v<clean_type> &&
          ( has_valid_depth_interval_v<clean_type> ||
            has_max_depth_accessor_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_depth_bounded_container_v =
    is_depth_bounded_container<_Type>::value;

// is_depth_unbounded_container
//   type trait: true if hierarchical depth is not constrained.
template<typename _Type>
struct is_depth_unbounded_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_hierarchical_container_v<clean_type> &&
          !is_depth_bounded_container_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_depth_unbounded_container_v =
    is_depth_unbounded_container<_Type>::value;


// =============================================================================
// XIV. Backing Container Classification
// =============================================================================
// Some containers delegate storage to another container type.
// Detection is purely structural:
//   1. backing_container_type alias => backed.
//   2. Absence of backing_container_type, or C/std::array =>
//      fundamental.

// is_backed_container
//   type trait: true if container delegates storage to another
// container type.
template<typename _Type>
struct is_backed_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        has_backing_container_type_v<clean_type>;
};

template<typename _Type>
inline constexpr bool is_backed_container_v =
    is_backed_container<_Type>::value;

// is_fundamental_container
//   type trait: true if container provides its own storage.
// C-arrays and std::array are always fundamental.
// Any container without backing_container_type is fundamental.
template<typename _Type>
struct is_fundamental_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_c_array_v<clean_type>   ||
          is_std_array_v<clean_type> ||
          !is_backed_container_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_fundamental_container_v =
    is_fundamental_container<_Type>::value;

// backing_container_type_of
//   type trait: extracts the backing container type if
// present, otherwise yields void.

NS_INTERNAL

    template<typename _Type, typename = void>
    struct backing_container_type_of_impl
    {
        using type = void;
    };

    template<typename _Type>
    struct backing_container_type_of_impl<_Type,
        std::void_t<
            typename _Type::backing_container_type>>
    {
        using type =
            typename _Type::backing_container_type;
    };

NS_END  // internal

template<typename _Type>
struct backing_container_type_of
{
    using type =
        typename internal::backing_container_type_of_impl<
            clean_t<_Type>>::type;
};

template<typename _Type>
using backing_container_type_of_t =
    typename backing_container_type_of<_Type>::type;


// =============================================================================
// XV.  Interval Extraction
// =============================================================================
// Type aliases that extract the interval types from containers
// that declare them, yielding void when absent.

NS_INTERNAL

    template<typename _Type, typename = void>
    struct size_interval_of_impl
    {
        using type = void;
    };

    template<typename _Type>
    struct size_interval_of_impl<_Type,
        std::enable_if_t<
            has_valid_size_interval<_Type>::value>>
    {
        using type = typename _Type::size_interval;
    };

    template<typename _Type, typename = void>
    struct depth_interval_of_impl
    {
        using type = void;
    };

    template<typename _Type>
    struct depth_interval_of_impl<_Type,
        std::enable_if_t<
            has_valid_depth_interval<_Type>::value>>
    {
        using type = typename _Type::depth_interval;
    };

    template<typename _Type, typename = void>
    struct multiplicity_interval_of_impl
    {
        using type = void;
    };

    template<typename _Type>
    struct multiplicity_interval_of_impl<_Type,
        std::enable_if_t<
            has_valid_multiplicity_interval<
                _Type>::value>>
    {
        using type =
            typename _Type::multiplicity_interval;
    };

NS_END  // internal

// size_interval_of
//   type trait: yields the container's size_interval type,
// or void if none is declared.
template<typename _Type>
struct size_interval_of
{
    using type =
        typename internal::size_interval_of_impl<
            clean_t<_Type>>::type;
};

template<typename _Type>
using size_interval_of_t =
    typename size_interval_of<_Type>::type;

// depth_interval_of
//   type trait: yields the container's depth_interval type,
// or void if none is declared.
template<typename _Type>
struct depth_interval_of
{
    using type =
        typename internal::depth_interval_of_impl<
            clean_t<_Type>>::type;
};

template<typename _Type>
using depth_interval_of_t =
    typename depth_interval_of<_Type>::type;

// multiplicity_interval_of
//   type trait: yields the container's multiplicity_interval
// type, or void if none is declared.
template<typename _Type>
struct multiplicity_interval_of
{
    using type =
        typename internal::multiplicity_interval_of_impl<
            clean_t<_Type>>::type;
};

template<typename _Type>
using multiplicity_interval_of_t =
    typename multiplicity_interval_of<_Type>::type;


// =============================================================================
// XVI. Combined Classification
// =============================================================================

// container_class
//   struct: complete classification of a container type.
// All classification is compile-time using static constexpr
// bool members.
template<typename _Type>
struct container_class
{
    // lifetime classification
    static constexpr bool is_compile_time =
        is_compile_time_container_v<_Type>;
    static constexpr bool is_immutable =
        is_immutable_container_v<_Type>;
    static constexpr bool is_mutable =
        is_mutable_container_v<_Type>;
    static constexpr bool is_runtime =
        is_runtime_container_v<_Type>;

    // bounds classification
    static constexpr bool is_unbounded =
        is_unbounded_v<_Type>;
    static constexpr bool is_lower_bounded =
        is_lower_bounded_v<_Type>;
    static constexpr bool is_upper_bounded =
        is_upper_bounded_v<_Type>;
    static constexpr bool is_bounded =
        is_bounded_v<_Type>;
    static constexpr bool has_size_interval =
        has_valid_size_interval_v<_Type>;

    // storage classification
    static constexpr bool is_static_storage =
        is_static_storage_v<_Type>;
    static constexpr bool is_dynamic_storage =
        is_dynamic_storage_v<_Type>;

    // iteration classification
    static constexpr bool is_iterable =
        is_iterable_container_v<_Type>;
    static constexpr bool is_const_iterable =
        is_const_iterable_container_v<_Type>;
    static constexpr bool is_non_iterable =
        is_non_iterable_container_v<_Type>;
    static constexpr bool has_reverse_iterators =
        has_reverse_iteration_v<_Type>;

    // ordering classification
    static constexpr bool is_ordered =
        is_ordered_container_v<_Type>;
    static constexpr bool is_unordered =
        is_unordered_container_v<_Type>;

    // sorted classification
    static constexpr bool is_sorted =
        is_sorted_container_v<_Type>;
    static constexpr bool is_unsorted =
        is_unsorted_container_v<_Type>;

    // uniqueness and multiplicity
    static constexpr bool enforces_uniqueness =
        enforces_uniqueness_v<_Type>;
    static constexpr bool allows_duplicates =
        allows_duplicates_v<_Type>;
    static constexpr bool has_bounded_multiplicity =
        has_bounded_multiplicity_v<_Type>;
    static constexpr bool has_multiplicity_interval =
        has_valid_multiplicity_interval_v<_Type>;

    // structure classification
    static constexpr bool is_flat =
        is_flat_container_v<_Type>;
    static constexpr bool is_hierarchical =
        is_hierarchical_container_v<_Type>;
    static constexpr bool is_depth_bounded =
        is_depth_bounded_container_v<_Type>;
    static constexpr bool is_depth_unbounded =
        is_depth_unbounded_container_v<_Type>;
    static constexpr bool has_depth_interval =
        has_valid_depth_interval_v<_Type>;

    // backing container classification
    static constexpr bool is_backed =
        is_backed_container_v<_Type>;
    static constexpr bool is_fundamental =
        is_fundamental_container_v<_Type>;
};


NS_END	// traits
NS_END	// container
NS_END	// djinterp


#endif	// DJINTERP_CONTAINER_TRAITS_
