/*******************************************************************************
* djinterp [net]                                                   ssh_session.c
*
*   Definitions for ssh_session.h: setting a session up, the handshake,
* host-key ordering and verification, and teardown.
*
*
* path:      /src/djinterp/net/ssh/ssh_session.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_session.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free, getenv
#include <string.h>   // memcpy, memset, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/config/net/ssh/cfg_ssh.h"  // D_INTERNAL_SSH_*
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"  // d_ssh_engine_vtable
#include "../../../../inc/djinterp/net/ssh/ssh_key.h"  // d_ssh_key_type, d_ssh_fingerprint
#include "../../../../inc/djinterp/net/ssh/ssh_known_hosts.h"  // d_ssh_known_hosts_add
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"  // d_ssh_name_list_contains
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*

#if (D_INTERNAL_SSH_WINSOCK == 1)
    #include <winsock2.h>    // SOCKET, closesocket
#endif
#if (D_INTERNAL_SSH_BSD == 1)
    #include <sys/socket.h>  // setsockopt, socklen_t, SOL_SOCKET
    #include <unistd.h>      // close
#endif


// the configuration's policy sentinels mirror the enum
D_STATIC_ASSERT(D_SSH_HOST_KEY_STRICT ==
                    D_CFG_SSH_HOST_KEY_STRICT,
                "d_ssh_host_key_policy is out of step with cfg_ssh.h");

D_STATIC_ASSERT(D_SSH_HOST_KEY_ACCEPT_NEW ==
                    D_CFG_SSH_HOST_KEY_ACCEPT_NEW,
                "d_ssh_host_key_policy is out of step with cfg_ssh.h");

D_STATIC_ASSERT(D_SSH_HOST_KEY_CALLBACK ==
                    D_CFG_SSH_HOST_KEY_CALLBACK,
                "d_ssh_host_key_policy is out of step with cfg_ssh.h");

D_STATIC_ASSERT(D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY ==
                    D_CFG_SSH_HOST_KEY_INSECURE_ACCEPT_ANY,
                "d_ssh_host_key_policy is out of step with cfg_ssh.h");

/*
d_ssh_internal_engine_complete
  File-local: whether an engine provides every required operation;
channel_pty and detail are optional.
*/
D_STATIC bool
d_ssh_internal_engine_complete(
    const struct d_ssh_engine_vtable* _engine
)
{
    return ( (_engine->create)           &&
             (_engine->handshake)        &&
             (_engine->host_key)         &&
             (_engine->auth_methods)     &&
             (_engine->auth_password)    &&
             (_engine->auth_key_file)    &&
             (_engine->auth_agent)       &&
             (_engine->auth_interactive) &&
             (_engine->channel_open)     &&
             (_engine->channel_request)  &&
             (_engine->channel_read)     &&
             (_engine->channel_write)    &&
             (_engine->channel_send_eof) &&
             (_engine->channel_close)    &&
             (_engine->channel_free)     &&
             (_engine->disconnect)       &&
             (_engine->destroy) );
}

/*
d_ssh_internal_list_valid
  File-local: whether an optional algorithm list is absent, or a non-empty
name-list.
*/
D_STATIC bool
d_ssh_internal_list_valid(
    const char* _list
)
{
    return ( (!_list) ||
             ( (_list[0] != '\0') &&
               (d_ssh_internal_name_list_valid(_list, strlen(_list))) ) );
}

/*
d_ssh_internal_pool_size
  File-local: the bytes a string occupies in the session's string pool.
*/
D_STATIC size_t
d_ssh_internal_pool_size(
    const char* _text
)
{
    return (_text) ? (strlen(_text) + 1) : 0;
}

/*
d_ssh_internal_pool_copy
  File-local: copies a string into the pool at `*_cursor` and advances past
it; NULL stays NULL.
*/
D_STATIC char*
d_ssh_internal_pool_copy(
    char**      _cursor,
    const char* _text
)
{
    if (!_text)
    {
        return NULL;
    }

    const size_t size = strlen(_text) + 1;
    char* const  copy = *_cursor;

    memcpy(copy, _text, size);
    *_cursor += size;

    return copy;
}

/*
d_ssh_internal_default_known_hosts
  File-local: the user's .ssh/known_hosts under the home directory, in a new
allocation; NULL, with *_status still D_SSH_OK, when no home is set.
*/
D_STATIC char*
d_ssh_internal_default_known_hosts(
    enum d_ssh_status* _status
)
{
    const char* const home   = getenv(D_INTERNAL_SSH_HOME_VARIABLE);
    const char* const suffix = "/.ssh/known_hosts";

    *_status = D_SSH_OK;

    if ( (!home) ||
         (home[0] == '\0') )
    {
        return NULL;
    }

    const size_t home_length   = strlen(home);
    const size_t suffix_length = strlen(suffix);
    char* const  path          = malloc(home_length + suffix_length + 1);

    if (!path)
    {
        *_status = D_SSH_ERR_MEMORY;

        return NULL;
    }

    memcpy(path, home, home_length);
    memcpy(path + home_length, suffix, suffix_length + 1);

    return path;
}

/*
d_ssh_internal_copy_strings
  File-local: copies the options' strings into one allocation the session
owns, lowercasing the host once, here, because known_hosts compares names
without regard to case.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_copy_strings(
    struct d_ssh_session*       _session,
    const struct d_ssh_options* _options,
    const char*                 _known_hosts
)
{
    const struct d_ssh_algorithms* const lists = &_options->algorithms;
    const size_t                         total =
        d_ssh_internal_pool_size(_options->host)               +
        d_ssh_internal_pool_size(_known_hosts)                 +
        d_ssh_internal_pool_size(_options->system_known_hosts) +
        d_ssh_internal_pool_size(lists->kex)                   +
        d_ssh_internal_pool_size(lists->host_keys)             +
        d_ssh_internal_pool_size(lists->ciphers)               +
        d_ssh_internal_pool_size(lists->macs);
    char* const                          strings = malloc(total);

    if (!strings)
    {
        return D_SSH_ERR_MEMORY;
    }

    char*       cursor = strings;
    char* const host   = d_ssh_internal_pool_copy(&cursor, _options->host);

    for (size_t i = 0; host[i] != '\0'; i++)
    {
        host[i] = d_ssh_internal_lower(host[i]);
    }

    _session->strings            = strings;
    _session->host               = host;
    _session->known_hosts        = d_ssh_internal_pool_copy(&cursor,
                                                            _known_hosts);
    _session->system_known_hosts =
        d_ssh_internal_pool_copy(&cursor, _options->system_known_hosts);
    _session->algorithms.kex       =
        d_ssh_internal_pool_copy(&cursor, lists->kex);
    _session->algorithms.host_keys =
        d_ssh_internal_pool_copy(&cursor, lists->host_keys);
    _session->algorithms.ciphers   =
        d_ssh_internal_pool_copy(&cursor, lists->ciphers);
    _session->algorithms.macs      =
        d_ssh_internal_pool_copy(&cursor, lists->macs);

    return D_SSH_OK;
}

#if ( (D_INTERNAL_SSH_BSD == 1) && defined(SO_NOSIGPIPE) )

/*
d_ssh_internal_socket_prepare
  File-local: sets SO_NOSIGPIPE, so that writing to a vanished peer is an
error rather than a SIGPIPE that ends the process. libssh2 passes
MSG_NOSIGNAL where it exists; this covers the platforms where it does not.
*/
D_STATIC void
d_ssh_internal_socket_prepare(
    d_ssh_socket _socket
)
{
    const int on = 1;

    (void)setsockopt(_socket,
                     SOL_SOCKET,
                     SO_NOSIGPIPE,
                     &on,
                     (socklen_t)sizeof(on));

    return;
}

#else

/*
d_ssh_internal_socket_prepare
  File-local: nothing to set where SO_NOSIGPIPE does not exist.
*/
D_STATIC void
d_ssh_internal_socket_prepare(
    d_ssh_socket _socket
)
{
    (void)_socket;

    return;
}

#endif

#if (D_INTERNAL_SSH_BSD == 1)

/*
d_ssh_internal_socket_close
  File-local: closes a descriptor.
*/
D_STATIC void
d_ssh_internal_socket_close(
    d_ssh_socket _socket
)
{
    (void)close(_socket);

    return;
}

#elif (D_INTERNAL_SSH_WINSOCK == 1)

/*
d_ssh_internal_socket_close
  File-local: closes a Winsock handle.
*/
D_STATIC void
d_ssh_internal_socket_close(
    d_ssh_socket _socket
)
{
    (void)closesocket((SOCKET)_socket);

    return;
}

#else

/*
d_ssh_internal_socket_close
  File-local: no socket API, so no socket could have been owned.
*/
D_STATIC void
d_ssh_internal_socket_close(
    d_ssh_socket _socket
)
{
    (void)_socket;

    return;
}

#endif

/*
d_ssh_internal_note
  Internal: records why the session's last call failed: `_detail` when this
module decided, otherwise the engine's description, copied so it outlives
the engine.
*/
void
d_ssh_internal_note(
    struct d_ssh_session* _session,
    const char*           _detail
)
{
    if (_detail)
    {
        _session->detail = _detail;

        return;
    }

    const char* const text = ( (_session->handle) &&
                               (_session->engine->detail) )
                                 ? _session->engine->detail(_session->handle)
                                 : NULL;

    if (text)
    {
        const size_t limit  = sizeof(_session->message) - 1;
        const size_t length = strlen(text);
        const size_t size   = (length < limit) ? length : limit;

        memcpy(_session->message, text, size);
        _session->message[size] = '\0';
        _session->detail        = _session->message;
    }

    return;
}

/*
d_ssh_internal_fail
  Internal: ends the session with `_status`.
*/
enum d_ssh_status
d_ssh_internal_fail(
    struct d_ssh_session* _session,
    enum d_ssh_status     _status,
    const char*           _detail
)
{
    _session->state = D_SSH_STATE_FAILED;
    d_ssh_internal_note(_session, _detail);

    return _status;
}

/*
d_ssh_internal_settle
  Internal: applies an engine result to the session: a fatal one fails it,
any other failure is described, and success changes nothing.
*/
enum d_ssh_status
d_ssh_internal_settle(
    struct d_ssh_session* _session,
    enum d_ssh_status     _status
)
{
    if (d_ssh_internal_is_fatal(_status))
    {
        return d_ssh_internal_fail(_session, _status, NULL);
    }

    // an ended stream is an outcome, not a failure worth describing
    if ( (_status != D_SSH_OK) &&
         (_status != D_SSH_ERR_CLOSED) )
    {
        d_ssh_internal_note(_session, NULL);
    }

    return _status;
}

/*
d_ssh_internal_release
  File-local: frees the engine's session before closing an owned socket. The
engine may touch the descriptor while it shuts down, and a closed number can
be reused at once by another thread's open.
*/
D_STATIC void
d_ssh_internal_release(
    struct d_ssh_session* _session
)
{
    if (_session->handle)
    {
        _session->engine->destroy(_session->handle);
        _session->handle = NULL;
    }

    if ( (_session->owns_socket) &&
         (_session->socket != D_SSH_SOCKET_INVALID) )
    {
        d_ssh_internal_socket_close(_session->socket);
    }

    _session->socket      = D_SSH_SOCKET_INVALID;
    _session->owns_socket = false;

    return;
}

/*
d_ssh_internal_scan_host
  File-local: one scan of the user's and the system's files for the session's
host -- with the host key once it has arrived -- reporting the verdict and
the key types recorded.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_scan_host(
    const struct d_ssh_session* _session,
    const unsigned char*        _key,
    size_t                      _key_size,
    enum d_ssh_host_match*      _match,
    char*                       _types
)
{
    char                       name[D_SSH_INTERNAL_HOST_MAX];
    struct d_ssh_internal_scan scan;
    enum d_ssh_status          status =
        d_ssh_internal_host_string(_session->host,
                                   _session->port,
                                   name,
                                   sizeof(name));

    if (status != D_SSH_OK)
    {
        return status;
    }

    status = d_ssh_internal_scan_init(&scan, name, _key, _key_size);

    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_scan_file(&scan, _session->known_hosts);
    }

    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_scan_file(&scan,
                                          _session->system_known_hosts);
    }

    if (status == D_SSH_OK)
    {
        *_match = d_ssh_internal_scan_verdict(&scan);
        memcpy(_types, scan.types, sizeof(scan.types));
    }

    d_ssh_internal_scan_free(&scan);

    return status;
}

/*
d_ssh_internal_algorithm_known
  File-local: whether a host-key algorithm signs with a key type in `_types`.
rsa-sha2-256 and rsa-sha2-512 use "ssh-rsa" keys; every other algorithm is
named after its key type.
*/
D_STATIC bool
d_ssh_internal_algorithm_known(
    const char* _algorithm,
    size_t      _length,
    const char* _types
)
{
    char name[D_SSH_KEY_TYPE_SIZE];

    if (_length >= sizeof(name))
    {
        return false;
    }

    // the RSA signature algorithms share one key type
    if ( (_length > 9) &&
         (memcmp(_algorithm, "rsa-sha2-", 9) == 0) )
    {
        memcpy(name, "ssh-rsa", sizeof("ssh-rsa"));
    }
    else
    {
        memcpy(name, _algorithm, _length);
        name[_length] = '\0';
    }

    return d_ssh_name_list_contains(_types, strlen(_types), name);
}

/*
d_ssh_internal_order_host_keys
  File-local: copies `_base` to `_ordered` with the algorithms for recorded
key types first, keeping the order within each group. This is OpenSSH's
defense against a server presenting a key of a type the client never
recorded, which would read as a changed key. Nothing is added, and only
empty names are dropped, so `_ordered` needs no more room than `_base`.
*/
D_STATIC void
d_ssh_internal_order_host_keys(
    const char* _base,
    const char* _types,
    char*       _ordered
)
{
    size_t written = 0;

    // pass 0 takes the recorded types, pass 1 the rest
    for (int pass = 0; pass < 2; pass++)
    {
        const char* cursor = _base;

        while (*cursor != '\0')
        {
            const char* const end    = strchr(cursor, ',');
            const size_t      length = (end) ? (size_t)(end - cursor)
                                             : strlen(cursor);
            const bool        known  =
                d_ssh_internal_algorithm_known(cursor, length, _types);

            if ( (length > 0) &&
                 (known == (pass == 0)) )
            {
                if (written > 0)
                {
                    _ordered[written++] = ',';
                }

                memcpy(_ordered + written, cursor, length);
                written += length;
            }

            cursor = (end) ? (end + 1) : (cursor + length);
        }
    }

    _ordered[written] = '\0';

    return;
}

/*
d_ssh_internal_start
  File-local: resolves the algorithm lists, creates the engine's session, and
runs the handshake; any failure fails the session. Unspecified ciphers and
MACs become the module's own only where D_CFG_SSH_RESTRICT_ALGORITHMS is on.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_start(
    struct d_ssh_session* _session
)
{
    char                  types[D_SSH_INTERNAL_TYPES_MAX] = { 0 };
    enum d_ssh_host_match match  = D_SSH_HOST_UNKNOWN;
    enum d_ssh_status     status = D_SSH_OK;

    // the insecure policy reads no file, so it has no types to prefer
    if (_session->policy != D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY)
    {
        status = d_ssh_internal_scan_host(_session, NULL, 0, &match, types);
    }

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_fail(_session,
                                   status,
                                   "known_hosts could not be read");
    }

    const char* const base    = (_session->algorithms.host_keys)
                                    ? _session->algorithms.host_keys
                                    : D_SSH_HOST_KEY_ALGORITHMS;
    char* const       ordered = malloc(strlen(base) + 1);

    if (!ordered)
    {
        return d_ssh_internal_fail(_session, D_SSH_ERR_MEMORY, NULL);
    }

    struct d_ssh_algorithms algorithms = _session->algorithms;
    void*                   handle     = NULL;

    d_ssh_internal_order_host_keys(base, types, ordered);
    algorithms.host_keys = ordered;

    if (D_INTERNAL_SSH_RESTRICT_ALGORITHMS == 1)
    {
        algorithms.ciphers = (algorithms.ciphers) ? algorithms.ciphers
                                                  : D_SSH_CIPHERS;
        algorithms.macs    = (algorithms.macs) ? algorithms.macs
                                               : D_SSH_MACS;
    }

    status = _session->engine->create(&algorithms,
                                      _session->timeout_ms,
                                      &handle);
    free(ordered);

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_fail(_session,
                                   status,
                                   "the engine refused to start a session");
    }

    _session->handle = handle;
    status           = _session->engine->handshake(handle, _session->socket);

    return (status == D_SSH_OK) ? D_SSH_OK
                                : d_ssh_internal_fail(_session, status, NULL);
}

/*
d_ssh_internal_take_host_key
  File-local: copies the engine's host key into the session, with its type
and fingerprint. A blob with no readable type is a protocol failure: no
known_hosts line could ever name it.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_take_host_key(
    struct d_ssh_session* _session
)
{
    const unsigned char* key    = NULL;
    size_t               size   = 0;
    enum d_ssh_status    status =
        _session->engine->host_key(_session->handle, &key, &size);

    if ( (status == D_SSH_OK) &&
         ( (!key) ||
           (size == 0) ) )
    {
        status = D_SSH_ERR_PROTOCOL;
    }

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_fail(_session, status, NULL);
    }

    unsigned char* const copy = malloc(size);

    if (!copy)
    {
        return d_ssh_internal_fail(_session, D_SSH_ERR_MEMORY, NULL);
    }

    memcpy(copy, key, size);
    _session->host_key      = copy;
    _session->host_key_size = size;

    status = d_ssh_key_type(copy,
                            size,
                            _session->host_key_type,
                            sizeof(_session->host_key_type));

    if (status == D_SSH_OK)
    {
        status = d_ssh_fingerprint(copy,
                                   size,
                                   _session->fingerprint,
                                   sizeof(_session->fingerprint));
    }

    return (status == D_SSH_OK)
               ? D_SSH_OK
               : d_ssh_internal_fail(_session,
                                     D_SSH_ERR_PROTOCOL,
                                     "the host key is malformed");
}

/*
d_ssh_internal_accept_new
  File-local: decides a host no file records. ACCEPT_NEW trusts and records
it, CALLBACK asks the program, and STRICT refuses. A key that should be
recorded but cannot be fails the connection, rather than be trusted again
next time with no record of it.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_accept_new(
    struct d_ssh_session* _session,
    const char**          _detail
)
{
    bool accept   = (_session->policy == D_SSH_HOST_KEY_ACCEPT_NEW);
    bool remember = accept;

    // the program sees the fingerprint before anything is trusted
    if (_session->policy == D_SSH_HOST_KEY_CALLBACK)
    {
        accept = _session->decide(_session->decide_context,
                                  _session->host,
                                  _session->port,
                                  _session->host_key_type,
                                  _session->fingerprint,
                                  &remember);
    }

    if (!accept)
    {
        *_detail = "the host key is not recorded in known_hosts";

        return D_SSH_ERR_HOST_UNKNOWN;
    }

    if (!remember)
    {
        return D_SSH_OK;
    }

    if ( (!_session->known_hosts) ||
         (d_ssh_known_hosts_add(_session->known_hosts,
                                _session->host,
                                _session->port,
                                _session->host_key,
                                _session->host_key_size,
                                _session->hash_known_hosts) != D_SSH_OK) )
    {
        *_detail = "the host key could not be recorded in known_hosts";

        return D_SSH_ERR_KNOWN_HOSTS;
    }

    return D_SSH_OK;
}

/*
d_ssh_internal_verify
  File-local: judges the host key under the session's policy. A key that
differs from the record, or is revoked, fails under every policy that reads
the files; only an unrecorded host is left to the policy to decide.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_verify(
    struct d_ssh_session* _session,
    const char**          _detail
)
{
    char                  types[D_SSH_INTERNAL_TYPES_MAX];
    enum d_ssh_host_match match = D_SSH_HOST_UNKNOWN;

    *_detail = NULL;

    // nothing is consulted, so the verdict stays unknown
    if (_session->policy == D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY)
    {
        return D_SSH_OK;
    }

    const enum d_ssh_status status =
        d_ssh_internal_scan_host(_session,
                                 _session->host_key,
                                 _session->host_key_size,
                                 &match,
                                 types);

    if (status != D_SSH_OK)
    {
        *_detail = "known_hosts could not be read";

        return status;
    }

    _session->host_match = match;

    if (match == D_SSH_HOST_KNOWN)
    {
        return D_SSH_OK;
    }

    if (match == D_SSH_HOST_REVOKED)
    {
        *_detail = "the host key is marked @revoked in known_hosts";

        return D_SSH_ERR_HOST_REVOKED;
    }

    if (match == D_SSH_HOST_CHANGED)
    {
        *_detail = "the host key differs from every key known_hosts records "
                   "for this host";

        return D_SSH_ERR_HOST_CHANGED;
    }

    return d_ssh_internal_accept_new(_session, _detail);
}

/*
d_ssh_options_default
  Every field is set explicitly, from cfg_ssh.h's derived values.
*/
struct d_ssh_options
d_ssh_options_default(void)
{
    const struct d_ssh_options options =
    {
        .engine             = d_ssh_engine_default(),
        .host               = NULL,
        .port               = D_SSH_PORT,
        .host_key_policy    =
            (enum d_ssh_host_key_policy)D_INTERNAL_SSH_HOST_KEY_POLICY,
        .known_hosts        = NULL,
        .system_known_hosts = D_INTERNAL_SSH_SYSTEM_KNOWN_HOSTS,
        .hash_known_hosts   = (D_INTERNAL_SSH_HASH_KNOWN_HOSTS == 1),
        .decide             = NULL,
        .decide_context     = NULL,
        .algorithms         = { NULL, NULL, NULL, NULL },
        .timeout_ms         = (unsigned int)D_INTERNAL_SSH_TIMEOUT_MS
    };

    return options;
}

/*
d_ssh_internal_options_check
  File-local: judges options before anything is allocated. A missing engine
is D_SSH_ERR_NO_BACKEND and a host that known_hosts could not record is
D_SSH_ERR_FORMAT; anything else missing or out of range is
D_SSH_ERR_ARGUMENT.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_options_check(
    const struct d_ssh_options* _options,
    d_ssh_socket                _socket
)
{
    if ( (!_options)       ||
         (!_options->host) ||
         (_socket == D_SSH_SOCKET_INVALID) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    if (!_options->engine)
    {
        return D_SSH_ERR_NO_BACKEND;
    }

    const enum d_ssh_host_key_policy policy = _options->host_key_policy;

    // the engine, the port, the policy, and the algorithm lists
    if ( (!d_ssh_internal_engine_complete(_options->engine))          ||
         (_options->port == 0)                                        ||
         (_options->port > 65535)                                     ||
         (policy < D_SSH_HOST_KEY_STRICT)                             ||
         (policy > D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY)                ||
         ( (policy == D_SSH_HOST_KEY_CALLBACK) &&
           (!_options->decide) )                                      ||
         (!d_ssh_internal_list_valid(_options->algorithms.kex))       ||
         (!d_ssh_internal_list_valid(_options->algorithms.host_keys)) ||
         (!d_ssh_internal_list_valid(_options->algorithms.ciphers))   ||
         (!d_ssh_internal_list_valid(_options->algorithms.macs)) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    return (d_ssh_internal_host_valid(_options->host)) ? D_SSH_OK
                                                       : D_SSH_ERR_FORMAT;
}

/*
d_ssh_init
  Validation finishes before anything is allocated or the socket touched, so
a refused session has nothing to release.
*/
enum d_ssh_status
d_ssh_init(
    struct d_ssh_session*       _session,
    const struct d_ssh_options* _options,
    d_ssh_socket                _socket,
    bool                        _owns
)
{
    if (!_session)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    memset(_session, 0, sizeof(*_session));
    _session->socket     = D_SSH_SOCKET_INVALID;
    _session->state      = D_SSH_STATE_CLOSED;
    _session->host_match = D_SSH_HOST_UNKNOWN;

    enum d_ssh_status status = d_ssh_internal_options_check(_options, _socket);

    if (status != D_SSH_OK)
    {
        return status;
    }

    // with no user file named, the default one under the home directory
    char* const home_path = (_options->known_hosts)
                                ? NULL
                                : d_ssh_internal_default_known_hosts(&status);

    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_copy_strings(
                     _session,
                     _options,
                     (_options->known_hosts) ? _options->known_hosts
                                             : home_path);
    }

    free(home_path);

    if (status != D_SSH_OK)
    {
        return status;
    }

    d_ssh_internal_socket_prepare(_socket);

    _session->engine           = _options->engine;
    _session->socket           = _socket;
    _session->owns_socket      = _owns;
    _session->policy           = _options->host_key_policy;
    _session->hash_known_hosts = _options->hash_known_hosts;
    _session->timeout_ms       = _options->timeout_ms;
    _session->port             = _options->port;
    _session->decide           = _options->decide;
    _session->decide_context   = _options->decide_context;
    _session->state            = D_SSH_STATE_NEW;

    return D_SSH_OK;
}

/*
d_ssh_connect
  The host-key algorithms are ordered by what known_hosts records before any
byte is sent. After the handshake the key is copied, typed, and
fingerprinted before it is judged, so that a refusal can still show it.
*/
enum d_ssh_status
d_ssh_connect(
    struct d_ssh_session* _session
)
{
    if (!_session)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    if (_session->state != D_SSH_STATE_NEW)
    {
        return D_SSH_ERR_STATE;
    }

    enum d_ssh_status status = d_ssh_internal_start(_session);

    if (status == D_SSH_OK)
    {
        status = d_ssh_internal_take_host_key(_session);
    }

    // start and take_host_key have already failed the session
    if (status != D_SSH_OK)
    {
        return status;
    }

    const char* detail = NULL;

    status = d_ssh_internal_verify(_session, &detail);

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_fail(_session, status, detail);
    }

    _session->state = D_SSH_STATE_VERIFIED;

    return D_SSH_OK;
}

/*
d_ssh_disconnect
  Only a healthy transport carries the courtesy message; either way the
engine's session is released and an owned socket closed.
*/
enum d_ssh_status
d_ssh_disconnect(
    struct d_ssh_session* _session,
    const char*           _reason
)
{
    if (!_session)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    if (_session->state == D_SSH_STATE_CLOSED)
    {
        return D_SSH_OK;
    }

    enum d_ssh_status status = D_SSH_OK;

    if ( (_session->handle) &&
         ( (_session->state == D_SSH_STATE_VERIFIED) ||
           (_session->state == D_SSH_STATE_AUTHENTICATED) ) )
    {
        status = _session->engine->disconnect(
                     _session->handle,
                     (_reason) ? _reason : "closed by application");

        if (status != D_SSH_OK)
        {
            d_ssh_internal_note(_session, NULL);
        }
    }

    d_ssh_internal_release(_session);
    _session->state = D_SSH_STATE_CLOSED;

    return status;
}

/*
d_ssh_destroy
  Releases before freeing, so the engine never outlives the strings it was
given.
*/
void
d_ssh_destroy(
    struct d_ssh_session* _session
)
{
    if (!_session)
    {
        return;
    }

    d_ssh_internal_release(_session);
    free(_session->strings);
    free(_session->host_key);

    memset(_session, 0, sizeof(*_session));
    _session->socket     = D_SSH_SOCKET_INVALID;
    _session->state      = D_SSH_STATE_CLOSED;
    _session->host_match = D_SSH_HOST_UNKNOWN;

    return;
}

/*
d_ssh_get_state
  NULL reads as closed.
*/
enum d_ssh_state
d_ssh_get_state(
    const struct d_ssh_session* _session
)
{
    return (_session) ? _session->state : D_SSH_STATE_CLOSED;
}

/*
d_ssh_is_authenticated
  Only an authenticated session opens channels.
*/
bool
d_ssh_is_authenticated(
    const struct d_ssh_session* _session
)
{
    return ( (_session) &&
             (_session->state == D_SSH_STATE_AUTHENTICATED) );
}

/*
d_ssh_host_key_type
  Empty until a key arrives.
*/
const char*
d_ssh_host_key_type(
    const struct d_ssh_session* _session
)
{
    return ( (_session) &&
             (_session->host_key_type[0] != '\0') )
               ? _session->host_key_type
               : NULL;
}

/*
d_ssh_host_key_fingerprint
  Empty until a key arrives.
*/
const char*
d_ssh_host_key_fingerprint(
    const struct d_ssh_session* _session
)
{
    return ( (_session) &&
             (_session->fingerprint[0] != '\0') )
               ? _session->fingerprint
               : NULL;
}

/*
d_ssh_host_key_match
  The verdict of the last verification.
*/
enum d_ssh_host_match
d_ssh_host_key_match(
    const struct d_ssh_session* _session
)
{
    return (_session) ? _session->host_match : D_SSH_HOST_UNKNOWN;
}

/*
d_ssh_detail
  Static text, or the session's own copy of the engine's.
*/
const char*
d_ssh_detail(
    const struct d_ssh_session* _session
)
{
    return (_session) ? _session->detail : NULL;
}

/*
d_ssh_state_string
  One name per state.
*/
const char*
d_ssh_state_string(
    enum d_ssh_state _state
)
{
    switch (_state)
    {
        case D_SSH_STATE_NEW:
            return "new";
        case D_SSH_STATE_VERIFIED:
            return "verified";
        case D_SSH_STATE_AUTHENTICATED:
            return "authenticated";
        case D_SSH_STATE_FAILED:
            return "failed";
        case D_SSH_STATE_CLOSED:
            return "closed";
    }

    return "unknown";
}

/*
d_ssh_host_key_policy_string
  Named as OpenSSH's StrictHostKeyChecking values are, where one matches.
*/
const char*
d_ssh_host_key_policy_string(
    enum d_ssh_host_key_policy _policy
)
{
    switch (_policy)
    {
        case D_SSH_HOST_KEY_STRICT:
            return "strict";
        case D_SSH_HOST_KEY_ACCEPT_NEW:
            return "accept-new";
        case D_SSH_HOST_KEY_CALLBACK:
            return "callback";
        case D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY:
            return "insecure-accept-any";
    }

    return "unknown";
}
