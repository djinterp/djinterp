/*******************************************************************************
* djinterp [config]                                                    dconfig.h
*
*   Configuration umbrella. Including this file resolves the ENTIRE config
* graph up front. Use it when you want cross-cutting deductions applied
* deterministically, or to precompile all configuration into a PCH.
*
*   You do NOT need this for normal use: each module pulls its own *_cfg.h,
* which pulls dconfig_common.h -- so you only pay for the modules you include
* (demand-loading). This umbrella is the opt-in "resolve everything" path.
*
*
* path:      /inc/djinterp/config/dconfig.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_DCONFIG_H
#define DJINTERP_CONFIG_DCONFIG_H 1

// djinterp
// Root first: user overrides + testing preset + shared helpers.
#include "cfg_common.h"
#include "djinterp/config/core/env/cfg_env.h"           // env: shared knobs
#include "djinterp/config/core/env/cfg_env_lang.h"      // env: language
#include "djinterp/config/core/env/cfg_env_posix.h"     // env: POSIX / XSI
#include "djinterp/config/core/env/cfg_env_arch.h"      // env: architecture
#include "djinterp/config/core/env/cfg_env_os.h"        // env: OS
#include "djinterp/config/core/env/cfg_env_compiler.h"  // env: compiler
#include "djinterp/config/core/env/cfg_env_build.h"     // env: build type
#include "cfg_qualifiers.h"      // storage / linkage qualifiers
#include "djinterp/config/core/container/table/cfg_table.h"  // the table DSL subframework
#include "djinterp/config/parse/cfg_parse.h"        // the parse substrate
#include "djinterp/config/parsegen/cfg_parsegen.h"  // parser generation (after parse)
#include "djinterp/config/net/cfg_net.h"            // net foundation
#include "djinterp/config/net/curl/cfg_curl.h"      // libcurl binding
#include "djinterp/config/net/pop/cfg_pop.h"        // POP3 common kernel
#include "djinterp/config/net/ssh/cfg_ssh.h"        // SSH common kernel
#include "djinterp/config/net/ssl/cfg_ssl.h"        // SSL/TLS common kernel
#include "djinterp/config/net/tcp/cfg_tcp.h"        // TCP transport
#include "djinterp/config/net/http/cfg_http.h"      // HTTP
// #include "core/<sub>/cfg_<sub>.h"  // <- add future subframeworks here


// ===========================================================================
// II.  CROSS-CUTTING DEDUCTIONS
// ===========================================================================
//   Long-range propagation that demand-loading cannot order correctly belongs
// here (module A influencing an otherwise-unrelated module B). Every rule is
// #ifndef-guarded so explicit user overrides still win. Example:
//
//     #if D_CFG_IS_ON(D_CFG_FOO_ADVANCED)
//     #  ifndef D_CFG_BAR_BACKEND
//     #    define D_CFG_BAR_BACKEND 1
//     #  endif
//     #endif
//
//   (none defined yet)


#endif  // DJINTERP_CONFIG_DCONFIG_H
