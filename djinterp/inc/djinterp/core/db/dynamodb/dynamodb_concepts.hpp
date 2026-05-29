/******************************************************************************
* djinterp [database]                                    dynamodb_concepts.hpp
*
*  djinterp DynamoDB classification concepts
*   C++20 concepts layered on top of dynamodb_traits.hpp.  These concepts
* provide readable `requires` constraints for Amazon DynamoDB database
* connections, including item operations, batch operations, query and
* scan, transactions, PartiQL, conditional writes, table management,
* secondary indexes, DynamoDB Streams, TTL management, backup / PITR,
* diagnostics, global tables, and resource tagging.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the DynamoDB trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core DynamoDB Connection Concepts
* 3.   DynamoDB Capability Concepts
* 4.   Tagless DynamoDB Capability Concepts
*
* path:      /inc/djinterp/core/db/dynamodb/dynamodb_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_DYNAMODB_CONCEPTS_
#define DJINTERP_DATABASE_DYNAMODB_CONCEPTS_ 1

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "dynamodb_concepts.hpp requires C++20 concepts support."
#endif

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./dynamodb_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   Core DynamoDB Connection Concepts
// =============================================================================

// dynamodb_connection_c
//   concept: constrains types implementing the DynamoDB connection
// interface. Suffixed with `_c` to avoid clashing with the
// `dynamodb_connection` class type.
template<typename _Type>
concept dynamodb_connection_c =
    is_dynamodb_connection<clean_t<_Type>>::value;

// non_dynamodb_connection
//   concept: constrains types that do not implement the DynamoDB
// connection interface.
template<typename _Type>
concept non_dynamodb_connection =
    !dynamodb_connection_c<_Type>;

// dynamodb_item_ops_connection
//   concept: constrains DynamoDB connections supporting core item
// operations (PutItem / GetItem / UpdateItem / DeleteItem).
template<typename _Type>
concept dynamodb_item_ops_connection =
    has_dynamodb_item_ops<clean_t<_Type>>::value;

// dynamodb_batch_connection
//   concept: constrains DynamoDB connections supporting batch
// operations (BatchGetItem / BatchWriteItem).
template<typename _Type>
concept dynamodb_batch_connection =
    has_dynamodb_batch_ops<clean_t<_Type>>::value;

// dynamodb_query_scan_connection
//   concept: constrains DynamoDB connections supporting query and
// scan (Query / Scan).
template<typename _Type>
concept dynamodb_query_scan_connection =
    has_dynamodb_query_scan<clean_t<_Type>>::value;

// dynamodb_transactional_connection
//   concept: constrains DynamoDB connections supporting transactions
// (TransactWriteItems / TransactGetItems).
template<typename _Type>
concept dynamodb_transactional_connection =
    has_dynamodb_transactions<clean_t<_Type>>::value;

// dynamodb_partiql_connection
//   concept: constrains DynamoDB connections supporting PartiQL
// (ExecuteStatement / BatchExecuteStatement / ExecuteTransaction).
template<typename _Type>
concept dynamodb_partiql_connection =
    has_dynamodb_partiql<clean_t<_Type>>::value;

// dynamodb_conditional_connection
//   concept: constrains DynamoDB connections supporting conditional
// writes (condition-expression PutItem / DeleteItem).
template<typename _Type>
concept dynamodb_conditional_connection =
    has_dynamodb_conditional_writes<clean_t<_Type>>::value;

// dynamodb_table_admin_connection
//   concept: constrains DynamoDB connections supporting table
// management (CreateTable / DeleteTable / DescribeTable / UpdateTable /
// ListTables).
template<typename _Type>
concept dynamodb_table_admin_connection =
    has_dynamodb_table_management<clean_t<_Type>>::value;

// dynamodb_indexable_connection
//   concept: constrains DynamoDB connections supporting secondary
// index operations.
template<typename _Type>
concept dynamodb_indexable_connection =
    has_dynamodb_secondary_indexes<clean_t<_Type>>::value;

// dynamodb_stream_connection
//   concept: constrains DynamoDB connections supporting DynamoDB
// Streams.
template<typename _Type>
concept dynamodb_stream_connection =
    has_dynamodb_streams<clean_t<_Type>>::value;

// dynamodb_ttl_connection
//   concept: constrains DynamoDB connections supporting TTL
// management.
template<typename _Type>
concept dynamodb_ttl_connection =
    has_dynamodb_ttl_management<clean_t<_Type>>::value;

// dynamodb_backup_connection
//   concept: constrains DynamoDB connections supporting backup /
// point-in-time recovery operations.
template<typename _Type>
concept dynamodb_backup_connection =
    has_dynamodb_backup<clean_t<_Type>>::value;

// dynamodb_diagnostics_connection
//   concept: constrains DynamoDB connections supporting diagnostics
// (DescribeLimits / DescribeEndpoints / table status).
template<typename _Type>
concept dynamodb_diagnostics_connection =
    has_dynamodb_diagnostics<clean_t<_Type>>::value;

// dynamodb_global_table_connection
//   concept: constrains DynamoDB connections supporting global table
// operations.
template<typename _Type>
concept dynamodb_global_table_connection =
    has_dynamodb_global_tables<clean_t<_Type>>::value;

// dynamodb_taggable_connection
//   concept: constrains DynamoDB connections supporting resource
// tagging.
template<typename _Type>
concept dynamodb_taggable_connection =
    has_dynamodb_tagging<clean_t<_Type>>::value;


// =============================================================================
// II.  DynamoDB Capability Concepts
// =============================================================================

// dynamodb_put_capable_connection
//   concept: constrains types exposing put_item(table, item).
template<typename _Type>
concept dynamodb_put_capable_connection =
    dynamodb_can_put_item<clean_t<_Type>>;

// dynamodb_get_capable_connection
//   concept: constrains types exposing get_item(table, key).
template<typename _Type>
concept dynamodb_get_capable_connection =
    dynamodb_can_get_item<clean_t<_Type>>;

// dynamodb_update_capable_connection
//   concept: constrains types exposing update_item(table, key, updates).
template<typename _Type>
concept dynamodb_update_capable_connection =
    dynamodb_can_update_item<clean_t<_Type>>;

// dynamodb_delete_capable_connection
//   concept: constrains types exposing delete_item(table, key).
template<typename _Type>
concept dynamodb_delete_capable_connection =
    dynamodb_can_delete_item<clean_t<_Type>>;

// dynamodb_batch_write_capable_connection
//   concept: constrains types exposing batch_write_item(table, items).
template<typename _Type>
concept dynamodb_batch_write_capable_connection =
    dynamodb_can_batch_write<clean_t<_Type>>;

// dynamodb_queryable_connection
//   concept: constrains types exposing query(table, key_condition).
template<typename _Type>
concept dynamodb_queryable_connection =
    dynamodb_can_query<clean_t<_Type>>;

// dynamodb_scannable_connection
//   concept: constrains types exposing scan(table).
template<typename _Type>
concept dynamodb_scannable_connection =
    dynamodb_can_scan<clean_t<_Type>>;

// dynamodb_transact_write_capable_connection
//   concept: constrains types exposing transact_write_items(items).
template<typename _Type>
concept dynamodb_transact_write_capable_connection =
    dynamodb_can_transact_write<clean_t<_Type>>;

// dynamodb_statement_capable_connection
//   concept: constrains types exposing execute_statement(statement).
template<typename _Type>
concept dynamodb_statement_capable_connection =
    dynamodb_can_execute_statement<clean_t<_Type>>;

// dynamodb_conditional_put_connection
//   concept: constrains types exposing put_item_conditional(...).
template<typename _Type>
concept dynamodb_conditional_put_connection =
    dynamodb_can_put_conditional<clean_t<_Type>>;

// dynamodb_table_creatable_connection
//   concept: constrains types exposing create_table(...).
template<typename _Type>
concept dynamodb_table_creatable_connection =
    dynamodb_can_create_table<clean_t<_Type>>;

// dynamodb_index_queryable_connection
//   concept: constrains types exposing query_index(table, index, cond).
template<typename _Type>
concept dynamodb_index_queryable_connection =
    dynamodb_can_query_index<clean_t<_Type>>;

// dynamodb_records_readable_connection
//   concept: constrains types exposing get_records(shard_iterator).
template<typename _Type>
concept dynamodb_records_readable_connection =
    dynamodb_can_get_records<clean_t<_Type>>;

// dynamodb_ttl_updatable_connection
//   concept: constrains types exposing update_time_to_live(...).
template<typename _Type>
concept dynamodb_ttl_updatable_connection =
    dynamodb_can_update_ttl<clean_t<_Type>>;

// dynamodb_backup_creatable_connection
//   concept: constrains types exposing create_backup(table, name).
template<typename _Type>
concept dynamodb_backup_creatable_connection =
    dynamodb_can_create_backup<clean_t<_Type>>;

// dynamodb_global_table_creatable_connection
//   concept: constrains types exposing create_global_table(table, regions).
template<typename _Type>
concept dynamodb_global_table_creatable_connection =
    dynamodb_can_create_global_table<clean_t<_Type>>;


// =============================================================================
// III. Tagless DynamoDB Capability Concepts
// =============================================================================

// dynamodb_item_addressable
//   concept: constrains types satisfying the full tagless item-
// operation capability set.
template<typename _Type>
concept dynamodb_item_addressable =
    dynamodb_does_item_ops<clean_t<_Type>>;

// dynamodb_batch_capable
//   concept: constrains types satisfying the full tagless batch-
// operation capability set.
template<typename _Type>
concept dynamodb_batch_capable =
    dynamodb_does_batch_ops<clean_t<_Type>>;

// dynamodb_query_scannable
//   concept: constrains types satisfying the full tagless query/scan
// capability set.
template<typename _Type>
concept dynamodb_query_scannable =
    dynamodb_does_query_scan<clean_t<_Type>>;

// dynamodb_transactional
//   concept: constrains types satisfying the full tagless transaction
// capability set.
template<typename _Type>
concept dynamodb_transactional =
    dynamodb_does_transactions<clean_t<_Type>>;

// dynamodb_partiql_capable
//   concept: constrains types satisfying the full tagless PartiQL
// capability set.
template<typename _Type>
concept dynamodb_partiql_capable =
    dynamodb_does_partiql<clean_t<_Type>>;

// dynamodb_conditional_capable
//   concept: constrains types satisfying the full tagless conditional-
// write capability set.
template<typename _Type>
concept dynamodb_conditional_capable =
    dynamodb_does_conditional_writes<clean_t<_Type>>;

// dynamodb_table_manageable
//   concept: constrains types satisfying the full tagless table-
// management capability set.
template<typename _Type>
concept dynamodb_table_manageable =
    dynamodb_does_table_management<clean_t<_Type>>;

// dynamodb_index_manageable
//   concept: constrains types satisfying the full tagless secondary-
// index capability set.
template<typename _Type>
concept dynamodb_index_manageable =
    dynamodb_does_secondary_indexes<clean_t<_Type>>;

// dynamodb_stream_capable
//   concept: constrains types satisfying the full tagless Streams
// capability set.
template<typename _Type>
concept dynamodb_stream_capable =
    dynamodb_does_streams<clean_t<_Type>>;

// dynamodb_ttl_manageable
//   concept: constrains types satisfying the full tagless TTL-
// management capability set.
template<typename _Type>
concept dynamodb_ttl_manageable =
    dynamodb_does_ttl_management<clean_t<_Type>>;

// dynamodb_backup_capable
//   concept: constrains types satisfying the full tagless backup/PITR
// capability set.
template<typename _Type>
concept dynamodb_backup_capable =
    dynamodb_does_backup<clean_t<_Type>>;

// dynamodb_diagnostic_capable
//   concept: constrains types satisfying the full tagless diagnostics
// capability set.
template<typename _Type>
concept dynamodb_diagnostic_capable =
    dynamodb_does_diagnostics<clean_t<_Type>>;

// dynamodb_global_table_capable
//   concept: constrains types satisfying the full tagless global-table
// capability set.
template<typename _Type>
concept dynamodb_global_table_capable =
    dynamodb_does_global_tables<clean_t<_Type>>;

// dynamodb_taggable
//   concept: constrains types satisfying the full tagless resource-
// tagging capability set.
template<typename _Type>
concept dynamodb_taggable =
    dynamodb_does_tagging<clean_t<_Type>>;

// dynamodb_full_connection
//   concept: constrains types satisfying the complete tagless
// DynamoDB connection capability set.
template<typename _Type>
concept dynamodb_full_connection =
    dynamodb_is_full_connection<clean_t<_Type>>;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_DYNAMODB_CONCEPTS_
