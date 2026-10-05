/*******************************************************************************
* djinterp [net]                                                          sftp.h
*
* Umbrella header for SFTP, the SSH File Transfer Protocol.
*   Includes both SFTP modules: sftp_common.h, the version 3 vocabulary and
* codec, and sftp_client.h, the client that speaks it over the "sftp"
* subsystem of an SSH channel. Sessions, host keys, and authentication
* belong to the SSH module (net/ssh/ssh.h), which this builds on.
*
*
* path:      /inc/djinterp/net/sftp/sftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

#ifndef DJINTERP_NET_SFTP_SFTP_H
#define DJINTERP_NET_SFTP_SFTP_H 1

// djinterp
#include "./sftp_common.h"  // codec, attributes, errors
#include "./sftp_client.h"  // the client


#endif  // DJINTERP_NET_SFTP_SFTP_H
