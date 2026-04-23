/******************************************************************************
* djinterp [database]                                    database_concepts.hpp
*
*  djinterp database classification concepts
*   C++20 concepts layered on top of database_traits.hpp.  These concepts
* provide readable `requires` constraints for database connections,
* transactions, result sets, statements, vendor-specific capabilities, and
* extracted database interface types.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the database trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Connection Concepts
* 3.   Transaction Concepts
* 4.   Result Set Concepts
* 5.   Statement Concepts
* 6.   Vendor Connection Concepts
* 7.   Tagless Capability Concepts
* 8.   Extracted Type Concepts
*
* 
* path:      /inc/djinterp/core/db/database_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.01
******************************************************************************/

#ifndef DJINTERP_DATABASE_CONCEPTS_
#define DJINTERP_DATABASE_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "database_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "database_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_DATABASE

// =============================================================================
// I.   Connection Concepts
// =============================================================================

// connection
//   concept: constrains types implementing the basic connection interface.
template<typename _Type>
concept connection =
    is_connection<clean_t<_Type>>::value;

// non_connection
//   concept: constrains types that do not implement the basic connection
// interface.
template<typename _Type>
concept non_connection =
    !connection<_Type>;

// connectable_connection
//   concept: constrains types exposing connect().
template<typename _Type>
concept connectable_connection =
    has_connect<clean_t<_Type>>::value;

// disconnectable_connection
//   concept: constrains types exposing disconnect().
template<typename _Type>
concept disconnectable_connection =
    has_disconnect<clean_t<_Type>>::value;

// connection_state_query
//   concept: constrains types exposing is_connected() const.
template<typename _Type>
concept connection_state_query =
    is_detected<is_connected_t, clean_t<_Type>>::value;

// query_connection
//   concept: constrains connections exposing execute_query(const string&).
template<typename _Type>
concept query_connection =
    has_execute_query<clean_t<_Type>>::value;

// update_connection
//   concept: constrains connections exposing execute_update(const string&).
template<typename _Type>
concept update_connection =
    is_detected<execute_update_t, clean_t<_Type>>::value;

// prepared_connection
//   concept: constrains connections exposing prepare(const string&).
template<typename _Type>
concept prepared_connection =
    has_prepared_statements<clean_t<_Type>>::value;


// =============================================================================
// II.  Transaction Concepts
// =============================================================================

// transactional_connection
//   concept: constrains connections supporting begin/commit/rollback.
template<typename _Type>
concept transactional_connection =
    has_transactions<clean_t<_Type>>::value;

// savepoint_connection
//   concept: constrains connections supporting savepoints.
template<typename _Type>
concept savepoint_connection =
    has_savepoints<clean_t<_Type>>::value;

// transaction_state_query
//   concept: constrains connections exposing in_transaction() const.
template<typename _Type>
concept transaction_state_query =
    is_detected<in_transaction_t, clean_t<_Type>>::value;

// begin_transaction_connection
//   concept: constrains connections exposing begin_transaction().
template<typename _Type>
concept begin_transaction_connection =
    is_detected<begin_transaction_t, clean_t<_Type>>::value;

// committable_connection
//   concept: constrains connections exposing commit().
template<typename _Type>
concept committable_connection =
    is_detected<commit_t, clean_t<_Type>>::value;

// rollback_connection
//   concept: constrains connections exposing rollback().
template<typename _Type>
concept rollback_connection =
    is_detected<rollback_t, clean_t<_Type>>::value;

// savepoint_creating_connection
//   concept: constrains connections exposing create_savepoint().
template<typename _Type>
concept savepoint_creating_connection =
    is_detected<create_savepoint_t, clean_t<_Type>>::value;

// savepoint_rollback_connection
//   concept: constrains connections exposing rollback_to_savepoint().
template<typename _Type>
concept savepoint_rollback_connection =
    is_detected<rollback_to_savepoint_t, clean_t<_Type>>::value;


// =============================================================================
// III. Result Set Concepts
// =============================================================================

// result_set
//   concept: constrains types implementing the result-set interface.
template<typename _Type>
concept result_set =
    is_result_set<clean_t<_Type>>::value;

// non_result_set
//   concept: constrains types that do not implement the result-set interface.
template<typename _Type>
concept non_result_set =
    !result_set<_Type>;

// navigable_result_set
//   concept: constrains result sets supporting next().
template<typename _Type>
concept navigable_result_set =
    is_detected<next_t, clean_t<_Type>>::value;

// counted_result_set
//   concept: constrains result sets exposing column_count().
template<typename _Type>
concept counted_result_set =
    is_detected<column_count_t, clean_t<_Type>>::value;

// named_result_set
//   concept: constrains result sets exposing column_name(size_t).
template<typename _Type>
concept named_result_set =
    is_detected<column_name_t, clean_t<_Type>>::value;

// index_addressable_result_set
//   concept: constrains result sets exposing get_value(size_t).
template<typename _Type>
concept index_addressable_result_set =
    is_detected<get_value_index_t, clean_t<_Type>>::value;

// key_addressable_result_set
//   concept: constrains result sets exposing get_value(const string&).
template<typename _Type>
concept key_addressable_result_set =
    is_detected<get_value_name_t, clean_t<_Type>>::value;


// =============================================================================
// IV.  Statement Concepts
// =============================================================================

// statement
//   concept: constrains types implementing the prepared-statement interface.
template<typename _Type>
concept statement =
    is_statement<clean_t<_Type>>::value;

// non_statement
//   concept: constrains types that do not implement the statement interface.
template<typename _Type>
concept non_statement =
    !statement<_Type>;

// int_bindable_statement
//   concept: constrains statements exposing bind_int().
template<typename _Type>
concept int_bindable_statement =
    is_detected<bind_int_t, clean_t<_Type>>::value;

// string_bindable_statement
//   concept: constrains statements exposing bind_string().
template<typename _Type>
concept string_bindable_statement =
    is_detected<bind_string_t, clean_t<_Type>>::value;

// executable_statement
//   concept: constrains statements exposing execute().
template<typename _Type>
concept executable_statement =
    is_detected<execute_t, clean_t<_Type>>::value;

// parameterized_statement
//   concept: constrains statements exposing parameter_count() const.
template<typename _Type>
concept parameterized_statement =
    is_detected<parameter_count_t, clean_t<_Type>>::value;


// =============================================================================
// V.   Vendor Connection Concepts
// =============================================================================

// vendor_connection
//   concept: constrains types implementing the full vendor connection
// interface.
template<typename _Type>
concept vendor_connection =
    is_vendor_connection<clean_t<_Type>>::value;

// pingable_connection
//   concept: constrains connections exposing ping().
template<typename _Type>
concept pingable_connection =
    has_ping<clean_t<_Type>>::value;

// metadata_connection
//   concept: constrains connections exposing vendor metadata.
template<typename _Type>
concept metadata_connection =
    has_metadata<clean_t<_Type>>::value;

// error_reporting_connection
//   concept: constrains connections exposing error reporting.
template<typename _Type>
concept error_reporting_connection =
    has_error_reporting<clean_t<_Type>>::value;

// native_handle_connection
//   concept: constrains connections exposing a native handle.
template<typename _Type>
concept native_handle_connection =
    has_native_handle<clean_t<_Type>>::value;

// reconnectable_connection
//   concept: constrains connections exposing reconnect().
template<typename _Type>
concept reconnectable_connection =
    has_reconnect<clean_t<_Type>>::value;

// row_info_connection
//   concept: constrains connections exposing row mutation information.
template<typename _Type>
concept row_info_connection =
    has_row_info<clean_t<_Type>>::value;


// =============================================================================
// VI.  Tagless Capability Concepts
// =============================================================================

// can_connect_connection
//   concept: constrains types satisfying the tagless can_connect capability.
template<typename _Type>
concept can_connect_connection =
    can_connect<clean_t<_Type>>;

// can_disconnect_connection
//   concept: constrains types satisfying the tagless can_disconnect
// capability.
template<typename _Type>
concept can_disconnect_connection =
    can_disconnect<clean_t<_Type>>;

// can_reconnect_connection
//   concept: constrains types satisfying the tagless can_reconnect
// capability.
template<typename _Type>
concept can_reconnect_connection =
    can_reconnect<clean_t<_Type>>;

// can_ping_connection
//   concept: constrains types satisfying the tagless can_ping capability.
template<typename _Type>
concept can_ping_connection =
    can_ping<clean_t<_Type>>;

// can_prepare_connection
//   concept: constrains types satisfying the tagless can_prepare capability.
template<typename _Type>
concept can_prepare_connection =
    can_prepare<clean_t<_Type>>;

// transacting_connection
//   concept: constrains types satisfying the tagless transaction compound
// capability.
template<typename _Type>
concept transacting_connection =
    does_transact<clean_t<_Type>>;

// savepointing_connection
//   concept: constrains types satisfying the tagless savepoint compound
// capability.
template<typename _Type>
concept savepointing_connection =
    does_savepoint<clean_t<_Type>>;

// connectable_database
//   concept: constrains types satisfying the tagless basic connection
// interface.
template<typename _Type>
concept connectable_database =
    is_connectable<clean_t<_Type>>;

// full_vendor_database
//   concept: constrains types satisfying the tagless full vendor interface.
template<typename _Type>
concept full_vendor_database =
    is_full_vendor<clean_t<_Type>>;

// error_reporting_database
//   concept: constrains types satisfying the tagless error-reporting
// capability.
template<typename _Type>
concept error_reporting_database =
    does_report_errors<clean_t<_Type>>;

// native_handle_database
//   concept: constrains types satisfying the tagless native-handle
// capability.
template<typename _Type>
concept native_handle_database =
    does_expose_native_handle<clean_t<_Type>>;

// row_info_database
//   concept: constrains types satisfying the tagless row-info capability.
template<typename _Type>
concept row_info_database =
    does_report_row_info<clean_t<_Type>>;

// version_reporting_database
//   concept: constrains types satisfying the tagless server-version
// capability.
template<typename _Type>
concept version_reporting_database =
    does_report_version<clean_t<_Type>>;

// navigable_database_result
//   concept: constrains types satisfying the tagless result-set interface.
template<typename _Type>
concept navigable_database_result =
    is_navigable_result<clean_t<_Type>>;

// bindable_database_statement
//   concept: constrains types satisfying the tagless statement interface.
template<typename _Type>
concept bindable_database_statement =
    is_bindable_statement<clean_t<_Type>>;


// =============================================================================
// VII. Extracted Type Concepts
// =============================================================================

// query_result_connection
//   concept: constrains connections whose execute_query() return type yields
// an extracted result-set type.
template<typename _Type>
concept query_result_connection =
    requires
    {
        typename result_set_type<clean_t<_Type>>::type;
    } && result_set<result_set_type_t<clean_t<_Type>>>;

// prepared_statement_connection
//   concept: constrains connections whose prepare() return type yields an
// extracted statement type.
template<typename _Type>
concept prepared_statement_connection =
    requires
    {
        typename statement_type<clean_t<_Type>>::type;
    } && statement<statement_type_t<clean_t<_Type>>>;

// extracted_result_connection
//   concept: constrains query connections whose extracted result type is not
// void.
template<typename _Type>
concept extracted_result_connection =
    ( query_result_connection<_Type> &&
      !std::is_void_v<result_set_type_t<clean_t<_Type>>> );

// extracted_statement_connection
//   concept: constrains prepared connections whose extracted statement type is
// not void.
template<typename _Type>
concept extracted_statement_connection =
    ( prepared_statement_connection<_Type> &&
      !std::is_void_v<statement_type_t<clean_t<_Type>>> );


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CONCEPTS_
