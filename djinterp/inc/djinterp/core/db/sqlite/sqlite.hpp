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
* the concrete _helper method definitions in sqlite.cpp include it.
*
* 
*   DETECTION:
*   Also carries this database's capability-detection traits and C++20 concepts
* (trailing sections), folded in from sqlite_traits.hpp and the matching *_concepts.hpp;
* detection now lives with the connection. Concepts gated on concept support.
*
* path:      /inc/djinterp/core/db/sqlite/sqlite.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                 created: date: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_SQLITE_
#define DJINTERP_DATABASE_SQLITE_

// djinterp
#include "../../../djinterp.hpp"
#include "../../../env/db/sqlite/env_sqlite.h"
#include "../database_connection.hpp"
#include "../database_traits.hpp"


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
// CRTP leaf class that provides the _helper methods required by the
// base class chain. The _helper method bodies would be defined in the
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
        self().set_journal_mode_helper(_mode);
    }

    // get_journal_mode
    //   function: returns the current journal mode.
    std::string get_journal_mode() const
    {
        return self().get_journal_mode_helper();
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
        self().checkpoint_helper(_mode);
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
        self().execute_pragma_helper(_pragma, _value);
    }

    // get_pragma
    //   function: queries a PRAGMA and returns its current value.
    std::string get_pragma(const std::string& _pragma) const
    {
        return self().get_pragma_helper(_pragma);
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
        self().set_busy_timeout_helper(_timeout_ms);
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
        self().attach_helper(_file_path, _alias);
    }

    // detach
    //   function: detaches a previously attached database.
    // executes DETACH DATABASE <alias>.
    void detach(const std::string& _alias)
    {
        this->ensure_connected();
        self().detach_helper(_alias);
    }

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // table_exists
    //   function: tests whether a table exists in the database.
    bool table_exists(const std::string& _table_name) const
    {
        return self().table_exists_helper(_table_name);
    }

    // get_table_names
    //   function: returns a vector of all table names in the database.
    std::vector<std::string> get_table_names() const
    {
        return self().get_table_names_helper();
    }

    // -----------------------------------------------------------------
    // transaction modes
    // -----------------------------------------------------------------

    // begin_deferred
    //   function: begins a DEFERRED transaction.
    void begin_deferred()
    {
        this->ensure_connected();
        self().begin_deferred_helper();
        this->m_in_transaction = true;
    }

    // begin_immediate
    //   function: begins an IMMEDIATE transaction.
    void begin_immediate()
    {
        this->ensure_connected();
        self().begin_immediate_helper();
        this->m_in_transaction = true;
    }

    // begin_exclusive
    //   function: begins an EXCLUSIVE transaction.
    void begin_exclusive()
    {
        this->ensure_connected();
        self().begin_exclusive_helper();
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
    // _helper methods (defined in sqlite.cpp)
    // -----------------------------------------------------------------

    void        connect_helper();
    void        disconnect_helper();
    bool        is_connected_helper() const;
    bool        ping_helper() const;

    auto        execute_query_helper(const std::string& _query)
                    -> std::unique_ptr<
                        result_set<struct sqlite_result_set_helper>>;
    std::int64_t execute_update_helper(const std::string& _query);
    bool        execute_helper(const std::string& _query);

    auto        prepare_helper(const std::string& _query)
                    -> std::unique_ptr<
                        statement<struct sqlite_statement_helper>>;

    std::string  get_server_version_helper() const;
    std::string  get_last_error_helper() const;
    int          get_last_error_code_helper() const;
    std::int64_t get_last_insert_id_helper() const;
    std::int64_t get_affected_rows_helper() const;

    // SQLite-specific _helper methods
    void        set_journal_mode_helper(const std::string& _mode);
    std::string get_journal_mode_helper() const;
    void        checkpoint_helper(int _mode);
    void        execute_pragma_helper(const std::string& _pragma,
                                    const std::string& _value);
    std::string get_pragma_helper(const std::string& _pragma) const;
    void        set_busy_timeout_helper(int _timeout_ms);
    void        attach_helper(const std::string& _file_path,
                            const std::string& _alias);
    void        detach_helper(const std::string& _alias);
    bool        table_exists_helper(const std::string& _name) const;
    std::vector<std::string> get_table_names_helper() const;
    void        begin_deferred_helper();
    void        begin_immediate_helper();
    void        begin_exclusive_helper();

    // transaction _helper methods
    void begin_transaction_helper();
    void commit_helper();
    void rollback_helper();

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

// sqlite_result_set_helper
//   struct: forward declaration of the SQLite result set
// implementation.
struct sqlite_result_set_helper;

// sqlite_statement_helper
//   struct: forward declaration of the SQLite prepared statement
// implementation.
struct sqlite_statement_helper;


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former sqlite_traits.hpp / sqlite_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// =============================================================================
// X.   EXPRESSION DETECTORS
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
// XI.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_sqlite_journal
//   trait: checks if type _T supports journal mode management
// (set_journal_mode + get_journal_mode).
template<typename _T>
struct has_sqlite_journal : djinterp::conjunction<
    is_detected<sqlite_set_journal_mode_t, clean_t<_T>>,
    is_detected<sqlite_get_journal_mode_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_journal_v = has_sqlite_journal<clean_t<_T>>::value;
#endif

// has_sqlite_pragma
//   trait: checks if type _T supports the PRAGMA interface
// (execute_pragma + get_pragma).
template<typename _T>
struct has_sqlite_pragma : djinterp::conjunction<
    is_detected<sqlite_execute_pragma_t, clean_t<_T>>,
    is_detected<sqlite_get_pragma_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_pragma_v = has_sqlite_pragma<clean_t<_T>>::value;
#endif

// has_sqlite_backup
//   trait: checks if type _T supports the backup API
// (backup_to + backup_from).
template<typename _T>
struct has_sqlite_backup : djinterp::conjunction<
    is_detected<sqlite_backup_to_t, clean_t<_T>>,
    is_detected<sqlite_backup_from_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_backup_v = has_sqlite_backup<clean_t<_T>>::value;
#endif

// has_sqlite_attach
//   trait: checks if type _T supports database attachment
// (attach + detach).
template<typename _T>
struct has_sqlite_attach : djinterp::conjunction<
    is_detected<sqlite_attach_t, clean_t<_T>>,
    is_detected<sqlite_detach_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_attach_v = has_sqlite_attach<clean_t<_T>>::value;
#endif

// has_sqlite_schema_query
//   trait: checks if type _T supports schema introspection
// (table_exists + get_table_names).
template<typename _T>
struct has_sqlite_schema_query : djinterp::conjunction<
    is_detected<sqlite_table_exists_t, clean_t<_T>>,
    is_detected<sqlite_get_table_names_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_schema_query_v =
        has_sqlite_schema_query<clean_t<_T>>::value;
#endif

// has_sqlite_extension_loading
//   trait: checks if type _T supports extension loading
// (load_extension + enable_load_extension).
template<typename _T>
struct has_sqlite_extension_loading : djinterp::conjunction<
    is_detected<sqlite_load_extension_t, clean_t<_T>>,
    is_detected<sqlite_enable_load_extension_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_extension_loading_v =
        has_sqlite_extension_loading<clean_t<_T>>::value;
#endif

// has_sqlite_serialization
//   trait: checks if type _T supports serialization
// (serialize + deserialize).
template<typename _T>
struct has_sqlite_serialization : djinterp::conjunction<
    is_detected<sqlite_serialize_t, clean_t<_T>>,
    is_detected<sqlite_deserialize_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_serialization_v =
        has_sqlite_serialization<clean_t<_T>>::value;
#endif

// has_sqlite_transaction_modes
//   trait: checks if type _T supports SQLite transaction modes
// (deferred + immediate + exclusive).
template<typename _T>
struct has_sqlite_transaction_modes : djinterp::conjunction<
    is_detected<sqlite_begin_deferred_t, clean_t<_T>>,
    is_detected<sqlite_begin_immediate_t, clean_t<_T>>,
    is_detected<sqlite_begin_exclusive_t, clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool has_sqlite_transaction_modes_v =
        has_sqlite_transaction_modes<clean_t<_T>>::value;
#endif

// is_sqlite_connection
//   trait: compound trait verifying type _T implements a SQLite
// connection interface (connection + journal + pragma + schema
// queries + transaction modes).
template<typename _T>
struct is_sqlite_connection : djinterp::conjunction<
    has_connect<clean_t<_T>>,
    has_disconnect<clean_t<_T>>,
    has_execute_query<clean_t<_T>>,
    has_sqlite_journal<clean_t<_T>>,
    has_sqlite_pragma<clean_t<_T>>,
    has_sqlite_schema_query<clean_t<_T>>,
    has_sqlite_transaction_modes<clean_t<_T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    constexpr bool is_sqlite_connection_v =
        is_sqlite_connection<clean_t<_T>>::value;
#endif


// =============================================================================
// XII. TAGLESS CAPABILITY TRAITS (constexpr bool)
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
    ( sqlite_can_set_journal_mode<clean_t<_T>> &&
      sqlite_can_checkpoint<clean_t<_T>> );

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
    ( can_connect<clean_t<_T>>               &&
      can_disconnect<clean_t<_T>>            &&
      can_execute_query<clean_t<_T>>         &&
      sqlite_does_journal<clean_t<_T>>       &&
      sqlite_does_pragma<clean_t<_T>>        &&
      sqlite_can_query_schema<clean_t<_T>> );


// =============================================================================
// XIII.  SFINAE HELPERS
// =============================================================================

// enable_if_sqlite_connection
//   type: SFINAE helper for SQLite connection constraints.
template<typename _T>
using enable_if_sqlite_connection =
    typename std::enable_if<is_sqlite_connection<clean_t<_T>>::value>::type;

// enable_if_has_sqlite_journal
//   type: SFINAE helper for SQLite journal constraints.
template<typename _T>
using enable_if_has_sqlite_journal =
    typename std::enable_if<has_sqlite_journal<clean_t<_T>>::value>::type;

// enable_if_has_sqlite_pragma
//   type: SFINAE helper for SQLite PRAGMA constraints.
template<typename _T>
using enable_if_has_sqlite_pragma =
    typename std::enable_if<has_sqlite_pragma<clean_t<_T>>::value>::type;

// enable_if_has_sqlite_backup
//   type: SFINAE helper for SQLite backup constraints.
template<typename _T>
using enable_if_has_sqlite_backup =
    typename std::enable_if<has_sqlite_backup<clean_t<_T>>::value>::type;


// ===========================================================================
// XIV.   C++20 CONCEPTS
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
template<typename _Type>
concept Sqlite_connection =
    is_sqlite_connection<clean_t<_Type>>::value;

// non_sqlite_connection
//   concept: constrains types that do not implement the SQLite connection
// interface.
template<typename _Type>
concept non_sqlite_connection =
    !Sqlite_connection<_Type>;

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
// B.  SQLite Capability Concepts
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
// C. SQLite Transaction Mode Concepts
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
// D.  Tagless SQLite Capability Concepts
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


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_SQLITE_