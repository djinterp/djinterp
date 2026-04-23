/******************************************************************************
* djinterp [config][db]                                    env_oracle_config.h
*
* Per-module configuration for env_oracle.h. Owns all D_CFG_ENV_ORACLE_*
* defaults plus D_CFG_ENV_ORA_CUSTOM and the pre-defined-detection
* auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
* 
* path:      /inc/djinterp/config/core/env/db/oracle/env_oracle_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_ORACLE_
#define DJINTERP_CONFIG_ENV_ORACLE_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// =============================================================================
// I.   ENABLE / PATH CONFIGURATION
// =============================================================================

// D_CFG_ENV_USING_ORACLE
//   configuration: 1 to enable Oracle DB header inclusion and detection.
#ifndef D_CFG_ENV_USING_ORACLE
    #define D_CFG_ENV_USING_ORACLE 0
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


// =============================================================================
// II.  DETECTION-MODE CONFIGURATION
// =============================================================================

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


#endif  // DJINTERP_CONFIG_ENV_ORACLE_
