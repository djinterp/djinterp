/*******************************************************************************
* djinterp [config]                                            cfg_env_mongodb.h
*
* Per-module configuration for env_mongodb.h. Owns all D_CFG_ENV_MONGODB_*
* defaults plus D_CFG_ENV_MONGO_CUSTOM and the pre-defined-detection
* auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*
* path:      /inc/djinterp/config/core/env/db/mongodb/cfg_env_mongodb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.22
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_MONGODB_CFG_ENV_MONGODB_H
#define DJINTERP_CONFIG_CORE_ENV_DB_MONGODB_CFG_ENV_MONGODB_H 1

// cfg_common.h, never the dconfig.h umbrella: it picks up user overrides
// itself, and the umbrella, reached from here, re-entered env detection
// before it had finished
#include "../../../../cfg_common.h"  // D_CFG_* helpers, user overrides


// ===========================================================================
// I.   ENABLE / PATH CONFIGURATION
// ===========================================================================

// D_CFG_ENV_USING_MONGODB
//   configuration: 1 to enable MongoDB header inclusion and detection.
#ifndef D_CFG_ENV_USING_MONGODB
    #define D_CFG_ENV_USING_MONGODB 0
#endif

// D_CFG_ENV_MONGODB_C_PATH
//   configuration: include path for the MongoDB C driver header (mongoc).
#ifndef D_CFG_ENV_MONGODB_C_PATH
    #define D_CFG_ENV_MONGODB_C_PATH <mongoc/mongoc.h>
#endif

// D_CFG_ENV_MONGODB_CPP_PATH
//   configuration: include path for the mongocxx C++ driver header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_MONGODB_CPP_PATH
    #define D_CFG_ENV_MONGODB_CPP_PATH <mongocxx/client.hpp>
#endif


// ===========================================================================
// II.  DETECTION-MODE CONFIGURATION
// ===========================================================================

// D_CFG_ENV_MONGO_CUSTOM
//   configuration: master MongoDB environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_MONGO_DETECTED_*
//      variables)
#ifndef D_CFG_ENV_MONGO_CUSTOM
    #define D_CFG_ENV_MONGO_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_MONGO_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_MONGO_DETECTED_DRIVER_VERSION)  ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_VERSION)  ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_3_6)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_4_0)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_4_2)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_4_4)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_5_0)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_6_0)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_7_0)      ||  \
      defined(D_ENV_MONGO_DETECTED_SERVER_8_0)      ||  \
      defined(D_ENV_MONGO_DETECTED_ENTERPRISE) )
    #undef  D_CFG_ENV_MONGO_CUSTOM
    #define D_CFG_ENV_MONGO_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_MONGODB_CFG_ENV_MONGODB_H
