/****************************************************************************
* djinterp [database]                             arango_traits_revised.hpp
*
* djinterp ArangoDB traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to ArangoDB connections, including:
*   - AQL query execution (execute_aql, explain_aql)
*   - document CRUD operations (insert_document, get_document,
*     update_document, replace_document, remove_document)
*   - collection management (create_collection, drop_collection,
*     collection_exists, get_collection_names)
*   - graph management (create_graph, traverse, shortest_path)
*   - transaction management (begin_stream_trx, commit_trx, abort_trx)
*   - cursor iteration (create_cursor, next_batch)
*   - index management (create_index, get_indexes)
*   - ArangoSearch / View management (create_view)
*   - database-level operations (current_database, list_databases)
*
*   ArangoDB is a multi-model NoSQL database; its API surface is
* fundamentally different from SQL-based vendors. There is no concept
* of prepared statements with positional bind parameters; instead,
* AQL uses named bind variables and the HTTP REST API for all
* operations.
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided.
*
*   NAMING CONVENTION:
*   Expression detectors:   arango_<method>_t
*   Struct-based traits:    has_arango_<capability>
*   Variable template _v:   has_arango_<capability>_v
*   Tagless traits:         arango_can_<action>
*   Compound tagless traits:
*                           arango_does_<category>
*
* path:      /inc/database/arangodb/arango_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.06.15
****************************************************************************/

#ifndef DJINTERP_DATABASE_ARANGO_TRAITS_
#define DJINTERP_DATABASE_ARANGO_TRAITS_

#include "../database_traits.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


NS_DJINTERP
NS_DB

// =========================================================================
//  NS_ARANGO
// =========================================================================

#ifndef D_KEYWORD_ARANGO
    #define D_KEYWORD_ARANGO    arango
#endif

#ifndef NS_ARANGO
    #define NS_ARANGO           D_NAMESPACE(D_KEYWORD_ARANGO)
#endif

NS_ARANGO


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  AQL execution
// -------------------------------------------------------------------------

// arango_execute_aql_t
//   detector: execute_aql(const std::string&) method.
template<typename _Type>
using arango_execute_aql_t =
    decltype(std::declval<_Type&>().execute_aql(
        std::declval<const std::string&>()));

// arango_explain_aql_t
//   detector: explain_aql(const std::string&) const method.
template<typename _Type>
using arango_explain_aql_t =
    decltype(std::declval<const _Type&>().explain_aql(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// B.  document CRUD
// -------------------------------------------------------------------------

// arango_insert_document_t
//   detector: insert_document(const std::string&, const std::string&)
// method.
template<typename _Type>
using arango_insert_document_t =
    decltype(std::declval<_Type&>().insert_document(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// arango_get_document_t
//   detector: get_document(const std::string&, const std::string&)
// const method.
template<typename _Type>
using arango_get_document_t =
    decltype(std::declval<const _Type&>().get_document(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// arango_update_document_t
//   detector: update_document(const std::string&, const std::string&,
// const std::string&) method.
template<typename _Type>
using arango_update_document_t =
    decltype(std::declval<_Type&>().update_document(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// arango_replace_document_t
//   detector: replace_document(const std::string&, const std::string&,
// const std::string&) method.
template<typename _Type>
using arango_replace_document_t =
    decltype(std::declval<_Type&>().replace_document(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// arango_remove_document_t
//   detector: remove_document(const std::string&, const std::string&)
// method.
template<typename _Type>
using arango_remove_document_t =
    decltype(std::declval<_Type&>().remove_document(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// C.  collection management
// -------------------------------------------------------------------------

// arango_create_collection_t
//   detector: create_collection(const std::string&, int) method.
template<typename _Type>
using arango_create_collection_t =
    decltype(std::declval<_Type&>().create_collection(
        std::declval<const std::string&>(),
        std::declval<int>()));

// arango_drop_collection_t
//   detector: drop_collection(const std::string&) method.
template<typename _Type>
using arango_drop_collection_t =
    decltype(std::declval<_Type&>().drop_collection(
        std::declval<const std::string&>()));

// arango_collection_exists_t
//   detector: collection_exists(const std::string&) const method.
template<typename _Type>
using arango_collection_exists_t =
    decltype(std::declval<const _Type&>().collection_exists(
        std::declval<const std::string&>()));

// arango_get_collection_names_t
//   detector: get_collection_names() const method.
template<typename _Type>
using arango_get_collection_names_t =
    decltype(std::declval<const _Type&>().get_collection_names());

// -------------------------------------------------------------------------
// D.  graph operations
// -------------------------------------------------------------------------

// arango_create_graph_t
//   detector: create_graph(const std::string&) method.
template<typename _Type>
using arango_create_graph_t =
    decltype(std::declval<_Type&>().create_graph(
        std::declval<const std::string&>()));

// arango_traverse_t
//   detector: traverse(const std::string&, const std::string&, int)
// method.
template<typename _Type>
using arango_traverse_t =
    decltype(std::declval<_Type&>().traverse(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<int>()));

// arango_shortest_path_t
//   detector: shortest_path(const std::string&, const std::string&,
// const std::string&) method.
template<typename _Type>
using arango_shortest_path_t =
    decltype(std::declval<_Type&>().shortest_path(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// E.  streaming transactions
// -------------------------------------------------------------------------

// arango_begin_stream_trx_t
//   detector: begin_stream_trx(const std::vector<std::string>&) method.
template<typename _Type>
using arango_begin_stream_trx_t =
    decltype(std::declval<_Type&>().begin_stream_trx(
        std::declval<const std::vector<std::string>&>()));

// arango_commit_trx_t
//   detector: commit_trx(const std::string&) method.
template<typename _Type>
using arango_commit_trx_t =
    decltype(std::declval<_Type&>().commit_trx(
        std::declval<const std::string&>()));

// arango_abort_trx_t
//   detector: abort_trx(const std::string&) method.
template<typename _Type>
using arango_abort_trx_t =
    decltype(std::declval<_Type&>().abort_trx(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// F.  cursor
// -------------------------------------------------------------------------

// arango_create_cursor_t
//   detector: create_cursor(const std::string&) method.
template<typename _Type>
using arango_create_cursor_t =
    decltype(std::declval<_Type&>().create_cursor(
        std::declval<const std::string&>()));

// arango_next_batch_t
//   detector: next_batch(const std::string&) method.
template<typename _Type>
using arango_next_batch_t =
    decltype(std::declval<_Type&>().next_batch(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// G.  index management
// -------------------------------------------------------------------------

// arango_create_index_t
//   detector: create_index(const std::string&, const std::string&)
// method.
template<typename _Type>
using arango_create_index_t =
    decltype(std::declval<_Type&>().create_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// arango_get_indexes_t
//   detector: get_indexes(const std::string&) const method.
template<typename _Type>
using arango_get_indexes_t =
    decltype(std::declval<const _Type&>().get_indexes(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// H.  database-level operations
// -------------------------------------------------------------------------

// arango_current_database_t
//   detector: current_database() const method.
template<typename _Type>
using arango_current_database_t =
    decltype(std::declval<const _Type&>().current_database());

// arango_list_databases_t
//   detector: list_databases() const method.
template<typename _Type>
using arango_list_databases_t =
    decltype(std::declval<const _Type&>().list_databases());

// -------------------------------------------------------------------------
// I.  view management
// -------------------------------------------------------------------------

// arango_create_view_t
//   detector: create_view(const std::string&, const std::string&) method.
template<typename _Type>
using arango_create_view_t =
    decltype(std::declval<_Type&>().create_view(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_arango_aql
//   trait: checks if type _Type supports AQL execution.
template<typename _Type>
struct has_arango_aql : djinterp::conjunction<
    is_detected<arango_execute_aql_t, _Type>,
    is_detected<arango_explain_aql_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_aql_v = has_arango_aql<_Type>::value;
#endif

// has_arango_document_crud
//   trait: checks if type _Type supports document CRUD
// (insert + get + update + replace + remove).
template<typename _Type>
struct has_arango_document_crud : djinterp::conjunction<
    is_detected<arango_insert_document_t, _Type>,
    is_detected<arango_get_document_t, _Type>,
    is_detected<arango_update_document_t, _Type>,
    is_detected<arango_replace_document_t, _Type>,
    is_detected<arango_remove_document_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_document_crud_v =
        has_arango_document_crud<_Type>::value;
#endif

// has_arango_collections
//   trait: checks if type _Type supports collection management.
template<typename _Type>
struct has_arango_collections : djinterp::conjunction<
    is_detected<arango_create_collection_t, _Type>,
    is_detected<arango_drop_collection_t, _Type>,
    is_detected<arango_collection_exists_t, _Type>,
    is_detected<arango_get_collection_names_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_collections_v =
        has_arango_collections<_Type>::value;
#endif

// has_arango_graph
//   trait: checks if type _Type supports graph operations.
template<typename _Type>
struct has_arango_graph : djinterp::conjunction<
    is_detected<arango_create_graph_t, _Type>,
    is_detected<arango_traverse_t, _Type>,
    is_detected<arango_shortest_path_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_graph_v = has_arango_graph<_Type>::value;
#endif

// has_arango_stream_trx
//   trait: checks if type _Type supports streaming transactions.
template<typename _Type>
struct has_arango_stream_trx : djinterp::conjunction<
    is_detected<arango_begin_stream_trx_t, _Type>,
    is_detected<arango_commit_trx_t, _Type>,
    is_detected<arango_abort_trx_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_stream_trx_v =
        has_arango_stream_trx<_Type>::value;
#endif

// has_arango_cursor
//   trait: checks if type _Type supports cursor-based iteration.
template<typename _Type>
struct has_arango_cursor : djinterp::conjunction<
    is_detected<arango_create_cursor_t, _Type>,
    is_detected<arango_next_batch_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_cursor_v = has_arango_cursor<_Type>::value;
#endif

// has_arango_indexes
//   trait: checks if type _Type supports index management.
template<typename _Type>
struct has_arango_indexes : djinterp::conjunction<
    is_detected<arango_create_index_t, _Type>,
    is_detected<arango_get_indexes_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_indexes_v =
        has_arango_indexes<_Type>::value;
#endif

// has_arango_database_ops
//   trait: checks if type _Type supports database-level operations.
template<typename _Type>
struct has_arango_database_ops : djinterp::conjunction<
    is_detected<arango_current_database_t, _Type>,
    is_detected<arango_list_databases_t, _Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_database_ops_v =
        has_arango_database_ops<_Type>::value;
#endif

// has_arango_views
//   trait: checks if type _Type supports view management.
template<typename _Type>
struct has_arango_views : is_detected<arango_create_view_t, _Type>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_arango_views_v =
        has_arango_views<_Type>::value;
#endif

// is_arango_connection
//   trait: compound trait verifying type _Type implements an ArangoDB
// connection interface (connection + AQL + documents + collections +
// cursors).
template<typename _Type>
struct is_arango_connection : djinterp::conjunction<
    has_connect<_Type>,
    has_disconnect<_Type>,
    has_arango_aql<_Type>,
    has_arango_document_crud<_Type>,
    has_arango_collections<_Type>,
    has_arango_cursor<_Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_arango_connection_v =
        is_arango_connection<_Type>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

template<typename _Type, typename = void>
constexpr bool arango_can_execute_aql = false;

template<typename _Type>
constexpr bool arango_can_execute_aql<_Type,
    std::void_t<arango_execute_aql_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_insert_document = false;

template<typename _Type>
constexpr bool arango_can_insert_document<_Type,
    std::void_t<arango_insert_document_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_get_document = false;

template<typename _Type>
constexpr bool arango_can_get_document<_Type,
    std::void_t<arango_get_document_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_replace_document = false;

template<typename _Type>
constexpr bool arango_can_replace_document<_Type,
    std::void_t<arango_replace_document_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_traverse = false;

template<typename _Type>
constexpr bool arango_can_traverse<_Type,
    std::void_t<arango_traverse_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_create_collection = false;

template<typename _Type>
constexpr bool arango_can_create_collection<_Type,
    std::void_t<arango_create_collection_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_stream_trx = false;

template<typename _Type>
constexpr bool arango_can_stream_trx<_Type,
    std::void_t<arango_begin_stream_trx_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_create_view = false;

template<typename _Type>
constexpr bool arango_can_create_view<_Type,
    std::void_t<arango_create_view_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_create_index = false;

template<typename _Type>
constexpr bool arango_can_create_index<_Type,
    std::void_t<arango_create_index_t<_Type>>> = true;

template<typename _Type, typename = void>
constexpr bool arango_can_list_databases = false;

template<typename _Type>
constexpr bool arango_can_list_databases<_Type,
    std::void_t<arango_list_databases_t<_Type>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// arango_does_document_crud
//   tagless trait: true if _Type supports full document CRUD.
template<typename _Type, typename = void>
constexpr bool arango_does_document_crud = false;

template<typename _Type>
constexpr bool arango_does_document_crud<_Type, std::void_t<
    arango_insert_document_t<_Type>,
    arango_get_document_t<_Type>,
    arango_update_document_t<_Type>,
    arango_replace_document_t<_Type>,
    arango_remove_document_t<_Type>>> = true;

// arango_does_graph
//   tagless trait: true if _Type supports graph operations.
template<typename _Type, typename = void>
constexpr bool arango_does_graph = false;

template<typename _Type>
constexpr bool arango_does_graph<_Type, std::void_t<
    arango_create_graph_t<_Type>,
    arango_traverse_t<_Type>,
    arango_shortest_path_t<_Type>>> = true;

// arango_does_stream_trx
//   tagless trait: true if _Type supports streaming transactions.
template<typename _Type, typename = void>
constexpr bool arango_does_stream_trx = false;

template<typename _Type>
constexpr bool arango_does_stream_trx<_Type, std::void_t<
    arango_begin_stream_trx_t<_Type>,
    arango_commit_trx_t<_Type>,
    arango_abort_trx_t<_Type>>> = true;

// arango_does_database_ops
//   tagless trait: true if _Type supports database-level operations.
template<typename _Type, typename = void>
constexpr bool arango_does_database_ops = false;

template<typename _Type>
constexpr bool arango_does_database_ops<_Type, std::void_t<
    arango_current_database_t<_Type>,
    arango_list_databases_t<_Type>>> = true;

// arango_is_full_connection
//   tagless trait: true if _Type satisfies the complete ArangoDB
// connection interface.
template<typename _Type>
constexpr bool arango_is_full_connection =
    ( can_connect<_Type>               &&
      can_disconnect<_Type>            &&
      arango_can_execute_aql<_Type>    &&
      arango_does_document_crud<_Type> &&
      arango_can_create_collection<_Type> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

template<typename _Type>
using enable_if_arango_connection =
    typename std::enable_if<is_arango_connection<_Type>::value>::type;

template<typename _Type>
using enable_if_has_arango_graph =
    typename std::enable_if<has_arango_graph<_Type>::value>::type;

template<typename _Type>
using enable_if_has_arango_stream_trx =
    typename std::enable_if<has_arango_stream_trx<_Type>::value>::type;

template<typename _Type>
using enable_if_has_arango_database_ops =
    typename std::enable_if<has_arango_database_ops<_Type>::value>::type;


NS_END  // arango
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ARANGO_TRAITS_
