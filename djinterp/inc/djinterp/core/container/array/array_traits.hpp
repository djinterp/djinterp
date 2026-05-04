/******************************************************************************
* djinterp [container]                                        array_traits.hpp
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
* path:      /inc/djinterp/container/array/meta/array_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.   C-array / std::array detection
2.   capacity model detection
3.   contiguity detection
4.   circular buffer detection
5.   chunked array detection
6.   element metrics
7.   shift and rotation detection
8.   growth policy detection
9.   lifetime classification
10.  iterability classification
11.  strategy classification
12.  combined classification
*/

#ifndef DJINTERP_CONTAINER_ARRAY_TRAITS_
#define DJINTERP_CONTAINER_ARRAY_TRAITS_ 1

// std
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
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
    //   trait: detects whether a type is an instantiation
    // of std::array<T, N>.
    template<typename _Type>
    struct is_std_array_helper : std::false_type
    {};

    template<typename _Elem,
             std::size_t _N>
    struct is_std_array_helper<std::array<_Elem, _N>>
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

    // small-buffer optimization: inline for small,
    // heap for large (e.g. llvm::SmallVector)
    small_buffer,

    // externally managed: data() is valid but the
    // container does not own the memory (span, view)
    external
};

// has_capacity_method
//   trait: detects a const member `capacity()`.
D_TYPE_TRAIT_TRUE(has_capacity_method,
    decltype(std::declval<const _Type&>().capacity()))

// has_reserve_method
//   trait: detects a member `reserve(size_t)`.
D_TYPE_TRAIT_TRUE(has_reserve_method,
    decltype(std::declval<_Type&>().reserve(
        std::declval<std::size_t>())))

// has_shrink_to_fit_method
//   trait: detects a member `shrink_to_fit()`.
D_TYPE_TRAIT_TRUE(has_shrink_to_fit_method,
    decltype(std::declval<_Type&>().shrink_to_fit()))

// has_max_size_method
//   trait: detects a const member `max_size()`.
D_TYPE_TRAIT_TRUE(has_max_size_method,
    decltype(std::declval<const _Type&>().max_size()))

NS_INTERNAL

    // has_extent_check
    //   helper: detects a static `extent` member.
    template<typename _Type,
             typename = void>
    struct has_extent_check : std::false_type
    {};

    template<typename _Type>
    struct has_extent_check<_Type, void_t<
        decltype(_Type::extent)
    >> : std::true_type
    {};

    // has_tuple_size_check
    //   helper: detects std::tuple_size specialization
    // (std::array pattern).
    template<typename _Type,
             typename = void>
    struct has_tuple_size_check : std::false_type
    {};

    template<typename _Type>
    struct has_tuple_size_check<_Type, void_t<
        decltype(std::tuple_size<_Type>::value)
    >> : std::true_type
    {};

NS_END  // internal

// has_static_extent
//   trait: true if the container has a compile-time known
// size (::extent or std::tuple_size).
template<typename _Type>
struct has_static_extent
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( internal::has_extent_check<cleaned>::value      ||
          internal::has_tuple_size_check<cleaned>::value  ||
          is_c_array<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_static_extent_v
    //   variable template: value of has_static_extent<_Type>.
    template<typename _Type>
    constexpr bool has_static_extent_v =
        has_static_extent<_Type>::value;
#endif

// is_fixed_capacity
//   trait: true if the array has compile-time fixed size
// and cannot grow.
template<typename _Type>
struct is_fixed_capacity
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_static_extent<cleaned>::value &&
          !has_reserve_method<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_fixed_capacity_v
    //   variable template: value of is_fixed_capacity<_Type>.
    template<typename _Type>
    constexpr bool is_fixed_capacity_v =
        is_fixed_capacity<_Type>::value;
#endif

// is_dynamic_capacity
//   trait: true if the array can grow (has reserve or
// capacity).
template<typename _Type>
struct is_dynamic_capacity
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_data_method<cleaned>::value        &&
          ( has_capacity_method<cleaned>::value  ||
            has_reserve_method<cleaned>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_dynamic_capacity_v
    //   variable template: value of is_dynamic_capacity<_Type>.
    template<typename _Type>
    constexpr bool is_dynamic_capacity_v =
        is_dynamic_capacity<_Type>::value;
#endif

NS_INTERNAL

    // has_inline_capacity_check
    //   helper: detects a static `inline_capacity` member
    // (small-buffer-optimized arrays advertise this).
    template<typename _Type,
             typename = void>
    struct has_inline_capacity_check : std::false_type
    {};

    template<typename _Type>
    struct has_inline_capacity_check<_Type, void_t<
        decltype(_Type::inline_capacity)
    >> : std::true_type
    {};

NS_END  // internal

// is_small_buffer_optimized
//   trait: true if the container advertises an inline
// capacity for small-buffer optimization.
template<typename _Type>
struct is_small_buffer_optimized
    : internal::has_inline_capacity_check<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_small_buffer_optimized_v
    //   variable template: value of
    // is_small_buffer_optimized<_Type>.
    template<typename _Type>
    constexpr bool is_small_buffer_optimized_v =
        is_small_buffer_optimized<_Type>::value;
#endif

NS_INTERNAL

    // capacity_model_helper
    //   trait: priority cascade selecting the array's
    // capacity model.  Order: small_buffer > fixed >
    // dynamic > external > none.
    template<typename _Type>
    struct capacity_model_helper
    {
    private:
        using cleaned = clean_t<_Type>;

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
template<typename _Type>
struct capacity_model_of
{
    static constexpr capacity_model value =
        internal::capacity_model_helper<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // capacity_model_of_v
    //   variable template: value of capacity_model_of<_Type>.
    template<typename _Type>
    constexpr capacity_model capacity_model_of_v =
        capacity_model_of<_Type>::value;
#endif


// ===========================================================================
// III. Contiguity Detection
// ===========================================================================

// is_contiguous_array
//   trait: true if the container is contiguous (data() +
// random-access iterators), or is a raw C array.
template<typename _Type>
struct is_contiguous_array
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_c_array<cleaned>::value         ||
          ( has_data_method<cleaned>::value  &&
            is_random_access_iterable<cleaned>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_contiguous_array_v
    //   variable template: value of is_contiguous_array<_Type>.
    template<typename _Type>
    constexpr bool is_contiguous_array_v =
        is_contiguous_array<_Type>::value;
#endif


// ===========================================================================
// IV.  Circular Buffer Detection
// ===========================================================================

// has_head_method, has_tail_method
//   These are owned by node_container_traits.hpp and re-exported
// via the include above.  Earlier revisions defined them here as
// well; the duplicate definitions caused ODR conflicts when both
// headers were pulled into the same TU.

// has_is_full_method
D_TYPE_TRAIT_TRUE(has_is_full_method,
    decltype(std::declval<const _Type&>().is_full()))

// has_push_front_method
D_TYPE_TRAIT_TRUE(has_push_front_method,
    decltype(std::declval<_Type&>().push_front(
        std::declval<typename _Type::value_type>())))

// has_pop_front_method
D_TYPE_TRAIT_TRUE(has_pop_front_method,
    decltype(std::declval<_Type&>().pop_front()))

// is_circular_buffer
//   trait: true if the container is a circular buffer
// (has head + tail + is_full + capacity).
template<typename _Type>
struct is_circular_buffer
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_head_method<cleaned>::value     &&
          has_tail_method<cleaned>::value     &&
          has_is_full_method<cleaned>::value  &&
          has_capacity_method<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_circular_buffer_v
    //   variable template: value of is_circular_buffer<_Type>.
    template<typename _Type>
    constexpr bool is_circular_buffer_v =
        is_circular_buffer<_Type>::value;
#endif


// ===========================================================================
// V.   Chunked Array Detection
// ===========================================================================

NS_INTERNAL

    // has_chunk_size_field_check
    //   helper: detects a static `chunk_size` member.
    template<typename _Type,
             typename = void>
    struct has_chunk_size_field_check : std::false_type
    {};

    template<typename _Type>
    struct has_chunk_size_field_check<_Type, void_t<
        decltype(_Type::chunk_size)
    >> : std::true_type
    {};

NS_END  // internal

// has_chunk_size_field
//   trait: true if the type exposes a static `chunk_size`
// member (compile-time chunk dimension).
template<typename _Type>
struct has_chunk_size_field
    : internal::has_chunk_size_field_check<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_chunk_size_field_v =
        has_chunk_size_field<_Type>::value;
#endif

// has_chunk_size_method
D_TYPE_TRAIT_TRUE(has_chunk_size_method,
    decltype(std::declval<const _Type&>().chunk_size()))

// has_chunk_count_method
D_TYPE_TRAIT_TRUE(has_chunk_count_method,
    decltype(std::declval<const _Type&>().chunk_count()))

// has_chunk_at_method
D_TYPE_TRAIT_TRUE(has_chunk_at_method,
    decltype(std::declval<const _Type&>().chunk_at(
        std::declval<std::size_t>())))

// is_chunked_array
//   trait: true if the container organizes its storage in
// fixed-size chunks (for hierarchical array layouts,
// B-tree nodes, etc.).
template<typename _Type>
struct is_chunked_array
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( ( has_chunk_size_field<cleaned>::value     ||
            has_chunk_size_method<cleaned>::value )  &&
          has_chunk_count_method<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_chunked_array_v
    //   variable template: value of is_chunked_array<_Type>.
    template<typename _Type>
    constexpr bool is_chunked_array_v =
        is_chunked_array<_Type>::value;
#endif


// ===========================================================================
// VI.  Element Metrics
// ===========================================================================

NS_INTERNAL

    // safe_value_type
    //   helper: extracts ::value_type, or void if absent
    // (raw C arrays are also handled by checking
    // std::remove_extent for arrays).
    template<typename _Type,
             typename = void>
    struct safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct safe_value_type<_Type, void_t<
        typename _Type::value_type
    >>
    {
        using type = typename _Type::value_type;
    };

    // safe_value_type_t
    template<typename _Type>
    using safe_value_type_t =
        typename safe_value_type<_Type>::type;

    // c_array_element
    //   helper: yields std::remove_extent<_Type>::type for
    // C arrays, void otherwise.
    template<typename _Type,
             bool _IsArr = std::is_array<_Type>::value>
    struct c_array_element
    {
        using type = void;
    };

    template<typename _Type>
    struct c_array_element<_Type, true>
    {
        using type = typename std::remove_extent<_Type>::type;
    };

    // resolved_element_type
    //   helper: prefers ::value_type, falls back to
    // remove_extent for C arrays.
    template<typename _Type>
    struct resolved_element_type
    {
    private:
        using cleaned    = clean_t<_Type>;
        using _Member   = safe_value_type_t<cleaned>;
        using _CArrElem = typename c_array_element<cleaned>::type;

    public:
        using type =
            typename std::conditional<
                std::is_void<_Member>::value,
                _CArrElem,
                _Member>::type;
    };

NS_END  // internal

// array_element_type_of
//   trait: yields the array's element type (value_type or
// remove_extent for raw C arrays), or void if undetermined.
template<typename _Type>
struct array_element_type_of
{
    using type =
        typename internal::resolved_element_type<_Type>::type;
};

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
    // array_element_type_of_t
    //   alias: convenience for
    // array_element_type_of<_Type>::type.
    template<typename _Type>
    using array_element_type_of_t =
        typename array_element_type_of<_Type>::type;
#endif

// element_size_of
//   trait: sizeof(value_type) when the element type is
// non-void, 0 otherwise.
template<typename _Type>
struct element_size_of
{
private:
    using _Elem =
        typename array_element_type_of<_Type>::type;

public:
    static constexpr std::size_t value =
        std::is_void<_Elem>::value ? 0 : sizeof(_Elem);
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // element_size_of_v
    //   variable template: value of element_size_of<_Type>.
    template<typename _Type>
    constexpr std::size_t element_size_of_v =
        element_size_of<_Type>::value;
#endif

// element_alignment_of
//   trait: alignof(value_type), 0 if undetermined.
template<typename _Type>
struct element_alignment_of
{
private:
    using _Elem =
        typename array_element_type_of<_Type>::type;

public:
    static constexpr std::size_t value =
        std::is_void<_Elem>::value ? 0 : alignof(_Elem);
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // element_alignment_of_v
    //   variable template: value of
    // element_alignment_of<_Type>.
    template<typename _Type>
    constexpr std::size_t element_alignment_of_v =
        element_alignment_of<_Type>::value;
#endif

// element_stride_of
//   trait: logical element stride; defaults to
// element_size_of.  Custom containers may specialize this
// for non-contiguous strided storage.
template<typename _Type>
struct element_stride_of
{
    static constexpr std::size_t value =
        element_size_of<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr std::size_t element_stride_of_v =
        element_stride_of<_Type>::value;
#endif

// is_trivially_relocatable_array
//   trait: true if elements can be relocated via
// memcpy/memmove (trivially copyable + trivially
// destructible - safe for realloc-style growth).
template<typename _Type>
struct is_trivially_relocatable_array
{
private:
    using _Elem =
        typename array_element_type_of<_Type>::type;

public:
    static constexpr bool value =
        ( is_contiguous_array<clean_t<_Type>>::value  &&
          !std::is_void<_Elem>::value                 &&
          std::is_trivially_copyable<_Elem>::value    &&
          std::is_trivially_destructible<_Elem>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_trivially_relocatable_array_v =
        is_trivially_relocatable_array<_Type>::value;
#endif


// ===========================================================================
// VII. Shift and Rotation Detection
// ===========================================================================

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
//   trait: true if the array supports logical shift
// operations (contiguous + sized).
template<typename _Type>
struct is_shiftable_array
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_contiguous_array<cleaned>::value  &&
          has_size_accessor<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_shiftable_array_v =
        is_shiftable_array<_Type>::value;
#endif


// ===========================================================================
// VIII. Growth Policy Detection
// ===========================================================================

NS_INTERNAL

    // has_growth_factor_field_check
    //   helper: detects a static `growth_factor` member.
    template<typename _Type,
             typename = void>
    struct has_growth_factor_field_check : std::false_type
    {};

    template<typename _Type>
    struct has_growth_factor_field_check<_Type, void_t<
        decltype(_Type::growth_factor)
    >> : std::true_type
    {};

NS_END  // internal

// has_growth_factor_field
template<typename _Type>
struct has_growth_factor_field
    : internal::has_growth_factor_field_check<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_growth_factor_field_v =
        has_growth_factor_field<_Type>::value;
#endif

// has_growth_factor_method
D_TYPE_TRAIT_TRUE(has_growth_factor_method,
    decltype(std::declval<const _Type&>().growth_factor()))

// has_resize_method
D_TYPE_TRAIT_TRUE(has_resize_method,
    decltype(std::declval<_Type&>().resize(
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

    // has_lifetime_marker
    //   helper: true if _Type exposes a static `lifetime`
    // member.  Our own array<> stamps the template parameter
    // into this member, so detection is exact whenever it
    // fires.  Foreign array-shaped types lack the marker and
    // fall through to duck-type detection in the predicates
    // below.
    template<typename _Type,
             typename = void>
    struct has_lifetime_marker : std::false_type
    {};

    template<typename _Type>
    struct has_lifetime_marker<_Type, void_t<
        decltype(_Type::lifetime)
    >> : std::true_type
    {};

    // marker_eq
    //   helper: true if _Type has a `lifetime` marker AND that
    // marker equals _V.  Two specializations: absent marker
    // -> false; present marker -> compare.  This is the
    // primary signal consulted by is_constexpr_array,
    // is_mutable_array, and is_immutable_array.
    template<typename _Type, array_lifetime _V,
             bool _Has = has_lifetime_marker<
                 clean_t<_Type>>::value>
    struct marker_eq : std::false_type
    {};

    template<typename _Type, array_lifetime _V>
    struct marker_eq<_Type, _V, true>
        : std::integral_constant<bool,
              (clean_t<_Type>::lifetime == _V)>
    {};

    // has_fill_check
    //   helper: detects a fill(value_type) member; the
    // canonical array-style bulk mutator.
    template<typename _Type,
             typename = void>
    struct has_fill_check : std::false_type
    {};

    template<typename _Type>
    struct has_fill_check<_Type, void_t<
        decltype(std::declval<_Type&>().fill(
            std::declval<typename _Type::value_type>()))
    >> : std::true_type
    {};

    // has_swap_check
    //   helper: detects a swap(_Type&) member.
    template<typename _Type,
             typename = void>
    struct has_swap_check : std::false_type
    {};

    template<typename _Type>
    struct has_swap_check<_Type, void_t<
        decltype(std::declval<_Type&>().swap(
            std::declval<_Type&>()))
    >> : std::true_type
    {};

    // has_mutable_subscript_check
    //   helper: detects a non-const operator[] yielding a
    // mutable lvalue.  The probe writes a value_type back
    // through the subscript, so it only succeeds when the
    // returned reference is non-const.
    template<typename _Type,
             typename = void>
    struct has_mutable_subscript_check : std::false_type
    {};

    template<typename _Type>
    struct has_mutable_subscript_check<_Type, void_t<
        decltype(std::declval<_Type&>()[std::size_t{}] =
                 std::declval<typename _Type::value_type>())
    >> : std::true_type
    {};

NS_END  // internal

// is_constexpr_array
//   trait: true if the array is intended for compile-time
// consumption.
// Detection priority:
//   1. lifetime marker present -> true iff
//      `lifetime == constexpr_lifetime`.  Authoritative for
//      our own array<> instantiations.
//   2. lifetime marker absent  -> fall through to
//      has_constexpr_iteration (foreign types like std::array,
//      raw C arrays, third-party containers).
template<typename _Type>
struct is_constexpr_array
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( internal::has_lifetime_marker<cleaned>::value
              ? internal::marker_eq<cleaned,
                    array_lifetime::constexpr_lifetime>::value
              : has_constexpr_iteration<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_constexpr_array_v =
        is_constexpr_array<_Type>::value;
#endif

// is_mutable_array
//   trait: true if the array exposes mutation.
// Detection priority:
//   1. lifetime marker present -> true iff
//      `lifetime == mutable_lifetime`.  Authoritative for our
//      own array<> instantiations.
//   2. lifetime marker absent  -> fall through to duck typing.
//      Recognizes BOTH vector-style growable mutators
//      (push_back, clear, resize, reserve) AND array-style
//      fixed-extent mutators (fill, swap, non-const
//      operator[]).  The earlier revision recognized only the
//      vector-style set, so fixed-extent mutable arrays were
//      misclassified as not-mutable.
NS_INTERNAL

    // has_push_back_check
    template<typename _Type,
             typename = void>
    struct has_push_back_check : std::false_type
    {};

    template<typename _Type>
    struct has_push_back_check<_Type, void_t<
        decltype(std::declval<_Type&>().push_back(
            std::declval<typename _Type::value_type>()))
    >> : std::true_type
    {};

    // has_clear_check
    template<typename _Type,
             typename = void>
    struct has_clear_check : std::false_type
    {};

    template<typename _Type>
    struct has_clear_check<_Type, void_t<
        decltype(std::declval<_Type&>().clear())
    >> : std::true_type
    {};

NS_END  // internal

// is_mutable_array
template<typename _Type>
struct is_mutable_array
{
private:
    using cleaned = clean_t<_Type>;

    // duck-typed fallback used only when the lifetime marker
    // is absent.  Recognizes both growable and fixed-extent
    // mutators.
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
    template<typename _Type>
    constexpr bool is_mutable_array_v =
        is_mutable_array<_Type>::value;
#endif

// is_immutable_array
//   trait: true if the array exposes data() but NOT
// mutation entry points.
// Detection priority:
//   1. lifetime marker present -> true iff
//      `lifetime == immutable_lifetime`.  Authoritative for
//      our own array<> instantiations.
//   2. lifetime marker absent  -> fall through to the
//      structural rule: contiguous AND not mutable AND not
//      constexpr.
template<typename _Type>
struct is_immutable_array
{
private:
    using cleaned = clean_t<_Type>;

    // duck-typed fallback used only when the lifetime marker
    // is absent.
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
    template<typename _Type>
    constexpr bool is_immutable_array_v =
        is_immutable_array<_Type>::value;
#endif

NS_INTERNAL

    // `has_lifetime_marker` was defined earlier in this header
    // (just after the `array_lifetime` enum) so the trait
    // predicates above could consume it.  No second definition
    // is needed here.

    // array_lifetime_helper
    //   trait: priority cascade selecting the array's
    // lifetime mode.  Detection priority:
    //   1. Explicit `lifetime` static member (set by our
    //      array<> primary template / specializations);
    //   2. Duck-type cascade for non-djinterp containers:
    //      constexpr > immutable > mutable.
    template<typename _Type,
             bool _HasMarker = has_lifetime_marker<_Type>::value>
    struct array_lifetime_helper
    {
    private:
        using cleaned = clean_t<_Type>;

    public:
        static constexpr array_lifetime value =
            is_constexpr_array<cleaned>::value
                ? array_lifetime::constexpr_lifetime

            : is_mutable_array<cleaned>::value
                ? array_lifetime::mutable_lifetime

            : array_lifetime::immutable_lifetime;
    };

    // partial specialization: marker present, use it directly.
    template<typename _Type>
    struct array_lifetime_helper<_Type, true>
    {
        static constexpr array_lifetime value =
            clean_t<_Type>::lifetime;
    };

NS_END  // internal

// array_lifetime_of
//   trait: deduces the array's lifetime mode.
template<typename _Type>
struct array_lifetime_of
{
    static constexpr array_lifetime value =
        internal::array_lifetime_helper<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr array_lifetime array_lifetime_of_v =
        array_lifetime_of<_Type>::value;
#endif


// ===========================================================================
// X.   Iterability Classification
// ===========================================================================
// Boolean axis: an array may be iterable (has begin/end)
// or non-iterable (raw storage with data()/size() but no
// iteration entry points).

// is_iterable_array
//   trait: true if the array provides at least input-level
// iteration via begin()/end().
template<typename _Type>
struct is_iterable_array
    : is_iterable<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_iterable_array_v =
        is_iterable_array<_Type>::value;
#endif

// is_non_iterable_array
//   trait: true if the type looks like an array (has
// data()) but does NOT expose iteration.
template<typename _Type>
struct is_non_iterable_array
{
private:
    using cleaned = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_data_method<cleaned>::value  &&
          has_size_accessor<cleaned>::value  &&
          !is_iterable<cleaned>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_non_iterable_array_v =
        is_non_iterable_array<_Type>::value;
#endif


// ===========================================================================
// XI.  Strategy Classification
// ===========================================================================

// array_operations_strategy
//   enum: classifies the best general-purpose strategy
// for bulk operations on this array.
enum class array_operations_strategy
{
    // contiguous + trivially relocatable - memcpy/
    // memmove for shifts, bulk copy, realloc
    bulk_memcpy,

    // contiguous + non-trivial elements - element-wise
    // move via move assignment
    element_move,

    // circular buffer - advance head/tail cursors
    circular,

    // chunked - operate per-chunk
    chunked,

    // non-contiguous / unknown
    generic
};

NS_INTERNAL

    // array_strategy_helper
    //   trait: priority cascade selecting the bulk-
    // operation strategy for an array.
    template<typename _Type>
    struct array_strategy_helper
    {
    private:
        using cleaned = clean_t<_Type>;

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

// array_strategy
//   trait: deduces the array's bulk-operation strategy.
template<typename _Type>
struct array_strategy
{
    static constexpr array_operations_strategy value =
        internal::array_strategy_helper<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr array_operations_strategy array_strategy_v =
        array_strategy<_Type>::value;
#endif


// ===========================================================================
// XII. Combined Classification
// ===========================================================================

// array_class
//   struct: aggregate compile-time classification of an
// array type along every axis defined in this header.
template<typename _Type>
struct array_class
{
    // capacity model
    static constexpr capacity_model capacity = capacity_model_of<_Type>::value;
    static constexpr bool is_fixed           = is_fixed_capacity<_Type>::value;
    static constexpr bool is_dynamic = is_dynamic_capacity<_Type>::value;
    static constexpr bool is_sbo = is_small_buffer_optimized<_Type>::value;
    static constexpr bool has_static_size = has_static_extent<_Type>::value;
    // contiguity
    static constexpr bool is_contiguous = is_contiguous_array<_Type>::value;
    // circular
    static constexpr bool is_circular = is_circular_buffer<_Type>::value;
    // chunked
    static constexpr bool is_chunked = is_chunked_array<_Type>::value;

    // element metrics
    static constexpr std::size_t elem_size = element_size_of<_Type>::value;
    static constexpr std::size_t elem_align = element_alignment_of<_Type>::value;
    static constexpr bool trivially_relocatable =
        is_trivially_relocatable_array<_Type>::value;

    // shift / rotation
    static constexpr bool is_shiftable =
        is_shiftable_array<_Type>::value;

    // growth
    static constexpr bool has_reserve =
        has_reserve_method<_Type>::value;
    static constexpr bool has_shrink =
        has_shrink_to_fit_method<_Type>::value;
    static constexpr bool has_capacity_acc =
        has_capacity_method<_Type>::value;

    // lifetime
    static constexpr array_lifetime lifetime =
        array_lifetime_of<_Type>::value;
    static constexpr bool is_constexpr_life =
        is_constexpr_array<_Type>::value;
    static constexpr bool is_immutable_life =
        is_immutable_array<_Type>::value;
    static constexpr bool is_mutable_life =
        is_mutable_array<_Type>::value;

    // iterability
    static constexpr bool is_iter_able =
        is_iterable_array<_Type>::value;
    static constexpr bool is_non_iter_able =
        is_non_iterable_array<_Type>::value;

    // strategy
    static constexpr array_operations_strategy strategy =
        array_strategy<_Type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ARRAY_TRAITS_