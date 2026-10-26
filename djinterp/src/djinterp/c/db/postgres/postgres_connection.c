/*******************************************************************************
* djinterp [c]                                             postgres_connection.c
*
* Implementation of postgres_connection.h.
*   Every connect builds one plan -- libpq's keyword and value arrays, the
* layer's defaults first in them -- and hands it to PQconnectdbParams, or to
* PQconnectStartParams for a connect that does not block: libpq takes the
* last value given for a keyword, so whatever the caller names -- in the
* struct, in keywords, or in a connection string, which the layer reads into
* its parts -- overrides them. A default is given only where libpq's own
* environment variable for it is unset, so that PGCONNECT_TIMEOUT and
* PGCLIENTENCODING keep their meaning, and the layer's startup option joins
* PGOPTIONS rather than replacing it. Options of the caller's replace the
* layer's, so the layer's extra_float_digits goes ahead of them, in the same
* startup packet: nothing follows the handshake, which a connect that does
* not block could not wait for, and a reconnect, sending the same packet,
* keeps the setting too.
*   A connection that fails is finished at once, its reason copied out first,
* so that a caller never holds a connection that does not work.
*   libpq keeps connect_timeout only in its own blocking connect, so a
* connect, reconnect or cancel that does not block keeps it here, in the
* pending call's deadline, on the clock that libpq's blocking connect uses.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_connection.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.24
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_connection.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // snprintf
#include <stdlib.h>  // getenv, malloc, free, strtol
#include <string.h>  // memcpy, memset, strcat, strchr, strlen,
                     // strncmp, strcmp, strstr
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
//   constant: room for every keyword a connect from the options struct passes
// -- the layer's three defaults, six fields, and the terminating NULL.
#define D_INTERNAL_POSTGRES_KEYWORDS 10u

// D_INTERNAL_POSTGRES_DIGITS
//   constant: the startup option that makes a server before 12 write a double
// with every digit it needs to be read back exactly, as 12 and later do by
// default.
#define D_INTERNAL_POSTGRES_DIGITS "-c extra_float_digits=3"

// d_internal_postgres_params
//   struct: the layer's defaults as keyword and value arrays, with the text
// the layer made for them; for a connect from the options struct, its fields
// too.
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

// d_internal_postgres_plan
//   struct: everything a connect hands libpq -- the keyword and value arrays,
// NULL-terminated, libpq's own copies of what it parsed, and the text the
// layer made -- whichever form the connect took.
struct d_internal_postgres_plan
{
    struct d_internal_postgres_params defaults;  // the layer's defaults, and
                                                 // the options struct's
                                                 // fields
    PQconninfoOption*                 parsed;    // a connection string's
                                                 // parts, or NULL
    const char**                      keywords;  // the keywords libpq takes
    const char**                      values;    // their values
    bool                              owned;     // whether the arrays were
                                                 // allocated, rather than
                                                 // the defaults' own
    char*                             options;   // the caller's options with
                                                 // the layer's digits ahead,
                                                 // or NULL
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
  The layer's digits; "-c client_min_messages=<level>" where the build names
a level; and PGOPTIONS after them, whose own -c settings then win. 53200 when
memory ran out.
*/
static int
d_internal_postgres_startup(
    char** _startup
)
{
    static const char  digits[] = D_INTERNAL_POSTGRES_DIGITS;
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
d_internal_postgres_tell
  A reason, copied out where the caller wants one.
*/
static void
d_internal_postgres_tell(
    char**      _message,
    const char* _reason
)
{
    if (_message)
    {
        *_message = d_internal_postgres_copy_message(_reason);
    }

    return;
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

    char*                   reason = NULL;
    PQconninfoOption* const parsed = PQconninfoParse(probe, &reason);
    int                     result = 0;

    free(probe);

    // every keyword one libpq knows; or its reason, or memory
    if (parsed)
    {
        PQconninfoFree(parsed);
    }
    else if (!reason)
    {
        result = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }
    else
    {
        d_internal_postgres_tell(_message, reason);
        PQfreemem(reason);
        result = D_DB_INVALID_ARGUMENT;
    }

    return result;
}

/*
d_internal_postgres_parse
  A connection string, read as libpq reads it; a string libpq cannot read is
the caller's argument, its reason copied out where the caller wants it.
libpq answers no reason at all only when memory ran out.
*/
static int
d_internal_postgres_parse(
    const char*        _conninfo,
    PQconninfoOption** _parsed,
    char**             _message
)
{
    char* reason = NULL;

    *_parsed = PQconninfoParse(_conninfo, &reason);

    // readable
    if (*_parsed)
    {
        return 0;
    }

    if (!reason)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    d_internal_postgres_tell(_message, reason);
    PQfreemem(reason);

    return D_DB_INVALID_ARGUMENT;
}

/*
d_internal_postgres_plan_free
  Whatever a plan holds; a plan never filled holds nothing.
*/
static void
d_internal_postgres_plan_free(
    struct d_internal_postgres_plan* _plan
)
{
    // the arrays, where they were allocated rather than the defaults' own
    if (_plan->owned)
    {
        free((void*)_plan->keywords);
        free((void*)_plan->values);
    }

    free(_plan->options);
    free(_plan->defaults.startup);

    if (_plan->parsed)
    {
        PQconninfoFree(_plan->parsed);
    }

    return;
}

/*
d_internal_postgres_plan_options
  The defaults, then each field the options give -- NULL options give none
-- in the defaults' own arrays, which have room for them all. No field is
options, so the layer's own startup option stands.
*/
static int
d_internal_postgres_plan_options(
    struct d_internal_postgres_plan* _plan,
    const struct d_postgres_options* _options
)
{
    const int result = d_internal_postgres_defaults(&_plan->defaults);

    // the defaults, or the memory they needed
    if (result != 0)
    {
        return result;
    }

    if (_options)
    {
        d_internal_postgres_add_options(&_plan->defaults, _options);
    }

    _plan->defaults.keywords[_plan->defaults.count] = NULL;
    _plan->defaults.values[_plan->defaults.count]   = NULL;
    _plan->keywords = _plan->defaults.keywords;
    _plan->values   = _plan->defaults.values;

    return 0;
}

/*
d_internal_postgres_digits_ahead
  Options of the caller's -- among its keywords, or its connection string's
parts -- replace the layer's, libpq taking the last value given; unless they
name extra_float_digits themselves, the layer's digits go ahead of them, so
that the startup packet sets it as the layer's own options would, and the
caller's own settings, coming after, still win. Nothing after the handshake
is then needed, by a connect or by a reconnect, which sends the same
packet.
53200 when memory ran out.
*/
static int
d_internal_postgres_digits_ahead(
    struct d_internal_postgres_plan* _plan,
    size_t                           _count
)
{
    static const char digits[] = D_INTERNAL_POSTGRES_DIGITS " ";
    size_t            last     = _count;

    // the options libpq will take: the last given
    for (size_t i = 0u; i < _count; ++i)
    {
        if (strcmp(_plan->keywords[i], "options") == 0)
        {
            last = i;
        }
    }

    // the layer's own, or options that set the digits themselves
    if ( (last == _count)                                      ||
         (_plan->values[last] == _plan->defaults.startup)      ||
         (strstr(_plan->values[last], "extra_float_digits")) )
    {
        return 0;
    }

    const size_t length = strlen(_plan->values[last]);

    _plan->options = (char*)malloc(sizeof(digits) + length);

    if (!_plan->options)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    memcpy(_plan->options, digits, sizeof(digits) - 1u);
    memcpy(_plan->options + sizeof(digits) - 1u,
           _plan->values[last],
           length + 1u);
    _plan->values[last] = _plan->options;

    return 0;
}

/*
d_internal_postgres_plan_fill
  The arrays, in the order libpq is to read them: the defaults, the caller's
keywords -- a NULL or empty value left out -- then the connection string's
parts, or the database its single word names, so that the string overrides
the keywords and both the defaults.
*/
static int
d_internal_postgres_plan_fill(
    struct d_internal_postgres_plan* _plan,
    const char*                      _conninfo,
    const char* const*               _keywords,
    const char* const*               _values,
    size_t                           _count
)
{
    size_t                  room   = _plan->defaults.count + _count + 2u;
    size_t                  count  = 0u;
    const PQconninfoOption* option = _plan->parsed;

    // room for the string's parts too
    while ( (option) &&
            (option->keyword) )
    {
        ++room;
        ++option;
    }

    _plan->keywords = (const char**)malloc(room * sizeof(*_plan->keywords));
    _plan->values   = (const char**)malloc(room * sizeof(*_plan->values));
    _plan->owned    = true;

    if ( (!_plan->keywords) ||
         (!_plan->values) )
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    for (size_t i = 0u; i < _plan->defaults.count; ++i)
    {
        _plan->keywords[count] = _plan->defaults.keywords[i];
        _plan->values[count]   = _plan->defaults.values[i];
        ++count;
    }

    // the caller's, a NULL or empty value left out
    for (size_t i = 0u; i < _count; ++i)
    {
        if ( (_values[i]) &&
             (_values[i][0] != '\0') )
        {
            _plan->keywords[count] = _keywords[i];
            _plan->values[count]   = _values[i];
            ++count;
        }
    }

    // the string's parts, or the database its single word names
    for (option = _plan->parsed;
         (option) && (option->keyword);
         ++option)
    {
        if ( (option->val) &&
             (option->val[0] != '\0') )
        {
            _plan->keywords[count] = option->keyword;
            _plan->values[count]   = option->val;
            ++count;
        }
    }

    if ( (_conninfo)               &&
         (_conninfo[0] != '\0')    &&
         (!_plan->parsed) )
    {
        _plan->keywords[count] = "dbname";
        _plan->values[count]   = _conninfo;
        ++count;
    }

    _plan->keywords[count] = NULL;
    _plan->values[count]   = NULL;

    return d_internal_postgres_digits_ahead(_plan, count);
}

/*
d_internal_postgres_plan_with
  The defaults, the caller's keywords, then the connection string's parts,
as libpq reads them, or the database a single word names. The string is read
here rather than expanded at connect, which would expand only the first
dbname -- a keyword's dbname, given one -- and so a dbname among the keywords
stays a name; reading it here also shows the options it gives. A keyword or
a string libpq cannot read is checked first, so that its reason is the
caller's, not a failed connection's.
*/
static int
d_internal_postgres_plan_with(
    struct d_internal_postgres_plan* _plan,
    const char*                      _conninfo,
    const char* const*               _keywords,
    const char* const*               _values,
    size_t                           _count,
    char**                           _message
)
{
    const bool given  = ( (_conninfo) &&
                          (_conninfo[0] != '\0') );
    int        result = d_internal_postgres_check_keywords(_keywords,
                                                           _count,
                                                           _message);

    // the string, read as libpq reads it, with its reason when it cannot be
    if ( (result == 0) &&
         (given)       &&
         (d_internal_postgres_is_conninfo(_conninfo)) )
    {
        result = d_internal_postgres_parse(_conninfo,
                                           &_plan->parsed,
                                           _message);
    }

    if (result == 0)
    {
        result = d_internal_postgres_defaults(&_plan->defaults);
    }

    return (result == 0)
               ? d_internal_postgres_plan_fill(_plan,
                                               _conninfo,
                                               _keywords,
                                               _values,
                                               _count)
               : result;
}

/*
d_internal_postgres_failed
  A connection that failed is not handed back: its reason copied out first,
where the caller wants it, then finished. 08001, libpq giving none of its
own numbers for the reasons a connection cannot be made.
*/
static int
d_internal_postgres_failed(
    PGconn*          _native,
    struct pg_conn** _db,
    char**           _message
)
{
    d_internal_postgres_tell(_message, PQerrorMessage(_native));
    PQfinish(_native);
    *_db = NULL;

    return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
}

/*
d_internal_postgres_connected
  What PQconnectdbParams made, kept when it works and finished when it does
not. Nothing follows the handshake: the plan put the layer's settings in the
startup packet.
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
        return d_internal_postgres_failed(_native, _db, _message);
    }

    *_db = _native;

    return 0;
}

/*
d_internal_postgres_deadline
  The deadline for a connection being made, from the connect_timeout libpq
settled for it -- the caller's, its environment's or the layer's default --
read as libpq reads it: an integer, surrounded by spaces or not; 0 or less
for none; and, counted in whole seconds before libpq 17, at least two, since
the seconds round. Text that is no integer is refused, as libpq 12 and later
refuse it, with their words. 53200 when memory ran out.
*/
static int
d_internal_postgres_deadline(
    PGconn*              _native,
    struct d_db_pending* _pending,
    char**               _message
)
{
    PQconninfoOption* const options = PQconninfo(_native);
    int                     result  = 0;

    _pending->deadline = 0.0;

    // libpq could not describe the connection: memory
    if (!options)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    for (const PQconninfoOption* option = options;
         (option) && (option->keyword);
         ++option)
    {
        // the timeout libpq settled on, where there is one
        if ( (strcmp(option->keyword, "connect_timeout") != 0) ||
             (!option->val) ||
             (option->val[0] == '\0') )
        {
            continue;
        }

        char*      end     = NULL;
        const long seconds = strtol(option->val, &end, 10);

        while ( (*end == ' ')  ||
                (*end == '\t') ||
                (*end == '\n') )
        {
            ++end;
        }

        if ( (end == option->val) ||
             (*end != '\0') )
        {
            const size_t length = strlen(option->val) + 64u;
            char* const  reason = (char*)malloc(length);

            if (reason)
            {
                (void)snprintf(reason,
                               length,
                               "invalid integer value \"%s\" for connection "
                               "option \"connect_timeout\"",
                               option->val);
            }

            if (_message)
            {
                *_message = reason;
            }
            else
            {
                free(reason);
            }

            result = D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
        }
        else if (seconds > 0)
        {
            const bool whole = (PQlibVersion() < 170000);

            _pending->deadline = d_internal_postgres_now() +
                                 (double)( ( (whole) && (seconds < 2) )
                                               ? 2
                                               : seconds );
        }

        break;
    }

    PQconninfoFree(options);

    return result;
}

/*
d_internal_postgres_polled
  What libpq's polling asked for, as d_db_wait.
*/
static unsigned
d_internal_postgres_polled(
    PostgresPollingStatusType _polling
)
{
    return (_polling == PGRES_POLLING_READING) ? (unsigned)D_DB_WAIT_READ
                                               : (unsigned)D_DB_WAIT_WRITE;
}

/*
d_internal_postgres_held_back
  Whether libpq may be asked to go on: only once the socket is ready, since
polling a connection still being made before then reads it as made. A
continuation without a ready socket is the deadline's: past it, the call has
timed out; before it, it waits again for what it waited for.
*/
static bool
d_internal_postgres_held_back(
    unsigned             _events,
    struct d_db_pending* _pending,
    bool*                _expired
)
{
    const unsigned ready = (unsigned)D_DB_WAIT_READ |
                           (unsigned)D_DB_WAIT_WRITE;

    *_expired = false;

    // ready: libpq goes on
    if ((_events & ready) != 0u)
    {
        return false;
    }

    *_expired = ( (_pending->deadline > 0.0) &&
                  (d_internal_postgres_now() >= _pending->deadline) );

    return true;
}

/*
d_internal_postgres_begin
  A connection started from a plan: libpq copies what the arrays point to,
so the plan can be freed at once. Started, it waits to write, as libpq's
start leaves it.
*/
static int
d_internal_postgres_begin(
    struct d_internal_postgres_plan* _plan,
    struct pg_conn**                 _db,
    struct d_db_pending*             _pending,
    char**                           _message
)
{
    PGconn* const native = PQconnectStartParams(_plan->keywords,
                                                _plan->values,
                                                0);

    // libpq could not allocate even the connection
    if (!native)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    // refused at once: an address that does not resolve, say
    if (PQstatus(native) == CONNECTION_BAD)
    {
        return d_internal_postgres_failed(native, _db, _message);
    }

    const int deadline = d_internal_postgres_deadline(native,
                                                      _pending,
                                                      _message);

    if (deadline != 0)
    {
        PQfinish(native);

        return deadline;
    }

    *_db = native;

    return d_internal_postgres_pend(PQsocket(native),
                                    (unsigned)D_DB_WAIT_WRITE,
                                    _pending);
}

/*
d_internal_postgres_starting
  What every start checks first: somewhere to put the connection and what it
waits for, both cleared, and the message.
*/
static bool
d_internal_postgres_starting(
    struct pg_conn**     _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    if (_db)
    {
        *_db = NULL;
    }

    if (_pending)
    {
        d_internal_postgres_settled(_pending);
    }

    return ( (_db) &&
             (_pending) );
}

/*
d_internal_postgres_cancel_failed
  A reason for a cancel request that could not be delivered, copied out
where the caller wants it. 08001.
*/
static int
d_internal_postgres_cancel_failed(
    const char* _reason,
    char**      _message
)
{
    d_internal_postgres_tell(_message, _reason);

    return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
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
    struct d_internal_postgres_plan plan;

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
    memset(&plan, 0, sizeof(plan));

    int result = d_internal_postgres_plan_options(&plan, _options);

    // the plan, then the connection
    if (result == 0)
    {
        result = d_internal_postgres_connected(
                     PQconnectdbParams(plan.keywords, plan.values, 0),
                     _db,
                     _message);
    }

    d_internal_postgres_plan_free(&plan);

    return result;
}

/*
d_postgres_connect_string
  d_postgres_connect_with with the string alone: its parts, read here,
override the defaults, and a single word is a database's name.
*/
int
d_postgres_connect_string(
    const char*      _conninfo,
    struct pg_conn** _db,
    char**           _message
)
{
    return d_postgres_connect_with(_conninfo, NULL, NULL, 0u, _db, _message);
}

/*
d_postgres_connect_with
  The plan (d_internal_postgres_plan_with), then the connection.
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
    struct d_internal_postgres_plan plan;

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
    memset(&plan, 0, sizeof(plan));

    int result = d_internal_postgres_plan_with(&plan,
                                               _conninfo,
                                               _keywords,
                                               _values,
                                               _count,
                                               _message);

    // the plan, then the connection
    if (result == 0)
    {
        result = d_internal_postgres_connected(
                     PQconnectdbParams(plan.keywords, plan.values, 0),
                     _db,
                     _message);
    }

    d_internal_postgres_plan_free(&plan);

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

// 1.4    Connecting without blocking
//------------------------------------------------------------------------------
/*
d_postgres_connect_start
  The plan d_postgres_connect makes, then libpq's start.
*/
int
d_postgres_connect_start(
    const struct d_postgres_options* _options,
    struct pg_conn**                 _db,
    struct d_db_pending*             _pending,
    char**                           _message
)
{
    struct d_internal_postgres_plan plan;

    // parameter validation
    if (!d_internal_postgres_starting(_db, _pending, _message))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    memset(&plan, 0, sizeof(plan));

    int result = d_internal_postgres_plan_options(&plan, _options);

    // the plan, then the start
    if (result == 0)
    {
        result = d_internal_postgres_begin(&plan, _db, _pending, _message);
    }

    d_internal_postgres_plan_free(&plan);

    return result;
}

/*
d_postgres_connect_string_start
  d_postgres_connect_with_start with the string alone.
*/
int
d_postgres_connect_string_start(
    const char*          _conninfo,
    struct pg_conn**     _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    return d_postgres_connect_with_start(_conninfo,
                                         NULL,
                                         NULL,
                                         0u,
                                         _db,
                                         _pending,
                                         _message);
}

/*
d_postgres_connect_with_start
  The plan d_postgres_connect_with makes, then libpq's start.
*/
int
d_postgres_connect_with_start(
    const char*          _conninfo,
    const char* const*   _keywords,
    const char* const*   _values,
    size_t               _count,
    struct pg_conn**     _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    struct d_internal_postgres_plan plan;

    // parameter validation
    if ( (!d_internal_postgres_starting(_db, _pending, _message)) ||
         ( (_count != 0u) &&
           ( (!_keywords) ||
             (!_values) ) ) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    memset(&plan, 0, sizeof(plan));

    int result = d_internal_postgres_plan_with(&plan,
                                               _conninfo,
                                               _keywords,
                                               _values,
                                               _count,
                                               _message);

    // the plan, then the start
    if (result == 0)
    {
        result = d_internal_postgres_begin(&plan, _db, _pending, _message);
    }

    d_internal_postgres_plan_free(&plan);

    return result;
}

/*
d_postgres_connect_continue
  PQconnectPoll, once the socket is ready; a connection libpq already made
is done, and one it gave up on is finished.
*/
int
d_postgres_connect_continue(
    struct pg_conn**     _db,
    unsigned             _events,
    struct d_db_pending* _pending,
    char**               _message
)
{
    bool expired = false;

    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if ( (!_db)  ||
         (!*_db) ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    PGconn* const native = *_db;

    // made already, or given up on
    if (PQstatus(native) == CONNECTION_OK)
    {
        d_internal_postgres_settled(_pending);

        return 0;
    }

    if (PQstatus(native) == CONNECTION_BAD)
    {
        d_internal_postgres_settled(_pending);

        return d_internal_postgres_failed(native, _db, _message);
    }

    // the socket not ready: the deadline passed, or a wait for the rest
    if (d_internal_postgres_held_back(_events, _pending, &expired))
    {
        if (expired)
        {
            d_internal_postgres_settled(_pending);
            d_internal_postgres_tell(_message, "timeout expired");
            PQfinish(native);
            *_db = NULL;

            return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
        }

        return d_internal_postgres_pend(
                   PQsocket(native),
                   _pending->events & ~(unsigned)D_DB_WAIT_TIMEOUT,
                   _pending);
    }

    const PostgresPollingStatusType polling = PQconnectPoll(native);

    switch (polling)
    {
        case PGRES_POLLING_OK:
            d_internal_postgres_settled(_pending);

            return 0;

        case PGRES_POLLING_FAILED:
            d_internal_postgres_settled(_pending);

            return d_internal_postgres_failed(native, _db, _message);

        default:
            break;
    }

    return d_internal_postgres_pend(PQsocket(native),
                                    d_internal_postgres_polled(polling),
                                    _pending);
}

// 1.5    Resetting
//------------------------------------------------------------------------------
/*
d_postgres_reconnect
  PQreset, which closes the connection and makes it again with what it was
made with, its startup packet the layer's settings among them.
*/
int
d_postgres_reconnect(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    PQreset(_db);

    return (PQstatus(_db) == CONNECTION_OK)
               ? 0
               : D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
}

/*
d_postgres_reconnect_start
  PQresetStart closes the connection and starts it again, waiting to write,
as libpq's start leaves it; the deadline as a connect's.
*/
int
d_postgres_reconnect_start(
    struct pg_conn*      _db,
    struct d_db_pending* _pending
)
{
    // parameter validation
    if ( (!_db) ||
         (!_pending) )
    {
        if (_pending)
        {
            d_internal_postgres_settled(_pending);
        }

        return D_DB_INVALID_ARGUMENT;
    }

    d_internal_postgres_settled(_pending);

    // refused at once
    if ( (PQresetStart(_db) != 1) ||
         (PQstatus(_db) == CONNECTION_BAD) )
    {
        return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
    }

    const int deadline = d_internal_postgres_deadline(_db, _pending, NULL);

    // a timeout that is no integer cannot have been made with; memory
    if (deadline != 0)
    {
        return deadline;
    }

    return d_internal_postgres_pend(PQsocket(_db),
                                    (unsigned)D_DB_WAIT_WRITE,
                                    _pending);
}

/*
d_postgres_reconnect_continue
  PQresetPoll, once the socket is ready, as d_postgres_connect_continue polls;
a connection that cannot be made again is left as libpq leaves it, lost.
*/
int
d_postgres_reconnect_continue(
    struct pg_conn*      _db,
    unsigned             _events,
    struct d_db_pending* _pending
)
{
    bool expired = false;

    // parameter validation
    if ( (!_db) ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // made again already, or given up on
    if (PQstatus(_db) == CONNECTION_OK)
    {
        d_internal_postgres_settled(_pending);

        return 0;
    }

    if (PQstatus(_db) == CONNECTION_BAD)
    {
        d_internal_postgres_settled(_pending);

        return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
    }

    // the socket not ready: the deadline passed, or a wait for the rest
    if (d_internal_postgres_held_back(_events, _pending, &expired))
    {
        if (expired)
        {
            d_internal_postgres_settled(_pending);

            return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;
        }

        return d_internal_postgres_pend(
                   PQsocket(_db),
                   _pending->events & ~(unsigned)D_DB_WAIT_TIMEOUT,
                   _pending);
    }

    const PostgresPollingStatusType polling = PQresetPoll(_db);

    switch (polling)
    {
        case PGRES_POLLING_OK:
            d_internal_postgres_settled(_pending);

            return 0;

        case PGRES_POLLING_FAILED:
            d_internal_postgres_settled(_pending);

            return D_POSTGRES_SQLSTATE_CANNOT_CONNECT;

        default:
            break;
    }

    return d_internal_postgres_pend(PQsocket(_db),
                                    d_internal_postgres_polled(polling),
                                    _pending);
}

// 1.6    Cancelling
//------------------------------------------------------------------------------
/*
d_postgres_cancel
  From libpq 17, a cancel connection made from the connection, which carries
its settings -- encryption, connect_timeout -- and PQcancelBlocking; before
it, PQcancel, which writes its reason into a buffer of the caller's. A lost
connection has no backend to reach.
*/
int
d_postgres_cancel(
    struct pg_conn* _db,
    char**          _message
)
{
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

    if (PQstatus(_db) != CONNECTION_OK)
    {
        return D_POSTGRES_SQLSTATE_CONNECTION_FAILURE;
    }

#if defined(LIBPQ_HAS_ASYNC_CANCEL)
    // libpq 17's request, where the library loaded is 17 too
    if (d_postgres_client_supports(D_POSTGRES_CLIENT_ASYNC_CANCEL))
    {
        PGcancelConn* const cancel = PQcancelCreate(_db);

        if (!cancel)
        {
            return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
        }

        const int result = ( (PQcancelStatus(cancel) != CONNECTION_BAD) &&
                             (PQcancelBlocking(cancel) == 1) )
                               ? 0
                               : d_internal_postgres_cancel_failed(
                                     PQcancelErrorMessage(cancel),
                                     _message);

        PQcancelFinish(cancel);

        return result;
    }
#endif  // LIBPQ_HAS_ASYNC_CANCEL

    char            reason[256] = "";
    PGcancel* const cancel      = PQgetCancel(_db);

    // the key libpq keeps, copied: only memory can fail
    if (!cancel)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    const int sent = PQcancel(cancel, reason, (int)sizeof(reason));

    PQfreeCancel(cancel);

    return (sent == 1) ? 0
                       : d_internal_postgres_cancel_failed(reason, _message);
}

/*
d_postgres_cancel_start
  libpq 17's cancel connection, started, which waits to write, as libpq's
start leaves it; the deadline from the connection's connect_timeout, which
the cancel connection carries.
*/
int
d_postgres_cancel_start(
    struct pg_conn*         _db,
    struct pg_cancel_conn** _cancel,
    struct d_db_pending*    _pending,
    char**                  _message
)
{
    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    if (_cancel)
    {
        *_cancel = NULL;
    }

    if (_pending)
    {
        d_internal_postgres_settled(_pending);
    }

    // parameter validation
    if ( (!_db)     ||
         (!_cancel) ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // libpq 17's, in the header and the library alike
    if (!d_postgres_client_supports(D_POSTGRES_CLIENT_ASYNC_CANCEL))
    {
        return D_DB_UNSUPPORTED;
    }

    if (PQstatus(_db) != CONNECTION_OK)
    {
        return D_POSTGRES_SQLSTATE_CONNECTION_FAILURE;
    }

#if defined(LIBPQ_HAS_ASYNC_CANCEL)
    PGcancelConn* const cancel = PQcancelCreate(_db);

    if (!cancel)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    // refused at once
    if ( (PQcancelStatus(cancel) == CONNECTION_BAD) ||
         (PQcancelStart(cancel) != 1) )
    {
        const int failed = d_internal_postgres_cancel_failed(
                               PQcancelErrorMessage(cancel),
                               _message);

        PQcancelFinish(cancel);

        return failed;
    }

    const int deadline = d_internal_postgres_deadline(_db,
                                                      _pending,
                                                      _message);

    if (deadline != 0)
    {
        PQcancelFinish(cancel);

        return deadline;
    }

    *_cancel = cancel;

    return d_internal_postgres_pend(PQcancelSocket(cancel),
                                    (unsigned)D_DB_WAIT_WRITE,
                                    _pending);
#else
    return D_DB_UNSUPPORTED;
#endif  // LIBPQ_HAS_ASYNC_CANCEL
}

/*
d_postgres_cancel_continue
  PQcancelPoll, once the socket is ready, as d_postgres_connect_continue
polls; the cancel connection is finished once the request is delivered or
fails.
*/
int
d_postgres_cancel_continue(
    struct pg_cancel_conn** _cancel,
    unsigned                _events,
    struct d_db_pending*    _pending,
    char**                  _message
)
{
    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if ( (!_cancel)  ||
         (!*_cancel) ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

#if defined(LIBPQ_HAS_ASYNC_CANCEL)
    PGcancelConn* const cancel  = *_cancel;
    bool                expired = false;

    // the socket not ready: the deadline passed, or a wait for the rest
    if (d_internal_postgres_held_back(_events, _pending, &expired))
    {
        if (!expired)
        {
            return d_internal_postgres_pend(
                       PQcancelSocket(cancel),
                       _pending->events & ~(unsigned)D_DB_WAIT_TIMEOUT,
                       _pending);
        }

        d_internal_postgres_settled(_pending);
        PQcancelFinish(cancel);
        *_cancel = NULL;

        return d_internal_postgres_cancel_failed("timeout expired",
                                                 _message);
    }

    const PostgresPollingStatusType polling = PQcancelPoll(cancel);

    // under way still
    if ( (polling != PGRES_POLLING_OK) &&
         (polling != PGRES_POLLING_FAILED) )
    {
        return d_internal_postgres_pend(PQcancelSocket(cancel),
                                        d_internal_postgres_polled(polling),
                                        _pending);
    }

    const int result = (polling == PGRES_POLLING_OK)
                           ? 0
                           : d_internal_postgres_cancel_failed(
                                 PQcancelErrorMessage(cancel),
                                 _message);

    d_internal_postgres_settled(_pending);
    PQcancelFinish(cancel);
    *_cancel = NULL;

    return result;
#else
    (void)_events;

    return D_DB_UNSUPPORTED;
#endif  // LIBPQ_HAS_ASYNC_CANCEL
}

/*
d_postgres_cancel_finish
  PQcancelFinish, which closes the request's connection and frees it.
*/
int
d_postgres_cancel_finish(
    struct pg_cancel_conn* _cancel
)
{
#if defined(LIBPQ_HAS_ASYNC_CANCEL)
    // NULL finishes as nothing
    if (_cancel)
    {
        PQcancelFinish(_cancel);
    }
#else
    (void)_cancel;
#endif  // LIBPQ_HAS_ASYNC_CANCEL

    return 0;
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

/*
d_internal_postgres_unsupported
  What every start and continuation leaves behind without libpq: no
connection, nothing pending, no message. D_DB_UNSUPPORTED.
*/
static int
d_internal_postgres_unsupported(
    struct pg_conn**     _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    if (_db)
    {
        *_db = NULL;
    }

    if (_pending)
    {
        _pending->socket     = D_DB_SOCKET_NONE;
        _pending->events     = 0u;
        _pending->timeout_ms = 0u;
        _pending->deadline   = 0.0;
        _pending->state      = 0u;
    }

    if (_message)
    {
        *_message = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_connect_start
  Without libpq there is no connection to start: D_DB_UNSUPPORTED.
*/
int
d_postgres_connect_start(
    const struct d_postgres_options* _options,
    struct pg_conn**                 _db,
    struct d_db_pending*             _pending,
    char**                           _message
)
{
    (void)_options;

    return d_internal_postgres_unsupported(_db, _pending, _message);
}

/*
d_postgres_connect_string_start
  Without libpq there is no connection to start: D_DB_UNSUPPORTED.
*/
int
d_postgres_connect_string_start(
    const char*          _conninfo,
    struct pg_conn**     _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    (void)_conninfo;

    return d_internal_postgres_unsupported(_db, _pending, _message);
}

/*
d_postgres_connect_with_start
  Without libpq there is no connection to start: D_DB_UNSUPPORTED.
*/
int
d_postgres_connect_with_start(
    const char*          _conninfo,
    const char* const*   _keywords,
    const char* const*   _values,
    size_t               _count,
    struct pg_conn**     _db,
    struct d_db_pending* _pending,
    char**               _message
)
{
    (void)_conninfo;
    (void)_keywords;
    (void)_values;
    (void)_count;

    return d_internal_postgres_unsupported(_db, _pending, _message);
}

/*
d_postgres_connect_continue
  Without libpq no connection was started: D_DB_UNSUPPORTED, the handle
left as it is, since nothing here made it.
*/
int
d_postgres_connect_continue(
    struct pg_conn**     _db,
    unsigned             _events,
    struct d_db_pending* _pending,
    char**               _message
)
{
    (void)_db;
    (void)_events;

    return d_internal_postgres_unsupported(NULL, _pending, _message);
}

/*
d_postgres_reconnect
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_reconnect(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_reconnect_start
  Without libpq, D_DB_UNSUPPORTED, nothing pending.
*/
int
d_postgres_reconnect_start(
    struct pg_conn*      _db,
    struct d_db_pending* _pending
)
{
    (void)_db;

    return d_internal_postgres_unsupported(NULL, _pending, NULL);
}

/*
d_postgres_reconnect_continue
  Without libpq, D_DB_UNSUPPORTED, nothing pending.
*/
int
d_postgres_reconnect_continue(
    struct pg_conn*      _db,
    unsigned             _events,
    struct d_db_pending* _pending
)
{
    (void)_db;
    (void)_events;

    return d_internal_postgres_unsupported(NULL, _pending, NULL);
}

/*
d_postgres_cancel
  Without libpq there is nothing to cancel: D_DB_UNSUPPORTED.
*/
int
d_postgres_cancel(
    struct pg_conn* _db,
    char**          _message
)
{
    (void)_db;

    return d_internal_postgres_unsupported(NULL, NULL, _message);
}

/*
d_postgres_cancel_start
  Without libpq there is nothing to cancel: D_DB_UNSUPPORTED, no request.
*/
int
d_postgres_cancel_start(
    struct pg_conn*         _db,
    struct pg_cancel_conn** _cancel,
    struct d_db_pending*    _pending,
    char**                  _message
)
{
    (void)_db;

    if (_cancel)
    {
        *_cancel = NULL;
    }

    return d_internal_postgres_unsupported(NULL, _pending, _message);
}

/*
d_postgres_cancel_continue
  Without libpq no request was started: D_DB_UNSUPPORTED.
*/
int
d_postgres_cancel_continue(
    struct pg_cancel_conn** _cancel,
    unsigned                _events,
    struct d_db_pending*    _pending,
    char**                  _message
)
{
    (void)_cancel;
    (void)_events;

    return d_internal_postgres_unsupported(NULL, _pending, _message);
}

/*
d_postgres_cancel_finish
  Without libpq no request exists: NULL finishes as it always does, and
anything else is no request.
*/
int
d_postgres_cancel_finish(
    struct pg_cancel_conn* _cancel
)
{
    return (_cancel) ? D_DB_UNSUPPORTED : D_DB_OK;
}

#endif  // D_INTERNAL_POSTGRES
