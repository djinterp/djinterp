/*******************************************************************************
* djinterp [c][net]                                          smtp_tests_common.c
*
* djinterp SMTP kernel tests - protocol vocabulary.
*   Tests of smtp_common: buffers, reply parsing and formatting,
* capability discovery, mailbox and domain validation, command and path
* parsing, base64 against the RFC 4648 vectors, the DATA transparency
* encoder fed in chunks of every size, and the mappings between SMTP's
* errors and net.h's and the TLS kernel's.
*
*
* path:      /tests/djinterp/c/modules/net/smtp/smtp_tests_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.31
*******************************************************************************/
#include "./smtp_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memcmp, memset, strcmp, strlen, strstr


// dj_tests_smtp_address_case
//   struct: one mailbox and whether it should validate.
struct dj_tests_smtp_address_case
{
    const char* address;
    bool        utf8;
    bool        valid;
};

// dj_tests_smtp_base64_case
//   struct: an RFC 4648 section 10 test vector.
struct dj_tests_smtp_base64_case
{
    const char* plain;
    const char* encoded;
};

// dj_tests_smtp_path_case
//   struct: a MAIL or RCPT argument and what parsing should find.
struct dj_tests_smtp_path_case
{
    const char* argument;
    const char* keyword;
    bool        parses;
    const char* address;
    const char* parameters;
};

// ADDRESS_CASES
//   constant: mailboxes, valid and invalid.
static const struct dj_tests_smtp_address_case ADDRESS_CASES[] =
{
    { "user@example.com",                    false, true  },
    { "first.last+tag@sub.example.co",       false, true  },
    { "\"quoted user\"@example.com",         false, true  },
    { "\"a>b\\\"c\"@example.com",            false, true  },
    { "user@[192.168.0.1]",                  false, true  },
    { "user@[IPv6:2001:db8::1]",             false, true  },
    { "j\xc3\xbcrgen@m\xc3\xbcnchen.de",     true,  true  },
    { "j\xc3\xbcrgen@m\xc3\xbcnchen.de",     false, false },
    { "\xc0\xaf@example.com",                true,  false },
    { "",                                    false, false },
    { "no-at-sign",                          false, false },
    { "a..b@example.com",                    false, false },
    { ".a@example.com",                      false, false },
    { "a.@example.com",                      false, false },
    { "a@-bad.example",                      false, false },
    { "a@bad-.example",                      false, false },
    { "a@exa mple.com",                      false, false },
    { "a@b.example\r\nRCPT TO:<x@y.example>", false, false },
    { "a@[300.1.1.1]",                       false, false },
    { "a@",                                  false, false },
    { "@example.com",                        false, false }
};

// BASE64_CASES
//   constant: the RFC 4648 section 10 vectors.
static const struct dj_tests_smtp_base64_case BASE64_CASES[] =
{
    { "",       ""         },
    { "f",      "Zg=="     },
    { "fo",     "Zm8="     },
    { "foo",    "Zm9v"     },
    { "foob",   "Zm9vYg==" },
    { "fooba",  "Zm9vYmE=" },
    { "foobar", "Zm9vYmFy" }
};

// BASE64_INVALID
//   constant: text a strict decoder must refuse.
static const char* const BASE64_INVALID[] =
{
    "Zm9",
    "Zm=v",
    "Z===",
    "Zm9v!A==",
    "Zm 9v"
};

// PATH_CASES
//   constant: path arguments and their parses.
static const struct dj_tests_smtp_path_case PATH_CASES[] =
{
    { "FROM:<a@b.example> SIZE=100 BODY=8BITMIME", "FROM:", true,
      "a@b.example", "SIZE=100 BODY=8BITMIME" },
    { "from:<>", "FROM:", true, "", NULL },
    { "TO: <x@y.example>", "TO:", true, "x@y.example", NULL },
    { "TO:<@relay.example,@hop.example:u@z.example>", "TO:", true,
      "u@z.example", NULL },
    { "TO:<\"a>b\"@c.example>", "TO:", true, "\"a>b\"@c.example", NULL },
    { "TO:x@y.example", "TO:", false, NULL, NULL },
    { "TO:<x@y.example", "TO:", false, NULL, NULL },
    { "TO:<x@y.example>junk", "TO:", false, NULL, NULL },
    { "FROM:<x@y.example>", "TO:", false, NULL, NULL }
};

/*
d_tests_smtp_feed
  Feeds a CRLF-separated reply into a dj_smtp_reply one line at a time.
*/
static enum dj_smtp_error
d_tests_smtp_feed(
    struct dj_smtp_reply* _reply,
    const char*          _text
)
{
    const char*       cursor = _text;
    enum dj_smtp_error result = D_SMTP_OK;

    d_smtp_reply_clear(_reply);

    while ( (result == D_SMTP_OK) &&
            (*cursor != '\0') )
    {
        const char*  end    = strstr(cursor,
                                     "\r\n");
        const size_t length = (end) ? (size_t)(end - cursor) : strlen(cursor);

        result  = d_smtp_reply_feed(_reply,
                                    cursor,
                                    length);
        cursor += length + ((end) ? 2u : 0u);
    }

    return result;
}

/*
d_tests_smtp_span_is
  Whether a counted span equals a C string (NULL matching NULL).
*/
static
bool d_tests_smtp_span_is(
    const char* _span,
    size_t      _length,
    const char* _text
)
{
    if ( (!_span) ||
         (!_text) )
    {
        return (_span == _text);
    }

    return ( (strlen(_text) == _length) &&
             (memcmp(_span,
                     _text,
                     _length) == 0) );
}

/*
d_tests_smtp_buffer
  Tests the following:
  - a fresh buffer is an empty C string
  - text and numbers append, keeping the NUL
  - an append that does not fit leaves the buffer unchanged
  - a NULL buffer is rejected
*/
bool d_tests_smtp_buffer(
    struct dj_test_suite* _suite
)
{
    const char* const    TEST       = "dj_smtp_buffer";
    char                 storage[8] = "xxxxxxx";
    struct dj_smtp_buffer buffer     = { .data = NULL };
    bool                 result     = true;

    d_smtp_buffer_init(&buffer,
                       storage,
                       sizeof(storage));
    result = D_TESTS_NET_ASSERT( ( (buffer.length == 0) &&
                                   (storage[0] == '\0') ),
                                 TEST,
                                 "a fresh buffer is an empty string",
                                 _suite) && result;

    const bool text = ( (d_smtp_buffer_append_text(&buffer,
                                                   "abc") ==
                         D_SMTP_OK) &&
                        (d_smtp_buffer_append_number(&buffer,
                                                     1234u) ==
                         D_SMTP_OK) &&
                        (strcmp(storage,
                                "abc1234") == 0) );

    result = D_TESTS_NET_ASSERT(text,
                                TEST,
                                "text and numbers append up to capacity - 1",
                                _suite) && result;

    const bool full = ( (d_smtp_buffer_append_text(&buffer,
                                                   "y") ==
                         D_SMTP_ERROR_LINE_TOO_LONG) &&
                        (buffer.length == 7u)        &&
                        (strcmp(storage,
                                "abc1234") == 0) );

    result = D_TESTS_NET_ASSERT(full,
                                TEST,
                                "an append that does not fit changes nothing",
                                _suite) && result;
    result = D_TESTS_NET_ASSERT( (d_smtp_buffer_append(NULL,
                                                       "x",
                                                       1) ==
                                  D_SMTP_ERROR_INVALID_ARGUMENT),
                                 TEST,
                                 "a NULL buffer is rejected",
                                 _suite) && result;

    return result;
}

/*
d_tests_smtp_reply
  Tests the following:
  - multiline replies: code, line count, text, completion
  - enhanced status codes, and the class check that rejects false ones
  - code mismatches and malformed lines are protocol errors
  - reply formatting with hyphens, status, and rejection of CR
  - classification by first digit
  It runs past 75 lines because one reply and its output buffer serve every
check.
*/
bool d_tests_smtp_reply(
    struct dj_test_suite* _suite
)
{
    const char* const    TEST         = "dj_smtp_reply";
    struct dj_smtp_reply  reply        = { 0 };
    char                 storage[256] = { 0 };
    struct dj_smtp_buffer out          = { .data     = storage,
                                           .capacity = sizeof(storage),
                                           .length   = 0 };
    const struct dj_smtp_status status = { 5u, 1u, 1u };
    size_t                     length = 0;
    bool                       result = true;

    const enum dj_smtp_error fed    = d_tests_smtp_feed(&reply,
                                                       "250-first\r\n"
                                                       "250-second\r\n"
                                                       "250 third");
    const char*             second = d_smtp_reply_line(&reply,
                                                       1,
                                                       &length);
    const bool multiline =
        ( (fed == D_SMTP_OK)                    &&
          (reply.code == 250u)                  &&
          (reply.line_count == 3u)              &&
          (reply.complete)                      &&
          (strcmp(reply.text,
                  "first\nsecond\nthird") == 0) &&
          (d_tests_smtp_span_is(second,
                                length,
                                "second"))      &&
          (d_smtp_reply_line(&reply,
                             3,
                             &length) == NULL) );

    result = D_TESTS_NET_ASSERT(multiline,
                                TEST,
                                "a multiline reply joins its lines",
                                _suite) && result;

    const bool enhanced =
        ( (d_tests_smtp_feed(&reply,
                             "550 5.1.1 User unknown") == D_SMTP_OK) &&
          (reply.status.status_class == 5u)                          &&
          (reply.status.subject == 1u)                               &&
          (reply.status.detail == 1u)                                &&
          (d_tests_smtp_feed(&reply,
                             "250 5.1.1 not a status") ==
           D_SMTP_OK)                                                &&
          (reply.status.status_class == 0u) );

    result = D_TESTS_NET_ASSERT(enhanced,
                                TEST,
                                "an enhanced status must match the code",
                                _suite) && result;

    const bool broken =
        ( (d_tests_smtp_feed(&reply,
                             "250-a\r\n251 b") ==
           D_SMTP_ERROR_PROTOCOL)                               &&
          (d_tests_smtp_feed(&reply,
                             "25") == D_SMTP_ERROR_PROTOCOL)    &&
          (d_tests_smtp_feed(&reply,
                             "150 x") == D_SMTP_ERROR_PROTOCOL) &&
          (d_tests_smtp_feed(&reply,
                             "250x") == D_SMTP_ERROR_PROTOCOL) );

    result = D_TESTS_NET_ASSERT(broken,
                                TEST,
                                "malformed and inconsistent lines are refused",
                                _suite) && result;

    const bool formatted =
        ( (d_smtp_reply_format(&out,
                               250u,
                               NULL,
                               "a\nb") == D_SMTP_OK)          &&
          (d_smtp_reply_format(&out,
                               550u,
                               &status,
                               "No") == D_SMTP_OK)            &&
          (strcmp(storage,
                  "250-a\r\n250 b\r\n550 5.1.1 No\r\n") == 0) &&
          (d_smtp_reply_format(&out,
                               250u,
                               NULL,
                               "a\rb") ==
           D_SMTP_ERROR_INVALID_ARGUMENT)                     &&
          (d_smtp_reply_format(&out,
                               199u,
                               NULL,
                               "x") ==
           D_SMTP_ERROR_INVALID_ARGUMENT) );

    result = D_TESTS_NET_ASSERT(formatted,
                                TEST,
                                "replies format with hyphens and status",
                                _suite) && result;

    const bool classes = ( (d_smtp_code_is_positive(250u))     &&
                           (d_smtp_code_is_intermediate(354u)) &&
                           (d_smtp_code_is_transient(451u))    &&
                           (d_smtp_code_is_permanent(550u))    &&
                           (!d_smtp_code_is_positive(354u)) );

    result = D_TESTS_NET_ASSERT(classes,
                                TEST,
                                "codes classify by their first digit",
                                _suite) && result;

    return result;
}

/*
d_tests_smtp_capabilities
  Tests the following:
  - every recognized EHLO keyword sets its bit; unknown ones are ignored
  - SIZE's value and both AUTH syntaxes are read
  - mechanism names round-trip
  - only a complete 250 reply parses
*/
bool d_tests_smtp_capabilities(
    struct dj_test_suite* _suite
)
{
    const char* const          TEST         = "dj_smtp_capabilities";
    struct dj_smtp_reply        reply        = { 0 };
    struct dj_smtp_capabilities capabilities = { .extensions = 0u };
    bool                       result       = true;

    (void)d_tests_smtp_feed(&reply,
                            "250-mail.example.com greets you\r\n"
                            "250-PIPELINING\r\n"
                            "250-SIZE 35882577\r\n"
                            "250-8bitmime\r\n"
                            "250-AUTH PLAIN LOGIN XOAUTH2\r\n"
                            "250-AUTH=CRAM-MD5\r\n"
                            "250-STARTTLS\r\n"
                            "250-X-UNKNOWN-EXTENSION\r\n"
                            "250-ENHANCEDSTATUSCODES\r\n"
                            "250 SMTPUTF8");

    const unsigned int extensions = D_SMTP_EXTENSION_PIPELINING          |
                                    D_SMTP_EXTENSION_SIZE                |
                                    D_SMTP_EXTENSION_8BITMIME            |
                                    D_SMTP_EXTENSION_AUTH                |
                                    D_SMTP_EXTENSION_STARTTLS            |
                                    D_SMTP_EXTENSION_ENHANCEDSTATUSCODES |
                                    D_SMTP_EXTENSION_SMTPUTF8;
    const unsigned int mechanisms = D_SMTP_AUTH_PLAIN    |
                                    D_SMTP_AUTH_LOGIN    |
                                    D_SMTP_AUTH_XOAUTH2  |
                                    D_SMTP_AUTH_CRAM_MD5;
    const bool parsed = ( (d_smtp_capabilities_parse(&capabilities,
                                                     &reply) == D_SMTP_OK) &&
                          (capabilities.extensions == extensions)          &&
                          (capabilities.auth == mechanisms)                &&
                          (capabilities.size_limit == 35882577u)           &&
                          (capabilities.extended) );

    result = D_TESTS_NET_ASSERT(parsed,
                                TEST,
                                "EHLO keywords, SIZE and AUTH are read",
                                _suite) && result;

    const bool names = ( (strcmp(d_smtp_auth_name(D_SMTP_AUTH_CRAM_MD5),
                                 "CRAM-MD5") == 0) &&
                         (d_smtp_auth_from_name("xoauth2",
                                                7) ==
                          D_SMTP_AUTH_XOAUTH2)     &&
                         (d_smtp_auth_from_name("GSSAPI",
                                                6) ==
                          D_SMTP_AUTH_NONE)        &&
                         (d_smtp_auth_name(D_SMTP_AUTH_NONE) == NULL) );

    result = D_TESTS_NET_ASSERT(names,
                                TEST,
                                "mechanism names round-trip",
                                _suite) && result;

    (void)d_tests_smtp_feed(&reply,
                            "500 5.5.2 what");
    result = D_TESTS_NET_ASSERT( (d_smtp_capabilities_parse(&capabilities,
                                                            &reply) ==
                                  D_SMTP_ERROR_INVALID_ARGUMENT),
                                 TEST,
                                 "only a 250 reply parses",
                                 _suite) && result;

    return result;
}

/*
d_tests_smtp_validation
  Tests the following:
  - the ADDRESS_CASES table, ASCII and SMTPUTF8
  - the 64-byte local-part limit
  - address literals as EHLO domains
*/
bool d_tests_smtp_validation(
    struct dj_test_suite* _suite
)
{
    const char* const TEST  = "d_smtp_*_is_valid";
    const size_t      count = sizeof(ADDRESS_CASES) /
                                   sizeof(ADDRESS_CASES[0]);
    char local[128] = { 0 };
    bool result     = true;

    for (size_t i = 0; i < count; ++i)
    {
        const struct dj_tests_smtp_address_case* test  = &ADDRESS_CASES[i];
        const bool                              valid = d_smtp_address_is_valid(test->address,
                                                   strlen(test->address),
                                                   test->utf8);

        result = D_TESTS_NET_ASSERT( (valid == test->valid),
                                     TEST,
                                     test->address,
                                     _suite) && result;
    }

    memset(local,
           'a',
           64);
    memcpy(local + 64,
           "@example.com",
           13);

    const bool at_limit = d_smtp_address_is_valid(local,
                                                  strlen(local),
                                                  false);

    memset(local,
           'a',
           65);
    memcpy(local + 65,
           "@example.com",
           13);

    const bool over_limit = d_smtp_address_is_valid(local,
                                                    strlen(local),
                                                    false);

    result = D_TESTS_NET_ASSERT( ( (at_limit) &&
                                   (!over_limit) ),
                                 TEST,
                                 "a local-part may be 64 bytes, not 65",
                                 _suite) && result;

    const bool domains = ( (d_smtp_domain_is_valid("[127.0.0.1]",
                                                   11,
                                                   false)) &&
                           (d_smtp_domain_is_valid("localhost",
                                                   9,
                                                   false)) &&
                           (!d_smtp_domain_is_valid("a..b",
                                                    4,
                                                    false)) );

    result = D_TESTS_NET_ASSERT(domains,
                                TEST,
                                "EHLO domains and address literals",
                                _suite) && result;

    return result;
}

/*
d_tests_smtp_commands
  Tests the following:
  - verbs are matched without regard to case; unknown ones are reported
  - arguments are trimmed
  - the PATH_CASES table: null paths, tolerated spaces, source routes,
    quoted '>' and malformed paths
  - ESMTP parameters iterate with and without values
  It runs past 75 lines because one command and its parameters serve every
check.
*/
bool d_tests_smtp_commands(
    struct dj_test_suite* _suite
)
{
    const char* const TEST  = "d_smtp_command_parse";
    const size_t      count = sizeof(PATH_CASES) /
                                        sizeof(PATH_CASES[0]);
    struct dj_smtp_command   command   = { .verb = D_SMTP_VERB_UNKNOWN };
    struct dj_smtp_parameter parameter = { .keyword = NULL };
    bool                    result    = true;

    const bool verbs = ( (d_smtp_command_parse("ehlo  host.example  ",
                                               20,
                                               &command) == D_SMTP_OK) &&
                         (command.verb == D_SMTP_VERB_EHLO)            &&
                         (d_tests_smtp_span_is(command.argument,
                                               command.argument_length,
                                               "host.example"))        &&
                         (d_smtp_command_parse("XYZZY",
                                               5,
                                               &command) == D_SMTP_OK) &&
                         (command.verb == D_SMTP_VERB_UNKNOWN) );

    result = D_TESTS_NET_ASSERT(verbs,
                                TEST,
                                "verbs ignore case and arguments are trimmed",
                                _suite) && result;

    for (size_t i = 0; i < count; ++i)
    {
        const struct dj_tests_smtp_path_case* test   = &PATH_CASES[i];
        struct dj_smtp_path                   path   = { .address = NULL };
        const bool                           parsed = (d_smtp_path_parse(test->argument,
                                                       strlen(test->argument),
                                                       test->keyword,
                                                       &path) == D_SMTP_OK);
        const bool         agrees = ( (parsed == test->parses) &&
                                      ( (!parsed) ||
                                        ( (d_tests_smtp_span_is(
                                               path.address,
                                               path.address_length,
                                               test->address)) &&
                                          (d_tests_smtp_span_is(
                                               path.parameters,
                                               path.parameters_length,
                                               test->parameters)) ) ) );

        result = D_TESTS_NET_ASSERT(agrees,
                                    TEST,
                                    test->argument,
                                    _suite) && result;
    }

    const char* cursor = "SIZE=100  SMTPUTF8";
    const char* end    = cursor + strlen(cursor);
    const bool  first  = ( (d_smtp_parameter_next(&cursor,
                                                  end,
                                                  &parameter)) &&
                           (d_tests_smtp_span_is(parameter.keyword,
                                                 parameter.keyword_length,
                                                 "SIZE"))      &&
                           (d_tests_smtp_span_is(parameter.value,
                                                 parameter.value_length,
                                                 "100")) );
    const bool  second = ( (d_smtp_parameter_next(&cursor,
                                                  end,
                                                  &parameter)) &&
                           (d_tests_smtp_span_is(parameter.keyword,
                                                 parameter.keyword_length,
                                                 "SMTPUTF8"))  &&
                           (parameter.value == NULL)           &&
                           (!d_smtp_parameter_next(&cursor,
                                                   end,
                                                   &parameter)) );

    result = D_TESTS_NET_ASSERT( ( (first) &&
                                   (second) ),
                                 TEST,
                                 "parameters iterate with and without values",
                                 _suite) && result;

    return result;
}

/*
d_tests_smtp_base64
  Tests the following:
  - the RFC 4648 vectors append, encoded and decoded
  - binary data with NULs round-trips
  - strict decoding refuses bad length, padding and alphabet
  - an encoding, or a decoding, that does not fit is refused, and the buffer
    is unchanged
  It runs past 75 lines because its case tables share one output buffer.
*/
bool d_tests_smtp_base64(
    struct dj_test_suite* _suite
)
{
    const char* const TEST   = "dj_smtp_buffer (base64)";
    const size_t      count  = sizeof(BASE64_CASES) / sizeof(BASE64_CASES[0]);
    const size_t      broken = sizeof(BASE64_INVALID) /
                               sizeof(BASE64_INVALID[0]);
    char storage[64] = { 0 };
    bool result      = true;

    for (size_t i = 0; i < count; ++i)
    {
        const struct dj_tests_smtp_base64_case* test    = &BASE64_CASES[i];
        struct dj_smtp_buffer                   encoded = { .data     = storage,
                                         .capacity = sizeof(storage),
                                         .length   = 0 };
        const bool encodes = ( (d_smtp_buffer_append_base64(
                                    &encoded,
                                    test->plain,
                                    strlen(test->plain)) == D_SMTP_OK) &&
                               (strcmp(storage,
                                       test->encoded) == 0) );
        struct dj_smtp_buffer decoded = { .data     = storage,
                                         .capacity = sizeof(storage),
                                         .length   = 0 };
        const bool decodes = ( (d_smtp_buffer_append_from_base64(
                                    &decoded,
                                    test->encoded,
                                    strlen(test->encoded)) == D_SMTP_OK) &&
                               (strcmp(storage,
                                       test->plain) == 0) );

        result = D_TESTS_NET_ASSERT( ( (encodes) &&
                                       (decodes) ),
                                     TEST,
                                     test->encoded,
                                     _suite) && result;
    }

    struct dj_smtp_buffer binary = { .data     = storage,
                                    .capacity = sizeof(storage),
                                    .length   = 0 };
    const bool nul = ( (d_smtp_buffer_append_base64(&binary,
                                                    "\0user\0pass",
                                                    10) == D_SMTP_OK) &&
                       (strcmp(storage,
                               "AHVzZXIAcGFzcw==") == 0) );

    result = D_TESTS_NET_ASSERT(nul,
                                TEST,
                                "NUL bytes encode like any other",
                                _suite) && result;

    for (size_t i = 0; i < broken; ++i)
    {
        struct dj_smtp_buffer decoded = { .data     = storage,
                                         .capacity = sizeof(storage),
                                         .length   = 0 };

        result = D_TESTS_NET_ASSERT( (d_smtp_buffer_append_from_base64(
                                          &decoded,
                                          BASE64_INVALID[i],
                                          strlen(BASE64_INVALID[i])) ==
                                      D_SMTP_ERROR_PROTOCOL),
                                     TEST,
                                     BASE64_INVALID[i],
                                     _suite) && result;
    }

    (void)memcpy(storage,
                 "ab",
                 3);

    struct dj_smtp_buffer small = { .data     = storage,
                                   .capacity = 6u,
                                   .length   = 2u };

    result = D_TESTS_NET_ASSERT(
                 ( (d_smtp_buffer_append_base64(&small,
                                                "foo",
                                                3) ==
                    D_SMTP_ERROR_LINE_TOO_LONG) &&
                   (d_smtp_buffer_append_from_base64(&small,
                                                     "Zm9vYg==",
                                                     8) ==
                    D_SMTP_ERROR_LINE_TOO_LONG) &&
                   (small.length == 2u)         &&
                   (strcmp(storage,
                           "ab") == 0) ),
                 TEST,
                 "what does not fit is refused, the buffer unchanged",
                 _suite) && result;

    return result;
}

/*
d_tests_smtp_encode_all
  Runs the DATA encoder over `_input` with output buffers of `_capacity`
bytes, concatenating everything into `_out`, then finishes the message.
*/
static
void d_tests_smtp_encode_all(
    const char*            _input,
    size_t                 _capacity,
    struct dj_smtp_buffer* _out
)
{
    struct dj_smtp_data_encoder encoder  = { .line_start = true,
                                            .after_cr   = false };
    const size_t length            = strlen(_input);
    size_t       consumed          = 0;
    char         chunk_storage[64] = { 0 };

    while (consumed < length)
    {
        struct dj_smtp_buffer chunk = { .data     = chunk_storage,
                                       .capacity = _capacity,
                                       .length   = 0 };

        consumed += d_smtp_data_encode(&encoder,
                                       _input + consumed,
                                       length - consumed,
                                       &chunk);
        (void)d_smtp_buffer_append(_out,
                                   chunk.data,
                                   chunk.length);
    }

    struct dj_smtp_buffer tail = { .data     = chunk_storage,
                                  .capacity = sizeof(chunk_storage),
                                  .length   = 0 };

    (void)d_smtp_data_finish(&encoder,
                             &tail);
    (void)d_smtp_buffer_append(_out,
                               tail.data,
                               tail.length);

    return;
}

/*
d_tests_smtp_data
  Tests the following:
  - LF, CR and CRLF all become CRLF, and leading dots are doubled
  - the output is identical for every chunk size
  - an unterminated last line is ended before the marker
  - unstuffing finds the marker and strips one transparency dot
*/
bool d_tests_smtp_data(
    struct dj_test_suite* _suite
)
{
    const char* const TEST     = "d_smtp_data_encode";
    const char* const INPUT    = ".a\nb\r\nc\rd\r\r\n..e";
    const char* const EXPECTED = "..a\r\nb\r\nc\r\nd\r\n\r\n...e\r\n.\r\n";
    bool              result   = true;

    for (size_t capacity = 3; capacity <= 64; capacity *= 2u)
    {
        char                 storage[128] = { 0 };
        struct dj_smtp_buffer out          = { .data     = storage,
                                     .capacity = sizeof(storage),
                                     .length   = 0 };

        d_tests_smtp_encode_all(INPUT,
                                capacity,
                                &out);
        result = D_TESTS_NET_ASSERT( (strcmp(storage,
                                             EXPECTED) == 0),
                                     TEST,
                                     "every chunk size encodes identically",
                                     _suite) && result;
    }

    char                 storage[32] = { 0 };
    struct dj_smtp_buffer out         = { .data     = storage,
                                 .capacity = sizeof(storage),
                                 .length   = 0 };

    d_tests_smtp_encode_all("x\n",
                            8u,
                            &out);
    result = D_TESTS_NET_ASSERT( (strcmp(storage,
                                         "x\r\n.\r\n") == 0),
                                 TEST,
                                 "a terminated last line gets only the marker",
                                 _suite) && result;

    const char* payload = NULL;
    size_t      size    = 0;
    const bool  unstuff = ( (d_smtp_data_unstuff(".",
                                                 1,
                                                 &payload,
                                                 &size))  &&
                            (!d_smtp_data_unstuff("..x",
                                                  3,
                                                  &payload,
                                                  &size)) &&
                            (d_tests_smtp_span_is(payload,
                                                  size,
                                                  ".x"))  &&
                            (!d_smtp_data_unstuff("y",
                                                  1,
                                                  &payload,
                                                  &size)) &&
                            (d_tests_smtp_span_is(payload,
                                                  size,
                                                  "y")) );

    result = D_TESTS_NET_ASSERT(unstuff,
                                TEST,
                                "unstuffing finds the marker and one dot",
                                _suite) && result;

    return result;
}

// dj_tests_smtp_mapping
//   struct: an error, and the dj_net_error it maps to.
struct dj_tests_smtp_mapping
{
    enum dj_smtp_error error;
    enum dj_net_error  net;
};

/*
d_tests_smtp_errors_to_net
  Tests the following:
  - each error maps onto the dj_net_error net.h's rules give it
  - a value outside the enum maps to D_NET_ERROR_UNKNOWN
*/
bool d_tests_smtp_errors_to_net(
    struct dj_test_suite* _suite
)
{
    const struct dj_tests_smtp_mapping MAP[] =
    {
        { D_SMTP_OK,                     D_NET_OK                       },
        { D_SMTP_ERROR_INVALID_ARGUMENT, D_NET_ERROR_INVALID_ARGUMENT   },
        { D_SMTP_ERROR_STATE,            D_NET_ERROR_INVALID_ARGUMENT   },
        { D_SMTP_ERROR_OUT_OF_MEMORY,    D_NET_ERROR_OUT_OF_MEMORY      },
        { D_SMTP_ERROR_RESOLVE,          D_NET_ERROR_HOST_NOT_FOUND     },
        { D_SMTP_ERROR_CONNECT,          D_NET_ERROR_CONNECTION_REFUSED },
        { D_SMTP_ERROR_TIMED_OUT,        D_NET_ERROR_TIMED_OUT          },
        { D_SMTP_ERROR_TRANSPORT,        D_NET_ERROR_UNKNOWN            },
        { D_SMTP_ERROR_CLOSED,           D_NET_ERROR_CLOSED             },
        { D_SMTP_ERROR_TLS,              D_NET_ERROR_PROTOCOL           },
        { D_SMTP_ERROR_PROTOCOL,         D_NET_ERROR_PROTOCOL           },
        { D_SMTP_ERROR_LINE_TOO_LONG,    D_NET_ERROR_MESSAGE_TOO_LARGE  },
        { D_SMTP_ERROR_REJECTED,         D_NET_ERROR_UNKNOWN            },
        { D_SMTP_ERROR_UNSUPPORTED,      D_NET_ERROR_UNKNOWN            },
        { D_SMTP_ERROR_AUTH,             D_NET_ERROR_ACCESS_DENIED      },
        { D_SMTP_ERROR_INSECURE,         D_NET_ERROR_ACCESS_DENIED      }
    };
    const size_t count  = sizeof(MAP) / sizeof(MAP[0]);
    bool         mapped = true;
    bool         result = true;

    // each error against the table
    for (size_t i = 0u; i < count; ++i)
    {
        mapped = ( (mapped) &&
                   (d_smtp_error_to_net(MAP[i].error) == MAP[i].net) );
    }

    const bool outside =
        (d_smtp_error_to_net((enum dj_smtp_error)99) ==
         D_NET_ERROR_UNKNOWN);

    result = D_TESTS_NET_ASSERT(mapped,
                                "d_smtp_error_to_net",
                                "each error maps by net.h's rules",
                                _suite) && result;
    result = D_TESTS_NET_ASSERT(outside,
                                "d_smtp_error_to_net",
                                "a value outside the enum is unknown",
                                _suite) && result;

    return result;
}

// dj_tests_smtp_from_net
//   struct: a dj_net_error, and the error it is here.
struct dj_tests_smtp_from_net
{
    enum dj_net_error  net;
    enum dj_smtp_error error;
};

/*
FROM_NET
  Every dj_net_error in the enum's order, placed by net.h's rules the other
way: the namesakes, the three ways a connection ends, and TRANSPORT.
*/
static const struct dj_tests_smtp_from_net FROM_NET[] =
{
    { D_NET_OK,                        D_SMTP_OK                     },
    { D_NET_ERROR_CLOSED,              D_SMTP_ERROR_CLOSED           },
    { D_NET_ERROR_WOULD_BLOCK,         D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_TIMED_OUT,           D_SMTP_ERROR_TIMED_OUT        },
    { D_NET_ERROR_INTERRUPTED,         D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_CONNECTION_RESET,    D_SMTP_ERROR_CLOSED           },
    { D_NET_ERROR_CONNECTION_REFUSED,  D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_CONNECTION_ABORTED,  D_SMTP_ERROR_CLOSED           },
    { D_NET_ERROR_ADDRESS_IN_USE,      D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_ADDRESS_INVALID,     D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_HOST_UNREACHABLE,    D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_NETWORK_DOWN,        D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_ACCESS_DENIED,       D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_INVALID_ARGUMENT,    D_SMTP_ERROR_INVALID_ARGUMENT },
    { D_NET_ERROR_MESSAGE_TOO_LARGE,   D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_TOO_MANY_OPEN_FILES, D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_OUT_OF_MEMORY,       D_SMTP_ERROR_OUT_OF_MEMORY    },
    { D_NET_ERROR_UNKNOWN,             D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_PROTOCOL,            D_SMTP_ERROR_PROTOCOL         },
    { D_NET_ERROR_UNSUPPORTED,         D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_ADDRESS_UNAVAILABLE, D_SMTP_ERROR_TRANSPORT        },
    { D_NET_ERROR_HOST_NOT_FOUND,      D_SMTP_ERROR_TRANSPORT        }
};

/*
d_tests_smtp_errors_from_net
  Tests the following:
  - every dj_net_error becomes the error net.h's rules give it
  - a value outside the enum is TRANSPORT
  - the namesakes come back from d_smtp_error_to_net unchanged
*/
bool d_tests_smtp_errors_from_net(
    struct dj_test_suite* _suite
)
{
    static const enum dj_smtp_error NAMESAKES[] =
    {
        D_SMTP_ERROR_INVALID_ARGUMENT,
        D_SMTP_ERROR_OUT_OF_MEMORY,
        D_SMTP_ERROR_TIMED_OUT,
        D_SMTP_ERROR_CLOSED,
        D_SMTP_ERROR_PROTOCOL
    };
    const size_t count  = sizeof(FROM_NET) / sizeof(FROM_NET[0]);
    bool         table  = (count == (size_t)D_NET_ERROR_HOST_NOT_FOUND + 1u);
    bool         back   = true;
    bool         result = true;

    // each row, in the enum's order
    for (size_t i = 0u; i < count; ++i)
    {
        table = ( (FROM_NET[i].net == (enum dj_net_error)i) &&
                  (d_smtp_error_from_net(FROM_NET[i].net) ==
                   FROM_NET[i].error)                      &&
                  (table) );
    }

    // each namesake, there and back
    for (size_t i = 0u; i < sizeof(NAMESAKES) / sizeof(NAMESAKES[0]); ++i)
    {
        back = ( (d_smtp_error_from_net(d_smtp_error_to_net(NAMESAKES[i])) ==
                  NAMESAKES[i]) &&
                 (back) );
    }

    result = D_TESTS_NET_ASSERT(table,
                                "d_smtp_error_from_net",
                                "places every dj_net_error by net.h's rules",
                                _suite) && result;
    result = D_TESTS_NET_ASSERT( (d_smtp_error_from_net(
                                      (enum dj_net_error)0x7FFF) ==
                                  D_SMTP_ERROR_TRANSPORT),
                                 "d_smtp_error_from_net",
                                 "a value outside the enum is TRANSPORT",
                                 _suite) && result;
    result = D_TESTS_NET_ASSERT(back,
                                "d_smtp_error_from_net",
                                "namesakes come back from to_net unchanged",
                                _suite) && result;

    return result;
}

// dj_tests_smtp_from_tls
//   struct: an TLS kernel status, what its connection last reported, and
// the error the two are here.
struct dj_tests_smtp_from_tls
{
    enum dj_tls_error  status;
    enum dj_net_error  cause;
    enum dj_smtp_error error;
};

/*
FROM_TLS
  Every dj_tls_error with no cause, then the transport failures with causes:
a connection's own failure is its error; a peer gone, with close_notify or
without, is CLOSED; a lack of memory is OUT_OF_MEMORY; the rest is TLS.
*/
static const struct dj_tests_smtp_from_tls FROM_TLS[] =
{
    { D_TLS_OK,                     D_NET_OK, D_SMTP_OK                  },
    { D_TLS_ERROR_INVALID_ARGUMENT, D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_MALFORMED,        D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_NOT_FOUND,        D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_UNSUPPORTED,      D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_CONFIG,           D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_STATE,            D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_NOT_TLS,          D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_HANDSHAKE,        D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_VERIFY,           D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_PROTOCOL,         D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_TRUNCATED,        D_NET_OK, D_SMTP_ERROR_CLOSED        },
    { D_TLS_ERROR_INSECURE,         D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_BUFFER_TOO_SMALL, D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_WOULD_BLOCK,      D_NET_OK, D_SMTP_ERROR_TRANSPORT     },
    { D_TLS_ERROR_TRANSPORT,        D_NET_OK, D_SMTP_ERROR_TRANSPORT     },
    { D_TLS_ERROR_CLOSED,           D_NET_OK, D_SMTP_ERROR_CLOSED        },
    { D_TLS_ERROR_WANT_READ,        D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_OUT_OF_MEMORY,    D_NET_OK, D_SMTP_ERROR_OUT_OF_MEMORY },
    { D_TLS_ERROR_BACKEND,          D_NET_OK, D_SMTP_ERROR_TLS           },
    { D_TLS_ERROR_TRANSPORT,        D_NET_ERROR_TIMED_OUT,
      D_SMTP_ERROR_TIMED_OUT                                             },
    { D_TLS_ERROR_TRANSPORT,        D_NET_ERROR_CONNECTION_RESET,
      D_SMTP_ERROR_CLOSED                                                },
    { D_TLS_ERROR_TRANSPORT,        D_NET_ERROR_UNKNOWN,
      D_SMTP_ERROR_TRANSPORT                                             },
    { D_TLS_ERROR_WOULD_BLOCK,      D_NET_ERROR_WOULD_BLOCK,
      D_SMTP_ERROR_TRANSPORT                                             },
    { D_TLS_ERROR_WOULD_BLOCK,      D_NET_ERROR_TIMED_OUT,
      D_SMTP_ERROR_TIMED_OUT                                             },
    { D_TLS_ERROR_TRUNCATED,        D_NET_ERROR_CONNECTION_RESET,
      D_SMTP_ERROR_CLOSED                                                },
    { D_TLS_ERROR_HANDSHAKE,        D_NET_ERROR_TIMED_OUT,
      D_SMTP_ERROR_TLS                                                   }
};

/*
d_tests_smtp_errors_from_tls
  Tests the following:
  - every dj_tls_error with no cause: OK, CLOSED for a peer gone, either
    way, OUT_OF_MEMORY, TRANSPORT for a transport failure that reported
    nothing, and TLS for the rest
  - a transport failure with a cause is that cause's error; a TLS failure
    is TLS whatever its connection last said
  - a value outside the enum is TLS
*/
bool d_tests_smtp_errors_from_tls(
    struct dj_test_suite* _suite
)
{
    bool table  = true;
    bool result = true;

    // each row
    for (size_t i = 0u; i < sizeof(FROM_TLS) / sizeof(FROM_TLS[0]); ++i)
    {
        table = ( (d_smtp_error_from_tls(FROM_TLS[i].status,
                                         FROM_TLS[i].cause) ==
                   FROM_TLS[i].error) &&
                  (table) );
    }

    result = D_TESTS_NET_ASSERT(table,
                                "d_smtp_error_from_tls",
                                "each status, with and without a cause",
                                _suite) && result;
    result = D_TESTS_NET_ASSERT( (d_smtp_error_from_tls(
                                      (enum dj_tls_error)0x7FFF,
                                      D_NET_OK) == D_SMTP_ERROR_TLS),
                                 "d_smtp_error_from_tls",
                                 "a value outside the enum is TLS",
                                 _suite) && result;

    return result;
}
