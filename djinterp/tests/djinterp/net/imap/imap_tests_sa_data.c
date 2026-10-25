/*******************************************************************************
* djinterp [net]                                            imap_tests_sa_data.c
*
* Tests of the IMAP kernel's response-data parsers: capability lists, flag
* and mailbox-attribute lists, STATUS items, SEARCH results and LIST
* entries, and the two writers that go with them.
*   Each parser meets the examples of RFC 9051 and the extensions it names,
* names outside its vocabulary, which it counts rather than refuses, and
* input it must refuse; a parser that promises to leave its output untouched
* on failure is held to it.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_data.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <string.h>  // strcmp


// d_tests_imap_capabilities_case
//   struct: a capability list, and what parsing it gives: the error, and on
// success the set, the AUTH= mechanisms, the count of names outside the
// vocabulary and the APPENDLIMIT.
struct d_tests_imap_capabilities_case
{
    const char*       list;
    enum d_imap_error error;
    uint64_t          set;
    uint32_t          auth;
    uint32_t          unknown;
    uint64_t          limit;
    const char*       what;
};

// d_tests_imap_set_case
//   struct: a parenthesized list and what parsing it gives: the error, and on
// success the set of known names and the count of others.
struct d_tests_imap_set_case
{
    const char*       list;
    enum d_imap_error error;
    uint32_t          set;
    uint32_t          unknown;
    const char*       what;
};

// d_tests_imap_write_case
//   struct: a set written as a list: the error, and on success the octets.
struct d_tests_imap_write_case
{
    uint32_t          set;
    enum d_imap_error error;
    const char*       output;
};

// d_tests_imap_status_case
//   struct: a STATUS item list, and the error and items parsing it gives.
struct d_tests_imap_status_case
{
    const char*               list;
    enum d_imap_error         error;
    struct d_imap_status_data data;
    const char*               what;
};

// d_tests_imap_search_case
//   struct: SEARCH data, and the error, results and MODSEQ it gives.
struct d_tests_imap_search_case
{
    const char*       data;
    enum d_imap_error error;
    size_t            count;
    uint32_t          ids[3];
    uint64_t          modseq;
    const char*       what;
};

// d_tests_imap_list_case
//   struct: LIST data, and the error and, on success, the entry it gives.
struct d_tests_imap_list_case
{
    const char*       data;
    enum d_imap_error error;
    uint32_t          attributes;
    uint32_t          unknown;
    int32_t           delimiter;
    int               kind;
    const char*       name;
    const char*       extended;
    const char*       what;
};

/*
CAPABILITY_LISTS
  Dovecot's greeting, the extensions whose names hold "=", the vocabulary's
lookup ignoring case, and what the parser refuses.
*/
static const struct d_tests_imap_capabilities_case CAPABILITY_LISTS[] =
{
    { "IMAP4rev1 SASL-IR LOGIN-REFERRALS ID ENABLE IDLE LITERAL+ STARTTLS "
      "AUTH=PLAIN AUTH=LOGIN", D_IMAP_OK,
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_IMAP4REV1)    |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_SASL_IR)      |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_ID)           |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_ENABLE)       |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_IDLE)         |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_LITERAL_PLUS) |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_STARTTLS),
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_PLAIN) | D_IMAP_AUTH_BIT(D_IMAP_AUTH_LOGIN),
      1u, 0u, "Dovecot's greeting; LOGIN-REFERRALS is unknown" },
    { "IMAP4rev1 APPENDLIMIT=35651584\r\n", D_IMAP_OK,
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_IMAP4REV1) |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_APPENDLIMIT),
      0u, 0u, 35651584u, "APPENDLIMIT=n, and a trailing CRLF" },
    { "APPENDLIMIT", D_IMAP_OK,
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_APPENDLIMIT),
      0u, 0u, 0u, "APPENDLIMIT alone sets no limit" },
    { "APPENDLIMIT=9223372036854775807", D_IMAP_OK,
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_APPENDLIMIT),
      0u, 0u, D_IMAP_NUMBER64_MAX, "up to the largest number64" },
    { "THREAD=REFERENCES THREAD=ORDEREDSUBJECT COMPRESS=DEFLATE UTF8=ACCEPT",
      D_IMAP_OK,
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_THREAD_REFERENCES)      |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_THREAD_ORDEREDSUBJECT)  |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_COMPRESS_DEFLATE)       |
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_UTF8_ACCEPT),
      0u, 0u, 0u, "names with = in them" },
    { "imap4REV1 auth=plain AUTH=FOO X-GM-EXT-1", D_IMAP_OK,
      D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_IMAP4REV1),
      D_IMAP_AUTH_BIT(D_IMAP_AUTH_PLAIN), 2u, 0u,
      "any case; an unknown mechanism and extension counted" },
    { "", D_IMAP_OK, 0u, 0u, 0u, 0u,
      "an empty list" },
    { "APPENDLIMIT=x", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0u, 0u,
      "an APPENDLIMIT that is no number" },
    { "APPENDLIMIT=", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0u, 0u,
      "or that is missing" },
    { "APPENDLIMIT=9223372036854775808", D_IMAP_ERROR_RANGE, 0u, 0u, 0u, 0u,
      "or that is too large" },
    { "IMAP4rev1 (IDLE)", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0u, 0u,
      "an entry that is no atom" },
    { "IMAP4rev1 \"x\"", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0u, 0u,
      "nor a quoted one" }
};

/*
FLAG_LISTS, FLAG_WRITES
  RFC 9051's FLAGS example, the keywords, \* and unknown flags; what is no
flag list; and the writer's enumeration order.
*/
static const struct d_tests_imap_set_case FLAG_LISTS[] =
{
    { "(\\Answered \\Flagged \\Deleted \\Seen \\Draft)", D_IMAP_OK,
      0x1Fu, 0u, "RFC 9051's FLAGS example" },
    { "(\\Seen $Forwarded $Junk Custom \\*)", D_IMAP_OK,
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN)      |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_FORWARDED) |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_JUNK)      |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_WILDCARD),
      1u, "keywords and \\*; Custom is counted" },
    { "(\\Recent $MDNSent $NotJunk $Phishing)", D_IMAP_OK,
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_RECENT)   |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_MDNSENT)  |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_NOTJUNK)  |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_PHISHING),
      0u, "\\Recent and the other keywords" },
    { "()", D_IMAP_OK, 0u, 0u, "an empty list" },
    { "(\\seen)\r\n", D_IMAP_OK, D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN), 0u,
      "any case, and a trailing CRLF" },
    { "(\\Unknownflag)", D_IMAP_OK, 0u, 1u,
      "a flag outside the vocabulary is counted" },
    { "(\\Seen \\Seen)", D_IMAP_OK, D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN), 0u,
      "a repeat is one flag" },
    { "(\\Seen", D_IMAP_ERROR_MALFORMED, 0u, 0u, "an unclosed list" },
    { "\\Seen", D_IMAP_ERROR_MALFORMED, 0u, 0u, "no list at all" },
    { "(\\Seen)x", D_IMAP_ERROR_MALFORMED, 0u, 0u, "something after it" },
    { "(\"q\")", D_IMAP_ERROR_MALFORMED, 0u, 0u, "a string in it" },
    { "((\\Seen))", D_IMAP_ERROR_MALFORMED, 0u, 0u, "a list in it" },
    { "(\\)", D_IMAP_ERROR_MALFORMED, 0u, 0u, "a backslash alone" }
};
static const struct d_tests_imap_write_case FLAG_WRITES[] =
{
    { D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN) |
      D_IMAP_FLAG_BIT(D_IMAP_FLAG_DELETED), D_IMAP_OK,
      "(\\Seen \\Deleted)" },
    { 0u, D_IMAP_OK, "()" },
    { 0xFFFu, D_IMAP_OK,
      "(\\Seen \\Answered \\Flagged \\Deleted \\Draft \\Recent \\* "
      "$Forwarded $MDNSent $Junk $NotJunk $Phishing)" },
    { 1u << D_IMAP_FLAG_COUNT, D_IMAP_ERROR_INVALID_ARGUMENT, "" }
};

/*
ATTRIBUTE_LISTS
  LIST attributes: RFC 3501's, CHILDREN's, LIST-EXTENDED's and the special
uses, in any case; an extension, and a message flag, counted.
*/
static const struct d_tests_imap_set_case ATTRIBUTE_LISTS[] =
{
    { "(\\HasNoChildren \\Sent)", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASNOCHILDREN) |
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_SENT),
      0u, "a special use" },
    { "(\\Noselect \\HasChildren \\XFoo)", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_NOSELECT) |
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASCHILDREN),
      1u, "an extension, counted" },
    { "()", D_IMAP_OK, 0u, 0u, "none" },
    { "(\\noinferiors \\marked \\unmarked \\nonexistent \\subscribed "
      "\\remote \\all \\archive \\drafts \\flagged \\junk \\trash "
      "\\important)", D_IMAP_OK, 0x1BFCDu, 0u,
      "every other attribute, in lower case" },
    { "(\\Seen)", D_IMAP_OK, 0u, 1u,
      "a message flag is no mailbox attribute" },
    { "(\\HasNoChildren", D_IMAP_ERROR_MALFORMED, 0u, 0u,
      "an unclosed list" }
};

/*
STATUS_LISTS, ITEM_WRITES
  Every item, an extension's of each shape skipped, and the items' widths:
32 bits for counts, 63 for SIZE and HIGHESTMODSEQ.
*/
static const struct d_tests_imap_status_case STATUS_LISTS[] =
{
    { "(MESSAGES 231 UIDNEXT 44292 UIDVALIDITY 1 UNSEEN 3 SIZE 123456789012 "
      "HIGHESTMODSEQ 7011231777 RECENT 0 DELETED 5)", D_IMAP_OK,
      { 123456789012u, 7011231777u, 0xFFu, 231u, 0u, 44292u, 1u, 3u, 5u },
      "every item" },
    { "(MESSAGES 231 X-GUID \"abc\" X-LIST (a (b)) X-NIL NIL "
      "X-LIT {3}\r\nabc)", D_IMAP_OK,
      { 0u, 0u, 0x01u, 231u, 0u, 0u, 0u, 0u, 0u },
      "an extension's item of any shape is skipped" },
    { "()", D_IMAP_OK,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "no items" },
    { "(messages 7)\r\n", D_IMAP_OK,
      { 0u, 0u, 0x01u, 7u, 0u, 0u, 0u, 0u, 0u },
      "any case, and a trailing CRLF" },
    { "(HIGHESTMODSEQ 9223372036854775807)", D_IMAP_OK,
      { 0u, D_IMAP_NUMBER64_MAX, 0x80u, 0u, 0u, 0u, 0u, 0u, 0u },
      "the largest mod-sequence" },
    { "(MESSAGES 4294967296)", D_IMAP_ERROR_RANGE,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "a count past 32 bits" },
    { "(SIZE 9223372036854775808)", D_IMAP_ERROR_RANGE,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "a size past number64" },
    { "(MESSAGES)", D_IMAP_ERROR_MALFORMED,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "an item without its number" },
    { "(MESSAGES NIL)", D_IMAP_ERROR_MALFORMED,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "or with NIL for it" },
    { "(MESSAGES -1)", D_IMAP_ERROR_MALFORMED,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "or a negative one" },
    { "MESSAGES 1", D_IMAP_ERROR_MALFORMED,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "no list" },
    { "(MESSAGES 1)x", D_IMAP_ERROR_MALFORMED,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "something after it" },
    { "(MESSAGES 1", D_IMAP_ERROR_INCOMPLETE,
      { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
      "a list cut short" }
};
static const struct d_tests_imap_write_case ITEM_WRITES[] =
{
    { D_IMAP_STATUS_BIT(D_IMAP_STATUS_MESSAGES) |
      D_IMAP_STATUS_BIT(D_IMAP_STATUS_UIDNEXT)  |
      D_IMAP_STATUS_BIT(D_IMAP_STATUS_UNSEEN), D_IMAP_OK,
      "(MESSAGES UIDNEXT UNSEEN)" },
    { 0xFFu, D_IMAP_OK,
      "(MESSAGES RECENT UIDNEXT UIDVALIDITY UNSEEN DELETED SIZE "
      "HIGHESTMODSEQ)" },
    { 0u, D_IMAP_ERROR_INVALID_ARGUMENT, "" },
    { 1u << D_IMAP_STATUS_ITEM_COUNT, D_IMAP_ERROR_INVALID_ARGUMENT, "" }
};

/*
SEARCHES
  RFC 9051's example, CONDSTORE's trailing MODSEQ, and what is no result.
*/
static const struct d_tests_imap_search_case SEARCHES[] =
{
    { "2 84 882", D_IMAP_OK, 3u, { 2u, 84u, 882u }, 0u,
      "RFC 9051's example" },
    { "", D_IMAP_OK, 0u, { 0u, 0u, 0u }, 0u,
      "no message matched" },
    { "2 84 882 (MODSEQ 917162500)", D_IMAP_OK, 3u, { 2u, 84u, 882u },
      917162500u, "CONDSTORE's MODSEQ" },
    { "1 2 3\r\n", D_IMAP_OK, 3u, { 1u, 2u, 3u }, 0u,
      "a trailing CRLF" },
    { "0", D_IMAP_ERROR_RANGE, 0u, { 0u, 0u, 0u }, 0u,
      "0 is no message" },
    { "4294967296", D_IMAP_ERROR_RANGE, 0u, { 0u, 0u, 0u }, 0u,
      "nor a number past 32 bits" },
    { "2 (MODSEQ 9223372036854775808)", D_IMAP_ERROR_RANGE, 0u,
      { 0u, 0u, 0u }, 0u, "nor a MODSEQ past number64" },
    { "2 x", D_IMAP_ERROR_MALFORMED, 0u, { 0u, 0u, 0u }, 0u,
      "an atom among the numbers" },
    { "2 (MODSEQ x)", D_IMAP_ERROR_MALFORMED, 0u, { 0u, 0u, 0u }, 0u,
      "a MODSEQ that is no number" },
    { "2 (FOO 1)", D_IMAP_ERROR_MALFORMED, 0u, { 0u, 0u, 0u }, 0u,
      "a list other than MODSEQ" },
    { "2 (MODSEQ 5) 3", D_IMAP_ERROR_MALFORMED, 0u, { 0u, 0u, 0u }, 0u,
      "a number after MODSEQ" }
};

/*
LISTINGS
  Names as atom, quoted string, number and literal; NIL and escaped
delimiters; LIST-EXTENDED data; and what is no listing.
*/
static const struct d_tests_imap_list_case LISTINGS[] =
{
    { "(\\HasNoChildren) \"/\" INBOX", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASNOCHILDREN), 0u, '/',
      D_IMAP_TOKEN_ATOM, "INBOX", "", "an atom name" },
    { "(\\Noselect) NIL \"\"", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_NOSELECT), 0u, -1,
      D_IMAP_TOKEN_QUOTED, "", "", "no delimiter; an empty quoted name" },
    { "() \".\" {8}\r\nMy\"Box!x", D_IMAP_OK, 0u, 0u, '.',
      D_IMAP_TOKEN_LITERAL, "My\"Box!x", "", "a literal name" },
    { "(\\HasChildren) \"/\" \"[Gmail]\"", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASCHILDREN), 0u, '/',
      D_IMAP_TOKEN_QUOTED, "[Gmail]", "", "a quoted name" },
    { "(\\HasNoChildren \\Sent) \"/\" [Gmail]/Sent", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASNOCHILDREN) |
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_SENT), 0u, '/',
      D_IMAP_TOKEN_ATOM, "[Gmail]/Sent", "", "brackets in an atom name" },
    { "(\\Subscribed) \"/\" \"Foo\" (\"CHILDINFO\" (\"SUBSCRIBED\"))",
      D_IMAP_OK, D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_SUBSCRIBED), 0u, '/',
      D_IMAP_TOKEN_QUOTED, "Foo", "(\"CHILDINFO\" (\"SUBSCRIBED\"))",
      "LIST-EXTENDED data after the name" },
    { "(\\X) \"/\" 123", D_IMAP_OK, 0u, 1u, '/',
      D_IMAP_TOKEN_NUMBER, "123", "", "an unknown attribute; a number" },
    { "(\\HasNoChildren) \"\\\\\" \"a\\\\b\"", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASNOCHILDREN), 0u, '\\',
      D_IMAP_TOKEN_QUOTED, "a\\\\b", "",
      "an escaped delimiter; the name kept escaped" },
    { "(\\HasNoChildren) \"/\" INBOX\r\n", D_IMAP_OK,
      D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASNOCHILDREN), 0u, '/',
      D_IMAP_TOKEN_ATOM, "INBOX", "", "a trailing CRLF" },
    { "() \"/\" ~peter/mail/&U,BTFw-", D_IMAP_OK, 0u, 0u, '/',
      D_IMAP_TOKEN_ATOM, "~peter/mail/&U,BTFw-", "",
      "a name left in modified UTF-7" },
    { "() \".\" {8}\r\nMy", D_IMAP_ERROR_INCOMPLETE, 0u, 0u, 0,
      0, "", "", "a literal name not all here" },
    { "(\\Noselect) \"//\" x", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0,
      0, "", "", "a delimiter of two octets" },
    { "(\\HasNoChildren) \"/\"", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0,
      0, "", "", "no name" },
    { "\"/\" INBOX", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0,
      0, "", "", "no attributes" },
    { "(\\HasNoChildren) \"/\" NIL", D_IMAP_ERROR_MALFORMED, 0u, 0u, 0,
      0, "", "", "a NIL name" }
};

/*
d_tests_imap_capabilities_row
  A row parsed over a sentinel: on success the parse must match, and on
failure the sentinel must survive, as the header promises.
*/
static bool
d_tests_imap_capabilities_row(
    const struct d_tests_imap_capabilities_case* _row
)
{
    struct d_imap_capabilities caps = { 7u, 7u, 7u, 7u };

    const enum d_imap_error error =
        d_imap_capabilities_parse(d_pack_text_from_cstr(_row->list),
                                  &caps);

    // a failure leaves the set as it was
    if (error != D_IMAP_OK)
    {
        return ( (error == _row->error) &&
                 (caps.set == 7u)       &&
                 (caps.append_limit == 7u) );
    }

    return ( (_row->error == D_IMAP_OK)      &&
             (caps.set == _row->set)         &&
             (caps.auth == _row->auth)       &&
             (caps.unknown == _row->unknown) &&
             (caps.append_limit == _row->limit) );
}

/*
d_tests_sa_imap_capabilities
  Tests the following:
  - each list parses into its set, mechanisms, unknowns and APPENDLIMIT
  - a parse replaces the set it is given, rather than adding to it
  - the queries answer for a parsed set, and refuse what is outside it
*/
bool
d_tests_sa_imap_capabilities(
    struct d_test_counter* _counter
)
{
    struct d_imap_capabilities caps   = { 0u, 0u, 0u, 0u };
    const size_t               count  = sizeof(CAPABILITY_LISTS) /
                                        sizeof(CAPABILITY_LISTS[0]);
    bool                       result = true;

    // each list
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(
                     d_tests_imap_capabilities_row(&CAPABILITY_LISTS[i]),
                     "d_imap_capabilities_parse",
                     CAPABILITY_LISTS[i].what,
                     _counter) && result;
    }

    (void)d_imap_capabilities_parse(d_pack_text_from_cstr("IMAP4rev1 MOVE"),
                                    &caps);

    const bool replaced =
        ( (d_imap_capabilities_parse(d_pack_text_from_cstr("IDLE"),
                                     &caps) == D_IMAP_OK) &&
          (caps.set ==
           D_IMAP_CAPABILITY_BIT(D_IMAP_CAPABILITY_IDLE)) );

    (void)d_imap_capabilities_parse(
              d_pack_text_from_cstr(CAPABILITY_LISTS[0].list),
              &caps);

    const bool queried =
        ( (d_imap_capabilities_has(caps,
                                   D_IMAP_CAPABILITY_IDLE))           &&
          (!d_imap_capabilities_has(caps,
                                    D_IMAP_CAPABILITY_MOVE))          &&
          (!d_imap_capabilities_has(caps,
                                    (enum d_imap_capability)64))      &&
          (d_imap_capabilities_has_auth(caps,
                                        D_IMAP_AUTH_PLAIN))           &&
          (!d_imap_capabilities_has_auth(caps,
                                         D_IMAP_AUTH_LOGIN_COMMAND))  &&
          (d_imap_capabilities_parse(d_pack_text_from_cstr("IDLE"),
                                     NULL) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(replaced,
                                "d_imap_capabilities_parse",
                                "replaces the set rather than adding to it",
                                _counter) && result;
    result = d_tests_net_assert(queried,
                                "d_imap_capabilities_has",
                                "answers for the set, nothing outside it",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_set_row
  A flag list, or with `_attributes` a LIST attribute list, parsed; the set
and count are compared on success.
*/
static bool
d_tests_imap_set_row(
    const struct d_tests_imap_set_case* _row,
    bool                                _attributes
)
{
    uint32_t                 set     = 0u;
    uint32_t                 unknown = 0u;
    const struct d_pack_text list    = d_pack_text_from_cstr(_row->list);
    const enum d_imap_error  error   =
        (_attributes)
            ? d_imap_mailbox_attributes_parse(list,
                                              &set,
                                              &unknown)
            : d_imap_flags_parse(list,
                                 &set,
                                 &unknown);

    return ( (error == _row->error) &&
             ( (error != D_IMAP_OK) ||
               ( (set == _row->set) &&
                 (unknown == _row->unknown) ) ) );
}

/*
d_tests_imap_write_row
  A set written as a flag list, or with `_items` as a STATUS item list; the
octets are compared on success.
*/
static bool
d_tests_imap_write_row(
    const struct d_tests_imap_write_case* _row,
    bool                                  _items
)
{
    struct d_tests_imap_capture capture = { .length = 0u };

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const struct d_pack_sink sink  = d_tests_imap_capture_sink(&capture);
    const enum d_imap_error  error =
        (_items)
            ? d_imap_status_items_write(sink,
                                        _row->set)
            : d_imap_flags_write(sink,
                                 _row->set);

    return ( (error == _row->error) &&
             ( (error != D_IMAP_OK) ||
               (d_tests_imap_capture_is(&capture,
                                        _row->output)) ) );
}

/*
d_tests_sa_imap_flags
  Tests the following:
  - flag lists parse into known flags and a count of others
  - what is no flag list is refused, and the set is required
  - a set is written in enumeration order; an unknown bit is refused
*/
bool
d_tests_sa_imap_flags(
    struct d_test_counter* _counter
)
{
    uint32_t set     = 0u;
    bool     written = true;
    bool     result  = true;

    // each list
    for (size_t i = 0u; i < (sizeof(FLAG_LISTS) / sizeof(FLAG_LISTS[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_set_row(&FLAG_LISTS[i],
                                                         false),
                                    "d_imap_flags_parse",
                                    FLAG_LISTS[i].what,
                                    _counter) && result;
    }

    // each set written
    for (size_t i = 0u; i < (sizeof(FLAG_WRITES) / sizeof(FLAG_WRITES[0])); ++i)
    {
        written = ( (d_tests_imap_write_row(&FLAG_WRITES[i],
                                            false)) &&
                    (written) );
    }

    const bool optional =
        ( (d_imap_flags_parse(d_pack_text_from_cstr("(\\Seen x)"),
                              &set,
                              NULL) == D_IMAP_OK)             &&
          (set == D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN))          &&
          (d_imap_flags_parse(d_pack_text_from_cstr("()"),
                              NULL,
                              NULL) ==
           D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(written,
                                "d_imap_flags_write",
                                "in enumeration order; no unknown bit",
                                _counter) && result;
    result = d_tests_net_assert(optional,
                                "d_imap_flags_parse",
                                "the count is optional, the set is not",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_mailbox_attributes
  Tests the following:
  - LIST attribute lists parse into known attributes and a count of others
  - D_IMAP_MAILBOX_SPECIAL_USE holds exactly the special uses
*/
bool
d_tests_sa_imap_mailbox_attributes(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(ATTRIBUTE_LISTS) /
                          sizeof(ATTRIBUTE_LISTS[0]);
    uint32_t     set    = 0u;
    bool         result = true;

    // each list
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_set_row(&ATTRIBUTE_LISTS[i],
                                                         true),
                                    "d_imap_mailbox_attributes_parse",
                                    ATTRIBUTE_LISTS[i].what,
                                    _counter) && result;
    }

    const bool special =
        ( (D_IMAP_MAILBOX_SPECIAL_USE == 0x1FE00u)                     &&
          ( (D_IMAP_MAILBOX_SPECIAL_USE &
             D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_SENT)) != 0u )          &&
          ( (D_IMAP_MAILBOX_SPECIAL_USE &
             D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_HASNOCHILDREN)) == 0u ) &&
          (d_imap_mailbox_attributes_parse(d_pack_text_from_cstr("()"),
                                           NULL,
                                           NULL) ==
           D_IMAP_ERROR_INVALID_ARGUMENT)                              &&
          (d_imap_mailbox_attributes_parse(d_pack_text_from_cstr("()"),
                                           &set,
                                           NULL) == D_IMAP_OK) );

    result = d_tests_net_assert(special,
                                "D_IMAP_MAILBOX_SPECIAL_USE",
                                "\\All through \\Important, no others",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_status_same
  Whether two STATUS records hold the same items, field by field.
*/
static bool
d_tests_imap_status_same(
    const struct d_imap_status_data* _a,
    const struct d_imap_status_data* _b
)
{
    return ( (_a->size == _b->size)                   &&
             (_a->highestmodseq == _b->highestmodseq) &&
             (_a->present == _b->present)             &&
             (_a->messages == _b->messages)           &&
             (_a->recent == _b->recent)               &&
             (_a->uidnext == _b->uidnext)             &&
             (_a->uidvalidity == _b->uidvalidity)     &&
             (_a->unseen == _b->unseen)               &&
             (_a->deleted == _b->deleted) );
}

/*
d_tests_imap_status_row
  A row parsed over a sentinel: on success the record must match, and on
failure the sentinel must survive.
*/
static bool
d_tests_imap_status_row(
    const struct d_tests_imap_status_case* _row
)
{
    const struct d_imap_status_data sentinel =
        { 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u };
    struct d_imap_status_data       data     = sentinel;

    const enum d_imap_error error =
        d_imap_status_data_parse(d_pack_text_from_cstr(_row->list),
                                 &data);

    return ( (error == _row->error) &&
             (d_tests_imap_status_same(&data,
                                       (error == D_IMAP_OK)
                                           ? &_row->data
                                           : &sentinel)) );
}

/*
d_tests_sa_imap_status
  Tests the following:
  - STATUS items parse, each to its width; others are skipped whatever
    their shape; a failure leaves the record untouched
  - item lists are written in enumeration order; empty and unknown sets
    are refused
*/
bool
d_tests_sa_imap_status(
    struct d_test_counter* _counter
)
{
    const size_t count   = sizeof(STATUS_LISTS) / sizeof(STATUS_LISTS[0]);
    bool         written = true;
    bool         result  = true;

    // each list
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_status_row(&STATUS_LISTS[i]),
                                    "d_imap_status_data_parse",
                                    STATUS_LISTS[i].what,
                                    _counter) && result;
    }

    // each item set written
    for (size_t i = 0u; i < (sizeof(ITEM_WRITES) / sizeof(ITEM_WRITES[0])); ++i)
    {
        written = ( (d_tests_imap_write_row(&ITEM_WRITES[i],
                                            true)) &&
                    (written) );
    }

    written = ( (d_imap_status_data_parse(d_pack_text_from_cstr("()"),
                                          NULL) ==
                 D_IMAP_ERROR_INVALID_ARGUMENT) &&
                (written) );

    result = d_tests_net_assert(written,
                                "d_imap_status_items_write",
                                "in enumeration order; never empty",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_search_row
  A row parsed with room for three results; they and the MODSEQ are
compared on success.
*/
static bool
d_tests_imap_search_row(
    const struct d_tests_imap_search_case* _row
)
{
    uint32_t ids[3] = { 0u, 0u, 0u };
    size_t   count  = 0u;
    uint64_t modseq = 7u;

    const enum d_imap_error error =
        d_imap_search_parse(d_pack_text_from_cstr(_row->data),
                            ids,
                            3u,
                            &count,
                            &modseq);
    bool same = ( (error == _row->error) &&
                  ( (error != D_IMAP_OK) ||
                    ( (count == _row->count) &&
                      (modseq == _row->modseq) ) ) );

    // each result, on success
    for (size_t i = 0u; (same) && (error == D_IMAP_OK) && (i < count); ++i)
    {
        same = (ids[i] == _row->ids[i]);
    }

    return same;
}

/*
d_tests_sa_imap_search
  Tests the following:
  - SEARCH data parses into its numbers and CONDSTORE's MODSEQ
  - a measuring call, and one with too little room, count every result
  - NULL arguments are refused, but the MODSEQ is optional
*/
bool
d_tests_sa_imap_search(
    struct d_test_counter* _counter
)
{
    const struct d_pack_text data   = d_pack_text_from_cstr("2 84 882");
    uint32_t                 two[2] = { 0u, 0u };
    size_t                   count  = 0u;
    bool                     result = true;

    // each row
    for (size_t i = 0u; i < (sizeof(SEARCHES) / sizeof(SEARCHES[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_search_row(&SEARCHES[i]),
                                    "d_imap_search_parse",
                                    SEARCHES[i].what,
                                    _counter) && result;
    }

    const bool measured =
        ( (d_imap_search_parse(data,
                               NULL,
                               0u,
                               &count,
                               NULL) == D_IMAP_ERROR_BUFFER_TOO_SMALL) &&
          (count == 3u) );
    const bool short_room =
        ( (d_imap_search_parse(data,
                               two,
                               2u,
                               &count,
                               NULL) == D_IMAP_ERROR_BUFFER_TOO_SMALL) &&
          (count == 3u)                                                &&
          (two[0] == 2u)                                               &&
          (two[1] == 84u) );
    const bool nulls =
        ( (d_imap_search_parse(data,
                               NULL,
                               2u,
                               &count,
                               NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_search_parse(data,
                               two,
                               2u,
                               NULL,
                               NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert((measured) && (short_room),
                                "d_imap_search_parse",
                                "counts every result whatever the room",
                                _counter) && result;
    result = d_tests_net_assert(nulls,
                                "d_imap_search_parse",
                                "refuses NULL with room, and a NULL count",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_list_row
  A row parsed over a sentinel entry: on success the entry must match, and
on failure the sentinel must survive.
*/
static bool
d_tests_imap_list_row(
    const struct d_tests_imap_list_case* _row
)
{
    struct d_imap_list_entry entry = { 7u, 7u, 7, { 0u, 0, 0u, { NULL, 0u } },
                                       { NULL, 0u } };

    const enum d_imap_error error =
        d_imap_list_parse(d_pack_text_from_cstr(_row->data),
                          &entry);

    // a failure leaves the entry as it was
    if (error != D_IMAP_OK)
    {
        return ( (error == _row->error) &&
                 (entry.attributes == 7u) &&
                 (entry.delimiter == 7) );
    }

    return ( (_row->error == D_IMAP_OK)                &&
             (entry.attributes == _row->attributes)    &&
             (entry.unknown == _row->unknown)          &&
             (entry.delimiter == _row->delimiter)      &&
             (entry.name.kind == _row->kind)           &&
             (d_tests_imap_span_is(entry.name.text,
                                   _row->name))        &&
             (d_tests_imap_span_is(entry.extended,
                                   _row->extended)) );
}

/*
d_tests_sa_imap_list
  Tests the following:
  - LIST data parses into attributes, delimiter, name and extended data
  - a failure leaves the entry untouched, and a NULL entry is refused
  - a listed name decodes from modified UTF-7
*/
bool
d_tests_sa_imap_list(
    struct d_test_counter* _counter
)
{
    struct d_imap_list_entry    entry   = { 0u, 0u, 0, { 0u, 0, 0u,
                                                         { NULL, 0u } },
                                            { NULL, 0u } };
    struct d_tests_imap_capture capture = { .length = 0u };
    bool                        result  = true;

    // each listing
    for (size_t i = 0u; i < (sizeof(LISTINGS) / sizeof(LISTINGS[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_list_row(&LISTINGS[i]),
                                    "d_imap_list_parse",
                                    LISTINGS[i].what,
                                    _counter) && result;
    }

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const bool decoded =
        ( (d_imap_list_parse(d_pack_text_from_cstr(LISTINGS[9].data),
                             &entry) == D_IMAP_OK)                  &&
          (d_imap_mailbox_decode(d_tests_imap_capture_sink(&capture),
                                 &entry.name,
                                 0u) == D_IMAP_OK)                  &&
          (d_tests_imap_capture_is(&capture,
                                   "~peter/mail/\xe5\x8f\xb0\xe5\x8c\x97")) &&
          (d_imap_list_parse(d_pack_text_from_cstr(LISTINGS[0].data),
                             NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(decoded,
                                "d_imap_mailbox_decode",
                                "a listed name decodes to UTF-8",
                                _counter) && result;

    return result;
}
