/*******************************************************************************
* djinterp [net]                                                    ssh_engine.h
*
* The SSH engine interface: the operations an SSH library provides.
*   An engine owns the SSH transport -- key exchange, encryption, framing
* -- and nothing more; host-key trust, the order of authentication
* methods, and when a session has failed are decided by the session, the
* same for every engine. The library-wide lifetime calls and the default
* engine are here; each built-in engine has its own header.
*
*
* path:      /inc/djinterp/net/ssh/ssh_engine.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Streams
         1.  d_ssh_stream
    2.  Engines
         1.  fn_ssh_prompt
         2.  d_ssh_algorithms
         3.  d_ssh_engine_vtable
2.  CONSTANTS
    ---------
    1.  Algorithm lists
         1.  D_SSH_HOST_KEY_ALGORITHMS
         2.  D_SSH_CIPHERS
         3.  D_SSH_MACS
3.  ENGINES
    -------
    1.  Library lifetime
    2.  Default engine
*/

#ifndef DJINTERP_NET_SSH_SSH_ENGINE_H
#define DJINTERP_NET_SSH_SSH_ENGINE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_common.h"      // d_ssh_status, d_ssh_socket


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Streams
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_stream
//   enum: which of a channel's two inbound streams to read.
enum d_ssh_stream
{
    D_SSH_STREAM_STDOUT = 0,  // channel data
    D_SSH_STREAM_STDERR = 1   // extended data of type stderr
};

// 1.2    Engines
//------------------------------------------------------------------------------
// 1.2.1
// fn_ssh_prompt
//   typedef: answers one keyboard-interactive prompt. `_prompt` is the
// server's text, NUL-terminated and untrusted: display it, never interpret it.
// `_echo` says whether the answer may be shown as it is typed. Writes a
// NUL-terminated answer of fewer than `_capacity` bytes to `_answer` and
// returns true, or returns false to abandon the method.
typedef bool (*fn_ssh_prompt)(void*       _context,
                              const char* _prompt,
                              bool        _echo,
                              char*       _answer,
                              size_t      _capacity);

// 1.2.2
// d_ssh_algorithms
//   struct: algorithm preferences, each an SSH name-list in order of
// preference, or NULL for its default: the library's for `kex`;
// D_SSH_HOST_KEY_ALGORITHMS for `host_keys`; and D_SSH_CIPHERS and D_SSH_MACS
// for `ciphers` and `macs` where D_CFG_SSH_RESTRICT_ALGORITHMS is on, else
// the library's. Before the handshake, `host_keys` is reordered so that the
// key types known_hosts records for the host come first, as OpenSSH does.
// Names the library does not implement are dropped; a list left with none
// fails the connection with D_SSH_ERR_UNSUPPORTED.
struct d_ssh_algorithms
{
    const char* kex;        // key exchange
    const char* host_keys;  // host-key signature algorithms
    const char* ciphers;    // ciphers, both directions
    const char* macs;       // MACs, both directions
};

// 1.2.3
// d_ssh_engine_vtable
//   struct: the operations an SSH library provides. An engine owns the SSH
// transport -- key exchange, encryption, framing -- and nothing more: whether
// the host key is trusted, which methods are tried, and when a session has
// failed are decided here, the same for every engine. `_handle` is one engine
// session and `_channel` one engine channel. Each operation returns
// D_SSH_ERR_AUTH_DENIED for a refused credential, D_SSH_ERR_KEY for a key file
// it cannot load, D_SSH_ERR_CHANNEL for a refused channel or request,
// D_SSH_ERR_CLOSED from channel_read at the end of a stream, and
// D_SSH_ERR_UNSUPPORTED for a missing capability, such as auth_agent with no
// agent running. `channel_pty` and `detail` may be NULL; the rest are
// required.
struct d_ssh_engine_vtable
{
    // a short name, such as "libssh2", for diagnostics
    const char* name;

    // makes a session that applies `_algorithms` -- a NULL member keeps the
    // library's default -- and the deadline (0 for none) to every operation
    enum d_ssh_status (*create)(const struct d_ssh_algorithms* _algorithms,
                                unsigned int                   _timeout_ms,
                                void**                         _handle);

    // exchanges identification strings and keys over the socket
    enum d_ssh_status (*handshake)(void*        _handle,
                                   d_ssh_socket _socket);

    // exposes the server's host-key blob, valid until destroy
    enum d_ssh_status (*host_key)(void*                 _handle,
                                  const unsigned char** _key,
                                  size_t*               _size);

    // the methods the server accepts for `_user`, as a name-list valid until
    // the next call, or NULL when "none" authentication has already succeeded
    enum d_ssh_status (*auth_methods)(void*        _handle,
                                      const char*  _user,
                                      const char** _methods);

    // password authentication
    enum d_ssh_status (*auth_password)(void*       _handle,
                                       const char* _user,
                                       const char* _password);

    // public-key authentication from files; `_public_key` and `_passphrase`
    // may be NULL
    enum d_ssh_status (*auth_key_file)(void*       _handle,
                                       const char* _user,
                                       const char* _public_key,
                                       const char* _private_key,
                                       const char* _passphrase);

    // public-key authentication with each key the agent holds
    enum d_ssh_status (*auth_agent)(void*       _handle,
                                    const char* _user);

    // keyboard-interactive authentication, answering through `_prompt`
    enum d_ssh_status (*auth_interactive)(void*         _handle,
                                          const char*   _user,
                                          fn_ssh_prompt _prompt,
                                          void*         _context);

    // a session channel when `_host` is NULL; otherwise a direct-tcpip
    // channel to `_host` and `_port`
    enum d_ssh_status (*channel_open)(void*        _handle,
                                      const char*  _host,
                                      unsigned int _port,
                                      void**       _channel);

    // "exec" with a command, "subsystem" with a name, or "shell" with NULL
    enum d_ssh_status (*channel_request)(void*       _channel,
                                         const char* _type,
                                         const char* _value);

    // a pseudo-terminal of `_columns` by `_rows` characters
    enum d_ssh_status (*channel_pty)(void*        _channel,
                                     const char*  _term,
                                     unsigned int _columns,
                                     unsigned int _rows);

    // at least one byte of `_stream`, or D_SSH_ERR_CLOSED at its end
    enum d_ssh_status (*channel_read)(void*             _channel,
                                      enum d_ssh_stream _stream,
                                      void*             _buffer,
                                      size_t            _capacity,
                                      size_t*           _received);

    // at least one byte of `_data`
    enum d_ssh_status (*channel_write)(void*       _channel,
                                       const void* _data,
                                       size_t      _size,
                                       size_t*     _sent);

    // tells the server no more data follows
    enum d_ssh_status (*channel_send_eof)(void* _channel);

    // closes, waits for the server's close, and reports the exit status
    enum d_ssh_status (*channel_close)(void* _channel,
                                       int*  _exit_status);

    // releases a channel, closed or not
    void (*channel_free)(void* _channel);

    // sends SSH_MSG_DISCONNECT with `_reason`
    enum d_ssh_status (*disconnect)(void*       _handle,
                                    const char* _reason);

    // a description of the last failure, valid until destroy, or NULL
    const char* (*detail)(void* _handle);

    // releases the session and every channel not yet freed, whose handle is
    // never used again; never closes the socket
    void (*destroy)(void* _handle);
};


//==============================================================================
// 2.  CONSTANTS
//==============================================================================


// 2.1    Algorithm lists
//------------------------------------------------------------------------------
// 2.1.1
// D_SSH_HOST_KEY_ALGORITHMS
//   constant: the default host-key algorithms, OpenSSH's order without the
// SHA-1 "ssh-rsa" and DSA signatures that OpenSSH 8.8 retired.
#define D_SSH_HOST_KEY_ALGORITHMS                                              \
    "ssh-ed25519,"                                                             \
    "ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,"             \
    "rsa-sha2-512,rsa-sha2-256"

// 2.1.2
// D_SSH_CIPHERS
//   constant: the restricted cipher list: AES-GCM, then AES-CTR. It leaves
// out chacha20-poly1305@openssh.com, which the Terrapin attack truncates when
// strict key exchange is missing, and the CBC modes.
#define D_SSH_CIPHERS                                                          \
    "aes256-gcm@openssh.com,aes128-gcm@openssh.com,"                           \
    "aes256-ctr,aes192-ctr,aes128-ctr"

// 2.1.3
// D_SSH_MACS
//   constant: the restricted MAC list, used with AES-CTR: SHA-2 HMACs in
// their original mode, leaving out the encrypt-then-MAC variants Terrapin
// targets and the SHA-1 and MD5 ones.
#define D_SSH_MACS "hmac-sha2-512,hmac-sha2-256"


//==============================================================================
// 3.  ENGINES
//==============================================================================


// 3.1    Library lifetime
//------------------------------------------------------------------------------
/**
 * @brief Initializes the built-in engines' libraries.
 *
 * @note optional -- engines initialize on first use -- but that first use is
 *       not thread-safe, so a program that starts sessions on several threads
 *       calls this once first.
 *
 * @return `D_SSH_OK`, including when no engine is compiled; or
 *         `D_SSH_ERR_UNSUPPORTED` if a library failed to initialize its
 *         cryptography.
 */
D_NODISCARD enum d_ssh_status
d_ssh_library_init(void);
/**
 * @brief Releases what d_ssh_library_init acquired.
 *
 * @pre no session is in use on any thread.
 */
void
d_ssh_library_cleanup(void);

// 3.2    Default engine
//------------------------------------------------------------------------------
/**
 * @brief Returns the preferred built-in engine.
 *
 * @return the libssh2 engine where compiled; otherwise `NULL`.
 */
D_NODISCARD const struct d_ssh_engine_vtable*
d_ssh_engine_default(void);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_ENGINE_H
