/******************************************************************************
* djinterp [database]                                      oracle_concepts.hpp
*
*  djinterp Oracle classification concepts
*   C++20 concepts layered on top of oracle_traits.hpp.  These concepts
* provide readable `requires` constraints for Oracle / OCI-specific database
* connections, including statement caching, batch DML, LOB handling,
* implicit results, session pooling, server output capture, flashback query,
* edition-based redefinition, SODA access, diagnostics, schema queries, and
* continuous query notification.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the Oracle trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core Oracle Connection Concepts
* 3.   Oracle Capability Concepts
* 4.   Tagless Oracle Capability Concepts
*
* 
* path:      /inc/djinterp/core/db/oracle/oracle_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.01
******************************************************************************/

#ifndef DJINTERP_DATABASE_ORACLE_CONCEPTS_
#define DJINTERP_DATABASE_ORACLE_CONCEPTS_ 1

#include <type_traits>
#include "oracle_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "oracle_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_DATABASE
NS_ORA

// =============================================================================
// I.   Core Oracle Connection Concepts
// =============================================================================

// oci_connection
//   concept: constrains types implementing the Oracle OCI connection
// interface.
template<typename _Type>
concept oci_connection =
    is_oci_connection<clean_t<_Type>>::value;

// non_oci_connection
//   concept: constrains types that do not implement the Oracle OCI
// connection interface.
template<typename _Type>
concept non_oci_connection =
    !oci_connection<_Type>;

// plsql_connection
//   concept: constrains types exposing execute_plsql(const string&).
template<typename _Type>
concept plsql_connection =
    is_detected<oci_execute_plsql_t, clean_t<_Type>>::value;

// diagnostics_connection
//   concept: constrains types exposing Oracle connection diagnostics.
template<typename _Type>
concept diagnostics_connection =
    has_oci_diagnostics<clean_t<_Type>>::value;

// schema_query_connection
//   concept: constrains types exposing Oracle schema introspection.
template<typename _Type>
concept schema_query_connection =
    has_oci_schema_query<clean_t<_Type>>::value;


// =============================================================================
// II.  Oracle Capability Concepts
// =============================================================================

// statement_cache_connection
//   concept: constrains types supporting statement caching.
template<typename _Type>
concept statement_cache_connection =
    has_oci_statement_cache<clean_t<_Type>>::value;

// batch_dml_connection
//   concept: constrains types supporting batch / array DML execution.
template<typename _Type>
concept batch_dml_connection =
    has_oci_batch_dml<clean_t<_Type>>::value;

// lob_connection
//   concept: constrains types supporting Oracle LOB read/write operations.
template<typename _Type>
concept lob_connection =
    has_oci_lob<clean_t<_Type>>::value;

// lob_length_query
//   concept: constrains types exposing lob_length() const.
template<typename _Type>
concept lob_length_query =
    has_oci_lob_length<clean_t<_Type>>::value;

// implicit_results_connection
//   concept: constrains types exposing implicit PL/SQL result retrieval.
template<typename _Type>
concept implicit_results_connection =
    has_oci_implicit_results<clean_t<_Type>>::value;

// session_pool_connection
//   concept: constrains types supporting Oracle session pooling.
template<typename _Type>
concept session_pool_connection =
    has_oci_session_pool<clean_t<_Type>>::value;

// server_output_connection
//   concept: constrains types supporting DBMS_OUTPUT capture.
template<typename _Type>
concept server_output_connection =
    has_oci_server_output<clean_t<_Type>>::value;

// flashback_connection
//   concept: constrains types supporting flashback query controls.
template<typename _Type>
concept flashback_connection =
    has_oci_flashback<clean_t<_Type>>::value;

// edition_connection
//   concept: constrains types supporting edition-based redefinition.
template<typename _Type>
concept edition_connection =
    has_oci_edition<clean_t<_Type>>::value;

// soda_connection
//   concept: constrains types supporting Oracle SODA access.
template<typename _Type>
concept soda_connection =
    has_oci_soda<clean_t<_Type>>::value;

// cqn_connection
//   concept: constrains types supporting continuous query notification.
template<typename _Type>
concept cqn_connection =
    has_oci_cqn<clean_t<_Type>>::value;


// =============================================================================
// III. Tagless Oracle Capability Concepts
// =============================================================================

// oci_batch_executable
//   concept: constrains types satisfying the tagless batch-DML capability.
template<typename _Type>
concept oci_batch_executable =
    oci_can_execute_batch<clean_t<_Type>>;

// oci_plsql_executable
//   concept: constrains types satisfying the tagless PL/SQL capability.
template<typename _Type>
concept oci_plsql_executable =
    oci_can_execute_plsql<clean_t<_Type>>;

// oci_lob_readable
//   concept: constrains types satisfying the tagless LOB-read capability.
template<typename _Type>
concept oci_lob_readable =
    oci_can_read_lob<clean_t<_Type>>;

// oci_lob_sized
//   concept: constrains types satisfying the tagless LOB-length capability.
template<typename _Type>
concept oci_lob_sized =
    oci_can_lob_length<clean_t<_Type>>;

// oci_implicit_results_capable
//   concept: constrains types satisfying the tagless implicit-results
// capability.
template<typename _Type>
concept oci_implicit_results_capable =
    oci_can_get_implicit_results<clean_t<_Type>>;

// oci_session_pool_creatable
//   concept: constrains types satisfying the tagless session-pool creation
// capability.
template<typename _Type>
concept oci_session_pool_creatable =
    oci_can_create_session_pool<clean_t<_Type>>;

// oci_session_acquirable
//   concept: constrains types satisfying the tagless get-session capability.
template<typename _Type>
concept oci_session_acquirable =
    oci_can_get_session<clean_t<_Type>>;

// oci_flashback_scn_connection
//   concept: constrains types satisfying the tagless SCN flashback
// capability.
template<typename _Type>
concept oci_flashback_scn_connection =
    oci_can_set_scn<clean_t<_Type>>;

// oci_editionable_connection
//   concept: constrains types satisfying the tagless edition capability.
template<typename _Type>
concept oci_editionable_connection =
    oci_can_set_edition<clean_t<_Type>>;

// oci_subscribable_connection
//   concept: constrains types satisfying the tagless CQN subscription
// capability.
template<typename _Type>
concept oci_subscribable_connection =
    oci_can_subscribe<clean_t<_Type>>;

// oci_schema_queryable_connection
//   concept: constrains types satisfying the tagless schema-query
// capability.
template<typename _Type>
concept oci_schema_queryable_connection =
    oci_can_query_schema<clean_t<_Type>>;

// oci_lob_capable_connection
//   concept: constrains types satisfying the tagless full LOB capability set.
template<typename _Type>
concept oci_lob_capable_connection =
    oci_does_lob<clean_t<_Type>>;

// oci_flashback_capable_connection
//   concept: constrains types satisfying the tagless full flashback
// capability set.
template<typename _Type>
concept oci_flashback_capable_connection =
    oci_does_flashback<clean_t<_Type>>;

// oci_soda_capable_connection
//   concept: constrains types satisfying the tagless SODA capability set.
template<typename _Type>
concept oci_soda_capable_connection =
    oci_does_soda<clean_t<_Type>>;

// oci_server_output_capable_connection
//   concept: constrains types satisfying the tagless DBMS_OUTPUT capability
// set.
template<typename _Type>
concept oci_server_output_capable_connection =
    oci_does_server_output<clean_t<_Type>>;

// oci_session_pool_capable_connection
//   concept: constrains types satisfying the tagless session-pool capability
// set.
template<typename _Type>
concept oci_session_pool_capable_connection =
    oci_does_session_pool<clean_t<_Type>>;

// full_oci_connection
//   concept: constrains types satisfying the tagless complete Oracle OCI
// connection capability set.
template<typename _Type>
concept full_oci_connection =
    oci_is_full_connection<clean_t<_Type>>;


NS_END  // ora
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ORACLE_CONCEPTS_
