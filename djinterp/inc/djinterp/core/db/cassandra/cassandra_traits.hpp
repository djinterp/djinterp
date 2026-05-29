/******************************************************************************
* djinterp [database]                                     cassandra_traits.hpp
*
* djinterp Apache Cassandra traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to Apache Cassandra connections via the
* DataStax C/C++ driver (or equivalent), including:
*   - CQL execution (execute_cql, prepare_cql, execute_prepared, ping)
*   - asynchronous execution (async_execute, async_prepare)
*   - batch operations (batch_start, batch_add, batch_execute,
*     batch_discard) covering LOGGED / UNLOGGED / COUNTER batches
*   - keyspace management (create_keyspace, drop_keyspace,
*     use_keyspace, list_keyspaces)
*   - table management / schema (create_table, drop_table, alter_table,
*     describe_table, list_tables)
*   - data operations (insert_row, select_rows, update_row, delete_row,
*     row_exists)
*   - lightweight transactions / Paxos (insert_if_not_exists,
*     update_if, delete_if)
*   - user-defined types (create_type, drop_type, alter_type)
*   - materialized views (create_materialized_view,
*     drop_materialized_view)
*   - secondary indexes (create_index, drop_index)
*   - consistency level and request tracing (set_consistency,
*     get_consistency, set_tracing, get_last_trace)
*   - cluster topology (cluster_name, partitioner, local_node, peers)
*   - user-defined functions / aggregates (create_function,
*     drop_function, create_aggregate, drop_aggregate)
*   - paged result iteration (execute_paged, fetch_next_page)
*
*   Cassandra is a wide-column / partitioned-row store with a SQL-like
* query language (CQL). Unlike SQL back-ends it has no JOIN support, no
* OFFSET clause (paging proceeds via an opaque paging state token), and
* its schema introspection lives under `system_schema.*` rather than
* `INFORMATION_SCHEMA`. The traits below target C++ wrapper methods that
* a concrete cassandra_connection implementation would expose around the
* underlying DataStax C driver (CassSession, CassStatement, CassResult).
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17.
*
*   NAMING CONVENTION:
*   expression detectors:    cassandra_<method>_t
*   struct-based traits:     has_cassandra_<capability>
*   variable template _v:    has_cassandra_<capability>_v
*   tagless traits:          cassandra_can_<action>
*   compound tagless traits: cassandra_does_<category>
*
*
* path:      /inc/djinterp/core/db/cassandra/cassandra_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_CASSANDRA_TRAITS_
#define DJINTERP_DATABASE_CASSANDRA_TRAITS_

// std
#include <cstdint>
#include <map>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../database.hpp"
#include "../database_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  CQL execution
// -------------------------------------------------------------------------

// cassandra_execute_cql_t
//   detector: execute_cql(const std::string&) method.
// wraps cass_session_execute() with a simple statement.
template<typename _Type>
using cassandra_execute_cql_t =
    decltype(std::declval<_Type&>().execute_cql(
        std::declval<const std::string&>()));

// cassandra_prepare_cql_t
//   detector: prepare_cql(const std::string&) method.
// wraps cass_session_prepare() — returns a handle to a prepared
// statement.
template<typename _Type>
using cassandra_prepare_cql_t =
    decltype(std::declval<_Type&>().prepare_cql(
        std::declval<const std::string&>()));

// cassandra_execute_prepared_t
//   detector: execute_prepared(query_id, params) method.
// executes a previously-prepared statement with a named parameter map.
template<typename _Type>
using cassandra_execute_prepared_t =
    decltype(std::declval<_Type&>().execute_prepared(
        std::declval<const std::string&>(),
        std::declval<const parameter_map&>()));

// cassandra_ping_t
//   detector: ping() const method.
// issues a no-op statement against system.local to verify liveness.
template<typename _Type>
using cassandra_ping_t =
    decltype(std::declval<const _Type&>().ping());


// -------------------------------------------------------------------------
// B.  asynchronous execution
// -------------------------------------------------------------------------

// cassandra_async_execute_t
//   detector: async_execute(const std::string&) method.
// wraps cass_session_execute() returning a future-like handle.
template<typename _Type>
using cassandra_async_execute_t =
    decltype(std::declval<_Type&>().async_execute(
        std::declval<const std::string&>()));

// cassandra_async_prepare_t
//   detector: async_prepare(const std::string&) method.
// wraps cass_session_prepare() returning a future-like handle.
template<typename _Type>
using cassandra_async_prepare_t =
    decltype(std::declval<_Type&>().async_prepare(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// C.  batch operations
// -------------------------------------------------------------------------

// cassandra_batch_start_t
//   detector: batch_start(int) method.
// begins a batch (LOGGED / UNLOGGED / COUNTER).
template<typename _Type>
using cassandra_batch_start_t =
    decltype(std::declval<_Type&>().batch_start(
        std::declval<int>()));

// cassandra_batch_add_t
//   detector: batch_add(const std::string&) method.
// appends a CQL statement to the pending batch.
template<typename _Type>
using cassandra_batch_add_t =
    decltype(std::declval<_Type&>().batch_add(
        std::declval<const std::string&>()));

// cassandra_batch_execute_t
//   detector: batch_execute() method.
// dispatches the pending batch atomically (LOGGED) or as a best-effort
// group (UNLOGGED).
template<typename _Type>
using cassandra_batch_execute_t =
    decltype(std::declval<_Type&>().batch_execute());

// cassandra_batch_discard_t
//   detector: batch_discard() method.
// drops the pending batch without dispatching.
template<typename _Type>
using cassandra_batch_discard_t =
    decltype(std::declval<_Type&>().batch_discard());


// -------------------------------------------------------------------------
// D.  keyspace management
// -------------------------------------------------------------------------

// cassandra_create_keyspace_t
//   detector: create_keyspace(name, replication_spec) method.
// CREATE KEYSPACE — replication_spec is a CQL fragment such as
// {'class':'NetworkTopologyStrategy','dc1':3}.
template<typename _Type>
using cassandra_create_keyspace_t =
    decltype(std::declval<_Type&>().create_keyspace(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_keyspace_t
//   detector: drop_keyspace(name) method.
// DROP KEYSPACE.
template<typename _Type>
using cassandra_drop_keyspace_t =
    decltype(std::declval<_Type&>().drop_keyspace(
        std::declval<const std::string&>()));

// cassandra_use_keyspace_t
//   detector: use_keyspace(name) method.
// USE <keyspace> — sets the session's default keyspace.
template<typename _Type>
using cassandra_use_keyspace_t =
    decltype(std::declval<_Type&>().use_keyspace(
        std::declval<const std::string&>()));

// cassandra_list_keyspaces_t
//   detector: list_keyspaces() const method.
// queries system_schema.keyspaces.
template<typename _Type>
using cassandra_list_keyspaces_t =
    decltype(std::declval<const _Type&>().list_keyspaces());


// -------------------------------------------------------------------------
// E.  table management (schema)
// -------------------------------------------------------------------------

// cassandra_create_table_t
//   detector: create_table(keyspace, table, schema_spec) method.
// CREATE TABLE — schema_spec is a CQL column-definition fragment.
template<typename _Type>
using cassandra_create_table_t =
    decltype(std::declval<_Type&>().create_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_table_t
//   detector: drop_table(keyspace, table) method.
// DROP TABLE.
template<typename _Type>
using cassandra_drop_table_t =
    decltype(std::declval<_Type&>().drop_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_alter_table_t
//   detector: alter_table(keyspace, table, alter_spec) method.
// ALTER TABLE — adds/drops/renames columns or changes options.
template<typename _Type>
using cassandra_alter_table_t =
    decltype(std::declval<_Type&>().alter_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_describe_table_t
//   detector: describe_table(keyspace, table) const method.
// queries system_schema.tables + system_schema.columns.
template<typename _Type>
using cassandra_describe_table_t =
    decltype(std::declval<const _Type&>().describe_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_list_tables_t
//   detector: list_tables(keyspace) const method.
// queries system_schema.tables for a keyspace.
template<typename _Type>
using cassandra_list_tables_t =
    decltype(std::declval<const _Type&>().list_tables(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// F.  data operations (CRUD)
// -------------------------------------------------------------------------

// cassandra_insert_row_t
//   detector: insert_row(keyspace, table, row) method.
// builds and executes an INSERT INTO from a row map.
template<typename _Type>
using cassandra_insert_row_t =
    decltype(std::declval<_Type&>().insert_row(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>()));

// cassandra_select_rows_t
//   detector: select_rows(cql) const method.
// executes a SELECT and returns the result rows.
template<typename _Type>
using cassandra_select_rows_t =
    decltype(std::declval<const _Type&>().select_rows(
        std::declval<const std::string&>()));

// cassandra_update_row_t
//   detector: update_row(keyspace, table, key, updates) method.
// builds and executes an UPDATE.
template<typename _Type>
using cassandra_update_row_t =
    decltype(std::declval<_Type&>().update_row(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>(),
        std::declval<const row&>()));

// cassandra_delete_row_t
//   detector: delete_row(keyspace, table, key) method.
// builds and executes a DELETE.
template<typename _Type>
using cassandra_delete_row_t =
    decltype(std::declval<_Type&>().delete_row(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>()));

// cassandra_row_exists_t
//   detector: row_exists(keyspace, table, key) const method.
// SELECT COUNT(*) WHERE <key> — note Cassandra COUNTs are bounded.
template<typename _Type>
using cassandra_row_exists_t =
    decltype(std::declval<const _Type&>().row_exists(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>()));


// -------------------------------------------------------------------------
// G.  lightweight transactions (LWT / Paxos)
// -------------------------------------------------------------------------

// cassandra_insert_if_not_exists_t
//   detector: insert_if_not_exists(keyspace, table, row) method.
// INSERT ... IF NOT EXISTS (Paxos-coordinated).
template<typename _Type>
using cassandra_insert_if_not_exists_t =
    decltype(std::declval<_Type&>().insert_if_not_exists(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>()));

// cassandra_update_if_t
//   detector: update_if(keyspace, table, key, updates, condition) method.
// UPDATE ... IF <condition>.
template<typename _Type>
using cassandra_update_if_t =
    decltype(std::declval<_Type&>().update_if(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>(),
        std::declval<const row&>(),
        std::declval<const std::string&>()));

// cassandra_delete_if_t
//   detector: delete_if(keyspace, table, key, condition) method.
// DELETE ... IF <condition>.
template<typename _Type>
using cassandra_delete_if_t =
    decltype(std::declval<_Type&>().delete_if(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const row&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// H.  user-defined types (UDT)
// -------------------------------------------------------------------------

// cassandra_create_type_t
//   detector: create_type(keyspace, type_def) method.
// CREATE TYPE.
template<typename _Type>
using cassandra_create_type_t =
    decltype(std::declval<_Type&>().create_type(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_type_t
//   detector: drop_type(keyspace, type_name) method.
// DROP TYPE.
template<typename _Type>
using cassandra_drop_type_t =
    decltype(std::declval<_Type&>().drop_type(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_alter_type_t
//   detector: alter_type(keyspace, type_name, alter_spec) method.
// ALTER TYPE.
template<typename _Type>
using cassandra_alter_type_t =
    decltype(std::declval<_Type&>().alter_type(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// I.  materialized views
// -------------------------------------------------------------------------

// cassandra_create_mv_t
//   detector: create_materialized_view(keyspace, view, base_table,
// definition) method. CREATE MATERIALIZED VIEW.
template<typename _Type>
using cassandra_create_mv_t =
    decltype(std::declval<_Type&>().create_materialized_view(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_mv_t
//   detector: drop_materialized_view(keyspace, view) method.
// DROP MATERIALIZED VIEW.
template<typename _Type>
using cassandra_drop_mv_t =
    decltype(std::declval<_Type&>().drop_materialized_view(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// J.  secondary indexes
// -------------------------------------------------------------------------

// cassandra_create_index_t
//   detector: create_index(keyspace, table, index_name, column) method.
// CREATE INDEX.
template<typename _Type>
using cassandra_create_index_t =
    decltype(std::declval<_Type&>().create_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_index_t
//   detector: drop_index(keyspace, index_name) method.
// DROP INDEX.
template<typename _Type>
using cassandra_drop_index_t =
    decltype(std::declval<_Type&>().drop_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// K.  consistency level and tracing
// -------------------------------------------------------------------------

// cassandra_set_consistency_t
//   detector: set_consistency(int) method.
// sets the request consistency level (ANY / ONE / QUORUM /
// LOCAL_QUORUM / ALL / etc.).
template<typename _Type>
using cassandra_set_consistency_t =
    decltype(std::declval<_Type&>().set_consistency(
        std::declval<int>()));

// cassandra_get_consistency_t
//   detector: get_consistency() const method.
// returns the current consistency level.
template<typename _Type>
using cassandra_get_consistency_t =
    decltype(std::declval<const _Type&>().get_consistency());

// cassandra_set_tracing_t
//   detector: set_tracing(bool) method.
// enables/disables request tracing for subsequent statements.
template<typename _Type>
using cassandra_set_tracing_t =
    decltype(std::declval<_Type&>().set_tracing(
        std::declval<bool>()));

// cassandra_get_trace_t
//   detector: get_last_trace() const method.
// returns the trace events for the most recent traced statement.
template<typename _Type>
using cassandra_get_trace_t =
    decltype(std::declval<const _Type&>().get_last_trace());


// -------------------------------------------------------------------------
// L.  cluster topology
// -------------------------------------------------------------------------

// cassandra_cluster_name_t
//   detector: cluster_name() const method.
// queries system.local for the cluster name.
template<typename _Type>
using cassandra_cluster_name_t =
    decltype(std::declval<const _Type&>().cluster_name());

// cassandra_partitioner_t
//   detector: partitioner() const method.
// returns the configured partitioner (Murmur3Partitioner by default).
template<typename _Type>
using cassandra_partitioner_t =
    decltype(std::declval<const _Type&>().partitioner());

// cassandra_local_node_t
//   detector: local_node() const method.
// returns details of the coordinator node from system.local.
template<typename _Type>
using cassandra_local_node_t =
    decltype(std::declval<const _Type&>().local_node());

// cassandra_peers_t
//   detector: peers() const method.
// returns the peer nodes from system.peers.
template<typename _Type>
using cassandra_peers_t =
    decltype(std::declval<const _Type&>().peers());


// -------------------------------------------------------------------------
// M.  user-defined functions and aggregates (UDF / UDA)
// -------------------------------------------------------------------------

// cassandra_create_function_t
//   detector: create_function(keyspace, function_def) method.
// CREATE FUNCTION.
template<typename _Type>
using cassandra_create_function_t =
    decltype(std::declval<_Type&>().create_function(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_function_t
//   detector: drop_function(keyspace, function_name) method.
// DROP FUNCTION.
template<typename _Type>
using cassandra_drop_function_t =
    decltype(std::declval<_Type&>().drop_function(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_create_aggregate_t
//   detector: create_aggregate(keyspace, aggregate_def) method.
// CREATE AGGREGATE.
template<typename _Type>
using cassandra_create_aggregate_t =
    decltype(std::declval<_Type&>().create_aggregate(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// cassandra_drop_aggregate_t
//   detector: drop_aggregate(keyspace, aggregate_name) method.
// DROP AGGREGATE.
template<typename _Type>
using cassandra_drop_aggregate_t =
    decltype(std::declval<_Type&>().drop_aggregate(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// N.  paging
// -------------------------------------------------------------------------

// cassandra_execute_paged_t
//   detector: execute_paged(cql, page_size) const method.
// executes a SELECT with a paging-size hint; returns the first page
// plus an opaque paging state.
template<typename _Type>
using cassandra_execute_paged_t =
    decltype(std::declval<const _Type&>().execute_paged(
        std::declval<const std::string&>(),
        std::declval<int>()));

// cassandra_fetch_next_page_t
//   detector: fetch_next_page(paging_state) method.
// continues a paged result by submitting a previously-returned paging
// state.
template<typename _Type>
using cassandra_fetch_next_page_t =
    decltype(std::declval<_Type&>().fetch_next_page(
        std::declval<const std::string&>()));


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_cassandra_cql_execution
//   trait: checks if type _Type supports core CQL execution
// (execute_cql + prepare_cql + execute_prepared).
template<typename _Type>
struct has_cassandra_cql_execution : djinterp::conjunction<
    is_detected<cassandra_execute_cql_t, _Type>,
    is_detected<cassandra_prepare_cql_t, _Type>,
    is_detected<cassandra_execute_prepared_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_cql_execution_v =
        has_cassandra_cql_execution<_Type>::value;
#endif

// has_cassandra_async
//   trait: checks if type _Type supports asynchronous execution
// (async_execute + async_prepare).
template<typename _Type>
struct has_cassandra_async : djinterp::conjunction<
    is_detected<cassandra_async_execute_t, _Type>,
    is_detected<cassandra_async_prepare_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_async_v =
        has_cassandra_async<_Type>::value;
#endif

// has_cassandra_batch
//   trait: checks if type _Type supports batch operations
// (batch_start + batch_add + batch_execute + batch_discard).
template<typename _Type>
struct has_cassandra_batch : djinterp::conjunction<
    is_detected<cassandra_batch_start_t, _Type>,
    is_detected<cassandra_batch_add_t, _Type>,
    is_detected<cassandra_batch_execute_t, _Type>,
    is_detected<cassandra_batch_discard_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_batch_v =
        has_cassandra_batch<_Type>::value;
#endif

// has_cassandra_keyspace_management
//   trait: checks if type _Type supports keyspace management
// (create_keyspace + drop_keyspace + use_keyspace + list_keyspaces).
template<typename _Type>
struct has_cassandra_keyspace_management : djinterp::conjunction<
    is_detected<cassandra_create_keyspace_t, _Type>,
    is_detected<cassandra_drop_keyspace_t, _Type>,
    is_detected<cassandra_use_keyspace_t, _Type>,
    is_detected<cassandra_list_keyspaces_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_keyspace_management_v =
        has_cassandra_keyspace_management<_Type>::value;
#endif

// has_cassandra_table_management
//   trait: checks if type _Type supports table management
// (create_table + drop_table + alter_table + describe_table +
// list_tables).
template<typename _Type>
struct has_cassandra_table_management : djinterp::conjunction<
    is_detected<cassandra_create_table_t, _Type>,
    is_detected<cassandra_drop_table_t, _Type>,
    is_detected<cassandra_alter_table_t, _Type>,
    is_detected<cassandra_describe_table_t, _Type>,
    is_detected<cassandra_list_tables_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_table_management_v =
        has_cassandra_table_management<_Type>::value;
#endif

// has_cassandra_data_ops
//   trait: checks if type _Type supports core data operations
// (insert_row + select_rows + update_row + delete_row + row_exists).
template<typename _Type>
struct has_cassandra_data_ops : djinterp::conjunction<
    is_detected<cassandra_insert_row_t, _Type>,
    is_detected<cassandra_select_rows_t, _Type>,
    is_detected<cassandra_update_row_t, _Type>,
    is_detected<cassandra_delete_row_t, _Type>,
    is_detected<cassandra_row_exists_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_data_ops_v =
        has_cassandra_data_ops<_Type>::value;
#endif

// has_cassandra_lwt
//   trait: checks if type _Type supports lightweight transactions
// (insert_if_not_exists + update_if + delete_if).
template<typename _Type>
struct has_cassandra_lwt : djinterp::conjunction<
    is_detected<cassandra_insert_if_not_exists_t, _Type>,
    is_detected<cassandra_update_if_t, _Type>,
    is_detected<cassandra_delete_if_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_lwt_v =
        has_cassandra_lwt<_Type>::value;
#endif

// has_cassandra_udt
//   trait: checks if type _Type supports user-defined types
// (create_type + drop_type + alter_type).
template<typename _Type>
struct has_cassandra_udt : djinterp::conjunction<
    is_detected<cassandra_create_type_t, _Type>,
    is_detected<cassandra_drop_type_t, _Type>,
    is_detected<cassandra_alter_type_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_udt_v =
        has_cassandra_udt<_Type>::value;
#endif

// has_cassandra_materialized_views
//   trait: checks if type _Type supports materialized views
// (create_materialized_view + drop_materialized_view).
template<typename _Type>
struct has_cassandra_materialized_views : djinterp::conjunction<
    is_detected<cassandra_create_mv_t, _Type>,
    is_detected<cassandra_drop_mv_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_materialized_views_v =
        has_cassandra_materialized_views<_Type>::value;
#endif

// has_cassandra_secondary_indexes
//   trait: checks if type _Type supports secondary indexes
// (create_index + drop_index).
template<typename _Type>
struct has_cassandra_secondary_indexes : djinterp::conjunction<
    is_detected<cassandra_create_index_t, _Type>,
    is_detected<cassandra_drop_index_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_secondary_indexes_v =
        has_cassandra_secondary_indexes<_Type>::value;
#endif

// has_cassandra_consistency
//   trait: checks if type _Type supports consistency level control
// (set_consistency + get_consistency).
template<typename _Type>
struct has_cassandra_consistency : djinterp::conjunction<
    is_detected<cassandra_set_consistency_t, _Type>,
    is_detected<cassandra_get_consistency_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_consistency_v =
        has_cassandra_consistency<_Type>::value;
#endif

// has_cassandra_tracing
//   trait: checks if type _Type supports request tracing
// (set_tracing + get_last_trace).
template<typename _Type>
struct has_cassandra_tracing : djinterp::conjunction<
    is_detected<cassandra_set_tracing_t, _Type>,
    is_detected<cassandra_get_trace_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_tracing_v =
        has_cassandra_tracing<_Type>::value;
#endif

// has_cassandra_topology
//   trait: checks if type _Type supports cluster topology queries
// (cluster_name + partitioner + local_node + peers).
template<typename _Type>
struct has_cassandra_topology : djinterp::conjunction<
    is_detected<cassandra_cluster_name_t, _Type>,
    is_detected<cassandra_partitioner_t, _Type>,
    is_detected<cassandra_local_node_t, _Type>,
    is_detected<cassandra_peers_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_topology_v =
        has_cassandra_topology<_Type>::value;
#endif

// has_cassandra_udf
//   trait: checks if type _Type supports user-defined functions and
// aggregates (create_function + drop_function + create_aggregate +
// drop_aggregate).
template<typename _Type>
struct has_cassandra_udf : djinterp::conjunction<
    is_detected<cassandra_create_function_t, _Type>,
    is_detected<cassandra_drop_function_t, _Type>,
    is_detected<cassandra_create_aggregate_t, _Type>,
    is_detected<cassandra_drop_aggregate_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_udf_v =
        has_cassandra_udf<_Type>::value;
#endif

// has_cassandra_paging
//   trait: checks if type _Type supports paged execution
// (execute_paged + fetch_next_page).
template<typename _Type>
struct has_cassandra_paging : djinterp::conjunction<
    is_detected<cassandra_execute_paged_t, _Type>,
    is_detected<cassandra_fetch_next_page_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_cassandra_paging_v =
        has_cassandra_paging<_Type>::value;
#endif

// is_cassandra_connection
//   trait: compound trait verifying type _Type implements a Cassandra
// connection interface (CQL execution + data ops + table management +
// topology).
template<typename _Type>
struct is_cassandra_connection : djinterp::conjunction<
    has_cassandra_cql_execution<_Type>,
    has_cassandra_data_ops<_Type>,
    has_cassandra_table_management<_Type>,
    has_cassandra_topology<_Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_cassandra_connection_v =
        is_cassandra_connection<_Type>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// cassandra_can_execute_cql
//   tagless trait: true if _Type has execute_cql().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_execute_cql = false;

template<typename _Type>
constexpr bool cassandra_can_execute_cql<_Type,
    std::void_t<cassandra_execute_cql_t<_Type>>> = true;

// cassandra_can_prepare
//   tagless trait: true if _Type has prepare_cql().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_prepare = false;

template<typename _Type>
constexpr bool cassandra_can_prepare<_Type,
    std::void_t<cassandra_prepare_cql_t<_Type>>> = true;

// cassandra_can_execute_prepared
//   tagless trait: true if _Type has execute_prepared().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_execute_prepared = false;

template<typename _Type>
constexpr bool cassandra_can_execute_prepared<_Type,
    std::void_t<cassandra_execute_prepared_t<_Type>>> = true;

// cassandra_can_async_execute
//   tagless trait: true if _Type has async_execute().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_async_execute = false;

template<typename _Type>
constexpr bool cassandra_can_async_execute<_Type,
    std::void_t<cassandra_async_execute_t<_Type>>> = true;

// cassandra_can_batch
//   tagless trait: true if _Type has batch_start().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_batch = false;

template<typename _Type>
constexpr bool cassandra_can_batch<_Type,
    std::void_t<cassandra_batch_start_t<_Type>>> = true;

// cassandra_can_create_keyspace
//   tagless trait: true if _Type has create_keyspace().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_create_keyspace = false;

template<typename _Type>
constexpr bool cassandra_can_create_keyspace<_Type,
    std::void_t<cassandra_create_keyspace_t<_Type>>> = true;

// cassandra_can_use_keyspace
//   tagless trait: true if _Type has use_keyspace().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_use_keyspace = false;

template<typename _Type>
constexpr bool cassandra_can_use_keyspace<_Type,
    std::void_t<cassandra_use_keyspace_t<_Type>>> = true;

// cassandra_can_create_table
//   tagless trait: true if _Type has create_table().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_create_table = false;

template<typename _Type>
constexpr bool cassandra_can_create_table<_Type,
    std::void_t<cassandra_create_table_t<_Type>>> = true;

// cassandra_can_insert_row
//   tagless trait: true if _Type has insert_row().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_insert_row = false;

template<typename _Type>
constexpr bool cassandra_can_insert_row<_Type,
    std::void_t<cassandra_insert_row_t<_Type>>> = true;

// cassandra_can_select_rows
//   tagless trait: true if _Type has select_rows().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_select_rows = false;

template<typename _Type>
constexpr bool cassandra_can_select_rows<_Type,
    std::void_t<cassandra_select_rows_t<_Type>>> = true;

// cassandra_can_lwt_insert
//   tagless trait: true if _Type has insert_if_not_exists().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_lwt_insert = false;

template<typename _Type>
constexpr bool cassandra_can_lwt_insert<_Type,
    std::void_t<cassandra_insert_if_not_exists_t<_Type>>> = true;

// cassandra_can_create_type
//   tagless trait: true if _Type has create_type().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_create_type = false;

template<typename _Type>
constexpr bool cassandra_can_create_type<_Type,
    std::void_t<cassandra_create_type_t<_Type>>> = true;

// cassandra_can_create_mv
//   tagless trait: true if _Type has create_materialized_view().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_create_mv = false;

template<typename _Type>
constexpr bool cassandra_can_create_mv<_Type,
    std::void_t<cassandra_create_mv_t<_Type>>> = true;

// cassandra_can_create_index
//   tagless trait: true if _Type has create_index().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_create_index = false;

template<typename _Type>
constexpr bool cassandra_can_create_index<_Type,
    std::void_t<cassandra_create_index_t<_Type>>> = true;

// cassandra_can_set_consistency
//   tagless trait: true if _Type has set_consistency().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_set_consistency = false;

template<typename _Type>
constexpr bool cassandra_can_set_consistency<_Type,
    std::void_t<cassandra_set_consistency_t<_Type>>> = true;

// cassandra_can_trace
//   tagless trait: true if _Type has set_tracing().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_trace = false;

template<typename _Type>
constexpr bool cassandra_can_trace<_Type,
    std::void_t<cassandra_set_tracing_t<_Type>>> = true;

// cassandra_can_topology_query
//   tagless trait: true if _Type has cluster_name().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_topology_query = false;

template<typename _Type>
constexpr bool cassandra_can_topology_query<_Type,
    std::void_t<cassandra_cluster_name_t<_Type>>> = true;

// cassandra_can_create_function
//   tagless trait: true if _Type has create_function().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_create_function = false;

template<typename _Type>
constexpr bool cassandra_can_create_function<_Type,
    std::void_t<cassandra_create_function_t<_Type>>> = true;

// cassandra_can_page
//   tagless trait: true if _Type has execute_paged().
template<typename _Type,
         typename = void>
constexpr bool cassandra_can_page = false;

template<typename _Type>
constexpr bool cassandra_can_page<_Type,
    std::void_t<cassandra_execute_paged_t<_Type>>> = true;


// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// cassandra_does_cql_execution
//   tagless trait: true if _Type supports the full CQL-execution
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_cql_execution = false;

template<typename _Type>
constexpr bool cassandra_does_cql_execution<_Type, std::void_t<
    cassandra_execute_cql_t<_Type>,
    cassandra_prepare_cql_t<_Type>,
    cassandra_execute_prepared_t<_Type>>> = true;

// cassandra_does_async
//   tagless trait: true if _Type supports the full asynchronous
// execution surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_async = false;

template<typename _Type>
constexpr bool cassandra_does_async<_Type, std::void_t<
    cassandra_async_execute_t<_Type>,
    cassandra_async_prepare_t<_Type>>> = true;

// cassandra_does_batch
//   tagless trait: true if _Type supports the full batch surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_batch = false;

template<typename _Type>
constexpr bool cassandra_does_batch<_Type, std::void_t<
    cassandra_batch_start_t<_Type>,
    cassandra_batch_add_t<_Type>,
    cassandra_batch_execute_t<_Type>,
    cassandra_batch_discard_t<_Type>>> = true;

// cassandra_does_keyspace_management
//   tagless trait: true if _Type supports the full keyspace-management
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_keyspace_management = false;

template<typename _Type>
constexpr bool cassandra_does_keyspace_management<_Type, std::void_t<
    cassandra_create_keyspace_t<_Type>,
    cassandra_drop_keyspace_t<_Type>,
    cassandra_use_keyspace_t<_Type>,
    cassandra_list_keyspaces_t<_Type>>> = true;

// cassandra_does_table_management
//   tagless trait: true if _Type supports the full table-management
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_table_management = false;

template<typename _Type>
constexpr bool cassandra_does_table_management<_Type, std::void_t<
    cassandra_create_table_t<_Type>,
    cassandra_drop_table_t<_Type>,
    cassandra_alter_table_t<_Type>,
    cassandra_describe_table_t<_Type>,
    cassandra_list_tables_t<_Type>>> = true;

// cassandra_does_data_ops
//   tagless trait: true if _Type supports the full data-operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_data_ops = false;

template<typename _Type>
constexpr bool cassandra_does_data_ops<_Type, std::void_t<
    cassandra_insert_row_t<_Type>,
    cassandra_select_rows_t<_Type>,
    cassandra_update_row_t<_Type>,
    cassandra_delete_row_t<_Type>,
    cassandra_row_exists_t<_Type>>> = true;

// cassandra_does_lwt
//   tagless trait: true if _Type supports the full LWT surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_lwt = false;

template<typename _Type>
constexpr bool cassandra_does_lwt<_Type, std::void_t<
    cassandra_insert_if_not_exists_t<_Type>,
    cassandra_update_if_t<_Type>,
    cassandra_delete_if_t<_Type>>> = true;

// cassandra_does_udt
//   tagless trait: true if _Type supports the full UDT surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_udt = false;

template<typename _Type>
constexpr bool cassandra_does_udt<_Type, std::void_t<
    cassandra_create_type_t<_Type>,
    cassandra_drop_type_t<_Type>,
    cassandra_alter_type_t<_Type>>> = true;

// cassandra_does_materialized_views
//   tagless trait: true if _Type supports the full materialized-view
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_materialized_views = false;

template<typename _Type>
constexpr bool cassandra_does_materialized_views<_Type, std::void_t<
    cassandra_create_mv_t<_Type>,
    cassandra_drop_mv_t<_Type>>> = true;

// cassandra_does_secondary_indexes
//   tagless trait: true if _Type supports the full secondary-index
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_secondary_indexes = false;

template<typename _Type>
constexpr bool cassandra_does_secondary_indexes<_Type, std::void_t<
    cassandra_create_index_t<_Type>,
    cassandra_drop_index_t<_Type>>> = true;

// cassandra_does_consistency
//   tagless trait: true if _Type supports the full consistency
// control surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_consistency = false;

template<typename _Type>
constexpr bool cassandra_does_consistency<_Type, std::void_t<
    cassandra_set_consistency_t<_Type>,
    cassandra_get_consistency_t<_Type>>> = true;

// cassandra_does_tracing
//   tagless trait: true if _Type supports the full request-tracing
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_tracing = false;

template<typename _Type>
constexpr bool cassandra_does_tracing<_Type, std::void_t<
    cassandra_set_tracing_t<_Type>,
    cassandra_get_trace_t<_Type>>> = true;

// cassandra_does_topology
//   tagless trait: true if _Type supports the full cluster-topology
// surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_topology = false;

template<typename _Type>
constexpr bool cassandra_does_topology<_Type, std::void_t<
    cassandra_cluster_name_t<_Type>,
    cassandra_partitioner_t<_Type>,
    cassandra_local_node_t<_Type>,
    cassandra_peers_t<_Type>>> = true;

// cassandra_does_udf
//   tagless trait: true if _Type supports the full UDF/UDA surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_udf = false;

template<typename _Type>
constexpr bool cassandra_does_udf<_Type, std::void_t<
    cassandra_create_function_t<_Type>,
    cassandra_drop_function_t<_Type>,
    cassandra_create_aggregate_t<_Type>,
    cassandra_drop_aggregate_t<_Type>>> = true;

// cassandra_does_paging
//   tagless trait: true if _Type supports the full paging surface.
template<typename _Type,
         typename = void>
constexpr bool cassandra_does_paging = false;

template<typename _Type>
constexpr bool cassandra_does_paging<_Type, std::void_t<
    cassandra_execute_paged_t<_Type>,
    cassandra_fetch_next_page_t<_Type>>> = true;

// cassandra_is_full_connection
//   tagless trait: true if _Type satisfies the complete Cassandra
// connection interface (CQL execution + data ops + table management +
// topology + batch).
template<typename _Type>
constexpr bool cassandra_is_full_connection =
    ( cassandra_does_cql_execution<_Type>    &&
      cassandra_does_data_ops<_Type>         &&
      cassandra_does_table_management<_Type> &&
      cassandra_does_topology<_Type>         &&
      cassandra_does_batch<_Type> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

// enable_if_cassandra_connection
//   type: SFINAE helper for Cassandra connection constraints.
template<typename _Type>
using enable_if_cassandra_connection =
    typename std::enable_if<is_cassandra_connection<_Type>::value>::type;

// enable_if_has_cassandra_async
//   type: SFINAE helper for Cassandra asynchronous-execution
// constraints.
template<typename _Type>
using enable_if_has_cassandra_async =
    typename std::enable_if<has_cassandra_async<_Type>::value>::type;

// enable_if_has_cassandra_batch
//   type: SFINAE helper for Cassandra batch-operation constraints.
template<typename _Type>
using enable_if_has_cassandra_batch =
    typename std::enable_if<has_cassandra_batch<_Type>::value>::type;

// enable_if_has_cassandra_lwt
//   type: SFINAE helper for Cassandra LWT constraints.
template<typename _Type>
using enable_if_has_cassandra_lwt =
    typename std::enable_if<has_cassandra_lwt<_Type>::value>::type;

// enable_if_has_cassandra_paging
//   type: SFINAE helper for Cassandra paging constraints.
template<typename _Type>
using enable_if_has_cassandra_paging =
    typename std::enable_if<has_cassandra_paging<_Type>::value>::type;

// enable_if_has_cassandra_materialized_views
//   type: SFINAE helper for Cassandra materialized-view constraints.
template<typename _Type>
using enable_if_has_cassandra_materialized_views =
    typename std::enable_if<
        has_cassandra_materialized_views<_Type>::value>::type;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CASSANDRA_TRAITS_
