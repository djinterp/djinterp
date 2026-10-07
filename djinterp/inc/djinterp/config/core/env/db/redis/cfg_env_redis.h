/*******************************************************************************
* djinterp [config]                                              cfg_env_redis.h
*
* djinterp Redis detection configuration.
*   The settings env_redis.h reads: whether the hiredis client header is
* included and where to find it, whether automatic client detection is
* bypassed, and which target server version feature gating assumes.
*   Every setting is #ifndef-guarded: define it before this header, for
* instance with -D, in cfg_custom.h or in a project-wide configuration header,
* to override it.
*   In manual mode (D_CFG_ENV_REDIS_CUSTOM set to 1), env_redis.h takes the
* client version from D_ENV_REDIS_DETECTED_CLIENT_VERSION. The target server
* version and the distribution are always manual:
* D_CFG_ENV_REDIS_SERVER_VERSION or D_ENV_REDIS_DETECTED_SERVER_*, and
* D_ENV_REDIS_DETECTED_VALKEY, _ENTERPRISE, _CLOUD and _STACK.
*   targets:  env/db/redis/env_redis.h -> D_CFG_ENV_*
*   requires: cfg_common.h (D_CFG_IS_BOOL, user overrides, testing preset)
*
*
* path:      /inc/djinterp/config/core/env/db/redis/cfg_env_redis.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  Client header
         1.  D_CFG_ENV_USING_REDIS
         2.  D_CFG_ENV_REDIS_C_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_REDIS_CUSTOM
3.  TARGET SERVER VERSION
    ---------------------
    1.  Server version
         1.  D_CFG_ENV_REDIS_SERVER_VERSION
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_REDIS_CFG_ENV_REDIS_H
#define DJINTERP_CONFIG_CORE_ENV_DB_REDIS_CFG_ENV_REDIS_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


//==============================================================================
// 1.  VENDOR HEADER INCLUSION CONTROL
//==============================================================================


// 1.1    Client header
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_USING_REDIS
//   configuration: 1 to include the hiredis client header, and its optional SSL
// header, and detect the client at compile time. When 0, no hiredis symbol is
// referenced and the client reads as absent; server-version feature gating
// still works if configured.
#ifndef D_CFG_ENV_USING_REDIS
    #define D_CFG_ENV_USING_REDIS 0
#endif  // D_CFG_ENV_USING_REDIS

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_REDIS)
    #error "D_CFG_ENV_USING_REDIS must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_REDIS_C_PATH
//   configuration: the include path of the hiredis header. It is tried first,
// and included unconditionally where __has_include is unavailable. With a
// standard install, leave the default and rely on env_redis.h's
// <hiredis/hiredis.h> and <hiredis.h> fallbacks.
#ifndef D_CFG_ENV_REDIS_C_PATH
    #define D_CFG_ENV_REDIS_C_PATH <hiredis/hiredis.h>
#endif  // D_CFG_ENV_REDIS_C_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_REDIS_CUSTOM
//   configuration: 0 (the default) for automatic client detection from
// HIREDIS_MAJOR, HIREDIS_MINOR and HIREDIS_PATCH; 1 to skip it and use a
// pre-defined D_ENV_REDIS_DETECTED_CLIENT_VERSION instead.
#ifndef D_CFG_ENV_REDIS_CUSTOM
    #define D_CFG_ENV_REDIS_CUSTOM 0
#endif  // D_CFG_ENV_REDIS_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_REDIS_CUSTOM)
    #error "D_CFG_ENV_REDIS_CUSTOM must be 0 or 1"
#endif


//==============================================================================
// 3.  TARGET SERVER VERSION
//==============================================================================
// Feature gating by server version.


// 3.1    Server version
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_ENV_REDIS_SERVER_VERSION
//   configuration: the target server version, encoded as MAJOR*10000 +
// MINOR*100 + PATCH; for Redis 7.4.0, define it as 70400. The server version is
// a runtime property, so it can't be detected at compile time. It deliberately
// has no default, so that an unconfigured target stays distinguishable from a
// chosen one; server-gated features then read 0. The
// D_ENV_REDIS_DETECTED_SERVER_* shortcuts in env_redis.h work as well.


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_REDIS_CFG_ENV_REDIS_H
