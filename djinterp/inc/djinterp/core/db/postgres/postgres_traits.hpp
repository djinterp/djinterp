/******************************************************************************
* djinterp [database]                                      postgres_traits.hpp
* 
* djinterp PostgreSQL traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to PostgreSQL connections via libpq,
* including:
*   - asynchronous / non-blocking query dispatch (send_query, get_result)
*   - pipeline mode management (enter_pipeline, exit_pipeline)
*   - COPY protocol (copy_in, copy_out, copy_data, copy_end)
*   - LISTEN / NOTIFY asynchronous notification
*   - large object API (lo_import, lo_export)
*   - parameterized query execution (exec_params)
*   - connection string / keyword-value pair API
*   - server-side PREPARE / EXECUTE
*   - result field OID introspection (field_type_oid)
*   - connection status and diagnostics (status, error_message,
*     backend_pid, transaction_status, parameter_status)
*   - schema introspection helpers (table_exists, get_table_names)
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided. The detectors target C++ wrapper methods that a concrete
* pg_connection implementation would expose around the libpq C API.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17.
*
*   NAMING CONVENTION:
*   expression detectors:   pg_<method>_t
*   struct-based traits:    has_pg_<capability>
*   variable template _v:   has_pg_<capability>_v
*   tagless traits:          pg_can_<action>
*   compound tagless traits: pg_does_<category>
*
*
* path:      /inc/djinterp/core/db/postgres/postgres_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_DATABASE_POSTGRES_TRAITS_
#define DJINTERP_DATABASE_POSTGRES_TRAITS_

#include "../djinterp.hpp"
#include "./database_traits.hpp"
#include <vector>


NS_DJINTERP
NS_DATABASE
// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  asynchronous query dispatch
// -------------------------------------------------------------------------

// pg_send_query_t
//   detector: send_query(const std::string&) method.
// wraps PQsendQuery().
template<typename _T>
using pg_send_query_t = decltype(std::declval<_T&>().send_query(
    std::declval<const std::string&>()));

// pg_get_result_t
//   detector: get_result() method.
// wraps PQgetResult().
template<typename _T>
using pg_get_result_t =
    decltype(std::declval<_T&>().get_result());

// pg_is_busy_t
//   detector: is_busy() const method.
// wraps PQisBusy().
template<typename _T>
using pg_is_busy_t =
    decltype(std::declval<const _T&>().is_busy());

// pg_consume_input_t
//   detector: consume_input() method.
// wraps PQconsumeInput().
template<typename _T>
using pg_consume_input_t =
    decltype(std::declval<_T&>().consume_input());

// -------------------------------------------------------------------------
// B.  pipeline mode
// -------------------------------------------------------------------------

// pg_enter_pipeline_t
//   detector: enter_pipeline() method.
// wraps PQenterPipelineMode().
template<typename _T>
using pg_enter_pipeline_t =
    decltype(std::declval<_T&>().enter_pipeline());

// pg_exit_pipeline_t
//   detector: exit_pipeline() method.
// wraps PQexitPipelineMode().
template<typename _T>
using pg_exit_pipeline_t =
    decltype(std::declval<_T&>().exit_pipeline());

// pg_pipeline_sync_t
//   detector: pipeline_sync() method.
// wraps PQpipelineSync().
template<typename _T>
using pg_pipeline_sync_t =
    decltype(std::declval<_T&>().pipeline_sync());

// -------------------------------------------------------------------------
// C.  COPY protocol
// -------------------------------------------------------------------------

// pg_copy_in_start_t
//   detector: copy_in_start(const std::string&) method.
template<typename _T>
using pg_copy_in_start_t =
    decltype(std::declval<_T&>().copy_in_start(
        std::declval<const std::string&>()));

// pg_copy_data_t
//   detector: copy_data(const char*, std::size_t) method.
// wraps PQputCopyData().
template<typename _T>
using pg_copy_data_t = decltype(std::declval<_T&>().copy_data(
    std::declval<const char*>(),
    std::declval<std::size_t>()));

// pg_copy_end_t
//   detector: copy_end() method.
// wraps PQputCopyEnd().
template<typename _T>
using pg_copy_end_t =
    decltype(std::declval<_T&>().copy_end());

// -------------------------------------------------------------------------
// D.  LISTEN / NOTIFY
// -------------------------------------------------------------------------

// pg_listen_t
//   detector: listen(const std::string&) method.
// executes LISTEN <channel>.
template<typename _T>
using pg_listen_t = decltype(std::declval<_T&>().listen(
    std::declval<const std::string&>()));

// pg_notify_t
//   detector: notify(const std::string&, const std::string&) method.
// executes pg_notify().
template<typename _T>
using pg_notify_t = decltype(std::declval<_T&>().notify(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// pg_get_notification_t
//   detector: get_notification() method.
// wraps PQnotifies().
template<typename _T>
using pg_get_notification_t =
    decltype(std::declval<_T&>().get_notification());

// -------------------------------------------------------------------------
// E.  parameterized execution
// -------------------------------------------------------------------------

// pg_exec_params_t
//   detector: exec_params(const std::string&,
// const std::vector<std::string>&) method.
// wraps PQexecParams().
template<typename _T>
using pg_exec_params_t = decltype(std::declval<_T&>().exec_params(
    std::declval<const std::string&>(),
    std::declval<const std::vector<std::string>&>()));

// -------------------------------------------------------------------------
// F.  connection diagnostics
// -------------------------------------------------------------------------

// pg_get_backend_pid_t
//   detector: get_backend_pid() const method.
// wraps PQbackendPID().
template<typename _T>
using pg_get_backend_pid_t =
    decltype(std::declval<const _T&>().get_backend_pid());

// pg_transaction_status_t
//   detector: get_transaction_status() const method.
// wraps PQtransactionStatus().
template<typename _T>
using pg_transaction_status_t =
    decltype(std::declval<const _T&>().get_transaction_status());

// pg_parameter_status_t
//   detector: get_parameter_status(const std::string&) const method.
// wraps PQparameterStatus().
template<typename _T>
using pg_parameter_status_t =
    decltype(std::declval<const _T&>().get_parameter_status(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// G.  result field OID introspection
// -------------------------------------------------------------------------

// pg_field_type_oid_t
//   detector: field_type_oid(int) const method.
// wraps PQftype().
template<typename _T>
using pg_field_type_oid_t =
    decltype(std::declval<const _T&>().field_type_oid(
        std::declval<int>()));

// -------------------------------------------------------------------------
// H.  schema introspection
// -------------------------------------------------------------------------

// pg_table_exists_t
//   detector: table_exists(const std::string&) const method.
template<typename _T>
using pg_table_exists_t =
    decltype(std::declval<const _T&>().table_exists(
        std::declval<const std::string&>()));

// pg_get_table_names_t
//   detector: get_table_names() const method.
template<typename _T>
using pg_get_table_names_t =
    decltype(std::declval<const _T&>().get_table_names());

// -------------------------------------------------------------------------
// I.  escape
// -------------------------------------------------------------------------

// pg_escape_literal_t
//   detector: escape_literal(const std::string&) const method.
// wraps PQescapeLiteral().
template<typename _T>
using pg_escape_literal_t =
    decltype(std::declval<const _T&>().escape_literal(
        std::declval<const std::string&>()));

// pg_escape_identifier_t
//   detector: escape_identifier(const std::string&) const method.
// wraps PQescapeIdentifier().
template<typename _T>
using pg_escape_identifier_t =
    decltype(std::declval<const _T&>().escape_identifier(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// J.  large objects
// -------------------------------------------------------------------------

// pg_lo_import_t
//   detector: lo_import(const std::string&) method.
template<typename _T>
using pg_lo_import_t = decltype(std::declval<_T&>().lo_import(
    std::declval<const std::string&>()));

// pg_lo_export_t
//   detector: lo_export(unsigned int, const std::string&) method.
template<typename _T>
using pg_lo_export_t = decltype(std::declval<_T&>().lo_export(
    std::declval<unsigned int>(),
    std::declval<const std::string&>()));


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_pg_async
//   trait: checks if type _T supports asynchronous query dispatch
// (send_query + get_result + is_busy + consume_input).
template<typename _T>
struct has_pg_async : djinterp::conjunction<
    is_detected<pg_send_query_t, _T>,
    is_detected<pg_get_result_t, _T>,
    is_detected<pg_is_busy_t, _T>,
    is_detected<pg_consume_input_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_async_v = has_pg_async<_T>::value;
#endif

// has_pg_pipeline
//   trait: checks if type _T supports pipeline mode
// (enter_pipeline + exit_pipeline + pipeline_sync).
template<typename _T>
struct has_pg_pipeline : djinterp::conjunction<
    is_detected<pg_enter_pipeline_t, _T>,
    is_detected<pg_exit_pipeline_t, _T>,
    is_detected<pg_pipeline_sync_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_pipeline_v = has_pg_pipeline<_T>::value;
#endif

// has_pg_copy
//   trait: checks if type _T supports the COPY protocol
// (copy_in_start + copy_data + copy_end).
template<typename _T>
struct has_pg_copy : djinterp::conjunction<
    is_detected<pg_copy_in_start_t, _T>,
    is_detected<pg_copy_data_t, _T>,
    is_detected<pg_copy_end_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_copy_v = has_pg_copy<_T>::value;
#endif

// has_pg_listen_notify
//   trait: checks if type _T supports LISTEN/NOTIFY
// (listen + notify + get_notification).
template<typename _T>
struct has_pg_listen_notify : djinterp::conjunction<
    is_detected<pg_listen_t, _T>,
    is_detected<pg_notify_t, _T>,
    is_detected<pg_get_notification_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_listen_notify_v =
        has_pg_listen_notify<_T>::value;
#endif

// has_pg_diagnostics
//   trait: checks if type _T supports connection diagnostics
// (get_backend_pid + get_transaction_status + get_parameter_status).
template<typename _T>
struct has_pg_diagnostics : djinterp::conjunction<
    is_detected<pg_get_backend_pid_t, _T>,
    is_detected<pg_transaction_status_t, _T>,
    is_detected<pg_parameter_status_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_diagnostics_v =
        has_pg_diagnostics<_T>::value;
#endif

// has_pg_escape
//   trait: checks if type _T supports PostgreSQL escaping
// (escape_literal + escape_identifier).
template<typename _T>
struct has_pg_escape : djinterp::conjunction<
    is_detected<pg_escape_literal_t, _T>,
    is_detected<pg_escape_identifier_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_escape_v = has_pg_escape<_T>::value;
#endif

// has_pg_large_objects
//   trait: checks if type _T supports large object API
// (lo_import + lo_export).
template<typename _T>
struct has_pg_large_objects : djinterp::conjunction<
    is_detected<pg_lo_import_t, _T>,
    is_detected<pg_lo_export_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_large_objects_v =
        has_pg_large_objects<_T>::value;
#endif

// has_pg_schema_query
//   trait: checks if type _T supports schema introspection
// (table_exists + get_table_names).
template<typename _T>
struct has_pg_schema_query : djinterp::conjunction<
    is_detected<pg_table_exists_t, _T>,
    is_detected<pg_get_table_names_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_pg_schema_query_v =
        has_pg_schema_query<_T>::value;
#endif

// is_pg_connection
//   trait: compound trait verifying type _T implements a PostgreSQL
// connection interface (connection + async + diagnostics + escape +
// schema queries).
template<typename _T>
struct is_pg_connection : djinterp::conjunction<
    is_connection<_T>,
    has_pg_async<_T>,
    has_pg_diagnostics<_T>,
    has_pg_escape<_T>,
    has_pg_schema_query<_T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool is_pg_connection_v = is_pg_connection<_T>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// pg_can_send_query
//   tagless trait: true if _T has send_query().
template<typename _T,
         typename = void>
constexpr bool pg_can_send_query = false;

template<typename _T>
constexpr bool pg_can_send_query<_T,
    std::void_t<pg_send_query_t<_T>>> = true;

// pg_can_enter_pipeline
//   tagless trait: true if _T has enter_pipeline().
template<typename _T,
         typename = void>
constexpr bool pg_can_enter_pipeline = false;

template<typename _T>
constexpr bool pg_can_enter_pipeline<_T,
    std::void_t<pg_enter_pipeline_t<_T>>> = true;

// pg_can_copy_data
//   tagless trait: true if _T has copy_data().
template<typename _T,
         typename = void>
constexpr bool pg_can_copy_data = false;

template<typename _T>
constexpr bool pg_can_copy_data<_T,
    std::void_t<pg_copy_data_t<_T>>> = true;

// pg_can_listen
//   tagless trait: true if _T has listen().
template<typename _T,
         typename = void>
constexpr bool pg_can_listen = false;

template<typename _T>
constexpr bool pg_can_listen<_T,
    std::void_t<pg_listen_t<_T>>> = true;

// pg_can_notify
//   tagless trait: true if _T has notify().
template<typename _T,
         typename = void>
constexpr bool pg_can_notify = false;

template<typename _T>
constexpr bool pg_can_notify<_T,
    std::void_t<pg_notify_t<_T>>> = true;

// pg_can_exec_params
//   tagless trait: true if _T has exec_params().
template<typename _T,
         typename = void>
constexpr bool pg_can_exec_params = false;

template<typename _T>
constexpr bool pg_can_exec_params<_T,
    std::void_t<pg_exec_params_t<_T>>> = true;

// pg_can_escape_literal
//   tagless trait: true if _T has escape_literal().
template<typename _T,
         typename = void>
constexpr bool pg_can_escape_literal = false;

template<typename _T>
constexpr bool pg_can_escape_literal<_T,
    std::void_t<pg_escape_literal_t<_T>>> = true;

// pg_can_query_schema
//   tagless trait: true if _T has table_exists().
template<typename _T,
         typename = void>
constexpr bool pg_can_query_schema = false;

template<typename _T>
constexpr bool pg_can_query_schema<_T,
    std::void_t<pg_table_exists_t<_T>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// pg_does_async
//   tagless trait: true if _T supports async query dispatch.
template<typename _T,
         typename = void>
constexpr bool pg_does_async = false;

template<typename _T>
constexpr bool pg_does_async<_T, std::void_t<
    pg_send_query_t<_T>,
    pg_get_result_t<_T>,
    pg_is_busy_t<_T>,
    pg_consume_input_t<_T>>> = true;

// pg_does_pipeline
//   tagless trait: true if _T supports pipeline mode.
template<typename _T,
         typename = void>
constexpr bool pg_does_pipeline = false;

template<typename _T>
constexpr bool pg_does_pipeline<_T, std::void_t<
    pg_enter_pipeline_t<_T>,
    pg_exit_pipeline_t<_T>,
    pg_pipeline_sync_t<_T>>> = true;

// pg_does_copy
//   tagless trait: true if _T supports the COPY protocol.
template<typename _T,
         typename = void>
constexpr bool pg_does_copy = false;

template<typename _T>
constexpr bool pg_does_copy<_T, std::void_t<
    pg_copy_in_start_t<_T>,
    pg_copy_data_t<_T>,
    pg_copy_end_t<_T>>> = true;

// pg_does_listen_notify
//   tagless trait: true if _T supports LISTEN/NOTIFY.
template<typename _T,
         typename = void>
constexpr bool pg_does_listen_notify = false;

template<typename _T>
constexpr bool pg_does_listen_notify<_T, std::void_t<
    pg_listen_t<_T>,
    pg_notify_t<_T>,
    pg_get_notification_t<_T>>> = true;

// pg_is_full_connection
//   tagless trait: true if _T satisfies the complete PostgreSQL
// connection interface.
template<typename _T>
constexpr bool pg_is_full_connection =
    ( is_connectable<_T>         &&
      pg_does_async<_T>          &&
      pg_can_escape_literal<_T>  &&
      pg_can_query_schema<_T> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

// enable_if_pg_connection
//   type: SFINAE helper for PostgreSQL connection constraints.
template<typename _T>
using enable_if_pg_connection =
    typename std::enable_if<is_pg_connection<_T>::value>::type;

// enable_if_has_pg_async
//   type: SFINAE helper for PostgreSQL async constraints.
template<typename _T>
using enable_if_has_pg_async =
    typename std::enable_if<has_pg_async<_T>::value>::type;

// enable_if_has_pg_pipeline
//   type: SFINAE helper for PostgreSQL pipeline constraints.
template<typename _T>
using enable_if_has_pg_pipeline =
    typename std::enable_if<has_pg_pipeline<_T>::value>::type;

// enable_if_has_pg_copy
//   type: SFINAE helper for PostgreSQL COPY constraints.
template<typename _T>
using enable_if_has_pg_copy =
    typename std::enable_if<has_pg_copy<_T>::value>::type;

// enable_if_has_pg_listen_notify
//   type: SFINAE helper for PostgreSQL LISTEN/NOTIFY constraints.
template<typename _T>
using enable_if_has_pg_listen_notify =
    typename std::enable_if<has_pg_listen_notify<_T>::value>::type;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_POSTGRES_TRAITS_
