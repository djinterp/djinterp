/******************************************************************************
* djinterp [core]                                        env_arangodb_config.h
*
* Per-module configuration for env_arangodb.h. Owns all
* D_CFG_ENV_ARANGODB_* defaults plus D_CFG_ENV_ARANGO_CUSTOM and the
* pre-defined-detection auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*   NOTE: ArangoDB has no canonical C client surface; only CPP_PATH is
* provided (velocypack).
*
* 
* path:      /inc/djinterp/config/core/env/db/arangodb/env_arangodb_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_ARANGODB_
#define DJINTERP_CONFIG_ENV_ARANGODB_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// =============================================================================
// I.   ENABLE / PATH CONFIGURATION
// =============================================================================

// D_CFG_ENV_USING_ARANGODB
//   configuration: 1 to enable ArangoDB header inclusion and detection
// (C++ only; raises #error in C builds).
#ifndef D_CFG_ENV_USING_ARANGODB
    #define D_CFG_ENV_USING_ARANGODB 0
#endif

// D_CFG_ENV_ARANGODB_CPP_PATH
//   configuration: include path for the ArangoDB velocypack C++ header.
#ifndef D_CFG_ENV_ARANGODB_CPP_PATH
    #define D_CFG_ENV_ARANGODB_CPP_PATH <velocypack/vpack.h>
#endif


// =============================================================================
// II.  DETECTION-MODE CONFIGURATION
// =============================================================================

// D_CFG_ENV_ARANGO_CUSTOM
//   configuration: master ArangoDB environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_ARANGO_DETECTED_*
//      variables)
#ifndef D_CFG_ENV_ARANGO_CUSTOM
    #define D_CFG_ENV_ARANGO_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_ARANGO_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_ARANGO_DETECTED_VERSION)       ||  \
      defined(D_ENV_ARANGO_DETECTED_3_4)           ||  \
      defined(D_ENV_ARANGO_DETECTED_3_5)           ||  \
      defined(D_ENV_ARANGO_DETECTED_3_6)           ||  \
      defined(D_ENV_ARANGO_DETECTED_3_7)           ||  \
      defined(D_ENV_ARANGO_DETECTED_3_8)           ||  \
      defined(D_ENV_ARANGO_DETECTED_3_9)           ||  \
      defined(D_ENV_ARANGO_DETECTED_3_10)          ||  \
      defined(D_ENV_ARANGO_DETECTED_3_11)          ||  \
      defined(D_ENV_ARANGO_DETECTED_3_12)          ||  \
      defined(D_ENV_ARANGO_DETECTED_ENTERPRISE) )
    #undef  D_CFG_ENV_ARANGO_CUSTOM
    #define D_CFG_ENV_ARANGO_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_ENV_ARANGODB_
