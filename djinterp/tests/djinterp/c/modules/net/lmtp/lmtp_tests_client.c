/*******************************************************************************
* djinterp [c][net]                                          lmtp_tests_client.c
*
* LMTP's client over a scripted peer: LHLO and a pipelined transaction with
* a reply for each recipient; every recipient refused; a server that hangs
* up between replies; no HELO after a refused LHLO; the transaction one
* command at a time; the data of a transaction the caller opened; the
* arguments refused; and SMTP's deliveries, which the same report carries.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "./lmtp_tests.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <string.h>   // strlen


// RECIPIENTS
//   constant: the envelope most tests send: one the server takes, one it
// refuses at RCPT, and one it refuses after the data.
static const char* const RECIPIENTS[] = { "bob@example.com",
                                          "nobody@example.com",
                                          "full@example.com" };

// GREETING
//   constant: an LMTP server's greeting and its LHLO reply, with PIPELINING.
#define GREETING                                                               \
    "220 lmtp.example LMTP ready\r\n"                                          \
    "250-lmtp.example\r\n250-PIPELINING\r\n250-ENHANCEDSTATUSCODES\r\n"        \
    "250 8BITMIME\r\n"

/*
d_tests_lmtp_envelope
  An envelope from alice to `_count` of RECIPIENTS.
*/
static
struct dj_smtp_envelope d_tests_lmtp_envelope(
    size_t _count
)
{
    struct dj_smtp_envelope envelope = { .sender = "alice@example.org" };

    envelope.recipients      = RECIPIENTS;
    envelope.recipient_count = _count;

    return envelope;
}

/*
d_tests_lmtp_client_pipelined
  Tests the following:
  - LHLO, never EHLO, opens the session
  - MAIL, three RCPTs and DATA leave in one write
  - after the data, one reply for each recipient accepted, in their order:
    the report's deliveries have bob's 250, nobody's 0, since it was
    refused at RCPT, and full's 452, with its enhanced status
  - one delivered is success, and the session goes on to QUIT
*/
bool d_tests_lmtp_client_pipelined(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST          =
        "dj_lmtp_client (pipelined)";
    const struct dj_smtp_envelope   envelope      = d_tests_lmtp_envelope(3u);
    struct dj_smtp_recipient_status rcpt[3]       = { { .code = 0u } };
    struct dj_smtp_recipient_status delivery[3]   = { { .code = 0u } };
    struct dj_smtp_send_report      report        = { .recipients = rcpt,
                                                     .deliveries = delivery };
    struct dj_lmtp_client           client;
    struct dj_tests_net_peer*       peer          = d_tests_lmtp_peer(
        GREETING
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n550 5.1.1 No such user\r\n"
        "250 2.1.5 OK\r\n354 OK\r\n"
        "250 2.0.0 <bob@example.com> Saved\r\n"
        "452 4.2.2 <full@example.com> Mailbox full\r\n"
        "221 2.0.0 Bye\r\n");
    enum dj_smtp_error              step          = D_SMTP_ERROR_STATE;
    enum dj_smtp_error              sent          = D_SMTP_ERROR_STATE;
    bool                            result        = true;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    peer->recv_chunk = 7u;
    step = d_tests_lmtp_open(&client,
                             d_tests_net_peer_connection(peer,
                                                         true));

    if (step == D_SMTP_OK)
    {
        sent = d_lmtp_client_send(&client,
                                  &envelope,
                                  "Subject: hi\r\n\r\nbody",
                                  strlen("Subject: hi\r\n\r\nbody"),
                                  &report);
        step = d_smtp_client_quit(&client.smtp);
    }

    result = D_TESTS_NET_ASSERT(
                 ( (sent == D_SMTP_OK) &&
                   (step == D_SMTP_OK) &&
                   (d_tests_net_peer_sent(
                        peer,
                        "LHLO c.example\r\n"
                        "MAIL FROM:<alice@example.org>\r\n"
                        "RCPT TO:<bob@example.com>\r\n"
                        "RCPT TO:<nobody@example.com>\r\n"
                        "RCPT TO:<full@example.com>\r\n"
                        "DATA\r\n"
                        "Subject: hi\r\n\r\nbody\r\n.\r\n"
                        "QUIT\r\n")) ),
                 TEST,
                 "LHLO, the pipelined transaction, the data, QUIT",
                 _suite) && result;
    result = D_TESTS_NET_ASSERT( (peer->writes == 5),
                                 TEST,
                                 "MAIL, the RCPTs and DATA leave in one "
                                 "write",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( ( (rcpt[0].code == 250u) &&
                                   (rcpt[1].code == 550u) &&
                                   (rcpt[2].code == 250u) &&
                                   (report.accepted == 2u) ),
                                 TEST,
                                 "each RCPT's reply is kept",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT(
                 ( (delivery[0].code == 250u)               &&
                   (delivery[1].code == 0u)                 &&
                   (delivery[2].code == 452u)               &&
                   (delivery[2].status.status_class == 4u)   &&
                   (delivery[2].status.subject == 2u)       &&
                   (delivery[2].status.detail == 2u)        &&
                   (report.delivered == 1u)                 &&
                   (report.data_code == 452u) ),
                 TEST,
                 "each recipient's own final reply is kept",
                 _suite) && result;
    d_smtp_client_close(&client.smtp);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_lmtp_client_refused
  Tests the following:
  - with every recipient refused after the data, the send is refused, the
    last refusal left in the client's reply
  - the session stays in step: the transaction is over, and QUIT follows
*/
bool d_tests_lmtp_client_refused(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST        = "dj_lmtp_client (refused)";
    static const char* const        PAIR[]      = { "bob@example.com",
                                                    "carol@example.com" };
    struct dj_smtp_envelope         two         = d_tests_lmtp_envelope(1u);
    struct dj_smtp_recipient_status pair[2]     = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .deliveries = pair };
    struct dj_lmtp_client           client;
    struct dj_tests_net_peer*       peer        = d_tests_lmtp_peer(
        GREETING
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n250 2.1.5 OK\r\n354 OK\r\n"
        "452 4.2.2 Mailbox full\r\n550 5.7.1 Refused\r\n"
        "221 2.0.0 Bye\r\n");
    enum dj_smtp_error              sent        = D_SMTP_OK;
    enum dj_smtp_error              quit        = D_SMTP_ERROR_STATE;
    bool                            result      = true;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    two.recipients      = PAIR;
    two.recipient_count = 2u;

    if (d_tests_lmtp_open(&client,
                          d_tests_net_peer_connection(peer,
                                                      true)) == D_SMTP_OK)
    {
        sent = d_lmtp_client_send(&client,
                                  &two,
                                  "x",
                                  1u,
                                  &report);
        quit = d_smtp_client_quit(&client.smtp);
    }

    result = D_TESTS_NET_ASSERT( ( (sent == D_SMTP_ERROR_REJECTED) &&
                                   (report.delivered == 0u)        &&
                                   (pair[0].code == 452u)          &&
                                   (pair[1].code == 550u) ),
                                 TEST,
                                 "no mailbox took it: refused",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( (quit == D_SMTP_OK),
                                 TEST,
                                 "the session stays in step",
                                 _suite) && result;
    d_smtp_client_close(&client.smtp);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_lmtp_client_hang_up
  Tests the following:
  - a server that hangs up after one of two final replies: the one that
    came is kept and counted, the other is 0, a temporary failure (RFC 2033
    section 5), and the send reports the connection closed
*/
bool d_tests_lmtp_client_hang_up(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST        = "dj_lmtp_client (hang-up)";
    static const char* const        PAIR[]      = { "bob@example.com",
                                                    "carol@example.com" };
    struct dj_smtp_envelope         envelope    = d_tests_lmtp_envelope(1u);
    struct dj_smtp_recipient_status delivery[2] = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .deliveries = delivery };
    struct dj_lmtp_client           client;
    struct dj_tests_net_peer*       peer        = d_tests_lmtp_peer(
        GREETING
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n250 2.1.5 OK\r\n354 OK\r\n"
        "250 2.0.0 <bob@example.com> Saved\r\n");
    enum dj_smtp_error              sent        = D_SMTP_OK;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    envelope.recipients      = PAIR;
    envelope.recipient_count = 2u;

    if (d_tests_lmtp_open(&client,
                          d_tests_net_peer_connection(peer,
                                                      true)) == D_SMTP_OK)
    {
        sent = d_lmtp_client_send(&client,
                                  &envelope,
                                  "x",
                                  1u,
                                  &report);
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (sent == D_SMTP_ERROR_CLOSED)  &&
                            (delivery[0].code == 250u)     &&
                            (delivery[1].code == 0u)       &&
                            (report.delivered == 1u)       &&
                            (report.data_code == 250u) ),
                          TEST,
                          "the reply that came is kept, the other is 0",
                          _suite);

    d_smtp_client_close(&client.smtp);
    d_tests_net_peer_free(peer);

    return held;
}

/*
d_tests_lmtp_client_no_helo
  Tests the following:
  - an LMTP client whose LHLO is refused says nothing after it: an LMTP
    server never accepts HELO
  - an SMTP client on the same refusal falls back to HELO, as before
*/
bool d_tests_lmtp_client_no_helo(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST   = "dj_lmtp_client (no HELO)";
    struct dj_lmtp_client     client;
    struct dj_smtp_client     smtp;
    struct dj_tests_net_peer* lmtp   = d_tests_lmtp_peer(
        "220 lmtp.example LMTP ready\r\n500 5.5.1 Unknown command\r\n");
    struct dj_tests_net_peer* esmtp  = d_tests_lmtp_peer(
        "220 mail.example\r\n500 5.5.1 Unknown command\r\n"
        "250 mail.example\r\n");
    enum dj_smtp_error        said   = D_SMTP_OK;
    enum dj_smtp_error        fell   = D_SMTP_ERROR_STATE;
    bool                      result = true;

    if (!D_TESTS_NET_ASSERT( ( (lmtp != NULL) &&
                               (esmtp != NULL) ),
                             TEST,
                             "two scripted peers",
                             _suite))
    {
        d_tests_net_peer_free(lmtp);
        d_tests_net_peer_free(esmtp);

        return false;
    }

    said = d_tests_lmtp_open(&client,
                             d_tests_net_peer_connection(lmtp,
                                                         true));
    d_smtp_client_init(&smtp);

    if (d_smtp_client_attach(&smtp,
                             d_tests_net_peer_connection(esmtp,
                                                         true),
                             false) == D_SMTP_OK)
    {
        (void)d_smtp_client_greeting(&smtp);
        fell = d_smtp_client_hello(&smtp,
                                   "c.example");
    }

    result = D_TESTS_NET_ASSERT( ( (said == D_SMTP_ERROR_REJECTED)    &&
                                   (client.smtp.reply.code == 500u)   &&
                                   (d_tests_net_peer_sent(
                                        lmtp,
                                        "LHLO c.example\r\n")) ),
                                 TEST,
                                 "LHLO refused, and nothing after",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( ( (fell == D_SMTP_OK) &&
                                   (d_tests_net_peer_sent(
                                        esmtp,
                                        "EHLO c.example\r\n"
                                        "HELO c.example\r\n")) ),
                                 TEST,
                                 "SMTP still falls back to HELO",
                                 _suite) && result;
    d_smtp_client_close(&client.smtp);
    d_smtp_client_close(&smtp);
    d_tests_net_peer_free(lmtp);
    d_tests_net_peer_free(esmtp);

    return result;
}

/*
d_tests_lmtp_client_serial
  Tests the following:
  - without PIPELINING, one command at a time, and still a reply for each
    recipient after the data
*/
bool d_tests_lmtp_client_serial(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST        = "dj_lmtp_client (serial)";
    static const char* const        PAIR[]      = { "bob@example.com",
                                                    "carol@example.com" };
    struct dj_smtp_envelope         envelope    = d_tests_lmtp_envelope(1u);
    struct dj_smtp_recipient_status delivery[2] = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .deliveries = delivery };
    struct dj_lmtp_client           client;
    struct dj_tests_net_peer*       peer        = d_tests_lmtp_peer(
        "220 lmtp.example LMTP ready\r\n250 lmtp.example\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n250 2.1.5 OK\r\n354 OK\r\n"
        "250 2.0.0 one\r\n250 2.0.0 two\r\n");
    enum dj_smtp_error              sent        = D_SMTP_ERROR_STATE;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    envelope.recipients      = PAIR;
    envelope.recipient_count = 2u;

    if (d_tests_lmtp_open(&client,
                          d_tests_net_peer_connection(peer,
                                                      true)) == D_SMTP_OK)
    {
        sent = d_lmtp_client_send(&client,
                                  &envelope,
                                  "hi",
                                  2u,
                                  &report);
    }

    const bool held = D_TESTS_NET_ASSERT(
                          ( (sent == D_SMTP_OK)          &&
                            (delivery[0].code == 250u)   &&
                            (delivery[1].code == 250u)   &&
                            (report.delivered == 2u)     &&
                            (peer->writes > 4)           &&
                            (d_tests_net_peer_sent(
                                 peer,
                                 "LHLO c.example\r\n"
                                 "MAIL FROM:<alice@example.org>\r\n"
                                 "RCPT TO:<bob@example.com>\r\n"
                                 "RCPT TO:<carol@example.com>\r\n"
                                 "DATA\r\nhi\r\n.\r\n")) ),
                          TEST,
                          "one command at a time, a reply for each",
                          _suite);

    d_smtp_client_close(&client.smtp);
    d_tests_net_peer_free(peer);

    return held;
}

/*
d_tests_lmtp_client_data
  Tests the following:
  - a transaction the caller opens with MAIL and RCPT: d_lmtp_client_data
    keeps each final reply in order, and counts them
  - d_smtp_client_data on an LMTP client reads every final reply too, so
    the session stays in step for the next transaction and QUIT
*/
bool d_tests_lmtp_client_data(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST       = "dj_lmtp_client (data)";
    const struct dj_smtp_envelope   envelope   = d_tests_lmtp_envelope(1u);
    struct dj_smtp_recipient_status replies[3] = { { .code = 0u } };
    struct dj_lmtp_client           client;
    struct dj_tests_net_peer*       peer       = d_tests_lmtp_peer(
        GREETING
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n550 5.1.1 No\r\n250 2.1.5 OK\r\n"
        "354 OK\r\n250 2.0.0 a\r\n550 5.2.1 b\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n250 2.1.5 OK\r\n354 OK\r\n"
        "250 2.0.0 c\r\n250 2.0.0 d\r\n221 2.0.0 Bye\r\n");
    size_t                          count      = 9u;
    enum dj_smtp_error              first      = D_SMTP_ERROR_STATE;
    enum dj_smtp_error              second     = D_SMTP_ERROR_STATE;
    enum dj_smtp_error              quit       = D_SMTP_ERROR_STATE;
    bool                            result     = true;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    if (d_tests_lmtp_open(&client,
                          d_tests_net_peer_connection(peer,
                                                      true)) == D_SMTP_OK)
    {
        (void)d_smtp_client_mail(&client.smtp,
                                 &envelope,
                                 0u);
        (void)d_smtp_client_rcpt(&client.smtp,
                                 "bob@example.com");
        (void)d_smtp_client_rcpt(&client.smtp,
                                 "nobody@example.com");
        (void)d_smtp_client_rcpt(&client.smtp,
                                 "carol@example.com");
        first = d_lmtp_client_data(&client,
                                   "x",
                                   1u,
                                   replies,
                                   3u,
                                   &count);
        (void)d_smtp_client_mail(&client.smtp,
                                 &envelope,
                                 0u);
        (void)d_smtp_client_rcpt(&client.smtp,
                                 "bob@example.com");
        (void)d_smtp_client_rcpt(&client.smtp,
                                 "carol@example.com");
        second = d_smtp_client_data(&client.smtp,
                                    "y",
                                    1u);
        quit   = d_smtp_client_quit(&client.smtp);
    }

    result = D_TESTS_NET_ASSERT( ( (first == D_SMTP_OK)       &&
                                   (count == 2u)              &&
                                   (replies[0].code == 250u)  &&
                                   (replies[1].code == 550u)  &&
                                   (replies[2].code == 0u) ),
                                 TEST,
                                 "a reply for each accepted, in order",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( ( (second == D_SMTP_OK) &&
                                   (quit == D_SMTP_OK) ),
                                 TEST,
                                 "d_smtp_client_data reads them all too",
                                 _suite) && result;
    d_smtp_client_close(&client.smtp);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_lmtp_client_guards
  Tests the following:
  - LMTP's options: its dialect, no port, STARTTLS when offered, TCP
  - port 0 and port 25 are refused before anything connects, by
    d_lmtp_client_connect and by the kernel's connect alike, and a dialect
    the enum does not list
  - send and data refuse a client that does not speak LMTP, and NULL
*/
bool d_tests_lmtp_client_guards(
    struct dj_test_suite* _suite
)
{
    const char* const             TEST     = "dj_lmtp_client (guards)";
    const struct dj_smtp_envelope envelope = d_tests_lmtp_envelope(1u);
    struct dj_smtp_client_options options;
    struct dj_lmtp_client         client;
    struct dj_smtp_client         smtp;
    size_t                        count    = 9u;
    bool                          result   = true;

    d_lmtp_client_options_init(&options);
    result = D_TESTS_NET_ASSERT(
                 ( (options.dialect == D_SMTP_DIALECT_LMTP)                &&
                   (options.port == 0u)                                    &&
                   (options.security == D_SMTP_SECURITY_STARTTLS_OPTIONAL) &&
                   (!options.local) ),
                 TEST,
                 "LMTP's defaults",
                 _suite) && result;

    d_lmtp_client_init(&client);
    options.host = "127.0.0.1";
    result = D_TESTS_NET_ASSERT(
                 (d_lmtp_client_connect(&client,
                                        &options) ==
                  D_SMTP_ERROR_INVALID_ARGUMENT),
                 TEST,
                 "no port: refused",
                 _suite) && result;
    options.port = D_LMTP_PORT_FORBIDDEN;
    result = D_TESTS_NET_ASSERT(
                 (d_lmtp_client_connect(&client,
                                        &options) ==
                  D_SMTP_ERROR_INVALID_ARGUMENT),
                 TEST,
                 "port 25: refused",
                 _suite) && result;
    d_smtp_client_init(&smtp);
    result = D_TESTS_NET_ASSERT(
                 (d_smtp_client_connect(&smtp,
                                        &options) ==
                  D_SMTP_ERROR_INVALID_ARGUMENT),
                 TEST,
                 "the kernel refuses LMTP on port 25 too",
                 _suite) && result;
    options.dialect = (enum dj_smtp_dialect)7;
    result = D_TESTS_NET_ASSERT(
                 (d_smtp_client_connect(&smtp,
                                        &options) ==
                  D_SMTP_ERROR_INVALID_ARGUMENT),
                 TEST,
                 "a dialect the enum does not list: refused",
                 _suite) && result;
    result = D_TESTS_NET_ASSERT( ( (client.smtp.state ==
                                    D_SMTP_CLIENT_CLOSED)  &&
                                   (client.smtp.dialect ==
                                    D_SMTP_DIALECT_LMTP) ),
                                 TEST,
                                 "the refused client is still closed, and "
                                 "LMTP's",
                                 _suite) && result;

    client.smtp.dialect = D_SMTP_DIALECT_SMTP;
    result = D_TESTS_NET_ASSERT(
                 ( (d_lmtp_client_send(&client,
                                       &envelope,
                                       "x",
                                       1u,
                                       NULL) == D_SMTP_ERROR_STATE)   &&
                   (d_lmtp_client_data(&client,
                                       "x",
                                       1u,
                                       NULL,
                                       0u,
                                       &count) == D_SMTP_ERROR_STATE) &&
                   (count == 0u) ),
                 TEST,
                 "a client not speaking LMTP: refused",
                 _suite) && result;
    result = D_TESTS_NET_ASSERT(
                 ( (d_lmtp_client_send(NULL,
                                       &envelope,
                                       "x",
                                       1u,
                                       NULL) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) &&
                   (d_lmtp_client_connect(NULL,
                                          &options) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) &&
                   (d_lmtp_client_attach(NULL,
                                         NULL,
                                         NULL) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) ),
                 TEST,
                 "NULL: refused",
                 _suite) && result;

    return result;
}

/*
d_tests_lmtp_smtp_deliveries
  Tests the following:
  - in SMTP, the report's deliveries hold the one reply to the data for
    every recipient accepted, and 0 for the one refused
  - a refused final reply is every accepted recipient's, and refuses the
    send
*/
bool d_tests_lmtp_smtp_deliveries(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST        = "dj_smtp_client (deliveries)";
    const struct dj_smtp_envelope   envelope    = d_tests_lmtp_envelope(3u);
    struct dj_smtp_recipient_status delivery[3] = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .deliveries = delivery };
    struct dj_smtp_client           client;
    struct dj_tests_net_peer*       peer        = d_tests_lmtp_peer(
        "220 mail.example ESMTP\r\n"
        "250-mail.example\r\n250-PIPELINING\r\n250 ENHANCEDSTATUSCODES\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n550 5.1.1 No such user\r\n"
        "250 2.1.5 OK\r\n354 OK\r\n250 2.0.0 Queued\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n550 5.1.1 No such user\r\n"
        "250 2.1.5 OK\r\n354 OK\r\n554 5.6.0 Refused\r\n250 2.0.0 OK\r\n");
    enum dj_smtp_error              first       = D_SMTP_ERROR_STATE;
    enum dj_smtp_error              second      = D_SMTP_ERROR_STATE;
    bool                            once        = false;
    bool                            result      = true;

    if (!D_TESTS_NET_ASSERT( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _suite))
    {
        return false;
    }

    d_smtp_client_init(&client);

    if ( (d_smtp_client_attach(&client,
                               d_tests_net_peer_connection(peer,
                                                           true),
                               false) == D_SMTP_OK) &&
         (d_smtp_client_greeting(&client) == D_SMTP_OK) &&
         (d_smtp_client_hello(&client,
                              "c.example") == D_SMTP_OK) )
    {
        first = d_smtp_client_send(&client,
                                   &envelope,
                                   "x",
                                   1u,
                                   &report);
        once  = ( (delivery[0].code == 250u) &&
                  (delivery[1].code == 0u)   &&
                  (delivery[2].code == 250u) &&
                  (report.delivered == 2u) );
        second = d_smtp_client_send(&client,
                                    &envelope,
                                    "x",
                                    1u,
                                    &report);
    }

    result = D_TESTS_NET_ASSERT( ( (first == D_SMTP_OK) &&
                                   (once) ),
                                 TEST,
                                 "one reply for every recipient accepted",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( ( (second == D_SMTP_ERROR_REJECTED) &&
                                   (delivery[0].code == 554u)        &&
                                   (delivery[1].code == 0u)          &&
                                   (delivery[2].code == 554u)        &&
                                   (report.delivered == 0u)          &&
                                   (report.data_code == 554u) ),
                                 TEST,
                                 "a refusal for every recipient accepted",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT( (d_smtp_client_noop(&client) ==
                                  D_SMTP_OK),
                                 TEST,
                                 "the session stays in step",
                                 _suite) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);

    return result;
}
