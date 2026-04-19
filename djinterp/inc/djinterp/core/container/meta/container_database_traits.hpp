/******************************************************************************
* djinterp [container]                          container_database_traits.hpp
*
* Database persistence traits for the djinterp container framework.
*   Detects whether a container can be persisted to/from a database via
* the canonical method pair:
*
*   WRITE:  void serialize(Connection& _conn) const
*   READ:   static C deserialize(Connection& _conn,
*                                const std::string& _source)
*
*   The connection type is templated — vendor-specific modules
* (sqlite, postgresql, etc.) supply the concrete connection type.
* Detection uses &_Type::serialize / &_Type::deserialize existence
* checks so the probe works regardless of the connection template
* parameter.
*
*   Additionally detects element-level row mapping and schema
* metadata for the strategy fallback tiers.
*
* TABLE OF CONTENTS
* =================
* I.      Container-Level Detection
* II.     Element-Level Detection
* III.    Schema Protocol Detection
* IV.     Field Type Mapping
* V.      Strategy Classification
* VI.     Convenience Predicates
* VII.    Combined Classification
*
*
* path:      \inc\container\meta\container_database_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_DATABASE_TRAITS_
#define DJINTERP_CONTAINER_DATABASE_TRAITS_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "container_traits.hpp"
#include "..\..\database\database_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Container-Level Detection
// =============================================================================

// has_serialize_method
//   type trait: true if container has a serialize method.
//     void serialize(Connection& _conn) const
// Detected via &_Type::serialize (method pointer
// existence), agnostic to connection type.
NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_serialize_check : std::false_type
    {};

    template<typename _Type>
    struct has_serialize_check<_Type,
        std::void_t<decltype(
            &_Type::serialize)>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_deserialize_check : std::false_type
    {};

    template<typename _Type>
    struct has_deserialize_check<_Type,
        std::void_t<decltype(
            &_Type::deserialize)>>
        : std::true_type
    {};

NS_END  // internal

// has_serialize_method
template<typename _Type>
struct has_serialize_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_serialize_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_serialize_method_v =
    has_serialize_method<_Type>::value;

// has_deserialize_method
template<typename _Type>
struct has_deserialize_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_deserialize_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_deserialize_method_v =
    has_deserialize_method<_Type>::value;


// =============================================================================
// II.  Element-Level Detection
// =============================================================================

NS_INTERNAL

    template<typename _Type, typename = void>
    struct db_safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct db_safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using db_safe_value_type_t =
        typename db_safe_value_type<_Type>::type;

    // element .to_row() — converts to database::row
    template<typename _Elem, typename = void>
    struct elem_has_to_row : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_to_row<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().to_row())>>
        : std::true_type
    {};

    // element static from_row(const row&)
    template<typename _Elem, typename = void>
    struct elem_has_from_row : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_from_row<_Elem,
        std::void_t<decltype(
            _Elem::from_row(
                std::declval<
                    const database::row&>()))>>
        : std::true_type
    {};

    // element is a database::row directly
    template<typename _Elem>
    struct elem_is_db_row
        : std::is_same<_Elem, database::row>
    {};

    // element is a database primitive (arithmetic or
    // string — storable in a single column)
    template<typename _Elem>
    struct elem_is_db_primitive
        : std::integral_constant<bool,
              ( std::is_arithmetic_v<_Elem> ||
                std::is_same_v<_Elem,
                    std::string> )>
    {};

NS_END  // internal

// has_row_mappable_elements
//   type trait: elements have to_row() and from_row().
template<typename _Type>
struct has_row_mappable_elements
{
    using elem_type =
        internal::db_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        ( internal::elem_has_to_row<
              elem_type>::value &&
          internal::elem_has_from_row<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_row_mappable_elements_v =
    has_row_mappable_elements<_Type>::value;

// has_db_row_elements
//   type trait: elements are database::row directly.
template<typename _Type>
struct has_db_row_elements
{
    using elem_type =
        internal::db_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        internal::elem_is_db_row<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_db_row_elements_v =
    has_db_row_elements<_Type>::value;

// has_db_primitive_elements
//   type trait: elements are primitives (arithmetic or
// string).
template<typename _Type>
struct has_db_primitive_elements
{
    using elem_type =
        internal::db_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        internal::elem_is_db_primitive<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_db_primitive_elements_v =
    has_db_primitive_elements<_Type>::value;

// is_element_db_capable
//   type trait: element can participate in database
// operations via any mechanism.
template<typename _Type>
struct is_element_db_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_row_mappable_elements_v<clean_type> ||
          has_db_row_elements_v<clean_type>       ||
          has_db_primitive_elements_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_element_db_capable_v =
    is_element_db_capable<_Type>::value;


// =============================================================================
// III. Schema Protocol Detection
// =============================================================================

D_TYPE_TRAIT_TRUE(has_table_name,
    decltype(std::declval<const _Type&>()
        .table_name()))

D_TYPE_TRAIT_TRUE(has_schema_name,
    decltype(std::declval<const _Type&>()
        .schema_name()))

D_TYPE_TRAIT_TRUE(has_create_table_sql,
    decltype(std::declval<const _Type&>()
        .create_table_sql()))

// has_schema
//   type trait: true if container or its elements expose
// at least table_name.
template<typename _Type>
struct has_schema
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( has_table_name_v<clean_type> ||
          has_table_name_v<elem_type> );
};

template<typename _Type>
inline constexpr bool has_schema_v =
    has_schema<_Type>::value;


// =============================================================================
// IV.  Field Type Mapping
// =============================================================================

NS_INTERNAL

    template<typename _Elem>
    struct cpp_to_field_type
    {
        static constexpr database::field_type value =
            database::field_type::custom;
    };

    template<>
    struct cpp_to_field_type<bool>
    {
        static constexpr database::field_type value =
            database::field_type::boolean;
    };

    template<>
    struct cpp_to_field_type<std::int32_t>
    {
        static constexpr database::field_type value =
            database::field_type::integer;
    };

    template<>
    struct cpp_to_field_type<std::int64_t>
    {
        static constexpr database::field_type value =
            database::field_type::big_integer;
    };

    template<>
    struct cpp_to_field_type<float>
    {
        static constexpr database::field_type value =
            database::field_type::floating_point;
    };

    template<>
    struct cpp_to_field_type<double>
    {
        static constexpr database::field_type value =
            database::field_type::floating_point;
    };

    template<>
    struct cpp_to_field_type<std::string>
    {
        static constexpr database::field_type value =
            database::field_type::string;
    };

    template<>
    struct cpp_to_field_type<
        std::vector<std::uint8_t>>
    {
        static constexpr database::field_type value =
            database::field_type::binary;
    };

NS_END  // internal

// element_field_type
template<typename _Type>
struct element_field_type
{
    using elem_type =
        internal::db_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr database::field_type value =
        internal::cpp_to_field_type<
            elem_type>::value;
};

template<typename _Type>
inline constexpr database::field_type
    element_field_type_v =
        element_field_type<_Type>::value;


// =============================================================================
// V.   Strategy Classification
// =============================================================================

// --- serialize strategy ---

enum class DDbSerializeStrategy
{
    // container has serialize(conn) — delegate
    native,

    // iterable + elements have to_row() — iterate
    // and insert each row
    element_row,

    // iterable + elements are primitives — iterate
    // and insert each value
    element_primitive,

    // no serialize path
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct serialize_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr DDbSerializeStrategy value =
            has_serialize_method_v<C>
                ? DDbSerializeStrategy::native

            : ( is_iterable_container_v<C> &&
                has_row_mappable_elements_v<C> )
                ? DDbSerializeStrategy::element_row

            : ( is_iterable_container_v<C> &&
                has_db_primitive_elements_v<C> )
                ? DDbSerializeStrategy::
                      element_primitive

            : DDbSerializeStrategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_serialize_strategy
{
    static constexpr DDbSerializeStrategy value =
        internal::serialize_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DDbSerializeStrategy
    container_serialize_strategy_v =
        container_serialize_strategy<_Type>::value;

// --- deserialize strategy ---

enum class DDbDeserializeStrategy
{
    // container has static deserialize(conn, src)
    native,

    // output-capable + elements have from_row()
    element_row,

    // output-capable + elements are primitives
    element_primitive,

    // no deserialize path
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct deserialize_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr DDbDeserializeStrategy value
            = has_deserialize_method_v<C>
                ? DDbDeserializeStrategy::native

            : ( ( has_push_back_v<C> ||
                  has_insert_v<C> )  &&
                has_row_mappable_elements_v<C> )
                ? DDbDeserializeStrategy::element_row

            : ( ( has_push_back_v<C> ||
                  has_insert_v<C> )  &&
                has_db_primitive_elements_v<C> )
                ? DDbDeserializeStrategy::
                      element_primitive

            : DDbDeserializeStrategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_deserialize_strategy
{
    static constexpr DDbDeserializeStrategy value =
        internal::deserialize_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DDbDeserializeStrategy
    container_deserialize_strategy_v =
        container_deserialize_strategy<_Type>::value;


// =============================================================================
// VI.  Convenience Predicates
// =============================================================================

template<typename _Type>
struct is_db_serializable
{
    static constexpr bool value =
        ( container_serialize_strategy_v<_Type> !=
          DDbSerializeStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_db_serializable_v =
    is_db_serializable<_Type>::value;

template<typename _Type>
struct is_db_deserializable
{
    static constexpr bool value =
        ( container_deserialize_strategy_v<_Type> !=
          DDbDeserializeStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_db_deserializable_v =
    is_db_deserializable<_Type>::value;

template<typename _Type>
struct is_db_round_trip
{
    static constexpr bool value =
        ( is_db_serializable_v<_Type> &&
          is_db_deserializable_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_db_round_trip_v =
    is_db_round_trip<_Type>::value;


// =============================================================================
// VII. Combined Classification
// =============================================================================

template<typename _Type>
struct container_database_class
{
    // container-level
    static constexpr bool has_serialize =
        has_serialize_method_v<_Type>;
    static constexpr bool has_deserialize =
        has_deserialize_method_v<_Type>;

    // element-level
    static constexpr bool elems_row_mappable =
        has_row_mappable_elements_v<_Type>;
    static constexpr bool elems_db_row =
        has_db_row_elements_v<_Type>;
    static constexpr bool elems_db_primitive =
        has_db_primitive_elements_v<_Type>;
    static constexpr bool elems_db_capable =
        is_element_db_capable_v<_Type>;

    // field type
    static constexpr database::field_type
        elem_field_type =
            element_field_type_v<_Type>;

    // schema
    static constexpr bool has_any_schema =
        has_schema_v<_Type>;
    static constexpr bool has_create_sql =
        has_create_table_sql_v<_Type>;

    // strategies
    static constexpr DDbSerializeStrategy
        serialize_strategy =
            container_serialize_strategy_v<_Type>;
    static constexpr DDbDeserializeStrategy
        deserialize_strategy =
            container_deserialize_strategy_v<_Type>;

    // aggregate
    static constexpr bool is_serializable =
        is_db_serializable_v<_Type>;
    static constexpr bool is_deserializable =
        is_db_deserializable_v<_Type>;
    static constexpr bool is_round_trip =
        is_db_round_trip_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_DATABASE_TRAITS_
