/*******************************************************************************
* djinterp [net]                                                           ssh.h
*
* SSH common module: the part of SSH every SSH module shares.
*   SSH runs each service -- a remote command, a shell, SFTP, SCP, a
* forwarded port -- as a channel inside one encrypted, authenticated
* session. The services differ; the path to a trustworthy session does not,
* and neither does the byte format their packets are written in. That
* shared part lives here once, one concern to a header:
*     ssh_common.h       outcomes, sockets, and the default port
*     ssh_wire.h         RFC 4251 readers and writers, for SFTP, the agent
*                        protocol, and key blobs
*     ssh_base64.h       strict Base64, as known_hosts and fingerprints
*                        use it
*     ssh_key.h          key blobs, SHA256 fingerprints, secure randomness
*     ssh_known_hosts.h  OpenSSH known_hosts lookup and recording
*     ssh_engine.h       the engine interface over an SSH library
*     ssh_libssh2.h      the built-in libssh2 engine
*     ssh_session.h      the session: a handshake that no caller can use
*                        before the host key is verified
*     ssh_auth.h         authentication by agent, key file,
*                        keyboard-interactive, and password
*     ssh_channel.h      exec, subsystem, shell, and direct-tcpip channels
*   This header includes them all. A service module derives from this one
* by opening channels on an authenticated d_ssh_session: an exec module
* runs "exec"; an SFTP module builds its packets with d_ssh_writer and
* speaks them over the "sftp" subsystem.
*   The session runs over a connected stream socket the caller supplies --
* the one transport every SSH library accepts. Every call blocks, and a
* session and its channels belong to one thread at a time.
*   Requires cfg_ssh.h, and through it env_ssh.h. The libssh2 engine links
* against libssh2; on Windows, the random source links against bcrypt.
*
*
* path:      /inc/djinterp/net/ssh/ssh.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

#ifndef DJINTERP_NET_SSH_SSH_H
#define DJINTERP_NET_SSH_SSH_H 1

// djinterp
#include "./ssh_common.h"       // d_ssh_status, d_ssh_socket
#include "./ssh_wire.h"         // d_ssh_reader, d_ssh_writer
#include "./ssh_base64.h"       // d_ssh_base64_*
#include "./ssh_key.h"          // key blobs, fingerprints, d_ssh_random
#include "./ssh_known_hosts.h"  // d_ssh_known_hosts_*
#include "./ssh_engine.h"       // d_ssh_engine_vtable, library lifetime
#include "./ssh_libssh2.h"      // d_ssh_engine_libssh2
#include "./ssh_session.h"      // d_ssh_session
#include "./ssh_auth.h"         // d_ssh_authenticate
#include "./ssh_channel.h"      // d_ssh_channel


#endif  // DJINTERP_NET_SSH_SSH_H
