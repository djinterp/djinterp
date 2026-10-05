/*******************************************************************************
* djinterp [env]                                                       env_ssh.h
*
* djinterp SSH environment detection.
*   Compile-time detection of what the SSH family of modules needs from the
* environment, expressed through the D_ENV_SSH_* interface. SSH's transport
* layer -- key exchange, host-key signatures, the cipher and MAC suites -- is
* code that belongs in a maintained library rather than in each program, so
* the first question is which SSH library is present and how recent it is: the
* release decides which host-key algorithms it can verify, and whether it
* resists the Terrapin prefix-truncation attack (CVE-2023-48795). It covers:
*     - the SSH libraries present (libssh2, libssh, wolfSSH) and their
*       versions                                                          [1]
*     - the version-gated features of each library that the SSH modules
*       depend on                                                         [2]
*     - how an authentication agent can be reached: a Unix-domain socket,
*       or the OpenSSH agent's named pipe on Windows                      [3]
*     - the operating system's cryptographically secure random source,
*       for salts, cookies, and key generation                            [4]
*     - the library the SSH modules prefer, and a rolled-up availability
*       gate, D_ENV_SSH_AVAILABLE                                         [5]
*   Like the other net headers, it answers "is this present to call?", never
* "will the peer agree?": which algorithms a server accepts is a runtime fact.
*   Naming: D_ENV_SSH_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_SSH_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_SSH_*
* macro before inclusion to override detection.
*   It requires env_net.h, for the header probe and the socket classification,
* and env.h, for the platform; it includes both itself. It is an opt-in module
* and may be included directly.
*
*
* path:      /inc/djinterp/env/net/ssh/env_ssh.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SSH LIBRARIES
    -------------
    1.  Library headers
         1.  D_ENV_SSH_HAS_LIBSSH2
         2.  D_ENV_SSH_HAS_LIBSSH
         3.  D_ENV_SSH_HAS_LIBSSH_VERSION_H
         4.  D_ENV_SSH_HAS_WOLFSSH
    2.  Versions
         1.  D_CFG_ENV_SSH_PROBE_VERSION
         2.  D_ENV_SSH_LIBSSH2_VERSION
         3.  D_ENV_SSH_LIBSSH_VERSION
2.  LIBRARY FEATURES
    ----------------
    1.  libssh2
         1.  D_ENV_SSH_LIBSSH2_HAS_ED25519
         2.  D_ENV_SSH_LIBSSH2_HAS_RSA_SHA2
         3.  D_ENV_SSH_LIBSSH2_HAS_AES_GCM
         4.  D_ENV_SSH_LIBSSH2_HAS_STRICT_KEX
    2.  libssh
         1.  D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API
3.  AUTHENTICATION AGENT
    --------------------
    1.  Agent transports
         1.  D_ENV_SSH_HAS_SYS_UN_H
         2.  D_ENV_SSH_HAS_AFUNIX_H
         3.  D_ENV_SSH_CAN_REACH_AGENT
4.  SECURE RANDOMNESS
    -----------------
    1.  Random sources
         1.  D_ENV_SSH_RANDOM_*
              1.  D_ENV_SSH_RANDOM_NONE
              2.  D_ENV_SSH_RANDOM_GETRANDOM
              3.  D_ENV_SSH_RANDOM_ARC4RANDOM
              4.  D_ENV_SSH_RANDOM_BCRYPT
              5.  D_ENV_SSH_RANDOM_DEV_URANDOM
         2.  D_ENV_SSH_HAS_SYS_RANDOM_H
         3.  D_ENV_SSH_HAS_BCRYPT_H
         4.  D_ENV_SSH_RANDOM_SOURCE
         5.  D_ENV_SSH_HAS_RANDOM
5.  BACKEND SELECTION
    -----------------
    1.  Backend identifiers
         1.  D_ENV_SSH_BACKEND_*
              1.  D_ENV_SSH_BACKEND_NONE
              2.  D_ENV_SSH_BACKEND_LIBSSH2
              3.  D_ENV_SSH_BACKEND_LIBSSH
              4.  D_ENV_SSH_BACKEND_WOLFSSH
    2.  Usable libraries
         1.  D_ENV_SSH_LIBSSH2_USABLE
         2.  D_ENV_SSH_LIBSSH_USABLE
         3.  D_ENV_SSH_WOLFSSH_USABLE
    3.  Selection
         1.  D_ENV_SSH_BACKEND
         2.  D_ENV_SSH_BACKEND_NAME
         3.  D_ENV_SSH_HAS_BACKEND
         4.  D_ENV_SSH_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_SSH_ENV_SSH_H
#define DJINTERP_ENV_NET_SSH_ENV_SSH_H 1

// djinterp
#include "../../env.h"     // D_ENV_OS_ID, D_ENV_IS_OS_*, D_ENV_OS_FLAG_*
#include "../../../config/cfg_common.h"  // D_CFG_IS_BOOL, for the probe knob
#include "../env_net.h"   // D_ENV_NET_HAS_INCLUDE, D_ENV_NET_CAN_TCP


//==============================================================================
// 1.  SSH LIBRARIES
//==============================================================================
// The libraries are found by their headers. A library's version is read from
// its own version macro, because the header alone says nothing about which
// algorithms the release can negotiate.


// 1.1    Library headers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_SSH_HAS_LIBSSH2
//   feature: <libssh2.h> is includable (libssh2, the BSD-licensed client
// library used by curl and libgit2).
#ifndef D_ENV_SSH_HAS_LIBSSH2
    #if D_ENV_NET_HAS_INCLUDE(<libssh2.h>)
        #define D_ENV_SSH_HAS_LIBSSH2 1
    #else
        #define D_ENV_SSH_HAS_LIBSSH2 0
    #endif
#endif  // D_ENV_SSH_HAS_LIBSSH2

// 1.1.2
// D_ENV_SSH_HAS_LIBSSH
//   feature: <libssh/libssh.h> is includable (libssh, the LGPL client and
// server library).
#ifndef D_ENV_SSH_HAS_LIBSSH
    #if D_ENV_NET_HAS_INCLUDE(<libssh/libssh.h>)
        #define D_ENV_SSH_HAS_LIBSSH 1
    #else
        #define D_ENV_SSH_HAS_LIBSSH 0
    #endif
#endif  // D_ENV_SSH_HAS_LIBSSH

// 1.1.3
// D_ENV_SSH_HAS_LIBSSH_VERSION_H
//   feature: <libssh/libssh_version.h> is includable. libssh 0.10 moved its
// version macros into this light header; older releases define them only in
// <libssh/libssh.h>, whose version is therefore reported as unknown.
#ifndef D_ENV_SSH_HAS_LIBSSH_VERSION_H
    #if D_ENV_NET_HAS_INCLUDE(<libssh/libssh_version.h>)
        #define D_ENV_SSH_HAS_LIBSSH_VERSION_H 1
    #else
        #define D_ENV_SSH_HAS_LIBSSH_VERSION_H 0
    #endif
#endif  // D_ENV_SSH_HAS_LIBSSH_VERSION_H

// 1.1.4
// D_ENV_SSH_HAS_WOLFSSH
//   feature: <wolfssh/ssh.h> is includable (wolfSSH, built on wolfSSL, which
// like wolfSSL needs its build settings included first).
#ifndef D_ENV_SSH_HAS_WOLFSSH
    #if D_ENV_NET_HAS_INCLUDE(<wolfssh/ssh.h>)
        #define D_ENV_SSH_HAS_WOLFSSH 1
    #else
        #define D_ENV_SSH_HAS_WOLFSSH 0
    #endif
#endif  // D_ENV_SSH_HAS_WOLFSSH

// 1.2    Versions
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_ENV_SSH_PROBE_VERSION
//   knob: 1 (the default) to read library versions by including <libssh2.h>
// and <libssh/libssh_version.h>; 0 to keep both out of every translation unit
// that includes this header, leaving each version at 0 (unknown). On Windows,
// <libssh2.h> brings <winsock2.h> with it.
#ifndef D_CFG_ENV_SSH_PROBE_VERSION
    #define D_CFG_ENV_SSH_PROBE_VERSION 1
#endif  // D_CFG_ENV_SSH_PROBE_VERSION
#if !D_CFG_IS_BOOL(D_CFG_ENV_SSH_PROBE_VERSION)
    #error "D_CFG_ENV_SSH_PROBE_VERSION must be 0 or 1"
#endif

// 1.2.2
// D_ENV_SSH_LIBSSH2_VERSION
//   value: libssh2's LIBSSH2_VERSION_NUM, 0xMMmmpp (0x010b00 is 1.11.0), or 0
// when libssh2 is absent or its version was not probed.
#ifndef D_ENV_SSH_LIBSSH2_VERSION
    #if ( (D_CFG_IS_ON(D_CFG_ENV_SSH_PROBE_VERSION)) &&                        \
          (D_ENV_SSH_HAS_LIBSSH2) )
        #include <libssh2.h>  // LIBSSH2_VERSION_NUM
        #if defined(LIBSSH2_VERSION_NUM)
            #define D_ENV_SSH_LIBSSH2_VERSION LIBSSH2_VERSION_NUM
        #else
            #define D_ENV_SSH_LIBSSH2_VERSION 0
        #endif
    #else
        #define D_ENV_SSH_LIBSSH2_VERSION 0
    #endif
#endif  // D_ENV_SSH_LIBSSH2_VERSION

// 1.2.3
// D_ENV_SSH_LIBSSH_VERSION
//   value: libssh's LIBSSH_VERSION_INT, 0xMMmmpp (0x000a06 is 0.10.6), or 0
// when libssh is absent, predates <libssh/libssh_version.h>, or its version
// was not probed.
#ifndef D_ENV_SSH_LIBSSH_VERSION
    #if ( (D_CFG_IS_ON(D_CFG_ENV_SSH_PROBE_VERSION)) &&                        \
          (D_ENV_SSH_HAS_LIBSSH_VERSION_H) )
        #include <libssh/libssh_version.h>  // LIBSSH_VERSION_INT
        #if defined(LIBSSH_VERSION_INT)
            #define D_ENV_SSH_LIBSSH_VERSION LIBSSH_VERSION_INT
        #else
            #define D_ENV_SSH_LIBSSH_VERSION 0
        #endif
    #else
        #define D_ENV_SSH_LIBSSH_VERSION 0
    #endif
#endif  // D_ENV_SSH_LIBSSH_VERSION


//==============================================================================
// 2.  LIBRARY FEATURES
//==============================================================================
// Features that arrived in a known release. A version test cannot see a fix a
// distribution backported, so each flag can read 0 on a patched older build;
// a project that knows better pre-defines it.


// 2.1    libssh2
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_SSH_LIBSSH2_HAS_ED25519
//   feature: 1 if libssh2 is 1.9.0 or later, the first release to verify
// Ed25519 host keys and authenticate with Ed25519 user keys.
#ifndef D_ENV_SSH_LIBSSH2_HAS_ED25519
    #if (D_ENV_SSH_LIBSSH2_VERSION >= 0x010900)
        #define D_ENV_SSH_LIBSSH2_HAS_ED25519 1
    #else
        #define D_ENV_SSH_LIBSSH2_HAS_ED25519 0
    #endif
#endif  // D_ENV_SSH_LIBSSH2_HAS_ED25519

// 2.1.2
// D_ENV_SSH_LIBSSH2_HAS_RSA_SHA2
//   feature: 1 if libssh2 is 1.10.0 or later, the first release to speak
// rsa-sha2-256 and rsa-sha2-512. Without them, an OpenSSH 8.8 or later server
// that holds only an RSA host key cannot be verified: those servers no longer
// sign with SHA-1.
#ifndef D_ENV_SSH_LIBSSH2_HAS_RSA_SHA2
    #if (D_ENV_SSH_LIBSSH2_VERSION >= 0x010a00)
        #define D_ENV_SSH_LIBSSH2_HAS_RSA_SHA2 1
    #else
        #define D_ENV_SSH_LIBSSH2_HAS_RSA_SHA2 0
    #endif
#endif  // D_ENV_SSH_LIBSSH2_HAS_RSA_SHA2

// 2.1.3
// D_ENV_SSH_LIBSSH2_HAS_AES_GCM
//   feature: 1 if libssh2 is 1.11.0 or later, the first release with the
// aes128-gcm@openssh.com and aes256-gcm@openssh.com ciphers, where its crypto
// backend implements them.
#ifndef D_ENV_SSH_LIBSSH2_HAS_AES_GCM
    #if (D_ENV_SSH_LIBSSH2_VERSION >= 0x010b00)
        #define D_ENV_SSH_LIBSSH2_HAS_AES_GCM 1
    #else
        #define D_ENV_SSH_LIBSSH2_HAS_AES_GCM 0
    #endif
#endif  // D_ENV_SSH_LIBSSH2_HAS_AES_GCM

// 2.1.4
// D_ENV_SSH_LIBSSH2_HAS_STRICT_KEX
//   feature: 1 if libssh2 is 1.11.1 or later, the first release with strict
// key exchange, the countermeasure to the Terrapin attack. Without it, the
// ChaCha20-Poly1305 cipher and the encrypt-then-MAC modes are exposed to
// prefix truncation, which is why the SSH modules prefer AES-GCM and AES-CTR
// with plain SHA-2 MACs.
#ifndef D_ENV_SSH_LIBSSH2_HAS_STRICT_KEX
    #if (D_ENV_SSH_LIBSSH2_VERSION >= 0x010b01)
        #define D_ENV_SSH_LIBSSH2_HAS_STRICT_KEX 1
    #else
        #define D_ENV_SSH_LIBSSH2_HAS_STRICT_KEX 0
    #endif
#endif  // D_ENV_SSH_LIBSSH2_HAS_STRICT_KEX

// 2.2    libssh
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API
//   feature: 1 if libssh is 0.8.0 or later, which introduced the known-hosts
// and public-key API (ssh_session_is_known_server, ssh_get_server_publickey)
// that an adapter needs to expose the server's host key.
#ifndef D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API
    #if (D_ENV_SSH_LIBSSH_VERSION >= 0x000800)
        #define D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API 1
    #else
        #define D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API 0
    #endif
#endif  // D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API


//==============================================================================
// 3.  AUTHENTICATION AGENT
//==============================================================================
// An agent holds private keys so that no program using them has to. Where it
// listens is a platform fact: OpenSSH's ssh-agent names a Unix-domain socket
// in SSH_AUTH_SOCK, and OpenSSH for Windows listens on the named pipe
// \\.\pipe\openssh-ssh-agent.


// 3.1    Agent transports
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_SSH_HAS_SYS_UN_H
//   feature: <sys/un.h>, which declares the Unix-domain socket address, is
// includable.
#ifndef D_ENV_SSH_HAS_SYS_UN_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/un.h>)
            #define D_ENV_SSH_HAS_SYS_UN_H 1
        #else
            #define D_ENV_SSH_HAS_SYS_UN_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_SSH_HAS_SYS_UN_H 1
    #else
        #define D_ENV_SSH_HAS_SYS_UN_H 0
    #endif
#endif  // D_ENV_SSH_HAS_SYS_UN_H

// 3.1.2
// D_ENV_SSH_HAS_AFUNIX_H
//   feature: <afunix.h> is includable (Windows 10 SDKs from build 17063),
// giving Winsock Unix-domain sockets as well.
#ifndef D_ENV_SSH_HAS_AFUNIX_H
    #if D_ENV_NET_HAS_INCLUDE(<afunix.h>)
        #define D_ENV_SSH_HAS_AFUNIX_H 1
    #else
        #define D_ENV_SSH_HAS_AFUNIX_H 0
    #endif
#endif  // D_ENV_SSH_HAS_AFUNIX_H

// 3.1.3
// D_ENV_SSH_CAN_REACH_AGENT
//   feature: 1 if an agent could be reached here: over a Unix-domain socket
// where <sys/un.h> exists, and over the named pipe, which needs no header of
// its own, on Windows. Whether one is running is a runtime fact.
#ifndef D_ENV_SSH_CAN_REACH_AGENT
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_SSH_CAN_REACH_AGENT 1
    #elif D_ENV_SSH_HAS_SYS_UN_H
        #define D_ENV_SSH_CAN_REACH_AGENT 1
    #else
        #define D_ENV_SSH_CAN_REACH_AGENT 0
    #endif
#endif  // D_ENV_SSH_CAN_REACH_AGENT


//==============================================================================
// 4.  SECURE RANDOMNESS
//==============================================================================
// SSH needs unpredictable bytes outside its transport too: the salt of a
// hashed known_hosts entry, and every key a module generates. Each platform
// has one source it guarantees to be cryptographically secure, and it is
// chosen here once, with /dev/urandom as the last resort on other POSIX-like
// systems.


// 4.1    Random sources
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_SSH_RANDOM_*
//   constant: identifiers for D_ENV_SSH_RANDOM_SOURCE.
// 4.1.1.1
// D_ENV_SSH_RANDOM_NONE
//   constant: no secure random source is known.
#define D_ENV_SSH_RANDOM_NONE        0

// 4.1.1.2
// D_ENV_SSH_RANDOM_GETRANDOM
//   constant: getrandom() from <sys/random.h> (Linux).
#define D_ENV_SSH_RANDOM_GETRANDOM   1

// 4.1.1.3
// D_ENV_SSH_RANDOM_ARC4RANDOM
//   constant: arc4random_buf() from <stdlib.h> (Apple, the BSDs, Android).
#define D_ENV_SSH_RANDOM_ARC4RANDOM  2

// 4.1.1.4
// D_ENV_SSH_RANDOM_BCRYPT
//   constant: BCryptGenRandom() from <bcrypt.h> (Windows; links bcrypt).
#define D_ENV_SSH_RANDOM_BCRYPT      3

// 4.1.1.5
// D_ENV_SSH_RANDOM_DEV_URANDOM
//   constant: reads from /dev/urandom (other POSIX-like systems).
#define D_ENV_SSH_RANDOM_DEV_URANDOM 4

// 4.1.2
// D_ENV_SSH_HAS_SYS_RANDOM_H
//   feature: <sys/random.h> is includable. It declares getrandom() on Linux
// (glibc 2.25 and musl 1.1.20 onward) but only getentropy() on Apple, so it
// selects a source only together with the platform.
#ifndef D_ENV_SSH_HAS_SYS_RANDOM_H
    #if D_ENV_NET_HAS_INCLUDE(<sys/random.h>)
        #define D_ENV_SSH_HAS_SYS_RANDOM_H 1
    #else
        #define D_ENV_SSH_HAS_SYS_RANDOM_H 0
    #endif
#endif  // D_ENV_SSH_HAS_SYS_RANDOM_H

// 4.1.3
// D_ENV_SSH_HAS_BCRYPT_H
//   feature: <bcrypt.h>, the Windows CNG header that declares
// BCryptGenRandom, is includable.
#ifndef D_ENV_SSH_HAS_BCRYPT_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<bcrypt.h>)
            #define D_ENV_SSH_HAS_BCRYPT_H 1
        #else
            #define D_ENV_SSH_HAS_BCRYPT_H 0
        #endif
    #elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_SSH_HAS_BCRYPT_H 1
    #else
        #define D_ENV_SSH_HAS_BCRYPT_H 0
    #endif
#endif  // D_ENV_SSH_HAS_BCRYPT_H

// 4.1.4
// D_ENV_SSH_RANDOM_SOURCE
//   value: the secure random source, as a D_ENV_SSH_RANDOM_* identifier.
// Windows uses CNG; Apple, iOS, the BSDs, and Android use arc4random_buf,
// which each of them guarantees; Linux uses getrandom; any other POSIX-like
// system reads /dev/urandom.
#ifndef D_ENV_SSH_RANDOM_SOURCE
    #if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) &&                                \
          (D_ENV_SSH_HAS_BCRYPT_H) )
        #define D_ENV_SSH_RANDOM_SOURCE D_ENV_SSH_RANDOM_BCRYPT
    #elif ( (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)) ||                   \
            (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)) ||                   \
            (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x9)) ||                   \
            (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0xA)) )
        #define D_ENV_SSH_RANDOM_SOURCE D_ENV_SSH_RANDOM_ARC4RANDOM
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX) &&                            \
            (D_ENV_SSH_HAS_SYS_RANDOM_H) )
        #define D_ENV_SSH_RANDOM_SOURCE D_ENV_SSH_RANDOM_GETRANDOM
    #elif D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_SSH_RANDOM_SOURCE D_ENV_SSH_RANDOM_DEV_URANDOM
    #else
        #define D_ENV_SSH_RANDOM_SOURCE D_ENV_SSH_RANDOM_NONE
    #endif
#endif  // D_ENV_SSH_RANDOM_SOURCE

// 4.1.5
// D_ENV_SSH_HAS_RANDOM
//   feature: 1 if a secure random source is known.
#ifndef D_ENV_SSH_HAS_RANDOM
    #if (D_ENV_SSH_RANDOM_SOURCE != D_ENV_SSH_RANDOM_NONE)
        #define D_ENV_SSH_HAS_RANDOM 1
    #else
        #define D_ENV_SSH_HAS_RANDOM 0
    #endif
#endif  // D_ENV_SSH_HAS_RANDOM


//==============================================================================
// 5.  BACKEND SELECTION
//==============================================================================
// A library is usable when it is recent enough to meet today's servers: it
// must verify the host-key types they present. libssh2 is preferred, because
// the SSH modules ship an engine for it.


// 5.1    Backend identifiers
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_SSH_BACKEND_*
//   constant: identifiers for D_ENV_SSH_BACKEND.
// 5.1.1.1
// D_ENV_SSH_BACKEND_NONE
//   constant: no usable SSH library.
#define D_ENV_SSH_BACKEND_NONE    0

// 5.1.1.2
// D_ENV_SSH_BACKEND_LIBSSH2
//   constant: libssh2.
#define D_ENV_SSH_BACKEND_LIBSSH2 1

// 5.1.1.3
// D_ENV_SSH_BACKEND_LIBSSH
//   constant: libssh.
#define D_ENV_SSH_BACKEND_LIBSSH  2

// 5.1.1.4
// D_ENV_SSH_BACKEND_WOLFSSH
//   constant: wolfSSH.
#define D_ENV_SSH_BACKEND_WOLFSSH 3

// 5.2    Usable libraries
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_SSH_LIBSSH2_USABLE
//   feature: 1 if libssh2 is present at 1.9.0 or later, which verifies every
// host-key type OpenSSH generates by default. Reads 0 while the version is
// unknown.
#ifndef D_ENV_SSH_LIBSSH2_USABLE
    #if ( (D_ENV_SSH_HAS_LIBSSH2) &&                                           \
          (D_ENV_SSH_LIBSSH2_HAS_ED25519) )
        #define D_ENV_SSH_LIBSSH2_USABLE 1
    #else
        #define D_ENV_SSH_LIBSSH2_USABLE 0
    #endif
#endif  // D_ENV_SSH_LIBSSH2_USABLE

// 5.2.2
// D_ENV_SSH_LIBSSH_USABLE
//   feature: 1 if libssh is present with its known-hosts and public-key API.
#ifndef D_ENV_SSH_LIBSSH_USABLE
    #if ( (D_ENV_SSH_HAS_LIBSSH) &&                                            \
          (D_ENV_SSH_LIBSSH_HAS_KNOWN_HOSTS_API) )
        #define D_ENV_SSH_LIBSSH_USABLE 1
    #else
        #define D_ENV_SSH_LIBSSH_USABLE 0
    #endif
#endif  // D_ENV_SSH_LIBSSH_USABLE

// 5.2.3
// D_ENV_SSH_WOLFSSH_USABLE
//   feature: 1 if wolfSSH is present. It carries no version gate here: every
// release takes caller-supplied I/O callbacks and exposes the host key.
#ifndef D_ENV_SSH_WOLFSSH_USABLE
    #if D_ENV_SSH_HAS_WOLFSSH
        #define D_ENV_SSH_WOLFSSH_USABLE 1
    #else
        #define D_ENV_SSH_WOLFSSH_USABLE 0
    #endif
#endif  // D_ENV_SSH_WOLFSSH_USABLE

// 5.3    Selection
//------------------------------------------------------------------------------
// 5.3.1
// D_ENV_SSH_BACKEND
//   value: the library the SSH modules use, as a D_ENV_SSH_BACKEND_*
// identifier: the first usable one of libssh2, libssh, and wolfSSH.
#ifndef D_ENV_SSH_BACKEND
    #if D_ENV_SSH_LIBSSH2_USABLE
        #define D_ENV_SSH_BACKEND D_ENV_SSH_BACKEND_LIBSSH2
    #elif D_ENV_SSH_LIBSSH_USABLE
        #define D_ENV_SSH_BACKEND D_ENV_SSH_BACKEND_LIBSSH
    #elif D_ENV_SSH_WOLFSSH_USABLE
        #define D_ENV_SSH_BACKEND D_ENV_SSH_BACKEND_WOLFSSH
    #else
        #define D_ENV_SSH_BACKEND D_ENV_SSH_BACKEND_NONE
    #endif
#endif  // D_ENV_SSH_BACKEND

// 5.3.2
// D_ENV_SSH_BACKEND_NAME
//   value: a human-readable name for the library the SSH modules use.
#ifndef D_ENV_SSH_BACKEND_NAME
    #if (D_ENV_SSH_BACKEND == D_ENV_SSH_BACKEND_LIBSSH2)
        #define D_ENV_SSH_BACKEND_NAME "libssh2"
    #elif (D_ENV_SSH_BACKEND == D_ENV_SSH_BACKEND_LIBSSH)
        #define D_ENV_SSH_BACKEND_NAME "libssh"
    #elif (D_ENV_SSH_BACKEND == D_ENV_SSH_BACKEND_WOLFSSH)
        #define D_ENV_SSH_BACKEND_NAME "wolfSSH"
    #else
        #define D_ENV_SSH_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_SSH_BACKEND_NAME

// 5.3.3
// D_ENV_SSH_HAS_BACKEND
//   feature: 1 if any usable SSH library is present.
#ifndef D_ENV_SSH_HAS_BACKEND
    #if (D_ENV_SSH_BACKEND != D_ENV_SSH_BACKEND_NONE)
        #define D_ENV_SSH_HAS_BACKEND 1
    #else
        #define D_ENV_SSH_HAS_BACKEND 0
    #endif
#endif  // D_ENV_SSH_HAS_BACKEND

// 5.3.4
// D_ENV_SSH_AVAILABLE
//   feature: 1 if an SSH connection can actually be made here: a usable
// library plus a TCP transport. The gate an SSH module should #error on when
// it cannot work without one.
#ifndef D_ENV_SSH_AVAILABLE
    #if ( (D_ENV_SSH_HAS_BACKEND) &&                                           \
          (D_ENV_NET_CAN_TCP) )
        #define D_ENV_SSH_AVAILABLE 1
    #else
        #define D_ENV_SSH_AVAILABLE 0
    #endif
#endif  // D_ENV_SSH_AVAILABLE


#endif  // DJINTERP_ENV_NET_SSH_ENV_SSH_H
