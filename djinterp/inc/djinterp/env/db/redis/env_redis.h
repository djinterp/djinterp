/*******************************************************************************
* djinterp [env]                                                     env_redis.h
*
* djinterp Redis environment detection.
*   Compile-time detection of a Redis environment: the hiredis client and its
* version, the target server version, the distribution (Redis OSS, Redis
* Stack, Redis Enterprise, Redis Cloud, or the Valkey fork), and the
* capabilities those gate: the RESP protocol, client features, core data
* structures, commands and execution, persistence, replication and high
* availability, memory and threading, security, and modules.
*   Two versions are tracked. D_ENV_REDIS_CLIENT_* is hiredis's, read at
* compile time. D_ENV_REDIS_SERVER_* is the target server's, a runtime property
* that must be configured, through D_CFG_ENV_REDIS_SERVER_VERSION or a
* pre-defined D_ENV_REDIS_DETECTED_SERVER_*; without one, server-gated features
* read 0. Both are encoded as MAJOR*10000 + MINOR*100 + PATCH. Distributions
* other than OSS are manual too, through D_ENV_REDIS_DETECTED_VALKEY,
* _ENTERPRISE, _CLOUD and _STACK.
*   D_ENV_REDIS_HAS_* are capability flags, and D_ENV_REDIS_IS_* the
* distribution and release series. Sections 5 onward exist only when a client
* or a server version is known; the last of them publishes the extra names the
* connection layer uses.
*   Settings live in cfg_env_redis.h: D_CFG_ENV_USING_REDIS includes the
* hiredis headers, and D_CFG_ENV_REDIS_CUSTOM switches the client to manual
* detection. This header also includes env_db.h, for the base database
* detection.
*
*
* path:      /inc/djinterp/env/db/redis/env_redis.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION
    -----------------------
    1.  Client headers
         1.  D_ENV_REDIS_HEADER_INCLUDED
         2.  D_ENV_REDIS_SSL_HEADER_INCLUDED
2.  VERSION ENCODING
    ----------------
    1.  Encoding and decoding
         1.  D_ENV_REDIS_ENCODE_VERSION
         2.  D_ENV_REDIS_DECODE_MAJOR
         3.  D_ENV_REDIS_DECODE_MINOR
         4.  D_ENV_REDIS_DECODE_PATCH
3.  CLIENT VERSION DETECTION
    ------------------------
    1.  Server release IDs
         1.  D_ENV_REDIS_SERVER_<MAJOR>_<MINOR>
    2.  Detected client
         1.  D_ENV_REDIS_CLIENT_DETECTED / D_ENV_REDIS_CLIENT_*
4.  TARGET SERVER VERSION DETECTION
    -------------------------------
    1.  Target server version
         1.  D_ENV_REDIS_SERVER_VERSION_ID
         2.  D_ENV_REDIS_SERVER_MAJOR / D_ENV_REDIS_SERVER_MINOR
         3.  D_ENV_REDIS_SERVER_KNOWN
         4.  D_ENV_REDIS_DETECTED
5.  VERSION COMPARISON MACROS
    -------------------------
    1.  Client comparisons
         1.  D_ENV_REDIS_CLIENT_AT_LEAST
    2.  Server comparisons
         1.  D_ENV_REDIS_SERVER_AT_LEAST
         2.  D_ENV_REDIS_SERVER_BELOW
         3.  D_ENV_REDIS_SERVER_IN_RANGE
    3.  Release series
         1.  D_ENV_REDIS_IS_<SERIES>
6.  DISTRIBUTION DETECTION
    ----------------------
    1.  Distributions
         1.  D_ENV_REDIS_IS_VALKEY
         2.  D_ENV_REDIS_IS_ENTERPRISE
         3.  D_ENV_REDIS_IS_CLOUD
         4.  D_ENV_REDIS_IS_STACK
         5.  D_ENV_REDIS_IS_OSS
         6.  D_ENV_REDIS_IS_MANAGED_COMMERCIAL
7.  RESP PROTOCOL DETECTION
    -----------------------
    1.  Protocol
         1.  D_ENV_REDIS_HAS_RESP3
         2.  D_ENV_REDIS_CLIENT_HAS_RESP3
8.  CLIENT LIBRARY FEATURES (HIREDIS)
    ---------------------------------
    1.  Client features
         1.  D_ENV_REDIS_HAS_HIREDIS
         2.  D_ENV_REDIS_HAS_CLIENT_SSL
         3.  D_ENV_REDIS_HAS_CLIENT_ASYNC
         4.  D_ENV_REDIS_HAS_CLIENT_REUSEADDR
9.  CORE DATA STRUCTURES
    --------------------
    1.  Data structures
         1.  Foundational data-type flags
         2.  D_ENV_REDIS_HAS_BITMAPS
         3.  D_ENV_REDIS_HAS_BITFIELD
         4.  D_ENV_REDIS_HAS_HYPERLOGLOG
         5.  D_ENV_REDIS_HAS_GEO
         6.  D_ENV_REDIS_HAS_STREAMS
         7.  D_ENV_REDIS_HAS_HASH_FIELD_TTL
10. COMMAND-GROUP AND EXECUTION FEATURES
    ------------------------------------
    1.  Commands and execution
         1.  D_ENV_REDIS_HAS_TRANSACTIONS
         2.  D_ENV_REDIS_HAS_PUBSUB
         3.  D_ENV_REDIS_HAS_SHARDED_PUBSUB
         4.  D_ENV_REDIS_HAS_SCRIPTING
         5.  D_ENV_REDIS_HAS_FUNCTIONS
         6.  D_ENV_REDIS_HAS_KEYSPACE_NOTIFICATIONS
         7.  D_ENV_REDIS_HAS_CLIENT_SIDE_CACHING
         8.  D_ENV_REDIS_HAS_EXPIRE_OPTIONS
11. PERSISTENCE
    -----------
    1.  Persistence
         1.  D_ENV_REDIS_HAS_RDB
         2.  D_ENV_REDIS_HAS_AOF
         3.  D_ENV_REDIS_HAS_HYBRID_PERSISTENCE
         4.  D_ENV_REDIS_HAS_MULTI_PART_AOF
12. REPLICATION AND HIGH AVAILABILITY
    ---------------------------------
    1.  Replication and high availability
         1.  D_ENV_REDIS_HAS_REPLICATION
         2.  D_ENV_REDIS_HAS_SENTINEL
         3.  D_ENV_REDIS_HAS_CLUSTER
         4.  D_ENV_REDIS_HAS_WAIT
         5.  D_ENV_REDIS_HAS_WAITAOF
         6.  D_ENV_REDIS_HAS_DISKLESS_REPLICATION
         7.  D_ENV_REDIS_HAS_ACTIVE_ACTIVE
13. MEMORY MANAGEMENT AND THREADING
    -------------------------------
    1.  Memory and threading
         1.  D_ENV_REDIS_HAS_LRU_EVICTION
         2.  D_ENV_REDIS_HAS_LFU_EVICTION
         3.  D_ENV_REDIS_HAS_LAZY_FREE
         4.  D_ENV_REDIS_HAS_THREADED_IO
         5.  D_ENV_REDIS_HAS_TIERED_STORAGE
14. SECURITY
    --------
    1.  Security
         1.  D_ENV_REDIS_HAS_AUTH
         2.  D_ENV_REDIS_HAS_ACL
         3.  D_ENV_REDIS_HAS_ACL_SELECTORS
         4.  D_ENV_REDIS_HAS_TLS
15. MODULES
    -------
    1.  Modules
         1.  D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED
         2.  D_ENV_REDIS_HAS_SEARCH
         3.  D_ENV_REDIS_HAS_JSON
         4.  D_ENV_REDIS_HAS_TIME_SERIES
         5.  D_ENV_REDIS_HAS_PROBABILISTIC
         6.  D_ENV_REDIS_HAS_VECTOR_SEARCH
         7.  D_ENV_REDIS_HAS_VECTOR_SETS
16. COMPOSITE CHECKS
    ----------------
    1.  Composite checks
         1.  D_ENV_REDIS_HAS_PROGRAMMABILITY
         2.  D_ENV_REDIS_HAS_MODERN_SECURITY
         3.  D_ENV_REDIS_HAS_MODERN_HA
         4.  D_ENV_REDIS_HAS_DATA_PLATFORM
         5.  D_ENV_REDIS_IS_FULLY_MODERN
17. DEPRECATION AND REMOVAL
    -----------------------
    1.  Deprecations
         1.  D_ENV_REDIS_DEPRECATED_SLAVE_TERMINOLOGY
         2.  D_ENV_REDIS_DEPRECATED_GEORADIUS
         3.  D_ENV_REDIS_LICENSE_IS_OSI
18. CONSUMER COMPATIBILITY LAYER
    ----------------------------
    1.  Consumer vocabulary
         1.  D_ENV_REDIS_VERSION_*
         2.  Consumer feature aliases
*/

#ifndef DJINTERP_ENV_DB_REDIS_ENV_REDIS_H
#define DJINTERP_ENV_DB_REDIS_ENV_REDIS_H 1

// djinterp
#include "../../../config/core/env/db/redis/cfg_env_redis.h"  // D_CFG_ENV_*
#include "../env_db.h"  // base database detection (D_ENV_DB_*)


//==============================================================================
// 1.  VENDOR HEADER INCLUSION
//==============================================================================
// Driven by D_CFG_ENV_USING_REDIS, from cfg_env_redis.h. When on, this
// section includes the hiredis client header and, when present, its SSL
// header; hiredis is a C API usable from C and C++ alike. Detection below is
// gated on D_ENV_REDIS_HEADER_INCLUDED, so that no hiredis symbol is
// referenced unless the header is in scope.


// 1.1    Client headers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_REDIS_HEADER_INCLUDED
//   detection: 1 once the hiredis header is included, and 0 when
// D_CFG_ENV_USING_REDIS is off. D_CFG_ENV_REDIS_C_PATH is tried first, then
// <hiredis/hiredis.h> and <hiredis.h>; without __has_include, the configured
// path is included as is, and with none found, #error.
// D_ENV_DB_HAS_REDIS_CLIENT_C follows it unless pre-defined.
#if D_CFG_IS_ON(D_CFG_ENV_USING_REDIS)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_REDIS_C_PATH)
            #include D_CFG_ENV_REDIS_C_PATH  // hiredis
            #define D_ENV_REDIS_HEADER_INCLUDED 1
        #elif __has_include(<hiredis/hiredis.h>)
            #include <hiredis/hiredis.h>  // hiredis
            #define D_ENV_REDIS_HEADER_INCLUDED 1
        #elif __has_include(<hiredis.h>)
            #include <hiredis.h>  // hiredis
            #define D_ENV_REDIS_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_REDIS=1 but no hiredis.h header was "      \
                   "found. Install libhiredis-dev (or equivalent), or "        \
                   "define D_CFG_ENV_REDIS_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_REDIS_C_PATH  // hiredis
        #define D_ENV_REDIS_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_REDIS_CLIENT_C
        #define D_ENV_DB_HAS_REDIS_CLIENT_C 1
    #endif  // D_ENV_DB_HAS_REDIS_CLIENT_C

#else
    #define D_ENV_REDIS_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_REDIS_CLIENT_C
        #define D_ENV_DB_HAS_REDIS_CLIENT_C 0
    #endif  // D_ENV_DB_HAS_REDIS_CLIENT_C
#endif  // D_CFG_ENV_USING_REDIS

// 1.1.2
// D_ENV_REDIS_SSL_HEADER_INCLUDED
//   detection: 1 once hiredis's optional SSL/TLS header,
// <hiredis/hiredis_ssl.h> or <hiredis_ssl.h>, is found and included; 0
// otherwise, and always 0 unless D_CFG_ENV_USING_REDIS is on where
// __has_include is available.
#if D_CFG_IS_ON(D_CFG_ENV_USING_REDIS)

    #if defined(__has_include)
        #if __has_include(<hiredis/hiredis_ssl.h>)
            #include <hiredis/hiredis_ssl.h>  // hiredis SSL
            #define D_ENV_REDIS_SSL_HEADER_INCLUDED 1
        #elif __has_include(<hiredis_ssl.h>)
            #include <hiredis_ssl.h>  // hiredis SSL
            #define D_ENV_REDIS_SSL_HEADER_INCLUDED 1
        #else
            #define D_ENV_REDIS_SSL_HEADER_INCLUDED 0
        #endif
    #else
        #define D_ENV_REDIS_SSL_HEADER_INCLUDED 0
    #endif

#else
    #define D_ENV_REDIS_SSL_HEADER_INCLUDED 0
#endif  // D_CFG_ENV_USING_REDIS


//==============================================================================
// 2.  VERSION ENCODING
//==============================================================================


// 2.1    Encoding and decoding
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_REDIS_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple.
#define D_ENV_REDIS_ENCODE_VERSION(major, minor, patch)                        \
    ((major) * 10000 + (minor) * 100 + (patch))

// 2.1.2
// D_ENV_REDIS_DECODE_MAJOR
//   macro: extracts the major version.
#define D_ENV_REDIS_DECODE_MAJOR(ver)                                          \
    ((ver) / 10000)

// 2.1.3
// D_ENV_REDIS_DECODE_MINOR
//   macro: extracts the minor version.
#define D_ENV_REDIS_DECODE_MINOR(ver)                                          \
    (((ver) / 100) % 100)

// 2.1.4
// D_ENV_REDIS_DECODE_PATCH
//   macro: extracts the patch version.
#define D_ENV_REDIS_DECODE_PATCH(ver)                                          \
    ((ver) % 100)


//==============================================================================
// 3.  CLIENT VERSION DETECTION
//==============================================================================
// The hiredis client is detected from the HIREDIS_MAJOR / HIREDIS_MINOR /
// HIREDIS_PATCH macros exposed by hiredis.h. The client version determines
// which client-side APIs (RESP3 push handlers, SSL context, async adapters)
// are available at compile time.


// 3.1    Server release IDs
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_REDIS_SERVER_<MAJOR>_<MINOR>
//   constant: encoded IDs of the server releases that gate features below.
#define D_ENV_REDIS_SERVER_2_6         20600
#define D_ENV_REDIS_SERVER_2_8         20800
#define D_ENV_REDIS_SERVER_3_0         30000
#define D_ENV_REDIS_SERVER_3_2         30200
#define D_ENV_REDIS_SERVER_4_0         40000
#define D_ENV_REDIS_SERVER_5_0         50000
#define D_ENV_REDIS_SERVER_6_0         60000
#define D_ENV_REDIS_SERVER_6_2         60200
#define D_ENV_REDIS_SERVER_7_0         70000
#define D_ENV_REDIS_SERVER_7_2         70200
#define D_ENV_REDIS_SERVER_7_4         70400
#define D_ENV_REDIS_SERVER_8_0         80000

// 3.2    Detected client
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_REDIS_CLIENT_DETECTED / D_ENV_REDIS_CLIENT_*
//   detection: D_ENV_REDIS_CLIENT_DETECTED is 1 when the hiredis version is
// known, and D_ENV_REDIS_CLIENT_VERSION_ID, _MAJOR, _MINOR and _PATCH then
// describe it, with D_ENV_REDIS_CLIENT_SONAME when hiredis defines
// HIREDIS_SONAME. Automatic mode reads HIREDIS_MAJOR, HIREDIS_MINOR and
// HIREDIS_PATCH; manual mode (D_CFG_ENV_REDIS_CUSTOM) reads
// D_ENV_REDIS_DETECTED_CLIENT_VERSION.
#if D_CFG_IS_OFF(D_CFG_ENV_REDIS_CUSTOM)

    // automatic detection requires hiredis.h to be in scope; if
    // D_CFG_ENV_USING_REDIS was not enabled the sentinel is 0 and we skip
    // cleanly (no reference to HIREDIS_MAJOR).
    #if ( (D_ENV_REDIS_HEADER_INCLUDED) &&                                     \
          (defined(HIREDIS_MAJOR)) )
        #define D_ENV_REDIS_CLIENT_DETECTED    1
        #define D_ENV_REDIS_CLIENT_MAJOR       HIREDIS_MAJOR
        #define D_ENV_REDIS_CLIENT_MINOR       HIREDIS_MINOR
        #define D_ENV_REDIS_CLIENT_PATCH       HIREDIS_PATCH
        #define D_ENV_REDIS_CLIENT_VERSION_ID                                  \
            D_ENV_REDIS_ENCODE_VERSION(HIREDIS_MAJOR,                          \
                                        HIREDIS_MINOR,                         \
                                        HIREDIS_PATCH)
        #ifdef HIREDIS_SONAME
            #define D_ENV_REDIS_CLIENT_SONAME  HIREDIS_SONAME
        #endif  // HIREDIS_SONAME
    #else
        #define D_ENV_REDIS_CLIENT_DETECTED    0
    #endif

#else
    // manual mode
    #ifdef D_ENV_REDIS_DETECTED_CLIENT_VERSION
        #define D_ENV_REDIS_CLIENT_DETECTED    1
        #define D_ENV_REDIS_CLIENT_VERSION_ID                                  \
            D_ENV_REDIS_DETECTED_CLIENT_VERSION
        #define D_ENV_REDIS_CLIENT_MAJOR                                       \
            D_ENV_REDIS_DECODE_MAJOR(D_ENV_REDIS_DETECTED_CLIENT_VERSION)
        #define D_ENV_REDIS_CLIENT_MINOR                                       \
            D_ENV_REDIS_DECODE_MINOR(D_ENV_REDIS_DETECTED_CLIENT_VERSION)
        #define D_ENV_REDIS_CLIENT_PATCH                                       \
            D_ENV_REDIS_DECODE_PATCH(D_ENV_REDIS_DETECTED_CLIENT_VERSION)
    #else
        #define D_ENV_REDIS_CLIENT_DETECTED    0
    #endif  // D_ENV_REDIS_DETECTED_CLIENT_VERSION

#endif  // D_CFG_ENV_REDIS_CUSTOM


//==============================================================================
// 4.  TARGET SERVER VERSION DETECTION
//==============================================================================
// The server version is a runtime property. These macros allow compile-
// time gating against a known deployment target. Set manually via
// D_CFG_ENV_REDIS_SERVER_VERSION or D_ENV_REDIS_DETECTED_SERVER_*.


// 4.1    Target server version
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_REDIS_SERVER_VERSION_ID
//   detection: the encoded target server version, from
// D_CFG_ENV_REDIS_SERVER_VERSION, D_ENV_REDIS_DETECTED_SERVER_VERSION, or the
// newest D_ENV_REDIS_DETECTED_SERVER_<MAJOR>_<MINOR> defined, in that order; 0
// when none is.
#ifndef D_ENV_REDIS_SERVER_VERSION_ID
    #ifdef D_CFG_ENV_REDIS_SERVER_VERSION
        #define D_ENV_REDIS_SERVER_VERSION_ID                                  \
            D_CFG_ENV_REDIS_SERVER_VERSION
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_VERSION)
        #define D_ENV_REDIS_SERVER_VERSION_ID                                  \
            D_ENV_REDIS_DETECTED_SERVER_VERSION
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_8_0)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_8_0
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_7_4)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_7_4
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_7_2)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_7_2
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_7_0)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_7_0
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_6_2)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_6_2
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_6_0)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_6_0
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_5_0)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_5_0
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_4_0)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_4_0
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_3_2)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_3_2
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_3_0)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_3_0
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_2_8)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_2_8
    #elif defined(D_ENV_REDIS_DETECTED_SERVER_2_6)
        #define D_ENV_REDIS_SERVER_VERSION_ID D_ENV_REDIS_SERVER_2_6
    #else
        // no server version specified; default to 0 (unknown).
        // server-gated features will evaluate to 0.
        #define D_ENV_REDIS_SERVER_VERSION_ID 0
    #endif  // D_CFG_ENV_REDIS_SERVER_VERSION
#endif  // D_ENV_REDIS_SERVER_VERSION_ID

// 4.1.2
// D_ENV_REDIS_SERVER_MAJOR / D_ENV_REDIS_SERVER_MINOR
//   macro: the major and minor parts of D_ENV_REDIS_SERVER_VERSION_ID.
#define D_ENV_REDIS_SERVER_MAJOR                                               \
    D_ENV_REDIS_DECODE_MAJOR(D_ENV_REDIS_SERVER_VERSION_ID)
#define D_ENV_REDIS_SERVER_MINOR                                               \
    D_ENV_REDIS_DECODE_MINOR(D_ENV_REDIS_SERVER_VERSION_ID)

// 4.1.3
// D_ENV_REDIS_SERVER_KNOWN
//   status: 1 if a target server version has been configured.
#define D_ENV_REDIS_SERVER_KNOWN                                               \
    (D_ENV_REDIS_SERVER_VERSION_ID > 0)

// 4.1.4
// D_ENV_REDIS_DETECTED
//   detection: 1 if the client is detected or a target server version is known.
#define D_ENV_REDIS_DETECTED                                                   \
    ( (D_ENV_REDIS_CLIENT_DETECTED) ||                                         \
      (D_ENV_REDIS_SERVER_KNOWN) )


// sections 5 to 18 exist only when D_ENV_REDIS_DETECTED is 1; see 4.1.4
#if D_ENV_REDIS_DETECTED


//==============================================================================
// 5.  VERSION COMPARISON MACROS
//==============================================================================


// 5.1    Client comparisons
//------------------------------------------------------------------------------
    // 5.1.1
    // D_ENV_REDIS_CLIENT_AT_LEAST
    //   macro: 1 if the detected client is at least major.minor.patch, and 0
    // when no client is detected.
    #if D_ENV_REDIS_CLIENT_DETECTED
        #define D_ENV_REDIS_CLIENT_AT_LEAST(major, minor, patch)               \
            (D_ENV_REDIS_CLIENT_VERSION_ID >=                                  \
                D_ENV_REDIS_ENCODE_VERSION(major, minor, patch))
    #else
        #define D_ENV_REDIS_CLIENT_AT_LEAST(major, minor, patch) 0
    #endif

// 5.2    Server comparisons
//------------------------------------------------------------------------------
    // 5.2.1
    // D_ENV_REDIS_SERVER_AT_LEAST
    //   macro: 1 if the target server version is at least major.minor.patch.
    #define D_ENV_REDIS_SERVER_AT_LEAST(major, minor, patch)                   \
        (D_ENV_REDIS_SERVER_VERSION_ID >=                                      \
            D_ENV_REDIS_ENCODE_VERSION(major, minor, patch))

    // 5.2.2
    // D_ENV_REDIS_SERVER_BELOW
    //   macro: 1 if the target server version is below major.minor.patch.
    #define D_ENV_REDIS_SERVER_BELOW(major, minor, patch)                      \
        (D_ENV_REDIS_SERVER_VERSION_ID <                                       \
            D_ENV_REDIS_ENCODE_VERSION(major, minor, patch))

    // 5.2.3
    // D_ENV_REDIS_SERVER_IN_RANGE
    //   macro: 1 if the target server version is at least the first triple and
    // below the second.
    #define D_ENV_REDIS_SERVER_IN_RANGE(min_maj, min_min, min_pat,             \
                                        max_maj, max_min, max_pat)             \
        ( (D_ENV_REDIS_SERVER_AT_LEAST(min_maj, min_min, min_pat)) &&          \
          (D_ENV_REDIS_SERVER_BELOW(max_maj, max_min, max_pat)) )

// 5.3    Release series
//------------------------------------------------------------------------------
    // 5.3.1
    // D_ENV_REDIS_IS_<SERIES>
    //   macro: 1 if the target server version is in that release series, 2.8 to
    // 8.0; D_ENV_REDIS_IS_8_0 also covers every later release.
    #define D_ENV_REDIS_IS_2_8                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(2, 8, 0, 3, 0, 0)
    #define D_ENV_REDIS_IS_3_0                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(3, 0, 0, 3, 2, 0)
    #define D_ENV_REDIS_IS_3_2                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(3, 2, 0, 4, 0, 0)
    #define D_ENV_REDIS_IS_4_0                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(4, 0, 0, 5, 0, 0)
    #define D_ENV_REDIS_IS_5_0                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(5, 0, 0, 6, 0, 0)
    #define D_ENV_REDIS_IS_6_0                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(6, 0, 0, 6, 2, 0)
    #define D_ENV_REDIS_IS_6_2                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(6, 2, 0, 7, 0, 0)
    #define D_ENV_REDIS_IS_7_0                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(7, 0, 0, 7, 2, 0)
    #define D_ENV_REDIS_IS_7_2                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(7, 2, 0, 7, 4, 0)
    #define D_ENV_REDIS_IS_7_4                                                 \
        D_ENV_REDIS_SERVER_IN_RANGE(7, 4, 0, 8, 0, 0)
    #define D_ENV_REDIS_IS_8_0                                                 \
        D_ENV_REDIS_SERVER_AT_LEAST(8, 0, 0)


//==============================================================================
// 6.  DISTRIBUTION DETECTION
//==============================================================================
// Redis ships in several distributions. Redis OSS is the open-source
// server; Redis Stack bundles the query/JSON/time-series/probabilistic
// modules; Redis Enterprise and Redis Cloud are commercial offerings with
// clustering, active-active (CRDT) geo-distribution, and tiered storage.
// Valkey is the Linux Foundation fork created after the 2024 license change.
// All distributions other than OSS must be set manually.


// 6.1    Distributions
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_REDIS_IS_VALKEY
    //   detection: 1 if targeting the Valkey fork. Valkey reports its own
    // version line; set manually.
    #ifndef D_ENV_REDIS_IS_VALKEY
        #if defined(D_ENV_REDIS_DETECTED_VALKEY)
            #define D_ENV_REDIS_IS_VALKEY 1
        #else
            #define D_ENV_REDIS_IS_VALKEY 0
        #endif
    #endif  // D_ENV_REDIS_IS_VALKEY

    // 6.1.2
    // D_ENV_REDIS_IS_ENTERPRISE
    //   detection: 1 if targeting Redis Enterprise (self-managed
    // commercial). Set manually.
    #ifndef D_ENV_REDIS_IS_ENTERPRISE
        #if defined(D_ENV_REDIS_DETECTED_ENTERPRISE)
            #define D_ENV_REDIS_IS_ENTERPRISE 1
        #else
            #define D_ENV_REDIS_IS_ENTERPRISE 0
        #endif
    #endif  // D_ENV_REDIS_IS_ENTERPRISE

    // 6.1.3
    // D_ENV_REDIS_IS_CLOUD
    //   detection: 1 if targeting Redis Cloud (fully managed). Set
    // manually. Implies Enterprise-class capabilities.
    #ifndef D_ENV_REDIS_IS_CLOUD
        #if defined(D_ENV_REDIS_DETECTED_CLOUD)
            #define D_ENV_REDIS_IS_CLOUD 1
        #else
            #define D_ENV_REDIS_IS_CLOUD 0
        #endif
    #endif  // D_ENV_REDIS_IS_CLOUD

    // 6.1.4
    // D_ENV_REDIS_IS_STACK
    //   detection: 1 if targeting Redis Stack (OSS server bundled with
    // the query, JSON, time-series, and probabilistic modules). Set
    // manually. Note: Redis 8.0+ folds these modules into the core
    // server, so a modern OSS target may also expose them (see the
    // module section below).
    #ifndef D_ENV_REDIS_IS_STACK
        #if defined(D_ENV_REDIS_DETECTED_STACK)
            #define D_ENV_REDIS_IS_STACK 1
        #else
            #define D_ENV_REDIS_IS_STACK 0
        #endif
    #endif  // D_ENV_REDIS_IS_STACK

    // 6.1.5
    // D_ENV_REDIS_IS_OSS
    //   detection: 1 if plain open-source Redis (none of the commercial
    // or forked distributions, and not the Valkey fork).
    #define D_ENV_REDIS_IS_OSS                                                 \
        ( (!D_ENV_REDIS_IS_VALKEY)     &&                                      \
          (!D_ENV_REDIS_IS_ENTERPRISE) &&                                      \
          (!D_ENV_REDIS_IS_CLOUD) )

    // 6.1.6
    // D_ENV_REDIS_IS_MANAGED_COMMERCIAL
    //   detection: 1 if any commercial offering (Enterprise or Cloud).
    #define D_ENV_REDIS_IS_MANAGED_COMMERCIAL                                  \
        ( (D_ENV_REDIS_IS_ENTERPRISE) ||                                       \
          (D_ENV_REDIS_IS_CLOUD) )


//==============================================================================
// 7.  RESP PROTOCOL DETECTION
//==============================================================================
// Redis speaks the REdis Serialization Protocol (RESP). RESP3 (introduced
// with Redis 6.0) adds typed replies, push messages, and is required for
// client-side caching in the default mode.


// 7.1    Protocol
//------------------------------------------------------------------------------
    // 7.1.1
    // D_ENV_REDIS_HAS_RESP3
    //   feature: RESP3 protocol (HELLO 3, typed replies, push frames).
    // Introduced in Redis 6.0.
    #define D_ENV_REDIS_HAS_RESP3                                              \
        D_ENV_REDIS_SERVER_AT_LEAST(6, 0, 0)

    // 7.1.2
    // D_ENV_REDIS_CLIENT_HAS_RESP3
    //   feature: hiredis client supports RESP3 push handlers
    // (redisContext push callbacks). Available since hiredis 1.0.0.
    #define D_ENV_REDIS_CLIENT_HAS_RESP3                                       \
        D_ENV_REDIS_CLIENT_AT_LEAST(1, 0, 0)


//==============================================================================
// 8.  CLIENT LIBRARY FEATURES (HIREDIS)
//==============================================================================


// 8.1    Client features
//------------------------------------------------------------------------------
    // 8.1.1
    // D_ENV_REDIS_HAS_HIREDIS
    //   feature: detect if the hiredis C client is available.
    #define D_ENV_REDIS_HAS_HIREDIS D_ENV_REDIS_CLIENT_DETECTED

    // 8.1.2
    // D_ENV_REDIS_HAS_CLIENT_SSL
    //   feature: hiredis SSL/TLS support (redisInitiateSSL, redisSSLContext
    // from hiredis_ssl.h). Present when the SSL header is in scope and
    // hiredis is at least 1.0.0.
    #define D_ENV_REDIS_HAS_CLIENT_SSL                                         \
        ( (D_ENV_REDIS_SSL_HEADER_INCLUDED) &&                                 \
          (D_ENV_REDIS_CLIENT_AT_LEAST(1, 0, 0)) )

    // 8.1.3
    // D_ENV_REDIS_HAS_CLIENT_ASYNC
    //   feature: hiredis asynchronous API (redisAsyncContext and event
    // loop adapters). Present in all modern hiredis versions.
    #define D_ENV_REDIS_HAS_CLIENT_ASYNC                                       \
        D_ENV_REDIS_CLIENT_AT_LEAST(0, 11, 0)

    // 8.1.4
    // D_ENV_REDIS_HAS_CLIENT_REUSEADDR
    //   feature: reconnect / keepalive helpers on the sync context.
    // Present since hiredis 0.13.0.
    #define D_ENV_REDIS_HAS_CLIENT_KEEPALIVE                                   \
        D_ENV_REDIS_CLIENT_AT_LEAST(0, 13, 0)


//==============================================================================
// 9.  CORE DATA STRUCTURES
//==============================================================================


// 9.1    Data structures
//------------------------------------------------------------------------------
    // 9.1.1
    // Foundational data-type flags
    //   feature: D_ENV_REDIS_HAS_STRINGS, D_ENV_REDIS_HAS_LISTS,
    // D_ENV_REDIS_HAS_SETS, D_ENV_REDIS_HAS_HASHES and
    // D_ENV_REDIS_HAS_SORTED_SETS, the foundational data types. Present
    // whenever a server version is known, since all date back to the earliest
    // releases.
    #define D_ENV_REDIS_HAS_STRINGS                                            \
        D_ENV_REDIS_SERVER_KNOWN
    #define D_ENV_REDIS_HAS_LISTS                                              \
        D_ENV_REDIS_SERVER_KNOWN
    #define D_ENV_REDIS_HAS_SETS                                               \
        D_ENV_REDIS_SERVER_KNOWN
    #define D_ENV_REDIS_HAS_HASHES                                             \
        D_ENV_REDIS_SERVER_KNOWN
    #define D_ENV_REDIS_HAS_SORTED_SETS                                        \
        D_ENV_REDIS_SERVER_KNOWN

    // 9.1.2
    // D_ENV_REDIS_HAS_BITMAPS
    //   feature: bitmap operations (SETBIT/GETBIT/BITCOUNT). Available
    // since Redis 2.2; BITFIELD added in 3.2.
    #define D_ENV_REDIS_HAS_BITMAPS                                            \
        D_ENV_REDIS_SERVER_KNOWN

    // 9.1.3
    // D_ENV_REDIS_HAS_BITFIELD
    //   feature: the BITFIELD command. Introduced in Redis 3.2.
    #define D_ENV_REDIS_HAS_BITFIELD                                           \
        D_ENV_REDIS_SERVER_AT_LEAST(3, 2, 0)

    // 9.1.4
    // D_ENV_REDIS_HAS_HYPERLOGLOG
    //   feature: HyperLogLog cardinality estimation (PFADD/PFCOUNT).
    // Introduced in Redis 2.8.9.
    #define D_ENV_REDIS_HAS_HYPERLOGLOG                                        \
        D_ENV_REDIS_SERVER_AT_LEAST(2, 8, 9)

    // 9.1.5
    // D_ENV_REDIS_HAS_GEO
    //   feature: geospatial indexes and commands (GEOADD/GEOSEARCH).
    // Introduced in Redis 3.2.
    #define D_ENV_REDIS_HAS_GEO                                                \
        D_ENV_REDIS_SERVER_AT_LEAST(3, 2, 0)

    // 9.1.6
    // D_ENV_REDIS_HAS_STREAMS
    //   feature: stream data type and consumer groups (XADD/XREADGROUP).
    // Introduced in Redis 5.0.
    #define D_ENV_REDIS_HAS_STREAMS                                            \
        D_ENV_REDIS_SERVER_AT_LEAST(5, 0, 0)

    // 9.1.7
    // D_ENV_REDIS_HAS_HASH_FIELD_TTL
    //   feature: per-field TTL on hashes (HEXPIRE/HPEXPIRE/HGETEX).
    // Introduced in Redis 7.4.
    #define D_ENV_REDIS_HAS_HASH_FIELD_TTL                                     \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 4, 0)


//==============================================================================
// 10.  COMMAND-GROUP AND EXECUTION FEATURES
//==============================================================================


// 10.1   Commands and execution
//------------------------------------------------------------------------------
    // 10.1.1
    // D_ENV_REDIS_HAS_TRANSACTIONS
    //   feature: MULTI/EXEC/DISCARD/WATCH transactions. Core feature.
    #define D_ENV_REDIS_HAS_TRANSACTIONS                                       \
        D_ENV_REDIS_SERVER_KNOWN

    // 10.1.2
    // D_ENV_REDIS_HAS_PUBSUB
    //   feature: publish/subscribe messaging. Core feature.
    #define D_ENV_REDIS_HAS_PUBSUB                                             \
        D_ENV_REDIS_SERVER_KNOWN

    // 10.1.3
    // D_ENV_REDIS_HAS_SHARDED_PUBSUB
    //   feature: sharded pub/sub (SSUBSCRIBE/SPUBLISH) for Cluster mode.
    // Introduced in Redis 7.0.
    #define D_ENV_REDIS_HAS_SHARDED_PUBSUB                                     \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 0, 0)

    // 10.1.4
    // D_ENV_REDIS_HAS_SCRIPTING
    //   feature: server-side Lua scripting (EVAL/EVALSHA). Introduced in
    // Redis 2.6.
    #define D_ENV_REDIS_HAS_SCRIPTING                                          \
        D_ENV_REDIS_SERVER_AT_LEAST(2, 6, 0)

    // 10.1.5
    // D_ENV_REDIS_HAS_FUNCTIONS
    //   feature: Redis Functions (FUNCTION LOAD, persisted/replicated Lua
    // libraries). Introduced in Redis 7.0.
    #define D_ENV_REDIS_HAS_FUNCTIONS                                          \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 0, 0)

    // 10.1.6
    // D_ENV_REDIS_HAS_KEYSPACE_NOTIFICATIONS
    //   feature: keyspace event notifications via pub/sub. Introduced in
    // Redis 2.8.
    #define D_ENV_REDIS_HAS_KEYSPACE_NOTIFICATIONS                             \
        D_ENV_REDIS_SERVER_AT_LEAST(2, 8, 0)

    // 10.1.7
    // D_ENV_REDIS_HAS_CLIENT_SIDE_CACHING
    //   feature: server-assisted client-side caching (CLIENT TRACKING).
    // Introduced in Redis 6.0; works best over RESP3.
    #define D_ENV_REDIS_HAS_CLIENT_SIDE_CACHING                                \
        D_ENV_REDIS_SERVER_AT_LEAST(6, 0, 0)

    // 10.1.8
    // D_ENV_REDIS_HAS_EXPIRE_OPTIONS
    //   feature: NX/XX/GT/LT options on EXPIRE/SET. Introduced in
    // Redis 7.0.
    #define D_ENV_REDIS_HAS_EXPIRE_OPTIONS                                     \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 0, 0)


//==============================================================================
// 11.  PERSISTENCE
//==============================================================================


// 11.1   Persistence
//------------------------------------------------------------------------------
    // 11.1.1
    // D_ENV_REDIS_HAS_RDB
    //   feature: RDB point-in-time snapshots. Core feature.
    #define D_ENV_REDIS_HAS_RDB                                                \
        D_ENV_REDIS_SERVER_KNOWN

    // 11.1.2
    // D_ENV_REDIS_HAS_AOF
    //   feature: append-only file persistence. Core feature.
    #define D_ENV_REDIS_HAS_AOF                                                \
        D_ENV_REDIS_SERVER_KNOWN

    // 11.1.3
    // D_ENV_REDIS_HAS_HYBRID_PERSISTENCE
    //   feature: RDB-preamble AOF (aof-use-rdb-preamble). Introduced in
    // Redis 4.0.
    #define D_ENV_REDIS_HAS_HYBRID_PERSISTENCE                                 \
        D_ENV_REDIS_SERVER_AT_LEAST(4, 0, 0)

    // 11.1.4
    // D_ENV_REDIS_HAS_MULTI_PART_AOF
    //   feature: multi-part AOF (base + incremental files with a
    // manifest). Introduced in Redis 7.0.
    #define D_ENV_REDIS_HAS_MULTI_PART_AOF                                     \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 0, 0)


//==============================================================================
// 12.  REPLICATION AND HIGH AVAILABILITY
//==============================================================================


// 12.1   Replication and high availability
//------------------------------------------------------------------------------
    // 12.1.1
    // D_ENV_REDIS_HAS_REPLICATION
    //   feature: asynchronous primary/replica replication. Core feature.
    #define D_ENV_REDIS_HAS_REPLICATION                                        \
        D_ENV_REDIS_SERVER_KNOWN

    // 12.1.2
    // D_ENV_REDIS_HAS_SENTINEL
    //   feature: Redis Sentinel for monitoring and automatic failover.
    // Stable since Redis 2.8.
    #define D_ENV_REDIS_HAS_SENTINEL                                           \
        D_ENV_REDIS_SERVER_AT_LEAST(2, 8, 0)

    // 12.1.3
    // D_ENV_REDIS_HAS_CLUSTER
    //   feature: Redis Cluster (sharding with hash slots). Stable since
    // Redis 3.0.
    #define D_ENV_REDIS_HAS_CLUSTER                                            \
        D_ENV_REDIS_SERVER_AT_LEAST(3, 0, 0)

    // 12.1.4
    // D_ENV_REDIS_HAS_WAIT
    //   feature: the WAIT command for synchronous-ish replication
    // acknowledgement. Introduced in Redis 3.0.
    #define D_ENV_REDIS_HAS_WAIT                                               \
        D_ENV_REDIS_SERVER_AT_LEAST(3, 0, 0)

    // 12.1.5
    // D_ENV_REDIS_HAS_WAITAOF
    //   feature: the WAITAOF command (acknowledged local/replica AOF
    // fsync). Introduced in Redis 7.2.
    #define D_ENV_REDIS_HAS_WAITAOF                                            \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 2, 0)

    // 12.1.6
    // D_ENV_REDIS_HAS_DISKLESS_REPLICATION
    //   feature: diskless replication (repl-diskless-sync). Introduced
    // in Redis 3.2.
    #define D_ENV_REDIS_HAS_DISKLESS_REPLICATION                               \
        D_ENV_REDIS_SERVER_AT_LEAST(3, 2, 0)

    // 12.1.7
    // D_ENV_REDIS_HAS_ACTIVE_ACTIVE
    //   feature: active-active geo-distributed databases (CRDT-based).
    // Enterprise / Cloud only.
    #define D_ENV_REDIS_HAS_ACTIVE_ACTIVE                                      \
        D_ENV_REDIS_IS_MANAGED_COMMERCIAL


//==============================================================================
// 13.  MEMORY MANAGEMENT AND THREADING
//==============================================================================


// 13.1   Memory and threading
//------------------------------------------------------------------------------
    // 13.1.1
    // D_ENV_REDIS_HAS_LRU_EVICTION
    //   feature: maxmemory with LRU/random eviction policies. Core
    // feature.
    #define D_ENV_REDIS_HAS_LRU_EVICTION                                       \
        D_ENV_REDIS_SERVER_KNOWN

    // 13.1.2
    // D_ENV_REDIS_HAS_LFU_EVICTION
    //   feature: LFU (least-frequently-used) eviction policies.
    // Introduced in Redis 4.0.
    #define D_ENV_REDIS_HAS_LFU_EVICTION                                       \
        D_ENV_REDIS_SERVER_AT_LEAST(4, 0, 0)

    // 13.1.3
    // D_ENV_REDIS_HAS_LAZY_FREE
    //   feature: asynchronous lazy freeing (UNLINK, lazyfree-* configs).
    // Introduced in Redis 4.0.
    #define D_ENV_REDIS_HAS_LAZY_FREE                                          \
        D_ENV_REDIS_SERVER_AT_LEAST(4, 0, 0)

    // 13.1.4
    // D_ENV_REDIS_HAS_THREADED_IO
    //   feature: multi-threaded I/O for reads/writes (io-threads).
    // Introduced in Redis 6.0.
    #define D_ENV_REDIS_HAS_THREADED_IO                                        \
        D_ENV_REDIS_SERVER_AT_LEAST(6, 0, 0)

    // 13.1.5
    // D_ENV_REDIS_HAS_TIERED_STORAGE
    //   feature: tiered storage / Auto Tiering (RAM + flash). Enterprise
    // / Cloud only (formerly Redis on Flash).
    #define D_ENV_REDIS_HAS_TIERED_STORAGE                                     \
        D_ENV_REDIS_IS_MANAGED_COMMERCIAL


//==============================================================================
// 14.  SECURITY
//==============================================================================


// 14.1   Security
//------------------------------------------------------------------------------
    // 14.1.1
    // D_ENV_REDIS_HAS_AUTH
    //   feature: password authentication (AUTH / requirepass). Core
    // feature.
    #define D_ENV_REDIS_HAS_AUTH                                               \
        D_ENV_REDIS_SERVER_KNOWN

    // 14.1.2
    // D_ENV_REDIS_HAS_ACL
    //   feature: access control lists (ACL SETUSER, multiple users,
    // command/key/channel rules). Introduced in Redis 6.0.
    #define D_ENV_REDIS_HAS_ACL                                                \
        D_ENV_REDIS_SERVER_AT_LEAST(6, 0, 0)

    // 14.1.3
    // D_ENV_REDIS_HAS_ACL_SELECTORS
    //   feature: ACL selectors (multiple permission sets per user).
    // Introduced in Redis 7.0.
    #define D_ENV_REDIS_HAS_ACL_SELECTORS                                      \
        D_ENV_REDIS_SERVER_AT_LEAST(7, 0, 0)

    // 14.1.4
    // D_ENV_REDIS_HAS_TLS
    //   feature: server-side TLS for client and replication links.
    // Introduced in Redis 6.0.
    #define D_ENV_REDIS_HAS_TLS                                                \
        D_ENV_REDIS_SERVER_AT_LEAST(6, 0, 0)


//==============================================================================
// 15.  MODULES
//==============================================================================
// Redis modules extend the server with new data types and commands.
// Historically distributed via Redis Stack; Redis 8.0 reunified the core
// query and data modules (Search, JSON, Time Series, and the probabilistic
// "Bloom" types) into the open-source server. A module is therefore present
// if EITHER the target is Redis 8.0+ OSS, OR a Stack/commercial build that
// bundles it is in use.


// 15.1   Modules
//------------------------------------------------------------------------------
    // 15.1.1
    // D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED
    //   status: 1 if the target bundles the core modules out of the box
    // (Redis 8.0+ OSS, Redis Stack, or a commercial offering).
    #define D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED                               \
        ( (D_ENV_REDIS_SERVER_AT_LEAST(8, 0, 0)) ||                            \
          (D_ENV_REDIS_IS_STACK)                 ||                            \
          (D_ENV_REDIS_IS_MANAGED_COMMERCIAL) )

    // 15.1.2
    // D_ENV_REDIS_HAS_SEARCH
    //   feature: secondary indexing and full-text/query engine
    // (RediSearch / FT.* commands).
    #define D_ENV_REDIS_HAS_SEARCH                                             \
        D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED

    // 15.1.3
    // D_ENV_REDIS_HAS_JSON
    //   feature: native JSON document type (RedisJSON / JSON.* commands).
    #define D_ENV_REDIS_HAS_JSON                                               \
        D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED

    // 15.1.4
    // D_ENV_REDIS_HAS_TIME_SERIES
    //   feature: time-series data type (RedisTimeSeries / TS.* commands).
    #define D_ENV_REDIS_HAS_TIME_SERIES                                        \
        D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED

    // 15.1.5
    // D_ENV_REDIS_HAS_PROBABILISTIC
    //   feature: probabilistic data structures (RedisBloom: Bloom/Cuckoo
    // filters, Count-Min Sketch, Top-K, t-digest).
    #define D_ENV_REDIS_HAS_PROBABILISTIC                                      \
        D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED

    // 15.1.6
    // D_ENV_REDIS_HAS_VECTOR_SEARCH
    //   feature: vector similarity search. Available via the search
    // module (vector fields), and as native vector sets (VADD/VSIM) in
    // Redis 8.0+.
    #define D_ENV_REDIS_HAS_VECTOR_SEARCH                                      \
        ( (D_ENV_REDIS_HAS_SEARCH) ||                                          \
          (D_ENV_REDIS_SERVER_AT_LEAST(8, 0, 0)) )

    // 15.1.7
    // D_ENV_REDIS_HAS_VECTOR_SETS
    //   feature: native vector set data type (VADD/VSIM/VREM).
    // Introduced in Redis 8.0.
    #define D_ENV_REDIS_HAS_VECTOR_SETS                                        \
        D_ENV_REDIS_SERVER_AT_LEAST(8, 0, 0)


//==============================================================================
// 16.  COMPOSITE CHECKS
//==============================================================================


// 16.1   Composite checks
//------------------------------------------------------------------------------
    // 16.1.1
    // D_ENV_REDIS_HAS_PROGRAMMABILITY
    //   macro: evaluates to 1 if both Lua scripting and Redis Functions
    // are available (Redis 7.0+).
    #define D_ENV_REDIS_HAS_PROGRAMMABILITY                                    \
        ( (D_ENV_REDIS_HAS_SCRIPTING) &&                                       \
          (D_ENV_REDIS_HAS_FUNCTIONS) )

    // 16.1.2
    // D_ENV_REDIS_HAS_MODERN_SECURITY
    //   macro: evaluates to 1 if ACLs, ACL selectors, and TLS are all
    // available (Redis 7.0+).
    #define D_ENV_REDIS_HAS_MODERN_SECURITY                                    \
        ( (D_ENV_REDIS_HAS_ACL)           &&                                   \
          (D_ENV_REDIS_HAS_ACL_SELECTORS) &&                                   \
          (D_ENV_REDIS_HAS_TLS) )

    // 16.1.3
    // D_ENV_REDIS_HAS_MODERN_HA
    //   macro: evaluates to 1 if Cluster, sharded pub/sub, and WAITAOF
    // are all available (Redis 7.2+).
    #define D_ENV_REDIS_HAS_MODERN_HA                                          \
        ( (D_ENV_REDIS_HAS_CLUSTER)        &&                                  \
          (D_ENV_REDIS_HAS_SHARDED_PUBSUB) &&                                  \
          (D_ENV_REDIS_HAS_WAITAOF) )

    // 16.1.4
    // D_ENV_REDIS_HAS_DATA_PLATFORM
    //   macro: evaluates to 1 if the target is a full data platform with
    // search, JSON, time-series, and probabilistic structures bundled.
    #define D_ENV_REDIS_HAS_DATA_PLATFORM                                      \
        ( (D_ENV_REDIS_HAS_SEARCH)      &&                                     \
          (D_ENV_REDIS_HAS_JSON)        &&                                     \
          (D_ENV_REDIS_HAS_TIME_SERIES) &&                                     \
          (D_ENV_REDIS_HAS_PROBABILISTIC) )

    // 16.1.5
    // D_ENV_REDIS_IS_FULLY_MODERN
    //   macro: evaluates to 1 if the server has a comprehensive modern
    // feature set (roughly Redis 8.0+ with programmability, modern
    // security, modern HA, the bundled data platform, and native vector
    // sets).
    #define D_ENV_REDIS_IS_FULLY_MODERN                                        \
        ( (D_ENV_REDIS_HAS_PROGRAMMABILITY) &&                                 \
          (D_ENV_REDIS_HAS_MODERN_SECURITY) &&                                 \
          (D_ENV_REDIS_HAS_MODERN_HA)       &&                                 \
          (D_ENV_REDIS_HAS_DATA_PLATFORM)   &&                                 \
          (D_ENV_REDIS_HAS_VECTOR_SETS) )


//==============================================================================
// 17.  DEPRECATION AND REMOVAL
//==============================================================================


// 17.1   Deprecations
//------------------------------------------------------------------------------
    // 17.1.1
    // D_ENV_REDIS_DEPRECATED_SLAVE_TERMINOLOGY
    //   status: 1 if "slave" config/command terminology is deprecated in
    // favor of "replica" (SLAVEOF -> REPLICAOF). Changed in Redis 5.0.
    #define D_ENV_REDIS_DEPRECATED_SLAVE_TERMINOLOGY                           \
        D_ENV_REDIS_SERVER_AT_LEAST(5, 0, 0)

    // 17.1.2
    // D_ENV_REDIS_DEPRECATED_GEORADIUS
    //   status: 1 if GEORADIUS / GEORADIUSBYMEMBER are deprecated in
    // favor of GEOSEARCH / GEOSEARCHSTORE (Redis 6.2+).
    #define D_ENV_REDIS_DEPRECATED_GEORADIUS                                   \
        D_ENV_REDIS_SERVER_AT_LEAST(6, 2, 0)

    // 17.1.3
    // D_ENV_REDIS_LICENSE_IS_OSI
    //   status: 1 if the server is under an OSI-approved open-source
    // license. Redis OSS through 7.2 was BSD; 7.4 moved to the dual
    // RSALv2/SSPLv1 source-available license; Redis 8.0 added AGPLv3 as
    // an OSI-approved option. Valkey remains BSD-licensed throughout.
    #define D_ENV_REDIS_LICENSE_IS_OSI                                         \
        ( (D_ENV_REDIS_IS_VALKEY)             ||                               \
          (D_ENV_REDIS_SERVER_BELOW(7, 4, 0)) ||                               \
          (D_ENV_REDIS_SERVER_AT_LEAST(8, 0, 0)) )


//==============================================================================
// 18.  CONSUMER COMPATIBILITY LAYER
//==============================================================================
// The djinterp Redis connection layer (redis.hpp, redis_table.hpp) consumes
// some names the sections above don't produce; this section publishes them in
// terms of the detection model, so that the connection layer needs no
// knowledge of the client and server split. Names that already exist above,
// such as D_ENV_REDIS_HAS_STREAMS, are left alone. Feature availability
// follows the target server version, so D_ENV_REDIS_VERSION_* mirror it,
// falling back to the client's version.


// 18.1   Consumer vocabulary
//------------------------------------------------------------------------------
    // 18.1.1
    // D_ENV_REDIS_VERSION_*
    //   macro: the public version, D_ENV_REDIS_VERSION_ID, _MAJOR, _MINOR,
    // _PATCH and _STRING: the target server's when known, otherwise the
    // client's, otherwise 0; D_ENV_REDIS_VERSION_STRING is always "unknown".
    #if D_ENV_REDIS_SERVER_KNOWN
        #define D_ENV_REDIS_VERSION_ID    D_ENV_REDIS_SERVER_VERSION_ID
        #define D_ENV_REDIS_VERSION_MAJOR D_ENV_REDIS_SERVER_MAJOR
        #define D_ENV_REDIS_VERSION_MINOR D_ENV_REDIS_SERVER_MINOR
        #define D_ENV_REDIS_VERSION_PATCH                                      \
            D_ENV_REDIS_DECODE_PATCH(D_ENV_REDIS_SERVER_VERSION_ID)
    #elif D_ENV_REDIS_CLIENT_DETECTED
        #define D_ENV_REDIS_VERSION_ID    D_ENV_REDIS_CLIENT_VERSION_ID
        #define D_ENV_REDIS_VERSION_MAJOR D_ENV_REDIS_CLIENT_MAJOR
        #define D_ENV_REDIS_VERSION_MINOR D_ENV_REDIS_CLIENT_MINOR
        #define D_ENV_REDIS_VERSION_PATCH D_ENV_REDIS_CLIENT_PATCH
    #else
        #define D_ENV_REDIS_VERSION_ID    0
        #define D_ENV_REDIS_VERSION_MAJOR 0
        #define D_ENV_REDIS_VERSION_MINOR 0
        #define D_ENV_REDIS_VERSION_PATCH 0
    #endif
    #define D_ENV_REDIS_VERSION_STRING "unknown"

    // 18.1.2
    // Consumer feature aliases
    //   macro: names the connection layer uses: D_ENV_REDIS_HAS_RESP2 and
    // D_ENV_REDIS_HAS_SCAN, always 1 here; D_ENV_REDIS_HAS_MODERN_PROTOCOL,
    // D_ENV_REDIS_HAS_LUA_SCRIPTING, D_ENV_REDIS_HAS_OPTIMISTIC_LOCKING,
    // D_ENV_REDIS_HAS_PSYNC, D_ENV_REDIS_HAS_CLIENT_TRACKING,
    // D_ENV_REDIS_HAS_LAZY_EXPIRATION, D_ENV_REDIS_HAS_MODULES, and the module
    // names, D_ENV_REDIS_HAS_REJSON, _RSEARCH, _RTS and _RBLOOM, each an alias
    // of a flag above; and D_ENV_REDIS_HAS_REDISGRAPH, 0 unless pre-defined.
    // protocol
    //   RESP2 is always available on any modern server; RESP3 (and thus the
    // "modern protocol") arrived with Redis 6.0.
    #define D_ENV_REDIS_HAS_RESP2          1
    #define D_ENV_REDIS_HAS_MODERN_PROTOCOL D_ENV_REDIS_HAS_RESP3

    // scripting / concurrency aliases
    #define D_ENV_REDIS_HAS_LUA_SCRIPTING      D_ENV_REDIS_HAS_SCRIPTING
    #define D_ENV_REDIS_HAS_OPTIMISTIC_LOCKING D_ENV_REDIS_HAS_TRANSACTIONS

    // replication / persistence aliases
    //   PSYNC (partial resynchronization) is part of the replication
    // protocol and is present wherever replication is.
    #define D_ENV_REDIS_HAS_PSYNC          D_ENV_REDIS_HAS_REPLICATION

    // caching / expiration aliases
    #define D_ENV_REDIS_HAS_CLIENT_TRACKING D_ENV_REDIS_HAS_CLIENT_SIDE_CACHING
    #define D_ENV_REDIS_HAS_LAZY_EXPIRATION D_ENV_REDIS_HAS_LAZY_FREE

    // key-space iteration
    //   SCAN / HSCAN / SSCAN / ZSCAN cursors are available on every modern
    // server; expose as always-on when detected.
    #define D_ENV_REDIS_HAS_SCAN           1

    // modules
    //   The presence of loadable modules in general, and the bundled core
    // data modules in particular, both follow HAS_CORE_MODULES_BUNDLED.
    #define D_ENV_REDIS_HAS_MODULES        D_ENV_REDIS_HAS_CORE_MODULES_BUNDLED
    #define D_ENV_REDIS_HAS_REJSON         D_ENV_REDIS_HAS_JSON
    #define D_ENV_REDIS_HAS_RSEARCH        D_ENV_REDIS_HAS_SEARCH
    #define D_ENV_REDIS_HAS_RTS            D_ENV_REDIS_HAS_TIME_SERIES
    #define D_ENV_REDIS_HAS_RBLOOM         D_ENV_REDIS_HAS_PROBABILISTIC

    // RedisGraph
    //   RedisGraph reached end-of-life in 2023 and was never folded into the
    // reunified core; it is therefore reported as unavailable. Integrators
    // pinning a legacy Stack build that still ships it may override this
    // macro before including this header.
    #ifndef D_ENV_REDIS_HAS_RGRAPH
        #define D_ENV_REDIS_HAS_RGRAPH     0
    #endif  // D_ENV_REDIS_HAS_RGRAPH


#endif  // D_ENV_REDIS_DETECTED


#endif  // DJINTERP_ENV_DB_REDIS_ENV_REDIS_H
