/******************************************************************************
* djinterp [database]                                      mongo_traits.hpp
* 
* djinterp MongoDB traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to MongoDB connections via libmongoc,
* including:
*   - document CRUD (insert_one, find_one, update_one, delete_one,
*     replace_one)
*   - collection management (create_collection, drop_collection,
*     collection_exists, list_collection_names)
*   - aggregation pipeline execution (aggregate)
*   - change stream watching (watch_collection, watch_database)
*   - bulk write operations (create_bulk_write, execute_bulk)
*   - GridFS file storage (gridfs_upload, gridfs_download)
*   - index management (create_index, list_indexes)
*   - session and transaction management (start_session, start_transaction,
*     commit_transaction, abort_transaction)
*   - read/write concern configuration (set_read_concern,
*     set_write_concern, set_read_preference)
*   - find with options (find, count_documents)
*
*   MongoDB is a document-oriented NoSQL database; its API surface is
* fundamentally different from SQL-based vendors. There are no prepared
* statements or SQL queries; instead, operations use BSON documents as
* both queries (filters) and data.
*
*   NAMING CONVENTION:
*   Expression detectors:   mongo_<method>_t
*   Struct-based traits:    has_mongo_<capability>
*   Variable template _v:   has_mongo_<capability>_v
*   Tagless traits:          mongo_can_<action>
*   Compound tagless traits: mongo_does_<category>
*
* path:      \inc\database\mongodb\mongo_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_DATABASE_MONGO_TRAITS_
#define DJINTERP_DATABASE_MONGO_TRAITS_

#include "..\database_traits.hpp"

#include <vector>


NS_DJINTERP
NS_DB

// =========================================================================
//  NS_MONGO
// =========================================================================

#ifndef D_KEYWORD_MONGO
    #define D_KEYWORD_MONGO     mongo
#endif

#ifndef NS_MONGO
    #define NS_MONGO            D_NAMESPACE(D_KEYWORD_MONGO)
#endif

NS_MONGO


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  document CRUD
// -------------------------------------------------------------------------

// mongo_insert_one_t
//   detector: insert_one(const std::string&, const std::string&) method.
template<typename _T>
using mongo_insert_one_t = decltype(std::declval<_T&>().insert_one(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// mongo_find_one_t
//   detector: find_one(const std::string&, const std::string&) const
// method.
template<typename _T>
using mongo_find_one_t =
    decltype(std::declval<const _T&>().find_one(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_update_one_t
//   detector: update_one(const std::string&, const std::string&,
// const std::string&) method.
template<typename _T>
using mongo_update_one_t = decltype(std::declval<_T&>().update_one(
    std::declval<const std::string&>(),
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// mongo_delete_one_t
//   detector: delete_one(const std::string&, const std::string&) method.
template<typename _T>
using mongo_delete_one_t = decltype(std::declval<_T&>().delete_one(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// mongo_replace_one_t
//   detector: replace_one(const std::string&, const std::string&,
// const std::string&) method.
template<typename _T>
using mongo_replace_one_t = decltype(std::declval<_T&>().replace_one(
    std::declval<const std::string&>(),
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// B.  collection management
// -------------------------------------------------------------------------

// mongo_create_collection_t
//   detector: create_collection(const std::string&) method.
template<typename _T>
using mongo_create_collection_t =
    decltype(std::declval<_T&>().create_collection(
        std::declval<const std::string&>()));

// mongo_drop_collection_t
//   detector: drop_collection(const std::string&) method.
template<typename _T>
using mongo_drop_collection_t =
    decltype(std::declval<_T&>().drop_collection(
        std::declval<const std::string&>()));

// mongo_collection_exists_t
//   detector: collection_exists(const std::string&) const method.
template<typename _T>
using mongo_collection_exists_t =
    decltype(std::declval<const _T&>().collection_exists(
        std::declval<const std::string&>()));

// mongo_list_collection_names_t
//   detector: list_collection_names() const method.
template<typename _T>
using mongo_list_collection_names_t =
    decltype(std::declval<const _T&>().list_collection_names());

// -------------------------------------------------------------------------
// C.  aggregation
// -------------------------------------------------------------------------

// mongo_aggregate_t
//   detector: aggregate(const std::string&, const std::string&) method.
template<typename _T>
using mongo_aggregate_t = decltype(std::declval<_T&>().aggregate(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// D.  change streams
// -------------------------------------------------------------------------

// mongo_watch_collection_t
//   detector: watch_collection(const std::string&) method.
template<typename _T>
using mongo_watch_collection_t =
    decltype(std::declval<_T&>().watch_collection(
        std::declval<const std::string&>()));

// mongo_watch_database_t
//   detector: watch_database() method.
template<typename _T>
using mongo_watch_database_t =
    decltype(std::declval<_T&>().watch_database());

// -------------------------------------------------------------------------
// E.  bulk write
// -------------------------------------------------------------------------

// mongo_execute_bulk_t
//   detector: execute_bulk(const std::string&, const std::string&)
// method.
template<typename _T>
using mongo_execute_bulk_t =
    decltype(std::declval<_T&>().execute_bulk(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// F.  GridFS
// -------------------------------------------------------------------------

// mongo_gridfs_upload_t
//   detector: gridfs_upload(const std::string&,
// const std::vector<std::uint8_t>&) method.
template<typename _T>
using mongo_gridfs_upload_t =
    decltype(std::declval<_T&>().gridfs_upload(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::uint8_t>&>()));

// mongo_gridfs_download_t
//   detector: gridfs_download(const std::string&) method.
template<typename _T>
using mongo_gridfs_download_t =
    decltype(std::declval<_T&>().gridfs_download(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// G.  index management
// -------------------------------------------------------------------------

// mongo_create_index_t
//   detector: create_index(const std::string&, const std::string&)
// method.
template<typename _T>
using mongo_create_index_t =
    decltype(std::declval<_T&>().create_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_list_indexes_t
//   detector: list_indexes(const std::string&) const method.
template<typename _T>
using mongo_list_indexes_t =
    decltype(std::declval<const _T&>().list_indexes(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// H.  session and transactions
// -------------------------------------------------------------------------

// mongo_start_session_t
//   detector: start_session() method.
template<typename _T>
using mongo_start_session_t =
    decltype(std::declval<_T&>().start_session());

// mongo_start_transaction_t
//   detector: start_transaction() method.
template<typename _T>
using mongo_start_transaction_t =
    decltype(std::declval<_T&>().start_transaction());

// mongo_commit_transaction_t
//   detector: commit_transaction() method.
template<typename _T>
using mongo_commit_transaction_t =
    decltype(std::declval<_T&>().commit_transaction());

// mongo_abort_transaction_t
//   detector: abort_transaction() method.
template<typename _T>
using mongo_abort_transaction_t =
    decltype(std::declval<_T&>().abort_transaction());

// -------------------------------------------------------------------------
// I.  read/write concern
// -------------------------------------------------------------------------

// mongo_set_read_concern_t
//   detector: set_read_concern(const std::string&) method.
template<typename _T>
using mongo_set_read_concern_t =
    decltype(std::declval<_T&>().set_read_concern(
        std::declval<const std::string&>()));

// mongo_set_write_concern_t
//   detector: set_write_concern(int) method.
template<typename _T>
using mongo_set_write_concern_t =
    decltype(std::declval<_T&>().set_write_concern(
        std::declval<int>()));

// -------------------------------------------------------------------------
// J.  find
// -------------------------------------------------------------------------

// mongo_find_t
//   detector: find(const std::string&, const std::string&) method.
template<typename _T>
using mongo_find_t = decltype(std::declval<_T&>().find(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// mongo_count_documents_t
//   detector: count_documents(const std::string&, const std::string&)
// const method.
template<typename _T>
using mongo_count_documents_t =
    decltype(std::declval<const _T&>().count_documents(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_mongo_document_crud
//   trait: checks if type _T supports document CRUD.
template<typename _T>
struct has_mongo_document_crud : djinterp::conjunction<
    is_detected<mongo_insert_one_t, _T>,
    is_detected<mongo_find_one_t, _T>,
    is_detected<mongo_update_one_t, _T>,
    is_detected<mongo_delete_one_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mongo_document_crud_v =
        has_mongo_document_crud<_T>::value;
#endif

// has_mongo_collections
//   trait: checks if type _T supports collection management.
template<typename _T>
struct has_mongo_collections : djinterp::conjunction<
    is_detected<mongo_create_collection_t, _T>,
    is_detected<mongo_drop_collection_t, _T>,
    is_detected<mongo_collection_exists_t, _T>,
    is_detected<mongo_list_collection_names_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mongo_collections_v =
        has_mongo_collections<_T>::value;
#endif

// has_mongo_change_streams
//   trait: checks if type _T supports change streams.
template<typename _T>
struct has_mongo_change_streams : djinterp::conjunction<
    is_detected<mongo_watch_collection_t, _T>,
    is_detected<mongo_watch_database_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mongo_change_streams_v =
        has_mongo_change_streams<_T>::value;
#endif

// has_mongo_gridfs
//   trait: checks if type _T supports GridFS.
template<typename _T>
struct has_mongo_gridfs : djinterp::conjunction<
    is_detected<mongo_gridfs_upload_t, _T>,
    is_detected<mongo_gridfs_download_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mongo_gridfs_v = has_mongo_gridfs<_T>::value;
#endif

// has_mongo_transactions
//   trait: checks if type _T supports transactions.
template<typename _T>
struct has_mongo_transactions : djinterp::conjunction<
    is_detected<mongo_start_session_t, _T>,
    is_detected<mongo_start_transaction_t, _T>,
    is_detected<mongo_commit_transaction_t, _T>,
    is_detected<mongo_abort_transaction_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mongo_transactions_v =
        has_mongo_transactions<_T>::value;
#endif

// has_mongo_indexes
//   trait: checks if type _T supports index management.
template<typename _T>
struct has_mongo_indexes : djinterp::conjunction<
    is_detected<mongo_create_index_t, _T>,
    is_detected<mongo_list_indexes_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_mongo_indexes_v =
        has_mongo_indexes<_T>::value;
#endif

// is_mongo_connection
//   trait: compound trait verifying type _T implements a MongoDB
// connection interface (connect + documents + collections +
// aggregation).
template<typename _T>
struct is_mongo_connection : djinterp::conjunction<
    has_connect<_T>,
    has_disconnect<_T>,
    has_mongo_document_crud<_T>,
    has_mongo_collections<_T>,
    is_detected<mongo_aggregate_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool is_mongo_connection_v =
        is_mongo_connection<_T>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

template<typename _T, typename = void>
constexpr bool mongo_can_insert = false;
template<typename _T>
constexpr bool mongo_can_insert<_T,
    std::void_t<mongo_insert_one_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_find = false;
template<typename _T>
constexpr bool mongo_can_find<_T,
    std::void_t<mongo_find_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_aggregate = false;
template<typename _T>
constexpr bool mongo_can_aggregate<_T,
    std::void_t<mongo_aggregate_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_watch = false;
template<typename _T>
constexpr bool mongo_can_watch<_T,
    std::void_t<mongo_watch_collection_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_bulk_write = false;
template<typename _T>
constexpr bool mongo_can_bulk_write<_T,
    std::void_t<mongo_execute_bulk_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_gridfs = false;
template<typename _T>
constexpr bool mongo_can_gridfs<_T,
    std::void_t<mongo_gridfs_upload_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_transact = false;
template<typename _T>
constexpr bool mongo_can_transact<_T,
    std::void_t<mongo_start_transaction_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_can_create_collection = false;
template<typename _T>
constexpr bool mongo_can_create_collection<_T,
    std::void_t<mongo_create_collection_t<_T>>> = true;

// compound

template<typename _T, typename = void>
constexpr bool mongo_does_document_crud = false;
template<typename _T>
constexpr bool mongo_does_document_crud<_T, std::void_t<
    mongo_insert_one_t<_T>,
    mongo_find_one_t<_T>,
    mongo_update_one_t<_T>,
    mongo_delete_one_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_does_change_streams = false;
template<typename _T>
constexpr bool mongo_does_change_streams<_T, std::void_t<
    mongo_watch_collection_t<_T>,
    mongo_watch_database_t<_T>>> = true;

template<typename _T, typename = void>
constexpr bool mongo_does_transactions = false;
template<typename _T>
constexpr bool mongo_does_transactions<_T, std::void_t<
    mongo_start_session_t<_T>,
    mongo_start_transaction_t<_T>,
    mongo_commit_transaction_t<_T>,
    mongo_abort_transaction_t<_T>>> = true;

template<typename _T>
constexpr bool mongo_is_full_connection =
    ( can_connect<_T>                  &&
      can_disconnect<_T>               &&
      mongo_does_document_crud<_T>     &&
      mongo_can_aggregate<_T>          &&
      mongo_can_create_collection<_T> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

template<typename _T>
using enable_if_mongo_connection =
    typename std::enable_if<is_mongo_connection<_T>::value>::type;

template<typename _T>
using enable_if_has_mongo_transactions =
    typename std::enable_if<has_mongo_transactions<_T>::value>::type;

template<typename _T>
using enable_if_has_mongo_change_streams =
    typename std::enable_if<has_mongo_change_streams<_T>::value>::type;

template<typename _T>
using enable_if_has_mongo_gridfs =
    typename std::enable_if<has_mongo_gridfs<_T>::value>::type;


NS_END  // mongo
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MONGO_TRAITS_
