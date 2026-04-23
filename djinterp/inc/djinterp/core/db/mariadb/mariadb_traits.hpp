/******************************************************************************
* djinterp [database]                                       mariadb_traits.hpp
* 
* djinterp MariaDB traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to MariaDB connections, extending the
* MySQL-family common traits from mysql_common_traits.hpp with MariaDB-
* specific detectors, including:
*   - connection reset (reset_connection, available 10.2.4+)
*   - non-blocking / async API (connect_async_start, connect_async_cont,
*     available since MariaDB Connector/C 5.5)
*   - table existence and schema introspection queries
*
*   MariaDB shares the MySQL C API and wire protocol, so the vast majority
* of detectors are inherited from mysql_common_traits.hpp. This file adds
* only the MariaDB-specific extensions that diverge from the common MySQL
* API surface.
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided, following the same conventions as mysql_common_traits.hpp.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17.
*
*   NAMING CONVENTION:
*   Expression detectors:   mariadb_<method>_t
*   Struct-based traits:    has_mariadb_<capability>
*   Variable template _v:   has_mariadb_<capability>_v
*   Tagless traits:          mariadb_can_<action>
*   Compound tagless traits: mariadb_does_<category> / mariadb_is_*
*
*   All mysql_common detectors and traits are available via the included
* mysql_common_traits.hpp - callers do not need to include both.
*
* 
* path:      /inc/djinterp/core/db/mariadb/mariadb_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_MARIADB_TRAITS_
#define DJINTERP_DATABASE_MARIADB_TRAITS_

// djinterp
#include "./mysql_common_traits.hpp"


NS_DJINTERP
NS_DATABASE


// ===========================================================================
// I.   EXPRESSION DETECTORS (MariaDB-specific)
// ===========================================================================
// These detect methods that exist on MariaDB connections but not on
// generic MySQL connections. All MySQL-common detectors (charset,
// multi-result, options, diagnostics, result modes, autocommit,
// escape, select_db, change_user, storage engine) are inherited from
// mysql_common_traits.hpp in the mysql_common namespace.

// -------------------------------------------------------------------------
// A.  connection reset
// -------------------------------------------------------------------------

// mariadb_reset_connection_t
//   detector: reset_connection() method.
// wraps mysql_reset_connection() which was introduced in MariaDB
// 10.2.4. Resets the connection to a clean state (clears session
// variables, temporary tables, prepared statements) without
// re-authenticating.
template<typename _Type>
using mariadb_reset_connection_t =
    decltype(std::declval<_Type&>().reset_connection());

// -------------------------------------------------------------------------
// B.  non-blocking / async API
// -------------------------------------------------------------------------

// mariadb_connect_async_start_t
//   detector: connect_async_start() method.
// wraps mysql_real_connect_start() from the MariaDB non-blocking API.
template<typename _Type>
using mariadb_connect_async_start_t =
    decltype(std::declval<_Type&>().connect_async_start());

// mariadb_connect_async_cont_t
//   detector: connect_async_cont(int) method.
// wraps mysql_real_connect_cont() for continuing the non-blocking
// connection handshake.
template<typename _Type>
using mariadb_connect_async_cont_t =
    decltype(std::declval<_Type&>().connect_async_cont(
        std::declval<int>()));

// -------------------------------------------------------------------------
// C.  schema introspection
// -------------------------------------------------------------------------

// mariadb_table_exists_t
//   detector: table_exists(const std::string&) const method.
template<typename _Type>
using mariadb_table_exists_t =
    decltype(std::declval<const _Type&>().table_exists(
        std::declval<const std::string&>()));

// mariadb_get_table_names_t
//   detector: get_table_names() const method.
template<typename _Type>
using mariadb_get_table_names_t =
    decltype(std::declval<const _Type&>().get_table_names());


// ===========================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// ===========================================================================
// MariaDB-specific traits. All mysql_common tagged traits (has_mysql_charset,
// has_mysql_multi_result, has_mysql_options, has_mysql_diagnostics,
// has_mysql_result_modes, has_mysql_autocommit, has_mysql_escape,
// has_mysql_select_db, is_mysql_connection, etc.) are inherited from
// mysql_common_traits.hpp and available in the mysql_common namespace.

// has_mariadb_reset
//   trait: checks if type _Type supports connection reset.
template<typename _Type>
struct has_mariadb_reset
    : is_detected<mariadb_reset_connection_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mariadb_reset_v =
        has_mariadb_reset<_Type>::value;
#endif

// has_mariadb_async
//   trait: checks if type _Type supports the non-blocking API
// (connect_async_start + connect_async_cont).
template<typename _Type>
struct has_mariadb_async : djinterp::conjunction<
    is_detected<mariadb_connect_async_start_t, _Type>,
    is_detected<mariadb_connect_async_cont_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mariadb_async_v =
        has_mariadb_async<_Type>::value;
#endif

// has_mariadb_schema_query
//   trait: checks if type _Type supports schema introspection
// (table_exists + get_table_names).
template<typename _Type>
struct has_mariadb_schema_query : djinterp::conjunction<
    is_detected<mariadb_table_exists_t, _Type>,
    is_detected<mariadb_get_table_names_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mariadb_schema_query_v =
        has_mariadb_schema_query<_Type>::value;
#endif

// is_mariadb_connection
//   trait: compound trait verifying type _Type implements a MariaDB
// connection interface. This extends is_mysql_connection (from the
// mysql_common namespace) with MariaDB-specific capabilities.
// A type satisfies this if it is a valid MySQL-family connection
// AND has schema query support (which all mariadb_connection
// instances provide).
template<typename _Type>
struct is_mariadb_connection : djinterp::conjunction<
    is_mysql_connection<_Type>,
    has_mariadb_schema_query<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_mariadb_connection_v =
        is_mariadb_connection<_Type>::value;
#endif


// ===========================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// ===========================================================================
// All mysql_common tagless traits (mysql_can_set_charset,
// mysql_can_iterate_results, mysql_can_escape_string,
// mysql_does_charset, mysql_does_multi_result,
// mysql_is_full_connection, etc.) are available via the mysql_common
// namespace and do not need to be redeclared here.

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// mariadb_can_reset_connection
//   tagless trait: true if _Type has reset_connection().
template<typename _Type,
         typename = void>
constexpr bool mariadb_can_reset_connection = false;

template<typename _Type>
constexpr bool mariadb_can_reset_connection<_Type,
    std::void_t<mariadb_reset_connection_t<_Type>>> = true;

// mariadb_can_async_connect
//   tagless trait: true if _Type has connect_async_start().
template<typename _Type,
         typename = void>
constexpr bool mariadb_can_async_connect = false;

template<typename _Type>
constexpr bool mariadb_can_async_connect<_Type,
    std::void_t<mariadb_connect_async_start_t<_Type>>> = true;

// mariadb_can_query_schema
//   tagless trait: true if _Type has table_exists().
template<typename _Type,
         typename = void>
constexpr bool mariadb_can_query_schema = false;

template<typename _Type>
constexpr bool mariadb_can_query_schema<_Type,
    std::void_t<mariadb_table_exists_t<_Type>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// mariadb_does_async
//   tagless trait: true if _Type supports the full non-blocking API.
template<typename _Type,
         typename = void>
constexpr bool mariadb_does_async = false;

template<typename _Type>
constexpr bool mariadb_does_async<_Type, std::void_t<
    mariadb_connect_async_start_t<_Type>,
    mariadb_connect_async_cont_t<_Type>>> = true;

// mariadb_does_schema_query
//   tagless trait: true if _Type supports full schema introspection.
template<typename _Type,
         typename = void>
constexpr bool mariadb_does_schema_query = false;

template<typename _Type>
constexpr bool mariadb_does_schema_query<_Type, std::void_t<
    mariadb_table_exists_t<_Type>,
    mariadb_get_table_names_t<_Type>>> = true;

// mariadb_is_full_connection
//   tagless trait: true if _Type satisfies the complete MariaDB
// connection interface (full MySQL-family connection + schema query).
template<typename _Type>
constexpr bool mariadb_is_full_connection =
    ( mysql_is_full_connection<_Type>  &&
      mariadb_can_query_schema<_Type> );


// ===========================================================================
// IV.  SFINAE HELPERS
// ===========================================================================

// enable_if_mariadb_connection
//   type: SFINAE helper for MariaDB connection constraints.
template<typename _Type>
using enable_if_mariadb_connection =
    typename std::enable_if<is_mariadb_connection<_Type>::value>::type;

// enable_if_has_mariadb_reset
//   type: SFINAE helper for MariaDB reset_connection constraints.
template<typename _Type>
using enable_if_has_mariadb_reset =
    typename std::enable_if<has_mariadb_reset<_Type>::value>::type;

// enable_if_has_mariadb_async
//   type: SFINAE helper for MariaDB async API constraints.
template<typename _Type>
using enable_if_has_mariadb_async =
    typename std::enable_if<has_mariadb_async<_Type>::value>::type;

// enable_if_has_mariadb_schema_query
//   type: SFINAE helper for MariaDB schema query constraints.
template<typename _Type>
using enable_if_has_mariadb_schema_query =
    typename std::enable_if<has_mariadb_schema_query<_Type>::value>::type;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MARIADB_TRAITS_
