/*******************************************************************************
* djinterp [c][net]                                          lmtp_tests_server.c
*
* LMTP's server over a scripted peer that plays the client: a reply for each
* recipient, from `deliver`; EHLO and HELO refused, and LHLO taken; DATA
* with no recipient accepted; one verdict from `message` for every
* recipient; the size limit, which refuses each; and SMTP's server, which
* knows no LHLO and keeps its 554.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests_server.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "./lmtp_tests.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <string.h>   // memcmp, strcmp, strlen


/*
d_tests_lmtp_said
  Whether the server's output holds every one of `_parts`, in order, each
after the one before; a NULL ends the list.
*/
static
bool d_tests_lmtp_said(
    const struct dj_tests_net_peer* _peer,
    const char* const*              _parts
)
{
    size_t at = 0u;

    // each part, from where the one before ended
    for (size_t part = 0u; _parts[part]; ++part)
    {
        const size_t length = strlen(_parts[part]);
        bool         found  = false;

        for (; ( (!found) &&
                 (at + length <= _peer->sent) ); ++at)
        {
            found = (memcmp(&_peer->output[at],
                            _parts[part],
                            length) == 0);
        }

        if (!found)
        {
            return false;
        }

        at += length - 1u;
    }

    return true;
}

/*
d_tests_lmtp_count
  How many times the server's output holds `_part`.
*/
static
size_t d_tests_lmtp_count(
    const struct dj_tests_net_peer* _peer,
    const char*                     _part
)
{
    const size_t length = strlen(_part);
    size_t       count  = 0u;

    for (size_t at = 0u; at + length <= _peer->sent; ++at)
    {
        count += (memcmp(&_peer->output[at],
                         _part,
                         length) == 0) ? 1u : 0u;
    }

    return count;
}

/*
d_tests_lmtp_serve
  One session of `_script`, in LMTP or, without `_lmtp`, in SMTP, with the
test handler; the peer is left for the caller to read and free, NULL when
it could not be made.
*/
static
struct dj_tests_net_peer* d_tests_lmtp_serve(
    const char*                     _script,
    bool                            _lmtp,
    bool                            _each,
    size_t                          _limit,
    struct dj_tests_lmtp_mailboxes* _mailboxes,
    enum dj_smtp_error*             _result
)
{
    struct dj_smtp_server_config  config;
    struct dj_smtp_server_handler handler;
    struct dj_tests_net_peer*     peer = d_tests_lmtp_peer(_script);

    *_result = D_SMTP_ERROR_STATE;

    if (!peer)
    {
        return NULL;
    }

    if (_lmtp)
    {
        d_lmtp_server_config_init(&config);
    }
    else
    {
        d_smtp_server_config_init(&config);
    }

    if (_limit > 0u)
    {
        config.max_message_size = _limit;
    }

    d_tests_lmtp_handler(&handler,
                         _mailboxes,
                         _each);
    *_result = (_lmtp) ? d_lmtp_server_session(
                             d_tests_net_peer_connection(peer,
                                                         true),
                             &config,
                             &handler)
                       : d_smtp_server_session(
                             d_tests_net_peer_connection(peer,
                                                         true),
                             &config,
                             &handler);

    return peer;
}

/*
d_tests_lmtp_server_each
  Tests the following:
  - the greeting says LMTP, and LHLO is answered as EHLO is, PIPELINING and
    ENHANCEDSTATUSCODES among what it offers
  - after the data, one reply for each recipient accepted, in their order:
    bob's and carol's 250, full's 452 from `deliver`
  - `deliver` is called for each, with its index, and `message` not at all
*/
bool d_tests_lmtp_server_each(
    struct dj_test_suite* _suite
)
{
    static const char* const FINALS[] =
    {
        "220 localhost LMTP ready\r\n",
        "250-localhost Hello c.example\r\n",
        "250-PIPELINING\r\n",
        "250-ENHANCEDSTATUSCODES\r\n",
        "550 ",
        "354 ",
        "250 2.0.0 Message accepted\r\n"
        "452 4.3.0 Message rejected\r\n"
        "250 2.0.0 Message accepted\r\n"
        "221 2.0.0 Closing connection\r\n",
        NULL
    };
    const char* const              TEST  = "LMTP server (each recipient)";
    struct dj_tests_lmtp_mailboxes boxes;
    enum dj_smtp_error             ended = D_SMTP_ERROR_STATE;
    struct dj_tests_net_peer*      peer  = d_tests_lmtp_serve(
        "LHLO c.example\r\nMAIL FROM:<alice@example.org>\r\n"
        "RCPT TO:<bob@example.com>\r\nRCPT TO:<reject@example.com>\r\n"
        "RCPT TO:<full@example.com>\r\nRCPT TO:<carol@example.com>\r\n"
        "DATA\r\nSubject: hi\r\n\r\nbody\r\n.\r\nQUIT\r\n",
        true,
        true,
        0u,
        &boxes,
        &ended);
    bool                           result = true;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    result = D_TESTS_NET_ASSERT( ( (ended == D_SMTP_OK) &&
                                   (d_tests_lmtp_said(peer,
                                                      FINALS)) ),
                                 TEST,
                                 "LMTP's greeting, LHLO, and a reply for "
                                 "each recipient",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT(
                 ( (boxes.deliveries == 3u)                         &&
                   (boxes.messages == 0u)                           &&
                   (boxes.index[0] == 0u)                           &&
                   (boxes.index[1] == 1u)                           &&
                   (boxes.index[2] == 2u)                           &&
                   (strcmp(boxes.recipient[0],
                           "bob@example.com") == 0)                 &&
                   (strcmp(boxes.recipient[1],
                           "full@example.com") == 0)                &&
                   (strcmp(boxes.recipient[2],
                           "carol@example.com") == 0)               &&
                   (strcmp(boxes.data,
                           "Subject: hi\r\n\r\nbody\r\n") == 0) ),
                 TEST,
                 "deliver is called for each recipient accepted, in order",
                 _suite) && result;
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_lmtp_server_hellos
  Tests the following:
  - EHLO and HELO are refused with 500, never with a positive reply (RFC
    2033 section 4.1), and count as the client's errors
  - MAIL before LHLO is out of sequence
  - LHLO then opens the session
*/
bool d_tests_lmtp_server_hellos(
    struct dj_test_suite* _suite
)
{
    static const char* const ORDER[] =
    {
        "220 localhost LMTP ready\r\n",
        "500 5.5.1 This is LMTP: say LHLO\r\n",
        "500 5.5.1 This is LMTP: say LHLO\r\n",
        "503 5.5.1 Bad sequence of commands\r\n",
        "250-localhost Hello c.example\r\n",
        NULL
    };
    const char* const              TEST  = "LMTP server (hellos)";
    struct dj_tests_lmtp_mailboxes boxes;
    enum dj_smtp_error             ended = D_SMTP_ERROR_STATE;
    struct dj_tests_net_peer*      peer  = d_tests_lmtp_serve(
        "EHLO c.example\r\nHELO c.example\r\nMAIL FROM:<a@example.org>\r\n"
        "LHLO c.example\r\nQUIT\r\n",
        true,
        true,
        0u,
        &boxes,
        &ended);

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (ended == D_SMTP_OK)                    &&
                            (d_tests_lmtp_said(peer,
                                               ORDER))              &&
                            (d_tests_lmtp_count(peer,
                                                "250") ==
                             d_tests_lmtp_count(peer,
                                                "250-") + 1u) ),
                          TEST,
                          "EHLO and HELO refused, LHLO taken",
                          _suite);

    d_tests_net_peer_free(peer);

    return held;
}

/*
d_tests_lmtp_server_no_recipient
  Tests the following:
  - DATA with no recipient accepted fails with 503 (RFC 2033 section 4.2),
    where SMTP's 554 would be, and no data is read
*/
bool d_tests_lmtp_server_no_recipient(
    struct dj_test_suite* _suite
)
{
    static const char* const ORDER[] =
    {
        "550 ",
        "503 5.5.1 No valid recipients\r\n",
        "221 ",
        NULL
    };
    const char* const              TEST  = "LMTP server (no recipient)";
    struct dj_tests_lmtp_mailboxes boxes;
    enum dj_smtp_error             ended = D_SMTP_ERROR_STATE;
    struct dj_tests_net_peer*      peer  = d_tests_lmtp_serve(
        "LHLO c.example\r\nMAIL FROM:<a@example.org>\r\n"
        "RCPT TO:<reject@example.com>\r\nDATA\r\nQUIT\r\n",
        true,
        true,
        0u,
        &boxes,
        &ended);

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (ended == D_SMTP_OK)            &&
                            (d_tests_lmtp_said(peer,
                                               ORDER))      &&
                            (d_tests_lmtp_count(peer,
                                                "354") == 0u) &&
                            (boxes.deliveries == 0u) ),
                          TEST,
                          "503, and no data",
                          _suite);

    d_tests_net_peer_free(peer);

    return held;
}

/*
d_tests_lmtp_server_one_verdict
  Tests the following:
  - without `deliver`, `message` decides once, and its verdict is every
    recipient's: two 554s for a message it refuses, two 250s for one it
    takes
*/
bool d_tests_lmtp_server_one_verdict(
    struct dj_test_suite* _suite
)
{
    static const char* const ORDER[] =
    {
        "354 ",
        "554 5.6.0 Message rejected\r\n554 5.6.0 Message rejected\r\n",
        "354 ",
        "250 2.0.0 Message accepted\r\n250 2.0.0 Message accepted\r\n",
        "221 ",
        NULL
    };
    const char* const              TEST  = "LMTP server (one verdict)";
    struct dj_tests_lmtp_mailboxes boxes;
    enum dj_smtp_error             ended = D_SMTP_ERROR_STATE;
    struct dj_tests_net_peer*      peer  = d_tests_lmtp_serve(
        "LHLO c.example\r\nMAIL FROM:<a@example.org>\r\n"
        "RCPT TO:<b@example.com>\r\nRCPT TO:<c@example.com>\r\n"
        "DATA\r\nplease refuse this\r\n.\r\n"
        "MAIL FROM:<a@example.org>\r\n"
        "RCPT TO:<b@example.com>\r\nRCPT TO:<c@example.com>\r\n"
        "DATA\r\ntake this\r\n.\r\nQUIT\r\n",
        true,
        false,
        0u,
        &boxes,
        &ended);

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (ended == D_SMTP_OK)       &&
                            (d_tests_lmtp_said(peer,
                                               ORDER)) &&
                            (boxes.messages == 2u)     &&
                            (boxes.deliveries == 0u) ),
                          TEST,
                          "one verdict, every recipient's",
                          _suite);

    d_tests_net_peer_free(peer);

    return held;
}

/*
d_tests_lmtp_server_oversize
  Tests the following:
  - data over the size limit is refused for every recipient, and `deliver`
    is not asked
*/
bool d_tests_lmtp_server_oversize(
    struct dj_test_suite* _suite
)
{
    static const char* const ORDER[] =
    {
        "354 ",
        "552 5.3.4 Message size exceeds fixed maximum message size\r\n"
        "552 5.3.4 Message size exceeds fixed maximum message size\r\n",
        NULL
    };
    const char* const              TEST  = "LMTP server (oversize)";
    struct dj_tests_lmtp_mailboxes boxes;
    enum dj_smtp_error             ended = D_SMTP_ERROR_STATE;
    struct dj_tests_net_peer*      peer  = d_tests_lmtp_serve(
        "LHLO c.example\r\nMAIL FROM:<a@example.org>\r\n"
        "RCPT TO:<b@example.com>\r\nRCPT TO:<c@example.com>\r\n"
        "DATA\r\nthis line is longer than sixteen bytes\r\n.\r\nQUIT\r\n",
        true,
        true,
        16u,
        &boxes,
        &ended);

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (ended == D_SMTP_OK)       &&
                            (d_tests_lmtp_said(peer,
                                               ORDER)) &&
                            (boxes.deliveries == 0u) ),
                          TEST,
                          "552 for each, deliver not asked",
                          _suite);

    d_tests_net_peer_free(peer);

    return held;
}

/*
d_tests_lmtp_server_smtp
  Tests the following:
  - SMTP's server knows no LHLO, and still says EHLO's reply to EHLO
  - its DATA with no recipient accepted is still 554 (RFC 2920)
  - its greeting still says ESMTP
*/
bool d_tests_lmtp_server_smtp(
    struct dj_test_suite* _suite
)
{
    static const char* const ORDER[] =
    {
        "220 localhost ESMTP ready\r\n",
        "500 5.5.2 Command not recognized\r\n",
        "250-localhost Hello c.example\r\n",
        "554 5.5.1 No valid recipients\r\n",
        NULL
    };
    const char* const              TEST  = "SMTP server (no LHLO)";
    struct dj_tests_lmtp_mailboxes boxes;
    enum dj_smtp_error             ended = D_SMTP_ERROR_STATE;
    struct dj_tests_net_peer*      peer  = d_tests_lmtp_serve(
        "LHLO c.example\r\nEHLO c.example\r\nMAIL FROM:<a@example.org>\r\n"
        "DATA\r\nQUIT\r\n",
        false,
        true,
        0u,
        &boxes,
        &ended);

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (ended == D_SMTP_OK) &&
                            (d_tests_lmtp_said(peer,
                                               ORDER)) ),
                          TEST,
                          "LHLO unknown, 554 kept",
                          _suite);

    d_tests_net_peer_free(peer);

    return held;
}
