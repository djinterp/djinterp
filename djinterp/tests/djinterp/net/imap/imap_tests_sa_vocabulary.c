/*******************************************************************************
* djinterp [net]                                      imap_tests_sa_vocabulary.c
*
* Tests of the IMAP kernel's vocabulary: its errors and backends, every
* enumeration's names and lookups, and the states each command may be sent
* in.
*   Each enumerator is named as the RFC that defines it spells it, and is
* looked up again from that name, as spelled and in lower case; a value and a
* name outside each enumeration read as the header says they do.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_vocabulary.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <string.h>  // strcmp
// djinterp
#include "../../../../inc/djinterp/net/net_common.h"  // d_net_error


// d_tests_imap_error_case
//   struct: one row of the errors table: an error, its name, the d_net_error
// net_common.h's rules map it to, and its class: 's' the server's, 't' the
// transport's, '-' neither.
struct d_tests_imap_error_case
{
    enum d_imap_error error;
    const char*       name;
    enum d_net_error  net;
    char              kind;
};

// d_tests_imap_vocabulary
//   struct: one enumeration's names, by value. A lookup finds each from
// `first` on; a value outside the enumeration is named `unknown`, and a name
// outside it looks up as `missing`, -1 meaning not found. `find` is NULL for
// an enumeration without a lookup.
struct d_tests_imap_vocabulary
{
    const char*        what;
    const char* const* names;
    int                count;
    int                first;
    const char*        unknown;
    int                missing;
    const char*        (*name)(int _value);
    int                (*find)(struct d_pack_text _name);
};

// d_tests_imap_command_rule
//   struct: the states a run of commands may be sent in, as RFC 9051 section
// 3 has it: bit n for the state whose value is n.
struct d_tests_imap_command_rule
{
    enum d_imap_command first;
    enum d_imap_command last;
    unsigned            states;
};

/*
ERRORS
  Every error, its name -- the enumerator's suffix, lower-cased -- and where
net_common.h's rules place it.
*/
static const struct d_tests_imap_error_case ERRORS[] =
{
    { D_IMAP_OK,                     "ok",
      D_NET_OK,                       '-' },
    { D_IMAP_ERROR_INVALID_ARGUMENT, "invalid_argument",
      D_NET_ERROR_INVALID_ARGUMENT,   '-' },
    { D_IMAP_ERROR_SINK,             "sink",
      D_NET_ERROR_UNKNOWN,            '-' },
    { D_IMAP_ERROR_BUFFER_TOO_SMALL, "buffer_too_small",
      D_NET_ERROR_INVALID_ARGUMENT,   '-' },
    { D_IMAP_ERROR_MALFORMED,        "malformed",
      D_NET_ERROR_PROTOCOL,           '-' },
    { D_IMAP_ERROR_INCOMPLETE,       "incomplete",
      D_NET_ERROR_WOULD_BLOCK,        '-' },
    { D_IMAP_ERROR_RANGE,            "range",
      D_NET_ERROR_PROTOCOL,           '-' },
    { D_IMAP_ERROR_ENCODING,         "encoding",
      D_NET_ERROR_PROTOCOL,           '-' },
    { D_IMAP_ERROR_LIMIT,            "limit",
      D_NET_ERROR_MESSAGE_TOO_LARGE,  '-' },
    { D_IMAP_ERROR_STATE,            "state",
      D_NET_ERROR_INVALID_ARGUMENT,   '-' },
    { D_IMAP_ERROR_UNSUPPORTED,      "unsupported",
      D_NET_ERROR_UNKNOWN,            '-' },
    { D_IMAP_ERROR_SECURITY,         "security",
      D_NET_ERROR_ACCESS_DENIED,      '-' },
    { D_IMAP_ERROR_AUTH,             "auth",
      D_NET_ERROR_ACCESS_DENIED,      '-' },
    { D_IMAP_ERROR_NO,               "no",
      D_NET_ERROR_UNKNOWN,            's' },
    { D_IMAP_ERROR_BAD,              "bad",
      D_NET_ERROR_PROTOCOL,           's' },
    { D_IMAP_ERROR_BYE,              "bye",
      D_NET_ERROR_CLOSED,             's' },
    { D_IMAP_ERROR_PROTOCOL,         "protocol",
      D_NET_ERROR_PROTOCOL,           '-' },
    { D_IMAP_ERROR_RESOLVE,          "resolve",
      D_NET_ERROR_HOST_UNREACHABLE,   't' },
    { D_IMAP_ERROR_CONNECT,          "connect",
      D_NET_ERROR_CONNECTION_REFUSED, 't' },
    { D_IMAP_ERROR_TLS,              "tls",
      D_NET_ERROR_PROTOCOL,           't' },
    { D_IMAP_ERROR_TIMED_OUT,        "timed_out",
      D_NET_ERROR_TIMED_OUT,          't' },
    { D_IMAP_ERROR_CLOSED,           "closed",
      D_NET_ERROR_CLOSED,             't' },
    { D_IMAP_ERROR_TRANSPORT,        "transport",
      D_NET_ERROR_UNKNOWN,            't' },
    { D_IMAP_ERROR_OUT_OF_MEMORY,    "out_of_memory",
      D_NET_ERROR_OUT_OF_MEMORY,      '-' },
    { D_IMAP_ERROR_UNKNOWN,          "unknown",
      D_NET_ERROR_UNKNOWN,            '-' }
};

/*
STATES, SECURITY_MODES, BACKENDS
  The descriptions and implementation names imap.h gives, by value.
*/
static const char* const STATES[] =
{
    "disconnected", "not authenticated", "authenticated", "selected",
    "logout"
};
static const char* const SECURITY_MODES[] =
{
    "tls", "starttls", "opportunistic", "none"
};
static const char* const BACKENDS[] =
{
    "none", "native", "libcurl", "libetpan", "GNU Mailutils", "VMime"
};

/*
COMMANDS, MECHANISMS, CAPABILITIES
  Wire spellings, by value: RFC 9051's keywords, and the SASL and capability
names each defining RFC gives. UNKNOWN has no keyword, and the LOGIN
command, which is no SASL mechanism, a description.
*/
static const char* const COMMANDS[] =
{
    "",
    "CAPABILITY", "NOOP", "LOGOUT", "ID",
    "STARTTLS", "AUTHENTICATE", "LOGIN",
    "ENABLE", "SELECT", "EXAMINE", "CREATE", "DELETE", "RENAME", "SUBSCRIBE",
    "UNSUBSCRIBE", "LIST", "LSUB", "NAMESPACE", "STATUS", "APPEND", "IDLE",
    "CHECK", "CLOSE", "UNSELECT", "EXPUNGE", "SEARCH", "FETCH", "STORE",
    "COPY", "MOVE", "UID", "SORT", "THREAD"
};
static const char* const MECHANISMS[] =
{
    "login-command", "PLAIN", "LOGIN", "CRAM-MD5", "DIGEST-MD5",
    "SCRAM-SHA-1", "SCRAM-SHA-1-PLUS", "SCRAM-SHA-256", "SCRAM-SHA-256-PLUS",
    "XOAUTH2", "OAUTHBEARER", "GSSAPI", "EXTERNAL", "NTLM", "ANONYMOUS"
};
static const char* const CAPABILITIES[] =
{
    "IMAP4rev1", "IMAP4rev2", "STARTTLS", "LOGINDISABLED", "SASL-IR", "IDLE",
    "NAMESPACE", "UIDPLUS", "MOVE", "UNSELECT", "ENABLE", "CONDSTORE",
    "QRESYNC", "LITERAL+", "LITERAL-", "ID", "CHILDREN", "SPECIAL-USE",
    "CREATE-SPECIAL-USE", "LIST-EXTENDED", "LIST-STATUS", "ESEARCH",
    "SEARCHRES", "SORT", "THREAD=ORDEREDSUBJECT", "THREAD=REFERENCES",
    "COMPRESS=DEFLATE", "UTF8=ACCEPT", "UTF8=ONLY", "QUOTA", "ACL", "BINARY",
    "CATENATE", "MULTIAPPEND", "WITHIN", "METADATA", "METADATA-SERVER",
    "NOTIFY", "OBJECTID", "SAVEDATE", "STATUS=SIZE", "APPENDLIMIT", "PREVIEW",
    "UNAUTHENTICATE", "LIST-MYRIGHTS", "REPLACE"
};

/*
CONDITIONS, CODES
  Wire spellings, by value; NONE, and OTHER, have none.
*/
static const char* const CONDITIONS[] =
{
    "", "OK", "NO", "BAD", "PREAUTH", "BYE"
};
static const char* const CODES[] =
{
    "", "", "ALERT", "BADCHARSET", "CAPABILITY", "PARSE", "PERMANENTFLAGS",
    "READ-ONLY", "READ-WRITE", "TRYCREATE", "UIDNEXT", "UIDVALIDITY",
    "UNSEEN", "HASCHILDREN", "APPENDUID", "COPYUID", "UIDNOTSTICKY",
    "UNAVAILABLE", "AUTHENTICATIONFAILED", "AUTHORIZATIONFAILED", "EXPIRED",
    "PRIVACYREQUIRED", "CONTACTADMIN", "NOPERM", "INUSE", "EXPUNGEISSUED",
    "CORRUPTION", "SERVERBUG", "CLIENTBUG", "CANNOT", "LIMIT", "OVERQUOTA",
    "ALREADYEXISTS", "NONEXISTENT", "HIGHESTMODSEQ", "NOMODSEQ", "MODIFIED",
    "CLOSED", "UNKNOWN-CTE", "BADURL", "TOOBIG", "COMPRESSIONACTIVE",
    "NOTSAVED", "USEATTR", "MAILBOXID"
};

/*
FLAGS, ATTRIBUTES, STATUS_ITEMS
  Wire spellings, by value, with their backslash or dollar.
*/
static const char* const FLAGS[] =
{
    "\\Seen", "\\Answered", "\\Flagged", "\\Deleted", "\\Draft", "\\Recent",
    "\\*", "$Forwarded", "$MDNSent", "$Junk", "$NotJunk", "$Phishing"
};
static const char* const ATTRIBUTES[] =
{
    "\\Noinferiors", "\\Noselect", "\\Marked", "\\Unmarked", "\\HasChildren",
    "\\HasNoChildren", "\\NonExistent", "\\Subscribed", "\\Remote", "\\All",
    "\\Archive", "\\Drafts", "\\Flagged", "\\Junk", "\\Sent", "\\Trash",
    "\\Important"
};
static const char* const STATUS_ITEMS[] =
{
    "MESSAGES", "RECENT", "UIDNEXT", "UIDVALIDITY", "UNSEEN", "DELETED",
    "SIZE", "HIGHESTMODSEQ"
};

D_STATIC_ASSERT((sizeof(COMMANDS) / sizeof(COMMANDS[0])) ==
                    D_IMAP_COMMAND_COUNT,
                "COMMANDS: one name per command");
D_STATIC_ASSERT((sizeof(CAPABILITIES) / sizeof(CAPABILITIES[0])) ==
                    D_IMAP_CAPABILITY_COUNT,
                "CAPABILITIES: one name per capability");
D_STATIC_ASSERT((sizeof(CODES) / sizeof(CODES[0])) == D_IMAP_CODE_COUNT,
                "CODES: one name per code");

/*
RULES
  RFC 9051 section 3: CAPABILITY, NOOP, LOGOUT and ID in any of the three
command states; STARTTLS, AUTHENTICATE and LOGIN before authenticating;
ENABLE once authenticated, before selecting; SELECT through IDLE once
authenticated; CHECK through THREAD with a mailbox selected.
*/
static const struct d_tests_imap_command_rule RULES[] =
{
    { D_IMAP_COMMAND_UNKNOWN,    D_IMAP_COMMAND_UNKNOWN, 0x0u },
    { D_IMAP_COMMAND_CAPABILITY, D_IMAP_COMMAND_ID,      0xEu },
    { D_IMAP_COMMAND_STARTTLS,   D_IMAP_COMMAND_LOGIN,   0x2u },
    { D_IMAP_COMMAND_ENABLE,     D_IMAP_COMMAND_ENABLE,  0x4u },
    { D_IMAP_COMMAND_SELECT,     D_IMAP_COMMAND_IDLE,    0xCu },
    { D_IMAP_COMMAND_CHECK,      D_IMAP_COMMAND_THREAD,  0x8u }
};

/*
d_tests_imap_state_name, d_tests_imap_security_name,
d_tests_imap_backend_name, d_tests_imap_command_name,
d_tests_imap_auth_name, d_tests_imap_capability_name,
d_tests_imap_condition_name, d_tests_imap_code_name,
d_tests_imap_flag_name, d_tests_imap_attribute_name,
d_tests_imap_status_item_name
  Each enumeration's name function, over an int, so that one table can hold
them all.
*/
static const char*
d_tests_imap_state_name(
    int _value
)
{
    return d_imap_state_name((enum d_imap_state)_value);
}

static const char*
d_tests_imap_security_name(
    int _value
)
{
    return d_imap_security_name((enum d_imap_security)_value);
}

static const char*
d_tests_imap_backend_name(
    int _value
)
{
    return d_imap_backend_name((enum d_imap_backend)_value);
}

static const char*
d_tests_imap_command_name(
    int _value
)
{
    return d_imap_command_name((enum d_imap_command)_value);
}

static const char*
d_tests_imap_auth_name(
    int _value
)
{
    return d_imap_auth_name((enum d_imap_auth_mechanism)_value);
}

static const char*
d_tests_imap_capability_name(
    int _value
)
{
    return d_imap_capability_name((enum d_imap_capability)_value);
}

static const char*
d_tests_imap_condition_name(
    int _value
)
{
    return d_imap_condition_name((enum d_imap_condition)_value);
}

static const char*
d_tests_imap_code_name(
    int _value
)
{
    return d_imap_code_name((enum d_imap_code)_value);
}

static const char*
d_tests_imap_flag_name(
    int _value
)
{
    return d_imap_flag_name((enum d_imap_flag)_value);
}

static const char*
d_tests_imap_attribute_name(
    int _value
)
{
    return d_imap_mailbox_attribute_name(
               (enum d_imap_mailbox_attribute)_value);
}

static const char*
d_tests_imap_status_item_name(
    int _value
)
{
    return d_imap_status_item_name((enum d_imap_status_item)_value);
}

/*
d_tests_imap_command_find, d_tests_imap_condition_find,
d_tests_imap_code_find
  The lookups that answer with an enumerator, as an int.
*/
static int
d_tests_imap_command_find(
    struct d_pack_text _name
)
{
    return (int)d_imap_command_from_name(_name);
}

static int
d_tests_imap_condition_find(
    struct d_pack_text _name
)
{
    return (int)d_imap_condition_from_name(_name);
}

static int
d_tests_imap_code_find(
    struct d_pack_text _name
)
{
    return (int)d_imap_code_from_name(_name);
}

/*
d_tests_imap_auth_find, d_tests_imap_capability_find,
d_tests_imap_flag_find, d_tests_imap_attribute_find,
d_tests_imap_status_item_find
  The lookups that answer whether they found a name, as an int: the value
found, or -1.
*/
static int
d_tests_imap_auth_find(
    struct d_pack_text _name
)
{
    enum d_imap_auth_mechanism found = D_IMAP_AUTH_LOGIN_COMMAND;

    return (d_imap_auth_from_name(_name,
                                  &found))
               ? (int)found
               : -1;
}

static int
d_tests_imap_capability_find(
    struct d_pack_text _name
)
{
    enum d_imap_capability found = D_IMAP_CAPABILITY_IMAP4REV1;

    return (d_imap_capability_from_name(_name,
                                        &found))
               ? (int)found
               : -1;
}

static int
d_tests_imap_flag_find(
    struct d_pack_text _name
)
{
    enum d_imap_flag found = D_IMAP_FLAG_SEEN;

    return (d_imap_flag_from_name(_name,
                                  &found))
               ? (int)found
               : -1;
}

static int
d_tests_imap_attribute_find(
    struct d_pack_text _name
)
{
    enum d_imap_mailbox_attribute found = D_IMAP_MAILBOX_NOINFERIORS;

    return (d_imap_mailbox_attribute_from_name(_name,
                                               &found))
               ? (int)found
               : -1;
}

static int
d_tests_imap_status_item_find(
    struct d_pack_text _name
)
{
    enum d_imap_status_item found = D_IMAP_STATUS_MESSAGES;

    return (d_imap_status_item_from_name(_name,
                                         &found))
               ? (int)found
               : -1;
}

/*
VOCABULARIES
  Every enumeration with names but the errors, whose own table holds more.
An enumeration without a lookup starts finding at its count, which is to
say never.
*/
static const struct d_tests_imap_vocabulary VOCABULARIES[] =
{
    { "d_imap_state_name",       STATES,         D_IMAP_STATE_COUNT,
      D_IMAP_STATE_COUNT,        "unknown",      0,
      d_tests_imap_state_name,   NULL },
    { "d_imap_security_name",    SECURITY_MODES, D_IMAP_SECURITY_COUNT,
      D_IMAP_SECURITY_COUNT,     "unknown",      0,
      d_tests_imap_security_name, NULL },
    { "d_imap_backend_name",     BACKENDS,       D_IMAP_BACKEND_COUNT,
      D_IMAP_BACKEND_COUNT,      "unknown",      0,
      d_tests_imap_backend_name, NULL },
    { "d_imap_command_name",     COMMANDS,       D_IMAP_COMMAND_COUNT,
      1,                         "",             D_IMAP_COMMAND_UNKNOWN,
      d_tests_imap_command_name, d_tests_imap_command_find },
    { "d_imap_auth_name",        MECHANISMS,     D_IMAP_AUTH_COUNT,
      1,                         "",             -1,
      d_tests_imap_auth_name,    d_tests_imap_auth_find },
    { "d_imap_capability_name",  CAPABILITIES,   D_IMAP_CAPABILITY_COUNT,
      0,                         "",             -1,
      d_tests_imap_capability_name, d_tests_imap_capability_find },
    { "d_imap_condition_name",   CONDITIONS,     D_IMAP_CONDITION_COUNT,
      1,                         "",             D_IMAP_CONDITION_NONE,
      d_tests_imap_condition_name, d_tests_imap_condition_find },
    { "d_imap_code_name",        CODES,          D_IMAP_CODE_COUNT,
      2,                         "",             D_IMAP_CODE_OTHER,
      d_tests_imap_code_name,    d_tests_imap_code_find },
    { "d_imap_flag_name",        FLAGS,          D_IMAP_FLAG_COUNT,
      0,                         "",             -1,
      d_tests_imap_flag_name,    d_tests_imap_flag_find },
    { "d_imap_mailbox_attribute_name", ATTRIBUTES,
      D_IMAP_MAILBOX_ATTRIBUTE_COUNT, 0,         "",             -1,
      d_tests_imap_attribute_name, d_tests_imap_attribute_find },
    { "d_imap_status_item_name", STATUS_ITEMS,   D_IMAP_STATUS_ITEM_COUNT,
      0,                         "",             -1,
      d_tests_imap_status_item_name, d_tests_imap_status_item_find }
};

/*
d_tests_imap_lower
  `_text`, ASCII letters folded to lower case, in `_buffer`, which holds 64
octets; a longer text is cut short.
*/
static struct d_pack_text
d_tests_imap_lower(
    const char* _text,
    char*       _buffer
)
{
    size_t length = 0u;

    // each octet, until the text or the buffer ends
    while ( (_text[length] != '\0') &&
            (length < 63u) )
    {
        const char octet = _text[length];

        _buffer[length] = ( (octet >= 'A') &&
                            (octet <= 'Z') )
                              ? (char)(octet - 'A' + 'a')
                              : octet;
        ++length;
    }

    return d_pack_text_from_span(_buffer,
                                 length);
}

/*
d_tests_imap_vocabulary_check
  One enumeration: every value's name, the lookup of each name as spelled
and in lower case, and a value and a name outside it.
*/
static bool
d_tests_imap_vocabulary_check(
    const struct d_tests_imap_vocabulary* _vocabulary,
    struct d_test_counter*                _counter
)
{
    char lower[64] = { 0 };
    bool named     = true;
    bool found     = true;
    bool folded    = true;
    bool result    = true;

    // every value's name, and each findable name looked up again
    for (int value = 0; value < _vocabulary->count; ++value)
    {
        const char* name = _vocabulary->names[value];

        named = ( (strcmp(_vocabulary->name(value),
                          name) == 0) &&
                  (named) );

        // only from the first value a lookup can find
        if ( (_vocabulary->find != NULL) &&
             (value >= _vocabulary->first) )
        {
            found  = ( (_vocabulary->find(d_pack_text_from_cstr(name)) ==
                        value) &&
                       (found) );
            folded = ( (_vocabulary->find(d_tests_imap_lower(name,
                                                             lower)) ==
                        value) &&
                       (folded) );
        }
    }

    const bool outside =
        ( (strcmp(_vocabulary->name(_vocabulary->count),
                  _vocabulary->unknown) == 0) &&
          (strcmp(_vocabulary->name(-1),
                  _vocabulary->unknown) == 0) &&
          ( (_vocabulary->find == NULL) ||
            (_vocabulary->find(d_pack_text_from_cstr("X-NOT-A-NAME")) ==
             _vocabulary->missing) ) );

    result = d_tests_net_assert(named,
                                _vocabulary->what,
                                "each value is named as its RFC spells it",
                                _counter) && result;
    result = d_tests_net_assert(found,
                                _vocabulary->what,
                                "each name looks up its value",
                                _counter) && result;
    result = d_tests_net_assert(folded,
                                _vocabulary->what,
                                "in lower case too",
                                _counter) && result;
    result = d_tests_net_assert(outside,
                                _vocabulary->what,
                                "a value or a name outside it is unknown",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_lookup_refusals
  The lookups' particular refusals: the LOGIN command is no SASL mechanism,
"AUTH=" and "=n" are not part of a name, a flag's name keeps its backslash,
an empty code name is no code, and a NULL out-pointer finds nothing.
*/
static bool
d_tests_imap_lookup_refusals(
    struct d_test_counter* _counter
)
{
    enum d_imap_flag found  = D_IMAP_FLAG_SEEN;
    bool             result = true;

    const bool refused =
        ( (d_tests_imap_auth_find(d_pack_text_from_cstr("login-command")) ==
           -1)                                                          &&
          (d_tests_imap_auth_find(d_pack_text_from_cstr("AUTH=PLAIN")) ==
           -1)                                                          &&
          (d_tests_imap_capability_find(
               d_pack_text_from_cstr("APPENDLIMIT=5")) == -1)           &&
          (d_tests_imap_capability_find(
               d_pack_text_from_cstr("AUTH=PLAIN")) == -1)              &&
          (d_tests_imap_flag_find(d_pack_text_from_cstr("Seen")) == -1) &&
          (d_imap_code_from_name(d_pack_text_from_cstr("")) ==
           D_IMAP_CODE_NONE) );
    const bool null_out =
        ( (!d_imap_auth_from_name(d_pack_text_from_cstr("PLAIN"),
                                  NULL))                        &&
          (!d_imap_capability_from_name(d_pack_text_from_cstr("IDLE"),
                                        NULL))                  &&
          (!d_imap_flag_from_name(d_pack_text_from_cstr("\\Seen"),
                                  NULL))                        &&
          (!d_imap_mailbox_attribute_from_name(
               d_pack_text_from_cstr("\\Sent"),
               NULL))                                           &&
          (!d_imap_status_item_from_name(d_pack_text_from_cstr("SIZE"),
                                         NULL))                 &&
          (d_imap_flag_from_name(d_pack_text_from_cstr("$junk"),
                                 &found))                       &&
          (found == D_IMAP_FLAG_JUNK) );

    result = d_tests_net_assert(refused,
                                "lookups",
                                "refuse what only resembles a name",
                                _counter) && result;
    result = d_tests_net_assert(null_out,
                                "lookups",
                                "find nothing without somewhere to put it",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_message_is_own
  Whether error `_index`'s message is a sentence unlike any other's.
*/
static bool
d_tests_imap_message_is_own(
    size_t _index
)
{
    const size_t count   = sizeof(ERRORS) / sizeof(ERRORS[0]);
    const char*  message = d_imap_error_message(ERRORS[_index].error);
    bool         own     = (message[0] != '\0');

    // against every other error's
    for (size_t j = 0u; j < count; ++j)
    {
        own = ( ( (j == _index) ||
                  (strcmp(message,
                          d_imap_error_message(ERRORS[j].error)) != 0) ) &&
                (own) );
    }

    return own;
}

/*
d_tests_imap_errors_outside
  A value outside the enum is named and described "unknown", belongs to
neither class and maps to D_NET_ERROR_UNKNOWN; and a condition becomes the
error a command meeting it reports.
*/
static bool
d_tests_imap_errors_outside(
    struct d_test_counter* _counter
)
{
    static const int OUTSIDE[] = { -1, D_IMAP_ERROR_COUNT, 0x7FFF };
    bool             unknown   = true;
    bool             result    = true;

    // each value no error has
    for (size_t i = 0u; i < (sizeof(OUTSIDE) / sizeof(OUTSIDE[0])); ++i)
    {
        const enum d_imap_error error = (enum d_imap_error)OUTSIDE[i];

        unknown = ( (strcmp(d_imap_error_name(error),
                            "unknown") == 0)                    &&
                    (strcmp(d_imap_error_message(error),
                            "unknown") == 0)                    &&
                    (!d_imap_error_is_server(error))            &&
                    (!d_imap_error_is_transport(error))         &&
                    (d_imap_error_to_net(error) ==
                     D_NET_ERROR_UNKNOWN)                       &&
                    (unknown) );
    }

    const bool conditions =
        ( (d_imap_error_from_condition(D_IMAP_CONDITION_NO) ==
           D_IMAP_ERROR_NO)                                        &&
          (d_imap_error_from_condition(D_IMAP_CONDITION_BAD) ==
           D_IMAP_ERROR_BAD)                                       &&
          (d_imap_error_from_condition(D_IMAP_CONDITION_BYE) ==
           D_IMAP_ERROR_BYE)                                       &&
          (d_imap_error_from_condition(D_IMAP_CONDITION_OK) ==
           D_IMAP_OK)                                              &&
          (d_imap_error_from_condition(D_IMAP_CONDITION_PREAUTH) ==
           D_IMAP_OK)                                              &&
          (d_imap_error_from_condition(D_IMAP_CONDITION_NONE) ==
           D_IMAP_OK)                                              &&
          (d_imap_error_from_condition(
               (enum d_imap_condition)D_IMAP_CONDITION_COUNT) ==
           D_IMAP_OK) );

    result = d_tests_net_assert(unknown,
                                "d_imap_error_name",
                                "a value outside the enum is unknown",
                                _counter) && result;
    result = d_tests_net_assert(conditions,
                                "d_imap_error_from_condition",
                                "NO, BAD and BYE fail a command; nothing else",
                                _counter) && result;

    return result;
}

/*
d_tests_imap_backends
  Neither NONE nor VMime, which is C++'s, is ever available to C, nor is a
value outside the enum; the default is NONE or an available backend.
*/
static bool
d_tests_imap_backends(
    struct d_test_counter* _counter
)
{
    const enum d_imap_backend fallback = d_imap_backend_default();
    const bool                never    =
        ( (!d_imap_backend_is_available(D_IMAP_BACKEND_NONE))  &&
          (!d_imap_backend_is_available(D_IMAP_BACKEND_VMIME)) &&
          (!d_imap_backend_is_available(
                (enum d_imap_backend)D_IMAP_BACKEND_COUNT)) );
    const bool usable =
        ( (fallback == D_IMAP_BACKEND_NONE) ||
          (d_imap_backend_is_available(fallback)) );
    bool result = true;

    result = d_tests_net_assert(never,
                                "d_imap_backend_is_available",
                                "never NONE, VMime or an unknown backend",
                                _counter) && result;
    result = d_tests_net_assert(usable,
                                "d_imap_backend_default",
                                "is NONE or an available backend",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_errors
  Tests the following:
  - the table lists every error, in the enum's order
  - each error is named by its enumerator's suffix, in lower case
  - each maps to its d_net_error by net_common.h's rules
  - NO, BAD and BYE are the server's errors, RESOLVE through TRANSPORT the
    transport's, and no other error is either
  - each error's message is a sentence of its own
  - a value outside the enum, conditions, and the backends' availability
*/
bool
d_tests_sa_imap_errors(
    struct d_test_counter* _counter
)
{
    const size_t count   = sizeof(ERRORS) / sizeof(ERRORS[0]);
    bool         table   = (count == (size_t)D_IMAP_ERROR_COUNT);
    bool         named   = true;
    bool         mapped  = true;
    bool         classed = true;
    bool         own     = true;
    bool         result  = true;

    // each row, in the enum's order
    for (size_t i = 0u; i < count; ++i)
    {
        const enum d_imap_error error = ERRORS[i].error;

        table   = ( (error == (enum d_imap_error)i) &&
                    (table) );
        named   = ( (strcmp(d_imap_error_name(error),
                            ERRORS[i].name) == 0) &&
                    (named) );
        mapped  = ( (d_imap_error_to_net(error) == ERRORS[i].net) &&
                    (mapped) );
        classed = ( (d_imap_error_is_server(error) ==
                     (ERRORS[i].kind == 's'))          &&
                    (d_imap_error_is_transport(error) ==
                     (ERRORS[i].kind == 't'))          &&
                    (classed) );
        own     = ( (d_tests_imap_message_is_own(i)) &&
                    (own) );
    }

    result = d_tests_net_assert(table,
                                "d_imap_error",
                                "the table lists every error, in order",
                                _counter) && result;
    result = d_tests_net_assert(named,
                                "d_imap_error_name",
                                "each is its enumerator's suffix, lower-cased",
                                _counter) && result;
    result = d_tests_net_assert(mapped,
                                "d_imap_error_to_net",
                                "places every error by net_common.h's rules",
                                _counter) && result;
    result = d_tests_net_assert(classed,
                                "d_imap_error_is_server",
                                "the server's and the transport's errors",
                                _counter) && result;
    result = d_tests_net_assert(own,
                                "d_imap_error_message",
                                "each error has a sentence of its own",
                                _counter) && result;
    result = d_tests_imap_errors_outside(_counter) && result;
    result = d_tests_imap_backends(_counter) && result;

    return result;
}

/*
d_tests_sa_imap_names
  Tests the following:
  - every enumeration's values are named as their RFCs spell them
  - each name looks up its value, as spelled and in lower case
  - a value or a name outside an enumeration reads as unknown
  - the lookups refuse what only resembles a name
*/
bool
d_tests_sa_imap_names(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(VOCABULARIES) / sizeof(VOCABULARIES[0]);
    bool         result = true;

    // each enumeration in turn
    for (size_t i = 0u; i < count; ++i)
    {
        result = d_tests_imap_vocabulary_check(&VOCABULARIES[i],
                                               _counter) && result;
    }

    result = d_tests_imap_lookup_refusals(_counter) && result;

    return result;
}

/*
d_tests_sa_imap_command_states
  Tests the following:
  - each command is valid in exactly the states RFC 9051 section 3 gives it
  - no command is valid before the greeting, after LOGOUT, or in a state
    outside the enum, and no command outside the enum is valid anywhere
*/
bool
d_tests_sa_imap_command_states(
    struct d_test_counter* _counter
)
{
    const size_t rules  = sizeof(RULES) / sizeof(RULES[0]);
    bool         ruled  = true;
    bool         result = true;

    // every command, and one past them
    for (int command = 0; command <= D_IMAP_COMMAND_COUNT; ++command)
    {
        unsigned states = 0u;

        // the rule covering the command; none covers one past them
        for (size_t r = 0u; r < rules; ++r)
        {
            if ( (command >= (int)RULES[r].first) &&
                 (command <= (int)RULES[r].last) )
            {
                states = RULES[r].states;
            }
        }

        // every state, and one past them
        for (int state = 0; state <= D_IMAP_STATE_COUNT; ++state)
        {
            const bool valid =
                d_imap_command_is_valid_in((enum d_imap_command)command,
                                           (enum d_imap_state)state);

            ruled = ( (valid == (((states >> state) & 1u) != 0u)) &&
                      (ruled) );
        }
    }

    result = d_tests_net_assert(ruled,
                                "d_imap_command_is_valid_in",
                                "each command in RFC 9051's states only",
                                _counter) && result;

    return result;
}
