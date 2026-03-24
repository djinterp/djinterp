/******************************************************************************
* djinterp [database]                                     database_traits.hpp
* 
* djinterp database traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting and verifying database implementation capabilities without using
* virtual functions, including:
*   - capability detection (transactions, savepoints, prepared statements)
*   - interface verification
*   - type requirements and concepts
*   - compile-time feature checking
*
*   The traits system enables zero-overhead abstraction by resolving all
* polymorphism at compile time through templates rather than runtime virtual
* dispatch.
*
*   PORTABILITY:
*   This header reuses the detection idiom (is_detected, nonesuch, void_t)
* and logical connectives (conjunction) from type_traits.hpp, which provides
* portable C++11+ implementations of these C++17 facilities. Variable
* template (_v) aliases are gated behind
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES.
*
* path:      \inc\database\database_traits.hpp                                           
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.10
******************************************************************************/

#ifndef DJINTERP_DATABASE_TRAITS_HPP_
#define DJINTERP_DATABASE_TRAITS_HPP_

// type_traits.hpp provides the portable detection idiom (is_detected,
// nonesuch, void_t, conjunction, etc.) and pulls in djinterp.hpp for
// namespace macros and env.h for version/feature detection.
#include "..\meta\type_traits.hpp"

#include <string>
#include <cstddef>
#include <cstdint>


// =========================================================================
//  D_KEYWORD_DATABASE  /  NS_DB
// =========================================================================
// These should ideally live in djinterp.hpp alongside the other NS_*
// macros. They are guarded here so that database headers remain
// self-contained until the core header is updated.

#ifndef D_KEYWORD_DATABASE
    #define D_KEYWORD_DATABASE      database
#endif

#ifndef NS_DB
    #define NS_DB                   D_NAMESPACE(D_KEYWORD_DATABASE)
#endif


NS_DJINTERP
NS_DB


// =============================================================================
// I.   DETECTION IDIOM IMPORT
// =============================================================================
// The detection idiom (nonesuch, is_detected, detected_t, detected_or)
// is provided by djinterp::type_traits.hpp with C++11+ portability.
// We import the names into djinterp::database so that existing code
// using database::is_detected<...> continues to compile.

using djinterp::is_detected;
using djinterp::detected_t;
using djinterp::detected_or;


// =============================================================================
// II.  CONNECTION TRAIT DETECTORS
// =============================================================================

// connect_t
//   detector: connect() method.
template<typename _T>
using connect_t = decltype(std::declval<_T&>().connect());

// disconnect_t
//   detector: disconnect() method.
template<typename _T>
using disconnect_t = decltype(std::declval<_T&>().disconnect());

// is_connected_t
//   detector: is_connected() const method.
template<typename _T>
using is_connected_t = decltype(std::declval<const _T&>().is_connected());

// execute_query_t
//   detector: execute_query(const std::string&) method.
template<typename _T>
using execute_query_t = decltype(std::declval<_T&>().execute_query(
    std::declval<const std::string&>()));

// execute_update_t
//   detector: execute_update(const std::string&) method.
template<typename _T>
using execute_update_t = decltype(std::declval<_T&>().execute_update(
    std::declval<const std::string&>()));

// prepare_t
//   detector: prepare(const std::string&) method.
template<typename _T>
using prepare_t = decltype(std::declval<_T&>().prepare(
    std::declval<const std::string&>()));


// =============================================================================
// III. TRANSACTION TRAIT DETECTORS
// =============================================================================

// begin_transaction_t
//   detector: begin_transaction() method.
template<typename _T>
using begin_transaction_t =
    decltype(std::declval<_T&>().begin_transaction());

// commit_t
//   detector: commit() method.
template<typename _T>
using commit_t = decltype(std::declval<_T&>().commit());

// rollback_t
//   detector: rollback() method.
template<typename _T>
using rollback_t = decltype(std::declval<_T&>().rollback());

// in_transaction_t
//   detector: in_transaction() const method.
template<typename _T>
using in_transaction_t =
    decltype(std::declval<const _T&>().in_transaction());

// create_savepoint_t
//   detector: create_savepoint(const std::string&) method.
template<typename _T>
using create_savepoint_t = decltype(std::declval<_T&>().create_savepoint(
    std::declval<const std::string&>()));

// rollback_to_savepoint_t
//   detector: rollback_to_savepoint(const std::string&) method.
template<typename _T>
using rollback_to_savepoint_t =
    decltype(std::declval<_T&>().rollback_to_savepoint(
        std::declval<const std::string&>()));


// =============================================================================
// IV.  RESULT SET TRAIT DETECTORS
// =============================================================================

// next_t
//   detector: next() method.
template<typename _T>
using next_t = decltype(std::declval<_T&>().next());

// column_count_t
//   detector: column_count() const method.
template<typename _T>
using column_count_t =
    decltype(std::declval<const _T&>().column_count());

// column_name_t
//   detector: column_name(std::size_t) const method.
template<typename _T>
using column_name_t = decltype(std::declval<const _T&>().column_name(
    std::declval<std::size_t>()));

// get_value_index_t
//   detector: get_value(std::size_t) const method.
template<typename _T>
using get_value_index_t =
    decltype(std::declval<const _T&>().get_value(
        std::declval<std::size_t>()));

// get_value_name_t
//   detector: get_value(const std::string&) const method.
template<typename _T>
using get_value_name_t =
    decltype(std::declval<const _T&>().get_value(
        std::declval<const std::string&>()));


// =============================================================================
// V.   STATEMENT TRAIT DETECTORS
// =============================================================================

// bind_int_t
//   detector: bind_int(std::size_t, std::int32_t) method.
template<typename _T>
using bind_int_t = decltype(std::declval<_T&>().bind_int(
    std::declval<std::size_t>(),
    std::declval<std::int32_t>()));

// bind_string_t
//   detector: bind_string(std::size_t, const std::string&) method.
template<typename _T>
using bind_string_t = decltype(std::declval<_T&>().bind_string(
    std::declval<std::size_t>(),
    std::declval<const std::string&>()));

// execute_t
//   detector: execute() method.
template<typename _T>
using execute_t = decltype(std::declval<_T&>().execute());

// parameter_count_t
//   detector: parameter_count() const method.
template<typename _T>
using parameter_count_t =
    decltype(std::declval<const _T&>().parameter_count());


// =============================================================================
// VI.  CAPABILITY TRAITS
// =============================================================================

// has_connect
//   trait: checks if type _T has a connect() method.
template<typename _T>
struct has_connect : is_detected<connect_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_connect_v
    //   value: convenience alias for has_connect<_T>::value.
    template<typename _T>
    constexpr bool has_connect_v = has_connect<_T>::value;
#endif

// has_disconnect
//   trait: checks if type _T has a disconnect() method.
template<typename _T>
struct has_disconnect : is_detected<disconnect_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_disconnect_v
    //   value: convenience alias for has_disconnect<_T>::value.
    template<typename _T>
    constexpr bool has_disconnect_v = has_disconnect<_T>::value;
#endif

// has_execute_query
//   trait: checks if type _T has an execute_query() method.
template<typename _T>
struct has_execute_query : is_detected<execute_query_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_execute_query_v
    //   value: convenience alias for has_execute_query<_T>::value.
    template<typename _T>
    constexpr bool has_execute_query_v = has_execute_query<_T>::value;
#endif

// has_transactions
//   trait: checks if type _T supports transactions
// (begin_transaction, commit, rollback).
template<typename _T>
struct has_transactions : djinterp::conjunction<
    is_detected<begin_transaction_t, _T>,
    is_detected<commit_t, _T>,
    is_detected<rollback_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_transactions_v
    //   value: convenience alias for has_transactions<_T>::value.
    template<typename _T>
    constexpr bool has_transactions_v = has_transactions<_T>::value;
#endif

// has_savepoints
//   trait: checks if type _T supports savepoints
// (create_savepoint, rollback_to_savepoint).
template<typename _T>
struct has_savepoints : djinterp::conjunction<
    is_detected<create_savepoint_t, _T>,
    is_detected<rollback_to_savepoint_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_savepoints_v
    //   value: convenience alias for has_savepoints<_T>::value.
    template<typename _T>
    constexpr bool has_savepoints_v = has_savepoints<_T>::value;
#endif

// has_prepared_statements
//   trait: checks if type _T supports prepared statements.
template<typename _T>
struct has_prepared_statements : is_detected<prepare_t, _T>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prepared_statements_v
    //   value: convenience alias for
    // has_prepared_statements<_T>::value.
    template<typename _T>
    constexpr bool has_prepared_statements_v =
        has_prepared_statements<_T>::value;
#endif


// =============================================================================
// VII. INTERFACE VERIFICATION TRAITS
// =============================================================================

// is_connection
//   trait: verifies type _T implements the connection interface
// (connect, disconnect, is_connected, execute_query, execute_update).
template<typename _T>
struct is_connection : djinterp::conjunction<
    has_connect<_T>,
    has_disconnect<_T>,
    is_detected<is_connected_t, _T>,
    has_execute_query<_T>,
    is_detected<execute_update_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_connection_v
    //   value: convenience alias for is_connection<_T>::value.
    template<typename _T>
    constexpr bool is_connection_v = is_connection<_T>::value;
#endif

// is_result_set
//   trait: verifies type _T implements the result_set interface
// (next, column_count, column_name, get_value).
template<typename _T>
struct is_result_set : djinterp::conjunction<
    is_detected<next_t, _T>,
    is_detected<column_count_t, _T>,
    is_detected<column_name_t, _T>,
    is_detected<get_value_index_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_result_set_v
    //   value: convenience alias for is_result_set<_T>::value.
    template<typename _T>
    constexpr bool is_result_set_v = is_result_set<_T>::value;
#endif

// is_statement
//   trait: verifies type _T implements the statement interface
// (bind_int, bind_string, execute, parameter_count).
template<typename _T>
struct is_statement : djinterp::conjunction<
    is_detected<bind_int_t, _T>,
    is_detected<bind_string_t, _T>,
    is_detected<execute_t, _T>,
    is_detected<parameter_count_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_statement_v
    //   value: convenience alias for is_statement<_T>::value.
    template<typename _T>
    constexpr bool is_statement_v = is_statement<_T>::value;
#endif


// =============================================================================
// VIII. RESULT TYPE EXTRACTION
// =============================================================================

// result_set_type
//   trait: extracts the result_set type from a connection
// implementation's execute_query return type.
template<typename _Connection>
struct result_set_type
{
    using type = typename std::decay<
        decltype(std::declval<_Connection&>().execute_query(
            std::declval<const std::string&>()))>::type::element_type;
};

// result_set_type_t
//   type: convenience alias for result_set_type<_Connection>::type.
template<typename _Connection>
using result_set_type_t = typename result_set_type<_Connection>::type;

// statement_type
//   trait: extracts the statement type from a connection
// implementation's prepare return type.
template<typename _Connection>
struct statement_type
{
    using type = typename std::decay<
        decltype(std::declval<_Connection&>().prepare(
            std::declval<const std::string&>()))>::type::element_type;
};

// statement_type_t
//   type: convenience alias for statement_type<_Connection>::type.
template<typename _Connection>
using statement_type_t = typename statement_type<_Connection>::type;


// =============================================================================
// IX.  CONCEPT-LIKE CONSTRAINTS (C++11+ via enable_if)
// =============================================================================

// enable_if_connection
//   type: SFINAE helper for connection constraints.
template<typename _T>
using enable_if_connection =
    typename std::enable_if<is_connection<_T>::value>::type;

// enable_if_result_set
//   type: SFINAE helper for result_set constraints.
template<typename _T>
using enable_if_result_set =
    typename std::enable_if<is_result_set<_T>::value>::type;

// enable_if_statement
//   type: SFINAE helper for statement constraints.
template<typename _T>
using enable_if_statement =
    typename std::enable_if<is_statement<_T>::value>::type;

// enable_if_has_transactions
//   type: SFINAE helper for transaction support constraints.
template<typename _T>
using enable_if_has_transactions =
    typename std::enable_if<has_transactions<_T>::value>::type;

// enable_if_has_savepoints
//   type: SFINAE helper for savepoint support constraints.
template<typename _T>
using enable_if_has_savepoints =
    typename std::enable_if<has_savepoints<_T>::value>::type;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_TRAITS_HPP_
