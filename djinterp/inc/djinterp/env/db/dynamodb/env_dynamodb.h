/*******************************************************************************
* djinterp [env]                                                  env_dynamodb.h
*
* djinterp Amazon DynamoDB environment detection.
*   Compile-time detection of an Amazon DynamoDB environment: the AWS SDK for
* C++ and its version, the optional DAX client, the deployment target (the
* managed cloud service or DynamoDB Local), and the capabilities built on them:
* SDK client features, the data model, indexes, capacity and scaling, requests
* and queries, streams, replication and backup, caching, and security.
*   DynamoDB is a managed service with no installable server and no server
* version. The only version visible at compile time is the SDK's, encoded as
* MAJOR*10000 + MINOR*100 + PATCH. Service capabilities are runtime properties
* instead: available on the cloud service unless opted out of with
* D_ENV_DYNAMODB_NO_<CAP>, and mostly absent on Local.
*   D_ENV_DDB_HAS_* are capability flags and D_ENV_DDB_IS_* the deployment
* target. Sections 4 onward exist only when the SDK is detected or its
* inclusion is enabled; the last of them publishes the D_ENV_DYNAMODB_*
* vocabulary that dynamodb.hpp consumes.
*   Settings live in env_dynamodb_config.h: D_CFG_ENV_USING_DYNAMODB includes
* the SDK header (C++ only), D_CFG_ENV_DYNAMODB_CUSTOM switches the SDK to
* manual detection, and D_CFG_ENV_DYNAMODB_TARGET names the deployment target.
* This header also includes env_db.h, for the base database detection.
*
*
* path:      /inc/djinterp/env/db/dynamodb/env_dynamodb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION
    -----------------------
    1.  Vendor headers
         1.  D_ENV_DYNAMODB_HEADER_INCLUDED
         2.  D_ENV_DYNAMODB_DAX_HEADER_INCLUDED
2.  VERSION ENCODING
    ----------------
    1.  Encoding and decoding
         1.  D_ENV_DDB_ENCODE_VERSION
         2.  D_ENV_DDB_DECODE_MAJOR
         3.  D_ENV_DDB_DECODE_MINOR
         4.  D_ENV_DDB_DECODE_PATCH
3.  AWS SDK VERSION DETECTION
    -------------------------
    1.  Detected SDK
         1.  D_ENV_DDB_SDK_DETECTED / D_ENV_DDB_SDK_*
         2.  D_ENV_DDB_SDK_KNOWN
         3.  D_ENV_DDB_DETECTED
4.  SDK VERSION COMPARISON MACROS
    -----------------------------
    1.  SDK comparisons
         1.  D_ENV_DDB_SDK_AT_LEAST / D_ENV_DDB_SDK_BELOW
5.  DEPLOYMENT TARGET
    -----------------
    1.  Target flags
         1.  D_ENV_DDB_IS_CLOUD
         2.  D_ENV_DDB_IS_LOCAL
6.  CAPABILITY PROFILE HELPERS
    --------------------------
    1.  Cloud opt-outs
         1.  D_ENV_DDB_OPT_NO_<CAP>
7.  SDK CLIENT FEATURES
    -------------------
    1.  SDK client features
         1.  D_ENV_DDB_HAS_SDK
         2.  D_ENV_DDB_HAS_ASYNC_API
         3.  D_ENV_DDB_HAS_DOCUMENT_INTERFACE
         4.  D_ENV_DDB_HAS_DAX_CLIENT
8.  DATA MODEL
    ----------
    1.  Data model
         1.  D_ENV_DDB_HAS_DOCUMENT_TYPES
         2.  D_ENV_DDB_HAS_SET_TYPES
9.  INDEXING
    --------
    1.  Indexes
         1.  D_ENV_DDB_HAS_GSI
         2.  D_ENV_DDB_HAS_LSI
10. CAPACITY AND SCALING
    --------------------
    1.  Capacity and scaling
         1.  D_ENV_DDB_HAS_PROVISIONED_CAPACITY
         2.  D_ENV_DDB_HAS_ON_DEMAND_CAPACITY
         3.  D_ENV_DDB_HAS_AUTO_SCALING
         4.  D_ENV_DDB_HAS_TABLE_CLASS_IA
11. REQUEST AND QUERY FEATURES
    --------------------------
    1.  Requests and queries
         1.  D_ENV_DDB_HAS_BATCH_OPS
         2.  D_ENV_DDB_HAS_CONDITIONAL_WRITES
         3.  D_ENV_DDB_HAS_ATOMIC_COUNTERS
         4.  D_ENV_DDB_HAS_PROJECTION_EXPRESSIONS
         5.  D_ENV_DDB_HAS_PARALLEL_SCAN
         6.  D_ENV_DDB_HAS_STRONG_CONSISTENCY
         7.  D_ENV_DDB_HAS_TRANSACTIONS
         8.  D_ENV_DDB_HAS_PARTIQL
12. STREAMS AND CHANGE DATA
    -----------------------
    1.  Streams and change data
         1.  D_ENV_DDB_HAS_STREAMS
         2.  D_ENV_DDB_HAS_KINESIS_STREAMS
13. REPLICATION, BACKUP, AND LIFECYCLE
    ----------------------------------
    1.  Replication, backup and lifecycle
         1.  D_ENV_DDB_HAS_GLOBAL_TABLES
         2.  D_ENV_DDB_HAS_PITR
         3.  D_ENV_DDB_HAS_ON_DEMAND_BACKUP
         4.  D_ENV_DDB_HAS_TTL
         5.  D_ENV_DDB_HAS_EXPORT_TO_S3
         6.  D_ENV_DDB_HAS_CONTRIBUTOR_INSIGHTS
14. CACHING (DAX)
    -------------
    1.  Caching
         1.  D_ENV_DDB_HAS_DAX
15. SECURITY
    --------
    1.  Security
         1.  D_ENV_DDB_HAS_ENCRYPTION_AT_REST
         2.  D_ENV_DDB_HAS_KMS_CMK
         3.  D_ENV_DDB_HAS_TLS
         4.  D_ENV_DDB_HAS_IAM_ACCESS_CONTROL
         5.  D_ENV_DDB_HAS_FINE_GRAINED_ACCESS
         6.  D_ENV_DDB_HAS_VPC_ENDPOINTS
16. COMPOSITE CHECKS
    ----------------
    1.  Composite checks
         1.  D_ENV_DDB_HAS_FLEXIBLE_CAPACITY
         2.  D_ENV_DDB_HAS_MODERN_QUERYING
         3.  D_ENV_DDB_HAS_RESILIENCE
         4.  D_ENV_DDB_HAS_MODERN_SECURITY
         5.  D_ENV_DDB_IS_FULLY_MANAGED_CLOUD
17. CONSUMER COMPATIBILITY LAYER
    ----------------------------
    1.  Consumer vocabulary
         1.  D_ENV_DYNAMODB_*
*/

#ifndef DJINTERP_ENV_DB_DYNAMODB_ENV_DYNAMODB_H
#define DJINTERP_ENV_DB_DYNAMODB_ENV_DYNAMODB_H 1

// djinterp
#include "./env_dynamodb_config.h"  // D_CFG_ENV_*
#include "../env_db.h"              // base database detection (D_ENV_DB_*)


//==============================================================================
// 1.  VENDOR HEADER INCLUSION
//==============================================================================
// Driven by D_CFG_ENV_USING_DYNAMODB, from env_dynamodb_config.h. The AWS SDK
// for C++ is a C++-only API, so this section engages only in C++ builds, and
// a C build with the setting on is an #error: DynamoDB is consumed over HTTPS,
// with no C client. Detection below is gated on
// D_ENV_DYNAMODB_HEADER_INCLUDED.


// 1.1    Vendor headers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_DYNAMODB_HEADER_INCLUDED
//   detection: 1 once this section has included the SDK's DynamoDB client
// header, and 0 when D_CFG_ENV_USING_DYNAMODB is off;
// D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP follows it unless pre-defined. With the
// setting on, a C build or a missing header is an #error. The SDK's
// aws/core/VersionConfig.h, when present, is included first, for its
// version macros.
#if (D_CFG_ENV_USING_DYNAMODB == 1)

    #ifdef __cplusplus

        // SDK version header is small and dependency-free; include it first
        // so version macros are available even if the client header layout
        // changes between SDK releases.
        #if defined(__has_include)
            #if __has_include(<aws/core/VersionConfig.h>)
                // aws
                #include <aws/core/VersionConfig.h>  // AWS_SDK_VERSION_*
            #endif
        #endif

        #if defined(__has_include)
            #if __has_include(D_CFG_ENV_DYNAMODB_CPP_PATH)
                #include D_CFG_ENV_DYNAMODB_CPP_PATH  // SDK DynamoDB client
                #define D_ENV_DYNAMODB_HEADER_INCLUDED 1
            #elif __has_include(<aws/dynamodb/DynamoDBClient.h>)
                // aws
                #include <aws/dynamodb/DynamoDBClient.h>  // SDK DynamoDB client
                #define D_ENV_DYNAMODB_HEADER_INCLUDED 1
            #else
                #error "D_CFG_ENV_USING_DYNAMODB=1 but no AWS SDK DynamoDB "   \
                       "client header was found. Install the AWS SDK for "     \
                       "C++ (aws-sdk-cpp dynamodb component), or define "      \
                       "D_CFG_ENV_DYNAMODB_CPP_PATH to the correct location."
            #endif
        #else
            #include D_CFG_ENV_DYNAMODB_CPP_PATH  // SDK DynamoDB client
            #define D_ENV_DYNAMODB_HEADER_INCLUDED 1
        #endif

        #ifndef D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP
            #define D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP 1
        #endif  // D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP

    #else  // !__cplusplus
        #error "D_CFG_ENV_USING_DYNAMODB=1 requires a C++ build. DynamoDB "    \
               "has no C client surface; use the AWS SDK for C++ or consume "  \
               "the service via its HTTPS API directly."
    #endif  // __cplusplus

#else
    #define D_ENV_DYNAMODB_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP
        #define D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP 0
    #endif  // D_ENV_DB_HAS_DYNAMODB_CLIENT_CPP
#endif  // D_CFG_ENV_USING_DYNAMODB

// 1.1.2
// D_ENV_DYNAMODB_DAX_HEADER_INCLUDED
//   detection: 1 once the optional DAX (DynamoDB Accelerator) C++ client, a
// separate library, is found at its conventional path and included; 0
// otherwise, and always 0 unless D_CFG_ENV_USING_DYNAMODB is on in a C++
// build with __has_include.
#if ( (D_CFG_ENV_USING_DYNAMODB == 1) &&                                       \
      (defined(__cplusplus))          &&                                       \
      (defined(__has_include)) )
    #if __has_include(<aws/dax/DaxClient.h>)
        // aws
        #include <aws/dax/DaxClient.h>  // DAX client
        #define D_ENV_DYNAMODB_DAX_HEADER_INCLUDED 1
    #else
        #define D_ENV_DYNAMODB_DAX_HEADER_INCLUDED 0
    #endif
#else
    #define D_ENV_DYNAMODB_DAX_HEADER_INCLUDED 0
#endif


//==============================================================================
// 2.  VERSION ENCODING
//==============================================================================


// 2.1    Encoding and decoding
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_DDB_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple.
#define D_ENV_DDB_ENCODE_VERSION(major, minor, patch)                          \
    ((major) * 10000 + (minor) * 100 + (patch))

// 2.1.2
// D_ENV_DDB_DECODE_MAJOR
//   macro: extracts the major version.
#define D_ENV_DDB_DECODE_MAJOR(ver)                                            \
    ((ver) / 10000)

// 2.1.3
// D_ENV_DDB_DECODE_MINOR
//   macro: extracts the minor version.
#define D_ENV_DDB_DECODE_MINOR(ver)                                            \
    (((ver) / 100) % 100)

// 2.1.4
// D_ENV_DDB_DECODE_PATCH
//   macro: extracts the patch version.
#define D_ENV_DDB_DECODE_PATCH(ver)                                            \
    ((ver) % 100)


//==============================================================================
// 3.  AWS SDK VERSION DETECTION
//==============================================================================
// The AWS SDK for C++ exposes its version via AWS_SDK_VERSION_MAJOR /
// _MINOR / _PATCH (declared in aws/core/VersionConfig.h). This is the only
// software version DynamoDB detection can observe at compile time.


// 3.1    Detected SDK
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_DDB_SDK_DETECTED / D_ENV_DDB_SDK_*
//   detection: D_ENV_DDB_SDK_DETECTED is 1 when the AWS SDK for C++ version is
// known, and D_ENV_DDB_SDK_VERSION_ID, _MAJOR, _MINOR and _PATCH then describe
// it. Automatic mode reads AWS_SDK_VERSION_MAJOR, _MINOR and _PATCH; manual
// mode (D_CFG_ENV_DYNAMODB_CUSTOM) reads D_ENV_DYNAMODB_DETECTED_SDK_VERSION.
#if (D_CFG_ENV_DYNAMODB_CUSTOM == 0)

    #if ( (D_ENV_DYNAMODB_HEADER_INCLUDED) &&                                  \
          (defined(AWS_SDK_VERSION_MAJOR)) )
        #define D_ENV_DDB_SDK_DETECTED     1
        #define D_ENV_DDB_SDK_MAJOR        AWS_SDK_VERSION_MAJOR
        #define D_ENV_DDB_SDK_MINOR        AWS_SDK_VERSION_MINOR
        #define D_ENV_DDB_SDK_PATCH        AWS_SDK_VERSION_PATCH
        #define D_ENV_DDB_SDK_VERSION_ID                                       \
            D_ENV_DDB_ENCODE_VERSION(AWS_SDK_VERSION_MAJOR,                    \
                                      AWS_SDK_VERSION_MINOR,                   \
                                      AWS_SDK_VERSION_PATCH)
    #else
        #define D_ENV_DDB_SDK_DETECTED     0
    #endif

#else
    // manual mode
    #ifdef D_ENV_DYNAMODB_DETECTED_SDK_VERSION
        #define D_ENV_DDB_SDK_DETECTED     1
        #define D_ENV_DDB_SDK_VERSION_ID                                       \
            D_ENV_DYNAMODB_DETECTED_SDK_VERSION
        #define D_ENV_DDB_SDK_MAJOR                                            \
            D_ENV_DDB_DECODE_MAJOR(D_ENV_DYNAMODB_DETECTED_SDK_VERSION)
        #define D_ENV_DDB_SDK_MINOR                                            \
            D_ENV_DDB_DECODE_MINOR(D_ENV_DYNAMODB_DETECTED_SDK_VERSION)
        #define D_ENV_DDB_SDK_PATCH                                            \
            D_ENV_DDB_DECODE_PATCH(D_ENV_DYNAMODB_DETECTED_SDK_VERSION)
    #else
        #define D_ENV_DDB_SDK_DETECTED     0
    #endif  // D_ENV_DYNAMODB_DETECTED_SDK_VERSION

#endif  // D_CFG_ENV_DYNAMODB_CUSTOM

// 3.1.2
// D_ENV_DDB_SDK_KNOWN
//   status: 1 if an AWS SDK version is known at compile time.
#define D_ENV_DDB_SDK_KNOWN                                                    \
    D_ENV_DDB_SDK_DETECTED

// 3.1.3
// D_ENV_DDB_DETECTED
//   detection: 1 if the SDK is detected or D_CFG_ENV_USING_DYNAMODB is on.
// Neither the deployment target, which always resolves, nor a declared
// capability profile counts.
#define D_ENV_DDB_DETECTED                                                     \
    ( (D_ENV_DDB_SDK_DETECTED) ||                                              \
      (D_CFG_ENV_USING_DYNAMODB == 1) )


// sections 4 to 17 exist only when D_ENV_DDB_DETECTED is 1; see 3.1.3
#if D_ENV_DDB_DETECTED


//==============================================================================
// 4.  SDK VERSION COMPARISON MACROS
//==============================================================================


// 4.1    SDK comparisons
//------------------------------------------------------------------------------
    // 4.1.1
    // D_ENV_DDB_SDK_AT_LEAST / D_ENV_DDB_SDK_BELOW
    //   macro: 1 if the detected SDK is at least, or below, major.minor.patch;
    // both are 0 when no SDK version is known.
    #if D_ENV_DDB_SDK_DETECTED
        #define D_ENV_DDB_SDK_AT_LEAST(major, minor, patch)                    \
            (D_ENV_DDB_SDK_VERSION_ID >=                                       \
                D_ENV_DDB_ENCODE_VERSION(major, minor, patch))
        #define D_ENV_DDB_SDK_BELOW(major, minor, patch)                       \
            (D_ENV_DDB_SDK_VERSION_ID <                                        \
                D_ENV_DDB_ENCODE_VERSION(major, minor, patch))
    #else
        #define D_ENV_DDB_SDK_AT_LEAST(major, minor, patch) 0
        #define D_ENV_DDB_SDK_BELOW(major, minor, patch)    0
    #endif


//==============================================================================
// 5.  DEPLOYMENT TARGET
//==============================================================================
// The deployment target is supplied via D_CFG_ENV_DYNAMODB_TARGET (see
// env_dynamodb_config.h). These convenience macros expose it as boolean
// flags. Several capability flags below differ between CLOUD and LOCAL.


// 5.1    Target flags
//------------------------------------------------------------------------------
    // 5.1.1
    // D_ENV_DDB_IS_CLOUD
    //   detection: 1 if targeting the managed AWS DynamoDB service.
    #define D_ENV_DDB_IS_CLOUD                                                 \
        (D_CFG_ENV_DYNAMODB_TARGET == D_CFG_ENV_DYNAMODB_TARGET_CLOUD)

    // 5.1.2
    // D_ENV_DDB_IS_LOCAL
    //   detection: 1 if targeting DynamoDB Local (development build).
    #define D_ENV_DDB_IS_LOCAL                                                 \
        (D_CFG_ENV_DYNAMODB_TARGET == D_CFG_ENV_DYNAMODB_TARGET_LOCAL)


//==============================================================================
// 6.  CAPABILITY PROFILE HELPERS
//==============================================================================
// Service capabilities are runtime properties, not versioned features. On the
// managed cloud service, a capability is available unless the integrator
// opts out by defining the matching D_ENV_DYNAMODB_NO_<CAP>. On DynamoDB
// Local, which emulates only part of the service, the cloud-only
// capabilities read 0. Each opt-out resolves here to a 0 or 1 constant, so
// that no capability macro below expands to a `defined` operator, which is
// undefined behaviour inside an #if.


// 6.1    Cloud opt-outs
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_DDB_OPT_NO_<CAP>
    //   constant: 1 if the integrator defined D_ENV_DYNAMODB_NO_<CAP>, and 0
    // otherwise; one per capability that can be opted out of.
    #ifdef D_ENV_DYNAMODB_NO_GSI
        #define D_ENV_DDB_OPT_NO_GSI 1
    #else
        #define D_ENV_DDB_OPT_NO_GSI 0
    #endif  // D_ENV_DYNAMODB_NO_GSI
    #ifdef D_ENV_DYNAMODB_NO_LSI
        #define D_ENV_DDB_OPT_NO_LSI 1
    #else
        #define D_ENV_DDB_OPT_NO_LSI 0
    #endif  // D_ENV_DYNAMODB_NO_LSI
    #ifdef D_ENV_DYNAMODB_NO_PROVISIONED
        #define D_ENV_DDB_OPT_NO_PROVISIONED 1
    #else
        #define D_ENV_DDB_OPT_NO_PROVISIONED 0
    #endif  // D_ENV_DYNAMODB_NO_PROVISIONED
    #ifdef D_ENV_DYNAMODB_NO_ON_DEMAND
        #define D_ENV_DDB_OPT_NO_ON_DEMAND 1
    #else
        #define D_ENV_DDB_OPT_NO_ON_DEMAND 0
    #endif  // D_ENV_DYNAMODB_NO_ON_DEMAND
    #ifdef D_ENV_DYNAMODB_NO_AUTO_SCALING
        #define D_ENV_DDB_OPT_NO_AUTO_SCALING 1
    #else
        #define D_ENV_DDB_OPT_NO_AUTO_SCALING 0
    #endif  // D_ENV_DYNAMODB_NO_AUTO_SCALING
    #ifdef D_ENV_DYNAMODB_NO_TABLE_CLASS_IA
        #define D_ENV_DDB_OPT_NO_TABLE_CLASS_IA 1
    #else
        #define D_ENV_DDB_OPT_NO_TABLE_CLASS_IA 0
    #endif  // D_ENV_DYNAMODB_NO_TABLE_CLASS_IA
    #ifdef D_ENV_DYNAMODB_NO_TRANSACTIONS
        #define D_ENV_DDB_OPT_NO_TRANSACTIONS 1
    #else
        #define D_ENV_DDB_OPT_NO_TRANSACTIONS 0
    #endif  // D_ENV_DYNAMODB_NO_TRANSACTIONS
    #ifdef D_ENV_DYNAMODB_NO_PARTIQL
        #define D_ENV_DDB_OPT_NO_PARTIQL 1
    #else
        #define D_ENV_DDB_OPT_NO_PARTIQL 0
    #endif  // D_ENV_DYNAMODB_NO_PARTIQL
    #ifdef D_ENV_DYNAMODB_NO_STREAMS
        #define D_ENV_DDB_OPT_NO_STREAMS 1
    #else
        #define D_ENV_DDB_OPT_NO_STREAMS 0
    #endif  // D_ENV_DYNAMODB_NO_STREAMS
    #ifdef D_ENV_DYNAMODB_NO_KINESIS
        #define D_ENV_DDB_OPT_NO_KINESIS 1
    #else
        #define D_ENV_DDB_OPT_NO_KINESIS 0
    #endif  // D_ENV_DYNAMODB_NO_KINESIS
    #ifdef D_ENV_DYNAMODB_NO_GLOBAL_TABLES
        #define D_ENV_DDB_OPT_NO_GLOBAL_TABLES 1
    #else
        #define D_ENV_DDB_OPT_NO_GLOBAL_TABLES 0
    #endif  // D_ENV_DYNAMODB_NO_GLOBAL_TABLES
    #ifdef D_ENV_DYNAMODB_NO_PITR
        #define D_ENV_DDB_OPT_NO_PITR 1
    #else
        #define D_ENV_DDB_OPT_NO_PITR 0
    #endif  // D_ENV_DYNAMODB_NO_PITR
    #ifdef D_ENV_DYNAMODB_NO_BACKUP
        #define D_ENV_DDB_OPT_NO_BACKUP 1
    #else
        #define D_ENV_DDB_OPT_NO_BACKUP 0
    #endif  // D_ENV_DYNAMODB_NO_BACKUP
    #ifdef D_ENV_DYNAMODB_NO_TTL
        #define D_ENV_DDB_OPT_NO_TTL 1
    #else
        #define D_ENV_DDB_OPT_NO_TTL 0
    #endif  // D_ENV_DYNAMODB_NO_TTL
    #ifdef D_ENV_DYNAMODB_NO_EXPORT_S3
        #define D_ENV_DDB_OPT_NO_EXPORT_S3 1
    #else
        #define D_ENV_DDB_OPT_NO_EXPORT_S3 0
    #endif  // D_ENV_DYNAMODB_NO_EXPORT_S3
    #ifdef D_ENV_DYNAMODB_NO_CONTRIBUTOR_INSIGHTS
        #define D_ENV_DDB_OPT_NO_CONTRIBUTOR_INSIGHTS 1
    #else
        #define D_ENV_DDB_OPT_NO_CONTRIBUTOR_INSIGHTS 0
    #endif  // D_ENV_DYNAMODB_NO_CONTRIBUTOR_INSIGHTS
    #ifdef D_ENV_DYNAMODB_NO_KMS_CMK
        #define D_ENV_DDB_OPT_NO_KMS_CMK 1
    #else
        #define D_ENV_DDB_OPT_NO_KMS_CMK 0
    #endif  // D_ENV_DYNAMODB_NO_KMS_CMK
    #ifdef D_ENV_DYNAMODB_NO_FINE_GRAINED_ACCESS
        #define D_ENV_DDB_OPT_NO_FINE_GRAINED_ACCESS 1
    #else
        #define D_ENV_DDB_OPT_NO_FINE_GRAINED_ACCESS 0
    #endif  // D_ENV_DYNAMODB_NO_FINE_GRAINED_ACCESS
    #ifdef D_ENV_DYNAMODB_NO_VPC_ENDPOINTS
        #define D_ENV_DDB_OPT_NO_VPC_ENDPOINTS 1
    #else
        #define D_ENV_DDB_OPT_NO_VPC_ENDPOINTS 0
    #endif  // D_ENV_DYNAMODB_NO_VPC_ENDPOINTS


//==============================================================================
// 7.  SDK CLIENT FEATURES
//==============================================================================


// 7.1    SDK client features
//------------------------------------------------------------------------------
    // 7.1.1
    // D_ENV_DDB_HAS_SDK
    //   feature: detect if the AWS SDK for C++ DynamoDB client is present.
    #define D_ENV_DDB_HAS_SDK D_ENV_DDB_SDK_DETECTED

    // 7.1.2
    // D_ENV_DDB_HAS_ASYNC_API
    //   feature: asynchronous client operations (…Async methods returning
    // futures / invoking callbacks). Present in all modern SDK versions.
    #define D_ENV_DDB_HAS_ASYNC_API D_ENV_DDB_SDK_DETECTED

    // 7.1.3
    // D_ENV_DDB_HAS_DOCUMENT_INTERFACE
    //   feature: higher-level document/attribute-value modeling helpers
    // in the SDK. Present in all modern SDK versions.
    #define D_ENV_DDB_HAS_DOCUMENT_INTERFACE D_ENV_DDB_SDK_DETECTED

    // 7.1.4
    // D_ENV_DDB_HAS_DAX_CLIENT
    //   feature: the DAX (in-memory accelerator) client library is in
    // scope. Optional and separate from the core DynamoDB client.
    #define D_ENV_DDB_HAS_DAX_CLIENT D_ENV_DYNAMODB_DAX_HEADER_INCLUDED


//==============================================================================
// 8.  DATA MODEL
//==============================================================================
// The DynamoDB data model is service-intrinsic and present on every
// target (cloud and local). These flags are gated only on detection.


// 8.1    Data model
//------------------------------------------------------------------------------
    // D_ENV_DDB_HAS_PARTITION_KEY / SORT_KEY
    //   feature: partition (hash) keys and optional sort (range) keys.
    #define D_ENV_DDB_HAS_PARTITION_KEY D_ENV_DDB_DETECTED
    #define D_ENV_DDB_HAS_SORT_KEY      D_ENV_DDB_DETECTED

    // 8.1.1
    // D_ENV_DDB_HAS_DOCUMENT_TYPES
    //   feature: nested document types (map, list) alongside scalar and
    // set types.
    #define D_ENV_DDB_HAS_DOCUMENT_TYPES D_ENV_DDB_DETECTED

    // 8.1.2
    // D_ENV_DDB_HAS_SET_TYPES
    //   feature: typed set attributes (string set, number set, binary
    // set).
    #define D_ENV_DDB_HAS_SET_TYPES D_ENV_DDB_DETECTED


//==============================================================================
// 9.  INDEXING
//==============================================================================


// 9.1    Indexes
//------------------------------------------------------------------------------
    // 9.1.1
    // D_ENV_DDB_HAS_GSI
    //   capability: global secondary indexes. Available on cloud and
    // emulated by DynamoDB Local; honors the GSI opt-out.
    #define D_ENV_DDB_HAS_GSI                                                  \
        (!D_ENV_DDB_OPT_NO_GSI)

    // 9.1.2
    // D_ENV_DDB_HAS_LSI
    //   capability: local secondary indexes.
    #define D_ENV_DDB_HAS_LSI                                                  \
        (!D_ENV_DDB_OPT_NO_LSI)


//==============================================================================
// 10.  CAPACITY AND SCALING
//==============================================================================
// These are cloud-only operational features; DynamoDB Local ignores
// capacity settings entirely.


// 10.1   Capacity and scaling
//------------------------------------------------------------------------------
    // 10.1.1
    // D_ENV_DDB_HAS_PROVISIONED_CAPACITY
    //   capability: provisioned read/write capacity mode.
    #define D_ENV_DDB_HAS_PROVISIONED_CAPACITY                                 \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_PROVISIONED) )

    // 10.1.2
    // D_ENV_DDB_HAS_ON_DEMAND_CAPACITY
    //   capability: on-demand (pay-per-request) capacity mode.
    #define D_ENV_DDB_HAS_ON_DEMAND_CAPACITY                                   \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_ON_DEMAND) )

    // 10.1.3
    // D_ENV_DDB_HAS_AUTO_SCALING
    //   capability: application auto scaling of provisioned capacity.
    #define D_ENV_DDB_HAS_AUTO_SCALING                                         \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_AUTO_SCALING) )

    // 10.1.4
    // D_ENV_DDB_HAS_TABLE_CLASS_IA
    //   capability: Standard-Infrequent Access table class.
    #define D_ENV_DDB_HAS_TABLE_CLASS_IA                                       \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_TABLE_CLASS_IA) )


//==============================================================================
// 11.  REQUEST AND QUERY FEATURES
//==============================================================================
// These are protocol/API features available wherever the service (or its
// local emulation) responds. Gated on detection, with opt-out for unusual
// constrained environments.


// 11.1   Requests and queries
//------------------------------------------------------------------------------
    // 11.1.1
    // D_ENV_DDB_HAS_BATCH_OPS
    //   feature: BatchGetItem / BatchWriteItem.
    #define D_ENV_DDB_HAS_BATCH_OPS D_ENV_DDB_DETECTED

    // 11.1.2
    // D_ENV_DDB_HAS_CONDITIONAL_WRITES
    //   feature: conditional expressions on writes (optimistic
    // concurrency, attribute_exists, etc.).
    #define D_ENV_DDB_HAS_CONDITIONAL_WRITES D_ENV_DDB_DETECTED

    // 11.1.3
    // D_ENV_DDB_HAS_ATOMIC_COUNTERS
    //   feature: atomic counter updates via UpdateItem ADD/SET.
    #define D_ENV_DDB_HAS_ATOMIC_COUNTERS D_ENV_DDB_DETECTED

    // 11.1.4
    // D_ENV_DDB_HAS_PROJECTION_EXPRESSIONS
    //   feature: projection and filter expressions.
    #define D_ENV_DDB_HAS_PROJECTION_EXPRESSIONS D_ENV_DDB_DETECTED

    // 11.1.5
    // D_ENV_DDB_HAS_PARALLEL_SCAN
    //   feature: segmented parallel Scan.
    #define D_ENV_DDB_HAS_PARALLEL_SCAN D_ENV_DDB_DETECTED

    // 11.1.6
    // D_ENV_DDB_HAS_STRONG_CONSISTENCY
    //   feature: strongly consistent reads (in addition to eventually
    // consistent). Available on cloud; DynamoDB Local treats all reads as
    // strongly consistent.
    #define D_ENV_DDB_HAS_STRONG_CONSISTENCY D_ENV_DDB_DETECTED

    // 11.1.7
    // D_ENV_DDB_HAS_TRANSACTIONS
    //   capability: ACID transactions (TransactWriteItems /
    // TransactGetItems). Available on cloud; emulated by recent local
    // builds.
    #define D_ENV_DDB_HAS_TRANSACTIONS                                         \
        ( (D_ENV_DDB_DETECTED) &&                                              \
          (!D_ENV_DDB_OPT_NO_TRANSACTIONS) )

    // 11.1.8
    // D_ENV_DDB_HAS_PARTIQL
    //   capability: PartiQL (SQL-compatible) statements and batch
    // execution. Requires SDK support and cloud (or recent local).
    #define D_ENV_DDB_HAS_PARTIQL                                              \
        ( (D_ENV_DDB_DETECTED) &&                                              \
          (!D_ENV_DDB_OPT_NO_PARTIQL) )


//==============================================================================
// 12.  STREAMS AND CHANGE DATA
//==============================================================================


// 12.1   Streams and change data
//------------------------------------------------------------------------------
    // 12.1.1
    // D_ENV_DDB_HAS_STREAMS
    //   capability: DynamoDB Streams (ordered change records). Available
    // on cloud; emulated by local.
    #define D_ENV_DDB_HAS_STREAMS                                              \
        ( (D_ENV_DDB_DETECTED) &&                                              \
          (!D_ENV_DDB_OPT_NO_STREAMS) )

    // 12.1.2
    // D_ENV_DDB_HAS_KINESIS_STREAMS
    //   capability: Kinesis Data Streams for DynamoDB (change capture to
    // Kinesis). Cloud only.
    #define D_ENV_DDB_HAS_KINESIS_STREAMS                                      \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_KINESIS) )


//==============================================================================
// 13.  REPLICATION, BACKUP, AND LIFECYCLE
//==============================================================================
// These are cloud-only managed features with no local emulation.


// 13.1   Replication, backup and lifecycle
//------------------------------------------------------------------------------
    // 13.1.1
    // D_ENV_DDB_HAS_GLOBAL_TABLES
    //   capability: global tables (active-active multi-region
    // replication).
    #define D_ENV_DDB_HAS_GLOBAL_TABLES                                        \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_GLOBAL_TABLES) )

    // 13.1.2
    // D_ENV_DDB_HAS_PITR
    //   capability: point-in-time recovery (continuous backups).
    #define D_ENV_DDB_HAS_PITR                                                 \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_PITR) )

    // 13.1.3
    // D_ENV_DDB_HAS_ON_DEMAND_BACKUP
    //   capability: on-demand full backups and restore.
    #define D_ENV_DDB_HAS_ON_DEMAND_BACKUP                                     \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_BACKUP) )

    // 13.1.4
    // D_ENV_DDB_HAS_TTL
    //   capability: time-to-live automatic item expiry.
    #define D_ENV_DDB_HAS_TTL                                                  \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_TTL) )

    // 13.1.5
    // D_ENV_DDB_HAS_EXPORT_TO_S3
    //   capability: export table data to Amazon S3 (and import from S3).
    #define D_ENV_DDB_HAS_EXPORT_TO_S3                                         \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_EXPORT_S3) )

    // 13.1.6
    // D_ENV_DDB_HAS_CONTRIBUTOR_INSIGHTS
    //   capability: CloudWatch Contributor Insights for DynamoDB.
    #define D_ENV_DDB_HAS_CONTRIBUTOR_INSIGHTS                                 \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_CONTRIBUTOR_INSIGHTS) )


//==============================================================================
// 14.  CACHING (DAX)
//==============================================================================


// 14.1   Caching
//------------------------------------------------------------------------------
    // 14.1.1
    // D_ENV_DDB_HAS_DAX
    //   capability: DynamoDB Accelerator (DAX) microsecond read caching.
    // Requires the DAX client library and cloud deployment.
    #define D_ENV_DDB_HAS_DAX                                                  \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (D_ENV_DDB_HAS_DAX_CLIENT) )


//==============================================================================
// 15.  SECURITY
//==============================================================================


// 15.1   Security
//------------------------------------------------------------------------------
    // 15.1.1
    // D_ENV_DDB_HAS_ENCRYPTION_AT_REST
    //   capability: encryption at rest. Always on for the cloud service;
    // not applicable to local.
    #define D_ENV_DDB_HAS_ENCRYPTION_AT_REST                                   \
        D_ENV_DDB_IS_CLOUD

    // 15.1.2
    // D_ENV_DDB_HAS_KMS_CMK
    //   capability: customer-managed KMS keys for encryption at rest.
    #define D_ENV_DDB_HAS_KMS_CMK                                              \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_KMS_CMK) )

    // 15.1.3
    // D_ENV_DDB_HAS_TLS
    //   capability: TLS-encrypted transport. The cloud service requires
    // HTTPS; local can be reached over plain HTTP in dev.
    #define D_ENV_DDB_HAS_TLS                                                  \
        D_ENV_DDB_IS_CLOUD

    // 15.1.4
    // D_ENV_DDB_HAS_IAM_ACCESS_CONTROL
    //   capability: IAM policy-based access control.
    #define D_ENV_DDB_HAS_IAM_ACCESS_CONTROL                                   \
        D_ENV_DDB_IS_CLOUD

    // 15.1.5
    // D_ENV_DDB_HAS_FINE_GRAINED_ACCESS
    //   capability: fine-grained access control (IAM conditions on
    // partition keys / attributes).
    #define D_ENV_DDB_HAS_FINE_GRAINED_ACCESS                                  \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_FINE_GRAINED_ACCESS) )

    // 15.1.6
    // D_ENV_DDB_HAS_VPC_ENDPOINTS
    //   capability: VPC gateway/interface endpoints for private access.
    #define D_ENV_DDB_HAS_VPC_ENDPOINTS                                        \
        ( (D_ENV_DDB_IS_CLOUD) &&                                              \
          (!D_ENV_DDB_OPT_NO_VPC_ENDPOINTS) )


//==============================================================================
// 16.  COMPOSITE CHECKS
//==============================================================================


// 16.1   Composite checks
//------------------------------------------------------------------------------
    // 16.1.1
    // D_ENV_DDB_HAS_FLEXIBLE_CAPACITY
    //   macro: evaluates to 1 if both provisioned (with auto scaling) and
    // on-demand capacity modes are available.
    #define D_ENV_DDB_HAS_FLEXIBLE_CAPACITY                                    \
        ( (D_ENV_DDB_HAS_PROVISIONED_CAPACITY) &&                              \
          (D_ENV_DDB_HAS_ON_DEMAND_CAPACITY)   &&                              \
          (D_ENV_DDB_HAS_AUTO_SCALING) )

    // 16.1.2
    // D_ENV_DDB_HAS_MODERN_QUERYING
    //   macro: evaluates to 1 if transactions and PartiQL are both
    // available alongside conditional writes.
    #define D_ENV_DDB_HAS_MODERN_QUERYING                                      \
        ( (D_ENV_DDB_HAS_TRANSACTIONS) &&                                      \
          (D_ENV_DDB_HAS_PARTIQL)      &&                                      \
          (D_ENV_DDB_HAS_CONDITIONAL_WRITES) )

    // 16.1.3
    // D_ENV_DDB_HAS_RESILIENCE
    //   macro: evaluates to 1 if global tables, point-in-time recovery,
    // and on-demand backup are all available.
    #define D_ENV_DDB_HAS_RESILIENCE                                           \
        ( (D_ENV_DDB_HAS_GLOBAL_TABLES) &&                                     \
          (D_ENV_DDB_HAS_PITR)          &&                                     \
          (D_ENV_DDB_HAS_ON_DEMAND_BACKUP) )

    // 16.1.4
    // D_ENV_DDB_HAS_MODERN_SECURITY
    //   macro: evaluates to 1 if encryption at rest with customer-managed
    // keys, TLS, and fine-grained IAM access control are all available.
    #define D_ENV_DDB_HAS_MODERN_SECURITY                                      \
        ( (D_ENV_DDB_HAS_ENCRYPTION_AT_REST) &&                                \
          (D_ENV_DDB_HAS_KMS_CMK)            &&                                \
          (D_ENV_DDB_HAS_TLS)                &&                                \
          (D_ENV_DDB_HAS_FINE_GRAINED_ACCESS) )

    // 16.1.5
    // D_ENV_DDB_IS_FULLY_MANAGED_CLOUD
    //   macro: evaluates to 1 if the target is the managed cloud service
    // with the full resilience, querying, capacity, and security feature
    // set available.
    #define D_ENV_DDB_IS_FULLY_MANAGED_CLOUD                                   \
        ( (D_ENV_DDB_IS_CLOUD)              &&                                 \
          (D_ENV_DDB_HAS_FLEXIBLE_CAPACITY) &&                                 \
          (D_ENV_DDB_HAS_MODERN_QUERYING)   &&                                 \
          (D_ENV_DDB_HAS_RESILIENCE)        &&                                 \
          (D_ENV_DDB_HAS_MODERN_SECURITY) )


//==============================================================================
// 17.  CONSUMER COMPATIBILITY LAYER
//==============================================================================
// The djinterp DynamoDB connection layer (dynamodb.hpp, dynamodb_table.hpp)
// consumes a stable vocabulary prefixed D_ENV_DYNAMODB_*, while this header
// uses the shorter D_ENV_DDB_* internally; this section republishes the public
// names in terms of that model. DynamoDB has no server version, so the public
// version is the AWS SDK for C++'s, and D_ENV_DYNAMODB_API_VERSION carries the
// wire API date, a fixed service constant.


// 17.1   Consumer vocabulary
//------------------------------------------------------------------------------
    // 17.1.1
    // D_ENV_DYNAMODB_*
    //   macro: D_ENV_DYNAMODB_DETECTED; the AWS SDK version as _SDK_VERSION_ID,
    // _MAJOR, _MINOR, _PATCH and _STRING, 0 and "unknown" without an SDK;
    // D_ENV_DYNAMODB_API_VERSION, the service's API date; and
    // D_ENV_DYNAMODB_HAS_* capability flags, each defined in terms of the
    // macros above. Pre-defining D_ENV_DYNAMODB_DETECTED skips the whole layer.
    #ifndef D_ENV_DYNAMODB_DETECTED
        #define D_ENV_DYNAMODB_DETECTED 1

        // SDK version
        #if D_ENV_DDB_SDK_DETECTED
            #define D_ENV_DYNAMODB_SDK_VERSION_ID    D_ENV_DDB_SDK_VERSION_ID
            #define D_ENV_DYNAMODB_SDK_VERSION_MAJOR D_ENV_DDB_SDK_MAJOR
            #define D_ENV_DYNAMODB_SDK_VERSION_MINOR D_ENV_DDB_SDK_MINOR
            #define D_ENV_DYNAMODB_SDK_VERSION_PATCH D_ENV_DDB_SDK_PATCH
        #else
            #define D_ENV_DYNAMODB_SDK_VERSION_ID    0
            #define D_ENV_DYNAMODB_SDK_VERSION_MAJOR 0
            #define D_ENV_DYNAMODB_SDK_VERSION_MINOR 0
            #define D_ENV_DYNAMODB_SDK_VERSION_PATCH 0
        #endif
        #define D_ENV_DYNAMODB_SDK_VERSION_STRING "unknown"

        // service API version
        //   the DynamoDB low-level API is versioned by date and has been
        // stable at this value across the service's life.
        #ifndef D_ENV_DYNAMODB_API_VERSION
            #define D_ENV_DYNAMODB_API_VERSION "2012-08-10"
        #endif  // D_ENV_DYNAMODB_API_VERSION

        // request / query feature flags
        #define D_ENV_DYNAMODB_HAS_CONDITIONAL_WRITES                          \
            D_ENV_DDB_HAS_CONDITIONAL_WRITES
        #define D_ENV_DYNAMODB_HAS_TRANSACTIONS                                \
            D_ENV_DDB_HAS_TRANSACTIONS
        #define D_ENV_DYNAMODB_HAS_PARTIQL                                     \
            D_ENV_DDB_HAS_PARTIQL
        #define D_ENV_DYNAMODB_HAS_STRONG_CONSISTENCY                          \
            D_ENV_DDB_HAS_STRONG_CONSISTENCY

        // indexing
        #define D_ENV_DYNAMODB_HAS_GSI D_ENV_DDB_HAS_GSI
        #define D_ENV_DYNAMODB_HAS_LSI D_ENV_DDB_HAS_LSI

        // streams
        #define D_ENV_DYNAMODB_HAS_STREAMS D_ENV_DDB_HAS_STREAMS

        // capacity
        #define D_ENV_DYNAMODB_HAS_ON_DEMAND                                   \
            D_ENV_DDB_HAS_ON_DEMAND_CAPACITY

        // lifecycle / resilience
        #define D_ENV_DYNAMODB_HAS_TTL           D_ENV_DDB_HAS_TTL
        #define D_ENV_DYNAMODB_HAS_BACKUP        D_ENV_DDB_HAS_ON_DEMAND_BACKUP
        #define D_ENV_DYNAMODB_HAS_PITR          D_ENV_DDB_HAS_PITR
        #define D_ENV_DYNAMODB_HAS_GLOBAL_TABLES D_ENV_DDB_HAS_GLOBAL_TABLES

        // caching
        #define D_ENV_DYNAMODB_HAS_DAX D_ENV_DDB_HAS_DAX

        // security
        #define D_ENV_DYNAMODB_HAS_ENCRYPTION                                  \
            D_ENV_DDB_HAS_ENCRYPTION_AT_REST

        // resource tagging
        //   tagging is a control-plane capability of the managed service;
        // expose it on cloud targets.
        #define D_ENV_DYNAMODB_HAS_TAGGING D_ENV_DDB_IS_CLOUD

    #endif  // D_ENV_DYNAMODB_DETECTED (compatibility layer)


#else  // !D_ENV_DDB_DETECTED

    #ifndef D_ENV_DYNAMODB_DETECTED
        #define D_ENV_DYNAMODB_DETECTED 0
    #endif  // D_ENV_DYNAMODB_DETECTED

#endif  // D_ENV_DDB_DETECTED


#endif  // DJINTERP_ENV_DB_DYNAMODB_ENV_DYNAMODB_H
