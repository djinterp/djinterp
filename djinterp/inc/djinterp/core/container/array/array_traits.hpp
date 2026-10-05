/*******************************************************************************
* djinterp [core]                                               array_traits.hpp
*
* Array-specific compile-time classification traits.
*   Detects capabilities unique to array-based (contiguous, random-
* access) containers:
*     - capacity model:  fixed vs dynamic vs small-buffer optimized
*     - contiguity:      data() + contiguous iterators
*     - circular:        head/tail cursor, wrap-around access
*     - chunked:         hierarchical array segmentation
*     - element metrics: compile-time sizeof, alignment, stride
*     - shift support:   logical shift left/right
*     - growth policy:   reserve, shrink_to_fit, growth factor
*     - lifetime:        constexpr / immutable / mutable
*     - iterability:     iterable / non-iterable
*
*   PORTABILITY:
*   C++11 baseline.  Variable template `_v` aliases are gated behind
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES (C++14+).  Detection
* uses void_t (C++17 std, polyfilled for earlier standards).
*
*
* path:      /inc/djinterp/core/container/array/array_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.24
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    C-array / std::array detection
      ------------------------------

2.    capacity model detection
      ------------------------

3.    contiguity detection
      --------------------

4.    circular buffer detection
      -------------------------

5.    chunked array detection
      -----------------------

6.    element metrics
      ---------------

7.    shift and rotation detection
      ----------------------------

8.    growth policy detection
      -----------------------

9.    lifetime classification
      -----------------------

10.   iterability classification
      --------------------------

11.   strategy classification
      -----------------------

12.   combined classification
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_ARRAY_TRAITS_HPP
#define DJINTERP_CONTAINER_ARRAY_ARRAY_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"
#include "../traits/node_container_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/constexpr_iterator_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   C-Array / std::array Detection
// ===========================================================================

NS_INTERNAL

    // is_std_array_helper
    //   trait: detects whether a type is an instantiation of std::array<T, N>.
    template<typename Type>
    struct is_std_array_helper : std::false_type
    {};

    // is_std_array_helper<std::array<Elem, N>>
    //   trait: the `std::array<Elem, N>` case; it reports true.
    template<typename Elem,
             std::size_t N>
    struct is_std_array_helper<std::array<Elem, N>>
        : std::true_type
    {};

NS_END  // internal


// ===========================================================================
// II.  Capacity Model Detection
// ===========================================================================

// capacity_model
//   enum: classifies how the array manages capacity.
enum class capacity_model
{
    // unknown / not an array
    none,

    // compile-time fixed size (std::array, C array)
    fixed,

    // heap-allocated growable (std::vector)
    dynamic,

    // small-buffer optimization: inline for small, heap for large (e.g.
    // llvm::SmallVector)
    small_buffer,

    // externally managed: data() is valid but the container does not own the
    // memory (span, view)
    external
};

// has_capacity_method
//   trait: detects a const member `capacity()`.
D_TYPE_TRAIT_DETECTED(has_capacity_method,
    decltype(std::declval<const Type&>().capacity()))

// has_reserve_method
//   trait: detects a member `reserve(size_t)`.
D_TYPE_TRAIT_DETECTED(has_reserve_method,
    decltype(std::declval<Type&>().reserve(
        std::declval<std::size_t>())))

// has_shrink_to_fit_method
//   trait: detects a member `shrink_to_fit()`.
D_TYPE_TRAIT_DETECTED(has_shrink_to_fit_method,
    decltype(std::declval<Type&>().shrink_to_fit()))

// has_max_size_method
//   trait: detects a const member `max_size()`.
D_TYPE_TRAIT_DETECTED(has_max_size_method,
    decltype(std::declval<const Type&>().max_size()))

NS_INTERNAL

    // has_extent_check
    //   helper: detects a static `extent` member.
    template<typename Type,
             typename = void>
    struct has_extent_check : std::false_type
    {};

    // has_extent_check<Type, void_t< decltype(Type::extent) >>
    //   trait: the `void_t< decltype(Type::extent) >` case; it reports true.
    template<typename Type>
    struct has_extent_check<Type, void_t<
        decltype(Type::extent)
    >> : std::true_type
    {};

    // has_tuple_size_check
    //   helper: detects std::tuple_size specialization (std::array pattern).
    template<typename Type,
             typename = void>
    struct has_tuple_size_check : std::false_type
    {};

    // has_tuple_size_check<Type, void_t< decltype(std::tuple_size<Type>
    //   trait: the `void_t< decltype(std::tuple_size<Type` case; it reports
    // true.
    template<typename Type>
    struct has_tuple_size_check<Type, void_t<
        decltype(std::tuple_size<Type>::value)
    >> : std::true_type
    {};

NS_END  // internal

// has_static_extent
//   trait: true if the container has a compile-time known size (::extent or
// std::tuple_size).
template<typename Type>
struct has_static_extent
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( internal::has_extent_check<cleaned>::value      ||
          internal::has_tuple_size_check<cleaned>::value  ||
          is_c_array<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_static_extent_v =
        has_static_extent<Type>::value;
#endif

// is_fixed_capacity
//   trait: true if the array has compile-time fixed size and cannot grow.
template<typename Type>
struct is_fixed_capacity
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_static_extent<cleaned>::value &&
          !has_reserve_method<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_fixed_capacity_v =
        is_fixed_capacity<Type>::value;
#endif

// is_dynamic_capacity
//   trait: true if the array can grow (has reserve or capacity).
template<typename Type>
struct is_dynamic_capacity
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_data_method<cleaned>::value        &&
          ( has_capacity_method<cleaned>::value  ||
            has_reserve_method<cleaned>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_dynamic_capacity_v =
        is_dynamic_capacity<Type>::value;
#endif

NS_INTERNAL

    // has_inline_capacity_check
    //   helper: detects a static `inline_capacity` member.
    template<typename Type,
             typename = void>
    struct has_inline_capacity_check : std::false_type
    {};

    // has_inline_capacity_check<Type, void_t<
    // decltype(Type::inline_capacity) >>
    //   trait: the `void_t< decltype(Type::inline_capacity) >` case; it
    // reports true.
    template<typename Type>
    struct has_inline_capacity_check<Type, void_t<
        decltype(Type::inline_capacity)
    >> : std::true_type
    {};

NS_END  // internal

// is_small_buffer_optimized
//   trait: true if the container advertises an inline capacity for
// small-buffer optimization.
template<typename Type>
struct is_small_buffer_optimized
    : internal::has_inline_capacity_check<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_small_buffer_optimized_v =
        is_small_buffer_optimized<Type>::value;
#endif

NS_INTERNAL

    // capacity_model_helper
    //   trait: priority cascade selecting the array's capacity model.
    template<typename Type>
    struct capacity_model_helper
    {
    private:
        using cleaned = clean_t<Type>;

    public:
        static constexpr capacity_model value =
            is_small_buffer_optimized<cleaned>::value
                ? capacity_model::small_buffer

            : is_fixed_capacity<cleaned>::value
                ? capacity_model::fixed

            : is_dynamic_capacity<cleaned>::value
                ? capacity_model::dynamic

            : ( has_data_method<cleaned>::value      &&
                !has_reserve_method<cleaned>::value  &&
                !has_static_extent<cleaned>::value )
                ? capacity_model::external

            : capacity_model::none;
    };

NS_END  // internal

// capacity_model_of
//   trait: deduces the capacity model.
template<typename Type>
struct capacity_model_of
{
    static constexpr capacity_model value =
        internal::capacity_model_helper<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr capacity_model capacity_model_of_v =
        capacity_model_of<Type>::value;
#endif


// ===========================================================================
// III. Contiguity Detection
// ===========================================================================

// is_contiguous_array
//   trait: true if the container is contiguous (data() + random-access
// iterators), or is a raw C array.
template<typename Type>
struct is_contiguous_array
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( is_c_array<cleaned>::value         ||
          ( has_data_method<cleaned>::value  &&
            is_random_access_iterable<cleaned>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_contiguous_array_v =
        is_contiguous_array<Type>::value;
#endif


// ===========================================================================
// IV.  Circular Buffer Detection
// ===========================================================================

// has_is_full_method
D_TYPE_TRAIT_DETECTED(has_is_full_method,
    decltype(std::declval<const Type&>().is_full()))

// has_push_front_method
D_TYPE_TRAIT_DETECTED(has_push_front_method,
    decltype(std::declval<Type&>().push_front(
        std::declval<typename Type::value_type>())))

// has_pop_front_method
D_TYPE_TRAIT_DETECTED(has_pop_front_method,
    decltype(std::declval<Type&>().pop_front()))

// is_circular_buffer
//   trait: true if the container is a circular buffer (has head + tail +
// is_full + capacity).
template<typename Type>
struct is_circular_buffer
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_head_method<cleaned>::value     &&
          has_tail_method<cleaned>::value     &&
          has_is_full_method<cleaned>::value  &&
          has_capacity_method<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_circular_buffer_v =
        is_circular_buffer<Type>::value;
#endif


// ===========================================================================
// V.   Chunked Array Detection
// ===========================================================================

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct has_chunk_size_field_check : std::false_type
    {};

    // has_chunk_size_field_check<Type, void_t< decltype(Type::chunk_size) >>
    //   trait: the `void_t< decltype(Type::chunk_size) >` case; it reports
    // true.
    template<typename Type>
    struct has_chunk_size_field_check<Type, void_t<
        decltype(Type::chunk_size)
    >> : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_chunk_size_field
    : internal::has_chunk_size_field_check<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_chunk_size_field_v =
        has_chunk_size_field<Type>::value;
#endif

// has_chunk_size_method
D_TYPE_TRAIT_DETECTED(has_chunk_size_method,
    decltype(std::declval<const Type&>().chunk_size()))

// has_chunk_count_method
D_TYPE_TRAIT_DETECTED(has_chunk_count_method,
    decltype(std::declval<const Type&>().chunk_count()))

// has_chunk_at_method
D_TYPE_TRAIT_DETECTED(has_chunk_at_method,
    decltype(std::declval<const Type&>().chunk_at(
        std::declval<std::size_t>())))

template<typename Type>
struct is_chunked_array
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( ( has_chunk_size_field<cleaned>::value     ||
            has_chunk_size_method<cleaned>::value )  &&
          has_chunk_count_method<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_chunked_array_v =
        is_chunked_array<Type>::value;
#endif


// ===========================================================================
// VI.  Element Metrics
// ===========================================================================

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct safe_value_type
    {
        using type = void;
    };

    // safe_value_type<Type, void_t< typename Type::value_type >>
    //   trait: the `void_t< typename Type::value_type >` case; it maps to
    // `typename Type::value_type`.
    template<typename Type>
    struct safe_value_type<Type, void_t<
        typename Type::value_type
    >>
    {
        using type = typename Type::value_type;
    };

    template<typename Type>
    using safe_value_type_t =
        typename safe_value_type<Type>::type;

    template<typename Type,
             bool IsArr = std::is_array<Type>::value>
    struct c_array_element
    {
        using type = void;
    };

    // c_array_element<Type, true>
    //   helper: the case where `std::is_array<Type>::value` is true; it maps
    // to `typename std::remove_extent<Type>::type`.
    template<typename Type>
    struct c_array_element<Type, true>
    {
        using type = typename std::remove_extent<Type>::type;
    };

    template<typename Type>
    struct resolved_element_type
    {
    private:
        using cleaned       = clean_t<Type>;
        using member_value  = safe_value_type_t<cleaned>;
        using c_array_value = typename c_array_element<cleaned>::type;

    public:
        using type =
            typename std::conditional<
                std::is_void<member_value>::value,
                c_array_value,
                member_value>::type;
    };

NS_END  // internal

template<typename Type>
struct array_element_type_of
{
    using type =
        typename internal::resolved_element_type<Type>::type;
};

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
    template<typename Type>
    using array_element_type_of_t =
        typename array_element_type_of<Type>::type;
#endif

template<typename Type>
struct element_size_of
{
private:
    using Elem =
        typename array_element_type_of<Type>::type;

public:
    static constexpr std::size_t value =
        std::is_void<Elem>::value ? 0 : sizeof(Elem);
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr std::size_t element_size_of_v =
        element_size_of<Type>::value;
#endif

template<typename Type>
struct element_alignment_of
{
private:
    using Elem =
        typename array_element_type_of<Type>::type;

public:
    static constexpr std::size_t value =
        std::is_void<Elem>::value ? 0 : alignof(Elem);
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr std::size_t element_alignment_of_v =
        element_alignment_of<Type>::value;
#endif

template<typename Type>
struct element_stride_of
{
    static constexpr std::size_t value =
        element_size_of<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr std::size_t element_stride_of_v =
        element_stride_of<Type>::value;
#endif

template<typename Type>
struct is_trivially_relocatable_array
{
private:
    using Elem =
        typename array_element_type_of<Type>::type;

public:
    static constexpr bool value =
        ( is_contiguous_array<Type>::value           &&
          !std::is_void<Elem>::value                 &&
          std::is_trivially_copyable<Elem>::value    &&
          std::is_trivially_destructible<Elem>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_trivially_relocatable_array_v =
        is_trivially_relocatable_array<Type>::value;
#endif


// ===========================================================================
// VII. Shift and Rotation Detection
// ===========================================================================

// has_shift_left_method
D_TYPE_TRAIT_DETECTED(has_shift_left_method,
    decltype(std::declval<Type&>().shift_left(
        std::declval<std::size_t>())))

// has_shift_right_method
D_TYPE_TRAIT_DETECTED(has_shift_right_method,
    decltype(std::declval<Type&>().shift_right(
        std::declval<std::size_t>())))

// has_rotate_method
D_TYPE_TRAIT_DETECTED(has_rotate_method,
    decltype(std::declval<Type&>().rotate(
        std::declval<std::size_t>())))

template<typename Type>
struct is_shiftable_array
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( is_contiguous_array<cleaned>::value  &&
          has_size_accessor<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_shiftable_array_v =
        is_shiftable_array<Type>::value;
#endif


// ===========================================================================
// VIII. Growth Policy Detection
// ===========================================================================

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct has_growth_factor_field_check : std::false_type
    {};

    // has_growth_factor_field_check<Type, void_t<
    // decltype(Type::growth_factor) >>
    //   trait: the `void_t< decltype(Type::growth_factor) >` case; it reports
    // true.
    template<typename Type>
    struct has_growth_factor_field_check<Type, void_t<
        decltype(Type::growth_factor)
    >> : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_growth_factor_field
    : internal::has_growth_factor_field_check<clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_growth_factor_field_v =
        has_growth_factor_field<Type>::value;
#endif

// has_growth_factor_method
D_TYPE_TRAIT_DETECTED(has_growth_factor_method,
    decltype(std::declval<const Type&>().growth_factor()))

// has_resize_method
D_TYPE_TRAIT_DETECTED(has_resize_method,
    decltype(std::declval<Type&>().resize(
        std::declval<std::size_t>())))


// ===========================================================================
// IX.  Lifetime Classification
// ===========================================================================
// Three positions on the lifetime axis:
//   constexpr_lifetime - data exists at compile time, fully
//                        immutable.
//   immutable_lifetime - data exists at runtime but cannot be
//                        modified after construction.
//   mutable_lifetime   - data can be modified at runtime.

// array_lifetime
//   enum: classifies lifetime mode.
enum class array_lifetime
{
    constexpr_lifetime,
    immutable_lifetime,
    mutable_lifetime
};

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct has_lifetime_marker : std::false_type
    {};

    // has_lifetime_marker<Type, void_t< decltype(Type::lifetime) >>
    //   trait: the `void_t< decltype(Type::lifetime) >` case; it reports
    // true.
    template<typename Type>
    struct has_lifetime_marker<Type, void_t<
        decltype(Type::lifetime)
    >> : std::true_type
    {};

    // marker_eq
    template<typename Type, array_lifetime V,
             bool Has = has_lifetime_marker<
                 clean_t<Type>>::value>
    struct marker_eq : std::false_type
    {};

    // marker_eq<Type, V, true>
    //   helper: the case where `has_lifetime_marker< clean_t<Type>>::value`
    // is true; it reports false.
    template<typename Type, array_lifetime V>
    struct marker_eq<Type, V, true>
        : std::integral_constant<bool,
              (clean_t<Type>::lifetime == V)>
    {};

    template<typename Type,
             typename = void>
    struct has_fill_check : std::false_type
    {};

    // has_fill_check<Type, void_t< decltype(std::declval<Type&>().fill(
    // std::declval<typename Type::value_type>())) >>
    //   trait: the `void_t< decltype(std::declval<Type&>().fill(
    // std::declval<typename Type::value_type>())) >` case; it reports true.
    template<typename Type>
    struct has_fill_check<Type, void_t<
        decltype(std::declval<Type&>().fill(
            std::declval<typename Type::value_type>()))
    >> : std::true_type
    {};

    template<typename Type,
             typename = void>
    struct has_swap_check : std::false_type
    {};

    // has_swap_check<Type, void_t< decltype(std::declval<Type&>().swap(
    // std::declval<Type&>())) >>
    //   trait: the `void_t< decltype(std::declval<Type&>().swap(
    // std::declval<Type&>())) >` case; it reports true.
    template<typename Type>
    struct has_swap_check<Type, void_t<
        decltype(std::declval<Type&>().swap(
            std::declval<Type&>()))
    >> : std::true_type
    {};

    template<typename Type,
             typename = void>
    struct has_mutable_subscript_check : std::false_type
    {};

    template<typename Type>
    struct has_mutable_subscript_check<Type, void_t<
        decltype(std::declval<Type&>()[std::size_t{}] =
                 std::declval<typename Type::value_type>())
    >> : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_constexpr_array
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( internal::has_lifetime_marker<cleaned>::value
              ? internal::marker_eq<cleaned,
                    array_lifetime::constexpr_lifetime>::value
              : has_constexpr_iteration<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_constexpr_array_v =
        is_constexpr_array<Type>::value;
#endif

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct has_push_back_check : std::false_type
    {};

    // has_push_back_check<Type, void_t<
    // decltype(std::declval<Type&>().push_back( std::declval<typename
    // Type::value_type>())) >>
    //   trait: the `void_t< decltype(std::declval<Type&>().push_back(
    // std::declval<typename Type::value_type>())) >` case; it reports true.
    template<typename Type>
    struct has_push_back_check<Type, void_t<
        decltype(std::declval<Type&>().push_back(
            std::declval<typename Type::value_type>()))
    >> : std::true_type
    {};

    template<typename Type,
             typename = void>
    struct has_clear_check : std::false_type
    {};

    // has_clear_check<Type, void_t< decltype(std::declval<Type&>().clear())
    // >>
    //   trait: the `void_t< decltype(std::declval<Type&>().clear()) >` case;
    // it reports true.
    template<typename Type>
    struct has_clear_check<Type, void_t<
        decltype(std::declval<Type&>().clear())
    >> : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct is_mutable_array
{
private:
    using cleaned = clean_t<Type>;

    static constexpr bool duck_value =
        ( internal::has_push_back_check<cleaned>::value         ||
          internal::has_clear_check<cleaned>::value             ||
          has_resize_method<cleaned>::value                     ||
          has_reserve_method<cleaned>::value                    ||
          internal::has_fill_check<cleaned>::value              ||
          internal::has_swap_check<cleaned>::value              ||
          internal::has_mutable_subscript_check<cleaned>::value );

public:
    static constexpr bool value =
        ( internal::has_lifetime_marker<cleaned>::value
              ? internal::marker_eq<cleaned,
                    array_lifetime::mutable_lifetime>::value
              : duck_value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_mutable_array_v =
        is_mutable_array<Type>::value;
#endif

template<typename Type>
struct is_immutable_array
{
private:
    using cleaned = clean_t<Type>;

    static constexpr bool duck_value =
        ( ( is_contiguous_array<cleaned>::value  ||
            is_c_array<cleaned>::value )         &&
          !is_mutable_array<cleaned>::value      &&
          !is_constexpr_array<cleaned>::value );

public:
    static constexpr bool value =
        ( internal::has_lifetime_marker<cleaned>::value
              ? internal::marker_eq<cleaned,
                    array_lifetime::immutable_lifetime>::value
              : duck_value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_immutable_array_v =
        is_immutable_array<Type>::value;
#endif

NS_INTERNAL

    template<typename Type,
             bool HasMarker = has_lifetime_marker<Type>::value>
    struct array_lifetime_helper
    {
    private:
        using cleaned = clean_t<Type>;

    public:
        static constexpr array_lifetime value =
            is_constexpr_array<cleaned>::value
                ? array_lifetime::constexpr_lifetime

            : is_mutable_array<cleaned>::value
                ? array_lifetime::mutable_lifetime

            : array_lifetime::immutable_lifetime;
    };

    // partial specialization: marker present, use it directly.
    // array_lifetime_helper<Type, true>
    //   helper: the case where `has_lifetime_marker<Type>::value` is true; it
    // reports `clean_t<Type>::lifetime`.
    template<typename Type>
    struct array_lifetime_helper<Type, true>
    {
        static constexpr array_lifetime value =
            clean_t<Type>::lifetime;
    };

NS_END  // internal

template<typename Type>
struct array_lifetime_of
{
    static constexpr array_lifetime value =
        internal::array_lifetime_helper<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr array_lifetime array_lifetime_of_v =
        array_lifetime_of<Type>::value;
#endif


// ===========================================================================
// X.   Iterability Classification
// ===========================================================================
// Two-position axis: an array may be iterable (has begin/end)
// or non-iterable (raw storage with data()/size() but no
// iteration entry points).
//
//   The axis is exposed both as a boolean (for SFINAE
// predicates that want a yes/no answer) and as a named
// enum (for resolver / re-export chains that need a
// tagged position).

// array_iterability
//   enum: classifies iteration capability. Mirrors the universal
// `container_iterability` enum from container_options.hpp; the per-axis
// translation lives in array.hpp's `to_array_iterability` helper.
//
//   This is the missing enum that every wrapper header (atomic_array.hpp,
// cow_array.hpp, threadsafe_array.hpp) references in its axis re-export and
// trait specializations. Keeping it here, alongside the existing
// `array_lifetime` enum, keeps the classification axes co-located in one file.
enum class array_iterability
{
    iterable,
    non_iterable
};

NS_INTERNAL

    // array_begin_expr_t
    //   alias template: yields decltype(std::begin(t)) for an lvalue of Type,
    // or substitution failure. Detection candidate for is_iterable_array.
    template<typename Type>
    using array_begin_expr_t =
        decltype(std::begin(std::declval<Type&>()));

    // array_end_expr_t
    //   alias template: yields decltype(std::end(t)) for an lvalue of Type,
    // or substitution failure. Detection candidate for is_iterable_array.
    template<typename Type>
    using array_end_expr_t =
        decltype(std::end(std::declval<Type&>()));

NS_END  // internal

// is_iterable_array
//   trait: true if std::begin(t) and std::end(t) are both well-formed for an
// lvalue of Type. Uses the detection idiom from meta/type_traits.hpp so this
// works back to C++11.
template<typename Type>
struct is_iterable_array
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( is_detected<internal::array_begin_expr_t, cleaned>::value &&
          is_detected<internal::array_end_expr_t,   cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_iterable_array_v =
        is_iterable_array<Type>::value;
#endif

// is_non_iterable_array
//   trait: true if the type looks like an array (has data() and size()) but
// does NOT expose iteration.
template<typename Type>
struct is_non_iterable_array
{
private:
    using cleaned = clean_t<Type>;

public:
    static constexpr bool value =
        ( has_data_method<cleaned>::value     &&
          has_size_accessor<cleaned>::value   &&
          !is_iterable_array<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_non_iterable_array_v = is_non_iterable_array<Type>::value;
#endif


// ===========================================================================
// XI.  Strategy Classification
// ===========================================================================

enum class array_operations_strategy
{
    bulk_memcpy,
    element_move,
    circular,
    chunked,
    generic
};

NS_INTERNAL

    template<typename Type>
    struct array_strategy_helper
    {
    private:
        using cleaned = clean_t<Type>;

    public:
        static constexpr array_operations_strategy value =
            ( is_circular_buffer<cleaned>::value
                  ? array_operations_strategy::circular
                  : is_chunked_array<cleaned>::value
                      ? array_operations_strategy::chunked
                      : is_trivially_relocatable_array<cleaned>::value
                          ? array_operations_strategy::bulk_memcpy
                          : is_contiguous_array<cleaned>::value
                              ? array_operations_strategy::element_move
                              : array_operations_strategy::generic );
    };

NS_END  // internal

template<typename Type>
struct array_strategy
{
    static constexpr array_operations_strategy value =
        internal::array_strategy_helper<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr array_operations_strategy array_strategy_v =
        array_strategy<Type>::value;
#endif


// ===========================================================================
// XII. Combined Classification
// ===========================================================================

template<typename Type>
struct array_class
{
    // capacity model
    static constexpr capacity_model capacity = capacity_model_of<Type>::value;
    static constexpr bool is_fixed           = is_fixed_capacity<Type>::value;
    static constexpr bool is_dynamic         = is_dynamic_capacity<Type>::value;
    static constexpr bool is_sbo             = is_small_buffer_optimized<Type>::value;
    static constexpr bool has_static_size    = has_static_extent<Type>::value;
    // contiguity
    static constexpr bool is_contiguous = is_contiguous_array<Type>::value;
    // circular
    static constexpr bool is_circular = is_circular_buffer<Type>::value;
    // chunked
    static constexpr bool is_chunked = is_chunked_array<Type>::value;

    // element metrics
    static constexpr std::size_t elem_size  = element_size_of<Type>::value;
    static constexpr std::size_t elem_align = element_alignment_of<Type>::value;
    static constexpr bool trivially_relocatable =
        is_trivially_relocatable_array<Type>::value;

    // shift / rotation
    static constexpr bool is_shiftable =
        is_shiftable_array<Type>::value;

    // growth
    static constexpr bool has_reserve =
        has_reserve_method<Type>::value;
    static constexpr bool has_shrink =
        has_shrink_to_fit_method<Type>::value;
    static constexpr bool has_capacity_acc =
        has_capacity_method<Type>::value;

    // lifetime
    static constexpr array_lifetime lifetime =
        array_lifetime_of<Type>::value;
    static constexpr bool is_constexpr_life =
        is_constexpr_array<Type>::value;
    static constexpr bool is_immutable_life =
        is_immutable_array<Type>::value;
    static constexpr bool is_mutable_life =
        is_mutable_array<Type>::value;

    // iterability
    static constexpr bool is_iter_able =
        is_iterable_array<Type>::value;
    static constexpr bool is_non_iter_able =
        is_non_iterable_array<Type>::value;

    // strategy
    static constexpr array_operations_strategy strategy =
        array_strategy<Type>::value;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_ARRAY_TRAITS_HPP
