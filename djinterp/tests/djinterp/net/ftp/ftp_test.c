/*******************************************************************************
* djinterp [net]                                                      ftp_test.c
*
* Exercises `ftp.h`, and the client's connection helpers in its private
* ftp_client_internal.h over a scripted transport.
*
*
* path:      /tests/djinterp/net/ftp/ftp_test.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.16
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf
#include <string.h>   // memcmp, memcpy, memset, strcmp, strlen
#include <time.h>     // clock_gettime, struct timespec
// POSIX
#include <arpa/inet.h>   // htonl
#include <netinet/in.h>  // struct sockaddr_in
#include <sys/socket.h>  // socket, bind, listen, getsockname
#include <unistd.h>      // close
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp.h"          // d_ftp_*
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"  // the OpenSSL engine
#include "../../../../src/djinterp/net/ftp/ftp_client_internal.h"
                           // the client's private connection helpers


static int g_fail   = 0;
static int g_checks = 0;

/*
d_tests_ftp_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_ftp_check(
    bool        _ok,
    const char* _text
)
{
    g_checks++;

    // a failed check says which
    if (!_ok)
    {
        g_fail++;
        printf("FAIL: %s\n",
               _text);
    }

    return;
}

// D_TESTS_FTP_CHECK
//   macro: one check, its condition's text naming it when it fails.
#define D_TESTS_FTP_CHECK(c) d_tests_ftp_check((c), #c)

/*
d_tests_ftp_span_is
  Whether a span holds exactly the literal's characters.
*/
static bool
d_tests_ftp_span_is(
    struct d_pack_text _span,
    const char*        _literal
)
{
    const size_t length = strlen(_literal);

    return ((_span.length == length) &&
            ((length == 0) || (memcmp(_span.data,
                                      _literal,
                                      length) == 0)));
}

/*
d_tests_ftp_buffer_is
  Whether a buffer holds exactly the literal, terminator and length agreeing.
*/
static bool
d_tests_ftp_buffer_is(
    const struct d_ftp_buffer* _buffer,
    const char*                _literal
)
{
    return ((strcmp(_buffer->data,
                    _literal) == 0) &&
            (_buffer->length == strlen(_literal)));
}

/*
d_tests_ftp_buffer_reset
  Empties a buffer for the next formatting call.
*/
static void
d_tests_ftp_buffer_reset(
    struct d_ftp_buffer* _buffer
)
{
    _buffer->length  = 0;
    _buffer->data[0] = '\0';

    return;
}

static char                g_out[512];
static struct d_ftp_buffer B;

static enum d_ftp_status
feed(
    struct d_ftp_reply_parser* _p,
    const char*                _s,
    size_t                     _n,
    size_t*                    _used,
    struct d_ftp_reply*        _r
)
{
    return d_ftp_reply_parser_feed(_p,
                                   _s,
                                   _n,
                                   _used,
                                   _r);
}

/*
t_replies
  Checks d_ftp_reply_parser_feed, d_ftp_reply_parser_init,
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_replies(
    void
)
{
    char                      st[256];  // filled before it is read
    struct d_ftp_reply_parser p = { 0 };
    struct d_ftp_reply        r = { 0 };
    size_t                    used = 0;
    d_ftp_reply_parser_init(&p,
                            st,
                            sizeof st);
    const char* s1 = "220 Service ready\r\n";
    D_TESTS_FTP_CHECK(feed(&p,
                           s1,
                           strlen(s1),
                           &used,
                           &r) ==
                      D_FTP_STATUS_COMPLETE);
    D_TESTS_FTP_CHECK( (used == strlen(s1))                   &&
                       (r.code == 220)                        &&
                       (r.line_count == 1)                    &&
                       (d_tests_ftp_span_is(r.text,
                                            "Service ready")) &&
                       (!r.truncated) );
    const char* m = "211-Features:\r\n EPSV\r\n MDTM\r\n211 End\r\n";
    D_TESTS_FTP_CHECK(feed(&p,
                           m,
                           strlen(m),
                           &used,
                           &r) ==
                      D_FTP_STATUS_COMPLETE);
    D_TESTS_FTP_CHECK( (r.code == 211)     &&
                       (r.line_count == 4) &&
                       (d_tests_ftp_span_is(r.text,
                                            "Features:\n EPSV\n MDTM\nEnd")) );
    enum d_ftp_status s     = D_FTP_STATUS_PENDING;
    size_t            total = 0;
    for (size_t i = 0; i < strlen(m); i++)
    {
        s = feed(&p,
                 m + i,
                 1,
                 &used,
                 &r);
        total += used;
        if (s != D_FTP_STATUS_PENDING)
        {
            break;
        }
    }

    D_TESTS_FTP_CHECK( (s == D_FTP_STATUS_COMPLETE) &&
                       (total == strlen(m))         &&
                       (d_tests_ftp_span_is(r.text,
                                            "Features:\n EPSV\n MDTM\nEnd")) );
    const char* two = "150 Opening\r\n226 Done\r\n";
    D_TESTS_FTP_CHECK( (feed(&p,
                             two,
                             strlen(two),
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (r.code == 150)                     &&
                       (used == 13) );
    size_t off = used;
    D_TESTS_FTP_CHECK( (feed(&p,
                             two + off,
                             strlen(two) - off,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (r.code == 226)                     &&
                       (d_tests_ftp_span_is(r.text,
                                            "Done")) );
    const char tel[] = "\xff\xfb\x01"
                       "220 Hi\r\n";
    D_TESTS_FTP_CHECK( (feed(&p,
                             tel,
                             sizeof tel - 1,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (d_tests_ftp_span_is(r.text,
                                            "Hi")) );
    const char esc[] = "220 a\xff\xff"
                       "b\r\n";
    D_TESTS_FTP_CHECK( (feed(&p,
                             esc,
                             sizeof esc - 1,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (d_tests_ftp_span_is(r.text,
                                            "a\xff" "b")) );
    const char* h = "250-a\r\n250-b\r\n250 c\n";
    D_TESTS_FTP_CHECK( (feed(&p,
                             h,
                             strlen(h),
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (d_tests_ftp_span_is(r.text,
                                            "a\nb\nc"))    &&
                       (r.line_count == 3) );
    D_TESTS_FTP_CHECK( (feed(&p,
                             "226\r\n",
                             5,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (r.code == 226)                     &&
                       (r.text.length == 0) );
    const char* o = "211-x\r\n123 y\r\n211 z\r\n";
    D_TESTS_FTP_CHECK( (feed(&p,
                             o,
                             strlen(o),
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (d_tests_ftp_span_is(r.text,
                                            "x\n123 y\nz")) );
    D_TESTS_FTP_CHECK( (feed(&p,
                             "230 partial",
                             11,
                             &used,
                             &r) == D_FTP_STATUS_PENDING) &&
                       (used == 11) );
    D_TESTS_FTP_CHECK( (feed(&p,
                             "\r\n",
                             2,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (d_tests_ftp_span_is(r.text,
                                            "partial")) );
    char                      sm[8];  // filled before it is read
    struct d_ftp_reply_parser q = { 0 };
    d_ftp_reply_parser_init(&q,
                            sm,
                            sizeof sm);
    D_TESTS_FTP_CHECK( (feed(&q,
                             "220 0123456789\r\n",
                             16,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (r.truncated)                       &&
                       (d_tests_ftp_span_is(r.text,
                                            "0123456"))    &&
                       (sm[7] == '\0') );
    d_ftp_reply_parser_reset(&p);
    D_TESTS_FTP_CHECK(feed(&p,
                           "hello\r\n",
                           7,
                           &used,
                           &r) ==
                      D_FTP_STATUS_FAILED);
    D_TESTS_FTP_CHECK(feed(&p,
                           "220 ok\r\n",
                           8,
                           &used,
                           &r) ==
                      D_FTP_STATUS_FAILED);
    d_ftp_reply_parser_reset(&p);
    D_TESTS_FTP_CHECK(feed(&p,
                           "220 ok\r\n",
                           8,
                           &used,
                           &r) ==
                      D_FTP_STATUS_COMPLETE);
    D_TESTS_FTP_CHECK(feed(&p,
                           "720 no\r\n",
                           8,
                           &used,
                           &r) ==
                      D_FTP_STATUS_FAILED);
    struct d_ftp_reply_parser c = { 0 };
    d_ftp_reply_parser_init(&c,
                            NULL,
                            0);
    D_TESTS_FTP_CHECK( (feed(&c,
                             "200 x\r\n",
                             7,
                             &used,
                             &r) == D_FTP_STATUS_COMPLETE) &&
                       (r.code == 200)                     &&
                       (r.text.length == 0) );
    D_TESTS_FTP_CHECK(d_ftp_reply_parser_feed(NULL,
                                              "x",
                                              1,
                                              &used,
                                              &r) ==
                      D_FTP_STATUS_FAILED);

    return;
}

/*
t_replies_2
  t_replies, continued: d_ftp_buffer_init, d_ftp_error_from_reply,
d_ftp_reply_category_of, d_ftp_reply_class_of, d_ftp_reply_format
*/
static void
t_replies_2(
    void
)
{
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_reply_format(220,
                                           "Ready",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "220 Ready\r\n")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK(d_ftp_reply_format(211,
                                         "Features:\n EPSV\n2 digits\nEnd",
                                         &B) == D_FTP_OK);
    D_TESTS_FTP_CHECK(d_tests_ftp_buffer_is(
        &B,
        "211-Features:\r\n EPSV\r\n 2 digits\r\n211 End\r\n"));
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_reply_format(550,
                                           "no such file\r\n230 fake",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "550-no such file\r\n550 230 fake\r\n")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_reply_format(421,
                                           NULL,
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "421 Service not available, closing control connection.\r\n")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK(d_ftp_reply_format(299,
                                         NULL,
                                         &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);
    D_TESTS_FTP_CHECK(d_ftp_reply_format(99,
                                         "x",
                                         &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);
    char                t8[8];  // filled before it is read
    struct d_ftp_buffer tb = { 0 };
    d_ftp_buffer_init(&tb,
                      t8,
                      sizeof t8);
    D_TESTS_FTP_CHECK( (d_ftp_reply_format(220,
                                           "Ready now",
                                           &tb) ==
                        D_FTP_ERROR_BUFFER_TOO_SMALL) &&
                       (tb.length == 0)               &&
                       (t8[0] == '\0') );
    D_TESTS_FTP_CHECK( (d_ftp_error_from_reply(530) ==
                        D_FTP_ERROR_LOGIN_DENIED) &&
                       (d_ftp_error_from_reply(226) == D_FTP_OK) );
    D_TESTS_FTP_CHECK( (d_ftp_error_from_reply(550) ==
                        D_FTP_ERROR_FILE_UNAVAILABLE) &&
                       (d_ftp_error_from_reply(599) == D_FTP_ERROR_REJECTED) );
    D_TESTS_FTP_CHECK( (d_ftp_error_from_reply(99) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_error_from_reply(632) ==
                        D_FTP_ERROR_UNSUPPORTED) );
    D_TESTS_FTP_CHECK( (d_ftp_error_from_reply(452) ==
                        D_FTP_ERROR_INSUFFICIENT_STORAGE) &&
                       (d_ftp_error_from_reply(535) == D_FTP_ERROR_SECURITY) );
    D_TESTS_FTP_CHECK( (d_ftp_reply_class_of(150) ==
                        D_FTP_REPLY_CLASS_PRELIMINARY) &&
                       (d_ftp_reply_category_of(530) ==
                        D_FTP_REPLY_CATEGORY_AUTHENTICATION) );

    return;
}

/*
t_replies_3
  t_replies, continued: d_ftp_error_string, d_ftp_reply_is_valid,
d_ftp_reply_text
*/
static void
t_replies_3(
    void
)
{
    D_TESTS_FTP_CHECK( (d_ftp_reply_is_valid(553))  &&
                       (!d_ftp_reply_is_valid(599)) &&
                       (!d_ftp_reply_is_valid(700)) &&
                       (!d_ftp_reply_is_valid(99)) );
    D_TESTS_FTP_CHECK( (d_ftp_reply_text(227) != NULL) &&
                       (d_ftp_reply_text(299) == NULL) );
    for (int e = 0; e <= (int)D_FTP_ERROR_UNKNOWN; e++)
    {
        D_TESTS_FTP_CHECK(d_ftp_error_string((enum d_ftp_error)e)[0] != '\0');
    }

    return;
}

/*
t_commands
  Checks d_ftp_command_format, d_ftp_command_format_raw.
*/
static void
t_commands(
    void
)
{
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_command_format(D_FTP_COMMAND_RETR,
                                             "file.txt",
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "RETR file.txt\r\n")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_command_format(D_FTP_COMMAND_PASV,
                                             NULL,
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "PASV\r\n")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_command_format(D_FTP_COMMAND_PASS,
                                             "",
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "PASS \r\n")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_command_format(D_FTP_COMMAND_RETR,
                                             "a\r\nDELE b",
                                             &B) ==
                        D_FTP_ERROR_INVALID_ARGUMENT) &&
                       (B.length == 0) );
    D_TESTS_FTP_CHECK(d_ftp_command_format(D_FTP_COMMAND_UNKNOWN,
                                           NULL,
                                           &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_command_format_raw("XSHA256",
                                                 "f",
                                                 &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "XSHA256 f\r\n")) );
    D_TESTS_FTP_CHECK(d_ftp_command_format_raw("1BAD",
                                               NULL,
                                               &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);

    return;
}

/*
t_commands_2
  t_commands, continued: d_ftp_command_lookup, d_ftp_command_name,
d_ftp_command_parse, d_ftp_command_policy, d_ftp_command_uses_data,
d_ftp_telnet_strip
*/
static void
t_commands_2(
    void
)
{
    struct d_ftp_command_line cl = { 0 };
    const char*               l1 = "retr file name.txt\r\n";

    D_TESTS_FTP_CHECK( (d_ftp_command_parse(l1,
                                            strlen(l1),
                                            &cl) == D_FTP_OK) &&
                       (cl.command == D_FTP_COMMAND_RETR)     &&
                       (cl.has_argument)                      &&
                       (d_tests_ftp_span_is(cl.argument,
                                            "file name.txt")) &&
                       (d_tests_ftp_span_is(cl.verb,
                                            "retr")) );
    D_TESTS_FTP_CHECK( (d_ftp_command_parse("NOOP",
                                            4,
                                            &cl) == D_FTP_OK) &&
                       (cl.command == D_FTP_COMMAND_NOOP)     &&
                       (!cl.has_argument) );
    D_TESTS_FTP_CHECK( (d_ftp_command_parse("FOO bar",
                                            7,
                                            &cl) == D_FTP_OK) &&
                       (cl.command == D_FTP_COMMAND_UNKNOWN)  &&
                       (d_tests_ftp_span_is(cl.verb,
                                            "FOO")) );
    D_TESTS_FTP_CHECK( (d_ftp_command_parse("",
                                            0,
                                            &cl) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_command_parse("RE\tTR x",
                                            7,
                                            &cl) == D_FTP_ERROR_MALFORMED) );
    char ln[] = "\xff\xf4\xff\xf2"
                "ABOR\r\n";
    D_TESTS_FTP_CHECK( (d_ftp_telnet_strip(ln,
                                           strlen(ln)) == 6) &&
                       (memcmp(ln,
                               "ABOR\r\n",
                               6) == 0) );
    D_TESTS_FTP_CHECK( (d_ftp_command_lookup("mlsd",
                                             4) == D_FTP_COMMAND_MLSD) &&
                       (d_ftp_command_policy(D_FTP_COMMAND_CDUP) ==
                        D_FTP_ARGUMENT_NONE) );
    D_TESTS_FTP_CHECK( (d_ftp_command_uses_data(D_FTP_COMMAND_LIST)) &&
                       (!d_ftp_command_uses_data(D_FTP_COMMAND_MLST)) );
    D_TESTS_FTP_CHECK( (strcmp(d_ftp_command_name(D_FTP_COMMAND_RETR),
                               "RETR") == 0) &&
                       (d_ftp_command_name((enum d_ftp_command)999) == NULL) );
    for (int i = 1; i < (int)D_FTP_COMMAND_COUNT; i++)
    {
        D_TESTS_FTP_CHECK(d_ftp_command_name((enum d_ftp_command)i) != NULL);
    }

    d_tests_ftp_buffer_reset(&B);

    return;
}

/*
t_commands_3
  t_commands, continued: d_ftp_mode_from_code, d_ftp_protection_from_code,
d_ftp_structure_from_code, d_ftp_type_format, d_ftp_type_parse
*/
static void
t_commands_3(
    void
)
{
    struct d_ftp_type t = { D_FTP_TYPE_ASCII, D_FTP_FORMAT_NONE, 8 };

    D_TESTS_FTP_CHECK( (d_ftp_type_format(&t,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "A")) );
    t.format = D_FTP_FORMAT_NON_PRINT;
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_type_format(&t,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "A N")) );
    t.data_type = D_FTP_TYPE_LOCAL;
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_type_format(&t,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "L 8")) );
    t.byte_size = 0;
    D_TESTS_FTP_CHECK(d_ftp_type_format(&t,
                                        &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);
    D_TESTS_FTP_CHECK( (d_ftp_type_parse("a n",
                                         3,
                                         &t) == D_FTP_OK) &&
                       (t.data_type == D_FTP_TYPE_ASCII)  &&
                       (t.format == D_FTP_FORMAT_NON_PRINT) );
    D_TESTS_FTP_CHECK( (d_ftp_type_parse("I",
                                         1,
                                         &t) == D_FTP_OK) &&
                       (t.data_type == D_FTP_TYPE_IMAGE) );
    D_TESTS_FTP_CHECK( (d_ftp_type_parse("L 36",
                                         4,
                                         &t) == D_FTP_OK) &&
                       (t.data_type == D_FTP_TYPE_LOCAL)  &&
                       (t.byte_size == 36) );
    D_TESTS_FTP_CHECK( (d_ftp_type_parse("X",
                                         1,
                                         &t) == D_FTP_ERROR_UNSUPPORTED) &&
                       (d_ftp_type_parse("A Q",
                                         3,
                                         &t) == D_FTP_ERROR_MALFORMED) );
    enum d_ftp_structure     so = D_FTP_STRUCTURE_FILE;
    enum d_ftp_transfer_mode mo = D_FTP_MODE_STREAM;
    enum d_ftp_protection    pr = D_FTP_PROTECTION_CLEAR;
    D_TESTS_FTP_CHECK( (d_ftp_structure_from_code('r',
                                                  &so)) &&
                       (so == D_FTP_STRUCTURE_RECORD) );
    D_TESTS_FTP_CHECK( (d_ftp_mode_from_code('z',
                                             &mo)) &&
                       (mo == D_FTP_MODE_DEFLATE) );
    D_TESTS_FTP_CHECK( (d_ftp_protection_from_code('p',
                                                   &pr)) &&
                       (pr == D_FTP_PROTECTION_PRIVATE)  &&
                       (!d_ftp_protection_from_code('x',
                                                    &pr)) );

    return;
}

/*
t_endpoints
  Checks d_ftp_parse_eprt, d_ftp_parse_epsv, d_ftp_parse_pasv,
d_ftp_parse_port.
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_endpoints(
    void
)
{
    struct d_ftp_endpoint ep = { 0 };
    const char*           p1 = "Entering Passive Mode (192,168,1,2,19,137).";
    D_TESTS_FTP_CHECK( (d_ftp_parse_pasv(p1,
                                         strlen(p1),
                                         &ep) == D_FTP_OK) &&
                       (ep.family == D_FTP_FAMILY_IPV4)    &&
                       (strcmp(ep.address,
                               "192.168.1.2") == 0)        &&
                       (ep.port == 5001) );
    const char* p2 = "Entering Passive Mode 10,0,0,1,4,1";
    D_TESTS_FTP_CHECK( (d_ftp_parse_pasv(p2,
                                         strlen(p2),
                                         &ep) == D_FTP_OK) &&
                       (strcmp(ep.address,
                               "10.0.0.1") == 0)           &&
                       (ep.port == 1025) );
    const char* p3 = "v2 mode =127,0,0,1,0,21";
    D_TESTS_FTP_CHECK( (d_ftp_parse_pasv(p3,
                                         strlen(p3),
                                         &ep) == D_FTP_OK) &&
                       (strcmp(ep.address,
                               "127.0.0.1") == 0)          &&
                       (ep.port == 21) );
    D_TESTS_FTP_CHECK(d_ftp_parse_pasv("(300,1,1,1,1,1)",
                                       15,
                                       &ep) ==
                      D_FTP_ERROR_MALFORMED);
    const char* e1 = "Entering Extended Passive Mode (|||6446|)";
    D_TESTS_FTP_CHECK( (d_ftp_parse_epsv(e1,
                                         strlen(e1),
                                         &ep) == D_FTP_OK) &&
                       (ep.port == 6446)                   &&
                       (ep.family == D_FTP_FAMILY_NONE) );
    D_TESTS_FTP_CHECK( (d_ftp_parse_epsv("(!!!80!)",
                                         8,
                                         &ep) == D_FTP_OK) &&
                       (ep.port == 80) );
    D_TESTS_FTP_CHECK( (d_ftp_parse_epsv("(|||0|)",
                                         7,
                                         &ep) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_parse_epsv("(|||80",
                                         6,
                                         &ep) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_parse_port("132,235,1,2,24,131",
                                         18,
                                         &ep) == D_FTP_OK) &&
                       (strcmp(ep.address,
                               "132.235.1.2") == 0)        &&
                       (ep.port == 6275) );
    D_TESTS_FTP_CHECK(d_ftp_parse_port("1,2,3,4,5,6,7",
                                       13,
                                       &ep) ==
                      D_FTP_ERROR_MALFORMED);
    const char* x1 = "|1|132.235.1.2|6275|";
    const char* x2 = "|2|1080::8:800:200C:417A|5282|";
    D_TESTS_FTP_CHECK( (d_ftp_parse_eprt(x1,
                                         strlen(x1),
                                         &ep) == D_FTP_OK) &&
                       (ep.family == D_FTP_FAMILY_IPV4)    &&
                       (ep.port == 6275) );
    D_TESTS_FTP_CHECK( (d_ftp_parse_eprt(x2,
                                         strlen(x2),
                                         &ep) == D_FTP_OK) &&
                       (ep.family == D_FTP_FAMILY_IPV6)    &&
                       (strcmp(ep.address,
                               "1080::8:800:200C:417A") == 0) );
    D_TESTS_FTP_CHECK(d_ftp_parse_eprt("|3|x|1|",
                                       7,
                                       &ep) ==
                      D_FTP_ERROR_PROTOCOL_UNSUPPORTED);
    D_TESTS_FTP_CHECK( (d_ftp_parse_eprt("|1|1.2.3|5|",
                                         11,
                                         &ep) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_parse_eprt("|1|1.2.3.4|5|6|",
                                         15,
                                         &ep) == D_FTP_ERROR_MALFORMED) );

    return;
}

/*
t_endpoints_2
  t_endpoints, continued: d_ftp_format_eprt, d_ftp_format_epsv,
d_ftp_format_pasv, d_ftp_format_port
*/
static void
t_endpoints_2(
    void
)
{
    struct d_ftp_endpoint ep = { 0 };

    memset(&ep,
           0,
           sizeof ep);
    ep.family = D_FTP_FAMILY_IPV4;
    strcpy(ep.address,
           "192.168.1.2");
    ep.port = 5001;
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_format_port(&ep,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "192,168,1,2,19,137")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_format_pasv(&ep,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "Entering Passive Mode (192,168,1,2,19,137).")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_format_eprt(&ep,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "|1|192.168.1.2|5001|")) );
    struct d_ftp_endpoint e6 = { 0 };
    memset(&e6,
           0,
           sizeof e6);
    e6.family = D_FTP_FAMILY_IPV6;
    strcpy(e6.address,
           "::1");
    e6.port = 21;
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_format_eprt(&e6,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "|2|::1|21|")) );
    D_TESTS_FTP_CHECK(d_ftp_format_port(&e6,
                                        &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);
    ep.port = 6446;
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_format_epsv(&ep,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "Entering Extended Passive Mode (|||6446|)")) );
    struct d_ftp_endpoint bad = { 0 };
    memset(&bad,
           'x',
           sizeof bad);
    bad.family = D_FTP_FAMILY_IPV4;
    bad.port   = 1;
    D_TESTS_FTP_CHECK(d_ftp_format_port(&bad,
                                        &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);

    return;
}

/*
t_paths_time
  Checks d_ftp_path_resolve, d_ftp_pathname_quote, d_ftp_pathname_unquote.
*/
static void
t_paths_time(
    void
)
{
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_pathname_quote("/a\"b",
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "\"/a\"\"b\"")) );
    const char* q1 = "\"/usr/dm\" created";
    const char* q2 = "\"/a\"\"b\" is current";
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_pathname_unquote(q1,
                                               strlen(q1),
                                               &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/usr/dm")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_pathname_unquote(q2,
                                               strlen(q2),
                                               &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/a\"b")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_pathname_unquote("no quotes",
                                               9,
                                               &B) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_pathname_unquote("\"open",
                                               5,
                                               &B) == D_FTP_ERROR_MALFORMED) &&
                       (B.length == 0) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_path_resolve("/home/user",
                                           "../etc/./passwd",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/home/etc/passwd")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_path_resolve("/a",
                                           "/b//c/",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/b/c")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_path_resolve("/",
                                           "../../..",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_path_resolve(NULL,
                                           "x",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/x")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_path_resolve("/a/b",
                                           "..",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/a")) );
    d_tests_ftp_buffer_reset(&B);

    return;
}

/*
t_paths_time_2
  t_paths_time, continued: d_ftp_path_resolve
*/
static void
t_paths_time_2(
    void
)
{
    D_TESTS_FTP_CHECK( (d_ftp_path_resolve("/a",
                                           "../../../etc/passwd",
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/etc/passwd")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK(d_ftp_path_resolve("/a",
                                         "x\ny",
                                         &B) ==
                      D_FTP_ERROR_INVALID_ARGUMENT);

    return;
}

/*
t_paths_time_3
  t_paths_time, continued: d_ftp_time_format, d_ftp_time_from_unix,
d_ftp_time_parse, d_ftp_time_to_unix
*/
static void
t_paths_time_3(
    void
)
{
    struct d_ftp_time tm = { 0 };

    D_TESTS_FTP_CHECK( (d_ftp_time_parse("20260925123456",
                                         14,
                                         &tm) == D_FTP_OK) &&
                       (tm.year == 2026)                   &&
                       (tm.month == 9)                     &&
                       (tm.day == 25)                      &&
                       (tm.hour == 12)                     &&
                       (tm.minute == 34)                   &&
                       (tm.second == 56) );
    D_TESTS_FTP_CHECK( (d_ftp_time_parse(" 20260925123456.5 ",
                                         18,
                                         &tm) == D_FTP_OK) &&
                       (tm.millisecond == 500) );
    D_TESTS_FTP_CHECK( (d_ftp_time_parse("20260230000000",
                                         14,
                                         &tm) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_time_parse("2026092512345",
                                         13,
                                         &tm) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK(d_ftp_time_parse("20260925123456.",
                                       15,
                                       &tm) ==
                      D_FTP_ERROR_MALFORMED);
    d_ftp_time_parse("20260925123456.5",
                     16,
                     &tm);
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_time_format(&tm,
                                          true,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "20260925123456.500")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_time_format(&tm,
                                          false,
                                          &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "20260925123456")) );
    struct d_ftp_time ep0 = { 1970, 1, 1, 0, 0, 0, 0 },
                      y2k = { 2000, 3, 1, 0, 0, 0, 0 };
    int64_t sec           = 7;
    D_TESTS_FTP_CHECK( (d_ftp_time_to_unix(&ep0,
                                           &sec) == D_FTP_OK) &&
                       (sec == 0) );
    D_TESTS_FTP_CHECK( (d_ftp_time_to_unix(&y2k,
                                           &sec) == D_FTP_OK) &&
                       (sec == 951868800) );
    D_TESTS_FTP_CHECK( (d_ftp_time_from_unix(951868800,
                                             &tm) == D_FTP_OK) &&
                       (tm.year == 2000)                       &&
                       (tm.month == 3)                         &&
                       (tm.day == 1) );
    D_TESTS_FTP_CHECK( (d_ftp_time_from_unix(-1,
                                             &tm) == D_FTP_OK) &&
                       (tm.year == 1969)                       &&
                       (tm.month == 12)                        &&
                       (tm.day == 31)                          &&
                       (tm.hour == 23)                         &&
                       (tm.minute == 59)                       &&
                       (tm.second == 59) );
    D_TESTS_FTP_CHECK( (d_ftp_time_from_unix(1000000000,
                                             &tm) == D_FTP_OK) &&
                       (tm.year == 2001)                       &&
                       (tm.month == 9)                         &&
                       (tm.day == 9)                           &&
                       (tm.hour == 1)                          &&
                       (tm.minute == 46)                       &&
                       (tm.second == 40) );

    return;
}

/*
t_paths_time_4
  t_paths_time, continued: d_ftp_features_parse, d_ftp_parse_size,
d_ftp_time_from_unix, d_ftp_time_to_unix
*/
static void
t_paths_time_4(
    void
)
{
    for (int64_t s = -5000000000LL; s < 5000000000LL; s += 7777777)
    {
        struct d_ftp_time a = { 0 };
        int64_t           b2 = 0;
        d_ftp_time_from_unix(s,
                             &a);
        d_ftp_time_to_unix(&a,
                           &b2);
        if (b2 != s)
        {
            D_TESTS_FTP_CHECK(b2 == s);
            break;
        }
    }

    uint64_t sz = 0;
    D_TESTS_FTP_CHECK( (d_ftp_parse_size("  12345 ",
                                         8,
                                         &sz) == D_FTP_OK) &&
                       (sz == 12345) );
    D_TESTS_FTP_CHECK( (d_ftp_parse_size("18446744073709551615",
                                         20,
                                         &sz) == D_FTP_OK) &&
                       (sz == UINT64_MAX) );
    D_TESTS_FTP_CHECK( (d_ftp_parse_size("18446744073709551616",
                                         20,
                                         &sz) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_parse_size("12a",
                                         3,
                                         &sz) == D_FTP_ERROR_MALFORMED) );
    const char* feat =
        "Features:\n EPSV\n MDTM\n MLST type*;size*;modify*;\n REST STREAM\n "
        "AUTH TLS;SSL\n UTF8\n LANG EN*\n MODE Z\nEnd";
    struct d_ftp_features f = { 0 };
    D_TESTS_FTP_CHECK(d_ftp_features_parse(feat,
                                           strlen(feat),
                                           &f) == D_FTP_OK);
    uint32_t want = D_FTP_FEATURE_EPSV        |
                    D_FTP_FEATURE_MDTM        |
                    D_FTP_FEATURE_MLST        |
                    D_FTP_FEATURE_REST_STREAM |
                    D_FTP_FEATURE_AUTH_TLS    |
                    D_FTP_FEATURE_AUTH_SSL    |
                    D_FTP_FEATURE_UTF8        |
                    D_FTP_FEATURE_LANG        |
                    D_FTP_FEATURE_MODE_Z;
    D_TESTS_FTP_CHECK( (f.flags == want)                             &&
                       (d_tests_ftp_span_is(f.mlst_facts,
                                            "type*;size*;modify*;")) &&
                       (d_tests_ftp_span_is(f.languages,
                                            "EN*")) );

    return;
}

static enum d_ftp_line_result
P(
    const char*               _l,
    enum d_ftp_listing_format _fm,
    const struct d_ftp_time*  _now,
    struct d_ftp_entry*       _e
)
{
    return d_ftp_listing_parse(_l,
                               strlen(_l),
                               _fm,
                               _now,
                               _e);
}

/*
t_listings
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_listings(
    void
)
{
    struct d_ftp_time  now = { 2026, 9, 26, 12, 0, 0, 0 };
    struct d_ftp_entry e = { 0 };
    const char*        L1 =
        "-rw-r--r--    1 owner    group        1234 Sep 25 12:34 my file.txt";
    D_TESTS_FTP_CHECK( (P(L1,
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)  &&
                       (e.type == D_FTP_ENTRY_FILE) &&
                       (e.size == 1234)             &&
                       (e.mode == 0644) );
    D_TESTS_FTP_CHECK( (d_tests_ftp_span_is(e.owner,
                                            "owner")) &&
                       (d_tests_ftp_span_is(e.group,
                                            "group")) &&
                       (d_tests_ftp_span_is(e.name,
                                            "my file.txt")) );
    D_TESTS_FTP_CHECK( (e.modified.year == 2026) &&
                       (e.modified.month == 9)   &&
                       (e.modified.day == 25)    &&
                       (e.modified.hour == 12)   &&
                       (e.modified.minute == 34) );
    D_TESTS_FTP_CHECK( ((e.known & (D_FTP_ENTRY_KNOWN_MODIFIED |
                                    D_FTP_ENTRY_KNOWN_TIME     |
                                    D_FTP_ENTRY_KNOWN_SIZE)) ==
                        (D_FTP_ENTRY_KNOWN_MODIFIED |
                         D_FTP_ENTRY_KNOWN_TIME     |
                         D_FTP_ENTRY_KNOWN_SIZE)) &&
                       (!(e.known & D_FTP_ENTRY_KNOWN_UTC)) );
    D_TESTS_FTP_CHECK( (P(L1,
                          D_FTP_LISTING_UNIX,
                          NULL,
                          &e) == D_FTP_LINE_ENTRY)               &&
                       (!(e.known & D_FTP_ENTRY_KNOWN_MODIFIED)) &&
                       (d_tests_ftp_span_is(e.name,
                                            "my file.txt")) );
    D_TESTS_FTP_CHECK( (P("drwxr-xr-x 2 root root 4096 Jan  5  2024 dir",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)       &&
                       (e.type == D_FTP_ENTRY_DIRECTORY) &&
                       (e.modified.year == 2024)         &&
                       (e.modified.day == 5)             &&
                       (e.mode == 0755)                  &&
                       (!(e.known & D_FTP_ENTRY_KNOWN_TIME)) );
    D_TESTS_FTP_CHECK( (P("-rw-r--r-- 1 a b 1 Dec 31 23:59 f",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (e.modified.year == 2025) );
    D_TESTS_FTP_CHECK( (P("-rw-r--r-- 1 a b 1 Sep 27 23:59 f",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (e.modified.year == 2026) );
    D_TESTS_FTP_CHECK( (P("lrwxrwxrwx 1 root root 7 Jan 1 2020 bin -> usr/bin",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)     &&
                       (e.type == D_FTP_ENTRY_SYMLINK) &&
                       (d_tests_ftp_span_is(e.name,
                                            "bin"))    &&
                       (d_tests_ftp_span_is(e.link_target,
                                            "usr/bin")) );
    D_TESTS_FTP_CHECK( (P("crw-rw-rw- 1 root root 1, 3 Jan 1 2020 null",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)           &&
                       (e.type == D_FTP_ENTRY_OTHER)         &&
                       (!(e.known & D_FTP_ENTRY_KNOWN_SIZE)) &&
                       (d_tests_ftp_span_is(e.owner,
                                            "root"))         &&
                       (d_tests_ftp_span_is(e.group,
                                            "root")) );
    D_TESTS_FTP_CHECK( (P("-rw-r--r-- 1 ftp 42 Mar 3 2021 x",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)            &&
                       (d_tests_ftp_span_is(e.owner,
                                            "ftp"))           &&
                       (!(e.known & D_FTP_ENTRY_KNOWN_GROUP)) &&
                       (e.size == 42) );
    D_TESTS_FTP_CHECK( (P("-rw-r--r-- 1 u g 10 2026-09-25 12:34 iso.txt",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (e.modified.year == 2026)   &&
                       (e.modified.minute == 34)   &&
                       (d_tests_ftp_span_is(e.name,
                                            "iso.txt")) );
    D_TESTS_FTP_CHECK( (P("-rwsr-xr-x 1 root root 10 Jan 1 2020 su",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (e.mode == 04755) );
    D_TESTS_FTP_CHECK( (P("drwxrwxrwt+ 1 root root 10 Jan 1 2020 tmp",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (e.mode == 01777) );
    D_TESTS_FTP_CHECK( (P("-rw-r--r-- 1 u g 1 Jan 1 2020 sp \r\n",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (d_tests_ftp_span_is(e.name,
                                            "sp ")) );
    D_TESTS_FTP_CHECK( (P("total 48",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_SKIP) &&
                       (P("\r\n",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_SKIP) );
    D_TESTS_FTP_CHECK(
        P("-rw-r--r-- 1 u g 1 Feb 30 2020 bad",
          D_FTP_LISTING_AUTO,
          &now,
          &e) ==
        D_FTP_LINE_MALFORMED);
    D_TESTS_FTP_CHECK( (P("09-25-26  12:34PM       <DIR>          My Dir",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)       &&
                       (e.type == D_FTP_ENTRY_DIRECTORY) &&
                       (e.modified.year == 2026)         &&
                       (e.modified.hour == 12)           &&
                       (d_tests_ftp_span_is(e.name,
                                            "My Dir")) );
    D_TESTS_FTP_CHECK( (P("09-25-2026  01:05AM              1,234 file.txt",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY)  &&
                       (e.type == D_FTP_ENTRY_FILE) &&
                       (e.size == 1234)             &&
                       (e.modified.hour == 1) );
    D_TESTS_FTP_CHECK( (P("12-01-99  12:00AM   5 old.txt",
                          D_FTP_LISTING_AUTO,
                          &now,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (e.modified.year == 1999)   &&
                       (e.modified.hour == 0) );
    D_TESTS_FTP_CHECK(
        P("09-25-26  13:34PM   5 x",
          D_FTP_LISTING_DOS,
          &now,
          &e) ==
        D_FTP_LINE_MALFORMED);
    D_TESTS_FTP_CHECK( (P("type=file;size=1024;modify=20260925123456;perm=adfrw;" "unique=U1; report.pdf",
                          D_FTP_LISTING_AUTO,
                          NULL,
                          &e) == D_FTP_LINE_ENTRY)       &&
                       (e.type == D_FTP_ENTRY_FILE)      &&
                       (e.size == 1024)                  &&
                       (e.known & D_FTP_ENTRY_KNOWN_UTC) &&
                       (d_tests_ftp_span_is(e.permissions,
                                            "adfrw"))    &&
                       (d_tests_ftp_span_is(e.unique,
                                            "U1"))       &&
                       (d_tests_ftp_span_is(e.name,
                                            "report.pdf")) );
    D_TESTS_FTP_CHECK( (P(" type=dir;modify=20260101000000; /pub",
                          D_FTP_LISTING_AUTO,
                          NULL,
                          &e) == D_FTP_LINE_ENTRY)       &&
                       (e.type == D_FTP_ENTRY_DIRECTORY) &&
                       (d_tests_ftp_span_is(e.name,
                                            "/pub")) );
    D_TESTS_FTP_CHECK( (P("type=cdir; .",
                          D_FTP_LISTING_MLSX,
                          NULL,
                          &e) == D_FTP_LINE_ENTRY)               &&
                       (e.type == D_FTP_ENTRY_CURRENT_DIRECTORY) &&
                       (d_tests_ftp_span_is(e.name,
                                            ".")) );
    D_TESTS_FTP_CHECK( (P("type=OS.unix=slink:/target;UNIX.mode=0777; link",
                          D_FTP_LISTING_AUTO,
                          NULL,
                          &e) == D_FTP_LINE_ENTRY)      &&
                       (e.type == D_FTP_ENTRY_SYMLINK)  &&
                       (d_tests_ftp_span_is(e.link_target,
                                            "/target")) &&
                       (e.mode == 0777)                 &&
                       (d_tests_ftp_span_is(e.name,
                                            "link")) );
    D_TESTS_FTP_CHECK( (d_ftp_listing_detect("random",
                                             6) == D_FTP_LISTING_AUTO) &&
                       (P("random",
                          D_FTP_LISTING_AUTO,
                          NULL,
                          &e) == D_FTP_LINE_MALFORMED) );
    D_TESTS_FTP_CHECK( (P("random name",
                          D_FTP_LISTING_NAMES,
                          NULL,
                          &e) == D_FTP_LINE_ENTRY) &&
                       (d_tests_ftp_span_is(e.name,
                                            "random name")) );

    return;
}

/*
t_urls_ascii
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_urls_ascii(
    void
)
{
    struct d_ftp_url u = { 0 };
    const char*      u1 =
        "ftp://user:p%40ss@ftp.example.com:2121/pub/file.txt;type=i";
    D_TESTS_FTP_CHECK( (d_ftp_url_parse(u1,
                                        strlen(u1),
                                        &u) == D_FTP_OK) &&
                       (u.scheme == D_FTP_SCHEME_FTP)    &&
                       (d_tests_ftp_span_is(u.user,
                                            "user"))     &&
                       (d_tests_ftp_span_is(u.password,
                                            "p%40ss")) );
    D_TESTS_FTP_CHECK( (d_tests_ftp_span_is(u.host,
                                            "ftp.example.com")) &&
                       (u.has_port)                             &&
                       (u.port == 2121)                         &&
                       (d_tests_ftp_span_is(u.path,
                                            "pub/file.txt"))    &&
                       (u.type_code == 'i')                     &&
                       (d_ftp_url_port(&u) == 2121) );
    const char* u2 = "ftps://[2001:db8::1]/x";
    D_TESTS_FTP_CHECK( (d_ftp_url_parse(u2,
                                        strlen(u2),
                                        &u) == D_FTP_OK)    &&
                       (u.scheme == D_FTP_SCHEME_FTPS)      &&
                       (u.ipv6_literal)                     &&
                       (d_tests_ftp_span_is(u.host,
                                            "2001:db8::1")) &&
                       (d_ftp_url_port(&u) == 990)          &&
                       (d_tests_ftp_span_is(u.path,
                                            "x")) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://host",
                                        10,
                                        &u) == D_FTP_OK) &&
                       (d_tests_ftp_span_is(u.host,
                                            "host"))     &&
                       (u.path.length == 0)              &&
                       (!u.has_user)                     &&
                       (d_ftp_url_port(&u) == 21) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("FTPES://a@b/",
                                        12,
                                        &u) == D_FTP_OK) &&
                       (u.scheme == D_FTP_SCHEME_FTPES)  &&
                       (d_tests_ftp_span_is(u.user,
                                            "a"))        &&
                       (d_tests_ftp_span_is(u.host,
                                            "b"))        &&
                       (!u.has_password) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://u:p@ss@h/",
                                        15,
                                        &u) == D_FTP_OK) &&
                       (d_tests_ftp_span_is(u.password,
                                            "p@ss"))     &&
                       (d_tests_ftp_span_is(u.host,
                                            "h")) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("http://x/",
                                        9,
                                        &u) == D_FTP_ERROR_UNSUPPORTED) &&
                       (d_ftp_url_parse("ftp:/x",
                                        6,
                                        &u) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://",
                                        6,
                                        &u) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_url_parse("ftp://h:0/",
                                        10,
                                        &u) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://h/a b",
                                        11,
                                        &u) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_url_parse("ftp://h/x;type=q",
                                        16,
                                        &u) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://h:/p",
                                        10,
                                        &u) == D_FTP_OK) &&
                       (!u.has_port)                     &&
                       (d_ftp_url_port(&u) == 21) );
    /* hosts and ports are net_url's, as RFC 3986 and RFC 6874 have them */
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://[fe80::1%25eth0]/x",
                                        24,
                                        &u) == D_FTP_OK)       &&
                       (u.ipv6_literal)                        &&
                       (d_tests_ftp_span_is(u.host,
                                            "fe80::1%25eth0")) &&
                       (d_ftp_url_port(&u) == 21) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://[fe80::1%eth0]/",
                                        21,
                                        &u) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_url_parse("ftp://[::G]/",
                                        12,
                                        &u) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://ho^st/",
                                        12,
                                        &u) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_url_parse("ftp://h%zz/",
                                        11,
                                        &u) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://u@10.0.0.1:00021/",
                                        23,
                                        &u) == D_FTP_OK) &&
                       (d_tests_ftp_span_is(u.host,
                                            "10.0.0.1")) &&
                       (u.has_port)                      &&
                       (u.port == 21)                    &&
                       (!u.ipv6_literal) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://h:65536/",
                                        14,
                                        &u) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_url_parse("ftp://h:2x/",
                                        11,
                                        &u) == D_FTP_ERROR_MALFORMED) );
    D_TESTS_FTP_CHECK( (d_ftp_url_parse("ftp://h:7/",
                                        10,
                                        &u) == D_FTP_OK) &&
                       (u.has_port)                      &&
                       (u.port == 7)                     &&
                       (d_ftp_url_port(&u) == 7) );

    return;
}

/*
t_urls_ascii_2
  t_urls_ascii, continued: d_ftp_percent_decode, d_ftp_scheme_security
*/
static void
t_urls_ascii_2(
    void
)
{
    D_TESTS_FTP_CHECK( (d_ftp_scheme_security(D_FTP_SCHEME_FTPS) ==
                        D_FTP_SECURITY_IMPLICIT) &&
                       (d_ftp_scheme_security(D_FTP_SCHEME_FTPES) ==
                        D_FTP_SECURITY_EXPLICIT) &&
                       (d_ftp_scheme_security(D_FTP_SCHEME_FTP) ==
                        D_FTP_SECURITY_NONE) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_percent_decode("p%40ss",
                                             6,
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "p@ss")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_percent_decode("%0d%0a",
                                             6,
                                             &B) == D_FTP_ERROR_MALFORMED) &&
                       (B.length == 0) );
    D_TESTS_FTP_CHECK( (d_ftp_percent_decode("%zz",
                                             3,
                                             &B) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_percent_decode("%4",
                                             2,
                                             &B) == D_FTP_ERROR_MALFORMED) &&
                       (d_ftp_percent_decode("a%00",
                                             4,
                                             &B) == D_FTP_ERROR_MALFORMED) );
    // a NUL, CR or LF is refused however it arrives; a bad escape beats a full
    // buffer
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_percent_decode("a\rb",
                                             3,
                                             &B) == D_FTP_ERROR_MALFORMED) &&
                       (B.length == 0)                                     &&
                       (d_ftp_percent_decode("a\0b",
                                             3,
                                             &B) == D_FTP_ERROR_MALFORMED) );

    return;
}

/*
t_urls_ascii_3
  t_urls_ascii, continued: d_ftp_buffer_init, d_ftp_percent_decode,
d_ftp_percent_encode
*/
static void
t_urls_ascii_3(
    void
)
{
    {
        char                small[4] = { 0 };
        struct d_ftp_buffer sb       = { 0 };
        d_ftp_buffer_init(&sb,
                          small,
                          sizeof(small));
        D_TESTS_FTP_CHECK( (d_ftp_percent_decode("abcdef%zz",
                                                 9,
                                                 &sb) ==
                            D_FTP_ERROR_MALFORMED) &&
                           (sb.length == 0)        &&
                           (small[0] == '\0') );
        D_TESTS_FTP_CHECK( (d_ftp_percent_decode("abcdef",
                                                 6,
                                                 &sb) ==
                            D_FTP_ERROR_BUFFER_TOO_SMALL) &&
                           (sb.length == 0)               &&
                           (small[0] == '\0') );
        D_TESTS_FTP_CHECK( (d_ftp_percent_encode("a b",
                                                 3,
                                                 false,
                                                 &sb) ==
                            D_FTP_ERROR_BUFFER_TOO_SMALL) &&
                           (sb.length == 0)               &&
                           (small[0] == '\0') );
        D_TESTS_FTP_CHECK( (d_ftp_percent_encode("a/b",
                                                 3,
                                                 true,
                                                 &sb) == D_FTP_OK) &&
                           (d_tests_ftp_buffer_is(&sb,
                                                  "a/b")) );
    }
    /* appending: what the buffer holds stays, and only the room left counts */

    return;
}

/*
t_urls_ascii_4
  t_urls_ascii, continued: d_ftp_buffer_init, d_ftp_percent_decode,
d_ftp_percent_encode
*/
static void
t_urls_ascii_4(
    void
)
{
    {
        char                room[8] = { 0 };
        struct d_ftp_buffer ab      = { 0 };
        d_ftp_buffer_init(&ab,
                          room,
                          sizeof(room));
        D_TESTS_FTP_CHECK( (d_ftp_percent_decode("ab",
                                                 2,
                                                 &ab) == D_FTP_OK) &&
                           (d_ftp_percent_decode("c%41",
                                                 4,
                                                 &ab) == D_FTP_OK) &&
                           (d_tests_ftp_buffer_is(&ab,
                                                  "abcA")) );
        D_TESTS_FTP_CHECK( (d_ftp_percent_decode("%0d",
                                                 3,
                                                 &ab) ==
                            D_FTP_ERROR_MALFORMED) &&
                           (d_tests_ftp_buffer_is(&ab,
                                                  "abcA")) );
        D_TESTS_FTP_CHECK( (d_ftp_percent_decode("defg",
                                                 4,
                                                 &ab) ==
                            D_FTP_ERROR_BUFFER_TOO_SMALL) &&
                           (d_tests_ftp_buffer_is(&ab,
                                                  "abcA")) );
        D_TESTS_FTP_CHECK( (d_ftp_percent_encode("d e",
                                                 3,
                                                 false,
                                                 &ab) ==
                            D_FTP_ERROR_BUFFER_TOO_SMALL) &&
                           (d_tests_ftp_buffer_is(&ab,
                                                  "abcA")) );
        D_TESTS_FTP_CHECK( (d_ftp_percent_encode("d/",
                                                 2,
                                                 true,
                                                 &ab) == D_FTP_OK) &&
                           (d_tests_ftp_buffer_is(&ab,
                                                  "abcAd/")) );
    }
    D_TESTS_FTP_CHECK( (d_ftp_percent_decode(NULL,
                                             1,
                                             &B) ==
                        D_FTP_ERROR_INVALID_ARGUMENT) &&
                       (d_ftp_percent_decode("a",
                                             1,
                                             NULL) ==
                        D_FTP_ERROR_INVALID_ARGUMENT) );
    D_TESTS_FTP_CHECK( (d_ftp_percent_encode(NULL,
                                             1,
                                             false,
                                             &B) ==
                        D_FTP_ERROR_INVALID_ARGUMENT) &&
                       (d_ftp_percent_encode("a",
                                             1,
                                             false,
                                             NULL) ==
                        D_FTP_ERROR_INVALID_ARGUMENT) );
    d_tests_ftp_buffer_reset(&B);

    return;
}

/*
t_urls_ascii_5
  t_urls_ascii, continued: d_ftp_percent_encode
*/
static void
t_urls_ascii_5(
    void
)
{
    D_TESTS_FTP_CHECK( (d_ftp_percent_encode("a b/c",
                                             5,
                                             true,
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "a%20b/c")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_percent_encode("/a/b c/",
                                             7,
                                             true,
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "/a/b%20c/")) );
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_percent_encode("a b/c\xff",
                                             6,
                                             false,
                                             &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "a%20b%2Fc%FF")) );

    return;
}

/*
t_urls_ascii_6
  t_urls_ascii, continued: d_ftp_ascii_finish, d_ftp_ascii_from_network,
d_ftp_ascii_init, d_ftp_ascii_to_network, d_ftp_buffer_init
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_urls_ascii_6(
    void
)
{
    struct d_ftp_ascii_state st = { 0 };
    size_t                   used = 0;

    d_ftp_ascii_init(&st);
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_ascii_to_network(&st,
                                               "a\nb\r\nc",
                                               6,
                                               &used,
                                               &B) == D_FTP_OK) &&
                       (used == 6)                              &&
                       (d_tests_ftp_buffer_is(&B,
                                              "a\r\nb\r\nc")) );
    d_ftp_ascii_init(&st);
    d_tests_ftp_buffer_reset(&B);
    d_ftp_ascii_to_network(&st,
                           "a\r",
                           2,
                           &used,
                           &B);
    d_ftp_ascii_to_network(&st,
                           "\nb",
                           2,
                           &used,
                           &B);
    D_TESTS_FTP_CHECK(d_tests_ftp_buffer_is(&B,
                                            "a\r\nb"));
    d_ftp_ascii_init(&st);
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_ascii_from_network(&st,
                                                 "a\r\nb\r\0c",
                                                 7,
                                                 &used,
                                                 &B) == D_FTP_OK) &&
                       (used == 7)                                &&
                       (B.length == 5)                            &&
                       (memcmp(B.data,
                               "a\nb\rc",
                               5) == 0) );
    d_ftp_ascii_init(&st);
    d_tests_ftp_buffer_reset(&B);
    d_ftp_ascii_from_network(&st,
                             "a\r",
                             2,
                             &used,
                             &B);
    d_ftp_ascii_from_network(&st,
                             "\nb",
                             2,
                             &used,
                             &B);
    D_TESTS_FTP_CHECK(d_tests_ftp_buffer_is(&B,
                                            "a\nb"));
    d_ftp_ascii_init(&st);
    d_tests_ftp_buffer_reset(&B);
    d_ftp_ascii_from_network(&st,
                             "x\r",
                             2,
                             &used,
                             &B);
    D_TESTS_FTP_CHECK( (d_tests_ftp_buffer_is(&B,
                                              "x"))         &&
                       (d_ftp_ascii_finish(&st,
                                           &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "x\r")) );
    d_ftp_ascii_init(&st);
    d_tests_ftp_buffer_reset(&B);
    D_TESTS_FTP_CHECK( (d_ftp_ascii_from_network(&st,
                                                 "a\rb",
                                                 3,
                                                 &used,
                                                 &B) == D_FTP_OK) &&
                       (d_tests_ftp_buffer_is(&B,
                                              "a\rb")) );
    char                s3[3];  // filled before it is read
    struct d_ftp_buffer b3 = { 0 };
    d_ftp_buffer_init(&b3,
                      s3,
                      sizeof s3);
    d_ftp_ascii_init(&st);
    D_TESTS_FTP_CHECK( (d_ftp_ascii_to_network(&st,
                                               "\n\n",
                                               2,
                                               &used,
                                               &b3) == D_FTP_OK) &&
                       (used == 1)                               &&
                       (d_tests_ftp_buffer_is(&b3,
                                              "\r\n")) );

    return;
}

/*
t_urls_ascii_7
  t_urls_ascii, continued: d_ftp_buffer_init, d_ftp_command_format,
d_ftp_options_init, d_net_line_next
*/
static void
t_urls_ascii_7(
    void
)
{
    struct d_ftp_options o = { 0 };
    d_ftp_options_init(&o);
    D_TESTS_FTP_CHECK( (o.user == NULL)                                &&
                       (o.require_security)                            &&
                       (o.data_protection == D_FTP_PROTECTION_PRIVATE) &&
                       (o.data_mode == D_FTP_DATA_PASSIVE_AUTO) );
    D_TESTS_FTP_CHECK( (o.type.data_type == D_FTP_TYPE_IMAGE) &&
                       (o.ignore_pasv_address)                &&
                       (o.verify_peer)                        &&
                       (o.verify_host)                        &&
                       (o.connect_timeout_ms == 30000)        &&
                       (o.response_timeout_ms == 60000) );
    struct d_ftp_buffer nb = { 0 };
    d_ftp_buffer_init(&nb,
                      NULL,
                      64);
    D_TESTS_FTP_CHECK( (nb.capacity == 0) &&
                       (d_ftp_command_format(D_FTP_COMMAND_NOOP,
                                             NULL,
                                             &nb) ==
                        D_FTP_ERROR_INVALID_ARGUMENT) );
    struct d_pack_text cur    = { "a\r\nb\n\nc\n", 8 }, line;
    int                n      = 0;
    const char*        want[] = { "a", "b", "", "c" };
    while (d_net_line_next(&cur,
                           &line))
    {
        D_TESTS_FTP_CHECK( (n < 4) &&
                           (d_tests_ftp_span_is(line,
                                                want[n])) );
        n++;
    }

    D_TESTS_FTP_CHECK(n == 4);

    return;
}

/*
t_errors_to_net
  Checks d_ftp_error_to_net.
*/
static void
t_errors_to_net(
    void
)
{
    static const struct
    {
        enum d_ftp_error from;
        enum d_net_error to;
    } MAP[] = { { D_FTP_OK, D_NET_OK },
                { D_FTP_ERROR_INVALID_ARGUMENT, D_NET_ERROR_INVALID_ARGUMENT },
                { D_FTP_ERROR_BUFFER_TOO_SMALL, D_NET_ERROR_INVALID_ARGUMENT },
                { D_FTP_ERROR_MALFORMED, D_NET_ERROR_PROTOCOL },
                { D_FTP_ERROR_UNSUPPORTED, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_OUT_OF_MEMORY, D_NET_ERROR_OUT_OF_MEMORY },
                { D_FTP_ERROR_RESOLVE, D_NET_ERROR_HOST_UNREACHABLE },
                { D_FTP_ERROR_CONNECT, D_NET_ERROR_CONNECTION_REFUSED },
                { D_FTP_ERROR_TIMED_OUT, D_NET_ERROR_TIMED_OUT },
                { D_FTP_ERROR_CLOSED, D_NET_ERROR_CLOSED },
                { D_FTP_ERROR_TLS, D_NET_ERROR_PROTOCOL },
                { D_FTP_ERROR_SERVICE_UNAVAILABLE, D_NET_ERROR_CLOSED },
                { D_FTP_ERROR_DATA_CONNECTION, D_NET_ERROR_CONNECTION_REFUSED },
                { D_FTP_ERROR_TRANSFER_ABORTED, D_NET_ERROR_CLOSED },
                { D_FTP_ERROR_FILE_UNAVAILABLE, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_LOCAL_ERROR, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_INSUFFICIENT_STORAGE, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_COMMAND_UNRECOGNIZED, D_NET_ERROR_PROTOCOL },
                { D_FTP_ERROR_SYNTAX, D_NET_ERROR_PROTOCOL },
                { D_FTP_ERROR_NOT_IMPLEMENTED, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_BAD_SEQUENCE, D_NET_ERROR_PROTOCOL },
                { D_FTP_ERROR_PROTOCOL_UNSUPPORTED, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_LOGIN_DENIED, D_NET_ERROR_ACCESS_DENIED },
                { D_FTP_ERROR_ACCOUNT_REQUIRED, D_NET_ERROR_ACCESS_DENIED },
                { D_FTP_ERROR_PAGE_TYPE_UNKNOWN, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_NAME_NOT_ALLOWED, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_SECURITY, D_NET_ERROR_ACCESS_DENIED },
                { D_FTP_ERROR_REJECTED, D_NET_ERROR_UNKNOWN },
                { D_FTP_ERROR_UNEXPECTED_REPLY, D_NET_ERROR_PROTOCOL },
                { D_FTP_ERROR_UNKNOWN, D_NET_ERROR_UNKNOWN } };

    for (size_t i = 0; i < sizeof MAP / sizeof MAP[0]; i++)
    {
        D_TESTS_FTP_CHECK(d_ftp_error_to_net(MAP[i].from) == MAP[i].to);
    }

    D_TESTS_FTP_CHECK(d_ftp_error_to_net((enum d_ftp_error)99) ==
                      D_NET_ERROR_UNKNOWN);

    return;
}

// a scripted transport behind a connection's TCP: reads deliver `in`, then
// report `read_error`; writes take at most `limit` bytes (0: any) into `out`,
// or nothing -- with `write_error`, or, when `stalls`, with no error at all
static struct
{
    const char*      in;
    size_t           in_len, in_at;
    enum d_net_error read_error;
    char             out[4096];
    size_t           out_len, limit;
    enum d_net_error write_error;
    bool             stalls;
} g_fake;

static struct d_net_io_result
fake_read(
    struct d_net_connection* _c,
    void*                    _b,
    size_t                   _n
)
{
    size_t left = g_fake.in_len - g_fake.in_at;
    (void)_c;
    if (left == 0)
    {
        return d_net_io_result_make(0,
                                    g_fake.read_error);
    }

    if (_n > left)
    {
        _n = left;
    }

    memcpy(_b,
           g_fake.in + g_fake.in_at,
           _n);
    g_fake.in_at += _n;

    return d_net_io_result_make(_n,
                                D_NET_OK);
}

static struct d_net_io_result
fake_write(
    struct d_net_connection* _c,
    const void*              _d,
    size_t                   _n
)
{
    (void)_c;
    if (g_fake.write_error != D_NET_OK || g_fake.stalls)
    {
        return d_net_io_result_make(0,
                                    g_fake.write_error);
    }

    if (g_fake.limit && _n > g_fake.limit)
    {
        _n = g_fake.limit;
    }

    if (_n > sizeof g_fake.out - g_fake.out_len)
    {
        _n = sizeof g_fake.out - g_fake.out_len;
    }

    memcpy(g_fake.out + g_fake.out_len,
           _d,
           _n);
    g_fake.out_len += _n;

    return d_net_io_result_make(_n,
                                D_NET_OK);
}

static bool
fake_is_open(
    const struct d_net_connection* _c
)
{
    (void)_c;

    return true;
}

static void
fake_close(
    struct d_net_connection* _c
)
{
    (void)_c;

    return;
}
static const struct d_net_connection_vtable FAKE = {
    fake_read, fake_write, fake_is_open, fake_close, NULL,
    NULL,      NULL,       NULL,         NULL
};

static void
fake_on(
    struct d_ftp_connection* _k,
    const char*              _in,
    enum d_net_error         _read_error,
    size_t                   _limit,
    enum d_net_error         _write_error
)
{
    memset(&g_fake,
           0,
           sizeof g_fake);
    g_fake.in          = _in;
    g_fake.in_len      = _in ? strlen(_in) : 0;
    g_fake.read_error  = _read_error;
    g_fake.limit       = _limit;
    g_fake.write_error = _write_error;
    memset(_k,
           0,
           sizeof *_k);
    d_tcp_connection_init(&_k->tcp);
    (void)d_net_connection_init(&_k->tcp.base,
                                &FAKE);

    return;
}

static struct d_ftp_client g_cl;

/*
t_adapter_plain
  Checks d_ftp_client_init.
*/
static void
t_adapter_plain(
    void
)
{
    d_ftp_client_init(&g_cl,
                      NULL,
                      NULL);

    return;
}

/*
t_adapter_plain_2
  t_adapter_plain, continued: d_ftp_internal_client_failure,
d_ftp_internal_client_read, d_ftp_internal_client_write
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_adapter_plain_2(
    void
)
{
    static const enum d_net_error GONE[] = { D_NET_ERROR_CLOSED,
                                             D_NET_ERROR_CONNECTION_RESET,
                                             D_NET_ERROR_CONNECTION_ABORTED };
    struct d_ftp_connection*      k      = &g_cl.control;
    char                          buf[64];  // filled before it is read
    size_t                        n = 99;

    // reads: the bytes, the end of the stream, a failure with its error kept
    fake_on(k,
            "220 hi\r\n",
            D_NET_OK,
            0,
            D_NET_OK);
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_read(k,
                                                   buf,
                                                   sizeof buf,
                                                   &n) == D_SSL_OK) &&
                       (n == 8)                                     &&
                       (!memcmp(buf,
                                "220 hi\r\n",
                                8))                                 &&
                       (k->failure == D_NET_OK) );
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_read(k,
                                                   buf,
                                                   sizeof buf,
                                                   &n) == D_SSL_ERROR_CLOSED) &&
                       (n == 0) );
    fake_on(k,
            NULL,
            D_NET_ERROR_TIMED_OUT,
            0,
            D_NET_OK);
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_read(k,
                                                   buf,
                                                   sizeof buf,
                                                   &n) ==
                        D_SSL_ERROR_TRANSPORT) &&
                       (k->failure == D_NET_ERROR_TIMED_OUT) );
    // writes: whole through short ones, a departed peer CLOSED, any other
    // failure TRANSPORT
    fake_on(k,
            NULL,
            D_NET_OK,
            5,
            D_NET_OK);
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_write(k,
                                                    "NOOP\r\n",
                                                    6) == D_SSL_OK) &&
                       (g_fake.out_len == 6)                        &&
                       (!memcmp(g_fake.out,
                                "NOOP\r\n",
                                6))                                 &&
                       (k->failure == D_NET_OK) );
    for (size_t i = 0; i < sizeof GONE / sizeof GONE[0]; i++)
    {
        fake_on(k,
                NULL,
                D_NET_OK,
                0,
                GONE[i]);
        D_TESTS_FTP_CHECK( (d_ftp_internal_client_write(k,
                                                        "QUIT\r\n",
                                                        6) ==
                            D_SSL_ERROR_CLOSED) &&
                           (k->failure == GONE[i]) );
    }

    fake_on(k,
            NULL,
            D_NET_OK,
            0,
            D_NET_ERROR_TIMED_OUT);
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_write(k,
                                                    "QUIT\r\n",
                                                    6) ==
                        D_SSL_ERROR_TRANSPORT) &&
                       (k->failure == D_NET_ERROR_TIMED_OUT) );
    fake_on(k,
            NULL,
            D_NET_OK,
            0,
            D_NET_OK);
    g_fake.stalls = true;
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_write(k,
                                                    "QUIT\r\n",
                                                    6) ==
                        D_SSL_ERROR_TRANSPORT) &&
                       (k->failure == D_NET_OK) );
    // judging: the transport's timeout first, then a stream that ended or
    // broke, then TLS's -- truncation counting as lost on the control
    // connection alone
    k->failure = D_NET_ERROR_TIMED_OUT;
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_failure(&g_cl,
                                                      k,
                                                      D_SSL_ERROR_TRANSPORT,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_TIMED_OUT) &&
                       (g_cl.net_error == D_NET_ERROR_TIMED_OUT) );
    k->failure = D_NET_ERROR_CONNECTION_RESET;
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_failure(&g_cl,
                                                      k,
                                                      D_SSL_ERROR_CLOSED,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_CLOSED) &&
                       (g_cl.net_error == D_NET_ERROR_CONNECTION_RESET) );
    k->failure = D_NET_OK;
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_failure(&g_cl,
                                                      k,
                                                      D_SSL_ERROR_TRANSPORT,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_CLOSED) &&
                       (g_cl.net_error == D_NET_OK) );
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_failure(&g_cl,
                                                      k,
                                                      D_SSL_ERROR_TRUNCATED,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_CLOSED) &&
                       (g_cl.tls_status == D_SSL_OK) );
    const enum d_ftp_error data_failure =
        d_ftp_internal_client_failure(&g_cl,
                                      &g_cl.data,
                                      D_SSL_ERROR_TRUNCATED,
                                      D_FTP_ERROR_DATA_CONNECTION);

    D_TESTS_FTP_CHECK( (data_failure == D_FTP_ERROR_TLS) &&
                       (g_cl.tls_status == D_SSL_ERROR_TRUNCATED) );
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_failure(&g_cl,
                                                      k,
                                                      D_SSL_ERROR_VERIFY,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_TLS) &&
                       (g_cl.tls_status == D_SSL_ERROR_VERIFY) );

    return;
}

// a session in force over the scripted transport, as secure() leaves one
static int
secured(
    struct d_ftp_connection*    _k,
    const struct d_ssl_context* _ctx
)
{
    const struct d_pack_text host = { "ftp.example", 11 };
    _k->secured =
        d_ssl_session_init(&_k->tls,
                           _ctx,
                           &_k->tcp.base,
                           host) == D_SSL_OK;

    return _k->secured;
}

static void
unsecured(
    struct d_ftp_connection* _k
)
{
    d_ssl_session_destroy(&_k->tls);
    _k->secured = false;

    return;
}

/*
t_adapter_tls
  Checks d_ftp_client_init, d_ftp_client_tls_config,
d_ftp_internal_client_failure, d_ftp_internal_client_read,
d_ftp_internal_client_secure, d_ftp_internal_client_write,
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static void
t_adapter_tls(
    void
)
{
    struct d_ftp_options o = { 0 };
    struct d_ssl_config  c = { 0 };
    struct d_ssl_context ctx = { 0 };
    char                 buf[8];  // filled before it is read
    size_t               n = 0;
    enum d_ssl_error     st = D_SSL_OK;
    d_ftp_options_init(&o);
    o.verify_peer = false;
    o.verify_host = false;
    d_ftp_client_tls_config(&o,
                            &c);
    if (!d_ssl_engine_openssl() ||
        d_ssl_context_init(&ctx,
                           d_ssl_engine_openssl(),
                           &c) != D_SSL_OK)
    {
        printf("no TLS engine: the adapter's TLS half skipped\n");

        return;
    }

    d_ftp_client_init(&g_cl,
                      &o,
                      &ctx);
    strcpy(g_cl.host,
           "ftp.example");
    // securing: a handshake whose transport times out leaves no session and
    // says why; one whose peer hangs up is TLS's failure
    fake_on(&g_cl.control,
            NULL,
            D_NET_ERROR_TIMED_OUT,
            0,
            D_NET_OK);
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_secure(&g_cl,
                                                     &g_cl.control) ==
                        D_FTP_ERROR_TIMED_OUT) &&
                       (!g_cl.control.secured) &&
                       (g_fake.out_len > 0) );
    D_TESTS_FTP_CHECK( (g_cl.tls_status == D_SSL_ERROR_TRANSPORT) &&
                       (g_cl.net_error == D_NET_ERROR_TIMED_OUT)  &&
                       (g_cl.control.failure == D_NET_ERROR_TIMED_OUT) );
    fake_on(&g_cl.control,
            NULL,
            D_NET_OK,
            0,
            D_NET_OK);
    D_TESTS_FTP_CHECK( (d_ftp_internal_client_secure(&g_cl,
                                                     &g_cl.control) ==
                        D_FTP_ERROR_TLS)                          &&
                       (!g_cl.control.secured)                    &&
                       (g_cl.tls_status == D_SSL_ERROR_TRUNCATED) &&
                       (g_cl.net_error == D_NET_OK) );
    // in force: a read's transport error, then a write's, reach the judgment
    fake_on(&g_cl.control,
            NULL,
            D_NET_ERROR_TIMED_OUT,
            0,
            D_NET_OK);
    D_TESTS_FTP_CHECK(secured(&g_cl.control,
                              &ctx));
    st = d_ftp_internal_client_read(&g_cl.control,
                                    buf,
                                    sizeof buf,
                                    &n);
    D_TESTS_FTP_CHECK( (st == D_SSL_ERROR_TRANSPORT) &&
                       (d_ftp_internal_client_failure(&g_cl,
                                                      &g_cl.control,
                                                      st,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_TIMED_OUT)       &&
                       (g_cl.net_error == D_NET_ERROR_TIMED_OUT) );
    unsecured(&g_cl.control);
    fake_on(&g_cl.control,
            NULL,
            D_NET_OK,
            0,
            D_NET_ERROR_CONNECTION_RESET);
    D_TESTS_FTP_CHECK(secured(&g_cl.control,
                              &ctx));
    st = d_ftp_internal_client_write(&g_cl.control,
                                     "NOOP\r\n",
                                     6);
    D_TESTS_FTP_CHECK( (st == D_SSL_ERROR_TRUNCATED) &&
                       (d_ftp_internal_client_failure(&g_cl,
                                                      &g_cl.control,
                                                      st,
                                                      D_FTP_ERROR_CLOSED) ==
                        D_FTP_ERROR_CLOSED)          &&
                       (g_cl.net_error == D_NET_ERROR_CONNECTION_RESET) );
    unsecured(&g_cl.control);
    d_ssl_context_destroy(&ctx);

    return;
}

/* d_ftp_error_from_net: each of net.h's errors, an unknown value, and the round
   trip from d_ftp_error_to_net for every transport error. */
static void
t_errors_from_net(
    void
)
{
    static const struct
    {
        enum d_net_error n;
        enum d_ftp_error f;
    } T[] = { { D_NET_OK, D_FTP_OK },
              { D_NET_ERROR_INVALID_ARGUMENT, D_FTP_ERROR_INVALID_ARGUMENT },
              { D_NET_ERROR_OUT_OF_MEMORY, D_FTP_ERROR_OUT_OF_MEMORY },
              { D_NET_ERROR_TIMED_OUT, D_FTP_ERROR_TIMED_OUT },
              { D_NET_ERROR_CLOSED, D_FTP_ERROR_CLOSED },
              { D_NET_ERROR_CONNECTION_RESET, D_FTP_ERROR_CLOSED },
              { D_NET_ERROR_CONNECTION_ABORTED, D_FTP_ERROR_CLOSED },
              { D_NET_ERROR_ADDRESS_INVALID, D_FTP_ERROR_RESOLVE },
              { D_NET_ERROR_HOST_UNREACHABLE, D_FTP_ERROR_RESOLVE },
              { D_NET_ERROR_CONNECTION_REFUSED, D_FTP_ERROR_CONNECT },
              { D_NET_ERROR_NETWORK_DOWN, D_FTP_ERROR_CONNECT },
              { D_NET_ERROR_ACCESS_DENIED, D_FTP_ERROR_CONNECT },
              { D_NET_ERROR_PROTOCOL, D_FTP_ERROR_MALFORMED },
              { D_NET_ERROR_WOULD_BLOCK, D_FTP_ERROR_UNKNOWN },
              { D_NET_ERROR_INTERRUPTED, D_FTP_ERROR_UNKNOWN },
              { D_NET_ERROR_ADDRESS_IN_USE, D_FTP_ERROR_UNKNOWN },
              { D_NET_ERROR_MESSAGE_TOO_LARGE, D_FTP_ERROR_UNKNOWN },
              { D_NET_ERROR_TOO_MANY_OPEN_FILES, D_FTP_ERROR_UNKNOWN },
              { D_NET_ERROR_UNKNOWN, D_FTP_ERROR_UNKNOWN },
              { (enum d_net_error)9999, D_FTP_ERROR_UNKNOWN } };

    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++)
    {
        D_TESTS_FTP_CHECK(d_ftp_error_from_net(T[i].n) == T[i].f);
    }

    static const enum d_ftp_error R[] = { D_FTP_OK,
                                          D_FTP_ERROR_INVALID_ARGUMENT,
                                          D_FTP_ERROR_OUT_OF_MEMORY,
                                          D_FTP_ERROR_RESOLVE,
                                          D_FTP_ERROR_CONNECT,
                                          D_FTP_ERROR_TIMED_OUT,
                                          D_FTP_ERROR_CLOSED };

    for (size_t i = 0; i < sizeof R / sizeof R[0]; i++)
    {
        D_TESTS_FTP_CHECK(d_ftp_error_from_net(d_ftp_error_to_net(R[i])) ==
                          R[i]);
    }

    return;
}

/*
t_timeouts
  Phase 3's finding: the control connection is bounded by
response_timeout_ms. A listener that never greets -- the kernel completes
the handshake from its backlog, and no 220 comes -- and connect must give
up near the limit rather than wait for ever.
*/
static void
t_timeouts(
    void
)
{
    int                l = socket(AF_INET,
                                  SOCK_STREAM,
                                  0);
    struct sockaddr_in a = { 0 };
    socklen_t          al = sizeof a;
    memset(&a,
           0,
           sizeof a);
    a.sin_family      = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    D_TESTS_FTP_CHECK( (l >= 0)              &&
                       (bind(l,
                             (struct sockaddr*)&a,
                             sizeof a) == 0) &&
                       (listen(l,
                               4) == 0)      &&
                       (getsockname(l,
                                    (struct sockaddr*)&a,
                                    &al) == 0) );
    struct d_ftp_options o = { 0 };
    d_ftp_options_init(&o);
    o.response_timeout_ms = 300;
    static struct d_ftp_client c = { 0 };
    d_ftp_client_init(&c,
                      &o,
                      NULL);
    struct timespec t0 = { 0 };
    struct timespec t1 = { 0 };
    clock_gettime(CLOCK_MONOTONIC,
                  &t0);
    enum d_ftp_error e =
        d_ftp_client_connect(&c,
                             "127.0.0.1",
                             ntohs(a.sin_port));
    clock_gettime(CLOCK_MONOTONIC,
                  &t1);
    long ms = (long)(t1.tv_sec - t0.tv_sec) * 1000L +
              (t1.tv_nsec - t0.tv_nsec) / 1000000L;
    D_TESTS_FTP_CHECK(e == D_FTP_ERROR_TIMED_OUT);
    D_TESTS_FTP_CHECK( (ms >= 250) &&
                       (ms < 5000) );
    d_ftp_client_close(&c);
    close(l);

    return;
}

/*
main
  Runs every test, then prints the count; the exit status is whether any check
failed.
*/
int
main(
    void
)
{
    d_ftp_buffer_init(&B,
                      g_out,
                      sizeof g_out);
    t_replies();
    t_replies_2();
    t_replies_3();
    t_commands();
    t_commands_2();
    t_commands_3();
    t_endpoints();
    t_endpoints_2();
    t_paths_time();
    t_paths_time_2();
    t_paths_time_3();
    t_paths_time_4();
    t_listings();
    t_urls_ascii();
    t_urls_ascii_2();
    t_urls_ascii_3();
    t_urls_ascii_4();
    t_urls_ascii_5();
    t_urls_ascii_6();
    t_urls_ascii_7();
    t_errors_to_net();
    t_adapter_plain();
    t_adapter_plain_2();
    t_adapter_tls();
    t_errors_from_net();
    t_timeouts();
    printf("%d checks, %d failures\n",
           g_checks,
           g_fail);

    return g_fail != 0;
}
