/******************************************************************************
* djinterp [database]                                    mysql_concepts.hpp
*
*  djinterp MySQL-family classification concepts
*   C++20 concepts layered on top of mysql_common_traits.hpp.  These concepts
* provide readable `requires` constraints for MySQL-family database
* connections, including character-set management, multi-result iteration,
* diagnostics, result-mode selection, options, escaping, engine queries,
* auto-commit control, database selection, and runtime user switching.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the MySQL-family trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core MySQL-family Connection Concepts
* 3.   MySQL-family Capability Concepts
* 4.   Tagless MySQL-family Capability Concepts
*
* path:      /inc/database/mysql/mysql_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                 date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_CONCEPTS_
#define DJINTERP_DATABASE_MYSQL_CONCEPTS_ 1

#include <type_traits>
#include "mysql_common_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "mysql_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP
NS_DB
NS_MYSQL_COMMON

// =============================================================================
// I.   Core MySQL-family Connection Concepts
// =============================================================================

// mysql_connection
//   concept: constrains types implementing the MySQL-family connection
// interface.
template<typename _Type>
concept mysql_connection =
    is_mysql_connection<clean_t<_Type>>::value;

// non_mysql_connection
//   concept: constrains types that do not implement the MySQL-family
// connection interface.
template<typename _Type>
concept non_mysql_connection =
    !mysql_connection<_Type>;

// mysql_charset_connection
//   concept: constrains MySQL-family connections supporting character-set
// management.
template<typename _Type>
concept mysql_charset_connection =
    has_mysql_charset<clean_t<_Type>>::value;

// mysql_multi_result_connection
//   concept: constrains MySQL-family connections supporting multi-result
// iteration.
template<typename _Type>
concept mysql_multi_result_connection =
    has_mysql_multi_result<clean_t<_Type>>::value;

// mysql_diagnostics_connection
//   concept: constrains MySQL-family connections supporting MySQL-family
// server diagnostics.
template<typename _Type>
concept mysql_diagnostics_connection =
    has_mysql_diagnostics<clean_t<_Type>>::value;

// mysql_result_modes_connection
//   concept: constrains MySQL-family connections supporting both buffered and
// streaming result modes.
template<typename _Type>
concept mysql_result_modes_connection =
    has_mysql_result_modes<clean_t<_Type>>::value;

// mysql_escape_connection
//   concept: constrains MySQL-family connections supporting SQL string
// escaping.
template<typename _Type>
concept mysql_escape_connection =
    has_mysql_escape<clean_t<_Type>>::value;

// mysql_options_connection
//   concept: constrains MySQL-family connections supporting the options API.
template<typename _Type>
concept mysql_options_connection =
    has_mysql_options<clean_t<_Type>>::value;

// mysql_engine_connection
//   concept: constrains MySQL-family connections supporting storage-engine
// queries.
template<typename _Type>
concept mysql_engine_connection =
    has_mysql_engine<clean_t<_Type>>::value;


// =============================================================================
// II.  MySQL-family Capability Concepts
// =============================================================================

// mysql_select_db_connection
//   concept: constrains types exposing select_db(database).
template<typename _Type>
concept mysql_select_db_connection =
    is_detected<mysql_select_db_t, clean_t<_Type>>::value;

// mysql_autocommit_connection
//   concept: constrains types exposing set_autocommit(bool).
template<typename _Type>
concept mysql_autocommit_connection =
    is_detected<mysql_set_autocommit_t, clean_t<_Type>>::value;

// mysql_set_option_connection
//   concept: constrains types exposing set_option(option, value).
template<typename _Type>
concept mysql_set_option_connection =
    mysql_can_set_option<clean_t<_Type>>;

// mysql_get_option_connection
//   concept: constrains types exposing get_option(option, value).
template<typename _Type>
concept mysql_get_option_connection =
    is_detected<mysql_get_option_t, clean_t<_Type>>::value;

// mysql_stat_connection
//   concept: constrains types exposing get_stat().
template<typename _Type>
concept mysql_stat_connection =
    mysql_can_get_stat<clean_t<_Type>>;

// mysql_thread_id_connection
//   concept: constrains types exposing get_thread_id().
template<typename _Type>
concept mysql_thread_id_connection =
    is_detected<mysql_get_thread_id_t, clean_t<_Type>>::value;

// mysql_warning_count_connection
//   concept: constrains types exposing get_warning_count().
template<typename _Type>
concept mysql_warning_count_connection =
    is_detected<mysql_get_warning_count_t, clean_t<_Type>>::value;

// mysql_sqlstate_connection
//   concept: constrains types exposing get_sqlstate().
template<typename _Type>
concept mysql_sqlstate_connection =
    mysql_can_get_sqlstate<clean_t<_Type>>;

// mysql_store_result_connection
//   concept: constrains types exposing store_result().
template<typename _Type>
concept mysql_store_result_connection =
    mysql_can_store_result<clean_t<_Type>>;

// mysql_use_result_connection
//   concept: constrains types exposing use_result().
template<typename _Type>
concept mysql_use_result_connection =
    mysql_can_use_result<clean_t<_Type>>;

// mysql_change_user_connection
//   concept: constrains types exposing change_user(user, password,
// database).
template<typename _Type>
concept mysql_change_user_connection =
    has_mysql_change_user<clean_t<_Type>>::value;

// mysql_set_engine_connection
//   concept: constrains types exposing set_engine(name).
template<typename _Type>
concept mysql_set_engine_connection =
    is_detected<mysql_set_engine_t, clean_t<_Type>>::value;

// mysql_get_engine_connection
//   concept: constrains types exposing get_engine().
template<typename _Type>
concept mysql_get_engine_connection =
    is_detected<mysql_get_engine_t, clean_t<_Type>>::value;


// =============================================================================
// III. Tagless MySQL-family Capability Concepts
// =============================================================================

// mysql_charset_manageable
//   concept: constrains types satisfying the tagless charset capability set.
template<typename _Type>
concept mysql_charset_manageable =
    mysql_does_charset<clean_t<_Type>>;

// mysql_multi_result_iterable
//   concept: constrains types satisfying the tagless multi-result
// capability set.
template<typename _Type>
concept mysql_multi_result_iterable =
    mysql_does_multi_result<clean_t<_Type>>;

// mysql_result_mode_selectable
//   concept: constrains types satisfying the tagless result-mode capability
// set.
template<typename _Type>
concept mysql_result_mode_selectable =
    mysql_does_result_modes<clean_t<_Type>>;

// mysql_diagnostic_connection_tagless
//   concept: constrains types satisfying the tagless diagnostics capability
// set.
template<typename _Type>
concept mysql_diagnostic_connection_tagless =
    mysql_does_diagnostics<clean_t<_Type>>;

// mysql_charset_settable
//   concept: constrains types satisfying the tagless set-charset capability.
template<typename _Type>
concept mysql_charset_settable =
    mysql_can_set_charset<clean_t<_Type>>;

// mysql_charset_gettable
//   concept: constrains types satisfying the tagless get-charset capability.
template<typename _Type>
concept mysql_charset_gettable =
    mysql_can_get_charset<clean_t<_Type>>;

// mysql_result_iterable
//   concept: constrains types satisfying the tagless next-result capability.
template<typename _Type>
concept mysql_result_iterable =
    mysql_can_iterate_results<clean_t<_Type>>;

// mysql_more_results_query
//   concept: constrains types satisfying the tagless more-results capability.
template<typename _Type>
concept mysql_more_results_query =
    mysql_can_check_more_results<clean_t<_Type>>;

// mysql_string_escapable
//   concept: constrains types satisfying the tagless escape-string
// capability.
template<typename _Type>
concept mysql_string_escapable =
    mysql_can_escape_string<clean_t<_Type>>;

// mysql_database_selectable
//   concept: constrains types satisfying the tagless select-db capability.
template<typename _Type>
concept mysql_database_selectable =
    mysql_can_select_db<clean_t<_Type>>;

// mysql_user_switchable
//   concept: constrains types satisfying the tagless change-user capability.
template<typename _Type>
concept mysql_user_switchable =
    mysql_can_change_user<clean_t<_Type>>;

// mysql_full_connection
//   concept: constrains types satisfying the complete tagless MySQL-family
// connection capability set.
template<typename _Type>
concept mysql_full_connection =
    mysql_is_full_connection<clean_t<_Type>>;


NS_END  // mysql_common
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_CONCEPTS_
