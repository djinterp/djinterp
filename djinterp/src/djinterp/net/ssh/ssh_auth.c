/*******************************************************************************
* djinterp [net]                                                      ssh_auth.c
*
*   Definitions for ssh_auth.h. The engine reports each attempt; which
* attempts are made, in what order, and what their failures add up to
* are decided here, the same for every engine.
*
*
* path:      /src/djinterp/net/ssh/ssh_auth.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_auth.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/config/net/ssh/cfg_ssh.h"  // D_INTERNAL_SSH_AUTH_AGENT
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"  // d_ssh_engine_vtable
#include "../../../../inc/djinterp/net/ssh/ssh_session.h"  // d_ssh_session
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"  // d_ssh_name_list_contains
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


// d_ssh_internal_password
//   the context of the answerer that gives keyboard-interactive the password.
struct d_ssh_internal_password
{
    const char* password;  // borrowed from the credentials
};

/*
d_ssh_internal_answer_password
  File-local: answers each prompt that hides its input with the password, and
declines any that echoes, which is asking for something else.
*/
D_STATIC bool
d_ssh_internal_answer_password(
    void*       _context,
    const char* _prompt,
    bool        _echo,
    char*       _answer,
    size_t      _capacity
)
{
    const struct d_ssh_internal_password* const context = _context;

    (void)_prompt;

    if ( (_echo)     ||
         (!context)  ||
         (!context->password) )
    {
        return false;
    }

    const size_t length = strlen(context->password);

    if (length >= _capacity)
    {
        return false;
    }

    memcpy(_answer, context->password, length + 1);

    return true;
}

/*
d_ssh_internal_final
  File-local: whether an attempt ends authentication: it succeeded, or it
broke the session.
*/
D_STATIC bool
d_ssh_internal_final(
    enum d_ssh_status _status
)
{
    return ( (_status == D_SSH_OK) ||
             (d_ssh_internal_is_fatal(_status)) );
}

/*
d_ssh_internal_auth_outcome
  File-local: folds a failed attempt into the overall result. A method that
could not run here offered nothing; a key file that failed to load counts
only until the server has refused something.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_auth_outcome(
    enum d_ssh_status _outcome,
    enum d_ssh_status _attempt
)
{
    if (_attempt == D_SSH_ERR_UNSUPPORTED)
    {
        return _outcome;
    }

    if (_attempt == D_SSH_ERR_KEY)
    {
        return (_outcome == D_SSH_ERR_NO_AUTH_METHOD) ? D_SSH_ERR_KEY
                                                      : _outcome;
    }

    return D_SSH_ERR_AUTH_DENIED;
}

/*
d_ssh_internal_try_publickey
  File-local: the agent's keys, then the key file. Returns a final status, or
the outcome so far.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_try_publickey(
    struct d_ssh_session*           _session,
    const struct d_ssh_credentials* _credentials,
    enum d_ssh_status*              _outcome
)
{
    const struct d_ssh_engine_vtable* const engine = _session->engine;
    enum d_ssh_status                       status = D_SSH_OK;

    if (_credentials->use_agent)
    {
        status = engine->auth_agent(_session->handle, _credentials->user);

        if (d_ssh_internal_final(status))
        {
            return status;
        }

        *_outcome = d_ssh_internal_auth_outcome(*_outcome, status);
    }

    if (_credentials->private_key)
    {
        status = engine->auth_key_file(_session->handle,
                                       _credentials->user,
                                       _credentials->public_key,
                                       _credentials->private_key,
                                       _credentials->passphrase);

        if (d_ssh_internal_final(status))
        {
            return status;
        }

        *_outcome = d_ssh_internal_auth_outcome(*_outcome, status);
    }

    return *_outcome;
}

/*
d_ssh_internal_try_methods
  File-local: tries what the credentials allow, in OpenSSH's order and only
where the server offers the method: publickey, keyboard-interactive, then
password.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_try_methods(
    struct d_ssh_session*           _session,
    const struct d_ssh_credentials* _credentials,
    const char*                     _methods
)
{
    const struct d_ssh_engine_vtable* const engine   = _session->engine;
    const size_t                            length   = strlen(_methods);
    const char* const                       user     = _credentials->user;
    struct d_ssh_internal_password          password =
        { _credentials->password };
    enum d_ssh_status                       outcome  =
        D_SSH_ERR_NO_AUTH_METHOD;
    enum d_ssh_status                       status   = D_SSH_OK;

    if (d_ssh_name_list_contains(_methods, length, "publickey"))
    {
        status = d_ssh_internal_try_publickey(_session, _credentials, &outcome);

        if (d_ssh_internal_final(status))
        {
            return status;
        }
    }

    // the caller's prompt, or else the password for hidden prompts
    if ( (d_ssh_name_list_contains(_methods, length, "keyboard-interactive")) &&
         ( (_credentials->prompt) ||
           (_credentials->password) ) )
    {
        status = (_credentials->prompt)
                     ? engine->auth_interactive(_session->handle,
                                                user,
                                                _credentials->prompt,
                                                _credentials->prompt_context)
                     : engine->auth_interactive(_session->handle,
                                                user,
                                                d_ssh_internal_answer_password,
                                                &password);

        if (d_ssh_internal_final(status))
        {
            return status;
        }

        outcome = d_ssh_internal_auth_outcome(outcome, status);
    }

    if ( (d_ssh_name_list_contains(_methods, length, "password")) &&
         (_credentials->password) )
    {
        status = engine->auth_password(_session->handle,
                                       user,
                                       _credentials->password);

        if (d_ssh_internal_final(status))
        {
            return status;
        }

        outcome = d_ssh_internal_auth_outcome(outcome, status);
    }

    return outcome;
}

/*
d_ssh_credentials_default
  Offers nothing but the agent, and that only where one can be reached.
*/
struct d_ssh_credentials
d_ssh_credentials_default(void)
{
    const struct d_ssh_credentials credentials =
    {
        .user           = NULL,
        .use_agent      = (D_INTERNAL_SSH_AUTH_AGENT == 1),
        .private_key    = NULL,
        .public_key     = NULL,
        .passphrase     = NULL,
        .password       = NULL,
        .prompt         = NULL,
        .prompt_context = NULL
    };

    return credentials;
}

/*
d_ssh_authenticate
  The engine's method list lives only until its next call, so it is copied
before any attempt is made.
*/
enum d_ssh_status
d_ssh_authenticate(
    struct d_ssh_session*           _session,
    const struct d_ssh_credentials* _credentials
)
{
    if ( (!_session)            ||
         (!_credentials)        ||
         (!_credentials->user)  ||
         (_credentials->user[0] == '\0') )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    if (_session->state != D_SSH_STATE_VERIFIED)
    {
        return D_SSH_ERR_STATE;
    }

    const char*       offered = NULL;
    enum d_ssh_status status  =
        _session->engine->auth_methods(_session->handle,
                                       _credentials->user,
                                       &offered);

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_settle(_session, status);
    }

    // "none" already let the user in
    if (!offered)
    {
        _session->state = D_SSH_STATE_AUTHENTICATED;

        return D_SSH_OK;
    }

    const size_t length  = strlen(offered);
    char* const  methods = malloc(length + 1);

    if (!methods)
    {
        return d_ssh_internal_settle(_session, D_SSH_ERR_MEMORY);
    }

    memcpy(methods, offered, length + 1);
    status = d_ssh_internal_try_methods(_session, _credentials, methods);
    free(methods);

    if (status == D_SSH_OK)
    {
        _session->state = D_SSH_STATE_AUTHENTICATED;
    }

    if (status == D_SSH_ERR_NO_AUTH_METHOD)
    {
        _session->detail = "no credential fits a method the server offers";

        return status;
    }

    return d_ssh_internal_settle(_session, status);
}
