/*******************************************************************************
* djinterp [c][net]                                         lmtp_tests_support.c
*
* What net's LMTP tests share: a peer that says its script at once, a
* client opened as far as LHLO, the test server's handler, and the
* environment's settings.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "./lmtp_tests.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stdlib.h>   // getenv, free
#include <string.h>   // memcpy, memset, strlen, strncmp, strstr


/*
d_tests_lmtp_peer
  A peer whose whole script is released at once, and which hangs up once it
is read.
*/
struct dj_tests_net_peer* d_tests_lmtp_peer(
    const char* _script
)
{
    struct dj_tests_net_peer* peer = d_tests_net_peer_new();

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
d_tests_lmtp_open
  Attach, greeting and LHLO; the first failure ends it.
*/
enum dj_smtp_error
d_tests_lmtp_open(
    struct dj_lmtp_client*    _client,
    struct dj_net_connection* _connection
)
{
    d_lmtp_client_init(_client);

    enum dj_smtp_error result = d_lmtp_client_attach(_client,
                                                    _connection,
                                                    NULL);

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_greeting(&_client->smtp);
    }

    return (result == D_SMTP_OK) ? d_smtp_client_hello(&_client->smtp,
                                                       "c.example")
                                 : result;
}

/*
d_tests_lmtp_copy
  A text into a fixed field, cut to fit.
*/
static
void d_tests_lmtp_copy(
    char*       _out,
    size_t      _capacity,
    const char* _text,
    size_t      _length
)
{
    const size_t kept = (_length < _capacity) ? _length
                                              : (_capacity - 1u);

    memcpy(_out,
           _text,
           kept);
    _out[kept] = '\0';

    return;
}

/*
d_tests_lmtp_on_rcpt
  Refuses recipients whose local part starts with "reject".
*/
static
unsigned int d_tests_lmtp_on_rcpt(
    void*                              _context,
    const struct dj_smtp_session_info* _session,
    const char*                        _recipient
)
{
    struct dj_tests_lmtp_mailboxes* boxes = _context;

    d_tests_lmtp_copy(boxes->domain,
                      sizeof(boxes->domain),
                      _session->client_domain,
                      strlen(_session->client_domain));

    return (strncmp(_recipient,
                    "reject",
                    6u) == 0) ? 550u : 0u;
}

/*
d_tests_lmtp_on_deliver
  Records one recipient's delivery, refusing a full mailbox.
*/
static
unsigned int d_tests_lmtp_on_deliver(
    void*                              _context,
    const struct dj_smtp_session_info* _session,
    size_t                             _index,
    const char*                        _recipient,
    const char*                        _data,
    size_t                             _length
)
{
    struct dj_tests_lmtp_mailboxes* boxes = _context;
    const size_t                   at    = boxes->deliveries;

    (void)_session;

    if (at < D_TESTS_LMTP_MAILBOXES)
    {
        boxes->index[at] = _index;
        d_tests_lmtp_copy(boxes->recipient[at],
                          sizeof(boxes->recipient[at]),
                          _recipient,
                          strlen(_recipient));
    }

    boxes->deliveries += 1u;
    d_tests_lmtp_copy(boxes->data,
                      sizeof(boxes->data),
                      _data,
                      _length);

    return (strncmp(_recipient,
                    "full",
                    4u) == 0) ? 452u : 0u;
}

/*
d_tests_lmtp_on_message
  Takes a message once for every recipient, refusing one that asks.
*/
static
unsigned int d_tests_lmtp_on_message(
    void*                              _context,
    const struct dj_smtp_session_info* _session,
    const char*                        _data,
    size_t                             _length
)
{
    struct dj_tests_lmtp_mailboxes* boxes = _context;

    (void)_session;
    boxes->messages += 1u;
    d_tests_lmtp_copy(boxes->data,
                      sizeof(boxes->data),
                      _data,
                      _length);

    return (strstr(_data,
                   "refuse")) ? 554u : 0u;
}

/*
d_tests_lmtp_handler
  Recipients first, then the data, each or once; all of it recorded.
*/
void d_tests_lmtp_handler(
    struct dj_smtp_server_handler*  _handler,
    struct dj_tests_lmtp_mailboxes* _mailboxes,
    bool                            _each
)
{
    d_smtp_server_handler_init(_handler);
    memset(_mailboxes,
           0,
           sizeof(*_mailboxes));
    _handler->context = _mailboxes;
    _handler->rcpt    = d_tests_lmtp_on_rcpt;
    _handler->message = d_tests_lmtp_on_message;
    _handler->deliver = (_each) ? d_tests_lmtp_on_deliver : NULL;

    return;
}

/*
d_tests_lmtp_setting
  Through _dupenv_s under MSVC, which deprecates getenv, and getenv
elsewhere.
*/
bool d_tests_lmtp_setting(
    const char* _name,
    char*       _out,
    size_t      _capacity
)
{
#if defined(_MSC_VER)
    char*  value  = NULL;
    size_t size   = 0u;
    bool   stored = false;

    // a copy the runtime allocates, freed after
    if ( (_dupenv_s(&value,
                    &size,
                    _name) == 0) &&
         (value != NULL)         &&
         (value[0] != '\0')      &&
         (strlen(value) < _capacity) )
    {
        memcpy(_out,
               value,
               strlen(value) + 1u);
        stored = true;
    }

    free(value);

    return stored;
#else
    const char* const value = getenv(_name);

    // set, not empty, and one that fits
    if ( (!value)               ||
         (value[0] == '\0')     ||
         (strlen(value) >= _capacity) )
    {
        return false;
    }

    memcpy(_out,
           value,
           strlen(value) + 1u);

    return true;
#endif  // _MSC_VER
}
