/******************************************************************************
* djinterp [database]                                    postgres_concepts.hpp
*
*  djinterp PostgreSQL classification concepts
*   C++20 concepts layered on top of postgres_traits.hpp.  These concepts
* provide readable `requires` constraints for PostgreSQL database
* connections, including asynchronous query dispatch, pipeline mode, COPY,
* LISTEN / NOTIFY, parameterized execution, diagnostics, schema
* introspection, escaping, and large object support.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the PostgreSQL trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core PostgreSQL Connection Concepts
* 3.   PostgreSQL Capability Concepts
* 4.   Tagless PostgreSQL Capability Concepts
*
* path:      /inc/djinterp/core/db/postgres/postgres_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_DATABASE_POSTGRES_CONCEPTS_
#define DJINTERP_DATABASE_POSTGRES_CONCEPTS_ 1

#include <type_traits>
#include "postgres_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "postgres_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_DATABASE
// =============================================================================
// I.   Core PostgreSQL Connection Concepts
// =============================================================================

// postgres_connection
//   concept: constrains types implementing the PostgreSQL connection
// interface.
template<typename _Type>
concept postgres_connection =
    is_pg_connection<clean_t<_Type>>::value;

// pg_connection
//   concept: alias for postgres_connection using the shorter pg prefix.
template<typename _Type>
concept pg_connection =
    postgres_connection<_Type>;

// non_postgres_connection
//   concept: constrains types that do not implement the PostgreSQL
// connection interface.
template<typename _Type>
concept non_postgres_connection =
    !postgres_connection<_Type>;

// postgres_async_connection
//   concept: constrains PostgreSQL connections supporting asynchronous
// query dispatch.
template<typename _Type>
concept postgres_async_connection =
    has_pg_async<clean_t<_Type>>::value;

// postgres_pipeline_connection
//   concept: constrains PostgreSQL connections supporting pipeline mode.
template<typename _Type>
concept postgres_pipeline_connection =
    has_pg_pipeline<clean_t<_Type>>::value;

// postgres_copy_connection
//   concept: constrains PostgreSQL connections supporting the COPY
// protocol.
template<typename _Type>
concept postgres_copy_connection =
    has_pg_copy<clean_t<_Type>>::value;

// postgres_listen_notify_connection
//   concept: constrains PostgreSQL connections supporting LISTEN /
// NOTIFY.
template<typename _Type>
concept postgres_listen_notify_connection =
    has_pg_listen_notify<clean_t<_Type>>::value;

// postgres_diagnostics_connection
//   concept: constrains PostgreSQL connections supporting backend
// diagnostics and parameter status queries.
template<typename _Type>
concept postgres_diagnostics_connection =
    has_pg_diagnostics<clean_t<_Type>>::value;

// postgres_escape_connection
//   concept: constrains PostgreSQL connections supporting literal and
// identifier escaping.
template<typename _Type>
concept postgres_escape_connection =
    has_pg_escape<clean_t<_Type>>::value;

// postgres_large_object_connection
//   concept: constrains PostgreSQL connections supporting the large
// object API.
template<typename _Type>
concept postgres_large_object_connection =
    has_pg_large_objects<clean_t<_Type>>::value;

// postgres_schema_query_connection
//   concept: constrains PostgreSQL connections supporting schema
// introspection.
template<typename _Type>
concept postgres_schema_query_connection =
    has_pg_schema_query<clean_t<_Type>>::value;


// =============================================================================
// II.  PostgreSQL Capability Concepts
// =============================================================================

// postgres_send_query_connection
//   concept: constrains types exposing send_query(const string&).
template<typename _Type>
concept postgres_send_query_connection =
    pg_can_send_query<clean_t<_Type>>;

// postgres_result_polling_connection
//   concept: constrains types exposing get_result().
template<typename _Type>
concept postgres_result_polling_connection =
    is_detected<pg_get_result_t, clean_t<_Type>>::value;

// postgres_busy_state_query
//   concept: constrains types exposing is_busy() const.
template<typename _Type>
concept postgres_busy_state_query =
    is_detected<pg_is_busy_t, clean_t<_Type>>::value;

// postgres_input_consuming_connection
//   concept: constrains types exposing consume_input().
template<typename _Type>
concept postgres_input_consuming_connection =
    is_detected<pg_consume_input_t, clean_t<_Type>>::value;

// postgres_pipeline_enterable_connection
//   concept: constrains types exposing enter_pipeline().
template<typename _Type>
concept postgres_pipeline_enterable_connection =
    pg_can_enter_pipeline<clean_t<_Type>>;

// postgres_pipeline_exitable_connection
//   concept: constrains types exposing exit_pipeline().
template<typename _Type>
concept postgres_pipeline_exitable_connection =
    is_detected<pg_exit_pipeline_t, clean_t<_Type>>::value;

// postgres_pipeline_sync_connection
//   concept: constrains types exposing pipeline_sync().
template<typename _Type>
concept postgres_pipeline_sync_connection =
    is_detected<pg_pipeline_sync_t, clean_t<_Type>>::value;

// postgres_copy_in_connection
//   concept: constrains types exposing copy_in_start(const string&).
template<typename _Type>
concept postgres_copy_in_connection =
    is_detected<pg_copy_in_start_t, clean_t<_Type>>::value;

// postgres_copy_data_connection
//   concept: constrains types exposing copy_data(const char*, size_t).
template<typename _Type>
concept postgres_copy_data_connection =
    pg_can_copy_data<clean_t<_Type>>;

// postgres_copy_end_connection
//   concept: constrains types exposing copy_end().
template<typename _Type>
concept postgres_copy_end_connection =
    is_detected<pg_copy_end_t, clean_t<_Type>>::value;

// postgres_listenable_connection
//   concept: constrains types exposing listen(const string&).
template<typename _Type>
concept postgres_listenable_connection =
    pg_can_listen<clean_t<_Type>>;

// postgres_notifiable_connection
//   concept: constrains types exposing notify(channel, payload).
template<typename _Type>
concept postgres_notifiable_connection =
    pg_can_notify<clean_t<_Type>>;

// postgres_notification_query_connection
//   concept: constrains types exposing get_notification().
template<typename _Type>
concept postgres_notification_query_connection =
    is_detected<pg_get_notification_t, clean_t<_Type>>::value;

// postgres_parameterized_execution_connection
//   concept: constrains types exposing exec_params(query, params).
template<typename _Type>
concept postgres_parameterized_execution_connection =
    pg_can_exec_params<clean_t<_Type>>;

// postgres_backend_pid_connection
//   concept: constrains types exposing get_backend_pid() const.
template<typename _Type>
concept postgres_backend_pid_connection =
    is_detected<pg_get_backend_pid_t, clean_t<_Type>>::value;

// postgres_transaction_status_connection
//   concept: constrains types exposing get_transaction_status() const.
template<typename _Type>
concept postgres_transaction_status_connection =
    is_detected<pg_transaction_status_t, clean_t<_Type>>::value;

// postgres_parameter_status_connection
//   concept: constrains types exposing get_parameter_status(name) const.
template<typename _Type>
concept postgres_parameter_status_connection =
    is_detected<pg_parameter_status_t, clean_t<_Type>>::value;

// postgres_field_oid_introspection_connection
//   concept: constrains types exposing field_type_oid(int) const.
template<typename _Type>
concept postgres_field_oid_introspection_connection =
    is_detected<pg_field_type_oid_t, clean_t<_Type>>::value;

// postgres_table_exists_query
//   concept: constrains types exposing table_exists(name) const.
template<typename _Type>
concept postgres_table_exists_query =
    pg_can_query_schema<clean_t<_Type>>;

// postgres_table_names_query
//   concept: constrains types exposing get_table_names() const.
template<typename _Type>
concept postgres_table_names_query =
    is_detected<pg_get_table_names_t, clean_t<_Type>>::value;

// postgres_escape_literal_connection
//   concept: constrains types exposing escape_literal(text) const.
template<typename _Type>
concept postgres_escape_literal_connection =
    pg_can_escape_literal<clean_t<_Type>>;

// postgres_escape_identifier_connection
//   concept: constrains types exposing escape_identifier(text) const.
template<typename _Type>
concept postgres_escape_identifier_connection =
    is_detected<pg_escape_identifier_t, clean_t<_Type>>::value;

// postgres_lo_import_connection
//   concept: constrains types exposing lo_import(path).
template<typename _Type>
concept postgres_lo_import_connection =
    is_detected<pg_lo_import_t, clean_t<_Type>>::value;

// postgres_lo_export_connection
//   concept: constrains types exposing lo_export(oid, path).
template<typename _Type>
concept postgres_lo_export_connection =
    is_detected<pg_lo_export_t, clean_t<_Type>>::value;


// =============================================================================
// III. Tagless PostgreSQL Capability Concepts
// =============================================================================

// postgres_async_dispatchable
//   concept: constrains types satisfying the full tagless async query
// dispatch capability set.
template<typename _Type>
concept postgres_async_dispatchable =
    pg_does_async<clean_t<_Type>>;

// postgres_pipelined_connection
//   concept: constrains types satisfying the full tagless pipeline
// capability set.
template<typename _Type>
concept postgres_pipelined_connection =
    pg_does_pipeline<clean_t<_Type>>;

// postgres_copy_protocol_connection
//   concept: constrains types satisfying the full tagless COPY protocol
// capability set.
template<typename _Type>
concept postgres_copy_protocol_connection =
    pg_does_copy<clean_t<_Type>>;

// postgres_listen_notifiable_connection
//   concept: constrains types satisfying the full tagless LISTEN /
// NOTIFY capability set.
template<typename _Type>
concept postgres_listen_notifiable_connection =
    pg_does_listen_notify<clean_t<_Type>>;

// postgres_full_connection
//   concept: constrains types satisfying the complete tagless
// PostgreSQL connection capability set.
template<typename _Type>
concept postgres_full_connection =
    pg_is_full_connection<clean_t<_Type>>;

// pg_full_connection
//   concept: alias for postgres_full_connection using the shorter pg
// prefix.
template<typename _Type>
concept pg_full_connection =
    postgres_full_connection<_Type>;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_POSTGRES_CONCEPTS_
