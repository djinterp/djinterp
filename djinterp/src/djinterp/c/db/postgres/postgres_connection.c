/*******************************************************************************
* djinterp [c]                                             postgres_connection.c
*
* Implementation of postgres_connection.h.
*   Every connect goes through PQconnectdbParams, the layer's defaults first
* in its arrays: libpq takes the last value given for a keyword, so whatever
* the caller names -- in the struct, in a connection string, which libpq
* expands where the dbname keyword holds one, or in keywords and a string
* read here -- overrides them. A default is
* given only where libpq's own environment variable for it is unset, so that
* PGCONNECT_TIMEOUT and PGCLIENTENCODING keep their meaning, and the layer's
* startup option joins PGOPTIONS rather than replacing it.
*   A connection that fails is finished at once, its reason copied out first,
* so that a caller never holds a connection that does not work.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_connection.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.21
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_connection.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // snprintf
#include <stdlib.h>  // getenv, malloc, free
#include <string.h>  // memcpy, strcat, strchr, strlen, strncmp,
                     // strcmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_connection.h"  // the defaults
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing


#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_POSTGRES_KEYWORDS
//   constant: room for every keyword a connect passes -- the layer's three
// defaults, six fields or a connection string, and the terminating NULL.
#define D_INTERNAL_POSTGRES_KEYWORDS 10u

// d_internal_postgres_params
//   struct: the keyword and value arrays PQconnectdbParams takes, as they
// fill, with the text the layer made for them.
struct d_internal_postgres_params
{
    const char* keywords[D_INTERNAL_POSTGRES_KEYWORDS];  // the keywords
    const char* values[D_INTERNAL_POSTGRES_KEYWORDS];    // their values
    size_t      count;                                   // pairs so far
    char        timeout[24];                             // the connect
                                                         // timeout, as text
    char        port[24];                                // the port, as text
    char*       startup;                                 // the options
                                                         // value, allocated
};

/*
d_internal_postgres_add
  One keyword and value; a NULL or empty value is left out, as libpq would
ignore it anyway.
*/
static void
d_internal_postgres_add(
    struct d_internal_postgres_params* _params,
    const char*                        _keyword,
    const char*                        _value
)
{
    // only a value there is, and room for it beside the terminator
    if ( (_value)                                                 &&
         (_value[0] != '\0')                                      &&
         (_params->count + 1u < D_INTERNAL_POSTGRES_KEYWORDS) )
    {
        _params->keywords[_params->count] = _keyword;
        _params->values[_params->count]   = _value;
        ++_params->count;
    }

    return;
}

/*
d_internal_postgres_unset
  Whether an environment variable is unset or empty, which is how libpq
reads it.
*/
static bool
d_internal_postgres_unset(
    const char* _name
)
{
    const char* const value = getenv(_name);

    return ( (!value) ||
             (value[0] == '\0') );
}

/*
d_internal_postgres_startup
  "-c extra_float_digits=3", which makes a server before 12 write a double
with every digit it needs to be read back exactly, as 12 and later do by
default; "-c client_min_messages=<level>" where the build names a level; and
PGOPTIONS after them, whose own -c settings then win. 53200 when memory ran
out.
*/
static int
d_internal_postgres_startup(
    char** _startup
)
{
    static const char  digits[] = "-c extra_float_digits=3";
    static const char  prefix[] = " -c client_min_messages=";
    const char* const  level    = D_INTERNAL_POSTGRES_CLIENT_MIN_MESSAGES;
    const char* const  more     = getenv("PGOPTIONS");
    const size_t       leveled  = (level[0] != '\0')
                                      ? (sizeof(prefix) - 1u) + strlen(level)
                                      : 0u;
    const size_t       extra    = ( (more) && (more[0] != '\0') )
                                      ? strlen(more) + 1u
                                      : 0u;
    const size_t       length   = (sizeof(digits) - 1u) + leveled;
    char* const        text     = (char*)malloc(length + extra + 1u);

    *_startup = NULL;

    if (!text)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    memcpy(text, digits, sizeof(digits) - 1u);

    // the build's level, where it names one
    if (leveled)
    {
        memcpy(text + sizeof(digits) - 1u, prefix, sizeof(prefix) - 1u);
        memcpy(text + sizeof(digits) - 1u + sizeof(prefix) - 1u,
               level,
               strlen(level));
    }

    // PGOPTIONS after the layer's, so that its settings win
    if (extra)
    {
        text[length] = ' ';
        memcpy(text + length + 1u, more, extra - 1u);
    }

    text[length + extra] = '\0';
    *_startup            = text;

    return 0;
}

/*
d_internal_postgres_defaults
  The layer's three defaults, each where the build asks for it and libpq's
environment does not already say.
*/
static int
d_internal_postgres_defaults(
    struct d_internal_postgres_params* _params
)
{
    const char* const encoding = D_INTERNAL_POSTGRES_CLIENT_ENCODING;
    const int         result   = d_internal_postgres_startup(&_params->startup);

    // the startup option, or the memory it needed
    if (result != 0)
    {
        return result;
    }

    if ( (D_INTERNAL_POSTGRES_CONNECT_TIMEOUT > 0u) &&
         (d_internal_postgres_unset("PGCONNECT_TIMEOUT")) )
    {
        (void)snprintf(_params->timeout,
                       sizeof(_params->timeout),
                       "%u",
                       D_INTERNAL_POSTGRES_CONNECT_TIMEOUT);
        d_internal_postgres_add(_params, "connect_timeout", _params->timeout);
    }

    if (d_internal_postgres_unset("PGCLIENTENCODING"))
    {
        d_internal_postgres_add(_params, "client_encoding", encoding);
    }

    d_internal_postgres_add(_params, "options", _params->startup);

    return 0;
}

/*
d_internal_postgres_add_options
  Each field the options give, the port written out as text first; a port
of 0 is libpq's to choose.
*/
static void
d_internal_postgres_add_options(
    struct d_internal_postgres_params* _params,
    const struct d_postgres_options*   _options
)
{
    d_internal_postgres_add(_params, "host", _options->host);
    d_internal_postgres_add(_params, "user", _options->user);
    d_internal_postgres_add(_params, "password", _options->password);
    d_internal_postgres_add(_params, "dbname", _options->database);
    d_internal_postgres_add(_params, "sslmode", _options->sslmode);

    if (_options->port)
    {
        (void)snprintf(_params->port,
                       sizeof(_params->port),
                       "%u",
                       _options->port);
        d_internal_postgres_add(_params, "port", _params->port);
    }

    return;
}

/*
d_internal_postgres_copy_message
  libpq's message, without the newline it ends with, in memory the caller
releases with d_db_free; NULL when memory ran out.
*/
static char*
d_internal_postgres_copy_message(
    const char* _message
)
{
    size_t length = strlen(_message);

    // the trailing newlines go
    while ( (length > 0u) &&
            (_message[length - 1u] == '\n') )
    {
        --length;
    }

    char* const copy = (char*)malloc(length + 1u);

    if (copy)
    {
        memcpy(copy, _message, length);
        copy[length] = '\0';
    }

    return copy;
}

/*
d_internal_postgres_float_digits
  The options a connection was made with are the layer's, which set
extra_float_digits, unless the caller named options of its own, which
replace them; then, unless they set it themselves, the layer sets it to 3 on
the connection, so that every double the server writes reads back exactly.
*/
static int
d_internal_postgres_float_digits(
    PGconn* _native
)
{
    PQconninfoOption* const options = PQconninfo(_native);
    bool                    named   = false;

    // libpq could not describe the connection: memory
    if (!options)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    for (const PQconninfoOption* option = options;
         (option) && (option->keyword);
         ++option)
    {
        if ( (strcmp(option->keyword, "options") == 0) &&
             (option->val) &&
             (strstr(option->val, "extra_float_digits")) )
        {
            named = true;

            break;
        }
    }

    PQconninfoFree(options);

    return (named) ? 0
                   : d_internal_postgres_send(_native,
                                              "SET extra_float_digits = 3",
                                              NULL);
}

/*
d_internal_postgres_connected
  What PQconnectdbParams made, kept when it works and finished when it does
not, its reason copied out first where the caller wants it.
*/
static int
d_internal_postgres_connected(
    PGconn*          _native,
    struct pg_conn** _db,
    char**           _message
)
{
    // libpq could not allocate even the connection
    if (!_native)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    // a connection that failed is not handed back
    if (PQstatus(_native) != CONNECTION_OK)
    {
        if (_message)
        {
            *_message = d_internal_postgres_copy_message(
                            PQerrorMessage(_native));
        }

        PQfinish(_native);

        return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
    }

    // the caller's own options replaced the layer's: the digits, by SET
    const int digits = d_internal_postgres_float_digits(_native);

    if (digits != 0)
    {
        if (_message)
        {
            *_message = d_internal_postgres_copy_message(
                            PQerrorMessage(_native));
        }

        PQfinish(_native);

        return digits;
    }

    *_db = _native;

    return 0;
}

/*
d_internal_postgres_is_conninfo
  Whether libpq reads a string as a connection string rather than a database
name: a URI, or keywords, which an equals sign marks.
*/
static bool
d_internal_postgres_is_conninfo(
    const char* _text
)
{
    return ( (strchr(_text, '=') != NULL)                 ||
             (strncmp(_text, "postgresql://", 13u) == 0)  ||
             (strncmp(_text, "postgres://", 11u) == 0) );
}

/*
d_internal_postgres_parse_check
  A connection string libpq cannot read is the caller's argument, its reason
copied out where the caller wants it; libpq answers no reason at all only
when memory ran out.
*/
static int
d_internal_postgres_parse_check(
    const char* _conninfo,
    char**      _message
)
{
    char*                   reason = NULL;
    PQconninfoOption* const parsed = PQconninfoParse(_conninfo, &reason);

    // readable
    if (parsed)
    {
        PQconninfoFree(parsed);

        return 0;
    }

    if (!reason)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    if (_message)
    {
        *_message = d_internal_postgres_copy_message(reason);
    }

    PQfreemem(reason);

    return D_DB_INVALID_ARGUMENT;
}

/*
d_internal_postgres_check_keywords
  Every keyword must be one libpq knows. libpq's keywords are lower-case
letters, digits and underscores, so anything else is refused here, in
libpq's own words; the rest are read by PQconninfoParse, as a string that
gives each an empty value, which names the first libpq does not know.
*/
static int
d_internal_postgres_check_keywords(
    const char* const* _keywords,
    size_t             _count,
    char**             _message
)
{
    size_t size = 1u;
    size_t i    = 0u;

    // each keyword there, and spelled as libpq spells its keywords
    for (i = 0u; i < _count; ++i)
    {
        const char* letter = _keywords[i];

        if ( (!letter) ||
             (letter[0] == '\0') )
        {
            return D_DB_INVALID_ARGUMENT;
        }

        while ( ( (*letter >= 'a') && (*letter <= 'z') ) ||
                ( (*letter >= '0') && (*letter <= '9') ) ||
                (*letter == '_') )
        {
            ++letter;
        }

        if (*letter != '\0')
        {
            if (_message)
            {
                const size_t length = strlen(_keywords[i]) + 32u;

                *_message = (char*)malloc(length);

                if (*_message)
                {
                    (void)snprintf(*_message,
                                   length,
                                   "invalid connection option \"%s\"",
                                   _keywords[i]);
                }
            }

            return D_DB_INVALID_ARGUMENT;
        }

        size += strlen(_keywords[i]) + 4u;
    }

    // nothing to read
    if (_count == 0u)
    {
        return 0;
    }

    char* const probe = (char*)malloc(size);

    if (!probe)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    probe[0] = '\0';

    for (i = 0u; i < _count; ++i)
    {
        (void)strcat(probe, _keywords[i]);
        (void)strcat(probe, "='' ");
    }

    const int result = d_internal_postgres_parse_check(probe, _message);

    free(probe);

    return result;
}

//==============================================================================
// 1.  CONNECTIONS
//==============================================================================

// 1.2    Connecting and closing
//------------------------------------------------------------------------------
/*
d_postgres_connect
  The defaults, then each field the struct gives; libpq takes the rest from
its environment.
*/
int
d_postgres_connect(
    const struct d_postgres_options* _options,
    struct pg_conn**                 _db,
    char**                           _message
)
{
    struct d_internal_postgres_params params =
    {
        { NULL }, { NULL }, 0u, "", "", NULL
    };

    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_db = NULL;

    const int result = d_internal_postgres_defaults(&params);

    // the defaults, or the memory they needed
    if (result != 0)
    {
        return result;
    }

    // NULL options give no fields
    if (_options)
    {
        d_internal_postgres_add_options(&params, _options);
    }

    PGconn* const native = PQconnectdbParams(params.keywords,
                                             params.values,
                                             0);

    free(params.startup);

    return d_internal_postgres_connected(native, _db, _message);
}

/*
d_postgres_connect_string
  The defaults, then the string as the dbname keyword, which libpq expands
into its parts -- so the string's own parts override the defaults -- or
reads as a database name when it is a single word. A string libpq cannot
read is checked first, so its reason is the caller's, not a failed
connection's.
*/
int
d_postgres_connect_string(
    const char*      _conninfo,
    struct pg_conn** _db,
    char**           _message
)
{
    struct d_internal_postgres_params params =
    {
        { NULL }, { NULL }, 0u, "", "", NULL
    };
    const bool                        given  = ( (_conninfo) &&
                                                 (_conninfo[0] != '\0') );

    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_db = NULL;

    int result = ( (given) &&
                   (d_internal_postgres_is_conninfo(_conninfo)) )
                     ? d_internal_postgres_parse_check(_conninfo, _message)
                     : 0;

    // a string libpq can read, then the defaults
    if (result == 0)
    {
        result = d_internal_postgres_defaults(&params);
    }

    if (result != 0)
    {
        return result;
    }

    d_internal_postgres_add(&params, "dbname", (given) ? _conninfo : NULL);

    PGconn* const native = PQconnectdbParams(params.keywords,
                                             params.values,
                                             1);

    free(params.startup);

    return d_internal_postgres_connected(native, _db, _message);
}

/*
d_postgres_connect_with
  The defaults, the caller's keywords, then the connection string's parts,
as libpq reads them, or the database a single word names: libpq takes the
last value given for a keyword, so the string overrides the keywords and
both the defaults. The string is read here rather than expanded at connect,
which would expand only the first dbname -- a keyword's dbname, given one --
and so a dbname among the keywords stays a name.
*/
int
d_postgres_connect_with(
    const char*        _conninfo,
    const char* const* _keywords,
    const char* const* _values,
    size_t             _count,
    struct pg_conn**   _db,
    char**             _message
)
{
    struct d_internal_postgres_params defaults =
    {
        { NULL }, { NULL }, 0u, "", "", NULL
    };
    PQconninfoOption*                 parsed   = NULL;
    const char**                      keywords = NULL;
    const char**                      values   = NULL;
    size_t                            count    = 0u;
    size_t                            i        = 0u;
    const bool                        given    = ( (_conninfo) &&
                                                   (_conninfo[0] != '\0') );

    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if ( (!_db) ||
         ( (_count != 0u) &&
           ( (!_keywords) ||
             (!_values) ) ) )
    {
        if (_db)
        {
            *_db = NULL;
        }

        return D_DB_INVALID_ARGUMENT;
    }

    *_db = NULL;

    int result = d_internal_postgres_check_keywords(_keywords,
                                                    _count,
                                                    _message);

    // the string, read as libpq reads it, with its reason when it cannot be
    if ( (result == 0) &&
         (given)       &&
         (d_internal_postgres_is_conninfo(_conninfo)) )
    {
        char* reason = NULL;

        parsed = PQconninfoParse(_conninfo, &reason);

        if ( (!parsed) &&
             (!reason) )
        {
            result = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
        }
        else if (!parsed)
        {
            if (_message)
            {
                *_message = d_internal_postgres_copy_message(reason);
            }

            PQfreemem(reason);
            result = D_DB_INVALID_ARGUMENT;
        }
    }

    if (result == 0)
    {
        result = d_internal_postgres_defaults(&defaults);
    }

    // room for the defaults, the keywords, the string's parts or its word,
    // and the terminator
    if (result == 0)
    {
        size_t                  room   = defaults.count + _count + 2u;
        const PQconninfoOption* option = parsed;

        while ( (option) &&
                (option->keyword) )
        {
            ++room;
            ++option;
        }

        keywords = (const char**)malloc(room * sizeof(*keywords));
        values   = (const char**)malloc(room * sizeof(*values));

        if ( (!keywords) ||
             (!values) )
        {
            result = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
        }
    }

    if (result == 0)
    {
        const PQconninfoOption* option = parsed;

        for (i = 0u; i < defaults.count; ++i)
        {
            keywords[count] = defaults.keywords[i];
            values[count]   = defaults.values[i];
            ++count;
        }

        // the caller's, a NULL or empty value left out
        for (i = 0u; i < _count; ++i)
        {
            if ( (_values[i]) &&
                 (_values[i][0] != '\0') )
            {
                keywords[count] = _keywords[i];
                values[count]   = _values[i];
                ++count;
            }
        }

        // the string's parts, or the database its single word names
        while ( (option) &&
                (option->keyword) )
        {
            if ( (option->val) &&
                 (option->val[0] != '\0') )
            {
                keywords[count] = option->keyword;
                values[count]   = option->val;
                ++count;
            }

            ++option;
        }

        if ( (given) &&
             (!parsed) )
        {
            keywords[count] = "dbname";
            values[count]   = _conninfo;
            ++count;
        }

        keywords[count] = NULL;
        values[count]   = NULL;

        PGconn* const native = PQconnectdbParams(keywords, values, 0);

        result = d_internal_postgres_connected(native, _db, _message);
    }

    free((void*)keywords);
    free((void*)values);
    free(defaults.startup);

    if (parsed)
    {
        PQconninfoFree(parsed);
    }

    return result;
}

/*
d_postgres_close
  PQfinish, which also frees the connection.
*/
int
d_postgres_close(
    struct pg_conn* _db
)
{
    // NULL closes as nothing
    if (_db)
    {
        PQfinish(_db);
    }

    return 0;
}

// 1.3    Liveness
//------------------------------------------------------------------------------
/*
d_postgres_ping
  An empty query is a round trip that runs nothing, which the server answers
even inside a failed transaction. A connection libpq already knows is lost
needs no trip.
*/
int
d_postgres_ping(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (PQstatus(_db) != CONNECTION_OK)
    {
        return D_POSTGRES_SQLSTATE_CONNECTION_FAILURE;
    }

    return d_internal_postgres_send(_db, "", NULL);
}

/*
d_postgres_backend_pid
  libpq's copy, which it reads as 0 for a lost connection.
*/
int
d_postgres_backend_pid(
    const struct pg_conn* _db
)
{
    return (_db) ? PQbackendPID(_db) : 0;
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_connect
  Without libpq there is no connection to make: D_DB_UNSUPPORTED, and the
connection and the message NULL, as a failure leaves them.
*/
int
d_postgres_connect(
    const struct d_postgres_options* _options,
    struct pg_conn**                 _db,
    char**                           _message
)
{
    (void)_options;

    // a failure leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    if (_message)
    {
        *_message = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_connect_string
  Without libpq there is no connection to make: D_DB_UNSUPPORTED, and the
connection and the message NULL, as a failure leaves them.
*/
int
d_postgres_connect_string(
    const char*      _conninfo,
    struct pg_conn** _db,
    char**           _message
)
{
    (void)_conninfo;

    // a failure leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    if (_message)
    {
        *_message = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_connect_with
  Without libpq there is no connection to make: D_DB_UNSUPPORTED, and the
connection and the message NULL, as a failure leaves them.
*/
int
d_postgres_connect_with(
    const char*        _conninfo,
    const char* const* _keywords,
    const char* const* _values,
    size_t             _count,
    struct pg_conn**   _db,
    char**             _message
)
{
    (void)_conninfo;
    (void)_keywords;
    (void)_values;
    (void)_count;

    // a failure leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    if (_message)
    {
        *_message = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_close
  Without libpq no connection exists: NULL closes as it always does, and
anything else is no connection.
*/
int
d_postgres_close(
    struct pg_conn* _db
)
{
    return (_db) ? D_DB_UNSUPPORTED : D_DB_OK;
}

/*
d_postgres_ping
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_ping(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_backend_pid
  Without libpq there is no backend: 0.
*/
int
d_postgres_backend_pid(
    const struct pg_conn* _db
)
{
    (void)_db;

    return 0;
}

#endif  // D_INTERNAL_POSTGRES
