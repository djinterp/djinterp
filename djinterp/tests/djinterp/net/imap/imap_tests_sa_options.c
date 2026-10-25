/*******************************************************************************
* djinterp [net]                                         imap_tests_sa_options.c
*
* Tests of the IMAP kernel's session pieces: the tags a client prefixes to
* its commands, the option block every backend accepts, and the choice of
* how to authenticate.
*   The choice is tried against the advertisements real servers make, with
* and without TLS, credentials and the backend's support, and each answer is
* the one imap.h's ranking and security rules give.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_options.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <string.h>  // strcmp


// d_tests_imap_prefix_case
//   struct: a prefix a tagger is offered, and whether it takes it.
struct d_tests_imap_prefix_case
{
    const char* prefix;
    bool        taken;
};

// d_tests_imap_options_case
//   struct: an option block -- `security` and `port` and so on set over a
// zeroed block, NULL leaving a text empty -- and what validating it gives.
struct d_tests_imap_options_case
{
    int32_t           security;
    uint32_t          port;
    uint32_t          mechanisms;
    uint32_t          flags;
    const char*       host;
    const char*       username;
    const char*       password;
    const char*       token;
    enum d_imap_error error;
    const char*       what;
};

// d_tests_imap_auth_case
//   struct: one choice of mechanism: the server's capabilities, what the
// options permit (0: the default set) and the backend supports, the option
// flags and credentials, whether TLS protects the connection, and what
// d_imap_auth_select gives -- a mechanism, or -1 when it fails.
struct d_tests_imap_auth_case
{
    const char*       capabilities;
    uint32_t          permitted;
    uint32_t          supported;
    uint32_t          flags;
    const char*       username;
    const char*       password;
    const char*       token;
    bool              secure;
    enum d_imap_error error;
    int               chosen;
    const char*       what;
};

// EVERY_MECHANISM, NO_SCRAM, NAMED_ONLY, WEAK
//   constant: D_IMAP_AUTH_BIT sets: all fifteen mechanisms; all but the
// SCRAM family; the three only an option block naming them permits; the
// three weak ones.
enum
{
    EVERY_MECHANISM = (1 << D_IMAP_AUTH_COUNT) - 1,
    NO_SCRAM        = EVERY_MECHANISM & ~(0xF << D_IMAP_AUTH_SCRAM_SHA_1),
    NAMED_ONLY      = (1 << D_IMAP_AUTH_GSSAPI)   |
                      (1 << D_IMAP_AUTH_EXTERNAL) |
                      (1 << D_IMAP_AUTH_ANONYMOUS),
    WEAK            = (1 << D_IMAP_AUTH_CRAM_MD5)   |
                      (1 << D_IMAP_AUTH_DIGEST_MD5) |
                      (1 << D_IMAP_AUTH_NTLM)
};

/*
HOST
  The server every option block here names.
*/
static const char HOST[] = "imap.example.com";

/*
PREFIXES
  A tag is 1*<any ASTRING-CHAR except "+">, and a tagger holds eight octets
of prefix: "]" may be in one, an atom-special, a control or 8-bit octet may
not.
*/
static const struct d_tests_imap_prefix_case PREFIXES[] =
{
    { "abcdefgh",  true  },
    { "abcdefghi", false },
    { "a]",        true  },
    { "~-_.:!",    true  },
    { "a+",        false },
    { "a b",       false },
    { "a*",        false },
    { "a%",        false },
    { "a{",        false },
    { "a(",        false },
    { "a\"",       false },
    { "a\\",       false },
    { "a\x01",     false },
    { "\xc3\xa9",  false }
};

/*
OPTIONS
  validate's checks, one change at a time from a block naming HOST.
*/
static const struct d_tests_imap_options_case OPTIONS[] =
{
    { D_IMAP_SECURITY_TLS, 0u, 0u, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_OK, "a host is all a block needs" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 0u, NULL, NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "but it needs one" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 0u, "imap example.com", NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "a host holds no space" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 0u, "imap\x01.example", NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "nor a control character" },
    { 4, 0u, 0u, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "the security mode is one of four" },
    { -1, 0u, 0u, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "none of them negative" },
    { D_IMAP_SECURITY_TLS, 65535u, 0u, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_OK, "a port up to 65535" },
    { D_IMAP_SECURITY_TLS, 65536u, 0u, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_ERROR_RANGE, "and no higher" },
    { D_IMAP_SECURITY_TLS, 0u, 1u << 15, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "no mechanism bit past ANONYMOUS's" },
    { D_IMAP_SECURITY_TLS, 0u, 1u << 14, 0u, HOST, NULL, NULL, NULL,
      D_IMAP_OK, "ANONYMOUS's is known" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 1u << 3, HOST, NULL, NULL, NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "no option bit past the three" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 7u, HOST, NULL, NULL, NULL,
      D_IMAP_OK, "all three together" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 0u, HOST, NULL, "pw", NULL,
      D_IMAP_ERROR_INVALID_ARGUMENT, "a password needs a username" },
    { D_IMAP_SECURITY_TLS, 0u, 0u, 0u, HOST, "fred", "pw", NULL,
      D_IMAP_OK, "credentials travel over TLS" },
    { D_IMAP_SECURITY_STARTTLS, 0u, 0u, 0u, HOST, "fred", "pw", NULL,
      D_IMAP_OK, "and over STARTTLS" },
    { D_IMAP_SECURITY_OPPORTUNISTIC, 0u, 0u, 0u, HOST, "fred", "pw", NULL,
      D_IMAP_OK, "an opportunistic block may carry them" },
    { D_IMAP_SECURITY_NONE, 0u, 0u, 0u, HOST, "fred", "pw", NULL,
      D_IMAP_ERROR_SECURITY, "a password never goes in the clear" },
    { D_IMAP_SECURITY_NONE, 0u, 0u, D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH,
      HOST, "fred", "pw", NULL,
      D_IMAP_OK, "unless the options allow it" },
    { D_IMAP_SECURITY_NONE, 0u, 0u, 0u, HOST, "fred", NULL, "tok",
      D_IMAP_ERROR_SECURITY, "nor a token" },
    { D_IMAP_SECURITY_NONE, 0u, 0u, 0u, HOST, "fred", NULL, NULL,
      D_IMAP_OK, "a username alone may" }
};

/*
CHOICES
  imap.h's ranking -- SCRAM-SHA-256-PLUS down to ANONYMOUS -- over the
mechanisms permitted, supported, advertised and served by the credentials
held, channel binding and exposed secrets needing TLS.
*/
static const struct d_tests_imap_auth_case CHOICES[] =
{
    { "IMAP4rev2 AUTH=PLAIN AUTH=SCRAM-SHA-256 AUTH=SCRAM-SHA-256-PLUS",
      0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_SCRAM_SHA_256_PLUS,
      "channel binding first, under TLS" },
    { "IMAP4rev2 AUTH=PLAIN AUTH=SCRAM-SHA-256 AUTH=SCRAM-SHA-256-PLUS",
      0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, false,
      D_IMAP_OK, D_IMAP_AUTH_SCRAM_SHA_256,
      "and SCRAM without binding when there is no TLS" },
    { "IMAP4rev2 AUTH=PLAIN AUTH=SCRAM-SHA-256 AUTH=SCRAM-SHA-256-PLUS",
      0u, NO_SCRAM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_PLAIN,
      "PLAIN before the LOGIN command" },
    { "IMAP4rev2 AUTH=PLAIN AUTH=SCRAM-SHA-256 AUTH=SCRAM-SHA-256-PLUS",
      0u, NO_SCRAM, 0u, "fred", "pw", NULL, false,
      D_IMAP_ERROR_SECURITY, -1,
      "neither exposes a password without TLS" },
    { "IMAP4rev2 AUTH=PLAIN AUTH=SCRAM-SHA-256 AUTH=SCRAM-SHA-256-PLUS",
      0u, NO_SCRAM, D_IMAP_OPTION_ALLOW_CLEARTEXT_AUTH,
      "fred", "pw", NULL, false,
      D_IMAP_OK, D_IMAP_AUTH_PLAIN,
      "unless the options allow it" },
    { "IMAP4rev1", 0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_LOGIN_COMMAND,
      "the LOGIN command when no AUTH= is offered" },
    { "IMAP4rev1 LOGINDISABLED", 0u, EVERY_MECHANISM, 0u,
      "fred", "pw", NULL, true,
      D_IMAP_ERROR_UNSUPPORTED, -1,
      "but not under LOGINDISABLED" },
    { "IMAP4rev1 AUTH=XOAUTH2 AUTH=OAUTHBEARER AUTH=PLAIN",
      0u, EVERY_MECHANISM, 0u, "fred", NULL, "tok", true,
      D_IMAP_OK, D_IMAP_AUTH_OAUTHBEARER,
      "a token: OAUTHBEARER before XOAUTH2" },
    { "IMAP4rev1 AUTH=XOAUTH2 AUTH=OAUTHBEARER AUTH=PLAIN",
      0u, EVERY_MECHANISM, 0u, "fred", NULL, "tok", false,
      D_IMAP_ERROR_SECURITY, -1,
      "a bearer token needs TLS" },
    { "IMAP4rev1 AUTH=XOAUTH2 AUTH=OAUTHBEARER AUTH=PLAIN",
      0u, EVERY_MECHANISM, 0u, "fred", NULL, NULL, true,
      D_IMAP_ERROR_UNSUPPORTED, -1,
      "no credential, no mechanism" },
    { "IMAP4rev1 AUTH=EXTERNAL AUTH=GSSAPI AUTH=ANONYMOUS",
      0u, EVERY_MECHANISM, 0u, "fred", NULL, NULL, true,
      D_IMAP_ERROR_UNSUPPORTED, -1,
      "EXTERNAL, GSSAPI and ANONYMOUS need naming" },
    { "IMAP4rev1 AUTH=EXTERNAL AUTH=GSSAPI AUTH=ANONYMOUS",
      NAMED_ONLY, EVERY_MECHANISM, 0u, "fred", NULL, NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_EXTERNAL,
      "EXTERNAL before GSSAPI" },
    { "IMAP4rev1 AUTH=EXTERNAL AUTH=GSSAPI AUTH=ANONYMOUS",
      NAMED_ONLY, EVERY_MECHANISM, 0u, "fred", NULL, NULL, false,
      D_IMAP_OK, D_IMAP_AUTH_EXTERNAL,
      "EXTERNAL exposes nothing, so needs no TLS" },
    { "IMAP4rev1 AUTH=EXTERNAL AUTH=GSSAPI AUTH=ANONYMOUS",
      NAMED_ONLY, NAMED_ONLY & ~(1 << D_IMAP_AUTH_EXTERNAL), 0u,
      "fred", NULL, NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_GSSAPI,
      "what the backend lacks is passed over" },
    { "IMAP4rev1 AUTH=EXTERNAL AUTH=GSSAPI AUTH=ANONYMOUS",
      1u << D_IMAP_AUTH_ANONYMOUS, EVERY_MECHANISM, 0u,
      "fred", NULL, NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_ANONYMOUS,
      "ANONYMOUS when it alone is named" },
    { "IMAP4rev1 AUTH=CRAM-MD5 AUTH=DIGEST-MD5 AUTH=NTLM",
      0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_LOGIN_COMMAND,
      "the weak mechanisms need naming" },
    { "IMAP4rev1 AUTH=CRAM-MD5 AUTH=DIGEST-MD5 AUTH=NTLM",
      WEAK, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_CRAM_MD5,
      "CRAM-MD5 first of them" },
    { "IMAP4rev1 AUTH=CRAM-MD5 AUTH=DIGEST-MD5 AUTH=NTLM",
      WEAK, EVERY_MECHANISM, 0u, "fred", "pw", NULL, false,
      D_IMAP_OK, D_IMAP_AUTH_CRAM_MD5,
      "it sends no password, so needs no TLS" },
    { "IMAP4rev1 AUTH=SCRAM-SHA-1-PLUS AUTH=SCRAM-SHA-1",
      0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_SCRAM_SHA_1_PLUS,
      "SCRAM-SHA-1 with binding under TLS" },
    { "IMAP4rev1 AUTH=SCRAM-SHA-1-PLUS AUTH=SCRAM-SHA-1",
      0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, false,
      D_IMAP_OK, D_IMAP_AUTH_SCRAM_SHA_1,
      "and without it otherwise" },
    { "IMAP4rev1 AUTH=PLAIN AUTH=LOGIN",
      0u, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_PLAIN,
      "PLAIN before SASL LOGIN" },
    { "IMAP4rev1 AUTH=PLAIN AUTH=LOGIN",
      1u << D_IMAP_AUTH_LOGIN, EVERY_MECHANISM, 0u, "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_LOGIN,
      "SASL LOGIN when it alone is permitted" },
    { "IMAP4rev1 AUTH=PLAIN AUTH=LOGIN",
      1u << D_IMAP_AUTH_LOGIN_COMMAND, EVERY_MECHANISM, 0u,
      "fred", "pw", NULL, true,
      D_IMAP_OK, D_IMAP_AUTH_LOGIN_COMMAND,
      "and the LOGIN command likewise" },
    { "IMAP4rev1 AUTH=PLAIN",
      0u, EVERY_MECHANISM, 0u, NULL, "pw", NULL, true,
      D_IMAP_ERROR_UNSUPPORTED, -1,
      "a password without a username serves nothing" }
};

/*
d_tests_imap_tags_counter
  The counter: zero-padded to four digits, widening past 9999, and wrapping
after 4294967295 to 1.
*/
static bool
d_tests_imap_tags_counter(
    struct d_test_counter* _counter
)
{
    struct d_imap_tagger tagger = { .next = 0u };
    struct d_imap_tag    tag    = { .length = 0u };
    bool                 result = true;

    (void)d_imap_tagger_init(&tagger,
                             d_pack_text_from_cstr("x"));
    tagger.next = 9999u;
    (void)d_imap_tagger_next(&tagger,
                             &tag);

    const bool four = (strcmp(tag.text,
                              "x9999") == 0);

    (void)d_imap_tagger_next(&tagger,
                             &tag);

    const bool five = (strcmp(tag.text,
                              "x10000") == 0);

    tagger.next = 4294967295u;
    (void)d_imap_tagger_next(&tagger,
                             &tag);

    const bool last = ( (strcmp(tag.text,
                                "x4294967295") == 0) &&
                        (tag.length == 11u) );

    (void)d_imap_tagger_next(&tagger,
                             &tag);

    const bool wrapped = (strcmp(tag.text,
                                 "x0001") == 0);

    result = d_tests_net_assert((four) && (five),
                                "d_imap_tagger_next",
                                "four digits, widening as needed",
                                _counter) && result;
    result = d_tests_net_assert((last) && (wrapped),
                                "d_imap_tagger_next",
                                "after 4294967295 the counter wraps to 1",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_tags_refusals
  Prefixes a tag may not hold, and NULL or corrupt arguments.
*/
static bool
d_tests_imap_tags_refusals(
    struct d_test_counter* _counter
)
{
    struct d_imap_tagger tagger   = { .next = 0u };
    struct d_imap_tag    tag      = { .length = 0u };
    const size_t         count    = sizeof(PREFIXES) / sizeof(PREFIXES[0]);
    bool                 prefixes = true;
    bool                 result   = true;

    // each prefix, taken or refused
    for (size_t i = 0u; i < count; ++i)
    {
        const enum d_imap_error error =
            d_imap_tagger_init(&tagger,
                               d_pack_text_from_cstr(PREFIXES[i].prefix));

        prefixes = ( ( (error == D_IMAP_OK) == PREFIXES[i].taken ) &&
                     (prefixes) );
    }

    (void)d_imap_tagger_init(&tagger,
                             d_pack_text_from_cstr("A"));

    const bool nulls =
        ( (d_imap_tagger_init(NULL,
                              d_pack_text_from_cstr("A")) ==
           D_IMAP_ERROR_INVALID_ARGUMENT)                    &&
          (d_imap_tagger_next(NULL,
                              &tag) ==
           D_IMAP_ERROR_INVALID_ARGUMENT)                    &&
          (d_imap_tagger_next(&tagger,
                              NULL) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    tagger.prefix_length = D_IMAP_TAG_PREFIX_MAX + 1u;

    const bool corrupt = (d_imap_tagger_next(&tagger,
                                             &tag) ==
                          D_IMAP_ERROR_INVALID_ARGUMENT);

    result = d_tests_net_assert(prefixes,
                                "d_imap_tagger_init",
                                "takes ASTRING-CHARs but +, up to eight",
                                _counter) && result;
    result = d_tests_net_assert((nulls) && (corrupt),
                                "d_imap_tagger_next",
                                "refuses NULL and a corrupt tagger",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_tags
  Tests the following:
  - an empty prefix is "A", and tags count from 0001
  - the counter widens past 9999 and wraps after 4294967295
  - prefixes a tag may not hold, NULL and corrupt arguments are refused
  - a tag's text is a view of it, and a tag matches only its exact echo
*/
bool
d_tests_sa_imap_tags(
    struct d_test_counter* _counter
)
{
    struct d_imap_tagger tagger = { .next = 0u };
    struct d_imap_tag    first  = { .length = 0u };
    struct d_imap_tag    second = { .length = 0u };
    bool                 result = true;

    const bool issued =
        ( (d_imap_tagger_init(&tagger,
                              d_pack_text_from_cstr("")) == D_IMAP_OK) &&
          (d_imap_tagger_next(&tagger,
                              &first) == D_IMAP_OK)                    &&
          (d_imap_tagger_next(&tagger,
                              &second) == D_IMAP_OK)                   &&
          (strcmp(first.text,
                  "A0001") == 0)                                       &&
          (first.length == 5u)                                         &&
          (strcmp(second.text,
                  "A0002") == 0) );
    const bool viewed =
        ( (d_tests_imap_span_is(d_imap_tag_text(&first),
                                "A0001"))               &&
          (d_imap_tag_text(NULL).length == 0u) );
    const bool matched =
        ( (d_imap_tag_matches(&first,
                              d_pack_text_from_cstr("A0001")))   &&
          (!d_imap_tag_matches(&first,
                               d_pack_text_from_cstr("a0001")))  &&
          (!d_imap_tag_matches(&first,
                               d_pack_text_from_cstr("A00011"))) &&
          (!d_imap_tag_matches(&first,
                               d_pack_text_from_cstr("")))       &&
          (!d_imap_tag_matches(NULL,
                               d_pack_text_from_cstr("A0001"))) );

    result = d_tests_net_assert(issued,
                                "d_imap_tagger_next",
                                "an empty prefix issues A0001, then A0002",
                                _counter) && result;
    result = d_tests_net_assert(viewed,
                                "d_imap_tag_text",
                                "views the tag, and a NULL tag as empty",
                                _counter) && result;
    result = d_tests_net_assert(matched,
                                "d_imap_tag_matches",
                                "only the tag's exact echo, case and all",
                                _counter) && result;
    result = d_tests_imap_tags_counter(_counter) && result;
    result = d_tests_imap_tags_refusals(_counter) && result;

    return result;
}

/*
d_tests_imap_options_row
  The block a row describes, validated.
*/
static enum d_imap_error
d_tests_imap_options_row(
    const struct d_tests_imap_options_case* _row
)
{
    struct d_imap_options options = { .literal_limit = 0u };

    d_imap_options_init(&options);
    options.security        = _row->security;
    options.port            = _row->port;
    options.auth_mechanisms = _row->mechanisms;
    options.flags           = _row->flags;
    options.host            = d_pack_text_from_cstr(_row->host);
    options.username        = d_pack_text_from_cstr(_row->username);
    options.password        = d_pack_text_from_cstr(_row->password);
    options.token           = d_pack_text_from_cstr(_row->token);

    return d_imap_options_validate(&options);
}

/*
d_tests_imap_options_credentials
  A NUL in any of the four credentials, and a NULL block, are refused.
*/
static bool
d_tests_imap_options_credentials(
    struct d_test_counter* _counter
)
{
    struct d_imap_options    options = { .literal_limit = 0u };
    const struct d_pack_text nul     = d_pack_text_from_span("a\0b",
                                                             3u);
    bool                     refused = true;

    // each credential in turn holds the NUL
    for (int which = 0; which < 4; ++which)
    {
        d_imap_options_init(&options);
        options.host     = d_pack_text_from_cstr(HOST);
        options.username = (which == 0) ? nul : d_pack_text_from_cstr("fred");
        options.password = (which == 1) ? nul : d_pack_text_from_cstr("pw");
        options.token    = (which == 2) ? nul : d_pack_text_from_cstr("");
        options.authzid  = (which == 3) ? nul : d_pack_text_from_cstr("");
        refused = ( (d_imap_options_validate(&options) ==
                     D_IMAP_ERROR_INVALID_ARGUMENT) &&
                    (refused) );
    }

    refused = ( (d_imap_options_validate(NULL) ==
                 D_IMAP_ERROR_INVALID_ARGUMENT) &&
                (refused) );

    return d_tests_net_assert(refused,
                              "d_imap_options_validate",
                              "no NUL in a credential, and no NULL block",
                              _counter);
}

/*
d_tests_imap_options_ports
  The port to connect to, explicit or the mode's default, and 0 for what is
out of range; and the modes' default ports.
*/
static bool
d_tests_imap_options_ports(
    struct d_test_counter* _counter
)
{
    struct d_imap_options options = { .literal_limit = 0u };
    bool                  result  = true;

    d_imap_options_init(&options);

    const bool tls = (d_imap_options_port(&options) == D_IMAP_PORT_TLS);

    options.security = D_IMAP_SECURITY_STARTTLS;

    const bool plain = (d_imap_options_port(&options) == D_IMAP_PORT);

    options.port = 1993u;

    const bool explicit_port = (d_imap_options_port(&options) == 1993u);

    options.port = 65536u;

    const bool high = (d_imap_options_port(&options) == 0u);

    options.port     = 0u;
    options.security = D_IMAP_SECURITY_COUNT;

    const bool unknown = ( (d_imap_options_port(&options) == 0u) &&
                           (d_imap_options_port(NULL) == 0u) );
    const bool defaults =
        ( (d_imap_security_default_port(D_IMAP_SECURITY_TLS) == 993u)      &&
          (d_imap_security_default_port(D_IMAP_SECURITY_STARTTLS) == 143u) &&
          (d_imap_security_default_port(
               D_IMAP_SECURITY_OPPORTUNISTIC) == 143u)                     &&
          (d_imap_security_default_port(D_IMAP_SECURITY_NONE) == 143u)     &&
          (d_imap_security_default_port(
               (enum d_imap_security)D_IMAP_SECURITY_COUNT) == 0u) );

    result = d_tests_net_assert((tls) && (plain) && (explicit_port),
                                "d_imap_options_port",
                                "an explicit port, else the mode's default",
                                _counter) && result;
    result = d_tests_net_assert((high) && (unknown),
                                "d_imap_options_port",
                                "0 for a port or mode out of range",
                                _counter) && result;
    result = d_tests_net_assert(defaults,
                                "d_imap_security_default_port",
                                "993 for implicit TLS, 143 otherwise",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_options
  Tests the following:
  - init zeroes every field, the secure defaults, and ignores NULL
  - validate's every check, one change at a time
  - a NUL in a credential, and a NULL block, are refused
  - the port resolves to the explicit one or the mode's default
*/
bool
d_tests_sa_imap_options(
    struct d_test_counter* _counter
)
{
    struct d_imap_options options = { .literal_limit = 7u,
                                      .port          = 7u,
                                      .security      = 2,
                                      .flags         = 7u };
    const size_t          count   = sizeof(OPTIONS) / sizeof(OPTIONS[0]);
    bool                  result  = true;

    d_imap_options_init(NULL);
    d_imap_options_init(&options);

    const bool zeroed = ( (options.literal_limit == 0u) &&
                          (options.port == 0u)          &&
                          (options.security == 0)       &&
                          (options.flags == 0u)         &&
                          (options.host.length == 0u) );

    result = d_tests_net_assert(zeroed,
                                "d_imap_options_init",
                                "zeroes every field, and ignores NULL",
                                _counter) && result;

    // each change, validated
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert((d_tests_imap_options_row(&OPTIONS[i]) ==
                                     OPTIONS[i].error),
                                    "d_imap_options_validate",
                                    OPTIONS[i].what,
                                    _counter) && result;
    }

    result = d_tests_imap_options_credentials(_counter) && result;
    result = d_tests_imap_options_ports(_counter) && result;

    return result;
}

/*
d_tests_imap_auth_row
  A row's choice: its capabilities parsed and its options built, then the
mechanism chosen compared with the row's.
*/
static bool
d_tests_imap_auth_row(
    const struct d_tests_imap_auth_case* _row
)
{
    struct d_imap_options      options      = { .literal_limit = 0u };
    struct d_imap_capabilities capabilities = { .set = 0u };
    enum d_imap_auth_mechanism chosen       = D_IMAP_AUTH_LOGIN_COMMAND;

    d_imap_options_init(&options);
    options.host            = d_pack_text_from_cstr(HOST);
    options.auth_mechanisms = _row->permitted;
    options.flags           = _row->flags;
    options.username        = d_pack_text_from_cstr(_row->username);
    options.password        = d_pack_text_from_cstr(_row->password);
    options.token           = d_pack_text_from_cstr(_row->token);

    const bool parsed =
        (d_imap_capabilities_parse(d_pack_text_from_cstr(_row->capabilities),
                                   &capabilities) == D_IMAP_OK);
    const enum d_imap_error error =
        d_imap_auth_select(&options,
                           capabilities,
                           _row->supported,
                           _row->secure,
                           &chosen);

    return ( (parsed)               &&
             (error == _row->error) &&
             ( (_row->chosen < 0) ||
               ((int)chosen == _row->chosen) ) );
}

/*
d_tests_sa_imap_auth_select
  Tests the following:
  - the strongest permitted, supported and advertised mechanism the
    credentials serve is chosen, in imap.h's order
  - channel binding needs TLS, and so do the mechanisms that expose a
    password or token, unless the options allow cleartext
  - LOGINDISABLED rules out the LOGIN command
  - GSSAPI, EXTERNAL, ANONYMOUS and the weak mechanisms need naming
  - NULL arguments are refused
*/
bool
d_tests_sa_imap_auth_select(
    struct d_test_counter* _counter
)
{
    struct d_imap_options      options      = { .literal_limit = 0u };
    struct d_imap_capabilities capabilities = { .set = 0u };
    enum d_imap_auth_mechanism chosen       = D_IMAP_AUTH_LOGIN_COMMAND;
    const size_t               count        = sizeof(CHOICES) /
                                              sizeof(CHOICES[0]);
    bool                       result       = true;

    // each advertisement and set of credentials
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_auth_row(&CHOICES[i]),
                                    "d_imap_auth_select",
                                    CHOICES[i].what,
                                    _counter) && result;
    }

    d_imap_options_init(&options);

    const bool nulls =
        ( (d_imap_auth_select(NULL,
                              capabilities,
                              (uint32_t)EVERY_MECHANISM,
                              true,
                              &chosen) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_auth_select(&options,
                              capabilities,
                              (uint32_t)EVERY_MECHANISM,
                              true,
                              NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(nulls,
                                "d_imap_auth_select",
                                "refuses NULL options or out-pointer",
                                _counter) && result;

    return result;
}
