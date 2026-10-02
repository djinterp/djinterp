/*******************************************************************************
* djinterp [net]                                                   ssh_libssh2.c
*
*   Definitions for ssh_libssh2.h: the libssh2 engine, an adapter from
* the engine interface to libssh2's blocking API.
*   It works around three libssh2 behaviors. Each call to
* libssh2_userauth_list leaks the previous call's list, so the list is
* asked for once and cached. libssh2_session_free releases libssh2's
* channels but not the wrappers around them, so the engine tracks its
* wrappers. And a KEX list set in place of libssh2's own loses the
* pseudo-algorithms libssh2 appends to it, so they are appended again.
*
*
* path:      /src/djinterp/net/ssh/ssh_libssh2.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_libssh2.h"  // corresponding header
// std
#include <limits.h>   // INT_MAX, UINT_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint64_t
#include <stdlib.h>   // malloc, calloc, free
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/config/net/ssh/cfg_ssh.h"  // D_INTERNAL_SSH_LIBSSH2
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"  // d_ssh_engine_vtable
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"  // d_ssh_name_list_contains
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*

#if (D_INTERNAL_SSH_LIBSSH2 == 1)
    #include <libssh2.h>  // libssh2_*, LIBSSH2_*
#endif


#if (D_INTERNAL_SSH_LIBSSH2 == 1)

// file-local sizes
enum
{
    D_SSH_INTERNAL_ANSWER_MAX = 1024  // longest prompt answer
};

/*
d_ssh_internal_wipe
  File-local: zeroes memory that held a secret. The writes go through a
volatile pointer, so the compiler cannot drop them as dead stores.
*/
D_STATIC void
d_ssh_internal_wipe(
    void*  _data,
    size_t _size
)
{
    volatile unsigned char* const bytes = (volatile unsigned char*)_data;

    // one byte at a time, each store observable
    for (size_t i = 0; i < _size; i++)
    {
        bytes[i] = 0;
    }

    return;
}

// d_ssh_internal_libssh2
//   one libssh2 engine session. It tracks its channel wrappers, because
// libssh2_session_free releases libssh2's channels but cannot know about the
// wrappers, and it caches the server's method list, because each call to
// libssh2_userauth_list leaks the list the previous call returned.
struct d_ssh_internal_libssh2
{
    LIBSSH2_SESSION*                       session;         // libssh2's
    struct d_ssh_internal_libssh2_channel* channels;        // not yet freed
    char*                                  methods;         // cached list
    fn_ssh_prompt                          prompt;          // kbd-int only
    void*                                  prompt_context;  // for prompt
    const char*                            detail;          // or NULL
    char                                   message[256];    // detail's text
};

// d_ssh_internal_libssh2_channel
//   one libssh2 channel, linked into its engine session's list.
struct d_ssh_internal_libssh2_channel
{
    LIBSSH2_CHANNEL*                       channel;   // the libssh2 channel
    struct d_ssh_internal_libssh2*         engine;    // its engine session
    struct d_ssh_internal_libssh2_channel* previous;  // in engine->channels
    struct d_ssh_internal_libssh2_channel* next;      // in engine->channels
};

#if defined(LIBSSH2_ERROR_KEYFILE_AUTH_FAILED)

/*
d_ssh_internal_libssh2_is_key_error
  File-local: whether libssh2 reports a key file it could not load or
decrypt, which newer releases tell apart.
*/
D_STATIC bool
d_ssh_internal_libssh2_is_key_error(
    int _code
)
{
    return ( (_code == LIBSSH2_ERROR_FILE) ||
             (_code == LIBSSH2_ERROR_KEYFILE_AUTH_FAILED) );
}

#else

/*
d_ssh_internal_libssh2_is_key_error
  File-local: whether libssh2 reports a key file it could not load; older
releases have one code for every such failure.
*/
D_STATIC bool
d_ssh_internal_libssh2_is_key_error(
    int _code
)
{
    return (_code == LIBSSH2_ERROR_FILE);
}

#endif

/*
d_ssh_internal_libssh2_map
  File-local: translates a libssh2 error code. Codes not named here, which
later libssh2 releases may add, read as protocol failures: fatal, and so on
the safe side. Every named code exists in libssh2 1.9.0.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_map(
    int _code
)
{
    if (d_ssh_internal_libssh2_is_key_error(_code))
    {
        return D_SSH_ERR_KEY;
    }

    switch (_code)
    {
        case LIBSSH2_ERROR_NONE:
            return D_SSH_OK;

        case LIBSSH2_ERROR_SOCKET_NONE:
        case LIBSSH2_ERROR_SOCKET_SEND:
        case LIBSSH2_ERROR_SOCKET_RECV:
        case LIBSSH2_ERROR_SOCKET_DISCONNECT:
        case LIBSSH2_ERROR_BAD_SOCKET:
        case LIBSSH2_ERROR_EAGAIN:
            return D_SSH_ERR_IO;

        case LIBSSH2_ERROR_TIMEOUT:
        case LIBSSH2_ERROR_SOCKET_TIMEOUT:
            return D_SSH_ERR_TIMEOUT;

        case LIBSSH2_ERROR_BANNER_RECV:
        case LIBSSH2_ERROR_BANNER_SEND:
        case LIBSSH2_ERROR_KEX_FAILURE:
        case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE:
        case LIBSSH2_ERROR_HOSTKEY_INIT:
        case LIBSSH2_ERROR_HOSTKEY_SIGN:
        case LIBSSH2_ERROR_METHOD_NONE:
            return D_SSH_ERR_HANDSHAKE;

        case LIBSSH2_ERROR_ALLOC:
            return D_SSH_ERR_MEMORY;

        case LIBSSH2_ERROR_AUTHENTICATION_FAILED:
        case LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED:
        case LIBSSH2_ERROR_PASSWORD_EXPIRED:
            return D_SSH_ERR_AUTH_DENIED;

        case LIBSSH2_ERROR_AGENT_PROTOCOL:
        case LIBSSH2_ERROR_METHOD_NOT_SUPPORTED:
            return D_SSH_ERR_UNSUPPORTED;

        case LIBSSH2_ERROR_CHANNEL_OUTOFORDER:
        case LIBSSH2_ERROR_CHANNEL_FAILURE:
        case LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED:
        case LIBSSH2_ERROR_CHANNEL_UNKNOWN:
        case LIBSSH2_ERROR_REQUEST_DENIED:
            return D_SSH_ERR_CHANNEL;

        case LIBSSH2_ERROR_CHANNEL_CLOSED:
        case LIBSSH2_ERROR_CHANNEL_EOF_SENT:
            return D_SSH_ERR_STATE;

        case LIBSSH2_ERROR_INVAL:
        case LIBSSH2_ERROR_BAD_USE:
        case LIBSSH2_ERROR_BUFFER_TOO_SMALL:
            return D_SSH_ERR_ARGUMENT;

        default:
            return D_SSH_ERR_PROTOCOL;
    }
}

/*
d_ssh_internal_libssh2_fail
  File-local: keeps libssh2's description of a failure -- which it frees at
its next error -- and translates the code.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_fail(
    struct d_ssh_internal_libssh2* _engine,
    int                            _code
)
{
    char* message = NULL;
    int   length  = 0;

    (void)libssh2_session_last_error(_engine->session, &message, &length, 0);

    if ( (message) &&
         (length > 0) )
    {
        const size_t limit = sizeof(_engine->message) - 1;
        const size_t size  = ((size_t)length < limit) ? (size_t)length
                                                      : limit;

        memcpy(_engine->message, message, size);
        _engine->message[size] = '\0';
        _engine->detail        = _engine->message;
    }
    else
    {
        _engine->detail = "libssh2 reported a failure";
    }

    return d_ssh_internal_libssh2_map(_code);
}

/*
d_ssh_internal_libssh2_length
  File-local: a string's length as the unsigned int libssh2 takes, or false
if it does not fit one.
*/
D_STATIC bool
d_ssh_internal_libssh2_length(
    const char*   _text,
    unsigned int* _length
)
{
    const uint64_t length = (uint64_t)((_text) ? strlen(_text) : 0);

    *_length = 0;

    if (length > (uint64_t)UINT_MAX)
    {
        return false;
    }

    *_length = (unsigned int)length;

    return true;
}

/*
d_ssh_internal_libssh2_prefer
  File-local: sets one method preference when a list was given. libssh2 copies
the list and drops the names it does not implement.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_prefer(
    struct d_ssh_internal_libssh2* _engine,
    int                            _method,
    const char*                    _list
)
{
    if (!_list)
    {
        return D_SSH_OK;
    }

    const int code = libssh2_session_method_pref(_engine->session,
                                                 _method,
                                                 _list);

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(_engine, code);
}

// d_ssh_internal_libssh2_markers
//   the pseudo-algorithms libssh2 adds to its own KEX list, which a list set
// in their place would lose: the request for the server's extensions (RFC
// 8308), and OpenSSH's strict key exchange marker, the Terrapin
// countermeasure. A release that implements neither drops the names.
D_STATIC const char* const d_ssh_internal_libssh2_markers[2] =
{
    "ext-info-c",
    "kex-strict-c-v00@openssh.com"
};

/*
d_ssh_internal_libssh2_prefer_kex
  File-local: sets a KEX preference with libssh2's markers kept. Builds with
strict key exchange -- 1.11.1, and distributions' patched 1.11.0 -- enable it
whenever the server offers it, so a list without the client's marker leaves
the two sides numbering packets differently after the first key exchange,
which surfaces as a corrupted MAC under any non-AEAD cipher. Without
extension negotiation, RSA user keys fall back to SHA-1 signatures that
current servers refuse. The list alone is set first, so that one naming no
algorithm libssh2 knows is still refused as unsupported.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_prefer_kex(
    struct d_ssh_internal_libssh2* _engine,
    const char*                    _list
)
{
    enum d_ssh_status status =
        d_ssh_internal_libssh2_prefer(_engine, LIBSSH2_METHOD_KEX, _list);

    if ( (status != D_SSH_OK) ||
         (!_list) )
    {
        return status;
    }

    const size_t length = strlen(_list);
    size_t       extra  = 0;

    // room for each marker the list does not name already
    for (size_t i = 0; i < 2; i++)
    {
        const char* const marker = d_ssh_internal_libssh2_markers[i];

        if (!d_ssh_name_list_contains(_list, length, marker))
        {
            extra += strlen(marker) + 1;
        }
    }

    char* const list = malloc(length + extra + 1);

    if (!list)
    {
        return D_SSH_ERR_MEMORY;
    }

    size_t used = length;

    memcpy(list, _list, length + 1);

    for (size_t i = 0; i < 2; i++)
    {
        const char* const marker = d_ssh_internal_libssh2_markers[i];
        const size_t      size   = strlen(marker);

        if (!d_ssh_name_list_contains(_list, length, marker))
        {
            list[used] = ',';
            memcpy(list + used + 1, marker, size + 1);
            used += size + 1;
        }
    }

    status = d_ssh_internal_libssh2_prefer(_engine, LIBSSH2_METHOD_KEX, list);
    free(list);

    return status;
}

/*
d_ssh_internal_libssh2_prefer_all
  File-local: applies every preference, the KEX list first; ciphers and MACs
apply in both directions. The first refusal stops it.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_prefer_all(
    struct d_ssh_internal_libssh2* _engine,
    const struct d_ssh_algorithms* _algorithms
)
{
    const int         methods[5] = { LIBSSH2_METHOD_HOSTKEY,
                                     LIBSSH2_METHOD_CRYPT_CS,
                                     LIBSSH2_METHOD_CRYPT_SC,
                                     LIBSSH2_METHOD_MAC_CS,
                                     LIBSSH2_METHOD_MAC_SC };
    const char* const lists[5]   = { _algorithms->host_keys,
                                     _algorithms->ciphers,
                                     _algorithms->ciphers,
                                     _algorithms->macs,
                                     _algorithms->macs };
    enum d_ssh_status status     =
        d_ssh_internal_libssh2_prefer_kex(_engine, _algorithms->kex);

    // one preference at a time, until one is refused
    for (size_t i = 0; ( (i < 5) && (status == D_SSH_OK) ); i++)
    {
        status = d_ssh_internal_libssh2_prefer(_engine, methods[i], lists[i]);
    }

    return status;
}

/*
d_ssh_internal_libssh2_create
  The engine is libssh2's abstract pointer, which is how the
keyboard-interactive callback finds the prompt. Blocking mode, with the
deadline as libssh2's timeout, gives every operation the same bound.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_create(
    const struct d_ssh_algorithms* _algorithms,
    unsigned int                   _timeout_ms,
    void**                         _handle
)
{
    *_handle = NULL;

    struct d_ssh_internal_libssh2* const engine = calloc(1, sizeof(*engine));

    if (!engine)
    {
        return D_SSH_ERR_MEMORY;
    }

    engine->session = libssh2_session_init_ex(NULL, NULL, NULL, engine);

    if (!engine->session)
    {
        free(engine);

        return D_SSH_ERR_MEMORY;
    }

    libssh2_session_set_blocking(engine->session, 1);
    libssh2_session_set_timeout(engine->session, (long)_timeout_ms);

    const enum d_ssh_status status =
        d_ssh_internal_libssh2_prefer_all(engine, _algorithms);

    if (status != D_SSH_OK)
    {
        (void)libssh2_session_free(engine->session);
        free(engine);

        return status;
    }

    *_handle = engine;

    return D_SSH_OK;
}

/*
d_ssh_internal_libssh2_handshake
  Whatever the transport did not cause is a failed key exchange, however
libssh2 classifies it.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_handshake(
    void*        _handle,
    d_ssh_socket _socket
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    const int                            code   =
        libssh2_session_handshake(engine->session, (libssh2_socket_t)_socket);

    if (code == 0)
    {
        return D_SSH_OK;
    }

    const enum d_ssh_status status =
        d_ssh_internal_libssh2_fail(engine, code);

    if ( (status == D_SSH_ERR_IO)      ||
         (status == D_SSH_ERR_TIMEOUT) ||
         (status == D_SSH_ERR_MEMORY) )
    {
        return status;
    }

    return D_SSH_ERR_HANDSHAKE;
}

/*
d_ssh_internal_libssh2_host_key
  libssh2 keeps the blob for the session's lifetime.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_host_key(
    void*                 _handle,
    const unsigned char** _key,
    size_t*               _size
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    size_t                               length = 0;
    int                                  type   = 0;
    const char* const                    key    =
        libssh2_session_hostkey(engine->session, &length, &type);

    if ( (!key) ||
         (length == 0) )
    {
        engine->detail = "the server presented no host key";

        return D_SSH_ERR_PROTOCOL;
    }

    *_key  = (const unsigned char*)key;
    *_size = length;

    return D_SSH_OK;
}

/*
d_ssh_internal_libssh2_auth_methods
  libssh2 asks with the "none" method, so a server that needs no credentials
authenticates the user here, which libssh2 reports as a NULL list. The list
is asked for once and cached: a server fixes it for the session's user, and
OpenSSH refuses a change of user mid-authentication anyway.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_auth_methods(
    void*        _handle,
    const char*  _user,
    const char** _methods
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    unsigned int                         length = 0;

    *_methods = engine->methods;

    if (engine->methods)
    {
        return D_SSH_OK;
    }

    if (!d_ssh_internal_libssh2_length(_user, &length))
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const char* const list = libssh2_userauth_list(engine->session,
                                                   _user,
                                                   length);

    if (list)
    {
        const size_t size = strlen(list) + 1;
        char* const  copy = malloc(size);

        if (!copy)
        {
            return D_SSH_ERR_MEMORY;
        }

        memcpy(copy, list, size);
        engine->methods = copy;
        *_methods       = copy;

        return D_SSH_OK;
    }

    if (libssh2_userauth_authenticated(engine->session))
    {
        return D_SSH_OK;
    }

    return d_ssh_internal_libssh2_fail(engine,
                                       libssh2_session_last_errno(
                                           engine->session));
}

/*
d_ssh_internal_libssh2_auth_password
  No password-change callback: an expired password is a refusal.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_auth_password(
    void*       _handle,
    const char* _user,
    const char* _password
)
{
    struct d_ssh_internal_libssh2* const engine          = _handle;
    unsigned int                         user_length     = 0;
    unsigned int                         password_length = 0;

    if ( (!d_ssh_internal_libssh2_length(_user, &user_length)) ||
         (!d_ssh_internal_libssh2_length(_password, &password_length)) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const int code = libssh2_userauth_password_ex(engine->session,
                                                  _user,
                                                  user_length,
                                                  _password,
                                                  password_length,
                                                  NULL);

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(engine, code);
}

/*
d_ssh_internal_libssh2_auth_key_file
  A missing passphrase is passed as the empty one: libssh2 must never fall
back to a crypto library's terminal prompt.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_auth_key_file(
    void*       _handle,
    const char* _user,
    const char* _public_key,
    const char* _private_key,
    const char* _passphrase
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    unsigned int                         length = 0;

    if (!d_ssh_internal_libssh2_length(_user, &length))
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const int code =
        libssh2_userauth_publickey_fromfile_ex(engine->session,
                                               _user,
                                               length,
                                               _public_key,
                                               _private_key,
                                               (_passphrase) ? _passphrase
                                                             : "");

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(engine, code);
}

/*
d_ssh_internal_libssh2_try_identities
  File-local: offers each identity the agent lists until one is accepted.
Reports D_SSH_ERR_UNSUPPORTED if it held none, since nothing was offered.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_try_identities(
    struct d_ssh_internal_libssh2* _engine,
    LIBSSH2_AGENT*                 _agent,
    const char*                    _user
)
{
    struct libssh2_agent_publickey* previous = NULL;
    struct libssh2_agent_publickey* identity = NULL;
    enum d_ssh_status               status   = D_SSH_ERR_UNSUPPORTED;

    // until the list ends, one is accepted, or the session breaks
    for (;;)
    {
        const int next = libssh2_agent_get_identity(_agent,
                                                    &identity,
                                                    previous);

        if (next == 1)
        {
            break;
        }

        if (next < 0)
        {
            const enum d_ssh_status error =
                d_ssh_internal_libssh2_fail(_engine, next);

            return (d_ssh_internal_is_fatal(error)) ? error : status;
        }

        const int code = libssh2_agent_userauth(_agent, _user, identity);

        if (code == 0)
        {
            return D_SSH_OK;
        }

        status = d_ssh_internal_libssh2_fail(_engine, code);

        if (d_ssh_internal_is_fatal(status))
        {
            return status;
        }

        status   = D_SSH_ERR_AUTH_DENIED;
        previous = identity;
    }

    return status;
}

/*
d_ssh_internal_libssh2_auth_agent
  An agent that cannot be reached is a missing capability, not a refusal:
the session goes on to other methods.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_auth_agent(
    void*       _handle,
    const char* _user
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    LIBSSH2_AGENT* const                 agent  =
        libssh2_agent_init(engine->session);

    if (!agent)
    {
        return d_ssh_internal_libssh2_fail(engine, LIBSSH2_ERROR_ALLOC);
    }

    enum d_ssh_status status = D_SSH_ERR_UNSUPPORTED;

    if ( (libssh2_agent_connect(agent) == 0) &&
         (libssh2_agent_list_identities(agent) == 0) )
    {
        status = d_ssh_internal_libssh2_try_identities(engine, agent, _user);
        (void)libssh2_agent_disconnect(agent);
    }
    else
    {
        engine->detail = "no authentication agent could be reached";
    }

    libssh2_agent_free(agent);

    return status;
}

/*
d_ssh_internal_libssh2_answer
  File-local: answers one keyboard-interactive prompt. The prompt is copied
into a terminated string for the engine's prompt function, and the answer
is handed to libssh2 in a malloc'd copy -- which libssh2 frees with the
session's allocator, malloc's partner -- before the local copy is wiped.
Returns false if the prompt went unanswered.
*/
D_STATIC bool
d_ssh_internal_libssh2_answer(
    struct d_ssh_internal_libssh2*        _engine,
    const LIBSSH2_USERAUTH_KBDINT_PROMPT* _prompt,
    LIBSSH2_USERAUTH_KBDINT_RESPONSE*     _response
)
{
    const size_t length = (size_t)_prompt->length;
    char* const  prompt = malloc(length + 1);

    if (!prompt)
    {
        return false;
    }

    // an empty prompt may come with no text at all
    if (length > 0)
    {
        memcpy(prompt, _prompt->text, length);
    }

    prompt[length] = '\0';

    char       answer[D_SSH_INTERNAL_ANSWER_MAX] = { 0 };
    const bool answered = _engine->prompt(_engine->prompt_context,
                                          prompt,
                                          (_prompt->echo != 0),
                                          answer,
                                          sizeof(answer));

    free(prompt);
    answer[sizeof(answer) - 1] = '\0';

    const size_t answer_length = strlen(answer);
    char* const  copy          = (answered) ? malloc(answer_length + 1)
                                            : NULL;

    if (copy)
    {
        memcpy(copy, answer, answer_length + 1);
        _response->text   = copy;
        _response->length = (unsigned int)answer_length;
    }

    d_ssh_internal_wipe(answer, sizeof(answer));

    return (copy != NULL);
}

/*
d_ssh_internal_libssh2_respond
  File-local: libssh2's keyboard-interactive callback. Every response starts
out empty, so a prompt left unanswered, and every one after it, reaches the
server empty, and the server refuses.
*/
D_STATIC void
d_ssh_internal_libssh2_respond(
    const char*                           _name,
    int                                   _name_length,
    const char*                           _instruction,
    int                                   _instruction_length,
    int                                   _count,
    const LIBSSH2_USERAUTH_KBDINT_PROMPT* _prompts,
    LIBSSH2_USERAUTH_KBDINT_RESPONSE*     _responses,
    void**                                _abstract
)
{
    struct d_ssh_internal_libssh2* const engine =
        (_abstract) ? (struct d_ssh_internal_libssh2*)*_abstract : NULL;

    (void)_name;
    (void)_name_length;
    (void)_instruction;
    (void)_instruction_length;

    for (int i = 0; i < _count; i++)
    {
        _responses[i].text   = NULL;
        _responses[i].length = 0;
    }

    if ( (!engine) ||
         (!engine->prompt) )
    {
        return;
    }

    // one prompt at a time, stopping at the first left unanswered
    for (int i = 0; i < _count; i++)
    {
        const bool answered =
            d_ssh_internal_libssh2_answer(engine,
                                          &_prompts[i],
                                          &_responses[i]);

        if (!answered)
        {
            break;
        }
    }

    return;
}

/*
d_ssh_internal_libssh2_auth_interactive
  The prompt is installed only for the duration of the call.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_auth_interactive(
    void*         _handle,
    const char*   _user,
    fn_ssh_prompt _prompt,
    void*         _context
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    unsigned int                         length = 0;

    if (!d_ssh_internal_libssh2_length(_user, &length))
    {
        return D_SSH_ERR_ARGUMENT;
    }

    engine->prompt         = _prompt;
    engine->prompt_context = _context;

    const int code =
        libssh2_userauth_keyboard_interactive_ex(
            engine->session,
            _user,
            length,
            d_ssh_internal_libssh2_respond);

    engine->prompt         = NULL;
    engine->prompt_context = NULL;

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(engine, code);
}

/*
d_ssh_internal_libssh2_channel_open
  A direct-tcpip channel names 127.0.0.1:22 as its originator, as libssh2's
own convenience call does; the server only logs it.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_open(
    void*        _handle,
    const char*  _host,
    unsigned int _port,
    void**       _channel
)
{
    struct d_ssh_internal_libssh2* const engine  = _handle;
    LIBSSH2_CHANNEL* const               channel =
        (_host) ? libssh2_channel_direct_tcpip(engine->session,
                                               _host,
                                               (int)_port)
                : libssh2_channel_open_session(engine->session);

    *_channel = NULL;

    if (!channel)
    {
        return d_ssh_internal_libssh2_fail(engine,
                                           libssh2_session_last_errno(
                                               engine->session));
    }

    struct d_ssh_internal_libssh2_channel* const wrapper =
        malloc(sizeof(*wrapper));

    if (!wrapper)
    {
        (void)libssh2_channel_free(channel);

        return D_SSH_ERR_MEMORY;
    }

    wrapper->channel  = channel;
    wrapper->engine   = engine;
    wrapper->previous = NULL;
    wrapper->next     = engine->channels;

    // the engine frees what is still linked when it is destroyed
    if (engine->channels)
    {
        engine->channels->previous = wrapper;
    }

    engine->channels = wrapper;
    *_channel        = wrapper;

    return D_SSH_OK;
}

/*
d_ssh_internal_libssh2_channel_request
  exec, subsystem, and shell are all one channel request in SSH, with or
without a value.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_request(
    void*       _channel,
    const char* _type,
    const char* _value
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper      = _channel;
    unsigned int                                 type_length  = 0;
    unsigned int                                 value_length = 0;

    if ( (!d_ssh_internal_libssh2_length(_type, &type_length)) ||
         (!d_ssh_internal_libssh2_length(_value, &value_length)) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const int code = libssh2_channel_process_startup(wrapper->channel,
                                                     _type,
                                                     type_length,
                                                     _value,
                                                     value_length);

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(wrapper->engine, code);
}

/*
d_ssh_internal_libssh2_channel_pty
  No terminal modes are sent; the server's defaults apply.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_pty(
    void*        _channel,
    const char*  _term,
    unsigned int _columns,
    unsigned int _rows
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper = _channel;
    unsigned int                                 length  = 0;

    if ( (!d_ssh_internal_libssh2_length(_term, &length)) ||
         (_columns > (unsigned int)INT_MAX)                ||
         (_rows > (unsigned int)INT_MAX) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const int code = libssh2_channel_request_pty_ex(wrapper->channel,
                                                    _term,
                                                    length,
                                                    NULL,
                                                    0,
                                                    (int)_columns,
                                                    (int)_rows,
                                                    0,
                                                    0);

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(wrapper->engine, code);
}

/*
d_ssh_internal_libssh2_channel_read
  In blocking mode, libssh2 returns 0 only once the stream has ended.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_read(
    void*             _channel,
    enum d_ssh_stream _stream,
    void*             _buffer,
    size_t            _capacity,
    size_t*           _received
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper = _channel;
    const int                                    stream  =
        (_stream == D_SSH_STREAM_STDERR) ? SSH_EXTENDED_DATA_STDERR : 0;
    const ssize_t                                got     =
        libssh2_channel_read_ex(wrapper->channel,
                                stream,
                                (char*)_buffer,
                                _capacity);

    *_received = 0;

    if (got > 0)
    {
        *_received = (size_t)got;

        return D_SSH_OK;
    }

    if (got == 0)
    {
        return D_SSH_ERR_CLOSED;
    }

    return d_ssh_internal_libssh2_fail(wrapper->engine, (int)got);
}

/*
d_ssh_internal_libssh2_channel_write
  In blocking mode, libssh2 waits for window space and writes at least one
byte; a zero return would mean the transport stalled.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_write(
    void*       _channel,
    const void* _data,
    size_t      _size,
    size_t*     _sent
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper = _channel;
    const ssize_t                                put     =
        libssh2_channel_write_ex(wrapper->channel,
                                 0,
                                 (const char*)_data,
                                 _size);

    *_sent = 0;

    if (put > 0)
    {
        *_sent = (size_t)put;

        return D_SSH_OK;
    }

    if (put == 0)
    {
        return D_SSH_ERR_IO;
    }

    return d_ssh_internal_libssh2_fail(wrapper->engine, (int)put);
}

/*
d_ssh_internal_libssh2_channel_send_eof
  One message; the channel stays readable.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_send_eof(
    void* _channel
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper = _channel;
    const int                                    code    =
        libssh2_channel_send_eof(wrapper->channel);

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(wrapper->engine, code);
}

/*
d_ssh_internal_libssh2_channel_close
  libssh2_channel_wait_closed refuses a channel whose peer has not sent EOF,
so it is called only once one has; the exit status reads 0 when the server
sent none.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_channel_close(
    void* _channel,
    int*  _exit_status
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper = _channel;
    int                                          code    =
        libssh2_channel_close(wrapper->channel);

    if ( (code == 0) &&
         (libssh2_channel_eof(wrapper->channel) == 1) )
    {
        code = libssh2_channel_wait_closed(wrapper->channel);
    }

    *_exit_status = libssh2_channel_get_exit_status(wrapper->channel);

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(wrapper->engine, code);
}

/*
d_ssh_internal_libssh2_channel_free
  Unlinks the wrapper from its engine session's list; libssh2 closes a
channel that is still open before freeing it.
*/
D_STATIC void
d_ssh_internal_libssh2_channel_free(
    void* _channel
)
{
    struct d_ssh_internal_libssh2_channel* const wrapper = _channel;

    if (wrapper->previous)
    {
        wrapper->previous->next = wrapper->next;
    }
    else
    {
        wrapper->engine->channels = wrapper->next;
    }

    if (wrapper->next)
    {
        wrapper->next->previous = wrapper->previous;
    }

    (void)libssh2_channel_free(wrapper->channel);
    free(wrapper);

    return;
}

/*
d_ssh_internal_libssh2_disconnect
  SSH_DISCONNECT_BY_APPLICATION, with no language tag.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_libssh2_disconnect(
    void*       _handle,
    const char* _reason
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;
    const int                            code   =
        libssh2_session_disconnect_ex(engine->session,
                                      SSH_DISCONNECT_BY_APPLICATION,
                                      _reason,
                                      "");

    return (code == 0) ? D_SSH_OK
                       : d_ssh_internal_libssh2_fail(engine, code);
}

/*
d_ssh_internal_libssh2_detail
  The last failure's description, kept by d_ssh_internal_libssh2_fail.
*/
D_STATIC const char*
d_ssh_internal_libssh2_detail(
    void* _handle
)
{
    const struct d_ssh_internal_libssh2* const engine = _handle;

    return engine->detail;
}

/*
d_ssh_internal_libssh2_destroy
  libssh2_session_free releases the session's channels, so the wrappers still
linked are freed after it without touching their channels.
*/
D_STATIC void
d_ssh_internal_libssh2_destroy(
    void* _handle
)
{
    struct d_ssh_internal_libssh2* const engine = _handle;

    (void)libssh2_session_free(engine->session);

    // each wrapper's libssh2 channel is gone; only the wrapper remains
    while (engine->channels)
    {
        struct d_ssh_internal_libssh2_channel* const next =
            engine->channels->next;

        free(engine->channels);
        engine->channels = next;
    }

    free(engine->methods);
    free(engine);

    return;
}

// d_ssh_internal_libssh2_vtable
//   the libssh2 engine's operations.
D_STATIC const struct d_ssh_engine_vtable d_ssh_internal_libssh2_vtable =
{
    .name             = "libssh2",
    .create           = d_ssh_internal_libssh2_create,
    .handshake        = d_ssh_internal_libssh2_handshake,
    .host_key         = d_ssh_internal_libssh2_host_key,
    .auth_methods     = d_ssh_internal_libssh2_auth_methods,
    .auth_password    = d_ssh_internal_libssh2_auth_password,
    .auth_key_file    = d_ssh_internal_libssh2_auth_key_file,
    .auth_agent       = d_ssh_internal_libssh2_auth_agent,
    .auth_interactive = d_ssh_internal_libssh2_auth_interactive,
    .channel_open     = d_ssh_internal_libssh2_channel_open,
    .channel_request  = d_ssh_internal_libssh2_channel_request,
    .channel_pty      = d_ssh_internal_libssh2_channel_pty,
    .channel_read     = d_ssh_internal_libssh2_channel_read,
    .channel_write    = d_ssh_internal_libssh2_channel_write,
    .channel_send_eof = d_ssh_internal_libssh2_channel_send_eof,
    .channel_close    = d_ssh_internal_libssh2_channel_close,
    .channel_free     = d_ssh_internal_libssh2_channel_free,
    .disconnect       = d_ssh_internal_libssh2_disconnect,
    .detail           = d_ssh_internal_libssh2_detail,
    .destroy          = d_ssh_internal_libssh2_destroy
};

/*
d_ssh_internal_libssh2_startup
  libssh2_init sets up its crypto backend. It is the call that is not
thread-safe; every later one only counts.
*/
enum d_ssh_status
d_ssh_internal_libssh2_startup(void)
{
    return (libssh2_init(0) == 0) ? D_SSH_OK : D_SSH_ERR_UNSUPPORTED;
}

/*
d_ssh_internal_libssh2_shutdown
  Balances d_ssh_internal_libssh2_startup.
*/
void
d_ssh_internal_libssh2_shutdown(void)
{
    libssh2_exit();

    return;
}

/*
d_ssh_engine_libssh2
  The engine is stateless apart from its sessions, so one table serves all.
*/
const struct d_ssh_engine_vtable*
d_ssh_engine_libssh2(void)
{
    return &d_ssh_internal_libssh2_vtable;
}

#else

/*
d_ssh_internal_libssh2_startup
  Not compiled, so there is nothing to start.
*/
enum d_ssh_status
d_ssh_internal_libssh2_startup(void)
{
    return D_SSH_OK;
}

/*
d_ssh_internal_libssh2_shutdown
  Nothing was started.
*/
void
d_ssh_internal_libssh2_shutdown(void)
{
    return;
}

/*
d_ssh_engine_libssh2
  Not compiled: libssh2 was not detected, or D_CFG_SSH_LIBSSH2 is 0.
*/
const struct d_ssh_engine_vtable*
d_ssh_engine_libssh2(void)
{
    return NULL;
}

#endif
