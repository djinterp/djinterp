/******************************************************************************
* djinterp [database]                                  mysql_common_traits.hpp
* 
* djinterp MySQL-family common traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to the MySQL-compatible database family
* (Oracle MySQL and MariaDB). It extends database_traits.hpp with
* detectors for MySQL-family C API patterns, including:
*   - character set configuration (set_charset, get_charset)
*   - multi-result set iteration (next_result, more_results)
*   - MySQL-specific options API (set_option, get_option)
*   - server statistics and diagnostics (get_stat, get_thread_id,
*     get_warning_count, get_sqlstate)
*   - streaming vs buffered result set selection
*   - auto-commit mode management
*   - client flags and connect attributes
*   - MySQL-specific escape (escape_string)
*   - storage engine queries (get_engine, set_engine)
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided. Tagged traits follow the has_/is_ naming convention with
* _v variable template aliases. Tagless traits use can_/does_ prefixes
* and resolve directly to constexpr bool without a struct wrapper.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17
* (variable template partial specialization over void_t). The database
* module gates on C++17 through database.hpp.
*
*   NAMING CONVENTION:
*   Expression detectors:   mysql_<method>_t
*   Struct-based traits:    has_mysql_<capability>
*   Variable template _v:   has_mysql_<capability>_v
*   Tagless traits:          mysql_can_<action>
*   Compound tagless traits: mysql_does_<category>
*
* 
* path:      /inc/djinterp/core/db/mysql/mysql_common_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_COMMON_TRAITS_
#define DJINTERP_DATABASE_MYSQL_COMMON_TRAITS_

// djinterp
#include "../../../djinterp.hpp"
#include "../database_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   EXPRESSION DETECTORS
// ===========================================================================
// Expression alias templates for SFINAE-based detection of MySQL-family
// specific methods. These follow the same pattern as the generic
// detectors in database_traits.hpp but target MySQL C API wrapper
// methods.

// -------------------------------------------------------------------------
// A.  character set management
// -------------------------------------------------------------------------

// mysql_set_charset_t
//   detector: set_charset(const std::string&) method.
template<typename _Type>
using mysql_set_charset_t = decltype(std::declval<_Type&>().set_charset(
    std::declval<const std::string&>()));

// mysql_get_charset_t
//   detector: get_charset() const method.
template<typename _Type>
using mysql_get_charset_t =
    decltype(std::declval<const _Type&>().get_charset());

// -------------------------------------------------------------------------
// B.  multi-result set iteration
// -------------------------------------------------------------------------

// mysql_next_result_t
//   detector: next_result() method.
// wraps mysql_next_result() for iterating over multiple result sets
// from multi-statement queries or stored procedures.
template<typename _Type>
using mysql_next_result_t =
    decltype(std::declval<_Type&>().next_result());

// mysql_more_results_t
//   detector: more_results() const method.
// wraps mysql_more_results() to check if additional result sets
// remain.
template<typename _Type>
using mysql_more_results_t =
    decltype(std::declval<const _Type&>().more_results());

// -------------------------------------------------------------------------
// C.  options API
// -------------------------------------------------------------------------

// mysql_set_option_t
//   detector: set_option(int, const void*) method.
// wraps mysql_options() for setting connection options before
// connecting.
template<typename _Type>
using mysql_set_option_t = decltype(std::declval<_Type&>().set_option(
    std::declval<int>(),
    std::declval<const void*>()));

// mysql_get_option_t
//   detector: get_option(int, void*) const method.
// wraps mysql_get_option() for querying connection option values.
template<typename _Type>
using mysql_get_option_t = decltype(std::declval<const _Type&>().get_option(
    std::declval<int>(),
    std::declval<void*>()));

// -------------------------------------------------------------------------
// D.  server diagnostics
// -------------------------------------------------------------------------

// mysql_get_stat_t
//   detector: get_stat() const method.
// wraps mysql_stat() for server status string.
template<typename _Type>
using mysql_get_stat_t =
    decltype(std::declval<const _Type&>().get_stat());

// mysql_get_thread_id_t
//   detector: get_thread_id() const method.
// wraps mysql_thread_id() for the connection's thread identifier.
template<typename _Type>
using mysql_get_thread_id_t =
    decltype(std::declval<const _Type&>().get_thread_id());

// mysql_get_warning_count_t
//   detector: get_warning_count() const method.
// wraps mysql_warning_count().
template<typename _Type>
using mysql_get_warning_count_t =
    decltype(std::declval<const _Type&>().get_warning_count());

// mysql_get_sqlstate_t
//   detector: get_sqlstate() const method.
// wraps mysql_sqlstate() for SQLSTATE error code access.
template<typename _Type>
using mysql_get_sqlstate_t =
    decltype(std::declval<const _Type&>().get_sqlstate());

// -------------------------------------------------------------------------
// E.  result set mode
// -------------------------------------------------------------------------

// mysql_store_result_t
//   detector: store_result() method.
// wraps mysql_store_result() for buffered result sets.
template<typename _Type>
using mysql_store_result_t =
    decltype(std::declval<_Type&>().store_result());

// mysql_use_result_t
//   detector: use_result() method.
// wraps mysql_use_result() for streaming (unbuffered) result sets.
template<typename _Type>
using mysql_use_result_t =
    decltype(std::declval<_Type&>().use_result());

// -------------------------------------------------------------------------
// F.  auto-commit
// -------------------------------------------------------------------------

// mysql_set_autocommit_t
//   detector: set_autocommit(bool) method.
// wraps mysql_autocommit().
template<typename _Type>
using mysql_set_autocommit_t = decltype(std::declval<_Type&>().set_autocommit(
    std::declval<bool>()));

// -------------------------------------------------------------------------
// G.  escape and select
// -------------------------------------------------------------------------

// mysql_escape_string_t
//   detector: escape_string(const std::string&) const method.
// wraps mysql_real_escape_string().
template<typename _Type>
using mysql_escape_string_t =
    decltype(std::declval<const _Type&>().escape_string(
        std::declval<const std::string&>()));

// mysql_select_db_t
//   detector: select_db(const std::string&) method.
// wraps mysql_select_db() for switching the active database.
template<typename _Type>
using mysql_select_db_t = decltype(std::declval<_Type&>().select_db(
    std::declval<const std::string&>()));

// mysql_change_user_t
//   detector: change_user(const std::string&, const std::string&,
// const std::string&) method. wraps mysql_change_user().
template<typename _Type>
using mysql_change_user_t = decltype(std::declval<_Type&>().change_user(
    std::declval<const std::string&>(),
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// H.  storage engine
// -------------------------------------------------------------------------

// mysql_get_engine_t
//   detector: get_engine() const method.
// returns the default storage engine name.
template<typename _Type>
using mysql_get_engine_t =
    decltype(std::declval<const _Type&>().get_engine());

// mysql_set_engine_t
//   detector: set_engine(const std::string&) method.
// sets the session default storage engine.
template<typename _Type>
using mysql_set_engine_t = decltype(std::declval<_Type&>().set_engine(
    std::declval<const std::string&>()));


// ===========================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// ===========================================================================

// has_mysql_charset
//   trait: checks if type _Type supports character set management
// (set_charset + get_charset).
template<typename _Type>
struct has_mysql_charset : djinterp::conjunction<
    is_detected<mysql_set_charset_t, _Type>,
    is_detected<mysql_get_charset_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_charset_v
    //   value: convenience alias for has_mysql_charset<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_charset_v = has_mysql_charset<_Type>::value;
#endif

// has_mysql_multi_result
//   trait: checks if type _Type supports multi-result set iteration
// (next_result + more_results).
template<typename _Type>
struct has_mysql_multi_result : djinterp::conjunction<
    is_detected<mysql_next_result_t, _Type>,
    is_detected<mysql_more_results_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_multi_result_v
    //   value: convenience alias for has_mysql_multi_result<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_multi_result_v =
        has_mysql_multi_result<_Type>::value;
#endif

// has_mysql_options
//   trait: checks if type _Type supports the MySQL options API
// (set_option + get_option).
template<typename _Type>
struct has_mysql_options : djinterp::conjunction<
    is_detected<mysql_set_option_t, _Type>,
    is_detected<mysql_get_option_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_options_v
    //   value: convenience alias for has_mysql_options<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_options_v = has_mysql_options<_Type>::value;
#endif

// has_mysql_diagnostics
//   trait: checks if type _Type supports MySQL server diagnostics
// (get_stat + get_thread_id + get_warning_count + get_sqlstate).
template<typename _Type>
struct has_mysql_diagnostics : djinterp::conjunction<
    is_detected<mysql_get_stat_t, _Type>,
    is_detected<mysql_get_thread_id_t, _Type>,
    is_detected<mysql_get_warning_count_t, _Type>,
    is_detected<mysql_get_sqlstate_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_diagnostics_v
    //   value: convenience alias for has_mysql_diagnostics<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_diagnostics_v =
        has_mysql_diagnostics<_Type>::value;
#endif

// has_mysql_result_modes
//   trait: checks if type _Type supports both buffered and streaming
// result set modes (store_result + use_result).
template<typename _Type>
struct has_mysql_result_modes : djinterp::conjunction<
    is_detected<mysql_store_result_t, _Type>,
    is_detected<mysql_use_result_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_result_modes_v
    //   value: convenience alias for has_mysql_result_modes<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_result_modes_v =
        has_mysql_result_modes<_Type>::value;
#endif

// has_mysql_escape
//   trait: checks if type _Type supports MySQL string escaping.
template<typename _Type>
struct has_mysql_escape : is_detected<mysql_escape_string_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_escape_v
    //   value: convenience alias for has_mysql_escape<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_escape_v = has_mysql_escape<_Type>::value;
#endif

// has_mysql_change_user
//   trait: checks if type _Type supports runtime user switching.
template<typename _Type>
struct has_mysql_change_user : is_detected<mysql_change_user_t, _Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_change_user_v
    //   value: convenience alias for has_mysql_change_user<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_change_user_v =
        has_mysql_change_user<_Type>::value;
#endif

// has_mysql_engine
//   trait: checks if type _Type supports storage engine queries
// (get_engine + set_engine).
template<typename _Type>
struct has_mysql_engine : djinterp::conjunction<
    is_detected<mysql_get_engine_t, _Type>,
    is_detected<mysql_set_engine_t, _Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_engine_v
    //   value: convenience alias for has_mysql_engine<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_engine_v = has_mysql_engine<_Type>::value;
#endif

// is_mysql_connection
//   trait: compound trait verifying type _Type implements a MySQL-
// family connection interface (vendor connection + charset +
// multi-result + diagnostics + escape).
template<typename _Type>
struct is_mysql_connection : djinterp::conjunction<
    is_vendor_connection<_Type>,
    has_mysql_charset<_Type>,
    has_mysql_multi_result<_Type>,
    has_mysql_diagnostics<_Type>,
    has_mysql_escape<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_mysql_connection_v
    //   value: convenience alias for is_mysql_connection<_Type>::value.
    template<typename _Type>
    constexpr bool is_mysql_connection_v =
        is_mysql_connection<_Type>::value;
#endif


// ===========================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// ===========================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// mysql_can_set_charset
//   tagless trait: true if _Type has a set_charset() method.
template<typename _Type,
         typename = void>
constexpr bool mysql_can_set_charset = false;

template<typename _Type>
constexpr bool mysql_can_set_charset<_Type,
    std::void_t<mysql_set_charset_t<_Type>>> = true;

// mysql_can_get_charset
//   tagless trait: true if _Type has a get_charset() method.
template<typename _Type,
         typename = void>
constexpr bool mysql_can_get_charset = false;

template<typename _Type>
constexpr bool mysql_can_get_charset<_Type,
    std::void_t<mysql_get_charset_t<_Type>>> = true;

// mysql_can_iterate_results
//   tagless trait: true if _Type has next_result() for multi-result
// iteration.
template<typename _Type,
         typename = void>
constexpr bool mysql_can_iterate_results = false;

template<typename _Type>
constexpr bool mysql_can_iterate_results<_Type,
    std::void_t<mysql_next_result_t<_Type>>> = true;

// mysql_can_check_more_results
//   tagless trait: true if _Type has more_results().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_check_more_results = false;

template<typename _Type>
constexpr bool mysql_can_check_more_results<_Type,
    std::void_t<mysql_more_results_t<_Type>>> = true;

// mysql_can_set_option
//   tagless trait: true if _Type has set_option().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_set_option = false;

template<typename _Type>
constexpr bool mysql_can_set_option<_Type,
    std::void_t<mysql_set_option_t<_Type>>> = true;

// mysql_can_get_stat
//   tagless trait: true if _Type has get_stat().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_get_stat = false;

template<typename _Type>
constexpr bool mysql_can_get_stat<_Type,
    std::void_t<mysql_get_stat_t<_Type>>> = true;

// mysql_can_get_sqlstate
//   tagless trait: true if _Type has get_sqlstate().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_get_sqlstate = false;

template<typename _Type>
constexpr bool mysql_can_get_sqlstate<_Type,
    std::void_t<mysql_get_sqlstate_t<_Type>>> = true;

// mysql_can_store_result
//   tagless trait: true if _Type has store_result().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_store_result = false;

template<typename _Type>
constexpr bool mysql_can_store_result<_Type,
    std::void_t<mysql_store_result_t<_Type>>> = true;

// mysql_can_use_result
//   tagless trait: true if _Type has use_result() (streaming).
template<typename _Type,
         typename = void>
constexpr bool mysql_can_use_result = false;

template<typename _Type>
constexpr bool mysql_can_use_result<_Type,
    std::void_t<mysql_use_result_t<_Type>>> = true;

// mysql_can_escape_string
//   tagless trait: true if _Type has escape_string().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_escape_string = false;

template<typename _Type>
constexpr bool mysql_can_escape_string<_Type,
    std::void_t<mysql_escape_string_t<_Type>>> = true;

// mysql_can_select_db
//   tagless trait: true if _Type has select_db().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_select_db = false;

template<typename _Type>
constexpr bool mysql_can_select_db<_Type,
    std::void_t<mysql_select_db_t<_Type>>> = true;

// mysql_can_change_user
//   tagless trait: true if _Type has change_user().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_change_user = false;

template<typename _Type>
constexpr bool mysql_can_change_user<_Type,
    std::void_t<mysql_change_user_t<_Type>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// mysql_does_charset
//   tagless trait: true if _Type supports full charset management
// (set + get).
template<typename _Type>
constexpr bool mysql_does_charset =
    ( mysql_can_set_charset<_Type> &&
      mysql_can_get_charset<_Type> );

// mysql_does_multi_result
//   tagless trait: true if _Type supports multi-result iteration
// (next_result + more_results).
template<typename _Type>
constexpr bool mysql_does_multi_result =
    ( mysql_can_iterate_results<_Type>    &&
      mysql_can_check_more_results<_Type> );

// mysql_does_result_modes
//   tagless trait: true if _Type supports both buffered and streaming
// result sets.
template<typename _Type>
constexpr bool mysql_does_result_modes =
    ( mysql_can_store_result<_Type> &&
      mysql_can_use_result<_Type> );

// mysql_does_diagnostics
//   tagless trait: true if _Type supports server diagnostics
// (stat + sqlstate).
template<typename _Type>
constexpr bool mysql_does_diagnostics =
    ( mysql_can_get_stat<_Type>     &&
      mysql_can_get_sqlstate<_Type> );

// mysql_is_full_connection
//   tagless trait: true if _Type satisfies the complete MySQL-family
// connection interface (full vendor + charset + multi-result +
// diagnostics + escape).
template<typename _Type>
constexpr bool mysql_is_full_connection =
    ( is_full_vendor<_Type>              &&
      mysql_does_charset<_Type>          &&
      mysql_does_multi_result<_Type>     &&
      mysql_does_diagnostics<_Type>      &&
      mysql_can_escape_string<_Type> );


// ===========================================================================
// IV.  SFINAE HELPERS
// ===========================================================================

// enable_if_mysql_connection
//   type: SFINAE helper for MySQL-family connection constraints.
template<typename _Type>
using enable_if_mysql_connection =
    typename std::enable_if<is_mysql_connection<_Type>::value>::type;

// enable_if_has_mysql_charset
//   type: SFINAE helper for MySQL charset constraints.
template<typename _Type>
using enable_if_has_mysql_charset =
    typename std::enable_if<has_mysql_charset<_Type>::value>::type;

// enable_if_has_mysql_multi_result
//   type: SFINAE helper for MySQL multi-result constraints.
template<typename _Type>
using enable_if_has_mysql_multi_result =
    typename std::enable_if<has_mysql_multi_result<_Type>::value>::type;

// enable_if_has_mysql_diagnostics
//   type: SFINAE helper for MySQL diagnostics constraints.
template<typename _Type>
using enable_if_has_mysql_diagnostics =
    typename std::enable_if<has_mysql_diagnostics<_Type>::value>::type;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_COMMON_TRAITS_
