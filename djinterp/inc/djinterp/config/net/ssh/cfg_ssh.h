/*******************************************************************************
* djinterp [config]                                                    cfg_ssh.h
*
*   Configuration for the SSH common module. It selects which built-in engines
* are compiled -- today, the libssh2 engine -- and the defaults that
* d_ssh_options_default and d_ssh_credentials_default hand out: the host-key
* policy, how new hosts are recorded, whether the system-wide known_hosts file
* is consulted, the algorithm restriction, agent use, and the deadline for
* blocking operations.
*   The defaults are the secure ones. Each knob that relaxes one says what it
* gives up, and every default can still be changed per session at run time.
*
*
* path:      /inc/djinterp/config/net/ssh/cfg_ssh.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BUILT-IN IMPLEMENTATIONS
    ------------------------
    1.  Compile switches
         1.  D_CFG_SSH_LIBSSH2
2.  HOST-KEY VERIFICATION
    ---------------------
    1.  Policy
         1.  D_CFG_SSH_HOST_KEY_*
              1.  D_CFG_SSH_HOST_KEY_STRICT
              2.  D_CFG_SSH_HOST_KEY_ACCEPT_NEW
              3.  D_CFG_SSH_HOST_KEY_CALLBACK
              4.  D_CFG_SSH_HOST_KEY_INSECURE_ACCEPT_ANY
         2.  D_CFG_SSH_HOST_KEY_POLICY
    2.  Known-hosts files
         1.  D_CFG_SSH_HASH_KNOWN_HOSTS
         2.  D_CFG_SSH_SYSTEM_KNOWN_HOSTS
3.  SESSION DEFAULTS
    ----------------
    1.  Algorithms
         1.  D_CFG_SSH_RESTRICT_ALGORITHMS
    2.  Authentication
         1.  D_CFG_SSH_AUTH_AGENT
    3.  Deadlines
         1.  D_CFG_SSH_TIMEOUT_MS
4.  VALIDATION
    ----------
    1.  Knob validation
5.  DERIVED VALUES
    --------------
    1.  Built-in implementations
         1.  D_INTERNAL_SSH_LIBSSH2
         2.  D_INTERNAL_SSH_SOCKET_HANDLE
         3.  D_INTERNAL_SSH_BSD
         4.  D_INTERNAL_SSH_WINSOCK
    2.  Host keys
         1.  D_INTERNAL_SSH_HOST_KEY_POLICY
         2.  D_INTERNAL_SSH_HASH_KNOWN_HOSTS
         3.  D_INTERNAL_SSH_HOME_VARIABLE
         4.  D_INTERNAL_SSH_SYSTEM_KNOWN_HOSTS
    3.  Session defaults
         1.  D_INTERNAL_SSH_RESTRICT_ALGORITHMS
         2.  D_INTERNAL_SSH_AUTH_AGENT
         3.  D_INTERNAL_SSH_TIMEOUT_MS
*/

#ifndef DJINTERP_CONFIG_NET_SSH_CFG_SSH_H
#define DJINTERP_CONFIG_NET_SSH_CFG_SSH_H 1

// djinterp
#include "../../cfg_common.h"               // D_CFG_IS_ON, D_CFG_NORM
#include "../../../env/net/ssh/env_ssh.h"   // D_ENV_SSH_*, D_ENV_NET_*


//==============================================================================
// 1.  BUILT-IN IMPLEMENTATIONS
//==============================================================================
// A switch can remove a built-in, never force one in: an engine is compiled
// only when its switch is on and the library it wraps was detected.


// 1.1    Compile switches
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_SSH_LIBSSH2
//   knob: 1 (the default) to compile the built-in libssh2 engine where
// libssh2 1.9.0 or later is detected; 0 to leave it out, for a program that
// brings its own engine and should not link libssh2.
#ifndef D_CFG_SSH_LIBSSH2
#   define D_CFG_SSH_LIBSSH2 1
#endif  // D_CFG_SSH_LIBSSH2


//==============================================================================
// 2.  HOST-KEY VERIFICATION
//==============================================================================
// The host key is SSH's only defense against a man in the middle: the
// transport is encrypted either way, but only a verified key says who is on
// the other end. A connection that ends up with an unverified key never
// reaches authentication, so no password or key signature is ever offered to
// it.


// 2.1    Policy
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_SSH_HOST_KEY_*
//   constant: the values D_CFG_SSH_HOST_KEY_POLICY accepts. They equal the
// d_ssh_host_key_policy values of the same names, and ssh.c asserts that they
// do. Numbering starts at 1 so that a zero-initialized options struct is
// rejected rather than read as a policy.
// 2.1.1.1
// D_CFG_SSH_HOST_KEY_STRICT
//   constant: accept only a key recorded in a known_hosts file.
#define D_CFG_SSH_HOST_KEY_STRICT              1

// 2.1.1.2
// D_CFG_SSH_HOST_KEY_ACCEPT_NEW
//   constant: record and accept a host that has no recorded key (trust on
// first use); a host with any recorded key must present one of them.
#define D_CFG_SSH_HOST_KEY_ACCEPT_NEW          2

// 2.1.1.3
// D_CFG_SSH_HOST_KEY_CALLBACK
//   constant: hand a host that has no recorded key to the program's decision
// callback; a changed or revoked key still fails.
#define D_CFG_SSH_HOST_KEY_CALLBACK            3

// 2.1.1.4
// D_CFG_SSH_HOST_KEY_INSECURE_ACCEPT_ANY
//   constant: accept every key without looking. For tests against throwaway
// servers only: it gives up everything verification provides.
#define D_CFG_SSH_HOST_KEY_INSECURE_ACCEPT_ANY 4

// 2.1.2
// D_CFG_SSH_HOST_KEY_POLICY
//   knob: the default host-key policy, one of D_CFG_SSH_HOST_KEY_*. STRICT
// (the default) fails a host that was never recorded, so a first connection
// cannot be intercepted; ACCEPT_NEW trades that first connection's safety for
// not having to provision known_hosts in advance.
#ifndef D_CFG_SSH_HOST_KEY_POLICY
#   define D_CFG_SSH_HOST_KEY_POLICY D_CFG_SSH_HOST_KEY_STRICT
#endif  // D_CFG_SSH_HOST_KEY_POLICY

// 2.2    Known-hosts files
//------------------------------------------------------------------------------
// 2.2.1
// D_CFG_SSH_HASH_KNOWN_HOSTS
//   knob: 1 (the default) to record new hosts under a salted hash of their
// name, OpenSSH's "|1|" form, so that a stolen known_hosts file does not list
// the machines it vouches for; 0 to record names in clear, readable at a
// glance. Hashing needs a secure random source for the salt; where there is
// none, entries are written in clear.
#ifndef D_CFG_SSH_HASH_KNOWN_HOSTS
#   define D_CFG_SSH_HASH_KNOWN_HOSTS 1
#endif  // D_CFG_SSH_HASH_KNOWN_HOSTS

// 2.2.2
// D_CFG_SSH_SYSTEM_KNOWN_HOSTS
//   knob: 1 (the default) to consult the administrator's system-wide file,
// /etc/ssh/ssh_known_hosts, as well as the user's, where the platform has
// one; 0 to trust only the user's file. The system file is only ever read.
#ifndef D_CFG_SSH_SYSTEM_KNOWN_HOSTS
#   define D_CFG_SSH_SYSTEM_KNOWN_HOSTS 1
#endif  // D_CFG_SSH_SYSTEM_KNOWN_HOSTS


//==============================================================================
// 3.  SESSION DEFAULTS
//==============================================================================
// What d_ssh_options_default and d_ssh_credentials_default hand out.


// 3.1    Algorithms
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_SSH_RESTRICT_ALGORITHMS
//   knob: 1 (the default) to offer only the module's cipher and MAC lists,
// D_SSH_CIPHERS and D_SSH_MACS: AES-GCM, and AES-CTR with plain SHA-2 MACs.
// These resist the Terrapin attack whether or not the library implements
// strict key exchange. 0 to accept the library's defaults, which can include
// ChaCha20-Poly1305, the encrypt-then-MAC modes, and legacy CBC ciphers.
// Host-key algorithms are not affected: they always come from the module's
// list or the caller's, ordered by what known_hosts records.
#ifndef D_CFG_SSH_RESTRICT_ALGORITHMS
#   define D_CFG_SSH_RESTRICT_ALGORITHMS 1
#endif  // D_CFG_SSH_RESTRICT_ALGORITHMS

// 3.2    Authentication
//------------------------------------------------------------------------------
// 3.2.1
// D_CFG_SSH_AUTH_AGENT
//   knob: 1 (the default) for credentials to try the authentication agent's
// keys where an agent can be reached; 0 to use only the key file, password,
// and prompt the caller supplies.
#ifndef D_CFG_SSH_AUTH_AGENT
#   define D_CFG_SSH_AUTH_AGENT 1
#endif  // D_CFG_SSH_AUTH_AGENT

// 3.3    Deadlines
//------------------------------------------------------------------------------
// 3.3.1
// D_CFG_SSH_TIMEOUT_MS
//   knob: the default deadline, in milliseconds, for each blocking operation
// of a session, from 0 to 3600000 (one hour). A deadline that expires is
// fatal to the session. 0 disables it, and a silent server then stalls the
// caller forever. Defaults to 30000.
#ifndef D_CFG_SSH_TIMEOUT_MS
#   define D_CFG_SSH_TIMEOUT_MS 30000
#endif  // D_CFG_SSH_TIMEOUT_MS


//==============================================================================
// 4.  VALIDATION
//==============================================================================


// 4.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_SSH_LIBSSH2)
#   error "D_CFG_SSH_LIBSSH2 must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_SSH_HOST_KEY_POLICY) <                                 \
       D_CFG_SSH_HOST_KEY_STRICT) ||                                           \
      (D_CFG_NORM(D_CFG_SSH_HOST_KEY_POLICY) >                                 \
       D_CFG_SSH_HOST_KEY_INSECURE_ACCEPT_ANY) )
#   error "D_CFG_SSH_HOST_KEY_POLICY must be one of D_CFG_SSH_HOST_KEY_*"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSH_HASH_KNOWN_HOSTS)
#   error "D_CFG_SSH_HASH_KNOWN_HOSTS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSH_SYSTEM_KNOWN_HOSTS)
#   error "D_CFG_SSH_SYSTEM_KNOWN_HOSTS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSH_RESTRICT_ALGORITHMS)
#   error "D_CFG_SSH_RESTRICT_ALGORITHMS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_SSH_AUTH_AGENT)
#   error "D_CFG_SSH_AUTH_AGENT must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_SSH_TIMEOUT_MS) < 0) ||                                \
      (D_CFG_NORM(D_CFG_SSH_TIMEOUT_MS) > 3600000) )
#   error "D_CFG_SSH_TIMEOUT_MS must be between 0 and 3600000"
#endif


//==============================================================================
// 5.  DERIVED VALUES
//==============================================================================
// What the module reads. Each value combines a knob with what env_ssh.h and
// env_net.h detected; ssh.h and ssh.c test these, never the knobs.


// 5.1    Built-in implementations
//------------------------------------------------------------------------------
// 5.1.1
// D_INTERNAL_SSH_LIBSSH2
//   value: 1 if the libssh2 engine is compiled.
#if ( (D_CFG_IS_ON(D_CFG_SSH_LIBSSH2)) &&                                      \
      (D_ENV_SSH_LIBSSH2_USABLE) )
#   define D_INTERNAL_SSH_LIBSSH2 1
#else
#   define D_INTERNAL_SSH_LIBSSH2 0
#endif

// 5.1.2
// D_INTERNAL_SSH_SOCKET_HANDLE
//   value: 1 if sockets are Winsock handles (unsigned, pointer-sized) rather
// than file descriptors.
#if ( (D_ENV_NET_CAN_SOCKET) &&                                                \
      (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_WINSOCK) )
#   define D_INTERNAL_SSH_SOCKET_HANDLE 1
#else
#   define D_INTERNAL_SSH_SOCKET_HANDLE 0
#endif

// 5.1.3
// D_INTERNAL_SSH_BSD
//   value: 1 if the session closes and configures its socket through the BSD
// socket API.
#if ( (D_ENV_NET_CAN_SOCKET) &&                                                \
      (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_BSD) )
#   define D_INTERNAL_SSH_BSD 1
#else
#   define D_INTERNAL_SSH_BSD 0
#endif

// 5.1.4
// D_INTERNAL_SSH_WINSOCK
//   value: 1 if the session closes its socket through Winsock.
#if ( (D_ENV_NET_CAN_SOCKET) &&                                                \
      (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_WINSOCK) )
#   define D_INTERNAL_SSH_WINSOCK 1
#else
#   define D_INTERNAL_SSH_WINSOCK 0
#endif

// 5.2    Host keys
//------------------------------------------------------------------------------
// 5.2.1
// D_INTERNAL_SSH_HOST_KEY_POLICY
//   value: the default host-key policy.
#define D_INTERNAL_SSH_HOST_KEY_POLICY D_CFG_NORM(D_CFG_SSH_HOST_KEY_POLICY)

// 5.2.2
// D_INTERNAL_SSH_HASH_KNOWN_HOSTS
//   value: 1 if new hosts are recorded hashed by default.
#if ( (D_CFG_IS_ON(D_CFG_SSH_HASH_KNOWN_HOSTS)) &&                             \
      (D_ENV_SSH_HAS_RANDOM) )
#   define D_INTERNAL_SSH_HASH_KNOWN_HOSTS 1
#else
#   define D_INTERNAL_SSH_HASH_KNOWN_HOSTS 0
#endif

// 5.2.3
// D_INTERNAL_SSH_HOME_VARIABLE
//   value: the environment variable naming the user's home directory, under
// which the default known_hosts file lives as .ssh/known_hosts.
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
#   define D_INTERNAL_SSH_HOME_VARIABLE "USERPROFILE"
#else
#   define D_INTERNAL_SSH_HOME_VARIABLE "HOME"
#endif

// 5.2.4
// D_INTERNAL_SSH_SYSTEM_KNOWN_HOSTS
//   value: the default system-wide known_hosts path, or a null pointer where
// it is not consulted.
#if ( (D_CFG_IS_ON(D_CFG_SSH_SYSTEM_KNOWN_HOSTS)) &&                           \
      (D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)) )
#   define D_INTERNAL_SSH_SYSTEM_KNOWN_HOSTS "/etc/ssh/ssh_known_hosts"
#else
#   define D_INTERNAL_SSH_SYSTEM_KNOWN_HOSTS ((const char*)0)
#endif

// 5.3    Session defaults
//------------------------------------------------------------------------------
// 5.3.1
// D_INTERNAL_SSH_RESTRICT_ALGORITHMS
//   value: 1 if unspecified cipher and MAC lists default to the module's.
#if D_CFG_IS_ON(D_CFG_SSH_RESTRICT_ALGORITHMS)
#   define D_INTERNAL_SSH_RESTRICT_ALGORITHMS 1
#else
#   define D_INTERNAL_SSH_RESTRICT_ALGORITHMS 0
#endif

// 5.3.2
// D_INTERNAL_SSH_AUTH_AGENT
//   value: 1 if credentials try the agent by default.
#if ( (D_CFG_IS_ON(D_CFG_SSH_AUTH_AGENT)) &&                                   \
      (D_ENV_SSH_CAN_REACH_AGENT) )
#   define D_INTERNAL_SSH_AUTH_AGENT 1
#else
#   define D_INTERNAL_SSH_AUTH_AGENT 0
#endif

// 5.3.3
// D_INTERNAL_SSH_TIMEOUT_MS
//   value: the default deadline for blocking operations, in milliseconds.
#define D_INTERNAL_SSH_TIMEOUT_MS D_CFG_NORM(D_CFG_SSH_TIMEOUT_MS)


#endif  // DJINTERP_CONFIG_NET_SSH_CFG_SSH_H
