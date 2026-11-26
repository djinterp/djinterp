/*******************************************************************************
* djinterp [c][net]                                            lmtp_tests_live.c
*
* LMTP's client against a real LMTP server the environment names, such as
* Dovecot's: one session to each transport named, delivering to a mailbox
* that takes mail and, where named, to one the server refuses at RCPT and
* to one it refuses after the data, a mailbox over its quota, say.
*   DJINTERP_TEST_LMTP_SOCKET names a unix-domain socket's path, and
* DJINTERP_TEST_LMTP_HOST and _PORT a server over TCP; DJINTERP_TEST_LMTP_
* ACCEPTS the mailbox that takes mail, _REFUSES the one refused at RCPT, and
* _FULL the one refused after the data, each an address as RCPT gives it,
* local-part@domain. Without a server and ACCEPTS the test is skipped, as
* on CI.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests_live.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "./lmtp_tests.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <stdlib.h>   // strtoul
#include <string.h>   // strlen


// dj_tests_lmtp_live
//   struct: the mailboxes the environment names, in the envelope's order:
// the one that takes mail first, then those named of the other two.
struct dj_tests_lmtp_live
{
    char        accepts[256];
    char        refuses[256];
    char        full[256];
    const char* recipients[3];
    size_t      count;
    size_t      refuses_at;  // the refused one's place, or count
    size_t      full_at;     // the full one's place, or count
};

/*
d_tests_lmtp_live_names
  The mailboxes, from the environment; false without ACCEPTS.
*/
static
bool d_tests_lmtp_live_names(
    struct dj_tests_lmtp_live* _live
)
{
    _live->count = 0u;

    if (!d_tests_lmtp_setting("DJINTERP_TEST_LMTP_ACCEPTS",
                              _live->accepts,
                              sizeof(_live->accepts)))
    {
        return false;
    }

    _live->recipients[_live->count++] = _live->accepts;
    _live->refuses_at                 = 3u;
    _live->full_at                    = 3u;

    if (d_tests_lmtp_setting("DJINTERP_TEST_LMTP_REFUSES",
                             _live->refuses,
                             sizeof(_live->refuses)))
    {
        _live->refuses_at                 = _live->count;
        _live->recipients[_live->count++] = _live->refuses;
    }

    if (d_tests_lmtp_setting("DJINTERP_TEST_LMTP_FULL",
                             _live->full,
                             sizeof(_live->full)))
    {
        _live->full_at                    = _live->count;
        _live->recipients[_live->count++] = _live->full;
    }

    return true;
}

/*
d_tests_lmtp_live_round
  One session: connect, send to the named mailboxes, and quit; whether the
replies are what the names say - the accepting one delivered, the refused
one refused at RCPT and given no final reply, the full one accepted at RCPT
and refused after the data.
*/
static
bool d_tests_lmtp_live_round(
    struct dj_test_suite*                _suite,
    const char*                          _test,
    const struct dj_smtp_client_options* _options,
    const struct dj_tests_lmtp_live*     _live
)
{
    struct dj_smtp_recipient_status rcpt[3]     = { { .code = 0u } };
    struct dj_smtp_recipient_status delivery[3] = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .recipients = rcpt,
                                                   .deliveries = delivery };
    const struct dj_smtp_envelope   envelope    =
    {
        .sender          = "djinterp-tests@example.org",
        .recipients      = _live->recipients,
        .recipient_count = _live->count
    };
    struct dj_lmtp_client           client;
    static const char               MESSAGE[]   =
        "From: djinterp-tests@example.org\r\n"
        "Subject: djinterp's LMTP test\r\n\r\n"
        "Delivered by c_net_lmtp_tests.\r\n.a stuffed line\r\n";
    enum dj_smtp_error              sent        = D_SMTP_ERROR_STATE;
    bool                            result      = true;

    d_lmtp_client_init(&client);

    const enum dj_smtp_error opened = d_lmtp_client_connect(&client,
                                                           _options);

    if (opened == D_SMTP_OK)
    {
        sent = d_lmtp_client_send(&client,
                                  &envelope,
                                  MESSAGE,
                                  sizeof(MESSAGE) - 1u,
                                  &report);
    }

    // the reply the session ended on, before QUIT's replaces it
    (void)d_test_suite_note(_suite,
                            "%s: connect %d, send %d; last reply %u %s",
                            _test,
                            (int)opened,
                            (int)sent,
                            client.smtp.reply.code,
                            client.smtp.reply.text);

    if (opened == D_SMTP_OK)
    {
        (void)d_smtp_client_quit(&client.smtp);
    }

    d_smtp_client_close(&client.smtp);

    result = D_TESTS_NET_ASSERT( ( (opened == D_SMTP_OK) &&
                                   (sent == D_SMTP_OK) ),
                                 _test,
                                 "the session opens, and the message is "
                                 "delivered",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( ( ((rcpt[0].code / 100u) == 2u) &&
                                   ((delivery[0].code / 100u) == 2u) ),
                                 _test,
                                 "the mailbox that takes mail took it",
                                 _suite) && result;

    if (_live->refuses_at < _live->count)
    {
        result = D_TESTS_NET_ASSERT(
                     ( ((rcpt[_live->refuses_at].code / 100u) == 5u) &&
                       (delivery[_live->refuses_at].code == 0u) ),
                     _test,
                     "the unknown mailbox is refused at RCPT",
                     _suite) && result;
    }

    if (_live->full_at < _live->count)
    {
        const unsigned int final = delivery[_live->full_at].code;

        result = D_TESTS_NET_ASSERT(
                     ( ((rcpt[_live->full_at].code / 100u) == 2u) &&
                       ( ((final / 100u) == 4u) ||
                         ((final / 100u) == 5u) ) ),
                     _test,
                     "the full mailbox is refused after the data, alone",
                     _suite) && result;
    }

    return result;
}

/*
d_tests_lmtp_live
  Tests the following, against each transport the environment names:
  - LHLO, a pipelined transaction, and a final reply for each recipient
    from a real LMTP server
  - a mailbox that takes mail is delivered to; one unknown is refused at
    RCPT; one full is refused after the data while the first is delivered
*/
bool d_tests_lmtp_live(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_lmtp_live     live;
    struct dj_smtp_client_options options;
    char                          path[256]   = { 0 };
    char                          host[256]   = { 0 };
    char                          port[16]    = { 0 };
    bool                          ran         = false;
    bool                          result      = true;

    if (!d_tests_lmtp_live_names(&live))
    {
        return d_test_suite_skip(_suite,
                                 "DJINTERP_TEST_LMTP_ACCEPTS names no "
                                 "mailbox");
    }

    // over a unix-domain socket
    if (d_tests_lmtp_setting("DJINTERP_TEST_LMTP_SOCKET",
                             path,
                             sizeof(path)))
    {
        d_lmtp_client_options_init(&options);
        options.host  = path;
        options.local = true;
        ran           = true;
        result        = d_tests_lmtp_live_round(_suite,
                                                "live LMTP (unix)",
                                                &options,
                                                &live) && result;
    }

    // over TCP
    if ( (d_tests_lmtp_setting("DJINTERP_TEST_LMTP_HOST",
                               host,
                               sizeof(host))) &&
         (d_tests_lmtp_setting("DJINTERP_TEST_LMTP_PORT",
                               port,
                               sizeof(port))) )
    {
        d_lmtp_client_options_init(&options);
        options.host = host;
        options.port = (unsigned int)strtoul(port,
                                             NULL,
                                             10);
        ran          = true;
        result       = d_tests_lmtp_live_round(_suite,
                                               "live LMTP (TCP)",
                                               &options,
                                               &live) && result;
    }

    if (!ran)
    {
        return d_test_suite_skip(_suite,
                                 "neither DJINTERP_TEST_LMTP_SOCKET nor "
                                 "DJINTERP_TEST_LMTP_HOST and _PORT names a "
                                 "server");
    }

    return result;
}
