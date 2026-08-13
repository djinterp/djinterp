/******************************************************************************
* djinterp [db]                                                  env_mongodb.h
*
* djinterp MongoDB environmental detection header:
* This header provides comprehensive compile-time detection of MongoDB
* environments, capabilities, and version-gated features, including:
*   - C driver (libmongoc) and BSON library (libbson) version detection
*   - target server version detection for feature gating
*   - Community vs Enterprise edition detection
*   - BSON type system and document model detection
*   - index type detection (B-tree, hashed, text, 2dsphere, 2d, wildcard,
*     compound wildcard, partial, TTL, unique, sparse, clustered, columnstore)
*   - aggregation pipeline feature detection ($lookup, $graphLookup,
*     $merge, $unionWith, $setWindowFields, $densify, $fill, $documents)
*   - transaction support (single-document, replica-set multi-document,
*     distributed/sharded transactions)
*   - change stream detection and version-gated enhancements
*   - replica set and sharding features (read/write concern, read
*     preference, causal consistency, hedged reads, resharding)
*   - time series collection detection
*   - GridFS detection
*   - client-side field-level encryption (CSFLE) and Queryable Encryption
*   - Atlas Search and Vector Search detection
*   - authentication method detection (SCRAM, x.509, LDAP, Kerberos)
*   - SSL/TLS and transport encryption
*   - connection pooling, SRV discovery, and URI options
*   - server monitoring (SDAM) and command monitoring
*   - versioned API (Stable API) detection
*   - query and command features ($expr, $jsonSchema, $sample, etc.)
*
*   MongoDB is a document-oriented database using BSON (Binary JSON) as its
* native data format. The C driver (libmongoc) is the official low-level
* client library; it communicates over the MongoDB wire protocol. Feature
* availability depends on both the driver version (compile-time) and the
* target server version (runtime). This header detects the driver version
* at compile time and provides server-version macros that can be manually
* set to gate server-dependent features.
*
*   VERSION ENCODING:
*   Both the driver and server use semantic versioning (MAJOR.MINOR.PATCH).
* This header encodes versions as MAJOR*10000 + MINOR*100 + PATCH,
* consistent with the project convention.
*
*   NAMING CONVENTION:
*   D_ENV_MONGO_[CATEGORY]_[FEATURE]   - 1 if available, 0 otherwise
*   D_ENV_MONGO_DRIVER_[COMPONENT]     - driver version components
*   D_ENV_MONGO_SERVER_[COMPONENT]     - target server version components
*   D_ENV_MONGO_HAS_[CAPABILITY]       - capability flag (1/0)
*
*   DUAL-VERSION MODEL:
*   D_ENV_MONGO_DRIVER_* macros describe the compile-time driver version.
*   D_ENV_MONGO_SERVER_* macros describe the TARGET server version for
* feature gating. Because the server version is a runtime property, the
* SERVER macros must be set manually (via D_ENV_MONGO_DETECTED_SERVER_*)
* or by configuring D_CFG_ENV_MONGO_SERVER_VERSION. If no server version
* is configured, driver-only features are detected and server-gated
* features default to the minimum version the driver supports.
*
* 
* path:      /inc/djinterp/core/env/db/mongodb/env_mongodb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_MONGODB_
#define DJINTERP_ENVIRONMENT_MONGODB_ 1

// djinterp
#include "../../../../config/core/env/db/mongodb/env_mongodb_config.h"
#include "../env_db.h"


// =============================================================================
// 0.   VENDOR HEADER INCLUSION
// =============================================================================
//   Driven by D_CFG_ENV_USING_MONGODB from env_config.h. When enabled, this
// section includes the MongoDB C driver header (mongoc) and, in C++ builds,
// the optional mongocxx C++ driver header. Detection below is gated on
// D_ENV_MONGODB_HEADER_INCLUDED so that no mongoc symbols are referenced
// unless the header is actually in scope.

// --- C client header (libmongoc) ---
#if (D_CFG_ENV_USING_MONGODB == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MONGODB_C_PATH)
            #include D_CFG_ENV_MONGODB_C_PATH
            #define D_ENV_MONGODB_HEADER_INCLUDED 1
        #elif __has_include(<mongoc/mongoc.h>)
            #include <mongoc/mongoc.h>
            #define D_ENV_MONGODB_HEADER_INCLUDED 1
        #elif __has_include(<mongoc.h>)
            #include <mongoc.h>
            #define D_ENV_MONGODB_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_MONGODB=1 but no mongoc header was "     \
                   "found. Install libmongoc-dev (or equivalent), or "       \
                   "define D_CFG_ENV_MONGODB_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_MONGODB_C_PATH
        #define D_ENV_MONGODB_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_MONGODB_CLIENT_C
        #define D_ENV_DB_HAS_MONGODB_CLIENT_C 1
    #endif

#else
    #define D_ENV_MONGODB_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MONGODB_CLIENT_C
        #define D_ENV_DB_HAS_MONGODB_CLIENT_C 0
    #endif
#endif  // D_CFG_ENV_USING_MONGODB


// --- C++ client header (mongocxx, optional, C++ builds only) ---
#if ( (D_CFG_ENV_USING_MONGODB == 1) && defined(__cplusplus) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MONGODB_CPP_PATH)
            #include D_CFG_ENV_MONGODB_CPP_PATH
            #define D_ENV_MONGODB_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_MONGODB_CLIENT_CPP
                #define D_ENV_DB_HAS_MONGODB_CLIENT_CPP 1
            #endif
        #else
            #define D_ENV_MONGODB_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_MONGODB_CLIENT_CPP
                #define D_ENV_DB_HAS_MONGODB_CLIENT_CPP 0
            #endif
        #endif
    #else
        #define D_ENV_MONGODB_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_MONGODB_CLIENT_CPP
            #define D_ENV_DB_HAS_MONGODB_CLIENT_CPP 0
        #endif
    #endif

#else
    #define D_ENV_MONGODB_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MONGODB_CLIENT_CPP
        #define D_ENV_DB_HAS_MONGODB_CLIENT_CPP 0
    #endif
#endif  // D_CFG_ENV_USING_MONGODB && __cplusplus


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================

//   All D_CFG_* macros for this module live in env_mongodb_config.h,
// pulled in at the top of this file.


// =============================================================================
// II.  VERSION ENCODING
// =============================================================================

// D_ENV_MONGO_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple.
#define D_ENV_MONGO_ENCODE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))

// D_ENV_MONGO_DECODE_MAJOR
//   macro: extracts the major version.
#define D_ENV_MONGO_DECODE_MAJOR(ver) \
    ((ver) / 10000)

// D_ENV_MONGO_DECODE_MINOR
//   macro: extracts the minor version.
#define D_ENV_MONGO_DECODE_MINOR(ver) \
    (((ver) / 100) % 100)

// D_ENV_MONGO_DECODE_PATCH
//   macro: extracts the patch version.
#define D_ENV_MONGO_DECODE_PATCH(ver) \
    ((ver) % 100)


// =============================================================================
// III. DRIVER VERSION DETECTION (LIBMONGOC / LIBBSON)
// =============================================================================
//   The C driver (libmongoc) and BSON library (libbson) are detected from
// their respective header macros. The driver version determines which wire
// protocol features and client-side APIs are available at compile time.

// server version constants for feature gating
#define D_ENV_MONGO_SERVER_3_2         30200
#define D_ENV_MONGO_SERVER_3_4         30400
#define D_ENV_MONGO_SERVER_3_6         30600
#define D_ENV_MONGO_SERVER_4_0         40000
#define D_ENV_MONGO_SERVER_4_2         40200
#define D_ENV_MONGO_SERVER_4_4         40400
#define D_ENV_MONGO_SERVER_5_0         50000
#define D_ENV_MONGO_SERVER_5_1         50100
#define D_ENV_MONGO_SERVER_6_0         60000
#define D_ENV_MONGO_SERVER_7_0         70000
#define D_ENV_MONGO_SERVER_7_1         70100
#define D_ENV_MONGO_SERVER_8_0         80000

#if (D_CFG_ENV_MONGO_CUSTOM == 0)

    // --- driver detection ---
    // automatic detection requires mongoc.h to be in scope; if
    // D_CFG_ENV_USING_MONGODB was not enabled the sentinel is 0 and we skip
    // cleanly (no reference to MONGOC_MAJOR_VERSION).
    #if ( D_ENV_MONGODB_HEADER_INCLUDED  &&  \
          defined(MONGOC_MAJOR_VERSION) )
        #define D_ENV_MONGO_DRIVER_DETECTED    1
        #define D_ENV_MONGO_DRIVER_MAJOR       MONGOC_MAJOR_VERSION
        #define D_ENV_MONGO_DRIVER_MINOR       MONGOC_MINOR_VERSION
        #define D_ENV_MONGO_DRIVER_PATCH       MONGOC_MICRO_VERSION
        #define D_ENV_MONGO_DRIVER_VERSION_ID  \
            D_ENV_MONGO_ENCODE_VERSION(MONGOC_MAJOR_VERSION, \
                                        MONGOC_MINOR_VERSION, \
                                        MONGOC_MICRO_VERSION)
        #ifdef MONGOC_VERSION_S
            #define D_ENV_MONGO_DRIVER_VERSION_STRING MONGOC_VERSION_S
        #else
            #define D_ENV_MONGO_DRIVER_VERSION_STRING "unknown"
        #endif
    #else
        #define D_ENV_MONGO_DRIVER_DETECTED    0
    #endif

    // --- libbson detection ---
    #if defined(BSON_MAJOR_VERSION)
        #define D_ENV_MONGO_BSON_DETECTED      1
        #define D_ENV_MONGO_BSON_MAJOR         BSON_MAJOR_VERSION
        #define D_ENV_MONGO_BSON_MINOR         BSON_MINOR_VERSION
        #define D_ENV_MONGO_BSON_PATCH         BSON_MICRO_VERSION
        #define D_ENV_MONGO_BSON_VERSION_ID    \
            D_ENV_MONGO_ENCODE_VERSION(BSON_MAJOR_VERSION, \
                                        BSON_MINOR_VERSION, \
                                        BSON_MICRO_VERSION)
    #else
        #define D_ENV_MONGO_BSON_DETECTED      0
    #endif

#else
    // manual mode
    #ifdef D_ENV_MONGO_DETECTED_DRIVER_VERSION
        #define D_ENV_MONGO_DRIVER_DETECTED    1
        #define D_ENV_MONGO_DRIVER_VERSION_ID  \
            D_ENV_MONGO_DETECTED_DRIVER_VERSION
        #define D_ENV_MONGO_DRIVER_MAJOR       \
            D_ENV_MONGO_DECODE_MAJOR(D_ENV_MONGO_DETECTED_DRIVER_VERSION)
        #define D_ENV_MONGO_DRIVER_MINOR       \
            D_ENV_MONGO_DECODE_MINOR(D_ENV_MONGO_DETECTED_DRIVER_VERSION)
        #define D_ENV_MONGO_DRIVER_PATCH       \
            D_ENV_MONGO_DECODE_PATCH(D_ENV_MONGO_DETECTED_DRIVER_VERSION)
        #define D_ENV_MONGO_DRIVER_VERSION_STRING "manual"
    #else
        #define D_ENV_MONGO_DRIVER_DETECTED    0
    #endif

    #ifndef D_ENV_MONGO_BSON_DETECTED
        #define D_ENV_MONGO_BSON_DETECTED      0
    #endif

#endif  // D_CFG_ENV_MONGO_CUSTOM


// =============================================================================
// IV.  TARGET SERVER VERSION DETECTION
// =============================================================================
//   The server version is a runtime property. These macros allow compile-
// time gating against a known deployment target. Set manually via
// D_CFG_ENV_MONGO_SERVER_VERSION or D_ENV_MONGO_DETECTED_SERVER_*.

#ifndef D_ENV_MONGO_SERVER_VERSION_ID
    #ifdef D_CFG_ENV_MONGO_SERVER_VERSION
        #define D_ENV_MONGO_SERVER_VERSION_ID \
            D_CFG_ENV_MONGO_SERVER_VERSION
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_VERSION)
        #define D_ENV_MONGO_SERVER_VERSION_ID \
            D_ENV_MONGO_DETECTED_SERVER_VERSION
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_8_0)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_8_0
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_7_0)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_7_0
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_6_0)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_6_0
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_5_0)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_5_0
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_4_4)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_4_4
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_4_2)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_4_2
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_4_0)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_4_0
    #elif defined(D_ENV_MONGO_DETECTED_SERVER_3_6)
        #define D_ENV_MONGO_SERVER_VERSION_ID D_ENV_MONGO_SERVER_3_6
    #else
        // no server version specified; default to 0 (unknown).
        // server-gated features will evaluate to 0.
        #define D_ENV_MONGO_SERVER_VERSION_ID 0
    #endif
#endif

#define D_ENV_MONGO_SERVER_MAJOR \
    D_ENV_MONGO_DECODE_MAJOR(D_ENV_MONGO_SERVER_VERSION_ID)
#define D_ENV_MONGO_SERVER_MINOR \
    D_ENV_MONGO_DECODE_MINOR(D_ENV_MONGO_SERVER_VERSION_ID)

// D_ENV_MONGO_SERVER_KNOWN
//   status: 1 if a target server version has been configured.
#define D_ENV_MONGO_SERVER_KNOWN \
    (D_ENV_MONGO_SERVER_VERSION_ID > 0)

// overall detection: either driver or server (or both) detected
#define D_ENV_MONGO_DETECTED \
    ( D_ENV_MONGO_DRIVER_DETECTED || D_ENV_MONGO_SERVER_KNOWN )


// =============================================================================
// V.   VERSION COMPARISON MACROS
// =============================================================================

#if D_ENV_MONGO_DETECTED

    // driver version comparisons
    #if D_ENV_MONGO_DRIVER_DETECTED
        #define D_ENV_MONGO_DRIVER_AT_LEAST(major, minor, patch) \
            (D_ENV_MONGO_DRIVER_VERSION_ID >= \
                D_ENV_MONGO_ENCODE_VERSION(major, minor, patch))
    #else
        #define D_ENV_MONGO_DRIVER_AT_LEAST(major, minor, patch) 0
    #endif

    // server version comparisons
    #define D_ENV_MONGO_SERVER_AT_LEAST(major, minor, patch) \
        (D_ENV_MONGO_SERVER_VERSION_ID >= \
            D_ENV_MONGO_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_MONGO_SERVER_BELOW(major, minor, patch) \
        (D_ENV_MONGO_SERVER_VERSION_ID < \
            D_ENV_MONGO_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_MONGO_SERVER_IN_RANGE(min_maj, min_min, min_pat,       \
                                         max_maj, max_min, max_pat)       \
        ( D_ENV_MONGO_SERVER_AT_LEAST(min_maj, min_min, min_pat) &&      \
          D_ENV_MONGO_SERVER_BELOW(max_maj, max_min, max_pat) )

    // series macros
    #define D_ENV_MONGO_IS_3_6 \
        D_ENV_MONGO_SERVER_IN_RANGE(3, 6, 0, 4, 0, 0)
    #define D_ENV_MONGO_IS_4_0 \
        D_ENV_MONGO_SERVER_IN_RANGE(4, 0, 0, 4, 2, 0)
    #define D_ENV_MONGO_IS_4_2 \
        D_ENV_MONGO_SERVER_IN_RANGE(4, 2, 0, 4, 4, 0)
    #define D_ENV_MONGO_IS_4_4 \
        D_ENV_MONGO_SERVER_IN_RANGE(4, 4, 0, 5, 0, 0)
    #define D_ENV_MONGO_IS_5_0 \
        D_ENV_MONGO_SERVER_IN_RANGE(5, 0, 0, 6, 0, 0)
    #define D_ENV_MONGO_IS_6_0 \
        D_ENV_MONGO_SERVER_IN_RANGE(6, 0, 0, 7, 0, 0)
    #define D_ENV_MONGO_IS_7_0 \
        D_ENV_MONGO_SERVER_IN_RANGE(7, 0, 0, 8, 0, 0)
    #define D_ENV_MONGO_IS_8_0 \
        D_ENV_MONGO_SERVER_AT_LEAST(8, 0, 0)


// =============================================================================
// VI.  EDITION DETECTION
// =============================================================================
//   MongoDB has Community Server and Enterprise Server. Enterprise adds
// LDAP/Kerberos auth, auditing, encryption at rest, SNMP monitoring,
// and In-Memory storage engine.

    // D_ENV_MONGO_IS_ENTERPRISE
    //   detection: 1 if Enterprise Server is the target.
    #ifndef D_ENV_MONGO_IS_ENTERPRISE
        #if defined(D_ENV_MONGO_DETECTED_ENTERPRISE)
            #define D_ENV_MONGO_IS_ENTERPRISE 1
        #else
            #define D_ENV_MONGO_IS_ENTERPRISE 0
        #endif
    #endif

    // D_ENV_MONGO_IS_COMMUNITY
    //   detection: 1 if Community Server (not Enterprise).
    #define D_ENV_MONGO_IS_COMMUNITY \
        (!D_ENV_MONGO_IS_ENTERPRISE)

    // D_ENV_MONGO_IS_ATLAS
    //   detection: 1 if targeting MongoDB Atlas (cloud). Atlas provides
    // Enterprise features plus Atlas-specific services. Must be manually
    // set.
    #ifndef D_ENV_MONGO_IS_ATLAS
        #if defined(D_ENV_MONGO_DETECTED_ATLAS)
            #define D_ENV_MONGO_IS_ATLAS 1
        #else
            #define D_ENV_MONGO_IS_ATLAS 0
        #endif
    #endif

    // D_ENV_MONGO_LICENSE_IS_SSPL
    //   status: 1 if server is under Server Side Public License.
    // MongoDB switched from AGPL to SSPL in October 2018 (server 4.0.3+).
    #define D_ENV_MONGO_LICENSE_IS_SSPL \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 3)


// =============================================================================
// VII. CLIENT LIBRARY FEATURES (LIBMONGOC)
// =============================================================================

    // D_ENV_MONGO_HAS_LIBMONGOC
    //   feature: detect if libmongoc C driver is available.
    #define D_ENV_MONGO_HAS_LIBMONGOC D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_LIBBSON
    //   feature: detect if libbson BSON library is available.
    #define D_ENV_MONGO_HAS_LIBBSON D_ENV_MONGO_BSON_DETECTED

    // D_ENV_MONGO_HAS_CONNECTION_POOL
    //   feature: mongoc_client_pool_t connection pooling. Present in
    // all modern driver versions.
    #define D_ENV_MONGO_HAS_CONNECTION_POOL D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_URI_PARSING
    //   feature: mongoc_uri_t for MongoDB connection string parsing.
    #define D_ENV_MONGO_HAS_URI_PARSING D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_SRV_DISCOVERY
    //   feature: mongodb+srv:// DNS SRV record-based discovery.
    // Requires driver 1.10+.
    #ifndef D_ENV_MONGO_HAS_SRV_DISCOVERY
        #if D_ENV_MONGO_DRIVER_AT_LEAST(1, 10, 0)
            #define D_ENV_MONGO_HAS_SRV_DISCOVERY 1
        #elif (!D_ENV_MONGO_DRIVER_DETECTED)
            #define D_ENV_MONGO_HAS_SRV_DISCOVERY 0
        #else
            #define D_ENV_MONGO_HAS_SRV_DISCOVERY 0
        #endif
    #endif

    // D_ENV_MONGO_HAS_SESSION_API
    //   feature: mongoc_client_session_t explicit session API.
    // Requires driver 1.9+.
    #define D_ENV_MONGO_HAS_SESSION_API \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 9, 0)

    // D_ENV_MONGO_HAS_CHANGE_STREAM_API
    //   feature: mongoc_change_stream_t change stream API.
    // Requires driver 1.9+.
    #define D_ENV_MONGO_HAS_CHANGE_STREAM_API \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 9, 0)

    // D_ENV_MONGO_HAS_TRANSACTION_API
    //   feature: mongoc session transaction API
    // (mongoc_client_session_start_transaction, etc.).
    // Requires driver 1.11+.
    #define D_ENV_MONGO_HAS_TRANSACTION_API \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 11, 0)

    // D_ENV_MONGO_HAS_VERSIONED_API
    //   feature: mongoc_server_api_t Stable API version declaration.
    // Requires driver 1.18+.
    #define D_ENV_MONGO_HAS_VERSIONED_API \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 18, 0)

    // D_ENV_MONGO_HAS_SDAM_MONITORING
    //   feature: Server Discovery and Monitoring (SDAM) event callbacks
    // (mongoc_apm_*). Requires driver 1.4+.
    #define D_ENV_MONGO_HAS_SDAM_MONITORING \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 4, 0)

    // D_ENV_MONGO_HAS_COMMAND_MONITORING
    //   feature: command monitoring callbacks (started, succeeded,
    // failed). Requires driver 1.4+.
    #define D_ENV_MONGO_HAS_COMMAND_MONITORING \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 4, 0)

    // D_ENV_MONGO_HAS_CLIENT_ENCRYPTION
    //   feature: mongoc_client_encryption_t API for client-side field-
    // level encryption (CSFLE). Requires driver 1.14+.
    #define D_ENV_MONGO_HAS_CLIENT_ENCRYPTION \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 14, 0)

    // D_ENV_MONGO_HAS_BULK_WRITE
    //   feature: mongoc_bulk_operation_t bulk write operations.
    #define D_ENV_MONGO_HAS_BULK_WRITE D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_GRIDFS
    //   feature: GridFS API (mongoc_gridfs_t, mongoc_gridfs_bucket_t)
    // for storing files in chunks.
    #define D_ENV_MONGO_HAS_GRIDFS D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_GRIDFS_BUCKET
    //   feature: GridFS Bucket API (newer, recommended API).
    // Requires driver 1.14+.
    #define D_ENV_MONGO_HAS_GRIDFS_BUCKET \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 14, 0)

    // D_ENV_MONGO_HAS_FIND_AND_MODIFY
    //   feature: mongoc_collection_find_and_modify (findAndModify).
    #define D_ENV_MONGO_HAS_FIND_AND_MODIFY D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_AGGREGATE
    //   feature: mongoc_collection_aggregate (aggregation pipeline).
    #define D_ENV_MONGO_HAS_AGGREGATE D_ENV_MONGO_DRIVER_DETECTED

    // D_ENV_MONGO_HAS_COUNT_DOCUMENTS
    //   feature: mongoc_collection_count_documents (accurate count).
    // Requires driver 1.11+. Replaces deprecated count().
    #define D_ENV_MONGO_HAS_COUNT_DOCUMENTS \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 11, 0)

    // D_ENV_MONGO_HAS_WATCH
    //   feature: mongoc_collection_watch (change stream helper).
    // Requires driver 1.11+.
    #define D_ENV_MONGO_HAS_WATCH \
        D_ENV_MONGO_DRIVER_AT_LEAST(1, 11, 0)


// =============================================================================
// VIII. BSON TYPE SYSTEM
// =============================================================================
//   BSON types are defined by the BSON specification and are always
// available when libbson is present.

    #if D_ENV_MONGO_BSON_DETECTED
        // D_ENV_MONGO_HAS_BSON_DOUBLE
        //   type: 64-bit IEEE 754 floating point.
        #define D_ENV_MONGO_HAS_BSON_DOUBLE        1
        // D_ENV_MONGO_HAS_BSON_STRING
        //   type: UTF-8 string.
        #define D_ENV_MONGO_HAS_BSON_STRING        1
        // D_ENV_MONGO_HAS_BSON_DOCUMENT
        //   type: embedded document (sub-document).
        #define D_ENV_MONGO_HAS_BSON_DOCUMENT      1
        // D_ENV_MONGO_HAS_BSON_ARRAY
        //   type: array (ordered sequence of values).
        #define D_ENV_MONGO_HAS_BSON_ARRAY         1
        // D_ENV_MONGO_HAS_BSON_BINARY
        //   type: binary data (with subtype: generic, UUID, MD5, etc.).
        #define D_ENV_MONGO_HAS_BSON_BINARY        1
        // D_ENV_MONGO_HAS_BSON_OBJECTID
        //   type: ObjectId (12-byte unique identifier).
        #define D_ENV_MONGO_HAS_BSON_OBJECTID      1
        // D_ENV_MONGO_HAS_BSON_BOOL
        //   type: boolean (true/false).
        #define D_ENV_MONGO_HAS_BSON_BOOL          1
        // D_ENV_MONGO_HAS_BSON_DATETIME
        //   type: UTC datetime (milliseconds since epoch).
        #define D_ENV_MONGO_HAS_BSON_DATETIME      1
        // D_ENV_MONGO_HAS_BSON_NULL
        //   type: null value.
        #define D_ENV_MONGO_HAS_BSON_NULL          1
        // D_ENV_MONGO_HAS_BSON_REGEX
        //   type: regular expression with options.
        #define D_ENV_MONGO_HAS_BSON_REGEX         1
        // D_ENV_MONGO_HAS_BSON_INT32
        //   type: 32-bit signed integer.
        #define D_ENV_MONGO_HAS_BSON_INT32         1
        // D_ENV_MONGO_HAS_BSON_INT64
        //   type: 64-bit signed integer.
        #define D_ENV_MONGO_HAS_BSON_INT64         1
        // D_ENV_MONGO_HAS_BSON_TIMESTAMP
        //   type: internal timestamp (for replication oplog).
        #define D_ENV_MONGO_HAS_BSON_TIMESTAMP     1
        // D_ENV_MONGO_HAS_BSON_DECIMAL128
        //   type: 128-bit decimal floating point (IEEE 754-2008).
        // Added to BSON spec and libbson 1.5+.
        #define D_ENV_MONGO_HAS_BSON_DECIMAL128    1
        // D_ENV_MONGO_HAS_BSON_MINKEY_MAXKEY
        //   type: MinKey and MaxKey (comparison boundary values).
        #define D_ENV_MONGO_HAS_BSON_MINKEY_MAXKEY 1
    #else
        #define D_ENV_MONGO_HAS_BSON_DOUBLE        0
        #define D_ENV_MONGO_HAS_BSON_STRING        0
        #define D_ENV_MONGO_HAS_BSON_DOCUMENT      0
        #define D_ENV_MONGO_HAS_BSON_ARRAY         0
        #define D_ENV_MONGO_HAS_BSON_BINARY        0
        #define D_ENV_MONGO_HAS_BSON_OBJECTID      0
        #define D_ENV_MONGO_HAS_BSON_BOOL          0
        #define D_ENV_MONGO_HAS_BSON_DATETIME      0
        #define D_ENV_MONGO_HAS_BSON_NULL          0
        #define D_ENV_MONGO_HAS_BSON_REGEX         0
        #define D_ENV_MONGO_HAS_BSON_INT32         0
        #define D_ENV_MONGO_HAS_BSON_INT64         0
        #define D_ENV_MONGO_HAS_BSON_TIMESTAMP     0
        #define D_ENV_MONGO_HAS_BSON_DECIMAL128    0
        #define D_ENV_MONGO_HAS_BSON_MINKEY_MAXKEY 0
    #endif


// =============================================================================
// IX.  INDEX TYPES (SERVER VERSION-GATED)
// =============================================================================

    // D_ENV_MONGO_HAS_INDEX_BTREE
    //   feature: default B-tree index (single-field and compound).
    #define D_ENV_MONGO_HAS_INDEX_BTREE \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_COMPOUND
    //   feature: compound indexes (multiple fields in one index).
    #define D_ENV_MONGO_HAS_INDEX_COMPOUND \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_UNIQUE
    //   feature: unique indexes.
    #define D_ENV_MONGO_HAS_INDEX_UNIQUE \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_SPARSE
    //   feature: sparse indexes (only index documents containing the
    // indexed field).
    #define D_ENV_MONGO_HAS_INDEX_SPARSE \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_HASHED
    //   feature: hashed indexes (hash-based sharding support).
    // Available since MongoDB 2.4.
    #define D_ENV_MONGO_HAS_INDEX_HASHED \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_TEXT
    //   feature: text indexes (legacy full-text search, not Atlas Search).
    // Available since MongoDB 2.4.
    #define D_ENV_MONGO_HAS_INDEX_TEXT \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_2DSPHERE
    //   feature: 2dsphere geospatial index (GeoJSON objects on a sphere).
    // Available since MongoDB 2.4.
    #define D_ENV_MONGO_HAS_INDEX_2DSPHERE \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_2D
    //   feature: 2d index (legacy flat-plane geospatial).
    #define D_ENV_MONGO_HAS_INDEX_2D \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_TTL
    //   feature: TTL indexes (automatic document expiration after a
    // time period). Available since MongoDB 2.2.
    #define D_ENV_MONGO_HAS_INDEX_TTL \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_INDEX_PARTIAL
    //   feature: partial indexes (index only documents matching a filter
    // expression). Introduced in MongoDB 3.2.
    #define D_ENV_MONGO_HAS_INDEX_PARTIAL \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 2, 0)

    // D_ENV_MONGO_HAS_INDEX_WILDCARD
    //   feature: wildcard indexes (index all fields or all fields
    // matching a pattern in documents with dynamic schemas).
    // Introduced in MongoDB 4.2.
    #define D_ENV_MONGO_HAS_INDEX_WILDCARD \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 2, 0)

    // D_ENV_MONGO_HAS_INDEX_COMPOUND_WILDCARD
    //   feature: compound wildcard indexes (wildcard + regular fields).
    // Introduced in MongoDB 7.0.
    #define D_ENV_MONGO_HAS_INDEX_COMPOUND_WILDCARD \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)

    // D_ENV_MONGO_HAS_INDEX_HIDDEN
    //   feature: hidden indexes (optimizer-invisible, for evaluating
    // index drop impact). Introduced in MongoDB 4.4.
    #define D_ENV_MONGO_HAS_INDEX_HIDDEN \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 4, 0)

    // D_ENV_MONGO_HAS_INDEX_CLUSTERED
    //   feature: clustered indexes (documents stored in index order,
    // reducing storage overhead). Introduced in MongoDB 5.3.
    #define D_ENV_MONGO_HAS_INDEX_CLUSTERED \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 3, 0)

    // D_ENV_MONGO_HAS_INDEX_COLUMNSTORE
    //   feature: columnstore indexes (columnar analytics index,
    // experimental). Introduced in MongoDB 6.0 (behind feature flag),
    // improved in 7.0.
    #define D_ENV_MONGO_HAS_INDEX_COLUMNSTORE \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)

    // D_ENV_MONGO_HAS_COLLATION
    //   feature: collation support for indexes and queries (language-
    // aware string comparison). Introduced in MongoDB 3.4.
    #define D_ENV_MONGO_HAS_COLLATION \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 4, 0)


// =============================================================================
// X.   AGGREGATION PIPELINE FEATURES
// =============================================================================

    // D_ENV_MONGO_HAS_AGGREGATE_PIPELINE
    //   feature: aggregation framework (basic pipeline with $match,
    // $group, $sort, $project, $limit, $skip, $unwind).
    // Core feature since MongoDB 2.2.
    #define D_ENV_MONGO_HAS_AGGREGATE_PIPELINE \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_AGG_LOOKUP
    //   feature: $lookup stage (left outer join between collections).
    // Introduced in MongoDB 3.2. Enhanced with pipeline sub-queries
    // in 3.6, correlated sub-queries in 5.0.
    #define D_ENV_MONGO_HAS_AGG_LOOKUP \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 2, 0)

    // D_ENV_MONGO_HAS_AGG_LOOKUP_PIPELINE
    //   feature: $lookup with pipeline sub-queries (uncorrelated and
    // correlated joins). Introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_AGG_LOOKUP_PIPELINE \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_AGG_GRAPH_LOOKUP
    //   feature: $graphLookup stage (recursive graph traversal on
    // collections). Introduced in MongoDB 3.4.
    #define D_ENV_MONGO_HAS_AGG_GRAPH_LOOKUP \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 4, 0)

    // D_ENV_MONGO_HAS_AGG_FACET
    //   feature: $facet stage (multiple parallel aggregation pipelines).
    // Introduced in MongoDB 3.4.
    #define D_ENV_MONGO_HAS_AGG_FACET \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 4, 0)

    // D_ENV_MONGO_HAS_AGG_MERGE
    //   feature: $merge stage (output aggregation results to an existing
    // collection with upsert/replace semantics; on-demand materialized
    // views). Introduced in MongoDB 4.2.
    #define D_ENV_MONGO_HAS_AGG_MERGE \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 2, 0)

    // D_ENV_MONGO_HAS_AGG_UNION_WITH
    //   feature: $unionWith stage (union of multiple collections in a
    // pipeline). Introduced in MongoDB 4.4.
    #define D_ENV_MONGO_HAS_AGG_UNION_WITH \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 4, 0)

    // D_ENV_MONGO_HAS_AGG_SET_WINDOW_FIELDS
    //   feature: $setWindowFields stage (window functions: $sum, $avg,
    // $rank, $denseRank, $documentNumber, $shift, etc. over partitions
    // and sort orders). Introduced in MongoDB 5.0.
    #define D_ENV_MONGO_HAS_AGG_SET_WINDOW_FIELDS \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 0, 0)

    // D_ENV_MONGO_HAS_AGG_DENSIFY
    //   feature: $densify stage (fill gaps in time-series or numeric
    // sequences). Introduced in MongoDB 5.1.
    #define D_ENV_MONGO_HAS_AGG_DENSIFY \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 1, 0)

    // D_ENV_MONGO_HAS_AGG_FILL
    //   feature: $fill stage (populate null and missing field values).
    // Introduced in MongoDB 5.1.
    #define D_ENV_MONGO_HAS_AGG_FILL \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 1, 0)

    // D_ENV_MONGO_HAS_AGG_DOCUMENTS
    //   feature: $documents stage (create documents from expressions,
    // like a VALUES clause). Introduced in MongoDB 5.1.
    #define D_ENV_MONGO_HAS_AGG_DOCUMENTS \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 1, 0)

    // D_ENV_MONGO_HAS_AGG_OUT
    //   feature: $out stage (write results to a new collection).
    // Present since MongoDB 2.6.
    #define D_ENV_MONGO_HAS_AGG_OUT \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_AGG_BUCKET
    //   feature: $bucket and $bucketAuto stages for histogram-style
    // grouping. Introduced in MongoDB 3.4.
    #define D_ENV_MONGO_HAS_AGG_BUCKET \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 4, 0)

    // D_ENV_MONGO_HAS_AGG_ACCUMULATOR
    //   feature: $accumulator custom JavaScript accumulator in
    // $group. Introduced in MongoDB 4.4.
    #define D_ENV_MONGO_HAS_AGG_ACCUMULATOR \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 4, 0)

    // D_ENV_MONGO_HAS_AGG_FUNCTION
    //   feature: $function custom JavaScript expressions in
    // aggregation. Introduced in MongoDB 4.4.
    #define D_ENV_MONGO_HAS_AGG_FUNCTION \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 4, 0)

    // D_ENV_MONGO_HAS_AGG_PERCENTILE
    //   feature: $percentile and $median accumulators.
    // Introduced in MongoDB 7.0.
    #define D_ENV_MONGO_HAS_AGG_PERCENTILE \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)


// =============================================================================
// XI.  TRANSACTIONS
// =============================================================================

    // D_ENV_MONGO_HAS_SINGLE_DOC_ATOMICITY
    //   feature: single-document atomicity (all single-document
    // operations are atomic). Core guarantee since MongoDB inception.
    #define D_ENV_MONGO_HAS_SINGLE_DOC_ATOMICITY \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_REPLICA_SET_TXN
    //   feature: multi-document ACID transactions on replica sets.
    // Introduced in MongoDB 4.0.
    #define D_ENV_MONGO_HAS_REPLICA_SET_TXN \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_HAS_DISTRIBUTED_TXN
    //   feature: distributed (sharded cluster) multi-document ACID
    // transactions. Introduced in MongoDB 4.2.
    #define D_ENV_MONGO_HAS_DISTRIBUTED_TXN \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 2, 0)

    // D_ENV_MONGO_HAS_RETRYABLE_WRITES
    //   feature: retryable writes (automatic retry of certain write
    // operations on transient errors). Introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_RETRYABLE_WRITES \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_RETRYABLE_READS
    //   feature: retryable reads. Introduced in MongoDB 3.6,
    // enabled by default in drivers since 4.2-compatible drivers.
    #define D_ENV_MONGO_HAS_RETRYABLE_READS \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_CAUSAL_CONSISTENCY
    //   feature: causal consistency sessions (read-your-writes across
    // replica set members). Introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_CAUSAL_CONSISTENCY \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)


// =============================================================================
// XII. CHANGE STREAMS
// =============================================================================

    // D_ENV_MONGO_HAS_CHANGE_STREAMS
    //   feature: change streams (real-time notification of data changes).
    // Collection-level change streams introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_CHANGE_STREAMS \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_CHANGE_STREAMS_DB
    //   feature: database-level change streams (watch all collections
    // in a database). Introduced in MongoDB 4.0.
    #define D_ENV_MONGO_HAS_CHANGE_STREAMS_DB \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_HAS_CHANGE_STREAMS_CLUSTER
    //   feature: cluster-level change streams (watch all databases).
    // Introduced in MongoDB 4.0.
    #define D_ENV_MONGO_HAS_CHANGE_STREAMS_CLUSTER \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_HAS_CHANGE_STREAMS_PRE_POST_IMAGE
    //   feature: pre-image and post-image in change stream events
    // (full document before and after modification).
    // Introduced in MongoDB 6.0.
    #define D_ENV_MONGO_HAS_CHANGE_STREAMS_PRE_POST_IMAGE \
        D_ENV_MONGO_SERVER_AT_LEAST(6, 0, 0)

    // D_ENV_MONGO_HAS_CHANGE_STREAMS_SPLIT_EVENTS
    //   feature: split large change events into fragments.
    // Introduced in MongoDB 7.0.
    #define D_ENV_MONGO_HAS_CHANGE_STREAMS_SPLIT_EVENTS \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)


// =============================================================================
// XIII. REPLICA SET AND SHARDING
// =============================================================================

    // D_ENV_MONGO_HAS_REPLICA_SET
    //   feature: replica set architecture (automatic failover, read
    // scaling). Core feature.
    #define D_ENV_MONGO_HAS_REPLICA_SET \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_WRITE_CONCERN
    //   feature: write concern (configurable write acknowledgment).
    #define D_ENV_MONGO_HAS_WRITE_CONCERN \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_READ_CONCERN
    //   feature: read concern (configurable read isolation level:
    // local, available, majority, linearizable, snapshot).
    // Introduced in MongoDB 3.2.
    #define D_ENV_MONGO_HAS_READ_CONCERN \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 2, 0)

    // D_ENV_MONGO_HAS_READ_CONCERN_SNAPSHOT
    //   feature: snapshot read concern (point-in-time reads in
    // multi-document transactions). Introduced in MongoDB 4.0.
    #define D_ENV_MONGO_HAS_READ_CONCERN_SNAPSHOT \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_HAS_READ_PREFERENCE
    //   feature: read preference (directing reads to primary,
    // secondary, nearest, etc.).
    #define D_ENV_MONGO_HAS_READ_PREFERENCE \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_HEDGED_READS
    //   feature: hedged reads (sending read operations to multiple
    // replica set members, using the fastest response).
    // Introduced in MongoDB 4.4.
    #define D_ENV_MONGO_HAS_HEDGED_READS \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 4, 0)

    // D_ENV_MONGO_HAS_SHARDING
    //   feature: sharded cluster architecture. Core feature.
    #define D_ENV_MONGO_HAS_SHARDING \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_ZONE_SHARDING
    //   feature: zone-based sharding (tag-aware sharding for data
    // locality). Introduced in MongoDB 3.4 (renamed from tag ranges).
    #define D_ENV_MONGO_HAS_ZONE_SHARDING \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 4, 0)

    // D_ENV_MONGO_HAS_RESHARDING
    //   feature: online resharding (change the shard key of an existing
    // sharded collection without downtime). Introduced in MongoDB 5.0.
    #define D_ENV_MONGO_HAS_RESHARDING \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 0, 0)

    // D_ENV_MONGO_HAS_CONFIGSVR_AS_REPLICASET
    //   feature: config servers as a replica set (CSRS).
    // Required since MongoDB 3.4 (mongod config server mode removed).
    #define D_ENV_MONGO_HAS_CONFIGSVR_AS_REPLICASET \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 4, 0)

    // D_ENV_MONGO_HAS_EMBEDDED_CONFIG
    //   feature: embedded config server (config shard - config server
    // also serves data). Introduced in MongoDB 8.0.
    #define D_ENV_MONGO_HAS_EMBEDDED_CONFIG \
        D_ENV_MONGO_SERVER_AT_LEAST(8, 0, 0)


// =============================================================================
// XIV. QUERY AND COMMAND FEATURES
// =============================================================================

    // D_ENV_MONGO_HAS_EXPR_IN_FIND
    //   feature: $expr in find queries (use aggregation expressions in
    // regular queries). Introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_EXPR_IN_FIND \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_JSON_SCHEMA
    //   feature: $jsonSchema validation (JSON Schema-based document
    // validation on collections). Introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_JSON_SCHEMA \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_SCHEMA_VALIDATION
    //   feature: document validation ($jsonSchema or expression-based
    // validators on collections). Validators introduced in 3.2;
    // $jsonSchema in 3.6.
    #define D_ENV_MONGO_HAS_SCHEMA_VALIDATION \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 2, 0)

    // D_ENV_MONGO_HAS_VERSIONED_API_SERVER
    //   feature: Stable API (server-side version declaration for forward
    // compatibility). Introduced in MongoDB 5.0.
    #define D_ENV_MONGO_HAS_VERSIONED_API_SERVER \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 0, 0)

    // D_ENV_MONGO_HAS_SAMPLE
    //   feature: $sample stage (random document sampling).
    // Introduced in MongoDB 3.2.
    #define D_ENV_MONGO_HAS_SAMPLE \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 2, 0)

    // D_ENV_MONGO_HAS_UPDATE_ARRAY_FILTERS
    //   feature: arrayFilters in update operations (selectively update
    // array elements matching a condition). Introduced in MongoDB 3.6.
    #define D_ENV_MONGO_HAS_UPDATE_ARRAY_FILTERS \
        D_ENV_MONGO_SERVER_AT_LEAST(3, 6, 0)

    // D_ENV_MONGO_HAS_UPDATE_PIPELINE
    //   feature: aggregation pipeline in update operations (use
    // $set, $unset, $replaceWith in updates). Introduced in MongoDB 4.2.
    #define D_ENV_MONGO_HAS_UPDATE_PIPELINE \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 2, 0)

    // D_ENV_MONGO_HAS_BULK_WRITE_COMMAND
    //   feature: bulkWrite command (server-side command-level bulk
    // operations, not just driver-side batching). Introduced in
    // MongoDB 8.0.
    #define D_ENV_MONGO_HAS_BULK_WRITE_COMMAND \
        D_ENV_MONGO_SERVER_AT_LEAST(8, 0, 0)


// =============================================================================
// XV.  TIME SERIES COLLECTIONS
// =============================================================================

    // D_ENV_MONGO_HAS_TIME_SERIES
    //   feature: time series collections (optimized storage for
    // time-stamped data with automatic bucketing). Introduced in
    // MongoDB 5.0.
    #define D_ENV_MONGO_HAS_TIME_SERIES \
        D_ENV_MONGO_SERVER_AT_LEAST(5, 0, 0)

    // D_ENV_MONGO_HAS_TIME_SERIES_SECONDARY_INDEX
    //   feature: secondary indexes on time series collections.
    // Introduced in MongoDB 6.0.
    #define D_ENV_MONGO_HAS_TIME_SERIES_SECONDARY_INDEX \
        D_ENV_MONGO_SERVER_AT_LEAST(6, 0, 0)

    // D_ENV_MONGO_HAS_TIME_SERIES_UPDATE_DELETE
    //   feature: arbitrary update and delete operations on time series
    // collections. Introduced in MongoDB 7.0 (previously limited).
    #define D_ENV_MONGO_HAS_TIME_SERIES_UPDATE_DELETE \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)


// =============================================================================
// XVI. SECURITY AND ENCRYPTION
// =============================================================================

    // D_ENV_MONGO_HAS_AUTH_SCRAM_SHA1
    //   feature: SCRAM-SHA-1 authentication (default before 4.0).
    #define D_ENV_MONGO_HAS_AUTH_SCRAM_SHA1 \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_AUTH_SCRAM_SHA256
    //   feature: SCRAM-SHA-256 authentication (default since 4.0).
    // Introduced in MongoDB 4.0.
    #define D_ENV_MONGO_HAS_AUTH_SCRAM_SHA256 \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_HAS_AUTH_X509
    //   feature: x.509 certificate authentication.
    // Available since MongoDB 2.6.
    #define D_ENV_MONGO_HAS_AUTH_X509 \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_AUTH_LDAP
    //   feature: LDAP proxy authentication. Enterprise only.
    #define D_ENV_MONGO_HAS_AUTH_LDAP \
        D_ENV_MONGO_IS_ENTERPRISE

    // D_ENV_MONGO_HAS_AUTH_KERBEROS
    //   feature: Kerberos (GSSAPI) authentication. Enterprise only.
    #define D_ENV_MONGO_HAS_AUTH_KERBEROS \
        D_ENV_MONGO_IS_ENTERPRISE

    // D_ENV_MONGO_HAS_AUTH_OIDC
    //   feature: OpenID Connect (OIDC) authentication for Workload
    // Identity. Introduced in MongoDB 7.0 (Atlas and Enterprise).
    #define D_ENV_MONGO_HAS_AUTH_OIDC \
        ( D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0) && \
          ( D_ENV_MONGO_IS_ENTERPRISE || D_ENV_MONGO_IS_ATLAS ) )

    // D_ENV_MONGO_HAS_SSL
    //   feature: TLS/SSL transport encryption. Core feature.
    #define D_ENV_MONGO_HAS_SSL \
        D_ENV_MONGO_SERVER_KNOWN

    // D_ENV_MONGO_HAS_ENCRYPTION_AT_REST
    //   feature: encryption at rest (WiredTiger encrypted storage
    // engine). Enterprise only.
    #define D_ENV_MONGO_HAS_ENCRYPTION_AT_REST \
        D_ENV_MONGO_IS_ENTERPRISE

    // D_ENV_MONGO_HAS_CSFLE
    //   feature: Client-Side Field-Level Encryption (automatic and
    // explicit encryption of document fields). Introduced in MongoDB 4.2.
    #define D_ENV_MONGO_HAS_CSFLE \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 2, 0)

    // D_ENV_MONGO_HAS_QUERYABLE_ENCRYPTION
    //   feature: Queryable Encryption (encrypted fields that support
    // equality and range queries). Introduced in MongoDB 6.0 (preview),
    // generally available in 7.0.
    #define D_ENV_MONGO_HAS_QUERYABLE_ENCRYPTION \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)

    // D_ENV_MONGO_HAS_AUDIT_LOG
    //   feature: audit logging. Enterprise only.
    #define D_ENV_MONGO_HAS_AUDIT_LOG \
        D_ENV_MONGO_IS_ENTERPRISE

    // D_ENV_MONGO_HAS_RBAC
    //   feature: role-based access control (user-defined roles,
    // built-in roles). Present in all modern versions.
    #define D_ENV_MONGO_HAS_RBAC \
        D_ENV_MONGO_SERVER_KNOWN


// =============================================================================
// XVII. ATLAS-SPECIFIC FEATURES
// =============================================================================
//   Atlas features are only available when targeting MongoDB Atlas
// (cloud-managed). These require D_ENV_MONGO_IS_ATLAS to be set.

    // D_ENV_MONGO_HAS_ATLAS_SEARCH
    //   feature: Atlas Search (Lucene-based full-text search integrated
    // into the aggregation pipeline via $search stage).
    #define D_ENV_MONGO_HAS_ATLAS_SEARCH \
        D_ENV_MONGO_IS_ATLAS

    // D_ENV_MONGO_HAS_ATLAS_VECTOR_SEARCH
    //   feature: Atlas Vector Search (vector similarity search for
    // AI/ML embedding workloads via $vectorSearch stage).
    #define D_ENV_MONGO_HAS_ATLAS_VECTOR_SEARCH \
        D_ENV_MONGO_IS_ATLAS

    // D_ENV_MONGO_HAS_ATLAS_DATA_LAKE
    //   feature: Atlas Data Federation (query data across Atlas
    // clusters, S3, and HTTP sources).
    #define D_ENV_MONGO_HAS_ATLAS_DATA_LAKE \
        D_ENV_MONGO_IS_ATLAS

    // D_ENV_MONGO_HAS_ATLAS_TRIGGERS
    //   feature: Atlas Triggers (database triggers, scheduled triggers,
    // authentication triggers via Atlas App Services).
    #define D_ENV_MONGO_HAS_ATLAS_TRIGGERS \
        D_ENV_MONGO_IS_ATLAS

    // D_ENV_MONGO_HAS_ATLAS_ONLINE_ARCHIVE
    //   feature: Atlas Online Archive (automatic tiering of aging data
    // to cheaper storage with query federation).
    #define D_ENV_MONGO_HAS_ATLAS_ONLINE_ARCHIVE \
        D_ENV_MONGO_IS_ATLAS


// =============================================================================
// XVIII. CONVENIENCE / COMPOSITE MACROS
// =============================================================================

    // D_ENV_MONGO_HAS_MODERN_TRANSACTIONS
    //   macro: evaluates to 1 if distributed multi-document
    // transactions, retryable writes, and causal consistency are all
    // available.
    #define D_ENV_MONGO_HAS_MODERN_TRANSACTIONS \
        ( D_ENV_MONGO_HAS_DISTRIBUTED_TXN    && \
          D_ENV_MONGO_HAS_RETRYABLE_WRITES   && \
          D_ENV_MONGO_HAS_CAUSAL_CONSISTENCY )

    // D_ENV_MONGO_HAS_MODERN_AGGREGATION
    //   macro: evaluates to 1 if $merge, $unionWith, $setWindowFields,
    // and $lookup with pipeline are all available.
    #define D_ENV_MONGO_HAS_MODERN_AGGREGATION \
        ( D_ENV_MONGO_HAS_AGG_MERGE              && \
          D_ENV_MONGO_HAS_AGG_UNION_WITH          && \
          D_ENV_MONGO_HAS_AGG_SET_WINDOW_FIELDS   && \
          D_ENV_MONGO_HAS_AGG_LOOKUP_PIPELINE )

    // D_ENV_MONGO_HAS_MODERN_CHANGE_STREAMS
    //   macro: evaluates to 1 if cluster-level change streams with
    // pre/post images are available.
    #define D_ENV_MONGO_HAS_MODERN_CHANGE_STREAMS \
        ( D_ENV_MONGO_HAS_CHANGE_STREAMS_CLUSTER          && \
          D_ENV_MONGO_HAS_CHANGE_STREAMS_PRE_POST_IMAGE )

    // D_ENV_MONGO_HAS_MODERN_SECURITY
    //   macro: evaluates to 1 if SCRAM-SHA-256, CSFLE, and RBAC are
    // all available.
    #define D_ENV_MONGO_HAS_MODERN_SECURITY \
        ( D_ENV_MONGO_HAS_AUTH_SCRAM_SHA256 && \
          D_ENV_MONGO_HAS_CSFLE             && \
          D_ENV_MONGO_HAS_RBAC )

    // D_ENV_MONGO_HAS_MODERN_SHARDING
    //   macro: evaluates to 1 if resharding, zone sharding, and
    // distributed transactions are all available.
    #define D_ENV_MONGO_HAS_MODERN_SHARDING \
        ( D_ENV_MONGO_HAS_RESHARDING        && \
          D_ENV_MONGO_HAS_ZONE_SHARDING     && \
          D_ENV_MONGO_HAS_DISTRIBUTED_TXN )

    // D_ENV_MONGO_IS_FULLY_MODERN
    //   macro: evaluates to 1 if the server has a comprehensive modern
    // feature set (roughly MongoDB 7.0+ with transactions, aggregation,
    // change streams, security, and sharding).
    #define D_ENV_MONGO_IS_FULLY_MODERN \
        ( D_ENV_MONGO_HAS_MODERN_TRANSACTIONS   && \
          D_ENV_MONGO_HAS_MODERN_AGGREGATION    && \
          D_ENV_MONGO_HAS_MODERN_CHANGE_STREAMS && \
          D_ENV_MONGO_HAS_MODERN_SECURITY       && \
          D_ENV_MONGO_HAS_MODERN_SHARDING       && \
          D_ENV_MONGO_HAS_TIME_SERIES           && \
          D_ENV_MONGO_HAS_VERSIONED_API_SERVER )


// =============================================================================
// XIX.  DEPRECATION AND REMOVAL
// =============================================================================

    // D_ENV_MONGO_REMOVED_MMAPV1
    //   status: 1 if MMAPv1 storage engine is removed (4.2+).
    #define D_ENV_MONGO_REMOVED_MMAPV1 \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 2, 0)

    // D_ENV_MONGO_REMOVED_MASTER_SLAVE
    //   status: 1 if legacy master-slave replication is removed (4.0+).
    #define D_ENV_MONGO_REMOVED_MASTER_SLAVE \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_DEPRECATED_COUNT
    //   status: 1 if the count command/helper is deprecated in favor
    // of countDocuments / estimatedDocumentCount (4.0+).
    #define D_ENV_MONGO_DEPRECATED_COUNT \
        D_ENV_MONGO_SERVER_AT_LEAST(4, 0, 0)

    // D_ENV_MONGO_REMOVED_WIRE_PROTOCOL_OP_QUERY
    //   status: 1 if legacy OP_QUERY / OP_INSERT / OP_UPDATE / OP_DELETE
    // wire protocol opcodes are removed (6.0+ for most, fully in 7.0).
    #define D_ENV_MONGO_REMOVED_WIRE_PROTOCOL_OP_QUERY \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)

    // D_ENV_MONGO_DEPRECATED_TEXT_INDEX
    //   status: 1 if legacy $text / text indexes are deprecated in
    // favor of Atlas Search (7.0+).
    #define D_ENV_MONGO_DEPRECATED_TEXT_INDEX \
        D_ENV_MONGO_SERVER_AT_LEAST(7, 0, 0)


#endif  // D_ENV_MONGO_DETECTED


#endif  // DJINTERP_ENVIRONMENT_MONGODB_
