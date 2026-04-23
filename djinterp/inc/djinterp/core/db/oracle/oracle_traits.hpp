/****************************************************************************
* djinterp [database]                                      oracle_traits.hpp
*
* djinterp Oracle Database traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to Oracle Database connections via OCI
* (Oracle Call Interface), including:
*   - statement caching (enable_statement_cache, get_statement_cache_size)
*   - array DML / batch execution (execute_batch)
*   - LOB handling (read_lob, write_lob, lob_length)
*   - implicit results from PL/SQL (get_implicit_results)
*   - session pooling (create_session_pool, get_session)
*   - continuous query notification (subscribe, unsubscribe)
*   - server output capture (enable_server_output, get_server_output)
*   - flashback query (set_scn, set_as_of_timestamp)
*   - edition-based redefinition (set_edition)
*   - SODA document access (soda_create_collection, soda_get_collection)
*   - PL/SQL block execution (execute_plsql)
*   - connection diagnostics (get_instance_name, get_service_name,
*     get_session_id)
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17.
*
*   NAMING CONVENTION:
*   expression detectors:    oci_<method>_t
*   struct-based traits:     has_oci_<capability>
*   variable template _v:    has_oci_<capability>_v
*   tagless traits:          oci_can_<action>
*   compound tagless traits: oci_does_<category>
*
* 
* path:      /inc/djinterp/core/db/oracle/oracle_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.03.26
****************************************************************************/

#ifndef DJINTERP_DATABASE_ORACLE_TRAITS_
#define DJINTERP_DATABASE_ORACLE_TRAITS_

#include "database_traits.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>


NS_DJINTERP
NS_DATABASE
// =========================================================================
//  NS_ORA
// =========================================================================

#ifndef D_KEYWORD_ORA
    #define D_KEYWORD_ORA   ora
#endif

#ifndef NS_ORA
    #define NS_ORA          D_NAMESPACE(D_KEYWORD_ORA)
#endif

NS_ORA


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  statement caching
// -------------------------------------------------------------------------

// oci_enable_statement_cache_t
//   detector: enable_statement_cache(std::size_t) method.
template<typename _T>
using oci_enable_statement_cache_t =
    decltype(std::declval<_T&>().enable_statement_cache(
        std::declval<std::size_t>()));

// oci_get_statement_cache_size_t
//   detector: get_statement_cache_size() const method.
template<typename _T>
using oci_get_statement_cache_size_t =
    decltype(std::declval<const _T&>().get_statement_cache_size());

// -------------------------------------------------------------------------
// B.  batch / array DML
// -------------------------------------------------------------------------

// oci_execute_batch_t
//   detector: execute_batch(const std::string&, std::size_t) method.
template<typename _T>
using oci_execute_batch_t = decltype(std::declval<_T&>().execute_batch(
    std::declval<const std::string&>(),
    std::declval<std::size_t>()));

// -------------------------------------------------------------------------
// C.  LOB operations
// -------------------------------------------------------------------------

// oci_read_lob_t
//   detector: read_lob() method.
template<typename _T>
using oci_read_lob_t =
    decltype(std::declval<_T&>().read_lob());

// oci_write_lob_t
//   detector: write_lob(const std::vector<std::uint8_t>&) method.
template<typename _T>
using oci_write_lob_t = decltype(std::declval<_T&>().write_lob(
    std::declval<const std::vector<std::uint8_t>&>()));

// oci_lob_length_t
//   detector: lob_length() const method.
template<typename _T>
using oci_lob_length_t =
    decltype(std::declval<const _T&>().lob_length());

// -------------------------------------------------------------------------
// D.  implicit results
// -------------------------------------------------------------------------

// oci_get_implicit_results_t
//   detector: get_implicit_results() method.
template<typename _T>
using oci_get_implicit_results_t =
    decltype(std::declval<_T&>().get_implicit_results());

// -------------------------------------------------------------------------
// E.  session pooling
// -------------------------------------------------------------------------

// oci_create_session_pool_t
//   detector: create_session_pool(std::size_t) method.
template<typename _T>
using oci_create_session_pool_t =
    decltype(std::declval<_T&>().create_session_pool(
        std::declval<std::size_t>()));

// oci_get_session_t
//   detector: get_session() method.
template<typename _T>
using oci_get_session_t =
    decltype(std::declval<_T&>().get_session());

// -------------------------------------------------------------------------
// F.  server output
// -------------------------------------------------------------------------

// oci_enable_server_output_t
//   detector: enable_server_output(bool) method.
template<typename _T>
using oci_enable_server_output_t =
    decltype(std::declval<_T&>().enable_server_output(
        std::declval<bool>()));

// oci_get_server_output_t
//   detector: get_server_output() method.
template<typename _T>
using oci_get_server_output_t =
    decltype(std::declval<_T&>().get_server_output());

// -------------------------------------------------------------------------
// G.  flashback query
// -------------------------------------------------------------------------

// oci_set_scn_t
//   detector: set_scn(std::uint64_t) method.
template<typename _T>
using oci_set_scn_t = decltype(std::declval<_T&>().set_scn(
    std::declval<std::uint64_t>()));

// oci_set_as_of_timestamp_t
//   detector: set_as_of_timestamp(const std::string&) method.
template<typename _T>
using oci_set_as_of_timestamp_t =
    decltype(std::declval<_T&>().set_as_of_timestamp(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// H.  PL/SQL execution
// -------------------------------------------------------------------------

// oci_execute_plsql_t
//   detector: execute_plsql(const std::string&) method.
template<typename _T>
using oci_execute_plsql_t = decltype(std::declval<_T&>().execute_plsql(
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// I.  edition-based redefinition
// -------------------------------------------------------------------------

// oci_set_edition_t
//   detector: set_edition(const std::string&) method.
template<typename _T>
using oci_set_edition_t = decltype(std::declval<_T&>().set_edition(
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// J.  SODA
// -------------------------------------------------------------------------

// oci_soda_create_collection_t
//   detector: soda_create_collection(const std::string&) method.
template<typename _T>
using oci_soda_create_collection_t =
    decltype(std::declval<_T&>().soda_create_collection(
        std::declval<const std::string&>()));

// oci_soda_get_collection_t
//   detector: soda_get_collection(const std::string&) method.
template<typename _T>
using oci_soda_get_collection_t =
    decltype(std::declval<_T&>().soda_get_collection(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// K.  connection diagnostics
// -------------------------------------------------------------------------

// oci_get_instance_name_t
//   detector: get_instance_name() const method.
template<typename _T>
using oci_get_instance_name_t =
    decltype(std::declval<const _T&>().get_instance_name());

// oci_get_service_name_t
//   detector: get_service_name() const method.
template<typename _T>
using oci_get_service_name_t =
    decltype(std::declval<const _T&>().get_service_name());

// oci_get_session_id_t
//   detector: get_session_id() const method.
template<typename _T>
using oci_get_session_id_t =
    decltype(std::declval<const _T&>().get_session_id());

// -------------------------------------------------------------------------
// L.  schema introspection
// -------------------------------------------------------------------------

// oci_table_exists_t
//   detector: table_exists(const std::string&) const method.
template<typename _T>
using oci_table_exists_t =
    decltype(std::declval<const _T&>().table_exists(
        std::declval<const std::string&>()));

// oci_get_table_names_t
//   detector: get_table_names() const method.
template<typename _T>
using oci_get_table_names_t =
    decltype(std::declval<const _T&>().get_table_names());

// -------------------------------------------------------------------------
// M.  CQN (Continuous Query Notification)
// -------------------------------------------------------------------------

// oci_subscribe_t
//   detector: subscribe(const std::string&) method.
template<typename _T>
using oci_subscribe_t = decltype(std::declval<_T&>().subscribe(
    std::declval<const std::string&>()));

// oci_unsubscribe_t
//   detector: unsubscribe() method.
template<typename _T>
using oci_unsubscribe_t =
    decltype(std::declval<_T&>().unsubscribe());


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_oci_statement_cache
//   trait: checks if type _T supports statement caching.
template<typename _T>
struct has_oci_statement_cache : djinterp::conjunction<
    is_detected<oci_enable_statement_cache_t, _T>,
    is_detected<oci_get_statement_cache_size_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_statement_cache_v =
        has_oci_statement_cache<_T>::value;
#endif

// has_oci_batch_dml
//   trait: checks if type _T supports array DML / batch execution.
template<typename _T>
struct has_oci_batch_dml : is_detected<oci_execute_batch_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_batch_dml_v =
        has_oci_batch_dml<_T>::value;
#endif

// has_oci_lob
//   trait: checks if type _T supports LOB operations.
template<typename _T>
struct has_oci_lob : djinterp::conjunction<
    is_detected<oci_read_lob_t, _T>,
    is_detected<oci_write_lob_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_lob_v = has_oci_lob<_T>::value;
#endif

// has_oci_lob_length
//   trait: checks if type _T exposes LOB length.
template<typename _T>
struct has_oci_lob_length : is_detected<oci_lob_length_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_lob_length_v =
        has_oci_lob_length<_T>::value;
#endif

// has_oci_implicit_results
//   trait: checks if type _T supports implicit PL/SQL results.
template<typename _T>
struct has_oci_implicit_results
    : is_detected<oci_get_implicit_results_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_implicit_results_v =
        has_oci_implicit_results<_T>::value;
#endif

// has_oci_session_pool
//   trait: checks if type _T supports session pooling.
template<typename _T>
struct has_oci_session_pool : djinterp::conjunction<
    is_detected<oci_create_session_pool_t, _T>,
    is_detected<oci_get_session_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_session_pool_v =
        has_oci_session_pool<_T>::value;
#endif

// has_oci_server_output
//   trait: checks if type _T supports DBMS_OUTPUT capture.
template<typename _T>
struct has_oci_server_output : djinterp::conjunction<
    is_detected<oci_enable_server_output_t, _T>,
    is_detected<oci_get_server_output_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_server_output_v =
        has_oci_server_output<_T>::value;
#endif

// has_oci_flashback
//   trait: checks if type _T supports flashback query.
template<typename _T>
struct has_oci_flashback : djinterp::conjunction<
    is_detected<oci_set_scn_t, _T>,
    is_detected<oci_set_as_of_timestamp_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_flashback_v = has_oci_flashback<_T>::value;
#endif

// has_oci_edition
//   trait: checks if type _T supports edition-based redefinition.
template<typename _T>
struct has_oci_edition : is_detected<oci_set_edition_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_edition_v = has_oci_edition<_T>::value;
#endif

// has_oci_soda
//   trait: checks if type _T supports SODA document access.
template<typename _T>
struct has_oci_soda : djinterp::conjunction<
    is_detected<oci_soda_create_collection_t, _T>,
    is_detected<oci_soda_get_collection_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_soda_v = has_oci_soda<_T>::value;
#endif

// has_oci_diagnostics
//   trait: checks if type _T supports connection diagnostics.
template<typename _T>
struct has_oci_diagnostics : djinterp::conjunction<
    is_detected<oci_get_instance_name_t, _T>,
    is_detected<oci_get_service_name_t, _T>,
    is_detected<oci_get_session_id_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_diagnostics_v =
        has_oci_diagnostics<_T>::value;
#endif

// has_oci_schema_query
//   trait: checks if type _T supports schema introspection.
template<typename _T>
struct has_oci_schema_query : djinterp::conjunction<
    is_detected<oci_table_exists_t, _T>,
    is_detected<oci_get_table_names_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_schema_query_v =
        has_oci_schema_query<_T>::value;
#endif

// has_oci_cqn
//   trait: checks if type _T supports continuous query notification.
template<typename _T>
struct has_oci_cqn : djinterp::conjunction<
    is_detected<oci_subscribe_t, _T>,
    is_detected<oci_unsubscribe_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_oci_cqn_v = has_oci_cqn<_T>::value;
#endif

// is_oci_connection
//   trait: compound trait verifying type _T implements an Oracle OCI
// connection interface (connection + diagnostics + schema +
// statement cache + PL/SQL execution).
template<typename _T>
struct is_oci_connection : djinterp::conjunction<
    is_connection<_T>,
    has_oci_diagnostics<_T>,
    has_oci_schema_query<_T>,
    has_oci_statement_cache<_T>,
    is_detected<oci_execute_plsql_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool is_oci_connection_v = is_oci_connection<_T>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// oci_can_execute_batch
//   tagless trait: true if _T has execute_batch().
template<typename _T, typename = void>
constexpr bool oci_can_execute_batch = false;

template<typename _T>
constexpr bool oci_can_execute_batch<_T,
    std::void_t<oci_execute_batch_t<_T>>> = true;

// oci_can_execute_plsql
//   tagless trait: true if _T has execute_plsql().
template<typename _T, typename = void>
constexpr bool oci_can_execute_plsql = false;

template<typename _T>
constexpr bool oci_can_execute_plsql<_T,
    std::void_t<oci_execute_plsql_t<_T>>> = true;

// oci_can_read_lob
//   tagless trait: true if _T has read_lob().
template<typename _T, typename = void>
constexpr bool oci_can_read_lob = false;

template<typename _T>
constexpr bool oci_can_read_lob<_T,
    std::void_t<oci_read_lob_t<_T>>> = true;

// oci_can_lob_length
//   tagless trait: true if _T has lob_length().
template<typename _T, typename = void>
constexpr bool oci_can_lob_length = false;

template<typename _T>
constexpr bool oci_can_lob_length<_T,
    std::void_t<oci_lob_length_t<_T>>> = true;

// oci_can_get_implicit_results
//   tagless trait: true if _T has get_implicit_results().
template<typename _T, typename = void>
constexpr bool oci_can_get_implicit_results = false;

template<typename _T>
constexpr bool oci_can_get_implicit_results<_T,
    std::void_t<oci_get_implicit_results_t<_T>>> = true;

// oci_can_create_session_pool
//   tagless trait: true if _T has create_session_pool().
template<typename _T, typename = void>
constexpr bool oci_can_create_session_pool = false;

template<typename _T>
constexpr bool oci_can_create_session_pool<_T,
    std::void_t<oci_create_session_pool_t<_T>>> = true;

// oci_can_get_session
//   tagless trait: true if _T has get_session().
template<typename _T, typename = void>
constexpr bool oci_can_get_session = false;

template<typename _T>
constexpr bool oci_can_get_session<_T,
    std::void_t<oci_get_session_t<_T>>> = true;

// oci_can_set_scn
//   tagless trait: true if _T has set_scn().
template<typename _T, typename = void>
constexpr bool oci_can_set_scn = false;

template<typename _T>
constexpr bool oci_can_set_scn<_T,
    std::void_t<oci_set_scn_t<_T>>> = true;

// oci_can_set_edition
//   tagless trait: true if _T has set_edition().
template<typename _T, typename = void>
constexpr bool oci_can_set_edition = false;

template<typename _T>
constexpr bool oci_can_set_edition<_T,
    std::void_t<oci_set_edition_t<_T>>> = true;

// oci_can_subscribe
//   tagless trait: true if _T has subscribe().
template<typename _T, typename = void>
constexpr bool oci_can_subscribe = false;

template<typename _T>
constexpr bool oci_can_subscribe<_T,
    std::void_t<oci_subscribe_t<_T>>> = true;

// oci_can_query_schema
//   tagless trait: true if _T has table_exists().
template<typename _T, typename = void>
constexpr bool oci_can_query_schema = false;

template<typename _T>
constexpr bool oci_can_query_schema<_T,
    std::void_t<oci_table_exists_t<_T>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// oci_does_lob
//   tagless trait: true if _T supports full LOB operations.
template<typename _T, typename = void>
constexpr bool oci_does_lob = false;

template<typename _T>
constexpr bool oci_does_lob<_T, std::void_t<
    oci_read_lob_t<_T>,
    oci_write_lob_t<_T>>> = true;

// oci_does_flashback
//   tagless trait: true if _T supports flashback query.
template<typename _T, typename = void>
constexpr bool oci_does_flashback = false;

template<typename _T>
constexpr bool oci_does_flashback<_T, std::void_t<
    oci_set_scn_t<_T>,
    oci_set_as_of_timestamp_t<_T>>> = true;

// oci_does_soda
//   tagless trait: true if _T supports SODA.
template<typename _T, typename = void>
constexpr bool oci_does_soda = false;

template<typename _T>
constexpr bool oci_does_soda<_T, std::void_t<
    oci_soda_create_collection_t<_T>,
    oci_soda_get_collection_t<_T>>> = true;

// oci_does_server_output
//   tagless trait: true if _T supports DBMS_OUTPUT capture.
template<typename _T, typename = void>
constexpr bool oci_does_server_output = false;

template<typename _T>
constexpr bool oci_does_server_output<_T, std::void_t<
    oci_enable_server_output_t<_T>,
    oci_get_server_output_t<_T>>> = true;

// oci_does_session_pool
//   tagless trait: true if _T supports session pooling.
template<typename _T, typename = void>
constexpr bool oci_does_session_pool = false;

template<typename _T>
constexpr bool oci_does_session_pool<_T, std::void_t<
    oci_create_session_pool_t<_T>,
    oci_get_session_t<_T>>> = true;

// oci_is_full_connection
//   tagless trait: true if _T satisfies the complete Oracle OCI
// connection interface.
template<typename _T>
constexpr bool oci_is_full_connection =
    ( is_connectable<_T>             &&
      oci_can_execute_plsql<_T>      &&
      oci_can_query_schema<_T>       &&
      oci_can_get_implicit_results<_T> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

// enable_if_oci_connection
//   type: SFINAE helper for Oracle OCI connection constraints.
template<typename _T>
using enable_if_oci_connection =
    typename std::enable_if<is_oci_connection<_T>::value>::type;

// enable_if_has_oci_flashback
//   type: SFINAE helper for flashback constraints.
template<typename _T>
using enable_if_has_oci_flashback =
    typename std::enable_if<has_oci_flashback<_T>::value>::type;

// enable_if_has_oci_soda
//   type: SFINAE helper for SODA constraints.
template<typename _T>
using enable_if_has_oci_soda =
    typename std::enable_if<has_oci_soda<_T>::value>::type;

// enable_if_has_oci_lob
//   type: SFINAE helper for LOB constraints.
template<typename _T>
using enable_if_has_oci_lob =
    typename std::enable_if<has_oci_lob<_T>::value>::type;

// enable_if_has_oci_session_pool
//   type: SFINAE helper for session pool constraints.
template<typename _T>
using enable_if_has_oci_session_pool =
    typename std::enable_if<has_oci_session_pool<_T>::value>::type;


NS_END  // ora
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ORACLE_TRAITS_
