/******************************************************************************
* djinterp [database]                                      sqlite_concepts.hpp
*
*  djinterp SQLite classification concepts
*   C++20 concepts layered on top of sqlite_traits.hpp.  These concepts
* provide readable `requires` constraints for SQLite-specific database
* connections, including file-based open/close operations, journal and WAL
* management, PRAGMA support, backup, attachment, schema introspection,
* extension loading, serialization, and SQLite transaction modes.
*
*   This header is intentionally thin: it does not re-implement detection.
* Instead, each concept forwards to the corresponding public trait,
* variable template, or tagless capability from the SQLite trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Core SQLite Connection Concepts
* 3.   SQLite Capability Concepts
* 4.   SQLite Transaction Mode Concepts
* 5.   Tagless SQLite Capability Concepts
*
* 
* path:      /inc/djinterp/core/db/sqlite/sqlite_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.01
******************************************************************************/

#ifndef DJINTERP_DATABASE_SQLITE_CONCEPTS_
#define DJINTERP_DATABASE_SQLITE_CONCEPTS_ 1

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "sqlite_concepts.hpp requires C++20 concepts support."
#endif

// std
#include <type_traits>
// djinterp
#include "./sqlite_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   Core SQLite Connection Concepts
// =============================================================================

// sqlite_connection
//   concept: constrains types implementing the SQLite connection interface.
template<typename _Type>
concept sqlite_connection =
    is_sqlite_connection<clean_t<_Type>>::value;

// non_sqlite_connection
//   concept: constrains types that do not implement the SQLite connection
// interface.
template<typename _Type>
concept non_sqlite_connection =
    !sqlite_connection<_Type>;

// sqlite_openable_connection
//   concept: constrains SQLite connections exposing open(const string&).
template<typename _Type>
concept sqlite_openable_connection =
    is_detected<sqlite_open_t, clean_t<_Type>>::value;

// sqlite_v2_openable_connection
//   concept: constrains SQLite connections exposing open_v2(path, flags).
template<typename _Type>
concept sqlite_v2_openable_connection =
    sqlite_can_open_v2<clean_t<_Type>>;

// sqlite_closable_connection
//   concept: constrains SQLite connections exposing close().
template<typename _Type>
concept sqlite_closable_connection =
    is_detected<sqlite_close_t, clean_t<_Type>>::value;

// sqlite_basic_file_connection
//   concept: constrains SQLite connections exposing both open() and close().
template<typename _Type>
concept sqlite_basic_file_connection =
    ( sqlite_openable_connection<_Type> &&
      sqlite_closable_connection<_Type> );


// =============================================================================
// II.  SQLite Capability Concepts
// =============================================================================

// sqlite_journal_connection
//   concept: constrains SQLite connections supporting journal mode
// management.
template<typename _Type>
concept sqlite_journal_connection =
    has_sqlite_journal<clean_t<_Type>>::value;

// sqlite_pragma_connection
//   concept: constrains SQLite connections supporting the PRAGMA interface.
template<typename _Type>
concept sqlite_pragma_connection =
    has_sqlite_pragma<clean_t<_Type>>::value;

// sqlite_backup_connection
//   concept: constrains SQLite connections supporting backup to/from.
template<typename _Type>
concept sqlite_backup_connection =
    has_sqlite_backup<clean_t<_Type>>::value;

// sqlite_attach_connection
//   concept: constrains SQLite connections supporting ATTACH and DETACH.
template<typename _Type>
concept sqlite_attach_connection =
    has_sqlite_attach<clean_t<_Type>>::value;

// sqlite_schema_query_connection
//   concept: constrains SQLite connections supporting schema introspection.
template<typename _Type>
concept sqlite_schema_query_connection =
    has_sqlite_schema_query<clean_t<_Type>>::value;

// sqlite_extension_loading_connection
//   concept: constrains SQLite connections supporting extension loading.
template<typename _Type>
concept sqlite_extension_loading_connection =
    has_sqlite_extension_loading<clean_t<_Type>>::value;

// sqlite_serialization_connection
//   concept: constrains SQLite connections supporting serialization and
// deserialization.
template<typename _Type>
concept sqlite_serialization_connection =
    has_sqlite_serialization<clean_t<_Type>>::value;

// sqlite_busy_timeout_connection
//   concept: constrains SQLite connections exposing set_busy_timeout(int).
template<typename _Type>
concept sqlite_busy_timeout_connection =
    sqlite_can_set_busy_timeout<clean_t<_Type>>;

// sqlite_checkpoint_connection
//   concept: constrains SQLite connections exposing checkpoint(int).
template<typename _Type>
concept sqlite_checkpoint_connection =
    sqlite_can_checkpoint<clean_t<_Type>>;

// sqlite_attachable_connection
//   concept: constrains SQLite connections exposing attach(path, alias).
template<typename _Type>
concept sqlite_attachable_connection =
    sqlite_can_attach<clean_t<_Type>>;

// sqlite_detachable_connection
//   concept: constrains SQLite connections exposing detach(alias).
template<typename _Type>
concept sqlite_detachable_connection =
    is_detected<sqlite_detach_t, clean_t<_Type>>::value;

// sqlite_schema_table_query_connection
//   concept: constrains SQLite connections exposing table_exists(name).
template<typename _Type>
concept sqlite_schema_table_query_connection =
    sqlite_can_query_schema<clean_t<_Type>>;

// sqlite_table_name_query_connection
//   concept: constrains SQLite connections exposing get_table_names().
template<typename _Type>
concept sqlite_table_name_query_connection =
    is_detected<sqlite_get_table_names_t, clean_t<_Type>>::value;

// sqlite_extension_loadable_connection
//   concept: constrains SQLite connections exposing load_extension(path).
template<typename _Type>
concept sqlite_extension_loadable_connection =
    sqlite_can_load_extension<clean_t<_Type>>;

// sqlite_extension_toggle_connection
//   concept: constrains SQLite connections exposing
// enable_load_extension(bool).
template<typename _Type>
concept sqlite_extension_toggle_connection =
    is_detected<sqlite_enable_load_extension_t, clean_t<_Type>>::value;

// sqlite_serializable_connection
//   concept: constrains SQLite connections exposing serialize().
template<typename _Type>
concept sqlite_serializable_connection =
    sqlite_can_serialize<clean_t<_Type>>;

// sqlite_deserializable_connection
//   concept: constrains SQLite connections exposing deserialize(bytes).
template<typename _Type>
concept sqlite_deserializable_connection =
    is_detected<sqlite_deserialize_t, clean_t<_Type>>::value;


// =============================================================================
// III. SQLite Transaction Mode Concepts
// =============================================================================

// sqlite_transaction_modes_connection
//   concept: constrains SQLite connections supporting deferred, immediate,
// and exclusive begin modes.
template<typename _Type>
concept sqlite_transaction_modes_connection =
    has_sqlite_transaction_modes<clean_t<_Type>>::value;

// sqlite_deferred_transaction_connection
//   concept: constrains SQLite connections exposing begin_deferred().
template<typename _Type>
concept sqlite_deferred_transaction_connection =
    is_detected<sqlite_begin_deferred_t, clean_t<_Type>>::value;

// sqlite_immediate_transaction_connection
//   concept: constrains SQLite connections exposing begin_immediate().
template<typename _Type>
concept sqlite_immediate_transaction_connection =
    is_detected<sqlite_begin_immediate_t, clean_t<_Type>>::value;

// sqlite_exclusive_transaction_connection
//   concept: constrains SQLite connections exposing begin_exclusive().
template<typename _Type>
concept sqlite_exclusive_transaction_connection =
    is_detected<sqlite_begin_exclusive_t, clean_t<_Type>>::value;


// =============================================================================
// IV.  Tagless SQLite Capability Concepts
// =============================================================================

// sqlite_journaling_connection
//   concept: constrains types satisfying the tagless journal capability set.
template<typename _Type>
concept sqlite_journaling_connection =
    sqlite_does_journal<clean_t<_Type>>;

// sqlite_pragmatic_connection
//   concept: constrains types satisfying the tagless PRAGMA capability set.
template<typename _Type>
concept sqlite_pragmatic_connection =
    sqlite_does_pragma<clean_t<_Type>>;

// sqlite_backup_capable_connection
//   concept: constrains types satisfying the tagless backup capability set.
template<typename _Type>
concept sqlite_backup_capable_connection =
    sqlite_does_backup<clean_t<_Type>>;

// sqlite_attached_connection
//   concept: constrains types satisfying the tagless attach capability set.
template<typename _Type>
concept sqlite_attached_connection =
    sqlite_does_attach<clean_t<_Type>>;

// sqlite_full_connection
//   concept: constrains types satisfying the tagless full SQLite connection
// capability set.
template<typename _Type>
concept sqlite_full_connection =
    sqlite_is_full_connection<clean_t<_Type>>;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_SQLITE_CONCEPTS_
