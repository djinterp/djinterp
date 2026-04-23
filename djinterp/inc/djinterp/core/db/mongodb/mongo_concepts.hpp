/******************************************************************************
* djinterp [database]                                    mongo_concepts.hpp
*
*  djinterp MongoDB classification concepts
*   C++20 concepts layered on top of mongo_traits.hpp.  These concepts
* provide readable `requires` constraints for MongoDB-specific database
* connections, including document CRUD, collection management,
* aggregation, change streams, bulk write, GridFS, index management,
* transactions, read/write concern configuration, and find/count
* operations.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the MongoDB trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core MongoDB Connection Concepts
* 3.   MongoDB Capability Concepts
* 4.   Tagless MongoDB Capability Concepts
*
* path:      /inc/djinterp/core/db/mongodb/mongo_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_DATABASE_MONGO_CONCEPTS_
#define DJINTERP_DATABASE_MONGO_CONCEPTS_ 1

#include <type_traits>
#include "mongo_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "mongo_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_DATABASE
NS_MONGO

// =============================================================================
// I.   Core MongoDB Connection Concepts
// =============================================================================

// mongo_connection
//   concept: constrains types implementing the MongoDB connection interface.
template<typename _Type>
concept mongo_connection =
    is_mongo_connection<clean_t<_Type>>::value;

// non_mongo_connection
//   concept: constrains types that do not implement the MongoDB connection
// interface.
template<typename _Type>
concept non_mongo_connection =
    !mongo_connection<_Type>;

// mongo_document_connection
//   concept: constrains MongoDB connections supporting document CRUD.
template<typename _Type>
concept mongo_document_connection =
    has_mongo_document_crud<clean_t<_Type>>::value;

// mongo_collection_connection
//   concept: constrains MongoDB connections supporting collection
// management.
template<typename _Type>
concept mongo_collection_connection =
    has_mongo_collections<clean_t<_Type>>::value;

// mongo_aggregation_connection
//   concept: constrains MongoDB connections exposing aggregate().
template<typename _Type>
concept mongo_aggregation_connection =
    is_detected<mongo_aggregate_t, clean_t<_Type>>::value;


// =============================================================================
// II.  MongoDB Capability Concepts
// =============================================================================

// mongo_change_stream_connection
//   concept: constrains MongoDB connections supporting change streams.
template<typename _Type>
concept mongo_change_stream_connection =
    has_mongo_change_streams<clean_t<_Type>>::value;

// mongo_gridfs_connection
//   concept: constrains MongoDB connections supporting GridFS.
template<typename _Type>
concept mongo_gridfs_connection =
    has_mongo_gridfs<clean_t<_Type>>::value;

// mongo_transaction_connection
//   concept: constrains MongoDB connections supporting sessions and
// transactions.
template<typename _Type>
concept mongo_transaction_connection =
    has_mongo_transactions<clean_t<_Type>>::value;

// mongo_index_connection
//   concept: constrains MongoDB connections supporting index management.
template<typename _Type>
concept mongo_index_connection =
    has_mongo_indexes<clean_t<_Type>>::value;

// mongo_insert_connection
//   concept: constrains MongoDB connections exposing insert_one().
template<typename _Type>
concept mongo_insert_connection =
    is_detected<mongo_insert_one_t, clean_t<_Type>>::value;

// mongo_find_one_connection
//   concept: constrains MongoDB connections exposing find_one().
template<typename _Type>
concept mongo_find_one_connection =
    is_detected<mongo_find_one_t, clean_t<_Type>>::value;

// mongo_update_one_connection
//   concept: constrains MongoDB connections exposing update_one().
template<typename _Type>
concept mongo_update_one_connection =
    is_detected<mongo_update_one_t, clean_t<_Type>>::value;

// mongo_delete_one_connection
//   concept: constrains MongoDB connections exposing delete_one().
template<typename _Type>
concept mongo_delete_one_connection =
    is_detected<mongo_delete_one_t, clean_t<_Type>>::value;

// mongo_replace_one_connection
//   concept: constrains MongoDB connections exposing replace_one().
template<typename _Type>
concept mongo_replace_one_connection =
    is_detected<mongo_replace_one_t, clean_t<_Type>>::value;

// mongo_create_collection_connection
//   concept: constrains MongoDB connections exposing create_collection().
template<typename _Type>
concept mongo_create_collection_connection =
    is_detected<mongo_create_collection_t, clean_t<_Type>>::value;

// mongo_drop_collection_connection
//   concept: constrains MongoDB connections exposing drop_collection().
template<typename _Type>
concept mongo_drop_collection_connection =
    is_detected<mongo_drop_collection_t, clean_t<_Type>>::value;

// mongo_collection_query_connection
//   concept: constrains MongoDB connections exposing collection_exists().
template<typename _Type>
concept mongo_collection_query_connection =
    is_detected<mongo_collection_exists_t, clean_t<_Type>>::value;

// mongo_collection_list_connection
//   concept: constrains MongoDB connections exposing list_collection_names().
template<typename _Type>
concept mongo_collection_list_connection =
    is_detected<mongo_list_collection_names_t, clean_t<_Type>>::value;

// mongo_watch_collection_connection
//   concept: constrains MongoDB connections exposing watch_collection().
template<typename _Type>
concept mongo_watch_collection_connection =
    is_detected<mongo_watch_collection_t, clean_t<_Type>>::value;

// mongo_watch_database_connection
//   concept: constrains MongoDB connections exposing watch_database().
template<typename _Type>
concept mongo_watch_database_connection =
    is_detected<mongo_watch_database_t, clean_t<_Type>>::value;

// mongo_bulk_write_connection
//   concept: constrains MongoDB connections exposing execute_bulk().
template<typename _Type>
concept mongo_bulk_write_connection =
    is_detected<mongo_execute_bulk_t, clean_t<_Type>>::value;

// mongo_gridfs_upload_connection
//   concept: constrains MongoDB connections exposing gridfs_upload().
template<typename _Type>
concept mongo_gridfs_upload_connection =
    is_detected<mongo_gridfs_upload_t, clean_t<_Type>>::value;

// mongo_gridfs_download_connection
//   concept: constrains MongoDB connections exposing gridfs_download().
template<typename _Type>
concept mongo_gridfs_download_connection =
    is_detected<mongo_gridfs_download_t, clean_t<_Type>>::value;

// mongo_create_index_connection
//   concept: constrains MongoDB connections exposing create_index().
template<typename _Type>
concept mongo_create_index_connection =
    is_detected<mongo_create_index_t, clean_t<_Type>>::value;

// mongo_list_indexes_connection
//   concept: constrains MongoDB connections exposing list_indexes().
template<typename _Type>
concept mongo_list_indexes_connection =
    is_detected<mongo_list_indexes_t, clean_t<_Type>>::value;

// mongo_session_connection
//   concept: constrains MongoDB connections exposing start_session().
template<typename _Type>
concept mongo_session_connection =
    is_detected<mongo_start_session_t, clean_t<_Type>>::value;

// mongo_start_transaction_connection
//   concept: constrains MongoDB connections exposing start_transaction().
template<typename _Type>
concept mongo_start_transaction_connection =
    is_detected<mongo_start_transaction_t, clean_t<_Type>>::value;

// mongo_commit_transaction_connection
//   concept: constrains MongoDB connections exposing commit_transaction().
template<typename _Type>
concept mongo_commit_transaction_connection =
    is_detected<mongo_commit_transaction_t, clean_t<_Type>>::value;

// mongo_abort_transaction_connection
//   concept: constrains MongoDB connections exposing abort_transaction().
template<typename _Type>
concept mongo_abort_transaction_connection =
    is_detected<mongo_abort_transaction_t, clean_t<_Type>>::value;

// mongo_read_concern_connection
//   concept: constrains MongoDB connections exposing set_read_concern().
template<typename _Type>
concept mongo_read_concern_connection =
    is_detected<mongo_set_read_concern_t, clean_t<_Type>>::value;

// mongo_write_concern_connection
//   concept: constrains MongoDB connections exposing set_write_concern().
template<typename _Type>
concept mongo_write_concern_connection =
    is_detected<mongo_set_write_concern_t, clean_t<_Type>>::value;

// mongo_find_connection
//   concept: constrains MongoDB connections exposing find().
template<typename _Type>
concept mongo_find_connection =
    is_detected<mongo_find_t, clean_t<_Type>>::value;

// mongo_count_documents_connection
//   concept: constrains MongoDB connections exposing count_documents().
template<typename _Type>
concept mongo_count_documents_connection =
    is_detected<mongo_count_documents_t, clean_t<_Type>>::value;


// =============================================================================
// III. Tagless MongoDB Capability Concepts
// =============================================================================

// mongo_insertable_connection
//   concept: constrains types satisfying the tagless insert capability.
template<typename _Type>
concept mongo_insertable_connection =
    mongo_can_insert<clean_t<_Type>>;

// mongo_findable_connection
//   concept: constrains types satisfying the tagless find capability.
template<typename _Type>
concept mongo_findable_connection =
    mongo_can_find<clean_t<_Type>>;

// mongo_aggregating_connection
//   concept: constrains types satisfying the tagless aggregate capability.
template<typename _Type>
concept mongo_aggregating_connection =
    mongo_can_aggregate<clean_t<_Type>>;

// mongo_watchable_connection
//   concept: constrains types satisfying the tagless change-stream watch
// capability.
template<typename _Type>
concept mongo_watchable_connection =
    mongo_can_watch<clean_t<_Type>>;

// mongo_bulk_writable_connection
//   concept: constrains types satisfying the tagless bulk-write capability.
template<typename _Type>
concept mongo_bulk_writable_connection =
    mongo_can_bulk_write<clean_t<_Type>>;

// mongo_gridfs_capable_connection
//   concept: constrains types satisfying the tagless GridFS capability.
template<typename _Type>
concept mongo_gridfs_capable_connection =
    mongo_can_gridfs<clean_t<_Type>>;

// mongo_transactable_connection
//   concept: constrains types satisfying the tagless transaction start
// capability.
template<typename _Type>
concept mongo_transactable_connection =
    mongo_can_transact<clean_t<_Type>>;

// mongo_collection_creating_connection
//   concept: constrains types satisfying the tagless create_collection
// capability.
template<typename _Type>
concept mongo_collection_creating_connection =
    mongo_can_create_collection<clean_t<_Type>>;

// mongo_crud_connection
//   concept: constrains types satisfying the tagless full document CRUD
// capability set.
template<typename _Type>
concept mongo_crud_connection =
    mongo_does_document_crud<clean_t<_Type>>;

// mongo_streaming_connection
//   concept: constrains types satisfying the tagless change-stream
// capability set.
template<typename _Type>
concept mongo_streaming_connection =
    mongo_does_change_streams<clean_t<_Type>>;

// mongo_transactional_connection
//   concept: constrains types satisfying the tagless transaction
// capability set.
template<typename _Type>
concept mongo_transactional_connection =
    mongo_does_transactions<clean_t<_Type>>;

// mongo_full_connection
//   concept: constrains types satisfying the tagless full MongoDB
// connection capability set.
template<typename _Type>
concept mongo_full_connection =
    mongo_is_full_connection<clean_t<_Type>>;


NS_END  // mongo
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MONGO_CONCEPTS_
