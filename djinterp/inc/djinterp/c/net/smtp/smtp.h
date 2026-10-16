/*******************************************************************************
* djinterp [c]                                                            smtp.h
*
* djinterp SMTP kernel.
*   Umbrella header for the C SMTP kernel: environment detection, protocol
* vocabulary, transports, the client, and the server. Include the individual
* headers instead to take less.
*
*
* path:      /inc/djinterp/c/net/smtp/smtp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

#ifndef DJINTERP_C_NET_SMTP_SMTP_H
#define DJINTERP_C_NET_SMTP_SMTP_H 1

// djinterp
#include "../../djinterp.h"             // framework root
#include "../../../env/net/env_smtp.h"  // D_ENV_SMTP_* detection
#include "./smtp_common.h"              // protocol vocabulary
#include "./smtp_transport.h"           // transports, sockets, listeners
#include "./smtp_client.h"              // client sessions
#include "./smtp_server.h"              // server sessions


#endif  // DJINTERP_C_NET_SMTP_SMTP_H
