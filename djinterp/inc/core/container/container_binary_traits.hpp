/******************************************************************************
* djinterp [container]                            container_binary_traits.hpp
*
* Binary encoding/decoding traits for the djinterp container framework.
*   Detects whether a container can be converted to/from binary via
* the canonical method pair:
*
*   WRITE:  std::vector<char> encode() const
*   READ:   static C decode(const char* _data, std::size_t _size)
*
*   Detection is organized into three tiers per direction:
*     1. Native:   container has encode() / decode() directly.
*     2. Bulk:     contiguous trivially-copyable storage — memcpy.
*     3. Element:  iterable/output-capable + elements individually
*                  encodable/decodable.
*
*   All detection is purely structural SFINAE.
*
* TABLE OF CONTENTS
* =================
* I.      Container-Level Detection
* II.     Element-Level Detection
* III.    Bulk Detection
* IV.     Type Info Integration
* V.      Strategy Classification
* VI.     Convenience Predicates
* VII.    Combined Classification
*
*
* path:      \inc\container\meta\container_binary_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_BINARY_TRAITS_
#define DJINTERP_CONTAINER_BINARY_TRAITS_ 1

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "..\..\c\type_info.h"
#include "container_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Container-Level Detection
// =============================================================================

// has_encode_method
//   type trait: true if container has
//     std::vector<char> encode() const
D_TYPE_TRAIT_TRUE(has_encode_method,
    decltype(std::declval<const _Type&>().encode()))

// has_decode_method
//   type trait: true if container has static
//     C decode(const char*, std::size_t)
D_TYPE_TRAIT_TRUE(has_decode_method,
    decltype(_Type::decode(
        std::declval<const char*>(),
        std::declval<std::size_t>())))

// has_byte_size_method
//   type trait: true if container has
//     std::size_t byte_size() const
D_TYPE_TRAIT_TRUE(has_byte_size_method,
    decltype(
        std::declval<const _Type&>().byte_size()))


// =============================================================================
// II.  Element-Level Detection
// =============================================================================

NS_INTERNAL

    template<typename _Type, typename = void>
    struct binary_safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct binary_safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using binary_safe_value_type_t =
        typename binary_safe_value_type<_Type>::type;

    // element has encode()
    template<typename _Elem, typename = void>
    struct elem_has_encode : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_encode<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().encode())>>
        : std::true_type
    {};

    // element has static decode(const char*, size_t)
    template<typename _Elem, typename = void>
    struct elem_has_decode : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_decode<_Elem,
        std::void_t<decltype(
            _Elem::decode(
                std::declval<const char*>(),
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

NS_END  // internal

// has_trivially_copyable_elements
template<typename _Type>
struct has_trivially_copyable_elements
{
    using elem_type =
        internal::binary_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        std::is_trivially_copyable_v<elem_type>;
};

template<typename _Type>
inline constexpr bool
    has_trivially_copyable_elements_v =
        has_trivially_copyable_elements<_Type>::value;

// has_encodable_elements
//   type trait: trivially copyable or has encode().
template<typename _Type>
struct has_encodable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::binary_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( std::is_trivially_copyable_v<elem_type> ||
          internal::elem_has_encode<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_encodable_elements_v =
    has_encodable_elements<_Type>::value;

// has_decodable_elements
//   type trait: trivially copyable or has decode().
template<typename _Type>
struct has_decodable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::binary_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( std::is_trivially_copyable_v<elem_type> ||
          internal::elem_has_decode<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_decodable_elements_v =
    has_decodable_elements<_Type>::value;


// =============================================================================
// III. Bulk Detection
// =============================================================================

// has_resize_method
D_TYPE_TRAIT_TRUE(has_resize_method,
    decltype(std::declval<_Type&>().resize(
        std::declval<std::size_t>())))

// is_bulk_encodable
//   type trait: data() + size() + trivially copyable.
template<typename _Type>
struct is_bulk_encodable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_data_accessor_v<clean_type>            &&
          has_size_accessor_v<clean_type>            &&
          has_trivially_copyable_elements_v<
              clean_type> );
};

template<typename _Type>
inline constexpr bool is_bulk_encodable_v =
    is_bulk_encodable<_Type>::value;

// is_bulk_decodable
//   type trait: data() + resize() + trivially copyable.
template<typename _Type>
struct is_bulk_decodable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_data_accessor_v<clean_type>            &&
          has_resize_method_v<clean_type>            &&
          has_trivially_copyable_elements_v<
              clean_type> );
};

template<typename _Type>
inline constexpr bool is_bulk_decodable_v =
    is_bulk_decodable<_Type>::value;


// =============================================================================
// IV.  Type Info Integration
// =============================================================================

D_TYPE_TRAIT_TRUE(has_type_descriptor_field,
    decltype(_Type::type_descriptor))

D_TYPE_TRAIT_TRUE(has_type_info_method,
    decltype(
        std::declval<const _Type&>().type_info()))

template<typename _Type>
struct has_type_info_integration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_type_descriptor_field_v<clean_type> ||
          has_type_info_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_type_info_integration_v =
    has_type_info_integration<_Type>::value;


// =============================================================================
// V.   Strategy Classification
// =============================================================================

// --- encode strategy ---

enum class DBinaryEncodeStrategy
{
    native,       // container has encode()
    bulk,         // contiguous + trivially copyable
    element,      // iterable + elements encodable
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct encode_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr DBinaryEncodeStrategy value =
            has_encode_method_v<C>
                ? DBinaryEncodeStrategy::native

            : is_bulk_encodable_v<C>
                ? DBinaryEncodeStrategy::bulk

            : ( is_iterable_container_v<C> &&
                has_encodable_elements_v<C> )
                ? DBinaryEncodeStrategy::element

            : DBinaryEncodeStrategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_encode_strategy
{
    static constexpr DBinaryEncodeStrategy value =
        internal::encode_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DBinaryEncodeStrategy
    container_encode_strategy_v =
        container_encode_strategy<_Type>::value;

// --- decode strategy ---

enum class DBinaryDecodeStrategy
{
    native,       // container has static decode()
    bulk,         // contiguous + resize + trivially copyable
    element,      // output-capable + elements decodable
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct decode_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr DBinaryDecodeStrategy value =
            has_decode_method_v<C>
                ? DBinaryDecodeStrategy::native

            : is_bulk_decodable_v<C>
                ? DBinaryDecodeStrategy::bulk

            : ( ( has_push_back_v<C> ||
                  has_insert_v<C> )  &&
                has_decodable_elements_v<C> )
                ? DBinaryDecodeStrategy::element

            : DBinaryDecodeStrategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_decode_strategy
{
    static constexpr DBinaryDecodeStrategy value =
        internal::decode_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DBinaryDecodeStrategy
    container_decode_strategy_v =
        container_decode_strategy<_Type>::value;


// =============================================================================
// VI.  Convenience Predicates
// =============================================================================

template<typename _Type>
struct is_binary_encodable
{
    static constexpr bool value =
        ( container_encode_strategy_v<_Type> !=
          DBinaryEncodeStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_binary_encodable_v =
    is_binary_encodable<_Type>::value;

template<typename _Type>
struct is_binary_decodable
{
    static constexpr bool value =
        ( container_decode_strategy_v<_Type> !=
          DBinaryDecodeStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_binary_decodable_v =
    is_binary_decodable<_Type>::value;

template<typename _Type>
struct is_binary_round_trip
{
    static constexpr bool value =
        ( is_binary_encodable_v<_Type> &&
          is_binary_decodable_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_binary_round_trip_v =
    is_binary_round_trip<_Type>::value;


// =============================================================================
// VII. Combined Classification
// =============================================================================

template<typename _Type>
struct container_binary_class
{
    // container-level
    static constexpr bool has_encode =
        has_encode_method_v<_Type>;
    static constexpr bool has_decode =
        has_decode_method_v<_Type>;
    static constexpr bool has_byte_size =
        has_byte_size_method_v<_Type>;

    // element-level
    static constexpr bool elems_trivial =
        has_trivially_copyable_elements_v<_Type>;
    static constexpr bool elems_encodable =
        has_encodable_elements_v<_Type>;
    static constexpr bool elems_decodable =
        has_decodable_elements_v<_Type>;

    // bulk
    static constexpr bool bulk_encodable =
        is_bulk_encodable_v<_Type>;
    static constexpr bool bulk_decodable =
        is_bulk_decodable_v<_Type>;

    // type info
    static constexpr bool has_type_info =
        has_type_info_integration_v<_Type>;

    // strategies
    static constexpr DBinaryEncodeStrategy
        encode_strategy =
            container_encode_strategy_v<_Type>;
    static constexpr DBinaryDecodeStrategy
        decode_strategy =
            container_decode_strategy_v<_Type>;

    // aggregate
    static constexpr bool is_encodable =
        is_binary_encodable_v<_Type>;
    static constexpr bool is_decodable =
        is_binary_decodable_v<_Type>;
    static constexpr bool is_round_trip =
        is_binary_round_trip_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_BINARY_TRAITS_
