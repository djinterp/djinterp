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
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  Client header
         1.  D_CFG_ENV_USING_ORACLE
         2.  D_CFG_ENV_ORACLE_C_PATH
         3.  D_CFG_ENV_ORACLE_CPP_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_ORA_CUSTOM
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_ORACLE_CFG_ENV_ORACLE_H
#define DJINTERP_CONFIG_CORE_ENV_DB_ORACLE_CFG_ENV_ORACLE_H 1

// cfg_common.h, never the dconfig.h umbrella: it picks up user overrides
// itself, and the umbrella, reached from here, re-entered env detection
// before it had finished
#include "../../../../cfg_common.h"  // D_CFG_* helpers, user overrides


//==============================================================================
// 1.  VENDOR HEADER INCLUSION CONTROL
//==============================================================================


// 1.1    Client header
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_USING_ORACLE
//   configuration: 1 to enable Oracle DB header inclusion and detection.
#ifndef D_CFG_ENV_USING_ORACLE
    #define D_CFG_ENV_USING_ORACLE 0
#endif  // D_CFG_ENV_USING_ORACLE

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_ORACLE)
    #error "D_CFG_ENV_USING_ORACLE must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_ORACLE_C_PATH
//   configuration: include path for the Oracle OCI C header.
#ifndef D_CFG_ENV_ORACLE_C_PATH
    #define D_CFG_ENV_ORACLE_C_PATH <oci.h>
#endif  // D_CFG_ENV_ORACLE_C_PATH

// 1.1.3
// D_CFG_ENV_ORACLE_CPP_PATH
//   configuration: include path for the Oracle OCCI C++ header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_ORACLE_CPP_PATH
    #define D_CFG_ENV_ORACLE_CPP_PATH <occi.h>
#endif  // D_CFG_ENV_ORACLE_CPP_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_ORA_CUSTOM
//   configuration: master Oracle environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_ORA_DETECTED_*
//      variables)
// A pre-defined D_ENV_*_DETECTED_* result makes 1 the default, and an
// explicit setting still wins (decision 118 of the register).
#ifndef D_CFG_ENV_ORA_CUSTOM
    #if ( (defined(D_ENV_ORA_DETECTED_VERSION)) ||                             \
          (defined(D_ENV_ORA_DETECTED_11_2))    ||                             \
          (defined(D_ENV_ORA_DETECTED_12_1))    ||                             \
          (defined(D_ENV_ORA_DETECTED_12_2))    ||                             \
          (defined(D_ENV_ORA_DETECTED_18))      ||                             \
          (defined(D_ENV_ORA_DETECTED_19))      ||                             \
          (defined(D_ENV_ORA_DETECTED_21))      ||                             \
          (defined(D_ENV_ORA_DETECTED_23)) )
        #define D_CFG_ENV_ORA_CUSTOM 1
    #else
        #define D_CFG_ENV_ORA_CUSTOM 0
    #endif
#endif  // D_CFG_ENV_ORA_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_ORA_CUSTOM)
    #error "D_CFG_ENV_ORA_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_ORACLE_CFG_ENV_ORACLE_H
