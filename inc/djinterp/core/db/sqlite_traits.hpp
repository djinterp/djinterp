/******************************************************************************
* djinterp [database]                                      sqlite_traits.hpp
* 
* djinterp SQLite traits module:
*   This header provides compile-time type traits and SFINAE utilities for
* detecting capabilities specific to SQLite connections, including:
*   - file-based connection management (open, open_v2, close)
*   - journal mode / WAL management (set_journal_mode, checkpoint)
*   - PRAGMA interface (execute_pragma, get_pragma)
*   - backup API (backup_to, backup_from)
*   - busy handler and timeout configuration
*   - database attachment (attach, detach)
*   - table existence and schema introspection queries
*   - user-defined function registration
*   - transaction mode awareness (deferred, immediate, exclusive)
*   - serialization/deserialization support
*   - extension loading
*
*   Both tagged (struct-based) and tagless (constexpr bool) forms are
* provided. Because SQLite is embedded (no client/server split), some
* generic detectors from database_traits.hpp (e.g. ping, reconnect)
* do not apply. This module adds detectors specific to the sqlite3 C
* API wrapper methods.
*
*   PORTABILITY:
*   Tagged traits are available in C++11+. Tagless traits require C++17
* (variable template partial specialization over void_t).
*
*   NAMING CONVENTION:
*   Expression detectors:   sqlite_<method>_t
*   Struct-based traits:    has_sqlite_<capability>
*   Variable template _v:   has_sqlite_<capability>_v
*   Tagless traits:          sqlite_can_<action>
*   Compound tagless traits: sqlite_does_<category>
*
* path:      \inc\database\sqlite\sqlite_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_DATABASE_SQLITE_TRAITS_
#define DJINTERP_DATABASE_SQLITE_TRAITS_

#include "database_traits.hpp"


NS_DJINTERP
NS_DB

// =========================================================================
//  NS_SQLITE
// =========================================================================

#ifndef D_KEYWORD_SQLITE
    #define D_KEYWORD_SQLITE    sqlite
#endif

#ifndef NS_SQLITE
    #define NS_SQLITE           D_NAMESPACE(D_KEYWORD_SQLITE)
#endif

NS_SQLITE


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  file-based connection
// -------------------------------------------------------------------------

// sqlite_open_t
//   detector: open(const std::string&) method.
// wraps sqlite3_open() or sqlite3_open_v2().
template<typename _T>
using sqlite_open_t = decltype(std::declval<_T&>().open(
    std::declval<const std::string&>()));

// sqlite_open_v2_t
//   detector: open_v2(const std::string&, int) method.
// wraps sqlite3_open_v2() with flags parameter.
template<typename _T>
using sqlite_open_v2_t = decltype(std::declval<_T&>().open_v2(
    std::declval<const std::string&>(),
    std::declval<int>()));

// sqlite_close_t
//   detector: close() method.
template<typename _T>
using sqlite_close_t = decltype(std::declval<_T&>().close());

// -------------------------------------------------------------------------
// B.  journal and WAL management
// -------------------------------------------------------------------------

// sqlite_set_journal_mode_t
//   detector: set_journal_mode(const std::string&) method.
template<typename _T>
using sqlite_set_journal_mode_t =
    decltype(std::declval<_T&>().set_journal_mode(
        std::declval<const std::string&>()));

// sqlite_get_journal_mode_t
//   detector: get_journal_mode() const method.
template<typename _T>
using sqlite_get_journal_mode_t =
    decltype(std::declval<const _T&>().get_journal_mode());

// sqlite_checkpoint_t
//   detector: checkpoint(int) method.
// wraps sqlite3_wal_checkpoint_v2().
template<typename _T>
using sqlite_checkpoint_t = decltype(std::declval<_T&>().checkpoint(
    std::declval<int>()));

// -------------------------------------------------------------------------
// C.  PRAGMA interface
// -------------------------------------------------------------------------

// sqlite_execute_pragma_t
//   detector: execute_pragma(const std::string&, const std::string&)
// method.
template<typename _T>
using sqlite_execute_pragma_t =
    decltype(std::declval<_T&>().execute_pragma(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// sqlite_get_pragma_t
//   detector: get_pragma(const std::string&) const method.
template<typename _T>
using sqlite_get_pragma_t =
    decltype(std::declval<const _T&>().get_pragma(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// D.  backup API
// -------------------------------------------------------------------------

// sqlite_backup_to_t
//   detector: backup_to(const std::string&) method.
template<typename _T>
using sqlite_backup_to_t = decltype(std::declval<_T&>().backup_to(
    std::declval<const std::string&>()));

// sqlite_backup_from_t
//   detector: backup_from(const std::string&) method.
template<typename _T>
using sqlite_backup_from_t = decltype(std::declval<_T&>().backup_from(
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// E.  busy handler and timeout
// -------------------------------------------------------------------------

// sqlite_set_busy_timeout_t
//   detector: set_busy_timeout(int) method.
template<typename _T>
using sqlite_set_busy_timeout_t =
    decltype(std::declval<_T&>().set_busy_timeout(
        std::declval<int>()));

// -------------------------------------------------------------------------
// F.  database attachment
// -------------------------------------------------------------------------

// sqlite_attach_t
//   detector: attach(const std::string&, const std::string&) method.
template<typename _T>
using sqlite_attach_t = decltype(std::declval<_T&>().attach(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// sqlite_detach_t
//   detector: detach(const std::string&) method.
template<typename _T>
using sqlite_detach_t = decltype(std::declval<_T&>().detach(
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// G.  schema introspection
// -------------------------------------------------------------------------

// sqlite_table_exists_t
//   detector: table_exists(const std::string&) const method.
template<typename _T>
using sqlite_table_exists_t =
    decltype(std::declval<const _T&>().table_exists(
        std::declval<const std::string&>()));

// sqlite_get_table_names_t
//   detector: get_table_names() const method.
template<typename _T>
using sqlite_get_table_names_t =
    decltype(std::declval<const _T&>().get_table_names());

// -------------------------------------------------------------------------
// H.  extension loading
// -------------------------------------------------------------------------

// sqlite_load_extension_t
//   detector: load_extension(const std::string&) method.
template<typename _T>
using sqlite_load_extension_t =
    decltype(std::declval<_T&>().load_extension(
        std::declval<const std::string&>()));

// sqlite_enable_load_extension_t
//   detector: enable_load_extension(bool) method.
template<typename _T>
using sqlite_enable_load_extension_t =
    decltype(std::declval<_T&>().enable_load_extension(
        std::declval<bool>()));

// -------------------------------------------------------------------------
// I.  serialization
// -------------------------------------------------------------------------

// sqlite_serialize_t
//   detector: serialize() method.
template<typename _T>
using sqlite_serialize_t =
    decltype(std::declval<_T&>().serialize());

// sqlite_deserialize_t
//   detector: deserialize(const std::vector<std::uint8_t>&) method.
template<typename _T>
using sqlite_deserialize_t = decltype(std::declval<_T&>().deserialize(
    std::declval<const std::vector<std::uint8_t>&>()));

// -------------------------------------------------------------------------
// J.  transaction mode
// -------------------------------------------------------------------------

// sqlite_begin_deferred_t
//   detector: begin_deferred() method.
template<typename _T>
using sqlite_begin_deferred_t =
    decltype(std::declval<_T&>().begin_deferred());

// sqlite_begin_immediate_t
//   detector: begin_immediate() method.
template<typename _T>
using sqlite_begin_immediate_t =
    decltype(std::declval<_T&>().begin_immediate());

// sqlite_begin_exclusive_t
//   detector: begin_exclusive() method.
template<typename _T>
using sqlite_begin_exclusive_t =
    decltype(std::declval<_T&>().begin_exclusive());


// =============================================================================
// II.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_sqlite_journal
//   trait: checks if type _T supports journal mode management
// (set_journal_mode + get_journal_mode).
template<typename _T>
struct has_sqlite_journal : djinterp::conjunction<
    is_detected<sqlite_set_journal_mode_t, _T>,
    is_detected<sqlite_get_journal_mode_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_journal_v = has_sqlite_journal<_T>::value;
#endif

// has_sqlite_pragma
//   trait: checks if type _T supports the PRAGMA interface
// (execute_pragma + get_pragma).
template<typename _T>
struct has_sqlite_pragma : djinterp::conjunction<
    is_detected<sqlite_execute_pragma_t, _T>,
    is_detected<sqlite_get_pragma_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_pragma_v = has_sqlite_pragma<_T>::value;
#endif

// has_sqlite_backup
//   trait: checks if type _T supports the backup API
// (backup_to + backup_from).
template<typename _T>
struct has_sqlite_backup : djinterp::conjunction<
    is_detected<sqlite_backup_to_t, _T>,
    is_detected<sqlite_backup_from_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_backup_v = has_sqlite_backup<_T>::value;
#endif

// has_sqlite_attach
//   trait: checks if type _T supports database attachment
// (attach + detach).
template<typename _T>
struct has_sqlite_attach : djinterp::conjunction<
    is_detected<sqlite_attach_t, _T>,
    is_detected<sqlite_detach_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_attach_v = has_sqlite_attach<_T>::value;
#endif

// has_sqlite_schema_query
//   trait: checks if type _T supports schema introspection
// (table_exists + get_table_names).
template<typename _T>
struct has_sqlite_schema_query : djinterp::conjunction<
    is_detected<sqlite_table_exists_t, _T>,
    is_detected<sqlite_get_table_names_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_schema_query_v =
        has_sqlite_schema_query<_T>::value;
#endif

// has_sqlite_extension_loading
//   trait: checks if type _T supports extension loading
// (load_extension + enable_load_extension).
template<typename _T>
struct has_sqlite_extension_loading : djinterp::conjunction<
    is_detected<sqlite_load_extension_t, _T>,
    is_detected<sqlite_enable_load_extension_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_extension_loading_v =
        has_sqlite_extension_loading<_T>::value;
#endif

// has_sqlite_serialization
//   trait: checks if type _T supports serialization
// (serialize + deserialize).
template<typename _T>
struct has_sqlite_serialization : djinterp::conjunction<
    is_detected<sqlite_serialize_t, _T>,
    is_detected<sqlite_deserialize_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_serialization_v =
        has_sqlite_serialization<_T>::value;
#endif

// has_sqlite_transaction_modes
//   trait: checks if type _T supports SQLite transaction modes
// (deferred + immediate + exclusive).
template<typename _T>
struct has_sqlite_transaction_modes : djinterp::conjunction<
    is_detected<sqlite_begin_deferred_t, _T>,
    is_detected<sqlite_begin_immediate_t, _T>,
    is_detected<sqlite_begin_exclusive_t, _T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_transaction_modes_v =
        has_sqlite_transaction_modes<_T>::value;
#endif

// is_sqlite_connection
//   trait: compound trait verifying type _T implements a SQLite
// connection interface (connection + journal + pragma + schema
// queries + transaction modes).
template<typename _T>
struct is_sqlite_connection : djinterp::conjunction<
    has_connect<_T>,
    has_disconnect<_T>,
    has_execute_query<_T>,
    has_sqlite_journal<_T>,
    has_sqlite_pragma<_T>,
    has_sqlite_schema_query<_T>,
    has_sqlite_transaction_modes<_T>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool is_sqlite_connection_v =
        is_sqlite_connection<_T>::value;
#endif


// =============================================================================
// III. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// sqlite_can_open_v2
//   tagless trait: true if _T has open_v2() with flags.
template<typename _T,
         typename = void>
constexpr bool sqlite_can_open_v2 = false;

template<typename _T>
constexpr bool sqlite_can_open_v2<_T,
    std::void_t<sqlite_open_v2_t<_T>>> = true;

// sqlite_can_set_journal_mode
//   tagless trait: true if _T has set_journal_mode().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_set_journal_mode = false;

template<typename _T>
constexpr bool sqlite_can_set_journal_mode<_T,
    std::void_t<sqlite_set_journal_mode_t<_T>>> = true;

// sqlite_can_checkpoint
//   tagless trait: true if _T has checkpoint().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_checkpoint = false;

template<typename _T>
constexpr bool sqlite_can_checkpoint<_T,
    std::void_t<sqlite_checkpoint_t<_T>>> = true;

// sqlite_can_execute_pragma
//   tagless trait: true if _T has execute_pragma().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_execute_pragma = false;

template<typename _T>
constexpr bool sqlite_can_execute_pragma<_T,
    std::void_t<sqlite_execute_pragma_t<_T>>> = true;

// sqlite_can_backup
//   tagless trait: true if _T has backup_to().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_backup = false;

template<typename _T>
constexpr bool sqlite_can_backup<_T,
    std::void_t<sqlite_backup_to_t<_T>>> = true;

// sqlite_can_attach
//   tagless trait: true if _T has attach().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_attach = false;

template<typename _T>
constexpr bool sqlite_can_attach<_T,
    std::void_t<sqlite_attach_t<_T>>> = true;

// sqlite_can_load_extension
//   tagless trait: true if _T has load_extension().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_load_extension = false;

template<typename _T>
constexpr bool sqlite_can_load_extension<_T,
    std::void_t<sqlite_load_extension_t<_T>>> = true;

// sqlite_can_serialize
//   tagless trait: true if _T has serialize().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_serialize = false;

template<typename _T>
constexpr bool sqlite_can_serialize<_T,
    std::void_t<sqlite_serialize_t<_T>>> = true;

// sqlite_can_set_busy_timeout
//   tagless trait: true if _T has set_busy_timeout().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_set_busy_timeout = false;

template<typename _T>
constexpr bool sqlite_can_set_busy_timeout<_T,
    std::void_t<sqlite_set_busy_timeout_t<_T>>> = true;

// sqlite_can_query_schema
//   tagless trait: true if _T has table_exists().
template<typename _T,
         typename = void>
constexpr bool sqlite_can_query_schema = false;

template<typename _T>
constexpr bool sqlite_can_query_schema<_T,
    std::void_t<sqlite_table_exists_t<_T>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// sqlite_does_journal
//   tagless trait: true if _T supports full journal mode management.
template<typename _T>
constexpr bool sqlite_does_journal =
    ( sqlite_can_set_journal_mode<_T> &&
      sqlite_can_checkpoint<_T> );

// sqlite_does_pragma
//   tagless trait: true if _T supports full PRAGMA interface.
template<typename _T,
         typename = void>
constexpr bool sqlite_does_pragma = false;

template<typename _T>
constexpr bool sqlite_does_pragma<_T, std::void_t<
    sqlite_execute_pragma_t<_T>,
    sqlite_get_pragma_t<_T>>> = true;

// sqlite_does_backup
//   tagless trait: true if _T supports full backup (to + from).
template<typename _T,
         typename = void>
constexpr bool sqlite_does_backup = false;

template<typename _T>
constexpr bool sqlite_does_backup<_T, std::void_t<
    sqlite_backup_to_t<_T>,
    sqlite_backup_from_t<_T>>> = true;

// sqlite_does_attach
//   tagless trait: true if _T supports database attachment (attach +
// detach).
template<typename _T,
         typename = void>
constexpr bool sqlite_does_attach = false;

template<typename _T>
constexpr bool sqlite_does_attach<_T, std::void_t<
    sqlite_attach_t<_T>,
    sqlite_detach_t<_T>>> = true;

// sqlite_is_full_connection
//   tagless trait: true if _T satisfies the complete SQLite connection
// interface.
template<typename _T>
constexpr bool sqlite_is_full_connection =
    ( can_connect<_T>               &&
      can_disconnect<_T>            &&
      can_execute_query<_T>         &&
      sqlite_does_journal<_T>       &&
      sqlite_does_pragma<_T>        &&
      sqlite_can_query_schema<_T> );


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

// enable_if_sqlite_connection
//   type: SFINAE helper for SQLite connection constraints.
template<typename _T>
using enable_if_sqlite_connection =
    typename std::enable_if<is_sqlite_connection<_T>::value>::type;

// enable_if_has_sqlite_journal
//   type: SFINAE helper for SQLite journal constraints.
template<typename _T>
using enable_if_has_sqlite_journal =
    typename std::enable_if<has_sqlite_journal<_T>::value>::type;

// enable_if_has_sqlite_pragma
//   type: SFINAE helper for SQLite PRAGMA constraints.
template<typename _T>
using enable_if_has_sqlite_pragma =
    typename std::enable_if<has_sqlite_pragma<_T>::value>::type;

// enable_if_has_sqlite_backup
//   type: SFINAE helper for SQLite backup constraints.
template<typename _T>
using enable_if_has_sqlite_backup =
    typename std::enable_if<has_sqlite_backup<_T>::value>::type;


NS_END  // sqlite
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_SQLITE_TRAITS_
