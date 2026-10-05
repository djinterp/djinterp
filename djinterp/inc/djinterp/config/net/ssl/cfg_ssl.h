/*******************************************************************************
* djinterp [config]                                                    cfg_ssl.h
*
* Build-time configuration for the SSL/TLS modules.
*   Buffer sizing, the policy floor every configuration is validated against,
* and the engine enables for the modules derived from the common kernel.
*
*   targets:  net/ssl/ssl.h -> D_INTERNAL_SSL_IO_BUFFER,
*             D_INTERNAL_SSL_PINS_MAX, D_INTERNAL_SSL_ALLOW_LEGACY,
*             D_INTERNAL_SSL_KEYLOG, D_INTERNAL_SSL_OPENSSL
*   requires: cfg_common.h; env/net/env_ssl.h (engine defaults)
*
*
* path:      /inc/djinterp/config/net/ssl/cfg_ssl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KERNEL
    ------
    1.  Buffering
         1.  D_CFG_SSL_IO_BUFFER
         2.  D_CFG_SSL_PINS_MAX
    2.  Policy
         1.  D_CFG_SSL_ALLOW_LEGACY
         2.  D_CFG_SSL_KEYLOG
2.  ENGINES
    -------
    1.  Environment-defaulted enables
         1.  D_CFG_SSL_OPENSSL
3.  VALIDATION
    ----------
    1.  Knob validation
4.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_SSL_IO_BUFFER
         2.  D_INTERNAL_SSL_PINS_MAX
         3.  D_INTERNAL_SSL_ALLOW_LEGACY
         4.  D_INTERNAL_SSL_KEYLOG
         5.  D_INTERNAL_SSL_OPENSSL
*/

#ifndef DJINTERP_CONFIG_NET_SSL_CFG_SSL_H
#define DJINTERP_CONFIG_NET_SSL_CFG_SSL_H 1

// djinterp
#include "../../cfg_common.h"              // D_CFG_IS_BOOL, D_CFG_NORM
#include "../../../env/net/ssl/env_ssl.h"  // D_ENV_SSL_CAN_OPENSSL


//==============================================================================
// 1.  KERNEL
//==============================================================================
// Knobs read by the common kernel, net/ssl/ssl.h. They size its buffers and
// fix the policy floor every configuration is validated against; none depends
// on a TLS library.


// 1.1    Buffering
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_SSL_IO_BUFFER
//   brief: capacity in bytes of each session's outbound staging buffer, and
// the most a session reads from its transport in one call, through a stack
// buffer of the same size. Default 8192, which moves the longest TLS record
// (18437 bytes with its header) in three calls. A server holding many idle
// sessions may lower it; a bulk transfer may raise it. Must lie in
// [512, 65536].
#ifndef D_CFG_SSL_IO_BUFFER
#   define D_CFG_SSL_IO_BUFFER 8192
#endif  // D_CFG_SSL_IO_BUFFER

// 1.1.2
// D_CFG_SSL_PINS_MAX
//   brief: how many certificate fingerprints one configuration may pin.
// Default 4: the current certificate, its successor, and spares. Each pin
// costs 32 bytes in every d_ssl_config and d_ssl_context. Must lie in [1, 64].
#ifndef D_CFG_SSL_PINS_MAX
#   define D_CFG_SSL_PINS_MAX 4
#endif  // D_CFG_SSL_PINS_MAX

// 1.2    Policy
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_SSL_ALLOW_LEGACY
//   brief: permit TLS 1.0 and 1.1 as a configuration's minimum version (1),
// or refuse such configurations with D_SSL_STATUS_UNSUPPORTED (0, the
// default). RFC 8996 deprecates both versions. A build that must reach old
// servers opts in here, and even then each configuration must ask for them
// explicitly. SSL 3.0 is refused regardless (RFC 7568).
#ifndef D_CFG_SSL_ALLOW_LEGACY
#   define D_CFG_SSL_ALLOW_LEGACY 0
#endif  // D_CFG_SSL_ALLOW_LEGACY

// 1.2.2
// D_CFG_SSL_KEYLOG
//   brief: honour d_ssl_config.keylog (1), or refuse configurations that set
// it with D_SSL_STATUS_UNSUPPORTED (0, the default). A key log records the
// secrets that decrypt a session, in the NSS key-log format packet analyzers
// read. It is a debugging aid, and a build must ask for it.
#ifndef D_CFG_SSL_KEYLOG
#   define D_CFG_SSL_KEYLOG 0
#endif  // D_CFG_SSL_KEYLOG


//==============================================================================
// 2.  ENGINES
//==============================================================================
// Which engines -- the derived modules binding the kernel to a TLS library --
// are built. Each defaults to what env_ssl.h detected; forcing one on where
// the environment cannot support it is an error in section 3.


// 2.1    Environment-defaulted enables
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_SSL_OPENSSL
//   brief: build the OpenSSL-family engine (1) or not (0). Defaults to
// D_ENV_SSL_CAN_OPENSSL: on wherever OpenSSL 1.1.0 or later, LibreSSL, or
// BoringSSL is installed.
#ifndef D_CFG_SSL_OPENSSL
#   define D_CFG_SSL_OPENSSL D_ENV_SSL_CAN_OPENSSL
#endif  // D_CFG_SSL_OPENSSL


//==============================================================================
// 3.  VALIDATION
//==============================================================================

// 3.1    Knob validation
//------------------------------------------------------------------------------
#if ( (D_CFG_NORM(D_CFG_SSL_IO_BUFFER) < 512) ||                               \
      (D_CFG_NORM(D_CFG_SSL_IO_BUFFER) > 65536) )
#   error "cfg_ssl: D_CFG_SSL_IO_BUFFER must lie in [512, 65536]"
#endif

#if ( (D_CFG_NORM(D_CFG_SSL_PINS_MAX) < 1) ||                                  \
      (D_CFG_NORM(D_CFG_SSL_PINS_MAX) > 64) )
#   error "cfg_ssl: D_CFG_SSL_PINS_MAX must lie in [1, 64]"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSL_ALLOW_LEGACY)
#   error "cfg_ssl: D_CFG_SSL_ALLOW_LEGACY must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSL_KEYLOG)
#   error "cfg_ssl: D_CFG_SSL_KEYLOG must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSL_OPENSSL)
#   error "cfg_ssl: D_CFG_SSL_OPENSSL must be 0 or 1"
#endif

#if ( (D_CFG_IS_ON(D_CFG_SSL_OPENSSL)) &&                                      \
      (!D_ENV_SSL_CAN_OPENSSL) )
#   error "cfg_ssl: D_CFG_SSL_OPENSSL requires an OpenSSL-family library"
#endif


//==============================================================================
// 4.  RESOLVED VALUES
//==============================================================================
// The effective values modules read. Module code reads these, never the
// D_CFG_SSL_* knobs directly.


// 4.1    Effective values
//------------------------------------------------------------------------------
// 4.1.1
// D_INTERNAL_SSL_IO_BUFFER
//   value: the resolved staging-buffer and read-chunk size, in bytes.
#define D_INTERNAL_SSL_IO_BUFFER    D_CFG_NORM(D_CFG_SSL_IO_BUFFER)

// 4.1.2
// D_INTERNAL_SSL_PINS_MAX
//   value: the resolved pin capacity of a configuration.
#define D_INTERNAL_SSL_PINS_MAX     D_CFG_NORM(D_CFG_SSL_PINS_MAX)

// 4.1.3
// D_INTERNAL_SSL_ALLOW_LEGACY
//   value: 1 if TLS 1.0 and 1.1 may be configured.
#define D_INTERNAL_SSL_ALLOW_LEGACY D_CFG_NORM(D_CFG_SSL_ALLOW_LEGACY)

// 4.1.4
// D_INTERNAL_SSL_KEYLOG
//   value: 1 if key logging may be configured.
#define D_INTERNAL_SSL_KEYLOG       D_CFG_NORM(D_CFG_SSL_KEYLOG)

// 4.1.5
// D_INTERNAL_SSL_OPENSSL
//   value: 1 if the OpenSSL-family engine is built.
#define D_INTERNAL_SSL_OPENSSL      D_CFG_NORM(D_CFG_SSL_OPENSSL)


#endif  // DJINTERP_CONFIG_NET_SSL_CFG_SSL_H
