/*******************************************************************************
* djinterp [env]                                                       env_db2.h
*
* djinterp IBM Db2 environment detection.
*   Compile-time detection of an IBM Db2 environment: the CLI/ODBC client
* (sqlcli1.h) and its version, the target server version, the platform family
* (Db2 for Linux, UNIX and Windows; Db2 for z/OS; or Db2 for i), and the
* capabilities those gate: CLI features, SQL, data types, transactions,
* storage, high availability and replication, federation, security, and
* programmability.
*   Two versions are tracked. D_ENV_DB2_CLIENT_* is the CLI client's: its
* presence, and a version only when sqlcli1.h defines SQLCLI_VER or one is
* supplied manually. D_ENV_DB2_SERVER_* is the target server's, a runtime
* property that must be configured, through D_CFG_ENV_DB2_SERVER_VERSION or a
* pre-defined D_ENV_DB2_DETECTED_SERVER_*; without one, server-gated features
* read 0. Versions are encoded as MAJOR*10000 + MINOR*100 + PATCH on the LUW
* line, so Db2 11.5 is 110500; z/OS and Db2 for i targets must be mapped onto
* an equivalent value.
*   D_ENV_DB2_HAS_* are capability flags, and D_ENV_DB2_IS_* the platform and
* release series. Sections 6 onward exist only when a client or a server
* version is known; the last of them publishes the extra names the connection
* layer uses.
*   Settings live in env_db2_config.h: D_CFG_ENV_USING_DB2 includes the CLI
* header, D_CFG_ENV_DB2_CUSTOM switches the client to manual detection, and
* D_CFG_ENV_DB2_PLATFORM names the platform family. This header also includes
* env_db.h, for the base database detection.
*
*
* path:      /inc/djinterp/env/db/db2/env_db2.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION
    -----------------------
    1.  Vendor header
         1.  D_ENV_DB2_HEADER_INCLUDED
2.  VERSION ENCODING
    ----------------
    1.  Encoding and decoding
         1.  D_ENV_DB2_ENCODE_VERSION
         2.  D_ENV_DB2_DECODE_MAJOR
         3.  D_ENV_DB2_DECODE_MINOR
         4.  D_ENV_DB2_DECODE_PATCH
3.  CLIENT VERSION DETECTION
    ------------------------
    1.  Server release IDs
         1.  D_ENV_DB2_SERVER_<MAJOR>_<MINOR>
    2.  Detected client
         1.  D_ENV_DB2_CLIENT_DETECTED / D_ENV_DB2_CLIENT_*
4.  TARGET SERVER VERSION DETECTION
    -------------------------------
    1.  Target server version
         1.  D_ENV_DB2_SERVER_VERSION_ID
         2.  D_ENV_DB2_SERVER_MAJOR / D_ENV_DB2_SERVER_MINOR
         3.  D_ENV_DB2_SERVER_KNOWN
         4.  D_ENV_DB2_DETECTED
5.  PLATFORM FAMILY
    ---------------
    1.  Platform flags
         1.  D_ENV_DB2_IS_LUW
         2.  D_ENV_DB2_IS_ZOS
         3.  D_ENV_DB2_IS_ISERIES
6.  VERSION COMPARISON MACROS
    -------------------------
    1.  Server comparisons
         1.  D_ENV_DB2_SERVER_AT_LEAST
         2.  D_ENV_DB2_SERVER_BELOW
         3.  D_ENV_DB2_SERVER_IN_RANGE
    2.  Release series
         1.  D_ENV_DB2_IS_<SERIES>
7.  CLIENT (CLI) FEATURES
    ---------------------
    1.  CLI features
         1.  D_ENV_DB2_HAS_CLI
         2.  D_ENV_DB2_HAS_CLI_VERSION
         3.  D_ENV_DB2_HAS_ODBC_COMPAT
         4.  D_ENV_DB2_HAS_XA
8.  SQL LANGUAGE FEATURES
    ---------------------
    1.  SQL
         1.  D_ENV_DB2_HAS_CTE
         2.  D_ENV_DB2_HAS_MERGE
         3.  D_ENV_DB2_HAS_WINDOW_FUNCTIONS
         4.  D_ENV_DB2_HAS_GROUPING_SETS
         5.  D_ENV_DB2_HAS_MQT
         6.  D_ENV_DB2_HAS_TEMPORAL_TABLES
         7.  D_ENV_DB2_HAS_OLAP_LIMIT
9.  DATA TYPES
    ----------
    1.  Data types
         1.  D_ENV_DB2_HAS_LOB
         2.  D_ENV_DB2_HAS_PUREXML
         3.  D_ENV_DB2_HAS_DECFLOAT
         4.  D_ENV_DB2_HAS_BOOLEAN_TYPE
         5.  D_ENV_DB2_HAS_BINARY_TYPE
         6.  D_ENV_DB2_HAS_JSON_FUNCTIONS
10. TRANSACTIONS AND CONCURRENCY
    ----------------------------
    1.  Transactions and concurrency
         1.  D_ENV_DB2_HAS_CURRENTLY_COMMITTED
11. STORAGE AND TABLE ORGANIZATION
    ------------------------------
    1.  Storage and tables
         1.  D_ENV_DB2_HAS_RANGE_PARTITIONING
         2.  D_ENV_DB2_HAS_MDC
         3.  D_ENV_DB2_HAS_DPF
         4.  D_ENV_DB2_HAS_COLUMN_ORGANIZED
12. HIGH AVAILABILITY AND REPLICATION
    ---------------------------------
    1.  Availability and replication
         1.  D_ENV_DB2_HAS_HADR
         2.  D_ENV_DB2_HAS_HADR_MULTI_STANDBY
         3.  D_ENV_DB2_HAS_PURESCALE
         4.  D_ENV_DB2_HAS_DATA_SHARING
         5.  D_ENV_DB2_HAS_Q_REPLICATION
13. FEDERATION AND EXTERNAL DATA
    ----------------------------
    1.  Federation
         1.  D_ENV_DB2_HAS_FEDERATION
         2.  D_ENV_DB2_HAS_EXTERNAL_TABLES
14. SECURITY
    --------
    1.  Security
         1.  D_ENV_DB2_HAS_LBAC
         2.  D_ENV_DB2_HAS_RCAC
         3.  D_ENV_DB2_HAS_NATIVE_ENCRYPTION
         4.  D_ENV_DB2_HAS_SSL
         5.  D_ENV_DB2_HAS_KERBEROS
         6.  D_ENV_DB2_HAS_AUDIT
15. PROGRAMMABILITY
    ---------------
    1.  Programmability
         1.  D_ENV_DB2_HAS_SQL_PL
         2.  D_ENV_DB2_HAS_EXTERNAL_ROUTINES
         3.  D_ENV_DB2_HAS_COMPATIBILITY_MODE
         4.  D_ENV_DB2_HAS_ANCHORED_TYPES
16. COMPOSITE CHECKS
    ----------------
    1.  Composite checks
         1.  D_ENV_DB2_HAS_FULL_ACID
         2.  D_ENV_DB2_HAS_ANALYTICS
         3.  D_ENV_DB2_HAS_ADVANCED_SECURITY
         4.  D_ENV_DB2_HAS_ENTERPRISE_HA
         5.  D_ENV_DB2_IS_FULLY_MODERN
17. DEPRECATION AND REMOVAL
    -----------------------
    1.  Deprecations
         1.  D_ENV_DB2_REMOVED_TYPE1_INDEXES
         2.  D_ENV_DB2_DEPRECATED_CLP_LEGACY
18. CONSUMER COMPATIBILITY LAYER
    ----------------------------
    1.  Consumer vocabulary
         1.  D_ENV_DB2_VERSION_*
         2.  Consumer feature aliases
*/

#ifndef DJINTERP_ENV_DB_DB2_ENV_DB2_H
#define DJINTERP_ENV_DB_DB2_ENV_DB2_H 1

// djinterp
#include "./env_db2_config.h"  // D_CFG_ENV_*
#include "../env_db.h"         // base database detection (D_ENV_DB_*)


//==============================================================================
// 1.  VENDOR HEADER INCLUSION
//==============================================================================
// Driven by D_CFG_ENV_USING_DB2, from env_db2_config.h. When on, this section
// includes the Db2 CLI/ODBC client header (sqlcli1.h), a C API usable from C
// and C++ alike. Detection below is gated on D_ENV_DB2_HEADER_INCLUDED, so
// that no Db2 symbol is referenced unless the header is in scope.


// 1.1    Vendor header
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_DB2_HEADER_INCLUDED
//   detection: 1 once this section has included the CLI header, and 0 when
// D_CFG_ENV_USING_DB2 is off; D_ENV_DB_HAS_DB2_CLIENT_C follows it unless
// pre-defined. With the setting on, a missing header is an #error.
#if (D_CFG_ENV_USING_DB2 == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_DB2_C_PATH)
            #include D_CFG_ENV_DB2_C_PATH  // CLI client
            #define D_ENV_DB2_HEADER_INCLUDED 1
        #elif __has_include(<sqlcli1.h>)
            // db2
            #include <sqlcli1.h>  // CLI client
            #define D_ENV_DB2_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_DB2=1 but no sqlcli1.h header was "        \
                   "found. Install the IBM Db2 client/driver (Data Server "    \
                   "Driver or equivalent), or define D_CFG_ENV_DB2_C_PATH "    \
                   "to the correct location."
        #endif
    #else
        #include D_CFG_ENV_DB2_C_PATH  // CLI client
        #define D_ENV_DB2_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_DB2_CLIENT_C
        #define D_ENV_DB_HAS_DB2_CLIENT_C 1
    #endif  // D_ENV_DB_HAS_DB2_CLIENT_C

#else
    #define D_ENV_DB2_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_DB2_CLIENT_C
        #define D_ENV_DB_HAS_DB2_CLIENT_C 0
    #endif  // D_ENV_DB_HAS_DB2_CLIENT_C
#endif  // D_CFG_ENV_USING_DB2


//==============================================================================
// 2.  VERSION ENCODING
//==============================================================================


// 2.1    Encoding and decoding
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_DB2_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple.
#define D_ENV_DB2_ENCODE_VERSION(major, minor, patch)                          \
    ((major) * 10000 + (minor) * 100 + (patch))

// 2.1.2
// D_ENV_DB2_DECODE_MAJOR
//   macro: extracts the major version.
#define D_ENV_DB2_DECODE_MAJOR(ver)                                            \
    ((ver) / 10000)

// 2.1.3
// D_ENV_DB2_DECODE_MINOR
//   macro: extracts the minor version.
#define D_ENV_DB2_DECODE_MINOR(ver)                                            \
    (((ver) / 100) % 100)

// 2.1.4
// D_ENV_DB2_DECODE_PATCH
//   macro: extracts the patch version.
#define D_ENV_DB2_DECODE_PATCH(ver)                                            \
    ((ver) % 100)


//==============================================================================
// 3.  CLIENT VERSION DETECTION
//==============================================================================
// The Db2 CLI driver advertises its version through the SQL_DBMS_VER /
// driver version reported at runtime, but at compile time the most reliable
// signal is the SQLCLI_VER macro (when present) or a manually supplied
// version. Because sqlcli1.h does not universally expose a numeric compile-
// time version macro, automatic client detection is best-effort: it reports
// "present" from the header inclusion and a version when SQLCLI_VER is
// available.


// 3.1    Server release IDs
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_DB2_SERVER_<MAJOR>_<MINOR>
//   constant: encoded IDs of the LUW server releases that gate features below.
#define D_ENV_DB2_SERVER_9_5           90500
#define D_ENV_DB2_SERVER_9_7           90700
#define D_ENV_DB2_SERVER_10_1          100100
#define D_ENV_DB2_SERVER_10_5          100500
#define D_ENV_DB2_SERVER_11_1          110100
#define D_ENV_DB2_SERVER_11_5          110500
#define D_ENV_DB2_SERVER_12_1          120100

// 3.2    Detected client
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_DB2_CLIENT_DETECTED / D_ENV_DB2_CLIENT_*
//   detection: D_ENV_DB2_CLIENT_DETECTED is 1 when the CLI client is present,
// and D_ENV_DB2_CLIENT_VERSION_KNOWN when its version is known too. Automatic
// mode reports the client once sqlcli1.h is included, with SQLCLI_VER, when
// defined, as D_ENV_DB2_CLIENT_VERSION_RAW. Manual mode (D_CFG_ENV_DB2_CUSTOM)
// takes D_ENV_DB2_DETECTED_CLIENT_VERSION, and also defines
// D_ENV_DB2_CLIENT_VERSION_ID, _MAJOR, _MINOR and _PATCH.
#if (D_CFG_ENV_DB2_CUSTOM == 0)

    #if D_ENV_DB2_HEADER_INCLUDED
        #define D_ENV_DB2_CLIENT_DETECTED      1
        #if defined(SQLCLI_VER)
            // SQLCLI_VER, when present, is an integer such as 1150.
            #define D_ENV_DB2_CLIENT_VERSION_RAW   SQLCLI_VER
            #define D_ENV_DB2_CLIENT_VERSION_KNOWN 1
        #else
            #define D_ENV_DB2_CLIENT_VERSION_KNOWN 0
        #endif
    #else
        #define D_ENV_DB2_CLIENT_DETECTED      0
        #define D_ENV_DB2_CLIENT_VERSION_KNOWN 0
    #endif

#else
    // manual mode
    #ifdef D_ENV_DB2_DETECTED_CLIENT_VERSION
        #define D_ENV_DB2_CLIENT_DETECTED      1
        #define D_ENV_DB2_CLIENT_VERSION_ID                                    \
            D_ENV_DB2_DETECTED_CLIENT_VERSION
        #define D_ENV_DB2_CLIENT_VERSION_KNOWN 1
        #define D_ENV_DB2_CLIENT_MAJOR                                         \
            D_ENV_DB2_DECODE_MAJOR(D_ENV_DB2_DETECTED_CLIENT_VERSION)
        #define D_ENV_DB2_CLIENT_MINOR                                         \
            D_ENV_DB2_DECODE_MINOR(D_ENV_DB2_DETECTED_CLIENT_VERSION)
        #define D_ENV_DB2_CLIENT_PATCH                                         \
            D_ENV_DB2_DECODE_PATCH(D_ENV_DB2_DETECTED_CLIENT_VERSION)
    #else
        #define D_ENV_DB2_CLIENT_DETECTED      0
        #define D_ENV_DB2_CLIENT_VERSION_KNOWN 0
    #endif  // D_ENV_DB2_DETECTED_CLIENT_VERSION

#endif  // D_CFG_ENV_DB2_CUSTOM


//==============================================================================
// 4.  TARGET SERVER VERSION DETECTION
//==============================================================================
// The server version is a runtime property. These macros allow compile-
// time gating against a known deployment target. Set manually via
// D_CFG_ENV_DB2_SERVER_VERSION or D_ENV_DB2_DETECTED_SERVER_*. Version
// constants below follow the LUW line; z/OS and Db2 for i targets should be
// mapped onto an equivalent encoded value by the integrator.


// 4.1    Target server version
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_DB2_SERVER_VERSION_ID
//   detection: the encoded target server version, from
// D_CFG_ENV_DB2_SERVER_VERSION, D_ENV_DB2_DETECTED_SERVER_VERSION, or the
// newest D_ENV_DB2_DETECTED_SERVER_<MAJOR>_<MINOR> defined, in that order; 0
// when none is.
#ifndef D_ENV_DB2_SERVER_VERSION_ID
    #ifdef D_CFG_ENV_DB2_SERVER_VERSION
        #define D_ENV_DB2_SERVER_VERSION_ID                                    \
            D_CFG_ENV_DB2_SERVER_VERSION
    #elif defined(D_ENV_DB2_DETECTED_SERVER_VERSION)
        #define D_ENV_DB2_SERVER_VERSION_ID                                    \
            D_ENV_DB2_DETECTED_SERVER_VERSION
    #elif defined(D_ENV_DB2_DETECTED_SERVER_12_1)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_12_1
    #elif defined(D_ENV_DB2_DETECTED_SERVER_11_5)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_11_5
    #elif defined(D_ENV_DB2_DETECTED_SERVER_11_1)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_11_1
    #elif defined(D_ENV_DB2_DETECTED_SERVER_10_5)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_10_5
    #elif defined(D_ENV_DB2_DETECTED_SERVER_10_1)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_10_1
    #elif defined(D_ENV_DB2_DETECTED_SERVER_9_7)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_9_7
    #elif defined(D_ENV_DB2_DETECTED_SERVER_9_5)
        #define D_ENV_DB2_SERVER_VERSION_ID D_ENV_DB2_SERVER_9_5
    #else
        // no server version specified; default to 0 (unknown).
        // server-gated features will evaluate to 0.
        #define D_ENV_DB2_SERVER_VERSION_ID 0
    #endif  // D_CFG_ENV_DB2_SERVER_VERSION
#endif  // D_ENV_DB2_SERVER_VERSION_ID

// 4.1.2
// D_ENV_DB2_SERVER_MAJOR / D_ENV_DB2_SERVER_MINOR
//   macro: the major and minor parts of D_ENV_DB2_SERVER_VERSION_ID.
#define D_ENV_DB2_SERVER_MAJOR                                                 \
    D_ENV_DB2_DECODE_MAJOR(D_ENV_DB2_SERVER_VERSION_ID)
#define D_ENV_DB2_SERVER_MINOR                                                 \
    D_ENV_DB2_DECODE_MINOR(D_ENV_DB2_SERVER_VERSION_ID)

// 4.1.3
// D_ENV_DB2_SERVER_KNOWN
//   status: 1 if a target server version has been configured.
#define D_ENV_DB2_SERVER_KNOWN                                                 \
    (D_ENV_DB2_SERVER_VERSION_ID > 0)

// 4.1.4
// D_ENV_DB2_DETECTED
//   detection: 1 if the CLI client is detected or a target server version is
// known.
#define D_ENV_DB2_DETECTED                                                     \
    ( (D_ENV_DB2_CLIENT_DETECTED) ||                                           \
      (D_ENV_DB2_SERVER_KNOWN) )


//==============================================================================
// 5.  PLATFORM FAMILY
//==============================================================================
// The platform family is supplied via D_CFG_ENV_DB2_PLATFORM (see
// env_db2_config.h). These convenience macros expose it as boolean flags.


// 5.1    Platform flags
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_DB2_IS_LUW
//   detection: 1 if target is Db2 for Linux, UNIX, and Windows.
#define D_ENV_DB2_IS_LUW                                                       \
    (D_CFG_ENV_DB2_PLATFORM == D_CFG_ENV_DB2_PLATFORM_LUW)

// 5.1.2
// D_ENV_DB2_IS_ZOS
//   detection: 1 if target is Db2 for z/OS.
#define D_ENV_DB2_IS_ZOS                                                       \
    (D_CFG_ENV_DB2_PLATFORM == D_CFG_ENV_DB2_PLATFORM_ZOS)

// 5.1.3
// D_ENV_DB2_IS_ISERIES
//   detection: 1 if target is Db2 for i (IBM i / iSeries).
#define D_ENV_DB2_IS_ISERIES                                                   \
    (D_CFG_ENV_DB2_PLATFORM == D_CFG_ENV_DB2_PLATFORM_ISERIES)


// sections 6 to 18 exist only when D_ENV_DB2_DETECTED is 1; see 4.1.4
#if D_ENV_DB2_DETECTED


//==============================================================================
// 6.  VERSION COMPARISON MACROS
//==============================================================================


// 6.1    Server comparisons
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_DB2_SERVER_AT_LEAST
    //   macro: 1 if the target server version is at least major.minor.patch.
    #define D_ENV_DB2_SERVER_AT_LEAST(major, minor, patch)                     \
        (D_ENV_DB2_SERVER_VERSION_ID >=                                        \
            D_ENV_DB2_ENCODE_VERSION(major, minor, patch))

    // 6.1.2
    // D_ENV_DB2_SERVER_BELOW
    //   macro: 1 if the target server version is below major.minor.patch.
    #define D_ENV_DB2_SERVER_BELOW(major, minor, patch)                        \
        (D_ENV_DB2_SERVER_VERSION_ID <                                         \
            D_ENV_DB2_ENCODE_VERSION(major, minor, patch))

    // 6.1.3
    // D_ENV_DB2_SERVER_IN_RANGE
    //   macro: 1 if the target server version is at least the first triple
    // and below the second.
    #define D_ENV_DB2_SERVER_IN_RANGE(min_maj, min_min, min_pat,               \
                                      max_maj, max_min, max_pat)               \
        ( (D_ENV_DB2_SERVER_AT_LEAST(min_maj, min_min, min_pat)) &&            \
          (D_ENV_DB2_SERVER_BELOW(max_maj, max_min, max_pat)) )

// 6.2    Release series
//------------------------------------------------------------------------------
    // 6.2.1
    // D_ENV_DB2_IS_<SERIES>
    //   macro: 1 if the target server version is in that LUW release series,
    // 9.7 to 12.1; D_ENV_DB2_IS_12_1 also covers every later release.
    #define D_ENV_DB2_IS_9_7                                                   \
        D_ENV_DB2_SERVER_IN_RANGE(9, 7, 0, 10, 1, 0)
    #define D_ENV_DB2_IS_10_1                                                  \
        D_ENV_DB2_SERVER_IN_RANGE(10, 1, 0, 10, 5, 0)
    #define D_ENV_DB2_IS_10_5                                                  \
        D_ENV_DB2_SERVER_IN_RANGE(10, 5, 0, 11, 1, 0)
    #define D_ENV_DB2_IS_11_1                                                  \
        D_ENV_DB2_SERVER_IN_RANGE(11, 1, 0, 11, 5, 0)
    #define D_ENV_DB2_IS_11_5                                                  \
        D_ENV_DB2_SERVER_IN_RANGE(11, 5, 0, 12, 1, 0)
    #define D_ENV_DB2_IS_12_1                                                  \
        D_ENV_DB2_SERVER_AT_LEAST(12, 1, 0)


//==============================================================================
// 7.  CLIENT (CLI) FEATURES
//==============================================================================


// 7.1    CLI features
//------------------------------------------------------------------------------
    // 7.1.1
    // D_ENV_DB2_HAS_CLI
    //   feature: detect if the Db2 CLI/ODBC client is available.
    #define D_ENV_DB2_HAS_CLI D_ENV_DB2_CLIENT_DETECTED

    // 7.1.2
    // D_ENV_DB2_HAS_CLI_VERSION
    //   status: 1 if a numeric client version is known at compile time.
    #define D_ENV_DB2_HAS_CLI_VERSION D_ENV_DB2_CLIENT_VERSION_KNOWN

    // 7.1.3
    // D_ENV_DB2_HAS_ODBC_COMPAT
    //   feature: the CLI is ODBC-compatible (SQLAllocHandle, SQLConnect,
    // SQLExecDirect, etc.). Always true when the CLI is present.
    #define D_ENV_DB2_HAS_ODBC_COMPAT D_ENV_DB2_CLIENT_DETECTED

    // 7.1.4
    // D_ENV_DB2_HAS_XA
    //   feature: XA distributed-transaction (two-phase commit)
    // interface via the CLI. Present in all supported drivers.
    #define D_ENV_DB2_HAS_XA D_ENV_DB2_CLIENT_DETECTED


//==============================================================================
// 8.  SQL LANGUAGE FEATURES
//==============================================================================


// 8.1    SQL
//------------------------------------------------------------------------------
    // 8.1.1
    // D_ENV_DB2_HAS_CTE
    //   feature: common table expressions (WITH clause), including
    // recursive CTEs. Long-standing core SQL feature.
    #define D_ENV_DB2_HAS_CTE                                                  \
        D_ENV_DB2_SERVER_KNOWN

    // 8.1.2
    // D_ENV_DB2_HAS_MERGE
    //   feature: the MERGE statement (upsert). Core SQL feature in modern
    // Db2.
    #define D_ENV_DB2_HAS_MERGE                                                \
        D_ENV_DB2_SERVER_KNOWN

    // 8.1.3
    // D_ENV_DB2_HAS_WINDOW_FUNCTIONS
    //   feature: OLAP / window functions (OVER, PARTITION BY, ranking).
    // Core feature.
    #define D_ENV_DB2_HAS_WINDOW_FUNCTIONS                                     \
        D_ENV_DB2_SERVER_KNOWN

    // 8.1.4
    // D_ENV_DB2_HAS_GROUPING_SETS
    //   feature: GROUPING SETS, ROLLUP, CUBE super-aggregates. Core
    // feature.
    #define D_ENV_DB2_HAS_GROUPING_SETS                                        \
        D_ENV_DB2_SERVER_KNOWN

    // 8.1.5
    // D_ENV_DB2_HAS_MQT
    //   feature: materialized query tables (MQTs / summary tables). Core
    // feature on LUW and z/OS.
    #define D_ENV_DB2_HAS_MQT                                                  \
        D_ENV_DB2_SERVER_KNOWN

    // 8.1.6
    // D_ENV_DB2_HAS_TEMPORAL_TABLES
    //   feature: temporal tables (system-time, business-time, bitemporal).
    // Introduced in LUW 10.1 / z/OS 10.
    #define D_ENV_DB2_HAS_TEMPORAL_TABLES                                      \
        D_ENV_DB2_SERVER_AT_LEAST(10, 1, 0)

    // 8.1.7
    // D_ENV_DB2_HAS_OLAP_LIMIT
    //   feature: FETCH FIRST n ROWS ONLY / OFFSET row-limiting clauses.
    // Core feature.
    #define D_ENV_DB2_HAS_ROW_LIMITING                                         \
        D_ENV_DB2_SERVER_KNOWN


//==============================================================================
// 9.  DATA TYPES
//==============================================================================


// 9.1    Data types
//------------------------------------------------------------------------------
    // 9.1.1
    // D_ENV_DB2_HAS_LOB
    //   feature: large-object types (BLOB, CLOB, DBCLOB). Core feature.
    #define D_ENV_DB2_HAS_LOB                                                  \
        D_ENV_DB2_SERVER_KNOWN

    // 9.1.2
    // D_ENV_DB2_HAS_PUREXML
    //   feature: native XML storage and XQuery (pureXML, the XML data
    // type). Introduced in Db2 9.
    #define D_ENV_DB2_HAS_PUREXML                                              \
        D_ENV_DB2_SERVER_AT_LEAST(9, 5, 0)

    // 9.1.3
    // D_ENV_DB2_HAS_DECFLOAT
    //   feature: the DECFLOAT decimal floating-point type. Introduced in
    // Db2 9.5.
    #define D_ENV_DB2_HAS_DECFLOAT                                             \
        D_ENV_DB2_SERVER_AT_LEAST(9, 5, 0)

    // 9.1.4
    // D_ENV_DB2_HAS_BOOLEAN_TYPE
    //   feature: the BOOLEAN data type (in SQL, not just routines).
    // Introduced in LUW 11.1.
    #define D_ENV_DB2_HAS_BOOLEAN_TYPE                                         \
        D_ENV_DB2_SERVER_AT_LEAST(11, 1, 0)

    // 9.1.5
    // D_ENV_DB2_HAS_BINARY_TYPE
    //   feature: the BINARY / VARBINARY data types. Introduced in
    // LUW 11.1.
    #define D_ENV_DB2_HAS_BINARY_TYPE                                          \
        D_ENV_DB2_SERVER_AT_LEAST(11, 1, 0)

    // 9.1.6
    // D_ENV_DB2_HAS_JSON_FUNCTIONS
    //   feature: SQL/JSON functions (JSON_VALUE, JSON_TABLE, JSON_QUERY,
    // ISO SQL JSON). Available in LUW 11.1 (early ISO support) and
    // expanded in 11.5.
    #define D_ENV_DB2_HAS_JSON_FUNCTIONS                                       \
        D_ENV_DB2_SERVER_AT_LEAST(11, 1, 0)


//==============================================================================
// 10.  TRANSACTIONS AND CONCURRENCY
//==============================================================================


// 10.1   Transactions and concurrency
//------------------------------------------------------------------------------
    // D_ENV_DB2_HAS_TRANSACTIONS / SAVEPOINTS / TWO_PHASE_COMMIT
    //   feature: full transactional support, savepoints, and XA-style
    // two-phase commit. Core features.
    #define D_ENV_DB2_HAS_TRANSACTIONS                                         \
        D_ENV_DB2_SERVER_KNOWN
    #define D_ENV_DB2_HAS_SAVEPOINTS                                           \
        D_ENV_DB2_SERVER_KNOWN
    #define D_ENV_DB2_HAS_TWO_PHASE_COMMIT                                     \
        D_ENV_DB2_SERVER_KNOWN

    // 10.1.1
    // D_ENV_DB2_HAS_CURRENTLY_COMMITTED
    //   feature: currently-committed semantics (readers do not block
    // writers under CS isolation). Introduced in LUW 9.7.
    #define D_ENV_DB2_HAS_CURRENTLY_COMMITTED                                  \
        D_ENV_DB2_SERVER_AT_LEAST(9, 7, 0)


//==============================================================================
// 11.  STORAGE AND TABLE ORGANIZATION
//==============================================================================


// 11.1   Storage and tables
//------------------------------------------------------------------------------
    // 11.1.1
    // D_ENV_DB2_HAS_RANGE_PARTITIONING
    //   feature: table (range) partitioning. Introduced in Db2 9.
    #define D_ENV_DB2_HAS_RANGE_PARTITIONING                                   \
        D_ENV_DB2_SERVER_AT_LEAST(9, 5, 0)

    // 11.1.2
    // D_ENV_DB2_HAS_MDC
    //   feature: multidimensional clustering (MDC) tables. Core LUW
    // feature.
    #define D_ENV_DB2_HAS_MDC                                                  \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_KNOWN) )

    // 11.1.3
    // D_ENV_DB2_HAS_DPF
    //   feature: database partitioning feature (shared-nothing hash
    // partitioning across nodes). LUW only.
    #define D_ENV_DB2_HAS_DPF                                                  \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_KNOWN) )

    // 11.1.4
    // D_ENV_DB2_HAS_COLUMN_ORGANIZED
    //   feature: column-organized tables / BLU Acceleration (in-memory
    // columnar with actionable compression). Introduced in LUW 10.5.
    #define D_ENV_DB2_HAS_COLUMN_ORGANIZED                                     \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_AT_LEAST(10, 5, 0)) )


//==============================================================================
// 12.  HIGH AVAILABILITY AND REPLICATION
//==============================================================================


// 12.1   Availability and replication
//------------------------------------------------------------------------------
    // 12.1.1
    // D_ENV_DB2_HAS_HADR
    //   feature: High Availability Disaster Recovery (HADR) log shipping
    // with synchronous/near-sync/async modes. LUW feature; multi-standby
    // since 10.1.
    #define D_ENV_DB2_HAS_HADR                                                 \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_KNOWN) )

    // 12.1.2
    // D_ENV_DB2_HAS_HADR_MULTI_STANDBY
    //   feature: HADR with multiple standby databases. Introduced in
    // LUW 10.1.
    #define D_ENV_DB2_HAS_HADR_MULTI_STANDBY                                   \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_AT_LEAST(10, 1, 0)) )

    // 12.1.3
    // D_ENV_DB2_HAS_PURESCALE
    //   feature: Db2 pureScale (shared-data active-active clustering).
    // LUW feature, generally available from 9.8/10.x.
    #define D_ENV_DB2_HAS_PURESCALE                                            \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_AT_LEAST(10, 1, 0)) )

    // 12.1.4
    // D_ENV_DB2_HAS_DATA_SHARING
    //   feature: data sharing across a Parallel Sysplex. z/OS analogue
    // of pureScale.
    #define D_ENV_DB2_HAS_DATA_SHARING                                         \
        ( (D_ENV_DB2_IS_ZOS) &&                                                \
          (D_ENV_DB2_SERVER_KNOWN) )

    // 12.1.5
    // D_ENV_DB2_HAS_Q_REPLICATION
    //   feature: Q Replication (queue-based async replication via MQ).
    // Available across platforms with the replication feature installed.
    #define D_ENV_DB2_HAS_Q_REPLICATION                                        \
        D_ENV_DB2_SERVER_KNOWN


//==============================================================================
// 13.  FEDERATION AND EXTERNAL DATA
//==============================================================================


// 13.1   Federation
//------------------------------------------------------------------------------
    // 13.1.1
    // D_ENV_DB2_HAS_FEDERATION
    //   feature: federated access to remote data sources via nicknames
    // and wrappers. LUW feature.
    #define D_ENV_DB2_HAS_FEDERATION                                           \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_KNOWN) )

    // 13.1.2
    // D_ENV_DB2_HAS_EXTERNAL_TABLES
    //   feature: external tables (read/write flat files, including cloud
    // object storage). Introduced in LUW 11.5.
    #define D_ENV_DB2_HAS_EXTERNAL_TABLES                                      \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_AT_LEAST(11, 5, 0)) )


//==============================================================================
// 14.  SECURITY
//==============================================================================


// 14.1   Security
//------------------------------------------------------------------------------
    // 14.1.1
    // D_ENV_DB2_HAS_LBAC
    //   feature: label-based access control (LBAC). Introduced in Db2 9.
    #define D_ENV_DB2_HAS_LBAC                                                 \
        D_ENV_DB2_SERVER_AT_LEAST(9, 5, 0)

    // 14.1.2
    // D_ENV_DB2_HAS_RCAC
    //   feature: row and column access control (RCAC; row permissions
    // and column masks). Introduced in LUW 10.5 / z/OS 10.
    #define D_ENV_DB2_HAS_RCAC                                                 \
        D_ENV_DB2_SERVER_AT_LEAST(10, 5, 0)

    // 14.1.3
    // D_ENV_DB2_HAS_NATIVE_ENCRYPTION
    //   feature: native database encryption (encryption at rest, key
    // management). Introduced in LUW 10.5 FP5 / 11.1.
    #define D_ENV_DB2_HAS_NATIVE_ENCRYPTION                                    \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_AT_LEAST(11, 1, 0)) )

    // 14.1.4
    // D_ENV_DB2_HAS_SSL
    //   feature: SSL/TLS-encrypted client connections. Core feature.
    #define D_ENV_DB2_HAS_SSL                                                  \
        D_ENV_DB2_SERVER_KNOWN

    // 14.1.5
    // D_ENV_DB2_HAS_KERBEROS
    //   feature: Kerberos authentication. Core feature.
    #define D_ENV_DB2_HAS_KERBEROS                                             \
        D_ENV_DB2_SERVER_KNOWN

    // 14.1.6
    // D_ENV_DB2_HAS_AUDIT
    //   feature: the db2audit audit facility. Core feature.
    #define D_ENV_DB2_HAS_AUDIT                                                \
        D_ENV_DB2_SERVER_KNOWN


//==============================================================================
// 15.  PROGRAMMABILITY
//==============================================================================


// 15.1   Programmability
//------------------------------------------------------------------------------
    // 15.1.1
    // D_ENV_DB2_HAS_SQL_PL
    //   feature: SQL Procedural Language (SQL PL) for stored procedures,
    // functions, and triggers. Core feature.
    #define D_ENV_DB2_HAS_SQL_PL                                               \
        D_ENV_DB2_SERVER_KNOWN

    // 15.1.2
    // D_ENV_DB2_HAS_EXTERNAL_ROUTINES
    //   feature: external routines (C, Java, .NET) as stored procedures
    // and UDFs. Core feature.
    #define D_ENV_DB2_HAS_EXTERNAL_ROUTINES                                    \
        D_ENV_DB2_SERVER_KNOWN

    // 15.1.3
    // D_ENV_DB2_HAS_COMPATIBILITY_MODE
    //   feature: SQL compatibility features easing migration from other
    // RDBMSs (e.g. Oracle compatibility). Substantially expanded in
    // LUW 9.7 and again in 11.1.
    #define D_ENV_DB2_HAS_COMPATIBILITY_MODE                                   \
        ( (D_ENV_DB2_IS_LUW) &&                                                \
          (D_ENV_DB2_SERVER_AT_LEAST(9, 7, 0)) )

    // 15.1.4
    // D_ENV_DB2_HAS_ANCHORED_TYPES
    //   feature: anchored data types (ANCHOR clause) in SQL PL.
    // Introduced in LUW 9.7.
    #define D_ENV_DB2_HAS_ANCHORED_TYPES                                       \
        D_ENV_DB2_SERVER_AT_LEAST(9, 7, 0)


//==============================================================================
// 16.  COMPOSITE CHECKS
//==============================================================================


// 16.1   Composite checks
//------------------------------------------------------------------------------
    // 16.1.1
    // D_ENV_DB2_HAS_FULL_ACID
    //   macro: evaluates to 1 if transactions, savepoints, and two-phase
    // commit are all available.
    #define D_ENV_DB2_HAS_FULL_ACID                                            \
        ( (D_ENV_DB2_HAS_TRANSACTIONS) &&                                      \
          (D_ENV_DB2_HAS_SAVEPOINTS)   &&                                      \
          (D_ENV_DB2_HAS_TWO_PHASE_COMMIT) )

    // 16.1.2
    // D_ENV_DB2_HAS_ANALYTICS
    //   macro: evaluates to 1 if column-organized tables (BLU) and the
    // OLAP windowing surface are both available (LUW 10.5+).
    #define D_ENV_DB2_HAS_ANALYTICS                                            \
        ( (D_ENV_DB2_HAS_COLUMN_ORGANIZED) &&                                  \
          (D_ENV_DB2_HAS_WINDOW_FUNCTIONS) &&                                  \
          (D_ENV_DB2_HAS_GROUPING_SETS) )

    // 16.1.3
    // D_ENV_DB2_HAS_ADVANCED_SECURITY
    //   macro: evaluates to 1 if LBAC, RCAC, and native encryption are
    // all available.
    #define D_ENV_DB2_HAS_ADVANCED_SECURITY                                    \
        ( (D_ENV_DB2_HAS_LBAC) &&                                              \
          (D_ENV_DB2_HAS_RCAC) &&                                              \
          (D_ENV_DB2_HAS_NATIVE_ENCRYPTION) )

    // 16.1.4
    // D_ENV_DB2_HAS_ENTERPRISE_HA
    //   macro: evaluates to 1 if the target offers enterprise-grade HA
    // (HADR multi-standby on LUW, or data sharing on z/OS).
    #define D_ENV_DB2_HAS_ENTERPRISE_HA                                        \
        ( (D_ENV_DB2_HAS_HADR_MULTI_STANDBY) ||                                \
          (D_ENV_DB2_HAS_DATA_SHARING) )

    // 16.1.5
    // D_ENV_DB2_IS_FULLY_MODERN
    //   macro: evaluates to 1 if the target has a comprehensive modern
    // feature set (roughly LUW 11.5+ with analytics, JSON, advanced
    // security, and temporal tables).
    #define D_ENV_DB2_IS_FULLY_MODERN                                          \
        ( (D_ENV_DB2_HAS_ANALYTICS)         &&                                 \
          (D_ENV_DB2_HAS_JSON_FUNCTIONS)    &&                                 \
          (D_ENV_DB2_HAS_ADVANCED_SECURITY) &&                                 \
          (D_ENV_DB2_HAS_TEMPORAL_TABLES) )


//==============================================================================
// 17.  DEPRECATION AND REMOVAL
//==============================================================================


// 17.1   Deprecations
//------------------------------------------------------------------------------
    // 17.1.1
    // D_ENV_DB2_REMOVED_TYPE1_INDEXES
    //   status: 1 if legacy type-1 indexes are removed (modern LUW only
    // supports type-2 indexes).
    #define D_ENV_DB2_REMOVED_TYPE1_INDEXES                                    \
        D_ENV_DB2_SERVER_AT_LEAST(10, 1, 0)

    // 17.1.2
    // D_ENV_DB2_DEPRECATED_CLP_LEGACY
    //   status: 1 if certain legacy CLP behaviors are deprecated in
    // favor of the modern command-line processor (11.1+).
    #define D_ENV_DB2_DEPRECATED_CLP_LEGACY                                    \
        D_ENV_DB2_SERVER_AT_LEAST(11, 1, 0)


//==============================================================================
// 18.  CONSUMER COMPATIBILITY LAYER
//==============================================================================
// The djinterp Db2 connection layer (db2.hpp, db2_table.hpp) consumes a
// flattened vocabulary; this section publishes the names not already defined
// above. Feature availability follows the target server version, encoded on
// the LUW line, so D_ENV_DB2_VERSION_* mirror it, falling back to a manually
// supplied client version when no server version is configured.


// 18.1   Consumer vocabulary
//------------------------------------------------------------------------------
    // 18.1.1
    // D_ENV_DB2_VERSION_*
    //   macro: the public version: D_ENV_DB2_VERSION_ID, _MAJOR, _MINOR, _MOD
    // (Db2's modification level, where others say patch) and _STRING. They take
    // the target server version when known, otherwise the manually supplied
    // client version, otherwise 0; D_ENV_DB2_VERSION_STRING is always
    // "unknown".
    #if D_ENV_DB2_SERVER_KNOWN
        #define D_ENV_DB2_VERSION_ID    D_ENV_DB2_SERVER_VERSION_ID
        #define D_ENV_DB2_VERSION_MAJOR D_ENV_DB2_SERVER_MAJOR
        #define D_ENV_DB2_VERSION_MINOR D_ENV_DB2_SERVER_MINOR
        #define D_ENV_DB2_VERSION_MOD                                          \
            D_ENV_DB2_DECODE_PATCH(D_ENV_DB2_SERVER_VERSION_ID)
    #elif ( (D_ENV_DB2_CLIENT_DETECTED)      &&                                \
            (D_ENV_DB2_CLIENT_VERSION_KNOWN) &&                                \
            (D_CFG_ENV_DB2_CUSTOM == 1) )
        #define D_ENV_DB2_VERSION_ID    D_ENV_DB2_CLIENT_VERSION_ID
        #define D_ENV_DB2_VERSION_MAJOR D_ENV_DB2_CLIENT_MAJOR
        #define D_ENV_DB2_VERSION_MINOR D_ENV_DB2_CLIENT_MINOR
        #define D_ENV_DB2_VERSION_MOD   D_ENV_DB2_CLIENT_PATCH
    #else
        #define D_ENV_DB2_VERSION_ID    0
        #define D_ENV_DB2_VERSION_MAJOR 0
        #define D_ENV_DB2_VERSION_MINOR 0
        #define D_ENV_DB2_VERSION_MOD   0
    #endif
    #define D_ENV_DB2_VERSION_STRING "unknown"

    // 18.1.2
    // Consumer feature aliases
    //   macro: D_ENV_DB2_HAS_BOOLEAN, D_ENV_DB2_HAS_COMPOUND_SQL,
    // D_ENV_DB2_HAS_LOB_STREAMING and D_ENV_DB2_HAS_ARRAY_INPUT, the names the
    // connection layer uses for flags defined above.
    // SQL / type feature aliases
    #define D_ENV_DB2_HAS_BOOLEAN       D_ENV_DB2_HAS_BOOLEAN_TYPE

    // compound SQL
    //   compound SQL blocks (BEGIN ... END atomic/non-atomic) are part of
    // the SQL PL surface and are available wherever SQL PL is.
    #define D_ENV_DB2_HAS_COMPOUND_SQL  D_ENV_DB2_HAS_SQL_PL

    // LOB streaming
    #define D_ENV_DB2_HAS_LOB_STREAMING D_ENV_DB2_HAS_LOB

    // row-set array input
    //   array (row-set) INSERT is a CLI binding capability; expose it
    // whenever the CLI client is present, otherwise fall back to server
    // detection so server-only builds still advertise it.
    #if D_ENV_DB2_CLIENT_DETECTED
        #define D_ENV_DB2_HAS_ARRAY_INPUT 1
    #else
        #define D_ENV_DB2_HAS_ARRAY_INPUT D_ENV_DB2_SERVER_KNOWN
    #endif


#endif  // D_ENV_DB2_DETECTED


#endif  // DJINTERP_ENV_DB_DB2_ENV_DB2_H
