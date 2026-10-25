/*******************************************************************************
* djinterp [env]                                                      env_smtp.h
*
* djinterp SMTP environment detection.
*   Compile-time detection of what the SMTP modules can do here, expressed
* through the D_ENV_SMTP_* interface. The engines in net/smtp need nothing
* from the platform: they run over any net.h connection, connect through
* tcp.h, and take their TLS from the SSL kernel, so what a build can do is
* what tcp.c and the SSL engines can, which env_tcp.h and env_ssl.h answer.
* These restate those answers under SMTP's names, so the C engines, their
* C++ facade (smtp.hpp) and their tests read one family. It covers:
*     - rolled-up capabilities: connecting, serving, and TLS            [1]
*   Like env_ssl.h, it answers "is this present to call?", never "will the
* peer agree?": whether a server offers STARTTLS is a runtime fact, and so
* is whether an engine was compiled in, which d_ssl_engine_openssl reports.
*   Naming: D_ENV_SMTP_CAN_<ACTION> is 1 if the action can run here, 0
* otherwise.
*   Every flag is #ifndef-guarded, so a project may pre-define any
* D_ENV_SMTP_* macro before inclusion to override detection, to describe a
* cross-compilation target, say.
*   It requires env_tcp.h and env_ssl.h, and includes them itself. It is an
* opt-in module and may be included directly.
*
*
* path:      /inc/djinterp/env/net/smtp/env_smtp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CAPABILITIES
    ------------
    1.  Rolled-up capabilities
         1.  D_ENV_SMTP_CAN_CONNECT
         2.  D_ENV_SMTP_CAN_SERVE
         3.  D_ENV_SMTP_CAN_TLS
*/

#ifndef DJINTERP_ENV_NET_SMTP_ENV_SMTP_H
#define DJINTERP_ENV_NET_SMTP_ENV_SMTP_H 1

// djinterp
#include "../ssl/env_ssl.h"  // D_ENV_SSL_AVAILABLE
#include "../tcp/env_tcp.h"  // D_ENV_TCP_AVAILABLE


//==============================================================================
// 1.  CAPABILITIES
//==============================================================================
// What a build can do beyond running a session over a connection it is
// given, which every build can.


// 1.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_SMTP_CAN_CONNECT
//   capability: d_smtp_client_connect can open its own connection: tcp.c
// has a socket backend here.
#ifndef D_ENV_SMTP_CAN_CONNECT
    #define D_ENV_SMTP_CAN_CONNECT D_ENV_TCP_AVAILABLE
#endif  // D_ENV_SMTP_CAN_CONNECT

// 1.1.2
// D_ENV_SMTP_CAN_SERVE
//   capability: connections can be accepted for the server engine, through
// tcp.h's listeners, as smtp.hpp's server does.
#ifndef D_ENV_SMTP_CAN_SERVE
    #define D_ENV_SMTP_CAN_SERVE D_ENV_TCP_AVAILABLE
#endif  // D_ENV_SMTP_CAN_SERVE

// 1.1.3
// D_ENV_SMTP_CAN_TLS
//   capability: STARTTLS and implicit TLS can run here, as a client and as
// a server: an SSL engine can be built, and a TCP transport carries it --
// exactly D_ENV_SSL_AVAILABLE.
#ifndef D_ENV_SMTP_CAN_TLS
    #define D_ENV_SMTP_CAN_TLS D_ENV_SSL_AVAILABLE
#endif  // D_ENV_SMTP_CAN_TLS


#endif  // DJINTERP_ENV_NET_SMTP_ENV_SMTP_H
