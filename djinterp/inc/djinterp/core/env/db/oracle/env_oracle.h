/******************************************************************************
* djinterp [db]                                                   env_oracle.h
*
* djinterp Oracle Database environmental detection header:
* This header provides comprehensive compile-time detection of Oracle
* Database environments, capabilities, and version-gated features, including:
*   - version decomposition with pre-18c (four-part) and post-18c
*     (year-based) numbering awareness
*   - OCI (Oracle Call Interface) client library detection and API gating
*   - edition detection (Express, Standard Edition 2, Enterprise)
*   - Enterprise Edition option detection (Partitioning, RAC, In-Memory,
*     Advanced Security, Advanced Compression, Data Guard, etc.)
*   - multitenant architecture (CDB/PDB) detection
*   - PL/SQL feature detection
*   - SQL feature detection (analytic functions, MODEL clause,
*     MATCH_RECOGNIZE, lateral inline views, row pattern matching,
*     SQL/JSON, SQL domains, boolean type, schema-level IF NOT EXISTS)
*   - JSON support (JSON data type, JSON duality views, SODA, JSON
*     relational duality, JSON_TABLE, JSON_VALUE, JSON_SERIALIZE)
*   - XML DB and Oracle Text detection
*   - index type detection (B-tree, bitmap, function-based, domain,
*     partitioned, JSON search index)
*   - partitioning features (range, list, hash, composite, interval,
*     reference, auto-list, read-only)
*   - replication and HA (Data Guard, Active Data Guard, GoldenGate,
*     Real Application Clusters, Application Continuity)
*   - flashback features (query, table, database, data archive)
*   - security features (TDE, VPD, Database Vault, Label Security,
*     unified audit, privilege analysis, Data Redaction)
*   - In-Memory Column Store and dual-format storage
*   - Autonomous Database feature detection
*   - spatial and graph features
*   - advanced queuing and transactional event queues
*   - sharding (Oracle Sharding) detection
*
*   Oracle Database is a commercial RDBMS with the richest feature set in
* the industry. Feature availability depends on three axes:
*   1. Server version (detected from OCI client headers)
*   2. Edition (Express/Standard/Enterprise) - a runtime property
*   3. Separately-licensed options within Enterprise Edition
* Because edition and options are runtime properties (not compile-time
* defines), this header provides overridable macros for each. The default
* assumption is Enterprise Edition with no options, which can be overridden
* by pre-defining D_ENV_ORA_EDITION_* or D_ENV_ORA_OPTION_* macros.
*
*   VERSION ENCODING:
*   This header uses MAJOR*10000 + MINOR*100 + UPDATE for version encoding,
* consistent with the project convention. Oracle OCI headers provide
* OCI_MAJOR_VERSION and OCI_MINOR_VERSION.
*   Pre-18c releases used four-part numbering (12.2.0.1) where the first
* two parts determine features; post-18c uses year-based major versions
* (18, 19, 21, 23) with minor releases for patches.
*
*   NAMING CONVENTION:
*   D_ENV_ORA_[CATEGORY]_[FEATURE]  - 1 if available, 0 otherwise
*   D_ENV_ORA_VERSION_[COMPONENT]   - version number components
*   D_ENV_ORA_HAS_[CAPABILITY]      - capability flag (1/0)
*   D_ENV_ORA_OPTION_[NAME]         - Enterprise option flag (1/0)
*
* 
* path:      /inc/djinterp/core/env/db/oracle/env_oracle.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_ORACLE_
#define DJINTERP_ENVIRONMENT_ORACLE_ 1

#include "../../../../config/core/env/db/oracle/env_oracle_config.h"
#include "../env_db.h"


// =============================================================================
// 0.   VENDOR HEADER INCLUSION
// =============================================================================
//   Driven by D_CFG_ENV_USING_ORACLE from env_config.h. When enabled, this
// section includes <oci.h> (or the override configured via
// D_CFG_ENV_ORACLE_C_PATH) and, in C++ builds, the optional OCCI header.
// Detection below is gated on D_ENV_ORACLE_HEADER_INCLUDED so that no OCI
// symbols are referenced unless the header is actually in scope.

// --- C client header (OCI) ---
#if (D_CFG_ENV_USING_ORACLE == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_ORACLE_C_PATH)
            #include D_CFG_ENV_ORACLE_C_PATH
            #define D_ENV_ORACLE_HEADER_INCLUDED 1
        #elif __has_include(<oracle/oci.h>)
            #include <oracle/oci.h>
            #define D_ENV_ORACLE_HEADER_INCLUDED 1
        #elif __has_include(<oci.h>)
            #include <oci.h>
            #define D_ENV_ORACLE_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_ORACLE=1 but no oci.h header was "       \
                   "found. Install the Oracle Instant Client SDK, or "       \
                   "define D_CFG_ENV_ORACLE_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_ORACLE_C_PATH
        #define D_ENV_ORACLE_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_ORACLE_CLIENT_C
        #define D_ENV_DB_HAS_ORACLE_CLIENT_C 1
    #endif

#else
    #define D_ENV_ORACLE_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_ORACLE_CLIENT_C
        #define D_ENV_DB_HAS_ORACLE_CLIENT_C 0
    #endif
#endif  // D_CFG_ENV_USING_ORACLE


// --- C++ client header (OCCI, optional, C++ builds only) ---
#if ( (D_CFG_ENV_USING_ORACLE == 1) && defined(__cplusplus) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_ORACLE_CPP_PATH)
            #include D_CFG_ENV_ORACLE_CPP_PATH
            #define D_ENV_ORACLE_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_ORACLE_CLIENT_CPP
                #define D_ENV_DB_HAS_ORACLE_CLIENT_CPP 1
            #endif
        #else
            #define D_ENV_ORACLE_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_ORACLE_CLIENT_CPP
                #define D_ENV_DB_HAS_ORACLE_CLIENT_CPP 0
            #endif
        #endif
    #else
        #define D_ENV_ORACLE_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_ORACLE_CLIENT_CPP
            #define D_ENV_DB_HAS_ORACLE_CLIENT_CPP 0
        #endif
    #endif

#else
    #define D_ENV_ORACLE_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_ORACLE_CLIENT_CPP
        #define D_ENV_DB_HAS_ORACLE_CLIENT_CPP 0
    #endif
#endif  // D_CFG_ENV_USING_ORACLE && __cplusplus


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================

//   All D_CFG_* macros for this module live in env_oracle_config.h,
// pulled in at the top of this file.


// =============================================================================
// II.  VERSION ENCODING
// =============================================================================
//   Encoded as MAJOR*10000 + MINOR*100 + UPDATE.
//   Oracle OCI headers provide OCI_MAJOR_VERSION and OCI_MINOR_VERSION.
//   E.g. Oracle 19c Release 19.21 = 190021.
//   For feature gating, only MAJOR and MINOR typically matter.

// D_ENV_ORA_ENCODE_VERSION
//   macro: encodes a (major, minor, update) triple.
#define D_ENV_ORA_ENCODE_VERSION(major, minor, update) \
    ((major) * 10000 + (minor) * 100 + (update))

// D_ENV_ORA_DECODE_MAJOR
//   macro: extracts the major version.
#define D_ENV_ORA_DECODE_MAJOR(ver) \
    ((ver) / 10000)

// D_ENV_ORA_DECODE_MINOR
//   macro: extracts the minor version.
#define D_ENV_ORA_DECODE_MINOR(ver) \
    (((ver) / 100) % 100)

// D_ENV_ORA_DECODE_UPDATE
//   macro: extracts the update/patch level.
#define D_ENV_ORA_DECODE_UPDATE(ver) \
    ((ver) % 100)


// =============================================================================
// III. VERSION DETECTION
// =============================================================================

// version ID constants for feature-significant releases
// pre-18c (traditional numbering: major.minor becomes the feature gate)
#define D_ENV_ORA_VERSION_10_2        100200  // 10g R2
#define D_ENV_ORA_VERSION_11_1        110100  // 11g R1
#define D_ENV_ORA_VERSION_11_2        110200  // 11g R2
#define D_ENV_ORA_VERSION_12_1        120100  // 12c R1
#define D_ENV_ORA_VERSION_12_2        120200  // 12c R2

// post-18c (year-based: major is the year, minor is the first release)
#define D_ENV_ORA_VERSION_18          180000  // 18c
#define D_ENV_ORA_VERSION_19          190000  // 19c (LTS)
#define D_ENV_ORA_VERSION_21          210000  // 21c
#define D_ENV_ORA_VERSION_23          230000  // 23ai

// convenience: at-least checks by release name
// D_ENV_ORA_AT_LEAST
//   macro: shorthand for >= a major release.
#define D_ENV_ORA_AT_LEAST(major) \
    (D_ENV_ORA_VERSION_ID >= D_ENV_ORA_ENCODE_VERSION(major, 0, 0))

#if (D_CFG_ENV_ORA_CUSTOM == 0)

    // automatic detection via OCI headers
    // requires oci.h to be in scope; if D_CFG_ENV_USING_ORACLE was not
    // enabled the sentinel is 0 and we skip cleanly (no reference to
    // OCI_MAJOR_VERSION).
    #if ( D_ENV_ORACLE_HEADER_INCLUDED  &&  \
          defined(OCI_MAJOR_VERSION) )
        #define D_ENV_ORA_DETECTED             1

        #ifdef OCI_MINOR_VERSION
            #define D_ENV_ORA_VERSION_ID        \
                D_ENV_ORA_ENCODE_VERSION(OCI_MAJOR_VERSION, \
                                          OCI_MINOR_VERSION, 0)
            #define D_ENV_ORA_VERSION_MINOR    OCI_MINOR_VERSION
        #else
            #define D_ENV_ORA_VERSION_ID        \
                D_ENV_ORA_ENCODE_VERSION(OCI_MAJOR_VERSION, 0, 0)
            #define D_ENV_ORA_VERSION_MINOR    0
        #endif

        #define D_ENV_ORA_VERSION_MAJOR        OCI_MAJOR_VERSION

        // pre-18c used two-part feature versions (e.g. 12.2)
        #if (OCI_MAJOR_VERSION < 18)
            #define D_ENV_ORA_IS_LEGACY_VERSIONING 1
        #else
            #define D_ENV_ORA_IS_LEGACY_VERSIONING 0
        #endif

        #ifdef OCI_CLIENTVERSION
            #define D_ENV_ORA_VERSION_STRING    "OCI"
        #else
            #define D_ENV_ORA_VERSION_STRING    "unknown"
        #endif

    // fallback detection via other Oracle defines
    #elif defined(ORACLE_VERSION)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           0
        #define D_ENV_ORA_VERSION_MAJOR        0
        #define D_ENV_ORA_VERSION_MINOR        0
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 0
        #define D_ENV_ORA_VERSION_STRING       "unknown"

    #else
        #define D_ENV_ORA_DETECTED             0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_ORA_DETECTED_VERSION
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_DETECTED_VERSION
        #define D_ENV_ORA_VERSION_MAJOR        \
            D_ENV_ORA_DECODE_MAJOR(D_ENV_ORA_DETECTED_VERSION)
        #define D_ENV_ORA_VERSION_MINOR        \
            D_ENV_ORA_DECODE_MINOR(D_ENV_ORA_DETECTED_VERSION)
        #define D_ENV_ORA_IS_LEGACY_VERSIONING \
            (D_ENV_ORA_DECODE_MAJOR(D_ENV_ORA_DETECTED_VERSION) < 18)
        #define D_ENV_ORA_VERSION_STRING       "manual"

    #elif defined(D_ENV_ORA_DETECTED_23)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_23
        #define D_ENV_ORA_VERSION_MAJOR        23
        #define D_ENV_ORA_VERSION_MINOR        0
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 0
        #define D_ENV_ORA_VERSION_STRING       "23ai"

    #elif defined(D_ENV_ORA_DETECTED_21)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_21
        #define D_ENV_ORA_VERSION_MAJOR        21
        #define D_ENV_ORA_VERSION_MINOR        0
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 0
        #define D_ENV_ORA_VERSION_STRING       "21c"

    #elif defined(D_ENV_ORA_DETECTED_19)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_19
        #define D_ENV_ORA_VERSION_MAJOR        19
        #define D_ENV_ORA_VERSION_MINOR        0
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 0
        #define D_ENV_ORA_VERSION_STRING       "19c"

    #elif defined(D_ENV_ORA_DETECTED_18)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_18
        #define D_ENV_ORA_VERSION_MAJOR        18
        #define D_ENV_ORA_VERSION_MINOR        0
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 0
        #define D_ENV_ORA_VERSION_STRING       "18c"

    #elif defined(D_ENV_ORA_DETECTED_12_2)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_12_2
        #define D_ENV_ORA_VERSION_MAJOR        12
        #define D_ENV_ORA_VERSION_MINOR        2
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 1
        #define D_ENV_ORA_VERSION_STRING       "12.2"

    #elif defined(D_ENV_ORA_DETECTED_12_1)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_12_1
        #define D_ENV_ORA_VERSION_MAJOR        12
        #define D_ENV_ORA_VERSION_MINOR        1
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 1
        #define D_ENV_ORA_VERSION_STRING       "12.1"

    #elif defined(D_ENV_ORA_DETECTED_11_2)
        #define D_ENV_ORA_DETECTED             1
        #define D_ENV_ORA_VERSION_ID           D_ENV_ORA_VERSION_11_2
        #define D_ENV_ORA_VERSION_MAJOR        11
        #define D_ENV_ORA_VERSION_MINOR        2
        #define D_ENV_ORA_IS_LEGACY_VERSIONING 1
        #define D_ENV_ORA_VERSION_STRING       "11.2"

    #else
        #define D_ENV_ORA_DETECTED             0
    #endif

#endif  // D_CFG_ENV_ORA_CUSTOM


// =============================================================================
// IV.  VERSION COMPARISON MACROS
// =============================================================================

#if D_ENV_ORA_DETECTED

    #define D_ENV_ORA_VERSION_AT_LEAST(major, minor, update) \
        (D_ENV_ORA_VERSION_ID >= \
            D_ENV_ORA_ENCODE_VERSION(major, minor, update))

    #define D_ENV_ORA_VERSION_BELOW(major, minor, update) \
        (D_ENV_ORA_VERSION_ID < \
            D_ENV_ORA_ENCODE_VERSION(major, minor, update))

    #define D_ENV_ORA_VERSION_IN_RANGE(min_maj, min_min, min_upd,        \
                                        max_maj, max_min, max_upd)        \
        ( D_ENV_ORA_VERSION_AT_LEAST(min_maj, min_min, min_upd) &&       \
          D_ENV_ORA_VERSION_BELOW(max_maj, max_min, max_upd) )

    // series macros (using D_ENV_ORA_AT_LEAST for post-18c)
    #define D_ENV_ORA_IS_11G \
        D_ENV_ORA_VERSION_IN_RANGE(11, 0, 0, 12, 0, 0)
    #define D_ENV_ORA_IS_12C \
        D_ENV_ORA_VERSION_IN_RANGE(12, 0, 0, 18, 0, 0)
    #define D_ENV_ORA_IS_12_1 \
        D_ENV_ORA_VERSION_IN_RANGE(12, 1, 0, 12, 2, 0)
    #define D_ENV_ORA_IS_12_2 \
        D_ENV_ORA_VERSION_IN_RANGE(12, 2, 0, 18, 0, 0)
    #define D_ENV_ORA_IS_18C \
        D_ENV_ORA_VERSION_IN_RANGE(18, 0, 0, 19, 0, 0)
    #define D_ENV_ORA_IS_19C \
        D_ENV_ORA_VERSION_IN_RANGE(19, 0, 0, 21, 0, 0)
    #define D_ENV_ORA_IS_21C \
        D_ENV_ORA_VERSION_IN_RANGE(21, 0, 0, 23, 0, 0)
    #define D_ENV_ORA_IS_23AI \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_IS_LTS
    //   macro: evaluates to 1 if this is a Long-Term Support release.
    // Oracle LTS releases: 19c (premier support until 2024, extended
    // until 2027), 23ai (next LTS).
    #define D_ENV_ORA_IS_LTS \
        ( D_ENV_ORA_IS_19C || D_ENV_ORA_IS_23AI )


// =============================================================================
// V.   EDITION AND OPTION DETECTION
// =============================================================================
//   Oracle editions (Express, Standard Edition 2, Enterprise) and
// separately-licensed Enterprise options are runtime properties determined
// by the server license, not compile-time defines. These macros default
// to the most permissive assumption (Enterprise, all options disabled)
// and can be overridden by the user.

    // edition flags
    #ifndef D_ENV_ORA_EDITION_XE
        #define D_ENV_ORA_EDITION_XE 0
    #endif
    #ifndef D_ENV_ORA_EDITION_SE2
        #define D_ENV_ORA_EDITION_SE2 0
    #endif
    #ifndef D_ENV_ORA_EDITION_EE
        #if ( (!D_ENV_ORA_EDITION_XE) && (!D_ENV_ORA_EDITION_SE2) )
            #define D_ENV_ORA_EDITION_EE 1
        #else
            #define D_ENV_ORA_EDITION_EE 0
        #endif
    #endif

    // Enterprise Edition separately-licensed options
    // All default to 0 (not licensed). Override to 1 if the option is
    // available in your deployment.

    // D_ENV_ORA_OPTION_PARTITIONING
    //   option: Partitioning option (range, list, hash, composite,
    // interval, reference, auto-list partitioning). EE only.
    #ifndef D_ENV_ORA_OPTION_PARTITIONING
        #define D_ENV_ORA_OPTION_PARTITIONING 0
    #endif

    // D_ENV_ORA_OPTION_RAC
    //   option: Real Application Clusters (multi-instance on shared
    // storage). EE only.
    #ifndef D_ENV_ORA_OPTION_RAC
        #define D_ENV_ORA_OPTION_RAC 0
    #endif

    // D_ENV_ORA_OPTION_RAC_ONE_NODE
    //   option: RAC One Node (single-instance with HA failover). EE.
    #ifndef D_ENV_ORA_OPTION_RAC_ONE_NODE
        #define D_ENV_ORA_OPTION_RAC_ONE_NODE 0
    #endif

    // D_ENV_ORA_OPTION_IN_MEMORY
    //   option: Database In-Memory (columnar in-memory store). EE.
    // Introduced in Oracle 12.1.0.2.
    #ifndef D_ENV_ORA_OPTION_IN_MEMORY
        #define D_ENV_ORA_OPTION_IN_MEMORY 0
    #endif

    // D_ENV_ORA_OPTION_ADVANCED_SECURITY
    //   option: Advanced Security (TDE, network encryption, strong
    // authentication). EE only.
    #ifndef D_ENV_ORA_OPTION_ADVANCED_SECURITY
        #define D_ENV_ORA_OPTION_ADVANCED_SECURITY 0
    #endif

    // D_ENV_ORA_OPTION_ADVANCED_COMPRESSION
    //   option: Advanced Compression (table, index, network, RMAN
    // compression). EE only.
    #ifndef D_ENV_ORA_OPTION_ADVANCED_COMPRESSION
        #define D_ENV_ORA_OPTION_ADVANCED_COMPRESSION 0
    #endif

    // D_ENV_ORA_OPTION_DATA_GUARD
    //   option: Active Data Guard (read-only standby queries, far sync).
    // Note: basic Data Guard (standby without queries) is included in EE.
    #ifndef D_ENV_ORA_OPTION_DATA_GUARD
        #define D_ENV_ORA_OPTION_DATA_GUARD 0
    #endif

    // D_ENV_ORA_OPTION_MULTITENANT
    //   option: Multitenant option (multiple PDBs in one CDB). Without
    // this option, only one user-created PDB is allowed (free in 12.2+
    // for single PDB; 3 PDBs free in 19c+).
    #ifndef D_ENV_ORA_OPTION_MULTITENANT
        #define D_ENV_ORA_OPTION_MULTITENANT 0
    #endif

    // D_ENV_ORA_OPTION_LABEL_SECURITY
    //   option: Label Security (mandatory access control with row-level
    // labels). EE only.
    #ifndef D_ENV_ORA_OPTION_LABEL_SECURITY
        #define D_ENV_ORA_OPTION_LABEL_SECURITY 0
    #endif

    // D_ENV_ORA_OPTION_DATABASE_VAULT
    //   option: Database Vault (privileged user access control, realm
    // protections). EE only.
    #ifndef D_ENV_ORA_OPTION_DATABASE_VAULT
        #define D_ENV_ORA_OPTION_DATABASE_VAULT 0
    #endif

    // D_ENV_ORA_OPTION_OLAP
    //   option: OLAP option (analytic workspace, cube organized
    // materialized views). EE only.
    #ifndef D_ENV_ORA_OPTION_OLAP
        #define D_ENV_ORA_OPTION_OLAP 0
    #endif

    // D_ENV_ORA_OPTION_SPATIAL
    //   option: Spatial and Graph option (full spatial, network data
    // model, RDF/property graph). Note: Locator (basic spatial) is free.
    #ifndef D_ENV_ORA_OPTION_SPATIAL
        #define D_ENV_ORA_OPTION_SPATIAL 0
    #endif

    // D_ENV_ORA_OPTION_SHARDING
    //   option: Oracle Sharding (native horizontal partitioning across
    // databases). EE only. Introduced in 12.2.
    #ifndef D_ENV_ORA_OPTION_SHARDING
        #define D_ENV_ORA_OPTION_SHARDING 0
    #endif


// =============================================================================
// VI.  OCI (ORACLE CALL INTERFACE) CLIENT DETECTION
// =============================================================================

    // D_ENV_ORA_HAS_OCI
    //   feature: detect if OCI client library headers are available.
    #define D_ENV_ORA_HAS_OCI D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_STATEMENT_CACHE
    //   feature: OCI statement caching. Available since 10g.
    #define D_ENV_ORA_HAS_OCI_STATEMENT_CACHE D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_CONNECTION_POOL
    //   feature: OCI connection pooling (OCIConnectionPoolCreate).
    #define D_ENV_ORA_HAS_OCI_CONNECTION_POOL D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_SESSION_POOL
    //   feature: OCI session pooling (OCISessionPoolCreate).
    #define D_ENV_ORA_HAS_OCI_SESSION_POOL D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_CONTINUOUS_QUERY
    //   feature: Continuous Query Notification (CQN) via OCI
    // (OCISubscription*). Available since 10gR2.
    #define D_ENV_ORA_HAS_OCI_CONTINUOUS_QUERY D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_ARRAY_DML
    //   feature: array DML (batch INSERT/UPDATE/DELETE via array
    // binding). Core OCI feature.
    #define D_ENV_ORA_HAS_OCI_ARRAY_DML D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_LOB
    //   feature: LOB (Large Object) operations via OCI. Core feature.
    #define D_ENV_ORA_HAS_OCI_LOB D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_OBJECT
    //   feature: OCI Object-Relational functions (OCIObject*).
    #define D_ENV_ORA_HAS_OCI_OBJECT D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_OCI_JSON
    //   feature: OCI native JSON support (OSON binary format in OCI).
    // Introduced in Oracle 21c.
    #define D_ENV_ORA_HAS_OCI_JSON \
        D_ENV_ORA_AT_LEAST(21)

    // D_ENV_ORA_HAS_OCI_SODA
    //   feature: SODA (Simple Oracle Document Access) for schemaless
    // document access via OCI. Introduced in Oracle 18c.
    #define D_ENV_ORA_HAS_OCI_SODA \
        D_ENV_ORA_AT_LEAST(18)

    // D_ENV_ORA_HAS_OCI_helperICIT_RESULTS
    //   feature: implicit result sets from PL/SQL
    // (DBMS_SQL.RETURN_RESULT). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_OCI_helperICIT_RESULTS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_OCI_CLIENT_RESULT_CACHE
    //   feature: OCI client-side result cache. Introduced in 11g.
    #define D_ENV_ORA_HAS_OCI_CLIENT_RESULT_CACHE \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)


// =============================================================================
// VII. MULTITENANT ARCHITECTURE
// =============================================================================
//   The CDB/PDB (Container Database / Pluggable Database) architecture
// was introduced in Oracle 12.1 and became mandatory in 21c.

    // D_ENV_ORA_HAS_MULTITENANT
    //   feature: multitenant architecture is available. Introduced in
    // 12.1. In 12.1-12.2, one PDB is free without the option. In 19c+
    // three PDBs are free.
    #define D_ENV_ORA_HAS_MULTITENANT \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_MULTITENANT_IS_MANDATORY
    //   status: 1 if non-CDB architecture is desupported (21c+). All
    // databases must be CDBs from 21c onward.
    #define D_ENV_ORA_MULTITENANT_IS_MANDATORY \
        D_ENV_ORA_AT_LEAST(21)

    // D_ENV_ORA_HAS_PDB_LOCKDOWN
    //   feature: PDB lockdown profiles for restricting operations within
    // a pluggable database. Introduced in 12.2.
    #define D_ENV_ORA_HAS_PDB_LOCKDOWN \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_PDB_RELOCATE
    //   feature: PDB live relocation (moving a PDB between CDBs with
    // minimal downtime). Introduced in 12.2.
    #define D_ENV_ORA_HAS_PDB_RELOCATE \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_APPLICATION_CONTAINERS
    //   feature: application containers (application root + application
    // PDBs sharing common objects). Introduced in 12.2.
    #define D_ENV_ORA_HAS_APPLICATION_CONTAINERS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_PDB_SNAPSHOT_CAROUSEL
    //   feature: PDB snapshot carousel (automatic PDB snapshots on a
    // rotating schedule). Introduced in 18c.
    #define D_ENV_ORA_HAS_PDB_SNAPSHOT_CAROUSEL \
        D_ENV_ORA_AT_LEAST(18)


// =============================================================================
// VIII. SQL FEATURE DETECTION
// =============================================================================

    // D_ENV_ORA_HAS_ANALYTIC_FUNCTIONS
    //   feature: SQL analytic (window) functions (ROW_NUMBER, RANK,
    // NTILE, LAG, LEAD, etc.). Present since Oracle 8i.
    #define D_ENV_ORA_HAS_ANALYTIC_FUNCTIONS D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_MODEL_CLAUSE
    //   feature: MODEL clause (spreadsheet-like inter-row calculations).
    // Introduced in Oracle 10g.
    #define D_ENV_ORA_HAS_MODEL_CLAUSE D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_RECURSIVE_WITH
    //   feature: recursive WITH (recursive CTEs / recursive
    // subquery factoring). Introduced in Oracle 11gR2.
    #define D_ENV_ORA_HAS_RECURSIVE_WITH \
        D_ENV_ORA_VERSION_AT_LEAST(11, 2, 0)

    // D_ENV_ORA_HAS_LATERAL_INLINE_VIEW
    //   feature: LATERAL inline views (LATERAL keyword for correlated
    // derived tables). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_LATERAL_INLINE_VIEW \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_MATCH_RECOGNIZE
    //   feature: MATCH_RECOGNIZE (row pattern matching with regular
    // expressions over row sequences). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_MATCH_RECOGNIZE \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_IDENTITY_COLUMNS
    //   feature: GENERATED { ALWAYS | BY DEFAULT } AS IDENTITY.
    // Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_IDENTITY_COLUMNS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_ROW_LIMITING
    //   feature: FETCH FIRST / OFFSET row limiting clause (SQL:2008
    // standard alternative to ROWNUM). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_ROW_LIMITING \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_INVISIBLE_COLUMNS
    //   feature: invisible columns. Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_INVISIBLE_COLUMNS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_DEFAULT_ON_NULL
    //   feature: DEFAULT ON NULL column syntax (use default value when
    // NULL is inserted). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_DEFAULT_ON_NULL \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_APPROX_FUNCTIONS
    //   feature: approximate query processing (APPROX_COUNT_DISTINCT,
    // APPROX_PERCENTILE, etc.). Introduced in 12.1, expanded in 18c+.
    #define D_ENV_ORA_HAS_APPROX_FUNCTIONS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_LISTAGG_OVERFLOW
    //   feature: LISTAGG with ON OVERFLOW TRUNCATE/ERROR clause.
    // Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_LISTAGG_OVERFLOW \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_LISTAGG_DISTINCT
    //   feature: LISTAGG DISTINCT (deduplication within aggregation).
    // Introduced in Oracle 19c.
    #define D_ENV_ORA_HAS_LISTAGG_DISTINCT \
        D_ENV_ORA_AT_LEAST(19)

    // D_ENV_ORA_HAS_POLYMORPHIC_TABLE_FUNCTIONS
    //   feature: polymorphic table functions (PTFs). Introduced in 18c.
    #define D_ENV_ORA_HAS_POLYMORPHIC_TABLE_FUNCTIONS \
        D_ENV_ORA_AT_LEAST(18)

    // D_ENV_ORA_HAS_PRIVATE_TEMP_TABLES
    //   feature: private temporary tables (session/transaction scoped,
    // no DBA privileges needed). Introduced in 18c.
    #define D_ENV_ORA_HAS_PRIVATE_TEMP_TABLES \
        D_ENV_ORA_AT_LEAST(18)

    // D_ENV_ORA_HAS_MERGE_DELETE
    //   feature: DELETE clause in MERGE statement. Introduced in 10g.
    #define D_ENV_ORA_HAS_MERGE_DELETE D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_SQL_MACRO
    //   feature: SQL macros (scalar and table macros that inline SQL
    // expressions). Introduced in Oracle 21c.
    #define D_ENV_ORA_HAS_SQL_MACRO \
        D_ENV_ORA_AT_LEAST(21)

    // D_ENV_ORA_HAS_BOOLEAN_TYPE
    //   feature: native BOOLEAN data type in SQL (not just PL/SQL).
    // Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_BOOLEAN_TYPE \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_SQL_DOMAINS
    //   feature: SQL domains (named reusable column definitions with
    // constraints, display, ordering). Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_SQL_DOMAINS \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_IF_NOT_EXISTS
    //   feature: IF [NOT] EXISTS for DDL (CREATE, DROP, ALTER).
    // Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_IF_NOT_EXISTS \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_GROUP_BY_COLUMN_ALIAS
    //   feature: GROUP BY and HAVING can reference SELECT-list aliases.
    // Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_GROUP_BY_COLUMN_ALIAS \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_FROM_CLAUSE_OPTIONAL
    //   feature: SELECT without FROM clause (SELECT 1+1).
    // Introduced in Oracle 23ai (previously required FROM dual).
    #define D_ENV_ORA_HAS_FROM_CLAUSE_OPTIONAL \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_TABLE_VALUE_CONSTRUCTORS
    //   feature: VALUES clause as a row source.
    // Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_TABLE_VALUE_CONSTRUCTORS \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_RETURNING_INTO_OLD_NEW
    //   feature: UPDATE/DELETE RETURNING OLD/NEW values.
    // Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_RETURNING_INTO_OLD_NEW \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_ANNOTATIONS
    //   feature: schema object annotations (metadata key-value pairs on
    // tables, columns, views). Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_ANNOTATIONS \
        D_ENV_ORA_AT_LEAST(23)


// =============================================================================
// IX.  JSON SUPPORT
// =============================================================================

    // D_ENV_ORA_HAS_JSON_FUNCTIONS
    //   feature: SQL/JSON functions (JSON_VALUE, JSON_QUERY,
    // JSON_TABLE, JSON_EXISTS, IS JSON). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_JSON_FUNCTIONS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_JSON_DOT_NOTATION
    //   feature: simplified JSON dot-notation access (column.key).
    // Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_JSON_DOT_NOTATION \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_JSON_DATA_TYPE
    //   feature: native JSON data type (OSON binary format, distinct
    // from VARCHAR2/CLOB/BLOB storage). Introduced in Oracle 21c.
    #define D_ENV_ORA_HAS_JSON_DATA_TYPE \
        D_ENV_ORA_AT_LEAST(21)

    // D_ENV_ORA_HAS_JSON_DUALITY_VIEWS
    //   feature: JSON Relational Duality Views (RDBMS tables with
    // automatic JSON document API). Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_JSON_DUALITY_VIEWS \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_JSON_SCHEMA_VALIDATION
    //   feature: JSON Schema validation (IS JSON VALIDATE clause).
    // Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_JSON_SCHEMA_VALIDATION \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_JSON_COLLECTION_TABLE
    //   feature: JSON Collection Tables (document-model tables
    // with automatic _id generation). Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_JSON_COLLECTION_TABLE \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_SODA
    //   feature: SODA (Simple Oracle Document Access) for schemaless
    // document storage. Introduced in Oracle 18c.
    #define D_ENV_ORA_HAS_SODA \
        D_ENV_ORA_AT_LEAST(18)

    // D_ENV_ORA_HAS_JSON_SEARCH_INDEX
    //   feature: JSON search index (full-text and ad-hoc query index
    // over JSON documents). Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_JSON_SEARCH_INDEX \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_JSON_TRANSFORM
    //   feature: JSON_TRANSFORM function (in-place JSON modification).
    // Introduced in Oracle 21c.
    #define D_ENV_ORA_HAS_JSON_TRANSFORM \
        D_ENV_ORA_AT_LEAST(21)

    // D_ENV_ORA_HAS_JSON_SERIALIZE
    //   feature: JSON_SERIALIZE function (formatting control).
    // Introduced in Oracle 19c.
    #define D_ENV_ORA_HAS_JSON_SERIALIZE \
        D_ENV_ORA_AT_LEAST(19)


// =============================================================================
// X.   INDEX TYPES
// =============================================================================

    // D_ENV_ORA_HAS_INDEX_BTREE
    //   feature: B-tree indexes (always present).
    #define D_ENV_ORA_HAS_INDEX_BTREE D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_INDEX_BITMAP
    //   feature: bitmap indexes (EE only for DML tables; read-only
    // bitmap indexes are available in SE2).
    #define D_ENV_ORA_HAS_INDEX_BITMAP D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_INDEX_FUNCTION_BASED
    //   feature: function-based indexes (indexes on expressions).
    #define D_ENV_ORA_HAS_INDEX_FUNCTION_BASED D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_INDEX_REVERSE_KEY
    //   feature: reverse-key indexes (for reducing contention on
    // sequence-generated keys in RAC).
    #define D_ENV_ORA_HAS_INDEX_REVERSE_KEY D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_INDEX_DOMAIN
    //   feature: domain indexes (extensible indexing framework for Oracle
    // Text, Spatial, etc.). Present since Oracle 8i.
    #define D_ENV_ORA_HAS_INDEX_DOMAIN D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_INDEX_INVISIBLE
    //   feature: invisible indexes (optimizer ignores but DML maintains).
    // Introduced in Oracle 11g.
    #define D_ENV_ORA_HAS_INDEX_INVISIBLE \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)

    // D_ENV_ORA_HAS_INDEX_PARTIAL
    //   feature: partial indexes on partitioned tables (index only some
    // partitions). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_INDEX_PARTIAL \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_INDEX_COMPRESSION
    //   feature: advanced index compression (prefix and advanced key
    // compression). Introduced in 12.1; advanced compression in 12.2.
    #define D_ENV_ORA_HAS_INDEX_COMPRESSION \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_INDEX_JSON_SEARCH
    //   feature: JSON search index. Alias from IX.
    #define D_ENV_ORA_HAS_INDEX_JSON_SEARCH \
        D_ENV_ORA_HAS_JSON_SEARCH_INDEX


// =============================================================================
// XI.  PARTITIONING FEATURES
// =============================================================================
//   Partitioning requires the separately-licensed Partitioning option
// for Enterprise Edition. The features below are version-gated only;
// actual availability depends on D_ENV_ORA_OPTION_PARTITIONING.

    // D_ENV_ORA_HAS_RANGE_PARTITIONING
    //   feature: range partitioning. Available since Oracle 8.0.
    #define D_ENV_ORA_HAS_RANGE_PARTITIONING D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_LIST_PARTITIONING
    //   feature: list partitioning. Introduced in Oracle 9i.
    #define D_ENV_ORA_HAS_LIST_PARTITIONING D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_HASH_PARTITIONING
    //   feature: hash partitioning. Introduced in Oracle 8i.
    #define D_ENV_ORA_HAS_HASH_PARTITIONING D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_COMPOSITE_PARTITIONING
    //   feature: composite (sub-) partitioning (range-hash, range-list,
    // etc.). Extended in 11g to include all combinations.
    #define D_ENV_ORA_HAS_COMPOSITE_PARTITIONING D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_INTERVAL_PARTITIONING
    //   feature: interval partitioning (automatic partition creation as
    // data arrives). Introduced in Oracle 11g.
    #define D_ENV_ORA_HAS_INTERVAL_PARTITIONING \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)

    // D_ENV_ORA_HAS_REFERENCE_PARTITIONING
    //   feature: reference partitioning (child table inherits parent's
    // partitioning via FK). Introduced in Oracle 11g.
    #define D_ENV_ORA_HAS_REFERENCE_PARTITIONING \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)

    // D_ENV_ORA_HAS_AUTO_LIST_PARTITIONING
    //   feature: auto-list partitioning (automatic creation for new
    // list values). Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_AUTO_LIST_PARTITIONING \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_READ_ONLY_PARTITIONS
    //   feature: read-only partitions and subpartitions.
    // Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_READ_ONLY_PARTITIONS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_MULTI_COLUMN_LIST_PARTITION
    //   feature: multi-column list partitioning. Introduced in 12.2.
    #define D_ENV_ORA_HAS_MULTI_COLUMN_LIST_PARTITION \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)

    // D_ENV_ORA_HAS_ONLINE_PARTITION_OPERATIONS
    //   feature: online partition move/merge/split.
    // Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_ONLINE_PARTITION_OPERATIONS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)


// =============================================================================
// XII. REPLICATION AND HIGH AVAILABILITY
// =============================================================================

    // D_ENV_ORA_HAS_DATA_GUARD
    //   feature: basic Data Guard (physical/logical standby). Included
    // in Enterprise Edition without additional license.
    #define D_ENV_ORA_HAS_DATA_GUARD \
        D_ENV_ORA_EDITION_EE

    // D_ENV_ORA_HAS_ACTIVE_DATA_GUARD
    //   feature: Active Data Guard (read-only queries on physical standby,
    // far sync, DML redirect). Requires option license.
    #define D_ENV_ORA_HAS_ACTIVE_DATA_GUARD \
        ( D_ENV_ORA_EDITION_EE && D_ENV_ORA_OPTION_DATA_GUARD )

    // D_ENV_ORA_HAS_DG_BROKER
    //   feature: Data Guard Broker (automated DG configuration and
    // switchover/failover). Part of base Data Guard since 10g.
    #define D_ENV_ORA_HAS_DG_BROKER \
        D_ENV_ORA_EDITION_EE

    // D_ENV_ORA_HAS_RAC
    //   feature: Real Application Clusters. Requires option license.
    #define D_ENV_ORA_HAS_RAC \
        ( D_ENV_ORA_EDITION_EE && D_ENV_ORA_OPTION_RAC )

    // D_ENV_ORA_HAS_APPLICATION_CONTINUITY
    //   feature: Application Continuity (transparent replay of
    // in-flight transactions after failures). Introduced in 12.1.
    #define D_ENV_ORA_HAS_APPLICATION_CONTINUITY \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0) && D_ENV_ORA_EDITION_EE )

    // D_ENV_ORA_HAS_TRANSPARENT_APP_CONTINUITY
    //   feature: Transparent Application Continuity (no application
    // changes needed). Introduced in 18c.
    #define D_ENV_ORA_HAS_TRANSPARENT_APP_CONTINUITY \
        ( D_ENV_ORA_AT_LEAST(18) && D_ENV_ORA_EDITION_EE )

    // D_ENV_ORA_HAS_GOLDENGATE
    //   feature: GoldenGate integration (real-time data replication).
    // Separately licensed product; version detection not available via
    // OCI headers. Overridable.
    #ifndef D_ENV_ORA_HAS_GOLDENGATE
        #define D_ENV_ORA_HAS_GOLDENGATE 0
    #endif

    // D_ENV_ORA_HAS_SHARDING
    //   feature: Oracle Sharding. EE + option. Introduced in 12.2.
    #define D_ENV_ORA_HAS_SHARDING \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0) && \
          D_ENV_ORA_OPTION_SHARDING )

    // D_ENV_ORA_HAS_GLOBAL_DATA_SERVICES
    //   feature: Global Data Services (load balancing and service
    // management across replicated databases). Introduced in 12.1.
    #define D_ENV_ORA_HAS_GLOBAL_DATA_SERVICES \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)


// =============================================================================
// XIII. FLASHBACK FEATURES
// =============================================================================

    // D_ENV_ORA_HAS_FLASHBACK_QUERY
    //   feature: Flashback Query (AS OF TIMESTAMP/SCN). Introduced in
    // Oracle 9i. Available in all editions.
    #define D_ENV_ORA_HAS_FLASHBACK_QUERY D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_FLASHBACK_TABLE
    //   feature: FLASHBACK TABLE (restore a table to an earlier point).
    // Introduced in Oracle 10g.
    #define D_ENV_ORA_HAS_FLASHBACK_TABLE D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_FLASHBACK_DATABASE
    //   feature: FLASHBACK DATABASE (point-in-time database rewind).
    // EE only. Introduced in Oracle 10g.
    #define D_ENV_ORA_HAS_FLASHBACK_DATABASE \
        D_ENV_ORA_EDITION_EE

    // D_ENV_ORA_HAS_FLASHBACK_DATA_ARCHIVE
    //   feature: Flashback Data Archive (Total Recall - long-term
    // historical data retention). Introduced in 11g. Free (for basic
    // use) since 11.2.0.4; advanced features are EE.
    #define D_ENV_ORA_HAS_FLASHBACK_DATA_ARCHIVE \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)

    // D_ENV_ORA_HAS_FLASHBACK_VERSIONS_QUERY
    //   feature: VERSIONS BETWEEN clause (row-level change history).
    // Introduced in Oracle 10g.
    #define D_ENV_ORA_HAS_FLASHBACK_VERSIONS_QUERY D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_FLASHBACK_TRANSACTION
    //   feature: Flashback Transaction (compensating transaction to undo
    // a committed transaction). Introduced in 11gR2. EE only.
    #define D_ENV_ORA_HAS_FLASHBACK_TRANSACTION \
        ( D_ENV_ORA_VERSION_AT_LEAST(11, 2, 0) && D_ENV_ORA_EDITION_EE )


// =============================================================================
// XIV. SECURITY FEATURES
// =============================================================================

    // D_ENV_ORA_HAS_TDE
    //   feature: Transparent Data Encryption (column-level and tablespace-
    // level). Requires Advanced Security option. Introduced in 10gR2;
    // tablespace TDE in 11g.
    #define D_ENV_ORA_HAS_TDE \
        D_ENV_ORA_OPTION_ADVANCED_SECURITY

    // D_ENV_ORA_HAS_TDE_TABLESPACE
    //   feature: tablespace-level TDE. Introduced in 11g.
    #define D_ENV_ORA_HAS_TDE_TABLESPACE \
        ( D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0) && \
          D_ENV_ORA_OPTION_ADVANCED_SECURITY )

    // D_ENV_ORA_HAS_VPD
    //   feature: Virtual Private Database (fine-grained access control
    // via policy functions). EE only. Introduced in Oracle 8i.
    #define D_ENV_ORA_HAS_VPD \
        D_ENV_ORA_EDITION_EE

    // D_ENV_ORA_HAS_DATABASE_VAULT
    //   feature: Database Vault. Requires option license.
    #define D_ENV_ORA_HAS_DATABASE_VAULT \
        D_ENV_ORA_OPTION_DATABASE_VAULT

    // D_ENV_ORA_HAS_LABEL_SECURITY
    //   feature: Label Security. Requires option license.
    #define D_ENV_ORA_HAS_LABEL_SECURITY \
        D_ENV_ORA_OPTION_LABEL_SECURITY

    // D_ENV_ORA_HAS_UNIFIED_AUDIT
    //   feature: unified auditing (single audit trail replacing multiple
    // legacy audit trails). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_UNIFIED_AUDIT \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_PRIVILEGE_ANALYSIS
    //   feature: privilege analysis (identify used/unused privileges).
    // Introduced in Oracle 12.1. EE only. Free for all editions in 23ai.
    #define D_ENV_ORA_HAS_PRIVILEGE_ANALYSIS \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_DATA_REDACTION
    //   feature: Data Redaction (dynamic data masking on query output).
    // Introduced in Oracle 12.1. EE + Advanced Security.
    #define D_ENV_ORA_HAS_DATA_REDACTION \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0) && \
          D_ENV_ORA_OPTION_ADVANCED_SECURITY )

    // D_ENV_ORA_HAS_SQL_FIREWALL
    //   feature: SQL Firewall (allow-list based SQL injection
    // prevention). Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_SQL_FIREWALL \
        D_ENV_ORA_AT_LEAST(23)


// =============================================================================
// XV.  IN-MEMORY AND PERFORMANCE
// =============================================================================

    // D_ENV_ORA_HAS_IN_MEMORY
    //   feature: Database In-Memory Column Store (dual-format storage:
    // row-based for OLTP + columnar for analytics). Requires option.
    // Introduced in 12.1.0.2.
    #define D_ENV_ORA_HAS_IN_MEMORY \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0) && \
          D_ENV_ORA_OPTION_IN_MEMORY )

    // D_ENV_ORA_HAS_IN_MEMORY_EXPRESSIONS
    //   feature: In-Memory Expressions (precomputed virtual columns
    // in the IM column store). Introduced in 12.2.
    #define D_ENV_ORA_HAS_IN_MEMORY_EXPRESSIONS \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0) && \
          D_ENV_ORA_OPTION_IN_MEMORY )

    // D_ENV_ORA_HAS_IN_MEMORY_ON_STANDBY
    //   feature: In-Memory on Active Data Guard standby.
    // Introduced in 12.2.
    #define D_ENV_ORA_HAS_IN_MEMORY_ON_STANDBY \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0) && \
          D_ENV_ORA_OPTION_IN_MEMORY &&            \
          D_ENV_ORA_OPTION_DATA_GUARD )

    // D_ENV_ORA_HAS_RESULT_CACHE
    //   feature: SQL Result Cache and PL/SQL Function Result Cache.
    // Introduced in Oracle 11g.
    #define D_ENV_ORA_HAS_RESULT_CACHE \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)

    // D_ENV_ORA_HAS_SQL_PLAN_MANAGEMENT
    //   feature: SQL Plan Management (plan baselines for plan stability).
    // Introduced in Oracle 11g. EE only.
    #define D_ENV_ORA_HAS_SQL_PLAN_MANAGEMENT \
        ( D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0) && D_ENV_ORA_EDITION_EE )

    // D_ENV_ORA_HAS_REAL_TIME_SQL_MONITORING
    //   feature: Real-Time SQL Monitoring (V$SQL_MONITOR).
    // Introduced in 11g. EE + Diagnostics & Tuning Pack.
    #define D_ENV_ORA_HAS_REAL_TIME_SQL_MONITORING \
        ( D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0) && D_ENV_ORA_EDITION_EE )

    // D_ENV_ORA_HAS_AUTOMATIC_INDEXING
    //   feature: Automatic Indexing (AI-driven index creation and
    // lifecycle management). Introduced in Oracle 19c. EE only.
    #define D_ENV_ORA_HAS_AUTOMATIC_INDEXING \
        ( D_ENV_ORA_AT_LEAST(19) && D_ENV_ORA_EDITION_EE )

    // D_ENV_ORA_HAS_HEAT_MAP
    //   feature: Heat Map (tracking data access patterns for ILM).
    // Introduced in 12.1. EE + Advanced Compression.
    #define D_ENV_ORA_HAS_HEAT_MAP \
        ( D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0) && \
          D_ENV_ORA_OPTION_ADVANCED_COMPRESSION )


// =============================================================================
// XVI. PL/SQL AND PROCEDURAL FEATURES
// =============================================================================

    // D_ENV_ORA_HAS_PLSQL
    //   feature: PL/SQL procedural language. Core feature, always present.
    #define D_ENV_ORA_HAS_PLSQL D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_PLSQL_BOOLEAN_IN_SQL
    //   feature: PL/SQL BOOLEAN values usable in SQL context.
    // Introduced in Oracle 23ai (prior versions required conversion).
    #define D_ENV_ORA_HAS_PLSQL_BOOLEAN_IN_SQL \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_EDITION_BASED_REDEFINITION
    //   feature: Edition-Based Redefinition (EBR) for online application
    // upgrades via private editions. Introduced in Oracle 11gR2.
    #define D_ENV_ORA_HAS_EDITION_BASED_REDEFINITION \
        D_ENV_ORA_VERSION_AT_LEAST(11, 2, 0)

    // D_ENV_ORA_HAS_PLSQL_ACCESSIBLE_BY
    //   feature: ACCESSIBLE BY clause (white-listing callers of PL/SQL
    // units). Introduced in Oracle 12.1.
    #define D_ENV_ORA_HAS_PLSQL_ACCESSIBLE_BY \
        D_ENV_ORA_VERSION_AT_LEAST(12, 1, 0)

    // D_ENV_ORA_HAS_PLSQL_DEPRECATE_PRAGMA
    //   feature: DEPRECATE pragma for PL/SQL. Introduced in Oracle 12.2.
    #define D_ENV_ORA_HAS_PLSQL_DEPRECATE_PRAGMA \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)


// =============================================================================
// XVII. XML, TEXT, AND SPATIAL
// =============================================================================

    // D_ENV_ORA_HAS_XML_DB
    //   feature: Oracle XML DB (XMLType, XQuery, XML indexing, XML
    // Schema storage). Core feature since Oracle 9iR2.
    #define D_ENV_ORA_HAS_XML_DB D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_ORACLE_TEXT
    //   feature: Oracle Text (full-text search, document indexing,
    // CONTAINS queries). Core feature (free in all editions).
    #define D_ENV_ORA_HAS_ORACLE_TEXT D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_LOCATOR
    //   feature: Oracle Locator (basic spatial features: SDO_GEOMETRY,
    // spatial indexing, basic operations). Free in all editions.
    #define D_ENV_ORA_HAS_LOCATOR D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_SPATIAL_FULL
    //   feature: full Oracle Spatial and Graph (advanced spatial
    // analysis, network data model, geocoding, routing).
    // Requires Spatial and Graph option (EE).
    #define D_ENV_ORA_HAS_SPATIAL_FULL \
        D_ENV_ORA_OPTION_SPATIAL

    // D_ENV_ORA_HAS_PROPERTY_GRAPH
    //   feature: Property Graph support (SQL/PGQ). SQL property graph
    // queries via GRAPH_TABLE. Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_PROPERTY_GRAPH \
        D_ENV_ORA_AT_LEAST(23)

    // D_ENV_ORA_HAS_RDF_GRAPH
    //   feature: RDF Semantic Graph (triple store, SPARQL).
    // Requires Spatial and Graph option.
    #define D_ENV_ORA_HAS_RDF_GRAPH \
        D_ENV_ORA_OPTION_SPATIAL


// =============================================================================
// XVIII. ADVANCED QUEUING
// =============================================================================

    // D_ENV_ORA_HAS_AQ
    //   feature: Advanced Queuing (message queuing via database tables).
    // Core feature since Oracle 8i.
    #define D_ENV_ORA_HAS_AQ D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_TRANSACTIONAL_EVENT_QUEUES
    //   feature: Transactional Event Queues (TxEventQ, Kafka-compatible
    // event streaming). Introduced in Oracle 21c.
    #define D_ENV_ORA_HAS_TRANSACTIONAL_EVENT_QUEUES \
        D_ENV_ORA_AT_LEAST(21)

    // D_ENV_ORA_HAS_AQ_KAFKA_INTEROP
    //   feature: AQ/TxEventQ Kafka client interoperability (consume/
    // produce via standard Kafka protocol). Introduced in Oracle 23ai.
    #define D_ENV_ORA_HAS_AQ_KAFKA_INTEROP \
        D_ENV_ORA_AT_LEAST(23)


// =============================================================================
// XIX.  GENERATED COLUMNS AND VIRTUAL COLUMNS
// =============================================================================

    // D_ENV_ORA_HAS_VIRTUAL_COLUMNS
    //   feature: virtual columns (computed columns, not physically
    // stored). Introduced in Oracle 11g.
    #define D_ENV_ORA_HAS_VIRTUAL_COLUMNS \
        D_ENV_ORA_VERSION_AT_LEAST(11, 1, 0)

    // D_ENV_ORA_HAS_ONLINE_TABLE_REDEFINITION
    //   feature: DBMS_REDEFINITION (online table restructuring).
    // Introduced in Oracle 10g.
    #define D_ENV_ORA_HAS_ONLINE_TABLE_REDEFINITION D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_ONLINE_DDL
    //   feature: various online DDL operations (online index rebuild,
    // online table move, online datafile move). Expanded in each release.
    #define D_ENV_ORA_HAS_ONLINE_DDL D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_ONLINE_TABLE_MOVE
    //   feature: ALTER TABLE ... MOVE ONLINE. Introduced in 12.2.
    #define D_ENV_ORA_HAS_ONLINE_TABLE_MOVE \
        D_ENV_ORA_VERSION_AT_LEAST(12, 2, 0)


// =============================================================================
// XX.   PLATFORM INTEGRATION
// =============================================================================

    // D_ENV_ORA_HAS_BFILE
    //   feature: BFILE data type (pointer to OS file). Core feature.
    #define D_ENV_ORA_HAS_BFILE D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_DBLINK
    //   feature: database links (distributed queries across databases).
    #define D_ENV_ORA_HAS_DBLINK D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_SCHEDULER
    //   feature: DBMS_SCHEDULER (job scheduling). Introduced in Oracle
    // 10g, replacing DBMS_JOB.
    #define D_ENV_ORA_HAS_SCHEDULER D_ENV_ORA_DETECTED

    // D_ENV_ORA_HAS_REST_API
    //   feature: ORDS (Oracle REST Data Services) integration.
    // Not detected from OCI headers; overridable.
    #ifndef D_ENV_ORA_HAS_REST_API
        #define D_ENV_ORA_HAS_REST_API 0
    #endif


// =============================================================================
// XXI.  CONVENIENCE / COMPOSITE MACROS
// =============================================================================

    // D_ENV_ORA_HAS_MODERN_SQL
    //   macro: evaluates to 1 if recursive WITH, LATERAL, MATCH_RECOGNIZE,
    // row-limiting, and identity columns are all available.
    #define D_ENV_ORA_HAS_MODERN_SQL \
        ( D_ENV_ORA_HAS_RECURSIVE_WITH        && \
          D_ENV_ORA_HAS_LATERAL_INLINE_VIEW   && \
          D_ENV_ORA_HAS_MATCH_RECOGNIZE       && \
          D_ENV_ORA_HAS_ROW_LIMITING          && \
          D_ENV_ORA_HAS_IDENTITY_COLUMNS )

    // D_ENV_ORA_HAS_MODERN_JSON
    //   macro: evaluates to 1 if JSON data type, duality views, SODA,
    // and JSON_TABLE are all available.
    #define D_ENV_ORA_HAS_MODERN_JSON \
        ( D_ENV_ORA_HAS_JSON_DATA_TYPE        && \
          D_ENV_ORA_HAS_JSON_DUALITY_VIEWS    && \
          D_ENV_ORA_HAS_SODA                  && \
          D_ENV_ORA_HAS_JSON_FUNCTIONS )

    // D_ENV_ORA_HAS_MODERN_SECURITY
    //   macro: evaluates to 1 if unified audit, privilege analysis,
    // and SQL Firewall are all available.
    #define D_ENV_ORA_HAS_MODERN_SECURITY \
        ( D_ENV_ORA_HAS_UNIFIED_AUDIT         && \
          D_ENV_ORA_HAS_PRIVILEGE_ANALYSIS    && \
          D_ENV_ORA_HAS_SQL_FIREWALL )

    // D_ENV_ORA_HAS_MODERN_MULTITENANT
    //   macro: evaluates to 1 if multitenant with lockdown, relocate,
    // and app containers are all available.
    #define D_ENV_ORA_HAS_MODERN_MULTITENANT \
        ( D_ENV_ORA_HAS_MULTITENANT            && \
          D_ENV_ORA_HAS_PDB_LOCKDOWN          && \
          D_ENV_ORA_HAS_APPLICATION_CONTAINERS )

    // D_ENV_ORA_HAS_23AI_FEATURES
    //   macro: evaluates to 1 if all headline 23ai features are
    // available (boolean type, SQL domains, IF NOT EXISTS, JSON duality,
    // property graphs, SQL Firewall, annotations).
    #define D_ENV_ORA_HAS_23AI_FEATURES \
        ( D_ENV_ORA_HAS_BOOLEAN_TYPE           && \
          D_ENV_ORA_HAS_SQL_DOMAINS            && \
          D_ENV_ORA_HAS_IF_NOT_EXISTS          && \
          D_ENV_ORA_HAS_JSON_DUALITY_VIEWS     && \
          D_ENV_ORA_HAS_PROPERTY_GRAPH         && \
          D_ENV_ORA_HAS_SQL_FIREWALL           && \
          D_ENV_ORA_HAS_ANNOTATIONS )

    // D_ENV_ORA_IS_FULLY_MODERN
    //   macro: evaluates to 1 if Oracle has a comprehensive modern
    // feature set (roughly 23ai level for version features).
    #define D_ENV_ORA_IS_FULLY_MODERN \
        ( D_ENV_ORA_HAS_MODERN_SQL            && \
          D_ENV_ORA_HAS_MODERN_JSON           && \
          D_ENV_ORA_HAS_MODERN_SECURITY       && \
          D_ENV_ORA_HAS_MODERN_MULTITENANT    && \
          D_ENV_ORA_HAS_23AI_FEATURES )


#endif  // D_ENV_ORA_DETECTED


#endif  // DJINTERP_ENVIRONMENT_ORACLE_
