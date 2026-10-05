/*******************************************************************************
* djinterp [config]                                                    cfg_pop.h
*
* Build-time configuration for the POP3 modules.
*   Line-buffer sizing, default strictness, APOP, and the backend enables for
* the transport-bound modules derived from the common kernel.
*
*   targets:  net/pop/pop.h -> D_INTERNAL_POP_LINE_MAX, D_INTERNAL_POP_STRICT,
*             D_INTERNAL_POP_APOP, D_INTERNAL_POP_TLS, D_INTERNAL_POP_CURL,
*             D_INTERNAL_POP_SASL
*   requires: cfg_common.h; env/net/env_pop.h (backend defaults)
*
*
* path:      /inc/djinterp/config/net/pop/cfg_pop.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.25
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  WIRE BEHAVIOR
    -------------
    1.  Buffering and strictness
         1.  D_CFG_POP_LINE_MAX
         2.  D_CFG_POP_STRICT
    2.  Optional mechanisms
         1.  D_CFG_POP_APOP
2.  BACKENDS
    --------
    1.  Environment-defaulted enables
         1.  D_CFG_POP_TLS
         2.  D_CFG_POP_CURL
         3.  D_CFG_POP_SASL
3.  VALIDATION
    ----------
    1.  Knob validation
4.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_POP_LINE_MAX
         2.  D_INTERNAL_POP_STRICT
         3.  D_INTERNAL_POP_APOP
         4.  D_INTERNAL_POP_TLS
         5.  D_INTERNAL_POP_CURL
         6.  D_INTERNAL_POP_SASL
*/

#ifndef DJINTERP_CONFIG_NET_POP_CFG_POP_H
#define DJINTERP_CONFIG_NET_POP_CFG_POP_H 1

// djinterp
#include "../../cfg_common.h"              // D_CFG_IS_BOOL, D_CFG_NORM
#include "../../../env/net/pop/env_pop.h"  // D_ENV_POP_CAN_*, D_ENV_POP_HAS_SASL


//==============================================================================
// 1.  WIRE BEHAVIOR
//==============================================================================


// 1.1    Buffering and strictness
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_POP_LINE_MAX
//   brief: capacity in bytes of a d_pop_reader's line buffer, and so the
// longest status or CAPA line it accepts. Default 1024. RFC 2449 caps a
// response at 512 octets including CRLF, but greetings and IMPLEMENTATION
// lines from real servers exceed that often enough that a strict-sized buffer
// fails in the field; 1024 is the tolerant default. Multi-line bodies are
// streamed and are not bounded by this. Must lie in [512, 65536].
#ifndef D_CFG_POP_LINE_MAX
#   define D_CFG_POP_LINE_MAX 1024
#endif  // D_CFG_POP_LINE_MAX

// 1.1.2
// D_CFG_POP_STRICT
//   brief: the DEFAULT strictness of newly initialized readers and body
// decoders. 1 rejects bare-LF line endings on status lines and treats a bare
// LF in a body as content; 0 (the default) accepts bare LF as a line end, as
// most deployed servers and clients do. Strictness is a run-time field on each
// object, so one build can exercise both; this knob only picks its initial
// value.
#ifndef D_CFG_POP_STRICT
#   define D_CFG_POP_STRICT 0
#endif  // D_CFG_POP_STRICT

// 1.2    Optional mechanisms
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_POP_APOP
//   brief: compile in APOP digest support (1, the default) or leave it out
// (0). APOP uses MD5, which the kernel implements itself, so this costs no
// dependency; switching it off is for builds that must carry no MD5 at all.
// With it off, the APOP digest functions report D_POP_STATUS_UNSUPPORTED.
#ifndef D_CFG_POP_APOP
#   define D_CFG_POP_APOP 1
#endif  // D_CFG_POP_APOP


//==============================================================================
// 2.  BACKENDS
//==============================================================================
// Enables for the transport-bound modules derived from pop.h. Each defaults
// from env_pop.h (cascade layer 4), so a backend is on exactly when the
// environment can build it. A user may switch one off; switching one on where
// the environment cannot provide it is a configuration error, not a request,
// and is rejected in section 3.


// 2.1    Environment-defaulted enables
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_POP_TLS
//   brief: build STLS and POP3S support (1) or not (0). Defaults to
// D_ENV_POP_CAN_TLS.
#ifndef D_CFG_POP_TLS
#   if D_ENV_POP_CAN_TLS
#       define D_CFG_POP_TLS 1
#   else
#       define D_CFG_POP_TLS 0
#   endif
#endif  // D_CFG_POP_TLS

// 2.1.2
// D_CFG_POP_CURL
//   brief: build the libcurl-backed POP3 module (1) or not (0). Defaults to
// D_ENV_POP_CAN_CURL.
#ifndef D_CFG_POP_CURL
#   if D_ENV_POP_CAN_CURL
#       define D_CFG_POP_CURL 1
#   else
#       define D_CFG_POP_CURL 0
#   endif
#endif  // D_CFG_POP_CURL

// 2.1.3
// D_CFG_POP_SASL
//   brief: delegate AUTH to an external SASL library (1) or restrict AUTH to
// the built-in mechanisms (0). Defaults to D_ENV_POP_HAS_SASL.
#ifndef D_CFG_POP_SASL
#   if D_ENV_POP_HAS_SASL
#       define D_CFG_POP_SASL 1
#   else
#       define D_CFG_POP_SASL 0
#   endif
#endif  // D_CFG_POP_SASL


//==============================================================================
// 3.  VALIDATION
//==============================================================================


// 3.1    Knob validation
//------------------------------------------------------------------------------
#if ( (D_CFG_NORM(D_CFG_POP_LINE_MAX) < 512) ||                                \
      (D_CFG_NORM(D_CFG_POP_LINE_MAX) > 65536) )
#   error "D_CFG_POP_LINE_MAX must lie in [512, 65536]"
#endif
#if !D_CFG_IS_BOOL(D_CFG_POP_STRICT)
#   error "D_CFG_POP_STRICT must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_POP_APOP)
#   error "D_CFG_POP_APOP must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_POP_TLS)
#   error "D_CFG_POP_TLS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_POP_CURL)
#   error "D_CFG_POP_CURL must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_POP_SASL)
#   error "D_CFG_POP_SASL must be 0 or 1"
#endif
#if ( (D_CFG_IS_ON(D_CFG_POP_TLS)) &&                                          \
      (!D_ENV_POP_CAN_TLS) )
#   error "D_CFG_POP_TLS is 1 but env_pop.h found no usable TLS library"
#endif
#if ( (D_CFG_IS_ON(D_CFG_POP_CURL)) &&                                         \
      (!D_ENV_POP_CAN_CURL) )
#   error "D_CFG_POP_CURL is 1 but env_pop.h found no POP3-capable libcurl"
#endif
#if ( (D_CFG_IS_ON(D_CFG_POP_SASL)) &&                                         \
      (!D_ENV_POP_HAS_SASL) )
#   error "D_CFG_POP_SASL is 1 but env_pop.h found no SASL library"
#endif


//==============================================================================
// 4.  RESOLVED VALUES
//==============================================================================
// The only symbols pop.h and its derived modules read. Each normalizes its
// knob to a plain integer, so a module never sees an empty define.


// 4.1    Effective values
//------------------------------------------------------------------------------
// 4.1.1
// D_INTERNAL_POP_LINE_MAX
//   brief: the reader line-buffer capacity, from D_CFG_POP_LINE_MAX.
#define D_INTERNAL_POP_LINE_MAX D_CFG_NORM(D_CFG_POP_LINE_MAX)

// 4.1.2
// D_INTERNAL_POP_STRICT
//   brief: the initial strictness of readers and decoders (0 or 1).
#if D_CFG_IS_ON(D_CFG_POP_STRICT)
#   define D_INTERNAL_POP_STRICT 1
#else
#   define D_INTERNAL_POP_STRICT 0
#endif

// 4.1.3
// D_INTERNAL_POP_APOP
//   brief: 1 when the APOP digest (and its MD5) is compiled in.
#if D_CFG_IS_ON(D_CFG_POP_APOP)
#   define D_INTERNAL_POP_APOP 1
#else
#   define D_INTERNAL_POP_APOP 0
#endif

// 4.1.4
// D_INTERNAL_POP_TLS
//   brief: 1 when STLS / POP3S modules are enabled.
#if D_CFG_IS_ON(D_CFG_POP_TLS)
#   define D_INTERNAL_POP_TLS 1
#else
#   define D_INTERNAL_POP_TLS 0
#endif

// 4.1.5
// D_INTERNAL_POP_CURL
//   brief: 1 when the libcurl-backed module is enabled.
#if D_CFG_IS_ON(D_CFG_POP_CURL)
#   define D_INTERNAL_POP_CURL 1
#else
#   define D_INTERNAL_POP_CURL 0
#endif

// 4.1.6
// D_INTERNAL_POP_SASL
//   brief: 1 when AUTH may delegate to an external SASL library.
#if D_CFG_IS_ON(D_CFG_POP_SASL)
#   define D_INTERNAL_POP_SASL 1
#else
#   define D_INTERNAL_POP_SASL 0
#endif


#endif  // DJINTERP_CONFIG_NET_POP_CFG_POP_H
