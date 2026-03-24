/******************************************************************************
* djinterp [container]                                 container_db_traits.hpp
*
* Container-aware database persistence traits for the djinterp framework.
*   Bridges the container classification system (container_traits.hpp)
* with the database module (database_common.hpp, database_traits.hpp) to
* provide compile-time detection of which database operations a container
* and its elements support.
*
*   Detection is organized into four layers:
*     1. Element mapping:  can the container's value_type be stored
*        in / retrieved from a database row?
*     2. Schema protocol:  does the element or container expose
*        table name, column definitions, or primary key metadata?
*     3. Container persistence:  does the container itself expose
*        to_database() / from_database() methods?
*     4. Bulk capability:  can the container be efficiently loaded
*        or flushed as a batch?
*
*   The traits are vendor-agnostic.  Vendor-specific modules
* (e.g. sqlite_container.hpp, postgresql_container.hpp) can
* specialize the strategy traits or add vendor-specific
* detectors without modifying this header.
*
*   All detection is purely structural SFINAE.
*
* ELEMENT PROTOCOLS (optional members detected):
*   Row mapping:      to_row() / from_row(const row&)
*   Value mapping:    to_value() / from_value(const value&)
*   Schema:           table_name() / column_definitions()
*                     primary_key() / schema_name()
*   Field access:     field_count() / field_name(size_t)
*                     field_value(size_t) / field_type(size_t)
*
* CONTAINER PROTOCOLS (optional members detected):
*   Persistence:      to_database(Connection&)
*                     from_database(Connection&, string)
*   Schema:           table_name() / schema_name()
*                     create_table_sql() / drop_table_sql()
*   Batch:            insert_batch(Connection&)
*                     select_all(Connection&)
*
* DEPENDENCIES:
*   container_traits.hpp    - container classification
*   database_traits.hpp     - database interface detection
*   database_common.hpp     - value, row, field_type types
*
* TABLE OF CONTENTS
* =================
* I.      Element-Level Database Detection
* II.     Element Field Type Mapping
* III.    Schema Protocol Detection
* IV.    Container-Level Database Detection
* V.      Populatability Detection
* VI.     Database Strategy Classification
* VII.    Convenience Predicates
* VIII.   Combined Classification
*
*
* path:      /inc/container/meta/container_db_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_DATABASE_TRAITS_
#define DJINTERP_CONTAINER_DATABASE_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "container_traits.hpp"
#include "../../database/database_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Element-Level Database Detection
// =============================================================================
// Probes the container's value_type for database persistence
// capabilities.

NS_INTERNAL

    // safe_value_type (self-contained for independent
    // inclusion)
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

    // --- element .to_row() detection ---

    template<typename _Elem, typename = void>
    struct elem_has_to_row : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_to_row<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().to_row())>>
        : std::true_type
    {};

    // --- element .from_row(const row&) detection ---

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

    // --- element .to_value() detection ---

    template<typename _Elem, typename = void>
    struct elem_has_to_value : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_to_value<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().to_value())>>
        : std::true_type
    {};

    // --- element constructible from value ---

    template<typename _Elem, typename = void>
    struct elem_has_from_value : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_from_value<_Elem,
        std::void_t<decltype(
            _Elem::from_value(
                std::declval<
                    const database::value&>()))>>
        : std::true_type
    {};

    // --- element is a database::value variant ---

    template<typename _Elem>
    struct elem_is_db_value
        : std::is_same<_Elem, database::value>
    {};

    // --- element is a database::row ---

    template<typename _Elem>
    struct elem_is_db_row
        : std::is_same<_Elem, database::row>
    {};

    // --- element is a primitive mappable to a
    //     database field (arithmetic or string) ---

    template<typename _Elem>
    struct elem_is_db_primitive
        : std::integral_constant<bool,
              ( std::is_arithmetic_v<_Elem>      ||
                std::is_same_v<_Elem,
                    std::string> )>
    {};

    // --- element field introspection protocol ---

    template<typename _Elem, typename = void>
    struct elem_has_field_count : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_field_count<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>()
                .field_count())>>
        : std::true_type
    {};

    template<typename _Elem, typename = void>
    struct elem_has_field_name : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_field_name<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>()
                .field_name(
                    std::declval<std::size_t>()))>>
        : std::true_type
    {};

    template<typename _Elem, typename = void>
    struct elem_has_field_value : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_field_value<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>()
                .field_value(
                    std::declval<std::size_t>()))>>
        : std::true_type
    {};

NS_END  // internal

// has_row_mappable_elements
//   type trait: true if the container's value_type can be
// converted to/from a database::row via .to_row() and
// static from_row().
template<typename _Type>
struct has_row_mappable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( internal::elem_has_to_row<
              elem_type>::value &&
          internal::elem_has_from_row<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_row_mappable_elements_v =
    has_row_mappable_elements<_Type>::value;

// has_value_mappable_elements
//   type trait: true if the container's value_type can be
// converted to/from a database::value.
template<typename _Type>
struct has_value_mappable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( internal::elem_has_to_value<
              elem_type>::value ||
          internal::elem_has_from_value<
              elem_type>::value ||
          internal::elem_is_db_value<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_value_mappable_elements_v =
    has_value_mappable_elements<_Type>::value;

// has_db_primitive_elements
//   type trait: true if the container's value_type is a
// primitive type directly storable in a database column
// (arithmetic or std::string).
template<typename _Type>
struct has_db_primitive_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        internal::elem_is_db_primitive<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_db_primitive_elements_v =
    has_db_primitive_elements<_Type>::value;

// has_db_row_elements
//   type trait: true if the container stores database::row
// objects directly.
template<typename _Type>
struct has_db_row_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        internal::elem_is_db_row<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_db_row_elements_v =
    has_db_row_elements<_Type>::value;

// has_field_introspectable_elements
//   type trait: true if the container's value_type exposes
// a field introspection protocol (field_count, field_name,
// field_value) enabling generic column mapping.
template<typename _Type>
struct has_field_introspectable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( internal::elem_has_field_count<
              elem_type>::value &&
          internal::elem_has_field_name<
              elem_type>::value &&
          internal::elem_has_field_value<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool
    has_field_introspectable_elements_v =
        has_field_introspectable_elements<
            _Type>::value;

// is_element_db_capable
//   type trait: true if the container's value_type can
// participate in database operations via any mechanism.
template<typename _Type>
struct is_element_db_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_row_mappable_elements_v<clean_type>        ||
          has_value_mappable_elements_v<clean_type>      ||
          has_db_primitive_elements_v<clean_type>        ||
          has_db_row_elements_v<clean_type>              ||
          has_field_introspectable_elements_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_element_db_capable_v =
    is_element_db_capable<_Type>::value;


// =============================================================================
// II.  Element Field Type Mapping
// =============================================================================
// Maps C++ element types to database::field_type at compile
// time for schema generation and parameter binding.

NS_INTERNAL

    // cpp_to_field_type
    //   trait: maps a C++ type to its database::field_type.
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
    struct cpp_to_field_type<double>
    {
        static constexpr database::field_type value =
            database::field_type::floating_point;
    };

    template<>
    struct cpp_to_field_type<float>
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
    struct cpp_to_field_type<std::vector<std::uint8_t>>
    {
        static constexpr database::field_type value =
            database::field_type::binary;
    };

NS_END  // internal

// element_field_type
//   type trait: resolves the database::field_type for the
// container's value_type.  Returns field_type::custom for
// unrecognized types.
template<typename _Type>
struct element_field_type
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr database::field_type value =
        internal::cpp_to_field_type<elem_type>::value;
};

template<typename _Type>
inline constexpr database::field_type
    element_field_type_v =
        element_field_type<_Type>::value;

// has_known_field_type
//   type trait: true if the container's value_type maps to
// a known (non-custom) database field type.
template<typename _Type>
struct has_known_field_type
{
    static constexpr bool value =
        ( element_field_type_v<_Type> !=
          database::field_type::custom );
};

template<typename _Type>
inline constexpr bool has_known_field_type_v =
    has_known_field_type<_Type>::value;


// =============================================================================
// III. Schema Protocol Detection
// =============================================================================
// Detects whether the container or its elements expose
// database schema metadata (table name, column definitions,
// primary key, CREATE TABLE SQL, etc.).

// --- element-level schema ---

D_TYPE_TRAIT_TRUE(has_elem_table_name,
    decltype(std::declval<const _Type&>()
        .table_name()))

D_TYPE_TRAIT_TRUE(has_elem_column_definitions,
    decltype(std::declval<const _Type&>()
        .column_definitions()))

D_TYPE_TRAIT_TRUE(has_elem_primary_key,
    decltype(std::declval<const _Type&>()
        .primary_key()))

D_TYPE_TRAIT_TRUE(has_elem_schema_name,
    decltype(std::declval<const _Type&>()
        .schema_name()))

// has_element_schema
//   type trait: true if the container's value_type exposes
// at least table_name and column_definitions.
template<typename _Type>
struct has_element_schema
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::db_safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( has_elem_table_name_v<elem_type> &&
          has_elem_column_definitions_v<elem_type> );
};

template<typename _Type>
inline constexpr bool has_element_schema_v =
    has_element_schema<_Type>::value;

// --- container-level schema ---

D_TYPE_TRAIT_TRUE(has_table_name,
    decltype(std::declval<const _Type&>()
        .table_name()))

D_TYPE_TRAIT_TRUE(has_schema_name,
    decltype(std::declval<const _Type&>()
        .schema_name()))

D_TYPE_TRAIT_TRUE(has_create_table_sql,
    decltype(std::declval<const _Type&>()
        .create_table_sql()))

D_TYPE_TRAIT_TRUE(has_drop_table_sql,
    decltype(std::declval<const _Type&>()
        .drop_table_sql()))

// has_container_schema
//   type trait: true if the container itself exposes schema
// metadata (table name at minimum).
template<typename _Type>
struct has_container_schema
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        has_table_name_v<clean_type>;
};

template<typename _Type>
inline constexpr bool has_container_schema_v =
    has_container_schema<_Type>::value;

// has_schema
//   type trait: true if schema metadata is available from
// either the container or its elements.
template<typename _Type>
struct has_schema
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_container_schema_v<clean_type> ||
          has_element_schema_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_schema_v =
    has_schema<_Type>::value;


// =============================================================================
// IV.  Container-Level Database Detection
// =============================================================================
// Detects whether the container itself exposes persistence
// methods.  These are generic (connection-type-templated) to
// remain vendor-agnostic.

// has_to_database_method
//   type trait: true if container has a .to_database(conn)
// method template (detected via a mock connection argument).

NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_to_db_check : std::false_type
    {};

    // detect .to_database() accepting any argument
    // (the connection type is templated by the container)
    template<typename _Type>
    struct has_to_db_check<_Type,
        std::void_t<decltype(
            &_Type::to_database)>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_from_db_check : std::false_type
    {};

    template<typename _Type>
    struct has_from_db_check<_Type,
        std::void_t<decltype(
            &_Type::from_database)>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_insert_batch_check : std::false_type
    {};

    template<typename _Type>
    struct has_insert_batch_check<_Type,
        std::void_t<decltype(
            &_Type::insert_batch)>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_select_all_check : std::false_type
    {};

    template<typename _Type>
    struct has_select_all_check<_Type,
        std::void_t<decltype(
            &_Type::select_all)>>
        : std::true_type
    {};

NS_END  // internal

// has_to_database_method
//   type trait: true if container has a to_database method.
template<typename _Type>
struct has_to_database_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_to_db_check<clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_to_database_method_v =
    has_to_database_method<_Type>::value;

// has_from_database_method
//   type trait: true if container has a from_database method
// or static factory.
template<typename _Type>
struct has_from_database_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_from_db_check<clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_from_database_method_v =
    has_from_database_method<_Type>::value;

// has_insert_batch_method
//   type trait: true if container has a bulk insert_batch
// method.
template<typename _Type>
struct has_insert_batch_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_insert_batch_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_insert_batch_method_v =
    has_insert_batch_method<_Type>::value;

// has_select_all_method
//   type trait: true if container has a bulk select_all
// method.
template<typename _Type>
struct has_select_all_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_select_all_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_select_all_method_v =
    has_select_all_method<_Type>::value;


// =============================================================================
// V.   Populatability Detection
// =============================================================================
// Determines whether a container can be populated from a
// database result set.  Requires:
//   1. Output-capable (push_back or insert).
//   2. Elements constructible from row or value.

// is_db_populatable
//   type trait: true if container can be filled from
// database query results.
template<typename _Type>
struct is_db_populatable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( ( has_push_back_v<clean_type>          ||
            has_insert_v<clean_type> )           &&
          ( has_row_mappable_elements_v<
                clean_type>                      ||
            has_db_row_elements_v<clean_type>    ||
            has_db_primitive_elements_v<
                clean_type>                      ||
            has_value_mappable_elements_v<
                clean_type> ) );
};

template<typename _Type>
inline constexpr bool is_db_populatable_v =
    is_db_populatable<_Type>::value;

// is_db_persistable
//   type trait: true if container can persist its contents
// to a database (iterable + elements are db-capable).
template<typename _Type>
struct is_db_persistable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type> &&
          is_element_db_capable_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_db_persistable_v =
    is_db_persistable<_Type>::value;


// =============================================================================
// VI.  Database Strategy Classification
// =============================================================================

// --- insert strategy ---

// DDbInsertStrategy
//   enum: compile-time insert strategy tags.
enum class DDbInsertStrategy
{
    // container has native to_database() — delegate
    native,

    // container has insert_batch() — bulk insert
    batch,

    // elements have to_row() — iterate, convert, insert
    // each row
    element_row,

    // elements have field introspection — iterate,
    // introspect, bind fields
    element_fields,

    // elements are db primitives — iterate, bind single
    // column values
    element_primitive,

    // no insert path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct db_insert_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DDbInsertStrategy value =
            has_to_database_method_v<clean_type>
                ? DDbInsertStrategy::native

            : has_insert_batch_method_v<clean_type>
                ? DDbInsertStrategy::batch

            : ( is_iterable_container_v<clean_type> &&
                has_row_mappable_elements_v<
                    clean_type> )
                ? DDbInsertStrategy::element_row

            : ( is_iterable_container_v<clean_type> &&
                has_field_introspectable_elements_v<
                    clean_type> )
                ? DDbInsertStrategy::element_fields

            : ( is_iterable_container_v<clean_type> &&
                has_db_primitive_elements_v<
                    clean_type> )
                ? DDbInsertStrategy::element_primitive

            : DDbInsertStrategy::unsupported;
    };

NS_END  // internal

// container_db_insert_strategy
//   type trait: determines the best insert strategy.
template<typename _Type>
struct container_db_insert_strategy
{
    static constexpr DDbInsertStrategy value =
        internal::db_insert_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DDbInsertStrategy
    container_db_insert_strategy_v =
        container_db_insert_strategy<_Type>::value;

// --- select strategy ---

// DDbSelectStrategy
//   enum: compile-time select (populate) strategy tags.
enum class DDbSelectStrategy
{
    // container has native from_database() — delegate
    native,

    // container has select_all() — bulk select
    bulk,

    // container stores db::row directly — push rows
    direct_row,

    // elements have from_row() — construct from each
    // result row
    element_from_row,

    // elements are db primitives — extract single column
    element_primitive,

    // no select path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct db_select_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DDbSelectStrategy value =
            has_from_database_method_v<clean_type>
                ? DDbSelectStrategy::native

            : has_select_all_method_v<clean_type>
                ? DDbSelectStrategy::bulk

            : ( is_db_populatable_v<clean_type> &&
                has_db_row_elements_v<clean_type> )
                ? DDbSelectStrategy::direct_row

            : ( is_db_populatable_v<clean_type> &&
                has_row_mappable_elements_v<
                    clean_type> )
                ? DDbSelectStrategy::element_from_row

            : ( is_db_populatable_v<clean_type> &&
                has_db_primitive_elements_v<
                    clean_type> )
                ? DDbSelectStrategy::element_primitive

            : DDbSelectStrategy::unsupported;
    };

NS_END  // internal

// container_db_select_strategy
//   type trait: determines the best select (populate)
// strategy.
template<typename _Type>
struct container_db_select_strategy
{
    static constexpr DDbSelectStrategy value =
        internal::db_select_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DDbSelectStrategy
    container_db_select_strategy_v =
        container_db_select_strategy<_Type>::value;


// =============================================================================
// VII. Convenience Predicates
// =============================================================================

// is_db_insertable_container
//   type trait: true if any insert strategy is available.
template<typename _Type>
struct is_db_insertable_container
{
    static constexpr bool value =
        ( container_db_insert_strategy_v<_Type> !=
          DDbInsertStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_db_insertable_container_v =
    is_db_insertable_container<_Type>::value;

// is_db_selectable_container
//   type trait: true if any select strategy is available.
template<typename _Type>
struct is_db_selectable_container
{
    static constexpr bool value =
        ( container_db_select_strategy_v<_Type> !=
          DDbSelectStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_db_selectable_container_v =
    is_db_selectable_container<_Type>::value;

// is_db_round_trip_capable
//   type trait: true if container supports both insert and
// select.
template<typename _Type>
struct is_db_round_trip_capable
{
    static constexpr bool value =
        ( is_db_insertable_container_v<_Type> &&
          is_db_selectable_container_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_db_round_trip_capable_v =
    is_db_round_trip_capable<_Type>::value;

// has_any_db_support
//   type trait: true if the container has any database
// capability.
template<typename _Type>
struct has_any_db_support
{
    static constexpr bool value =
        ( is_db_insertable_container_v<_Type> ||
          is_db_selectable_container_v<_Type> ||
          has_schema_v<_Type> );
};

template<typename _Type>
inline constexpr bool has_any_db_support_v =
    has_any_db_support<_Type>::value;


// =============================================================================
// VIII. Combined Classification
// =============================================================================

// container_database_class
//   struct: complete database persistence classification.
// All members are static constexpr.
template<typename _Type>
struct container_database_class
{
    // element-level capabilities
    static constexpr bool elements_row_mappable =
        has_row_mappable_elements_v<_Type>;
    static constexpr bool elements_value_mappable =
        has_value_mappable_elements_v<_Type>;
    static constexpr bool elements_db_primitive =
        has_db_primitive_elements_v<_Type>;
    static constexpr bool elements_db_row =
        has_db_row_elements_v<_Type>;
    static constexpr bool elements_field_introspectable =
        has_field_introspectable_elements_v<_Type>;
    static constexpr bool elements_db_capable =
        is_element_db_capable_v<_Type>;

    // field type mapping
    static constexpr database::field_type
        element_field =
            element_field_type_v<_Type>;
    static constexpr bool has_known_field =
        has_known_field_type_v<_Type>;

    // schema
    static constexpr bool has_element_schema =
        has_element_schema_v<_Type>;
    static constexpr bool has_container_schema =
        has_container_schema_v<_Type>;
    static constexpr bool has_any_schema =
        has_schema_v<_Type>;
    static constexpr bool has_create_sql =
        has_create_table_sql_v<_Type>;

    // container-level persistence
    static constexpr bool has_to_database =
        has_to_database_method_v<_Type>;
    static constexpr bool has_from_database =
        has_from_database_method_v<_Type>;
    static constexpr bool has_batch_insert =
        has_insert_batch_method_v<_Type>;
    static constexpr bool has_bulk_select =
        has_select_all_method_v<_Type>;

    // populatability / persistability
    static constexpr bool is_populatable =
        is_db_populatable_v<_Type>;
    static constexpr bool is_persistable =
        is_db_persistable_v<_Type>;

    // strategies
    static constexpr DDbInsertStrategy
        insert_strategy =
            container_db_insert_strategy_v<_Type>;
    static constexpr DDbSelectStrategy
        select_strategy =
            container_db_select_strategy_v<_Type>;

    // aggregate
    static constexpr bool is_insertable =
        is_db_insertable_container_v<_Type>;
    static constexpr bool is_selectable =
        is_db_selectable_container_v<_Type>;
    static constexpr bool is_round_trip =
        is_db_round_trip_capable_v<_Type>;
    static constexpr bool has_db_support =
        has_any_db_support_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_DATABASE_TRAITS_
