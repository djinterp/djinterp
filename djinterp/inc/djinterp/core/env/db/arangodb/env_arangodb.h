/******************************************************************************
* djinterp [db]                                              env_arangodb.h
*
* djinterp ArangoDB environmental detection header:
* This header provides comprehensive compile-time detection of ArangoDB
* environments, capabilities, and version-gated features, including:
*   - version decomposition (major, minor, patch) and comparison macros
*   - client driver detection (C++ fuerte driver, VelocyPack library)
*   - multi-model capabilities (document, graph, key-value, search)
*   - AQL (ArangoDB Query Language) feature detection
*   - graph features (traversals, shortest path, k-shortest-paths, k-paths,
*     all-shortest-paths, pregel, SmartGraphs, EnterpriseGraphs,
*     SatelliteGraphs, DisjointSmartGraphs, HybridSmartGraphs)
*   - index type detection (persistent, TTL, fulltext, geo, inverted,
*     multi-dimensional, ZKD)
*   - ArangoSearch / Views detection (IResearch-based analyzers,
*     search-alias views, scoring functions, nested search, SEARCH
*     highlighting, GeoJSON and geo-spatial analysis via analyzers)
*   - replication and clustering (Active Failover, OneShard, SmartJoins,
*     SatelliteCollections, cluster-wide transactions, DC2DC)
*   - transaction features (single-document, multi-document, streaming,
*     JavaScript transactions)
*   - storage engine detection (RocksDB; MMFiles removal)
*   - Foxx microservices framework detection
*   - VelocyPack (VPack) binary format detection
*   - HTTP API and protocol features (HTTP/2, VST protocol, cursors)
*   - authentication (JWT, LDAP, Kerberos)
*   - SSL/TLS and encryption (at-rest, in-transit, key rotation)
*   - Community vs Enterprise edition feature gating
*   - optimizer and query profiling features
*   - backup and restore capabilities (arangodump, arangorestore, hot
*     backup)
*
*   ArangoDB is a native multi-model database supporting documents, graphs,
* and key-value pairs with a single query language (AQL). It uses a JSON-
* based document model with the VelocyPack binary serialization format for
* performance. Features are determined by server version and by whether the
* Community or Enterprise edition is deployed; several key capabilities
* (SmartGraphs, encryption, LDAP, DC2DC replication, hot backup) require
* the Enterprise edition.
*
*   VERSION ENCODING:
*   ArangoDB uses semantic versioning (MAJOR.MINOR.PATCH). This header
* encodes version as MAJOR*10000 + MINOR*100 + PATCH, matching the
* MySQL-family convention. E.g. ArangoDB 3.11.5 = 31105.
*
*   NAMING CONVENTION:
*   D_ENV_ARANGO_[CATEGORY]_[FEATURE]  - 1 if available, 0 otherwise
*   D_ENV_ARANGO_VERSION_[COMPONENT]   - version number components
*   D_ENV_ARANGO_HAS_[CAPABILITY]      - capability flag (1/0)
*
*   DEPENDENCIES:
*   This header includes env_db.h for base database environment detection
* capabilities. It should be included after ArangoDB client/server headers
* so that version macros are available.
* 
*
* path:      /inc/djinterp/core/env/db/arangodb/env_arangodb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_ARANGODB_
#define DJINTERP_ENVIRONMENT_ARANGODB_ 1

// djinterp
#include "../../../../config/core/env/db/arangodb/env_arangodb_config.h"
#include "../env_db.h"


// =============================================================================
// 0.   VENDOR HEADER INCLUSION
// =============================================================================
//   Driven by D_CFG_ENV_USING_ARANGODB from env_config.h. ArangoDB has no
// canonical C client API — the closest embedded entry point is the
// velocypack C++ header. Consequently this section only engages in C++
// builds; in C builds, a #error is raised if USING is enabled, since there
// is nothing sensible to include. Detection below is gated on
// D_ENV_ARANGODB_CPP_HEADER_INCLUDED.

#if (D_CFG_ENV_USING_ARANGODB == 1)

    #ifdef __cplusplus
        #if defined(__has_include)
            #if __has_include(D_CFG_ENV_ARANGODB_CPP_PATH)
                #include D_CFG_ENV_ARANGODB_CPP_PATH
                #define D_ENV_ARANGODB_CPP_HEADER_INCLUDED 1
            #elif __has_include(<velocypack/vpack.h>)
                #include <velocypack/vpack.h>
                #define D_ENV_ARANGODB_CPP_HEADER_INCLUDED 1
            #else
                #error "D_CFG_ENV_USING_ARANGODB=1 but no velocypack "       \
                       "header was found. Install libvelocypack-dev (or "    \
                       "equivalent), or define D_CFG_ENV_ARANGODB_CPP_PATH " \
                       "to the correct location."
            #endif
        #else
            #include D_CFG_ENV_ARANGODB_CPP_PATH
            #define D_ENV_ARANGODB_CPP_HEADER_INCLUDED 1
        #endif

        #ifndef D_ENV_DB_HAS_ARANGODB_CLIENT_CPP
            #define D_ENV_DB_HAS_ARANGODB_CLIENT_CPP 1
        #endif

    #else  // !__cplusplus
        #error "D_CFG_ENV_USING_ARANGODB=1 requires a C++ build. ArangoDB "  \
               "has no canonical C client surface; use the velocypack C++ "  \
               "header or consume ArangoDB via its HTTP API directly."
    #endif

#else
    #define D_ENV_ARANGODB_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_ARANGODB_CLIENT_CPP
        #define D_ENV_DB_HAS_ARANGODB_CLIENT_CPP 0
    #endif
#endif  // D_CFG_ENV_USING_ARANGODB


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================

//   All D_CFG_* macros for this module live in env_arangodb_config.h,
// pulled in at the top of this file.


// =============================================================================
// II.  VERSION ENCODING
// =============================================================================
//   Encoded as MAJOR*10000 + MINOR*100 + PATCH.
//   E.g. ArangoDB 3.11.5 = 31105.

// D_ENV_ARANGO_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple into the version ID.
#define D_ENV_ARANGO_ENCODE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))

// D_ENV_ARANGO_DECODE_MAJOR
//   macro: extracts the major version from an encoded version ID.
#define D_ENV_ARANGO_DECODE_MAJOR(ver) \
    ((ver) / 10000)

// D_ENV_ARANGO_DECODE_MINOR
//   macro: extracts the minor version from an encoded version ID.
#define D_ENV_ARANGO_DECODE_MINOR(ver) \
    (((ver) / 100) % 100)

// D_ENV_ARANGO_DECODE_PATCH
//   macro: extracts the patch version from an encoded version ID.
#define D_ENV_ARANGO_DECODE_PATCH(ver) \
    ((ver) % 100)


// =============================================================================
// III. VERSION DETECTION
// =============================================================================

// version ID constants for feature-significant releases
#define D_ENV_ARANGO_VERSION_3_3_0     30300
#define D_ENV_ARANGO_VERSION_3_4_0     30400   // ArangoSearch, stream trx
#define D_ENV_ARANGO_VERSION_3_5_0     30500   // SmartJoins
#define D_ENV_ARANGO_VERSION_3_6_0     30600   // OneShard
#define D_ENV_ARANGO_VERSION_3_7_0     30700   // insert-update, MMFiles rm
#define D_ENV_ARANGO_VERSION_3_8_0     30800   // analyzer pipelines
#define D_ENV_ARANGO_VERSION_3_9_0     30900   // hybrid SmartGraphs
#define D_ENV_ARANGO_VERSION_3_9_5     30905   // search-alias views
#define D_ENV_ARANGO_VERSION_3_9_6     30906
#define D_ENV_ARANGO_VERSION_3_10_0    31000   // multi-dim indexes, computed
#define D_ENV_ARANGO_VERSION_3_10_2    31002
#define D_ENV_ARANGO_VERSION_3_11_0    31100   // inverted index standalone
#define D_ENV_ARANGO_VERSION_3_11_1    31101
#define D_ENV_ARANGO_VERSION_3_12_0    31200   // latest

#if (D_CFG_ENV_ARANGO_CUSTOM == 0)

    // automatic detection via ArangoDB-provided version macros.
    // ArangoDB server headers define ARANGODB_VERSION as a string
    // ("3.11.5") and may define numeric components separately.
    // The C++ driver (fuerte) defines FUERTE_VERSION or similar.
    // VelocyPack defines VELOCYPACK_VERSION.
    // Requires the velocypack/ArangoDB header to be in scope; if
    // D_CFG_ENV_USING_ARANGODB was not enabled the sentinel is 0 and we skip
    // cleanly (no reference to ARANGODB_VERSION_MAJOR).
    #if ( D_ENV_ARANGODB_CPP_HEADER_INCLUDED  &&  \
          defined(ARANGODB_VERSION_MAJOR) )
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_MAJOR     ARANGODB_VERSION_MAJOR
        #define D_ENV_ARANGO_VERSION_MINOR     ARANGODB_VERSION_MINOR
        #define D_ENV_ARANGO_VERSION_PATCH     ARANGODB_VERSION_PATCH
        #define D_ENV_ARANGO_VERSION_ID        \
            D_ENV_ARANGO_ENCODE_VERSION(ARANGODB_VERSION_MAJOR,  \
                                         ARANGODB_VERSION_MINOR,  \
                                         ARANGODB_VERSION_PATCH)

        #ifdef ARANGODB_VERSION
            #define D_ENV_ARANGO_VERSION_STRING ARANGODB_VERSION
        #else
            #define D_ENV_ARANGO_VERSION_STRING "unknown"
        #endif

    #elif defined(ARANGODB_VERSION)
        // string-only detection; version ID must be derived from the
        // build system or provided manually. Mark as detected but
        // without version gating.
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        0
        #define D_ENV_ARANGO_VERSION_MAJOR     0
        #define D_ENV_ARANGO_VERSION_MINOR     0
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    ARANGODB_VERSION
    #else
        #define D_ENV_ARANGO_DETECTED          0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_ARANGO_DETECTED_VERSION
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_DETECTED_VERSION
        #define D_ENV_ARANGO_VERSION_MAJOR     \
            D_ENV_ARANGO_DECODE_MAJOR(D_ENV_ARANGO_DETECTED_VERSION)
        #define D_ENV_ARANGO_VERSION_MINOR     \
            D_ENV_ARANGO_DECODE_MINOR(D_ENV_ARANGO_DETECTED_VERSION)
        #define D_ENV_ARANGO_VERSION_PATCH     \
            D_ENV_ARANGO_DECODE_PATCH(D_ENV_ARANGO_DETECTED_VERSION)
        #define D_ENV_ARANGO_VERSION_STRING    "manual"

    #elif defined(D_ENV_ARANGO_DETECTED_3_12)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_12_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     12
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.12.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_11)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_11_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     11
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.11.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_10)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_10_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     10
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.10.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_9)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_9_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     9
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.9.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_8)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_8_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     8
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.8.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_7)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_7_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     7
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.7.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_6)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_6_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     6
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.6.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_5)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_5_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     5
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.5.0"

    #elif defined(D_ENV_ARANGO_DETECTED_3_4)
        #define D_ENV_ARANGO_DETECTED          1
        #define D_ENV_ARANGO_VERSION_ID        D_ENV_ARANGO_VERSION_3_4_0
        #define D_ENV_ARANGO_VERSION_MAJOR     3
        #define D_ENV_ARANGO_VERSION_MINOR     4
        #define D_ENV_ARANGO_VERSION_PATCH     0
        #define D_ENV_ARANGO_VERSION_STRING    "3.4.0"

    #else
        #define D_ENV_ARANGO_DETECTED          0
    #endif

#endif  // D_CFG_ENV_ARANGO_CUSTOM


// =============================================================================
// IV.  VERSION COMPARISON MACROS
// =============================================================================

#if D_ENV_ARANGO_DETECTED

    #define D_ENV_ARANGO_VERSION_AT_LEAST(major, minor, patch) \
        (D_ENV_ARANGO_VERSION_ID >= \
            D_ENV_ARANGO_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_ARANGO_VERSION_BELOW(major, minor, patch) \
        (D_ENV_ARANGO_VERSION_ID < \
            D_ENV_ARANGO_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_ARANGO_VERSION_EXACT(major, minor, patch) \
        (D_ENV_ARANGO_VERSION_ID == \
            D_ENV_ARANGO_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_ARANGO_VERSION_IN_RANGE(min_maj, min_min, min_pat,     \
                                           max_maj, max_min, max_pat)     \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(min_maj, min_min, min_pat) &&    \
          D_ENV_ARANGO_VERSION_BELOW(max_maj, max_min, max_pat) )

    // series macros
    #define D_ENV_ARANGO_IS_3_4 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 4, 0, 3, 5, 0)
    #define D_ENV_ARANGO_IS_3_5 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 5, 0, 3, 6, 0)
    #define D_ENV_ARANGO_IS_3_6 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 6, 0, 3, 7, 0)
    #define D_ENV_ARANGO_IS_3_7 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 7, 0, 3, 8, 0)
    #define D_ENV_ARANGO_IS_3_8 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 8, 0, 3, 9, 0)
    #define D_ENV_ARANGO_IS_3_9 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 9, 0, 3, 10, 0)
    #define D_ENV_ARANGO_IS_3_10 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 10, 0, 3, 11, 0)
    #define D_ENV_ARANGO_IS_3_11 \
        D_ENV_ARANGO_VERSION_IN_RANGE(3, 11, 0, 3, 12, 0)
    #define D_ENV_ARANGO_IS_3_12 \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 12, 0)


// =============================================================================
// V.   EDITION DETECTION (COMMUNITY vs ENTERPRISE)
// =============================================================================
//   ArangoDB ships in two editions. The Enterprise edition includes
// SmartGraphs, encryption at rest, LDAP authentication, DC2DC replication,
// hot backup, and other advanced clustering features. The Community
// edition is open-source with Apache 2.0 license (changed to BSL 1.1
// in 3.10.2 for server, driver remains Apache 2.0).

    // D_ENV_ARANGO_IS_ENTERPRISE
    //   detection: 1 if Enterprise edition is detected.
    #ifndef D_ENV_ARANGO_IS_ENTERPRISE
        #if ( defined(USE_ENTERPRISE)            ||  \
              defined(ARANGODB_ENTERPRISE)       ||  \
              defined(D_ENV_ARANGO_DETECTED_ENTERPRISE) )
            #define D_ENV_ARANGO_IS_ENTERPRISE 1
        #else
            #define D_ENV_ARANGO_IS_ENTERPRISE 0
        #endif
    #endif

    // D_ENV_ARANGO_IS_COMMUNITY
    //   detection: 1 if Community edition (not Enterprise).
    #define D_ENV_ARANGO_IS_COMMUNITY \
        (!D_ENV_ARANGO_IS_ENTERPRISE)

    // D_ENV_ARANGO_LICENSE_IS_BSL
    //   status: 1 if the server is under Business Source License (3.10.2+
    // for server code). Does not affect client drivers.
    #define D_ENV_ARANGO_LICENSE_IS_BSL \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 2)


// =============================================================================
// VI.  CLIENT DRIVER AND PROTOCOL DETECTION
// =============================================================================

    // D_ENV_ARANGO_HAS_FUERTE
    //   feature: detect if the C++ fuerte HTTP/VST driver is available.
    #ifndef D_ENV_ARANGO_HAS_FUERTE
        #if ( defined(FUERTE_VERSION)    ||  \
              defined(FUERTE_VERSION_ID) )
            #define D_ENV_ARANGO_HAS_FUERTE 1
        #else
            #define D_ENV_ARANGO_HAS_FUERTE 0
        #endif
    #endif

    // D_ENV_ARANGO_HAS_VELOCYPACK
    //   feature: detect if VelocyPack (VPack) binary serialization
    // library is available. VelocyPack is ArangoDB's compact binary
    // JSON-compatible format used on the wire and for internal storage.
    #ifndef D_ENV_ARANGO_HAS_VELOCYPACK
        #if ( defined(VELOCYPACK_VERSION)      ||  \
              defined(VELOCYPACK_HAS_BUILDER)  ||  \
              defined(VELOCYPACK_VELOCYPACK_H) )
            #define D_ENV_ARANGO_HAS_VELOCYPACK 1
        #else
            #define D_ENV_ARANGO_HAS_VELOCYPACK 0
        #endif
    #endif

    // D_ENV_ARANGO_HAS_VST_PROTOCOL
    //   feature: VelocyStream (VST) binary protocol for client-server
    // communication (alternative to HTTP). Available since ArangoDB 3.0.
    // Deprecated in favor of HTTP/2 in 3.12.
    #ifndef D_ENV_ARANGO_HAS_VST_PROTOCOL
        #if D_ENV_ARANGO_VERSION_BELOW(3, 12, 0)
            #define D_ENV_ARANGO_HAS_VST_PROTOCOL 1
        #else
            #define D_ENV_ARANGO_HAS_VST_PROTOCOL 0
        #endif
    #endif

    // D_ENV_ARANGO_VST_DEPRECATED
    //   status: 1 if VST protocol is deprecated (3.12+).
    #define D_ENV_ARANGO_VST_DEPRECATED \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 12, 0)

    // D_ENV_ARANGO_HAS_HTTP2
    //   feature: HTTP/2 protocol support for client-server communication.
    // Introduced in ArangoDB 3.7.1.
    #define D_ENV_ARANGO_HAS_HTTP2 \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 1)

    // D_ENV_ARANGO_HAS_CURSOR_API
    //   feature: cursor-based result set iteration via HTTP API.
    // Core feature present in all modern versions.
    #define D_ENV_ARANGO_HAS_CURSOR_API D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_BATCH_API
    //   feature: batch request API (multiple operations in one HTTP
    // request). Core feature.
    #define D_ENV_ARANGO_HAS_BATCH_API D_ENV_ARANGO_DETECTED


// =============================================================================
// VII. STORAGE ENGINE
// =============================================================================

    // D_ENV_ARANGO_HAS_ROCKSDB
    //   feature: RocksDB storage engine. Available since 3.2, sole engine
    // since 3.7 (MMFiles was removed).
    #define D_ENV_ARANGO_HAS_ROCKSDB D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_REMOVED_MMFILES
    //   status: 1 if MMFiles storage engine has been removed (3.7+).
    #define D_ENV_ARANGO_REMOVED_MMFILES \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0)

    // D_ENV_ARANGO_HAS_MMFILES
    //   feature: detect if MMFiles engine is still available (< 3.7).
    #ifndef D_ENV_ARANGO_HAS_MMFILES
        #if D_ENV_ARANGO_VERSION_BELOW(3, 7, 0)
            #define D_ENV_ARANGO_HAS_MMFILES 1
        #else
            #define D_ENV_ARANGO_HAS_MMFILES 0
        #endif
    #endif

    // D_ENV_ARANGO_HAS_COMPRESSION
    //   feature: RocksDB column family compression (LZ4, Snappy, zstd).
    // Configurable since 3.4.
    #define D_ENV_ARANGO_HAS_COMPRESSION \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)


// =============================================================================
// VIII. INDEX TYPES
// =============================================================================

    // D_ENV_ARANGO_HAS_INDEX_PERSISTENT
    //   feature: persistent (sorted, RocksDB-backed) index. This is the
    // primary index type since the move to RocksDB. Replaced the old
    // "hash" and "skiplist" index types in 3.9; those names became
    // aliases for persistent.
    #define D_ENV_ARANGO_HAS_INDEX_PERSISTENT D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_INDEX_HASH_IS_ALIAS
    //   status: 1 if "hash" index type is an alias for persistent.
    // The distinction was removed in 3.9.
    #define D_ENV_ARANGO_INDEX_HASH_IS_ALIAS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)

    // D_ENV_ARANGO_INDEX_SKIPLIST_IS_ALIAS
    //   status: 1 if "skiplist" index type is an alias for persistent.
    #define D_ENV_ARANGO_INDEX_SKIPLIST_IS_ALIAS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)

    // D_ENV_ARANGO_HAS_INDEX_GEO
    //   feature: geo-spatial index (S2-based). Present in all modern
    // versions. Supports GeoJSON objects and legacy coordinate pairs.
    #define D_ENV_ARANGO_HAS_INDEX_GEO D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_INDEX_FULLTEXT
    //   feature: legacy fulltext index. Present but deprecated since 3.10
    // in favor of ArangoSearch and inverted indexes.
    #define D_ENV_ARANGO_HAS_INDEX_FULLTEXT D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_INDEX_FULLTEXT_DEPRECATED
    //   status: 1 if legacy fulltext index is deprecated (3.10+).
    #define D_ENV_ARANGO_INDEX_FULLTEXT_DEPRECATED \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_INDEX_TTL
    //   feature: TTL (time-to-live) index for automatic document
    // expiration. Introduced in ArangoDB 3.5.
    #define D_ENV_ARANGO_HAS_INDEX_TTL \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 0)

    // D_ENV_ARANGO_HAS_INDEX_INVERTED
    //   feature: inverted index (standalone, outside of Views).
    // Introduced in ArangoDB 3.10, enhanced in 3.11+.
    #define D_ENV_ARANGO_HAS_INDEX_INVERTED \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_INDEX_MDI
    //   feature: multi-dimensional index (MDI / ZKD - Z-order Kurve
    // Decomposition) for multi-attribute range queries.
    // Introduced in ArangoDB 3.10 (experimental), stable in 3.12.
    #define D_ENV_ARANGO_HAS_INDEX_MDI \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_INDEX_MDI_PREFIXED
    //   feature: prefixed multi-dimensional index (MDI with a prefix
    // of regular persistent index fields). Introduced in 3.12.
    #define D_ENV_ARANGO_HAS_INDEX_MDI_PREFIXED \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 12, 0)

    // D_ENV_ARANGO_HAS_STORED_VALUES
    //   feature: storedValues on persistent indexes (covering index
    // capability, avoiding document lookups). Introduced in 3.10.
    #define D_ENV_ARANGO_HAS_STORED_VALUES \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_CACHE_ON_INDEX
    //   feature: per-index in-memory caching for persistent indexes.
    // Introduced in 3.10.
    #define D_ENV_ARANGO_HAS_CACHE_ON_INDEX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)


// =============================================================================
// IX.  ARANGOSEARCH AND VIEWS
// =============================================================================
//   ArangoSearch is built on the IResearch library and provides full-text
// search, ranking, and complex filtering capabilities via Views.

    // D_ENV_ARANGO_HAS_ARANGOSEARCH
    //   feature: ArangoSearch Views (arangosearch type). Introduced in
    // ArangoDB 3.4 using the IResearch engine.
    #define D_ENV_ARANGO_HAS_ARANGOSEARCH \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)

    // D_ENV_ARANGO_HAS_SEARCH_ALIAS_VIEWS
    //   feature: search-alias Views (lightweight Views backed by inverted
    // indexes on the collections, without separate data copies).
    // Introduced in ArangoDB 3.10.
    #define D_ENV_ARANGO_HAS_SEARCH_ALIAS_VIEWS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_ANALYZERS
    //   feature: custom analyzers framework (text, norm, stem, ngram,
    // delimiter, pipeline, etc.). Introduced with ArangoSearch in 3.4.
    #define D_ENV_ARANGO_HAS_ANALYZERS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)

    // D_ENV_ARANGO_HAS_ANALYZER_PIPELINE
    //   feature: pipeline analyzers (chaining multiple analyzers).
    // Introduced in ArangoDB 3.8.
    #define D_ENV_ARANGO_HAS_ANALYZER_PIPELINE \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)

    // D_ENV_ARANGO_HAS_ANALYZER_AQL
    //   feature: AQL analyzer type (applying AQL expressions as
    // analyzers for computed fields in Views). Introduced in 3.8.
    #define D_ENV_ARANGO_HAS_ANALYZER_AQL \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)

    // D_ENV_ARANGO_HAS_ANALYZER_GEO
    //   feature: geo analyzers (geojson, geopoint) for spatial queries
    // via ArangoSearch. Introduced in 3.8.
    #define D_ENV_ARANGO_HAS_ANALYZER_GEO \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)

    // D_ENV_ARANGO_HAS_ANALYZER_CLASSIFICATION
    //   feature: classification and nearest_neighbors analyzers for
    // ML-based text classification. Introduced in 3.10 (Enterprise).
    #define D_ENV_ARANGO_HAS_ANALYZER_CLASSIFICATION \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_ANALYZER_MINHASH
    //   feature: minhash analyzer for approximate set similarity.
    // Introduced in ArangoDB 3.10 (Enterprise).
    #define D_ENV_ARANGO_HAS_ANALYZER_MINHASH \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_NESTED_SEARCH
    //   feature: nested search (querying into nested arrays/objects with
    // correct conjunctive semantics across sub-documents).
    // Introduced in ArangoDB 3.10.
    #define D_ENV_ARANGO_HAS_NESTED_SEARCH \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_SEARCH_HIGHLIGHT
    //   feature: SEARCH highlighting (returning matched fragments with
    // surrounding context). Introduced in ArangoDB 3.11.
    #define D_ENV_ARANGO_HAS_SEARCH_HIGHLIGHT \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 11, 0)

    // D_ENV_ARANGO_HAS_SEARCH_OFFSET_INFO
    //   feature: offset information for SEARCH matches (start/length
    // positions in matched fields). Introduced in ArangoDB 3.11.
    #define D_ENV_ARANGO_HAS_SEARCH_OFFSET_INFO \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 11, 0)

    // D_ENV_ARANGO_HAS_SCORING_FUNCTIONS
    //   feature: BM25() and TFIDF() scoring functions in AQL for
    // relevance ranking in SEARCH queries. Available since 3.4.
    #define D_ENV_ARANGO_HAS_SCORING_FUNCTIONS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)


// =============================================================================
// X.   AQL (ARANGODB QUERY LANGUAGE) FEATURES
// =============================================================================
//   AQL is ArangoDB's native query language - declarative, but not SQL.
// It supports document operations, graph traversals, joins, aggregation,
// sub-queries, and data modification (INSERT, UPDATE, REPLACE, REMOVE,
// UPSERT) all in a single language.

    // D_ENV_ARANGO_HAS_AQL
    //   feature: AQL is always available. Core feature.
    #define D_ENV_ARANGO_HAS_AQL D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AQL_UPSERT
    //   feature: UPSERT operation in AQL. Present in all 3.x versions.
    #define D_ENV_ARANGO_HAS_AQL_UPSERT D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AQL_INSERT_UPDATE
    //   feature: INSERT ... OPTIONS { overwriteMode: "update" } for
    // insert-or-update semantics. Introduced in 3.7.
    #define D_ENV_ARANGO_HAS_AQL_INSERT_UPDATE \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0)

    // D_ENV_ARANGO_HAS_AQL_SUBQUERY_OPTIMIZATION
    //   feature: sub-query splicing optimization (inlining simple
    // sub-queries). Enhanced in 3.8+.
    #define D_ENV_ARANGO_HAS_AQL_SUBQUERY_OPTIMIZATION \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)

    // D_ENV_ARANGO_HAS_AQL_WINDOW
    //   feature: WINDOW clause for cumulative/sliding-window aggregations
    // in AQL (analogous to SQL window functions). Introduced in 3.8.
    #define D_ENV_ARANGO_HAS_AQL_WINDOW \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)

    // D_ENV_ARANGO_HAS_AQL_COLLECT_AGGREGATE
    //   feature: COLLECT ... AGGREGATE syntax for grouped aggregation.
    // Present in all 3.x versions.
    #define D_ENV_ARANGO_HAS_AQL_COLLECT_AGGREGATE D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AQL_GRAPH_TRAVERSAL
    //   feature: graph traversal syntax (FOR v, e, p IN ... GRAPH ...).
    // Core AQL feature.
    #define D_ENV_ARANGO_HAS_AQL_GRAPH_TRAVERSAL D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AQL_SHORTEST_PATH
    //   feature: SHORTEST_PATH query in AQL. Core feature.
    #define D_ENV_ARANGO_HAS_AQL_SHORTEST_PATH D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AQL_K_SHORTEST_PATHS
    //   feature: K_SHORTEST_PATHS query in AQL. Introduced in 3.5.
    #define D_ENV_ARANGO_HAS_AQL_K_SHORTEST_PATHS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 0)

    // D_ENV_ARANGO_HAS_AQL_K_PATHS
    //   feature: K_PATHS query (all paths between source and target).
    // Introduced in 3.9.
    #define D_ENV_ARANGO_HAS_AQL_K_PATHS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)

    // D_ENV_ARANGO_HAS_AQL_ALL_SHORTEST_PATHS
    //   feature: ALL_SHORTEST_PATHS query. Introduced in 3.9.
    #define D_ENV_ARANGO_HAS_AQL_ALL_SHORTEST_PATHS \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)

    // D_ENV_ARANGO_HAS_AQL_SEARCH_FUNCTION
    //   feature: SEARCH keyword in AQL (filtering on ArangoSearch Views
    // and inverted indexes). Available since ArangoSearch in 3.4.
    #define D_ENV_ARANGO_HAS_AQL_SEARCH_FUNCTION \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)

    // D_ENV_ARANGO_HAS_AQL_PRUNE
    //   feature: PRUNE clause for graph traversals (early termination
    // of traversal branches). Introduced in 3.4.5.
    #define D_ENV_ARANGO_HAS_AQL_PRUNE \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 5)

    // D_ENV_ARANGO_HAS_AQL_COMPUTED_VALUES
    //   feature: computed values (server-side computed attributes set
    // on INSERT/UPDATE/REPLACE). Introduced in 3.10.
    #define D_ENV_ARANGO_HAS_AQL_COMPUTED_VALUES \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_AQL_EXPLAIN_PROFILE
    //   feature: AQL query profiling (EXPLAIN with actual runtime data).
    // Enhanced in 3.5+.
    #define D_ENV_ARANGO_HAS_AQL_EXPLAIN_PROFILE \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 0)

    // D_ENV_ARANGO_HAS_AQL_LATE_MATERIALIZATION
    //   feature: late document materialization optimization (deferred
    // full document fetches until actually needed). Introduced in 3.10.
    #define D_ENV_ARANGO_HAS_AQL_LATE_MATERIALIZATION \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)


// =============================================================================
// XI.  GRAPH FEATURES
// =============================================================================

    // D_ENV_ARANGO_HAS_NAMED_GRAPHS
    //   feature: named graph management (CREATE/DROP/MODIFY graph API).
    #define D_ENV_ARANGO_HAS_NAMED_GRAPHS D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_SMART_GRAPHS
    //   feature: SmartGraphs (enterprise graphs with data locality for
    // graph traversals across shards). Enterprise only. Introduced in 3.4.
    #define D_ENV_ARANGO_HAS_SMART_GRAPHS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_ENTERPRISE_GRAPHS
    //   feature: EnterpriseGraphs (automatically sharded graphs without
    // manual SmartGraph attribute selection). Enterprise only.
    // Introduced in 3.10.
    #define D_ENV_ARANGO_HAS_ENTERPRISE_GRAPHS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_SATELLITE_GRAPHS
    //   feature: SatelliteGraphs (graph replicated to every DB server
    // for local traversals). Enterprise only. Introduced in 3.7.
    #define D_ENV_ARANGO_HAS_SATELLITE_GRAPHS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_DISJOINT_SMART_GRAPHS
    //   feature: DisjointSmartGraphs (SmartGraphs where edge collections
    // are disjoint for better performance). Enterprise only.
    // Introduced in 3.7.
    #define D_ENV_ARANGO_HAS_DISJOINT_SMART_GRAPHS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_HYBRID_SMART_GRAPHS
    //   feature: HybridSmartGraphs (SmartGraphs that also use
    // SatelliteCollections for certain vertex collections).
    // Enterprise only. Introduced in 3.9.
    #define D_ENV_ARANGO_HAS_HYBRID_SMART_GRAPHS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_PREGEL
    //   feature: Pregel graph processing framework (distributed iterative
    // graph algorithms: PageRank, community detection, etc.).
    // Introduced in 3.4.
    #define D_ENV_ARANGO_HAS_PREGEL \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)


// =============================================================================
// XII. TRANSACTIONS
// =============================================================================

    // D_ENV_ARANGO_HAS_SINGLE_DOC_TRX
    //   feature: single-document ACID transactions. Core feature (all
    // single-document operations are atomic by default).
    #define D_ENV_ARANGO_HAS_SINGLE_DOC_TRX D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_JS_TRANSACTIONS
    //   feature: JavaScript (server-side) multi-collection transactions.
    // Core feature.
    #define D_ENV_ARANGO_HAS_JS_TRANSACTIONS D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_STREAMING_TRX
    //   feature: streaming (HTTP-based) multi-document transactions
    // with explicit BEGIN/COMMIT/ABORT via REST API.
    // Introduced in ArangoDB 3.5.
    #define D_ENV_ARANGO_HAS_STREAMING_TRX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 0)

    // D_ENV_ARANGO_HAS_AQL_MULTI_DOC_TRX
    //   feature: implicit multi-document transactions within a single
    // AQL query (the query runs as one atomic operation for all
    // modifications). Available since 3.4.
    #define D_ENV_ARANGO_HAS_AQL_MULTI_DOC_TRX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)

    // D_ENV_ARANGO_HAS_CLUSTER_TRX
    //   feature: cluster-wide multi-shard transactions (intermediate
    // commits within AQL on clusters). Enhanced in 3.9+.
    #define D_ENV_ARANGO_HAS_CLUSTER_TRX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)


// =============================================================================
// XIII. REPLICATION AND CLUSTERING
// =============================================================================

    // D_ENV_ARANGO_HAS_CLUSTER
    //   feature: ArangoDB cluster deployment (Coordinators, DB-Servers,
    // Agents). Core clustering architecture present since 3.0.
    #define D_ENV_ARANGO_HAS_CLUSTER D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_ACTIVE_FAILOVER
    //   feature: Active Failover (single-server HA with automatic
    // leader election). Introduced in ArangoDB 3.3.
    #define D_ENV_ARANGO_HAS_ACTIVE_FAILOVER \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 3, 0)

    // D_ENV_ARANGO_HAS_ONESHARD
    //   feature: OneShard deployment (all collections of a database on
    // a single shard for local join performance). Introduced in 3.6.
    #define D_ENV_ARANGO_HAS_ONESHARD \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 6, 0)

    // D_ENV_ARANGO_HAS_SATELLITE_COLLECTIONS
    //   feature: SatelliteCollections (collections replicated to every
    // DB server for shard-local joins). Enterprise only. Introduced in 3.4.
    #define D_ENV_ARANGO_HAS_SATELLITE_COLLECTIONS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_SMART_JOINS
    //   feature: SmartJoins (shard-local joins on identically sharded
    // collections). Enterprise only. Introduced in 3.5.
    #define D_ENV_ARANGO_HAS_SMART_JOINS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_DC2DC_REPL
    //   feature: datacenter-to-datacenter replication (asynchronous cross-
    // datacenter replication). Enterprise only. Introduced in 3.3.
    #define D_ENV_ARANGO_HAS_DC2DC_REPL \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 3, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_SYNC_REPL
    //   feature: synchronous replication (configurable replication factor
    // per collection within a cluster). Core cluster feature.
    #define D_ENV_ARANGO_HAS_SYNC_REPL D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_WRITE_CONCERN
    //   feature: write concern (minimum number of in-sync replicas
    // required before acknowledging a write). Introduced in 3.6.
    #define D_ENV_ARANGO_HAS_WRITE_CONCERN \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 6, 0)


// =============================================================================
// XIV. SECURITY AND AUTHENTICATION
// =============================================================================

    // D_ENV_ARANGO_HAS_AUTH_JWT
    //   feature: JWT (JSON Web Token) authentication for HTTP API.
    // Core authentication method.
    #define D_ENV_ARANGO_HAS_AUTH_JWT D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AUTH_BASIC
    //   feature: HTTP Basic authentication. Core feature.
    #define D_ENV_ARANGO_HAS_AUTH_BASIC D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_AUTH_LDAP
    //   feature: LDAP authentication and authorization.
    // Enterprise only. Available since 3.4.
    #define D_ENV_ARANGO_HAS_AUTH_LDAP \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_AUTH_KERBEROS
    //   feature: Kerberos authentication. Enterprise only.
    // Introduced in ArangoDB 3.7.
    #define D_ENV_ARANGO_HAS_AUTH_KERBEROS \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_SSL
    //   feature: SSL/TLS for client-server encryption in transit.
    // Core feature, always available.
    #define D_ENV_ARANGO_HAS_SSL D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_ENCRYPTION_AT_REST
    //   feature: encryption at rest (RocksDB encryption).
    // Enterprise only. Available since 3.4.
    #define D_ENV_ARANGO_HAS_ENCRYPTION_AT_REST \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_KEY_ROTATION
    //   feature: encryption key rotation for at-rest encryption.
    // Enterprise only. Introduced in 3.7.
    #define D_ENV_ARANGO_HAS_KEY_ROTATION \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_AUDIT_LOG
    //   feature: audit logging. Enterprise only. Available since 3.4.
    #define D_ENV_ARANGO_HAS_AUDIT_LOG \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_JWT_SECRET_ROTATION
    //   feature: JWT secret rotation (reloading secrets without restart).
    // Introduced in 3.7.
    #define D_ENV_ARANGO_HAS_JWT_SECRET_ROTATION \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0)


// =============================================================================
// XV.  FOXX MICROSERVICES
// =============================================================================

    // D_ENV_ARANGO_HAS_FOXX
    //   feature: Foxx microservices framework (server-side JavaScript
    // services running inside ArangoDB). Core feature.
    #define D_ENV_ARANGO_HAS_FOXX D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_FOXX_QUEUES
    //   feature: Foxx job queues (background task execution).
    #define D_ENV_ARANGO_HAS_FOXX_QUEUES D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_FOXX_TYPESCRIPT
    //   feature: TypeScript support in Foxx services. Introduced in 3.4.
    #define D_ENV_ARANGO_HAS_FOXX_TYPESCRIPT \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 4, 0)


// =============================================================================
// XVI. BACKUP AND RESTORE
// =============================================================================

    // D_ENV_ARANGO_HAS_ARANGODUMP
    //   feature: arangodump / arangorestore utilities for logical backup.
    // Core tooling.
    #define D_ENV_ARANGO_HAS_ARANGODUMP D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_HOT_BACKUP
    //   feature: hot backup API (consistent cluster-wide snapshot without
    // downtime). Enterprise only. Introduced in 3.5.1.
    #define D_ENV_ARANGO_HAS_HOT_BACKUP \
        ( D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 1) && \
          D_ENV_ARANGO_IS_ENTERPRISE )

    // D_ENV_ARANGO_HAS_ARANGOEXPORT
    //   feature: arangoexport utility (JSONL/CSV/XML/XGMML export).
    #define D_ENV_ARANGO_HAS_ARANGOEXPORT D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_DUMP_PARALLEL
    //   feature: parallel dump/restore (multi-threaded arangodump).
    // Enhanced parallelism introduced in 3.8+.
    #define D_ENV_ARANGO_HAS_DUMP_PARALLEL \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)


// =============================================================================
// XVII. COLLECTION AND SCHEMA FEATURES
// =============================================================================

    // D_ENV_ARANGO_HAS_DOCUMENT_COLLECTIONS
    //   feature: document collections. Core data model.
    #define D_ENV_ARANGO_HAS_DOCUMENT_COLLECTIONS D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_EDGE_COLLECTIONS
    //   feature: edge collections (for graph relationships). Core.
    #define D_ENV_ARANGO_HAS_EDGE_COLLECTIONS D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_SCHEMA_VALIDATION
    //   feature: JSON Schema validation on collections (enforcing
    // document structure at write time). Introduced in 3.7.
    #define D_ENV_ARANGO_HAS_SCHEMA_VALIDATION \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 7, 0)

    // D_ENV_ARANGO_HAS_COMPUTED_VALUES
    //   feature: computed values (server-side computed attributes).
    // Introduced in 3.10.
    #define D_ENV_ARANGO_HAS_COMPUTED_VALUES \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_HAS_COLLECTION_SHARDING
    //   feature: collection sharding (distributing data across DB
    // servers by shard key). Core cluster feature.
    #define D_ENV_ARANGO_HAS_COLLECTION_SHARDING D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_KEY_GENERATORS
    //   feature: configurable key generators (traditional, autoincrement,
    // uuid, padded). Core feature.
    #define D_ENV_ARANGO_HAS_KEY_GENERATORS D_ENV_ARANGO_DETECTED


// =============================================================================
// XVIII. OPTIMIZER AND DIAGNOSTICS
// =============================================================================

    // D_ENV_ARANGO_HAS_AQL_OPTIMIZER
    //   feature: AQL query optimizer (rule-based optimization with
    // configurable rules). Core feature.
    #define D_ENV_ARANGO_HAS_AQL_OPTIMIZER D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_QUERY_PROFILING
    //   feature: per-query runtime profiling (execution statistics per
    // node in the query plan). Enhanced in 3.5+.
    #define D_ENV_ARANGO_HAS_QUERY_PROFILING \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 5, 0)

    // D_ENV_ARANGO_HAS_QUERY_CACHE
    //   feature: AQL query results cache. Present in all modern versions;
    // works in single-server mode.
    #define D_ENV_ARANGO_HAS_QUERY_CACHE D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_SLOW_QUERY_LOG
    //   feature: slow query log (logging queries exceeding a time
    // threshold). Core feature.
    #define D_ENV_ARANGO_HAS_SLOW_QUERY_LOG D_ENV_ARANGO_DETECTED

    // D_ENV_ARANGO_HAS_METRICS_API
    //   feature: Prometheus-compatible metrics endpoint. Introduced
    // in 3.8 (/_admin/metrics/v2).
    #define D_ENV_ARANGO_HAS_METRICS_API \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 8, 0)


// =============================================================================
// XIX.  CONVENIENCE / COMPOSITE MACROS
// =============================================================================

    // D_ENV_ARANGO_HAS_MODERN_SEARCH
    //   macro: evaluates to 1 if ArangoSearch, analyzers, search-alias
    // Views, inverted indexes, and nested search are all available.
    #define D_ENV_ARANGO_HAS_MODERN_SEARCH \
        ( D_ENV_ARANGO_HAS_ARANGOSEARCH       && \
          D_ENV_ARANGO_HAS_ANALYZERS          && \
          D_ENV_ARANGO_HAS_SEARCH_ALIAS_VIEWS && \
          D_ENV_ARANGO_HAS_INDEX_INVERTED     && \
          D_ENV_ARANGO_HAS_NESTED_SEARCH )

    // D_ENV_ARANGO_HAS_MODERN_AQL
    //   macro: evaluates to 1 if AQL window functions, late
    // materialization, and computed values are all available.
    #define D_ENV_ARANGO_HAS_MODERN_AQL \
        ( D_ENV_ARANGO_HAS_AQL_WINDOW              && \
          D_ENV_ARANGO_HAS_AQL_LATE_MATERIALIZATION && \
          D_ENV_ARANGO_HAS_AQL_COMPUTED_VALUES )

    // D_ENV_ARANGO_HAS_MODERN_GRAPH
    //   macro: evaluates to 1 if K_PATHS, ALL_SHORTEST_PATHS, and Pregel
    // are all available.
    #define D_ENV_ARANGO_HAS_MODERN_GRAPH \
        ( D_ENV_ARANGO_HAS_AQL_K_PATHS             && \
          D_ENV_ARANGO_HAS_AQL_ALL_SHORTEST_PATHS  && \
          D_ENV_ARANGO_HAS_PREGEL )

    // D_ENV_ARANGO_HAS_MODERN_CLUSTER
    //   macro: evaluates to 1 if OneShard, write concern, and cluster
    // transactions are all available.
    #define D_ENV_ARANGO_HAS_MODERN_CLUSTER \
        ( D_ENV_ARANGO_HAS_ONESHARD     && \
          D_ENV_ARANGO_HAS_WRITE_CONCERN && \
          D_ENV_ARANGO_HAS_CLUSTER_TRX )

    // D_ENV_ARANGO_HAS_ENTERPRISE_SUITE
    //   macro: evaluates to 1 if all core Enterprise features are
    // available (SmartGraphs, encryption, LDAP, hot backup, audit log).
    #define D_ENV_ARANGO_HAS_ENTERPRISE_SUITE \
        ( D_ENV_ARANGO_HAS_SMART_GRAPHS          && \
          D_ENV_ARANGO_HAS_ENCRYPTION_AT_REST    && \
          D_ENV_ARANGO_HAS_AUTH_LDAP             && \
          D_ENV_ARANGO_HAS_HOT_BACKUP           && \
          D_ENV_ARANGO_HAS_AUDIT_LOG )

    // D_ENV_ARANGO_IS_FULLY_MODERN
    //   macro: evaluates to 1 if ArangoDB has a comprehensive modern
    // feature set (roughly 3.10+ with search, AQL, graph, and cluster).
    #define D_ENV_ARANGO_IS_FULLY_MODERN \
        ( D_ENV_ARANGO_HAS_MODERN_SEARCH  && \
          D_ENV_ARANGO_HAS_MODERN_AQL     && \
          D_ENV_ARANGO_HAS_MODERN_GRAPH   && \
          D_ENV_ARANGO_HAS_MODERN_CLUSTER && \
          D_ENV_ARANGO_HAS_STREAMING_TRX )


// =============================================================================
// XX.   DEPRECATION AND REMOVAL
// =============================================================================

    // D_ENV_ARANGO_REMOVED_MMFILES
    //   status: already defined above (VII).

    // D_ENV_ARANGO_DEPRECATED_FULLTEXT_INDEX
    //   status: 1 if legacy fulltext index is deprecated (use
    // ArangoSearch / inverted index instead). 3.10+.
    #define D_ENV_ARANGO_DEPRECATED_FULLTEXT_INDEX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 10, 0)

    // D_ENV_ARANGO_DEPRECATED_VST
    //   status: alias from VI above.

    // D_ENV_ARANGO_DEPRECATED_HASH_INDEX
    //   status: 1 if "hash" index type name is deprecated (now alias for
    // persistent). 3.9+.
    #define D_ENV_ARANGO_DEPRECATED_HASH_INDEX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)

    // D_ENV_ARANGO_DEPRECATED_SKIPLIST_INDEX
    //   status: 1 if "skiplist" index type name is deprecated (now alias
    // for persistent). 3.9+.
    #define D_ENV_ARANGO_DEPRECATED_SKIPLIST_INDEX \
        D_ENV_ARANGO_VERSION_AT_LEAST(3, 9, 0)


#endif  // D_ENV_ARANGO_DETECTED


#endif  // DJINTERP_ENVIRONMENT_ARANGODB_
