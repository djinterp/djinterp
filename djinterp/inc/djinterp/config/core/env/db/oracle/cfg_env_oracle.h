/*******************************************************************************
* djinterp [config]                                             cfg_env_oracle.h
*
* Per-module configuration for env_oracle.h. Owns all D_CFG_ENV_ORACLE_*
* defaults plus D_CFG_ENV_ORA_CUSTOM and the pre-defined-detection
* auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*
* path:      /inc/djinterp/config/core/env/db/oracle/cfg_env_oracle.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.22
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_ORACLE_CFG_ENV_ORACLE_H
#define DJINTERP_CONFIG_CORE_ENV_DB_ORACLE_CFG_ENV_ORACLE_H 1

// cfg_common.h, never the dconfig.h umbrella: it picks up user overrides
// itself, and the umbrella, reached from here, re-entered env detection
// before it had finished
#include "../../../../cfg_common.h"  // D_CFG_* helpers, user overrides


// ===========================================================================
// I.   ENABLE / PATH CONFIGURATION
// ===========================================================================

// D_CFG_ENV_USING_ORACLE
//   configuration: 1 to enable Oracle DB header inclusion and detection.
#ifndef D_CFG_ENV_USING_ORACLE
    #define D_CFG_ENV_USING_ORACLE 0
#endif

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_ORACLE)
    #error "D_CFG_ENV_USING_ORACLE must be 0 or 1"
#endif

// D_CFG_ENV_ORACLE_C_PATH
//   configuration: include path for the Oracle OCI C header.
#ifndef D_CFG_ENV_ORACLE_C_PATH
    #define D_CFG_ENV_ORACLE_C_PATH <oci.h>
#endif

// D_CFG_ENV_ORACLE_CPP_PATH
//   configuration: include path for the Oracle OCCI C++ header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_ORACLE_CPP_PATH
    #define D_CFG_ENV_ORACLE_CPP_PATH <occi.h>
#endif


// ===========================================================================
// II.  DETECTION-MODE CONFIGURATION
// ===========================================================================

// D_CFG_ENV_ORA_CUSTOM
//   configuration: master Oracle environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_ORA_DETECTED_*
//      variables)
#ifndef D_CFG_ENV_ORA_CUSTOM
    #define D_CFG_ENV_ORA_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_ORA_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_ORA_DETECTED_VERSION)  ||  \
      defined(D_ENV_ORA_DETECTED_11_2)     ||  \
      defined(D_ENV_ORA_DETECTED_12_1)     ||  \
      defined(D_ENV_ORA_DETECTED_12_2)     ||  \
      defined(D_ENV_ORA_DETECTED_18)       ||  \
      defined(D_ENV_ORA_DETECTED_19)       ||  \
      defined(D_ENV_ORA_DETECTED_21)       ||  \
      defined(D_ENV_ORA_DETECTED_23) )
    #undef  D_CFG_ENV_ORA_CUSTOM
    #define D_CFG_ENV_ORA_CUSTOM 1
#endif

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_ORA_CUSTOM)
    #error "D_CFG_ENV_ORA_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_ORACLE_CFG_ENV_ORACLE_H
