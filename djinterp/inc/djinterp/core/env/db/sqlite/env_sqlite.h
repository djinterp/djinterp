/******************************************************************************
* djinterp [db]                                                   env_sqlite.h
*
* djinterp SQLite environmental detection header:
* This header provides comprehensive compile-time detection of SQLite
* environments, capabilities, and version-gated features, including:
*   - version decomposition (major, minor, patch) and comparison macros
*   - library presence and linkage detection
*   - threading model detection (single-thread, multi-thread, serialized)
*   - compile-time option detection (SQLITE_ENABLE_*, SQLITE_OMIT_*)
*   - full-text search engine detection (FTS3, FTS4, FTS5)
*   - JSON extension and built-in JSON function detection
*   - R*Tree and GeoJSON/Geopoly extension detection
*   - virtual table framework and built-in virtual table detection
*   - WAL mode and journal mode capabilities
*   - VFS (Virtual File System) layer detection
*   - memory management and allocator configuration
*   - SQL feature detection gated by version (CTEs, window functions,
*     UPSERT, RETURNING, generated columns, STRICT tables, etc.)
*   - security features (encryption, API armor, URI filenames)
*   - session and changeset extension detection
*   - backup, serialization, and snapshot API detection
*   - platform integration (OS-level VFS, large file support)
*   - limit and configuration constant detection
*   - diagnostic and debugging feature detection
*
*   SQLite is an embedded database engine compiled directly into the
* application. Unlike client/server databases, feature availability is
* determined entirely by compile-time options and the SQLite version
* number. There is no concept of client libraries, authentication,
* replication, or storage engine selection.
*
*   The header creates a unified D_ENV_SQLITE_* macro interface enabling
* portable SQLite code that adapts to different versions and compile-time
* configurations. All detection is performed at compile-time with zero
* runtime overhead.
*
*   CONFIGURATION SYSTEM:
*   This header supports custom SQLite environment simulation via
* D_CFG_ENV_SQLITE_CUSTOM:
*   - 0 (default): full automatic detection via sqlite3.h-provided macros
*   - 1: skip all detection (requires pre-defined D_ENV_SQLITE_DETECTED_*
*     variables)
*   Pre-defining D_ENV_SQLITE_DETECTED_* variables automatically enables
* custom mode.
*
*   NAMING CONVENTION:
*   D_ENV_SQLITE_[CATEGORY]_[FEATURE]  - 1 if available, 0 otherwise
*   D_ENV_SQLITE_VERSION_[COMPONENT]   - version number components
*   D_ENV_SQLITE_HAS_[CAPABILITY]      - capability flag (1/0)
*
*   VERSION ENCODING:
*   SQLite uses MAJOR*1000000 + MINOR*1000 + PATCH in
* SQLITE_VERSION_NUMBER.  E.g. SQLite 3.45.2 = 3045002.
* This differs from the MySQL-family MAJOR*10000 + MINOR*100 + PATCH
* encoding.
*
*   DEPENDENCIES:
*   This header includes env_db.h for base database environment detection
* capabilities. It should be included after sqlite3.h so that
* SQLITE_VERSION_NUMBER and compile-time option macros are available.
*
* 
* path:      /inc/djinterp/core/env/db/sqlite/env_sqlite.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_SQLITE_
#define DJINTERP_ENVIRONMENT_SQLITE_ 1

#include "../../../config/core/env/db/env_sqlite_config.h"
#include "./env_db.h"


// =============================================================================
// 0.   VENDOR HEADER INCLUSION
// =============================================================================
//   Driven by D_CFG_ENV_USING_SQLITE from env_config.h. When enabled, this
// section includes <sqlite3.h> (or the override configured via
// D_CFG_ENV_SQLITE_C_PATH). SQLite has no canonical C++ header in the
// official distribution, so there is no CPP path. Detection below is gated
// on D_ENV_SQLITE_HEADER_INCLUDED so that SQLITE_VERSION_NUMBER is never
// referenced unless the header is actually in scope.

#if (D_CFG_ENV_USING_SQLITE == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_SQLITE_C_PATH)
            #include D_CFG_ENV_SQLITE_C_PATH
            #define D_ENV_SQLITE_HEADER_INCLUDED 1
        #elif __has_include(<sqlite3.h>)
            #include <sqlite3.h>
            #define D_ENV_SQLITE_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_SQLITE=1 but no sqlite3.h header was "   \
                   "found. Install libsqlite3-dev (or equivalent), or "      \
                   "define D_CFG_ENV_SQLITE_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_SQLITE_C_PATH
        #define D_ENV_SQLITE_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_SQLITE_CLIENT_C
        #define D_ENV_DB_HAS_SQLITE_CLIENT_C 1
    #endif

#else
    #define D_ENV_SQLITE_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_SQLITE_CLIENT_C
        #define D_ENV_DB_HAS_SQLITE_CLIENT_C 0
    #endif
#endif  // D_CFG_ENV_USING_SQLITE


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================

//   All D_CFG_* macros for this module live in env_sqlite_config.h,
// pulled in at the top of this file.


// =============================================================================
// II.  VERSION ENCODING
// =============================================================================
//   SQLite uses MAJOR*1000000 + MINOR*1000 + PATCH in
// SQLITE_VERSION_NUMBER.  E.g. SQLite 3.45.2 = 3045002.

// D_ENV_SQLITE_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple into the SQLite
// version number format.
#define D_ENV_SQLITE_ENCODE_VERSION(major, minor, patch) \
    ((major) * 1000000 + (minor) * 1000 + (patch))

// D_ENV_SQLITE_DECODE_MAJOR
//   macro: extracts the major version from an encoded version number.
#define D_ENV_SQLITE_DECODE_MAJOR(ver) \
    ((ver) / 1000000)

// D_ENV_SQLITE_DECODE_MINOR
//   macro: extracts the minor version from an encoded version number.
#define D_ENV_SQLITE_DECODE_MINOR(ver) \
    (((ver) / 1000) % 1000)

// D_ENV_SQLITE_DECODE_PATCH
//   macro: extracts the patch version from an encoded version number.
#define D_ENV_SQLITE_DECODE_PATCH(ver) \
    ((ver) % 1000)


// =============================================================================
// III. VERSION DETECTION
// =============================================================================

// version ID constants for feature-significant releases
#define D_ENV_SQLITE_VERSION_3_6_11     3006011  // online backup API
#define D_ENV_SQLITE_VERSION_3_6_19     3006019  // WAL journal mode
#define D_ENV_SQLITE_VERSION_3_7_0      3007000  // WAL mode stable
#define D_ENV_SQLITE_VERSION_3_7_11     3007011  // multiple VALUES rows
#define D_ENV_SQLITE_VERSION_3_7_15     3007015  // RELEASE syntax
#define D_ENV_SQLITE_VERSION_3_7_17     3007017  // memory-mapped I/O
#define D_ENV_SQLITE_VERSION_3_8_0      3008000  // partial indexes
#define D_ENV_SQLITE_VERSION_3_8_2      3008002  // WITH (CTE)
#define D_ENV_SQLITE_VERSION_3_8_3      3008003  // WITH RECURSIVE
#define D_ENV_SQLITE_VERSION_3_8_7      3008007  // VALUES clause as vtab
#define D_ENV_SQLITE_VERSION_3_8_10     3008010  // LIKE optimization
#define D_ENV_SQLITE_VERSION_3_8_12     3008012  // FTS5
#define D_ENV_SQLITE_VERSION_3_9_0      3009000  // JSON1 extension
#define D_ENV_SQLITE_VERSION_3_10_0     3010000  // LIKE with ESCAPE
#define D_ENV_SQLITE_VERSION_3_14_0     3014000  // row values partial
#define D_ENV_SQLITE_VERSION_3_15_0     3015000  // row values
#define D_ENV_SQLITE_VERSION_3_18_0     3018000  // INSERT into WITH
#define D_ENV_SQLITE_VERSION_3_20_0     3020000  // pointer passing iface
#define D_ENV_SQLITE_VERSION_3_22_0     3022000  // UPSERT (partial)
#define D_ENV_SQLITE_VERSION_3_24_0     3024000  // UPSERT full
#define D_ENV_SQLITE_VERSION_3_25_0     3025000  // window functions
#define D_ENV_SQLITE_VERSION_3_26_0     3026000  // UPSERT enhancements
#define D_ENV_SQLITE_VERSION_3_28_0     3028000  // RENAME COLUMN
#define D_ENV_SQLITE_VERSION_3_30_0     3030000  // FILTER clause
#define D_ENV_SQLITE_VERSION_3_31_0     3031000  // generated columns
#define D_ENV_SQLITE_VERSION_3_33_0     3033000  // UPDATE FROM
#define D_ENV_SQLITE_VERSION_3_34_0     3034000  // multiple recursive CTE
#define D_ENV_SQLITE_VERSION_3_35_0     3035000  // DROP COLUMN, math fns
#define D_ENV_SQLITE_VERSION_3_36_0     3036000  // RETURNING clause
#define D_ENV_SQLITE_VERSION_3_37_0     3037000  // STRICT tables
#define D_ENV_SQLITE_VERSION_3_38_0     3038000  // -> ->> JSON operators
#define D_ENV_SQLITE_VERSION_3_39_0     3039000  // multi ON CONFLICT
#define D_ENV_SQLITE_VERSION_3_40_0     3040000  // JSONB
#define D_ENV_SQLITE_VERSION_3_41_0     3041000
#define D_ENV_SQLITE_VERSION_3_43_0     3043000  // JSON5 support
#define D_ENV_SQLITE_VERSION_3_44_0     3044000
#define D_ENV_SQLITE_VERSION_3_45_0     3045000
#define D_ENV_SQLITE_VERSION_3_46_0     3046000

#if (D_CFG_ENV_SQLITE_CUSTOM == 0)

    // automatic detection requires sqlite3.h to be in scope; if
    // D_CFG_ENV_USING_SQLITE was not enabled the sentinel is 0 and we skip
    // cleanly (no reference to SQLITE_VERSION_NUMBER).
    #if ( D_ENV_SQLITE_HEADER_INCLUDED  &&  \
          defined(SQLITE_VERSION_NUMBER) )
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        SQLITE_VERSION_NUMBER
        #define D_ENV_SQLITE_VERSION_MAJOR     \
            D_ENV_SQLITE_DECODE_MAJOR(SQLITE_VERSION_NUMBER)
        #define D_ENV_SQLITE_VERSION_MINOR     \
            D_ENV_SQLITE_DECODE_MINOR(SQLITE_VERSION_NUMBER)
        #define D_ENV_SQLITE_VERSION_PATCH     \
            D_ENV_SQLITE_DECODE_PATCH(SQLITE_VERSION_NUMBER)

        #ifdef SQLITE_VERSION
            #define D_ENV_SQLITE_VERSION_STRING SQLITE_VERSION
        #else
            #define D_ENV_SQLITE_VERSION_STRING "unknown"
        #endif

        #ifdef SQLITE_SOURCE_ID
            #define D_ENV_SQLITE_SOURCE_ID     SQLITE_SOURCE_ID
        #endif
    #else
        #define D_ENV_SQLITE_DETECTED          0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_SQLITE_DETECTED_VERSION
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_DETECTED_VERSION
        #define D_ENV_SQLITE_VERSION_MAJOR     \
            D_ENV_SQLITE_DECODE_MAJOR(D_ENV_SQLITE_DETECTED_VERSION)
        #define D_ENV_SQLITE_VERSION_MINOR     \
            D_ENV_SQLITE_DECODE_MINOR(D_ENV_SQLITE_DETECTED_VERSION)
        #define D_ENV_SQLITE_VERSION_PATCH     \
            D_ENV_SQLITE_DECODE_PATCH(D_ENV_SQLITE_DETECTED_VERSION)
        #define D_ENV_SQLITE_VERSION_STRING    "manual"

    #elif defined(D_ENV_SQLITE_DETECTED_3_46)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_46_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     46
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.46.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_45)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_45_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     45
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.45.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_43)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_43_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     43
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.43.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_40)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_40_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     40
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.40.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_39)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_39_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     39
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.39.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_38)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_38_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     38
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.38.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_37)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_37_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     37
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.37.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_36)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_36_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     36
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.36.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_35)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_35_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     35
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.35.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_31)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_31_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     31
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.31.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_25)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_25_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     25
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.25.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_24)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_24_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     24
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.24.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_9)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_9_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     9
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.9.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_8)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_8_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     8
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.8.0"

    #elif defined(D_ENV_SQLITE_DETECTED_3_7)
        #define D_ENV_SQLITE_DETECTED          1
        #define D_ENV_SQLITE_VERSION_ID        D_ENV_SQLITE_VERSION_3_7_0
        #define D_ENV_SQLITE_VERSION_MAJOR     3
        #define D_ENV_SQLITE_VERSION_MINOR     7
        #define D_ENV_SQLITE_VERSION_PATCH     0
        #define D_ENV_SQLITE_VERSION_STRING    "3.7.0"

    #else
        #define D_ENV_SQLITE_DETECTED          0
    #endif

#endif  // D_CFG_ENV_SQLITE_CUSTOM


// =============================================================================
// IV.  VERSION COMPARISON MACROS
// =============================================================================

#if D_ENV_SQLITE_DETECTED

    // D_ENV_SQLITE_VERSION_AT_LEAST
    //   macro: evaluates to 1 if SQLite version >= specified version.
    #define D_ENV_SQLITE_VERSION_AT_LEAST(major, minor, patch) \
        (D_ENV_SQLITE_VERSION_ID >= \
            D_ENV_SQLITE_ENCODE_VERSION(major, minor, patch))

    // D_ENV_SQLITE_VERSION_BELOW
    //   macro: evaluates to 1 if SQLite version < specified version.
    #define D_ENV_SQLITE_VERSION_BELOW(major, minor, patch) \
        (D_ENV_SQLITE_VERSION_ID < \
            D_ENV_SQLITE_ENCODE_VERSION(major, minor, patch))

    // D_ENV_SQLITE_VERSION_EXACT
    //   macro: evaluates to 1 if SQLite version matches exactly.
    #define D_ENV_SQLITE_VERSION_EXACT(major, minor, patch) \
        (D_ENV_SQLITE_VERSION_ID == \
            D_ENV_SQLITE_ENCODE_VERSION(major, minor, patch))

    // D_ENV_SQLITE_VERSION_IN_RANGE
    //   macro: evaluates to 1 if SQLite version is within [min, max).
    #define D_ENV_SQLITE_VERSION_IN_RANGE(min_maj, min_min, min_pat,     \
                                          max_maj, max_min, max_pat)      \
        ( D_ENV_SQLITE_VERSION_AT_LEAST(min_maj, min_min, min_pat) &&    \
          D_ENV_SQLITE_VERSION_BELOW(max_maj, max_min, max_pat) )


// =============================================================================
// V.   THREADING MODEL DETECTION
// =============================================================================
//   SQLite threading is determined at compile time by SQLITE_THREADSAFE:
//     0 = single-thread (all mutexes disabled)
//     1 = serialized (default; safe for multi-threaded with global mutex)
//     2 = multi-thread (safe if no connection shared across threads)

    // D_ENV_SQLITE_THREADSAFE
    //   feature: the raw SQLITE_THREADSAFE value.
    #ifdef SQLITE_THREADSAFE
        #define D_ENV_SQLITE_THREADSAFE SQLITE_THREADSAFE
    #else
        // default if not specified is serialized (1)
        #define D_ENV_SQLITE_THREADSAFE 1
    #endif

    // D_ENV_SQLITE_IS_SINGLE_THREAD
    //   status: 1 if compiled in single-thread mode (no mutexes).
    #define D_ENV_SQLITE_IS_SINGLE_THREAD \
        (D_ENV_SQLITE_THREADSAFE == 0)

    // D_ENV_SQLITE_IS_MULTI_THREAD
    //   status: 1 if compiled in multi-thread mode (per-connection
    // safety, no shared connections across threads).
    #define D_ENV_SQLITE_IS_MULTI_THREAD \
        (D_ENV_SQLITE_THREADSAFE == 2)

    // D_ENV_SQLITE_IS_SERIALIZED
    //   status: 1 if compiled in serialized mode (fully thread-safe
    // with global mutex; default).
    #define D_ENV_SQLITE_IS_SERIALIZED \
        (D_ENV_SQLITE_THREADSAFE == 1)

    // D_ENV_SQLITE_HAS_THREAD_SAFETY
    //   feature: 1 if any thread safety is enabled (multi-thread or
    // serialized).
    #define D_ENV_SQLITE_HAS_THREAD_SAFETY \
        (D_ENV_SQLITE_THREADSAFE > 0)

    // D_ENV_SQLITE_HAS_MUTEX
    //   feature: 1 if the mutex subsystem is available (THREADSAFE > 0).
    #define D_ENV_SQLITE_HAS_MUTEX \
        (D_ENV_SQLITE_THREADSAFE > 0)


// =============================================================================
// VI.  FULL-TEXT SEARCH (FTS) EXTENSIONS
// =============================================================================

    // D_ENV_SQLITE_HAS_FTS3
    //   feature: detect if FTS3 full-text search extension is compiled in.
    #ifndef D_ENV_SQLITE_HAS_FTS3
        #if defined(SQLITE_ENABLE_FTS3)
            #define D_ENV_SQLITE_HAS_FTS3 1
        #else
            #define D_ENV_SQLITE_HAS_FTS3 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_FTS3_PARENTHESIS
    //   feature: detect if FTS3 enhanced query syntax (AND/OR/NOT with
    // parentheses) is compiled in.
    #ifndef D_ENV_SQLITE_HAS_FTS3_PARENTHESIS
        #if defined(SQLITE_ENABLE_FTS3_PARENTHESIS)
            #define D_ENV_SQLITE_HAS_FTS3_PARENTHESIS 1
        #else
            #define D_ENV_SQLITE_HAS_FTS3_PARENTHESIS 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_FTS3_TOKENIZER
    //   feature: detect if the fts3_tokenizer() function (custom tokenizer
    // registration) is compiled in.
    #ifndef D_ENV_SQLITE_HAS_FTS3_TOKENIZER
        #if defined(SQLITE_ENABLE_FTS3_TOKENIZER)
            #define D_ENV_SQLITE_HAS_FTS3_TOKENIZER 1
        #else
            #define D_ENV_SQLITE_HAS_FTS3_TOKENIZER 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_FTS4
    //   feature: detect if FTS4 full-text search extension is compiled in.
    // FTS4 is a superset of FTS3 with additional features (compression,
    // content tables, languageid, notindexed columns).
    #ifndef D_ENV_SQLITE_HAS_FTS4
        #if defined(SQLITE_ENABLE_FTS4)
            #define D_ENV_SQLITE_HAS_FTS4 1
        #else
            #define D_ENV_SQLITE_HAS_FTS4 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_FTS5
    //   feature: detect if FTS5 full-text search extension is compiled in.
    // FTS5 is the newest FTS module (3.8.12+), with improved query syntax,
    // better ranking, and a cleaner extension API.
    #ifndef D_ENV_SQLITE_HAS_FTS5
        #if defined(SQLITE_ENABLE_FTS5)
            #define D_ENV_SQLITE_HAS_FTS5 1
        #else
            #define D_ENV_SQLITE_HAS_FTS5 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_ANY_FTS
    //   feature: 1 if any full-text search engine is available.
    #define D_ENV_SQLITE_HAS_ANY_FTS \
        ( D_ENV_SQLITE_HAS_FTS3 || \
          D_ENV_SQLITE_HAS_FTS4 || \
          D_ENV_SQLITE_HAS_FTS5 )


// =============================================================================
// VII. JSON SUPPORT
// =============================================================================
//   JSON function availability depends on both version and compile options:
//   - 3.9.0–3.37.x: JSON1 is a loadable extension, needs SQLITE_ENABLE_JSON1
//   - 3.38.0+: JSON functions are built-in (always available), JSON1 flag
//     is no longer needed. The -> and ->> operators are added.
//   - 3.40.0+: JSONB binary format
//   - 3.43.0+: JSON5 input support

    // D_ENV_SQLITE_HAS_JSON
    //   feature: detect if JSON functions (json(), json_extract(), etc.)
    // are available, either as built-in (3.38+) or as the JSON1 extension.
    #ifndef D_ENV_SQLITE_HAS_JSON
        #if D_ENV_SQLITE_VERSION_AT_LEAST(3, 38, 0)
            // JSON is built-in since 3.38.0
            #define D_ENV_SQLITE_HAS_JSON 1
        #elif defined(SQLITE_ENABLE_JSON1)
            // JSON1 extension enabled for 3.9.0–3.37.x
            #define D_ENV_SQLITE_HAS_JSON 1
        #else
            #define D_ENV_SQLITE_HAS_JSON 0
        #endif
    #endif

    // D_ENV_SQLITE_JSON_IS_BUILTIN
    //   status: 1 if JSON functions are built-in (not requiring the JSON1
    // loadable extension flag). True since 3.38.0.
    #define D_ENV_SQLITE_JSON_IS_BUILTIN \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 38, 0)

    // D_ENV_SQLITE_HAS_JSON_OPERATORS
    //   feature: detect if -> and ->> JSON extraction operators are
    // available. Introduced in 3.38.0.
    #define D_ENV_SQLITE_HAS_JSON_OPERATORS \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 38, 0)

    // D_ENV_SQLITE_HAS_JSONB
    //   feature: detect if JSONB binary JSON format and jsonb() function
    // family are available. Introduced in 3.40.0.
    #define D_ENV_SQLITE_HAS_JSONB \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 40, 0)

    // D_ENV_SQLITE_HAS_JSON5
    //   feature: detect if JSON5 input support (trailing commas, comments,
    // single-quoted strings, etc.) is available. Introduced in 3.43.0.
    #define D_ENV_SQLITE_HAS_JSON5 \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 43, 0)


// =============================================================================
// VIII. R*TREE AND SPATIAL EXTENSIONS
// =============================================================================

    // D_ENV_SQLITE_HAS_RTREE
    //   feature: detect if R*Tree index extension is compiled in.
    #ifndef D_ENV_SQLITE_HAS_RTREE
        #if defined(SQLITE_ENABLE_RTREE)
            #define D_ENV_SQLITE_HAS_RTREE 1
        #else
            #define D_ENV_SQLITE_HAS_RTREE 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_GEOPOLY
    //   feature: detect if Geopoly extension (GeoJSON polygon operations
    // built on R*Tree) is compiled in. Requires R*Tree.
    #ifndef D_ENV_SQLITE_HAS_GEOPOLY
        #if defined(SQLITE_ENABLE_GEOPOLY)
            #define D_ENV_SQLITE_HAS_GEOPOLY 1
        #else
            #define D_ENV_SQLITE_HAS_GEOPOLY 0
        #endif
    #endif


// =============================================================================
// IX.  VIRTUAL TABLE FRAMEWORK
// =============================================================================

    // D_ENV_SQLITE_HAS_VTAB
    //   feature: detect if virtual table support is available. Virtual
    // tables have been a core feature since 3.3.7 and are not omittable
    // unless SQLITE_OMIT_VIRTUALTABLE is defined.
    #ifndef D_ENV_SQLITE_HAS_VTAB
        #if defined(SQLITE_OMIT_VIRTUALTABLE)
            #define D_ENV_SQLITE_HAS_VTAB 0
        #else
            #define D_ENV_SQLITE_HAS_VTAB 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_DBSTAT_VTAB
    //   feature: detect if the dbstat virtual table (database page usage
    // analysis) is compiled in.
    #ifndef D_ENV_SQLITE_HAS_DBSTAT_VTAB
        #if defined(SQLITE_ENABLE_DBSTAT_VTAB)
            #define D_ENV_SQLITE_HAS_DBSTAT_VTAB 1
        #else
            #define D_ENV_SQLITE_HAS_DBSTAT_VTAB 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_DBPAGE_VTAB
    //   feature: detect if the sqlite_dbpage virtual table (raw page
    // access) is compiled in.
    #ifndef D_ENV_SQLITE_HAS_DBPAGE_VTAB
        #if defined(SQLITE_ENABLE_DBPAGE_VTAB)
            #define D_ENV_SQLITE_HAS_DBPAGE_VTAB 1
        #else
            #define D_ENV_SQLITE_HAS_DBPAGE_VTAB 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_STMTVTAB
    //   feature: detect if the sqlite_stmt virtual table (statement
    // introspection) is compiled in.
    #ifndef D_ENV_SQLITE_HAS_STMTVTAB
        #if defined(SQLITE_ENABLE_STMTVTAB)
            #define D_ENV_SQLITE_HAS_STMTVTAB 1
        #else
            #define D_ENV_SQLITE_HAS_STMTVTAB 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_BYTECODE_VTAB
    //   feature: detect if the bytecode and tables_used virtual tables
    // are available. Enabled via SQLITE_ENABLE_BYTECODE_VTAB.
    #ifndef D_ENV_SQLITE_HAS_BYTECODE_VTAB
        #if defined(SQLITE_ENABLE_BYTECODE_VTAB)
            #define D_ENV_SQLITE_HAS_BYTECODE_VTAB 1
        #else
            #define D_ENV_SQLITE_HAS_BYTECODE_VTAB 0
        #endif
    #endif


// =============================================================================
// X.   SQL FEATURE DETECTION (VERSION-GATED)
// =============================================================================

    // D_ENV_SQLITE_HAS_CTE
    //   feature: Common Table Expressions (WITH clause). 3.8.3+.
    #define D_ENV_SQLITE_HAS_CTE \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 8, 3)

    // D_ENV_SQLITE_HAS_CTE_RECURSIVE
    //   feature: recursive CTEs (WITH RECURSIVE). 3.8.3+.
    #define D_ENV_SQLITE_HAS_CTE_RECURSIVE \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 8, 3)

    // D_ENV_SQLITE_HAS_CTE_MATERIALIZED
    //   feature: MATERIALIZED / NOT MATERIALIZED hints on CTEs. 3.35.0+.
    #define D_ENV_SQLITE_HAS_CTE_MATERIALIZED \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 35, 0)

    // D_ENV_SQLITE_HAS_PARTIAL_INDEX
    //   feature: partial indexes (CREATE INDEX ... WHERE). 3.8.0+.
    #define D_ENV_SQLITE_HAS_PARTIAL_INDEX \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 8, 0)

    // D_ENV_SQLITE_HAS_UPSERT
    //   feature: INSERT ... ON CONFLICT (UPSERT). 3.24.0+.
    #define D_ENV_SQLITE_HAS_UPSERT \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 24, 0)

    // D_ENV_SQLITE_HAS_WINDOW_FUNCTIONS
    //   feature: SQL window functions (ROW_NUMBER, RANK, etc.). 3.25.0+.
    #define D_ENV_SQLITE_HAS_WINDOW_FUNCTIONS \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 25, 0)

    // D_ENV_SQLITE_HAS_FILTER_CLAUSE
    //   feature: FILTER clause on aggregate and window functions. 3.30.0+.
    #define D_ENV_SQLITE_HAS_FILTER_CLAUSE \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 30, 0)

    // D_ENV_SQLITE_HAS_GENERATED_COLUMNS
    //   feature: GENERATED ALWAYS AS (virtual and stored). 3.31.0+.
    #define D_ENV_SQLITE_HAS_GENERATED_COLUMNS \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 31, 0)

    // D_ENV_SQLITE_HAS_UPDATE_FROM
    //   feature: UPDATE ... FROM (join-based UPDATE). 3.33.0+.
    #define D_ENV_SQLITE_HAS_UPDATE_FROM \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 33, 0)

    // D_ENV_SQLITE_HAS_DROP_COLUMN
    //   feature: ALTER TABLE ... DROP COLUMN. 3.35.0+.
    #define D_ENV_SQLITE_HAS_DROP_COLUMN \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 35, 0)

    // D_ENV_SQLITE_HAS_RENAME_COLUMN
    //   feature: ALTER TABLE ... RENAME COLUMN. 3.25.0+.
    #define D_ENV_SQLITE_HAS_RENAME_COLUMN \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 25, 0)

    // D_ENV_SQLITE_HAS_RETURNING
    //   feature: INSERT/UPDATE/DELETE ... RETURNING clause. 3.35.0+.
    #define D_ENV_SQLITE_HAS_RETURNING \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 35, 0)

    // D_ENV_SQLITE_HAS_STRICT_TABLES
    //   feature: STRICT tables (enforced column type affinity). 3.37.0+.
    #define D_ENV_SQLITE_HAS_STRICT_TABLES \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 37, 0)

    // D_ENV_SQLITE_HAS_ROW_VALUES
    //   feature: row value comparisons ((a,b) IN (SELECT ...)). 3.15.0+.
    #define D_ENV_SQLITE_HAS_ROW_VALUES \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 15, 0)

    // D_ENV_SQLITE_HAS_MULTI_INSERT_VALUES
    //   feature: INSERT INTO ... VALUES (...), (...), (...). 3.7.11+.
    #define D_ENV_SQLITE_HAS_MULTI_INSERT_VALUES \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 7, 11)

    // D_ENV_SQLITE_HAS_VALUES_CLAUSE
    //   feature: VALUES clause usable as a stand-alone query. 3.8.7+.
    #define D_ENV_SQLITE_HAS_VALUES_CLAUSE \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 8, 7)

    // D_ENV_SQLITE_HAS_MATH_FUNCTIONS
    //   feature: built-in math functions (sqrt, log, cos, etc.).
    // Enabled via SQLITE_ENABLE_MATH_FUNCTIONS. Built-in in CLI since
    // 3.35.0. The compile-time flag is still required for the library.
    #ifndef D_ENV_SQLITE_HAS_MATH_FUNCTIONS
        #if defined(SQLITE_ENABLE_MATH_FUNCTIONS)
            #define D_ENV_SQLITE_HAS_MATH_FUNCTIONS 1
        #else
            #define D_ENV_SQLITE_HAS_MATH_FUNCTIONS 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_SOUNDEX
    //   feature: soundex() function. Enabled via SQLITE_SOUNDEX.
    #ifndef D_ENV_SQLITE_HAS_SOUNDEX
        #if defined(SQLITE_SOUNDEX)
            #define D_ENV_SQLITE_HAS_SOUNDEX 1
        #else
            #define D_ENV_SQLITE_HAS_SOUNDEX 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_LIKE_DOESNT_MATCH_BLOBS
    //   configuration: 1 if LIKE does not match BLOB values (security
    // hardening). Enabled via SQLITE_LIKE_DOESNT_MATCH_BLOBS.
    #ifndef D_ENV_SQLITE_HAS_LIKE_DOESNT_MATCH_BLOBS
        #if defined(SQLITE_LIKE_DOESNT_MATCH_BLOBS)
            #define D_ENV_SQLITE_HAS_LIKE_DOESNT_MATCH_BLOBS 1
        #else
            #define D_ENV_SQLITE_HAS_LIKE_DOESNT_MATCH_BLOBS 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_DQS_CONTROL
    //   configuration: 1 if SQLITE_DQS is defined, controlling whether
    // double-quoted strings are accepted as string literals (DQS=1) or
    // only as identifiers (DQS=0). Available since 3.29.0.
    #ifndef D_ENV_SQLITE_HAS_DQS_CONTROL
        #if defined(SQLITE_DQS)
            #define D_ENV_SQLITE_HAS_DQS_CONTROL 1
        #else
            #define D_ENV_SQLITE_HAS_DQS_CONTROL 0
        #endif
    #endif


// =============================================================================
// XI.  JOURNAL AND WAL MODE
// =============================================================================

    // D_ENV_SQLITE_HAS_WAL
    //   feature: WAL (Write-Ahead Logging) journal mode. 3.7.0+.
    #define D_ENV_SQLITE_HAS_WAL \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 7, 0)

    // D_ENV_SQLITE_HAS_WAL2
    //   feature: WAL2 mode (concurrent writers). This is an experimental
    // feature available in separate branches; not in mainline SQLite.
    #ifndef D_ENV_SQLITE_HAS_WAL2
        #if defined(SQLITE_ENABLE_WAL2)
            #define D_ENV_SQLITE_HAS_WAL2 1
        #else
            #define D_ENV_SQLITE_HAS_WAL2 0
        #endif
    #endif

    // D_ENV_SQLITE_WAL_IS_DEFAULT
    //   configuration: 1 if WAL is the default journal mode
    // (SQLITE_DEFAULT_JOURNAL_MODE set to WAL).
    #ifndef D_ENV_SQLITE_WAL_IS_DEFAULT
        #if defined(SQLITE_DEFAULT_WAL_SYNCHRONOUS)
            // this is only defined when WAL is the expected journal mode
            #define D_ENV_SQLITE_WAL_IS_DEFAULT 1
        #else
            #define D_ENV_SQLITE_WAL_IS_DEFAULT 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_CHECKPOINT
    //   feature: WAL checkpointing API (sqlite3_wal_checkpoint_v2).
    // Available since WAL was introduced (3.7.0).
    #define D_ENV_SQLITE_HAS_CHECKPOINT \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 7, 0)

    // D_ENV_SQLITE_HAS_SNAPSHOT
    //   feature: WAL snapshot API (sqlite3_snapshot_*) for reading a
    // consistent snapshot. Enabled via SQLITE_ENABLE_SNAPSHOT.
    #ifndef D_ENV_SQLITE_HAS_SNAPSHOT
        #if defined(SQLITE_ENABLE_SNAPSHOT)
            #define D_ENV_SQLITE_HAS_SNAPSHOT 1
        #else
            #define D_ENV_SQLITE_HAS_SNAPSHOT 0
        #endif
    #endif


// =============================================================================
// XII. MEMORY MANAGEMENT
// =============================================================================

    // D_ENV_SQLITE_HAS_MEMORY_MANAGEMENT
    //   feature: memory management API (sqlite3_release_memory,
    // sqlite3_soft_heap_limit64). SQLITE_ENABLE_MEMORY_MANAGEMENT.
    #ifndef D_ENV_SQLITE_HAS_MEMORY_MANAGEMENT
        #if defined(SQLITE_ENABLE_MEMORY_MANAGEMENT)
            #define D_ENV_SQLITE_HAS_MEMORY_MANAGEMENT 1
        #else
            #define D_ENV_SQLITE_HAS_MEMORY_MANAGEMENT 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_MEMSYS5
    //   feature: memsys5 allocator (bounded memory, zero-malloc).
    // Enabled via SQLITE_ENABLE_MEMSYS5.
    #ifndef D_ENV_SQLITE_HAS_MEMSYS5
        #if defined(SQLITE_ENABLE_MEMSYS5)
            #define D_ENV_SQLITE_HAS_MEMSYS5 1
        #else
            #define D_ENV_SQLITE_HAS_MEMSYS5 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_MEMSYS3
    //   feature: memsys3 allocator (alternative bounded allocator).
    // Enabled via SQLITE_ENABLE_MEMSYS3.
    #ifndef D_ENV_SQLITE_HAS_MEMSYS3
        #if defined(SQLITE_ENABLE_MEMSYS3)
            #define D_ENV_SQLITE_HAS_MEMSYS3 1
        #else
            #define D_ENV_SQLITE_HAS_MEMSYS3 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_MEMSTATUS
    //   configuration: 1 if memory usage statistics tracking is enabled.
    // Enabled by default; disabled via SQLITE_DEFAULT_MEMSTATUS=0.
    #ifndef D_ENV_SQLITE_HAS_MEMSTATUS
        #if defined(SQLITE_DEFAULT_MEMSTATUS) && (SQLITE_DEFAULT_MEMSTATUS == 0)
            #define D_ENV_SQLITE_HAS_MEMSTATUS 0
        #else
            #define D_ENV_SQLITE_HAS_MEMSTATUS 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_MMAP
    //   feature: memory-mapped I/O (PRAGMA mmap_size). 3.7.17+.
    #define D_ENV_SQLITE_HAS_MMAP \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 7, 17)

    // D_ENV_SQLITE_ZERO_MALLOC
    //   configuration: 1 if built with SQLITE_ZERO_MALLOC (no default
    // allocator; application must provide one).
    #ifndef D_ENV_SQLITE_ZERO_MALLOC
        #if defined(SQLITE_ZERO_MALLOC)
            #define D_ENV_SQLITE_ZERO_MALLOC 1
        #else
            #define D_ENV_SQLITE_ZERO_MALLOC 0
        #endif
    #endif


// =============================================================================
// XIII. BACKUP, SERIALIZATION, AND SESSION EXTENSIONS
// =============================================================================

    // D_ENV_SQLITE_HAS_BACKUP_API
    //   feature: online backup API (sqlite3_backup_*). 3.6.11+.
    #define D_ENV_SQLITE_HAS_BACKUP_API \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 6, 11)

    // D_ENV_SQLITE_HAS_SERIALIZE
    //   feature: serialize/deserialize API (sqlite3_serialize,
    // sqlite3_deserialize). Enabled via SQLITE_ENABLE_DESERIALIZE.
    // Built-in since 3.36.0.
    #ifndef D_ENV_SQLITE_HAS_SERIALIZE
        #if D_ENV_SQLITE_VERSION_AT_LEAST(3, 36, 0)
            #define D_ENV_SQLITE_HAS_SERIALIZE 1
        #elif defined(SQLITE_ENABLE_DESERIALIZE)
            #define D_ENV_SQLITE_HAS_SERIALIZE 1
        #else
            #define D_ENV_SQLITE_HAS_SERIALIZE 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_SESSION
    //   feature: session extension (change tracking, changesets,
    // patchsets). Enabled via SQLITE_ENABLE_SESSION.
    #ifndef D_ENV_SQLITE_HAS_SESSION
        #if defined(SQLITE_ENABLE_SESSION)
            #define D_ENV_SQLITE_HAS_SESSION 1
        #else
            #define D_ENV_SQLITE_HAS_SESSION 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_PREUPDATE_HOOK
    //   feature: pre-update hook (sqlite3_preupdate_*) for change
    // notification before writes. Enabled via SQLITE_ENABLE_PREUPDATE_HOOK.
    #ifndef D_ENV_SQLITE_HAS_PREUPDATE_HOOK
        #if defined(SQLITE_ENABLE_PREUPDATE_HOOK)
            #define D_ENV_SQLITE_HAS_PREUPDATE_HOOK 1
        #else
            #define D_ENV_SQLITE_HAS_PREUPDATE_HOOK 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_RBU
    //   feature: Resumable Bulk Update extension (sqlite3rbu_*).
    // Enabled via SQLITE_ENABLE_RBU.
    #ifndef D_ENV_SQLITE_HAS_RBU
        #if defined(SQLITE_ENABLE_RBU)
            #define D_ENV_SQLITE_HAS_RBU 1
        #else
            #define D_ENV_SQLITE_HAS_RBU 0
        #endif
    #endif


// =============================================================================
// XIV. SECURITY AND HARDENING
// =============================================================================

    // D_ENV_SQLITE_HAS_ENCRYPTION
    //   feature: detect if any encryption extension is available.
    // SQLite does not ship with built-in encryption. Third-party
    // solutions include SQLite Encryption Extension (SEE), SQLCipher,
    // and wxSQLite3. Detected via SQLITE_HAS_CODEC or
    // SQLITE_ENABLE_SEE.
    #ifndef D_ENV_SQLITE_HAS_ENCRYPTION
        #if ( defined(SQLITE_HAS_CODEC)   ||  \
              defined(SQLITE_ENABLE_SEE)  ||  \
              defined(SQLCIPHER_VERSION) )
            #define D_ENV_SQLITE_HAS_ENCRYPTION 1
        #else
            #define D_ENV_SQLITE_HAS_ENCRYPTION 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_SEE
    //   feature: detect SQLite Encryption Extension (official paid ext).
    #ifndef D_ENV_SQLITE_HAS_SEE
        #if defined(SQLITE_ENABLE_SEE)
            #define D_ENV_SQLITE_HAS_SEE 1
        #else
            #define D_ENV_SQLITE_HAS_SEE 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_SQLCIPHER
    //   feature: detect SQLCipher (open-source encryption fork).
    #ifndef D_ENV_SQLITE_HAS_SQLCIPHER
        #if defined(SQLCIPHER_VERSION)
            #define D_ENV_SQLITE_HAS_SQLCIPHER 1
        #else
            #define D_ENV_SQLITE_HAS_SQLCIPHER 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_API_ARMOR
    //   feature: API armor (extra parameter validation in public API).
    // Enabled via SQLITE_ENABLE_API_ARMOR.
    #ifndef D_ENV_SQLITE_HAS_API_ARMOR
        #if defined(SQLITE_ENABLE_API_ARMOR)
            #define D_ENV_SQLITE_HAS_API_ARMOR 1
        #else
            #define D_ENV_SQLITE_HAS_API_ARMOR 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_URI_FILENAMES
    //   feature: URI filename support (file:path?mode=ro&cache=shared).
    // Enabled via SQLITE_USE_URI. Default on since 3.7.7+, but
    // SQLITE_USE_URI=1 makes it the default for sqlite3_open().
    #ifndef D_ENV_SQLITE_HAS_URI_FILENAMES
        #if ( defined(SQLITE_USE_URI) ||  \
              D_ENV_SQLITE_VERSION_AT_LEAST(3, 7, 7) )
            #define D_ENV_SQLITE_HAS_URI_FILENAMES 1
        #else
            #define D_ENV_SQLITE_HAS_URI_FILENAMES 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_DEFENSIVE_MODE
    //   feature: SQLITE_DBCONFIG_DEFENSIVE mode (prevents corruption
    // via SQL). Available since 3.26.0.
    #define D_ENV_SQLITE_HAS_DEFENSIVE_MODE \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 26, 0)

    // D_ENV_SQLITE_HAS_TRUSTED_SCHEMA
    //   feature: SQLITE_DBCONFIG_TRUSTED_SCHEMA control (restrict
    // virtual table and function usage). Available since 3.31.0.
    #define D_ENV_SQLITE_HAS_TRUSTED_SCHEMA \
        D_ENV_SQLITE_VERSION_AT_LEAST(3, 31, 0)


// =============================================================================
// XV.  EXTENSION AND LOADABLE MODULE SUPPORT
// =============================================================================

    // D_ENV_SQLITE_HAS_LOAD_EXTENSION
    //   feature: detect if sqlite3_load_extension() (runtime extension
    // loading) is available. Disabled via SQLITE_OMIT_LOAD_EXTENSION.
    #ifndef D_ENV_SQLITE_HAS_LOAD_EXTENSION
        #if defined(SQLITE_OMIT_LOAD_EXTENSION)
            #define D_ENV_SQLITE_HAS_LOAD_EXTENSION 0
        #else
            #define D_ENV_SQLITE_HAS_LOAD_EXTENSION 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_AUTO_EXTENSION
    //   feature: sqlite3_auto_extension() (register extensions loaded
    // automatically for every connection).
    #ifndef D_ENV_SQLITE_HAS_AUTO_EXTENSION
        #if defined(SQLITE_OMIT_AUTOINIT)
            #define D_ENV_SQLITE_HAS_AUTO_EXTENSION 0
        #else
            #define D_ENV_SQLITE_HAS_AUTO_EXTENSION 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_ICU
    //   feature: detect if ICU (International Components for Unicode)
    // extension is compiled in (Unicode-aware collation, LIKE, UPPER,
    // LOWER). Enabled via SQLITE_ENABLE_ICU.
    #ifndef D_ENV_SQLITE_HAS_ICU
        #if defined(SQLITE_ENABLE_ICU)
            #define D_ENV_SQLITE_HAS_ICU 1
        #else
            #define D_ENV_SQLITE_HAS_ICU 0
        #endif
    #endif


// =============================================================================
// XVI. CORE API FEATURES
// =============================================================================

    // D_ENV_SQLITE_HAS_UNLOCK_NOTIFY
    //   feature: sqlite3_unlock_notify() for handling SQLITE_LOCKED in
    // multi-threaded WAL scenarios. SQLITE_ENABLE_UNLOCK_NOTIFY.
    #ifndef D_ENV_SQLITE_HAS_UNLOCK_NOTIFY
        #if defined(SQLITE_ENABLE_UNLOCK_NOTIFY)
            #define D_ENV_SQLITE_HAS_UNLOCK_NOTIFY 1
        #else
            #define D_ENV_SQLITE_HAS_UNLOCK_NOTIFY 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_COLUMN_METADATA
    //   feature: column metadata API (sqlite3_column_database_name,
    // sqlite3_column_table_name, sqlite3_column_origin_name).
    // SQLITE_ENABLE_COLUMN_METADATA.
    #ifndef D_ENV_SQLITE_HAS_COLUMN_METADATA
        #if defined(SQLITE_ENABLE_COLUMN_METADATA)
            #define D_ENV_SQLITE_HAS_COLUMN_METADATA 1
        #else
            #define D_ENV_SQLITE_HAS_COLUMN_METADATA 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_STAT4
    //   feature: STAT4 index statistics (detailed histogram data for
    // the query planner). SQLITE_ENABLE_STAT4.
    #ifndef D_ENV_SQLITE_HAS_STAT4
        #if defined(SQLITE_ENABLE_STAT4)
            #define D_ENV_SQLITE_HAS_STAT4 1
        #else
            #define D_ENV_SQLITE_HAS_STAT4 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_UPDATE_DELETE_LIMIT
    //   feature: ORDER BY and LIMIT on UPDATE and DELETE statements.
    // SQLITE_ENABLE_UPDATE_DELETE_LIMIT.
    #ifndef D_ENV_SQLITE_HAS_UPDATE_DELETE_LIMIT
        #if defined(SQLITE_ENABLE_UPDATE_DELETE_LIMIT)
            #define D_ENV_SQLITE_HAS_UPDATE_DELETE_LIMIT 1
        #else
            #define D_ENV_SQLITE_HAS_UPDATE_DELETE_LIMIT 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_NORMALIZE
    //   feature: sqlite3_normalized_sql() for query normalization.
    // SQLITE_ENABLE_NORMALIZE.
    #ifndef D_ENV_SQLITE_HAS_NORMALIZE
        #if defined(SQLITE_ENABLE_NORMALIZE)
            #define D_ENV_SQLITE_HAS_NORMALIZE 1
        #else
            #define D_ENV_SQLITE_HAS_NORMALIZE 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_EXPLAIN_COMMENTS
    //   feature: EXPLAIN output includes comments describing operations.
    // SQLITE_ENABLE_EXPLAIN_COMMENTS.
    #ifndef D_ENV_SQLITE_HAS_EXPLAIN_COMMENTS
        #if defined(SQLITE_ENABLE_EXPLAIN_COMMENTS)
            #define D_ENV_SQLITE_HAS_EXPLAIN_COMMENTS 1
        #else
            #define D_ENV_SQLITE_HAS_EXPLAIN_COMMENTS 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_STMT_SCANSTATUS
    //   feature: sqlite3_stmt_scanstatus() for per-loop scan statistics.
    // SQLITE_ENABLE_STMT_SCANSTATUS.
    #ifndef D_ENV_SQLITE_HAS_STMT_SCANSTATUS
        #if defined(SQLITE_ENABLE_STMT_SCANSTATUS)
            #define D_ENV_SQLITE_HAS_STMT_SCANSTATUS 1
        #else
            #define D_ENV_SQLITE_HAS_STMT_SCANSTATUS 0
        #endif
    #endif


// =============================================================================
// XVII. VFS (VIRTUAL FILE SYSTEM) DETECTION
// =============================================================================
//   SQLite uses VFS for all I/O operations. The default VFS depends on
// the platform. Custom VFS modules can be registered at runtime.

    // D_ENV_SQLITE_HAS_UNIX_VFS
    //   feature: detect if Unix VFS (posix I/O) is the expected default.
    #ifndef D_ENV_SQLITE_HAS_UNIX_VFS
        #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
            #define D_ENV_SQLITE_HAS_UNIX_VFS 1
        #else
            #define D_ENV_SQLITE_HAS_UNIX_VFS 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_WIN32_VFS
    //   feature: detect if Win32 VFS is the expected default.
    #ifndef D_ENV_SQLITE_HAS_WIN32_VFS
        #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
            #define D_ENV_SQLITE_HAS_WIN32_VFS 1
        #else
            #define D_ENV_SQLITE_HAS_WIN32_VFS 0
        #endif
    #endif

    // D_ENV_SQLITE_HAS_CUSTOM_VFS
    //   feature: detect if a custom VFS is registered (cannot be fully
    // determined at compile-time; this checks for common indicators).
    #ifndef D_ENV_SQLITE_HAS_CUSTOM_VFS
        #if defined(SQLITE_OS_OTHER)
            #define D_ENV_SQLITE_HAS_CUSTOM_VFS 1
        #else
            #define D_ENV_SQLITE_HAS_CUSTOM_VFS 0
        #endif
    #endif


// =============================================================================
// XVIII. OMITTED FEATURES (SQLITE_OMIT_*)
// =============================================================================
//   SQLite supports omitting features at compile-time for code size
// reduction. These detect ABSENCE of features.

    // D_ENV_SQLITE_HAS_FOREIGN_KEYS
    //   feature: detect if foreign key support is compiled in.
    // Omitted via SQLITE_OMIT_FOREIGN_KEY.
    #ifndef D_ENV_SQLITE_HAS_FOREIGN_KEYS
        #if defined(SQLITE_OMIT_FOREIGN_KEY)
            #define D_ENV_SQLITE_HAS_FOREIGN_KEYS 0
        #else
            #define D_ENV_SQLITE_HAS_FOREIGN_KEYS 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_TRIGGERS
    //   feature: detect if trigger support is compiled in.
    #ifndef D_ENV_SQLITE_HAS_TRIGGERS
        #if defined(SQLITE_OMIT_TRIGGER)
            #define D_ENV_SQLITE_HAS_TRIGGERS 0
        #else
            #define D_ENV_SQLITE_HAS_TRIGGERS 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_VIEWS
    //   feature: detect if VIEW support is compiled in.
    #ifndef D_ENV_SQLITE_HAS_VIEWS
        #if defined(SQLITE_OMIT_VIEW)
            #define D_ENV_SQLITE_HAS_VIEWS 0
        #else
            #define D_ENV_SQLITE_HAS_VIEWS 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_SUBQUERY
    //   feature: detect if subquery support is compiled in.
    #ifndef D_ENV_SQLITE_HAS_SUBQUERY
        #if defined(SQLITE_OMIT_SUBQUERY)
            #define D_ENV_SQLITE_HAS_SUBQUERY 0
        #else
            #define D_ENV_SQLITE_HAS_SUBQUERY 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_COMPOUND_SELECT
    //   feature: detect if UNION, INTERSECT, EXCEPT are compiled in.
    #ifndef D_ENV_SQLITE_HAS_COMPOUND_SELECT
        #if defined(SQLITE_OMIT_COMPOUND_SELECT)
            #define D_ENV_SQLITE_HAS_COMPOUND_SELECT 0
        #else
            #define D_ENV_SQLITE_HAS_COMPOUND_SELECT 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_ALTERTABLE
    //   feature: detect if ALTER TABLE is compiled in.
    #ifndef D_ENV_SQLITE_HAS_ALTERTABLE
        #if defined(SQLITE_OMIT_ALTERTABLE)
            #define D_ENV_SQLITE_HAS_ALTERTABLE 0
        #else
            #define D_ENV_SQLITE_HAS_ALTERTABLE 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_ANALYZE
    //   feature: detect if ANALYZE is compiled in.
    #ifndef D_ENV_SQLITE_HAS_ANALYZE
        #if defined(SQLITE_OMIT_ANALYZE)
            #define D_ENV_SQLITE_HAS_ANALYZE 0
        #else
            #define D_ENV_SQLITE_HAS_ANALYZE 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_ATTACH
    //   feature: detect if ATTACH DATABASE is compiled in.
    #ifndef D_ENV_SQLITE_HAS_ATTACH
        #if defined(SQLITE_OMIT_ATTACH)
            #define D_ENV_SQLITE_HAS_ATTACH 0
        #else
            #define D_ENV_SQLITE_HAS_ATTACH 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_AUTOINCREMENT
    //   feature: detect if AUTOINCREMENT is compiled in.
    #ifndef D_ENV_SQLITE_HAS_AUTOINCREMENT
        #if defined(SQLITE_OMIT_AUTOINCREMENT)
            #define D_ENV_SQLITE_HAS_AUTOINCREMENT 0
        #else
            #define D_ENV_SQLITE_HAS_AUTOINCREMENT 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_AUTHORIZATION
    //   feature: detect if the authorization callback
    // (sqlite3_set_authorizer) is compiled in.
    #ifndef D_ENV_SQLITE_HAS_AUTHORIZATION
        #if defined(SQLITE_OMIT_AUTHORIZATION)
            #define D_ENV_SQLITE_HAS_AUTHORIZATION 0
        #else
            #define D_ENV_SQLITE_HAS_AUTHORIZATION 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_VACUUM
    //   feature: detect if VACUUM is compiled in.
    #ifndef D_ENV_SQLITE_HAS_VACUUM
        #if defined(SQLITE_OMIT_VACUUM)
            #define D_ENV_SQLITE_HAS_VACUUM 0
        #else
            #define D_ENV_SQLITE_HAS_VACUUM 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_INTEGRITY_CHECK
    //   feature: detect if PRAGMA integrity_check is compiled in.
    #ifndef D_ENV_SQLITE_HAS_INTEGRITY_CHECK
        #if defined(SQLITE_OMIT_INTEGRITY_CHECK)
            #define D_ENV_SQLITE_HAS_INTEGRITY_CHECK 0
        #else
            #define D_ENV_SQLITE_HAS_INTEGRITY_CHECK 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_SCHEMA_PRAGMAS
    //   feature: detect if schema-introspection PRAGMAs
    // (table_info, etc.) are compiled in.
    #ifndef D_ENV_SQLITE_HAS_SCHEMA_PRAGMAS
        #if defined(SQLITE_OMIT_SCHEMA_PRAGMAS)
            #define D_ENV_SQLITE_HAS_SCHEMA_PRAGMAS 0
        #else
            #define D_ENV_SQLITE_HAS_SCHEMA_PRAGMAS 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_PROGRESS_CALLBACK
    //   feature: detect if sqlite3_progress_handler() is compiled in.
    #ifndef D_ENV_SQLITE_HAS_PROGRESS_CALLBACK
        #if defined(SQLITE_OMIT_PROGRESS_CALLBACK)
            #define D_ENV_SQLITE_HAS_PROGRESS_CALLBACK 0
        #else
            #define D_ENV_SQLITE_HAS_PROGRESS_CALLBACK 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_TRACE
    //   feature: detect if sqlite3_trace_v2() is compiled in.
    #ifndef D_ENV_SQLITE_HAS_TRACE
        #if defined(SQLITE_OMIT_TRACE)
            #define D_ENV_SQLITE_HAS_TRACE 0
        #else
            #define D_ENV_SQLITE_HAS_TRACE 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_UTF16
    //   feature: detect if UTF-16 support is compiled in.
    #ifndef D_ENV_SQLITE_HAS_UTF16
        #if defined(SQLITE_OMIT_UTF16)
            #define D_ENV_SQLITE_HAS_UTF16 0
        #else
            #define D_ENV_SQLITE_HAS_UTF16 1
        #endif
    #endif

    // D_ENV_SQLITE_HAS_COMPLETE
    //   feature: detect if sqlite3_complete() is compiled in.
    #ifndef D_ENV_SQLITE_HAS_COMPLETE
        #if defined(SQLITE_OMIT_COMPLETE)
            #define D_ENV_SQLITE_HAS_COMPLETE 0
        #else
            #define D_ENV_SQLITE_HAS_COMPLETE 1
        #endif
    #endif


// =============================================================================
// XIX.  CONVENIENCE / COMPOSITE MACROS
// =============================================================================

    // D_ENV_SQLITE_HAS_MODERN_SQL
    //   macro: evaluates to 1 if CTEs, window functions, UPSERT,
    // RETURNING, and generated columns are all available.
    #define D_ENV_SQLITE_HAS_MODERN_SQL \
        ( D_ENV_SQLITE_HAS_CTE              && \
          D_ENV_SQLITE_HAS_WINDOW_FUNCTIONS && \
          D_ENV_SQLITE_HAS_UPSERT           && \
          D_ENV_SQLITE_HAS_RETURNING        && \
          D_ENV_SQLITE_HAS_GENERATED_COLUMNS )

    // D_ENV_SQLITE_HAS_MODERN_JSON
    //   macro: evaluates to 1 if JSON is built-in with -> / ->>
    // operators and JSONB support.
    #define D_ENV_SQLITE_HAS_MODERN_JSON \
        ( D_ENV_SQLITE_JSON_IS_BUILTIN      && \
          D_ENV_SQLITE_HAS_JSON_OPERATORS   && \
          D_ENV_SQLITE_HAS_JSONB )

    // D_ENV_SQLITE_HAS_MODERN_DDL
    //   macro: evaluates to 1 if DROP COLUMN, RENAME COLUMN, and
    // generated columns are all available.
    #define D_ENV_SQLITE_HAS_MODERN_DDL \
        ( D_ENV_SQLITE_HAS_DROP_COLUMN      && \
          D_ENV_SQLITE_HAS_RENAME_COLUMN    && \
          D_ENV_SQLITE_HAS_GENERATED_COLUMNS )

    // D_ENV_SQLITE_HAS_FULL_SEARCH
    //   macro: evaluates to 1 if FTS5 and R*Tree are both available.
    #define D_ENV_SQLITE_HAS_FULL_SEARCH \
        ( D_ENV_SQLITE_HAS_FTS5 && D_ENV_SQLITE_HAS_RTREE )

    // D_ENV_SQLITE_HAS_STRICT_MODE
    //   macro: evaluates to 1 if STRICT tables and defensive mode are
    // both available.
    #define D_ENV_SQLITE_HAS_STRICT_MODE \
        ( D_ENV_SQLITE_HAS_STRICT_TABLES && \
          D_ENV_SQLITE_HAS_DEFENSIVE_MODE )

    // D_ENV_SQLITE_IS_FULLY_MODERN
    //   macro: evaluates to 1 if the SQLite version has all "modern"
    // capabilities (roughly 3.40.0+ with comprehensive feature set).
    #define D_ENV_SQLITE_IS_FULLY_MODERN \
        ( D_ENV_SQLITE_HAS_MODERN_SQL  && \
          D_ENV_SQLITE_HAS_MODERN_JSON && \
          D_ENV_SQLITE_HAS_MODERN_DDL  && \
          D_ENV_SQLITE_HAS_STRICT_MODE && \
          D_ENV_SQLITE_HAS_WAL )


#endif  // D_ENV_SQLITE_DETECTED


#endif  // DJINTERP_ENVIRONMENT_SQLITE_
