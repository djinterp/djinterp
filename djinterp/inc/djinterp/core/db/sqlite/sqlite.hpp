/*******************************************************************************
* djinterp [core]                                                     sqlite.hpp
*
* djinterp SQLite connection module:
*   The SQLite connection, sqlite_connection, over the SQLite layer's C++
* face (c/db/sqlite/sqlite_face.hpp), whose connection it holds: every
* operation is the layer's, and core/db adds exceptions, result sets that
* outlive their query, and prepared statements that close with the
* connection. With it, the data types SQLite has and how they map to
* field_type: the five storage classes, the declared types and their
* affinities, the open flags (SQLite's SQLITE_OPEN_*), the journal and
* transaction modes, and what the SQLite header and build env_sqlite.h found
* have, at compile time: the client's capabilities. What the library loaded
* can do, the connection asks of it at run time (supports_*).
*   SQLite is not a client and a server: the connection opens a file, or a
* database in memory; a column has an affinity, not a type, unless its table
* is STRICT (3.37); what SQLite can do is its version's and its build's.
*     sqlite_connection (this file)
*       -> database_connection<sqlite_connection, database_type::sqlite>
*         -> connection_template<sqlite_connection, database_type::sqlite>
*           -> connection<sqlite_connection>
*   The hooks are defined in sqlite.cpp. The header needs C++17 and includes
* no client header: env_sqlite.h includes sqlite3.h only where a program asks
* for env's detection (D_CFG_ENV_USING_SQLITE), which the client's
* capabilities read. Its trailing sections carry SQLite's
* capability-detection traits and C++20 concepts.
*
*
* path:      /inc/djinterp/core/db/sqlite/sqlite.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPE AFFINITY
    -------------
    1.  Storage classes and declared types
         1.  sqlite_affinity
         2.  sqlite_declared_type
2.  TYPE MAPPING
    ------------
    1.  To and from field_type
         1.  sqlite_affinity_to_field_type
         2.  sqlite_declared_type_to_field_type
         3.  field_type_to_sqlite_sql
         4.  sqlite_column_type_to_affinity
3.  OPEN FLAGS
    ----------
    1.  The flags
         1.  sqlite_open_flag
    2.  Operators
         1.  operator|
         2.  operator&
         3.  operator|=
4.  JOURNAL AND TRANSACTION MODES
    -----------------------------
    1.  Modes
         1.  sqlite_journal_mode
         2.  sqlite_transaction_mode
5.  CLIENT CAPABILITIES
    -------------------
    1.  Compile-time capabilities
         1.  sqlite_client_capabilities
         2.  sqlite_type_support
6.  VERSION INFORMATION
    -------------------
    1.  The version
         1.  sqlite_version_info
7.  CONNECTION CONFIGURATION
    ------------------------
    1.  The configuration
         1.  sqlite_connect_config
8.  SQLITE CONNECTION
    -----------------
    1.  The layer
         1.  sqlite_layer
         2.  prepared_statement<sqlite_layer>
         3.  streaming_result_set<sqlite_layer>
    2.  The class
         1.  sqlite_connection
9.  EXPRESSION DETECTORS
    --------------------
    1.  File-based connection
    2.  Journal and WAL management
    3.  PRAGMA interface
    4.  Backup API
    5.  Busy handler and timeout
    6.  Database attachment
    7.  Schema introspection
    8.  Extension loading
    9.  Serialization
    10. Transaction modes
10. TAGGED CAPABILITY TRAITS
    ------------------------
    1.  Traits
         1.  has_sqlite_journal
         2.  has_sqlite_pragma
         3.  has_sqlite_backup
         4.  has_sqlite_attach
         5.  has_sqlite_schema_query
         6.  has_sqlite_extension_loading
         7.  has_sqlite_serialization
         8.  has_sqlite_transaction_modes
         9.  is_sqlite_connection
11. TAGLESS CAPABILITY TRAITS
    -------------------------
    1.  Individual capabilities
    2.  Compound capabilities
12. SFINAE HELPERS
    --------------
    1.  enable_if helpers
13. C++20 CONCEPTS
    --------------
    1.  Connection concepts
    2.  Capability concepts
    3.  Transaction mode concepts
    4.  Tagless capability concepts
    5.  Former names
*/

#ifndef DJINTERP_DB_SQLITE_SQLITE_HPP
#define DJINTERP_DB_SQLITE_SQLITE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t, uint32_t,
                                                   // uint16_t, int64_t

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <map>     // std::map
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "../../../djinterp.hpp"                 // framework root
#include "../../../c/db/sqlite/sqlite_face.hpp"  // face::sqlite, the layer
#include "../../../env/db/sqlite/env_sqlite.h"   // D_ENV_SQLITE_*
#include "../../meta/type_utility.hpp"           // clean_t
#include "../database_connection.hpp"            // database_connection
#include "../database_statement.hpp"             // prepared_statement,
                                                 // failure_kind


NS_DJINTERP


//==============================================================================
// 1.  TYPE AFFINITY
//==============================================================================
// SQLite uses a dynamic type system based on five storage classes.
// Column declarations map to one of five affinities via the affinity
// determination rules in the SQLite documentation. STRICT tables
// (3.37+) enforce exact type matching instead of affinity.


// 1.1    Storage classes and declared types
//------------------------------------------------------------------------------
// 1.1.1
// sqlite_affinity
//   enumeration: the five SQLite storage classes / type affinities.
enum class sqlite_affinity : re_std::uint8_t
{
    type_integer   = 1,     // 64-bit signed integer
    type_real      = 2,     // 64-bit IEEE 754 float
    type_text      = 3,     // UTF-8/UTF-16 text
    type_blob      = 4,     // raw binary data
    type_null      = 5      // NULL
};

// 1.1.2
// sqlite_declared_type
//   enumeration: common declared column types and how they map to
// SQLite affinities. These are the conventional type names used in
// CREATE TABLE statements; SQLite determines affinity from substring
// matching on the declared type.
enum class sqlite_declared_type : re_std::uint8_t
{
    // INTEGER affinity
    dt_int          = 0x10,
    dt_integer      = 0x11,
    dt_tinyint      = 0x12,
    dt_smallint     = 0x13,
    dt_mediumint    = 0x14,
    dt_bigint       = 0x15,
    dt_int2         = 0x16,
    dt_int8         = 0x17,
    dt_boolean      = 0x18,

    // TEXT affinity
    dt_text         = 0x20,
    dt_varchar      = 0x21,
    dt_character    = 0x22,
    dt_clob         = 0x23,
    dt_nchar        = 0x24,
    dt_nvarchar     = 0x25,

    // REAL affinity
    dt_real         = 0x30,
    dt_double       = 0x31,
    dt_float        = 0x32,

    // NUMERIC affinity (can store as INTEGER or REAL)
    dt_numeric      = 0x40,
    dt_decimal      = 0x41,
    dt_date         = 0x42,
    dt_datetime     = 0x43,
    dt_timestamp    = 0x44,

    // BLOB affinity (no affinity / exact match in STRICT)
    dt_blob         = 0x50,

    // ANY (STRICT tables only, 3.37+)
    dt_any          = 0x60,

    // unrecognized
    dt_unknown      = 0x00
};


//==============================================================================
// 2.  TYPE MAPPING
//==============================================================================


// 2.1    To and from field_type
//------------------------------------------------------------------------------
// 2.1.1
// sqlite_affinity_to_field_type
//   function: maps a sqlite_affinity to the generic djinterp
// field_type.
inline field_type
sqlite_affinity_to_field_type(
    sqlite_affinity _affinity
) noexcept
{
    switch (_affinity)
    {
        case sqlite_affinity::type_integer:
            return field_type::big_integer;

        case sqlite_affinity::type_real:
            return field_type::floating_point;

        case sqlite_affinity::type_text:
            return field_type::string;

        case sqlite_affinity::type_blob:
            return field_type::binary;

        case sqlite_affinity::type_null:
            return field_type::null;

        default:
            return field_type::custom;
    }
}

// 2.1.2
// sqlite_declared_type_to_field_type
//   function: maps a sqlite_declared_type to the generic djinterp
// field_type. This provides richer mapping than pure affinity by
// interpreting conventional type names (e.g. BOOLEAN -> boolean,
// DATETIME -> datetime, DECIMAL -> decimal).
inline field_type
sqlite_declared_type_to_field_type(
    sqlite_declared_type _declared
) noexcept
{
    switch (_declared)
    {
        // integer family
        case sqlite_declared_type::dt_int:
        case sqlite_declared_type::dt_integer:
        case sqlite_declared_type::dt_tinyint:
        case sqlite_declared_type::dt_smallint:
        case sqlite_declared_type::dt_mediumint:
        case sqlite_declared_type::dt_int2:
            return field_type::integer;

        case sqlite_declared_type::dt_bigint:
        case sqlite_declared_type::dt_int8:
            return field_type::big_integer;

        case sqlite_declared_type::dt_boolean:
            return field_type::boolean;

        // text family
        case sqlite_declared_type::dt_text:
        case sqlite_declared_type::dt_varchar:
        case sqlite_declared_type::dt_character:
        case sqlite_declared_type::dt_clob:
        case sqlite_declared_type::dt_nchar:
        case sqlite_declared_type::dt_nvarchar:
            return field_type::string;

        // real family
        case sqlite_declared_type::dt_real:
        case sqlite_declared_type::dt_double:
        case sqlite_declared_type::dt_float:
            return field_type::floating_point;

        // numeric family
        case sqlite_declared_type::dt_numeric:
        case sqlite_declared_type::dt_decimal:
            return field_type::decimal;

        case sqlite_declared_type::dt_date:
            return field_type::date;

        case sqlite_declared_type::dt_datetime:
            return field_type::datetime;

        case sqlite_declared_type::dt_timestamp:
            return field_type::timestamp;

        // blob
        case sqlite_declared_type::dt_blob:
            return field_type::binary;

        // any / unknown
        case sqlite_declared_type::dt_any:
        case sqlite_declared_type::dt_unknown:
        default:
            return field_type::custom;
    }
}

// 2.1.3
// field_type_to_sqlite_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in SQLite CREATE TABLE statements.
inline const char*
field_type_to_sqlite_sql(
    field_type _type
) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "NULL";
        case field_type::boolean:        return "BOOLEAN";
        case field_type::integer:        return "INTEGER";
        case field_type::big_integer:    return "BIGINT";
        case field_type::floating_point: return "REAL";
        case field_type::decimal:        return "NUMERIC";
        case field_type::string:         return "TEXT";
        case field_type::binary:         return "BLOB";
        case field_type::date:           return "DATE";
        case field_type::time:           return "TEXT";
        case field_type::datetime:       return "DATETIME";
        case field_type::timestamp:      return "TIMESTAMP";
        case field_type::json:           return "TEXT";
        case field_type::xml:            return "TEXT";
        case field_type::uuid:           return "TEXT";
        case field_type::array:          return "TEXT";
        case field_type::custom:
        default:                         return "BLOB";
    }
}

// 2.1.4
// sqlite_column_type_to_affinity
//   function: maps the integer return value of sqlite3_column_type()
// (SQLITE_INTEGER=1, SQLITE_FLOAT=2, SQLITE_TEXT=3, SQLITE_BLOB=4,
// SQLITE_NULL=5) to the sqlite_affinity enum.
inline sqlite_affinity
sqlite_column_type_to_affinity(
    int _sqlite_type
) noexcept
{
    switch (_sqlite_type)
    {
        case 1:  return sqlite_affinity::type_integer;
        case 2:  return sqlite_affinity::type_real;
        case 3:  return sqlite_affinity::type_text;
        case 4:  return sqlite_affinity::type_blob;
        case 5:  return sqlite_affinity::type_null;
        default: return sqlite_affinity::type_null;
    }
}


//==============================================================================
// 3.  OPEN FLAGS
//==============================================================================
// Scoped enum mirroring the SQLITE_OPEN_* constants from sqlite3.h.
// Enables type-safe flag composition without including <sqlite3.h>.


// 3.1    The flags
//------------------------------------------------------------------------------
// 3.1.1
// sqlite_open_flag
//   enumeration: flags for sqlite3_open_v2().
enum class sqlite_open_flag : int
{
    readonly        = 0x00000001,   // SQLITE_OPEN_READONLY
    readwrite       = 0x00000002,   // SQLITE_OPEN_READWRITE
    create          = 0x00000004,   // SQLITE_OPEN_CREATE
    uri             = 0x00000040,   // SQLITE_OPEN_URI
    memory          = 0x00000080,   // SQLITE_OPEN_MEMORY
    nomutex         = 0x00008000,   // SQLITE_OPEN_NOMUTEX
    fullmutex       = 0x00010000,   // SQLITE_OPEN_FULLMUTEX
    shared_cache    = 0x00020000,   // SQLITE_OPEN_SHAREDCACHE
    private_cache   = 0x00040000,   // SQLITE_OPEN_PRIVATECACHE
    nofollow        = 0x01000000,   // SQLITE_OPEN_NOFOLLOW (3.31+)

    // common combinations
    read_only       = 0x00000001,
    read_write      = 0x00000002 | 0x00000004
};

// 3.2    Operators
//------------------------------------------------------------------------------
// 3.2.1
// operator|
//   function: bitwise OR for combining sqlite_open_flag values.
inline constexpr sqlite_open_flag
operator|(
    sqlite_open_flag _a,
    sqlite_open_flag _b
)
{
    return static_cast<sqlite_open_flag>(
        static_cast<int>(_a) | static_cast<int>(_b));
}

// 3.2.2
// operator&
//   function: bitwise AND for testing sqlite_open_flag values.
inline constexpr sqlite_open_flag
operator&(
    sqlite_open_flag _a,
    sqlite_open_flag _b
)
{
    return static_cast<sqlite_open_flag>(
        static_cast<int>(_a) & static_cast<int>(_b));
}

// 3.2.3
// operator|=
//   function: bitwise OR assignment for sqlite_open_flag.
inline constexpr sqlite_open_flag&
operator|=(
    sqlite_open_flag& _a,
    sqlite_open_flag  _b
)
{
    _a = _a | _b;

    return _a;
}


//==============================================================================
// 4.  JOURNAL AND TRANSACTION MODES
//==============================================================================


// 4.1    Modes
//------------------------------------------------------------------------------
// 4.1.1
// sqlite_journal_mode
//   enumeration: SQLite journal modes (PRAGMA journal_mode).
enum class sqlite_journal_mode : re_std::uint8_t
{
    mode_delete    = 0,     // default; rollback journal deleted after txn
    mode_truncate  = 1,     // journal truncated instead of deleted
    mode_persist   = 2,     // journal header zeroed, file kept
    mode_memory    = 3,     // journal stored in memory
    mode_wal       = 4,     // write-ahead logging
    mode_off       = 5      // journaling disabled (unsafe)
};

// 4.1.2
// sqlite_transaction_mode
//   enumeration: SQLite BEGIN TRANSACTION modes.
enum class sqlite_transaction_mode : re_std::uint8_t
{
    deferred       = 0,     // acquire locks lazily (default)
    immediate      = 1,     // acquire RESERVED lock immediately
    exclusive      = 2      // acquire EXCLUSIVE lock immediately
};


//==============================================================================
// 5.  CLIENT CAPABILITIES
//==============================================================================
// What env_sqlite.h found SQLite's header and build to have, by version, at
// compile time -- the client's capabilities, which the library loaded at run
// time may lack, or exceed: every flag is false unless a program asks for
// env's detection (D_CFG_ENV_USING_SQLITE) or supplies its results. What the
// library loaded can do, sqlite_connection's supports_* members ask it.


// 5.1    Compile-time capabilities
//------------------------------------------------------------------------------
// 5.1.1
// sqlite_client_capabilities
//   struct: compile-time data type and feature availability flags, as the
// SQLite header and build give them.
struct sqlite_client_capabilities
{
#if D_ENV_SQLITE_DETECTED

    // SQL features (version-gated)
    static constexpr bool has_cte =
    #if D_ENV_SQLITE_HAS_CTE
        true;
    #else
        false;
    #endif

    static constexpr bool has_cte_recursive =
    #if D_ENV_SQLITE_HAS_CTE_RECURSIVE
        true;
    #else
        false;
    #endif

    static constexpr bool has_cte_materialized =
    #if D_ENV_SQLITE_HAS_CTE_MATERIALIZED
        true;
    #else
        false;
    #endif

    static constexpr bool has_window_functions =
    #if D_ENV_SQLITE_HAS_WINDOW_FUNCTIONS
        true;
    #else
        false;
    #endif

    static constexpr bool has_upsert =
    #if D_ENV_SQLITE_HAS_UPSERT
        true;
    #else
        false;
    #endif

    static constexpr bool has_returning =
    #if D_ENV_SQLITE_HAS_RETURNING
        true;
    #else
        false;
    #endif

    static constexpr bool has_generated_columns =
    #if D_ENV_SQLITE_HAS_GENERATED_COLUMNS
        true;
    #else
        false;
    #endif

    static constexpr bool has_strict_tables =
    #if D_ENV_SQLITE_HAS_STRICT_TABLES
        true;
    #else
        false;
    #endif

    static constexpr bool has_update_from =
    #if D_ENV_SQLITE_HAS_UPDATE_FROM
        true;
    #else
        false;
    #endif

    static constexpr bool has_drop_column =
    #if D_ENV_SQLITE_HAS_DROP_COLUMN
        true;
    #else
        false;
    #endif

    static constexpr bool has_rename_column =
    #if D_ENV_SQLITE_HAS_RENAME_COLUMN
        true;
    #else
        false;
    #endif

    static constexpr bool has_partial_index =
    #if D_ENV_SQLITE_HAS_PARTIAL_INDEX
        true;
    #else
        false;
    #endif

    static constexpr bool has_filter_clause =
    #if D_ENV_SQLITE_HAS_FILTER_CLAUSE
        true;
    #else
        false;
    #endif

    static constexpr bool has_row_values =
    #if D_ENV_SQLITE_HAS_ROW_VALUES
        true;
    #else
        false;
    #endif

    // JSON support
    static constexpr bool has_json =
    #if D_ENV_SQLITE_HAS_JSON
        true;
    #else
        false;
    #endif

    static constexpr bool json_is_builtin =
    #if D_ENV_SQLITE_JSON_IS_BUILTIN
        true;
    #else
        false;
    #endif

    static constexpr bool has_json_operators =
    #if D_ENV_SQLITE_HAS_JSON_OPERATORS
        true;
    #else
        false;
    #endif

    static constexpr bool has_jsonb =
    #if D_ENV_SQLITE_HAS_JSONB
        true;
    #else
        false;
    #endif

    static constexpr bool has_json5 =
    #if D_ENV_SQLITE_HAS_JSON5
        true;
    #else
        false;
    #endif

    // full-text search
    static constexpr bool has_fts3 =
    #if D_ENV_SQLITE_HAS_FTS3
        true;
    #else
        false;
    #endif

    static constexpr bool has_fts4 =
    #if D_ENV_SQLITE_HAS_FTS4
        true;
    #else
        false;
    #endif

    static constexpr bool has_fts5 =
    #if D_ENV_SQLITE_HAS_FTS5
        true;
    #else
        false;
    #endif

    // spatial / R*Tree
    static constexpr bool has_rtree =
    #if D_ENV_SQLITE_HAS_RTREE
        true;
    #else
        false;
    #endif

    static constexpr bool has_geopoly =
    #if D_ENV_SQLITE_HAS_GEOPOLY
        true;
    #else
        false;
    #endif

    // WAL and journaling
    static constexpr bool has_wal =
    #if D_ENV_SQLITE_HAS_WAL
        true;
    #else
        false;
    #endif

    static constexpr bool has_mmap =
    #if D_ENV_SQLITE_HAS_MMAP
        true;
    #else
        false;
    #endif

    // backup and serialization
    static constexpr bool has_backup_api =
    #if D_ENV_SQLITE_HAS_BACKUP_API
        true;
    #else
        false;
    #endif

    static constexpr bool has_serialize =
    #if D_ENV_SQLITE_HAS_SERIALIZE
        true;
    #else
        false;
    #endif

    static constexpr bool has_session =
    #if D_ENV_SQLITE_HAS_SESSION
        true;
    #else
        false;
    #endif

    // security
    static constexpr bool has_encryption =
    #if D_ENV_SQLITE_HAS_ENCRYPTION
        true;
    #else
        false;
    #endif

    static constexpr bool has_api_armor =
    #if D_ENV_SQLITE_HAS_API_ARMOR
        true;
    #else
        false;
    #endif

    static constexpr bool has_defensive_mode =
    #if D_ENV_SQLITE_HAS_DEFENSIVE_MODE
        true;
    #else
        false;
    #endif

    // threading
    static constexpr bool has_thread_safety =
    #if D_ENV_SQLITE_HAS_THREAD_SAFETY
        true;
    #else
        false;
    #endif

    static constexpr int threadsafe_level =
    #ifdef D_ENV_SQLITE_THREADSAFE
        D_ENV_SQLITE_THREADSAFE;
    #else
        0;
    #endif

    // compile-time omittable features
    static constexpr bool has_foreign_keys =
    #if D_ENV_SQLITE_HAS_FOREIGN_KEYS
        true;
    #else
        false;
    #endif

    static constexpr bool has_triggers =
    #if D_ENV_SQLITE_HAS_TRIGGERS
        true;
    #else
        false;
    #endif

    static constexpr bool has_views =
    #if D_ENV_SQLITE_HAS_VIEWS
        true;
    #else
        false;
    #endif

    static constexpr bool has_vacuum =
    #if D_ENV_SQLITE_HAS_VACUUM
        true;
    #else
        false;
    #endif

    static constexpr bool has_attach =
    #if D_ENV_SQLITE_HAS_ATTACH
        true;
    #else
        false;
    #endif

    static constexpr bool has_autoincrement =
    #if D_ENV_SQLITE_HAS_AUTOINCREMENT
        true;
    #else
        false;
    #endif

    static constexpr bool has_load_extension =
    #if D_ENV_SQLITE_HAS_LOAD_EXTENSION
        true;
    #else
        false;
    #endif

    static constexpr bool has_math_functions =
    #if D_ENV_SQLITE_HAS_MATH_FUNCTIONS
        true;
    #else
        false;
    #endif

    static constexpr bool has_column_metadata =
    #if D_ENV_SQLITE_HAS_COLUMN_METADATA
        true;
    #else
        false;
    #endif

    // composite
    static constexpr bool has_modern_sql =
    #if D_ENV_SQLITE_HAS_MODERN_SQL
        true;
    #else
        false;
    #endif

    static constexpr bool has_modern_json =
    #if D_ENV_SQLITE_HAS_MODERN_JSON
        true;
    #else
        false;
    #endif

    static constexpr bool is_fully_modern =
    #if D_ENV_SQLITE_IS_FULLY_MODERN
        true;
    #else
        false;
    #endif

#else
    // SQLite not detected: everything unavailable
    static constexpr bool has_cte                = false;
    static constexpr bool has_cte_recursive      = false;
    static constexpr bool has_cte_materialized   = false;
    static constexpr bool has_window_functions   = false;
    static constexpr bool has_upsert             = false;
    static constexpr bool has_returning          = false;
    static constexpr bool has_generated_columns  = false;
    static constexpr bool has_strict_tables      = false;
    static constexpr bool has_update_from        = false;
    static constexpr bool has_drop_column        = false;
    static constexpr bool has_rename_column      = false;
    static constexpr bool has_partial_index      = false;
    static constexpr bool has_filter_clause      = false;
    static constexpr bool has_row_values         = false;
    static constexpr bool has_json               = false;
    static constexpr bool json_is_builtin        = false;
    static constexpr bool has_json_operators     = false;
    static constexpr bool has_jsonb              = false;
    static constexpr bool has_json5              = false;
    static constexpr bool has_fts3               = false;
    static constexpr bool has_fts4               = false;
    static constexpr bool has_fts5               = false;
    static constexpr bool has_rtree              = false;
    static constexpr bool has_geopoly            = false;
    static constexpr bool has_wal                = false;
    static constexpr bool has_mmap               = false;
    static constexpr bool has_backup_api         = false;
    static constexpr bool has_serialize          = false;
    static constexpr bool has_session            = false;
    static constexpr bool has_encryption         = false;
    static constexpr bool has_api_armor          = false;
    static constexpr bool has_defensive_mode     = false;
    static constexpr bool has_thread_safety      = false;
    static constexpr int  threadsafe_level       = 0;
    static constexpr bool has_foreign_keys       = false;
    static constexpr bool has_triggers           = false;
    static constexpr bool has_views              = false;
    static constexpr bool has_vacuum             = false;
    static constexpr bool has_attach             = false;
    static constexpr bool has_autoincrement      = false;
    static constexpr bool has_load_extension     = false;
    static constexpr bool has_math_functions     = false;
    static constexpr bool has_column_metadata    = false;
    static constexpr bool has_modern_sql         = false;
    static constexpr bool has_modern_json        = false;
    static constexpr bool is_fully_modern        = false;
#endif  // D_ENV_SQLITE_DETECTED
};

// 5.1.2
// sqlite_type_support
//   type: sqlite_client_capabilities, by its former name (2026.10.21), for
// one release.
using sqlite_type_support [[deprecated("sqlite_client_capabilities")]] =
    sqlite_client_capabilities;


//==============================================================================
// 6.  VERSION INFORMATION
//==============================================================================


// 6.1    The version
//------------------------------------------------------------------------------
// 6.1.1
// sqlite_version_info
//   struct: compile-time version decomposition of the header env detected;
// the library loaded's is the connection's to ask (get_server_version).
// NOTE: SQLite encodes version as MAJOR*1000000 + MINOR*1000 + PATCH,
// which differs from the MySQL family's MAJOR*10000 + MINOR*100 + PATCH.
struct sqlite_version_info
{
#if D_ENV_SQLITE_DETECTED
    static constexpr bool             detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_SQLITE_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_SQLITE_VERSION_MAJOR;
    static constexpr re_std::uint16_t minor    = D_ENV_SQLITE_VERSION_MINOR;
    static constexpr re_std::uint16_t patch    = D_ENV_SQLITE_VERSION_PATCH;
    static constexpr const char*      string   = D_ENV_SQLITE_VERSION_STRING;
#else
    static constexpr bool             detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t minor    = 0;
    static constexpr re_std::uint16_t patch    = 0;
    static constexpr const char*      string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected SQLite version is at
    // least (major, minor, patch).
    static constexpr bool
    at_least(
        re_std::uint16_t _major,
        re_std::uint16_t _minor,
        re_std::uint16_t _patch
    ) noexcept
    {
        return id >= (_major * 1000000u + _minor * 1000u + _patch);
    }
};


//==============================================================================
// 7.  CONNECTION CONFIGURATION
//==============================================================================


// 7.1    The configuration
//------------------------------------------------------------------------------
// 7.1.1
// sqlite_connect_config
//   struct: what an SQLite connection opens, and how. SQLite is a file, so
// the address is a path rather than a host and port: ":memory:" opens a
// private in-memory database, "" a private temporary one on disk. The
// settings are applied when the connection opens: the busy timeout and the
// foreign keys always, the journal mode, cache size and page size where they
// differ from SQLite's defaults, then each of `pragmas`, in order.
struct sqlite_connect_config
{
    std::string             file_path;
    sqlite_open_flag        open_flags;      // read_write: read, write, create
    sqlite_journal_mode     journal_mode;    // mode_delete, SQLite's default
    sqlite_transaction_mode default_transaction_mode;  // deferred
    int                     busy_timeout_ms;      // 5000
    int                     cache_size;           // -2000, SQLite's default
    int                     page_size;            // 4096, SQLite's default
    bool                    enable_foreign_keys;  // true
    bool                    enable_wal;           // false: journal_mode WAL
    bool                    enable_shared_cache;  // false
    std::string             vfs_name;             // "": SQLite's default VFS

    std::map<std::string, std::string> pragmas;

    // construction -- SQLite's defaults, for no file yet or for a path
    sqlite_connect_config()
        : open_flags(sqlite_open_flag::read_write),
          journal_mode(sqlite_journal_mode::mode_delete),
          default_transaction_mode(sqlite_transaction_mode::deferred),
          busy_timeout_ms(5000),
          cache_size(-2000),
          page_size(4096),
          enable_foreign_keys(true),
          enable_wal(false),
          enable_shared_cache(false)
    {}

    explicit sqlite_connect_config(
        const std::string& _path
    )
        : file_path(_path),
          open_flags(sqlite_open_flag::read_write),
          journal_mode(sqlite_journal_mode::mode_delete),
          default_transaction_mode(sqlite_transaction_mode::deferred),
          busy_timeout_ms(5000),
          cache_size(-2000),
          page_size(4096),
          enable_foreign_keys(true),
          enable_wal(false),
          enable_shared_cache(false)
    {}

    // in_memory
    //   function: a private in-memory database.
    static sqlite_connect_config
    in_memory()
    {
        sqlite_connect_config config;

        config.file_path  = ":memory:";
        config.open_flags = sqlite_open_flag::readwrite  |
                            sqlite_open_flag::create     |
                            sqlite_open_flag::memory;

        return config;
    }

    // temporary
    //   function: a private temporary database on disk, deleted when it
    // closes.
    static sqlite_connect_config
    temporary()
    {
        sqlite_connect_config config;

        config.file_path  = "";
        config.open_flags = sqlite_open_flag::readwrite |
                            sqlite_open_flag::create;

        return config;
    }

    // with_wal
    //   function: a database file in write-ahead-log mode.
    static sqlite_connect_config
    with_wal(
        const std::string& _path
    )
    {
        sqlite_connect_config config(_path);

        config.enable_wal   = true;
        config.journal_mode = sqlite_journal_mode::mode_wal;

        return config;
    }
};


//==============================================================================
// 8.  SQLITE CONNECTION
//==============================================================================


// 8.1    The layer
//------------------------------------------------------------------------------
// 8.1.1
// sqlite_layer
//   struct: the SQLite layer, as database_statement.hpp's templates take a
// layer: its statement, the connection a statement belongs to, and its
// failures. SQLite is embedded, so no result means a connection was lost;
// a point in time is bound as its UTC date and time alone, as SQLite's own
// date functions write one.
struct sqlite_layer
{
    using statement_face = face::sqlite_statement;
    using handle_type    = struct ::sqlite3*;

    static constexpr const char* utc_offset = "";

    // holds_connection -- false: SQLite steps a statement while the
    // connection runs others, so a stream's run leaves the connection free
    static constexpr bool holds_connection = false;

    // connection_lost -- whether a result means the connection was lost:
    // never, for an embedded database
    static constexpr bool
    connection_lost(
        int _result
    ) noexcept
    {
        // an embedded database has no connection to lose
        static_cast<void>(_result);

        return false;
    }

    /**
     * @brief Throws the exception for a failed call of the SQLite layer.
     *
     * @param[in] _db         the connection the call ran on; may be `NULL`.
     * @param[in] _kind       which exception to throw.
     * @param[in] _operation  the operation's name, for the message.
     * @param[in] _result     the call's result.
     * @throws connection_exception, query_exception or transaction_exception,
     *         as `_kind` says, with `_result` as its error_code() and, for a
     *         code of SQLite's, the connection's message (sqlite3_errmsg).
     */
    [[noreturn]] static void raise(handle_type  _db,
                                   failure_kind _kind,
                                   const char*  _operation,
                                   int          _result);
};

// 8.1.2
// prepared_statement<sqlite_layer>
//   class: SQLite's prepared statements, instantiated once, in sqlite.cpp.
extern template class prepared_statement<sqlite_layer>;

// 8.1.3
// streaming_result_set<sqlite_layer>
//   class: its streams, instantiated with them.
extern template class streaming_result_set<sqlite_layer>;

// 8.2    The class
//------------------------------------------------------------------------------
// 8.2.1
// sqlite_connection
//   class: a connection to an SQLite database, over the SQLite layer's face,
// which it holds and which closes the connection when it goes. Opening it
// opens the file database() names -- open(path) names one and opens it --
// with the open flags, VFS and settings sqlite_connect_config gives.
// Parameters are written as SQLite writes them (`?`, `?NNN`, `:name`,
// `@name`, `$name`), and a prepared statement binds by name too. Every
// transaction is serializable, the strongest level, whatever level is asked
// for; set_isolation_level(read_uncommitted) sets PRAGMA read_uncommitted,
// which lets a connection in shared-cache mode read the rows other
// connections sharing its cache have not committed. Each member the
// connection adds -- PRAGMAs, the journal, attached databases, the schema,
// backups, databases as bytes, extensions -- throws as the operations do
// (database_connection.hpp): connection_exception when the connection is
// not open, and otherwise the exception its contract names, with SQLite's
// result or a negative D_DB_* as its error_code(). Not copyable; movable.
//
// Usage:
//   sqlite_connection conn("app.db");
//   conn.connect();
//   auto rs = conn.execute_query("SELECT id, name FROM users");
class sqlite_connection : public database_connection<sqlite_connection,
                                                     database_type::sqlite>
{
public:
    using base_type           = database_connection<sqlite_connection,
                                                    database_type::sqlite>;
    using layer_type          = sqlite_layer;
    using statement_type      = prepared_statement<sqlite_layer>;
    using client_capabilities = sqlite_client_capabilities;
    using version_info        = sqlite_version_info;

    // type_support -- client_capabilities, by its former name (2026.10.21),
    // for one release
    using type_support [[deprecated("client_capabilities")]] =
        sqlite_client_capabilities;

    // construction -- with SQLite's defaults; with a generic configuration,
    // whose `database` is the file; with an SQLite one; or with a path.
    // Nothing opens before connect() or open()
    sqlite_connection();
    explicit sqlite_connection(const connection_config& _config);
    explicit sqlite_connection(const sqlite_connect_config& _config);
    explicit sqlite_connection(const std::string& _file_path);

    /**
     * @brief Closes the connection, its prepared statements first.
     */
    ~sqlite_connection();

    sqlite_connection(const sqlite_connection&)            = delete;
    sqlite_connection& operator=(const sqlite_connection&) = delete;

    sqlite_connection(sqlite_connection&&) noexcept            = default;
    sqlite_connection& operator=(sqlite_connection&&) noexcept = default;

    /**
     * @brief Closes the connection, if it is open, and opens a file, with
     *        the configured flags.
     *
     * @param[in] _file_path  the file, in UTF-8; ":memory:" for a private
     *                        in-memory database.
     * @throws as connect(): connection_exception, with SQLite's result --
     *         SQLITE_CANTOPEN for a path that cannot be opened.
     */
    void open(const std::string& _file_path);

    /**
     * @brief Closes the connection, if it is open, and opens a file, with
     *        the flags given.
     *
     * @param[in] _file_path  the file, as open(_file_path) takes it.
     * @param[in] _flags      sqlite3_open_v2's flags, which replace the
     *                        configured ones.
     * @throws as open(_file_path).
     */
    void open(const std::string& _file_path,
              sqlite_open_flag   _flags);

    // close -- disconnect()'s name for a file
    void close() noexcept;

    /**
     * @brief Sets the journal mode, by name, in any ASCII case: DELETE,
     *        TRUNCATE, PERSIST, MEMORY, WAL or OFF.
     *
     * @param[in] _mode  the mode's name.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for another name;
     *         with D_DB_UNSUPPORTED when the database keeps its mode, as an
     *         in-memory one keeps any but MEMORY and OFF; with SQLite's
     *         result for another failure, SQLITE_ERROR to enter or leave WAL
     *         inside a transaction among them.
     * @post On success the configuration's journal_mode is the mode set.
     */
    void set_journal_mode(const std::string& _mode);

    /**
     * @brief Reads the journal mode.
     *
     * @return the mode's name, in lower case, as SQLite writes it.
     * @throws query_exception with SQLite's result when SQLite cannot say,
     *         and with D_DB_UNSUPPORTED for a mode the layer does not name.
     */
    std::string get_journal_mode() const;

    /**
     * @brief Copies the write-ahead log's committed pages into the database;
     *        a database not in WAL mode has no log, and succeeds at once.
     *
     * @param[in] _mode  0 passive, 1 full, 2 restart, 3 truncate.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for another mode;
     *         with SQLITE_BUSY when full, restart or truncate could not
     *         finish for other connections, with as much done as could be;
     *         with D_DB_UNSUPPORTED for truncate before SQLite 3.8.8; with
     *         SQLite's result for another failure.
     */
    void checkpoint(int _mode = 0);

    /**
     * @brief Puts the database in write-ahead-log mode, as
     *        set_journal_mode("WAL") does.
     *
     * @throws as set_journal_mode.
     */
    void enable_wal();

    /**
     * @brief Sets any PRAGMA, as PRAGMA name = 'value'.
     *
     * @note Nothing is read back: SQLite ignores an unknown PRAGMA.
     *
     * @param[in] _pragma  the PRAGMA's name: letters, digits and underscores.
     * @param[in] _value   the value, passed as a string literal, which every
     *                     PRAGMA taking a number, a keyword or a name
     *                     accepts.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for text holding a
     *         NUL; with SQLITE_MISUSE for a name that is not a PRAGMA's; with
     *         SQLite's result for another failure.
     */
    void set_pragma(const std::string& _pragma,
                    const std::string& _value);

    /**
     * @brief Reads any PRAGMA: the first column of its first row, as text.
     *
     * @param[in] _pragma  the PRAGMA's name, as set_pragma takes it.
     * @return the value; "" for a NULL value.
     * @throws query_exception with D_DB_NOT_FOUND for a PRAGMA that reports
     *         nothing, as an unknown one does; and as set_pragma.
     */
    std::string get_pragma(const std::string& _pragma) const;

    /**
     * @brief Turns foreign-key enforcement on or off for the connection.
     *
     * @param[in] _enabled  whether to enforce.
     * @throws query_exception with D_DB_UNSUPPORTED when the setting did not
     *         take -- inside a transaction, or in a library built without
     *         foreign keys -- and with SQLite's result for another failure.
     * @post On success the configuration's enable_foreign_keys is `_enabled`.
     */
    void set_foreign_keys(bool _enabled);

    /**
     * @brief Sets how long the connection waits for a lock another
     *        connection holds before SQLITE_BUSY.
     *
     * @param[in] _timeout_ms  the wait, in milliseconds; 0 does not wait.
     * @throws query_exception with SQLITE_MISUSE for a negative wait, and
     *         with SQLite's result for another failure.
     * @post On success the configuration's busy_timeout_ms is `_timeout_ms`.
     */
    void set_busy_timeout(int _timeout_ms);

    /**
     * @brief Attaches a database file under a name, creating the file if it
     *        does not exist; SQL then reaches its objects as name.object.
     *
     * @note Not inside a transaction: SQLite refuses to attach there.
     *
     * @param[in] _file_path  the file, in UTF-8; ":memory:" or "" attach a
     *                        private in-memory or temporary database.
     * @param[in] _name       the name to attach it as.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for text holding a
     *         NUL; with SQLITE_MISUSE for an empty name; with SQLite's result
     *         for another failure, SQLITE_ERROR for a name already in use, or
     *         too many attached, among them.
     */
    void attach_database(const std::string& _file_path,
                         const std::string& _name);

    /**
     * @brief Detaches a database the connection attached.
     *
     * @param[in] _name  the name it was attached as; "main" and "temp"
     *                   cannot be detached.
     * @throws query_exception as attach_database does, SQLITE_ERROR when no
     *         such database is attached, or it is in use.
     */
    void detach_database(const std::string& _name);

    /**
     * @brief Says whether the main database has a table of a name, virtual
     *        tables included, matched as SQLite matches names, ignoring ASCII
     *        case.
     *
     * @param[in] _table_name  the name.
     * @return whether the table exists.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name holding a
     *         NUL, and with SQLite's result when SQLite cannot tell.
     */
    bool has_table(const std::string& _table_name) const;

    /**
     * @brief Lists the main database's tables, virtual tables included, but
     *        not SQLite's own (sqlite_*).
     *
     * @return the names, sorted.
     * @throws query_exception with SQLite's result when SQLite cannot list
     *         them.
     */
    std::vector<std::string> list_tables() const;

    // transaction modes -- begin a transaction that takes its write lock at
    // its first write, at once, or at once keeping readers out. Each throws
    // transaction_exception with SQLite's result: SQLITE_BUSY when another
    // connection holds the lock an immediate or exclusive begin needs,
    // beyond the busy timeout; SQLITE_ERROR inside a transaction already
    void begin_deferred();
    void begin_immediate();
    void begin_exclusive();

    /**
     * @brief Copies the main database to a file, with SQLite's online
     *        backup.
     *
     * @param[in] _dest_path  the file written; whatever it held is replaced.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a path holding a
     *         NUL; with SQLite's result for a failure: SQLITE_CANTOPEN for a
     *         file that cannot be opened, SQLITE_BUSY or SQLITE_LOCKED when a
     *         database is in use, either worth a retry.
     */
    void backup_to(const std::string& _dest_path);

    /**
     * @brief Copies a file over the main database, with SQLite's online
     *        backup.
     *
     * @param[in] _source_path  the file read.
     * @throws as backup_to; SQLITE_NOTADB for a file that is not a database.
     */
    void backup_from(const std::string& _source_path);

    // feature queries -- what the SQLite library loaded can do, asked of it
    // at run time (d_sqlite_supports): its release, and the options it was
    // built with; client_capabilities answers for the header at compile
    // time. supports_modern_sql is CTEs, window functions, upsert, RETURNING
    // and generated columns together; is_thread_safe, a library built to be
    // shared between threads (sqlite3_threadsafe); supports_encryption, a
    // build that encrypts, SQLCipher's or one with a codec
    static bool
    supports_wal() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_WAL);
    }

    static bool
    supports_json() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_JSON);
    }

    static bool
    supports_fts5() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_FTS5);
    }

    static bool
    supports_strict_tables() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_STRICT_TABLES);
    }

    static bool
    supports_returning() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_RETURNING);
    }

    static bool
    supports_upsert() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_UPSERT);
    }

    static bool
    supports_window_functions() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_WINDOW_FUNCTIONS);
    }

    static bool
    supports_cte() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_CTE);
    }

    static bool
    supports_generated_columns() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_GENERATED_COLUMNS);
    }

    static bool
    supports_modern_sql() noexcept
    {
        return ( (supports_cte())               &&
                 (supports_window_functions())  &&
                 (supports_upsert())            &&
                 (supports_returning())         &&
                 (supports_generated_columns()) );
    }

    static bool
    is_thread_safe() noexcept
    {
        return (face::sqlite::library_threadsafe() != 0);
    }

    static bool
    supports_encryption() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_ENCRYPTION);
    }

    // supports_serialization, supports_extension_loading -- whether serialize
    // and deserialize, and load_extension and enable_load_extension, can
    // work: the library loaded has them, and the layer's build calls them
    // (sqlite_database.h, sqlite_connection.h)
    static bool
    supports_serialization() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_SERIALIZE);
    }

    static bool
    supports_extension_loading() noexcept
    {
        return face::sqlite::supports(D_SQLITE_FEATURE_LOAD_EXTENSION);
    }

    // type mapping -- SQLite's types as field_type, and a field_type as the
    // type a column is declared with, as section 2 maps them
    static field_type
    map_affinity(
        sqlite_affinity _affinity
    ) noexcept
    {
        return sqlite_affinity_to_field_type(_affinity);
    }

    static field_type
    map_declared_type(
        sqlite_declared_type _declared
    ) noexcept
    {
        return sqlite_declared_type_to_field_type(_declared);
    }

    static field_type
    map_column_type(
        int _sqlite_type
    ) noexcept
    {
        return sqlite_affinity_to_field_type(
            sqlite_column_type_to_affinity(_sqlite_type));
    }

    static const char*
    sql_type_name(
        field_type _type
    ) noexcept
    {
        return field_type_to_sqlite_sql(_type);
    }

    // configuration -- the SQLite settings, the file among them as the
    // generic configuration's `database` now names it (connect(config) and
    // open change it); setting them names the file too, and both apply at
    // the next open
    sqlite_connect_config
         get_sqlite_config() const;
    void set_sqlite_config(const sqlite_connect_config& _config);

    // is_in_memory -- whether the connection's database is in memory:
    // ":memory:"; a path opened with the memory flag; or, with the uri flag,
    // a URI naming ":memory:" or with mode=memory ("file::memory:",
    // "file:name?mode=memory"), as SQLite reads them; or "main" since
    // deserialize replaced it, until the next open. An empty path is not:
    // SQLite opens it as a private temporary database on disk
    bool is_in_memory() const noexcept;

    // get_face -- the SQLite layer's face, for the layer's other calls;
    // closing or replacing the connection through it is the connection's to
    // do, not the face's
    face::sqlite&       get_face() noexcept;
    const face::sqlite& get_face() const noexcept;

    // hooks (sqlite.cpp) -- database_connection calls these, programs the
    // operations they implement, whose contracts they keep
    void            connect_impl();
    void            disconnect_impl() noexcept;
    bool            is_connected_impl() const noexcept;
    bool            ping_impl() const noexcept;
    face::rows      execute_query_impl(const std::string& _query);
    re_std::int64_t execute_update_impl(const std::string& _query);
    face::sqlite_statement
                    prepare_impl(const std::string& _query);
    std::string     get_server_version_impl() const;
    re_std::int64_t get_last_insert_id_impl() const noexcept;
    void            begin_transaction_impl();
    void            begin_transaction_impl(isolation_level _isolation);
    void            commit_impl();
    void            rollback_impl();
    bool            in_transaction_impl() const noexcept;
    isolation_level get_isolation_level_impl() const;
    void            set_isolation_level_impl(isolation_level _level);
    void            create_savepoint_impl(const std::string& _name);
    void            release_savepoint_impl(const std::string& _name);
    void            rollback_to_savepoint_impl(const std::string& _name);
    struct ::sqlite3*
                    native_handle_impl() const noexcept;

    /**
     * @brief Copies a database of the connection into bytes, as its file
     *        would hold them; an in-memory or temporary database as a
     *        backup to a file would write it.
     *
     * @note A database with no pages yet is given its first, by an empty
     *       transaction; inside a transaction, or opened read-only, it has
     *       none, and its bytes are none.
     *
     * @param[in] _schema  the database: "main", "temp", or an attached one's
     *                     name.
     * @return the bytes.
     * @throws query_exception with D_DB_NOT_FOUND for no such database; with
     *         D_DB_UNSUPPORTED where SQLite cannot (supports_serialization);
     *         with D_DB_INVALID_ARGUMENT for a name holding a NUL; with
     *         SQLite's result for a database that cannot be read, SQLITE_BUSY
     *         while another connection's lock keeps it, SQLITE_NOMEM when
     *         memory runs out.
     */
    std::vector<re_std::uint8_t> serialize(
        const std::string& _schema = "main");

    /**
     * @brief Replaces a database of the connection with one made of bytes,
     *        which SQLite copies and keeps in memory: the connection's writes
     *        change its copy, not `_data`, nor any file.
     *
     * @note The database stays in memory until it is detached or the
     *       connection opens again; replacing "main" leaves the file opened
     *       as it was. Bytes that are no database are found out when the
     *       database is first read, with SQLITE_NOTADB.
     *
     * @param[in] _data       the bytes, as serialize gives them, or as a
     *                        database file holds them.
     * @param[in] _schema     "main", or an attached one's name; never "temp".
     * @param[in] _read_only  whether the database is to refuse writes.
     * @throws query_exception with SQLITE_ERROR for "temp" or no such
     *         database; with SQLITE_BUSY inside a read transaction, or while
     *         the database is being backed up; with D_DB_UNSUPPORTED where
     *         SQLite cannot (supports_serialization); with
     *         D_DB_INVALID_ARGUMENT for a name holding a NUL; with
     *         SQLITE_NOMEM when memory runs out.
     */
    void deserialize(const std::vector<re_std::uint8_t>& _data,
                     const std::string&                  _schema = "main",
                     bool                                _read_only = false);

    /**
     * @brief Loads an SQLite extension from a shared library into the
     *        connection, through SQLite's C API, which this call alone turns
     *        on: SQL's load_extension() stays as enable_load_extension left
     *        it.
     *
     * @note Before SQLite 3.13.0 loading is refused unless
     *       enable_load_extension(true) turned it on.
     *
     * @param[in] _path         the shared library, as the platform's loader
     *                          finds it: a path, or a name it searches for;
     *                          SQLite tries it with the platform's suffix
     *                          too.
     * @param[in] _entry_point  the extension's initialization function; ""
     *                          for the one SQLite derives from the file's
     *                          name, or else sqlite3_extension_init.
     * @throws query_exception with SQLITE_ERROR and SQLite's description when
     *         the library cannot be loaded, has no such entry point, or fails
     *         to initialize, and before 3.13.0 when loading is off; with
     *         D_DB_UNSUPPORTED where SQLite cannot
     *         (supports_extension_loading); with D_DB_INVALID_ARGUMENT for
     *         text holding a NUL.
     */
    void load_extension(const std::string& _path,
                        const std::string& _entry_point = "");

    /**
     * @brief Turns extension loading on or off, SQL's load_extension() and
     *        the C API together (sqlite3_enable_load_extension).
     *
     * @warning With it on, any SQL the connection runs can load a shared
     *          library, SQL from outside the program among it.
     *          load_extension needs it only before SQLite 3.13.0.
     *
     * @param[in] _enabled  whether to allow loading.
     * @throws query_exception with D_DB_UNSUPPORTED where SQLite cannot
     *         (supports_extension_loading).
     */
    void enable_load_extension(bool _enabled);

private:
    /**
     * @brief Raises a failure of the SQLite layer, as sqlite_layer::raise
     *        does; a success returns.
     *
     * @param[in] _result     the call's result.
     * @param[in] _kind       which exception a failure throws.
     * @param[in] _operation  the operation's name, for the message.
     * @throws as sqlite_layer::raise, unless `_result` is SQLITE_OK.
     */
    void m_check(int          _result,
                 failure_kind _kind,
                 const char*  _operation) const;

    sqlite_connect_config m_sqlite_config;
    face::sqlite          m_db;
    bool                  m_main_deserialized;  // deserialize replaced main
};


//==============================================================================
// 9.  EXPRESSION DETECTORS
//==============================================================================
// Sections 9 to 13, once sqlite_traits.hpp and sqlite_concepts.hpp, detect
// which of sqlite_connection's members a type has: as traits from C++17 and
// as concepts under C++20. A detector is the type of one member's call,
// which is_detected tests.


// 9.1    File-based connection
//------------------------------------------------------------------------------
// sqlite_open_t
//   detector: open(const std::string&) method.
// wraps sqlite3_open() or sqlite3_open_v2().
template<typename T>
using sqlite_open_t = decltype(std::declval<T&>().open(
    std::declval<const std::string&>()));

// sqlite_open_with_flags_t
//   detector: open(const std::string&, sqlite_open_flag) method.
// wraps sqlite3_open_v2() with flags parameter.
template<typename T>
using sqlite_open_with_flags_t = decltype(std::declval<T&>().open(
    std::declval<const std::string&>(),
    std::declval<sqlite_open_flag>()));

// sqlite_close_t
//   detector: close() method.
template<typename T>
using sqlite_close_t = decltype(std::declval<T&>().close());

// 9.2    Journal and WAL management
//------------------------------------------------------------------------------
// sqlite_set_journal_mode_t
//   detector: set_journal_mode(const std::string&) method.
template<typename T>
using sqlite_set_journal_mode_t =
    decltype(std::declval<T&>().set_journal_mode(
        std::declval<const std::string&>()));

// sqlite_get_journal_mode_t
//   detector: get_journal_mode() const method.
template<typename T>
using sqlite_get_journal_mode_t =
    decltype(std::declval<const T&>().get_journal_mode());

// sqlite_checkpoint_t
//   detector: checkpoint(int) method.
// wraps sqlite3_wal_checkpoint_v2().
template<typename T>
using sqlite_checkpoint_t = decltype(std::declval<T&>().checkpoint(
    std::declval<int>()));

// 9.3    PRAGMA interface
//------------------------------------------------------------------------------
// sqlite_set_pragma_t
//   detector: set_pragma(const std::string&, const std::string&)
// method.
template<typename T>
using sqlite_set_pragma_t =
    decltype(std::declval<T&>().set_pragma(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// sqlite_get_pragma_t
//   detector: get_pragma(const std::string&) const method.
template<typename T>
using sqlite_get_pragma_t =
    decltype(std::declval<const T&>().get_pragma(
        std::declval<const std::string&>()));

// 9.4    Backup API
//------------------------------------------------------------------------------
// sqlite_backup_to_t
//   detector: backup_to(const std::string&) method.
template<typename T>
using sqlite_backup_to_t = decltype(std::declval<T&>().backup_to(
    std::declval<const std::string&>()));

// sqlite_backup_from_t
//   detector: backup_from(const std::string&) method.
template<typename T>
using sqlite_backup_from_t = decltype(std::declval<T&>().backup_from(
    std::declval<const std::string&>()));

// 9.5    Busy handler and timeout
//------------------------------------------------------------------------------
// sqlite_set_busy_timeout_t
//   detector: set_busy_timeout(int) method.
template<typename T>
using sqlite_set_busy_timeout_t =
    decltype(std::declval<T&>().set_busy_timeout(
        std::declval<int>()));

// 9.6    Database attachment
//------------------------------------------------------------------------------
// sqlite_attach_t
//   detector: attach_database(const std::string&, const std::string&)
// method.
template<typename T>
using sqlite_attach_t = decltype(std::declval<T&>().attach_database(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// sqlite_detach_t
//   detector: detach_database(const std::string&) method.
template<typename T>
using sqlite_detach_t = decltype(std::declval<T&>().detach_database(
    std::declval<const std::string&>()));

// 9.7    Schema introspection
//------------------------------------------------------------------------------
// sqlite_has_table_t
//   detector: has_table(const std::string&) const method.
template<typename T>
using sqlite_has_table_t =
    decltype(std::declval<const T&>().has_table(
        std::declval<const std::string&>()));

// sqlite_list_tables_t
//   detector: list_tables() const method.
template<typename T>
using sqlite_list_tables_t =
    decltype(std::declval<const T&>().list_tables());

// 9.8    Extension loading
//------------------------------------------------------------------------------
// sqlite_load_extension_t
//   detector: load_extension(const std::string&) method.
template<typename T>
using sqlite_load_extension_t =
    decltype(std::declval<T&>().load_extension(
        std::declval<const std::string&>()));

// sqlite_enable_load_extension_t
//   detector: enable_load_extension(bool) method.
template<typename T>
using sqlite_enable_load_extension_t =
    decltype(std::declval<T&>().enable_load_extension(
        std::declval<bool>()));

// 9.9    Serialization
//------------------------------------------------------------------------------
// sqlite_serialize_t
//   detector: serialize() method.
template<typename T>
using sqlite_serialize_t =
    decltype(std::declval<T&>().serialize());

// sqlite_deserialize_t
//   detector: deserialize(const std::vector<re_std::uint8_t>&) method.
template<typename T>
using sqlite_deserialize_t = decltype(std::declval<T&>().deserialize(
    std::declval<const std::vector<re_std::uint8_t>&>()));

// 9.10   Transaction modes
//------------------------------------------------------------------------------
// sqlite_begin_deferred_t
//   detector: begin_deferred() method.
template<typename T>
using sqlite_begin_deferred_t =
    decltype(std::declval<T&>().begin_deferred());

// sqlite_begin_immediate_t
//   detector: begin_immediate() method.
template<typename T>
using sqlite_begin_immediate_t =
    decltype(std::declval<T&>().begin_immediate());

// sqlite_begin_exclusive_t
//   detector: begin_exclusive() method.
template<typename T>
using sqlite_begin_exclusive_t =
    decltype(std::declval<T&>().begin_exclusive());


//==============================================================================
// 10.  TAGGED CAPABILITY TRAITS
//==============================================================================
// Each trait is a type with ::value, true where every detector it names finds
// its member; name_v is the value, where the language has variable templates.


// 10.1   Traits
//------------------------------------------------------------------------------
// 10.1.1
// has_sqlite_journal
//   trait: checks if type T supports journal mode management
// (set_journal_mode + get_journal_mode).
template<typename T>
struct has_sqlite_journal : djinterp::conjunction<
    is_detected<sqlite_set_journal_mode_t, clean_t<T>>,
    is_detected<sqlite_get_journal_mode_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_journal_v = has_sqlite_journal<clean_t<T>>::value;
#endif

// 10.1.2
// has_sqlite_pragma
//   trait: checks if type T supports the PRAGMA interface
// (set_pragma + get_pragma).
template<typename T>
struct has_sqlite_pragma : djinterp::conjunction<
    is_detected<sqlite_set_pragma_t, clean_t<T>>,
    is_detected<sqlite_get_pragma_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_pragma_v = has_sqlite_pragma<clean_t<T>>::value;
#endif

// 10.1.3
// has_sqlite_backup
//   trait: checks if type T supports the backup API
// (backup_to + backup_from).
template<typename T>
struct has_sqlite_backup : djinterp::conjunction<
    is_detected<sqlite_backup_to_t, clean_t<T>>,
    is_detected<sqlite_backup_from_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_backup_v = has_sqlite_backup<clean_t<T>>::value;
#endif

// 10.1.4
// has_sqlite_attach
//   trait: checks if type T supports database attachment
// (attach + detach).
template<typename T>
struct has_sqlite_attach : djinterp::conjunction<
    is_detected<sqlite_attach_t, clean_t<T>>,
    is_detected<sqlite_detach_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_attach_v = has_sqlite_attach<clean_t<T>>::value;
#endif

// 10.1.5
// has_sqlite_schema_query
//   trait: checks if type T supports schema introspection
// (has_table + list_tables).
template<typename T>
struct has_sqlite_schema_query : djinterp::conjunction<
    is_detected<sqlite_has_table_t, clean_t<T>>,
    is_detected<sqlite_list_tables_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_schema_query_v =
        has_sqlite_schema_query<clean_t<T>>::value;
#endif

// 10.1.6
// has_sqlite_extension_loading
//   trait: checks if type T supports extension loading
// (load_extension + enable_load_extension).
template<typename T>
struct has_sqlite_extension_loading : djinterp::conjunction<
    is_detected<sqlite_load_extension_t, clean_t<T>>,
    is_detected<sqlite_enable_load_extension_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_extension_loading_v =
        has_sqlite_extension_loading<clean_t<T>>::value;
#endif

// 10.1.7
// has_sqlite_serialization
//   trait: checks if type T supports serialization
// (serialize + deserialize).
template<typename T>
struct has_sqlite_serialization : djinterp::conjunction<
    is_detected<sqlite_serialize_t, clean_t<T>>,
    is_detected<sqlite_deserialize_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_serialization_v =
        has_sqlite_serialization<clean_t<T>>::value;
#endif

// 10.1.8
// has_sqlite_transaction_modes
//   trait: checks if type T supports SQLite transaction modes
// (deferred + immediate + exclusive).
template<typename T>
struct has_sqlite_transaction_modes : djinterp::conjunction<
    is_detected<sqlite_begin_deferred_t, clean_t<T>>,
    is_detected<sqlite_begin_immediate_t, clean_t<T>>,
    is_detected<sqlite_begin_exclusive_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_transaction_modes_v =
        has_sqlite_transaction_modes<clean_t<T>>::value;
#endif

// 10.1.9
// is_sqlite_connection
//   trait: compound trait verifying type T implements a SQLite
// connection interface (connection + journal + pragma + schema
// queries + transaction modes).
template<typename T>
struct is_sqlite_connection : djinterp::conjunction<
    has_connect<clean_t<T>>,
    has_disconnect<clean_t<T>>,
    has_execute_query<clean_t<T>>,
    has_sqlite_journal<clean_t<T>>,
    has_sqlite_pragma<clean_t<T>>,
    has_sqlite_schema_query<clean_t<T>>,
    has_sqlite_transaction_modes<clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool is_sqlite_connection_v =
        is_sqlite_connection<clean_t<T>>::value;
#endif


//==============================================================================
// 11.  TAGLESS CAPABILITY TRAITS
//==============================================================================
// The same capabilities as constexpr bool variable templates, for a condition
// that needs no type: one member each, then the sets.


// 11.1   Individual capabilities
//------------------------------------------------------------------------------
// sqlite_can_open_with_flags
//   tagless trait: true if T has open() taking flags.
template<typename T,
         typename = void>
constexpr bool sqlite_can_open_with_flags = false;

template<typename T>
constexpr bool sqlite_can_open_with_flags<T,
    std::void_t<sqlite_open_with_flags_t<T>>> = true;

// sqlite_can_set_journal_mode
//   tagless trait: true if T has set_journal_mode().
template<typename T,
         typename = void>
constexpr bool sqlite_can_set_journal_mode = false;

template<typename T>
constexpr bool sqlite_can_set_journal_mode<T,
    std::void_t<sqlite_set_journal_mode_t<T>>> = true;

// sqlite_can_checkpoint
//   tagless trait: true if T has checkpoint().
template<typename T,
         typename = void>
constexpr bool sqlite_can_checkpoint = false;

template<typename T>
constexpr bool sqlite_can_checkpoint<T,
    std::void_t<sqlite_checkpoint_t<T>>> = true;

// sqlite_can_set_pragma
//   tagless trait: true if T has set_pragma().
template<typename T,
         typename = void>
constexpr bool sqlite_can_set_pragma = false;

template<typename T>
constexpr bool sqlite_can_set_pragma<T,
    std::void_t<sqlite_set_pragma_t<T>>> = true;

// sqlite_can_backup
//   tagless trait: true if T has backup_to().
template<typename T,
         typename = void>
constexpr bool sqlite_can_backup = false;

template<typename T>
constexpr bool sqlite_can_backup<T,
    std::void_t<sqlite_backup_to_t<T>>> = true;

// sqlite_can_attach
//   tagless trait: true if T has attach_database().
template<typename T,
         typename = void>
constexpr bool sqlite_can_attach = false;

template<typename T>
constexpr bool sqlite_can_attach<T,
    std::void_t<sqlite_attach_t<T>>> = true;

// sqlite_can_load_extension
//   tagless trait: true if T has load_extension().
template<typename T,
         typename = void>
constexpr bool sqlite_can_load_extension = false;

template<typename T>
constexpr bool sqlite_can_load_extension<T,
    std::void_t<sqlite_load_extension_t<T>>> = true;

// sqlite_can_serialize
//   tagless trait: true if T has serialize().
template<typename T,
         typename = void>
constexpr bool sqlite_can_serialize = false;

template<typename T>
constexpr bool sqlite_can_serialize<T,
    std::void_t<sqlite_serialize_t<T>>> = true;

// sqlite_can_set_busy_timeout
//   tagless trait: true if T has set_busy_timeout().
template<typename T,
         typename = void>
constexpr bool sqlite_can_set_busy_timeout = false;

template<typename T>
constexpr bool sqlite_can_set_busy_timeout<T,
    std::void_t<sqlite_set_busy_timeout_t<T>>> = true;

// sqlite_can_query_schema
//   tagless trait: true if T has has_table().
template<typename T,
         typename = void>
constexpr bool sqlite_can_query_schema = false;

template<typename T>
constexpr bool sqlite_can_query_schema<T,
    std::void_t<sqlite_has_table_t<T>>> = true;

// 11.2   Compound capabilities
//------------------------------------------------------------------------------
// sqlite_does_journal
//   tagless trait: true if T supports full journal mode management.
template<typename T>
constexpr bool sqlite_does_journal =
    ( sqlite_can_set_journal_mode<clean_t<T>> &&
      sqlite_can_checkpoint<clean_t<T>> );

// sqlite_does_pragma
//   tagless trait: true if T supports full PRAGMA interface.
template<typename T,
         typename = void>
constexpr bool sqlite_does_pragma = false;

template<typename T>
constexpr bool sqlite_does_pragma<T, std::void_t<
    sqlite_set_pragma_t<T>,
    sqlite_get_pragma_t<T>>> = true;

// sqlite_does_backup
//   tagless trait: true if T supports full backup (to + from).
template<typename T,
         typename = void>
constexpr bool sqlite_does_backup = false;

template<typename T>
constexpr bool sqlite_does_backup<T, std::void_t<
    sqlite_backup_to_t<T>,
    sqlite_backup_from_t<T>>> = true;

// sqlite_does_attach
//   tagless trait: true if T supports database attachment (attach +
// detach).
template<typename T,
         typename = void>
constexpr bool sqlite_does_attach = false;

template<typename T>
constexpr bool sqlite_does_attach<T, std::void_t<
    sqlite_attach_t<T>,
    sqlite_detach_t<T>>> = true;

// sqlite_is_full_connection
//   tagless trait: true if T satisfies the complete SQLite connection
// interface.
template<typename T>
constexpr bool sqlite_is_full_connection =
    ( can_connect<clean_t<T>>               &&
      can_disconnect<clean_t<T>>            &&
      can_execute_query<clean_t<T>>         &&
      sqlite_does_journal<clean_t<T>>       &&
      sqlite_does_pragma<clean_t<T>>        &&
      sqlite_can_query_schema<clean_t<T>> );


//==============================================================================
// 12.  SFINAE HELPERS
//==============================================================================


// 12.1   enable_if helpers
//------------------------------------------------------------------------------
// enable_if_sqlite_connection
//   type: SFINAE helper for SQLite connection constraints.
template<typename T>
using enable_if_sqlite_connection =
    typename std::enable_if<is_sqlite_connection<clean_t<T>>::value>::type;

// enable_if_has_sqlite_journal
//   type: SFINAE helper for SQLite journal constraints.
template<typename T>
using enable_if_has_sqlite_journal =
    typename std::enable_if<has_sqlite_journal<clean_t<T>>::value>::type;

// enable_if_has_sqlite_pragma
//   type: SFINAE helper for SQLite PRAGMA constraints.
template<typename T>
using enable_if_has_sqlite_pragma =
    typename std::enable_if<has_sqlite_pragma<clean_t<T>>::value>::type;

// enable_if_has_sqlite_backup
//   type: SFINAE helper for SQLite backup constraints.
template<typename T>
using enable_if_has_sqlite_backup =
    typename std::enable_if<has_sqlite_backup<clean_t<T>>::value>::type;


//==============================================================================
// 13.  C++20 CONCEPTS
//==============================================================================
// Each concept forwards to a trait or a tagless capability above. They live
// in djinterp::concepts (2026.10.21), where a concept may take the name of
// the class it constrains; each former name in djinterp forwards to its
// concept for one release. They exist where the compiler has concepts; the
// traits serve at C++17 without them, as in functor.hpp and monoid.hpp.


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace concepts
{

// 13.1   Connection concepts
//------------------------------------------------------------------------------
// sqlite_connection
//   concept: constrains types implementing the SQLite connection interface.
template<typename Type>
concept sqlite_connection =
    is_sqlite_connection<clean_t<Type>>::value;

// non_sqlite_connection
//   concept: constrains types that do not implement the SQLite connection
// interface.
template<typename Type>
concept non_sqlite_connection =
    !sqlite_connection<Type>;

// sqlite_openable_connection
//   concept: constrains SQLite connections exposing open(const string&).
template<typename Type>
concept sqlite_openable_connection =
    is_detected<sqlite_open_t, clean_t<Type>>::value;

// sqlite_flag_openable_connection
//   concept: constrains SQLite connections exposing open(path, flags).
template<typename Type>
concept sqlite_flag_openable_connection =
    sqlite_can_open_with_flags<clean_t<Type>>;

// sqlite_closable_connection
//   concept: constrains SQLite connections exposing close().
template<typename Type>
concept sqlite_closable_connection =
    is_detected<sqlite_close_t, clean_t<Type>>::value;

// sqlite_basic_file_connection
//   concept: constrains SQLite connections exposing both open() and close().
template<typename Type>
concept sqlite_basic_file_connection =
    ( sqlite_openable_connection<Type> &&
      sqlite_closable_connection<Type> );

// 13.2   Capability concepts
//------------------------------------------------------------------------------
// sqlite_journal_connection
//   concept: constrains SQLite connections supporting journal mode
// management.
template<typename Type>
concept sqlite_journal_connection =
    has_sqlite_journal<clean_t<Type>>::value;

// sqlite_pragma_connection
//   concept: constrains SQLite connections supporting the PRAGMA interface.
template<typename Type>
concept sqlite_pragma_connection =
    has_sqlite_pragma<clean_t<Type>>::value;

// sqlite_backup_connection
//   concept: constrains SQLite connections supporting backup to/from.
template<typename Type>
concept sqlite_backup_connection =
    has_sqlite_backup<clean_t<Type>>::value;

// sqlite_attach_connection
//   concept: constrains SQLite connections supporting ATTACH and DETACH.
template<typename Type>
concept sqlite_attach_connection =
    has_sqlite_attach<clean_t<Type>>::value;

// sqlite_schema_query_connection
//   concept: constrains SQLite connections supporting schema introspection.
template<typename Type>
concept sqlite_schema_query_connection =
    has_sqlite_schema_query<clean_t<Type>>::value;

// sqlite_extension_loading_connection
//   concept: constrains SQLite connections supporting extension loading.
template<typename Type>
concept sqlite_extension_loading_connection =
    has_sqlite_extension_loading<clean_t<Type>>::value;

// sqlite_serialization_connection
//   concept: constrains SQLite connections supporting serialization and
// deserialization.
template<typename Type>
concept sqlite_serialization_connection =
    has_sqlite_serialization<clean_t<Type>>::value;

// sqlite_busy_timeout_connection
//   concept: constrains SQLite connections exposing set_busy_timeout(int).
template<typename Type>
concept sqlite_busy_timeout_connection =
    sqlite_can_set_busy_timeout<clean_t<Type>>;

// sqlite_checkpoint_connection
//   concept: constrains SQLite connections exposing checkpoint(int).
template<typename Type>
concept sqlite_checkpoint_connection =
    sqlite_can_checkpoint<clean_t<Type>>;

// sqlite_attachable_connection
//   concept: constrains SQLite connections exposing
// attach_database(path, name).
template<typename Type>
concept sqlite_attachable_connection =
    sqlite_can_attach<clean_t<Type>>;

// sqlite_detachable_connection
//   concept: constrains SQLite connections exposing detach_database(name).
template<typename Type>
concept sqlite_detachable_connection =
    is_detected<sqlite_detach_t, clean_t<Type>>::value;

// sqlite_schema_table_query_connection
//   concept: constrains SQLite connections exposing has_table(name).
template<typename Type>
concept sqlite_schema_table_query_connection =
    sqlite_can_query_schema<clean_t<Type>>;

// sqlite_table_name_query_connection
//   concept: constrains SQLite connections exposing list_tables().
template<typename Type>
concept sqlite_table_name_query_connection =
    is_detected<sqlite_list_tables_t, clean_t<Type>>::value;

// sqlite_extension_loadable_connection
//   concept: constrains SQLite connections exposing load_extension(path).
template<typename Type>
concept sqlite_extension_loadable_connection =
    sqlite_can_load_extension<clean_t<Type>>;

// sqlite_extension_toggle_connection
//   concept: constrains SQLite connections exposing
// enable_load_extension(bool).
template<typename Type>
concept sqlite_extension_toggle_connection =
    is_detected<sqlite_enable_load_extension_t, clean_t<Type>>::value;

// sqlite_serializable_connection
//   concept: constrains SQLite connections exposing serialize().
template<typename Type>
concept sqlite_serializable_connection =
    sqlite_can_serialize<clean_t<Type>>;

// sqlite_deserializable_connection
//   concept: constrains SQLite connections exposing deserialize(bytes).
template<typename Type>
concept sqlite_deserializable_connection =
    is_detected<sqlite_deserialize_t, clean_t<Type>>::value;

// 13.3   Transaction mode concepts
//------------------------------------------------------------------------------
// sqlite_transaction_modes_connection
//   concept: constrains SQLite connections supporting deferred, immediate,
// and exclusive begin modes.
template<typename Type>
concept sqlite_transaction_modes_connection =
    has_sqlite_transaction_modes<clean_t<Type>>::value;

// sqlite_deferred_transaction_connection
//   concept: constrains SQLite connections exposing begin_deferred().
template<typename Type>
concept sqlite_deferred_transaction_connection =
    is_detected<sqlite_begin_deferred_t, clean_t<Type>>::value;

// sqlite_immediate_transaction_connection
//   concept: constrains SQLite connections exposing begin_immediate().
template<typename Type>
concept sqlite_immediate_transaction_connection =
    is_detected<sqlite_begin_immediate_t, clean_t<Type>>::value;

// sqlite_exclusive_transaction_connection
//   concept: constrains SQLite connections exposing begin_exclusive().
template<typename Type>
concept sqlite_exclusive_transaction_connection =
    is_detected<sqlite_begin_exclusive_t, clean_t<Type>>::value;

// 13.4   Tagless capability concepts
//------------------------------------------------------------------------------
// sqlite_journaling_connection
//   concept: constrains types satisfying the tagless journal capability set.
template<typename Type>
concept sqlite_journaling_connection =
    sqlite_does_journal<clean_t<Type>>;

// sqlite_pragmatic_connection
//   concept: constrains types satisfying the tagless PRAGMA capability set.
template<typename Type>
concept sqlite_pragmatic_connection =
    sqlite_does_pragma<clean_t<Type>>;

// sqlite_backup_capable_connection
//   concept: constrains types satisfying the tagless backup capability set.
template<typename Type>
concept sqlite_backup_capable_connection =
    sqlite_does_backup<clean_t<Type>>;

// sqlite_attached_connection
//   concept: constrains types satisfying the tagless attach capability set.
template<typename Type>
concept sqlite_attached_connection =
    sqlite_does_attach<clean_t<Type>>;

// sqlite_full_connection
//   concept: constrains types satisfying the tagless full SQLite connection
// capability set.
template<typename Type>
concept sqlite_full_connection =
    sqlite_is_full_connection<clean_t<Type>>;

}  // namespace concepts

// 13.5   Former names
//------------------------------------------------------------------------------
// Sqlite_connection
//   concept: concepts::sqlite_connection, by its former name, for one release.
template<typename Type>
concept Sqlite_connection =
    concepts::sqlite_connection<Type>;

// non_sqlite_connection
//   concept: concepts::non_sqlite_connection, by its former name, for one
// release.
template<typename Type>
concept non_sqlite_connection =
    concepts::non_sqlite_connection<Type>;

// sqlite_openable_connection
//   concept: concepts::sqlite_openable_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_openable_connection =
    concepts::sqlite_openable_connection<Type>;

// sqlite_flag_openable_connection
//   concept: concepts::sqlite_flag_openable_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_flag_openable_connection =
    concepts::sqlite_flag_openable_connection<Type>;

// sqlite_closable_connection
//   concept: concepts::sqlite_closable_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_closable_connection =
    concepts::sqlite_closable_connection<Type>;

// sqlite_basic_file_connection
//   concept: concepts::sqlite_basic_file_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_basic_file_connection =
    concepts::sqlite_basic_file_connection<Type>;

// sqlite_journal_connection
//   concept: concepts::sqlite_journal_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_journal_connection =
    concepts::sqlite_journal_connection<Type>;

// sqlite_pragma_connection
//   concept: concepts::sqlite_pragma_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_pragma_connection =
    concepts::sqlite_pragma_connection<Type>;

// sqlite_backup_connection
//   concept: concepts::sqlite_backup_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_backup_connection =
    concepts::sqlite_backup_connection<Type>;

// sqlite_attach_connection
//   concept: concepts::sqlite_attach_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_attach_connection =
    concepts::sqlite_attach_connection<Type>;

// sqlite_schema_query_connection
//   concept: concepts::sqlite_schema_query_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_schema_query_connection =
    concepts::sqlite_schema_query_connection<Type>;

// sqlite_extension_loading_connection
//   concept: concepts::sqlite_extension_loading_connection, by its former name,
// for one release.
template<typename Type>
concept sqlite_extension_loading_connection =
    concepts::sqlite_extension_loading_connection<Type>;

// sqlite_serialization_connection
//   concept: concepts::sqlite_serialization_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_serialization_connection =
    concepts::sqlite_serialization_connection<Type>;

// sqlite_busy_timeout_connection
//   concept: concepts::sqlite_busy_timeout_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_busy_timeout_connection =
    concepts::sqlite_busy_timeout_connection<Type>;

// sqlite_checkpoint_connection
//   concept: concepts::sqlite_checkpoint_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_checkpoint_connection =
    concepts::sqlite_checkpoint_connection<Type>;

// sqlite_attachable_connection
//   concept: concepts::sqlite_attachable_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_attachable_connection =
    concepts::sqlite_attachable_connection<Type>;

// sqlite_detachable_connection
//   concept: concepts::sqlite_detachable_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_detachable_connection =
    concepts::sqlite_detachable_connection<Type>;

// sqlite_schema_table_query_connection
//   concept: concepts::sqlite_schema_table_query_connection, by its former
// name, for one release.
template<typename Type>
concept sqlite_schema_table_query_connection =
    concepts::sqlite_schema_table_query_connection<Type>;

// sqlite_table_name_query_connection
//   concept: concepts::sqlite_table_name_query_connection, by its former name,
// for one release.
template<typename Type>
concept sqlite_table_name_query_connection =
    concepts::sqlite_table_name_query_connection<Type>;

// sqlite_extension_loadable_connection
//   concept: concepts::sqlite_extension_loadable_connection, by its former
// name, for one release.
template<typename Type>
concept sqlite_extension_loadable_connection =
    concepts::sqlite_extension_loadable_connection<Type>;

// sqlite_extension_toggle_connection
//   concept: concepts::sqlite_extension_toggle_connection, by its former name,
// for one release.
template<typename Type>
concept sqlite_extension_toggle_connection =
    concepts::sqlite_extension_toggle_connection<Type>;

// sqlite_serializable_connection
//   concept: concepts::sqlite_serializable_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_serializable_connection =
    concepts::sqlite_serializable_connection<Type>;

// sqlite_deserializable_connection
//   concept: concepts::sqlite_deserializable_connection, by its former name,
// for one release.
template<typename Type>
concept sqlite_deserializable_connection =
    concepts::sqlite_deserializable_connection<Type>;

// sqlite_transaction_modes_connection
//   concept: concepts::sqlite_transaction_modes_connection, by its former name,
// for one release.
template<typename Type>
concept sqlite_transaction_modes_connection =
    concepts::sqlite_transaction_modes_connection<Type>;

// sqlite_deferred_transaction_connection
//   concept: concepts::sqlite_deferred_transaction_connection, by its former
// name, for one release.
template<typename Type>
concept sqlite_deferred_transaction_connection =
    concepts::sqlite_deferred_transaction_connection<Type>;

// sqlite_immediate_transaction_connection
//   concept: concepts::sqlite_immediate_transaction_connection, by its former
// name, for one release.
template<typename Type>
concept sqlite_immediate_transaction_connection =
    concepts::sqlite_immediate_transaction_connection<Type>;

// sqlite_exclusive_transaction_connection
//   concept: concepts::sqlite_exclusive_transaction_connection, by its former
// name, for one release.
template<typename Type>
concept sqlite_exclusive_transaction_connection =
    concepts::sqlite_exclusive_transaction_connection<Type>;

// sqlite_journaling_connection
//   concept: concepts::sqlite_journaling_connection, by its former name, for
// one release.
template<typename Type>
concept sqlite_journaling_connection =
    concepts::sqlite_journaling_connection<Type>;

// sqlite_pragmatic_connection
//   concept: concepts::sqlite_pragmatic_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_pragmatic_connection =
    concepts::sqlite_pragmatic_connection<Type>;

// sqlite_backup_capable_connection
//   concept: concepts::sqlite_backup_capable_connection, by its former name,
// for one release.
template<typename Type>
concept sqlite_backup_capable_connection =
    concepts::sqlite_backup_capable_connection<Type>;

// sqlite_attached_connection
//   concept: concepts::sqlite_attached_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_attached_connection =
    concepts::sqlite_attached_connection<Type>;

// sqlite_full_connection
//   concept: concepts::sqlite_full_connection, by its former name, for one
// release.
template<typename Type>
concept sqlite_full_connection =
    concepts::sqlite_full_connection<Type>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_SQLITE_SQLITE_HPP
