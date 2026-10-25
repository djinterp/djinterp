/*******************************************************************************
* djinterp [env]                                                  env_starttls.h
*
* djinterp STARTTLS environment detection.
*   Compile-time detection of whether an in-band TLS upgrade can run here,
* expressed through the D_ENV_STARTTLS_* interface. The upgrade is the one
* the STARTTLS family shares: SMTP and LMTP STARTTLS, IMAP STARTTLS, POP3
* STLS, NNTP STARTTLS, FTP AUTH TLS, ManageSieve STARTTLS, XMPP <starttls/>,
* the LDAP StartTLS extended operation, and the PostgreSQL SSLRequest.
*   The STARTTLS module itself needs nothing from the environment: it runs
* over any net.h connection, and its TLS is the SSL kernel's, whose engines
* are sans-I/O and so take over a stream that has already carried plaintext
* as readily as a fresh one. Whether an upgrade can actually run is
* therefore whether TLS can, which env_ssl.h answers. It covers:
*     - a rolled-up availability gate, D_ENV_STARTTLS_AVAILABLE           [1]
*   Like env_tls.h, it answers "is this present to call?", never "will the
* peer agree?": whether a server offers STARTTLS is a runtime fact.
*   Every flag is #ifndef-guarded, so a project may pre-define any
* D_ENV_STARTTLS_* macro before inclusion to override detection.
*   It requires env_ssl.h, and includes it itself. It is an opt-in module and
* may be included directly.
*
*
* path:      /inc/djinterp/env/net/starttls/env_starttls.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  AVAILABILITY GATE
    -----------------
    1.  Rolled-up availability
         1.  D_ENV_STARTTLS_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_STARTTLS_ENV_STARTTLS_H
#define DJINTERP_ENV_NET_STARTTLS_ENV_STARTTLS_H 1

// djinterp
#include "../ssl/env_ssl.h"  // D_ENV_SSL_AVAILABLE


//==============================================================================
// 1.  AVAILABILITY GATE
//==============================================================================
// What a STARTTLS protocol module checks before it builds.


// 1.1    Rolled-up availability
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_STARTTLS_AVAILABLE
//   feature: 1 if an upgrade can actually run here -- an SSL engine that can
// be built, and a TCP transport to carry the plaintext phase: exactly
// D_ENV_SSL_AVAILABLE. The gate a STARTTLS protocol module should #error on
// when it cannot work without one.
#ifndef D_ENV_STARTTLS_AVAILABLE
    #define D_ENV_STARTTLS_AVAILABLE D_ENV_SSL_AVAILABLE
#endif  // D_ENV_STARTTLS_AVAILABLE


#endif  // DJINTERP_ENV_NET_STARTTLS_ENV_STARTTLS_H
