/******************************************************************************
* djinterp [database]                                     mariadb_concepts.hpp
*
*  djinterp MariaDB classification concepts
*   C++20 concepts layered on top of mariadb_traits.hpp.  These concepts
* provide readable `requires` constraints for MariaDB database connections,
* including connection reset, async connection handshake, schema
* introspection, and the full MariaDB connection classification built on top
* of the shared MySQL-family trait layer.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the MariaDB trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core MariaDB Connection Concepts
* 3.   MariaDB Capability Concepts
* 4.   Tagless MariaDB Capability Concepts
*
* path:      /inc/djinterp/core/db/mariadb/mariadb_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                   date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_DATABASE_MARIADB_CONCEPTS_
#define DJINTERP_DATABASE_MARIADB_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./mariadb_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "mariadb_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_DATABASE


// =============================================================================
// I.   Core MariaDB Connection Concepts
// =============================================================================

// mariadb_connection
//   concept: constrains types implementing the MariaDB connection
// interface.
template<typename _Type>
concept mariadb_connection =
    is_mariadb_connection<clean_t<_Type>>::value;

// non_mariadb_connection
//   concept: constrains types that do not implement the MariaDB
// connection interface.
template<typename _Type>
concept non_mariadb_connection =
    !mariadb_connection<_Type>;

// mariadb_schema_connection
//   concept: constrains MariaDB connections supporting schema
// introspection.
template<typename _Type>
concept mariadb_schema_connection =
    has_mariadb_schema_query<clean_t<_Type>>::value;

// mariadb_resettable_connection
//   concept: constrains MariaDB connections supporting connection reset.
template<typename _Type>
concept mariadb_resettable_connection =
    has_mariadb_reset<clean_t<_Type>>::value;

// mariadb_async_connection
//   concept: constrains MariaDB connections supporting the non-blocking
// connection API.
template<typename _Type>
concept mariadb_async_connection =
    has_mariadb_async<clean_t<_Type>>::value;


// =============================================================================
// II.  MariaDB Capability Concepts
// =============================================================================

// mariadb_reset_connection_capable
//   concept: constrains types exposing reset_connection().
template<typename _Type>
concept mariadb_reset_connection_capable =
    mariadb_can_reset_connection<clean_t<_Type>>;

// mariadb_async_connect_startable
//   concept: constrains types exposing connect_async_start().
template<typename _Type>
concept mariadb_async_connect_startable =
    mariadb_can_async_connect<clean_t<_Type>>;

// mariadb_async_connect_continuable
//   concept: constrains types exposing connect_async_cont(status).
template<typename _Type>
concept mariadb_async_connect_continuable =
    is_detected<mariadb_connect_async_cont_t, clean_t<_Type>>::value;

// mariadb_table_exists_query
//   concept: constrains types exposing table_exists(name).
template<typename _Type>
concept mariadb_table_exists_query =
    mariadb_can_query_schema<clean_t<_Type>>;

// mariadb_table_names_query
//   concept: constrains types exposing get_table_names().
template<typename _Type>
concept mariadb_table_names_query =
    is_detected<mariadb_get_table_names_t, clean_t<_Type>>::value;


// =============================================================================
// III. Tagless MariaDB Capability Concepts
// =============================================================================

// mariadb_async_handshakeable
//   concept: constrains types satisfying the full tagless async
// handshake capability set.
template<typename _Type>
concept mariadb_async_handshakeable =
    mariadb_does_async<clean_t<_Type>>;

// mariadb_schema_queryable
//   concept: constrains types satisfying the full tagless schema-query
// capability set.
template<typename _Type>
concept mariadb_schema_queryable =
    mariadb_does_schema_query<clean_t<_Type>>;

// mariadb_full_connection
//   concept: constrains types satisfying the complete tagless MariaDB
// connection capability set.
template<typename _Type>
concept mariadb_full_connection =
    mariadb_is_full_connection<clean_t<_Type>>;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MARIADB_CONCEPTS_
