/*******************************************************************************
* djinterp [net]                                                      ssh_auth.h
*
* SSH user authentication.
*   Methods are tried in OpenSSH's order, each only if the server offers
* it and the credentials allow it: publickey (the agent's keys, then a
* key file), keyboard-interactive, then password. A refusal leaves the
* session verified, so another attempt can follow.
*
*
* path:      /inc/djinterp/net/ssh/ssh_auth.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Credentials
         1.  d_ssh_credentials
2.  AUTHENTICATION
    --------------
    1.  Defaults
    2.  Authenticating
*/

#ifndef DJINTERP_NET_SSH_SSH_AUTH_H
#define DJINTERP_NET_SSH_SSH_AUTH_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_common.h"      // d_ssh_status
#include "./ssh_engine.h"      // fn_ssh_prompt
#include "./ssh_session.h"     // d_ssh_session


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Credentials
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_credentials
//   struct: what d_ssh_authenticate may offer, all borrowed for the call.
// Methods are tried in OpenSSH's order, each only if the server offers it and
// the credentials allow it: publickey (the agent's keys, then `private_key`),
// keyboard-interactive (answered by `prompt`, or else with `password` for
// prompts that hide their input), then password.
struct d_ssh_credentials
{
    const char*   user;            // the account to log in as
    bool          use_agent;       // offer the agent's keys
    const char*   private_key;     // key file, or NULL
    const char*   public_key;      // its public half, or NULL to derive it
    const char*   passphrase;      // for private_key, or NULL
    const char*   password;        // password, or NULL
    fn_ssh_prompt prompt;          // keyboard-interactive, or NULL
    void*         prompt_context;  // passed to prompt
};


//==============================================================================
// 2.  AUTHENTICATION
//==============================================================================


// 2.1    Defaults
//------------------------------------------------------------------------------
/**
 * @brief Returns credentials that offer only the agent's keys, where
 *        D_CFG_SSH_AUTH_AGENT is on and an agent can be reached.
 *
 * @return the credentials; `user` is `NULL` and must be set.
 */
D_NODISCARD struct d_ssh_credentials
d_ssh_credentials_default(void);

// 2.2    Authenticating
//------------------------------------------------------------------------------
/**
 * @brief Authenticates the user with whatever the credentials and the
 *        server's offered methods have in common.
 *
 * @note a failed attempt that is not fatal leaves the session
 *       D_SSH_STATE_VERIFIED, so other credentials can be tried -- within the
 *       server's limit on attempts, past which it disconnects.
 *
 * @param[in,out] _session      a D_SSH_STATE_VERIFIED session.
 * @param[in]     _credentials  the credentials; `user` is required.
 * @return `D_SSH_OK`, leaving the session D_SSH_STATE_AUTHENTICATED;
 *         `D_SSH_ERR_AUTH_DENIED` if the server refused what was offered;
 *         `D_SSH_ERR_KEY` if the only failure was a key file that could not
 *         be loaded; `D_SSH_ERR_NO_AUTH_METHOD` if nothing could be offered;
 *         a FATAL status; or `D_SSH_ERR_STATE` or `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_authenticate(struct d_ssh_session*           _session,
                   const struct d_ssh_credentials* _credentials);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_AUTH_H
