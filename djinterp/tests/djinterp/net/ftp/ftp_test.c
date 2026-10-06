/*******************************************************************************
* djinterp [net]                                                      ftp_test.c
*
* Exercises `ftp.h`.
*
*
* path:      /tests/djinterp/net/ftp/ftp_test.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.27
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "../../../../inc/djinterp/net/ftp/ftp.h"

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)
#define SPAN_IS(s, lit) ((s).length == strlen(lit) && memcmp((s).data, (lit), (s).length) == 0)
#define BUF_IS(b, lit) (strcmp((b).data, (lit)) == 0 && (b).length == strlen(lit))
#define RESET(b) ((b).length = 0, (b).data[0] = '\0')

static char g_out[512];
static struct d_ftp_buffer B;

static enum d_ftp_status feed(struct d_ftp_reply_parser* p, const char* s, size_t n, size_t* used, struct d_ftp_reply* r)
{ return d_ftp_reply_parser_feed(p, s, n, used, r); }

static void t_replies(void)
{
    char st[256]; struct d_ftp_reply_parser p; struct d_ftp_reply r; size_t used = 0;
    d_ftp_reply_parser_init(&p, st, sizeof st);
    const char* s1 = "220 Service ready\r\n";
    CHECK(feed(&p, s1, strlen(s1), &used, &r) == D_FTP_STATUS_COMPLETE);
    CHECK(used == strlen(s1) && r.code == 220 && r.line_count == 1 && SPAN_IS(r.text, "Service ready") && !r.truncated);
    const char* m = "211-Features:\r\n EPSV\r\n MDTM\r\n211 End\r\n";
    CHECK(feed(&p, m, strlen(m), &used, &r) == D_FTP_STATUS_COMPLETE);
    CHECK(r.code == 211 && r.line_count == 4 && SPAN_IS(r.text, "Features:\n EPSV\n MDTM\nEnd"));
    enum d_ftp_status s = D_FTP_STATUS_PENDING; size_t total = 0;
    for (size_t i = 0; i < strlen(m); i++) { s = feed(&p, m + i, 1, &used, &r); total += used; if (s != D_FTP_STATUS_PENDING) break; }
    CHECK(s == D_FTP_STATUS_COMPLETE && total == strlen(m) && SPAN_IS(r.text, "Features:\n EPSV\n MDTM\nEnd"));
    const char* two = "150 Opening\r\n226 Done\r\n";
    CHECK(feed(&p, two, strlen(two), &used, &r) == D_FTP_STATUS_COMPLETE && r.code == 150 && used == 13);
    size_t off = used;
    CHECK(feed(&p, two + off, strlen(two) - off, &used, &r) == D_FTP_STATUS_COMPLETE && r.code == 226 && SPAN_IS(r.text, "Done"));
    const char tel[] = "\xff\xfb\x01" "220 Hi\r\n";
    CHECK(feed(&p, tel, sizeof tel - 1, &used, &r) == D_FTP_STATUS_COMPLETE && SPAN_IS(r.text, "Hi"));
    const char esc[] = "220 a\xff\xff" "b\r\n";
    CHECK(feed(&p, esc, sizeof esc - 1, &used, &r) == D_FTP_STATUS_COMPLETE && SPAN_IS(r.text, "a\xff" "b"));
    const char* h = "250-a\r\n250-b\r\n250 c\n";
    CHECK(feed(&p, h, strlen(h), &used, &r) == D_FTP_STATUS_COMPLETE && SPAN_IS(r.text, "a\nb\nc") && r.line_count == 3);
    CHECK(feed(&p, "226\r\n", 5, &used, &r) == D_FTP_STATUS_COMPLETE && r.code == 226 && r.text.length == 0);
    const char* o = "211-x\r\n123 y\r\n211 z\r\n";
    CHECK(feed(&p, o, strlen(o), &used, &r) == D_FTP_STATUS_COMPLETE && SPAN_IS(r.text, "x\n123 y\nz"));
    CHECK(feed(&p, "230 partial", 11, &used, &r) == D_FTP_STATUS_PENDING && used == 11);
    CHECK(feed(&p, "\r\n", 2, &used, &r) == D_FTP_STATUS_COMPLETE && SPAN_IS(r.text, "partial"));
    char sm[8]; struct d_ftp_reply_parser q; d_ftp_reply_parser_init(&q, sm, sizeof sm);
    CHECK(feed(&q, "220 0123456789\r\n", 16, &used, &r) == D_FTP_STATUS_COMPLETE && r.truncated && SPAN_IS(r.text, "0123456") && sm[7] == '\0');
    d_ftp_reply_parser_reset(&p);
    CHECK(feed(&p, "hello\r\n", 7, &used, &r) == D_FTP_STATUS_FAILED);
    CHECK(feed(&p, "220 ok\r\n", 8, &used, &r) == D_FTP_STATUS_FAILED);
    d_ftp_reply_parser_reset(&p);
    CHECK(feed(&p, "220 ok\r\n", 8, &used, &r) == D_FTP_STATUS_COMPLETE);
    CHECK(feed(&p, "720 no\r\n", 8, &used, &r) == D_FTP_STATUS_FAILED);
    struct d_ftp_reply_parser c; d_ftp_reply_parser_init(&c, NULL, 0);
    CHECK(feed(&c, "200 x\r\n", 7, &used, &r) == D_FTP_STATUS_COMPLETE && r.code == 200 && r.text.length == 0);
    CHECK(d_ftp_reply_parser_feed(NULL, "x", 1, &used, &r) == D_FTP_STATUS_FAILED);

    RESET(B); CHECK(d_ftp_reply_format(220, "Ready", &B) == D_FTP_OK && BUF_IS(B, "220 Ready\r\n"));
    RESET(B); CHECK(d_ftp_reply_format(211, "Features:\n EPSV\n2 digits\nEnd", &B) == D_FTP_OK);
    CHECK(BUF_IS(B, "211-Features:\r\n EPSV\r\n 2 digits\r\n211 End\r\n"));
    RESET(B); CHECK(d_ftp_reply_format(550, "no such file\r\n230 fake", &B) == D_FTP_OK && BUF_IS(B, "550-no such file\r\n550 230 fake\r\n"));
    RESET(B); CHECK(d_ftp_reply_format(421, NULL, &B) == D_FTP_OK && BUF_IS(B, "421 Service not available, closing control connection.\r\n"));
    RESET(B); CHECK(d_ftp_reply_format(299, NULL, &B) == D_FTP_ERROR_INVALID_ARGUMENT);
    CHECK(d_ftp_reply_format(99, "x", &B) == D_FTP_ERROR_INVALID_ARGUMENT);
    char t8[8]; struct d_ftp_buffer tb; d_ftp_buffer_init(&tb, t8, sizeof t8);
    CHECK(d_ftp_reply_format(220, "Ready now", &tb) == D_FTP_ERROR_BUFFER_TOO_SMALL && tb.length == 0 && t8[0] == '\0');
    CHECK(d_ftp_error_from_reply(530) == D_FTP_ERROR_LOGIN_DENIED && d_ftp_error_from_reply(226) == D_FTP_OK);
    CHECK(d_ftp_error_from_reply(550) == D_FTP_ERROR_FILE_UNAVAILABLE && d_ftp_error_from_reply(599) == D_FTP_ERROR_REJECTED);
    CHECK(d_ftp_error_from_reply(99) == D_FTP_ERROR_MALFORMED && d_ftp_error_from_reply(632) == D_FTP_ERROR_UNSUPPORTED);
    CHECK(d_ftp_error_from_reply(452) == D_FTP_ERROR_INSUFFICIENT_STORAGE && d_ftp_error_from_reply(535) == D_FTP_ERROR_SECURITY);
    CHECK(d_ftp_reply_class_of(150) == D_FTP_REPLY_CLASS_PRELIMINARY && d_ftp_reply_category_of(530) == D_FTP_REPLY_CATEGORY_AUTHENTICATION);
    CHECK(d_ftp_reply_is_valid(553) && !d_ftp_reply_is_valid(599) && !d_ftp_reply_is_valid(700) && !d_ftp_reply_is_valid(99));
    CHECK(d_ftp_reply_text(227) != NULL && d_ftp_reply_text(299) == NULL);
    for (int e = 0; e <= (int)D_FTP_ERROR_UNKNOWN; e++) CHECK(d_ftp_error_string((enum d_ftp_error)e)[0] != '\0');
}

static void t_commands(void)
{
    RESET(B); CHECK(d_ftp_command_format(D_FTP_COMMAND_RETR, "file.txt", &B) == D_FTP_OK && BUF_IS(B, "RETR file.txt\r\n"));
    RESET(B); CHECK(d_ftp_command_format(D_FTP_COMMAND_PASV, NULL, &B) == D_FTP_OK && BUF_IS(B, "PASV\r\n"));
    RESET(B); CHECK(d_ftp_command_format(D_FTP_COMMAND_PASS, "", &B) == D_FTP_OK && BUF_IS(B, "PASS \r\n"));
    RESET(B); CHECK(d_ftp_command_format(D_FTP_COMMAND_RETR, "a\r\nDELE b", &B) == D_FTP_ERROR_INVALID_ARGUMENT && B.length == 0);
    CHECK(d_ftp_command_format(D_FTP_COMMAND_UNKNOWN, NULL, &B) == D_FTP_ERROR_INVALID_ARGUMENT);
    RESET(B); CHECK(d_ftp_command_format_raw("XSHA256", "f", &B) == D_FTP_OK && BUF_IS(B, "XSHA256 f\r\n"));
    CHECK(d_ftp_command_format_raw("1BAD", NULL, &B) == D_FTP_ERROR_INVALID_ARGUMENT);
    struct d_ftp_command_line cl;
    const char* l1 = "retr file name.txt\r\n";
    CHECK(d_ftp_command_parse(l1, strlen(l1), &cl) == D_FTP_OK && cl.command == D_FTP_COMMAND_RETR && cl.has_argument && SPAN_IS(cl.argument, "file name.txt") && SPAN_IS(cl.verb, "retr"));
    CHECK(d_ftp_command_parse("NOOP", 4, &cl) == D_FTP_OK && cl.command == D_FTP_COMMAND_NOOP && !cl.has_argument);
    CHECK(d_ftp_command_parse("FOO bar", 7, &cl) == D_FTP_OK && cl.command == D_FTP_COMMAND_UNKNOWN && SPAN_IS(cl.verb, "FOO"));
    CHECK(d_ftp_command_parse("", 0, &cl) == D_FTP_ERROR_MALFORMED && d_ftp_command_parse("RE\tTR x", 7, &cl) == D_FTP_ERROR_MALFORMED);
    char ln[] = "\xff\xf4\xff\xf2" "ABOR\r\n";
    CHECK(d_ftp_telnet_strip(ln, strlen(ln)) == 6 && memcmp(ln, "ABOR\r\n", 6) == 0);
    CHECK(d_ftp_command_lookup("mlsd", 4) == D_FTP_COMMAND_MLSD && d_ftp_command_policy(D_FTP_COMMAND_CDUP) == D_FTP_ARGUMENT_NONE);
    CHECK(d_ftp_command_uses_data(D_FTP_COMMAND_LIST) && !d_ftp_command_uses_data(D_FTP_COMMAND_MLST));
    CHECK(strcmp(d_ftp_command_name(D_FTP_COMMAND_RETR), "RETR") == 0 && d_ftp_command_name((enum d_ftp_command)999) == NULL);
    for (int i = 1; i < (int)D_FTP_COMMAND_COUNT; i++) CHECK(d_ftp_command_name((enum d_ftp_command)i) != NULL);

    struct d_ftp_type t = { D_FTP_TYPE_ASCII, D_FTP_FORMAT_NONE, 8 };
    RESET(B); CHECK(d_ftp_type_format(&t, &B) == D_FTP_OK && BUF_IS(B, "A"));
    t.format = D_FTP_FORMAT_NON_PRINT; RESET(B); CHECK(d_ftp_type_format(&t, &B) == D_FTP_OK && BUF_IS(B, "A N"));
    t.data_type = D_FTP_TYPE_LOCAL; RESET(B); CHECK(d_ftp_type_format(&t, &B) == D_FTP_OK && BUF_IS(B, "L 8"));
    t.byte_size = 0; CHECK(d_ftp_type_format(&t, &B) == D_FTP_ERROR_INVALID_ARGUMENT);
    CHECK(d_ftp_type_parse("a n", 3, &t) == D_FTP_OK && t.data_type == D_FTP_TYPE_ASCII && t.format == D_FTP_FORMAT_NON_PRINT);
    CHECK(d_ftp_type_parse("I", 1, &t) == D_FTP_OK && t.data_type == D_FTP_TYPE_IMAGE);
    CHECK(d_ftp_type_parse("L 36", 4, &t) == D_FTP_OK && t.data_type == D_FTP_TYPE_LOCAL && t.byte_size == 36);
    CHECK(d_ftp_type_parse("X", 1, &t) == D_FTP_ERROR_UNSUPPORTED && d_ftp_type_parse("A Q", 3, &t) == D_FTP_ERROR_MALFORMED);
    enum d_ftp_structure so; enum d_ftp_transfer_mode mo; enum d_ftp_protection pr;
    CHECK(d_ftp_structure_from_code('r', &so) && so == D_FTP_STRUCTURE_RECORD);
    CHECK(d_ftp_mode_from_code('z', &mo) && mo == D_FTP_MODE_DEFLATE);
    CHECK(d_ftp_protection_from_code('p', &pr) && pr == D_FTP_PROTECTION_PRIVATE && !d_ftp_protection_from_code('x', &pr));
}

static void t_endpoints(void)
{
    struct d_ftp_endpoint ep;
    const char* p1 = "Entering Passive Mode (192,168,1,2,19,137).";
    CHECK(d_ftp_parse_pasv(p1, strlen(p1), &ep) == D_FTP_OK && ep.family == D_FTP_FAMILY_IPV4 && strcmp(ep.address, "192.168.1.2") == 0 && ep.port == 5001);
    const char* p2 = "Entering Passive Mode 10,0,0,1,4,1";
    CHECK(d_ftp_parse_pasv(p2, strlen(p2), &ep) == D_FTP_OK && strcmp(ep.address, "10.0.0.1") == 0 && ep.port == 1025);
    const char* p3 = "v2 mode =127,0,0,1,0,21";
    CHECK(d_ftp_parse_pasv(p3, strlen(p3), &ep) == D_FTP_OK && strcmp(ep.address, "127.0.0.1") == 0 && ep.port == 21);
    CHECK(d_ftp_parse_pasv("(300,1,1,1,1,1)", 15, &ep) == D_FTP_ERROR_MALFORMED);
    const char* e1 = "Entering Extended Passive Mode (|||6446|)";
    CHECK(d_ftp_parse_epsv(e1, strlen(e1), &ep) == D_FTP_OK && ep.port == 6446 && ep.family == D_FTP_FAMILY_NONE);
    CHECK(d_ftp_parse_epsv("(!!!80!)", 8, &ep) == D_FTP_OK && ep.port == 80);
    CHECK(d_ftp_parse_epsv("(|||0|)", 7, &ep) == D_FTP_ERROR_MALFORMED && d_ftp_parse_epsv("(|||80", 6, &ep) == D_FTP_ERROR_MALFORMED);
    CHECK(d_ftp_parse_port("132,235,1,2,24,131", 18, &ep) == D_FTP_OK && strcmp(ep.address, "132.235.1.2") == 0 && ep.port == 6275);
    CHECK(d_ftp_parse_port("1,2,3,4,5,6,7", 13, &ep) == D_FTP_ERROR_MALFORMED);
    const char* x1 = "|1|132.235.1.2|6275|"; const char* x2 = "|2|1080::8:800:200C:417A|5282|";
    CHECK(d_ftp_parse_eprt(x1, strlen(x1), &ep) == D_FTP_OK && ep.family == D_FTP_FAMILY_IPV4 && ep.port == 6275);
    CHECK(d_ftp_parse_eprt(x2, strlen(x2), &ep) == D_FTP_OK && ep.family == D_FTP_FAMILY_IPV6 && strcmp(ep.address, "1080::8:800:200C:417A") == 0);
    CHECK(d_ftp_parse_eprt("|3|x|1|", 7, &ep) == D_FTP_ERROR_PROTOCOL_UNSUPPORTED);
    CHECK(d_ftp_parse_eprt("|1|1.2.3|5|", 11, &ep) == D_FTP_ERROR_MALFORMED && d_ftp_parse_eprt("|1|1.2.3.4|5|6|", 15, &ep) == D_FTP_ERROR_MALFORMED);
    memset(&ep, 0, sizeof ep); ep.family = D_FTP_FAMILY_IPV4; strcpy(ep.address, "192.168.1.2"); ep.port = 5001;
    RESET(B); CHECK(d_ftp_format_port(&ep, &B) == D_FTP_OK && BUF_IS(B, "192,168,1,2,19,137"));
    RESET(B); CHECK(d_ftp_format_pasv(&ep, &B) == D_FTP_OK && BUF_IS(B, "Entering Passive Mode (192,168,1,2,19,137)."));
    RESET(B); CHECK(d_ftp_format_eprt(&ep, &B) == D_FTP_OK && BUF_IS(B, "|1|192.168.1.2|5001|"));
    struct d_ftp_endpoint e6; memset(&e6, 0, sizeof e6); e6.family = D_FTP_FAMILY_IPV6; strcpy(e6.address, "::1"); e6.port = 21;
    RESET(B); CHECK(d_ftp_format_eprt(&e6, &B) == D_FTP_OK && BUF_IS(B, "|2|::1|21|"));
    CHECK(d_ftp_format_port(&e6, &B) == D_FTP_ERROR_INVALID_ARGUMENT);
    ep.port = 6446; RESET(B); CHECK(d_ftp_format_epsv(&ep, &B) == D_FTP_OK && BUF_IS(B, "Entering Extended Passive Mode (|||6446|)"));
    struct d_ftp_endpoint bad; memset(&bad, 'x', sizeof bad); bad.family = D_FTP_FAMILY_IPV4; bad.port = 1;
    CHECK(d_ftp_format_port(&bad, &B) == D_FTP_ERROR_INVALID_ARGUMENT);
}

static void t_paths_time(void)
{
    RESET(B); CHECK(d_ftp_pathname_quote("/a\"b", &B) == D_FTP_OK && BUF_IS(B, "\"/a\"\"b\""));
    const char* q1 = "\"/usr/dm\" created"; const char* q2 = "\"/a\"\"b\" is current";
    RESET(B); CHECK(d_ftp_pathname_unquote(q1, strlen(q1), &B) == D_FTP_OK && BUF_IS(B, "/usr/dm"));
    RESET(B); CHECK(d_ftp_pathname_unquote(q2, strlen(q2), &B) == D_FTP_OK && BUF_IS(B, "/a\"b"));
    RESET(B); CHECK(d_ftp_pathname_unquote("no quotes", 9, &B) == D_FTP_ERROR_MALFORMED && d_ftp_pathname_unquote("\"open", 5, &B) == D_FTP_ERROR_MALFORMED && B.length == 0);
    RESET(B); CHECK(d_ftp_path_resolve("/home/user", "../etc/./passwd", &B) == D_FTP_OK && BUF_IS(B, "/home/etc/passwd"));
    RESET(B); CHECK(d_ftp_path_resolve("/a", "/b//c/", &B) == D_FTP_OK && BUF_IS(B, "/b/c"));
    RESET(B); CHECK(d_ftp_path_resolve("/", "../../..", &B) == D_FTP_OK && BUF_IS(B, "/"));
    RESET(B); CHECK(d_ftp_path_resolve(NULL, "x", &B) == D_FTP_OK && BUF_IS(B, "/x"));
    RESET(B); CHECK(d_ftp_path_resolve("/a/b", "..", &B) == D_FTP_OK && BUF_IS(B, "/a"));
    RESET(B); CHECK(d_ftp_path_resolve("/a", "../../../etc/passwd", &B) == D_FTP_OK && BUF_IS(B, "/etc/passwd"));
    RESET(B); CHECK(d_ftp_path_resolve("/a", "x\ny", &B) == D_FTP_ERROR_INVALID_ARGUMENT);

    struct d_ftp_time tm;
    CHECK(d_ftp_time_parse("20260925123456", 14, &tm) == D_FTP_OK && tm.year == 2026 && tm.month == 9 && tm.day == 25 && tm.hour == 12 && tm.minute == 34 && tm.second == 56);
    CHECK(d_ftp_time_parse(" 20260925123456.5 ", 18, &tm) == D_FTP_OK && tm.millisecond == 500);
    CHECK(d_ftp_time_parse("20260230000000", 14, &tm) == D_FTP_ERROR_MALFORMED && d_ftp_time_parse("2026092512345", 13, &tm) == D_FTP_ERROR_MALFORMED);
    CHECK(d_ftp_time_parse("20260925123456.", 15, &tm) == D_FTP_ERROR_MALFORMED);
    d_ftp_time_parse("20260925123456.5", 16, &tm);
    RESET(B); CHECK(d_ftp_time_format(&tm, true, &B) == D_FTP_OK && BUF_IS(B, "20260925123456.500"));
    RESET(B); CHECK(d_ftp_time_format(&tm, false, &B) == D_FTP_OK && BUF_IS(B, "20260925123456"));
    struct d_ftp_time ep0 = { 1970, 1, 1, 0, 0, 0, 0 }, y2k = { 2000, 3, 1, 0, 0, 0, 0 };
    int64_t sec = 7;
    CHECK(d_ftp_time_to_unix(&ep0, &sec) == D_FTP_OK && sec == 0);
    CHECK(d_ftp_time_to_unix(&y2k, &sec) == D_FTP_OK && sec == 951868800);
    CHECK(d_ftp_time_from_unix(951868800, &tm) == D_FTP_OK && tm.year == 2000 && tm.month == 3 && tm.day == 1);
    CHECK(d_ftp_time_from_unix(-1, &tm) == D_FTP_OK && tm.year == 1969 && tm.month == 12 && tm.day == 31 && tm.hour == 23 && tm.minute == 59 && tm.second == 59);
    CHECK(d_ftp_time_from_unix(1000000000, &tm) == D_FTP_OK && tm.year == 2001 && tm.month == 9 && tm.day == 9 && tm.hour == 1 && tm.minute == 46 && tm.second == 40);
    for (int64_t s = -5000000000LL; s < 5000000000LL; s += 7777777) { struct d_ftp_time a; int64_t b2 = 0; d_ftp_time_from_unix(s, &a); d_ftp_time_to_unix(&a, &b2); if (b2 != s) { CHECK(b2 == s); break; } }
    uint64_t sz = 0;
    CHECK(d_ftp_parse_size("  12345 ", 8, &sz) == D_FTP_OK && sz == 12345);
    CHECK(d_ftp_parse_size("18446744073709551615", 20, &sz) == D_FTP_OK && sz == UINT64_MAX);
    CHECK(d_ftp_parse_size("18446744073709551616", 20, &sz) == D_FTP_ERROR_MALFORMED && d_ftp_parse_size("12a", 3, &sz) == D_FTP_ERROR_MALFORMED);
    const char* feat = "Features:\n EPSV\n MDTM\n MLST type*;size*;modify*;\n REST STREAM\n AUTH TLS;SSL\n UTF8\n LANG EN*\n MODE Z\nEnd";
    struct d_ftp_features f;
    CHECK(d_ftp_features_parse(feat, strlen(feat), &f) == D_FTP_OK);
    uint32_t want = D_FTP_FEATURE_EPSV | D_FTP_FEATURE_MDTM | D_FTP_FEATURE_MLST | D_FTP_FEATURE_REST_STREAM | D_FTP_FEATURE_AUTH_TLS | D_FTP_FEATURE_AUTH_SSL | D_FTP_FEATURE_UTF8 | D_FTP_FEATURE_LANG | D_FTP_FEATURE_MODE_Z;
    CHECK(f.flags == want && SPAN_IS(f.mlst_facts, "type*;size*;modify*;") && SPAN_IS(f.languages, "EN*"));
}

static enum d_ftp_line_result P(const char* l, enum d_ftp_listing_format fm, const struct d_ftp_time* now, struct d_ftp_entry* e)
{ return d_ftp_listing_parse(l, strlen(l), fm, now, e); }

static void t_listings(void)
{
    struct d_ftp_time now = { 2026, 9, 26, 12, 0, 0, 0 };
    struct d_ftp_entry e;
    const char* L1 = "-rw-r--r--    1 owner    group        1234 Sep 25 12:34 my file.txt";
    CHECK(P(L1, D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_FILE && e.size == 1234 && e.mode == 0644);
    CHECK(SPAN_IS(e.owner, "owner") && SPAN_IS(e.group, "group") && SPAN_IS(e.name, "my file.txt"));
    CHECK(e.modified.year == 2026 && e.modified.month == 9 && e.modified.day == 25 && e.modified.hour == 12 && e.modified.minute == 34);
    CHECK((e.known & (D_FTP_ENTRY_KNOWN_MODIFIED | D_FTP_ENTRY_KNOWN_TIME | D_FTP_ENTRY_KNOWN_SIZE)) == (D_FTP_ENTRY_KNOWN_MODIFIED | D_FTP_ENTRY_KNOWN_TIME | D_FTP_ENTRY_KNOWN_SIZE) && !(e.known & D_FTP_ENTRY_KNOWN_UTC));
    CHECK(P(L1, D_FTP_LISTING_UNIX, NULL, &e) == D_FTP_LINE_ENTRY && !(e.known & D_FTP_ENTRY_KNOWN_MODIFIED) && SPAN_IS(e.name, "my file.txt"));
    CHECK(P("drwxr-xr-x 2 root root 4096 Jan  5  2024 dir", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_DIRECTORY && e.modified.year == 2024 && e.modified.day == 5 && e.mode == 0755 && !(e.known & D_FTP_ENTRY_KNOWN_TIME));
    CHECK(P("-rw-r--r-- 1 a b 1 Dec 31 23:59 f", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.modified.year == 2025);
    CHECK(P("-rw-r--r-- 1 a b 1 Sep 27 23:59 f", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.modified.year == 2026);
    CHECK(P("lrwxrwxrwx 1 root root 7 Jan 1 2020 bin -> usr/bin", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_SYMLINK && SPAN_IS(e.name, "bin") && SPAN_IS(e.link_target, "usr/bin"));
    CHECK(P("crw-rw-rw- 1 root root 1, 3 Jan 1 2020 null", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_OTHER && !(e.known & D_FTP_ENTRY_KNOWN_SIZE) && SPAN_IS(e.owner, "root") && SPAN_IS(e.group, "root"));
    CHECK(P("-rw-r--r-- 1 ftp 42 Mar 3 2021 x", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && SPAN_IS(e.owner, "ftp") && !(e.known & D_FTP_ENTRY_KNOWN_GROUP) && e.size == 42);
    CHECK(P("-rw-r--r-- 1 u g 10 2026-09-25 12:34 iso.txt", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.modified.year == 2026 && e.modified.minute == 34 && SPAN_IS(e.name, "iso.txt"));
    CHECK(P("-rwsr-xr-x 1 root root 10 Jan 1 2020 su", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.mode == 04755);
    CHECK(P("drwxrwxrwt+ 1 root root 10 Jan 1 2020 tmp", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.mode == 01777);
    CHECK(P("-rw-r--r-- 1 u g 1 Jan 1 2020 sp \r\n", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && SPAN_IS(e.name, "sp "));
    CHECK(P("total 48", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_SKIP && P("\r\n", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_SKIP);
    CHECK(P("-rw-r--r-- 1 u g 1 Feb 30 2020 bad", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_MALFORMED);
    CHECK(P("09-25-26  12:34PM       <DIR>          My Dir", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_DIRECTORY && e.modified.year == 2026 && e.modified.hour == 12 && SPAN_IS(e.name, "My Dir"));
    CHECK(P("09-25-2026  01:05AM              1,234 file.txt", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_FILE && e.size == 1234 && e.modified.hour == 1);
    CHECK(P("12-01-99  12:00AM   5 old.txt", D_FTP_LISTING_AUTO, &now, &e) == D_FTP_LINE_ENTRY && e.modified.year == 1999 && e.modified.hour == 0);
    CHECK(P("09-25-26  13:34PM   5 x", D_FTP_LISTING_DOS, &now, &e) == D_FTP_LINE_MALFORMED);
    CHECK(P("type=file;size=1024;modify=20260925123456;perm=adfrw;unique=U1; report.pdf", D_FTP_LISTING_AUTO, NULL, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_FILE && e.size == 1024 && (e.known & D_FTP_ENTRY_KNOWN_UTC) && SPAN_IS(e.permissions, "adfrw") && SPAN_IS(e.unique, "U1") && SPAN_IS(e.name, "report.pdf"));
    CHECK(P(" type=dir;modify=20260101000000; /pub", D_FTP_LISTING_AUTO, NULL, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_DIRECTORY && SPAN_IS(e.name, "/pub"));
    CHECK(P("type=cdir; .", D_FTP_LISTING_MLSX, NULL, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_CURRENT_DIRECTORY && SPAN_IS(e.name, "."));
    CHECK(P("type=OS.unix=slink:/target;UNIX.mode=0777; link", D_FTP_LISTING_AUTO, NULL, &e) == D_FTP_LINE_ENTRY && e.type == D_FTP_ENTRY_SYMLINK && SPAN_IS(e.link_target, "/target") && e.mode == 0777 && SPAN_IS(e.name, "link"));
    CHECK(d_ftp_listing_detect("random", 6) == D_FTP_LISTING_AUTO && P("random", D_FTP_LISTING_AUTO, NULL, &e) == D_FTP_LINE_MALFORMED);
    CHECK(P("random name", D_FTP_LISTING_NAMES, NULL, &e) == D_FTP_LINE_ENTRY && SPAN_IS(e.name, "random name"));
}

static void t_urls_ascii(void)
{
    struct d_ftp_url u;
    const char* u1 = "ftp://user:p%40ss@ftp.example.com:2121/pub/file.txt;type=i";
    CHECK(d_ftp_url_parse(u1, strlen(u1), &u) == D_FTP_OK && u.scheme == D_FTP_SCHEME_FTP && SPAN_IS(u.user, "user") && SPAN_IS(u.password, "p%40ss"));
    CHECK(SPAN_IS(u.host, "ftp.example.com") && u.has_port && u.port == 2121 && SPAN_IS(u.path, "pub/file.txt") && u.type_code == 'i' && d_ftp_url_port(&u) == 2121);
    const char* u2 = "ftps://[2001:db8::1]/x";
    CHECK(d_ftp_url_parse(u2, strlen(u2), &u) == D_FTP_OK && u.scheme == D_FTP_SCHEME_FTPS && u.ipv6_literal && SPAN_IS(u.host, "2001:db8::1") && d_ftp_url_port(&u) == 990 && SPAN_IS(u.path, "x"));
    CHECK(d_ftp_url_parse("ftp://host", 10, &u) == D_FTP_OK && SPAN_IS(u.host, "host") && u.path.length == 0 && !u.has_user && d_ftp_url_port(&u) == 21);
    CHECK(d_ftp_url_parse("FTPES://a@b/", 12, &u) == D_FTP_OK && u.scheme == D_FTP_SCHEME_FTPES && SPAN_IS(u.user, "a") && SPAN_IS(u.host, "b") && !u.has_password);
    CHECK(d_ftp_url_parse("ftp://u:p@ss@h/", 15, &u) == D_FTP_OK && SPAN_IS(u.password, "p@ss") && SPAN_IS(u.host, "h"));
    CHECK(d_ftp_url_parse("http://x/", 9, &u) == D_FTP_ERROR_UNSUPPORTED && d_ftp_url_parse("ftp:/x", 6, &u) == D_FTP_ERROR_MALFORMED);
    CHECK(d_ftp_url_parse("ftp://", 6, &u) == D_FTP_ERROR_MALFORMED && d_ftp_url_parse("ftp://h:0/", 10, &u) == D_FTP_ERROR_MALFORMED);
    CHECK(d_ftp_url_parse("ftp://h/a b", 11, &u) == D_FTP_ERROR_MALFORMED && d_ftp_url_parse("ftp://h/x;type=q", 16, &u) == D_FTP_ERROR_MALFORMED);
    CHECK(d_ftp_url_parse("ftp://h:/p", 10, &u) == D_FTP_OK && !u.has_port && d_ftp_url_port(&u) == 21);
    CHECK(d_ftp_scheme_security(D_FTP_SCHEME_FTPS) == D_FTP_SECURITY_IMPLICIT && d_ftp_scheme_security(D_FTP_SCHEME_FTPES) == D_FTP_SECURITY_EXPLICIT && d_ftp_scheme_security(D_FTP_SCHEME_FTP) == D_FTP_SECURITY_NONE);
    RESET(B); CHECK(d_ftp_percent_decode("p%40ss", 6, &B) == D_FTP_OK && BUF_IS(B, "p@ss"));
    RESET(B); CHECK(d_ftp_percent_decode("%0d%0a", 6, &B) == D_FTP_ERROR_MALFORMED && B.length == 0);
    CHECK(d_ftp_percent_decode("%zz", 3, &B) == D_FTP_ERROR_MALFORMED && d_ftp_percent_decode("%4", 2, &B) == D_FTP_ERROR_MALFORMED && d_ftp_percent_decode("a%00", 4, &B) == D_FTP_ERROR_MALFORMED);
    RESET(B); CHECK(d_ftp_percent_encode("a b/c", 5, true, &B) == D_FTP_OK && BUF_IS(B, "a%20b/c"));
    RESET(B); CHECK(d_ftp_percent_encode("a b/c\xff", 6, false, &B) == D_FTP_OK && BUF_IS(B, "a%20b%2Fc%FF"));

    struct d_ftp_ascii_state st; size_t used = 0;
    d_ftp_ascii_init(&st); RESET(B);
    CHECK(d_ftp_ascii_to_network(&st, "a\nb\r\nc", 6, &used, &B) == D_FTP_OK && used == 6 && BUF_IS(B, "a\r\nb\r\nc"));
    d_ftp_ascii_init(&st); RESET(B);
    d_ftp_ascii_to_network(&st, "a\r", 2, &used, &B); d_ftp_ascii_to_network(&st, "\nb", 2, &used, &B);
    CHECK(BUF_IS(B, "a\r\nb"));
    d_ftp_ascii_init(&st); RESET(B);
    CHECK(d_ftp_ascii_from_network(&st, "a\r\nb\r\0c", 7, &used, &B) == D_FTP_OK && used == 7 && B.length == 5 && memcmp(B.data, "a\nb\rc", 5) == 0);
    d_ftp_ascii_init(&st); RESET(B);
    d_ftp_ascii_from_network(&st, "a\r", 2, &used, &B); d_ftp_ascii_from_network(&st, "\nb", 2, &used, &B);
    CHECK(BUF_IS(B, "a\nb"));
    d_ftp_ascii_init(&st); RESET(B);
    d_ftp_ascii_from_network(&st, "x\r", 2, &used, &B);
    CHECK(BUF_IS(B, "x") && d_ftp_ascii_finish(&st, &B) == D_FTP_OK && BUF_IS(B, "x\r"));
    d_ftp_ascii_init(&st); RESET(B);
    CHECK(d_ftp_ascii_from_network(&st, "a\rb", 3, &used, &B) == D_FTP_OK && BUF_IS(B, "a\rb"));
    char s3[3]; struct d_ftp_buffer b3; d_ftp_buffer_init(&b3, s3, sizeof s3); d_ftp_ascii_init(&st);
    CHECK(d_ftp_ascii_to_network(&st, "\n\n", 2, &used, &b3) == D_FTP_OK && used == 1 && BUF_IS(b3, "\r\n"));

    struct d_ftp_options o; d_ftp_options_init(&o);
    CHECK(o.user == NULL && o.require_security && o.data_protection == D_FTP_PROTECTION_PRIVATE && o.data_mode == D_FTP_DATA_PASSIVE_AUTO);
    CHECK(o.type.data_type == D_FTP_TYPE_IMAGE && o.ignore_pasv_address && o.verify_peer && o.verify_host && o.connect_timeout_ms == 30000 && o.response_timeout_ms == 60000);
    struct d_ftp_buffer nb; d_ftp_buffer_init(&nb, NULL, 64);
    CHECK(nb.capacity == 0 && d_ftp_command_format(D_FTP_COMMAND_NOOP, NULL, &nb) == D_FTP_ERROR_INVALID_ARGUMENT);
    struct d_ftp_span cur = { "a\r\nb\n\nc\n", 8 }, line;
    int n = 0; const char* want[] = { "a", "b", "", "c" };
    while (d_ftp_span_next_line(&cur, &line)) { CHECK(n < 4 && SPAN_IS(line, want[n])); n++; }
    CHECK(n == 4);
}

int main(void)
{
    d_ftp_buffer_init(&B, g_out, sizeof g_out);
    t_replies(); t_commands(); t_endpoints(); t_paths_time(); t_listings(); t_urls_ascii();
    printf("%d checks, %d failures\n", g_checks, g_fail);
    return g_fail != 0;
}
