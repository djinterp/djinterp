/******************************************************************************
* djinterp [database]                                   cassandra_concepts.hpp
*
*  djinterp Apache Cassandra classification concepts
*   C++20 concepts layered on top of cassandra_traits.hpp.  These concepts
* provide readable `requires` constraints for Apache Cassandra database
* connections, including CQL execution, asynchronous execution, batch
* operations, keyspace and table management, data operations, lightweight
* transactions, user-defined types, materialized views, secondary
* indexes, consistency level and tracing control, cluster topology,
* user-defined functions / aggregates, and paged execution.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the Cassandra trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Cassandra Connection Concepts
* 3.   Cassandra Capability Concepts
* 4.   Tagless Cassandra Capability Concepts
*
* path:      /inc/djinterp/core/db/cassandra/cassandra_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_CASSANDRA_CONCEPTS_
#define DJINTERP_DATABASE_CASSANDRA_CONCEPTS_ 1

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "cassandra_concepts.hpp requires C++20 concepts support."
#endif

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./cassandra_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   Core Cassandra Connection Concepts
// =============================================================================

// cassandra_connection_c
//   concept: constrains types implementing the Cassandra connection
// interface. Suffixed with `_c` to avoid clashing with the
// `cassandra_connection` class type.
template<typename _Type>
concept cassandra_connection_c =
    is_cassandra_connection<clean_t<_Type>>::value;

// non_cassandra_connection
//   concept: constrains types that do not implement the Cassandra
// connection interface.
template<typename _Type>
concept non_cassandra_connection =
    !cassandra_connection_c<_Type>;

// cassandra_cql_connection
//   concept: constrains Cassandra connections supporting core CQL
// execution (execute_cql + prepare_cql + execute_prepared).
template<typename _Type>
concept cassandra_cql_connection =
    has_cassandra_cql_execution<clean_t<_Type>>::value;

// cassandra_async_connection
//   concept: constrains Cassandra connections supporting asynchronous
// execution.
template<typename _Type>
concept cassandra_async_connection =
    has_cassandra_async<clean_t<_Type>>::value;

// cassandra_batch_connection
//   concept: constrains Cassandra connections supporting batch
// operations (LOGGED / UNLOGGED / COUNTER).
template<typename _Type>
concept cassandra_batch_connection =
    has_cassandra_batch<clean_t<_Type>>::value;

// cassandra_keyspace_admin_connection
//   concept: constrains Cassandra connections supporting keyspace
// management (CREATE / DROP / USE / list keyspaces).
template<typename _Type>
concept cassandra_keyspace_admin_connection =
    has_cassandra_keyspace_management<clean_t<_Type>>::value;

// cassandra_table_admin_connection
//   concept: constrains Cassandra connections supporting table
// management (CREATE / DROP / ALTER / describe / list tables).
template<typename _Type>
concept cassandra_table_admin_connection =
    has_cassandra_table_management<clean_t<_Type>>::value;

// cassandra_data_connection
//   concept: constrains Cassandra connections supporting core data
// operations (INSERT / SELECT / UPDATE / DELETE / row_exists).
template<typename _Type>
concept cassandra_data_connection =
    has_cassandra_data_ops<clean_t<_Type>>::value;

// cassandra_lwt_connection
//   concept: constrains Cassandra connections supporting lightweight
// transactions / Paxos (IF NOT EXISTS / IF <condition>).
template<typename _Type>
concept cassandra_lwt_connection =
    has_cassandra_lwt<clean_t<_Type>>::value;

// cassandra_udt_connection
//   concept: constrains Cassandra connections supporting user-defined
// types.
template<typename _Type>
concept cassandra_udt_connection =
    has_cassandra_udt<clean_t<_Type>>::value;

// cassandra_materialized_view_connection
//   concept: constrains Cassandra connections supporting materialized
// views.
template<typename _Type>
concept cassandra_materialized_view_connection =
    has_cassandra_materialized_views<clean_t<_Type>>::value;

// cassandra_indexable_connection
//   concept: constrains Cassandra connections supporting secondary
// indexes.
template<typename _Type>
concept cassandra_indexable_connection =
    has_cassandra_secondary_indexes<clean_t<_Type>>::value;

// cassandra_consistency_connection
//   concept: constrains Cassandra connections exposing consistency-
// level control.
template<typename _Type>
concept cassandra_consistency_connection =
    has_cassandra_consistency<clean_t<_Type>>::value;

// cassandra_traceable_connection
//   concept: constrains Cassandra connections exposing request
// tracing.
template<typename _Type>
concept cassandra_traceable_connection =
    has_cassandra_tracing<clean_t<_Type>>::value;

// cassandra_topology_connection
//   concept: constrains Cassandra connections exposing cluster
// topology queries.
template<typename _Type>
concept cassandra_topology_connection =
    has_cassandra_topology<clean_t<_Type>>::value;

// cassandra_udf_connection
//   concept: constrains Cassandra connections supporting user-defined
// functions and aggregates.
template<typename _Type>
concept cassandra_udf_connection =
    has_cassandra_udf<clean_t<_Type>>::value;

// cassandra_paged_connection
//   concept: constrains Cassandra connections supporting paged
// execution.
template<typename _Type>
concept cassandra_paged_connection =
    has_cassandra_paging<clean_t<_Type>>::value;


// =============================================================================
// II.  Cassandra Capability Concepts
// =============================================================================

// cassandra_execute_capable_connection
//   concept: constrains types exposing execute_cql(cql).
template<typename _Type>
concept cassandra_execute_capable_connection =
    cassandra_can_execute_cql<clean_t<_Type>>;

// cassandra_preparable_connection
//   concept: constrains types exposing prepare_cql(cql).
template<typename _Type>
concept cassandra_preparable_connection =
    cassandra_can_prepare<clean_t<_Type>>;

// cassandra_prepared_executable_connection
//   concept: constrains types exposing execute_prepared(id, params).
template<typename _Type>
concept cassandra_prepared_executable_connection =
    cassandra_can_execute_prepared<clean_t<_Type>>;

// cassandra_async_executable_connection
//   concept: constrains types exposing async_execute(cql).
template<typename _Type>
concept cassandra_async_executable_connection =
    cassandra_can_async_execute<clean_t<_Type>>;

// cassandra_batchable_connection
//   concept: constrains types exposing batch_start(type).
template<typename _Type>
concept cassandra_batchable_connection =
    cassandra_can_batch<clean_t<_Type>>;

// cassandra_keyspace_creatable_connection
//   concept: constrains types exposing create_keyspace(...).
template<typename _Type>
concept cassandra_keyspace_creatable_connection =
    cassandra_can_create_keyspace<clean_t<_Type>>;

// cassandra_keyspace_switchable_connection
//   concept: constrains types exposing use_keyspace(name).
template<typename _Type>
concept cassandra_keyspace_switchable_connection =
    cassandra_can_use_keyspace<clean_t<_Type>>;

// cassandra_table_creatable_connection
//   concept: constrains types exposing create_table(...).
template<typename _Type>
concept cassandra_table_creatable_connection =
    cassandra_can_create_table<clean_t<_Type>>;

// cassandra_insertable_connection
//   concept: constrains types exposing insert_row(keyspace, table, row).
template<typename _Type>
concept cassandra_insertable_connection =
    cassandra_can_insert_row<clean_t<_Type>>;

// cassandra_selectable_connection
//   concept: constrains types exposing select_rows(cql).
template<typename _Type>
concept cassandra_selectable_connection =
    cassandra_can_select_rows<clean_t<_Type>>;

// cassandra_lwt_capable_connection
//   concept: constrains types exposing insert_if_not_exists(...).
template<typename _Type>
concept cassandra_lwt_capable_connection =
    cassandra_can_lwt_insert<clean_t<_Type>>;

// cassandra_type_creatable_connection
//   concept: constrains types exposing create_type(...).
template<typename _Type>
concept cassandra_type_creatable_connection =
    cassandra_can_create_type<clean_t<_Type>>;

// cassandra_mv_creatable_connection
//   concept: constrains types exposing create_materialized_view(...).
template<typename _Type>
concept cassandra_mv_creatable_connection =
    cassandra_can_create_mv<clean_t<_Type>>;

// cassandra_index_creatable_connection
//   concept: constrains types exposing create_index(...).
template<typename _Type>
concept cassandra_index_creatable_connection =
    cassandra_can_create_index<clean_t<_Type>>;

// cassandra_consistency_settable_connection
//   concept: constrains types exposing set_consistency(level).
template<typename _Type>
concept cassandra_consistency_settable_connection =
    cassandra_can_set_consistency<clean_t<_Type>>;

// cassandra_traceable_capable_connection
//   concept: constrains types exposing set_tracing(bool).
template<typename _Type>
concept cassandra_traceable_capable_connection =
    cassandra_can_trace<clean_t<_Type>>;

// cassandra_topology_queryable_connection
//   concept: constrains types exposing cluster_name().
template<typename _Type>
concept cassandra_topology_queryable_connection =
    cassandra_can_topology_query<clean_t<_Type>>;

// cassandra_function_creatable_connection
//   concept: constrains types exposing create_function(...).
template<typename _Type>
concept cassandra_function_creatable_connection =
    cassandra_can_create_function<clean_t<_Type>>;

// cassandra_pageable_connection
//   concept: constrains types exposing execute_paged(cql, size).
template<typename _Type>
concept cassandra_pageable_connection =
    cassandra_can_page<clean_t<_Type>>;


// =============================================================================
// III. Tagless Cassandra Capability Concepts
// =============================================================================

// cassandra_cql_executable
//   concept: constrains types satisfying the full tagless CQL-execution
// capability set.
template<typename _Type>
concept cassandra_cql_executable =
    cassandra_does_cql_execution<clean_t<_Type>>;

// cassandra_async_capable
//   concept: constrains types satisfying the full tagless asynchronous-
// execution capability set.
template<typename _Type>
concept cassandra_async_capable =
    cassandra_does_async<clean_t<_Type>>;

// cassandra_batch_capable
//   concept: constrains types satisfying the full tagless batch
// capability set.
template<typename _Type>
concept cassandra_batch_capable =
    cassandra_does_batch<clean_t<_Type>>;

// cassandra_keyspace_manageable
//   concept: constrains types satisfying the full tagless keyspace-
// management capability set.
template<typename _Type>
concept cassandra_keyspace_manageable =
    cassandra_does_keyspace_management<clean_t<_Type>>;

// cassandra_table_manageable
//   concept: constrains types satisfying the full tagless table-
// management capability set.
template<typename _Type>
concept cassandra_table_manageable =
    cassandra_does_table_management<clean_t<_Type>>;

// cassandra_data_capable
//   concept: constrains types satisfying the full tagless data-
// operation capability set.
template<typename _Type>
concept cassandra_data_capable =
    cassandra_does_data_ops<clean_t<_Type>>;

// cassandra_lwt_full
//   concept: constrains types satisfying the full tagless LWT
// capability set.
template<typename _Type>
concept cassandra_lwt_full =
    cassandra_does_lwt<clean_t<_Type>>;

// cassandra_udt_manageable
//   concept: constrains types satisfying the full tagless UDT
// capability set.
template<typename _Type>
concept cassandra_udt_manageable =
    cassandra_does_udt<clean_t<_Type>>;

// cassandra_mv_manageable
//   concept: constrains types satisfying the full tagless materialized-
// view capability set.
template<typename _Type>
concept cassandra_mv_manageable =
    cassandra_does_materialized_views<clean_t<_Type>>;

// cassandra_index_manageable
//   concept: constrains types satisfying the full tagless secondary-
// index capability set.
template<typename _Type>
concept cassandra_index_manageable =
    cassandra_does_secondary_indexes<clean_t<_Type>>;

// cassandra_consistency_controllable
//   concept: constrains types satisfying the full tagless consistency-
// control capability set.
template<typename _Type>
concept cassandra_consistency_controllable =
    cassandra_does_consistency<clean_t<_Type>>;

// cassandra_traceable
//   concept: constrains types satisfying the full tagless request-
// tracing capability set.
template<typename _Type>
concept cassandra_traceable =
    cassandra_does_tracing<clean_t<_Type>>;

// cassandra_topology_aware
//   concept: constrains types satisfying the full tagless cluster-
// topology capability set.
template<typename _Type>
concept cassandra_topology_aware =
    cassandra_does_topology<clean_t<_Type>>;

// cassandra_udf_manageable
//   concept: constrains types satisfying the full tagless UDF/UDA
// capability set.
template<typename _Type>
concept cassandra_udf_manageable =
    cassandra_does_udf<clean_t<_Type>>;

// cassandra_paginable
//   concept: constrains types satisfying the full tagless paging
// capability set.
template<typename _Type>
concept cassandra_paginable =
    cassandra_does_paging<clean_t<_Type>>;

// cassandra_full_connection
//   concept: constrains types satisfying the complete tagless
// Cassandra connection capability set.
template<typename _Type>
concept cassandra_full_connection =
    cassandra_is_full_connection<clean_t<_Type>>;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CASSANDRA_CONCEPTS_
