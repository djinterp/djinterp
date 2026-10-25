/*******************************************************************************
* djinterp [env]                                                     env_mysql.h
*
* djinterp Oracle MySQL environment detection.
*   Compile-time detection of an Oracle MySQL environment: the server version
* and its comparison macros, release series and release model (LTS or
* innovation), the client library, and the version-gated capabilities built
* on them: the C API, SSL/TLS, authentication, data types, storage engines,
* InnoDB, replication and high availability, the X Protocol and X DevAPI,
* character sets, the optimizer, and security and administration.
*   This header is for Oracle MySQL only; MariaDB has env_mariadb.h. The two
* share env_mysql_common.h, which includes the client headers when
* D_CFG_ENV_USING_MYSQL is on and tells the vendors apart. Versions are
* encoded as MAJOR*10000 + MINOR*100 + PATCH, as in MYSQL_VERSION_ID.
*   D_ENV_MYSQL_HAS_* are capability flags, D_ENV_MYSQL_VERSION_* the version
* and its parts, and D_ENV_MYSQL_IS_* the release series and model. Sections 3
* onward read Oracle MySQL when it is detected; without it, each of their
* macros is defined as 0. Settings live in
* cfg_env_mysql.h; D_CFG_ENV_MYSQL_CUSTOM switches to manual detection.
*   The release model: 5.x and 8.0 are classic series; since 8.1, MySQL ships
* quarterly innovation releases (8.1 to 8.3, then 9.0 to 9.6) alongside
* long-term support series: 8.4, and 9.7, released 2026.04.21. What follows
* 9.7 is not classified here.
*
*
* path:      /inc/djinterp/env/db/mysql/env_mysql.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR GUARD
    ------------
    1.  Vendor alias
         1.  D_ENV_MYSQL_IS_MARIADB
2.  VERSION DETECTION
    -----------------
    1.  Release version IDs
         1.  D_ENV_MYSQL_VERSION_<MAJOR>_<MINOR>_<PATCH>
    2.  Detected version
         1.  D_ENV_MYSQL_DETECTED / D_ENV_MYSQL_VERSION_*
3.  VERSION COMPARISON MACROS
    -------------------------
    1.  Comparisons
         1.  D_ENV_MYSQL_VERSION_AT_LEAST
         2.  D_ENV_MYSQL_VERSION_BELOW
         3.  D_ENV_MYSQL_VERSION_EXACT
         4.  D_ENV_MYSQL_VERSION_IN_RANGE
    2.  Release series
         1.  D_ENV_MYSQL_IS_<SERIES>
    3.  Release model
         1.  D_ENV_MYSQL_IS_LTS / D_ENV_MYSQL_IS_INNOVATION
4.  CLIENT LIBRARY (MYSQL-SPECIFIC)
    -------------------------------
    1.  Client library
         1.  D_ENV_MYSQL_HAS_CLIENT_LIB
         2.  D_ENV_MYSQL_HAS_CONNECTOR_C
         3.  D_ENV_MYSQL_HAS_LIBMYSQLCLIENT
         4.  D_ENV_MYSQL_HAS_EMBEDDED
5.  C API (MYSQL VERSION-GATED)
    ---------------------------
    1.  C API features
         1.  C API flags
6.  SSL/TLS PROTOCOL FEATURES
    -------------------------
    1.  SSL and TLS
         1.  D_ENV_MYSQL_HAS_SSL
         2.  TLS option flags
7.  AUTHENTICATION
    --------------
    1.  Authentication plugins
         1.  Authentication flags
8.  DATA TYPES
    ----------
    1.  Data types
         1.  Data-type flags
9.  STORAGE ENGINES
    ---------------
    1.  Storage engines
         1.  Storage-engine flags
10. INNODB FEATURES
    ---------------
    1.  InnoDB
         1.  InnoDB flags
11. REPLICATION AND HIGH AVAILABILITY
    ---------------------------------
    1.  Replication and high availability
         1.  Replication flags
12. X PROTOCOL AND X DEVAPI
    -----------------------
    1.  X Protocol
         1.  X Protocol flags
13. CHARACTER SET
    -------------
    1.  Character sets
         1.  Character-set flags
14. OPTIMIZER AND PERFORMANCE
    -------------------------
    1.  Optimizer and performance
         1.  Optimizer flags
15. SECURITY AND ADMINISTRATION
    ---------------------------
    1.  Security and administration
         1.  Security flags
16. PLATFORM
    --------
    1.  Platform
         1.  Connection-method flags
17. COMPOSITE CHECKS
    ----------------
    1.  Composite checks
         1.  Composite checks
18. DEPRECATION AND REMOVAL
    -----------------------
    1.  Deprecations
         1.  Removal and deprecation flags
*/

#ifndef DJINTERP_ENV_DB_MYSQL_ENV_MYSQL_H
#define DJINTERP_ENV_DB_MYSQL_ENV_MYSQL_H 1

// djinterp
#include "./env_mysql_common.h"  // D_ENV_MYSQL_COMMON_*, client headers


//==============================================================================
// 1.  VENDOR GUARD
//==============================================================================


// 1.1    Vendor alias
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_MYSQL_IS_MARIADB
//   detection: alias from common header for backward compatibility.
#define D_ENV_MYSQL_IS_MARIADB D_ENV_MYSQL_COMMON_IS_MARIADB


//==============================================================================
// 2.  VERSION DETECTION
//==============================================================================
// Oracle MySQL encodes its version as MAJOR*10000 + MINOR*100 + PATCH, in
// MYSQL_VERSION_ID; MySQL 8.0.35 is 80035.


// 2.1    Release version IDs
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_MYSQL_VERSION_<MAJOR>_<MINOR>_<PATCH>
//   constant: encoded IDs of the releases that gate features below.
#define D_ENV_MYSQL_VERSION_5_0_0      50000
#define D_ENV_MYSQL_VERSION_5_1_0      50100
#define D_ENV_MYSQL_VERSION_5_5_0      50500
#define D_ENV_MYSQL_VERSION_5_6_0      50600
#define D_ENV_MYSQL_VERSION_5_7_0      50700
#define D_ENV_MYSQL_VERSION_5_7_8      50708
#define D_ENV_MYSQL_VERSION_5_7_12     50712
#define D_ENV_MYSQL_VERSION_5_7_17     50717
#define D_ENV_MYSQL_VERSION_5_7_22     50722
#define D_ENV_MYSQL_VERSION_8_0_0      80000
#define D_ENV_MYSQL_VERSION_8_0_3      80003
#define D_ENV_MYSQL_VERSION_8_0_11     80011
#define D_ENV_MYSQL_VERSION_8_0_13     80013
#define D_ENV_MYSQL_VERSION_8_0_14     80014
#define D_ENV_MYSQL_VERSION_8_0_16     80016
#define D_ENV_MYSQL_VERSION_8_0_17     80017
#define D_ENV_MYSQL_VERSION_8_0_19     80019
#define D_ENV_MYSQL_VERSION_8_0_22     80022
#define D_ENV_MYSQL_VERSION_8_0_23     80023
#define D_ENV_MYSQL_VERSION_8_0_25     80025
#define D_ENV_MYSQL_VERSION_8_0_27     80027
#define D_ENV_MYSQL_VERSION_8_0_28     80028
#define D_ENV_MYSQL_VERSION_8_0_29     80029
#define D_ENV_MYSQL_VERSION_8_0_30     80030
#define D_ENV_MYSQL_VERSION_8_0_32     80032
#define D_ENV_MYSQL_VERSION_8_0_34     80034
#define D_ENV_MYSQL_VERSION_8_1_0      80100
#define D_ENV_MYSQL_VERSION_8_2_0      80200
#define D_ENV_MYSQL_VERSION_8_3_0      80300
#define D_ENV_MYSQL_VERSION_8_4_0      80400
#define D_ENV_MYSQL_VERSION_9_0_0      90000
#define D_ENV_MYSQL_VERSION_9_1_0      90100
#define D_ENV_MYSQL_VERSION_9_2_0      90200
#define D_ENV_MYSQL_VERSION_9_3_0      90300
#define D_ENV_MYSQL_VERSION_9_4_0      90400
#define D_ENV_MYSQL_VERSION_9_5_0      90500
#define D_ENV_MYSQL_VERSION_9_6_0      90600
#define D_ENV_MYSQL_VERSION_9_7_0      90700

// 2.2    Detected version
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_MYSQL_DETECTED / D_ENV_MYSQL_VERSION_*
//   detection: D_ENV_MYSQL_DETECTED is 1 when an Oracle MySQL version is
// known, and D_ENV_MYSQL_VERSION_ID, _MAJOR, _MINOR, _PATCH and _STRING then
// describe it. Automatic mode needs the MySQL header in scope
// (D_ENV_MYSQL_HEADER_INCLUDED, from env_mysql_common.h), and a header that
// isn't MariaDB's, and reads MYSQL_VERSION_ID. Manual mode
// (D_CFG_ENV_MYSQL_CUSTOM) reads D_ENV_MYSQL_DETECTED_VERSION or one of the
// D_ENV_MYSQL_DETECTED_<MAJOR>_<MINOR> series flags.
#if D_CFG_IS_OFF(D_CFG_ENV_MYSQL_CUSTOM)

    // automatic detection requires the MySQL header to be in scope; if
    // D_CFG_ENV_USING_MYSQL was not enabled the sentinel is 0 and we skip
    // cleanly (no reference to MYSQL_VERSION_ID etc.)
    #if ( (D_ENV_MYSQL_HEADER_INCLUDED) &&                                     \
          (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) )
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         MYSQL_VERSION_ID
        #define D_ENV_MYSQL_VERSION_MAJOR                                      \
            D_ENV_MYSQL_COMMON_DECODE_MAJOR(MYSQL_VERSION_ID)
        #define D_ENV_MYSQL_VERSION_MINOR                                      \
            D_ENV_MYSQL_COMMON_DECODE_MINOR(MYSQL_VERSION_ID)
        #define D_ENV_MYSQL_VERSION_PATCH                                      \
            D_ENV_MYSQL_COMMON_DECODE_PATCH(MYSQL_VERSION_ID)

        #ifdef MYSQL_SERVER_VERSION
            #define D_ENV_MYSQL_VERSION_STRING  MYSQL_SERVER_VERSION
        #else
            #define D_ENV_MYSQL_VERSION_STRING  "unknown"
        #endif  // MYSQL_SERVER_VERSION
    #else
        #define D_ENV_MYSQL_DETECTED           0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_MYSQL_DETECTED_VERSION
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_DETECTED_VERSION
        #define D_ENV_MYSQL_VERSION_MAJOR                                      \
            D_ENV_MYSQL_COMMON_DECODE_MAJOR(D_ENV_MYSQL_DETECTED_VERSION)
        #define D_ENV_MYSQL_VERSION_MINOR                                      \
            D_ENV_MYSQL_COMMON_DECODE_MINOR(D_ENV_MYSQL_DETECTED_VERSION)
        #define D_ENV_MYSQL_VERSION_PATCH                                      \
            D_ENV_MYSQL_COMMON_DECODE_PATCH(D_ENV_MYSQL_DETECTED_VERSION)
        #define D_ENV_MYSQL_VERSION_STRING     "manual"

    #elif defined(D_ENV_MYSQL_DETECTED_9_7)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_7_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      7
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.7.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_6)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_6_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      6
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.6.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_5)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_5_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      5
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.5.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_4)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_4_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      4
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.4.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_3)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_3_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      3
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.3.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_2)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_2_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      2
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.2.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_1)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_1_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      1
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.1.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_0)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_0_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      0
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.0.0"

    #elif defined(D_ENV_MYSQL_DETECTED_8_4)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_8_4_0
        #define D_ENV_MYSQL_VERSION_MAJOR      8
        #define D_ENV_MYSQL_VERSION_MINOR      4
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "8.4.0"

    #elif defined(D_ENV_MYSQL_DETECTED_8_0)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_8_0_0
        #define D_ENV_MYSQL_VERSION_MAJOR      8
        #define D_ENV_MYSQL_VERSION_MINOR      0
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "8.0.0"

    #elif defined(D_ENV_MYSQL_DETECTED_5_7)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_5_7_0
        #define D_ENV_MYSQL_VERSION_MAJOR      5
        #define D_ENV_MYSQL_VERSION_MINOR      7
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "5.7.0"

    #elif defined(D_ENV_MYSQL_DETECTED_5_6)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_5_6_0
        #define D_ENV_MYSQL_VERSION_MAJOR      5
        #define D_ENV_MYSQL_VERSION_MINOR      6
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "5.6.0"

    #elif defined(D_ENV_MYSQL_DETECTED_5_5)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_5_5_0
        #define D_ENV_MYSQL_VERSION_MAJOR      5
        #define D_ENV_MYSQL_VERSION_MINOR      5
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "5.5.0"

    #else
        #define D_ENV_MYSQL_DETECTED           0
    #endif  // D_ENV_MYSQL_DETECTED_VERSION

#endif  // D_CFG_ENV_MYSQL_CUSTOM


// sections 3 to 18 read Oracle MySQL when D_ENV_MYSQL_DETECTED is 1 (see
// 2.2.1), and define each of their macros as 0 when it is not
#if D_ENV_MYSQL_DETECTED


//==============================================================================
// 3.  VERSION COMPARISON MACROS
//==============================================================================
// env_mysql_common.h includes the vendor headers, under D_CFG_ENV_USING_MYSQL;
// this header never includes <mysql/mysql.h> itself, and never MariaDB's
// <mariadb/conncpp.hpp>, whose unconditional inclusion here once caused
// cross-vendor symbol collisions and spurious link errors.


// 3.1    Comparisons
//------------------------------------------------------------------------------
    // 3.1.1
    // D_ENV_MYSQL_VERSION_AT_LEAST
    //   macro: 1 if the detected version is at least major.minor.patch.
    #define D_ENV_MYSQL_VERSION_AT_LEAST(major, minor, patch)                  \
        (D_ENV_MYSQL_VERSION_ID >=                                             \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    // 3.1.2
    // D_ENV_MYSQL_VERSION_BELOW
    //   macro: 1 if the detected version is below major.minor.patch.
    #define D_ENV_MYSQL_VERSION_BELOW(major, minor, patch)                     \
        (D_ENV_MYSQL_VERSION_ID <                                              \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    // 3.1.3
    // D_ENV_MYSQL_VERSION_EXACT
    //   macro: 1 if the detected version is exactly major.minor.patch.
    #define D_ENV_MYSQL_VERSION_EXACT(major, minor, patch)                     \
        (D_ENV_MYSQL_VERSION_ID ==                                             \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    // 3.1.4
    // D_ENV_MYSQL_VERSION_IN_RANGE
    //   macro: 1 if the detected version is at least the first triple and
    // below the second.
    #define D_ENV_MYSQL_VERSION_IN_RANGE(min_maj, min_min, min_pat,            \
                                         max_maj, max_min, max_pat)            \
        ( (D_ENV_MYSQL_VERSION_AT_LEAST(min_maj, min_min, min_pat)) &&         \
          (D_ENV_MYSQL_VERSION_BELOW(max_maj, max_min, max_pat)) )

// 3.2    Release series
//------------------------------------------------------------------------------
    // 3.2.1
    // D_ENV_MYSQL_IS_<SERIES>
    //   macro: 1 if the detected version is in that release series: 5.5, 5.6,
    // 5.7, 8.0, 8.4 or 9.7. D_ENV_MYSQL_IS_9_PLUS covers 9.0 and later.
    #define D_ENV_MYSQL_IS_5_5                                                 \
        D_ENV_MYSQL_VERSION_IN_RANGE(5, 5, 0, 5, 6, 0)
    #define D_ENV_MYSQL_IS_5_6                                                 \
        D_ENV_MYSQL_VERSION_IN_RANGE(5, 6, 0, 5, 7, 0)
    #define D_ENV_MYSQL_IS_5_7                                                 \
        D_ENV_MYSQL_VERSION_IN_RANGE(5, 7, 0, 8, 0, 0)
    #define D_ENV_MYSQL_IS_8_0                                                 \
        D_ENV_MYSQL_VERSION_IN_RANGE(8, 0, 0, 8, 1, 0)
    #define D_ENV_MYSQL_IS_8_4                                                 \
        D_ENV_MYSQL_VERSION_IN_RANGE(8, 4, 0, 8, 5, 0)
    #define D_ENV_MYSQL_IS_9_PLUS                                              \
        D_ENV_MYSQL_VERSION_AT_LEAST(9, 0, 0)
    #define D_ENV_MYSQL_IS_9_7                                                 \
        D_ENV_MYSQL_VERSION_IN_RANGE(9, 7, 0, 9, 8, 0)

// 3.3    Release model
//------------------------------------------------------------------------------
    // 3.3.1
    // D_ENV_MYSQL_IS_LTS / D_ENV_MYSQL_IS_INNOVATION
    //   macro: D_ENV_MYSQL_IS_LTS is 1 for the long-term-support series, 8.4
    // and 9.7, and D_ENV_MYSQL_IS_INNOVATION for the innovation releases, 8.1
    // to 8.3 and 9.0 to 9.6; both are 0 for what follows 9.7, which this
    // header does not classify.
    #define D_ENV_MYSQL_IS_LTS                                                 \
        ( (D_ENV_MYSQL_IS_8_4) ||                                              \
          (D_ENV_MYSQL_IS_9_7) )
    #define D_ENV_MYSQL_IS_INNOVATION                                          \
        ( (D_ENV_MYSQL_VERSION_IN_RANGE(8, 1, 0, 8, 4, 0)) ||                  \
          (D_ENV_MYSQL_VERSION_IN_RANGE(9, 0, 0, 9, 7, 0)) )


//==============================================================================
// 4.  CLIENT LIBRARY (MYSQL-SPECIFIC)
//==============================================================================


// 4.1    Client library
//------------------------------------------------------------------------------
    // 4.1.1
    // D_ENV_MYSQL_HAS_CLIENT_LIB
    //   feature: alias of D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB.
    #define D_ENV_MYSQL_HAS_CLIENT_LIB                                         \
        D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB

    // 4.1.2
    // D_ENV_MYSQL_HAS_CONNECTOR_C
    //   feature: 1 if MySQL Connector/C is present, as MYSQL_CONNECTOR_VERSION
    // or a pre-defined D_ENV_MYSQL_DETECTED_CONNECTOR_C shows.
    #ifndef D_ENV_MYSQL_HAS_CONNECTOR_C
        #if ( (defined(MYSQL_CONNECTOR_VERSION)) ||                            \
              (defined(D_ENV_MYSQL_DETECTED_CONNECTOR_C)) )
            #define D_ENV_MYSQL_HAS_CONNECTOR_C 1
        #else
            #define D_ENV_MYSQL_HAS_CONNECTOR_C 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_CONNECTOR_C

    // 4.1.3
    // D_ENV_MYSQL_HAS_LIBMYSQLCLIENT
    //   feature: 1 if libmysqlclient is present: LIBMYSQL_VERSION or
    // LIBMYSQL_VERSION_ID is defined, the header is Oracle MySQL's, or
    // D_ENV_MYSQL_DETECTED_LIBMYSQLCLIENT is pre-defined.
    #ifndef D_ENV_MYSQL_HAS_LIBMYSQLCLIENT
        #if ( (defined(LIBMYSQL_VERSION))          ||                          \
              (defined(LIBMYSQL_VERSION_ID))       ||                          \
              (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) ||                          \
              (defined(D_ENV_MYSQL_DETECTED_LIBMYSQLCLIENT)) )
            #define D_ENV_MYSQL_HAS_LIBMYSQLCLIENT 1
        #else
            #define D_ENV_MYSQL_HAS_LIBMYSQLCLIENT 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_LIBMYSQLCLIENT

    // 4.1.4
    // D_ENV_MYSQL_HAS_EMBEDDED
    //   feature: alias of D_ENV_MYSQL_COMMON_HAS_EMBEDDED.
    #define D_ENV_MYSQL_HAS_EMBEDDED                                           \
        D_ENV_MYSQL_COMMON_HAS_EMBEDDED


//==============================================================================
// 5.  C API (MYSQL VERSION-GATED)
//==============================================================================


// 5.1    C API features
//------------------------------------------------------------------------------
    // 5.1.1
    // C API flags
    //   feature: C API capabilities, each 1 from the MySQL release its
    // definition names: the non-blocking connect and asynchronous API,
    // connection reset, session-state tracking, mysql_get_option(),
    // mysql_stmt_next_result(), zstd compression, optional result-set metadata,
    // and query attributes.
    #ifndef D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
            #define D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING 1
        #else
            #define D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING

    #ifndef D_ENV_MYSQL_HAS_ASYNC_API
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
            #define D_ENV_MYSQL_HAS_ASYNC_API 1
        #else
            #define D_ENV_MYSQL_HAS_ASYNC_API 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_ASYNC_API

    #ifndef D_ENV_MYSQL_HAS_RESET_CONNECTION
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 3)
            #define D_ENV_MYSQL_HAS_RESET_CONNECTION 1
        #else
            #define D_ENV_MYSQL_HAS_RESET_CONNECTION 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_RESET_CONNECTION

    #ifndef D_ENV_MYSQL_HAS_SESSION_TRACK
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 4)
            #define D_ENV_MYSQL_HAS_SESSION_TRACK 1
        #else
            #define D_ENV_MYSQL_HAS_SESSION_TRACK 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_SESSION_TRACK

    #ifndef D_ENV_MYSQL_HAS_GET_OPTION
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 3)
            #define D_ENV_MYSQL_HAS_GET_OPTION 1
        #else
            #define D_ENV_MYSQL_HAS_GET_OPTION 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_GET_OPTION

    #ifndef D_ENV_MYSQL_HAS_STMT_NEXT_RESULT
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 3)
            #define D_ENV_MYSQL_HAS_STMT_NEXT_RESULT 1
        #else
            #define D_ENV_MYSQL_HAS_STMT_NEXT_RESULT 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_STMT_NEXT_RESULT

    // protocol
    #ifndef D_ENV_MYSQL_HAS_ZSTD_COMPRESSION
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
            #define D_ENV_MYSQL_HAS_ZSTD_COMPRESSION 1
        #else
            #define D_ENV_MYSQL_HAS_ZSTD_COMPRESSION 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_ZSTD_COMPRESSION

    #ifndef D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
            #define D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA 1
        #else
            #define D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA

    #ifndef D_ENV_MYSQL_HAS_QUERY_ATTRS
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 23)
            #define D_ENV_MYSQL_HAS_QUERY_ATTRS 1
        #else
            #define D_ENV_MYSQL_HAS_QUERY_ATTRS 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_QUERY_ATTRS


//==============================================================================
// 6.  SSL/TLS PROTOCOL FEATURES
//==============================================================================


// 6.1    SSL and TLS
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_MYSQL_HAS_SSL
    //   feature: 1 if the client library was built with SSL
    // (D_ENV_MYSQL_COMMON_HAS_ANY_SSL), or from MySQL 5.7 on.
    #ifndef D_ENV_MYSQL_HAS_SSL
        #if D_ENV_MYSQL_COMMON_HAS_ANY_SSL
            #define D_ENV_MYSQL_HAS_SSL 1
        #elif D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 0)
            #define D_ENV_MYSQL_HAS_SSL 1
        #else
            #define D_ENV_MYSQL_HAS_SSL 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_SSL

    // 6.1.2
    // TLS option flags
    //   feature: TLS options, each 1 from the MySQL release its definition
    // names: the ssl-mode setting, TLS version selection, TLS 1.3 cipher
    // suites, and the session-reuse query. D_ENV_MYSQL_HAS_SSL_FIPS_MODE is 1
    // from 8.0.11 up to, but not including, 8.0.34.
    #define D_ENV_MYSQL_HAS_SSL_MODE                                           \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 11)
    #define D_ENV_MYSQL_HAS_TLS_VERSION_OPTION                                 \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 10)
    #define D_ENV_MYSQL_HAS_TLS_CIPHERSUITES                                   \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)

    #ifndef D_ENV_MYSQL_HAS_SSL_FIPS_MODE
        #if D_ENV_MYSQL_VERSION_IN_RANGE(8, 0, 11, 8, 0, 34)
            #define D_ENV_MYSQL_HAS_SSL_FIPS_MODE 1
        #else
            #define D_ENV_MYSQL_HAS_SSL_FIPS_MODE 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_SSL_FIPS_MODE

    #define D_ENV_MYSQL_HAS_GET_SSL_SESSION_REUSED                             \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 29)


//==============================================================================
// 7.  AUTHENTICATION
//==============================================================================


// 7.1    Authentication plugins
//------------------------------------------------------------------------------
    // 7.1.1
    // Authentication flags
    //   feature: authentication plugins and policies, each 1 from the MySQL
    // release its definition names: mysql_native_password, an alias of the
    // common flag, and D_ENV_MYSQL_AUTH_NATIVE_DEPRECATED, its deprecation;
    // pluggable authentication, also an alias; sha256_password;
    // caching_sha2_password, and D_ENV_MYSQL_DEFAULT_AUTH_IS_CACHING_SHA2, its
    // becoming the default; LDAP simple and SASL; Kerberos; OCI; FIDO; and
    // multi-factor authentication.
    #define D_ENV_MYSQL_HAS_AUTH_NATIVE                                        \
        D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE
    #define D_ENV_MYSQL_AUTH_NATIVE_DEPRECATED                                 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 34)
    #define D_ENV_MYSQL_HAS_PLUGGABLE_AUTH                                     \
        D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH

    #define D_ENV_MYSQL_HAS_AUTH_SHA256                                        \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 6)
    #define D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_DEFAULT_AUTH_IS_CACHING_SHA2                           \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 4)
    #define D_ENV_MYSQL_HAS_AUTH_LDAP_SIMPLE                                   \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 19)
    #define D_ENV_MYSQL_HAS_AUTH_LDAP_SASL                                     \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 19)
    #define D_ENV_MYSQL_HAS_AUTH_KERBEROS                                      \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 26)
    #define D_ENV_MYSQL_HAS_AUTH_OCI                                           \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)
    #define D_ENV_MYSQL_HAS_AUTH_FIDO                                          \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)
    #define D_ENV_MYSQL_HAS_MULTI_FACTOR_AUTH                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)


//==============================================================================
// 8.  DATA TYPES
//==============================================================================


// 8.1    Data types
//------------------------------------------------------------------------------
    // 8.1.1
    // Data-type flags
    //   feature: data types and related features, each 1 from the MySQL release
    // its definition names: the JSON type, JSON_TABLE, JSON schema validation,
    // JSON_VALUE and JSON_ARRAYAGG; multi-valued, functional and descending
    // indexes; spatial types and SRIDs; invisible and generated columns;
    // expression defaults; and enforced CHECK constraints.
    #define D_ENV_MYSQL_HAS_JSON_TYPE                                          \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 8)
    #define D_ENV_MYSQL_HAS_JSON_TABLE                                         \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 4)
    #define D_ENV_MYSQL_HAS_JSON_SCHEMA_VALIDATION                             \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 17)
    #define D_ENV_MYSQL_HAS_JSON_VALUE                                         \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 21)
    #define D_ENV_MYSQL_HAS_JSON_ARRAYAGG                                      \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 22)
    #define D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 17)
    #define D_ENV_MYSQL_HAS_GEOMETRY_TYPES                                     \
        D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES
    #define D_ENV_MYSQL_HAS_SRID_SUPPORT                                       \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_INVISIBLE_COLUMNS                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 23)
    #define D_ENV_MYSQL_HAS_FUNCTIONAL_INDEX                                   \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 13)
    #define D_ENV_MYSQL_HAS_DESCENDING_INDEX                                   \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_HAS_GENERATED_COLUMNS                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 6)
    #define D_ENV_MYSQL_HAS_DEFAULT_EXPRESSION                                 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 13)
    #define D_ENV_MYSQL_HAS_CHECK_CONSTRAINTS                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)


//==============================================================================
// 9.  STORAGE ENGINES
//==============================================================================


// 9.1    Storage engines
//------------------------------------------------------------------------------
    // 9.1.1
    // Storage-engine flags
    //   feature: InnoDB, MyISAM, MEMORY, NDB Cluster, ARCHIVE, CSV, BLACKHOLE
    // and FEDERATED, aliases of the matching D_ENV_MYSQL_COMMON_HAS_* flags;
    // and D_ENV_MYSQL_SYSTEM_TABLES_USE_INNODB, 1 from MySQL 8.0, where the
    // system tables moved to InnoDB.
    #define D_ENV_MYSQL_HAS_INNODB         D_ENV_MYSQL_COMMON_HAS_INNODB
    #define D_ENV_MYSQL_HAS_MYISAM         D_ENV_MYSQL_COMMON_HAS_MYISAM
    #define D_ENV_MYSQL_HAS_MEMORY_ENGINE  D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE
    #define D_ENV_MYSQL_HAS_NDB_CLUSTER    D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
    #define D_ENV_MYSQL_HAS_ARCHIVE_ENGINE D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE
    #define D_ENV_MYSQL_HAS_CSV_ENGINE     D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE
    #define D_ENV_MYSQL_HAS_BLACKHOLE_ENGINE                                   \
        D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE
    #define D_ENV_MYSQL_HAS_FEDERATED_ENGINE                                   \
        D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
    #define D_ENV_MYSQL_SYSTEM_TABLES_USE_INNODB                               \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)


//==============================================================================
// 10.  INNODB FEATURES
//==============================================================================


// 10.1   InnoDB
//------------------------------------------------------------------------------
    // 10.1.1
    // InnoDB flags
    //   feature: InnoDB features, each 1 from the MySQL release its definition
    // names: full-text and spatial indexes, online and instant DDL, tablespace
    // encryption, redo-log management, undo-tablespace truncation, and
    // dedicated-server configuration.
    #define D_ENV_MYSQL_HAS_INNODB_FULLTEXT                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 4)
    #define D_ENV_MYSQL_HAS_INNODB_SPATIAL_INDEX                               \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 5)
    #define D_ENV_MYSQL_HAS_INNODB_ONLINE_DDL                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 7)
    #define D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL                                 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 12)
    #define D_ENV_MYSQL_HAS_INNODB_TABLESPACE_ENCRYPTION                       \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 11)
    #define D_ENV_MYSQL_HAS_INNODB_REDO_LOG                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 30)
    #define D_ENV_MYSQL_HAS_INNODB_UNDO_TRUNCATION                             \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 5)
    #define D_ENV_MYSQL_HAS_INNODB_DEDICATED_SERVER                            \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)


//==============================================================================
// 11.  REPLICATION AND HIGH AVAILABILITY
//==============================================================================


// 11.1   Replication and high availability
//------------------------------------------------------------------------------
    // 11.1.1
    // Replication flags
    //   feature: replication and high availability, each 1 from the MySQL
    // release its definition names: GTIDs, semi-synchronous and multi-source
    // replication, Group Replication, InnoDB Cluster, ClusterSet and
    // ReplicaSet, the clone plugin, replication channels, and replication
    // privilege checks.
    #define D_ENV_MYSQL_HAS_GTID                                               \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 5)
    #define D_ENV_MYSQL_HAS_SEMI_SYNC_REPL                                     \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 1)
    #define D_ENV_MYSQL_HAS_MULTI_SOURCE_REPL                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 6)
    #define D_ENV_MYSQL_HAS_GROUP_REPLICATION                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 17)
    #define D_ENV_MYSQL_HAS_INNODB_CLUSTER                                     \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 17)
    #define D_ENV_MYSQL_HAS_INNODB_CLUSTERSET                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)
    #define D_ENV_MYSQL_HAS_INNODB_REPLICASET                                  \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 19)
    #define D_ENV_MYSQL_HAS_CLONE_PLUGIN                                       \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 17)
    #define D_ENV_MYSQL_HAS_REPL_CHANNELS                                      \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 6)
    #define D_ENV_MYSQL_HAS_REPL_PRIVILEGE_CHECKS                              \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)


//==============================================================================
// 12.  X PROTOCOL AND X DEVAPI
//==============================================================================


// 12.1   X Protocol
//------------------------------------------------------------------------------
    // 12.1.1
    // X Protocol flags
    //   feature: D_ENV_MYSQL_HAS_X_PROTOCOL and D_ENV_MYSQL_HAS_X_DEVAPI, each
    // 1 from the MySQL release its definition names; and
    // D_ENV_MYSQL_HAS_MYSQLX_PLUGIN, 1 when MYSQLX_VERSION or
    // MYSQLXCLIENT_VERSION is defined, or from MySQL 8.0.11.
    #define D_ENV_MYSQL_HAS_X_PROTOCOL                                         \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 12)
    #define D_ENV_MYSQL_HAS_X_DEVAPI                                           \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 12)

    #ifndef D_ENV_MYSQL_HAS_MYSQLX_PLUGIN
        #if ( (defined(MYSQLX_VERSION)) ||                                     \
              (defined(MYSQLXCLIENT_VERSION)) )
            #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 1
        #elif D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 11)
            #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 1
        #else
            #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_MYSQLX_PLUGIN


//==============================================================================
// 13.  CHARACTER SET
//==============================================================================


// 13.1   Character sets
//------------------------------------------------------------------------------
    // 13.1.1
    // Character-set flags
    //   feature: utf8mb4, D_ENV_MYSQL_DEFAULT_CHARSET_IS_UTF8MB4 (its becoming
    // the default) and the utf8mb4_0900 collations, each 1 from the MySQL
    // release its definition names; and D_ENV_MYSQL_UTF8_IS_UTF8MB3, an alias
    // of the common flag.
    #define D_ENV_MYSQL_HAS_UTF8MB4                                            \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 3)
    #define D_ENV_MYSQL_DEFAULT_CHARSET_IS_UTF8MB4                             \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_HAS_UTF8MB4_0900                                       \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_UTF8_IS_UTF8MB3                                        \
        D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3


//==============================================================================
// 14.  OPTIMIZER AND PERFORMANCE
//==============================================================================


// 14.1   Optimizer and performance
//------------------------------------------------------------------------------
    // 14.1.1
    // Optimizer flags
    //   feature: optimizer and performance features, each 1 from the MySQL
    // release its definition names: the transactional data dictionary, resource
    // groups, window functions, CTEs, lateral derived tables, hash joins,
    // histograms, EXPLAIN ANALYZE and EXPLAIN FORMAT=TREE, optimizer hints, the
    // Performance Schema, and the sys schema.
    #define D_ENV_MYSQL_HAS_DATA_DICTIONARY                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_HAS_RESOURCE_GROUPS                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS                                   \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 2)
    #define D_ENV_MYSQL_HAS_CTE                                                \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_HAS_LATERAL_DERIVED                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 14)
    #define D_ENV_MYSQL_HAS_HASH_JOIN                                          \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
    #define D_ENV_MYSQL_HAS_HISTOGRAMS                                         \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
    #define D_ENV_MYSQL_HAS_EXPLAIN_FORMAT_TREE                                \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
    #define D_ENV_MYSQL_HAS_OPTIMIZER_HINTS                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 7)
    #define D_ENV_MYSQL_HAS_PERFORMANCE_SCHEMA                                 \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 0)
    #define D_ENV_MYSQL_HAS_SYS_SCHEMA                                         \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 7)


//==============================================================================
// 15.  SECURITY AND ADMINISTRATION
//==============================================================================


// 15.1   Security and administration
//------------------------------------------------------------------------------
    // 15.1.1
    // Security flags
    //   feature: security and administration, each 1 from the MySQL release its
    // definition names: roles, partial revokes, the audit log and data masking,
    // password history, dual and random passwords, and atomic DDL.
    #define D_ENV_MYSQL_HAS_ROLES                                              \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_HAS_PARTIAL_REVOKE                                     \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
    #define D_ENV_MYSQL_HAS_AUDIT_LOG                                          \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 20)
    #define D_ENV_MYSQL_HAS_DATA_MASKING                                       \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 13)
    #define D_ENV_MYSQL_HAS_PASSWORD_HISTORY                                   \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_DUAL_PASSWORDS                                     \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 14)
    #define D_ENV_MYSQL_HAS_RANDOM_PASSWORD                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
    #define D_ENV_MYSQL_HAS_ATOMIC_DDL                                         \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)


//==============================================================================
// 16.  PLATFORM
//==============================================================================


// 16.1   Platform
//------------------------------------------------------------------------------
    // 16.1.1
    // Connection-method flags
    //   feature: UNIX-socket, named-pipe and shared-memory connections, aliases
    // of the matching D_ENV_MYSQL_COMMON_HAS_* flags; and
    // D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH, 1 from MySQL 5.5.10 where UNIX sockets
    // are available.
    #define D_ENV_MYSQL_HAS_UNIX_SOCKET_CONN                                   \
        D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
    #define D_ENV_MYSQL_HAS_NAMED_PIPE                                         \
        D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
    #define D_ENV_MYSQL_HAS_SHARED_MEMORY                                      \
        D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY

    #ifndef D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH
        #if ( (D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 10)) &&                      \
              (D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET) )
            #define D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH 1
        #else
            #define D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH 0
        #endif
    #endif  // D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH


//==============================================================================
// 17.  COMPOSITE CHECKS
//==============================================================================


// 17.1   Composite checks
//------------------------------------------------------------------------------
    // 17.1.1
    // Composite checks
    //   feature: 1 when every flag a composite combines is 1:
    // D_ENV_MYSQL_HAS_MODERN_AUTH, _MODERN_JSON, _MODERN_DDL, _MODERN_SQL,
    // _MODERN_SECURITY and _MODERN_OPTIMIZER, D_ENV_MYSQL_HAS_HA_SUITE, and
    // D_ENV_MYSQL_IS_FULLY_MODERN, which combines the others.
    #define D_ENV_MYSQL_HAS_MODERN_AUTH                                        \
        ( (D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2) &&                               \
          (D_ENV_MYSQL_HAS_PLUGGABLE_AUTH) )

    #define D_ENV_MYSQL_HAS_MODERN_JSON                                        \
        ( (D_ENV_MYSQL_HAS_JSON_TYPE)  &&                                      \
          (D_ENV_MYSQL_HAS_JSON_TABLE) &&                                      \
          (D_ENV_MYSQL_HAS_JSON_VALUE) &&                                      \
          (D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX) )

    #define D_ENV_MYSQL_HAS_MODERN_DDL                                         \
        ( (D_ENV_MYSQL_HAS_ATOMIC_DDL)         &&                              \
          (D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL) &&                              \
          (D_ENV_MYSQL_HAS_DATA_DICTIONARY) )

    #define D_ENV_MYSQL_HAS_MODERN_SQL                                         \
        ( (D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS) &&                                \
          (D_ENV_MYSQL_HAS_CTE)              &&                                \
          (D_ENV_MYSQL_HAS_LATERAL_DERIVED) )

    #define D_ENV_MYSQL_HAS_HA_SUITE                                           \
        ( (D_ENV_MYSQL_HAS_GROUP_REPLICATION) &&                               \
          (D_ENV_MYSQL_HAS_INNODB_CLUSTER)    &&                               \
          (D_ENV_MYSQL_HAS_CLONE_PLUGIN) )

    #define D_ENV_MYSQL_HAS_MODERN_SECURITY                                    \
        ( (D_ENV_MYSQL_HAS_ROLES)            &&                                \
          (D_ENV_MYSQL_HAS_PARTIAL_REVOKE)   &&                                \
          (D_ENV_MYSQL_HAS_PASSWORD_HISTORY) &&                                \
          (D_ENV_MYSQL_HAS_DUAL_PASSWORDS) )

    #define D_ENV_MYSQL_HAS_MODERN_OPTIMIZER                                   \
        ( (D_ENV_MYSQL_HAS_HASH_JOIN)  &&                                      \
          (D_ENV_MYSQL_HAS_HISTOGRAMS) &&                                      \
          (D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE) )

    #define D_ENV_MYSQL_IS_FULLY_MODERN                                        \
        ( (D_ENV_MYSQL_HAS_MODERN_AUTH)     &&                                 \
          (D_ENV_MYSQL_HAS_MODERN_DDL)      &&                                 \
          (D_ENV_MYSQL_HAS_MODERN_SQL)      &&                                 \
          (D_ENV_MYSQL_HAS_MODERN_SECURITY) &&                                 \
          (D_ENV_MYSQL_HAS_MODERN_OPTIMIZER) )


//==============================================================================
// 18.  DEPRECATION AND REMOVAL
//==============================================================================


// 18.1   Deprecations
//------------------------------------------------------------------------------
    // 18.1.1
    // Removal and deprecation flags
    //   status: 1 from the MySQL release that removed or deprecated the feature
    // named: D_ENV_MYSQL_REMOVED_QUERY_CACHE,
    // D_ENV_MYSQL_REMOVED_PARTITION_ENGINE,
    // D_ENV_MYSQL_REMOVED_PASSWORD_FUNCTION,
    // D_ENV_MYSQL_REMOVED_EMBEDDED_SERVER and D_ENV_MYSQL_REMOVED_FRM_FILES;
    // D_ENV_MYSQL_DEPRECATED_UTF8MB3_ALIAS and
    // D_ENV_MYSQL_DEPRECATED_MYSQL_NATIVE_PASSWORD.
    #define D_ENV_MYSQL_REMOVED_QUERY_CACHE                                    \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_REMOVED_PARTITION_ENGINE                               \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_REMOVED_PASSWORD_FUNCTION                              \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 11)
    #define D_ENV_MYSQL_REMOVED_EMBEDDED_SERVER                                \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_REMOVED_FRM_FILES                                      \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_DEPRECATED_UTF8MB3_ALIAS                               \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 28)
    #define D_ENV_MYSQL_DEPRECATED_MYSQL_NATIVE_PASSWORD                       \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 34)


#else
    // decision 92 of the register: without Oracle MySQL, every macro sections
    // 3 to 18 define is defined, as 0, the comparisons for any version among
    // them, so that #if and code may read each whatever the build; a flag
    // a build may pre-define keeps its value
    #define D_ENV_MYSQL_VERSION_AT_LEAST(major, minor, patch) 0
    #define D_ENV_MYSQL_VERSION_BELOW(major, minor, patch) 0
    #define D_ENV_MYSQL_VERSION_EXACT(major, minor, patch) 0
    #define D_ENV_MYSQL_VERSION_IN_RANGE(min_maj,                              \
                                         min_min,                              \
                                         min_pat,                              \
                                         max_maj,                              \
                                         max_min,                              \
                                         max_pat) 0
    #define D_ENV_MYSQL_IS_5_5 0
    #define D_ENV_MYSQL_IS_5_6 0
    #define D_ENV_MYSQL_IS_5_7 0
    #define D_ENV_MYSQL_IS_8_0 0
    #define D_ENV_MYSQL_IS_8_4 0
    #define D_ENV_MYSQL_IS_9_PLUS 0
    #define D_ENV_MYSQL_IS_9_7 0
    #define D_ENV_MYSQL_IS_LTS 0
    #define D_ENV_MYSQL_IS_INNOVATION 0
    #define D_ENV_MYSQL_HAS_CLIENT_LIB 0
    #ifndef D_ENV_MYSQL_HAS_CONNECTOR_C
        #define D_ENV_MYSQL_HAS_CONNECTOR_C 0
    #endif  // D_ENV_MYSQL_HAS_CONNECTOR_C
    #ifndef D_ENV_MYSQL_HAS_LIBMYSQLCLIENT
        #define D_ENV_MYSQL_HAS_LIBMYSQLCLIENT 0
    #endif  // D_ENV_MYSQL_HAS_LIBMYSQLCLIENT
    #define D_ENV_MYSQL_HAS_EMBEDDED 0
    #ifndef D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING
        #define D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING 0
    #endif  // D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING
    #ifndef D_ENV_MYSQL_HAS_ASYNC_API
        #define D_ENV_MYSQL_HAS_ASYNC_API 0
    #endif  // D_ENV_MYSQL_HAS_ASYNC_API
    #ifndef D_ENV_MYSQL_HAS_RESET_CONNECTION
        #define D_ENV_MYSQL_HAS_RESET_CONNECTION 0
    #endif  // D_ENV_MYSQL_HAS_RESET_CONNECTION
    #ifndef D_ENV_MYSQL_HAS_SESSION_TRACK
        #define D_ENV_MYSQL_HAS_SESSION_TRACK 0
    #endif  // D_ENV_MYSQL_HAS_SESSION_TRACK
    #ifndef D_ENV_MYSQL_HAS_GET_OPTION
        #define D_ENV_MYSQL_HAS_GET_OPTION 0
    #endif  // D_ENV_MYSQL_HAS_GET_OPTION
    #ifndef D_ENV_MYSQL_HAS_STMT_NEXT_RESULT
        #define D_ENV_MYSQL_HAS_STMT_NEXT_RESULT 0
    #endif  // D_ENV_MYSQL_HAS_STMT_NEXT_RESULT
    #ifndef D_ENV_MYSQL_HAS_ZSTD_COMPRESSION
        #define D_ENV_MYSQL_HAS_ZSTD_COMPRESSION 0
    #endif  // D_ENV_MYSQL_HAS_ZSTD_COMPRESSION
    #ifndef D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA
        #define D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA 0
    #endif  // D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA
    #ifndef D_ENV_MYSQL_HAS_QUERY_ATTRS
        #define D_ENV_MYSQL_HAS_QUERY_ATTRS 0
    #endif  // D_ENV_MYSQL_HAS_QUERY_ATTRS
    #ifndef D_ENV_MYSQL_HAS_SSL
        #define D_ENV_MYSQL_HAS_SSL 0
    #endif  // D_ENV_MYSQL_HAS_SSL
    #define D_ENV_MYSQL_HAS_SSL_MODE 0
    #define D_ENV_MYSQL_HAS_TLS_VERSION_OPTION 0
    #define D_ENV_MYSQL_HAS_TLS_CIPHERSUITES 0
    #ifndef D_ENV_MYSQL_HAS_SSL_FIPS_MODE
        #define D_ENV_MYSQL_HAS_SSL_FIPS_MODE 0
    #endif  // D_ENV_MYSQL_HAS_SSL_FIPS_MODE
    #define D_ENV_MYSQL_HAS_GET_SSL_SESSION_REUSED 0
    #define D_ENV_MYSQL_HAS_AUTH_NATIVE 0
    #define D_ENV_MYSQL_AUTH_NATIVE_DEPRECATED 0
    #define D_ENV_MYSQL_HAS_PLUGGABLE_AUTH 0
    #define D_ENV_MYSQL_HAS_AUTH_SHA256 0
    #define D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2 0
    #define D_ENV_MYSQL_DEFAULT_AUTH_IS_CACHING_SHA2 0
    #define D_ENV_MYSQL_HAS_AUTH_LDAP_SIMPLE 0
    #define D_ENV_MYSQL_HAS_AUTH_LDAP_SASL 0
    #define D_ENV_MYSQL_HAS_AUTH_KERBEROS 0
    #define D_ENV_MYSQL_HAS_AUTH_OCI 0
    #define D_ENV_MYSQL_HAS_AUTH_FIDO 0
    #define D_ENV_MYSQL_HAS_MULTI_FACTOR_AUTH 0
    #define D_ENV_MYSQL_HAS_JSON_TYPE 0
    #define D_ENV_MYSQL_HAS_JSON_TABLE 0
    #define D_ENV_MYSQL_HAS_JSON_SCHEMA_VALIDATION 0
    #define D_ENV_MYSQL_HAS_JSON_VALUE 0
    #define D_ENV_MYSQL_HAS_JSON_ARRAYAGG 0
    #define D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX 0
    #define D_ENV_MYSQL_HAS_GEOMETRY_TYPES 0
    #define D_ENV_MYSQL_HAS_SRID_SUPPORT 0
    #define D_ENV_MYSQL_HAS_INVISIBLE_COLUMNS 0
    #define D_ENV_MYSQL_HAS_FUNCTIONAL_INDEX 0
    #define D_ENV_MYSQL_HAS_DESCENDING_INDEX 0
    #define D_ENV_MYSQL_HAS_GENERATED_COLUMNS 0
    #define D_ENV_MYSQL_HAS_DEFAULT_EXPRESSION 0
    #define D_ENV_MYSQL_HAS_CHECK_CONSTRAINTS 0
    #define D_ENV_MYSQL_HAS_INNODB 0
    #define D_ENV_MYSQL_HAS_MYISAM 0
    #define D_ENV_MYSQL_HAS_MEMORY_ENGINE 0
    #define D_ENV_MYSQL_HAS_NDB_CLUSTER 0
    #define D_ENV_MYSQL_HAS_ARCHIVE_ENGINE 0
    #define D_ENV_MYSQL_HAS_CSV_ENGINE 0
    #define D_ENV_MYSQL_HAS_BLACKHOLE_ENGINE 0
    #define D_ENV_MYSQL_HAS_FEDERATED_ENGINE 0
    #define D_ENV_MYSQL_SYSTEM_TABLES_USE_INNODB 0
    #define D_ENV_MYSQL_HAS_INNODB_FULLTEXT 0
    #define D_ENV_MYSQL_HAS_INNODB_SPATIAL_INDEX 0
    #define D_ENV_MYSQL_HAS_INNODB_ONLINE_DDL 0
    #define D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL 0
    #define D_ENV_MYSQL_HAS_INNODB_TABLESPACE_ENCRYPTION 0
    #define D_ENV_MYSQL_HAS_INNODB_REDO_LOG 0
    #define D_ENV_MYSQL_HAS_INNODB_UNDO_TRUNCATION 0
    #define D_ENV_MYSQL_HAS_INNODB_DEDICATED_SERVER 0
    #define D_ENV_MYSQL_HAS_GTID 0
    #define D_ENV_MYSQL_HAS_SEMI_SYNC_REPL 0
    #define D_ENV_MYSQL_HAS_MULTI_SOURCE_REPL 0
    #define D_ENV_MYSQL_HAS_GROUP_REPLICATION 0
    #define D_ENV_MYSQL_HAS_INNODB_CLUSTER 0
    #define D_ENV_MYSQL_HAS_INNODB_CLUSTERSET 0
    #define D_ENV_MYSQL_HAS_INNODB_REPLICASET 0
    #define D_ENV_MYSQL_HAS_CLONE_PLUGIN 0
    #define D_ENV_MYSQL_HAS_REPL_CHANNELS 0
    #define D_ENV_MYSQL_HAS_REPL_PRIVILEGE_CHECKS 0
    #define D_ENV_MYSQL_HAS_X_PROTOCOL 0
    #define D_ENV_MYSQL_HAS_X_DEVAPI 0
    #ifndef D_ENV_MYSQL_HAS_MYSQLX_PLUGIN
        #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 0
    #endif  // D_ENV_MYSQL_HAS_MYSQLX_PLUGIN
    #define D_ENV_MYSQL_HAS_UTF8MB4 0
    #define D_ENV_MYSQL_DEFAULT_CHARSET_IS_UTF8MB4 0
    #define D_ENV_MYSQL_HAS_UTF8MB4_0900 0
    #define D_ENV_MYSQL_UTF8_IS_UTF8MB3 0
    #define D_ENV_MYSQL_HAS_DATA_DICTIONARY 0
    #define D_ENV_MYSQL_HAS_RESOURCE_GROUPS 0
    #define D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS 0
    #define D_ENV_MYSQL_HAS_CTE 0
    #define D_ENV_MYSQL_HAS_LATERAL_DERIVED 0
    #define D_ENV_MYSQL_HAS_HASH_JOIN 0
    #define D_ENV_MYSQL_HAS_HISTOGRAMS 0
    #define D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE 0
    #define D_ENV_MYSQL_HAS_EXPLAIN_FORMAT_TREE 0
    #define D_ENV_MYSQL_HAS_OPTIMIZER_HINTS 0
    #define D_ENV_MYSQL_HAS_PERFORMANCE_SCHEMA 0
    #define D_ENV_MYSQL_HAS_SYS_SCHEMA 0
    #define D_ENV_MYSQL_HAS_ROLES 0
    #define D_ENV_MYSQL_HAS_PARTIAL_REVOKE 0
    #define D_ENV_MYSQL_HAS_AUDIT_LOG 0
    #define D_ENV_MYSQL_HAS_DATA_MASKING 0
    #define D_ENV_MYSQL_HAS_PASSWORD_HISTORY 0
    #define D_ENV_MYSQL_HAS_DUAL_PASSWORDS 0
    #define D_ENV_MYSQL_HAS_RANDOM_PASSWORD 0
    #define D_ENV_MYSQL_HAS_ATOMIC_DDL 0
    #define D_ENV_MYSQL_HAS_UNIX_SOCKET_CONN 0
    #define D_ENV_MYSQL_HAS_NAMED_PIPE 0
    #define D_ENV_MYSQL_HAS_SHARED_MEMORY 0
    #ifndef D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH
        #define D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH 0
    #endif  // D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH
    #define D_ENV_MYSQL_HAS_MODERN_AUTH 0
    #define D_ENV_MYSQL_HAS_MODERN_JSON 0
    #define D_ENV_MYSQL_HAS_MODERN_DDL 0
    #define D_ENV_MYSQL_HAS_MODERN_SQL 0
    #define D_ENV_MYSQL_HAS_HA_SUITE 0
    #define D_ENV_MYSQL_HAS_MODERN_SECURITY 0
    #define D_ENV_MYSQL_HAS_MODERN_OPTIMIZER 0
    #define D_ENV_MYSQL_IS_FULLY_MODERN 0
    #define D_ENV_MYSQL_REMOVED_QUERY_CACHE 0
    #define D_ENV_MYSQL_REMOVED_PARTITION_ENGINE 0
    #define D_ENV_MYSQL_REMOVED_PASSWORD_FUNCTION 0
    #define D_ENV_MYSQL_REMOVED_EMBEDDED_SERVER 0
    #define D_ENV_MYSQL_REMOVED_FRM_FILES 0
    #define D_ENV_MYSQL_DEPRECATED_UTF8MB3_ALIAS 0
    #define D_ENV_MYSQL_DEPRECATED_MYSQL_NATIVE_PASSWORD 0
#endif  // D_ENV_MYSQL_DETECTED


#endif  // DJINTERP_ENV_DB_MYSQL_ENV_MYSQL_H
