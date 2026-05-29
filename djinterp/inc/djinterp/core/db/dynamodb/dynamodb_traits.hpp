/******************************************************************************
* djinterp [database]                                      dynamodb_traits.hpp
*
* djinterp DynamoDB traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to Amazon DynamoDB connections via the
* AWS SDK for C++ (Aws::DynamoDB::DynamoDBClient) or an equivalent client,
* including:
*   - item operations (put_item, get_item, update_item, delete_item)
*   - batch operations (batch_get_item, batch_write_item)
*   - query and scan (query, scan)
*   - transactions (transact_write_items, transact_get_items)
*   - PartiQL (execute_statement, batch_execute_statement,
*     execute_transaction)
*   - conditional writes (put_item_conditional, delete_item_conditional)
*   - table management / control plane (create_table, delete_table,
*     describe_table, update_table, list_tables)
*   - secondary indexes (create / delete GSI, query_index)
*   - DynamoDB Streams (describe_stream, get_shard_iterator, get_records,
*     list_streams)
*   - TTL management (update_time_to_live, describe_time_to_live)
*   - backup / point-in-time recovery (create_backup,
*     restore_table_from_backup, describe_continuous_backups)
*   - diagnostics (describe_limits, describe_endpoints, table_status)
*   - global tables (create / describe / update global table)
*   - resource tagging (tag_resource, untag_resource,
*     list_tags_of_resource)
*
*   DynamoDB differs fundamentally from SQL back-ends: there is no SQL
* dialect (PartiQL aside), no INFORMATION_SCHEMA, no server-side joins.
* Access is through a request/response control- and data-plane API over
* tables of schemaless items keyed by a partition key plus an optional
* sort key. The traits below target C++ wrapper methods that a concrete
* dynamodb_connection implementation would expose around the underlying
* AWS SDK client.
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided. The detectors target C++ wrapper methods that a concrete
* dynamodb_connection implementation would expose around the AWS SDK.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17.
*
*   NAMING CONVENTION:
*   expression detectors:    dynamodb_<method>_t
*   struct-based traits:     has_dynamodb_<capability>
*   variable template _v:    has_dynamodb_<capability>_v
*   tagless traits:          dynamodb_can_<action>
*   compound tagless traits: dynamodb_does_<category>
*
*
* path:      /inc/djinterp/core/db/dynamodb/dynamodb_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_DYNAMODB_TRAITS_
#define DJINTERP_DATABASE_DYNAMODB_TRAITS_

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
// 0.   DYNAMODB ITEM / KEY TYPE ALIASES
// =============================================================================
// DynamoDB items are schemaless attribute maps. At this abstraction layer an
// attribute value is represented by the generic djinterp `value` variant
// (from database.hpp); the wire-level distinction between DynamoDB's S / N /
// B / BOOL / NULL / M / L / SS / NS / BS descriptors is reconstructed by the
// concrete connection's marshalling code in dynamodb.cpp.

// dynamodb_item
//   type: a DynamoDB item as a map of attribute name to value.
using dynamodb_item = std::map<std::string, value>;

// dynamodb_key
//   type: a DynamoDB primary key (partition key, plus optional sort key)
// as a map of attribute name to value.
using dynamodb_key = std::map<std::string, value>;


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  item operations
// -------------------------------------------------------------------------

// dynamodb_put_item_t
//   detector: put_item(table, item) method.
// wraps PutItem — writes (or replaces) a single item.
template<typename _Type>
using dynamodb_put_item_t =
    decltype(std::declval<_Type&>().put_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_item&>()));

// dynamodb_get_item_t
//   detector: get_item(table, key) const method.
// wraps GetItem — reads a single item by primary key.
template<typename _Type>
using dynamodb_get_item_t =
    decltype(std::declval<const _Type&>().get_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>()));

// dynamodb_update_item_t
//   detector: update_item(table, key, updates) method.
// wraps UpdateItem — mutates attributes of an existing item.
template<typename _Type>
using dynamodb_update_item_t =
    decltype(std::declval<_Type&>().update_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>(),
        std::declval<const dynamodb_item&>()));

// dynamodb_delete_item_t
//   detector: delete_item(table, key) method.
// wraps DeleteItem — removes a single item by primary key.
template<typename _Type>
using dynamodb_delete_item_t =
    decltype(std::declval<_Type&>().delete_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>()));


// -------------------------------------------------------------------------
// B.  batch operations
// -------------------------------------------------------------------------

// dynamodb_batch_get_item_t
//   detector: batch_get_item(table, keys) const method.
// wraps BatchGetItem — reads up to 100 items in one round-trip.
template<typename _Type>
using dynamodb_batch_get_item_t =
    decltype(std::declval<const _Type&>().batch_get_item(
        std::declval<const std::string&>(),
        std::declval<const std::vector<dynamodb_key>&>()));

// dynamodb_batch_write_item_t
//   detector: batch_write_item(table, items) method.
// wraps BatchWriteItem — writes/deletes up to 25 items in one
// round-trip.
template<typename _Type>
using dynamodb_batch_write_item_t =
    decltype(std::declval<_Type&>().batch_write_item(
        std::declval<const std::string&>(),
        std::declval<const std::vector<dynamodb_item>&>()));


// -------------------------------------------------------------------------
// C.  query and scan
// -------------------------------------------------------------------------

// dynamodb_query_t
//   detector: query(table, key_condition) const method.
// wraps Query — partition-key-bounded retrieval with an optional
// sort-key condition expression.
template<typename _Type>
using dynamodb_query_t =
    decltype(std::declval<const _Type&>().query(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_scan_t
//   detector: scan(table) const method.
// wraps Scan — full-table sequential read (expensive; prefer query).
template<typename _Type>
using dynamodb_scan_t =
    decltype(std::declval<const _Type&>().scan(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// D.  transactions
// -------------------------------------------------------------------------

// dynamodb_transact_write_items_t
//   detector: transact_write_items(items) method.
// wraps TransactWriteItems — all-or-nothing write of up to 100 items.
template<typename _Type>
using dynamodb_transact_write_items_t =
    decltype(std::declval<_Type&>().transact_write_items(
        std::declval<const std::vector<dynamodb_item>&>()));

// dynamodb_transact_get_items_t
//   detector: transact_get_items(keys) const method.
// wraps TransactGetItems — consistent snapshot read of up to 100 items.
template<typename _Type>
using dynamodb_transact_get_items_t =
    decltype(std::declval<const _Type&>().transact_get_items(
        std::declval<const std::vector<dynamodb_key>&>()));


// -------------------------------------------------------------------------
// E.  PartiQL
// -------------------------------------------------------------------------

// dynamodb_execute_statement_t
//   detector: execute_statement(statement) method.
// wraps ExecuteStatement — a single PartiQL statement.
template<typename _Type>
using dynamodb_execute_statement_t =
    decltype(std::declval<_Type&>().execute_statement(
        std::declval<const std::string&>()));

// dynamodb_batch_execute_statement_t
//   detector: batch_execute_statement(statements) method.
// wraps BatchExecuteStatement — multiple PartiQL statements.
template<typename _Type>
using dynamodb_batch_execute_statement_t =
    decltype(std::declval<_Type&>().batch_execute_statement(
        std::declval<const std::vector<std::string>&>()));

// dynamodb_execute_transaction_t
//   detector: execute_transaction(statements) method.
// wraps ExecuteTransaction — transactional PartiQL statements.
template<typename _Type>
using dynamodb_execute_transaction_t =
    decltype(std::declval<_Type&>().execute_transaction(
        std::declval<const std::vector<std::string>&>()));


// -------------------------------------------------------------------------
// F.  conditional writes
// -------------------------------------------------------------------------

// dynamodb_put_item_conditional_t
//   detector: put_item_conditional(table, item, condition) method.
// wraps PutItem with a ConditionExpression.
template<typename _Type>
using dynamodb_put_item_conditional_t =
    decltype(std::declval<_Type&>().put_item_conditional(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_item&>(),
        std::declval<const std::string&>()));

// dynamodb_delete_item_conditional_t
//   detector: delete_item_conditional(table, key, condition) method.
// wraps DeleteItem with a ConditionExpression.
template<typename _Type>
using dynamodb_delete_item_conditional_t =
    decltype(std::declval<_Type&>().delete_item_conditional(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// G.  table management (control plane)
// -------------------------------------------------------------------------

// dynamodb_create_table_t
//   detector: create_table(table, partition_key, sort_key) method.
// wraps CreateTable. An empty sort_key denotes a simple (partition-only)
// key schema.
template<typename _Type>
using dynamodb_create_table_t =
    decltype(std::declval<_Type&>().create_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_delete_table_t
//   detector: delete_table(table) method.
// wraps DeleteTable.
template<typename _Type>
using dynamodb_delete_table_t =
    decltype(std::declval<_Type&>().delete_table(
        std::declval<const std::string&>()));

// dynamodb_describe_table_t
//   detector: describe_table(table) const method.
// wraps DescribeTable.
template<typename _Type>
using dynamodb_describe_table_t =
    decltype(std::declval<const _Type&>().describe_table(
        std::declval<const std::string&>()));

// dynamodb_update_table_t
//   detector: update_table(table, spec) method.
// wraps UpdateTable — throughput / billing / index changes.
template<typename _Type>
using dynamodb_update_table_t =
    decltype(std::declval<_Type&>().update_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_list_tables_t
//   detector: list_tables() const method.
// wraps ListTables.
template<typename _Type>
using dynamodb_list_tables_t =
    decltype(std::declval<const _Type&>().list_tables());


// -------------------------------------------------------------------------
// H.  secondary indexes
// -------------------------------------------------------------------------

// dynamodb_create_gsi_t
//   detector: create_global_secondary_index(table, index, partition_key)
// method. wraps UpdateTable with a GSI create action.
template<typename _Type>
using dynamodb_create_gsi_t =
    decltype(std::declval<_Type&>().create_global_secondary_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_delete_gsi_t
//   detector: delete_global_secondary_index(table, index) method.
// wraps UpdateTable with a GSI delete action.
template<typename _Type>
using dynamodb_delete_gsi_t =
    decltype(std::declval<_Type&>().delete_global_secondary_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_query_index_t
//   detector: query_index(table, index, key_condition) const method.
// wraps Query against a secondary index.
template<typename _Type>
using dynamodb_query_index_t =
    decltype(std::declval<const _Type&>().query_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// I.  DynamoDB Streams
// -------------------------------------------------------------------------

// dynamodb_describe_stream_t
//   detector: describe_stream(stream_arn) const method.
// wraps DescribeStream.
template<typename _Type>
using dynamodb_describe_stream_t =
    decltype(std::declval<const _Type&>().describe_stream(
        std::declval<const std::string&>()));

// dynamodb_get_shard_iterator_t
//   detector: get_shard_iterator(stream_arn, shard_id) const method.
// wraps GetShardIterator.
template<typename _Type>
using dynamodb_get_shard_iterator_t =
    decltype(std::declval<const _Type&>().get_shard_iterator(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_get_records_t
//   detector: get_records(shard_iterator) const method.
// wraps GetRecords.
template<typename _Type>
using dynamodb_get_records_t =
    decltype(std::declval<const _Type&>().get_records(
        std::declval<const std::string&>()));

// dynamodb_list_streams_t
//   detector: list_streams(table) const method.
// wraps ListStreams.
template<typename _Type>
using dynamodb_list_streams_t =
    decltype(std::declval<const _Type&>().list_streams(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// J.  TTL management
// -------------------------------------------------------------------------

// dynamodb_update_ttl_t
//   detector: update_time_to_live(table, attribute, enabled) method.
// wraps UpdateTimeToLive.
template<typename _Type>
using dynamodb_update_ttl_t =
    decltype(std::declval<_Type&>().update_time_to_live(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<bool>()));

// dynamodb_describe_ttl_t
//   detector: describe_time_to_live(table) const method.
// wraps DescribeTimeToLive.
template<typename _Type>
using dynamodb_describe_ttl_t =
    decltype(std::declval<const _Type&>().describe_time_to_live(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// K.  backup / point-in-time recovery
// -------------------------------------------------------------------------

// dynamodb_create_backup_t
//   detector: create_backup(table, backup_name) method.
// wraps CreateBackup — on-demand backup.
template<typename _Type>
using dynamodb_create_backup_t =
    decltype(std::declval<_Type&>().create_backup(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_restore_from_backup_t
//   detector: restore_table_from_backup(table, backup_arn) method.
// wraps RestoreTableFromBackup.
template<typename _Type>
using dynamodb_restore_from_backup_t =
    decltype(std::declval<_Type&>().restore_table_from_backup(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_describe_continuous_backups_t
//   detector: describe_continuous_backups(table) const method.
// wraps DescribeContinuousBackups — point-in-time recovery status.
template<typename _Type>
using dynamodb_describe_continuous_backups_t =
    decltype(std::declval<const _Type&>().describe_continuous_backups(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// L.  diagnostics
// -------------------------------------------------------------------------

// dynamodb_describe_limits_t
//   detector: describe_limits() const method.
// wraps DescribeLimits — account/table capacity limits.
template<typename _Type>
using dynamodb_describe_limits_t =
    decltype(std::declval<const _Type&>().describe_limits());

// dynamodb_describe_endpoints_t
//   detector: describe_endpoints() const method.
// wraps DescribeEndpoints — regional endpoint discovery.
template<typename _Type>
using dynamodb_describe_endpoints_t =
    decltype(std::declval<const _Type&>().describe_endpoints());

// dynamodb_table_status_t
//   detector: table_status(table) const method.
// convenience accessor over DescribeTable returning the table state
// (CREATING / ACTIVE / UPDATING / DELETING).
template<typename _Type>
using dynamodb_table_status_t =
    decltype(std::declval<const _Type&>().table_status(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// M.  global tables
// -------------------------------------------------------------------------

// dynamodb_create_global_table_t
//   detector: create_global_table(table, regions) method.
// wraps CreateGlobalTable — multi-region replication.
template<typename _Type>
using dynamodb_create_global_table_t =
    decltype(std::declval<_Type&>().create_global_table(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));

// dynamodb_describe_global_table_t
//   detector: describe_global_table(table) const method.
// wraps DescribeGlobalTable.
template<typename _Type>
using dynamodb_describe_global_table_t =
    decltype(std::declval<const _Type&>().describe_global_table(
        std::declval<const std::string&>()));

// dynamodb_update_global_table_t
//   detector: update_global_table(table, regions) method.
// wraps UpdateGlobalTable — add/remove replica regions.
template<typename _Type>
using dynamodb_update_global_table_t =
    decltype(std::declval<_Type&>().update_global_table(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));


// -------------------------------------------------------------------------
// N.  resource tagging
// -------------------------------------------------------------------------

// dynamodb_tag_resource_t
//   detector: tag_resource(arn, tags) method.
// wraps TagResource.
template<typename _Type>
using dynamodb_tag_resource_t =
    decltype(std::declval<_Type&>().tag_resource(
        std::declval<const std::string&>(),
        std::declval<const std::map<std::string, std::string>&>()));

// dynamodb_untag_resource_t
//   detector: untag_resource(arn, keys) method.
// wraps UntagResource.
template<typename _Type>
using dynamodb_untag_resource_t =
    decltype(std::declval<_Type&>().untag_resource(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));

// dynamodb_list_tags_of_resource_t
//   detector: list_tags_of_resource(arn) const method.
// wraps ListTagsOfResource.
template<typename _Type>
using dynamodb_list_tags_of_resource_t =
    decltype(std::declval<const _Type&>().list_tags_of_resource(
        std::declval<const std::string&>()));


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_dynamodb_item_ops
//   trait: checks if type _Type supports core item operations
// (put_item + get_item + update_item + delete_item).
template<typename _Type>
struct has_dynamodb_item_ops : djinterp::conjunction<
    is_detected<dynamodb_put_item_t, _Type>,
    is_detected<dynamodb_get_item_t, _Type>,
    is_detected<dynamodb_update_item_t, _Type>,
    is_detected<dynamodb_delete_item_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_item_ops_v =
        has_dynamodb_item_ops<_Type>::value;
#endif

// has_dynamodb_batch_ops
//   trait: checks if type _Type supports batch operations
// (batch_get_item + batch_write_item).
template<typename _Type>
struct has_dynamodb_batch_ops : djinterp::conjunction<
    is_detected<dynamodb_batch_get_item_t, _Type>,
    is_detected<dynamodb_batch_write_item_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_batch_ops_v =
        has_dynamodb_batch_ops<_Type>::value;
#endif

// has_dynamodb_query_scan
//   trait: checks if type _Type supports query and scan
// (query + scan).
template<typename _Type>
struct has_dynamodb_query_scan : djinterp::conjunction<
    is_detected<dynamodb_query_t, _Type>,
    is_detected<dynamodb_scan_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_query_scan_v =
        has_dynamodb_query_scan<_Type>::value;
#endif

// has_dynamodb_transactions
//   trait: checks if type _Type supports transactions
// (transact_write_items + transact_get_items).
template<typename _Type>
struct has_dynamodb_transactions : djinterp::conjunction<
    is_detected<dynamodb_transact_write_items_t, _Type>,
    is_detected<dynamodb_transact_get_items_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_transactions_v =
        has_dynamodb_transactions<_Type>::value;
#endif

// has_dynamodb_partiql
//   trait: checks if type _Type supports PartiQL
// (execute_statement + batch_execute_statement + execute_transaction).
template<typename _Type>
struct has_dynamodb_partiql : djinterp::conjunction<
    is_detected<dynamodb_execute_statement_t, _Type>,
    is_detected<dynamodb_batch_execute_statement_t, _Type>,
    is_detected<dynamodb_execute_transaction_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_partiql_v =
        has_dynamodb_partiql<_Type>::value;
#endif

// has_dynamodb_conditional_writes
//   trait: checks if type _Type supports conditional writes
// (put_item_conditional + delete_item_conditional).
template<typename _Type>
struct has_dynamodb_conditional_writes : djinterp::conjunction<
    is_detected<dynamodb_put_item_conditional_t, _Type>,
    is_detected<dynamodb_delete_item_conditional_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_conditional_writes_v =
        has_dynamodb_conditional_writes<_Type>::value;
#endif

// has_dynamodb_table_management
//   trait: checks if type _Type supports table management
// (create_table + delete_table + describe_table + update_table +
// list_tables).
template<typename _Type>
struct has_dynamodb_table_management : djinterp::conjunction<
    is_detected<dynamodb_create_table_t, _Type>,
    is_detected<dynamodb_delete_table_t, _Type>,
    is_detected<dynamodb_describe_table_t, _Type>,
    is_detected<dynamodb_update_table_t, _Type>,
    is_detected<dynamodb_list_tables_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_table_management_v =
        has_dynamodb_table_management<_Type>::value;
#endif

// has_dynamodb_secondary_indexes
//   trait: checks if type _Type supports secondary index operations
// (create_gsi + delete_gsi + query_index).
template<typename _Type>
struct has_dynamodb_secondary_indexes : djinterp::conjunction<
    is_detected<dynamodb_create_gsi_t, _Type>,
    is_detected<dynamodb_delete_gsi_t, _Type>,
    is_detected<dynamodb_query_index_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_secondary_indexes_v =
        has_dynamodb_secondary_indexes<_Type>::value;
#endif

// has_dynamodb_streams
//   trait: checks if type _Type supports DynamoDB Streams
// (describe_stream + get_shard_iterator + get_records + list_streams).
template<typename _Type>
struct has_dynamodb_streams : djinterp::conjunction<
    is_detected<dynamodb_describe_stream_t, _Type>,
    is_detected<dynamodb_get_shard_iterator_t, _Type>,
    is_detected<dynamodb_get_records_t, _Type>,
    is_detected<dynamodb_list_streams_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_streams_v =
        has_dynamodb_streams<_Type>::value;
#endif

// has_dynamodb_ttl_management
//   trait: checks if type _Type supports TTL management
// (update_time_to_live + describe_time_to_live).
template<typename _Type>
struct has_dynamodb_ttl_management : djinterp::conjunction<
    is_detected<dynamodb_update_ttl_t, _Type>,
    is_detected<dynamodb_describe_ttl_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_ttl_management_v =
        has_dynamodb_ttl_management<_Type>::value;
#endif

// has_dynamodb_backup
//   trait: checks if type _Type supports backup / PITR operations
// (create_backup + restore_from_backup + describe_continuous_backups).
template<typename _Type>
struct has_dynamodb_backup : djinterp::conjunction<
    is_detected<dynamodb_create_backup_t, _Type>,
    is_detected<dynamodb_restore_from_backup_t, _Type>,
    is_detected<dynamodb_describe_continuous_backups_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_backup_v =
        has_dynamodb_backup<_Type>::value;
#endif

// has_dynamodb_diagnostics
//   trait: checks if type _Type supports diagnostics
// (describe_limits + describe_endpoints + table_status).
template<typename _Type>
struct has_dynamodb_diagnostics : djinterp::conjunction<
    is_detected<dynamodb_describe_limits_t, _Type>,
    is_detected<dynamodb_describe_endpoints_t, _Type>,
    is_detected<dynamodb_table_status_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_diagnostics_v =
        has_dynamodb_diagnostics<_Type>::value;
#endif

// has_dynamodb_global_tables
//   trait: checks if type _Type supports global table operations
// (create_global_table + describe_global_table + update_global_table).
template<typename _Type>
struct has_dynamodb_global_tables : djinterp::conjunction<
    is_detected<dynamodb_create_global_table_t, _Type>,
    is_detected<dynamodb_describe_global_table_t, _Type>,
    is_detected<dynamodb_update_global_table_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_global_tables_v =
        has_dynamodb_global_tables<_Type>::value;
#endif

// has_dynamodb_tagging
//   trait: checks if type _Type supports resource tagging
// (tag_resource + untag_resource + list_tags_of_resource).
template<typename _Type>
struct has_dynamodb_tagging : djinterp::conjunction<
    is_detected<dynamodb_tag_resource_t, _Type>,
    is_detected<dynamodb_untag_resource_t, _Type>,
    is_detected<dynamodb_list_tags_of_resource_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_tagging_v =
        has_dynamodb_tagging<_Type>::value;
#endif

// is_dynamodb_connection
//   trait: compound trait verifying type _Type implements a DynamoDB
// connection interface (item ops + query/scan + table management +
// diagnostics).
template<typename _Type>
struct is_dynamodb_connection : djinterp::conjunction<
    has_dynamodb_item_ops<_Type>,
    has_dynamodb_query_scan<_Type>,
    has_dynamodb_table_management<_Type>,
    has_dynamodb_diagnostics<_Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_dynamodb_connection_v =
        is_dynamodb_connection<_Type>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// dynamodb_can_put_item
//   tagless trait: true if _Type has put_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_put_item = false;

template<typename _Type>
constexpr bool dynamodb_can_put_item<_Type,
    std::void_t<dynamodb_put_item_t<_Type>>> = true;

// dynamodb_can_get_item
//   tagless trait: true if _Type has get_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_get_item = false;

template<typename _Type>
constexpr bool dynamodb_can_get_item<_Type,
    std::void_t<dynamodb_get_item_t<_Type>>> = true;

// dynamodb_can_update_item
//   tagless trait: true if _Type has update_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_update_item = false;

template<typename _Type>
constexpr bool dynamodb_can_update_item<_Type,
    std::void_t<dynamodb_update_item_t<_Type>>> = true;

// dynamodb_can_delete_item
//   tagless trait: true if _Type has delete_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_delete_item = false;

template<typename _Type>
constexpr bool dynamodb_can_delete_item<_Type,
    std::void_t<dynamodb_delete_item_t<_Type>>> = true;

// dynamodb_can_batch_write
//   tagless trait: true if _Type has batch_write_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_batch_write = false;

template<typename _Type>
constexpr bool dynamodb_can_batch_write<_Type,
    std::void_t<dynamodb_batch_write_item_t<_Type>>> = true;

// dynamodb_can_query
//   tagless trait: true if _Type has query().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_query = false;

template<typename _Type>
constexpr bool dynamodb_can_query<_Type,
    std::void_t<dynamodb_query_t<_Type>>> = true;

// dynamodb_can_scan
//   tagless trait: true if _Type has scan().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_scan = false;

template<typename _Type>
constexpr bool dynamodb_can_scan<_Type,
    std::void_t<dynamodb_scan_t<_Type>>> = true;

// dynamodb_can_transact_write
//   tagless trait: true if _Type has transact_write_items().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_transact_write = false;

template<typename _Type>
constexpr bool dynamodb_can_transact_write<_Type,
    std::void_t<dynamodb_transact_write_items_t<_Type>>> = true;

// dynamodb_can_execute_statement
//   tagless trait: true if _Type has execute_statement().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_execute_statement = false;

template<typename _Type>
constexpr bool dynamodb_can_execute_statement<_Type,
    std::void_t<dynamodb_execute_statement_t<_Type>>> = true;

// dynamodb_can_put_conditional
//   tagless trait: true if _Type has put_item_conditional().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_put_conditional = false;

template<typename _Type>
constexpr bool dynamodb_can_put_conditional<_Type,
    std::void_t<dynamodb_put_item_conditional_t<_Type>>> = true;

// dynamodb_can_create_table
//   tagless trait: true if _Type has create_table().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_create_table = false;

template<typename _Type>
constexpr bool dynamodb_can_create_table<_Type,
    std::void_t<dynamodb_create_table_t<_Type>>> = true;

// dynamodb_can_query_index
//   tagless trait: true if _Type has query_index().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_query_index = false;

template<typename _Type>
constexpr bool dynamodb_can_query_index<_Type,
    std::void_t<dynamodb_query_index_t<_Type>>> = true;

// dynamodb_can_get_records
//   tagless trait: true if _Type has get_records().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_get_records = false;

template<typename _Type>
constexpr bool dynamodb_can_get_records<_Type,
    std::void_t<dynamodb_get_records_t<_Type>>> = true;

// dynamodb_can_update_ttl
//   tagless trait: true if _Type has update_time_to_live().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_update_ttl = false;

template<typename _Type>
constexpr bool dynamodb_can_update_ttl<_Type,
    std::void_t<dynamodb_update_ttl_t<_Type>>> = true;

// dynamodb_can_create_backup
//   tagless trait: true if _Type has create_backup().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_create_backup = false;

template<typename _Type>
constexpr bool dynamodb_can_create_backup<_Type,
    std::void_t<dynamodb_create_backup_t<_Type>>> = true;

// dynamodb_can_create_global_table
//   tagless trait: true if _Type has create_global_table().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_create_global_table = false;

template<typename _Type>
constexpr bool dynamodb_can_create_global_table<_Type,
    std::void_t<dynamodb_create_global_table_t<_Type>>> = true;


// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// dynamodb_does_item_ops
//   tagless trait: true if _Type supports the full item-operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_item_ops = false;

template<typename _Type>
constexpr bool dynamodb_does_item_ops<_Type, std::void_t<
    dynamodb_put_item_t<_Type>,
    dynamodb_get_item_t<_Type>,
    dynamodb_update_item_t<_Type>,
    dynamodb_delete_item_t<_Type>>> = true;

// dynamodb_does_batch_ops
//   tagless trait: true if _Type supports the full batch-operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_batch_ops = false;

template<typename _Type>
constexpr bool dynamodb_does_batch_ops<_Type, std::void_t<
    dynamodb_batch_get_item_t<_Type>,
    dynamodb_batch_write_item_t<_Type>>> = true;

// dynamodb_does_query_scan
//   tagless trait: true if _Type supports the full query/scan surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_query_scan = false;

template<typename _Type>
constexpr bool dynamodb_does_query_scan<_Type, std::void_t<
    dynamodb_query_t<_Type>,
    dynamodb_scan_t<_Type>>> = true;

// dynamodb_does_transactions
//   tagless trait: true if _Type supports the full transaction surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_transactions = false;

template<typename _Type>
constexpr bool dynamodb_does_transactions<_Type, std::void_t<
    dynamodb_transact_write_items_t<_Type>,
    dynamodb_transact_get_items_t<_Type>>> = true;

// dynamodb_does_partiql
//   tagless trait: true if _Type supports the full PartiQL surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_partiql = false;

template<typename _Type>
constexpr bool dynamodb_does_partiql<_Type, std::void_t<
    dynamodb_execute_statement_t<_Type>,
    dynamodb_batch_execute_statement_t<_Type>,
    dynamodb_execute_transaction_t<_Type>>> = true;

// dynamodb_does_conditional_writes
//   tagless trait: true if _Type supports the full conditional-write
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_conditional_writes = false;

template<typename _Type>
constexpr bool dynamodb_does_conditional_writes<_Type, std::void_t<
    dynamodb_put_item_conditional_t<_Type>,
    dynamodb_delete_item_conditional_t<_Type>>> = true;

// dynamodb_does_table_management
//   tagless trait: true if _Type supports the full table-management
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_table_management = false;

template<typename _Type>
constexpr bool dynamodb_does_table_management<_Type, std::void_t<
    dynamodb_create_table_t<_Type>,
    dynamodb_delete_table_t<_Type>,
    dynamodb_describe_table_t<_Type>,
    dynamodb_update_table_t<_Type>,
    dynamodb_list_tables_t<_Type>>> = true;

// dynamodb_does_secondary_indexes
//   tagless trait: true if _Type supports the full secondary-index
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_secondary_indexes = false;

template<typename _Type>
constexpr bool dynamodb_does_secondary_indexes<_Type, std::void_t<
    dynamodb_create_gsi_t<_Type>,
    dynamodb_delete_gsi_t<_Type>,
    dynamodb_query_index_t<_Type>>> = true;

// dynamodb_does_streams
//   tagless trait: true if _Type supports the full Streams surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_streams = false;

template<typename _Type>
constexpr bool dynamodb_does_streams<_Type, std::void_t<
    dynamodb_describe_stream_t<_Type>,
    dynamodb_get_shard_iterator_t<_Type>,
    dynamodb_get_records_t<_Type>,
    dynamodb_list_streams_t<_Type>>> = true;

// dynamodb_does_ttl_management
//   tagless trait: true if _Type supports the full TTL-management
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_ttl_management = false;

template<typename _Type>
constexpr bool dynamodb_does_ttl_management<_Type, std::void_t<
    dynamodb_update_ttl_t<_Type>,
    dynamodb_describe_ttl_t<_Type>>> = true;

// dynamodb_does_backup
//   tagless trait: true if _Type supports the full backup/PITR surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_backup = false;

template<typename _Type>
constexpr bool dynamodb_does_backup<_Type, std::void_t<
    dynamodb_create_backup_t<_Type>,
    dynamodb_restore_from_backup_t<_Type>,
    dynamodb_describe_continuous_backups_t<_Type>>> = true;

// dynamodb_does_diagnostics
//   tagless trait: true if _Type supports the full diagnostics surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_diagnostics = false;

template<typename _Type>
constexpr bool dynamodb_does_diagnostics<_Type, std::void_t<
    dynamodb_describe_limits_t<_Type>,
    dynamodb_describe_endpoints_t<_Type>,
    dynamodb_table_status_t<_Type>>> = true;

// dynamodb_does_global_tables
//   tagless trait: true if _Type supports the full global-table surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_global_tables = false;

template<typename _Type>
constexpr bool dynamodb_does_global_tables<_Type, std::void_t<
    dynamodb_create_global_table_t<_Type>,
    dynamodb_describe_global_table_t<_Type>,
    dynamodb_update_global_table_t<_Type>>> = true;

// dynamodb_does_tagging
//   tagless trait: true if _Type supports the full resource-tagging
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_tagging = false;

template<typename _Type>
constexpr bool dynamodb_does_tagging<_Type, std::void_t<
    dynamodb_tag_resource_t<_Type>,
    dynamodb_untag_resource_t<_Type>,
    dynamodb_list_tags_of_resource_t<_Type>>> = true;

// dynamodb_is_full_connection
//   tagless trait: true if _Type satisfies the complete DynamoDB
// connection interface (item ops + query/scan + table management +
// diagnostics + batch ops).
template<typename _Type>
constexpr bool dynamodb_is_full_connection =
    ( dynamodb_does_item_ops<_Type>         &&
      dynamodb_does_query_scan<_Type>       &&
      dynamodb_does_table_management<_Type> &&
      dynamodb_does_diagnostics<_Type>      &&
      dynamodb_does_batch_ops<_Type> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

// enable_if_dynamodb_connection
//   type: SFINAE helper for DynamoDB connection constraints.
template<typename _Type>
using enable_if_dynamodb_connection =
    typename std::enable_if<is_dynamodb_connection<_Type>::value>::type;

// enable_if_has_dynamodb_transactions
//   type: SFINAE helper for DynamoDB transaction constraints.
template<typename _Type>
using enable_if_has_dynamodb_transactions =
    typename std::enable_if<has_dynamodb_transactions<_Type>::value>::type;

// enable_if_has_dynamodb_partiql
//   type: SFINAE helper for DynamoDB PartiQL constraints.
template<typename _Type>
using enable_if_has_dynamodb_partiql =
    typename std::enable_if<has_dynamodb_partiql<_Type>::value>::type;

// enable_if_has_dynamodb_streams
//   type: SFINAE helper for DynamoDB Streams constraints.
template<typename _Type>
using enable_if_has_dynamodb_streams =
    typename std::enable_if<has_dynamodb_streams<_Type>::value>::type;

// enable_if_has_dynamodb_global_tables
//   type: SFINAE helper for DynamoDB global table constraints.
template<typename _Type>
using enable_if_has_dynamodb_global_tables =
    typename std::enable_if<has_dynamodb_global_tables<_Type>::value>::type;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_DYNAMODB_TRAITS_
