/******************************************************************************
* djinterp [db]                                                 env_postgres.h
*
* djinterp PostgreSQL environmental detection header:
* This header provides comprehensive compile-time detection of PostgreSQL
* environments, capabilities, and version-gated features, including:
*   - version decomposition with pre-10 / post-10 numbering awareness
*   - client library (libpq) detection and API feature gating
*   - data type detection (JSONB, range/multirange, arrays, domains,
*     composite, enum, IDENTITY columns, generated columns)
*   - index type detection (B-tree, Hash, GiST, SP-GiST, GIN, BRIN,
*     covering indexes)
*   - full-text search (tsvector/tsquery, built-in since 8.3)
*   - partitioning features (declarative, list/range/hash, pruning)
*   - replication features (streaming, logical, slots, pub/sub)
*   - parallel query and parallel DDL detection
*   - SQL feature detection (CTEs, window functions, LATERAL, UPSERT,
*     MERGE, GROUPING SETS, row-level security, materialized views)
*   - procedural language and extension framework detection
*   - foreign data wrapper (FDW) framework detection
*   - authentication method detection (scram-sha-256, peer, cert, etc.)
*   - SSL/TLS and security features
*   - MVCC, vacuum, and storage features
*   - WAL configuration and management
*   - contrib extension detection (hstore, pg_trgm, citext, ltree, etc.)
*   - third-party extension detection (PostGIS, pgvector)
*   - pipeline and asynchronous libpq API detection
*
*   PostgreSQL is a client/server ORDBMS with a rich extension ecosystem.
* Feature availability is primarily determined by server version, with
* additional features from compile-time options and loadable extensions.
*
*   VERSION ENCODING:
*   PostgreSQL uses PG_VERSION_NUM with the formula:
*     MAJOR * 10000 + MINOR * 100 + PATCH    (pre-10: e.g. 9.6.24 = 90624)
*     MAJOR * 10000 + MINOR                  (post-10: e.g. 16.2 = 160002)
*   Post-10 releases use a two-component scheme where what was formerly
* the "minor" version (e.g., the "6" in 9.6) no longer exists; the second
* component is the patch/update release. The encoding still uses three
* fields but the middle field is always 0 for post-10 releases.
*
*   NAMING CONVENTION:
*   D_ENV_PG_[CATEGORY]_[FEATURE]  - 1 if available, 0 otherwise
*   D_ENV_PG_VERSION_[COMPONENT]   - version number components
*   D_ENV_PG_HAS_[CAPABILITY]      - capability flag (1/0)
*
* 
* path:      /inc/djinterp/core/env/db/postgres/env_postgres.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_POSTGRESQL_
#define DJINTERP_ENVIRONMENT_POSTGRESQL_ 1

// djinterp
#include "../../../../config/core/env/db/postgres/env_postgres_config.h"
#include "../env_db.h"


// =============================================================================
// 0.   VENDOR HEADER INCLUSION
// =============================================================================
//   Driven by D_CFG_ENV_USING_POSTGRESQL from env_config.h. When enabled,
// this section includes <libpq-fe.h> (or the override configured via
// D_CFG_ENV_POSTGRESQL_C_PATH) and, in C++ builds, the optional libpqxx
// header. Detection below is gated on D_ENV_POSTGRESQL_HEADER_INCLUDED so
// that no libpq symbols are referenced unless the header is actually in
// scope.

// --- C client header (libpq) ---
#if (D_CFG_ENV_USING_POSTGRESQL == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_POSTGRESQL_C_PATH)
            #include D_CFG_ENV_POSTGRESQL_C_PATH
            #define D_ENV_POSTGRESQL_HEADER_INCLUDED 1
        #elif __has_include(<postgresql/libpq-fe.h>)
            #include <postgresql/libpq-fe.h>
            #define D_ENV_POSTGRESQL_HEADER_INCLUDED 1
        #elif __has_include(<libpq-fe.h>)
            #include <libpq-fe.h>
            #define D_ENV_POSTGRESQL_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_POSTGRESQL=1 but no libpq header was "   \
                   "found. Install libpq-dev (or equivalent), or define "    \
                   "D_CFG_ENV_POSTGRESQL_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_POSTGRESQL_C_PATH
        #define D_ENV_POSTGRESQL_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_POSTGRESQL_CLIENT_C
        #define D_ENV_DB_HAS_POSTGRESQL_CLIENT_C 1
    #endif

#else
    #define D_ENV_POSTGRESQL_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_POSTGRESQL_CLIENT_C
        #define D_ENV_DB_HAS_POSTGRESQL_CLIENT_C 0
    #endif
#endif  // D_CFG_ENV_USING_POSTGRESQL


// --- C++ client header (libpqxx, optional, C++ builds only) ---
#if ( (D_CFG_ENV_USING_POSTGRESQL == 1) && defined(__cplusplus) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_POSTGRESQL_CPP_PATH)
            #include D_CFG_ENV_POSTGRESQL_CPP_PATH
            #define D_ENV_POSTGRESQL_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP
                #define D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP 1
            #endif
        #else
            #define D_ENV_POSTGRESQL_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP
                #define D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP 0
            #endif
        #endif
    #else
        #define D_ENV_POSTGRESQL_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP
            #define D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP 0
        #endif
    #endif

#else
    #define D_ENV_POSTGRESQL_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP
        #define D_ENV_DB_HAS_POSTGRESQL_CLIENT_CPP 0
    #endif
#endif  // D_CFG_ENV_USING_POSTGRESQL && __cplusplus


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================
//   All D_CFG_ENV_POSTGRESQL_* macros and D_CFG_ENV_PG_CUSTOM live in
// env_postgresql_config.h, pulled in at the top of this file.


// =============================================================================
// II.  VERSION ENCODING
// =============================================================================
//   PG_VERSION_NUM: MAJOR*10000 + MINOR*100 + PATCH (pre-10) or
// MAJOR*10000 + MINOR (post-10).

// D_ENV_PG_ENCODE_VERSION
//   macro: encodes a post-10 (major, minor) version into PG_VERSION_NUM
// format, where minor is the patch-level release.
#define D_ENV_PG_ENCODE_VERSION(major, minor) \
    ((major) * 10000 + (minor))

// D_ENV_PG_ENCODE_VERSION_LEGACY
//   macro: encodes a pre-10 (major, minor, patch) version into
// PG_VERSION_NUM format.
#define D_ENV_PG_ENCODE_VERSION_LEGACY(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))

// D_ENV_PG_DECODE_MAJOR
//   macro: extracts the major version from PG_VERSION_NUM.
#define D_ENV_PG_DECODE_MAJOR(ver) \
    ((ver) / 10000)

// D_ENV_PG_DECODE_MINOR_LEGACY
//   macro: extracts the minor version from a pre-10 PG_VERSION_NUM.
#define D_ENV_PG_DECODE_MINOR_LEGACY(ver) \
    (((ver) / 100) % 100)

// D_ENV_PG_DECODE_PATCH
//   macro: extracts the patch/release from PG_VERSION_NUM.
// For post-10 this is the full minor component; for pre-10 it is the
// patch within a minor series.
#define D_ENV_PG_DECODE_PATCH(ver) \
    ((ver) % 100)


// =============================================================================
// III. VERSION DETECTION
// =============================================================================

// version ID constants for feature-significant releases
// pre-10 (three-part: major.minor.0)
#define D_ENV_PG_VERSION_8_3           80300
#define D_ENV_PG_VERSION_8_4           80400
#define D_ENV_PG_VERSION_9_0           90000
#define D_ENV_PG_VERSION_9_1           90100
#define D_ENV_PG_VERSION_9_2           90200
#define D_ENV_PG_VERSION_9_3           90300
#define D_ENV_PG_VERSION_9_4           90400
#define D_ENV_PG_VERSION_9_5           90500
#define D_ENV_PG_VERSION_9_6           90600

// post-10 (two-part: major.0)
#define D_ENV_PG_VERSION_10            100000
#define D_ENV_PG_VERSION_11            110000
#define D_ENV_PG_VERSION_12            120000
#define D_ENV_PG_VERSION_13            130000
#define D_ENV_PG_VERSION_14            140000
#define D_ENV_PG_VERSION_15            150000
#define D_ENV_PG_VERSION_16            160000
#define D_ENV_PG_VERSION_17            170000

#if (D_CFG_ENV_PG_CUSTOM == 0)

    // automatic detection requires the libpq header to be in scope; if
    // D_CFG_ENV_USING_POSTGRESQL was not enabled the sentinel is 0 and we
    // skip cleanly (no reference to PG_VERSION_NUM).
    #if ( D_ENV_POSTGRESQL_HEADER_INCLUDED  &&  \
          defined(PG_VERSION_NUM) )
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            PG_VERSION_NUM
        #define D_ENV_PG_VERSION_MAJOR         \
            D_ENV_PG_DECODE_MAJOR(PG_VERSION_NUM)

        // post-10: "minor" in old sense does not exist; the second
        // component is the patch release.
        #if (PG_VERSION_NUM >= D_ENV_PG_VERSION_10)
            #define D_ENV_PG_VERSION_RELEASE   \
                D_ENV_PG_DECODE_PATCH(PG_VERSION_NUM)
            #define D_ENV_PG_IS_LEGACY_VERSIONING 0
        #else
            #define D_ENV_PG_VERSION_MINOR_LEGACY \
                D_ENV_PG_DECODE_MINOR_LEGACY(PG_VERSION_NUM)
            #define D_ENV_PG_VERSION_RELEASE   \
                D_ENV_PG_DECODE_PATCH(PG_VERSION_NUM)
            #define D_ENV_PG_IS_LEGACY_VERSIONING 1
        #endif

        #ifdef PG_VERSION
            #define D_ENV_PG_VERSION_STRING    PG_VERSION
        #else
            #define D_ENV_PG_VERSION_STRING    "unknown"
        #endif
    #else
        #define D_ENV_PG_DETECTED              0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_PG_DETECTED_VERSION
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_DETECTED_VERSION
        #define D_ENV_PG_VERSION_MAJOR         \
            D_ENV_PG_DECODE_MAJOR(D_ENV_PG_DETECTED_VERSION)
        #define D_ENV_PG_VERSION_RELEASE       \
            D_ENV_PG_DECODE_PATCH(D_ENV_PG_DETECTED_VERSION)
        #define D_ENV_PG_IS_LEGACY_VERSIONING  \
            (D_ENV_PG_DETECTED_VERSION < D_ENV_PG_VERSION_10)
        #define D_ENV_PG_VERSION_STRING        "manual"

    #elif defined(D_ENV_PG_DETECTED_17)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_17
        #define D_ENV_PG_VERSION_MAJOR         17
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "17.0"

    #elif defined(D_ENV_PG_DETECTED_16)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_16
        #define D_ENV_PG_VERSION_MAJOR         16
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "16.0"

    #elif defined(D_ENV_PG_DETECTED_15)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_15
        #define D_ENV_PG_VERSION_MAJOR         15
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "15.0"

    #elif defined(D_ENV_PG_DETECTED_14)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_14
        #define D_ENV_PG_VERSION_MAJOR         14
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "14.0"

    #elif defined(D_ENV_PG_DETECTED_13)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_13
        #define D_ENV_PG_VERSION_MAJOR         13
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "13.0"

    #elif defined(D_ENV_PG_DETECTED_12)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_12
        #define D_ENV_PG_VERSION_MAJOR         12
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "12.0"

    #elif defined(D_ENV_PG_DETECTED_11)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_11
        #define D_ENV_PG_VERSION_MAJOR         11
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "11.0"

    #elif defined(D_ENV_PG_DETECTED_10)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_10
        #define D_ENV_PG_VERSION_MAJOR         10
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  0
        #define D_ENV_PG_VERSION_STRING        "10.0"

    #elif defined(D_ENV_PG_DETECTED_9_6)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_9_6
        #define D_ENV_PG_VERSION_MAJOR         9
        #define D_ENV_PG_VERSION_MINOR_LEGACY  6
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  1
        #define D_ENV_PG_VERSION_STRING        "9.6.0"

    #elif defined(D_ENV_PG_DETECTED_9_5)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_9_5
        #define D_ENV_PG_VERSION_MAJOR         9
        #define D_ENV_PG_VERSION_MINOR_LEGACY  5
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  1
        #define D_ENV_PG_VERSION_STRING        "9.5.0"

    #elif defined(D_ENV_PG_DETECTED_9_4)
        #define D_ENV_PG_DETECTED              1
        #define D_ENV_PG_VERSION_ID            D_ENV_PG_VERSION_9_4
        #define D_ENV_PG_VERSION_MAJOR         9
        #define D_ENV_PG_VERSION_MINOR_LEGACY  4
        #define D_ENV_PG_VERSION_RELEASE       0
        #define D_ENV_PG_IS_LEGACY_VERSIONING  1
        #define D_ENV_PG_VERSION_STRING        "9.4.0"

    #else
        #define D_ENV_PG_DETECTED              0
    #endif

#endif  // D_CFG_ENV_PG_CUSTOM


// =============================================================================
// IV.  VERSION COMPARISON MACROS
// =============================================================================

#if D_ENV_PG_DETECTED

    // D_ENV_PG_VERSION_AT_LEAST
    //   macro: evaluates to 1 if PostgreSQL version >= specified version.
    // Use two-argument form for post-10 (major, release) and
    // three-argument form for pre-10 (major, minor, patch).
    // For convenience, this macro works with the raw PG_VERSION_NUM
    // encoding: pass the constant directly or use the ENCODE helpers.
    #define D_ENV_PG_VERSION_AT_LEAST_NUM(ver_num) \
        (D_ENV_PG_VERSION_ID >= (ver_num))

    // D_ENV_PG_VERSION_BELOW_NUM
    //   macro: evaluates to 1 if PostgreSQL version < specified version.
    #define D_ENV_PG_VERSION_BELOW_NUM(ver_num) \
        (D_ENV_PG_VERSION_ID < (ver_num))

    // D_ENV_PG_VERSION_IN_RANGE_NUM
    //   macro: evaluates to 1 if PostgreSQL version is within [min, max).
    #define D_ENV_PG_VERSION_IN_RANGE_NUM(min_ver, max_ver) \
        ( D_ENV_PG_VERSION_AT_LEAST_NUM(min_ver) && \
          D_ENV_PG_VERSION_BELOW_NUM(max_ver) )

    // shorthand for common modern (post-10) version checks
    // D_ENV_PG_AT_LEAST
    //   macro: shorthand for post-10 version check by major version.
    // E.g. D_ENV_PG_AT_LEAST(14) checks >= PostgreSQL 14.0.
    #define D_ENV_PG_AT_LEAST(major) \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_ENCODE_VERSION(major, 0))

    // series macros
    #define D_ENV_PG_IS_9_4 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_9_4, \
                                       D_ENV_PG_VERSION_9_5)
    #define D_ENV_PG_IS_9_5 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_9_5, \
                                       D_ENV_PG_VERSION_9_6)
    #define D_ENV_PG_IS_9_6 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_9_6, \
                                       D_ENV_PG_VERSION_10)
    #define D_ENV_PG_IS_10 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_10, \
                                       D_ENV_PG_VERSION_11)
    #define D_ENV_PG_IS_11 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_11, \
                                       D_ENV_PG_VERSION_12)
    #define D_ENV_PG_IS_12 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_12, \
                                       D_ENV_PG_VERSION_13)
    #define D_ENV_PG_IS_13 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_13, \
                                       D_ENV_PG_VERSION_14)
    #define D_ENV_PG_IS_14 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_14, \
                                       D_ENV_PG_VERSION_15)
    #define D_ENV_PG_IS_15 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_15, \
                                       D_ENV_PG_VERSION_16)
    #define D_ENV_PG_IS_16 \
        D_ENV_PG_VERSION_IN_RANGE_NUM(D_ENV_PG_VERSION_16, \
                                       D_ENV_PG_VERSION_17)
    #define D_ENV_PG_IS_17 \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_17)


// =============================================================================
// V.   CLIENT LIBRARY (LIBPQ) DETECTION
// =============================================================================

    // D_ENV_PG_HAS_LIBPQ
    //   feature: detect if libpq (PostgreSQL C client library) headers
    // are available. Detected via PG_VERSION_NUM (defined in
    // pg_config.h or postgres_fe.h, included transitively by libpq-fe.h).
    #define D_ENV_PG_HAS_LIBPQ D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_LIBPQ_PIPELINE
    //   feature: detect if pipeline mode (PQpipelineStatus,
    // PQenterPipelineMode, PQsendFlushRequest) is available.
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_LIBPQ_PIPELINE \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_LIBPQ_TRACE
    //   feature: detect if the enhanced PQtrace() API (structured trace
    // output) is available. Redesigned in PostgreSQL 14.
    #define D_ENV_PG_HAS_LIBPQ_TRACE \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_LIBPQ_NONBLOCKING
    //   feature: PQsetnonblocking() / PQisnonblocking() for fully
    // nonblocking connection usage. Present in all modern libpq.
    #define D_ENV_PG_HAS_LIBPQ_NONBLOCKING D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_LIBPQ_SSLATTRIBUTE
    //   feature: PQsslAttribute() / PQsslAttributeNames() for
    // inspecting SSL connection details. Introduced in PostgreSQL 9.5.
    #define D_ENV_PG_HAS_LIBPQ_SSLATTRIBUTE \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_LIBPQ_RESULT_MEMORY
    //   feature: PQresultMemorySize() for querying result memory
    // consumption. Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_LIBPQ_RESULT_MEMORY \
        D_ENV_PG_AT_LEAST(12)

    // D_ENV_PG_HAS_LIBPQ_CANCEL_CONN
    //   feature: PQcancelCreate() / PQcancelStart() / PQcancelFinish()
    // asynchronous cancellation API. Introduced in PostgreSQL 17.
    #define D_ENV_PG_HAS_LIBPQ_CANCEL_CONN \
        D_ENV_PG_AT_LEAST(17)

    // D_ENV_PG_HAS_LIBPQ_CHUNKED_RESULT
    //   feature: PQsetChunkedRowsMode() for streaming large result sets
    // in fixed-size chunks. Introduced in PostgreSQL 17.
    #define D_ENV_PG_HAS_LIBPQ_CHUNKED_RESULT \
        D_ENV_PG_AT_LEAST(17)

    // D_ENV_PG_HAS_LIBPQ_CLOSE_PREPARED
    //   feature: PQclosePrepared() / PQclosePortal() for closing
    // server-side prepared statements and portals. PostgreSQL 17.
    #define D_ENV_PG_HAS_LIBPQ_CLOSE_PREPARED \
        D_ENV_PG_AT_LEAST(17)

    // D_ENV_PG_HAS_LIBPQ_SEND_QUERY_PARAMS
    //   feature: PQsendQueryParams() / PQsendQueryPrepared() for
    // parameterized asynchronous queries. Present since PostgreSQL 7.4.
    #define D_ENV_PG_HAS_LIBPQ_SEND_QUERY_PARAMS D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_LIBPQ_PREPARE
    //   feature: PQprepare() / PQexecPrepared() for server-side prepared
    // statements. Present since PostgreSQL 7.4.
    #define D_ENV_PG_HAS_LIBPQ_PREPARE D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_LIBPQ_NOTICE_RECEIVER
    //   feature: PQsetNoticeReceiver() for structured notice handling.
    #define D_ENV_PG_HAS_LIBPQ_NOTICE_RECEIVER D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_LIBPQ_COPY_BOTH
    //   feature: COPY BOTH protocol (used by streaming replication).
    // Protocol support since PostgreSQL 9.0.
    #define D_ENV_PG_HAS_LIBPQ_COPY_BOTH \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_0)


// =============================================================================
// VI.  SSL/TLS AND AUTHENTICATION
// =============================================================================

    // D_ENV_PG_HAS_SSL
    //   feature: detect if libpq was compiled with SSL support.
    #ifndef D_ENV_PG_HAS_SSL
        #if defined(USE_SSL) || defined(USE_OPENSSL)
            #define D_ENV_PG_HAS_SSL 1
        #else
            // most modern PostgreSQL packages include SSL by default;
            // assume available if version is modern enough
            #define D_ENV_PG_HAS_SSL 1
        #endif
    #endif

    // D_ENV_PG_HAS_SCRAM_SHA_256
    //   feature: SCRAM-SHA-256 authentication. Introduced in PostgreSQL
    // 10 as an alternative to md5; default since PostgreSQL 14.
    #define D_ENV_PG_HAS_SCRAM_SHA_256 \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_DEFAULT_AUTH_IS_SCRAM
    //   status: 1 if the default password_encryption is scram-sha-256.
    // Changed from md5 to scram-sha-256 in PostgreSQL 14.
    #define D_ENV_PG_DEFAULT_AUTH_IS_SCRAM \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_AUTH_PEER
    //   feature: peer authentication (OS user matching for local
    // connections via Unix socket). Available since PostgreSQL 9.1.
    #define D_ENV_PG_HAS_AUTH_PEER \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_1)

    // D_ENV_PG_HAS_AUTH_CERT
    //   feature: certificate-based authentication (client SSL cert
    // mapped to database user). Available since PostgreSQL 8.4.
    #define D_ENV_PG_HAS_AUTH_CERT \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_8_4)

    // D_ENV_PG_HAS_AUTH_GSS
    //   feature: detect if GSSAPI/Kerberos authentication is compiled in.
    #ifndef D_ENV_PG_HAS_AUTH_GSS
        #if defined(ENABLE_GSS)
            #define D_ENV_PG_HAS_AUTH_GSS 1
        #else
            #define D_ENV_PG_HAS_AUTH_GSS 0
        #endif
    #endif

    // D_ENV_PG_HAS_GSS_ENC
    //   feature: GSSAPI encryption (in-transit encryption without SSL).
    // Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_GSS_ENC \
        ( D_ENV_PG_AT_LEAST(12) && D_ENV_PG_HAS_AUTH_GSS )

    // D_ENV_PG_HAS_AUTH_LDAP
    //   feature: LDAP authentication support.
    #ifndef D_ENV_PG_HAS_AUTH_LDAP
        #if defined(USE_LDAP)
            #define D_ENV_PG_HAS_AUTH_LDAP 1
        #else
            #define D_ENV_PG_HAS_AUTH_LDAP 0
        #endif
    #endif

    // D_ENV_PG_HAS_AUTH_RADIUS
    //   feature: RADIUS authentication. Available since PostgreSQL 9.0.
    #define D_ENV_PG_HAS_AUTH_RADIUS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_0)

    // D_ENV_PG_HAS_AUTH_PAM
    //   feature: detect if PAM authentication is compiled in.
    #ifndef D_ENV_PG_HAS_AUTH_PAM
        #if defined(USE_PAM)
            #define D_ENV_PG_HAS_AUTH_PAM 1
        #else
            #define D_ENV_PG_HAS_AUTH_PAM 0
        #endif
    #endif

    // D_ENV_PG_HAS_CHANNEL_BINDING
    //   feature: SCRAM channel binding for TLS (defense against MITM
    // on authentication). Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_CHANNEL_BINDING \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_CLIENTCERT_OPTION
    //   feature: clientcert=verify-full hba option for mandatory client
    // certificate verification. Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_CLIENTCERT_OPTION \
        D_ENV_PG_AT_LEAST(12)


// =============================================================================
// VII. DATA TYPES (VERSION-GATED)
// =============================================================================

    // D_ENV_PG_HAS_JSON
    //   feature: JSON data type (text-based). Introduced in PostgreSQL 9.2.
    #define D_ENV_PG_HAS_JSON \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_2)

    // D_ENV_PG_HAS_JSONB
    //   feature: JSONB binary JSON type (GIN-indexable, decomposed
    // storage). Introduced in PostgreSQL 9.4.
    #define D_ENV_PG_HAS_JSONB \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_4)

    // D_ENV_PG_HAS_JSONPATH
    //   feature: SQL/JSON path language (jsonb_path_query, @? and @@
    // operators). Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_JSONPATH \
        D_ENV_PG_AT_LEAST(12)

    // D_ENV_PG_HAS_JSON_TABLE
    //   feature: JSON_TABLE() standard SQL/JSON function for tabular
    // decomposition of JSON data. Introduced in PostgreSQL 17.
    #define D_ENV_PG_HAS_JSON_TABLE \
        D_ENV_PG_AT_LEAST(17)

    // D_ENV_PG_HAS_SQLJSON_CONSTRUCTORS
    //   feature: SQL/JSON constructor functions (JSON_ARRAY,
    // JSON_OBJECT, JSON_ARRAYAGG, JSON_OBJECTAGG, IS JSON predicate).
    // Introduced in PostgreSQL 16.
    #define D_ENV_PG_HAS_SQLJSON_CONSTRUCTORS \
        D_ENV_PG_AT_LEAST(16)

    // D_ENV_PG_HAS_JSONB_SUBSCRIPT
    //   feature: JSONB subscripting syntax (jsonb_col['key']).
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_JSONB_SUBSCRIPT \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_RANGE_TYPES
    //   feature: range types (int4range, tsrange, etc.).
    // Introduced in PostgreSQL 9.2.
    #define D_ENV_PG_HAS_RANGE_TYPES \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_2)

    // D_ENV_PG_HAS_MULTIRANGE
    //   feature: multirange types (int4multirange, etc.).
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_MULTIRANGE \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_ARRAY_TYPES
    //   feature: array data types. Present since PostgreSQL inception.
    #define D_ENV_PG_HAS_ARRAY_TYPES D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_DOMAIN_TYPES
    //   feature: user-defined domain types. Present since PostgreSQL 7.3.
    #define D_ENV_PG_HAS_DOMAIN_TYPES D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_COMPOSITE_TYPES
    //   feature: composite (record/row) types. Present since 8.0.
    #define D_ENV_PG_HAS_COMPOSITE_TYPES D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_ENUM_TYPE
    //   feature: ENUM data type. Introduced in PostgreSQL 8.3.
    #define D_ENV_PG_HAS_ENUM_TYPE \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_8_3)

    // D_ENV_PG_HAS_UUID_TYPE
    //   feature: UUID data type. Introduced in PostgreSQL 8.3.
    #define D_ENV_PG_HAS_UUID_TYPE \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_8_3)

    // D_ENV_PG_HAS_IDENTITY_COLUMNS
    //   feature: GENERATED { ALWAYS | BY DEFAULT } AS IDENTITY.
    // Introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_IDENTITY_COLUMNS \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_GENERATED_COLUMNS
    //   feature: GENERATED ALWAYS AS (expr) STORED columns.
    // Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_GENERATED_COLUMNS \
        D_ENV_PG_AT_LEAST(12)

    // D_ENV_PG_HAS_GENERATED_VIRTUAL
    //   feature: GENERATED ALWAYS AS (expr) VIRTUAL columns.
    // Introduced in PostgreSQL 17 (partial).
    #define D_ENV_PG_HAS_GENERATED_VIRTUAL \
        D_ENV_PG_AT_LEAST(17)


// =============================================================================
// VIII. INDEX TYPES
// =============================================================================

    // D_ENV_PG_HAS_INDEX_BTREE
    //   feature: B-tree indexes (always present).
    #define D_ENV_PG_HAS_INDEX_BTREE D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_INDEX_HASH
    //   feature: Hash indexes. Present since early PostgreSQL. Not
    // WAL-logged (crash-unsafe) until PostgreSQL 10.
    #define D_ENV_PG_HAS_INDEX_HASH D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_INDEX_HASH_WAL
    //   feature: WAL-logged hash indexes (crash-safe). PostgreSQL 10+.
    #define D_ENV_PG_HAS_INDEX_HASH_WAL \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_INDEX_GIST
    //   feature: GiST (Generalized Search Tree) indexes. Present since
    // early PostgreSQL.
    #define D_ENV_PG_HAS_INDEX_GIST D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_INDEX_SPGIST
    //   feature: SP-GiST (Space-Partitioned GiST) indexes.
    // Introduced in PostgreSQL 9.2.
    #define D_ENV_PG_HAS_INDEX_SPGIST \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_2)

    // D_ENV_PG_HAS_INDEX_GIN
    //   feature: GIN (Generalized Inverted) indexes for full-text search
    // and JSONB containment.
    #define D_ENV_PG_HAS_INDEX_GIN D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_INDEX_BRIN
    //   feature: BRIN (Block Range) indexes for large naturally-ordered
    // tables. Introduced in PostgreSQL 9.5.
    #define D_ENV_PG_HAS_INDEX_BRIN \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_COVERING_INDEX
    //   feature: covering indexes (CREATE INDEX ... INCLUDE (...)).
    // Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_COVERING_INDEX \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_PARTIAL_INDEX
    //   feature: partial indexes (CREATE INDEX ... WHERE ...).
    // Present since PostgreSQL 7.2.
    #define D_ENV_PG_HAS_PARTIAL_INDEX D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_EXPRESSION_INDEX
    //   feature: expression indexes (CREATE INDEX ... ON (expr)).
    // Present since PostgreSQL 7.4.
    #define D_ENV_PG_HAS_EXPRESSION_INDEX D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_CONCURRENT_INDEX
    //   feature: CREATE INDEX CONCURRENTLY (non-blocking index builds).
    // Introduced in PostgreSQL 8.2.
    #define D_ENV_PG_HAS_CONCURRENT_INDEX D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_REINDEX_CONCURRENTLY
    //   feature: REINDEX CONCURRENTLY. Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_REINDEX_CONCURRENTLY \
        D_ENV_PG_AT_LEAST(12)


// =============================================================================
// IX.  SQL FEATURE DETECTION (VERSION-GATED)
// =============================================================================

    // D_ENV_PG_HAS_CTE
    //   feature: Common Table Expressions (WITH / WITH RECURSIVE).
    // Introduced in PostgreSQL 8.4.
    #define D_ENV_PG_HAS_CTE \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_8_4)

    // D_ENV_PG_HAS_CTE_MATERIALIZED
    //   feature: MATERIALIZED / NOT MATERIALIZED hints on CTEs.
    // Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_CTE_MATERIALIZED \
        D_ENV_PG_AT_LEAST(12)

    // D_ENV_PG_HAS_CTE_SEARCH_CYCLE
    //   feature: SEARCH and CYCLE clauses for recursive CTEs per
    // SQL:2008 standard. Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_CTE_SEARCH_CYCLE \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_WINDOW_FUNCTIONS
    //   feature: SQL window functions (ROW_NUMBER, RANK, etc.).
    // Introduced in PostgreSQL 8.4.
    #define D_ENV_PG_HAS_WINDOW_FUNCTIONS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_8_4)

    // D_ENV_PG_HAS_LATERAL
    //   feature: LATERAL subqueries and joins.
    // Introduced in PostgreSQL 9.3.
    #define D_ENV_PG_HAS_LATERAL \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_3)

    // D_ENV_PG_HAS_UPSERT
    //   feature: INSERT ... ON CONFLICT DO UPDATE/NOTHING (UPSERT).
    // Introduced in PostgreSQL 9.5.
    #define D_ENV_PG_HAS_UPSERT \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_MERGE
    //   feature: MERGE statement (SQL:2008 standard MERGE).
    // Introduced in PostgreSQL 15.
    #define D_ENV_PG_HAS_MERGE \
        D_ENV_PG_AT_LEAST(15)

    // D_ENV_PG_HAS_GROUPING_SETS
    //   feature: GROUPING SETS, CUBE, ROLLUP for multi-dimensional
    // aggregation. Introduced in PostgreSQL 9.5.
    #define D_ENV_PG_HAS_GROUPING_SETS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_MATERIALIZED_VIEWS
    //   feature: materialized views (CREATE MATERIALIZED VIEW).
    // Introduced in PostgreSQL 9.3.
    #define D_ENV_PG_HAS_MATERIALIZED_VIEWS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_3)

    // D_ENV_PG_HAS_MATVIEW_CONCURRENT_REFRESH
    //   feature: REFRESH MATERIALIZED VIEW CONCURRENTLY.
    // Introduced in PostgreSQL 9.4.
    #define D_ENV_PG_HAS_MATVIEW_CONCURRENT_REFRESH \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_4)

    // D_ENV_PG_HAS_TABLE_INHERITANCE
    //   feature: table inheritance (INHERITS). Core PostgreSQL feature
    // present since early releases.
    #define D_ENV_PG_HAS_TABLE_INHERITANCE D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_ROW_LEVEL_SECURITY
    //   feature: row-level security policies (CREATE POLICY).
    // Introduced in PostgreSQL 9.5.
    #define D_ENV_PG_HAS_ROW_LEVEL_SECURITY \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_STORED_PROCEDURES
    //   feature: CREATE PROCEDURE with CALL and transaction control
    // (COMMIT/ROLLBACK) inside procedures. Introduced in PostgreSQL 11.
    // Note: functions (CREATE FUNCTION) have been present since forever.
    #define D_ENV_PG_HAS_STORED_PROCEDURES \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_RETURNING
    //   feature: INSERT/UPDATE/DELETE ... RETURNING clause.
    // Introduced in PostgreSQL 8.2.
    #define D_ENV_PG_HAS_RETURNING D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_TABLESAMPLE
    //   feature: TABLESAMPLE clause (BERNOULLI, SYSTEM).
    // Introduced in PostgreSQL 9.5.
    #define D_ENV_PG_HAS_TABLESAMPLE \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_COPY_FREEZE
    //   feature: COPY ... WITH (FREEZE) for fast bulk loading.
    // Introduced in PostgreSQL 9.3.
    #define D_ENV_PG_HAS_COPY_FREEZE \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_3)

    // D_ENV_PG_HAS_FULL_TEXT_SEARCH
    //   feature: built-in full-text search (tsvector, tsquery, GIN/GiST).
    // Moved from contrib to core in PostgreSQL 8.3.
    #define D_ENV_PG_HAS_FULL_TEXT_SEARCH \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_8_3)

    // D_ENV_PG_HAS_GENERATED_AS_IDENTITY
    //   feature: columns with GENERATED AS IDENTITY. PostgreSQL 10+.
    // Aliased from D_ENV_PG_HAS_IDENTITY_COLUMNS above.
    #define D_ENV_PG_HAS_GENERATED_AS_IDENTITY \
        D_ENV_PG_HAS_IDENTITY_COLUMNS


// =============================================================================
// X.   PARTITIONING
// =============================================================================

    // D_ENV_PG_HAS_DECLARATIVE_PARTITIONING
    //   feature: declarative table partitioning (PARTITION BY RANGE/LIST).
    // Introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_DECLARATIVE_PARTITIONING \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_HASH_PARTITIONING
    //   feature: hash partitioning (PARTITION BY HASH).
    // Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_HASH_PARTITIONING \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_DEFAULT_PARTITION
    //   feature: DEFAULT partition (catch-all for unmatched rows).
    // Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_DEFAULT_PARTITION \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_PARTITION_PRUNING
    //   feature: runtime partition pruning (optimizer eliminates
    // partitions based on query parameters). Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_PARTITION_PRUNING \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_PARTITION_FOREIGN_KEY
    //   feature: foreign keys referencing partitioned tables.
    // Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_PARTITION_FOREIGN_KEY \
        D_ENV_PG_AT_LEAST(12)

    // D_ENV_PG_HAS_ATTACH_PARTITION_CONCURRENTLY
    //   feature: ALTER TABLE ... ATTACH PARTITION ... CONCURRENTLY.
    // Note: not yet available as of PostgreSQL 17; detach concurrently
    // was introduced in 14.
    #define D_ENV_PG_HAS_DETACH_PARTITION_CONCURRENTLY \
        D_ENV_PG_AT_LEAST(14)


// =============================================================================
// XI.  REPLICATION AND HIGH AVAILABILITY
// =============================================================================

    // D_ENV_PG_HAS_STREAMING_REPL
    //   feature: streaming replication (physical, WAL-based).
    // Introduced in PostgreSQL 9.0.
    #define D_ENV_PG_HAS_STREAMING_REPL \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_0)

    // D_ENV_PG_HAS_REPLICATION_SLOTS
    //   feature: replication slots (preventing WAL segment removal).
    // Introduced in PostgreSQL 9.4.
    #define D_ENV_PG_HAS_REPLICATION_SLOTS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_4)

    // D_ENV_PG_HAS_LOGICAL_REPL
    //   feature: logical replication (publication/subscription model).
    // Introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_LOGICAL_REPL \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_LOGICAL_REPL_COLUMNS
    //   feature: column lists in logical replication publications
    // (PUBLICATION ... FOR TABLE t (col1, col2)). PostgreSQL 15.
    #define D_ENV_PG_HAS_LOGICAL_REPL_COLUMNS \
        D_ENV_PG_AT_LEAST(15)

    // D_ENV_PG_HAS_LOGICAL_REPL_ROW_FILTER
    //   feature: row filter on logical replication publications
    // (WHERE clause). PostgreSQL 15.
    #define D_ENV_PG_HAS_LOGICAL_REPL_ROW_FILTER \
        D_ENV_PG_AT_LEAST(15)

    // D_ENV_PG_HAS_LOGICAL_REPL_SEQUENCES
    //   feature: logical replication of sequences. PostgreSQL 17.
    #define D_ENV_PG_HAS_LOGICAL_REPL_SEQUENCES \
        D_ENV_PG_AT_LEAST(17)

    // D_ENV_PG_HAS_SYNCHRONOUS_COMMIT_QUORUM
    //   feature: quorum-based synchronous commit
    // (synchronous_standby_names = ANY N (...)). PostgreSQL 10.
    #define D_ENV_PG_HAS_SYNCHRONOUS_COMMIT_QUORUM \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_PG_BASEBACKUP_INCREMENTAL
    //   feature: incremental base backups (pg_basebackup --incremental).
    // Introduced in PostgreSQL 17.
    #define D_ENV_PG_HAS_PG_BASEBACKUP_INCREMENTAL \
        D_ENV_PG_AT_LEAST(17)

    // D_ENV_PG_HAS_FAILOVER_SLOTS
    //   feature: failover logical replication slots (slots that survive
    // a standby promotion). PostgreSQL 17.
    #define D_ENV_PG_HAS_FAILOVER_SLOTS \
        D_ENV_PG_AT_LEAST(17)


// =============================================================================
// XII. PARALLEL QUERY AND DDL
// =============================================================================

    // D_ENV_PG_HAS_PARALLEL_QUERY
    //   feature: parallel sequential scans. Introduced in PostgreSQL 9.6.
    #define D_ENV_PG_HAS_PARALLEL_QUERY \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_6)

    // D_ENV_PG_HAS_PARALLEL_INDEX
    //   feature: parallel index scans. Introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_PARALLEL_INDEX \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_PARALLEL_INDEX_BUILD
    //   feature: parallel B-tree index builds (CREATE INDEX).
    // Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_PARALLEL_INDEX_BUILD \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_PARALLEL_HASH_JOIN
    //   feature: parallel hash join. Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_PARALLEL_HASH_JOIN \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_PARALLEL_APPEND
    //   feature: parallel append (for partitioned table scans).
    // Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_PARALLEL_APPEND \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_PARALLEL_VACUUM
    //   feature: parallel VACUUM. Introduced in PostgreSQL 13.
    #define D_ENV_PG_HAS_PARALLEL_VACUUM \
        D_ENV_PG_AT_LEAST(13)


// =============================================================================
// XIII. EXTENSION AND PROCEDURAL LANGUAGE FRAMEWORK
// =============================================================================

    // D_ENV_PG_HAS_CREATE_EXTENSION
    //   feature: CREATE EXTENSION / ALTER EXTENSION framework.
    // Introduced in PostgreSQL 9.1.
    #define D_ENV_PG_HAS_CREATE_EXTENSION \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_1)

    // D_ENV_PG_HAS_FDW
    //   feature: Foreign Data Wrapper (FDW) framework
    // (CREATE FOREIGN TABLE, CREATE SERVER).
    // Introduced in PostgreSQL 9.1.
    #define D_ENV_PG_HAS_FDW \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_1)

    // D_ENV_PG_HAS_FDW_JOIN_PUSH
    //   feature: FDW join pushdown (joining foreign tables server-side).
    // Introduced in PostgreSQL 9.6.
    #define D_ENV_PG_HAS_FDW_JOIN_PUSH \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_6)

    // D_ENV_PG_HAS_FDW_AGG_PUSH
    //   feature: FDW aggregate pushdown (computing aggregates remotely).
    // Introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_FDW_AGG_PUSH \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_PLPGSQL
    //   feature: PL/pgSQL procedural language. Bundled and enabled by
    // default since PostgreSQL 9.0.
    #define D_ENV_PG_HAS_PLPGSQL D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_EVENT_TRIGGERS
    //   feature: event triggers (DDL event hooks).
    // Introduced in PostgreSQL 9.3.
    #define D_ENV_PG_HAS_EVENT_TRIGGERS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_3)


// =============================================================================
// XIV. CONTRIB EXTENSION DETECTION
// =============================================================================
//   Contrib extensions ship with PostgreSQL but must be explicitly enabled
// via CREATE EXTENSION. Detection is via compile-time defines set by the
// extension headers; these are only available if the extension header has
// been included.

    // D_ENV_PG_HAS_HSTORE
    //   feature: detect if hstore extension header is included.
    #ifndef D_ENV_PG_HAS_HSTORE
        #if defined(HSTORE_H)
            #define D_ENV_PG_HAS_HSTORE 1
        #else
            #define D_ENV_PG_HAS_HSTORE 0
        #endif
    #endif

    // D_ENV_PG_HAS_LTREE
    //   feature: detect if ltree extension is available. ltree does not
    // define a unique sentinel macro; manual override recommended.
    #ifndef D_ENV_PG_HAS_LTREE
        #define D_ENV_PG_HAS_LTREE 0
    #endif

    // D_ENV_PG_HAS_CITEXT
    //   feature: detect if citext extension is available.
    #ifndef D_ENV_PG_HAS_CITEXT
        #define D_ENV_PG_HAS_CITEXT 0
    #endif

    // D_ENV_PG_HAS_PG_TRGM
    //   feature: detect if pg_trgm extension is available.
    #ifndef D_ENV_PG_HAS_PG_TRGM
        #define D_ENV_PG_HAS_PG_TRGM 0
    #endif

    // D_ENV_PG_HAS_UUID_OSSP
    //   feature: detect if uuid-ossp extension is available.
    #ifndef D_ENV_PG_HAS_UUID_OSSP
        #define D_ENV_PG_HAS_UUID_OSSP 0
    #endif

    // D_ENV_PG_HAS_PG_STAT_STATEMENTS
    //   feature: detect if pg_stat_statements extension is available.
    #ifndef D_ENV_PG_HAS_PG_STAT_STATEMENTS
        #define D_ENV_PG_HAS_PG_STAT_STATEMENTS 0
    #endif


// =============================================================================
// XV.  THIRD-PARTY EXTENSION DETECTION
// =============================================================================
//   Popular third-party extensions detected via their header macros.

    // D_ENV_PG_HAS_POSTGIS
    //   feature: detect if PostGIS extension headers are included.
    #ifndef D_ENV_PG_HAS_POSTGIS
        #if ( defined(POSTGIS_VERSION)     ||  \
              defined(POSTGIS_LIB_VERSION) )
            #define D_ENV_PG_HAS_POSTGIS 1
        #else
            #define D_ENV_PG_HAS_POSTGIS 0
        #endif
    #endif

    // D_ENV_PG_HAS_PGVECTOR
    //   feature: detect if pgvector (vector similarity search) extension
    // headers are included.
    #ifndef D_ENV_PG_HAS_PGVECTOR
        #if defined(VECTOR_VERSION)
            #define D_ENV_PG_HAS_PGVECTOR 1
        #else
            #define D_ENV_PG_HAS_PGVECTOR 0
        #endif
    #endif

    // D_ENV_PG_HAS_CITUS
    //   feature: detect if Citus distributed PostgreSQL extension is
    // available.
    #ifndef D_ENV_PG_HAS_CITUS
        #if defined(CITUS_VERSION)
            #define D_ENV_PG_HAS_CITUS 1
        #else
            #define D_ENV_PG_HAS_CITUS 0
        #endif
    #endif

    // D_ENV_PG_HAS_TIMESCALEDB
    //   feature: detect if TimescaleDB extension is available.
    #ifndef D_ENV_PG_HAS_TIMESCALEDB
        #if defined(TIMESCALEDB_VERSION)
            #define D_ENV_PG_HAS_TIMESCALEDB 1
        #else
            #define D_ENV_PG_HAS_TIMESCALEDB 0
        #endif
    #endif


// =============================================================================
// XVI. VACUUM AND STORAGE
// =============================================================================

    // D_ENV_PG_HAS_VACUUM_FULL_CONCURRENT
    //   feature: VACUUM (non-FULL) is always concurrent in PostgreSQL.
    // VACUUM FULL is a table-rewrite and takes an exclusive lock.
    #define D_ENV_PG_HAS_VACUUM_FULL_CONCURRENT 0

    // D_ENV_PG_HAS_AUTOVACUUM
    //   feature: autovacuum daemon. Present since PostgreSQL 8.1.
    #define D_ENV_PG_HAS_AUTOVACUUM D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_TOAST
    //   feature: TOAST (The Oversized-Attribute Storage Technique) for
    // transparent large-value compression/out-of-line storage.
    #define D_ENV_PG_HAS_TOAST D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_HEAP_TABLE_AM
    //   feature: pluggable table access method API (CREATE ACCESS METHOD).
    // Introduced in PostgreSQL 12.
    #define D_ENV_PG_HAS_HEAP_TABLE_AM \
        D_ENV_PG_AT_LEAST(12)

    // D_ENV_PG_HAS_WAL_COMPRESSION
    //   feature: wal_compression for reducing WAL volume.
    // Introduced in PostgreSQL 9.5. Additional algorithms (lz4, zstd)
    // added in PostgreSQL 15.
    #define D_ENV_PG_HAS_WAL_COMPRESSION \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_5)

    // D_ENV_PG_HAS_WAL_COMPRESSION_LZ4
    //   feature: lz4 compression for WAL. PostgreSQL 15.
    #define D_ENV_PG_HAS_WAL_COMPRESSION_LZ4 \
        D_ENV_PG_AT_LEAST(15)

    // D_ENV_PG_HAS_WAL_SUMMARIZER
    //   feature: WAL summarizer process (for incremental backups).
    // Introduced in PostgreSQL 17.
    #define D_ENV_PG_HAS_WAL_SUMMARIZER \
        D_ENV_PG_AT_LEAST(17)


// =============================================================================
// XVII. OPTIMIZER AND PERFORMANCE
// =============================================================================

    // D_ENV_PG_HAS_JIT
    //   feature: JIT compilation of queries (via LLVM).
    // Introduced in PostgreSQL 11.
    #define D_ENV_PG_HAS_JIT \
        D_ENV_PG_AT_LEAST(11)

    // D_ENV_PG_HAS_EXTENDED_STATISTICS
    //   feature: CREATE STATISTICS for multi-column dependency and
    // ndistinct statistics. Introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_EXTENDED_STATISTICS \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_EXTENDED_STATISTICS_EXPRESSIONS
    //   feature: expression statistics in CREATE STATISTICS.
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_EXTENDED_STATISTICS_EXPRESSIONS \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_MEMOIZE
    //   feature: Memoize plan node (caching inner loop results).
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_MEMOIZE \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_INCREMENTAL_SORT
    //   feature: incremental sort (exploit partial ordering).
    // Introduced in PostgreSQL 13.
    #define D_ENV_PG_HAS_INCREMENTAL_SORT \
        D_ENV_PG_AT_LEAST(13)

    // D_ENV_PG_HAS_ASYNC_APPEND
    //   feature: Asynchronous Append plan node (parallel foreign scan).
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_ASYNC_APPEND \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_EXPLAIN_WAL
    //   feature: EXPLAIN (WAL) option showing WAL usage.
    // Introduced in PostgreSQL 13.
    #define D_ENV_PG_HAS_EXPLAIN_WAL \
        D_ENV_PG_AT_LEAST(13)


// =============================================================================
// XVIII. SECURITY AND ADMINISTRATION
// =============================================================================

    // D_ENV_PG_HAS_ROLES
    //   feature: SQL roles (replacing users/groups).
    // Introduced in PostgreSQL 8.1.
    #define D_ENV_PG_HAS_ROLES D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_DEFAULT_ROLES
    //   feature: predefined roles (pg_read_all_data, pg_write_all_data,
    // pg_monitor, etc.). Expanded significantly in each release.
    // pg_monitor introduced in PostgreSQL 10.
    #define D_ENV_PG_HAS_DEFAULT_ROLES \
        D_ENV_PG_AT_LEAST(10)

    // D_ENV_PG_HAS_PREDEFINED_ROLES_EXPANDED
    //   feature: pg_read_all_data and pg_write_all_data roles.
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_PREDEFINED_ROLES_EXPANDED \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_PG_STAT_PROGRESS
    //   feature: pg_stat_progress_* views for monitoring long-running
    // operations (VACUUM, CREATE INDEX, CLUSTER, etc.).
    // First introduced for VACUUM in PostgreSQL 9.6.
    #define D_ENV_PG_HAS_PG_STAT_PROGRESS \
        D_ENV_PG_VERSION_AT_LEAST_NUM(D_ENV_PG_VERSION_9_6)

    // D_ENV_PG_HAS_PG_STAT_WAL
    //   feature: pg_stat_wal view for WAL activity monitoring.
    // Introduced in PostgreSQL 14.
    #define D_ENV_PG_HAS_PG_STAT_WAL \
        D_ENV_PG_AT_LEAST(14)

    // D_ENV_PG_HAS_SECURITY_INVOKER_VIEWS
    //   feature: security-invoker views (run with caller's permissions
    // instead of owner's). Introduced in PostgreSQL 15.
    #define D_ENV_PG_HAS_SECURITY_INVOKER_VIEWS \
        D_ENV_PG_AT_LEAST(15)


// =============================================================================
// XIX.  PLATFORM INTEGRATION
// =============================================================================

    // D_ENV_PG_HAS_UNIX_SOCKET
    //   feature: Unix domain socket connections.
    #ifndef D_ENV_PG_HAS_UNIX_SOCKET
        #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
            #define D_ENV_PG_HAS_UNIX_SOCKET 1
        #else
            #define D_ENV_PG_HAS_UNIX_SOCKET 0
        #endif
    #endif

    // D_ENV_PG_HAS_LARGE_OBJECT
    //   feature: large object API (lo_*). Core feature, always present.
    #define D_ENV_PG_HAS_LARGE_OBJECT D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_LISTEN_NOTIFY
    //   feature: LISTEN/NOTIFY asynchronous notification. Core feature.
    #define D_ENV_PG_HAS_LISTEN_NOTIFY D_ENV_PG_DETECTED

    // D_ENV_PG_HAS_ADVISORY_LOCKS
    //   feature: advisory locks (pg_advisory_lock). Core feature since
    // PostgreSQL 8.2.
    #define D_ENV_PG_HAS_ADVISORY_LOCKS D_ENV_PG_DETECTED


// =============================================================================
// XX.   CONVENIENCE / COMPOSITE MACROS
// =============================================================================

    // D_ENV_PG_HAS_MODERN_SQL
    //   macro: evaluates to 1 if CTEs, window functions, LATERAL,
    // UPSERT, MERGE, and row-level security are all available.
    #define D_ENV_PG_HAS_MODERN_SQL \
        ( D_ENV_PG_HAS_CTE                 && \
          D_ENV_PG_HAS_WINDOW_FUNCTIONS    && \
          D_ENV_PG_HAS_LATERAL             && \
          D_ENV_PG_HAS_UPSERT             && \
          D_ENV_PG_HAS_MERGE              && \
          D_ENV_PG_HAS_ROW_LEVEL_SECURITY )

    // D_ENV_PG_HAS_MODERN_JSON
    //   macro: evaluates to 1 if JSONB, JSON path queries, subscripting,
    // and SQL/JSON constructors are all available.
    #define D_ENV_PG_HAS_MODERN_JSON \
        ( D_ENV_PG_HAS_JSONB              && \
          D_ENV_PG_HAS_JSONPATH           && \
          D_ENV_PG_HAS_JSONB_SUBSCRIPT    && \
          D_ENV_PG_HAS_SQLJSON_CONSTRUCTORS )

    // D_ENV_PG_HAS_MODERN_PARTITIONING
    //   macro: evaluates to 1 if declarative partitioning with hash,
    // default partitions, and foreign keys are all available.
    #define D_ENV_PG_HAS_MODERN_PARTITIONING \
        ( D_ENV_PG_HAS_DECLARATIVE_PARTITIONING && \
          D_ENV_PG_HAS_HASH_PARTITIONING        && \
          D_ENV_PG_HAS_DEFAULT_PARTITION         && \
          D_ENV_PG_HAS_PARTITION_FOREIGN_KEY )

    // D_ENV_PG_HAS_MODERN_REPLICATION
    //   macro: evaluates to 1 if logical replication with column lists,
    // row filters, and replication slots are all available.
    #define D_ENV_PG_HAS_MODERN_REPLICATION \
        ( D_ENV_PG_HAS_LOGICAL_REPL            && \
          D_ENV_PG_HAS_LOGICAL_REPL_COLUMNS    && \
          D_ENV_PG_HAS_LOGICAL_REPL_ROW_FILTER && \
          D_ENV_PG_HAS_REPLICATION_SLOTS )

    // D_ENV_PG_HAS_MODERN_OPTIMIZER
    //   macro: evaluates to 1 if parallel query, JIT, memoize, and
    // incremental sort are all available.
    #define D_ENV_PG_HAS_MODERN_OPTIMIZER \
        ( D_ENV_PG_HAS_PARALLEL_QUERY     && \
          D_ENV_PG_HAS_JIT                && \
          D_ENV_PG_HAS_MEMOIZE            && \
          D_ENV_PG_HAS_INCREMENTAL_SORT )

    // D_ENV_PG_HAS_MODERN_AUTH
    //   macro: evaluates to 1 if SCRAM-SHA-256 and channel binding are
    // available.
    #define D_ENV_PG_HAS_MODERN_AUTH \
        ( D_ENV_PG_HAS_SCRAM_SHA_256 && \
          D_ENV_PG_HAS_CHANNEL_BINDING )

    // D_ENV_PG_IS_FULLY_MODERN
    //   macro: evaluates to 1 if the PostgreSQL version has all "modern"
    // capabilities (roughly PostgreSQL 16+ with comprehensive features).
    #define D_ENV_PG_IS_FULLY_MODERN \
        ( D_ENV_PG_HAS_MODERN_SQL          && \
          D_ENV_PG_HAS_MODERN_JSON         && \
          D_ENV_PG_HAS_MODERN_PARTITIONING && \
          D_ENV_PG_HAS_MODERN_REPLICATION  && \
          D_ENV_PG_HAS_MODERN_OPTIMIZER    && \
          D_ENV_PG_HAS_MODERN_AUTH )


#endif  // D_ENV_PG_DETECTED


#endif  // DJINTERP_ENVIRONMENT_POSTGRESQL_
