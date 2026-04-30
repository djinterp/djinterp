/******************************************************************************
* djinterp [container]                           container_database_traits.hpp
*
* Database persistence traits for the djinterp container framework.
*   Detects whether a container can be persisted to/from a database via
* the canonical method pair:
*   WRITE:  void serialize(Connection& _conn) const
*   READ:   static C deserialize(Connection& _conn,
*                                const std::string& _source)
* 
*   The connection type is templated - vendor-specific modules
* (sqlite, postgresql, etc.) supply the concrete connection type.
* Detection uses &_Type::serialize / &_Type::deserialize existence
* checks so the probe works regardless of the connection template
* parameter.
*   Additionally detects element-level row mapping and schema
* metadata for the strategy fall tiers.
*
*
* path:      /inc/djinterp/core/container/traits/container_database_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.23
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    container-level detection
2.    element-level detection
3.    schema protocol detection
4.    field type mapping
5.    strategy classification
6.    convenience predicates
7.    combined classification
*/


#ifndef DJINTERP_CONTAINER_DATABASE_TRAITS_
#define DJINTERP_CONTAINER_DATABASE_TRAITS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../db/database_traits.hpp"
#include "./container_traits.hpp"


NS_DJINTERP


D_TYPE_TRAIT_TRUE(has_table_name,
                  decltype(std::declval<const _Type&>().table_name()))

D_TYPE_TRAIT_TRUE(has_schema_name,
                  decltype(std::declval<const _Type&>().schema_name()))

D_TYPE_TRAIT_TRUE(has_create_table_sql,
                  decltype(std::declval<const _Type&>().create_table_sql()))

// ===========================================================================
// I.   container-level detection
// ===========================================================================

// has_serialize_method
//   type trait: true if container has a serialize method.
//     void serialize(Connection& _conn) const
// Detected via &_Type::serialize (method pointer
// existence), agnostic to connection type.
NS_INTERNAL
    template<typename _Type, 
             typename = void>
    struct has_serialize_check : std::false_type
    {};

    template<typename _Type>
    struct has_serialize_check<_Type,
        std::void_t<decltype(
            &_Type::serialize)>>
        : std::true_type
    {};

    template<typename _Type,
             typename = void>
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

    static constexpr bool value = internal::has_serialize_check<clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_serialize_method_v = has_serialize_method<_Type>::value;

// has_deserialize_method
template<typename _Type>
struct has_deserialize_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value = internal::has_deserialize_check<clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_deserialize_method_v = has_deserialize_method<_Type>::value;


// ===========================================================================
// II.  element-level detection
// ===========================================================================

NS_INTERNAL
    template<typename _Type,
             typename = void>
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
    using db_safe_value_type_t = typename db_safe_value_type<_Type>::type;

    // element .to_row() - converts to row
    template<typename _Element, 
             typename = void>
    struct elem_has_to_row : std::false_type
    {};

    template<typename _Element>
    struct elem_has_to_row<_Element,
        std::void_t<decltype(
            std::declval<const _Element&>().to_row())>>
        : std::true_type
    {};

    // element static from_row(const row&)
    template<typename _Element, 
             typename = void>
    struct elem_has_from_row : std::false_type
    {};

    template<typename _Element>
    struct elem_has_from_row<_Element,
        std::void_t<decltype(
            _Element::from_row(
                std::declval<
                    const row&>()))>>
        : std::true_type
    {};

    // element is a row directly
    template<typename _Element>
    struct elem_is_db_row
        : std::is_same<_Element, row>
    {};

    // element is a database primitive (arithmetic or
    // string - storable in a single column)
    template<typename _Element>
    struct elem_is_db_primitive
        : std::integral_constant<bool,
              ( std::is_arithmetic_v<_Element> ||
                std::is_same_v<_Element,
                    std::string> )>
    {};

NS_END  // internal

// has_row_mappable_elements
//   type trait: elements have to_row() and from_row().
template<typename _Type>
struct has_row_mappable_elements
{
    using element_type = internal::db_safe_value_type_t<clean_t<_Type>>;

    static constexpr bool value =
        ( internal::elem_has_to_row<element_type>::value &&
          internal::elem_has_from_row<element_type>::value );
};

template<typename _Type>
inline constexpr bool has_row_mappable_elements_v = has_row_mappable_elements<_Type>::value;

// has_db_row_elements
//   type trait: elements are row directly.
template<typename _Type>
struct has_db_row_elements
{
    using element_type = internal::db_safe_value_type_t<clean_t<_Type>>;

    static constexpr bool value = internal::elem_is_db_row<element_type>::value;
};

template<typename _Type>
inline constexpr bool has_db_row_elements_v = has_db_row_elements<_Type>::value;

// has_db_primitive_elements
//   type trait: elements are primitives (arithmetic or
// string).
template<typename _Type>
struct has_db_primitive_elements
{
    using element_type = internal::db_safe_value_type_t<clean_t<_Type>>;

    static constexpr bool value = internal::elem_is_db_primitive<element_type>::value;
};

template<typename _Type>
inline constexpr bool has_db_primitive_elements_v = has_db_primitive_elements<_Type>::value;

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
inline constexpr bool is_element_db_capable_v = is_element_db_capable<_Type>::value;


// ===========================================================================
// 3. schema protocol detection
// ===========================================================================

// has_schema
//   type trait: true if container or its elements expose
// at least table_name.
template<typename _Type>
struct has_schema
{
    using clean_type   = clean_t<_Type>;
    using element_type = internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( has_table_name_v<clean_type> ||
          has_table_name_v<element_type> );
};

template<typename _Type>
inline constexpr bool has_schema_v = has_schema<_Type>::value;


// ===========================================================================
// 4.  field type mapping
// ===========================================================================

NS_INTERNAL
    template<typename _Element>
    struct cpp_to_field_type
    {
        static constexpr field_type value = field_type::custom;
    };

    template<>
    struct cpp_to_field_type<bool>
    {
        static constexpr field_type value = field_type::boolean;
    };

    template<>
    struct cpp_to_field_type<std::int32_t>
    {
        static constexpr field_type value = field_type::integer;
    };

    template<>
    struct cpp_to_field_type<std::int64_t>
    {
        static constexpr field_type value = field_type::big_integer;
    };

    template<>
    struct cpp_to_field_type<float>
    {
        static constexpr field_type value = field_type::floating_point;
    };

    template<>
    struct cpp_to_field_type<double>
    {
        static constexpr field_type value = field_type::floating_point;
    };

    template<>
    struct cpp_to_field_type<std::string>
    {
        static constexpr field_type value = field_type::string;
    };

    template<>
    struct cpp_to_field_type<std::vector<std::uint8_t>>
    {
        static constexpr field_type value = field_type::binary;
    };

NS_END  // internal

// element_field_type
template<typename _Type>
struct element_field_type
{
    using element_type = internal::db_safe_value_type_t<clean_t<_Type>>;

    static constexpr field_type value = internal::cpp_to_field_type<element_type>::value;
};

template<typename _Type>
inline constexpr field_type element_field_type_v = element_field_type<_Type>::value;


// ===========================================================================
// 5.   strategy classification
// ===========================================================================

// --- serialize strategy ---

enum class database_serialize_strategy
{
    // container has serialize(conn) - delegate
    native,

    // iterable + elements have to_row() - iterate
    // and insert each row
    element_row,

    // iterable + elements are primitives - iterate
    // and insert each value
    element_primitive,

    // no serialize path
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct serialize_strategy_helper
    {
        using clean_type = clean_t<_Type>;

        static constexpr database_serialize_strategy value =
            has_serialize_method_v<clean_type>
                ? database_serialize_strategy::native
                : ( is_iterable_container_v<clean_type> &&
                    has_row_mappable_elements_v<clean_type> )
                    ? database_serialize_strategy::element_row
                    : ( is_iterable_container_v<clean_type> &&
                        has_db_primitive_elements_v<clean_type> )
                        ? database_serialize_strategy::element_primitive
                        : database_serialize_strategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_serialize_strategy
{
    static constexpr database_serialize_strategy value = internal::serialize_strategy_helper<_Type>::value;
};

template<typename _Type>
inline constexpr database_serialize_strategy container_serialize_strategy_v = container_serialize_strategy<_Type>::value;

// --- deserialize strategy ---

enum class database_deserialize_strategy
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
    struct deserialize_strategy_helper
    {
        using clean_type = clean_t<_Type>;

        static constexpr database_deserialize_strategy value = 
            has_deserialize_method_v<clean_type>
                ? database_deserialize_strategy::native
                : ( ( has_push__v<clean_type> ||
                      has_insert_v<clean_type> )  &&
                      has_row_mappable_elements_v<clean_type> )
                    ? database_deserialize_strategy::element_row
                    : ( ( has_push__v<clean_type> ||
                          has_insert_v<clean_type> )  &&
                          has_db_primitive_elements_v<clean_type> )
                        ? database_deserialize_strategy::element_primitive
                        : database_deserialize_strategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_deserialize_strategy
{
    static constexpr database_deserialize_strategy value = internal::deserialize_strategy_helper<_Type>::value;
};

template<typename _Type>
inline constexpr database_deserialize_strategy container_deserialize_strategy_v = container_deserialize_strategy<_Type>::value;


// ===========================================================================
// 6.  convenience predicates
// ===========================================================================

template<typename _Type>
struct is_db_serializable
{
    static constexpr bool value =
        ( container_serialize_strategy_v<_Type> !=
          database_serialize_strategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_db_serializable_v = is_db_serializable<_Type>::value;

template<typename _Type>
struct is_db_deserializable
{
    static constexpr bool value =
        ( container_deserialize_strategy_v<_Type> !=
          database_deserialize_strategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_db_deserializable_v = is_db_deserializable<_Type>::value;

template<typename _Type>
struct is_db_round_trip
{
    static constexpr bool value =
        ( is_db_serializable_v<_Type> &&
          is_db_deserializable_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_db_round_trip_v = is_db_round_trip<_Type>::value;


// ===========================================================================
// 7.   combined classification
// ===========================================================================

template<typename _Type>
struct container_database_class
{
    // container-level
    static constexpr bool has_serialize      = has_serialize_method_v<_Type>;
    static constexpr bool has_deserialize    = has_deserialize_method_v<_Type>;
    // element-level
    static constexpr bool elems_row_mappable = has_row_mappable_elements_v<_Type>;
    static constexpr bool elems_db_row       = has_db_row_elements_v<_Type>;
    static constexpr bool elems_db_primitive = has_db_primitive_elements_v<_Type>;
    static constexpr bool elems_db_capable   = is_element_db_capable_v<_Type>;
    // field type
    static constexpr field_type elem_field_type = element_field_type_v<_Type>;
    // schema
    static constexpr bool has_any_schema     = has_schema_v<_Type>;
    static constexpr bool has_create_sql     = has_create_table_sql_v<_Type>;
    // strategies
    static constexpr database_serialize_strategy serialize_strategy     = container_serialize_strategy_v<_Type>;
    static constexpr database_deserialize_strategy deserialize_strategy = container_deserialize_strategy_v<_Type>;
    // aggregate
    static constexpr bool is_serializable    = is_db_serializable_v<_Type>;
    static constexpr bool is_deserializable  = is_db_deserializable_v<_Type>;
    static constexpr bool is_round_trip      = is_db_round_trip_v<_Type>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_DATABASE_TRAITS_