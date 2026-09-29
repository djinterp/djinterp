/*******************************************************************************
* djinterp [env]                                          env_cassandra_config.h
*
* djinterp Apache Cassandra detection configuration.
*   The settings env_cassandra.h reads: whether the DataStax C/C++ driver
* header is included and where to find it, whether automatic driver detection
* is bypassed, and which target server version feature gating assumes.
*   Every setting is #ifndef-guarded: define it before this header, for
* instance with -D or in a project-wide configuration header, to override it.
*   In manual mode (D_CFG_ENV_CASS_CUSTOM set to 1), env_cassandra.h takes the
* driver version from D_ENV_CASS_DETECTED_DRIVER_VERSION. The target server
* version and the edition are always manual: D_CFG_ENV_CASS_SERVER_VERSION or
* D_ENV_CASS_DETECTED_SERVER_*, and D_ENV_CASS_DETECTED_DSE or
* D_ENV_CASS_DETECTED_ASTRA.
*
*
* path:      /inc/djinterp/env/db/cassandra/env_cassandra_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  Driver header
         1.  D_CFG_ENV_USING_CASSANDRA
         2.  D_CFG_ENV_CASSANDRA_C_PATH
2.  DETECTION MODE
    --------------
    1.  Driver detection
         1.  D_CFG_ENV_CASS_CUSTOM
3.  TARGET SERVER VERSION
    ---------------------
    1.  Server version
         1.  D_CFG_ENV_CASS_SERVER_VERSION
*/

#ifndef DJINTERP_ENV_DB_CASSANDRA_ENV_CASSANDRA_CONFIG_H
#define DJINTERP_ENV_DB_CASSANDRA_ENV_CASSANDRA_CONFIG_H 1


//==============================================================================
// 1.  VENDOR HEADER INCLUSION CONTROL
//==============================================================================


// 1.1    Driver header
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_USING_CASSANDRA
//   configuration: 1 to include the DataStax C/C++ driver header (cassandra.h)
// and detect the driver at compile time. When 0, no cassandra symbol is
// referenced and the driver reads as absent; server-version feature gating
// still works if configured.
#ifndef D_CFG_ENV_USING_CASSANDRA
    #define D_CFG_ENV_USING_CASSANDRA 0
#endif  // D_CFG_ENV_USING_CASSANDRA

// 1.1.2
// D_CFG_ENV_CASSANDRA_C_PATH
//   configuration: the include path of the driver header. It is tried first,
// and included unconditionally where __has_include is unavailable. With a
// standard install, leave the default and rely on env_cassandra.h's
// <cassandra.h> fallback.
#ifndef D_CFG_ENV_CASSANDRA_C_PATH
    #define D_CFG_ENV_CASSANDRA_C_PATH <cassandra.h>
#endif  // D_CFG_ENV_CASSANDRA_C_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Driver detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_CASS_CUSTOM
//   configuration: 0 (the default) for automatic driver detection from the
// CASS_VERSION_* macros; 1 to skip it and use a pre-defined
// D_ENV_CASS_DETECTED_DRIVER_VERSION instead.
#ifndef D_CFG_ENV_CASS_CUSTOM
    #define D_CFG_ENV_CASS_CUSTOM 0
#endif  // D_CFG_ENV_CASS_CUSTOM


//==============================================================================
// 3.  TARGET SERVER VERSION
//==============================================================================
// Feature gating by server version.


// 3.1    Server version
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_ENV_CASS_SERVER_VERSION
//   configuration: the target server version, encoded as MAJOR*10000 +
// MINOR*100 + PATCH; for Cassandra 4.1.3, define it as 40103. The server
// version is a runtime property, so it can't be detected at compile time. It
// deliberately has no default, so that an unconfigured target stays
// distinguishable from a chosen one; server-gated features then read 0. The
// D_ENV_CASS_DETECTED_SERVER_* shortcuts in env_cassandra.h work as well.


#endif  // DJINTERP_ENV_DB_CASSANDRA_ENV_CASSANDRA_CONFIG_H
