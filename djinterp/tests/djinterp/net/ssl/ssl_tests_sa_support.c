/*******************************************************************************
* djinterp [net]                                          ssl_tests_sa_support.c
*
* What the SSL kernel's tests share: the run's contexts, and a client and a
* server session over the two ends of a pipe.
*   One thread drives both sessions. A handshake or a transfer calls each
* side in turn, and a side whose ring is full or empty answers WOULD_BLOCK
* until the other has moved, so nothing here waits on anything.
*
*
* path:      /tests/djinterp/net/ssl/ssl_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./ssl_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memset, strlen


// D_TESTS_SSL_ROUNDS
//   constant: how many turns a handshake or a transfer may take before a
// test gives up on it; far more than any of them needs.
#define D_TESTS_SSL_ROUNDS 100000u

// d_tests_ssl_the_world
//   the run's world, and whether it is open.
static struct d_tests_ssl_world d_tests_ssl_the_world;
static bool                     d_tests_ssl_world_ready = false;

/*
d_tests_ssl_client_config
  A client that trusts the run's CA alone and checks the server's name.
*/
void
d_tests_ssl_client_config(
    struct d_ssl_config* _config
)
{
    d_ssl_config_init(_config,
                      D_SSL_ROLE_CLIENT);
    _config->use_system_trust = false;
    _config->ca_file          = d_tests_ssl_the_world.pki.ca;

    return;
}

/*
d_tests_ssl_server_config
  A server presenting `_certificate` and `_key`, asking nothing of clients.
*/
void
d_tests_ssl_server_config(
    struct d_ssl_config* _config,
    const char*          _certificate,
    const char*          _key
)
{
    d_ssl_config_init(_config,
                      D_SSL_ROLE_SERVER);
    _config->use_system_trust = false;
    _config->certificate_file = _certificate;
    _config->private_key_file = _key;

    return;
}

/*
d_tests_ssl_context
  A context over the OpenSSL-family engine.
*/
bool
d_tests_ssl_context(
    struct d_ssl_context*      _out,
    const struct d_ssl_config* _config
)
{
    return (d_ssl_context_init(_out,
                               d_ssl_engine_openssl(),
                               _config) == D_SSL_OK);
}

/*
d_tests_ssl_world_open
  The certificates first, then a context for each side; a failure leaves
nothing open.
*/
bool
d_tests_ssl_world_open(void)
{
    struct d_ssl_config client = { .verify_hostname = true };
    struct d_ssl_config server = { .verify_hostname = false };

    memset(&d_tests_ssl_the_world,
           0,
           sizeof(d_tests_ssl_the_world));

    // without the engine, or the certificates, there is nothing to test
    if ( (d_ssl_engine_openssl() == NULL) ||
         (!d_tests_net_pki_make(&d_tests_ssl_the_world.pki)) )
    {
        return false;
    }

    d_tests_ssl_client_config(&client);
    d_tests_ssl_server_config(&server,
                              d_tests_ssl_the_world.pki.server_cert,
                              d_tests_ssl_the_world.pki.server_key);

    d_tests_ssl_world_ready = ( (d_tests_ssl_context(
                                     &d_tests_ssl_the_world.client,
                                     &client)) &&
                                (d_tests_ssl_context(
                                     &d_tests_ssl_the_world.server,
                                     &server)) );

    // half a world is no world
    if (!d_tests_ssl_world_ready)
    {
        d_tests_ssl_world_close();
    }

    return d_tests_ssl_world_ready;
}

/*
d_tests_ssl_world_close
  Each context's destroy accepts one never made.
*/
void
d_tests_ssl_world_close(void)
{
    d_ssl_context_destroy(&d_tests_ssl_the_world.client);
    d_ssl_context_destroy(&d_tests_ssl_the_world.server);
    d_tests_net_pki_remove(&d_tests_ssl_the_world.pki);
    d_tests_ssl_world_ready = false;

    return;
}

/*
d_tests_ssl_world
  NULL until the world is open.
*/
struct d_tests_ssl_world*
d_tests_ssl_world(void)
{
    return (d_tests_ssl_world_ready) ? &d_tests_ssl_the_world
                                     : NULL;
}

/*
d_tests_ssl_pair_open
  The pipe, then the client over end A and the server over end B.
*/
bool
d_tests_ssl_pair_open(
    struct d_tests_ssl_pair*    _pair,
    const struct d_ssl_context* _client,
    const struct d_ssl_context* _server,
    const char*                 _host
)
{
    const struct d_pack_text host = { _host,
                                      strlen(_host) };
    const struct d_pack_text none = { "",
                                      0u };

    memset(_pair,
           0,
           sizeof(*_pair));

    return ( (d_net_pipe_init(&_pair->pipe,
                              _pair->to_server,
                              sizeof(_pair->to_server),
                              _pair->to_client,
                              sizeof(_pair->to_client)) == D_NET_OK) &&
             (d_ssl_session_init(&_pair->client,
                                 _client,
                                 d_net_pipe_connection(&_pair->pipe,
                                                       D_NET_PIPE_A),
                                 host) == D_SSL_OK)                  &&
             (d_ssl_session_init(&_pair->server,
                                 _server,
                                 d_net_pipe_connection(&_pair->pipe,
                                                       D_NET_PIPE_B),
                                 none) == D_SSL_OK) );
}

/*
d_tests_ssl_pair_close
  Both sessions; the pipe holds nothing to release.
*/
void
d_tests_ssl_pair_close(
    struct d_tests_ssl_pair* _pair
)
{
    d_ssl_session_destroy(&_pair->client);
    d_ssl_session_destroy(&_pair->server);

    return;
}

/*
d_tests_ssl_pair_handshake
  Each side handshakes in turn while it would block. A side that fails stops
being called; the other then blocks for want of its peer until the rounds
run out, and reports WOULD_BLOCK.
*/
bool
d_tests_ssl_pair_handshake(
    struct d_tests_ssl_pair* _pair,
    enum d_ssl_error*        _client_status,
    enum d_ssl_error*        _server_status
)
{
    enum d_ssl_error client = D_SSL_ERROR_WOULD_BLOCK;
    enum d_ssl_error server = D_SSL_ERROR_WOULD_BLOCK;

    // turn by turn, until neither side would block
    for (unsigned int round = 0u;
         ( (round < D_TESTS_SSL_ROUNDS) &&
           ( (client == D_SSL_ERROR_WOULD_BLOCK) ||
             (server == D_SSL_ERROR_WOULD_BLOCK) ) );
         ++round)
    {
        // the client, while it waits
        if (client == D_SSL_ERROR_WOULD_BLOCK)
        {
            client = d_ssl_session_handshake(&_pair->client);
        }

        // the server, likewise
        if (server == D_SSL_ERROR_WOULD_BLOCK)
        {
            server = d_ssl_session_handshake(&_pair->server);
        }
    }

    *_client_status = client;
    *_server_status = server;

    return ( (client == D_SSL_OK) &&
             (server == D_SSL_OK) );
}

/*
d_tests_ssl_byte
  The byte at offset `_at` of every transfer: a pattern no record boundary
lines up with.
*/
static unsigned char
d_tests_ssl_byte(
    size_t _at
)
{
    return (unsigned char)((_at * 31u + 7u) % 251u);
}

/*
d_tests_ssl_pair_send
  One turn of a transfer's sending side: pending ciphertext first, then as
much new data as one write takes. False on any failure.
*/
static bool
d_tests_ssl_pair_send(
    struct d_ssl_session* _from,
    size_t*               _sent,
    size_t                _size
)
{
    unsigned char chunk[1000] = { 0 };
    size_t        taken       = 0u;

    // ciphertext an earlier write left behind goes first
    if (d_ssl_session_wants(_from) == D_SSL_WANT_WRITE)
    {
        const enum d_ssl_error flushed = d_ssl_session_flush(_from);

        return ( (flushed == D_SSL_OK) ||
                 (flushed == D_SSL_ERROR_WOULD_BLOCK) );
    }

    // nothing left to send
    if (*_sent == _size)
    {
        return true;
    }

    const size_t count = ( (_size - *_sent) < sizeof(chunk) )
                             ? (_size - *_sent)
                             : sizeof(chunk);

    // the pattern from where the transfer is
    for (size_t i = 0u;
         i < count;
         ++i)
    {
        chunk[i] = d_tests_ssl_byte(*_sent + i);
    }

    const enum d_ssl_error status = d_ssl_session_write(_from,
                                                        chunk,
                                                        count,
                                                        &taken);

    *_sent += taken;

    return ( (status == D_SSL_OK) ||
             (status == D_SSL_ERROR_WOULD_BLOCK) );
}

/*
d_tests_ssl_pair_receive
  One turn of a transfer's receiving side: what one read gives, checked
against the pattern. False on any failure or wrong byte.
*/
static bool
d_tests_ssl_pair_receive(
    struct d_ssl_session* _to,
    size_t*               _received
)
{
    unsigned char          chunk[1500] = { 0 };
    size_t                 count       = 0u;
    const enum d_ssl_error status      = d_ssl_session_read(_to,
                                                            chunk,
                                                            sizeof(chunk),
                                                            &count);

    // nothing yet is no failure
    if (status == D_SSL_ERROR_WOULD_BLOCK)
    {
        return true;
    }

    // every byte where the pattern puts it
    for (size_t i = 0u;
         i < count;
         ++i)
    {
        // a wrong byte ends the transfer
        if (chunk[i] != d_tests_ssl_byte(*_received + i))
        {
            return false;
        }
    }

    *_received += count;

    return (status == D_SSL_OK);
}

/*
d_tests_ssl_pair_move
  Sends and receives in turns until every byte has arrived, a side fails,
or the rounds run out.
*/
bool
d_tests_ssl_pair_move(
    struct d_ssl_session* _from,
    struct d_ssl_session* _to,
    size_t                _size
)
{
    size_t sent     = 0u;
    size_t received = 0u;
    bool   going    = true;

    // turn by turn, until everything has arrived
    for (unsigned int round = 0u;
         ( (going)            &&
           (received < _size) &&
           (round < D_TESTS_SSL_ROUNDS) );
         ++round)
    {
        going = ( (d_tests_ssl_pair_send(_from,
                                         &sent,
                                         _size)) &&
                  (d_tests_ssl_pair_receive(_to,
                                            &received)) );
    }

    return ( (going) &&
             (received == _size) );
}
