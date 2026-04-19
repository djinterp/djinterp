/******************************************************************************
* djinterp [container]                            array_container_traits.hpp
*
* Array-specific traits for the djinterp container framework.
*   Detects capabilities unique to array-based (contiguous, random-
* access) containers:
*
*   - Capacity model:  fixed vs dynamic vs small-buffer optimized
*   - Contiguity:      data() + contiguous iterators
*   - Circular:        head/tail cursor, wrap-around access
*   - Chunked:         hierarchical array segmentation
*   - Element metrics: compile-time sizeof, alignment, stride
*   - Shift support:   logical shift left/right
*   - Growth policy:   reserve, shrink_to_fit, growth factor
*
* TABLE OF CONTENTS
* =================
* I.      Capacity Model Detection
* II.     Contiguity Detection
* III.    Circular Buffer Detection
* IV.     Chunked Array Detection
* V.      Element Metrics
* VI.     Shift and Rotation Detection
* VII.    Growth Policy Detection
* VIII.   Strategy Classification
* IX.     Combined Classification
*
*
* path:      \inc\container\meta\array_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_ARRAY_CONTAINER_TRAITS_
#define DJINTERP_ARRAY_CONTAINER_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "container_traits.hpp"
#include "iterator_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Capacity Model Detection
// =============================================================================

// DCapacityModel
//   enum: classifies how the array manages capacity.
enum class DCapacityModel
{
    // unknown / not an array
    none,

    // compile-time fixed size (std::array, C array)
    fixed,

    // heap-allocated growable (std::vector)
    dynamic,

    // small-buffer optimization: inline for small,
    // heap for large (e.g. llvm::SmallVector)
    small_buffer,

    // externally managed: data() is valid but the
    // container does not own the memory (span, view)
    external
};

// --- has_capacity ---
D_TYPE_TRAIT_TRUE(has_capacity_method,
    decltype(
        std::declval<const _Type&>().capacity()))

// --- has_reserve ---
D_TYPE_TRAIT_TRUE(has_reserve_method,
    decltype(std::declval<_Type&>().reserve(
        std::declval<std::size_t>())))

// --- has_shrink_to_fit ---
D_TYPE_TRAIT_TRUE(has_shrink_to_fit_method,
    decltype(
        std::declval<_Type&>().shrink_to_fit()))

// --- has_max_size ---
D_TYPE_TRAIT_TRUE(has_max_size_method,
    decltype(
        std::declval<const _Type&>().max_size()))

// --- has_static_extent (compile-time size) ---
NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_extent_check : std::false_type
    {};

    template<typename _Type>
    struct has_extent_check<_Type,
        std::void_t<decltype(_Type::extent)>>
        : std::true_type
    {};

    // tuple_size detection (std::array pattern)
    template<typename _Type, typename = void>
    struct has_tuple_size_check : std::false_type
    {};

    template<typename _Type>
    struct has_tuple_size_check<_Type,
        std::void_t<decltype(
            std::tuple_size<_Type>::value)>>
        : std::true_type
    {};

NS_END  // internal

// has_static_extent
//   type trait: true if the container has a compile-time
// known size (::extent or std::tuple_size).
template<typename _Type>
struct has_static_extent
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( internal::has_extent_check<
              clean_type>::value ||
          internal::has_tuple_size_check<
              clean_type>::value );
};

template<typename _Type>
inline constexpr bool has_static_extent_v =
    has_static_extent<_Type>::value;

// is_fixed_capacity
//   type trait: true if the array has compile-time fixed
// size and cannot grow.
template<typename _Type>
struct is_fixed_capacity
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_static_extent_v<clean_type>       &&
          has_data_accessor_v<clean_type>       &&
          !has_reserve_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_fixed_capacity_v =
    is_fixed_capacity<_Type>::value;

// is_dynamic_capacity
//   type trait: true if the array can grow (has reserve
// or capacity).
template<typename _Type>
struct is_dynamic_capacity
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_data_accessor_v<clean_type>       &&
          ( has_capacity_method_v<clean_type> ||
            has_reserve_method_v<clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_dynamic_capacity_v =
    is_dynamic_capacity<_Type>::value;

// --- small-buffer detection ---

NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_inline_capacity_check : std::false_type
    {};

    template<typename _Type>
    struct has_inline_capacity_check<_Type,
        std::void_t<decltype(
            _Type::inline_capacity)>>
        : std::true_type
    {};

NS_END  // internal

// is_small_buffer_optimized
//   type trait: true if the container advertises an inline
// capacity for small-buffer optimization.
template<typename _Type>
struct is_small_buffer_optimized
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_inline_capacity_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_small_buffer_optimized_v =
    is_small_buffer_optimized<_Type>::value;

// capacity_model_of
//   type trait: deduces the capacity model.
NS_INTERNAL

    template<typename _Type>
    struct capacity_model_impl
    {
        using C = clean_t<_Type>;

        static constexpr DCapacityModel value =
            is_small_buffer_optimized_v<C>
                ? DCapacityModel::small_buffer

            : is_fixed_capacity_v<C>
                ? DCapacityModel::fixed

            : is_dynamic_capacity_v<C>
                ? DCapacityModel::dynamic

            : ( has_data_accessor_v<C> &&
                !has_reserve_method_v<C> &&
                !has_static_extent_v<C> )
                ? DCapacityModel::external

            : DCapacityModel::none;
    };

NS_END  // internal

template<typename _Type>
struct capacity_model_of
{
    static constexpr DCapacityModel value =
        internal::capacity_model_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DCapacityModel
    capacity_model_of_v =
        capacity_model_of<_Type>::value;


// =============================================================================
// II.  Contiguity Detection
// =============================================================================

// is_contiguous_array
//   type trait: true if the container is contiguous
// (data() + random-access iterators).
template<typename _Type>
struct is_contiguous_array
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_data_accessor_v<clean_type>       &&
          is_random_access_iterable_v<
              clean_type> );
};

template<typename _Type>
inline constexpr bool is_contiguous_array_v =
    is_contiguous_array<_Type>::value;


// =============================================================================
// III. Circular Buffer Detection
// =============================================================================

// has_head_method
D_TYPE_TRAIT_TRUE(has_head_method,
    decltype(
        std::declval<const _Type&>().head()))

// has_tail_method
D_TYPE_TRAIT_TRUE(has_tail_method,
    decltype(
        std::declval<const _Type&>().tail()))

// has_is_full_method
D_TYPE_TRAIT_TRUE(has_is_full_method,
    decltype(
        std::declval<const _Type&>().is_full()))

// has_push_front_method
D_TYPE_TRAIT_TRUE(has_push_front_method,
    decltype(std::declval<_Type&>().push_front(
        std::declval<
            typename _Type::value_type>())))

// has_pop_front_method
D_TYPE_TRAIT_TRUE(has_pop_front_method,
    decltype(std::declval<_Type&>().pop_front()))

// is_circular_buffer
//   type trait: true if the container is a circular
// buffer (has head + tail + is_full + fixed capacity).
template<typename _Type>
struct is_circular_buffer
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_head_method_v<clean_type>        &&
          has_tail_method_v<clean_type>        &&
          has_is_full_method_v<clean_type>     &&
          has_capacity_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_circular_buffer_v =
    is_circular_buffer<_Type>::value;


// =============================================================================
// IV.  Chunked Array Detection
// =============================================================================

// has_chunk_size
D_TYPE_TRAIT_TRUE(has_chunk_size_field,
    decltype(_Type::chunk_size))

D_TYPE_TRAIT_TRUE(has_chunk_size_method,
    decltype(
        std::declval<const _Type&>().chunk_size()))

// has_chunk_count
D_TYPE_TRAIT_TRUE(has_chunk_count_method,
    decltype(
        std::declval<const _Type&>().chunk_count()))

// has_chunk_at
D_TYPE_TRAIT_TRUE(has_chunk_at_method,
    decltype(std::declval<const _Type&>().chunk_at(
        std::declval<std::size_t>())))

// is_chunked_array
//   type trait: true if the container organizes its
// storage in fixed-size chunks (for hierarchical
// array layouts, B-tree nodes, etc.).
template<typename _Type>
struct is_chunked_array
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( ( has_chunk_size_field_v<clean_type> ||
            has_chunk_size_method_v<clean_type> )  &&
          has_chunk_count_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_chunked_array_v =
    is_chunked_array<_Type>::value;


// =============================================================================
// V.   Element Metrics
// =============================================================================

NS_INTERNAL

    template<typename _Type, typename = void>
    struct elem_safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct elem_safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using elem_safe_value_type_t =
        typename elem_safe_value_type<_Type>::type;

NS_END  // internal

// element_size_of
//   type trait: sizeof(value_type) when value_type is
// complete, 0 otherwise.
template<typename _Type>
struct element_size_of
{
    using elem_type =
        internal::elem_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr std::size_t value =
        std::is_void_v<elem_type>
            ? 0 : sizeof(elem_type);
};

template<typename _Type>
inline constexpr std::size_t element_size_of_v =
    element_size_of<_Type>::value;

// element_alignment_of
//   type trait: alignof(value_type).
template<typename _Type>
struct element_alignment_of
{
    using elem_type =
        internal::elem_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr std::size_t value =
        std::is_void_v<elem_type>
            ? 0 : alignof(elem_type);
};

template<typename _Type>
inline constexpr std::size_t
    element_alignment_of_v =
        element_alignment_of<_Type>::value;

// is_trivially_relocatable_array
//   type trait: true if elements can be relocated via
// memcpy/memmove (trivially copyable + trivially
// destructible — safe for realloc-style growth).
template<typename _Type>
struct is_trivially_relocatable_array
{
    using elem_type =
        internal::elem_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        ( is_contiguous_array_v<clean_t<_Type>> &&
          std::is_trivially_copyable_v<
              elem_type>                        &&
          std::is_trivially_destructible_v<
              elem_type> );
};

template<typename _Type>
inline constexpr bool
    is_trivially_relocatable_array_v =
        is_trivially_relocatable_array<
            _Type>::value;


// =============================================================================
// VI.  Shift and Rotation Detection
// =============================================================================

// has_shift_left_method
D_TYPE_TRAIT_TRUE(has_shift_left_method,
    decltype(std::declval<_Type&>().shift_left(
        std::declval<std::size_t>())))

// has_shift_right_method
D_TYPE_TRAIT_TRUE(has_shift_right_method,
    decltype(std::declval<_Type&>().shift_right(
        std::declval<std::size_t>())))

// has_rotate_method
D_TYPE_TRAIT_TRUE(has_rotate_method,
    decltype(std::declval<_Type&>().rotate(
        std::declval<std::size_t>())))

// is_shiftable_array
//   type trait: true if the array supports logical shift
// operations (contiguous + random-access).
template<typename _Type>
struct is_shiftable_array
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_contiguous_array_v<clean_type>     &&
          has_size_accessor_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_shiftable_array_v =
    is_shiftable_array<_Type>::value;


// =============================================================================
// VII. Growth Policy Detection
// =============================================================================

// has_growth_factor
D_TYPE_TRAIT_TRUE(has_growth_factor_field,
    decltype(_Type::growth_factor))

D_TYPE_TRAIT_TRUE(has_growth_factor_method,
    decltype(
        std::declval<const _Type&>()
            .growth_factor()))

// has_resize_method
D_TYPE_TRAIT_TRUE(has_resize_method,
    decltype(std::declval<_Type&>().resize(
        std::declval<std::size_t>())))


// =============================================================================
// VIII. Strategy Classification
// =============================================================================

// DArrayStrategy
//   enum: classifies the best general-purpose strategy
// for bulk operations on this array.
enum class DArrayStrategy
{
    // contiguous + trivially relocatable — memcpy/
    // memmove for shifts, bulk copy, realloc
    bulk_memcpy,

    // contiguous + non-trivial elements — element-
    // wise move via move assignment
    element_move,

    // circular buffer — advance head/tail cursors
    circular,

    // chunked — operate per-chunk
    chunked,

    // non-contiguous / unknown
    generic
};

NS_INTERNAL

    template<typename _Type>
    struct array_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr DArrayStrategy value =
            is_circular_buffer_v<C>
                ? DArrayStrategy::circular

            : is_chunked_array_v<C>
                ? DArrayStrategy::chunked

            : is_trivially_relocatable_array_v<C>
                ? DArrayStrategy::bulk_memcpy

            : is_contiguous_array_v<C>
                ? DArrayStrategy::element_move

            : DArrayStrategy::generic;
    };

NS_END  // internal

template<typename _Type>
struct array_strategy
{
    static constexpr DArrayStrategy value =
        internal::array_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DArrayStrategy
    array_strategy_v =
        array_strategy<_Type>::value;


// =============================================================================
// IX.  Combined Classification
// =============================================================================

template<typename _Type>
struct array_container_class
{
    // capacity model
    static constexpr DCapacityModel capacity =
        capacity_model_of_v<_Type>;
    static constexpr bool is_fixed =
        is_fixed_capacity_v<_Type>;
    static constexpr bool is_dynamic =
        is_dynamic_capacity_v<_Type>;
    static constexpr bool is_sbo =
        is_small_buffer_optimized_v<_Type>;
    static constexpr bool has_static_size =
        has_static_extent_v<_Type>;

    // contiguity
    static constexpr bool is_contiguous =
        is_contiguous_array_v<_Type>;

    // circular
    static constexpr bool is_circular =
        is_circular_buffer_v<_Type>;

    // chunked
    static constexpr bool is_chunked =
        is_chunked_array_v<_Type>;

    // element metrics
    static constexpr std::size_t elem_size =
        element_size_of_v<_Type>;
    static constexpr std::size_t elem_align =
        element_alignment_of_v<_Type>;
    static constexpr bool trivially_relocatable =
        is_trivially_relocatable_array_v<_Type>;

    // shift/rotation
    static constexpr bool is_shiftable =
        is_shiftable_array_v<_Type>;

    // growth
    static constexpr bool has_reserve =
        has_reserve_method_v<_Type>;
    static constexpr bool has_shrink =
        has_shrink_to_fit_method_v<_Type>;
    static constexpr bool has_capacity =
        has_capacity_method_v<_Type>;

    // strategy
    static constexpr DArrayStrategy strategy =
        array_strategy_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARRAY_CONTAINER_TRAITS_
