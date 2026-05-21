/******************************************************************************
* djinterp [database]                                       mysql_concepts.hpp
*
*  djinterp Oracle MySQL classification concepts
*   C++20 concepts layered on top of mysql_traits.hpp.  These concepts
* provide readable `requires` constraints for Oracle MySQL database
* connections, including connection reset, async query / connect support,
* session tracking, query attributes, X Protocol presence queries, schema
* introspection, and the full Oracle MySQL connection classification built on
* top of the shared MySQL-family trait layer.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the Oracle MySQL trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Oracle MySQL Connection Concepts
* 3.   Oracle MySQL Capability Concepts
* 4.   Tagless Oracle MySQL Capability Concepts
*
* 
* path:      /inc/djinterp/core/db/mysql/mysql_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_CONCEPTS_
#define DJINTERP_DATABASE_MYSQL_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./mysql_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Core Oracle MySQL Connection Concepts
// ===========================================================================

// mysql_connection
//   concept: constrains types implementing the Oracle MySQL connection
// interface.
template<typename _Type>
concept mysql_connection =
    is_mysql_ora_connection<clean_t<_Type>>::value;

// non_mysql_connection
//   concept: constrains types that do not implement the Oracle MySQL
// connection interface.
template<typename _Type>
concept non_mysql_connection =
    !mysql_connection<_Type>;

// mysql_schema_connection
//   concept: constrains Oracle MySQL connections supporting schema
// introspection.
template<typename _Type>
concept mysql_schema_connection =
    has_mysql_ora_schema_query<clean_t<_Type>>::value;

// mysql_resettable_connection
//   concept: constrains Oracle MySQL connections supporting connection
// reset.
template<typename _Type>
concept mysql_resettable_connection =
    has_mysql_ora_reset<clean_t<_Type>>::value;

// mysql_async_connection
//   concept: constrains Oracle MySQL connections supporting the Oracle
// MySQL asynchronous API.
template<typename _Type>
concept mysql_async_connection =
    has_mysql_ora_async<clean_t<_Type>>::value;

// mysql_session_tracking_connection
//   concept: constrains Oracle MySQL connections supporting session
// tracking.
template<typename _Type>
concept mysql_session_tracking_connection =
    has_mysql_ora_session_tracking<clean_t<_Type>>::value;

// mysql_query_attributes_connection
//   concept: constrains Oracle MySQL connections supporting query
// attributes.
template<typename _Type>
concept mysql_query_attributes_connection =
    has_mysql_ora_query_attributes<clean_t<_Type>>::value;


// ===========================================================================
// II.  Oracle MySQL Capability Concepts
// ===========================================================================

// mysql_reset_connection_capable
//   concept: constrains types exposing reset_connection().
template<typename _Type>
concept mysql_reset_connection_capable =
    mysql_ora_can_reset_connection<clean_t<_Type>>;

// mysql_async_query_startable
//   concept: constrains types exposing async_query_start(sql).
template<typename _Type>
concept mysql_async_query_startable =
    mysql_ora_can_async_query<clean_t<_Type>>;

// mysql_async_query_continuable
//   concept: constrains types exposing async_query_cont().
template<typename _Type>
concept mysql_async_query_continuable =
    is_detected<mysql_ora_async_query_cont_t, clean_t<_Type>>::value;

// mysql_async_connect_startable
//   concept: constrains types exposing async_connect_start().
template<typename _Type>
concept mysql_async_connect_startable =
    is_detected<mysql_ora_async_connect_start_t, clean_t<_Type>>::value;

// mysql_session_trackable
//   concept: constrains types exposing get_session_track_info(kind).
template<typename _Type>
concept mysql_session_trackable =
    mysql_ora_can_track_session<clean_t<_Type>>;

// mysql_query_attribute_settable
//   concept: constrains types exposing set_query_attribute(name, value).
template<typename _Type>
concept mysql_query_attribute_settable =
    mysql_ora_can_set_query_attribute<clean_t<_Type>>;

// mysql_table_exists_query
//   concept: constrains types exposing table_exists(name).
template<typename _Type>
concept mysql_table_exists_query =
    mysql_ora_can_query_schema<clean_t<_Type>>;

// mysql_table_names_query
//   concept: constrains types exposing get_table_names().
template<typename _Type>
concept mysql_table_names_query =
    is_detected<mysql_ora_get_table_names_t, clean_t<_Type>>::value;

// mysql_x_protocol_query
//   concept: constrains types exposing supports_x_protocol().
template<typename _Type>
concept mysql_x_protocol_query =
    is_detected<mysql_ora_has_x_protocol_t, clean_t<_Type>>::value;


// ===========================================================================
// III. Tagless Oracle MySQL Capability Concepts
// ===========================================================================

// mysql_async_queryable
//   concept: constrains types satisfying the full tagless Oracle MySQL
// async capability set.
template<typename _Type>
concept mysql_async_queryable =
    mysql_ora_does_async<clean_t<_Type>>;

// mysql_schema_queryable
//   concept: constrains types satisfying the full tagless schema-query
// capability set.
template<typename _Type>
concept mysql_schema_queryable =
    mysql_ora_does_schema_query<clean_t<_Type>>;

// mysql_full_connection
//   concept: constrains types satisfying the complete tagless Oracle
// MySQL connection capability set.
template<typename _Type>
concept mysql_full_connection =
    mysql_ora_is_full_connection<clean_t<_Type>>;


// ===========================================================================
// IV.  Compact Oracle MySQL Aliases
// ===========================================================================

// mysql_ora_connection
//   concept: alias for mysql_connection.
template<typename _Type>
concept mysql_ora_connection =
    mysql_connection<_Type>;

// mysql_ora_full_connection
//   concept: alias for mysql_full_connection.
template<typename _Type>
concept mysql_ora_full_connection =
    mysql_full_connection<_Type>;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_CONCEPTS_
