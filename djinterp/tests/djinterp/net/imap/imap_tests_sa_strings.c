/*******************************************************************************
* djinterp [net]                                         imap_tests_sa_strings.c
*
* Tests of the IMAP kernel's string writers: the form an astring takes,
* quoted strings and their escapes, literal prefixes, modified UTF-7, and
* mailbox names.
*   Each writer's octets are compared whole with what RFC 9051's grammar
* requires, and each is stopped by its sink at every octet of its output, to
* show that it reports the refusal having written a prefix of what it would
* have.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_strings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp, memset


// d_tests_imap_string_case
//   struct: a writer called on a C string with `flags`, and what it should
// give: the error, and on success the form (-1 for a writer that reports
// none) and the octets the sink then holds.
struct d_tests_imap_string_case
{
    const char*       input;
    uint32_t          flags;
    enum d_imap_error error;
    int               form;
    const char*       output;
    const char*       what;
};

// d_tests_imap_literal_case
//   struct: a literal's prefix: the octet count and flags, and the error and,
// on success, the form and octets written.
struct d_tests_imap_literal_case
{
    uint64_t          size;
    uint32_t          flags;
    enum d_imap_error error;
    int               form;
    const char*       output;
    const char*       what;
};

// d_tests_imap_mutf7_case
//   struct: a mailbox name in UTF-8 and in modified UTF-7; each encodes or
// decodes to the other.
struct d_tests_imap_mutf7_case
{
    const char* utf8;
    const char* mutf7;
    const char* what;
};

// d_tests_imap_writer
//   struct: a writer called with fixed arguments, which a sink may stop at
// any octet.
struct d_tests_imap_writer
{
    const char*       name;
    enum d_imap_error (*write)(struct d_pack_sink _sink);
};

/*
ASTRINGS
  An astring is an atom when every octet is an ASTRING-CHAR and it is not
NIL, a quoted string when no octet forbids that, and otherwise a literal,
whose prefix alone is written.
*/
static const struct d_tests_imap_string_case ASTRINGS[] =
{
    { "INBOX", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "INBOX",
      "an atom goes bare" },
    { "NIL", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"NIL\"",
      "NIL is quoted, to stay a string" },
    { "nil", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"nil\"",
      "in any case" },
    { "", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"\"",
      "the empty string is quoted" },
    { "a b", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"a b\"",
      "a space is quoted" },
    { "a\"b", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"a\\\"b\"",
      "a quote is quoted and escaped" },
    { "a\\b", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"a\\\\b\"",
      "a backslash too" },
    { "x]", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "x]",
      "an astring may hold ]" },
    { "x[", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "x[",
      "and [" },
    { "x(", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"x(\"",
      "( is quoted" },
    { "a{b", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"a{b\"",
      "so is {" },
    { "%", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"%\"",
      "a wildcard is quoted" },
    { "*", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"*\"",
      "either wildcard" },
    { "%a*", D_IMAP_STRING_LIST_WILDCARDS, D_IMAP_OK, D_IMAP_STRING_ATOM,
      "%a*", "but not in a LIST pattern" },
    { "tab\t", 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{4}\r\n",
      "a control character needs a literal" },
    { "tab\t", D_IMAP_STRING_LITERAL_PLUS, D_IMAP_OK,
      D_IMAP_STRING_LITERAL_NONSYNC, "{4+}\r\n",
      "a non-synchronizing one under LITERAL+" },
    { "cr\r", 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{3}\r\n",
      "so does CR" },
    { "lf\n", 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{3}\r\n",
      "and LF" },
    { "\x7f", 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{1}\r\n",
      "and DEL" },
    { "\xc3\xa9", 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{2}\r\n",
      "8-bit text needs a literal" },
    { "\xc3\xa9", D_IMAP_STRING_UTF8, D_IMAP_OK, D_IMAP_STRING_QUOTED,
      "\"\xc3\xa9\"", "but is quoted under UTF8=ACCEPT" },
    { "\xff", D_IMAP_STRING_UTF8, D_IMAP_OK, D_IMAP_STRING_LITERAL,
      "{1}\r\n", "if it is valid UTF-8" },
    { "~peter/mail", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "~peter/mail",
      "~ and / are atom characters" },
    { "+x", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "+x",
      "so is +" }
};

/*
QUOTEDS, UNESCAPES
  A quoted string escapes '"' and '\' and holds no CR, LF or other control,
nor 8-bit text but valid UTF-8 under UTF8=ACCEPT; unescaping undoes the two
escapes and nothing else.
*/
static const struct d_tests_imap_string_case QUOTEDS[] =
{
    { "a\"b\\c", 0u, D_IMAP_OK, -1, "\"a\\\"b\\\\c\"",
      "a quote and a backslash are escaped" },
    { "", 0u, D_IMAP_OK, -1, "\"\"",
      "the empty string is two quotes" },
    { "NIL", 0u, D_IMAP_OK, -1, "\"NIL\"",
      "NIL, quoted, is a string" },
    { "\xc3\xa9", D_IMAP_STRING_UTF8, D_IMAP_OK, -1, "\"\xc3\xa9\"",
      "UTF-8 under UTF8=ACCEPT" },
    { "\xc3\xa9", 0u, D_IMAP_ERROR_ENCODING, -1, "",
      "8-bit text otherwise cannot be quoted" },
    { "\xff", D_IMAP_STRING_UTF8, D_IMAP_ERROR_ENCODING, -1, "",
      "nor invalid UTF-8" },
    { "a\rb", D_IMAP_STRING_UTF8, D_IMAP_ERROR_ENCODING, -1, "",
      "nor CR" },
    { "a\nb", 0u, D_IMAP_ERROR_ENCODING, -1, "",
      "nor LF" },
    { "a\tb", 0u, D_IMAP_ERROR_ENCODING, -1, "",
      "nor another control character" },
    { "\x7f", 0u, D_IMAP_ERROR_ENCODING, -1, "",
      "nor DEL" }
};
static const struct d_tests_imap_string_case UNESCAPES[] =
{
    { "a\\\"b", 0u, D_IMAP_OK, -1, "a\"b",
      "an escaped quote is a quote" },
    { "a\\\\b", 0u, D_IMAP_OK, -1, "a\\b",
      "an escaped backslash a backslash" },
    { "plain", 0u, D_IMAP_OK, -1, "plain",
      "text without escapes is itself" },
    { "", 0u, D_IMAP_OK, -1, "",
      "and nothing is nothing" },
    { "a\\x", 0u, D_IMAP_ERROR_MALFORMED, -1, "",
      "a backslash escapes only those two" },
    { "a\\", 0u, D_IMAP_ERROR_MALFORMED, -1, "",
      "and one of them must follow it" }
};

/*
LITERALS
  "{n}" synchronizes; "{n+}" does not, for any n under LITERAL+ and for up
to 4096 octets under LITERAL-; n is a number64.
*/
static const struct d_tests_imap_literal_case LITERALS[] =
{
    { 5u, 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{5}\r\n",
      "a synchronizing literal" },
    { 5u, D_IMAP_STRING_LITERAL_PLUS, D_IMAP_OK,
      D_IMAP_STRING_LITERAL_NONSYNC, "{5+}\r\n",
      "LITERAL+ makes it non-synchronizing" },
    { 4097u, D_IMAP_STRING_LITERAL_PLUS | D_IMAP_STRING_LITERAL_MINUS,
      D_IMAP_OK, D_IMAP_STRING_LITERAL_NONSYNC, "{4097+}\r\n",
      "at any size" },
    { 4096u, D_IMAP_STRING_LITERAL_MINUS, D_IMAP_OK,
      D_IMAP_STRING_LITERAL_NONSYNC, "{4096+}\r\n",
      "LITERAL- up to 4096 octets" },
    { 4097u, D_IMAP_STRING_LITERAL_MINUS, D_IMAP_OK,
      D_IMAP_STRING_LITERAL, "{4097}\r\n",
      "and no further" },
    { 0u, 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL, "{0}\r\n",
      "an empty literal" },
    { D_IMAP_NUMBER64_MAX, 0u, D_IMAP_OK, D_IMAP_STRING_LITERAL,
      "{9223372036854775807}\r\n", "the largest number64" },
    { D_IMAP_NUMBER64_MAX + 1u, 0u, D_IMAP_ERROR_RANGE, -1, "",
      "but not one more" }
};

/*
MUTF7, UNENCODABLE, UNDECODABLE
  RFC 3501 section 5.1.3: printable ASCII stands for itself, "&" is "&-",
and anything else is UTF-16 in a base64 run between "&" and "-", one run
for adjacent characters. The decoder is strict, refusing what no encoder
produces.
*/
static const struct d_tests_imap_mutf7_case MUTF7[] =
{
    { "INBOX", "INBOX",
      "printable ASCII is itself" },
    { "&", "&-",
      "& is shifted out" },
    { "a&b", "a&-b",
      "within a name too" },
    { "~peter/mail/\xe5\x8f\xb0\xe5\x8c\x97/"
      "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e",
      "~peter/mail/&U,BTFw-/&ZeVnLIqe-",
      "RFC 3501's example" },
    { "Entw\xc3\xbcrfe", "Entw&APw-rfe",
      "one character in a run" },
    { "\xc3\xa9\xc3\xa9", "&AOkA6Q-",
      "two characters share a run" },
    { "\xf0\x9f\x98\x80", "&2D3eAA-",
      "beyond the BMP, a surrogate pair" },
    { "\t", "&AAk-",
      "a control character is shifted" },
    { "\x7f", "&AH8-",
      "DEL too" },
    { "", "",
      "nothing is nothing" }
};
static const char* const UNENCODABLE[] =
{
    "\xff", "\xc3", "\xed\xa0\x80", "\xc0\xaf"
};
static const char* const UNDECODABLE[] =
{
    "&Jjo", "&", "&AOk-&AOk-", "&AOl-", "&2D0-", "&3gA-", "&AGE-", "&AAA-",
    "\x80", "\t", "&,,,-"
};

/*
MAILBOXES
  A mailbox name travels in modified UTF-7 as an atom or quoted string,
never a literal; under UTF8=ACCEPT it travels as an astring.
*/
static const struct d_tests_imap_string_case MAILBOXES[] =
{
    { "INBOX", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "INBOX",
      "INBOX is an atom" },
    { "Entw\xc3\xbcrfe", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM,
      "Entw&APw-rfe", "a name travels in modified UTF-7" },
    { "Entw\xc3\xbcrfe", D_IMAP_STRING_UTF8, D_IMAP_OK,
      D_IMAP_STRING_QUOTED, "\"Entw\xc3\xbcrfe\"",
      "or quoted, in UTF-8, under UTF8=ACCEPT" },
    { "\xe5\x8f\xb0\xe5\x8c\x97", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM,
      "&U,BTFw-", "a name with no ASCII at all" },
    { "My Folder", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"My Folder\"",
      "a space is quoted" },
    { "a\"b", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"a\\\"b\"",
      "a quote is escaped" },
    { "[Gmail]/Sent", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "[Gmail]/Sent",
      "brackets stay bare" },
    { "a\rb", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "a&AA0-b",
      "a control character is shifted, never a literal" },
    { "a\rb", D_IMAP_STRING_UTF8, D_IMAP_OK, D_IMAP_STRING_LITERAL,
      "{3}\r\n", "but needs one in UTF-8" },
    { "%", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"%\"",
      "a wildcard is quoted" },
    { "Drafts&Co", 0u, D_IMAP_OK, D_IMAP_STRING_ATOM, "Drafts&-Co",
      "& is shifted out" },
    { "Drafts&Co", D_IMAP_STRING_UTF8, D_IMAP_OK, D_IMAP_STRING_ATOM,
      "Drafts&Co", "but not in UTF-8" },
    { "NIL", 0u, D_IMAP_OK, D_IMAP_STRING_QUOTED, "\"NIL\"",
      "NIL is quoted" },
    { "\xff", 0u, D_IMAP_ERROR_ENCODING, -1, "",
      "invalid UTF-8 is refused" }
};

/*
d_tests_imap_astring_row
  A row through d_imap_astring_form and d_imap_astring_write, which must
agree with it and with each other.
*/
static bool
d_tests_imap_astring_row(
    const struct d_tests_imap_string_case* _row
)
{
    struct d_tests_imap_capture capture = { .length = 0u };
    enum d_imap_string_form     form    = D_IMAP_STRING_ATOM;
    enum d_imap_string_form     written = D_IMAP_STRING_ATOM;
    const struct d_pack_text    input   = d_pack_text_from_cstr(_row->input);

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const enum d_imap_error formed =
        d_imap_astring_form(input,
                            _row->flags,
                            &form);
    const enum d_imap_error error =
        d_imap_astring_write(d_tests_imap_capture_sink(&capture),
                             input,
                             _row->flags,
                             &written);

    return ( (formed == _row->error)      &&
             (error == _row->error)       &&
             ((int)form == _row->form)    &&
             ((int)written == _row->form) &&
             (d_tests_imap_capture_is(&capture,
                                      _row->output)) );
}

/*
d_tests_imap_astring_refusals
  A NUL, text with a length but no data, a NULL form for a literal, a sink
without a write function, and LITERAL-'s limit.
*/
static bool
d_tests_imap_astring_refusals(
    struct d_test_counter* _counter
)
{
    static char                 tabs[4097];
    struct d_tests_imap_capture capture  = { .length = 0u };
    enum d_imap_string_form     form     = D_IMAP_STRING_ATOM;
    enum d_imap_string_form     longer   = D_IMAP_STRING_ATOM;
    const struct d_pack_text    nul      = d_pack_text_from_span("a\0b",
                                                                 3u);
    const struct d_pack_text    dataless = { NULL, 3u };
    const struct d_pack_sink    deaf     = { NULL, NULL };
    bool                        result   = true;

    memset(tabs,
           '\t',
           sizeof(tabs));
    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink = d_tests_imap_capture_sink(&capture);
    const bool               nuls =
        ( (d_imap_astring_form(nul,
                               0u,
                               &form) == D_IMAP_ERROR_ENCODING)  &&
          (d_imap_astring_write(sink,
                                nul,
                                0u,
                                &form) == D_IMAP_ERROR_ENCODING) );
    const bool invalid =
        ( (d_imap_astring_form(dataless,
                               0u,
                               &form) == D_IMAP_ERROR_INVALID_ARGUMENT)  &&
          (d_imap_astring_form(d_pack_text_from_cstr("a"),
                               0u,
                               NULL) == D_IMAP_ERROR_INVALID_ARGUMENT)   &&
          (d_imap_astring_write(sink,
                                dataless,
                                0u,
                                &form) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_astring_write(deaf,
                                d_pack_text_from_cstr("a"),
                                0u,
                                &form) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_astring_write(sink,
                                d_pack_text_from_cstr("tab\t"),
                                0u,
                                NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );
    const bool minus =
        ( (d_imap_astring_form(d_pack_text_from_span(tabs,
                                                     4096u),
                               D_IMAP_STRING_LITERAL_MINUS,
                               &form) == D_IMAP_OK)             &&
          (form == D_IMAP_STRING_LITERAL_NONSYNC)               &&
          (d_imap_astring_form(d_pack_text_from_span(tabs,
                                                     4097u),
                               D_IMAP_STRING_LITERAL_MINUS,
                               &longer) == D_IMAP_OK)           &&
          (longer == D_IMAP_STRING_LITERAL) );

    result = d_tests_net_assert((nuls) && (capture.length == 0u),
                                "d_imap_astring_write",
                                "no string holding a NUL is written",
                                _counter) && result;
    result = d_tests_net_assert((invalid) && (capture.length == 0u),
                                "d_imap_astring_write",
                                "bad arguments are refused before writing",
                                _counter) && result;
    result = d_tests_net_assert(minus,
                                "d_imap_astring_form",
                                "LITERAL- covers 4096 octets, not 4097",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_astring
  Tests the following:
  - each string takes the most compact valid form: atom, quoted, literal
  - d_imap_astring_form and d_imap_astring_write agree on it
  - NUL, data-less text, a literal without a form and a deaf sink are
    refused before anything is written
  - LITERAL- reaches 4096 octets and no further
*/
bool
d_tests_sa_imap_astring(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(ASTRINGS) / sizeof(ASTRINGS[0]);
    bool         result = true;

    // each row, through the form and the writer alike
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_astring_row(&ASTRINGS[i]),
                                    "d_imap_astring_write",
                                    ASTRINGS[i].what,
                                    _counter) && result;
    }

    result = d_tests_imap_astring_refusals(_counter) && result;

    return result;
}

/*
d_tests_imap_quoted_row
  A row through d_imap_quoted_write, or d_imap_quoted_unescape when
`_unescape` is set; the octets are compared on success.
*/
static bool
d_tests_imap_quoted_row(
    const struct d_tests_imap_string_case* _row,
    bool                                   _unescape
)
{
    struct d_tests_imap_capture capture = { .length = 0u };

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink  = d_tests_imap_capture_sink(&capture);
    const struct d_pack_text input = d_pack_text_from_cstr(_row->input);
    const enum d_imap_error  error =
        (_unescape)
            ? d_imap_quoted_unescape(sink,
                                     input)
            : d_imap_quoted_write(sink,
                                  input,
                                  _row->flags);

    return ( (error == _row->error) &&
             ( (error != D_IMAP_OK) ||
               (d_tests_imap_capture_is(&capture,
                                        _row->output)) ) );
}

/*
d_tests_sa_imap_quoted
  Tests the following:
  - quoting escapes '"' and '\', and refuses what a quoted string may not
    hold
  - unescaping undoes exactly those two escapes
  - a NUL, data-less text and a deaf sink are refused
*/
bool
d_tests_sa_imap_quoted(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_capture capture  = { .length = 0u };
    const struct d_pack_text    dataless = { NULL, 3u };
    const struct d_pack_sink    deaf     = { NULL, NULL };
    bool                        result   = true;

    // each string quoted, then each content unescaped
    for (size_t i = 0u; i < (sizeof(QUOTEDS) / sizeof(QUOTEDS[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_quoted_row(&QUOTEDS[i],
                                                            false),
                                    "d_imap_quoted_write",
                                    QUOTEDS[i].what,
                                    _counter) && result;
    }

    for (size_t i = 0u; i < (sizeof(UNESCAPES) / sizeof(UNESCAPES[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_quoted_row(&UNESCAPES[i],
                                                            true),
                                    "d_imap_quoted_unescape",
                                    UNESCAPES[i].what,
                                    _counter) && result;
    }

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink    = d_tests_imap_capture_sink(&capture);
    const bool               refused =
        ( (d_imap_quoted_write(sink,
                               d_pack_text_from_span("a\0b",
                                                     3u),
                               0u) == D_IMAP_ERROR_ENCODING)         &&
          (d_imap_quoted_write(sink,
                               dataless,
                               0u) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_quoted_write(deaf,
                               d_pack_text_from_cstr("a"),
                               0u) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_quoted_unescape(sink,
                                  dataless) ==
           D_IMAP_ERROR_INVALID_ARGUMENT)                            &&
          (d_imap_quoted_unescape(deaf,
                                  d_pack_text_from_cstr("a")) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert((refused) && (capture.length == 0u),
                                "d_imap_quoted_write",
                                "a NUL and bad arguments are refused",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_literal_row
  A row through d_imap_literal_prefix_write; the form and octets are
compared on success.
*/
static bool
d_tests_imap_literal_row(
    const struct d_tests_imap_literal_case* _row
)
{
    struct d_tests_imap_capture capture = { .length = 0u };
    enum d_imap_string_form     form    = D_IMAP_STRING_ATOM;

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const enum d_imap_error error =
        d_imap_literal_prefix_write(d_tests_imap_capture_sink(&capture),
                                    _row->size,
                                    _row->flags,
                                    &form);

    return ( (error == _row->error) &&
             ( (error != D_IMAP_OK) ||
               ( ((int)form == _row->form) &&
                 (d_tests_imap_capture_is(&capture,
                                          _row->output)) ) ) );
}

/*
d_tests_sa_imap_literal
  Tests the following:
  - "{n}" and "{n+}", as LITERAL+ and LITERAL- permit
  - n up to the largest number64, and RANGE past it, writing nothing
  - the form may go unreported, and a deaf sink is refused
*/
bool
d_tests_sa_imap_literal(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_capture capture = { .length = 0u };
    const struct d_pack_sink    deaf    = { NULL, NULL };
    const size_t                count   = sizeof(LITERALS) /
                                          sizeof(LITERALS[0]);
    bool                        result  = true;

    // each prefix
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_literal_row(&LITERALS[i]),
                                    "d_imap_literal_prefix_write",
                                    LITERALS[i].what,
                                    _counter) && result;
    }

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink   = d_tests_imap_capture_sink(&capture);
    const bool               ranged =
        ( (d_imap_literal_prefix_write(sink,
                                       D_IMAP_NUMBER64_MAX + 1u,
                                       0u,
                                       NULL) == D_IMAP_ERROR_RANGE) &&
          (capture.length == 0u) );
    const bool unreported =
        ( (d_imap_literal_prefix_write(sink,
                                       5u,
                                       0u,
                                       NULL) == D_IMAP_OK)       &&
          (d_tests_imap_capture_is(&capture,
                                   "{5}\r\n"))                   &&
          (d_imap_literal_prefix_write(deaf,
                                       5u,
                                       0u,
                                       NULL) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(ranged,
                                "d_imap_literal_prefix_write",
                                "a size out of range writes nothing",
                                _counter) && result;
    result = d_tests_net_assert(unreported,
                                "d_imap_literal_prefix_write",
                                "the form is optional; the sink is not",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_mutf7_way
  `_input` encoded, or decoded when `_decode` is set, and compared with
`_expected`: true when the call succeeds and agrees.
*/
static bool
d_tests_imap_mutf7_way(
    const char* _input,
    const char* _expected,
    bool        _decode
)
{
    struct d_tests_imap_capture capture = { .length = 0u };

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink  = d_tests_imap_capture_sink(&capture);
    const struct d_pack_text input = d_pack_text_from_cstr(_input);
    const enum d_imap_error  error = (_decode)
                                         ? d_imap_mutf7_decode(sink,
                                                               input)
                                         : d_imap_mutf7_encode(sink,
                                                               input);

    return ( (error == D_IMAP_OK) &&
             (d_tests_imap_capture_is(&capture,
                                      _expected)) );
}

/*
d_tests_sa_imap_mutf7
  Tests the following:
  - each name encodes to its modified UTF-7 and decodes back
  - invalid UTF-8, a NUL, and every form the strict decoder refuses
  - data-less text and a deaf sink are refused
*/
bool
d_tests_sa_imap_mutf7(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_capture capture   = { .length = 0u };
    const struct d_pack_text    dataless  = { NULL, 3u };
    const struct d_pack_sink    deaf      = { NULL, NULL };
    bool                        encodings = true;
    bool                        result    = true;

    // each pair, both ways
    for (size_t i = 0u; i < (sizeof(MUTF7) / sizeof(MUTF7[0])); ++i)
    {
        result = d_tests_net_assert( (d_tests_imap_mutf7_way(MUTF7[i].utf8,
                                                             MUTF7[i].mutf7,
                                                             false)) &&
                                     (d_tests_imap_mutf7_way(MUTF7[i].mutf7,
                                                             MUTF7[i].utf8,
                                                             true)),
                                    "d_imap_mutf7_encode",
                                    MUTF7[i].what,
                                    _counter) && result;
    }

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink = d_tests_imap_capture_sink(&capture);

    // what neither direction accepts
    for (size_t i = 0u; i < (sizeof(UNENCODABLE) / sizeof(UNENCODABLE[0])); ++i)
    {
        encodings = ( (d_imap_mutf7_encode(sink,
                                           d_pack_text_from_cstr(
                                               UNENCODABLE[i])) ==
                       D_IMAP_ERROR_ENCODING) &&
                      (encodings) );
    }

    for (size_t i = 0u; i < (sizeof(UNDECODABLE) / sizeof(UNDECODABLE[0])); ++i)
    {
        encodings = ( (d_imap_mutf7_decode(sink,
                                           d_pack_text_from_cstr(
                                               UNDECODABLE[i])) ==
                       D_IMAP_ERROR_ENCODING) &&
                      (encodings) );
    }

    const bool lenient =
        ( (d_tests_imap_mutf7_way("&-&-",
                                  "&&",
                                  true))                    &&
          (d_imap_mutf7_encode(sink,
                               d_pack_text_from_span("a\0b",
                                                     3u)) ==
           D_IMAP_ERROR_ENCODING)                           &&
          (d_imap_mutf7_encode(sink,
                               dataless) ==
           D_IMAP_ERROR_INVALID_ARGUMENT)                   &&
          (d_imap_mutf7_decode(deaf,
                               d_pack_text_from_cstr("a")) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(encodings,
                                "d_imap_mutf7_decode",
                                "invalid UTF-8 and loose modified UTF-7",
                                _counter) && result;
    result = d_tests_net_assert(lenient,
                                "d_imap_mutf7_decode",
                                "&-&- is &&; a NUL and bad arguments",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_mailbox_row
  A row through d_imap_mailbox_write; the form and octets are compared on
success.
*/
static bool
d_tests_imap_mailbox_row(
    const struct d_tests_imap_string_case* _row
)
{
    struct d_tests_imap_capture capture = { .length = 0u };
    enum d_imap_string_form     form    = D_IMAP_STRING_ATOM;

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const enum d_imap_error error =
        d_imap_mailbox_write(d_tests_imap_capture_sink(&capture),
                             d_pack_text_from_cstr(_row->input),
                             _row->flags,
                             &form);

    return ( (error == _row->error) &&
             ( (error != D_IMAP_OK) ||
               ( ((int)form == _row->form) &&
                 (d_tests_imap_capture_is(&capture,
                                          _row->output)) ) ) );
}

/*
d_tests_sa_imap_mailbox
  Tests the following:
  - names are written in modified UTF-7, or as astrings under UTF8=ACCEPT
  - a name needing a literal is refused, unwritten, without a form
  - INBOX alone compares case-insensitively
*/
bool
d_tests_sa_imap_mailbox(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_capture capture = { .length = 0u };
    const size_t                count   = sizeof(MAILBOXES) /
                                          sizeof(MAILBOXES[0]);
    bool                        result  = true;

    // each name
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_mailbox_row(&MAILBOXES[i]),
                                    "d_imap_mailbox_write",
                                    MAILBOXES[i].what,
                                    _counter) && result;
    }

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const bool formless =
        ( (d_imap_mailbox_write(d_tests_imap_capture_sink(&capture),
                                d_pack_text_from_cstr("a\rb"),
                                D_IMAP_STRING_UTF8,
                                NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (capture.length == 0u)                                       &&
          (d_imap_mailbox_write(d_tests_imap_capture_sink(&capture),
                                d_pack_text_from_cstr("My Folder"),
                                0u,
                                NULL) == D_IMAP_OK) );
    const bool inbox =
        ( (d_imap_mailbox_is_inbox(d_pack_text_from_cstr("INBOX")))       &&
          (d_imap_mailbox_is_inbox(d_pack_text_from_cstr("inBox")))       &&
          (!d_imap_mailbox_is_inbox(d_pack_text_from_cstr("INBOX.Sent"))) &&
          (!d_imap_mailbox_is_inbox(d_pack_text_from_cstr("INBO")))       &&
          (!d_imap_mailbox_is_inbox(d_pack_text_from_cstr(""))) );

    result = d_tests_net_assert(formless,
                                "d_imap_mailbox_write",
                                "without a form, a literal is refused",
                                _counter) && result;
    result = d_tests_net_assert(inbox,
                                "d_imap_mailbox_is_inbox",
                                "INBOX in any case, and nothing else",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_write_astring, d_tests_imap_write_literal,
d_tests_imap_write_encoded, d_tests_imap_write_decoded,
d_tests_imap_write_mailbox, d_tests_imap_write_unescaped,
d_tests_imap_write_flags, d_tests_imap_write_items,
d_tests_imap_write_seqset, d_tests_imap_write_datetime
  Each writer, with arguments whose output takes several of its writes.
*/
static enum d_imap_error
d_tests_imap_write_astring(
    struct d_pack_sink _sink
)
{
    enum d_imap_string_form form = D_IMAP_STRING_ATOM;

    return d_imap_astring_write(_sink,
                                d_pack_text_from_cstr("a \"b\""),
                                0u,
                                &form);
}

static enum d_imap_error
d_tests_imap_write_literal(
    struct d_pack_sink _sink
)
{
    return d_imap_literal_prefix_write(_sink,
                                       4096u,
                                       0u,
                                       NULL);
}

static enum d_imap_error
d_tests_imap_write_encoded(
    struct d_pack_sink _sink
)
{
    return d_imap_mutf7_encode(_sink,
                               d_pack_text_from_cstr(MUTF7[3].utf8));
}

static enum d_imap_error
d_tests_imap_write_decoded(
    struct d_pack_sink _sink
)
{
    return d_imap_mutf7_decode(_sink,
                               d_pack_text_from_cstr(MUTF7[3].mutf7));
}

static enum d_imap_error
d_tests_imap_write_mailbox(
    struct d_pack_sink _sink
)
{
    enum d_imap_string_form form = D_IMAP_STRING_ATOM;

    return d_imap_mailbox_write(_sink,
                                d_pack_text_from_cstr(MUTF7[4].utf8),
                                0u,
                                &form);
}

static enum d_imap_error
d_tests_imap_write_unescaped(
    struct d_pack_sink _sink
)
{
    return d_imap_quoted_unescape(_sink,
                                  d_pack_text_from_cstr("a\\\"b\\\\c"));
}

static enum d_imap_error
d_tests_imap_write_flags(
    struct d_pack_sink _sink
)
{
    return d_imap_flags_write(_sink,
                              D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN)    |
                              D_IMAP_FLAG_BIT(D_IMAP_FLAG_DELETED) |
                              D_IMAP_FLAG_BIT(D_IMAP_FLAG_DRAFT));
}

static enum d_imap_error
d_tests_imap_write_items(
    struct d_pack_sink _sink
)
{
    return d_imap_status_items_write(_sink,
                                     D_IMAP_STATUS_BIT(D_IMAP_STATUS_MESSAGES) |
                                     D_IMAP_STATUS_BIT(D_IMAP_STATUS_UNSEEN));
}

static enum d_imap_error
d_tests_imap_write_seqset(
    struct d_pack_sink _sink
)
{
    static const struct d_imap_seq_range RANGES[] =
    {
        { 1u, 4u },
        { 7u, 7u },
        { 9u, D_IMAP_SEQ_STAR }
    };

    return d_imap_seqset_write(_sink,
                               RANGES,
                               3u);
}

static enum d_imap_error
d_tests_imap_write_datetime(
    struct d_pack_sink _sink
)
{
    const struct d_imap_datetime when = { 2026, 2, 1, 9, 5, 7, -300 };

    return d_imap_datetime_write(_sink,
                                 when);
}

/*
WRITERS
  Every writer whose output a sink can cut short.
*/
static const struct d_tests_imap_writer WRITERS[] =
{
    { "d_imap_astring_write",        d_tests_imap_write_astring },
    { "d_imap_literal_prefix_write", d_tests_imap_write_literal },
    { "d_imap_mutf7_encode",         d_tests_imap_write_encoded },
    { "d_imap_mutf7_decode",         d_tests_imap_write_decoded },
    { "d_imap_mailbox_write",        d_tests_imap_write_mailbox },
    { "d_imap_quoted_unescape",      d_tests_imap_write_unescaped },
    { "d_imap_flags_write",          d_tests_imap_write_flags },
    { "d_imap_status_items_write",   d_tests_imap_write_items },
    { "d_imap_seqset_write",         d_tests_imap_write_seqset },
    { "d_imap_datetime_write",       d_tests_imap_write_datetime }
};

/*
d_tests_imap_refusal_check
  A writer's whole output, then the writer again under a sink that refuses
after each shorter length: each time SINK, with exactly that prefix kept.
*/
static bool
d_tests_imap_refusal_check(
    const struct d_tests_imap_writer* _writer
)
{
    struct d_tests_imap_capture whole = { .length = 0u };
    struct d_tests_imap_capture part  = { .length = 0u };

    d_tests_imap_capture_init(&whole,
                              sizeof(whole.data));

    bool held = ( (_writer->write(d_tests_imap_capture_sink(&whole)) ==
                   D_IMAP_OK) &&
                  (whole.length > 0u) );

    // every length short of the whole
    for (size_t limit = 0u; (held) && (limit < whole.length); ++limit)
    {
        d_tests_imap_capture_init(&part,
                                  limit);
        held = ( (_writer->write(d_tests_imap_capture_sink(&part)) ==
                  D_IMAP_ERROR_SINK)         &&
                 (part.length == limit)      &&
                 (memcmp(part.data,
                         whole.data,
                         limit) == 0) );
    }

    return held;
}

/*
d_tests_sa_imap_sink_refusals
  Tests the following:
  - every writer reports SINK when its sink stops taking octets, at any
    octet of its output, having written exactly the prefix taken
*/
bool
d_tests_sa_imap_sink_refusals(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(WRITERS) / sizeof(WRITERS[0]);
    bool         result = true;

    // each writer, cut short at every octet
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_refusal_check(&WRITERS[i]),
                                    WRITERS[i].name,
                                    "SINK, with a prefix, at any octet",
                                    _counter) && result;
    }

    return result;
}
