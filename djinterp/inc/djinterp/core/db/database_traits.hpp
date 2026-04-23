/******************************************************************************
* djinterp [database]                                      database_traits.hpp
* 
* djinterp database traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting and verifying database implementation capabilities without using
* virtual functions, including:
*   - capability detection (transactions, savepoints, prepared statements)
*   - interface verification
*   - type requirements and concepts
*   - compile-time feature checking
*
*   The traits system enables zero-overhead abstraction by resolving all
* polymorphism at compile time through templates rather than runtime virtual
* dispatch.
*
*   PORTABILITY:
*   This header reuses the detection idiom (is_detected, nonesuch, void_t)
* and logical connectives (conjunction) from type_traits.hpp, which provides
* portable C++11+ implementations of these C++17 facilities. Variable
* template (_v) aliases are gated behind
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES.
*
* 
* path:      /inc/djinterp/core/db/database_traits.hpp                                           
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_TRAITS_
#define DJINTERP_DATABASE_TRAITS_

// std
#include <string>
#include <cstddef>
#include <cstdint>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"



NS_DJINTERP
NS_DATABASE

using djinterp::is_detected;


// =============================================================================
// II.  CONNECTION TRAIT DETECTORS
// =============================================================================

// connect_t
//   detector: connect() method.
template<typename _Type>
using connect_t = decltype(std::declval<_Type&>().connect());

// disconnect_t
//   detector: disconnect() method.
template<typename _Type>
using disconnect_t = decltype(std::declval<_Type&>().disconnect());

// is_connected_t
//   detector: is_connected() const method.
template<typename _Type>
using is_connected_t = decltype(std::declval<const _Type&>().is_connected());

// execute_query_t
//   detector: execute_query(const std::string&) method.
template<typename _Type>
using execute_query_t = decltype(std::declval<_Type&>().execute_query(
    std::declval<const std::string&>()));

// execute_update_t
//   detector: execute_update(const std::string&) method.
template<typename _Type>
using execute_update_t = decltype(std::declval<_Type&>().execute_update(
    std::declval<const std::string&>()));

// prepare_t
//   detector: prepare(const std::string&) method.
template<typename _Type>
using prepare_t = decltype(std::declval<_Type&>().prepare(
    std::declval<const std::string&>()));


// =============================================================================
// III. TRANSACTION TRAIT DETECTORS
// =============================================================================

// begin_transaction_t
//   detector: begin_transaction() method.
template<typename _Type>
using begin_transaction_t =
    decltype(std::declval<_Type&>().begin_transaction());

// commit_t
//   detector: commit() method.
template<typename _Type>
using commit_t = decltype(std::declval<_Type&>().commit());

// rollback_t
//   detector: rollback() method.
template<typename _Type>
using rollback_t = decltype(std::declval<_Type&>().rollback());

// in_transaction_t
//   detector: in_transaction() const method.
template<typename _Type>
using in_transaction_t =
    decltype(std::declval<const _Type&>().in_transaction());

// create_savepoint_t
//   detector: create_savepoint(const std::string&) method.
template<typename _Type>
using create_savepoint_t = decltype(std::declval<_Type&>().create_savepoint(
    std::declval<const std::string&>()));

// rollback_to_savepoint_t
//   detector: rollback_to_savepoint(const std::string&) method.
template<typename _Type>
using rollback_to_savepoint_t =
    decltype(std::declval<_Type&>().rollback_to_savepoint(
        std::declval<const std::string&>()));


// =============================================================================
// IV.  RESULT SET TRAIT DETECTORS
// =============================================================================

// next_t
//   detector: next() method.
template<typename _Type>
using next_t = decltype(std::declval<_Type&>().next());

// column_count_t
//   detector: column_count() const method.
template<typename _Type>
using column_count_t =
    decltype(std::declval<const _Type&>().column_count());

// column_name_t
//   detector: column_name(std::size_t) const method.
template<typename _Type>
using column_name_t = decltype(std::declval<const _Type&>().column_name(
    std::declval<std::size_t>()));

// get_value_index_t
//   detector: get_value(std::size_t) const method.
template<typename _Type>
using get_value_index_t =
    decltype(std::declval<const _Type&>().get_value(
        std::declval<std::size_t>()));

// get_value_name_t
//   detector: get_value(const std::string&) const method.
template<typename _Type>
using get_value_name_t =
    decltype(std::declval<const _Type&>().get_value(
        std::declval<const std::string&>()));


// =============================================================================
// V.   STATEMENT TRAIT DETECTORS
// =============================================================================

// bind_int_t
//   detector: bind_int(std::size_t, std::int32_t) method.
template<typename _Type>
using bind_int_t = decltype(std::declval<_Type&>().bind_int(
    std::declval<std::size_t>(),
    std::declval<std::int32_t>()));

// bind_string_t
//   detector: bind_string(std::size_t, const std::string&) method.
template<typename _Type>
using bind_string_t = decltype(std::declval<_Type&>().bind_string(
    std::declval<std::size_t>(),
    std::declval<const std::string&>()));

// execute_t
//   detector: execute() method.
template<typename _Type>
using execute_t = decltype(std::declval<_Type&>().execute());

// parameter_count_t
//   detector: parameter_count() const method.
template<typename _Type>
using parameter_count_t =
    decltype(std::declval<const _Type&>().parameter_count());


// =============================================================================
// VI.  CAPABILITY TRAITS
// =============================================================================

// has_connect
//   trait: checks if type _Type has a connect() method.
template<typename _Type>
struct has_connect : is_detected<connect_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_connect_v
    //   value: convenience alias for has_connect<_Type>::value.
    template<typename _Type>
    constexpr bool has_connect_v = has_connect<_Type>::value;
#endif

// has_disconnect
//   trait: checks if type _Type has a disconnect() method.
template<typename _Type>
struct has_disconnect : is_detected<disconnect_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_disconnect_v
    //   value: convenience alias for has_disconnect<_Type>::value.
    template<typename _Type>
    constexpr bool has_disconnect_v = has_disconnect<_Type>::value;
#endif

// has_execute_query
//   trait: checks if type _Type has an execute_query() method.
template<typename _Type>
struct has_execute_query : is_detected<execute_query_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_execute_query_v
    //   value: convenience alias for has_execute_query<_Type>::value.
    template<typename _Type>
    constexpr bool has_execute_query_v = has_execute_query<_Type>::value;
#endif

// has_transactions
//   trait: checks if type _Type supports transactions
// (begin_transaction, commit, rollback).
template<typename _Type>
struct has_transactions : djinterp::conjunction<
    is_detected<begin_transaction_t, _Type>,
    is_detected<commit_t, _Type>,
    is_detected<rollback_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_transactions_v
    //   value: convenience alias for has_transactions<_Type>::value.
    template<typename _Type>
    constexpr bool has_transactions_v = has_transactions<_Type>::value;
#endif

// has_savepoints
//   trait: checks if type _Type supports savepoints
// (create_savepoint, rollback_to_savepoint).
template<typename _Type>
struct has_savepoints : djinterp::conjunction<
    is_detected<create_savepoint_t, _Type>,
    is_detected<rollback_to_savepoint_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_savepoints_v
    //   value: convenience alias for has_savepoints<_Type>::value.
    template<typename _Type>
    constexpr bool has_savepoints_v = has_savepoints<_Type>::value;
#endif

// has_prepared_statements
//   trait: checks if type _Type supports prepared statements.
template<typename _Type>
struct has_prepared_statements : is_detected<prepare_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prepared_statements_v
    //   value: convenience alias for
    // has_prepared_statements<_Type>::value.
    template<typename _Type>
    constexpr bool has_prepared_statements_v =
        has_prepared_statements<_Type>::value;
#endif


// =============================================================================
// VII. INTERFACE VERIFICATION TRAITS
// =============================================================================

// is_connection
//   trait: verifies type _Type implements the connection interface
// (connect, disconnect, is_connected, execute_query, execute_update).
template<typename _Type>
struct is_connection : djinterp::conjunction<
    has_connect<_Type>,
    has_disconnect<_Type>,
    is_detected<is_connected_t, _Type>,
    has_execute_query<_Type>,
    is_detected<execute_update_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_connection_v
    //   value: convenience alias for is_connection<_Type>::value.
    template<typename _Type>
    constexpr bool is_connection_v = is_connection<_Type>::value;
#endif

// is_result_set
//   trait: verifies type _Type implements the result_set interface
// (next, column_count, column_name, get_value).
template<typename _Type>
struct is_result_set : djinterp::conjunction<
    is_detected<next_t, _Type>,
    is_detected<column_count_t, _Type>,
    is_detected<column_name_t, _Type>,
    is_detected<get_value_index_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_result_set_v
    //   value: convenience alias for is_result_set<_Type>::value.
    template<typename _Type>
    constexpr bool is_result_set_v = is_result_set<_Type>::value;
#endif

// is_statement
//   trait: verifies type _Type implements the statement interface
// (bind_int, bind_string, execute, parameter_count).
template<typename _Type>
struct is_statement : djinterp::conjunction<
    is_detected<bind_int_t, _Type>,
    is_detected<bind_string_t, _Type>,
    is_detected<execute_t, _Type>,
    is_detected<parameter_count_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_statement_v
    //   value: convenience alias for is_statement<_Type>::value.
    template<typename _Type>
    constexpr bool is_statement_v = is_statement<_Type>::value;
#endif


// =============================================================================
// VIII. VENDOR CONNECTION TRAIT DETECTORS
// =============================================================================

// ping_t
//   detector: ping() const method.
template<typename _Type>
using ping_t = decltype(std::declval<const _Type&>().ping());

// get_database_type_t
//   detector: get_database_type() const method.
template<typename _Type>
using get_database_type_t =
    decltype(std::declval<const _Type&>().get_database_type());

// get_server_version_t
//   detector: get_server_version() const method.
template<typename _Type>
using get_server_version_t =
    decltype(std::declval<const _Type&>().get_server_version());

// get_native_handle_t
//   detector: get_native_handle() method.
template<typename _Type>
using get_native_handle_t =
    decltype(std::declval<_Type&>().get_native_handle());

// get_last_error_t
//   detector: get_last_error() const method.
template<typename _Type>
using get_last_error_t =
    decltype(std::declval<const _Type&>().get_last_error());

// get_last_error_code_t
//   detector: get_last_error_code() const method.
template<typename _Type>
using get_last_error_code_t =
    decltype(std::declval<const _Type&>().get_last_error_code());

// set_auto_commit_t
//   detector: set_auto_commit(bool) method.
template<typename _Type>
using set_auto_commit_t = decltype(std::declval<_Type&>().set_auto_commit(
    std::declval<bool>()));

// reconnect_t
//   detector: reconnect() method.
template<typename _Type>
using reconnect_t = decltype(std::declval<_Type&>().reconnect());

// get_last_insert_id_t
//   detector: get_last_insert_id() const method.
template<typename _Type>
using get_last_insert_id_t =
    decltype(std::declval<const _Type&>().get_last_insert_id());

// get_affected_rows_t
//   detector: get_affected_rows() const method.
template<typename _Type>
using get_affected_rows_t =
    decltype(std::declval<const _Type&>().get_affected_rows());


// =============================================================================
// IX.   VENDOR CAPABILITY TRAITS
// =============================================================================

// has_ping
//   trait: checks if type _Type has a ping() method.
template<typename _Type>
struct has_ping : is_detected<ping_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_ping_v
    //   value: convenience alias for has_ping<_Type>::value.
    template<typename _Type>
    constexpr bool has_ping_v = has_ping<_Type>::value;
#endif

// has_native_handle
//   trait: checks if type _Type exposes a native database handle.
template<typename _Type>
struct has_native_handle : is_detected<get_native_handle_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_native_handle_v
    //   value: convenience alias for has_native_handle<_Type>::value.
    template<typename _Type>
    constexpr bool has_native_handle_v = has_native_handle<_Type>::value;
#endif

// has_reconnect
//   trait: checks if type _Type supports reconnection.
template<typename _Type>
struct has_reconnect : is_detected<reconnect_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_reconnect_v
    //   value: convenience alias for has_reconnect<_Type>::value.
    template<typename _Type>
    constexpr bool has_reconnect_v = has_reconnect<_Type>::value;
#endif

// has_metadata
//   trait: checks if type _Type provides connection metadata
// (get_database_type, get_server_version, get_last_error).
template<typename _Type>
struct has_metadata : djinterp::conjunction<
    is_detected<get_database_type_t, _Type>,
    is_detected<get_server_version_t, _Type>,
    is_detected<get_last_error_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_metadata_v
    //   value: convenience alias for has_metadata<_Type>::value.
    template<typename _Type>
    constexpr bool has_metadata_v = has_metadata<_Type>::value;
#endif

// has_error_reporting
//   trait: checks if type _Type provides error reporting
// (get_last_error, get_last_error_code).
template<typename _Type>
struct has_error_reporting : djinterp::conjunction<
    is_detected<get_last_error_t, _Type>,
    is_detected<get_last_error_code_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_error_reporting_v
    //   value: convenience alias for has_error_reporting<_Type>::value.
    template<typename _Type>
    constexpr bool has_error_reporting_v = has_error_reporting<_Type>::value;
#endif

// has_row_info
//   trait: checks if type _Type provides row mutation information
// (get_last_insert_id, get_affected_rows).
template<typename _Type>
struct has_row_info : djinterp::conjunction<
    is_detected<get_last_insert_id_t, _Type>,
    is_detected<get_affected_rows_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_row_info_v
    //   value: convenience alias for has_row_info<_Type>::value.
    template<typename _Type>
    constexpr bool has_row_info_v = has_row_info<_Type>::value;
#endif

// is_vendor_connection
//   trait: verifies type _Type implements a complete vendor connection
// interface (connection + metadata + ping + error reporting).
template<typename _Type>
struct is_vendor_connection : djinterp::conjunction<
    is_connection<_Type>,
    has_metadata<_Type>,
    has_ping<_Type>,
    has_error_reporting<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_vendor_connection_v
    //   value: convenience alias for is_vendor_connection<_Type>::value.
    template<typename _Type>
    constexpr bool is_vendor_connection_v =
        is_vendor_connection<_Type>::value;
#endif


// =============================================================================
// X.    TAGLESS SFINAE TRAITS
// =============================================================================
// Tagless traits resolve directly to constexpr bool via variable template
// partial specialization over void_t. Unlike the struct-based traits in
// sections VI-IX these have NO intermediate struct wrapper, NO ::value
// accessor, and NO _v alias. They are first-class bool values usable
// directly in if-constexpr, static_assert, and enable_if without
// syntactic noise.
//
// Naming convention: the prefix mirrors the struct-based form but
// drops the has_/is_ prefix and uses the `can_` or `does_` prefix to
// indicate an action/capability, or `is_` for identity checks.
//
// These require C++17 (void_t + variable template partial
// specialization). The database module already gates on C++17 via
// database_common.hpp.

// -------------------------------------------------------------------------
// A.  connection capability tags
// -------------------------------------------------------------------------

// can_connect
//   tagless trait: true if _Type has a connect() method.
template<typename _Type,
         typename = void>
constexpr bool can_connect = false;

template<typename _Type>
constexpr bool can_connect<_Type, std::void_t<connect_t<_Type>>> = true;

// can_disconnect
//   tagless trait: true if _Type has a disconnect() method.
template<typename _Type,
         typename = void>
constexpr bool can_disconnect = false;

template<typename _Type>
constexpr bool can_disconnect<_Type, std::void_t<disconnect_t<_Type>>> = true;

// can_reconnect
//   tagless trait: true if _Type has a reconnect() method.
template<typename _Type,
         typename = void>
constexpr bool can_reconnect = false;

template<typename _Type>
constexpr bool can_reconnect<_Type, std::void_t<reconnect_t<_Type>>> = true;

// can_ping
//   tagless trait: true if _Type has a ping() method.
template<typename _Type,
         typename = void>
constexpr bool can_ping = false;

template<typename _Type>
constexpr bool can_ping<_Type, std::void_t<ping_t<_Type>>> = true;

// can_execute_query
//   tagless trait: true if _Type has an execute_query() method.
template<typename _Type,
         typename = void>
constexpr bool can_execute_query = false;

template<typename _Type>
constexpr bool can_execute_query<_Type,
    std::void_t<execute_query_t<_Type>>> = true;

// can_execute_update
//   tagless trait: true if _Type has an execute_update() method.
template<typename _Type,
         typename = void>
constexpr bool can_execute_update = false;

template<typename _Type>
constexpr bool can_execute_update<_Type,
    std::void_t<execute_update_t<_Type>>> = true;

// can_prepare
//   tagless trait: true if _Type has a prepare() method.
template<typename _Type,
         typename = void>
constexpr bool can_prepare = false;

template<typename _Type>
constexpr bool can_prepare<_Type, std::void_t<prepare_t<_Type>>> = true;

// -------------------------------------------------------------------------
// B.  transaction capability tags
// -------------------------------------------------------------------------

// can_begin_transaction
//   tagless trait: true if _Type has a begin_transaction() method.
template<typename _Type,
         typename = void>
constexpr bool can_begin_transaction = false;

template<typename _Type>
constexpr bool can_begin_transaction<_Type,
    std::void_t<begin_transaction_t<_Type>>> = true;

// can_commit
//   tagless trait: true if _Type has a commit() method.
template<typename _Type,
         typename = void>
constexpr bool can_commit = false;

template<typename _Type>
constexpr bool can_commit<_Type, std::void_t<commit_t<_Type>>> = true;

// can_rollback
//   tagless trait: true if _Type has a rollback() method.
template<typename _Type,
         typename = void>
constexpr bool can_rollback = false;

template<typename _Type>
constexpr bool can_rollback<_Type, std::void_t<rollback_t<_Type>>> = true;

// can_create_savepoint
//   tagless trait: true if _Type has a create_savepoint() method.
template<typename _Type,
         typename = void>
constexpr bool can_create_savepoint = false;

template<typename _Type>
constexpr bool can_create_savepoint<_Type,
    std::void_t<create_savepoint_t<_Type>>> = true;

// can_rollback_to_savepoint
//   tagless trait: true if _Type has a rollback_to_savepoint() method.
template<typename _Type,
         typename = void>
constexpr bool can_rollback_to_savepoint = false;

template<typename _Type>
constexpr bool can_rollback_to_savepoint<_Type,
    std::void_t<rollback_to_savepoint_t<_Type>>> = true;

// -------------------------------------------------------------------------
// C.  metadata and error capability tags
// -------------------------------------------------------------------------

// does_report_errors
//   tagless trait: true if _Type provides both get_last_error() and
// get_last_error_code().
template<typename _Type,
         typename = void>
constexpr bool does_report_errors = false;

template<typename _Type>
constexpr bool does_report_errors<_Type, std::void_t<
    get_last_error_t<_Type>,
    get_last_error_code_t<_Type>>> = true;

// does_expose_native_handle
//   tagless trait: true if _Type provides get_native_handle().
template<typename _Type,
         typename = void>
constexpr bool does_expose_native_handle = false;

template<typename _Type>
constexpr bool does_expose_native_handle<_Type,
    std::void_t<get_native_handle_t<_Type>>> = true;

// does_report_row_info
//   tagless trait: true if _Type provides get_last_insert_id() and
// get_affected_rows().
template<typename _Type,
         typename = void>
constexpr bool does_report_row_info = false;

template<typename _Type>
constexpr bool does_report_row_info<_Type, std::void_t<
    get_last_insert_id_t<_Type>,
    get_affected_rows_t<_Type>>> = true;

// does_report_version
//   tagless trait: true if _Type provides get_server_version().
template<typename _Type,
         typename = void>
constexpr bool does_report_version = false;

template<typename _Type>
constexpr bool does_report_version<_Type,
    std::void_t<get_server_version_t<_Type>>> = true;

// -------------------------------------------------------------------------
// D.  compound capability tags
// -------------------------------------------------------------------------

// does_transact
//   tagless trait: true if _Type supports full transactions
// (begin_transaction + commit + rollback).
template<typename _Type>
constexpr bool does_transact =
    ( can_begin_transaction<_Type> &&
      can_commit<_Type>            &&
      can_rollback<_Type> );

// does_savepoint
//   tagless trait: true if _Type supports savepoints
// (create_savepoint + rollback_to_savepoint).
template<typename _Type>
constexpr bool does_savepoint =
    ( can_create_savepoint<_Type>      &&
      can_rollback_to_savepoint<_Type> );

// is_connectable
//   tagless trait: true if _Type satisfies the basic connection
// interface (connect + disconnect + is_connected + execute_query +
// execute_update).
template<typename _Type,
         typename = void>
constexpr bool is_connectable = false;

template<typename _Type>
constexpr bool is_connectable<_Type, std::void_t<
    connect_t<_Type>,
    disconnect_t<_Type>,
    is_connected_t<_Type>,
    execute_query_t<_Type>,
    execute_update_t<_Type>>> = true;

// is_full_vendor
//   tagless trait: true if _Type satisfies the full vendor connection
// interface (connectable + ping + metadata + errors).
template<typename _Type>
constexpr bool is_full_vendor =
    ( is_connectable<_Type>      &&
      can_ping<_Type>            &&
      does_report_errors<_Type>  &&
      does_report_version<_Type> );

// -------------------------------------------------------------------------
// E.  result set and statement tags
// -------------------------------------------------------------------------

// is_navigable_result
//   tagless trait: true if _Type satisfies the result set interface
// (next + column_count + column_name + get_value).
template<typename _Type,
         typename = void>
constexpr bool is_navigable_result = false;

template<typename _Type>
constexpr bool is_navigable_result<_Type, std::void_t<
    next_t<_Type>,
    column_count_t<_Type>,
    column_name_t<_Type>,
    get_value_index_t<_Type>>> = true;

// is_bindable_statement
//   tagless trait: true if _Type satisfies the statement interface
// (bind_int + bind_string + execute + parameter_count).
template<typename _Type,
         typename = void>
constexpr bool is_bindable_statement = false;

template<typename _Type>
constexpr bool is_bindable_statement<_Type, std::void_t<
    bind_int_t<_Type>,
    bind_string_t<_Type>,
    execute_t<_Type>,
    parameter_count_t<_Type>>> = true;


// =============================================================================
// XI.   RESULT TYPE EXTRACTION
// =============================================================================

// result_set_type
//   trait: extracts the result_set type from a connection
// implementation's execute_query return type.
template<typename _Connection>
struct result_set_type
{
    using type = typename std::decay<
        decltype(std::declval<_Connection&>().execute_query(
            std::declval<const std::string&>()))>::type::element_type;
};

// result_set_type_t
//   type: convenience alias for result_set_type<_Connection>::type.
template<typename _Connection>
using result_set_type_t = typename result_set_type<_Connection>::type;

// statement_type
//   trait: extracts the statement type from a connection
// implementation's prepare return type.
template<typename _Connection>
struct statement_type
{
    using type = typename std::decay<
        decltype(std::declval<_Connection&>().prepare(
            std::declval<const std::string&>()))>::type::element_type;
};

// statement_type_t
//   type: convenience alias for statement_type<_Connection>::type.
template<typename _Connection>
using statement_type_t = typename statement_type<_Connection>::type;


// =============================================================================
// XII.  CONCEPT-LIKE CONSTRAINTS (C++11+ via enable_if)
// =============================================================================

// enable_if_connection
//   type: SFINAE helper for connection constraints.
template<typename _Type>
using enable_if_connection =
    typename std::enable_if<is_connection<_Type>::value>::type;

// enable_if_result_set
//   type: SFINAE helper for result_set constraints.
template<typename _Type>
using enable_if_result_set =
    typename std::enable_if<is_result_set<_Type>::value>::type;

// enable_if_statement
//   type: SFINAE helper for statement constraints.
template<typename _Type>
using enable_if_statement =
    typename std::enable_if<is_statement<_Type>::value>::type;

// enable_if_has_transactions
//   type: SFINAE helper for transaction support constraints.
template<typename _Type>
using enable_if_has_transactions =
    typename std::enable_if<has_transactions<_Type>::value>::type;

// enable_if_has_savepoints
//   type: SFINAE helper for savepoint support constraints.
template<typename _Type>
using enable_if_has_savepoints =
    typename std::enable_if<has_savepoints<_Type>::value>::type;

// enable_if_vendor_connection
//   type: SFINAE helper for vendor connection constraints.
template<typename _Type>
using enable_if_vendor_connection =
    typename std::enable_if<is_vendor_connection<_Type>::value>::type;

// enable_if_has_ping
//   type: SFINAE helper for ping support constraints.
template<typename _Type>
using enable_if_has_ping =
    typename std::enable_if<has_ping<_Type>::value>::type;

// enable_if_has_native_handle
//   type: SFINAE helper for native handle constraints.
template<typename _Type>
using enable_if_has_native_handle =
    typename std::enable_if<has_native_handle<_Type>::value>::type;

// enable_if_has_reconnect
//   type: SFINAE helper for reconnection support constraints.
template<typename _Type>
using enable_if_has_reconnect =
    typename std::enable_if<has_reconnect<_Type>::value>::type;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_TRAITS_
