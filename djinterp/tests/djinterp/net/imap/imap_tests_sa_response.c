/*******************************************************************************
* djinterp [net]                                        imap_tests_sa_response.c
*
* Tests of the IMAP kernel's response parser and lexer, and of the decoders
* that turn tokens into strings and mailbox names.
*   Responses come from RFC 9051's examples and from real servers' habits; a
* token stream is compared whole, spelled out as text; and input that stops
* short of a token is shown to leave the lexer where it was, describing the
* partial token, until the octets it lacks arrive.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_response.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memcpy, strlen


// d_tests_imap_response_case
//   struct: a server line, and every field its parse should hold; "" for a
// text that should be empty.
struct d_tests_imap_response_case
{
    const char* line;
    int         kind;
    int         condition;
    int         code;
    bool        numbered;
    uint32_t    number;
    const char* tag;
    const char* keyword;
    const char* code_name;
    const char* argument;
    const char* text;
    const char* rest;
};

// d_tests_imap_refusal_case
//   struct: input a parser refuses, and the error it gives.
struct d_tests_imap_refusal_case
{
    const char*       input;
    enum d_imap_error error;
    const char*       what;
};

// d_tests_imap_lex_case
//   struct: input and its length (0: up to its NUL), the lexer flags, and
// the token stream as d_tests_imap_render spells it.
struct d_tests_imap_lex_case
{
    const char* input;
    size_t      length;
    uint32_t    flags;
    const char* tokens;
    const char* what;
};

// d_tests_imap_partial_case
//   struct: input stopping short of a token, and how the token is described:
// its kind (-1: not described), the count it declares and the octets here.
struct d_tests_imap_partial_case
{
    const char* input;
    int         kind;
    uint64_t    number;
    const char* present;
    const char* what;
};

// d_tests_imap_decode_case
//   struct: a token, lexed from `input`, the decoder's flags, and the error
// and octets decoding it gives.
struct d_tests_imap_decode_case
{
    const char*       input;
    uint32_t          flags;
    enum d_imap_error error;
    const char*       output;
    const char*       what;
};

// d_tests_imap_rendering
//   struct: a token stream spelled out: a<atom>, n<digits>#value, nil,
// q<content> (q!<...> when escaped), l<count>[+][~]<octets>, f<flag>,
// punctuation as itself, nl, end, and errN@position for an error.
struct d_tests_imap_rendering
{
    char   text[512];
    size_t length;
};

/*
RESPONSES
  Greetings, completions, untagged status and data, continuations, codes
with and without arguments, a bare LF and a missing line break.
*/
static const struct d_tests_imap_response_case RESPONSES[] =
{
    { "* OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE "
      "IDLE STARTTLS AUTH=PLAIN] Dovecot ready.\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_CAPABILITY,
      false, 0u, "", "OK", "CAPABILITY",
      "IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE STARTTLS "
      "AUTH=PLAIN", "Dovecot ready.", "" },
    { "A0001 OK [READ-WRITE] SELECT completed\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_READ_WRITE,
      false, 0u, "A0001", "OK", "READ-WRITE", "", "SELECT completed", "" },
    { "* 172 EXISTS\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      true, 172u, "", "EXISTS", "", "", "", "" },
    { "* 0 EXISTS\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      true, 0u, "", "EXISTS", "", "", "", "" },
    { "* FLAGS (\\Answered \\Flagged)\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "FLAGS", "", "", "", "(\\Answered \\Flagged)" },
    { "* OK [UIDVALIDITY 3857529045] UIDs valid\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_UIDVALIDITY,
      false, 0u, "", "OK", "UIDVALIDITY", "3857529045", "UIDs valid", "" },
    { "* OK [PERMANENTFLAGS (\\Deleted \\Seen \\*)] Limited\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_OK,
      D_IMAP_CODE_PERMANENTFLAGS, false, 0u, "", "OK", "PERMANENTFLAGS",
      "(\\Deleted \\Seen \\*)", "Limited", "" },
    { "* OK [X-FOO bar] baz\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_OTHER,
      false, 0u, "", "OK", "X-FOO", "bar", "baz", "" },
    { "+ idling\r\n",
      D_IMAP_RESPONSE_CONTINUATION, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "", "", "", "idling", "" },
    { "+ YGgGCSqGSIb3EgECAgIBAAD/////6jcyG4GE3KkTzBeBiVHeceP2CWY0SR0fAQAgAAQ"
      "EBAQ=\r\n",
      D_IMAP_RESPONSE_CONTINUATION, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "", "", "",
      "YGgGCSqGSIb3EgECAgIBAAD/////6jcyG4GE3KkTzBeBiVHeceP2CWY0SR0fAQAgAAQ"
      "EBAQ=", "" },
    { "+ \r\n",
      D_IMAP_RESPONSE_CONTINUATION, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "", "", "", "", "" },
    { "* BYE Logging out\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_BYE, D_IMAP_CODE_NONE,
      false, 0u, "", "BYE", "", "", "Logging out", "" },
    { "* PREAUTH IMAP4rev1 server logged in as Smith\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_PREAUTH, D_IMAP_CODE_NONE,
      false, 0u, "", "PREAUTH", "", "",
      "IMAP4rev1 server logged in as Smith", "" },
    { "A0002 NO [TRYCREATE] No such mailbox\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_NO, D_IMAP_CODE_TRYCREATE,
      false, 0u, "A0002", "NO", "TRYCREATE", "", "No such mailbox", "" },
    { "* 12 FETCH (BODY[TEXT] {5}\r\nHello)\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      true, 12u, "", "FETCH", "", "", "", "(BODY[TEXT] {5}\r\nHello)" },
    { "* NO [ALERT] System shutdown\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NO, D_IMAP_CODE_ALERT,
      false, 0u, "", "NO", "ALERT", "", "System shutdown", "" },
    { "A3 OK [COPYUID 38505 304,319:320 3956:3958] Done\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_COPYUID,
      false, 0u, "A3", "OK", "COPYUID", "38505 304,319:320 3956:3958",
      "Done", "" },
    { "a4 ok [read-only] done\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_READ_ONLY,
      false, 0u, "a4", "ok", "read-only", "", "done", "" },
    { "* SEARCH 2 84 882\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "SEARCH", "", "", "", "2 84 882" },
    { "* SEARCH\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "SEARCH", "", "", "", "" },
    { "* 23 EXPUNGE\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      true, 23u, "", "EXPUNGE", "", "", "", "" },
    { "A6 BAD Command unknown\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_BAD, D_IMAP_CODE_NONE,
      false, 0u, "A6", "BAD", "", "", "Command unknown", "" },
    { "A7 OK\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_NONE,
      false, 0u, "A7", "OK", "", "", "", "" },
    { "A8 OK [ALERT]\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_OK, D_IMAP_CODE_ALERT,
      false, 0u, "A8", "OK", "ALERT", "", "", "" },
    { "* STATUS \"Sent Items\" (MESSAGES 231 UIDNEXT 44292)\r\n",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      false, 0u, "", "STATUS", "", "", "",
      "\"Sent Items\" (MESSAGES 231 UIDNEXT 44292)" },
    { "* 1 FETCH (FLAGS (\\Seen))",
      D_IMAP_RESPONSE_UNTAGGED, D_IMAP_CONDITION_NONE, D_IMAP_CODE_NONE,
      true, 1u, "", "FETCH", "", "", "", "(FLAGS (\\Seen))" },
    { "A9 NO [AUTHENTICATIONFAILED] Invalid credentials\r\n",
      D_IMAP_RESPONSE_TAGGED, D_IMAP_CONDITION_NO,
      D_IMAP_CODE_AUTHENTICATIONFAILED, false, 0u, "A9", "NO",
      "AUTHENTICATIONFAILED", "", "Invalid credentials", "" }
};

/*
REFUSED
  What is no response head.
*/
static const struct d_tests_imap_refusal_case REFUSED[] =
{
    { "A1 PREAUTH x\r\n", D_IMAP_ERROR_MALFORMED,
      "a completion is OK, NO or BAD" },
    { "A1 BYE x\r\n", D_IMAP_ERROR_MALFORMED,
      "never BYE" },
    { "* 4294967296 EXISTS\r\n", D_IMAP_ERROR_RANGE,
      "a number past 32 bits" },
    { "*\r\n", D_IMAP_ERROR_MALFORMED,
      "a star alone" },
    { "", D_IMAP_ERROR_MALFORMED,
      "nothing at all" },
    { "A5 OK [UNSEEN 12 Message 12\r\n", D_IMAP_ERROR_MALFORMED,
      "an unclosed code" },
    { "* OK [] empty code\r\n", D_IMAP_ERROR_MALFORMED,
      "an empty one" },
    { "a\x01" "b OK x\r\n", D_IMAP_ERROR_MALFORMED,
      "a control character in a tag" },
    { "A+1 OK x\r\n", D_IMAP_ERROR_MALFORMED,
      "a + in one" },
    { "*  OK x\r\n", D_IMAP_ERROR_MALFORMED,
      "two spaces where one belongs" }
};

/*
LEXES
  Token streams: FETCH data, NIL and numbers at their edges, every kind of
literal, quoted strings, the two bracket contexts, flags, 8-bit octets,
spaces and line breaks, and input that is no token.
*/
static const struct d_tests_imap_lex_case LEXES[] =
{
    { "(FLAGS (\\Seen) UID 4827313 INTERNALDATE \"17-Jul-1996 02:44:25 "
      "-0700\" RFC822.SIZE 4286 BODY[HEADER.FIELDS (DATE FROM)] "
      "{12}\r\nDate: today\n)\r\n", 0u, 0u,
      "( a<FLAGS> ( f<\\Seen> ) a<UID> n<4827313>#4827313 a<INTERNALDATE> "
      "q<17-Jul-1996 02:44:25 -0700> a<RFC822.SIZE> n<4286>#4286 a<BODY> "
      "[ a<HEADER.FIELDS> ( a<DATE> a<FROM> ) ] l12<Date: today\\n> ) nl end",
      "FETCH data, RFC 3501's example with a literal" },
    { "nil Nil NIL nIl NILL", 0u, 0u,
      "nil nil nil nil a<NILL> end",
      "NIL in any case; NILL an atom" },
    { "18446744073709551615 18446744073709551616 007 0", 0u, 0u,
      "n<18446744073709551615>#18446744073709551615 "
      "a<18446744073709551616> n<007>#7 n<0>#0 end",
      "a number fits in 64 bits, or is an atom" },
    { "{5}\r\nHello", 0u, 0u, "l5<Hello> end",
      "a literal" },
    { "{5+}\r\nHello", 0u, 0u, "l5+<Hello> end",
      "a non-synchronizing one" },
    { "~{3}\r\na\0b", 9u, 0u, "l3~<a\\x00b> end",
      "a literal8, which may hold NUL" },
    { "~{3+}\r\nabc", 0u, 0u, "l3+~<abc> end",
      "a non-synchronizing literal8" },
    { "\"a\\\"b\" \"ab\" \"a\\\\b\" \"\"", 0u, 0u,
      "q!<a\\\"b> q<ab> q!<a\\\\b> q<> end",
      "quoted strings, escaped ones marked" },
    { "[Gmail]/Sent", 0u, 0u, "[ a<Gmail> ] a</Sent> end",
      "brackets are punctuation in FETCH data" },
    { "[Gmail]/Sent", 0u, D_IMAP_LEX_ASTRING, "a<[Gmail]/Sent> end",
      "but part of an astring" },
    { "a]b", 0u, 0u, "a<a> ] a<b> end",
      "] likewise" },
    { "a]b", 0u, D_IMAP_LEX_ASTRING, "a<a]b> end",
      "in both contexts" },
    { "\\Seen \\* \\Seen)", 0u, 0u, "f<\\Seen> f<\\*> f<\\Seen> ) end",
      "flags, \\* among them" },
    { "* caf\xc3\xa9 \"\xc3\xa9\" ~x", 0u, 0u,
      "* a<caf\\xc3\\xa9> q<\\xc3\\xa9> a<~x> end",
      "8-bit octets in atoms and quoted strings" },
    { "  a   b  ", 0u, 0u, "a<a> a<b> end",
      "spaces separate tokens" },
    { "a\nb", 0u, 0u, "a<a> nl a<b> end",
      "a bare LF is a line break" },
    { "{5x}\r\n", 0u, 0u, "err4@0",
      "a literal's count is digits" },
    { "{}\r\n", 0u, 0u, "err4@0",
      "and there is one" },
    { "{5}x", 0u, 0u, "err4@0",
      "and a line break follows it" },
    { "{9223372036854775808}\r\n", 0u, 0u, "err6@0",
      "a count past number64" },
    { "\"a\rb\"", 0u, 0u, "err4@0",
      "a quoted string holds no CR" },
    { "a\rb", 0u, 0u, "a<a> err4@1",
      "a CR without its LF" },
    { "\x01", 0u, 0u, "err4@0",
      "a control character" },
    { "* %", 0u, 0u, "* err4@1",
      "a wildcard in a response" }
};

/*
PARTIALS
  Input that stops short: a literal's octets, its announcement, a quoted
string, an escape, a flag.
*/
static const struct d_tests_imap_partial_case PARTIALS[] =
{
    { "{5}\r\nHel", D_IMAP_TOKEN_LITERAL, 5u, "Hel",
      "a literal's octets cut short" },
    { "{3}\r\n", D_IMAP_TOKEN_LITERAL, 3u, "",
      "none of them here yet" },
    { "{9223372036854775807}\r\n", D_IMAP_TOKEN_LITERAL,
      D_IMAP_NUMBER64_MAX, "", "the largest literal announced" },
    { "{5}", -1, 0u, "", "the announcement's line break not here" },
    { "{5}\r", -1, 0u, "", "half of it here" },
    { "{", -1, 0u, "", "a brace alone" },
    { "\"ab", -1, 0u, "", "an unclosed quoted string" },
    { "\"a\\", -1, 0u, "", "one ending mid-escape" },
    { "\\", -1, 0u, "", "a backslash alone" }
};

/*
STRING_VALUES, MAILBOX_VALUES
  Tokens decoded: as strings, and as mailbox names in modified UTF-7 or,
under UTF8=ACCEPT, UTF-8.
*/
static const struct d_tests_imap_decode_case STRING_VALUES[] =
{
    { "atom", 0u, D_IMAP_OK, "atom", "an atom is its text" },
    { "123", 0u, D_IMAP_OK, "123", "so is a number" },
    { "NIL", 0u, D_IMAP_OK, "", "NIL writes nothing" },
    { "\"q\\\"x\\\\y\"", 0u, D_IMAP_OK, "q\"x\\y",
      "a quoted string is unescaped" },
    { "{3}\r\nl i", 0u, D_IMAP_OK, "l i", "a literal is its octets" },
    { "\"\"", 0u, D_IMAP_OK, "", "an empty string" },
    { "(", 0u, D_IMAP_ERROR_INVALID_ARGUMENT, "",
      "punctuation is no string" },
    { "\\Seen", 0u, D_IMAP_ERROR_INVALID_ARGUMENT, "",
      "nor is a flag" },
    { "\"a\\x\"", 0u, D_IMAP_ERROR_MALFORMED, "",
      "a bad escape" }
};
static const struct d_tests_imap_decode_case MAILBOX_VALUES[] =
{
    { "Entw&APw-rfe", 0u, D_IMAP_OK, "Entw\xc3\xbcrfe",
      "modified UTF-7 decoded" },
    { "Entw&APw-rfe", D_IMAP_STRING_UTF8, D_IMAP_OK, "Entw&APw-rfe",
      "taken as UTF-8 under UTF8=ACCEPT" },
    { "\"&U,BTFw-\"", 0u, D_IMAP_OK, "\xe5\x8f\xb0\xe5\x8c\x97",
      "a quoted name" },
    { "\"\xe5\x8f\xb0\xe5\x8c\x97\"", 0u, D_IMAP_ERROR_ENCODING, "",
      "raw UTF-8 is no modified UTF-7" },
    { "\"\xe5\x8f\xb0\xe5\x8c\x97\"", D_IMAP_STRING_UTF8, D_IMAP_OK,
      "\xe5\x8f\xb0\xe5\x8c\x97", "but it is UTF-8" },
    { "\"\xff\"", D_IMAP_STRING_UTF8, D_IMAP_ERROR_ENCODING, "",
      "and invalid UTF-8 is neither" },
    { "&Jjo", 0u, D_IMAP_ERROR_ENCODING, "",
      "an unterminated run" },
    { "{12}\r\nEntw&APw-rfe", 0u, D_IMAP_OK, "Entw\xc3\xbcrfe",
      "a literal name" },
    { "42", 0u, D_IMAP_OK, "42", "a numeric name" },
    { "\"a\\\"b\"", 0u, D_IMAP_OK, "a\"b", "an escaped one" },
    { "NIL", 0u, D_IMAP_ERROR_INVALID_ARGUMENT, "",
      "NIL names no mailbox" }
};

/*
d_tests_imap_response_row
  A row's line parsed, every field compared.
*/
static bool
d_tests_imap_response_row(
    const struct d_tests_imap_response_case* _row
)
{
    struct d_imap_response response = { .kind = 7 };

    const bool parsed =
        (d_imap_response_parse(d_pack_text_from_cstr(_row->line),
                               &response) == D_IMAP_OK);

    return ( (parsed)                                     &&
             (response.kind == _row->kind)                &&
             (response.condition == _row->condition)      &&
             (response.code == _row->code)                &&
             ((response.has_number == 1) == _row->numbered) &&
             (response.number == _row->number)            &&
             (d_tests_imap_span_is(response.tag,
                                   _row->tag))            &&
             (d_tests_imap_span_is(response.keyword,
                                   _row->keyword))        &&
             (d_tests_imap_span_is(response.code_name,
                                   _row->code_name))      &&
             (d_tests_imap_span_is(response.code_argument,
                                   _row->argument))       &&
             (d_tests_imap_span_is(response.text,
                                   _row->text))           &&
             (d_tests_imap_span_is(response.rest,
                                   _row->rest)) );
}

/*
d_tests_sa_imap_responses
  Tests the following:
  - each line parses into its kind, condition, code, number, tag, keyword,
    code name and argument, text and rest
  - case is kept as sent, while the condition and code are recognized in
    any case
*/
bool
d_tests_sa_imap_responses(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(RESPONSES) / sizeof(RESPONSES[0]);
    bool         result = true;

    // each line
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_response_row(&RESPONSES[i]),
                                    "d_imap_response_parse",
                                    RESPONSES[i].line,
                                    _counter) && result;
    }

    return result;
}

/*
d_tests_sa_imap_response_refusals
  Tests the following:
  - what is no response head is refused, the response left untouched
  - NULL and data-less input are refused
*/
bool
d_tests_sa_imap_response_refusals(
    struct d_test_counter* _counter
)
{
    struct d_imap_response   response = { .kind = 7, .number = 7u };
    const struct d_pack_text dataless = { NULL, 3u };
    bool                     result   = true;

    // each line, over a response that must survive
    for (size_t i = 0u; i < (sizeof(REFUSED) / sizeof(REFUSED[0])); ++i)
    {
        const enum d_imap_error error =
            d_imap_response_parse(d_pack_text_from_cstr(REFUSED[i].input),
                                  &response);

        result = d_tests_net_assert( (error == REFUSED[i].error) &&
                                     (response.kind == 7)        &&
                                     (response.number == 7u),
                                    "d_imap_response_parse",
                                    REFUSED[i].what,
                                    _counter) && result;
    }

    const bool nulls =
        ( (d_imap_response_parse(d_pack_text_from_cstr("A1 OK x\r\n"),
                                 NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_response_parse(dataless,
                                 &response) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(nulls,
                                "d_imap_response_parse",
                                "refuses NULL, and text without data",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_render_octets
  Appends `_size` octets, spelling CR and LF as \r and \n and every other
octet outside printable ASCII as \xHH, and keeping what fits.
*/
static void
d_tests_imap_render_octets(
    struct d_tests_imap_rendering* _rendering,
    const char*                    _data,
    size_t                         _size
)
{
    static const char HEX[] = "0123456789abcdef";

    // each octet, escaped where it would not print
    for (size_t i = 0u; i < _size; ++i)
    {
        const unsigned char octet      = (unsigned char)_data[i];
        char                spelled[4] = { (char)octet, '\0', '\0', '\0' };
        size_t              width      = 1u;

        // CR and LF by name, other unprintable octets in hex
        if ( (octet == '\r') ||
             (octet == '\n') )
        {
            spelled[0] = '\\';
            spelled[1] = (octet == '\r') ? 'r' : 'n';
            width      = 2u;
        }
        else if ( (octet < 0x20u) ||
                  (octet > 0x7Eu) )
        {
            spelled[0] = '\\';
            spelled[1] = 'x';
            spelled[2] = HEX[octet >> 4];
            spelled[3] = HEX[octet & 0x0Fu];
            width      = 4u;
        }

        // only what fits, the text kept terminated
        if ((_rendering->length + width) < sizeof(_rendering->text))
        {
            memcpy(_rendering->text + _rendering->length,
                   spelled,
                   width);
            _rendering->length += width;
            _rendering->text[_rendering->length] = '\0';
        }
    }

    return;
}

/*
d_tests_imap_render_text
  Appends a C string, as d_tests_imap_render_octets would.
*/
static void
d_tests_imap_render_text(
    struct d_tests_imap_rendering* _rendering,
    const char*                    _text
)
{
    d_tests_imap_render_octets(_rendering,
                               _text,
                               strlen(_text));

    return;
}

/*
d_tests_imap_render_token
  Appends one token, after a space unless it is the first.
*/
static void
d_tests_imap_render_token(
    struct d_tests_imap_rendering* _rendering,
    const struct d_imap_token*     _token
)
{
    static const char* const SPELLINGS[] =
    {
        "end", "nl", "a<", "n<", "nil", "q<", "l", "f<", "(", ")", "[", "]",
        "*"
    };
    const bool texted  = ( (_token->kind == D_IMAP_TOKEN_ATOM)    ||
                           (_token->kind == D_IMAP_TOKEN_NUMBER)  ||
                           (_token->kind == D_IMAP_TOKEN_QUOTED)  ||
                           (_token->kind == D_IMAP_TOKEN_LITERAL) ||
                           (_token->kind == D_IMAP_TOKEN_FLAG) );
    char       head[48] = { 0 };

    // a literal's count and flags; an escaped string's mark; the others
    if (_token->kind == D_IMAP_TOKEN_LITERAL)
    {
        (void)snprintf(head,
                       sizeof(head),
                       "l%llu%s%s<",
                       (unsigned long long)_token->number,
                       ((_token->flags & D_IMAP_TOKEN_NONSYNC) != 0u)
                           ? "+"
                           : "",
                       ((_token->flags & D_IMAP_TOKEN_BINARY) != 0u)
                           ? "~"
                           : "");
    }
    else if ( (_token->kind == D_IMAP_TOKEN_QUOTED) &&
              ((_token->flags & D_IMAP_TOKEN_ESCAPED) != 0u) )
    {
        (void)snprintf(head,
                       sizeof(head),
                       "q!<");
    }
    else if ( (_token->kind >= 0) &&
              (_token->kind <= D_IMAP_TOKEN_STAR) )
    {
        (void)snprintf(head,
                       sizeof(head),
                       "%s",
                       SPELLINGS[_token->kind]);
    }

    d_tests_imap_render_text(_rendering,
                             (_rendering->length > 0u) ? " " : "");
    d_tests_imap_render_text(_rendering,
                             head);

    // the text of a token that has one, and a number's value
    if (texted)
    {
        d_tests_imap_render_octets(_rendering,
                                   _token->text.data,
                                   _token->text.length);
        d_tests_imap_render_text(_rendering,
                                 ">");

        // a number's value after its digits
        if (_token->kind == D_IMAP_TOKEN_NUMBER)
        {
            (void)snprintf(head,
                           sizeof(head),
                           "#%llu",
                           (unsigned long long)_token->number);
            d_tests_imap_render_text(_rendering,
                                     head);
        }
    }

    return;
}

/*
d_tests_imap_render
  Lexes all of `_input` with `_flags` into `_rendering`, to the end or the
first error, which it spells errN@position.
*/
static void
d_tests_imap_render(
    struct d_tests_imap_rendering* _rendering,
    struct d_pack_text             _input,
    uint32_t                       _flags
)
{
    struct d_imap_lexer lexer = { NULL, 0u, 0u };

    _rendering->length  = 0u;
    _rendering->text[0] = '\0';
    d_imap_lexer_init(&lexer,
                      _input);

    // token by token, sixty-four at most
    for (size_t n = 0u; n < 64u; ++n)
    {
        struct d_imap_token     token = { .kind = 0 };
        const enum d_imap_error error = d_imap_lexer_next(&lexer,
                                                          _flags,
                                                          &token);
        char                    fault[32] = { 0 };

        // an error ends the stream where the lexer stopped
        if (error != D_IMAP_OK)
        {
            (void)snprintf(fault,
                           sizeof(fault),
                           "%serr%d@%zu",
                           (_rendering->length > 0u) ? " " : "",
                           (int)error,
                           lexer.position);
            d_tests_imap_render_text(_rendering,
                                     fault);

            return;
        }

        d_tests_imap_render_token(_rendering,
                                  &token);

        // the end of the input ends the stream
        if (token.kind == D_IMAP_TOKEN_END)
        {
            return;
        }
    }

    return;
}

/*
d_tests_sa_imap_lexer
  Tests the following:
  - each input lexes into its token stream, spelled out whole
  - malformed input is refused where it stands, the position unmoved
  - the unread input is a view; NULL lexers and tokens are refused
*/
bool
d_tests_sa_imap_lexer(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_rendering rendering = { .length = 0u };
    struct d_imap_lexer           lexer     = { NULL, 0u, 0u };
    struct d_imap_token           token     = { .kind = 0 };
    bool                          result    = true;

    // each input, its stream compared whole
    for (size_t i = 0u; i < (sizeof(LEXES) / sizeof(LEXES[0])); ++i)
    {
        const size_t length = (LEXES[i].length != 0u)
                                  ? LEXES[i].length
                                  : strlen(LEXES[i].input);

        d_tests_imap_render(&rendering,
                            d_pack_text_from_span(LEXES[i].input,
                                                  length),
                            LEXES[i].flags);
        result = d_tests_net_assert((strcmp(rendering.text,
                                            LEXES[i].tokens) == 0),
                                    "d_imap_lexer_next",
                                    LEXES[i].what,
                                    _counter) && result;
    }

    d_imap_lexer_init(NULL,
                      d_pack_text_from_cstr("x"));
    d_imap_lexer_init(&lexer,
                      d_pack_text_from_cstr("A B C"));
    (void)d_imap_lexer_next(&lexer,
                            0u,
                            &token);
    (void)d_imap_lexer_next(&lexer,
                            0u,
                            &token);

    const bool rest =
        ( (d_tests_imap_span_is(d_imap_lexer_rest(&lexer),
                                " C"))                           &&
          (d_imap_lexer_rest(NULL).length == 0u)                 &&
          (d_imap_lexer_next(NULL,
                             0u,
                             &token) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_lexer_next(&lexer,
                             0u,
                             NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(rest,
                                "d_imap_lexer_rest",
                                "views the unread input; NULL is refused",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_partial_row
  A row lexed once: INCOMPLETE, the position unmoved, and the partial token
described as the row has it.
*/
static bool
d_tests_imap_partial_row(
    const struct d_tests_imap_partial_case* _row
)
{
    struct d_imap_lexer lexer = { NULL, 0u, 0u };
    struct d_imap_token token = { .kind = 0 };

    d_imap_lexer_init(&lexer,
                      d_pack_text_from_cstr(_row->input));

    const enum d_imap_error error = d_imap_lexer_next(&lexer,
                                                      0u,
                                                      &token);

    return ( (error == D_IMAP_ERROR_INCOMPLETE) &&
             (lexer.position == 0u)             &&
             ( (_row->kind < 0) ||
               ( (token.kind == _row->kind)     &&
                 (token.number == _row->number) &&
                 (d_tests_imap_span_is(token.text,
                                       _row->present)) ) ) );
}

/*
d_tests_imap_literal_arrives
  FETCH data whose literal arrives an octet at a time, lexed from a lexer
saved before it: each partial input is INCOMPLETE, the position kept and
the literal described -- its count, and the octets present -- until the
last octet completes it.
*/
static bool
d_tests_imap_literal_arrives(void)
{
    static const char   DATA[] = "(UID 7 BODY[] {11}\r\nHello world)\r\n";
    const size_t        head   = 20u;
    struct d_imap_lexer lexer  = { NULL, 0u, 0u };
    struct d_imap_token token  = { .kind = 0 };
    bool                held   = true;

    d_imap_lexer_init(&lexer,
                      d_pack_text_from_span(DATA,
                                            sizeof(DATA) - 1u));

    // the six tokens before the literal: ( UID 7 BODY [ ]
    for (int i = 0; i < 6; ++i)
    {
        held = ( (d_imap_lexer_next(&lexer,
                                    0u,
                                    &token) == D_IMAP_OK) &&
                 (held) );
    }

    const struct d_imap_lexer saved = lexer;

    // each octet of the literal arriving
    for (size_t present = 0u; (held) && (present < 11u); ++present)
    {
        struct d_imap_lexer partial = saved;

        partial.length = head + present;
        held = ( (d_imap_lexer_next(&partial,
                                    0u,
                                    &token) == D_IMAP_ERROR_INCOMPLETE) &&
                 (partial.position == saved.position)                   &&
                 (token.kind == D_IMAP_TOKEN_LITERAL)                   &&
                 (token.number == 11u)                                  &&
                 (token.text.length == present) );
    }

    return ( (held)                                      &&
             (d_imap_lexer_next(&lexer,
                                0u,
                                &token) == D_IMAP_OK)    &&
             (token.kind == D_IMAP_TOKEN_LITERAL)        &&
             (d_tests_imap_span_is(token.text,
                                   "Hello world")) );
}

/*
d_tests_sa_imap_lexer_incomplete
  Tests the following:
  - input that stops short of a token is INCOMPLETE, the position unmoved
  - a literal stopped short is described: its count and the octets here
  - a literal arriving an octet at a time completes with its last octet
*/
bool
d_tests_sa_imap_lexer_incomplete(
    struct d_test_counter* _counter
)
{
    bool result = true;

    // each input stopping short
    for (size_t i = 0u; i < (sizeof(PARTIALS) / sizeof(PARTIALS[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_partial_row(&PARTIALS[i]),
                                    "d_imap_lexer_next",
                                    PARTIALS[i].what,
                                    _counter) && result;
    }

    result = d_tests_net_assert(d_tests_imap_literal_arrives(),
                                "d_imap_lexer_next",
                                "a literal completes with its last octet",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_decode_row
  A row's token, lexed as an astring for a mailbox name, decoded; the
octets are compared on success.
*/
static bool
d_tests_imap_decode_row(
    const struct d_tests_imap_decode_case* _row,
    bool                                   _mailbox
)
{
    struct d_imap_lexer         lexer   = { NULL, 0u, 0u };
    struct d_imap_token         token   = { .kind = 0 };
    struct d_tests_imap_capture capture = { .length = 0u };

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));
    d_imap_lexer_init(&lexer,
                      d_pack_text_from_cstr(_row->input));
    (void)d_imap_lexer_next(&lexer,
                            (_mailbox) ? D_IMAP_LEX_ASTRING : 0u,
                            &token);

    const struct d_pack_sink sink  = d_tests_imap_capture_sink(&capture);
    const enum d_imap_error  error =
        (_mailbox)
            ? d_imap_mailbox_decode(sink,
                                    &token,
                                    _row->flags)
            : d_imap_string_decode(sink,
                                   &token);

    return ( (error == _row->error) &&
             ( (error != D_IMAP_OK) ||
               (d_tests_imap_capture_is(&capture,
                                        _row->output)) ) );
}

/*
d_tests_sa_imap_decode
  Tests the following:
  - string tokens decode to their values; others are refused
  - mailbox-name tokens decode from modified UTF-7, or validate as UTF-8
    under UTF8=ACCEPT
  - NULL tokens and deaf sinks are refused
*/
bool
d_tests_sa_imap_decode(
    struct d_test_counter* _counter
)
{
    const struct d_imap_token token  = { 0u, D_IMAP_TOKEN_ATOM, 0u,
                                         { "a", 1u } };
    const struct d_pack_sink  deaf   = { NULL, NULL };
    bool                      result = true;

    const size_t strings = sizeof(STRING_VALUES) / sizeof(STRING_VALUES[0]);

    // each string, then each mailbox name
    for (size_t i = 0u; i < strings; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_decode_row(&STRING_VALUES[i],
                                                            false),
                                    "d_imap_string_decode",
                                    STRING_VALUES[i].what,
                                    _counter) && result;
    }

    const size_t mailboxes = sizeof(MAILBOX_VALUES) / sizeof(MAILBOX_VALUES[0]);

    for (size_t i = 0u; i < mailboxes; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_decode_row(&MAILBOX_VALUES[i],
                                                            true),
                                    "d_imap_mailbox_decode",
                                    MAILBOX_VALUES[i].what,
                                    _counter) && result;
    }

    const bool refused =
        ( (d_imap_string_decode(deaf,
                                &token) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_mailbox_decode(deaf,
                                 &token,
                                 0u) == D_IMAP_ERROR_INVALID_ARGUMENT)    &&
          (d_imap_string_decode(d_tests_imap_capture_sink(NULL),
                                NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(refused,
                                "d_imap_string_decode",
                                "refuses a deaf sink and a NULL token",
                                _counter) && result;

    return result;
}
