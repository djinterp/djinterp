/*******************************************************************************
* djinterp [config]                                               cfg_starttls.h
*
*   Configuration for the STARTTLS common module: the defaults that
* d_starttls_options_default hands out -- the upgrade policy, the treatment
* of plaintext still buffered at the upgrade point, line-ending strictness,
* and the receive buffer's size. TLS itself is configured where every TLS
* session is, in the d_ssl_context a caller makes (cfg_ssl.h).
*   The defaults are the secure ones. Each knob that relaxes one says what it
* gives up, and every default can still be changed per session at run time.
*
*   targets:  net/starttls/starttls.h and starttls.c, through the
*             D_INTERNAL_STARTTLS_* values in section 3
*   requires: cfg_common.h (knob helpers)
*
*
* path:      /inc/djinterp/config/net/starttls/cfg_starttls.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SESSION DEFAULTS
    ----------------
    1.  Upgrade policy
         1.  D_CFG_STARTTLS_POLICY_*
              1.  D_CFG_STARTTLS_POLICY_DISABLED
              2.  D_CFG_STARTTLS_POLICY_OPPORTUNISTIC
              3.  D_CFG_STARTTLS_POLICY_REQUIRED
         2.  D_CFG_STARTTLS_POLICY
    2.  Input handling
         1.  D_CFG_STARTTLS_REJECT_LEFTOVER
         2.  D_CFG_STARTTLS_STRICT_CRLF
         3.  D_CFG_STARTTLS_BUFFER_BYTES
2.  VALIDATION
    ----------
    1.  Knob validation
3.  DERIVED VALUES
    --------------
    1.  Session defaults
         1.  D_INTERNAL_STARTTLS_POLICY
         2.  D_INTERNAL_STARTTLS_REJECT_LEFTOVER
         3.  D_INTERNAL_STARTTLS_STRICT_CRLF
         4.  D_INTERNAL_STARTTLS_BUFFER_BYTES
*/

#ifndef DJINTERP_CONFIG_NET_STARTTLS_CFG_STARTTLS_H
#define DJINTERP_CONFIG_NET_STARTTLS_CFG_STARTTLS_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_NORM


//==============================================================================
// 1.  SESSION DEFAULTS
//==============================================================================
// What d_starttls_options_default returns. Each value only seeds a session's
// options; a caller may change any of them before d_starttls_init.


// 1.1    Upgrade policy
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_STARTTLS_POLICY_*
//   constant: sentinels for D_CFG_STARTTLS_POLICY. Their values equal those of
// enum d_starttls_policy, which starttls.c asserts. Numbering starts at 1 on
// purpose: D_CFG_NORM reads an empty, misspelled, or non-numeric value as 0,
// and 0 is no policy, so such a value stops the build instead of turning TLS
// off.
// 1.1.1.1
// D_CFG_STARTTLS_POLICY_DISABLED
//   constant: never upgrade.
#define D_CFG_STARTTLS_POLICY_DISABLED      1

// 1.1.1.2
// D_CFG_STARTTLS_POLICY_OPPORTUNISTIC
//   constant: upgrade when offered, continue in plaintext otherwise.
#define D_CFG_STARTTLS_POLICY_OPPORTUNISTIC 2

// 1.1.1.3
// D_CFG_STARTTLS_POLICY_REQUIRED
//   constant: upgrade or fail.
#define D_CFG_STARTTLS_POLICY_REQUIRED      3

// 1.1.2
// D_CFG_STARTTLS_POLICY
//   knob: the default upgrade policy, one of D_CFG_STARTTLS_POLICY_*.
// REQUIRED is the only policy that holds against an active attacker, who can
// delete a STARTTLS offer from the plaintext exchange at will; OPPORTUNISTIC
// protects against passive eavesdropping only, and suits a peer that must
// keep talking to servers without TLS.
#ifndef D_CFG_STARTTLS_POLICY
    #define D_CFG_STARTTLS_POLICY D_CFG_STARTTLS_POLICY_REQUIRED
#endif  // D_CFG_STARTTLS_POLICY

// 1.2    Input handling
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_STARTTLS_REJECT_LEFTOVER
//   knob: 1 to fail an upgrade that finds plaintext still waiting at the
// switch point (D_STARTTLS_ERROR_INJECTION); 0 to discard it and count it in
// the session's `discarded` field. Rejecting is the stronger defense against
// the command-injection class of CVE-2011-0411. Discarding also keeps the bytes
// from ever being read as TLS data, and suits a peer that must tolerate a
// broken implementation which sends extra bytes after its go-ahead.
#ifndef D_CFG_STARTTLS_REJECT_LEFTOVER
    #define D_CFG_STARTTLS_REJECT_LEFTOVER 1
#endif  // D_CFG_STARTTLS_REJECT_LEFTOVER

// 1.2.2
// D_CFG_STARTTLS_STRICT_CRLF
//   knob: 1 to reject a line ended by a bare LF (D_STARTTLS_ERROR_PROTOCOL); 0
// to accept one. A CR or NUL inside a line is rejected either way. Strictness
// closes the gap in which two parsers disagree about where a line ends -- the
// mechanism behind SMTP smuggling -- at the cost of refusing peers that end
// lines with LF alone.
#ifndef D_CFG_STARTTLS_STRICT_CRLF
    #define D_CFG_STARTTLS_STRICT_CRLF 1
#endif  // D_CFG_STARTTLS_STRICT_CRLF

// 1.2.3
// D_CFG_STARTTLS_BUFFER_BYTES
//   knob: the size of a session's receive buffer, in bytes, which is also the
// longest line it accepts, terminator included. At least 512 (SMTP's reply
// line limit, D_STARTTLS_BUFFER_MIN) and at most 1 MiB. The default leaves
// room for long IMAP responses; a server facing untrusted clients may lower
// it, since every session allocates it in full.
#ifndef D_CFG_STARTTLS_BUFFER_BYTES
    #define D_CFG_STARTTLS_BUFFER_BYTES 8192
#endif  // D_CFG_STARTTLS_BUFFER_BYTES


//==============================================================================
// 2.  VALIDATION
//==============================================================================
// Each knob is checked after defaulting, so a bad value stops the build here
// instead of misbehaving at run time.


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if ( (D_CFG_NORM(D_CFG_STARTTLS_POLICY) < D_CFG_STARTTLS_POLICY_DISABLED) ||  \
      (D_CFG_NORM(D_CFG_STARTTLS_POLICY) > D_CFG_STARTTLS_POLICY_REQUIRED) )
    #error "D_CFG_STARTTLS_POLICY must be one of D_CFG_STARTTLS_POLICY_*"
#endif

#if !D_CFG_IS_BOOL(D_CFG_STARTTLS_REJECT_LEFTOVER)
    #error "D_CFG_STARTTLS_REJECT_LEFTOVER must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_STARTTLS_STRICT_CRLF)
    #error "D_CFG_STARTTLS_STRICT_CRLF must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_STARTTLS_BUFFER_BYTES) < 512) ||                       \
      (D_CFG_NORM(D_CFG_STARTTLS_BUFFER_BYTES) > 1048576) )
    #error "D_CFG_STARTTLS_BUFFER_BYTES must be between 512 and 1048576"
#endif


//==============================================================================
// 3.  DERIVED VALUES
//==============================================================================
// What the module reads. Every value is 0 or 1, or a normalized number. The
// module names D_CFG_* symbols only in the static assertions that hold its
// policy enumeration equal to the sentinels of section 1.


// 3.1    Session defaults
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_STARTTLS_POLICY
//   derived: the default policy, as an enum d_starttls_policy value.
#define D_INTERNAL_STARTTLS_POLICY D_CFG_NORM(D_CFG_STARTTLS_POLICY)

// 3.1.2
// D_INTERNAL_STARTTLS_REJECT_LEFTOVER
//   derived: 1 if leftover plaintext fails an upgrade by default.
#if D_CFG_IS_ON(D_CFG_STARTTLS_REJECT_LEFTOVER)
    #define D_INTERNAL_STARTTLS_REJECT_LEFTOVER 1
#else
    #define D_INTERNAL_STARTTLS_REJECT_LEFTOVER 0
#endif

// 3.1.3
// D_INTERNAL_STARTTLS_STRICT_CRLF
//   derived: 1 if a bare LF is rejected by default.
#if D_CFG_IS_ON(D_CFG_STARTTLS_STRICT_CRLF)
    #define D_INTERNAL_STARTTLS_STRICT_CRLF 1
#else
    #define D_INTERNAL_STARTTLS_STRICT_CRLF 0
#endif

// 3.1.4
// D_INTERNAL_STARTTLS_BUFFER_BYTES
//   derived: the default receive buffer size, in bytes.
#define D_INTERNAL_STARTTLS_BUFFER_BYTES D_CFG_NORM(D_CFG_STARTTLS_BUFFER_BYTES)


#endif  // DJINTERP_CONFIG_NET_STARTTLS_CFG_STARTTLS_H
