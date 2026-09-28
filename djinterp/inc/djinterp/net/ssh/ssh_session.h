/*******************************************************************************
* djinterp [net]                                                   ssh_session.h
*
* The SSH session: one connection, from key exchange to disconnect.
*   A session runs over a connected stream socket the caller supplies. Its
* handshake verifies the server's host key against known_hosts, under the
* chosen policy, before anything else is possible, so no credential is
* ever offered to an unverified server. ssh_auth.h then authenticates it,
* and ssh_channel.h opens channels on it.
*
*
* path:      /inc/djinterp/net/ssh/ssh_session.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Modes
         1.  d_ssh_state
         2.  d_ssh_host_key_policy
    2.  Setup
         1.  fn_ssh_host_key_decision
         2.  d_ssh_options
    3.  Sessions
         1.  d_ssh_session
2.  SESSION
    -------
    1.  Defaults
    2.  Lifecycle
3.  QUERIES
    -------
    1.  State
    2.  Names
*/

#ifndef DJINTERP_NET_SSH_SSH_SESSION_H
#define DJINTERP_NET_SSH_SSH_SESSION_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"   // framework root
#include "./ssh_common.h"       // d_ssh_status, d_ssh_socket
#include "./ssh_engine.h"       // d_ssh_engine_vtable
#include "./ssh_key.h"          // D_SSH_*_SIZE
#include "./ssh_known_hosts.h"  // d_ssh_host_match


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Modes
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_state
//   enum: where a session is. It only moves forward: a session reaches
// AUTHENTICATED only through VERIFIED, and VERIFIED only by accepting the
// host key, so no credential is ever offered to an unverified server.
enum d_ssh_state
{
    D_SSH_STATE_NEW           = 0,  // initialized; nothing sent yet
    D_SSH_STATE_VERIFIED      = 1,  // keys exchanged, host key accepted
    D_SSH_STATE_AUTHENTICATED = 2,  // the user is authenticated
    D_SSH_STATE_FAILED        = 3,  // a fatal error ended it
    D_SSH_STATE_CLOSED        = 4   // disconnected or destroyed
};

// 1.1.2
// d_ssh_host_key_policy
//   enum: how a host key is judged. Whatever the policy, a key that differs
// from every key recorded for the host, or one marked @revoked, fails the
// connection -- except under INSECURE_ACCEPT_ANY, which consults nothing.
// Numbering starts at 1 so that a zero-initialized options struct is rejected
// rather than read as a policy.
enum d_ssh_host_key_policy
{
    D_SSH_HOST_KEY_STRICT              = 1,  // only recorded keys
    D_SSH_HOST_KEY_ACCEPT_NEW          = 2,  // record unrecorded hosts
    D_SSH_HOST_KEY_CALLBACK            = 3,  // the program decides new hosts
    D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY = 4   // accept anything: tests only
};

// 1.2    Setup
//------------------------------------------------------------------------------
// 1.2.1
// fn_ssh_host_key_decision
//   typedef: decides a host no known_hosts file records, under
// D_SSH_HOST_KEY_CALLBACK. It receives the host and port being verified, the
// key's type, and its fingerprint in OpenSSH's form, to be compared with one
// obtained out of band. Returns true to accept the key -- setting *_remember
// to true to record it in the user's known_hosts -- or false to refuse it.
typedef bool (*fn_ssh_host_key_decision)(void*        _context,
                                         const char*  _host,
                                         unsigned int _port,
                                         const char*  _key_type,
                                         const char*  _fingerprint,
                                         bool*        _remember);

// 1.2.2
// d_ssh_options
//   struct: how to set up a session; start from d_ssh_options_default and set
// `host`. `host` is the name the caller connected to and the name keys are
// recorded under; it is never resolved. `known_hosts` NULL means the user's
// .ssh/known_hosts under $HOME (%USERPROFILE% on Windows), the file new keys
// are recorded in; `system_known_hosts` is only read, and NULL skips it.
// d_ssh_init copies every string, so none needs to outlive the call.
struct d_ssh_options
{
    const struct d_ssh_engine_vtable* engine;              // not owned
    const char*                       host;                // name to verify
    unsigned int                      port;                // 1 to 65535
    enum d_ssh_host_key_policy        host_key_policy;     // trust rule
    const char*                       known_hosts;         // user file
    const char*                       system_known_hosts;  // read-only file
    bool                              hash_known_hosts;    // record hashed
    fn_ssh_host_key_decision          decide;              // CALLBACK only
    void*                             decide_context;      // passed to decide
    struct d_ssh_algorithms           algorithms;          // NULL: defaults
    unsigned int                      timeout_ms;          // 0: no deadline
};

// 1.3    Sessions
//------------------------------------------------------------------------------
// 1.3.1
// d_ssh_session
//   struct: one SSH connection. The caller owns the storage -- usually as the
// first member of a service module's own session -- and treats every field
// as read-only. `strings` is one allocation holding the copies that `host`,
// the file paths, and the algorithm lists point into. `host_key_type` and
// `fingerprint` stay empty until a host key arrives. `detail` describes the
// last failure or refusal: static text, or the engine's description copied
// into `message`, so that it outlives the engine.
struct d_ssh_session
{
    const struct d_ssh_engine_vtable* engine;              // not owned
    void*                             handle;              // engine session
    d_ssh_socket                      socket;              // the connection
    bool                              owns_socket;         // closed at end
    enum d_ssh_state                  state;               // where it is
    enum d_ssh_host_key_policy        policy;              // trust rule
    bool                              hash_known_hosts;    // record hashed
    unsigned int                      timeout_ms;          // 0: no deadline
    char*                             strings;             // owned copies
    const char*                       host;                // lowercase
    unsigned int                      port;                // server port
    const char*                       known_hosts;         // or NULL
    const char*                       system_known_hosts;  // or NULL
    struct d_ssh_algorithms           algorithms;          // as configured
    fn_ssh_host_key_decision          decide;              // CALLBACK only
    void*                             decide_context;      // passed to decide
    unsigned char*                    host_key;            // blob; owned
    size_t                            host_key_size;       // its length
    char                              host_key_type[D_SSH_KEY_TYPE_SIZE];
    char                              fingerprint[D_SSH_FINGERPRINT_SIZE];
    enum d_ssh_host_match             host_match;          // the verdict
    const char*                       detail;              // or NULL
    char                              message[128];        // detail's copy
};


//==============================================================================
// 2.  SESSION
//==============================================================================


// 2.1    Defaults
//------------------------------------------------------------------------------
/**
 * @brief Returns options holding the configured defaults (cfg_ssh.h): the
 *        default engine, port 22, the configured host-key policy, recording
 *        and system-file settings, default algorithms, and deadline.
 *
 * @return the options; `host` is `NULL` and must be set.
 */
D_NODISCARD struct d_ssh_options
d_ssh_options_default(void);

// 2.2    Lifecycle
//------------------------------------------------------------------------------
/**
 * @brief Prepares a session over a connected socket; sends nothing.
 *
 * @note sets SO_NOSIGPIPE on the socket where the platform defines it.
 *
 * @param[out] _session  the session to initialize.
 * @param[in]  _options  the options; copied.
 * @param[in]  _socket   a connected, blocking stream socket.
 * @param[in]  _owns     `true` if the session closes `_socket` when it ends.
 * @return `D_SSH_OK`; `D_SSH_ERR_NO_BACKEND` if `engine` is `NULL`;
 *         `D_SSH_ERR_FORMAT` if `host` holds whitespace or a character
 *         known_hosts reserves; `D_SSH_ERR_MEMORY`; or `D_SSH_ERR_ARGUMENT`
 *         if a pointer, the port, the policy, or an engine operation is
 *         missing or invalid, or CALLBACK lacks `decide`. On failure the
 *         session is left closed and the socket untouched.
 */
D_NODISCARD enum d_ssh_status
d_ssh_init(struct d_ssh_session*       _session,
           const struct d_ssh_options* _options,
           d_ssh_socket                _socket,
           bool                        _owns);
/**
 * @brief Runs the handshake and verifies the host key under the session's
 *        policy.
 *
 * @note the host key and its fingerprint are available from the queries even
 *       when verification fails, to show whoever must investigate.
 * @note under ACCEPT_NEW, and under CALLBACK when the callback asks, a new
 *       key is recorded in the user's known_hosts before this returns; if it
 *       cannot be, the connection fails rather than go on unrecorded.
 *
 * @param[in,out] _session  a D_SSH_STATE_NEW session.
 * @return `D_SSH_OK`, leaving the session D_SSH_STATE_VERIFIED;
 *         `D_SSH_ERR_HOST_UNKNOWN`, `D_SSH_ERR_HOST_CHANGED`,
 *         `D_SSH_ERR_HOST_REVOKED`, `D_SSH_ERR_KNOWN_HOSTS`,
 *         `D_SSH_ERR_HANDSHAKE`, `D_SSH_ERR_IO`, `D_SSH_ERR_TIMEOUT`,
 *         `D_SSH_ERR_PROTOCOL`, `D_SSH_ERR_MEMORY`, or
 *         `D_SSH_ERR_UNSUPPORTED`, each leaving it D_SSH_STATE_FAILED; or
 *         `D_SSH_ERR_STATE` or `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_connect(struct d_ssh_session* _session);
/**
 * @brief Ends the session: sends SSH_MSG_DISCONNECT if the session is
 *        healthy, releases the engine's session, and closes an owned socket.
 *
 * @note a channel not yet freed is released with the session;
 *       d_ssh_channel_free then only clears it.
 *
 * @param[in,out] _session  the session.
 * @param[in]     _reason   a description for the server, or `NULL`.
 * @return `D_SSH_OK`, including when already closed; or the failure to send
 *         the message, after which the session is closed all the same; or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_disconnect(struct d_ssh_session* _session,
                 const char*           _reason);
/**
 * @brief Releases everything the session holds without sending anything,
 *        closing an owned socket; the storage can be reused.
 *
 * @note a channel not yet freed is released with the session;
 *       d_ssh_channel_free then only clears it.
 *
 * @param[in,out] _session  the session, or `NULL`.
 */
void
d_ssh_destroy(struct d_ssh_session* _session);


//==============================================================================
// 3.  QUERIES
//==============================================================================


// 3.1    State
//------------------------------------------------------------------------------
// session queries
//   Each tolerates NULL. d_ssh_get_state returns D_SSH_STATE_CLOSED for NULL.
// The host-key queries return NULL, or D_SSH_HOST_UNKNOWN, until
// d_ssh_connect has received a key; D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY
// leaves the verdict D_SSH_HOST_UNKNOWN because it consults no file.
// d_ssh_detail describes the last failure or refusal, or is NULL.
D_NODISCARD enum d_ssh_state
d_ssh_get_state(const struct d_ssh_session* _session);
D_NODISCARD bool
d_ssh_is_authenticated(const struct d_ssh_session* _session);
D_NODISCARD const char*
d_ssh_host_key_type(const struct d_ssh_session* _session);
D_NODISCARD const char*
d_ssh_host_key_fingerprint(const struct d_ssh_session* _session);
D_NODISCARD enum d_ssh_host_match
d_ssh_host_key_match(const struct d_ssh_session* _session);
D_NODISCARD const char*
d_ssh_detail(const struct d_ssh_session* _session);

// 3.2    Names
//------------------------------------------------------------------------------
// names
//   Each returns a static, human-readable name, or "unknown" for a value
// outside its enum.
D_NODISCARD const char*
d_ssh_state_string(enum d_ssh_state _state);
D_NODISCARD const char*
d_ssh_host_key_policy_string(enum d_ssh_host_key_policy _policy);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_SESSION_H
