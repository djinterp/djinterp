/******************************************************************************
* djinterp [database]                                               sqlite.hpp
* 
* djinterp SQLite connection module:
*   This header provides the SQLite-specific connection implementation
* and associated data type infrastructure for the djinterp database
* module, including:
*   - SQLite native type affinity enumeration (INTEGER, REAL, TEXT, BLOB,
*     NULL) and mapping to djinterp field_type
*   - SQLite declared type parsing (VARCHAR(N), BOOLEAN, etc. to affinity)
*   - compile-time type and feature availability using D_ENV_SQLITE_*
*     macros from env_sqlite.h, covering version-gated SQL features,
*     SQLITE_ENABLE_* compile options, and SQLITE_OMIT_* feature removal
*   - sqlite_open_flag scoped enum mirroring SQLITE_OPEN_* constants
*   - sqlite_journal_mode enumeration
*   - sqlite_transaction_mode enumeration (DEFERRED, IMMEDIATE, EXCLUSIVE)
*   - SQLite-specific connection configuration (file path, open flags,
*     journal mode, busy timeout, cache size, page size, foreign keys)
*   - the concrete sqlite_connection CRTP leaf class with WAL management,
*     PRAGMA interface, backup API, database attachment, extension loading,
*     and serialization support
*   - version-gated method declarations using D_ENV_SQLITE_* macros
*
*   SQLite is fundamentally different from client/server databases:
*   - embedded: no network, no authentication, no replication
*   - file-based: the "connection" opens a file (or :memory:)
*   - type affinity: columns have affinity, not rigid types (unless
*     STRICT tables are used, 3.37+)
*   - features determined by compile-time options AND version, not by
*     server configuration
*   - threading model is a compile-time constant (SQLITE_THREADSAFE)
*
*   LAYER DIAGRAM:
*     sqlite_connection (this file)
*       -> database_connection<sqlite_connection, database_type::sqlite>
*         -> connection_template<sqlite_connection, database_type::sqlite>
*           -> connection<sqlite_connection>
*
*   Note: sqlite_connection does NOT derive from mysql_common_connection;
* SQLite has no MySQL-family heritage.
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include <sqlite3.h>;
* the concrete _impl method definitions in sqlite.cpp include it.
*
* 
* path:      /inc/djinterp/core/db/sqlite/sqlite.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                 created: date: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_SQLITE_
#define DJINTERP_DATABASE_SQLITE_

#include "database_connection.hpp"
#include "sqlite_traits.hpp"

#include "../env/db/env_sqlite.h"


NS_DJINTERP
NS_DATABASE
NS_SQLITE


// =============================================================================
// I.   SQLITE TYPE AFFINITY
// =============================================================================
// SQLite uses a dynamic type system based on five storage classes.
// Column declarations map to one of five affinities via the affinity
// determination rules in the SQLite documentation. STRICT tables
// (3.37+) enforce exact type matching instead of affinity.

// sqlite_affinity
//   enumeration: the five SQLite storage classes / type affinities.
enum class sqlite_affinity : std::uint8_t
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
enum class sqlite_declared_type : std::uint8_t
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
enum class sqlite_journal_mode : std::uint8_t
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
enum class sqlite_transaction_mode : std::uint8_t
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
    static constexpr std::uint32_t id       = D_ENV_SQLITE_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_SQLITE_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_SQLITE_VERSION_MINOR;
    static constexpr std::uint16_t patch    = D_ENV_SQLITE_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_SQLITE_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected SQLite version is at
    // least (major, minor, patch).
    static constexpr bool at_least(std::uint16_t _major,
                                   std::uint16_t _minor,
                                   std::uint16_t _patch) noexcept
    {
        return id >= (_major * 1000000u + _minor * 1000u + _patch);
    }
};


// =============================================================================
// VII. SQLITE CONNECTION CONFIGURATION
// =============================================================================

// sqlite_connect_config
//   struct: SQLite-specific connection configuration.
// Because SQLite is file-based, the primary "address" is a file path
// rather than host:port. Special paths include ":memory:" for in-memory
// databases and "" for temporary on-disk databases.
struct sqlite_connect_config
{
    std::string              file_path;
    sqlite_open_flag         open_flags;
    sqlite_journal_mode      journal_mode;
    sqlite_transaction_mode  default_transaction_mode;
    int                      busy_timeout_ms;
    int                      cache_size;
    int                      page_size;
    bool                     enable_foreign_keys;
    bool                     enable_wal;
    bool                     enable_shared_cache;
    std::string              vfs_name;

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
    {
    }

    explicit sqlite_connect_config(
		const std::string& _pat
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
    {
    }

    // in_memory
    //   function: factory for an in-memory database configuration.
    static sqlite_connect_config in_memory()
    {
        sqlite_connect_config config;

        config.file_path  = ":memory:";
        config.open_flags = sqlite_open_flag::readwrite  |
                            sqlite_open_flag::create     |
                            sqlite_open_flag::memory;

        return config;
    }

    // temporary
    //   function: factory for a temporary on-disk database.
    static sqlite_connect_config temporary()
    {
        sqlite_connect_config config;

        config.file_path  = "";
        config.open_flags = sqlite_open_flag::readwrite
                          | sqlite_open_flag::create;

        return config;
    }

    // with_wal
    //   function: factory for a WAL-mode database.
    static sqlite_connect_config with_wal(
	const std::string& _path
	)
    {
        sqlite_connect_config config(_path);

        config.enable_wal    = true;
        config.journal_mode  = sqlite_journal_mode::mode_wal;

        return config;
    }
};


// =============================================================================
// VIII. SQLITE CONNECTION
// =============================================================================

// sqlite_connection
//   class: concrete SQLite connection implementation. This is the
// CRTP leaf class that provides the _impl methods required by the
// base class chain. The _impl method bodies would be defined in the
// corresponding sqlite.cpp source file that includes <sqlite3.h>.
//
// Usage:
//   sqlite_connection conn;
//   conn.connect(sqlite_connect_config::with_wal("my.db"));
//   auto rs = conn.execute_query("SELECT * FROM users");
class sqlite_connection
    : public database_connection<sqlite_connection,
                                 database_type::sqlite>
{
public:
    using base_type      = database_connection<
        sqlite_connection, database_type::sqlite>;
    using type_support   = sqlite_type_support;
    using version_info   = sqlite_version_info;

    sqlite_connection()
        : base_type()
    {}

    explicit sqlite_connection(const connection_config& _config)
        : base_type(_config)
    {}

    explicit sqlite_connection(const sqlite_connect_config& _config)
        : base_type(),
          m_sqlite_config(_config)
    {
        // map file_path into the generic config's database field
        this->m_config.database = _config.file_path;
    }

    explicit sqlite_connection(const std::string& _file_path)
        : base_type()
        , m_sqlite_config(_file_path)
    {
        this->m_config.database = _file_path;
    }

    ~sqlite_connection() = default;

    // disable copying
    sqlite_connection(const sqlite_connection&)            = delete;
    sqlite_connection& operator=(const sqlite_connection&) = delete;

    // enable moving
    sqlite_connection(sqlite_connection&&) noexcept            = default;
    sqlite_connection& operator=(sqlite_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // file-based connection
    // -----------------------------------------------------------------

    // open
    //   function: opens a database file. If a sqlite_connect_config
    // was provided, uses its file_path and open_flags. Otherwise
    // uses the generic config's database field.
    void open(
		const std::string& _file_path
	)
    {
        m_sqlite_config.file_path = _file_path;
        this->m_config.database   = _file_path;

        connect();
    }

    // open_v2
    //   function: opens a database file with explicit flags.
    void open_v2(
		const std::string& _file_path,
		int                _flags
	)
    {
        m_sqlite_config.file_path  = _file_path;
        m_sqlite_config.open_flags = static_cast<sqlite_open_flag>(_flags);
        this->m_config.database    = _file_path;

        connect();
    }

    // -----------------------------------------------------------------
    // journal and WAL management
    // -----------------------------------------------------------------

    // set_journal_mode
    //   function: sets the journal mode via PRAGMA journal_mode=X.
    void set_journal_mode(
		const std::string& _mode
	)
    {
        this->ensure_connected();
        self().set_journal_mode_impl(_mode);
    }

    // get_journal_mode
    //   function: returns the current journal mode.
    std::string get_journal_mode() const
    {
        return self().get_journal_mode_impl();
    }

    // checkpoint
    //   function: performs a WAL checkpoint.
    // wraps sqlite3_wal_checkpoint_v2(). The _mode parameter
    // corresponds to SQLITE_CHECKPOINT_PASSIVE (0),
    // SQLITE_CHECKPOINT_FULL (1), SQLITE_CHECKPOINT_RESTART (2),
    // SQLITE_CHECKPOINT_TRUNCATE (3).
    void checkpoint(
		int _mode = 0
	)
    {
        this->ensure_connected();
        self().checkpoint_impl(_mode);
    }

    // enable_wal
    //   function: convenience method to switch to WAL mode.
    void enable_wal()
    {
        set_journal_mode("WAL");
        m_sqlite_config.journal_mode = sqlite_journal_mode::mode_wal;
    }

    // -----------------------------------------------------------------
    // PRAGMA interface
    // -----------------------------------------------------------------

    // execute_pragma
    //   function: executes a PRAGMA with a value (e.g. "cache_size",
    // "-2000").
    void execute_pragma(const std::string& _pragma,
                        const std::string& _value)
    {
        this->ensure_connected();
        self().execute_pragma_impl(_pragma, _value);
    }

    // get_pragma
    //   function: queries a PRAGMA and returns its current value.
    std::string get_pragma(const std::string& _pragma) const
    {
        return self().get_pragma_impl(_pragma);
    }

    // set_foreign_keys
    //   function: enables or disables foreign key enforcement.
    void set_foreign_keys(bool _enabled)
    {
        execute_pragma("foreign_keys", _enabled ? "ON" : "OFF");
    }

    // -----------------------------------------------------------------
    // busy handler
    // -----------------------------------------------------------------

    // set_busy_timeout
    //   function: sets the busy timeout in milliseconds.
    // wraps sqlite3_busy_timeout().
    void set_busy_timeout(int _timeout_ms)
    {
        this->ensure_connected();
        m_sqlite_config.busy_timeout_ms = _timeout_ms;
        self().set_busy_timeout_impl(_timeout_ms);
    }

    // -----------------------------------------------------------------
    // database attachment
    // -----------------------------------------------------------------

    // attach
    //   function: attaches another database file under the given alias.
    // executes ATTACH DATABASE '<path>' AS <alias>.
    void attach(const std::string& _file_path,
                const std::string& _alias)
    {
        this->ensure_connected();
        self().attach_impl(_file_path, _alias);
    }

    // detach
    //   function: detaches a previously attached database.
    // executes DETACH DATABASE <alias>.
    void detach(const std::string& _alias)
    {
        this->ensure_connected();
        self().detach_impl(_alias);
    }

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // table_exists
    //   function: tests whether a table exists in the database.
    bool table_exists(const std::string& _table_name) const
    {
        return self().table_exists_impl(_table_name);
    }

    // get_table_names
    //   function: returns a vector of all table names in the database.
    std::vector<std::string> get_table_names() const
    {
        return self().get_table_names_impl();
    }

    // -----------------------------------------------------------------
    // transaction modes
    // -----------------------------------------------------------------

    // begin_deferred
    //   function: begins a DEFERRED transaction.
    void begin_deferred()
    {
        this->ensure_connected();
        self().begin_deferred_impl();
        this->m_in_transaction = true;
    }

    // begin_immediate
    //   function: begins an IMMEDIATE transaction.
    void begin_immediate()
    {
        this->ensure_connected();
        self().begin_immediate_impl();
        this->m_in_transaction = true;
    }

    // begin_exclusive
    //   function: begins an EXCLUSIVE transaction.
    void begin_exclusive()
    {
        this->ensure_connected();
        self().begin_exclusive_impl();
        this->m_in_transaction = true;
    }

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

    const sqlite_connect_config&
    get_sqlite_config() const noexcept
    {
        return m_sqlite_config;
    }

    void set_sqlite_config(const sqlite_connect_config& _config)
    {
        m_sqlite_config         = _config;
        this->m_config.database = _config.file_path;
    }

    // is_in_memory
    //   function: returns true if this connection targets an in-memory
    // database.
    bool is_in_memory() const noexcept
    {
        return (m_sqlite_config.file_path == ":memory:") ||
               (m_sqlite_config.file_path.empty());
    }

    // -----------------------------------------------------------------
    // _impl methods (defined in sqlite.cpp)
    // -----------------------------------------------------------------

    void        connect_impl();
    void        disconnect_impl();
    bool        is_connected_impl() const;
    bool        ping_impl() const;

    auto        execute_query_impl(const std::string& _query)
                    -> std::unique_ptr<
                        result_set<struct sqlite_result_set_impl>>;
    std::int64_t execute_update_impl(const std::string& _query);
    bool        execute_impl(const std::string& _query);

    auto        prepare_impl(const std::string& _query)
                    -> std::unique_ptr<
                        statement<struct sqlite_statement_impl>>;

    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;
    std::int64_t get_last_insert_id_impl() const;
    std::int64_t get_affected_rows_impl() const;

    // SQLite-specific _impl methods
    void        set_journal_mode_impl(const std::string& _mode);
    std::string get_journal_mode_impl() const;
    void        checkpoint_impl(int _mode);
    void        execute_pragma_impl(const std::string& _pragma,
                                    const std::string& _value);
    std::string get_pragma_impl(const std::string& _pragma) const;
    void        set_busy_timeout_impl(int _timeout_ms);
    void        attach_impl(const std::string& _file_path,
                            const std::string& _alias);
    void        detach_impl(const std::string& _alias);
    bool        table_exists_impl(const std::string& _name) const;
    std::vector<std::string> get_table_names_impl() const;
    void        begin_deferred_impl();
    void        begin_immediate_impl();
    void        begin_exclusive_impl();

    // transaction _impl methods
    void begin_transaction_impl();
    void commit_impl();
    void rollback_impl();

    // version-gated methods

#if D_ENV_SQLITE_DETECTED
    #if D_ENV_SQLITE_HAS_BACKUP_API
    // backup_to
    //   function: backs up this database to the given file path.
    // Available since SQLite 3.6.11.
    void backup_to(const std::string& _dest_path);

    // backup_from
    //   function: restores this database from the given file path.
    void backup_from(const std::string& _source_path);
    #endif

    #if D_ENV_SQLITE_HAS_SERIALIZE
    // serialize
    //   function: serializes the database into a byte vector.
    // Available since SQLite 3.36.0.
    std::vector<std::uint8_t> serialize();

    // deserialize
    //   function: replaces the database contents from serialized bytes.
    void deserialize(const std::vector<std::uint8_t>& _data);
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
    sqlite_connect_config m_sqlite_config;

    sqlite_connection& self()
    {
        return *this;
    }

    const sqlite_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// IX.  FORWARD DECLARATIONS
// =============================================================================

// sqlite_result_set_impl
//   struct: forward declaration of the SQLite result set
// implementation.
struct sqlite_result_set_impl;

// sqlite_statement_impl
//   struct: forward declaration of the SQLite prepared statement
// implementation.
struct sqlite_statement_impl;


NS_END  // sqlite
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_SQLITE_
