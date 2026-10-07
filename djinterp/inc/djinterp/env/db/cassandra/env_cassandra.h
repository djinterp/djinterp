/*******************************************************************************
* djinterp [env]                                                 env_cassandra.h
*
* djinterp Apache Cassandra environment detection.
*   Compile-time detection of an Apache Cassandra environment: the DataStax
* C/C++ driver (cpp-driver) and its version, the target server version, the
* edition (Apache Cassandra, DataStax Enterprise, or Astra), and the
* capabilities those gate: driver features, the CQL protocol and statements,
* the data model, indexes and views, user-defined functions, storage and
* compaction, replication and consistency, security, and DSE's extensions.
*   Two versions are tracked. D_ENV_CASS_DRIVER_* is the driver's, read at
* compile time. D_ENV_CASS_SERVER_* is the target server's, a runtime property
* that must be configured, through D_CFG_ENV_CASS_SERVER_VERSION or a
* pre-defined D_ENV_CASS_DETECTED_SERVER_*; without one, server-gated features
* read 0. Both are encoded as MAJOR*10000 + MINOR*100 + PATCH, so Cassandra
* 4.1.3 is 40103. DSE and Astra are manual as well, through
* D_ENV_CASS_DETECTED_DSE and D_ENV_CASS_DETECTED_ASTRA.
*   D_ENV_CASS_HAS_* are capability flags, and D_ENV_CASS_IS_* the edition and
* release series. Sections 5 onward exist only when a driver or a server
* version is known; the last of them publishes the flattened D_ENV_CASSANDRA_*
* vocabulary that cassandra.hpp consumes.
*   Settings live in cfg_env_cassandra.h: D_CFG_ENV_USING_CASSANDRA includes
* the driver header, and D_CFG_ENV_CASS_CUSTOM switches the driver to manual
* detection. This header also includes env_db.h, for the base database
* detection.
*
*
* path:      /inc/djinterp/env/db/cassandra/env_cassandra.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION
    -----------------------
    1.  Vendor header
         1.  D_ENV_CASSANDRA_HEADER_INCLUDED
2.  VERSION ENCODING
    ----------------
    1.  Encoding and decoding
         1.  D_ENV_CASS_ENCODE_VERSION
         2.  D_ENV_CASS_DECODE_MAJOR
         3.  D_ENV_CASS_DECODE_MINOR
         4.  D_ENV_CASS_DECODE_PATCH
3.  DRIVER VERSION DETECTION
    ------------------------
    1.  Server release IDs
         1.  D_ENV_CASS_SERVER_<MAJOR>_<MINOR>
    2.  Detected driver
         1.  D_ENV_CASS_DRIVER_DETECTED / D_ENV_CASS_DRIVER_*
4.  TARGET SERVER VERSION DETECTION
    -------------------------------
    1.  Target server version
         1.  D_ENV_CASS_SERVER_VERSION_ID
         2.  D_ENV_CASS_SERVER_MAJOR / D_ENV_CASS_SERVER_MINOR
         3.  D_ENV_CASS_SERVER_KNOWN
         4.  D_ENV_CASS_DETECTED
5.  VERSION COMPARISON MACROS
    -------------------------
    1.  Driver comparisons
         1.  D_ENV_CASS_DRIVER_AT_LEAST
    2.  Server comparisons
         1.  D_ENV_CASS_SERVER_AT_LEAST
         2.  D_ENV_CASS_SERVER_BELOW
         3.  D_ENV_CASS_SERVER_IN_RANGE
    3.  Release series
         1.  D_ENV_CASS_IS_<SERIES>
6.  EDITION DETECTION
    -----------------
    1.  Editions
         1.  D_ENV_CASS_IS_DSE
         2.  D_ENV_CASS_IS_APACHE
         3.  D_ENV_CASS_IS_ASTRA
7.  CLIENT DRIVER FEATURES
    ----------------------
    1.  Driver features
         1.  D_ENV_CASS_HAS_DRIVER
         2.  D_ENV_CASS_HAS_ASYNC_API
         3.  D_ENV_CASS_HAS_CONNECTION_POOL
         4.  D_ENV_CASS_HAS_TOKEN_AWARE_ROUTING
         5.  D_ENV_CASS_HAS_DC_AWARE_ROUTING
         6.  D_ENV_CASS_HAS_LATENCY_AWARE_ROUTING
         7.  D_ENV_CASS_HAS_SPECULATIVE_EXECUTION
         8.  D_ENV_CASS_HAS_RETRY_POLICY
         9.  D_ENV_CASS_HAS_DRIVER_METRICS
8.  CQL PROTOCOL AND STATEMENT FEATURES
    -----------------------------------
    1.  Protocol and statements
         1.  D_ENV_CASS_HAS_PREPARED_STATEMENTS
         2.  D_ENV_CASS_HAS_BATCH
         3.  D_ENV_CASS_HAS_BATCH_UNLOGGED
         4.  D_ENV_CASS_HAS_PAGING
         5.  D_ENV_CASS_HAS_LWT
         6.  D_ENV_CASS_HAS_JSON
         7.  D_ENV_CASS_HAS_PER_PARTITION_LIMIT
         8.  D_ENV_CASS_HAS_GROUP_BY
         9.  D_ENV_CASS_HAS_VIRTUAL_TABLES
9.  DATA MODEL AND TYPE SYSTEM
    --------------------------
    1.  Data model and types
         1.  D_ENV_CASS_HAS_COLLECTIONS
         2.  D_ENV_CASS_HAS_UDT
         3.  D_ENV_CASS_HAS_TUPLES
         4.  D_ENV_CASS_HAS_FROZEN_COLLECTIONS
         5.  D_ENV_CASS_HAS_STATIC_COLUMNS
         6.  D_ENV_CASS_HAS_COUNTERS
         7.  D_ENV_CASS_HAS_DURATION_TYPE
         8.  D_ENV_CASS_HAS_TTL
10. INDEXING AND VIEWS
    ------------------
    1.  Indexes and views
         1.  D_ENV_CASS_HAS_SECONDARY_INDEX
         2.  D_ENV_CASS_HAS_SASI
         3.  D_ENV_CASS_HAS_SAI
         4.  D_ENV_CASS_HAS_MATERIALIZED_VIEWS
         5.  D_ENV_CASS_HAS_VECTOR_SEARCH
11. USER-DEFINED FUNCTIONS AND AGGREGATES
    -------------------------------------
    1.  Functions and aggregates
         1.  D_ENV_CASS_HAS_UDF
         2.  D_ENV_CASS_HAS_UDA
12. STORAGE ENGINE AND COMPACTION
    -----------------------------
    1.  Storage and compaction
         1.  D_ENV_CASS_HAS_LSM_ENGINE
         2.  D_ENV_CASS_HAS_STCS
         3.  D_ENV_CASS_HAS_LCS
         4.  D_ENV_CASS_HAS_TWCS
         5.  D_ENV_CASS_HAS_UCS
         6.  D_ENV_CASS_HAS_INCREMENTAL_REPAIR
         7.  D_ENV_CASS_HAS_CDC
13. REPLICATION, CONSISTENCY, AND DISTRIBUTION
    ------------------------------------------
    1.  Replication and consistency
         1.  D_ENV_CASS_HAS_TUNABLE_CONSISTENCY
         2.  D_ENV_CASS_HAS_NETWORK_TOPOLOGY_STRATEGY
         3.  D_ENV_CASS_HAS_VNODES
         4.  D_ENV_CASS_HAS_HINTED_HANDOFF
         5.  D_ENV_CASS_HAS_LOCAL_CONSISTENCY
         6.  D_ENV_CASS_HAS_TRANSIENT_REPLICATION
14. SECURITY
    --------
    1.  Authentication, authorization and encryption
         1.  D_ENV_CASS_HAS_PASSWORD_AUTH
         2.  D_ENV_CASS_HAS_RBAC
         3.  D_ENV_CASS_HAS_SSL
         4.  D_ENV_CASS_HAS_AUTH_LDAP
         5.  D_ENV_CASS_HAS_AUTH_KERBEROS
         6.  D_ENV_CASS_HAS_TDE
         7.  D_ENV_CASS_HAS_ROW_LEVEL_ACCESS
         8.  D_ENV_CASS_HAS_AUDIT_LOG
15. DSE-SPECIFIC FEATURES
    ---------------------
    1.  DSE features
         1.  D_ENV_CASS_HAS_DSE_SEARCH
         2.  D_ENV_CASS_HAS_DSE_GRAPH
         3.  D_ENV_CASS_HAS_DSE_ANALYTICS
         4.  D_ENV_CASS_HAS_DSE_IN_MEMORY
16. COMPOSITE CHECKS
    ----------------
    1.  Composite checks
         1.  D_ENV_CASS_HAS_ADVANCED_TYPES
         2.  D_ENV_CASS_HAS_USER_LOGIC
         3.  D_ENV_CASS_HAS_MODERN_INDEXING
         4.  D_ENV_CASS_HAS_MODERN_SECURITY
         5.  D_ENV_CASS_HAS_SMART_ROUTING
         6.  D_ENV_CASS_IS_FULLY_MODERN
17. DEPRECATION AND REMOVAL
    -----------------------
    1.  Deprecations
         1.  D_ENV_CASS_REMOVED_THRIFT
         2.  D_ENV_CASS_REMOVED_COMPACT_STORAGE
         3.  D_ENV_CASS_DEPRECATED_SASI
18. CONSUMER COMPATIBILITY LAYER
    ----------------------------
    1.  Consumer vocabulary
         1.  D_ENV_CASSANDRA_*
*/

#ifndef DJINTERP_ENV_DB_CASSANDRA_ENV_CASSANDRA_H
#define DJINTERP_ENV_DB_CASSANDRA_ENV_CASSANDRA_H 1

// djinterp
#include "../../../config/core/env/db/cassandra/cfg_env_cassandra.h"  // D_CFG_ENV_*
#include "../env_db.h"  // base database detection (D_ENV_DB_*)


//==============================================================================
// 1.  VENDOR HEADER INCLUSION
//==============================================================================
// Driven by D_CFG_ENV_USING_CASSANDRA, from cfg_env_cassandra.h. When on,
// this section includes the DataStax C/C++ driver header (cassandra.h), a C
// API usable from C and C++ alike. Detection below is gated on
// D_ENV_CASSANDRA_HEADER_INCLUDED, so that no cassandra symbol is referenced
// unless the header is in scope.


// 1.1    Vendor header
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_CASSANDRA_HEADER_INCLUDED
//   detection: 1 once this section has included the driver header, and 0
// when D_CFG_ENV_USING_CASSANDRA is off; D_ENV_DB_HAS_CASSANDRA_CLIENT_C
// follows it unless pre-defined. With the setting on, a missing header is
// an #error.
#if D_CFG_IS_ON(D_CFG_ENV_USING_CASSANDRA)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_CASSANDRA_C_PATH)
            #include D_CFG_ENV_CASSANDRA_C_PATH  // cpp-driver
            #define D_ENV_CASSANDRA_HEADER_INCLUDED 1
        #elif __has_include(<cassandra.h>)
            // cassandra
            #include <cassandra.h>  // cpp-driver
            #define D_ENV_CASSANDRA_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_CASSANDRA=1 but no cassandra.h header "    \
                   "was found. Install the DataStax cpp-driver "               \
                   "(libcassandra-dev or equivalent), or define "              \
                   "D_CFG_ENV_CASSANDRA_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_CASSANDRA_C_PATH  // cpp-driver
        #define D_ENV_CASSANDRA_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_CASSANDRA_CLIENT_C
        #define D_ENV_DB_HAS_CASSANDRA_CLIENT_C 1
    #endif  // D_ENV_DB_HAS_CASSANDRA_CLIENT_C

#else
    #define D_ENV_CASSANDRA_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_CASSANDRA_CLIENT_C
        #define D_ENV_DB_HAS_CASSANDRA_CLIENT_C 0
    #endif  // D_ENV_DB_HAS_CASSANDRA_CLIENT_C
#endif  // D_CFG_ENV_USING_CASSANDRA


//==============================================================================
// 2.  VERSION ENCODING
//==============================================================================


// 2.1    Encoding and decoding
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_CASS_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple.
#define D_ENV_CASS_ENCODE_VERSION(major, minor, patch)                         \
    ((major) * 10000 + (minor) * 100 + (patch))

// 2.1.2
// D_ENV_CASS_DECODE_MAJOR
//   macro: extracts the major version.
#define D_ENV_CASS_DECODE_MAJOR(ver)                                           \
    ((ver) / 10000)

// 2.1.3
// D_ENV_CASS_DECODE_MINOR
//   macro: extracts the minor version.
#define D_ENV_CASS_DECODE_MINOR(ver)                                           \
    (((ver) / 100) % 100)

// 2.1.4
// D_ENV_CASS_DECODE_PATCH
//   macro: extracts the patch version.
#define D_ENV_CASS_DECODE_PATCH(ver)                                           \
    ((ver) % 100)


//==============================================================================
// 3.  DRIVER VERSION DETECTION
//==============================================================================
// The DataStax C/C++ driver is detected from the CASS_VERSION_* macros
// exposed by cassandra.h. The driver version determines which CQL binary
// protocol features and client-side APIs are available at compile time.


// 3.1    Server release IDs
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_CASS_SERVER_<MAJOR>_<MINOR>
//   constant: encoded IDs of the server releases that gate features below.
#define D_ENV_CASS_SERVER_2_0          20000
#define D_ENV_CASS_SERVER_2_1          20100
#define D_ENV_CASS_SERVER_2_2          20200
#define D_ENV_CASS_SERVER_3_0          30000
#define D_ENV_CASS_SERVER_3_11         31100
#define D_ENV_CASS_SERVER_4_0          40000
#define D_ENV_CASS_SERVER_4_1          40100
#define D_ENV_CASS_SERVER_5_0          50000

// 3.2    Detected driver
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_CASS_DRIVER_DETECTED / D_ENV_CASS_DRIVER_*
//   detection: D_ENV_CASS_DRIVER_DETECTED is 1 when the cpp-driver version is
// known, and D_ENV_CASS_DRIVER_VERSION_ID, _MAJOR, _MINOR, _PATCH and
// _VERSION_SUFFIX then describe it. Automatic mode reads CASS_VERSION_MAJOR,
// _MINOR, _PATCH and _SUFFIX from cassandra.h; manual mode
// (D_CFG_ENV_CASS_CUSTOM) reads D_ENV_CASS_DETECTED_DRIVER_VERSION.
#if D_CFG_IS_OFF(D_CFG_ENV_CASS_CUSTOM)

    // automatic detection requires cassandra.h to be in scope; if
    // D_CFG_ENV_USING_CASSANDRA was not enabled the sentinel is 0 and we
    // skip cleanly (no reference to CASS_VERSION_MAJOR).
    #if ( (D_ENV_CASSANDRA_HEADER_INCLUDED) &&                                 \
          (defined(CASS_VERSION_MAJOR)) )
        #define D_ENV_CASS_DRIVER_DETECTED    1
        #define D_ENV_CASS_DRIVER_MAJOR       CASS_VERSION_MAJOR
        #define D_ENV_CASS_DRIVER_MINOR       CASS_VERSION_MINOR
        #define D_ENV_CASS_DRIVER_PATCH       CASS_VERSION_PATCH
        #define D_ENV_CASS_DRIVER_VERSION_ID                                   \
            D_ENV_CASS_ENCODE_VERSION(CASS_VERSION_MAJOR,                      \
                                       CASS_VERSION_MINOR,                     \
                                       CASS_VERSION_PATCH)
        #ifdef CASS_VERSION_SUFFIX
            #define D_ENV_CASS_DRIVER_VERSION_SUFFIX CASS_VERSION_SUFFIX
        #else
            #define D_ENV_CASS_DRIVER_VERSION_SUFFIX ""
        #endif  // CASS_VERSION_SUFFIX
    #else
        #define D_ENV_CASS_DRIVER_DETECTED    0
    #endif

#else
    // manual mode
    #ifdef D_ENV_CASS_DETECTED_DRIVER_VERSION
        #define D_ENV_CASS_DRIVER_DETECTED    1
        #define D_ENV_CASS_DRIVER_VERSION_ID                                   \
            D_ENV_CASS_DETECTED_DRIVER_VERSION
        #define D_ENV_CASS_DRIVER_MAJOR                                        \
            D_ENV_CASS_DECODE_MAJOR(D_ENV_CASS_DETECTED_DRIVER_VERSION)
        #define D_ENV_CASS_DRIVER_MINOR                                        \
            D_ENV_CASS_DECODE_MINOR(D_ENV_CASS_DETECTED_DRIVER_VERSION)
        #define D_ENV_CASS_DRIVER_PATCH                                        \
            D_ENV_CASS_DECODE_PATCH(D_ENV_CASS_DETECTED_DRIVER_VERSION)
        #define D_ENV_CASS_DRIVER_VERSION_SUFFIX "manual"
    #else
        #define D_ENV_CASS_DRIVER_DETECTED    0
    #endif  // D_ENV_CASS_DETECTED_DRIVER_VERSION

#endif  // D_CFG_ENV_CASS_CUSTOM


//==============================================================================
// 4.  TARGET SERVER VERSION DETECTION
//==============================================================================
// The server version is a runtime property. These macros allow compile-
// time gating against a known deployment target. Set manually via
// D_CFG_ENV_CASS_SERVER_VERSION or D_ENV_CASS_DETECTED_SERVER_*.


// 4.1    Target server version
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_CASS_SERVER_VERSION_ID
//   detection: the encoded target server version, from
// D_CFG_ENV_CASS_SERVER_VERSION, D_ENV_CASS_DETECTED_SERVER_VERSION, or the
// newest D_ENV_CASS_DETECTED_SERVER_<MAJOR>_<MINOR> defined, in that order;
// 0 when none is.
#ifndef D_ENV_CASS_SERVER_VERSION_ID
    #ifdef D_CFG_ENV_CASS_SERVER_VERSION
        #define D_ENV_CASS_SERVER_VERSION_ID                                   \
            D_CFG_ENV_CASS_SERVER_VERSION
    #elif defined(D_ENV_CASS_DETECTED_SERVER_VERSION)
        #define D_ENV_CASS_SERVER_VERSION_ID                                   \
            D_ENV_CASS_DETECTED_SERVER_VERSION
    #elif defined(D_ENV_CASS_DETECTED_SERVER_5_0)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_5_0
    #elif defined(D_ENV_CASS_DETECTED_SERVER_4_1)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_4_1
    #elif defined(D_ENV_CASS_DETECTED_SERVER_4_0)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_4_0
    #elif defined(D_ENV_CASS_DETECTED_SERVER_3_11)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_3_11
    #elif defined(D_ENV_CASS_DETECTED_SERVER_3_0)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_3_0
    #elif defined(D_ENV_CASS_DETECTED_SERVER_2_2)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_2_2
    #elif defined(D_ENV_CASS_DETECTED_SERVER_2_1)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_2_1
    #elif defined(D_ENV_CASS_DETECTED_SERVER_2_0)
        #define D_ENV_CASS_SERVER_VERSION_ID D_ENV_CASS_SERVER_2_0
    #else
        // no server version specified; default to 0 (unknown).
        // server-gated features will evaluate to 0.
        #define D_ENV_CASS_SERVER_VERSION_ID 0
    #endif  // D_CFG_ENV_CASS_SERVER_VERSION
#endif  // D_ENV_CASS_SERVER_VERSION_ID

// 4.1.2
// D_ENV_CASS_SERVER_MAJOR / D_ENV_CASS_SERVER_MINOR
//   macro: the major and minor parts of D_ENV_CASS_SERVER_VERSION_ID.
#define D_ENV_CASS_SERVER_MAJOR                                                \
    D_ENV_CASS_DECODE_MAJOR(D_ENV_CASS_SERVER_VERSION_ID)
#define D_ENV_CASS_SERVER_MINOR                                                \
    D_ENV_CASS_DECODE_MINOR(D_ENV_CASS_SERVER_VERSION_ID)

// 4.1.3
// D_ENV_CASS_SERVER_KNOWN
//   status: 1 if a target server version has been configured.
#define D_ENV_CASS_SERVER_KNOWN                                                \
    (D_ENV_CASS_SERVER_VERSION_ID > 0)

// 4.1.4
// D_ENV_CASS_DETECTED
//   detection: 1 if the driver is detected or a target server version is
// known.
#define D_ENV_CASS_DETECTED                                                    \
    ( (D_ENV_CASS_DRIVER_DETECTED) ||                                          \
      (D_ENV_CASS_SERVER_KNOWN) )


// sections 5 to 18 exist only when D_ENV_CASS_DETECTED is 1; see 4.1.4
#if D_ENV_CASS_DETECTED


//==============================================================================
// 5.  VERSION COMPARISON MACROS
//==============================================================================


// 5.1    Driver comparisons
//------------------------------------------------------------------------------
    // 5.1.1
    // D_ENV_CASS_DRIVER_AT_LEAST
    //   macro: 1 if the detected driver is at least major.minor.patch, and 0
    // when no driver is detected.
    #if D_ENV_CASS_DRIVER_DETECTED
        #define D_ENV_CASS_DRIVER_AT_LEAST(major, minor, patch)                \
            (D_ENV_CASS_DRIVER_VERSION_ID >=                                   \
                D_ENV_CASS_ENCODE_VERSION(major, minor, patch))
    #else
        #define D_ENV_CASS_DRIVER_AT_LEAST(major, minor, patch) 0
    #endif

// 5.2    Server comparisons
//------------------------------------------------------------------------------
    // 5.2.1
    // D_ENV_CASS_SERVER_AT_LEAST
    //   macro: 1 if the target server version is at least major.minor.patch.
    #define D_ENV_CASS_SERVER_AT_LEAST(major, minor, patch)                    \
        (D_ENV_CASS_SERVER_VERSION_ID >=                                       \
            D_ENV_CASS_ENCODE_VERSION(major, minor, patch))

    // 5.2.2
    // D_ENV_CASS_SERVER_BELOW
    //   macro: 1 if the target server version is below major.minor.patch.
    #define D_ENV_CASS_SERVER_BELOW(major, minor, patch)                       \
        (D_ENV_CASS_SERVER_VERSION_ID <                                        \
            D_ENV_CASS_ENCODE_VERSION(major, minor, patch))

    // 5.2.3
    // D_ENV_CASS_SERVER_IN_RANGE
    //   macro: 1 if the target server version is at least the first triple
    // and below the second.
    #define D_ENV_CASS_SERVER_IN_RANGE(min_maj, min_min, min_pat,              \
                                       max_maj, max_min, max_pat)              \
        ( (D_ENV_CASS_SERVER_AT_LEAST(min_maj, min_min, min_pat)) &&           \
          (D_ENV_CASS_SERVER_BELOW(max_maj, max_min, max_pat)) )

// 5.3    Release series
//------------------------------------------------------------------------------
    // 5.3.1
    // D_ENV_CASS_IS_<SERIES>
    //   macro: 1 if the target server version is in that release series, 2.0
    // to 5.0; D_ENV_CASS_IS_5_0 also covers every later release.
    #define D_ENV_CASS_IS_2_0                                                  \
        D_ENV_CASS_SERVER_IN_RANGE(2, 0, 0, 2, 1, 0)
    #define D_ENV_CASS_IS_2_1                                                  \
        D_ENV_CASS_SERVER_IN_RANGE(2, 1, 0, 2, 2, 0)
    #define D_ENV_CASS_IS_2_2                                                  \
        D_ENV_CASS_SERVER_IN_RANGE(2, 2, 0, 3, 0, 0)
    #define D_ENV_CASS_IS_3_0                                                  \
        D_ENV_CASS_SERVER_IN_RANGE(3, 0, 0, 3, 11, 0)
    #define D_ENV_CASS_IS_3_11                                                 \
        D_ENV_CASS_SERVER_IN_RANGE(3, 11, 0, 4, 0, 0)
    #define D_ENV_CASS_IS_4_0                                                  \
        D_ENV_CASS_SERVER_IN_RANGE(4, 0, 0, 4, 1, 0)
    #define D_ENV_CASS_IS_4_1                                                  \
        D_ENV_CASS_SERVER_IN_RANGE(4, 1, 0, 5, 0, 0)
    #define D_ENV_CASS_IS_5_0                                                  \
        D_ENV_CASS_SERVER_AT_LEAST(5, 0, 0)


//==============================================================================
// 6.  EDITION DETECTION
//==============================================================================
// Cassandra ships as open-source Apache Cassandra and as the commercial
// DataStax Enterprise (DSE) distribution. DSE adds integrated search (Solr),
// graph (Gremlin/TinkerPop), analytics (Spark), advanced security
// (LDAP/Kerberos, transparent data encryption, row-level access control),
// and in-memory tables. DSE uses its own version line, so it must be set
// manually via D_ENV_CASS_DETECTED_DSE.


// 6.1    Editions
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_CASS_IS_DSE
    //   detection: 1 if DataStax Enterprise is the target.
    #ifndef D_ENV_CASS_IS_DSE
        #if defined(D_ENV_CASS_DETECTED_DSE)
            #define D_ENV_CASS_IS_DSE 1
        #else
            #define D_ENV_CASS_IS_DSE 0
        #endif
    #endif  // D_ENV_CASS_IS_DSE

    // 6.1.2
    // D_ENV_CASS_IS_APACHE
    //   detection: 1 if open-source Apache Cassandra (not DSE).
    #define D_ENV_CASS_IS_APACHE                                               \
        (!D_ENV_CASS_IS_DSE)

    // 6.1.3
    // D_ENV_CASS_IS_ASTRA
    //   detection: 1 if targeting DataStax Astra (cloud-managed,
    // serverless Cassandra). Astra provides DSE-class features plus
    // cloud-specific services. Must be manually set.
    #ifndef D_ENV_CASS_IS_ASTRA
        #if defined(D_ENV_CASS_DETECTED_ASTRA)
            #define D_ENV_CASS_IS_ASTRA 1
        #else
            #define D_ENV_CASS_IS_ASTRA 0
        #endif
    #endif  // D_ENV_CASS_IS_ASTRA


//==============================================================================
// 7.  CLIENT DRIVER FEATURES
//==============================================================================


// 7.1    Driver features
//------------------------------------------------------------------------------
    // 7.1.1
    // D_ENV_CASS_HAS_DRIVER
    //   feature: detect if the DataStax C/C++ driver is available.
    #define D_ENV_CASS_HAS_DRIVER D_ENV_CASS_DRIVER_DETECTED

    // 7.1.2
    // D_ENV_CASS_HAS_ASYNC_API
    //   feature: future-based asynchronous API (cass_session_execute
    // returning CassFuture). Present in all 2.x+ driver releases.
    #define D_ENV_CASS_HAS_ASYNC_API                                           \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)

    // 7.1.3
    // D_ENV_CASS_HAS_CONNECTION_POOL
    //   feature: per-host connection pooling with configurable core and
    // max connections. Present in all modern driver versions.
    #define D_ENV_CASS_HAS_CONNECTION_POOL                                     \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)

    // 7.1.4
    // D_ENV_CASS_HAS_TOKEN_AWARE_ROUTING
    //   feature: token-aware load-balancing policy that routes queries
    // to replica nodes owning the partition. Available since 2.0.
    #define D_ENV_CASS_HAS_TOKEN_AWARE_ROUTING                                 \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)

    // 7.1.5
    // D_ENV_CASS_HAS_DC_AWARE_ROUTING
    //   feature: datacenter-aware round-robin load-balancing policy.
    #define D_ENV_CASS_HAS_DC_AWARE_ROUTING                                    \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)

    // 7.1.6
    // D_ENV_CASS_HAS_LATENCY_AWARE_ROUTING
    //   feature: latency-aware routing that penalizes slow hosts.
    #define D_ENV_CASS_HAS_LATENCY_AWARE_ROUTING                               \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)

    // 7.1.7
    // D_ENV_CASS_HAS_SPECULATIVE_EXECUTION
    //   feature: constant speculative execution policy (pre-emptively
    // retries on additional hosts). Available since driver 2.1.
    #define D_ENV_CASS_HAS_SPECULATIVE_EXECUTION                               \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 1, 0)

    // 7.1.8
    // D_ENV_CASS_HAS_RETRY_POLICY
    //   feature: configurable retry policies (default, fallthrough,
    // logging). Present in all modern driver versions.
    #define D_ENV_CASS_HAS_RETRY_POLICY                                        \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)

    // 7.1.9
    // D_ENV_CASS_HAS_DRIVER_METRICS
    //   feature: driver-level performance metrics (request latencies,
    // connection statistics). Available since driver 2.0.
    #define D_ENV_CASS_HAS_DRIVER_METRICS                                      \
        D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)


//==============================================================================
// 8.  CQL PROTOCOL AND STATEMENT FEATURES
//==============================================================================
// These features depend on the negotiated CQL binary protocol version,
// which in turn depends on both driver and server versions. Server-gated
// macros below evaluate to 0 unless a target server version is configured.


// 8.1    Protocol and statements
//------------------------------------------------------------------------------
    // 8.1.1
    // D_ENV_CASS_HAS_PREPARED_STATEMENTS
    //   feature: server-side prepared statements with bound parameters.
    // Core CQL feature.
    #define D_ENV_CASS_HAS_PREPARED_STATEMENTS                                 \
        D_ENV_CASS_SERVER_KNOWN

    // 8.1.2
    // D_ENV_CASS_HAS_BATCH
    //   feature: BATCH statements grouping multiple writes. Available
    // since Cassandra 2.0.
    #define D_ENV_CASS_HAS_BATCH                                               \
        D_ENV_CASS_SERVER_AT_LEAST(2, 0, 0)

    // 8.1.3
    // D_ENV_CASS_HAS_BATCH_UNLOGGED
    //   feature: UNLOGGED and COUNTER batch types in addition to the
    // default LOGGED batch. Available since 2.0.
    #define D_ENV_CASS_HAS_BATCH_UNLOGGED                                      \
        D_ENV_CASS_SERVER_AT_LEAST(2, 0, 0)

    // 8.1.4
    // D_ENV_CASS_HAS_PAGING
    //   feature: automatic result paging (cass_statement_set_paging_size).
    // Available since 2.0.
    #define D_ENV_CASS_HAS_PAGING                                              \
        D_ENV_CASS_SERVER_AT_LEAST(2, 0, 0)

    // 8.1.5
    // D_ENV_CASS_HAS_LWT
    //   feature: lightweight transactions / compare-and-set via Paxos
    // (IF NOT EXISTS, IF conditions). Introduced in Cassandra 2.0.
    #define D_ENV_CASS_HAS_LWT                                                 \
        D_ENV_CASS_SERVER_AT_LEAST(2, 0, 0)

    // 8.1.6
    // D_ENV_CASS_HAS_JSON
    //   feature: INSERT JSON / SELECT JSON and fromJson()/toJson()
    // functions. Introduced in Cassandra 2.2.
    #define D_ENV_CASS_HAS_JSON                                                \
        D_ENV_CASS_SERVER_AT_LEAST(2, 2, 0)

    // 8.1.7
    // D_ENV_CASS_HAS_PER_PARTITION_LIMIT
    //   feature: PER PARTITION LIMIT clause. Introduced in Cassandra 3.6.
    #define D_ENV_CASS_HAS_PER_PARTITION_LIMIT                                 \
        D_ENV_CASS_SERVER_AT_LEAST(3, 6, 0)

    // 8.1.8
    // D_ENV_CASS_HAS_GROUP_BY
    //   feature: GROUP BY aggregation in SELECT. Introduced in
    // Cassandra 3.10.
    #define D_ENV_CASS_HAS_GROUP_BY                                            \
        D_ENV_CASS_SERVER_AT_LEAST(3, 10, 0)

    // 8.1.9
    // D_ENV_CASS_HAS_VIRTUAL_TABLES
    //   feature: system virtual tables exposing metrics and settings.
    // Introduced in Cassandra 4.0.
    #define D_ENV_CASS_HAS_VIRTUAL_TABLES                                      \
        D_ENV_CASS_SERVER_AT_LEAST(4, 0, 0)


//==============================================================================
// 9.  DATA MODEL AND TYPE SYSTEM
//==============================================================================


// 9.1    Data model and types
//------------------------------------------------------------------------------
    // 9.1.1
    // D_ENV_CASS_HAS_COLLECTIONS
    //   feature: collection types (list, set, map). Available since 1.2,
    // so present whenever a server version is known.
    #define D_ENV_CASS_HAS_COLLECTIONS                                         \
        D_ENV_CASS_SERVER_KNOWN

    // 9.1.2
    // D_ENV_CASS_HAS_UDT
    //   feature: user-defined types (UDTs). Introduced in Cassandra 2.1.
    #define D_ENV_CASS_HAS_UDT                                                 \
        D_ENV_CASS_SERVER_AT_LEAST(2, 1, 0)

    // 9.1.3
    // D_ENV_CASS_HAS_TUPLES
    //   feature: tuple types. Introduced in Cassandra 2.1.
    #define D_ENV_CASS_HAS_TUPLES                                              \
        D_ENV_CASS_SERVER_AT_LEAST(2, 1, 0)

    // 9.1.4
    // D_ENV_CASS_HAS_FROZEN_COLLECTIONS
    //   feature: frozen<> collections and nested collections.
    // Introduced in Cassandra 2.1.
    #define D_ENV_CASS_HAS_FROZEN_COLLECTIONS                                  \
        D_ENV_CASS_SERVER_AT_LEAST(2, 1, 0)

    // 9.1.5
    // D_ENV_CASS_HAS_STATIC_COLUMNS
    //   feature: static columns shared across rows of a partition.
    // Introduced in Cassandra 2.0.6.
    #define D_ENV_CASS_HAS_STATIC_COLUMNS                                      \
        D_ENV_CASS_SERVER_AT_LEAST(2, 0, 6)

    // 9.1.6
    // D_ENV_CASS_HAS_COUNTERS
    //   feature: counter columns for distributed increment/decrement.
    // Core feature.
    #define D_ENV_CASS_HAS_COUNTERS                                            \
        D_ENV_CASS_SERVER_KNOWN

    // 9.1.7
    // D_ENV_CASS_HAS_DURATION_TYPE
    //   feature: the duration data type. Introduced in Cassandra 3.10.
    #define D_ENV_CASS_HAS_DURATION_TYPE                                       \
        D_ENV_CASS_SERVER_AT_LEAST(3, 10, 0)

    // 9.1.8
    // D_ENV_CASS_HAS_TTL
    //   feature: per-column / per-row time-to-live expiry. Core feature.
    #define D_ENV_CASS_HAS_TTL                                                 \
        D_ENV_CASS_SERVER_KNOWN


//==============================================================================
// 10.  INDEXING AND VIEWS
//==============================================================================


// 10.1   Indexes and views
//------------------------------------------------------------------------------
    // 10.1.1
    // D_ENV_CASS_HAS_SECONDARY_INDEX
    //   feature: standard secondary indexes (2i). Core feature.
    #define D_ENV_CASS_HAS_SECONDARY_INDEX                                     \
        D_ENV_CASS_SERVER_KNOWN

    // 10.1.2
    // D_ENV_CASS_HAS_SASI
    //   feature: SSTable-Attached Secondary Index (SASI), supporting
    // LIKE / range queries. Introduced in Cassandra 3.4 (experimental).
    #define D_ENV_CASS_HAS_SASI                                                \
        D_ENV_CASS_SERVER_AT_LEAST(3, 4, 0)

    // 10.1.3
    // D_ENV_CASS_HAS_SAI
    //   feature: Storage-Attached Indexing (SAI), the modern secondary
    // index. Introduced in Cassandra 5.0 (available earlier in DSE).
    #define D_ENV_CASS_HAS_SAI                                                 \
        ( (D_ENV_CASS_SERVER_AT_LEAST(5, 0, 0)) ||                             \
          (D_ENV_CASS_IS_DSE) )

    // 10.1.4
    // D_ENV_CASS_HAS_MATERIALIZED_VIEWS
    //   feature: materialized views automatically maintained from a base
    // table. Introduced in Cassandra 3.0 (marked experimental in 3.x+).
    #define D_ENV_CASS_HAS_MATERIALIZED_VIEWS                                  \
        D_ENV_CASS_SERVER_AT_LEAST(3, 0, 0)

    // 10.1.5
    // D_ENV_CASS_HAS_VECTOR_SEARCH
    //   feature: vector data type and ANN (approximate nearest neighbor)
    // search for embeddings. Introduced in Cassandra 5.0.
    #define D_ENV_CASS_HAS_VECTOR_SEARCH                                       \
        ( (D_ENV_CASS_SERVER_AT_LEAST(5, 0, 0)) ||                             \
          (D_ENV_CASS_IS_ASTRA) )


//==============================================================================
// 11.  USER-DEFINED FUNCTIONS AND AGGREGATES
//==============================================================================


// 11.1   Functions and aggregates
//------------------------------------------------------------------------------
    // 11.1.1
    // D_ENV_CASS_HAS_UDF
    //   feature: user-defined functions (UDFs). Introduced in
    // Cassandra 2.2.
    #define D_ENV_CASS_HAS_UDF                                                 \
        D_ENV_CASS_SERVER_AT_LEAST(2, 2, 0)

    // 11.1.2
    // D_ENV_CASS_HAS_UDA
    //   feature: user-defined aggregates (UDAs). Introduced in
    // Cassandra 2.2.
    #define D_ENV_CASS_HAS_UDA                                                 \
        D_ENV_CASS_SERVER_AT_LEAST(2, 2, 0)


//==============================================================================
// 12.  STORAGE ENGINE AND COMPACTION
//==============================================================================
// Cassandra uses a log-structured merge-tree (LSM) storage engine:
// writes go to a commit log and an in-memory memtable, then flush to
// immutable SSTables that are periodically compacted.


// 12.1   Storage and compaction
//------------------------------------------------------------------------------
    // 12.1.1
    // D_ENV_CASS_HAS_LSM_ENGINE
    //   feature: LSM-tree storage engine with SSTables. Core
    // architecture, present whenever a server version is known.
    #define D_ENV_CASS_HAS_LSM_ENGINE                                          \
        D_ENV_CASS_SERVER_KNOWN

    // 12.1.2
    // D_ENV_CASS_HAS_STCS
    //   feature: SizeTieredCompactionStrategy. Default; always present.
    #define D_ENV_CASS_HAS_STCS                                                \
        D_ENV_CASS_SERVER_KNOWN

    // 12.1.3
    // D_ENV_CASS_HAS_LCS
    //   feature: LeveledCompactionStrategy. Available since 1.0.
    #define D_ENV_CASS_HAS_LCS                                                 \
        D_ENV_CASS_SERVER_KNOWN

    // 12.1.4
    // D_ENV_CASS_HAS_TWCS
    //   feature: TimeWindowCompactionStrategy, optimized for time-series
    // and TTL data. Introduced in Cassandra 3.0.8 / 3.8.
    #define D_ENV_CASS_HAS_TWCS                                                \
        D_ENV_CASS_SERVER_AT_LEAST(3, 8, 0)

    // 12.1.5
    // D_ENV_CASS_HAS_UCS
    //   feature: UnifiedCompactionStrategy. Introduced in Cassandra 5.0.
    #define D_ENV_CASS_HAS_UCS                                                 \
        D_ENV_CASS_SERVER_AT_LEAST(5, 0, 0)

    // 12.1.6
    // D_ENV_CASS_HAS_INCREMENTAL_REPAIR
    //   feature: incremental anti-entropy repair. Introduced in 2.1,
    // substantially reworked (safe by default) in 4.0.
    #define D_ENV_CASS_HAS_INCREMENTAL_REPAIR                                  \
        D_ENV_CASS_SERVER_AT_LEAST(2, 1, 0)

    // 12.1.7
    // D_ENV_CASS_HAS_CDC
    //   feature: change data capture (per-table CDC commit-log capture).
    // Introduced in Cassandra 3.8.
    #define D_ENV_CASS_HAS_CDC                                                 \
        D_ENV_CASS_SERVER_AT_LEAST(3, 8, 0)


//==============================================================================
// 13.  REPLICATION, CONSISTENCY, AND DISTRIBUTION
//==============================================================================


// 13.1   Replication and consistency
//------------------------------------------------------------------------------
    // 13.1.1
    // D_ENV_CASS_HAS_TUNABLE_CONSISTENCY
    //   feature: per-query tunable consistency levels (ONE, QUORUM,
    // LOCAL_QUORUM, ALL, etc.). Core feature.
    #define D_ENV_CASS_HAS_TUNABLE_CONSISTENCY                                 \
        D_ENV_CASS_SERVER_KNOWN

    // 13.1.2
    // D_ENV_CASS_HAS_NETWORK_TOPOLOGY_STRATEGY
    //   feature: NetworkTopologyStrategy for multi-datacenter
    // replication. Core feature.
    #define D_ENV_CASS_HAS_NETWORK_TOPOLOGY_STRATEGY                           \
        D_ENV_CASS_SERVER_KNOWN

    // 13.1.3
    // D_ENV_CASS_HAS_VNODES
    //   feature: virtual nodes (vnodes) for token distribution.
    // Introduced in Cassandra 1.2; default thereafter.
    #define D_ENV_CASS_HAS_VNODES                                              \
        D_ENV_CASS_SERVER_KNOWN

    // 13.1.4
    // D_ENV_CASS_HAS_HINTED_HANDOFF
    //   feature: hinted handoff for transient node failures. Core
    // feature.
    #define D_ENV_CASS_HAS_HINTED_HANDOFF                                      \
        D_ENV_CASS_SERVER_KNOWN

    // 13.1.5
    // D_ENV_CASS_HAS_LOCAL_CONSISTENCY
    //   feature: datacenter-local consistency levels (LOCAL_ONE,
    // LOCAL_QUORUM, LOCAL_SERIAL). LOCAL_ONE added in 1.2.10 / 2.0.2.
    #define D_ENV_CASS_HAS_LOCAL_CONSISTENCY                                   \
        D_ENV_CASS_SERVER_AT_LEAST(2, 0, 2)

    // 13.1.6
    // D_ENV_CASS_HAS_TRANSIENT_REPLICATION
    //   feature: transient replication (experimental). Introduced in
    // Cassandra 4.0.
    #define D_ENV_CASS_HAS_TRANSIENT_REPLICATION                               \
        D_ENV_CASS_SERVER_AT_LEAST(4, 0, 0)


//==============================================================================
// 14.  SECURITY
//==============================================================================


// 14.1   Authentication, authorization and encryption
//------------------------------------------------------------------------------
    // 14.1.1
    // D_ENV_CASS_HAS_PASSWORD_AUTH
    //   feature: PasswordAuthenticator (internal username/password
    // authentication). Core feature.
    #define D_ENV_CASS_HAS_PASSWORD_AUTH                                       \
        D_ENV_CASS_SERVER_KNOWN

    // 14.1.2
    // D_ENV_CASS_HAS_RBAC
    //   feature: role-based access control (CREATE ROLE / GRANT).
    // Introduced in Cassandra 2.2 (replacing user-based permissions).
    #define D_ENV_CASS_HAS_RBAC                                                \
        D_ENV_CASS_SERVER_AT_LEAST(2, 2, 0)

    // 14.1.3
    // D_ENV_CASS_HAS_SSL
    //   feature: client-to-node and node-to-node TLS/SSL encryption.
    // Core feature; driver provides cass_ssl_* APIs.
    #define D_ENV_CASS_HAS_SSL                                                 \
        D_ENV_CASS_SERVER_KNOWN

    // 14.1.4
    // D_ENV_CASS_HAS_AUTH_LDAP
    //   feature: LDAP authentication. DSE only.
    #define D_ENV_CASS_HAS_AUTH_LDAP                                           \
        D_ENV_CASS_IS_DSE

    // 14.1.5
    // D_ENV_CASS_HAS_AUTH_KERBEROS
    //   feature: Kerberos (GSSAPI) authentication. DSE only.
    #define D_ENV_CASS_HAS_AUTH_KERBEROS                                       \
        D_ENV_CASS_IS_DSE

    // 14.1.6
    // D_ENV_CASS_HAS_TDE
    //   feature: transparent data encryption (encryption at rest).
    // DSE only.
    #define D_ENV_CASS_HAS_TDE                                                 \
        D_ENV_CASS_IS_DSE

    // 14.1.7
    // D_ENV_CASS_HAS_ROW_LEVEL_ACCESS
    //   feature: row-level access control. DSE only.
    #define D_ENV_CASS_HAS_ROW_LEVEL_ACCESS                                    \
        D_ENV_CASS_IS_DSE

    // 14.1.8
    // D_ENV_CASS_HAS_AUDIT_LOG
    //   feature: full query audit logging. Introduced in Apache
    // Cassandra 4.0 (DSE has had auditing earlier).
    #define D_ENV_CASS_HAS_AUDIT_LOG                                           \
        ( (D_ENV_CASS_SERVER_AT_LEAST(4, 0, 0)) ||                             \
          (D_ENV_CASS_IS_DSE) )


//==============================================================================
// 15.  DSE-SPECIFIC FEATURES
//==============================================================================
// These features are only available on DataStax Enterprise. They require
// D_ENV_CASS_IS_DSE to be set.


// 15.1   DSE features
//------------------------------------------------------------------------------
    // 15.1.1
    // D_ENV_CASS_HAS_DSE_SEARCH
    //   feature: integrated search (Apache Solr / Lucene) over Cassandra
    // tables. DSE only.
    #define D_ENV_CASS_HAS_DSE_SEARCH                                          \
        D_ENV_CASS_IS_DSE

    // 15.1.2
    // D_ENV_CASS_HAS_DSE_GRAPH
    //   feature: integrated graph database (Gremlin / TinkerPop). DSE
    // only.
    #define D_ENV_CASS_HAS_DSE_GRAPH                                           \
        D_ENV_CASS_IS_DSE

    // 15.1.3
    // D_ENV_CASS_HAS_DSE_ANALYTICS
    //   feature: integrated analytics (Apache Spark) over Cassandra. DSE
    // only.
    #define D_ENV_CASS_HAS_DSE_ANALYTICS                                       \
        D_ENV_CASS_IS_DSE

    // 15.1.4
    // D_ENV_CASS_HAS_DSE_IN_MEMORY
    //   feature: in-memory tables. DSE only.
    #define D_ENV_CASS_HAS_DSE_IN_MEMORY                                       \
        D_ENV_CASS_IS_DSE


//==============================================================================
// 16.  COMPOSITE CHECKS
//==============================================================================


// 16.1   Composite checks
//------------------------------------------------------------------------------
    // 16.1.1
    // D_ENV_CASS_HAS_ADVANCED_TYPES
    //   macro: evaluates to 1 if UDTs, tuples, and frozen collections
    // are all available (Cassandra 2.1+).
    #define D_ENV_CASS_HAS_ADVANCED_TYPES                                      \
        ( (D_ENV_CASS_HAS_UDT)    &&                                           \
          (D_ENV_CASS_HAS_TUPLES) &&                                           \
          (D_ENV_CASS_HAS_FROZEN_COLLECTIONS) )

    // 16.1.2
    // D_ENV_CASS_HAS_USER_LOGIC
    //   macro: evaluates to 1 if both user-defined functions and
    // aggregates are available (Cassandra 2.2+).
    #define D_ENV_CASS_HAS_USER_LOGIC                                          \
        ( (D_ENV_CASS_HAS_UDF) &&                                              \
          (D_ENV_CASS_HAS_UDA) )

    // 16.1.3
    // D_ENV_CASS_HAS_MODERN_INDEXING
    //   macro: evaluates to 1 if storage-attached indexing and vector
    // search are available (Cassandra 5.0+ / Astra).
    #define D_ENV_CASS_HAS_MODERN_INDEXING                                     \
        ( (D_ENV_CASS_HAS_SAI) &&                                              \
          (D_ENV_CASS_HAS_VECTOR_SEARCH) )

    // 16.1.4
    // D_ENV_CASS_HAS_MODERN_SECURITY
    //   macro: evaluates to 1 if RBAC, SSL, and audit logging are all
    // available.
    #define D_ENV_CASS_HAS_MODERN_SECURITY                                     \
        ( (D_ENV_CASS_HAS_RBAC) &&                                             \
          (D_ENV_CASS_HAS_SSL)  &&                                             \
          (D_ENV_CASS_HAS_AUDIT_LOG) )

    // 16.1.5
    // D_ENV_CASS_HAS_SMART_ROUTING
    //   macro: evaluates to 1 if the driver supports token-aware routing
    // together with speculative execution.
    #define D_ENV_CASS_HAS_SMART_ROUTING                                       \
        ( (D_ENV_CASS_HAS_TOKEN_AWARE_ROUTING) &&                              \
          (D_ENV_CASS_HAS_SPECULATIVE_EXECUTION) )

    // 16.1.6
    // D_ENV_CASS_IS_FULLY_MODERN
    //   macro: evaluates to 1 if the server has a comprehensive modern
    // feature set (roughly Cassandra 5.0+ with modern indexing, user
    // logic, change data capture, virtual tables, and modern security).
    #define D_ENV_CASS_IS_FULLY_MODERN                                         \
        ( (D_ENV_CASS_HAS_MODERN_INDEXING) &&                                  \
          (D_ENV_CASS_HAS_USER_LOGIC)      &&                                  \
          (D_ENV_CASS_HAS_CDC)             &&                                  \
          (D_ENV_CASS_HAS_VIRTUAL_TABLES)  &&                                  \
          (D_ENV_CASS_HAS_MODERN_SECURITY) )


//==============================================================================
// 17.  DEPRECATION AND REMOVAL
//==============================================================================


// 17.1   Deprecations
//------------------------------------------------------------------------------
    // 17.1.1
    // D_ENV_CASS_REMOVED_THRIFT
    //   status: 1 if the legacy Thrift RPC interface is removed. Thrift
    // was deprecated in 2.1, disabled by default in 3.0, and removed in
    // Cassandra 4.0.
    #define D_ENV_CASS_REMOVED_THRIFT                                          \
        D_ENV_CASS_SERVER_AT_LEAST(4, 0, 0)

    // 17.1.2
    // D_ENV_CASS_REMOVED_COMPACT_STORAGE
    //   status: 1 if COMPACT STORAGE tables are removed. Dropping
    // COMPACT STORAGE became supported in 3.0; legacy compact tables are
    // no longer creatable in 4.0+.
    #define D_ENV_CASS_REMOVED_COMPACT_STORAGE                                 \
        D_ENV_CASS_SERVER_AT_LEAST(4, 0, 0)

    // 17.1.3
    // D_ENV_CASS_DEPRECATED_SASI
    //   status: 1 if SASI indexes are considered superseded by SAI
    // (Cassandra 5.0+).
    #define D_ENV_CASS_DEPRECATED_SASI                                         \
        D_ENV_CASS_SERVER_AT_LEAST(5, 0, 0)


//==============================================================================
// 18.  CONSUMER COMPATIBILITY LAYER
//==============================================================================
// The djinterp Cassandra connection layer (cassandra.hpp) consumes a stable,
// flattened vocabulary prefixed D_ENV_CASSANDRA_*. This section publishes it
// in terms of the detection model above, so that the connection layer needs
// no knowledge of the driver and server split. Feature availability follows
// the target server version, so D_ENV_CASSANDRA_VERSION_* mirror the server
// version when it is known, and the driver's otherwise.


// 18.1   Consumer vocabulary
//------------------------------------------------------------------------------
    // 18.1.1
    // D_ENV_CASSANDRA_*
    //   macro: D_ENV_CASSANDRA_DETECTED; the version as _VERSION_ID, _MAJOR,
    // _MINOR, _PATCH and _STRING (the server's when known, otherwise the
    // driver's); and D_ENV_CASSANDRA_HAS_* capability flags, each defined in
    // terms of the macros above. Pre-defining D_ENV_CASSANDRA_DETECTED skips
    // the whole layer.
    #ifndef D_ENV_CASSANDRA_DETECTED
        #define D_ENV_CASSANDRA_DETECTED 1

        // version (prefer server; fall back to driver)
        #if D_ENV_CASS_SERVER_KNOWN
            #define D_ENV_CASSANDRA_VERSION_ID    D_ENV_CASS_SERVER_VERSION_ID
            #define D_ENV_CASSANDRA_VERSION_MAJOR D_ENV_CASS_SERVER_MAJOR
            #define D_ENV_CASSANDRA_VERSION_MINOR D_ENV_CASS_SERVER_MINOR
            #define D_ENV_CASSANDRA_VERSION_PATCH                              \
                D_ENV_CASS_DECODE_PATCH(D_ENV_CASS_SERVER_VERSION_ID)
        #elif D_ENV_CASS_DRIVER_DETECTED
            #define D_ENV_CASSANDRA_VERSION_ID    D_ENV_CASS_DRIVER_VERSION_ID
            #define D_ENV_CASSANDRA_VERSION_MAJOR D_ENV_CASS_DRIVER_MAJOR
            #define D_ENV_CASSANDRA_VERSION_MINOR D_ENV_CASS_DRIVER_MINOR
            #define D_ENV_CASSANDRA_VERSION_PATCH D_ENV_CASS_DRIVER_PATCH
        #else
            #define D_ENV_CASSANDRA_VERSION_ID    0
            #define D_ENV_CASSANDRA_VERSION_MAJOR 0
            #define D_ENV_CASSANDRA_VERSION_MINOR 0
            #define D_ENV_CASSANDRA_VERSION_PATCH 0
        #endif

        // version string: prefer the driver's reported suffix when present;
        // otherwise expose the encoded id as an unknown-build placeholder.
        #if D_ENV_CASS_DRIVER_DETECTED
            #define D_ENV_CASSANDRA_VERSION_STRING                             \
                D_ENV_CASS_DRIVER_VERSION_SUFFIX
        #else
            #define D_ENV_CASSANDRA_VERSION_STRING "unknown"
        #endif

        // CQL binary protocol versions (derived from server version)
        //   v3 since 2.0, v4 since 2.2, v5 since 4.0. When only the driver
        // is known, assume the driver negotiates the protocols it supports
        // (modern drivers support through v4, and v5 from cpp-driver 2.16).
        #if D_ENV_CASS_SERVER_KNOWN
            #define D_ENV_CASSANDRA_HAS_PROTOCOL_V3                            \
                D_ENV_CASS_SERVER_AT_LEAST(2, 0, 0)
            #define D_ENV_CASSANDRA_HAS_PROTOCOL_V4                            \
                D_ENV_CASS_SERVER_AT_LEAST(2, 2, 0)
            #define D_ENV_CASSANDRA_HAS_PROTOCOL_V5                            \
                D_ENV_CASS_SERVER_AT_LEAST(4, 0, 0)
        #else
            #define D_ENV_CASSANDRA_HAS_PROTOCOL_V3                            \
                D_ENV_CASS_DRIVER_AT_LEAST(2, 0, 0)
            #define D_ENV_CASSANDRA_HAS_PROTOCOL_V4                            \
                D_ENV_CASS_DRIVER_AT_LEAST(2, 1, 0)
            #define D_ENV_CASSANDRA_HAS_PROTOCOL_V5                            \
                D_ENV_CASS_DRIVER_AT_LEAST(2, 16, 0)
        #endif

        // data model / type features
        #define D_ENV_CASSANDRA_HAS_UDT       D_ENV_CASS_HAS_UDT
        #define D_ENV_CASSANDRA_HAS_COUNTERS  D_ENV_CASS_HAS_COUNTERS
        #define D_ENV_CASSANDRA_HAS_DURATION  D_ENV_CASS_HAS_DURATION_TYPE
        #define D_ENV_CASSANDRA_HAS_VECTOR    D_ENV_CASS_HAS_VECTOR_SEARCH

        // query / execution features
        #define D_ENV_CASSANDRA_HAS_LWT                                        \
            D_ENV_CASS_HAS_LWT
        #define D_ENV_CASSANDRA_HAS_MATERIALIZED_VIEWS                         \
            D_ENV_CASS_HAS_MATERIALIZED_VIEWS
        #define D_ENV_CASSANDRA_HAS_SASI_INDEXES                               \
            D_ENV_CASS_HAS_SASI
        #define D_ENV_CASSANDRA_HAS_UDF                                        \
            D_ENV_CASS_HAS_UDF
        #define D_ENV_CASSANDRA_HAS_UDA                                        \
            D_ENV_CASS_HAS_UDA
        #define D_ENV_CASSANDRA_HAS_VIRTUAL_TABLES                             \
            D_ENV_CASS_HAS_VIRTUAL_TABLES

        // driver routing / execution policies
        #define D_ENV_CASSANDRA_HAS_TOKEN_AWARE                                \
            D_ENV_CASS_HAS_TOKEN_AWARE_ROUTING
        #define D_ENV_CASSANDRA_HAS_SPECULATIVE_EXECUTION                      \
            D_ENV_CASS_HAS_SPECULATIVE_EXECUTION

        // transport / compression
        //   the CQL binary protocol has supported frame compression (LZ4 /
        // Snappy) since the earliest framed protocol; treat as available
        // whenever the environment is detected.
        #define D_ENV_CASSANDRA_HAS_COMPRESSION 1
        #define D_ENV_CASSANDRA_HAS_TLS                                        \
            D_ENV_CASS_HAS_SSL

        // security
        #define D_ENV_CASSANDRA_HAS_AUTH                                       \
            D_ENV_CASS_HAS_PASSWORD_AUTH
        #define D_ENV_CASSANDRA_HAS_RBAC                                       \
            D_ENV_CASS_HAS_RBAC
        #define D_ENV_CASSANDRA_HAS_AUDIT_LOGGING                              \
            D_ENV_CASS_HAS_AUDIT_LOG

    #endif  // D_ENV_CASSANDRA_DETECTED (compatibility layer)


#else  // !D_ENV_CASS_DETECTED

    // environment not detected: publish a defined-zero DETECTED flag so the
    // consumer layer can test it without relying on implicit 0-expansion.
    #ifndef D_ENV_CASSANDRA_DETECTED
        #define D_ENV_CASSANDRA_DETECTED 0
    #endif  // D_ENV_CASSANDRA_DETECTED

#endif  // D_ENV_CASS_DETECTED


#endif  // DJINTERP_ENV_DB_CASSANDRA_ENV_CASSANDRA_H
