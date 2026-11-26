/*******************************************************************************
* djinterp [c][net]                                                       lmtp.h
*
* djinterp LMTP (RFC 2033).
*   Umbrella header for LMTP, the Local Mail Transfer Protocol, by which a
* mail system hands messages to a local delivery agent: the client
* (lmtp_client.h) and the server (lmtp_server.h). Both are the SMTP kernel's
* (net/smtp) in its LMTP dialect, so its environment detection, vocabulary
* and errors are SMTP's. Include the individual headers instead to take
* less.
*
*
* path:      /inc/djinterp/c/modules/net/lmtp/lmtp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/

#ifndef DJINTERP_C_NET_LMTP_LMTP_H
#define DJINTERP_C_NET_LMTP_LMTP_H 1

// djinterp
#include "../../../djinterp.h"                  // framework root
#include "../../../../env/net/smtp/env_smtp.h"  // D_ENV_SMTP_* detection
#include "./lmtp_client.h"                     // client sessions
#include "./lmtp_server.h"                     // server sessions


#endif  // DJINTERP_C_NET_LMTP_LMTP_H
