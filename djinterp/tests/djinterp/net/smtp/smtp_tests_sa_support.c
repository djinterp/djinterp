/*******************************************************************************
* djinterp [net]                                         smtp_tests_sa_support.c
*
* Support for the SMTP kernel's standalone tests.
*   Scripted peers that say everything at once, expectations built as the
* toy engine would put them on the wire, a pair of toy contexts, a client
* opened over a connection, and the server handlers the sessions run with.
* The peer and the toy engine themselves are net_tests_peer.c's.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./smtp_tests_sa.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <string.h>   // memcpy, strcmp, strlen, strncmp


// peers
/*
d_tests_smtp_peer
  A peer whose whole script is released at once, and which hangs up once it
is read: what the old scripted transports did.
*/
struct d_tests_net_peer*
d_tests_smtp_peer(
    const char* _script
)
{
    struct d_tests_net_peer* peer = d_tests_net_peer_new();

    // a peer that cannot say its script is no peer
    if ( (peer) &&
         (!d_tests_net_peer_say(peer,
                                "",
                                _script)) )
    {
        d_tests_net_peer_free(peer);

        return NULL;
    }

    if (peer)
    {
        peer->hangs_up = true;
    }

    return peer;
}

/*
d_tests_smtp_sent
  The peer's output against an expectation built with d_tests_smtp_expect.
*/
bool
d_tests_smtp_sent(
    const struct d_tests_net_peer* _peer,
    const struct d_smtp_buffer*    _expected
)
{
    return d_tests_net_peer_sent(_peer,
                                 _expected->data);
}

/*
d_tests_smtp_expect
  Appends what fits, sealed by the toy cipher when asked; an expectation
that does not fit fails its comparison, which is enough for a test.
*/
void
d_tests_smtp_expect(
    struct d_smtp_buffer* _out,
    const char*           _text,
    bool                  _sealed
)
{
    char sealed[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };

    // the toy's cipher, as its sessions apply it
    if (_sealed)
    {
        d_tests_net_toy_seal(sealed,
                             sizeof(sealed),
                             _text);
    }

    (void)d_smtp_buffer_append_text(_out,
                                    (_sealed) ? sealed : _text);

    return;
}

// contexts
/*
d_tests_smtp_toy_pair
  The server's first, and only with it the client's.
*/
bool
d_tests_smtp_toy_pair(
    struct d_ssl_context*   _contexts,
    struct d_tests_net_toy* _toys,
    bool                    _verify
)
{
    // a server's context that never verifies its clients
    if (!d_tests_net_toy_context(&_contexts[0],
                                 &_toys[0],
                                 D_SSL_ROLE_SERVER,
                                 false))
    {
        return false;
    }

    // a client's, without which the server's is not kept either
    if (!d_tests_net_toy_context(&_contexts[1],
                                 &_toys[1],
                                 D_SSL_ROLE_CLIENT,
                                 _verify))
    {
        d_ssl_context_destroy(&_contexts[0]);

        return false;
    }

    return true;
}

// clients
/*
d_tests_smtp_open
  Attach, greeting and EHLO; the first failure ends it.
*/
enum d_smtp_error
d_tests_smtp_open(
    struct d_smtp_client*    _client,
    struct d_net_connection* _connection,
    bool                     _secure
)
{
    d_smtp_client_init(_client);

    enum d_smtp_error result = d_smtp_client_attach(_client,
                                                    _connection,
                                                    _secure);

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_greeting(_client);
    }

    return (result == D_SMTP_OK) ? d_smtp_client_hello(_client,
                                                       "c.example")
                                 : result;
}

// servers
/*
d_tests_smtp_on_rcpt
  Refuses recipients whose local part starts with "reject".
*/
static unsigned int
d_tests_smtp_on_rcpt(
    void*                             _context,
    const struct d_smtp_session_info* _session,
    const char*                       _recipient
)
{
    (void)_context;
    (void)_session;

    return (strncmp(_recipient,
                    "reject",
                    6u) == 0) ? 550u : 0u;
}

/*
d_tests_smtp_on_message
  Records a delivered message in the inbox.
*/
static unsigned int
d_tests_smtp_on_message(
    void*                             _context,
    const struct d_smtp_session_info* _session,
    const char*                       _data,
    size_t                            _length
)
{
    struct d_tests_smtp_inbox* inbox = _context;
    const size_t               kept  = (_length < sizeof(inbox->data) - 1u)
                                           ? _length
                                           : sizeof(inbox->data) - 1u;

    inbox->messages   += 1;
    inbox->recipients  = _session->recipient_count;
    memcpy(inbox->data,
           _data,
           kept);
    inbox->data[kept] = '\0';
    (void)snprintf(inbox->sender,
                   sizeof(inbox->sender),
                   "%s",
                   _session->sender);
    (void)snprintf(inbox->user,
                   sizeof(inbox->user),
                   "%s",
                   (_session->user) ? _session->user : "");
    (void)snprintf(inbox->domain,
                   sizeof(inbox->domain),
                   "%s",
                   (_session->client_domain) ? _session->client_domain
                   : "");

    return 0u;
}

/*
d_tests_smtp_on_auth
  Accepts alice with the password s3cret.
*/
static bool
d_tests_smtp_on_auth(
    void*       _context,
    const char* _user,
    const char* _secret
)
{
    (void)_context;

    return ( (strcmp(_user,
                     "alice") == 0) &&
             (strcmp(_secret,
                     "s3cret") == 0) );
}

/*
d_tests_smtp_handler
  The test callbacks, recording into `_inbox`.
*/
void
d_tests_smtp_handler(
    struct d_smtp_server_handler* _handler,
    struct d_tests_smtp_inbox*    _inbox,
    bool                          _with_auth
)
{
    d_smtp_server_handler_init(_handler);
    _handler->context = _inbox;
    _handler->rcpt    = d_tests_smtp_on_rcpt;
    _handler->message = d_tests_smtp_on_message;
    _handler->auth    = (_with_auth) ? d_tests_smtp_on_auth : NULL;

    return;
}

/*
d_tests_smtp_serve
  One server session over `_connection` with the test handlers.
*/
enum d_smtp_error
d_tests_smtp_serve(
    struct d_net_connection*           _connection,
    const struct d_smtp_server_config* _config,
    struct d_tests_smtp_inbox*         _inbox,
    bool                               _with_auth
)
{
    struct d_smtp_server_handler handler = { .context = NULL };

    d_tests_smtp_handler(&handler,
                         _inbox,
                         _with_auth);

    return d_smtp_server_session(_connection,
                                 _config,
                                 &handler);
}
