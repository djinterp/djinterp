/*******************************************************************************
* djinterp [net]                                         imap_tests_sa_session.c
*
* Tests of the IMAP kernel in a session: a client's commands built by its
* writers, and both sides of a scripted session.
*   The first test writes a session's worth of commands and compares the
* octets with what RFC 9051 has a client send. The second runs a client over
* a memory stream that plays a server's whole side of a conversation, seven
* octets at a time: the client frames each response, parses it and follows
* the server -- capabilities, the selected mailbox's counts and flags, a
* fetched body, listed names, STATUS and SEARCH results, APPEND's
* go-ahead, LOGOUT's BYE -- and the stream keeps what the client sent.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_session.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <string.h>  // memchr, memmove, strlen
// djinterp
#include "../../../../inc/djinterp/net/net_common.h"  // connections
#include "../../../../inc/djinterp/net/net_memory.h"  // memory streams


// d_tests_imap_session
//   struct: a client's side of a scripted session: the stream the server's
// script arrives on, what the client sent through it, the octets read but
// not yet consumed, the tagger and the tag of the command in flight, and
// what the client has learned.
struct d_tests_imap_session
{
    struct d_net_memory         stream;
    struct d_tests_imap_capture sent;
    char                        held[512];
    size_t                      held_length;
    size_t                      reads;
    size_t                      responses;
    struct d_imap_tagger        tagger;
    struct d_imap_tag           tag;
    struct d_imap_capabilities  capabilities;
    bool                        greeted;
    uint32_t                    exists;
    uint64_t                    uidvalidity;
    uint64_t                    uidnext;
    uint32_t                    flags;
    uint32_t                    permanent;
    bool                        read_write;
    uint64_t                    uid;
    struct d_tests_imap_capture body;
    struct d_tests_imap_capture names[2];
    size_t                      listed;
    uint32_t                    special;
    struct d_imap_status_data   status;
    uint32_t                    found[4];
    size_t                      found_count;
    bool                        continued;
    struct d_tests_imap_capture appended;
    bool                        bye;
};

/*
COMMANDS
  A session's worth of commands as RFC 9051 has a client send them.
*/
static const char COMMANDS[] =
    "A0001 CAPABILITY\r\n"
    "A0002 LOGIN fred \"s3cret pass\"\r\n"
    "A0003 SELECT INBOX\r\n"
    "A0004 STATUS Entw&APw-rfe (MESSAGES UIDNEXT UNSEEN)\r\n"
    "A0005 UID FETCH 1:4,7,9:* (FLAGS)\r\n"
    "A0006 STORE 2:4 +FLAGS.SILENT (\\Seen \\Deleted)\r\n"
    "A0007 SEARCH SINCE 1-Feb-2026\r\n"
    "A0008 APPEND \"Sent Items\" (\\Seen) \" 1-Feb-2026 09:05:07 -0500\" "
    "{13}\r\n"
    "A0009 CREATE &U,BTFw-\r\n"
    "A0010 LOGOUT\r\n";

/*
SCRIPT, SENT
  A server's side of a session, after Dovecot's habits, and what the client
should send to it: twenty-one responses, two literals among them.
*/
static const char SCRIPT[] =
    "* OK [CAPABILITY IMAP4rev1 SASL-IR ID ENABLE IDLE LITERAL+ AUTH=PLAIN] "
    "Dovecot ready.\r\n"
    "A0001 OK [CAPABILITY IMAP4rev1 SASL-IR ID ENABLE IDLE SORT LITERAL+ "
    "MOVE] Logged in\r\n"
    "* 3 EXISTS\r\n"
    "* OK [UIDVALIDITY 1700000000] UIDs valid\r\n"
    "* OK [UIDNEXT 4] Predicted next UID\r\n"
    "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"
    "* OK [PERMANENTFLAGS (\\Deleted \\Seen \\*)] Limited\r\n"
    "A0002 OK [READ-WRITE] Select completed\r\n"
    "* 2 FETCH (UID 2 FLAGS (\\Seen) BODY[TEXT] {13}\r\nHello, world!)\r\n"
    "A0003 OK Fetch completed\r\n"
    "* LIST (\\HasNoChildren \\Drafts) \"/\" Entw&APw-rfe\r\n"
    "* LIST (\\HasNoChildren) \"/\" {8}\r\nMy Box!x\r\n"
    "A0004 OK List completed\r\n"
    "* STATUS Entw&APw-rfe (MESSAGES 2 UIDNEXT 9 UNSEEN 1)\r\n"
    "A0005 OK Status completed\r\n"
    "* SEARCH 1 3\r\n"
    "A0006 OK Search completed\r\n"
    "+ Ready for literal data\r\n"
    "A0007 OK [APPENDUID 1700000000 4] Append completed\r\n"
    "* BYE Logging out\r\n"
    "A0008 OK Logout completed\r\n";
static const char SENT[] =
    "A0001 LOGIN fred \"s3cret pass\"\r\n"
    "A0002 SELECT INBOX\r\n"
    "A0003 UID FETCH 2 (FLAGS BODY.PEEK[TEXT])\r\n"
    "A0004 LIST \"\" %\r\n"
    "A0005 STATUS Entw&APw-rfe (MESSAGES UIDNEXT UNSEEN)\r\n"
    "A0006 SEARCH SINCE 1-Feb-2026 UNSEEN\r\n"
    "A0007 APPEND \"Sent Items\" (\\Seen) \" 1-Feb-2026 09:05:07 -0500\" "
    "{13}\r\n"
    "Hello, world!\r\n"
    "A0008 LOGOUT\r\n";

/*
WHEN
  The date and time the commands name.
*/
static const struct d_imap_datetime WHEN = { 2026, 2, 1, 9, 5, 7, -300 };

/*
d_tests_imap_put
  Writes a C string to a sink, whole or not at all.
*/
static bool
d_tests_imap_put(
    struct d_pack_sink _sink,
    const char*        _text
)
{
    const size_t length = strlen(_text);

    return (_sink.write(_sink.context,
                        _text,
                        length) == length);
}

/*
d_tests_imap_put_span
  Writes a span to a sink, whole or not at all.
*/
static bool
d_tests_imap_put_span(
    struct d_pack_sink _sink,
    struct d_pack_text _text
)
{
    return ( (_text.length == 0u) ||
             (_sink.write(_sink.context,
                          _text.data,
                          _text.length) == _text.length) );
}

/*
d_tests_imap_begin
  Issues the next tag into `_tag` and writes it and the command's keyword.
*/
static bool
d_tests_imap_begin(
    struct d_pack_sink    _sink,
    struct d_imap_tagger* _tagger,
    struct d_imap_tag*    _tag,
    enum d_imap_command   _command
)
{
    return ( (d_imap_tagger_next(_tagger,
                                 _tag) == D_IMAP_OK)            &&
             (d_tests_imap_put(_sink,
                               _tag->text))                     &&
             (d_tests_imap_put(_sink,
                               " "))                            &&
             (d_tests_imap_put(_sink,
                               d_imap_command_name(_command))) );
}

/*
d_tests_imap_client_open
  CAPABILITY, LOGIN, SELECT and STATUS, written to `_sink`.
*/
static bool
d_tests_imap_client_open(
    struct d_pack_sink    _sink,
    struct d_imap_tagger* _tagger
)
{
    struct d_imap_tag tag = { .length = 0u };

    return ( (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_CAPABILITY))           &&
             (d_tests_imap_put(_sink,
                               "\r\n"))                         &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_LOGIN))                &&
             (d_tests_imap_put(_sink,
                               " "))                            &&
             (d_imap_astring_write(_sink,
                                   d_pack_text_from_cstr("fred"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(_sink,
                               " "))                            &&
             (d_imap_astring_write(_sink,
                                   d_pack_text_from_cstr("s3cret pass"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(_sink,
                               "\r\n"))                         &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_SELECT))               &&
             (d_tests_imap_put(_sink,
                               " "))                            &&
             (d_imap_mailbox_write(_sink,
                                   d_pack_text_from_cstr("INBOX"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(_sink,
                               "\r\n"))                         &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_STATUS))               &&
             (d_tests_imap_put(_sink,
                               " "))                            &&
             (d_imap_mailbox_write(_sink,
                                   d_pack_text_from_cstr(
                                       "Entw\xc3\xbcrfe"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(_sink,
                               " "))                            &&
             (d_imap_status_items_write(
                  _sink,
                  D_IMAP_STATUS_BIT(D_IMAP_STATUS_MESSAGES) |
                  D_IMAP_STATUS_BIT(D_IMAP_STATUS_UIDNEXT)  |
                  D_IMAP_STATUS_BIT(D_IMAP_STATUS_UNSEEN)) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               "\r\n")) );
}

/*
d_tests_imap_client_work
  UID FETCH, STORE and SEARCH, written to `_sink`.
*/
static bool
d_tests_imap_client_work(
    struct d_pack_sink    _sink,
    struct d_imap_tagger* _tagger
)
{
    static const struct d_imap_seq_range SET[] =
    {
        { 1u, 4u }, { 7u, 7u }, { 9u, D_IMAP_SEQ_STAR }
    };
    static const struct d_imap_seq_range RANGE[] = { { 2u, 4u } };
    struct d_imap_tag                    tag     = { .length = 0u };

    return ( (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_UID)) &&
             (d_tests_imap_put(_sink,
                               " FETCH ")) &&
             (d_imap_seqset_write(_sink,
                                  SET,
                                  3u) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               " (FLAGS)\r\n")) &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_STORE)) &&
             (d_tests_imap_put(_sink,
                               " ")) &&
             (d_imap_seqset_write(_sink,
                                  RANGE,
                                  1u) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               " +FLAGS.SILENT ")) &&
             (d_imap_flags_write(_sink,
                                 D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN) |
                                 D_IMAP_FLAG_BIT(D_IMAP_FLAG_DELETED)) ==
              D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               "\r\n")) &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_SEARCH)) &&
             (d_tests_imap_put(_sink,
                               " SINCE ")) &&
             (d_imap_date_write(_sink,
                                WHEN) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               "\r\n")) );
}

/*
d_tests_imap_client_close
  APPEND's line, CREATE and LOGOUT, written to `_sink`.
*/
static bool
d_tests_imap_client_close(
    struct d_pack_sink    _sink,
    struct d_imap_tagger* _tagger
)
{
    struct d_imap_tag       tag  = { .length = 0u };
    enum d_imap_string_form form = D_IMAP_STRING_ATOM;

    return ( (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_APPEND)) &&
             (d_tests_imap_put(_sink,
                               " ")) &&
             (d_imap_mailbox_write(_sink,
                                   d_pack_text_from_cstr("Sent Items"),
                                   0u,
                                   &form) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               " ")) &&
             (d_imap_flags_write(_sink,
                                 D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN)) ==
              D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               " ")) &&
             (d_imap_datetime_write(_sink,
                                    WHEN) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               " ")) &&
             (d_imap_literal_prefix_write(_sink,
                                          13u,
                                          0u,
                                          &form) == D_IMAP_OK) &&
             (form == D_IMAP_STRING_LITERAL) &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_CREATE)) &&
             (d_tests_imap_put(_sink,
                               " ")) &&
             (d_imap_mailbox_write(_sink,
                                   d_pack_text_from_cstr(
                                       "\xe5\x8f\xb0\xe5\x8c\x97"),
                                   0u,
                                   NULL) == D_IMAP_OK) &&
             (d_tests_imap_put(_sink,
                               "\r\n")) &&
             (d_tests_imap_begin(_sink,
                                 _tagger,
                                 &tag,
                                 D_IMAP_COMMAND_LOGOUT)) &&
             (d_tests_imap_put(_sink,
                               "\r\n")) );
}

/*
d_tests_sa_imap_session_client
  Tests the following:
  - tags, keywords, astrings, mailbox names, STATUS items, sequence sets,
    flags, dates and a literal's prefix, written in turn, make exactly the
    commands RFC 9051 has a client send
*/
bool
d_tests_sa_imap_session_client(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_capture capture = { .length = 0u };
    struct d_imap_tagger        tagger  = { .next = 0u };

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));
    (void)d_imap_tagger_init(&tagger,
                             d_pack_text_from_cstr("A"));

    const struct d_pack_sink sink    = d_tests_imap_capture_sink(&capture);
    const bool               written =
        ( (d_tests_imap_client_open(sink,
                                    &tagger)) &&
          (d_tests_imap_client_work(sink,
                                    &tagger)) &&
          (d_tests_imap_client_close(sink,
                                     &tagger)) );

    return d_tests_net_assert( (written) &&
                               (d_tests_imap_capture_is(&capture,
                                                        COMMANDS)),
                              "d_imap_astring_write",
                              "a session's commands, octet for octet",
                              _counter);
}

/*
d_tests_imap_wire_write
  A sink's write over a connection: what the connection took, or nothing
when it fails.
*/
static size_t
d_tests_imap_wire_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    const struct d_net_io_result put =
        d_net_connection_write((struct d_net_connection*)_context,
                               _data,
                               _size);

    return (put.error == D_NET_OK) ? put.count : 0u;
}

/*
d_tests_imap_wire
  A sink writing to the session's stream.
*/
static struct d_pack_sink
d_tests_imap_wire(
    struct d_tests_imap_session* _session
)
{
    const struct d_pack_sink sink = { d_tests_imap_wire_write,
                                      &_session->stream.base };

    return sink;
}

/*
d_tests_imap_announced
  Whether a line -- `_length` octets ending with its LF -- announces a
literal, "{n}" or "{n+}" just before its line break; `_count` then
receives n.
*/
static bool
d_tests_imap_announced(
    const char* _line,
    size_t      _length,
    size_t*     _count
)
{
    size_t end = _length - 1u;

    // a CR before the LF belongs to the line break
    if ( (end > 0u) &&
         (_line[end - 1u] == '\r') )
    {
        --end;
    }

    // the closing brace must end the line
    if ( (end == 0u) ||
         (_line[end - 1u] != '}') )
    {
        return false;
    }

    size_t digits = end - 1u;

    // a "+" before it marks a non-synchronizing literal
    if ( (digits > 0u) &&
         (_line[digits - 1u] == '+') )
    {
        --digits;
    }

    size_t first = digits;

    // the count's digits, back to the opening brace
    while ( (first > 0u)                 &&
            (_line[first - 1u] >= '0') &&
            (_line[first - 1u] <= '9') )
    {
        --first;
    }

    // a count, and the brace that opens it
    if ( (first == digits) ||
         (first == 0u)     ||
         (_line[first - 1u] != '{') )
    {
        return false;
    }

    size_t count = 0u;

    // the count's value
    for (size_t i = first; i < digits; ++i)
    {
        count = (count * 10u) + (size_t)(_line[i] - '0');
    }

    *_count = count;

    return true;
}

/*
d_tests_imap_frame
  The length of the first whole response in `_data`, or 0 when it is not
all there: a response ends at a line break no literal announcement
precedes, and an announced literal's octets follow its line break.
*/
static size_t
d_tests_imap_frame(
    const char* _data,
    size_t      _length
)
{
    size_t at = 0u;

    // line by line, past each announced literal's octets
    while (at < _length)
    {
        const char* lf    = memchr(_data + at,
                                   '\n',
                                   _length - at);
        size_t      count = 0u;

        // the line is not all here
        if (lf == NULL)
        {
            return 0u;
        }

        const size_t end = (size_t)(lf - _data) + 1u;

        // a line announcing no literal ends the response
        if (!d_tests_imap_announced(_data + at,
                                    end - at,
                                    &count))
        {
            return end;
        }

        at = end + count;
    }

    return 0u;
}

/*
d_tests_imap_receive
  The next whole response, parsed into `_response`, read from the stream as
needed; its length, for d_tests_imap_consume, or 0 when the stream ends
first or the response does not parse.
*/
static size_t
d_tests_imap_receive(
    struct d_tests_imap_session* _session,
    struct d_imap_response*      _response
)
{
    size_t length = d_tests_imap_frame(_session->held,
                                       _session->held_length);

    // read until a whole response is held
    while ( (length == 0u) &&
            (_session->held_length < sizeof(_session->held)) )
    {
        const struct d_net_io_result got =
            d_net_connection_read(&_session->stream.base,
                                  _session->held + _session->held_length,
                                  sizeof(_session->held) -
                                      _session->held_length);

        _session->reads += 1u;

        // the end of the stream, or a failure, ends the session
        if ( (got.error != D_NET_OK) ||
             (got.count == 0u) )
        {
            return 0u;
        }

        _session->held_length += got.count;
        length = d_tests_imap_frame(_session->held,
                                    _session->held_length);
    }

    // a response the kernel cannot parse ends it too
    if ( (length == 0u) ||
         (d_imap_response_parse(d_pack_text_from_span(_session->held,
                                                      length),
                                _response) != D_IMAP_OK) )
    {
        return 0u;
    }

    _session->responses += 1u;

    return length;
}

/*
d_tests_imap_consume
  Drops a handled response from the held octets.
*/
static void
d_tests_imap_consume(
    struct d_tests_imap_session* _session,
    size_t                       _length
)
{
    memmove(_session->held,
            _session->held + _length,
            _session->held_length - _length);
    _session->held_length -= _length;

    return;
}

/*
d_tests_imap_number
  The number a code's argument starts with, through the lexer.
*/
static bool
d_tests_imap_number(
    struct d_pack_text _text,
    uint64_t*          _value
)
{
    struct d_imap_lexer lexer = { NULL, 0u, 0u };
    struct d_imap_token token = { .kind = 0 };

    d_imap_lexer_init(&lexer,
                      _text);

    const bool numbered = ( (d_imap_lexer_next(&lexer,
                                               0u,
                                               &token) == D_IMAP_OK) &&
                            (token.kind == D_IMAP_TOKEN_NUMBER) );

    *_value = (numbered) ? token.number : 0u;

    return numbered;
}

/*
d_tests_imap_on_code
  What a response code tells the client: capabilities, the mailbox's UID
validity and next UID, its permanent flags, its access, an appended UID.
*/
static bool
d_tests_imap_on_code(
    struct d_tests_imap_session*  _session,
    const struct d_imap_response* _response
)
{
    const struct d_pack_text argument = _response->code_argument;

    switch (_response->code)
    {
        case D_IMAP_CODE_CAPABILITY:
            return (d_imap_capabilities_parse(argument,
                                              &_session->capabilities) ==
                    D_IMAP_OK);
        case D_IMAP_CODE_UIDVALIDITY:
            return d_tests_imap_number(argument,
                                       &_session->uidvalidity);
        case D_IMAP_CODE_UIDNEXT:
            return d_tests_imap_number(argument,
                                       &_session->uidnext);
        case D_IMAP_CODE_PERMANENTFLAGS:
            return (d_imap_flags_parse(argument,
                                       &_session->permanent,
                                       NULL) == D_IMAP_OK);
        case D_IMAP_CODE_READ_WRITE:
            _session->read_write = true;

            return true;
        case D_IMAP_CODE_APPENDUID:
            return d_tests_imap_put_span(
                       d_tests_imap_capture_sink(&_session->appended),
                       argument);
        default:
            return true;
    }
}

/*
d_tests_imap_on_fetch
  FETCH data: the number after UID, and the literal's octets as the body.
*/
static bool
d_tests_imap_on_fetch(
    struct d_tests_imap_session* _session,
    struct d_pack_text           _data
)
{
    struct d_imap_lexer lexer     = { NULL, 0u, 0u };
    struct d_imap_token token     = { .kind = 0 };
    bool                after_uid = false;

    d_imap_lexer_init(&lexer,
                      _data);

    // token by token, to the end of the data
    while (d_imap_lexer_next(&lexer,
                             0u,
                             &token) == D_IMAP_OK)
    {
        // the end of the data, the body found
        if (token.kind == D_IMAP_TOKEN_END)
        {
            return (_session->body.length > 0u);
        }

        // the number after UID
        if ( (after_uid) &&
             (token.kind == D_IMAP_TOKEN_NUMBER) )
        {
            _session->uid = token.number;
        }

        // the literal is the body
        if ( (token.kind == D_IMAP_TOKEN_LITERAL) &&
             (d_imap_string_decode(
                  d_tests_imap_capture_sink(&_session->body),
                  &token) != D_IMAP_OK) )
        {
            return false;
        }

        after_uid = ( (token.kind == D_IMAP_TOKEN_ATOM) &&
                      (d_tests_imap_span_is(token.text,
                                            "UID")) );
    }

    return false;
}

/*
d_tests_imap_on_list
  A LIST entry: its name decoded, and its special use noted.
*/
static bool
d_tests_imap_on_list(
    struct d_tests_imap_session* _session,
    struct d_pack_text           _data
)
{
    struct d_imap_list_entry entry = { .attributes = 0u };

    // room for two names, and a listing that parses
    if ( (_session->listed >= 2u) ||
         (d_imap_list_parse(_data,
                            &entry) != D_IMAP_OK) )
    {
        return false;
    }

    struct d_tests_imap_capture* name = &_session->names[_session->listed];

    _session->listed  += 1u;
    _session->special |= (entry.attributes & D_IMAP_MAILBOX_SPECIAL_USE);

    return (d_imap_mailbox_decode(d_tests_imap_capture_sink(name),
                                  &entry.name,
                                  0u) == D_IMAP_OK);
}

/*
d_tests_imap_on_status
  STATUS data: the mailbox's name, decoded, then its items, which follow
the name.
*/
static bool
d_tests_imap_on_status(
    struct d_tests_imap_session* _session,
    struct d_pack_text           _data
)
{
    struct d_imap_lexer         lexer   = { NULL, 0u, 0u };
    struct d_imap_token         name    = { .kind = 0 };
    struct d_tests_imap_capture decoded = { .length = 0u };

    d_tests_imap_capture_init(&decoded,
                              sizeof(decoded.data));
    d_imap_lexer_init(&lexer,
                      _data);

    return ( (d_imap_lexer_next(&lexer,
                                D_IMAP_LEX_ASTRING,
                                &name) == D_IMAP_OK)                  &&
             (d_imap_mailbox_decode(d_tests_imap_capture_sink(&decoded),
                                    &name,
                                    0u) == D_IMAP_OK)                 &&
             (d_tests_imap_capture_is(&decoded,
                                      "Entw\xc3\xbcrfe"))             &&
             (d_imap_status_data_parse(d_imap_lexer_rest(&lexer),
                                       &_session->status) == D_IMAP_OK) );
}

/*
d_tests_imap_on_data
  Untagged data, by its keyword: EXISTS, FLAGS, SEARCH, FETCH, LIST and
STATUS; anything else is unexpected here.
*/
static bool
d_tests_imap_on_data(
    struct d_tests_imap_session*  _session,
    const struct d_imap_response* _response
)
{
    const struct d_pack_text keyword = _response->keyword;
    const struct d_pack_text rest    = _response->rest;

    // the mailbox's size
    if (d_tests_imap_span_is(keyword,
                             "EXISTS"))
    {
        _session->exists = _response->number;

        return (_response->has_number == 1);
    }

    // its flags
    if (d_tests_imap_span_is(keyword,
                             "FLAGS"))
    {
        return (d_imap_flags_parse(rest,
                                   &_session->flags,
                                   NULL) == D_IMAP_OK);
    }

    // a search's results
    if (d_tests_imap_span_is(keyword,
                             "SEARCH"))
    {
        return (d_imap_search_parse(rest,
                                    _session->found,
                                    4u,
                                    &_session->found_count,
                                    NULL) == D_IMAP_OK);
    }

    return ( ( (d_tests_imap_span_is(keyword,
                                     "FETCH"))  &&
               (d_tests_imap_on_fetch(_session,
                                      rest)) )  ||
             ( (d_tests_imap_span_is(keyword,
                                     "LIST"))   &&
               (d_tests_imap_on_list(_session,
                                     rest)) )   ||
             ( (d_tests_imap_span_is(keyword,
                                     "STATUS")) &&
               (d_tests_imap_on_status(_session,
                                       rest)) ) );
}

/*
d_tests_imap_handle
  One untagged response or continuation: APPEND's go-ahead, BYE, a status
response's code, or data.
*/
static bool
d_tests_imap_handle(
    struct d_tests_imap_session*  _session,
    const struct d_imap_response* _response
)
{
    // a continuation lets APPEND send its literal
    if (_response->kind == D_IMAP_RESPONSE_CONTINUATION)
    {
        _session->continued = true;

        return true;
    }

    // BYE ends the session, as LOGOUT asked
    if (_response->condition == D_IMAP_CONDITION_BYE)
    {
        _session->bye = (d_imap_error_from_condition(D_IMAP_CONDITION_BYE) ==
                         D_IMAP_ERROR_BYE);

        return true;
    }

    // other status responses carry their news in a code
    if (_response->condition != D_IMAP_CONDITION_NONE)
    {
        return ( (_response->condition == D_IMAP_CONDITION_OK) &&
                 (d_tests_imap_on_code(_session,
                                       _response)) );
    }

    return d_tests_imap_on_data(_session,
                                _response);
}

/*
d_tests_imap_await
  Receives responses until the tagged one completing the command in
flight: each untagged one is handled, and the completion must echo the
command's tag with OK, its code handled too.
*/
static bool
d_tests_imap_await(
    struct d_tests_imap_session* _session
)
{
    // response by response, to the completion
    while (true)
    {
        struct d_imap_response response = { .kind = 0 };
        const size_t           length   = d_tests_imap_receive(_session,
                                                               &response);

        // the stream ended, or a response did not parse
        if (length == 0u)
        {
            return false;
        }

        const bool tagged  = (response.kind == D_IMAP_RESPONSE_TAGGED);
        const bool handled =
            (tagged)
                ? ( (d_imap_tag_matches(&_session->tag,
                                        response.tag))          &&
                    (response.condition == D_IMAP_CONDITION_OK) &&
                    (d_tests_imap_on_code(_session,
                                          &response)) )
                : d_tests_imap_handle(_session,
                                      &response);

        d_tests_imap_consume(_session,
                             length);

        // a failure, or the completion, ends the wait
        if ( (!handled) ||
             (tagged) )
        {
            return handled;
        }
    }
}

/*
d_tests_imap_greet
  The greeting: an untagged OK whose code advertises the capabilities.
*/
static bool
d_tests_imap_greet(
    struct d_tests_imap_session* _session
)
{
    struct d_imap_response response = { .kind = 0 };
    const size_t           length   = d_tests_imap_receive(_session,
                                                           &response);
    const bool             handled  =
        ( (length > 0u)                                     &&
          (response.kind == D_IMAP_RESPONSE_UNTAGGED)       &&
          (response.code == D_IMAP_CODE_CAPABILITY)         &&
          (d_tests_imap_handle(_session,
                               &response)) );

    // the greeting consumed
    if (length > 0u)
    {
        d_tests_imap_consume(_session,
                             length);
    }

    _session->greeted =
        ( (handled)                                                 &&
          (d_imap_capabilities_has_auth(_session->capabilities,
                                        D_IMAP_AUTH_PLAIN))         &&
          (d_imap_capabilities_has(_session->capabilities,
                                   D_IMAP_CAPABILITY_LITERAL_PLUS)) &&
          (!d_imap_capabilities_has(_session->capabilities,
                                    D_IMAP_CAPABILITY_MOVE)) );

    return handled;
}

/*
d_tests_imap_login
  LOGIN, then SELECT.
*/
static bool
d_tests_imap_login(
    struct d_tests_imap_session* _session
)
{
    const struct d_pack_sink sink = d_tests_imap_wire(_session);

    return ( (d_tests_imap_begin(sink,
                                 &_session->tagger,
                                 &_session->tag,
                                 D_IMAP_COMMAND_LOGIN))                &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_astring_write(sink,
                                   d_pack_text_from_cstr("fred"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_astring_write(sink,
                                   d_pack_text_from_cstr("s3cret pass"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(sink,
                               "\r\n"))                          &&
             (d_tests_imap_await(_session))                            &&
             (d_tests_imap_begin(sink,
                                 &_session->tagger,
                                 &_session->tag,
                                 D_IMAP_COMMAND_SELECT))               &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_mailbox_write(sink,
                                   d_pack_text_from_cstr("INBOX"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(sink,
                               "\r\n"))                          &&
             (d_tests_imap_await(_session)) );
}

/*
d_tests_imap_read
  UID FETCH of one message's flags and text, then LIST.
*/
static bool
d_tests_imap_read(
    struct d_tests_imap_session* _session
)
{
    static const struct d_imap_seq_range TWO[] = { { 2u, 2u } };
    const struct d_pack_sink             sink  = d_tests_imap_wire(_session);

    return ( (d_tests_imap_begin(sink,
                                 &_session->tagger,
                                 &_session->tag,
                                 D_IMAP_COMMAND_UID))                  &&
             (d_tests_imap_put(sink,
                               " FETCH "))                       &&
             (d_imap_seqset_write(sink,
                                  TWO,
                                  1u) == D_IMAP_OK)         &&
             (d_tests_imap_put(sink,
                               " (FLAGS BODY.PEEK[TEXT])\r\n"))  &&
             (d_tests_imap_await(_session))                            &&
             (d_tests_imap_begin(sink,
                                 &_session->tagger,
                                 &_session->tag,
                                 D_IMAP_COMMAND_LIST))                 &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_astring_write(sink,
                                   d_pack_text_from_cstr(""),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_astring_write(sink,
                                   d_pack_text_from_cstr("%"),
                                   D_IMAP_STRING_LIST_WILDCARDS,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(sink,
                               "\r\n"))                          &&
             (d_tests_imap_await(_session)) );
}

/*
d_tests_imap_query
  STATUS of a mailbox named in modified UTF-7, then SEARCH.
*/
static bool
d_tests_imap_query(
    struct d_tests_imap_session* _session
)
{
    const struct d_pack_sink sink = d_tests_imap_wire(_session);

    return ( (d_tests_imap_begin(sink,
                                 &_session->tagger,
                                 &_session->tag,
                                 D_IMAP_COMMAND_STATUS))               &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_mailbox_write(sink,
                                   d_pack_text_from_cstr(
                                       "Entw\xc3\xbcrfe"),
                                   0u,
                                   NULL) == D_IMAP_OK)                 &&
             (d_tests_imap_put(sink,
                               " "))                             &&
             (d_imap_status_items_write(
                  sink,
                  D_IMAP_STATUS_BIT(D_IMAP_STATUS_MESSAGES) |
                  D_IMAP_STATUS_BIT(D_IMAP_STATUS_UIDNEXT)  |
                  D_IMAP_STATUS_BIT(D_IMAP_STATUS_UNSEEN)) == D_IMAP_OK) &&
             (d_tests_imap_put(sink,
                               "\r\n"))                          &&
             (d_tests_imap_await(_session))                            &&
             (d_tests_imap_begin(sink,
                                 &_session->tagger,
                                 &_session->tag,
                                 D_IMAP_COMMAND_SEARCH))               &&
             (d_tests_imap_put(sink,
                               " SINCE "))                       &&
             (d_imap_date_write(sink,
                                WHEN) == D_IMAP_OK)              &&
             (d_tests_imap_put(sink,
                               " UNSEEN\r\n"))                   &&
             (d_tests_imap_await(_session)) );
}

/*
d_tests_imap_append
  APPEND with a synchronizing literal: the line, the server's go-ahead,
the octets, the completion.
*/
static bool
d_tests_imap_append(
    struct d_tests_imap_session* _session
)
{
    const struct d_pack_sink sink   = d_tests_imap_wire(_session);
    struct d_imap_response   prompt = { .kind = 0 };
    enum d_imap_string_form  form   = D_IMAP_STRING_ATOM;

    const bool announced =
        ( (d_tests_imap_begin(sink,
                              &_session->tagger,
                              &_session->tag,
                              D_IMAP_COMMAND_APPEND))                  &&
          (d_tests_imap_put(sink,
                            " "))                                &&
          (d_imap_mailbox_write(sink,
                                d_pack_text_from_cstr("Sent Items"),
                                0u,
                                &form) == D_IMAP_OK)                   &&
          (d_tests_imap_put(sink,
                            " "))                                &&
          (d_imap_flags_write(sink,
                              D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN)) ==
           D_IMAP_OK)                                                  &&
          (d_tests_imap_put(sink,
                            " "))                                &&
          (d_imap_datetime_write(sink,
                                 WHEN) == D_IMAP_OK)             &&
          (d_tests_imap_put(sink,
                            " "))                                &&
          (d_imap_literal_prefix_write(sink,
                                       13u,
                                       0u,
                                       &form) == D_IMAP_OK)            &&
          (form == D_IMAP_STRING_LITERAL) );
    const size_t length   = (announced)
                                ? d_tests_imap_receive(_session,
                                                       &prompt)
                                : 0u;
    const bool   prompted =
        ( (length > 0u)                                    &&
          (prompt.kind == D_IMAP_RESPONSE_CONTINUATION)    &&
          (d_tests_imap_handle(_session,
                               &prompt)) );

    // the go-ahead consumed
    if (length > 0u)
    {
        d_tests_imap_consume(_session,
                             length);
    }

    return ( (prompted)                                 &&
             (d_tests_imap_put(sink,
                               "Hello, world!"))  &&
             (d_tests_imap_put(sink,
                               "\r\n"))           &&
             (d_tests_imap_await(_session)) );
}

/*
d_tests_imap_session_learned
  What the session learned, asserted against what the server said.
*/
static bool
d_tests_imap_session_learned(
    const struct d_tests_imap_session* _session,
    struct d_test_counter*             _counter
)
{
    const struct d_imap_status_data* status = &_session->status;
    bool                             result = true;

    const bool capabilities =
        ( (_session->greeted)                                        &&
          (d_imap_capabilities_has(_session->capabilities,
                                   D_IMAP_CAPABILITY_MOVE))          &&
          (d_imap_capabilities_has(_session->capabilities,
                                   D_IMAP_CAPABILITY_SORT))          &&
          (!d_imap_capabilities_has_auth(_session->capabilities,
                                         D_IMAP_AUTH_PLAIN)) );
    const bool selected =
        ( (_session->exists == 3u)                                   &&
          (_session->uidvalidity == 1700000000u)                     &&
          (_session->uidnext == 4u)                                  &&
          (_session->flags == 0x1Fu)                                 &&
          (_session->permanent ==
           ( D_IMAP_FLAG_BIT(D_IMAP_FLAG_DELETED) |
             D_IMAP_FLAG_BIT(D_IMAP_FLAG_SEEN)    |
             D_IMAP_FLAG_BIT(D_IMAP_FLAG_WILDCARD) ))                &&
          (_session->read_write) );
    const bool fetched =
        ( (_session->uid == 2u)                                      &&
          (d_tests_imap_capture_is(&_session->body,
                                   "Hello, world!"))                 &&
          (_session->listed == 2u)                                   &&
          (d_tests_imap_capture_is(&_session->names[0],
                                   "Entw\xc3\xbcrfe"))               &&
          (d_tests_imap_capture_is(&_session->names[1],
                                   "My Box!x"))                      &&
          (_session->special ==
           D_IMAP_MAILBOX_BIT(D_IMAP_MAILBOX_DRAFTS)) );
    const bool queried =
        ( (status->present ==
           ( D_IMAP_STATUS_BIT(D_IMAP_STATUS_MESSAGES) |
             D_IMAP_STATUS_BIT(D_IMAP_STATUS_UIDNEXT)  |
             D_IMAP_STATUS_BIT(D_IMAP_STATUS_UNSEEN) ))              &&
          (status->messages == 2u)                                   &&
          (status->uidnext == 9u)                                    &&
          (status->unseen == 1u)                                     &&
          (_session->found_count == 2u)                              &&
          (_session->found[0] == 1u)                                 &&
          (_session->found[1] == 3u) );
    const bool appended =
        ( (_session->continued)                                      &&
          (d_tests_imap_capture_is(&_session->appended,
                                   "1700000000 4"))                  &&
          (_session->bye) );

    result = d_tests_net_assert(capabilities,
                                "d_imap_capabilities_parse",
                                "the greeting's, then LOGIN's in their place",
                                _counter) && result;
    result = d_tests_net_assert(selected,
                                "d_imap_response_parse",
                                "SELECT's counts, flags and access",
                                _counter) && result;
    result = d_tests_net_assert(fetched,
                                "d_imap_lexer_next",
                                "a fetched body, and listed names decoded",
                                _counter) && result;
    result = d_tests_net_assert(queried,
                                "d_imap_status_data_parse",
                                "STATUS's items and SEARCH's results",
                                _counter) && result;
    result = d_tests_net_assert(appended,
                                "d_imap_response_parse",
                                "APPEND's go-ahead and UID; LOGOUT's BYE",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_session_stream
  Tests the following:
  - a client built from the kernel's parts carries a session through over
    a stream that delivers the server's side seven octets at a time
  - every response frames, parses and is handled; every command completes
  - what the client learns is what the server said, and what it sent is
    what RFC 9051 has it send
*/
bool
d_tests_sa_imap_session_stream(
    struct d_test_counter* _counter
)
{
    struct d_tests_imap_session session = { .held_length = 0u };
    bool                        result  = true;

    d_tests_imap_capture_init(&session.sent,
                              sizeof(session.sent.data));
    d_tests_imap_capture_init(&session.body,
                              sizeof(session.body.data));
    d_tests_imap_capture_init(&session.names[0],
                              sizeof(session.names[0].data));
    d_tests_imap_capture_init(&session.names[1],
                              sizeof(session.names[1].data));
    d_tests_imap_capture_init(&session.appended,
                              sizeof(session.appended.data));
    d_net_memory_init(&session.stream,
                      SCRIPT,
                      sizeof(SCRIPT) - 1u,
                      d_tests_imap_capture_sink(&session.sent));
    session.stream.chunk = 7u;
    (void)d_imap_tagger_init(&session.tagger,
                             d_pack_text_from_cstr("A"));

    const bool ran      = ( (d_tests_imap_greet(&session))  &&
                            (d_tests_imap_login(&session))  &&
                            (d_tests_imap_read(&session))   &&
                            (d_tests_imap_query(&session))  &&
                            (d_tests_imap_append(&session)) &&
                            (d_tests_imap_begin(
                                 d_tests_imap_wire(&session),
                                 &session.tagger,
                                 &session.tag,
                                 D_IMAP_COMMAND_LOGOUT))    &&
                            (d_tests_imap_put(d_tests_imap_wire(&session),
                                              "\r\n"))      &&
                            (d_tests_imap_await(&session)) );
    const bool streamed =
        ( (session.responses == 21u)                             &&
          (session.held_length == 0u)                            &&
          (session.reads >= ((sizeof(SCRIPT) - 1u) / 7u))        &&
          (session.stream.input_offset == session.stream.input_size) );

    result = d_tests_net_assert(ran,
                                "d_imap_response_parse",
                                "every command completes",
                                _counter) && result;
    result = d_tests_net_assert(streamed,
                                "d_imap_response_parse",
                                "21 responses framed from 7-octet reads",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_imap_capture_is(&session.sent,
                                                        SENT),
                                "d_imap_astring_write",
                                "the client sent RFC 9051's lines",
                                _counter) && result;
    result = d_tests_imap_session_learned(&session,
                                          _counter) && result;

    return result;
}
