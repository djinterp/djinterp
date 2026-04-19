/******************************************************************************
* djinterp [database]                                       mysql_traits.hpp
* 
* djinterp Oracle MySQL traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to Oracle MySQL connections, extending
* the MySQL-family common traits from mysql_common_traits.hpp with Oracle
* MySQL-specific detectors, including:
*   - connection reset (reset_connection, available 5.7.3+)
*   - asynchronous C API (mysql_real_connect_nonblocking, 8.0.16+)
*   - session tracking (get_session_track_info, 5.7.4+)
*   - query attributes (set_query_attribute, 8.0.25+)
*   - X Protocol / X DevAPI presence queries
*   - table existence and schema introspection
*
*   Oracle MySQL shares the MySQL C API and wire protocol with MariaDB,
* so the vast majority of detectors are inherited from
* mysql_common_traits.hpp. This file adds only the Oracle MySQL-specific
* extensions that MariaDB does not have (X Protocol, session tracking,
* query attributes, HeatWave) or that have a different version gate
* (reset_connection at 5.7.3 vs MariaDB's 10.2.4, async API at 8.0.16
* vs MariaDB's Connector/C 5.5).
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided, following the same conventions as mysql_common_traits.hpp.
*
*   All mysql_common detectors and traits are available via the included
* mysql_common_traits.hpp — callers do not need to include both.
*
*   NAMING CONVENTION:
*   Expression detectors:   mysql_ora_<method>_t
*   Struct-based traits:    has_mysql_ora_<capability>
*   Variable template _v:   has_mysql_ora_<capability>_v
*   Tagless traits:          mysql_ora_can_<action>
*   Compound tagless traits: mysql_ora_does_<category>
*
* path:      \inc\database\mysql\mysql_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_TRAITS_
#define DJINTERP_DATABASE_MYSQL_TRAITS_

#include "mysql_common_traits.hpp"


NS_DJINTERP
NS_DB


// =============================================================================
// I.   EXPRESSION DETECTORS (Oracle MySQL-specific)
// =============================================================================

// -------------------------------------------------------------------------
// A.  connection reset
// -------------------------------------------------------------------------

// mysql_ora_reset_connection_t
//   detector: reset_connection() method.
// wraps mysql_reset_connection() introduced in Oracle MySQL 5.7.3.
// note: MariaDB has the same function but at a different version gate
// (10.2.4); the detector expression is identical but the version-gated
// availability differs.
template<typename _T>
using mysql_ora_reset_connection_t =
    decltype(std::declval<_T&>().reset_connection());

// -------------------------------------------------------------------------
// B.  asynchronous C API
// -------------------------------------------------------------------------

// mysql_ora_async_query_start_t
//   detector: async_query_start(const std::string&) method.
// wraps mysql_real_query_nonblocking() introduced in Oracle MySQL 8.0.16.
template<typename _T>
using mysql_ora_async_query_start_t =
    decltype(std::declval<_T&>().async_query_start(
        std::declval<const std::string&>()));

// mysql_ora_async_query_cont_t
//   detector: async_query_cont() method.
// wraps mysql_real_query_nonblocking() continuation.
template<typename _T>
using mysql_ora_async_query_cont_t =
    decltype(std::declval<_T&>().async_query_cont());

// mysql_ora_async_connect_start_t
//   detector: async_connect_start() method.
// wraps mysql_real_connect_nonblocking().
template<typename _T>
using mysql_ora_async_connect_start_t =
    decltype(std::declval<_T&>().async_connect_start());

// -------------------------------------------------------------------------
// C.  session tracking
// -------------------------------------------------------------------------

// mysql_ora_get_session_track_info_t
//   detector: get_session_track_info(int) const method.
// wraps mysql_session_track_get_first/next() introduced in 5.7.4.
template<typename _T>
using mysql_ora_get_session_track_info_t =
    decltype(std::declval<const _T&>().get_session_track_info(
        std::declval<int>()));

// -------------------------------------------------------------------------
// D.  query attributes
// -------------------------------------------------------------------------

// mysql_ora_set_query_attribute_t
//   detector: set_query_attribute(const std::string&,
// const std::string&) method.
// wraps mysql_bind_param() for query attributes introduced in 8.0.25.
template<typename _T>
using mysql_ora_set_query_attribute_t =
    decltype(std::declval<_T&>().set_query_attribute(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// E.  X Protocol
// -------------------------------------------------------------------------

// mysql_ora_has_x_protocol_t
//   detector: supports_x_protocol() const method.
template<typename _T>
using mysql_ora_has_x_protocol_t =
    decltype(std::declval<const _T&>().supports_x_protocol());

// -------------------------------------------------------------------------
// F.  schema introspection
// -------------------------------------------------------------------------

// mysql_ora_table_exists_t
//   detector: table_exists(const std::string&) const method.
template<typename _T>
using mysql_ora_table_exists_t =
    decltype(std::declval<const _T&>().table_exists(
        std::declval<const std::string&>()));

// mysql_ora_get_table_names_t
//   detector: get_table_names() const method.
template<typename _T>
using mysql_ora_get_table_names_t =
    decltype(std::declval<const _T&>().get_table_names());


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_mysql_ora_reset
//   trait: checks if type _T supports connection reset.
template<typename _T>
struct has_mysql_ora_reset
    : is_detected<mysql_ora_reset_connection_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mysql_ora_reset_v =
        has_mysql_ora_reset<_T>::value;
#endif

// has_mysql_ora_async
//   trait: checks if type _T supports the Oracle MySQL async API.
template<typename _T>
struct has_mysql_ora_async : djinterp::conjunction<
    is_detected<mysql_ora_async_query_start_t, _T>,
    is_detected<mysql_ora_async_query_cont_t, _T>,
    is_detected<mysql_ora_async_connect_start_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mysql_ora_async_v =
        has_mysql_ora_async<_T>::value;
#endif

// has_mysql_ora_session_tracking
//   trait: checks if type _T supports session tracking.
template<typename _T>
struct has_mysql_ora_session_tracking
    : is_detected<mysql_ora_get_session_track_info_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mysql_ora_session_tracking_v =
        has_mysql_ora_session_tracking<_T>::value;
#endif

// has_mysql_ora_query_attributes
//   trait: checks if type _T supports query attributes.
template<typename _T>
struct has_mysql_ora_query_attributes
    : is_detected<mysql_ora_set_query_attribute_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mysql_ora_query_attributes_v =
        has_mysql_ora_query_attributes<_T>::value;
#endif

// has_mysql_ora_schema_query
//   trait: checks if type _T supports schema introspection.
template<typename _T>
struct has_mysql_ora_schema_query : djinterp::conjunction<
    is_detected<mysql_ora_table_exists_t, _T>,
    is_detected<mysql_ora_get_table_names_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mysql_ora_schema_query_v =
        has_mysql_ora_schema_query<_T>::value;
#endif

// is_mysql_ora_connection
//   trait: compound trait verifying type _T implements an Oracle MySQL
// connection interface. Extends is_mysql_connection with schema query
// support.
template<typename _T>
struct is_mysql_ora_connection : djinterp::conjunction<
    mysql_common::is_mysql_connection<_T>,
    has_mysql_ora_schema_query<_T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool is_mysql_ora_connection_v =
        is_mysql_ora_connection<_T>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// mysql_ora_can_reset_connection
//   tagless trait: true if _T has reset_connection().
template<typename _T,
         typename = void>
constexpr bool mysql_ora_can_reset_connection = false;

template<typename _T>
constexpr bool mysql_ora_can_reset_connection<_T,
    std::void_t<mysql_ora_reset_connection_t<_T>>> = true;

// mysql_ora_can_async_query
//   tagless trait: true if _T has async_query_start().
template<typename _T,
         typename = void>
constexpr bool mysql_ora_can_async_query = false;

template<typename _T>
constexpr bool mysql_ora_can_async_query<_T,
    std::void_t<mysql_ora_async_query_start_t<_T>>> = true;

// mysql_ora_can_track_session
//   tagless trait: true if _T has get_session_track_info().
template<typename _T,
         typename = void>
constexpr bool mysql_ora_can_track_session = false;

template<typename _T>
constexpr bool mysql_ora_can_track_session<_T,
    std::void_t<mysql_ora_get_session_track_info_t<_T>>> = true;

// mysql_ora_can_set_query_attribute
//   tagless trait: true if _T has set_query_attribute().
template<typename _T,
         typename = void>
constexpr bool mysql_ora_can_set_query_attribute = false;

template<typename _T>
constexpr bool mysql_ora_can_set_query_attribute<_T,
    std::void_t<mysql_ora_set_query_attribute_t<_T>>> = true;

// mysql_ora_can_query_schema
//   tagless trait: true if _T has table_exists().
template<typename _T,
         typename = void>
constexpr bool mysql_ora_can_query_schema = false;

template<typename _T>
constexpr bool mysql_ora_can_query_schema<_T,
    std::void_t<mysql_ora_table_exists_t<_T>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// mysql_ora_does_async
//   tagless trait: true if _T supports the full Oracle MySQL async API.
template<typename _T,
         typename = void>
constexpr bool mysql_ora_does_async = false;

template<typename _T>
constexpr bool mysql_ora_does_async<_T, std::void_t<
    mysql_ora_async_query_start_t<_T>,
    mysql_ora_async_query_cont_t<_T>,
    mysql_ora_async_connect_start_t<_T>>> = true;

// mysql_ora_does_schema_query
//   tagless trait: true if _T supports full schema introspection.
template<typename _T,
         typename = void>
constexpr bool mysql_ora_does_schema_query = false;

template<typename _T>
constexpr bool mysql_ora_does_schema_query<_T, std::void_t<
    mysql_ora_table_exists_t<_T>,
    mysql_ora_get_table_names_t<_T>>> = true;

// mysql_ora_is_full_connection
//   tagless trait: true if _T satisfies the complete Oracle MySQL
// connection interface.
template<typename _T>
constexpr bool mysql_ora_is_full_connection =
    ( mysql_common::mysql_is_full_connection<_T> &&
      mysql_ora_can_query_schema<_T> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

// enable_if_mysql_ora_connection
//   type: SFINAE helper for Oracle MySQL connection constraints.
template<typename _T>
using enable_if_mysql_ora_connection =
    typename std::enable_if<is_mysql_ora_connection<_T>::value>::type;

// enable_if_has_mysql_ora_reset
//   type: SFINAE helper for Oracle MySQL reset_connection constraints.
template<typename _T>
using enable_if_has_mysql_ora_reset =
    typename std::enable_if<has_mysql_ora_reset<_T>::value>::type;

// enable_if_has_mysql_ora_async
//   type: SFINAE helper for Oracle MySQL async API constraints.
template<typename _T>
using enable_if_has_mysql_ora_async =
    typename std::enable_if<has_mysql_ora_async<_T>::value>::type;

// enable_if_has_mysql_ora_session_tracking
//   type: SFINAE helper for Oracle MySQL session tracking constraints.
template<typename _T>
using enable_if_has_mysql_ora_session_tracking =
    typename std::enable_if<
        has_mysql_ora_session_tracking<_T>::value>::type;

// enable_if_has_mysql_ora_query_attributes
//   type: SFINAE helper for Oracle MySQL query attribute constraints.
template<typename _T>
using enable_if_has_mysql_ora_query_attributes =
    typename std::enable_if<
        has_mysql_ora_query_attributes<_T>::value>::type;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_TRAITS_
