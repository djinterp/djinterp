/*******************************************************************************
* djinterp [net]                                                          smtp.h
*
* djinterp SMTP kernel.
*   Umbrella header for the C SMTP kernel: environment detection, protocol
* vocabulary, transports, the client, and the server. Include the individual
* headers instead to take less.
*
*
* path:      /inc/djinterp/net/smtp/smtp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_NET_SMTP_SMTP_H
#define DJINTERP_NET_SMTP_SMTP_H 1

// djinterp
#include "../../c/djinterp.h"             // framework root
#include "../../env/net/smtp/env_smtp.h"  // D_ENV_SMTP_* detection
#include "./smtp_common.h"                // protocol vocabulary
#include "./smtp_transport.h"             // transports, sockets, listeners
#include "./smtp_client.h"                // client sessions
#include "./smtp_server.h"                // server sessions


#endif  // DJINTERP_NET_SMTP_SMTP_H
