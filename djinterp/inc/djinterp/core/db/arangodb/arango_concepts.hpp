/******************************************************************************
* djinterp [database]                                      arango_concepts.hpp
*
*  djinterp ArangoDB classification concepts
*   C++20 concepts layered on top of arango_traits_revised.hpp.  These
* concepts provide readable `requires` constraints for ArangoDB-specific
* database connections, including AQL execution, document CRUD,
* collection management, graph operations, streaming transactions,
* cursor iteration, index management, view management, and database-level
* operations.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the ArangoDB trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core ArangoDB Connection Concepts
* 3.   ArangoDB Capability Concepts
* 4.   Tagless ArangoDB Capability Concepts
*
*
* path:      /inc/djinterp/core/db/arangodb/arango_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.03
******************************************************************************/

#ifndef DJINTERP_DATABASE_ARANGO_CONCEPTS_
#define DJINTERP_DATABASE_ARANGO_CONCEPTS_ 1

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "arango_concepts.hpp requires C++20 concepts support."
#endif

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./arango_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Core ArangoDB Connection Concepts
// ===========================================================================

// arango_connection
//   concept: constrains types implementing the ArangoDB connection
// interface.
template<typename _Type>
concept arango_connection = is_arango_connection<clean_t<_Type>>::value;

// non_arango_connection
//   concept: constrains types that do not implement the ArangoDB
// connection interface.
template<typename _Type>
concept non_arango_connection = !arango_connection<_Type>;

// aql_connection
//   concept: constrains types supporting AQL execution and explanation.
template<typename _Type>
concept aql_connection = has_arango_aql<clean_t<_Type>>::value;

// document_connection
//   concept: constrains types supporting full document CRUD.
template<typename _Type>
concept document_connection = has_arango_document_crud<clean_t<_Type>>::value;

// collection_connection
//   concept: constrains types supporting collection management.
template<typename _Type>
concept collection_connection = has_arango_collections<clean_t<_Type>>::value;

// cursor_connection
//   concept: constrains types supporting cursor-based iteration.
template<typename _Type>
concept cursor_connection = has_arango_cursor<clean_t<_Type>>::value;


// ===========================================================================
// II.  ArangoDB Capability Concepts
// ===========================================================================

// graph_connection
//   concept: constrains types supporting graph operations.
template<typename _Type>
concept graph_connection = has_arango_graph<clean_t<_Type>>::value;

// stream_transaction_connection
//   concept: constrains types supporting streaming transactions.
template<typename _Type>
concept stream_transaction_connection =
has_arango_stream_trx<clean_t<_Type>>::value;

// index_connection
//   concept: constrains types supporting index management.
template<typename _Type>
concept index_connection = has_arango_indexes<clean_t<_Type>>::value;

// database_ops_connection
//   concept: constrains types supporting database-level operations.
template<typename _Type>
concept database_ops_connection = has_arango_database_ops<clean_t<_Type>>::value;

// view_connection
//   concept: constrains types supporting view management.
template<typename _Type>
concept view_connection = has_arango_views<clean_t<_Type>>::value;

// aql_executable_connection
//   concept: constrains types exposing execute_aql(const string&).
template<typename _Type>
concept aql_executable_connection = arango_can_execute_aql<clean_t<_Type>>;

// aql_explainable_connection
//   concept: constrains types exposing explain_aql(const string&) const.
template<typename _Type>
concept aql_explainable_connection =
    is_detected<arango_explain_aql_t, clean_t<_Type>>::value;

// document_insert_connection
//   concept: constrains types exposing insert_document().
template<typename _Type>
concept document_insert_connection = 
	arango_can_insert_document<clean_t<_Type>>;

// document_get_connection
//   concept: constrains types exposing get_document().
template<typename _Type>
concept document_get_connection = arango_can_get_document<clean_t<_Type>>;

// document_update_connection
//   concept: constrains types exposing update_document().
template<typename _Type>
concept document_update_connection =
    is_detected<arango_update_document_t, clean_t<_Type>>::value;

// document_replace_connection
//   concept: constrains types exposing replace_document().
template<typename _Type>
concept document_replace_connection =
    arango_can_replace_document<clean_t<_Type>>;

// document_remove_connection
//   concept: constrains types exposing remove_document().
template<typename _Type>
concept document_remove_connection =
    is_detected<arango_remove_document_t, clean_t<_Type>>::value;

// collection_create_connection
//   concept: constrains types exposing create_collection().
template<typename _Type>
concept collection_create_connection =
    arango_can_create_collection<clean_t<_Type>>;

// collection_drop_connection
//   concept: constrains types exposing drop_collection().
template<typename _Type>
concept collection_drop_connection =
    is_detected<arango_drop_collection_t, clean_t<_Type>>::value;

// collection_exists_query
//   concept: constrains types exposing collection_exists().
template<typename _Type>
concept collection_exists_query =
    is_detected<arango_collection_exists_t, clean_t<_Type>>::value;

// collection_name_query
//   concept: constrains types exposing get_collection_names().
template<typename _Type>
concept collection_name_query =
    is_detected<arango_get_collection_names_t, clean_t<_Type>>::value;

// graph_create_connection
//   concept: constrains types exposing create_graph().
template<typename _Type>
concept graph_create_connection =
    is_detected<arango_create_graph_t, clean_t<_Type>>::value;

// graph_traversal_connection
//   concept: constrains types exposing traverse().
template<typename _Type>
concept graph_traversal_connection = arango_can_traverse<clean_t<_Type>>;

// shortest_path_connection
//   concept: constrains types exposing shortest_path().
template<typename _Type>
concept shortest_path_connection =
    is_detected<arango_shortest_path_t, clean_t<_Type>>::value;

// stream_transaction_begin_connection
//   concept: constrains types exposing begin_stream_trx().
template<typename _Type>
concept stream_transaction_begin_connection =
    arango_can_stream_trx<clean_t<_Type>>;

// stream_transaction_commit_connection
//   concept: constrains types exposing commit_trx().
template<typename _Type>
concept stream_transaction_commit_connection =
    is_detected<arango_commit_trx_t, clean_t<_Type>>::value;

// stream_transaction_abort_connection
//   concept: constrains types exposing abort_trx().
template<typename _Type>
concept stream_transaction_abort_connection =
    is_detected<arango_abort_trx_t, clean_t<_Type>>::value;

// cursor_create_connection
//   concept: constrains types exposing create_cursor().
template<typename _Type>
concept cursor_create_connection =
    is_detected<arango_create_cursor_t, clean_t<_Type>>::value;

// cursor_batch_connection
//   concept: constrains types exposing next_batch().
template<typename _Type>
concept cursor_batch_connection =
    is_detected<arango_next_batch_t, clean_t<_Type>>::value;

// index_create_connection
//   concept: constrains types exposing create_index().
template<typename _Type>
concept index_create_connection =
    arango_can_create_index<clean_t<_Type>>;

// index_query_connection
//   concept: constrains types exposing get_indexes().
template<typename _Type>
concept index_query_connection =
    is_detected<arango_get_indexes_t, clean_t<_Type>>::value;

// current_database_query
//   concept: constrains types exposing current_database().
template<typename _Type>
concept current_database_query =
    is_detected<arango_current_database_t, clean_t<_Type>>::value;

// list_databases_query
//   concept: constrains types exposing list_databases().
template<typename _Type>
concept list_databases_query =
    arango_can_list_databases<clean_t<_Type>>;

// view_create_connection
//   concept: constrains types exposing create_view().
template<typename _Type>
concept view_create_connection =
    arango_can_create_view<clean_t<_Type>>;


// ===========================================================================
// III. Tagless ArangoDB Capability Concepts
// ===========================================================================

// arango_document_crud_connection
//   concept: constrains types satisfying the tagless full document-CRUD
// capability set.
template<typename _Type>
concept arango_document_crud_connection =
    arango_does_document_crud<clean_t<_Type>>;

// arango_graph_capable_connection
//   concept: constrains types satisfying the tagless graph capability set.
template<typename _Type>
concept arango_graph_capable_connection =
    arango_does_graph<clean_t<_Type>>;

// arango_stream_transaction_capable_connection
//   concept: constrains types satisfying the tagless streaming-transaction
// capability set.
template<typename _Type>
concept arango_stream_transaction_capable_connection =
    arango_does_stream_trx<clean_t<_Type>>;

// arango_database_ops_capable_connection
//   concept: constrains types satisfying the tagless database-operations
// capability set.
template<typename _Type>
concept arango_database_ops_capable_connection =
    arango_does_database_ops<clean_t<_Type>>;

// full_arango_connection
//   concept: constrains types satisfying the tagless complete ArangoDB
// connection capability set.
template<typename _Type>
concept full_arango_connection =
    arango_is_full_connection<clean_t<_Type>>;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ARANGO_CONCEPTS_
