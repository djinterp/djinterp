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
* transaction modes, and the SQL features each version and build has, at
* compile time from env_sqlite.h.
*   SQLite is not a client and a server: the connection opens a file, or a
* database in memory; a column has an affinity, not a type, unless its table
* is STRICT (3.37); what SQLite can do is its version's and its build's.
*     sqlite_connection (this file)
*       -> database_connection<sqlite_connection, database_type::sqlite>
*         -> connection_template<sqlite_connection, database_type::sqlite>
*           -> connection<sqlite_connection>
*   The hooks are defined in sqlite.cpp. The header needs C++17 and includes
* no client header: env_sqlite.h includes sqlite3.h only where a program asks
* for env's detection (D_CFG_ENV_USING_SQLITE), which the compile-time
* feature queries read. Its trailing sections carry SQLite's
* capability-detection traits and C++20 concepts.
*
*
* path:      /inc/djinterp/core/db/sqlite/sqlite.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_DB_SQLITE_SQLITE_HPP
#define DJINTERP_DB_SQLITE_SQLITE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor
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


// =============================================================================
// I.   SQLITE TYPE AFFINITY
// =============================================================================
// SQLite uses a dynamic type system based on five storage classes.
// Column declarations map to one of five affinities via the affinity
// determination rules in the SQLite documentation. STRICT tables
// (3.37+) enforce exact type matching instead of affinity.

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


// =============================================================================
// II.  TYPE MAPPING
// =============================================================================

// sqlite_affinity_to_field_type
//   function: maps a sqlite_affinity to the generic djinterp
// field_type.
inline field_type sqlite_affinity_to_field_type(
    sqlite_affinity _affinity) noexcept
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

// sqlite_declared_type_to_field_type
//   function: maps a sqlite_declared_type to the generic djinterp
// field_type. This provides richer mapping than pure affinity by
// interpreting conventional type names (e.g. BOOLEAN -> boolean,
// DATETIME -> datetime, DECIMAL -> decimal).
inline field_type sqlite_declared_type_to_field_type(
    sqlite_declared_type _declared) noexcept
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

// field_type_to_sqlite_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in SQLite CREATE TABLE statements.
inline const char* field_type_to_sqlite_sql(
    field_type _type) noexcept
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

// sqlite_column_type_to_affinity
//   function: maps the integer return value of sqlite3_column_type()
// (SQLITE_INTEGER=1, SQLITE_FLOAT=2, SQLITE_TEXT=3, SQLITE_BLOB=4,
// SQLITE_NULL=5) to the sqlite_affinity enum.
inline sqlite_affinity sqlite_column_type_to_affinity(
    int _sqlite_type) noexcept
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


// =============================================================================
// III. OPEN FLAGS
// =============================================================================
// Scoped enum mirroring the SQLITE_OPEN_* constants from sqlite3.h.
// Enables type-safe flag composition without including <sqlite3.h>.

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

// operator|
//   function: bitwise OR for combining sqlite_open_flag values.
inline constexpr sqlite_open_flag operator|(sqlite_open_flag _a,
                                            sqlite_open_flag _b)
{
    return static_cast<sqlite_open_flag>(
        static_cast<int>(_a) | static_cast<int>(_b));
}

// operator&
//   function: bitwise AND for testing sqlite_open_flag values.
inline constexpr sqlite_open_flag operator&(sqlite_open_flag _a,
                                            sqlite_open_flag _b)
{
    return static_cast<sqlite_open_flag>(
        static_cast<int>(_a) & static_cast<int>(_b));
}

// operator|=
//   function: bitwise OR assignment for sqlite_open_flag.
inline constexpr sqlite_open_flag& operator|=(sqlite_open_flag&  _a,
                                              sqlite_open_flag   _b)
{
    _a = _a | _b;

    return _a;
}


// =============================================================================
// IV.  JOURNAL MODE AND TRANSACTION MODE
// =============================================================================

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

// sqlite_transaction_mode
//   enumeration: SQLite BEGIN TRANSACTION modes.
enum class sqlite_transaction_mode : re_std::uint8_t
{
    deferred       = 0,     // acquire locks lazily (default)
    immediate      = 1,     // acquire RESERVED lock immediately
    exclusive      = 2      // acquire EXCLUSIVE lock immediately
};


// =============================================================================
// V.   FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// sqlite_type_support
//   struct: compile-time data type and feature availability flags.
struct sqlite_type_support
{
#if D_ENV_SQLITE_DETECTED

    // -----------------------------------------------------------------
    // SQL features (version-gated)
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // JSON support
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // full-text search
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // spatial / R*Tree
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // WAL and journaling
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // backup and serialization
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // security
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // threading
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // compile-time omittable features
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // composite
    // -----------------------------------------------------------------

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
    static constexpr bool has_partial_index       = false;
    static constexpr bool has_filter_clause       = false;
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


// =============================================================================
// VI.  VERSION INFORMATION
// =============================================================================

// sqlite_version_info
//   struct: compile-time version decomposition.
// NOTE: SQLite encodes version as MAJOR*1000000 + MINOR*1000 + PATCH,
// which differs from the MySQL family's MAJOR*10000 + MINOR*100 + PATCH.
struct sqlite_version_info
{
#if D_ENV_SQLITE_DETECTED
    static constexpr bool          detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_SQLITE_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_SQLITE_VERSION_MAJOR;
    static constexpr re_std::uint16_t minor    = D_ENV_SQLITE_VERSION_MINOR;
    static constexpr re_std::uint16_t patch    = D_ENV_SQLITE_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_SQLITE_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t minor    = 0;
    static constexpr re_std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected SQLite version is at
    // least (major, minor, patch).
    static constexpr bool at_least(re_std::uint16_t _major,
                                   re_std::uint16_t _minor,
                                   re_std::uint16_t _patch) noexcept
    {
        return id >= (_major * 1000000u + _minor * 1000u + _patch);
    }
};


// =============================================================================
// VII. SQLITE CONNECTION CONFIGURATION
// =============================================================================

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


// =============================================================================
// VIII. SQLITE CONNECTION
// =============================================================================

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

// prepared_statement<sqlite_layer>
//   class: SQLite's prepared statements, instantiated once, in sqlite.cpp.
extern template class prepared_statement<sqlite_layer>;

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
// connections sharing its cache have not committed. The members the
// connection adds -- PRAGMAs, the journal, attached databases, the schema,
// backups -- throw as the operations do (database_connection.hpp). Not
// copyable; movable.
//
// Usage:
//   sqlite_connection conn("app.db");
//   conn.connect();
//   auto rs = conn.execute_query("SELECT id, name FROM users");
class sqlite_connection
    : public database_connection<sqlite_connection,
                                 database_type::sqlite>
{
public:
    using base_type      = database_connection<sqlite_connection,
                                               database_type::sqlite>;
    using layer_type     = sqlite_layer;
    using statement_type = prepared_statement<sqlite_layer>;
    using type_support   = sqlite_type_support;
    using version_info   = sqlite_version_info;

    // construction -- with SQLite's defaults; with a generic configuration,
    // whose `database` is the file; with an SQLite one; or with a path
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

    // -----------------------------------------------------------------
    // file-based connection
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------
    // journal and WAL management
    // -----------------------------------------------------------------

    // journal mode -- by name, in any ASCII case: DELETE, TRUNCATE, PERSIST,
    // MEMORY, WAL or OFF; get answers in lower case, as SQLite writes it.
    // set throws query_exception with D_DB_INVALID_ARGUMENT for another
    // name, and with D_DB_UNSUPPORTED when the database keeps its mode, as
    // an in-memory one keeps any but MEMORY and OFF
    void        set_journal_mode(const std::string& _mode);
    std::string get_journal_mode() const;

    // checkpoint -- the WAL's committed pages into the database:
    // 0 passive, 1 full, 2 restart, 3 truncate; query_exception with
    // D_DB_INVALID_ARGUMENT for another mode
    void        checkpoint(int _mode = 0);

    // enable_wal -- set_journal_mode("WAL")
    void        enable_wal();

    // -----------------------------------------------------------------
    // PRAGMA interface
    // -----------------------------------------------------------------

    // pragmas -- any PRAGMA by name, its value as text; get answers "" for
    // a NULL value, and throws query_exception with D_DB_NOT_FOUND for a
    // PRAGMA that reports nothing, as an unknown one does
    void        set_pragma(const std::string& _pragma,
                           const std::string& _value);
    std::string get_pragma(const std::string& _pragma) const;
    void        set_foreign_keys(bool _enabled);

    // -----------------------------------------------------------------
    // busy handler
    // -----------------------------------------------------------------

    // set_busy_timeout -- how long, in milliseconds, the connection waits
    // for another's lock before SQLITE_BUSY; 0 does not wait
    void set_busy_timeout(int _timeout_ms);

    // -----------------------------------------------------------------
    // database attachment
    // -----------------------------------------------------------------

    // attachment -- another database file, under a name, and detaching it
    void attach_database(const std::string& _file_path,
                         const std::string& _name);
    void detach_database(const std::string& _name);

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // schema -- whether a table of the main database exists, which throws
    // when it cannot tell; and its tables' names, sorted
    bool                     has_table(const std::string& _table_name) const;
    std::vector<std::string> list_tables() const;

    // -----------------------------------------------------------------
    // transaction modes
    // -----------------------------------------------------------------

    // modes -- begin a transaction that takes its locks at its first write,
    // at once, or at once keeping readers out
    void begin_deferred();
    void begin_immediate();
    void begin_exclusive();

    // -----------------------------------------------------------------
    // backups
    // -----------------------------------------------------------------

    /**
     * @brief Copies the main database to a file, with SQLite's online
     *        backup.
     *
     * @param[in] _dest_path  the file written; whatever it held is replaced.
     * @throws query_exception with SQLite's result: SQLITE_BUSY or
     *         SQLITE_LOCKED when a database is in use, either worth a retry.
     */
    void backup_to(const std::string& _dest_path);

    /**
     * @brief Copies a file over the main database, with SQLite's online
     *        backup.
     *
     * @param[in] _source_path  the file read.
     * @throws as backup_to.
     */
    void backup_from(const std::string& _source_path);

    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_wal() noexcept
    {
        return type_support::has_wal;
    }

    static constexpr bool supports_json() noexcept
    {
        return type_support::has_json;
    }

    static constexpr bool supports_fts5() noexcept
    {
        return type_support::has_fts5;
    }

    static constexpr bool supports_strict_tables() noexcept
    {
        return type_support::has_strict_tables;
    }

    static constexpr bool supports_returning() noexcept
    {
        return type_support::has_returning;
    }

    static constexpr bool supports_upsert() noexcept
    {
        return type_support::has_upsert;
    }

    static constexpr bool supports_window_functions() noexcept
    {
        return type_support::has_window_functions;
    }

    static constexpr bool supports_cte() noexcept
    {
        return type_support::has_cte;
    }

    static constexpr bool supports_generated_columns() noexcept
    {
        return type_support::has_generated_columns;
    }

    static constexpr bool supports_modern_sql() noexcept
    {
        return type_support::has_modern_sql;
    }

    static constexpr bool is_thread_safe() noexcept
    {
        return type_support::has_thread_safety;
    }

    static constexpr bool supports_encryption() noexcept
    {
        return type_support::has_encryption;
    }

    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_affinity(
        sqlite_affinity _affinity) noexcept
    {
        return sqlite_affinity_to_field_type(_affinity);
    }

    static field_type map_declared_type(
        sqlite_declared_type _declared) noexcept
    {
        return sqlite_declared_type_to_field_type(_declared);
    }

    static field_type map_column_type(int _sqlite_type) noexcept
    {
        return sqlite_affinity_to_field_type(
            sqlite_column_type_to_affinity(_sqlite_type));
    }

    static const char* sql_type_name(field_type _type) noexcept
    {
        return field_type_to_sqlite_sql(_type);
    }

    // -----------------------------------------------------------------
    // SQLite-specific configuration
    // -----------------------------------------------------------------

    // configuration -- the SQLite settings; setting them names the file
    // too, and both apply at the next open
    const sqlite_connect_config& get_sqlite_config() const noexcept;
    void                         set_sqlite_config(
                                     const sqlite_connect_config& _config);

    // is_in_memory -- whether the connection's database is in memory:
    // ":memory:", or a path opened with the memory flag. An empty path is
    // not: SQLite opens it as a private temporary database on disk
    bool is_in_memory() const noexcept;

    // get_face -- the SQLite layer's face, for the layer's other calls;
    // closing or replacing the connection through it is the connection's to
    // do, not the face's
    face::sqlite&       get_face() noexcept;
    const face::sqlite& get_face() const noexcept;

    // -----------------------------------------------------------------
    // hooks (sqlite.cpp): database_connection calls these, programs the
    // operations they implement
    // -----------------------------------------------------------------

    void            connect_helper();
    void            disconnect_helper() noexcept;
    bool            is_connected_helper() const noexcept;
    bool            ping_helper() const noexcept;
    face::rows      execute_query_helper(const std::string& _query);
    re_std::int64_t execute_update_helper(const std::string& _query);
    face::sqlite_statement
                    prepare_helper(const std::string& _query);
    std::string     get_server_version_helper() const;
    re_std::int64_t get_last_insert_id_helper() const noexcept;
    void            begin_transaction_helper();
    void            begin_transaction_helper(isolation_level _isolation);
    void            commit_helper();
    void            rollback_helper();
    bool            in_transaction_helper() const noexcept;
    isolation_level get_isolation_level_helper() const;
    void            set_isolation_level_helper(isolation_level _level);
    void            create_savepoint_helper(const std::string& _name);
    void            release_savepoint_helper(const std::string& _name);
    void            rollback_to_savepoint_helper(const std::string& _name);
    struct ::sqlite3*
                    native_handle_helper() const noexcept;

    // version-gated methods: declared where env detects the feature in
    // SQLite's header, and not yet defined -- the SQLite layer has no
    // serialization or extension loading for them to stand on

#if D_ENV_SQLITE_DETECTED
    #if D_ENV_SQLITE_HAS_SERIALIZE
    // serialize
    //   function: serializes the database into a byte vector.
    // Available since SQLite 3.36.0.
    std::vector<re_std::uint8_t> serialize();

    // deserialize
    //   function: replaces the database contents from serialized bytes.
    void deserialize(const std::vector<re_std::uint8_t>& _data);
    #endif

    #if D_ENV_SQLITE_HAS_LOAD_EXTENSION
    // load_extension
    //   function: loads a SQLite extension from a shared library.
    void load_extension(const std::string& _path);

    // enable_load_extension
    //   function: enables or disables extension loading.
    void enable_load_extension(bool _enabled);
    #endif
#endif  // D_ENV_SQLITE_DETECTED

private:
    // m_check -- nothing for SQLITE_OK; otherwise what the layer raises
    void m_check(int          _result,
                 failure_kind _kind,
                 const char*  _operation) const;

    sqlite_connect_config m_sqlite_config;
    face::sqlite          m_db;
};


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former sqlite_traits.hpp / sqlite_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// =============================================================================
// IX.  EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  file-based connection
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// B.  journal and WAL management
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// C.  PRAGMA interface
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// D.  backup API
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// E.  busy handler and timeout
// -------------------------------------------------------------------------

// sqlite_set_busy_timeout_t
//   detector: set_busy_timeout(int) method.
template<typename T>
using sqlite_set_busy_timeout_t =
    decltype(std::declval<T&>().set_busy_timeout(
        std::declval<int>()));

// -------------------------------------------------------------------------
// F.  database attachment
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// G.  schema introspection
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// H.  extension loading
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// I.  serialization
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// J.  transaction mode
// -------------------------------------------------------------------------

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


// =============================================================================
// X.   TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_sqlite_journal
//   trait: checks if type T supports journal mode management
// (set_journal_mode + get_journal_mode).
template<typename T>
struct has_sqlite_journal : djinterp::conjunction<
    is_detected<sqlite_set_journal_mode_t, clean_t<T>>,
    is_detected<sqlite_get_journal_mode_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_journal_v = has_sqlite_journal<clean_t<T>>::value;
#endif

// has_sqlite_pragma
//   trait: checks if type T supports the PRAGMA interface
// (set_pragma + get_pragma).
template<typename T>
struct has_sqlite_pragma : djinterp::conjunction<
    is_detected<sqlite_set_pragma_t, clean_t<T>>,
    is_detected<sqlite_get_pragma_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_pragma_v = has_sqlite_pragma<clean_t<T>>::value;
#endif

// has_sqlite_backup
//   trait: checks if type T supports the backup API
// (backup_to + backup_from).
template<typename T>
struct has_sqlite_backup : djinterp::conjunction<
    is_detected<sqlite_backup_to_t, clean_t<T>>,
    is_detected<sqlite_backup_from_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_backup_v = has_sqlite_backup<clean_t<T>>::value;
#endif

// has_sqlite_attach
//   trait: checks if type T supports database attachment
// (attach + detach).
template<typename T>
struct has_sqlite_attach : djinterp::conjunction<
    is_detected<sqlite_attach_t, clean_t<T>>,
    is_detected<sqlite_detach_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_attach_v = has_sqlite_attach<clean_t<T>>::value;
#endif

// has_sqlite_schema_query
//   trait: checks if type T supports schema introspection
// (has_table + list_tables).
template<typename T>
struct has_sqlite_schema_query : djinterp::conjunction<
    is_detected<sqlite_has_table_t, clean_t<T>>,
    is_detected<sqlite_list_tables_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_schema_query_v =
        has_sqlite_schema_query<clean_t<T>>::value;
#endif

// has_sqlite_extension_loading
//   trait: checks if type T supports extension loading
// (load_extension + enable_load_extension).
template<typename T>
struct has_sqlite_extension_loading : djinterp::conjunction<
    is_detected<sqlite_load_extension_t, clean_t<T>>,
    is_detected<sqlite_enable_load_extension_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_extension_loading_v =
        has_sqlite_extension_loading<clean_t<T>>::value;
#endif

// has_sqlite_serialization
//   trait: checks if type T supports serialization
// (serialize + deserialize).
template<typename T>
struct has_sqlite_serialization : djinterp::conjunction<
    is_detected<sqlite_serialize_t, clean_t<T>>,
    is_detected<sqlite_deserialize_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_serialization_v =
        has_sqlite_serialization<clean_t<T>>::value;
#endif

// has_sqlite_transaction_modes
//   trait: checks if type T supports SQLite transaction modes
// (deferred + immediate + exclusive).
template<typename T>
struct has_sqlite_transaction_modes : djinterp::conjunction<
    is_detected<sqlite_begin_deferred_t, clean_t<T>>,
    is_detected<sqlite_begin_immediate_t, clean_t<T>>,
    is_detected<sqlite_begin_exclusive_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_sqlite_transaction_modes_v =
        has_sqlite_transaction_modes<clean_t<T>>::value;
#endif

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
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool is_sqlite_connection_v =
        is_sqlite_connection<clean_t<T>>::value;
#endif


// =============================================================================
// XI.  TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

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

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

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


// =============================================================================
// XII. SFINAE HELPERS
// =============================================================================

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


// ===========================================================================
// XIII. C++20 CONCEPTS
// ===========================================================================
//   The SQLite classification concepts, folded in from the former
// sqlite_concepts.hpp.  Each forwards to a trait / tagless capability declared
// above.  Gated on concept support so the traits remain usable at the C++17
// baseline (matching functor.hpp / monoid.hpp).

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =============================================================================
// A.   Core SQLite Connection Concepts
// =============================================================================

// Sqlite_connection
//   concept: constrains types implementing the SQLite connection interface.
template<typename Type>
concept Sqlite_connection =
    is_sqlite_connection<clean_t<Type>>::value;

// non_sqlite_connection
//   concept: constrains types that do not implement the SQLite connection
// interface.
template<typename Type>
concept non_sqlite_connection =
    !Sqlite_connection<Type>;

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


// =============================================================================
// B.  SQLite Capability Concepts
// =============================================================================

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


// =============================================================================
// C. SQLite Transaction Mode Concepts
// =============================================================================

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


// =============================================================================
// D.  Tagless SQLite Capability Concepts
// =============================================================================

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


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_SQLITE_SQLITE_HPP
