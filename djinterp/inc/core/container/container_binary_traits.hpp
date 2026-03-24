/******************************************************************************
* djinterp [container]                            container_binary_traits.hpp
*
* Container-aware binary serialization traits for the djinterp framework.
*   Provides compile-time detection of binary serialization capabilities
* at both the container and element level, enabling tagless dispatch to
* the most efficient serialization strategy.
*
*   Detection is organized into three tiers:
*     1. Bulk:    contiguous storage + trivially copyable elements
*                 — raw memcpy of the entire data region.
*     2. Native:  container exposes .serialize() / .to_binary()
*                 — delegate to the container's own method.
*     3. Element: iterable + each element individually
*                 serializable (trivially copyable, or has
*                 .serialize() / .to_binary()).
*
*   Additionally detects:
*     - type_info integration:  container exposes a
*       d_type_info16 / d_type_info64 via a type_info() or
*       type_descriptor static/member, enabling the binary
*       module to prepend a framework type header.
*     - Fixed-size encoding: element size is constexpr-known.
*     - Endian awareness: container declares endianness.
*
*   All detection is purely structural SFINAE.
*
* DEPENDENCIES:
*   container_traits.hpp  - container classification
*   type_info.h           - d_type_info16/32/64 typedefs
*
* TABLE OF CONTENTS
* =================
* I.      Element-Level Binary Detection
* II.     Container-Level Binary Detection
* III.    Type Info Integration Detection
* IV.     Contiguous Bulk Detection
* V.      Binary Strategy Classification
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
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "..\..\c\type_info.h"
#include "container_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Element-Level Binary Detection
// =============================================================================

NS_INTERNAL

    // safe_value_type (may already be visible from
    // container_traits; redeclared here for self-containment
    // when included independently)
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

    // --- element .serialize() detection ---

    template<typename _Elem, typename = void>
    struct elem_has_serialize : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_serialize<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().serialize(
                std::declval<char*>(),
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // --- element .to_binary() detection ---

    template<typename _Elem, typename = void>
    struct elem_has_to_binary : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_to_binary<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().to_binary())>>
        : std::true_type
    {};

    // --- element .byte_size() detection ---

    template<typename _Elem, typename = void>
    struct elem_has_byte_size : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_byte_size<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().byte_size())>>
        : std::true_type
    {};

NS_END  // internal

// has_trivially_copyable_elements
//   type trait: true if the container's value_type is
// trivially copyable (safe for memcpy/binary dump).
template<typename _Type>
struct has_trivially_copyable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::binary_safe_value_type_t<clean_type>;

    static constexpr bool value =
        std::is_trivially_copyable_v<elem_type>;
};

template<typename _Type>
inline constexpr bool has_trivially_copyable_elements_v =
    has_trivially_copyable_elements<_Type>::value;

// has_serializable_elements
//   type trait: true if the container's value_type has a
// .serialize(char*, size_t) member function.
template<typename _Type>
struct has_serializable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::binary_safe_value_type_t<clean_type>;

    static constexpr bool value =
        internal::elem_has_serialize<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_serializable_elements_v =
    has_serializable_elements<_Type>::value;

// has_to_binary_elements
//   type trait: true if the container's value_type has a
// .to_binary() member function returning a byte
// representation.
template<typename _Type>
struct has_to_binary_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::binary_safe_value_type_t<clean_type>;

    static constexpr bool value =
        internal::elem_has_to_binary<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_to_binary_elements_v =
    has_to_binary_elements<_Type>::value;

// has_fixed_element_size
//   type trait: true if the container's value_type has a
// compile-time known size suitable for binary layout.
// Trivially copyable types always qualify; non-trivial
// types qualify if they expose .byte_size().
template<typename _Type>
struct has_fixed_element_size
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::binary_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( std::is_trivially_copyable_v<elem_type> ||
          internal::elem_has_byte_size<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_fixed_element_size_v =
    has_fixed_element_size<_Type>::value;

// is_element_binary_capable
//   type trait: true if the container's value_type can be
// written to binary via any mechanism (trivially copyable,
// .serialize(), or .to_binary()).
template<typename _Type>
struct is_element_binary_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_trivially_copyable_elements_v<
              clean_type>                       ||
          has_serializable_elements_v<clean_type> ||
          has_to_binary_elements_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_element_binary_capable_v =
    is_element_binary_capable<_Type>::value;


// =============================================================================
// II.  Container-Level Binary Detection
// =============================================================================

// has_serialize_method
//   type trait: true if container has a
// .serialize(char*, size_t) member function.
D_TYPE_TRAIT_TRUE(has_serialize_method,
    decltype(std::declval<const _Type&>().serialize(
        std::declval<char*>(),
        std::declval<std::size_t>())))

// has_to_binary_method
//   type trait: true if container has a .to_binary() member
// function returning a byte representation.
D_TYPE_TRAIT_TRUE(has_to_binary_method,
    decltype(std::declval<const _Type&>().to_binary()))

// has_from_binary_method
//   type trait: true if container has a static
// .from_binary(const char*, size_t) factory method.
D_TYPE_TRAIT_TRUE(has_from_binary_method,
    decltype(_Type::from_binary(
        std::declval<const char*>(),
        std::declval<std::size_t>())))

// has_deserialize_method
//   type trait: true if container has a
// .deserialize(const char*, size_t) member function.
D_TYPE_TRAIT_TRUE(has_deserialize_method,
    decltype(std::declval<_Type&>().deserialize(
        std::declval<const char*>(),
        std::declval<std::size_t>())))

// has_byte_size_method
//   type trait: true if container has a .byte_size() member
// returning the serialized size in bytes.
D_TYPE_TRAIT_TRUE(has_byte_size_method,
    decltype(std::declval<const _Type&>().byte_size()))


// =============================================================================
// III. Type Info Integration Detection
// =============================================================================
// Containers that participate in the type_info system expose
// a descriptor via one of:
//   - static constexpr d_type_info16 type_descriptor
//   - static constexpr d_type_info64 type_descriptor
//   - static/member type_info() function
//   - using type_info_type = d_type_info16 / d_type_info64

D_TYPE_TRAIT_TRUE(has_type_descriptor_field,
    decltype(_Type::type_descriptor))

D_TYPE_TRAIT_TRUE(has_type_info_method,
    decltype(std::declval<const _Type&>().type_info()))

D_TYPE_TRAIT_TRUE(has_type_info_type,
    typename _Type::type_info_type)

D_TYPE_TRAIT_TRUE(has_static_type_info,
    decltype(_Type::type_info()))

// has_type_info_integration
//   type trait: true if container exposes type_info metadata
// through any supported mechanism.
template<typename _Type>
struct has_type_info_integration
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_type_descriptor_field_v<clean_type> ||
          has_type_info_method_v<clean_type>      ||
          has_type_info_type_v<clean_type>        ||
          has_static_type_info_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_type_info_integration_v =
    has_type_info_integration<_Type>::value;


// =============================================================================
// IV.  Contiguous Bulk Detection
// =============================================================================
// The fastest binary path: when a container stores trivially
// copyable elements in contiguous memory, the entire data
// region can be written with a single memcpy.

// is_bulk_binary_capable
//   type trait: true if the container can be serialized as a
// single contiguous block of bytes.
// Requirements:
//   1. Contiguous storage (has data() accessor).
//   2. Has size() accessor.
//   3. Elements are trivially copyable.
template<typename _Type>
struct is_bulk_binary_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_data_accessor_v<clean_type>                &&
          has_size_accessor_v<clean_type>                &&
          has_trivially_copyable_elements_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_bulk_binary_capable_v =
    is_bulk_binary_capable<_Type>::value;


// =============================================================================
// V.   Binary Strategy Classification
// =============================================================================

// DBinaryStrategy
//   enum: compile-time binary serialization strategy tags.
enum class DBinaryStrategy
{
    // contiguous + trivially copyable — single memcpy
    bulk,

    // container has .serialize(char*, size_t) — delegate
    native_serialize,

    // container has .to_binary() — delegate
    native_to_binary,

    // iterable + trivially copyable elements — per-element
    // memcpy
    element_trivial,

    // iterable + elements have .serialize()
    element_serialize,

    // iterable + elements have .to_binary()
    element_to_binary,

    // no binary path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct binary_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DBinaryStrategy value =

            // tier 1: bulk memcpy
            is_bulk_binary_capable_v<clean_type>
                ? DBinaryStrategy::bulk

            // tier 2: native container methods
            : has_serialize_method_v<clean_type>
                ? DBinaryStrategy::native_serialize

            : has_to_binary_method_v<clean_type>
                ? DBinaryStrategy::native_to_binary

            // tier 3: per-element iteration
            : ( is_iterable_container_v<clean_type> &&
                has_trivially_copyable_elements_v<
                    clean_type> )
                ? DBinaryStrategy::element_trivial

            : ( is_iterable_container_v<clean_type> &&
                has_serializable_elements_v<
                    clean_type> )
                ? DBinaryStrategy::element_serialize

            : ( is_iterable_container_v<clean_type> &&
                has_to_binary_elements_v<clean_type> )
                ? DBinaryStrategy::element_to_binary

            : DBinaryStrategy::unsupported;
    };

NS_END  // internal

// container_binary_strategy
//   type trait: determines the most efficient binary
// serialization strategy for the given container type.
template<typename _Type>
struct container_binary_strategy
{
    static constexpr DBinaryStrategy value =
        internal::binary_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DBinaryStrategy
    container_binary_strategy_v =
        container_binary_strategy<_Type>::value;

// --- deserialization strategy ---

// DBinaryDeserializeStrategy
//   enum: compile-time binary deserialization strategy.
enum class DBinaryDeserializeStrategy
{
    // contiguous + trivially copyable — bulk read
    bulk,

    // container has static from_binary() factory
    native_factory,

    // container has .deserialize() member
    native_deserialize,

    // no deserialization path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct binary_deserialize_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DBinaryDeserializeStrategy value =
            is_bulk_binary_capable_v<clean_type>
                ? DBinaryDeserializeStrategy::bulk

            : has_from_binary_method_v<clean_type>
                ? DBinaryDeserializeStrategy::native_factory

            : has_deserialize_method_v<clean_type>
                ? DBinaryDeserializeStrategy::
                      native_deserialize

            : DBinaryDeserializeStrategy::unsupported;
    };

NS_END  // internal

// container_binary_deserialize_strategy
//   type trait: determines the best deserialization strategy.
template<typename _Type>
struct container_binary_deserialize_strategy
{
    static constexpr DBinaryDeserializeStrategy value =
        internal::binary_deserialize_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DBinaryDeserializeStrategy
    container_binary_deserialize_strategy_v =
        container_binary_deserialize_strategy<
            _Type>::value;


// =============================================================================
// VI.  Convenience Predicates
// =============================================================================

// is_binary_serializable_container
//   type trait: true if any serialization strategy is
// available.
template<typename _Type>
struct is_binary_serializable_container
{
    static constexpr bool value =
        ( container_binary_strategy_v<_Type> !=
          DBinaryStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_binary_serializable_container_v =
    is_binary_serializable_container<_Type>::value;

// is_binary_deserializable_container
//   type trait: true if any deserialization strategy is
// available.
template<typename _Type>
struct is_binary_deserializable_container
{
    static constexpr bool value =
        ( container_binary_deserialize_strategy_v<
              _Type> !=
          DBinaryDeserializeStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool
    is_binary_deserializable_container_v =
        is_binary_deserializable_container<
            _Type>::value;

// is_binary_round_trip_capable
//   type trait: true if the container supports both
// serialization and deserialization.
template<typename _Type>
struct is_binary_round_trip_capable
{
    static constexpr bool value =
        ( is_binary_serializable_container_v<_Type> &&
          is_binary_deserializable_container_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_binary_round_trip_capable_v =
    is_binary_round_trip_capable<_Type>::value;


// =============================================================================
// VII. Combined Classification
// =============================================================================

// container_binary_class
//   struct: complete binary serialization classification.
// All members are static constexpr.
template<typename _Type>
struct container_binary_class
{
    // element-level capabilities
    static constexpr bool elements_trivially_copyable =
        has_trivially_copyable_elements_v<_Type>;
    static constexpr bool elements_serializable =
        has_serializable_elements_v<_Type>;
    static constexpr bool elements_to_binary =
        has_to_binary_elements_v<_Type>;
    static constexpr bool elements_fixed_size =
        has_fixed_element_size_v<_Type>;
    static constexpr bool elements_binary_capable =
        is_element_binary_capable_v<_Type>;

    // container-level capabilities
    static constexpr bool has_serialize =
        has_serialize_method_v<_Type>;
    static constexpr bool has_to_binary =
        has_to_binary_method_v<_Type>;
    static constexpr bool has_from_binary =
        has_from_binary_method_v<_Type>;
    static constexpr bool has_deserialize =
        has_deserialize_method_v<_Type>;
    static constexpr bool has_byte_size =
        has_byte_size_method_v<_Type>;
    static constexpr bool is_bulk_capable =
        is_bulk_binary_capable_v<_Type>;

    // type_info integration
    static constexpr bool has_type_info =
        has_type_info_integration_v<_Type>;
    static constexpr bool has_type_descriptor =
        has_type_descriptor_field_v<_Type>;

    // strategies
    static constexpr DBinaryStrategy
        serialize_strategy =
            container_binary_strategy_v<_Type>;
    static constexpr DBinaryDeserializeStrategy
        deserialize_strategy =
            container_binary_deserialize_strategy_v<
                _Type>;

    // aggregate
    static constexpr bool is_serializable =
        is_binary_serializable_container_v<_Type>;
    static constexpr bool is_deserializable =
        is_binary_deserializable_container_v<_Type>;
    static constexpr bool is_round_trip =
        is_binary_round_trip_capable_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_BINARY_TRAITS_
