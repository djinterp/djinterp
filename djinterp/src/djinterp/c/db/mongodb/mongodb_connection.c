/*******************************************************************************
* djinterp [c]                                              mongodb_connection.c
*
* Implementation of mongodb_connection.h.
*   A connection is made in four steps. The connection string is built: the
* one given, or one written from the host and the port, with every
* URI_OPTION setting appended to it, percent-encoded, so that the driver's
* own parser reads each option as it reads a string's. The driver parses it;
* the options' fields, then the other settings in their order, are applied
* to what it parsed through the driver's typed calls, and the layer's
* defaults where nothing named them. The client is made from it and asked
* for the second error API, and for a declared API version where one was
* set. Then the layer pings, which signs in, and asks hello what the
* server is part of and buildInfo its version.
*   buildInfo is refused by a strict declared API, which it is outside of,
* and by a server that requires a sign-in to a connection without one; the
* layer then takes the oldest release that speaks the server's wire version,
* and leaves the version string empty.
*   The handle is allocated first, so that every failure, the driver's or
* the layer's, is recorded on it in full, and copied out for the caller
* before it is closed.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_connection.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_connection.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // snprintf
#include <stdlib.h>  // calloc, free, malloc
#include <string.h>  // memchr, memcpy, memset, strchr, strcmp, strcspn,
                     // strlen, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_connection.h"  // the defaults
#include "../db_internal.h"       // buffers
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t, INT32_MAX,
                                                         // int64_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// API_STRICT_ERROR
//   constant: the server's code, APIStrictError, for a command outside the
// declared API, which buildInfo is.
static const int API_STRICT_ERROR = 323;

// dj_internal_mongodb_wire_release
//   struct: the oldest release that speaks a wire version.
struct dj_internal_mongodb_wire_release
{
    int           wire;     // the maxWireVersion
    unsigned long version;  // its release, MAJOR*10000 + MINOR*100
};

// WIRE_RELEASES
//   table: the releases from the floor on, in order; the wire versions
// between 4.4's and 5.0's were development releases'.
static const struct dj_internal_mongodb_wire_release WIRE_RELEASES[] =
{
    {  8, 40200ul }, {  9, 40400ul }, { 13, 50000ul }, { 14, 50100ul },
    { 15, 50200ul }, { 16, 50300ul }, { 17, 60000ul }, { 18, 60100ul },
    { 19, 60200ul }, { 20, 60300ul }, { 21, 70000ul }, { 22, 70100ul },
    { 23, 70200ul }, { 24, 70300ul }, { 25, 80000ul }, { 26, 80100ul },
    { 27, 80200ul }, { 28, 80300ul }
};


//==============================================================================
// 1.  THE CONNECTION STRING
//==============================================================================

/**
 * @brief Appends text to a buffer, percent-encoded but for URI's unreserved
 *        characters and those named.
 *
 * @param[in,out] _out   the buffer.
 * @param[in]     _text  the text.
 * @param[in]     _keep  the characters to write as they are, besides.
 * @return `true`, or `false` when memory ran out.
 */
static
bool d_internal_mongodb_encode(
    struct dj_internal_db_buffer* _out,
    const char*                   _text,
    const char*                   _keep
)
{
    static const char HEX[] = "0123456789ABCDEF";

    const char* cursor = _text;
    bool        ok     = true;

    // each byte, as it is or as %XX
    for (cursor = _text; (ok) && (*cursor); ++cursor)
    {
        const unsigned char c = (unsigned char)*cursor;
        const bool          plain = ( ( (c >= 'a') && (c <= 'z') ) ||
                                      ( (c >= 'A') && (c <= 'Z') ) ||
                                      ( (c >= '0') && (c <= '9') ) ||
                                      (strchr("-._~", (int)c))     ||
                                      (strchr(_keep, (int)c)) );
        const char          escape[3] = { '%', HEX[c >> 4], HEX[c & 0x0Fu] };

        ok = (plain) ? d_internal_db_buffer_append(_out, cursor, 1u)
                     : d_internal_db_buffer_append(_out, escape, 3u);
    }

    return ok;
}

/**
 * @brief Writes a connection string from a host and a port.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in]     _options  the host and the port.
 * @param[in,out] _out      the buffer.
 * @return 0; D_DB_INVALID_ARGUMENT for a port past 65535, or one beside a
 *         Unix socket's path; D_MONGODB_LAYER_NO_MEMORY.
 *
 * A socket's path, which begins with a slash, is percent-encoded, as the
 * driver reads one; an IPv6 address goes in brackets.
 */
static
int d_internal_mongodb_host_uri(
    struct dj_mongodb*               _db,
    const struct dj_mongodb_options* _options,
    struct dj_internal_db_buffer*    _out
)
{
    const char* const host   = (_options->host) ? _options->host
                                                : "localhost";
    const char* const colon  = strchr(host, ':');
    const bool        socket = (host[0] == '/');
    const bool        ipv6   = ( (!socket)        &&
                                 (host[0] != '[') &&
                                 (colon)          &&
                                 (strchr(colon + 1, ':')) );
    char              port[8] = "";

    // a port is 16 bits, and a socket has none
    if ( (_options->port > 65535u) ||
         ( (socket) &&
           (_options->port != 0u) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         (socket) ? "a Unix socket's path "
                                                    "takes no port"
                                                  : "a port is 65535 at "
                                                    "most");
    }

    // the port, when one is given
    if (_options->port != 0u)
    {
        (void)snprintf(port, sizeof(port), ":%u", _options->port);
    }

    const char* const keep = (socket) ? "" : "[]:!$&'()*+,;=";
    const bool        ok   =
        ( (d_internal_db_buffer_append(_out, "mongodb://", 10u))     &&
          ( (!ipv6) ||
            (d_internal_db_buffer_append(_out, "[", 1u)) )          &&
          (d_internal_mongodb_encode(_out, host, keep))              &&
          ( (!ipv6) ||
            (d_internal_db_buffer_append(_out, "]", 1u)) )          &&
          (d_internal_db_buffer_append(_out, port, strlen(port)))    &&
          (d_internal_db_buffer_append(_out, "/", 1u)) );

    return (ok) ? 0
                : d_internal_mongodb_answer(_db,
                                            D_MONGODB_LAYER_NO_MEMORY,
                                            "no memory for the connection "
                                            "string");
}

/**
 * @brief Copies a connection string given, with a slash before its options
 *        where it has none, which the driver needs.
 *
 * @param[in,out] _out  the buffer.
 * @param[in]     _uri  the string.
 * @return `true`, or `false` when memory ran out.
 */
static
bool d_internal_mongodb_given_uri(
    struct dj_internal_db_buffer* _out,
    const char*                   _uri
)
{
    const char* const scheme = strstr(_uri, "://");
    const char* const hosts  = (scheme) ? scheme + 3 : NULL;
    const size_t      span   = (hosts) ? strcspn(hosts, "/?") : 0u;

    // a string the driver will refuse, or one with its slash, as it is
    if ( (!hosts) ||
         (hosts[span] == '/') )
    {
        return d_internal_db_buffer_append(_out, _uri, strlen(_uri));
    }

    const size_t head = (size_t)(hosts - _uri) + span;

    return ( (d_internal_db_buffer_append(_out, _uri, head)) &&
             (d_internal_db_buffer_append(_out, "/", 1u))    &&
             (d_internal_db_buffer_append(_out,
                                          _uri + head,
                                          strlen(_uri + head))) );
}

/**
 * @brief Builds the connection string: the one given or one from the host,
 *        then every URI_OPTION setting, as `name=value`.
 *
 * @param[in,out] _db        the handle, on which a refusal is recorded.
 * @param[in]     _options   the options.
 * @param[in]     _settings  the settings.
 * @param[in]     _count     how many.
 * @param[out]    _out       receives the string, terminated.
 * @return 0; D_DB_INVALID_ARGUMENT for a URI_OPTION without its name or
 *         value, or a host's refusal; D_MONGODB_LAYER_NO_MEMORY.
 *
 * A value keeps `,` and `:` as they are, which separate the parts of the
 * options that have parts -- readPreferenceTags, authMechanismProperties.
 */
static
int d_internal_mongodb_uri_text(
    struct dj_mongodb*               _db,
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count,
    struct dj_internal_db_buffer*    _out
)
{
    int result = (_options->uri)
                     ? ( (d_internal_mongodb_given_uri(_out, _options->uri))
                             ? 0
                             : D_MONGODB_LAYER_NO_MEMORY )
                     : d_internal_mongodb_host_uri(_db, _options, _out);
    size_t index = 0u;
    bool   first = ( (_out->length == 0u) ||
                     (!memchr(_out->data, '?', _out->length)) );

    // each URI_OPTION, as the string would carry it
    for (index = 0u; (result == 0) && (index < _count); ++index)
    {
        const struct dj_mongodb_setting* const setting = &_settings[index];

        // the others are applied once the string is parsed
        if (setting->name != D_MONGODB_SETTING_URI_OPTION)
        {
            continue;
        }

        // an option needs its name and its value
        if ( (!setting->text)  ||
             (!*setting->text) ||
             (!setting->value) )
        {
            return d_internal_mongodb_answer(_db,
                                             D_DB_INVALID_ARGUMENT,
                                             "a URI_OPTION setting needs its "
                                             "option's name and its value");
        }

        result = ( (d_internal_db_buffer_append(_out,
                                                (first) ? "?" : "&",
                                                1u))                        &&
                   (d_internal_mongodb_encode(_out, setting->text, ""))     &&
                   (d_internal_db_buffer_append(_out, "=", 1u))             &&
                   (d_internal_mongodb_encode(_out,
                                              (const char*)setting->value,
                                              ",:")) )
                     ? 0
                     : D_MONGODB_LAYER_NO_MEMORY;
        first  = false;
    }

    // the terminator
    if ( (result == 0) &&
         (!d_internal_db_buffer_append(_out, "", 1u)) )
    {
        result = D_MONGODB_LAYER_NO_MEMORY;
    }

    return (result == D_MONGODB_LAYER_NO_MEMORY)
               ? d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the connection "
                                           "string")
               : result;
}

/**
 * @brief Clears and frees a connection string, which may hold a password.
 *
 * @param[in,out] _text  the buffer.
 */
static
void d_internal_mongodb_wipe(
    struct dj_internal_db_buffer* _text
)
{
    // the bytes, before they go back to the allocator
    if (_text->data)
    {
        memset(_text->data, 0, _text->capacity);
    }

    d_internal_db_buffer_release(_text);

    return;
}


//==============================================================================
// 2.  SETTINGS
//==============================================================================

/**
 * @brief Applies the options' fields to a parsed connection string.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in]     _options  the options.
 * @param[in,out] _uri      the parsed string.
 * @return 0, or D_DB_INVALID_ARGUMENT for a field the driver refused.
 */
static
int d_internal_mongodb_apply_fields(
    struct dj_mongodb*               _db,
    const struct dj_mongodb_options* _options,
    mongoc_uri_t*                    _uri
)
{
    const char* refused = NULL;

    // each field given, replacing what the string says
    if ( (_options->user) &&
         (!mongoc_uri_set_username(_uri, _options->user)) )
    {
        refused = "the user";
    }
    else if ( (_options->password) &&
              (!mongoc_uri_set_password(_uri, _options->password)) )
    {
        refused = "the password";
    }
    else if ( (_options->database) &&
              (!mongoc_uri_set_database(_uri, _options->database)) )
    {
        refused = "the database";
    }
    else if ( (_options->auth_source) &&
              (!mongoc_uri_set_auth_source(_uri, _options->auth_source)) )
    {
        refused = "the authentication database";
    }
    else if ( (_options->replica_set) &&
              (!mongoc_uri_set_option_as_utf8(_uri,
                                              MONGOC_URI_REPLICASET,
                                              _options->replica_set)) )
    {
        refused = "the replica set's name";
    }

    return (refused) ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "the driver refused %s, "
                                                 "which must be UTF-8",
                                                 refused)
                     : 0;
}

/**
 * @brief Applies a number of milliseconds to an option of 32 bits.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in,out] _uri      the parsed string.
 * @param[in]     _option   the option's name.
 * @param[in]     _setting  the setting.
 * @return 0, or D_DB_INVALID_ARGUMENT for a number past INT32_MAX or one
 *         the driver refused.
 */
static
int d_internal_mongodb_apply_int32(
    struct dj_mongodb*               _db,
    mongoc_uri_t*                    _uri,
    const char*                      _option,
    const struct dj_mongodb_setting* _setting
)
{
    // the option's 32 bits
    if ( (_setting->number > (unsigned long)INT32_MAX) ||
         (!mongoc_uri_set_option_as_int32(_uri,
                                          _option,
                                          (int32_t)_setting->number)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "%s cannot be %lu",
                                         _option,
                                         _setting->number);
    }

    return 0;
}

/**
 * @brief Applies text to an option, or to one the driver has a call for.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in,out] _uri      the parsed string.
 * @param[in]     _option   the option's name.
 * @param[in]     _setting  the setting.
 * @return 0, or D_DB_INVALID_ARGUMENT for no text or text the driver
 *         refused.
 */
static
int d_internal_mongodb_apply_text(
    struct dj_mongodb*               _db,
    mongoc_uri_t*                    _uri,
    const char*                      _option,
    const struct dj_mongodb_setting* _setting
)
{
    const char* const text = _setting->text;
    bool              ok   = (text != NULL);

    // the call the option has
    if (ok)
    {
        switch (_setting->name)
        {
            case D_MONGODB_SETTING_APP_NAME:
                ok = mongoc_uri_set_appname(_uri, text);
                break;

            case D_MONGODB_SETTING_AUTH_MECHANISM:
                ok = mongoc_uri_set_auth_mechanism(_uri, text);
                break;

            case D_MONGODB_SETTING_COMPRESSORS:
                ok = mongoc_uri_set_compressors(_uri, text);
                break;

            default:
                ok = mongoc_uri_set_option_as_utf8(_uri, _option, text);
                break;
        }
    }

    return (ok) ? 0
                : d_internal_mongodb_answer(_db,
                                            D_DB_INVALID_ARGUMENT,
                                            (text) ? "the driver refused "
                                                     "the text for %s"
                                                   : "%s needs text",
                                            _option);
}

/**
 * @brief Applies authentication's properties, written `NAME:value,...`.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in,out] _uri      the parsed string.
 * @param[in]     _setting  the setting.
 * @return 0, or D_DB_INVALID_ARGUMENT for no text, a pair without its
 *         colon, or properties the driver refused.
 */
static
int d_internal_mongodb_apply_properties(
    struct dj_mongodb*               _db,
    mongoc_uri_t*                    _uri,
    const struct dj_mongodb_setting* _setting
)
{
    const char* cursor = _setting->text;
    bool        ok     = (cursor != NULL);
    bson_t      properties;

    bson_init(&properties);

    // each pair, to its comma
    while ( (ok) &&
            (*cursor) )
    {
        const size_t      length = strcspn(cursor, ",");
        const char* const colon  = (const char*)memchr(cursor, ':', length);

        ok = ( (colon)          &&
               (colon > cursor) &&
               (bson_append_utf8(&properties,
                                 cursor,
                                 (int)(colon - cursor),
                                 colon + 1,
                                 (int)(length - (size_t)(colon - cursor) -
                                       1u))) );
        cursor += length + ( (cursor[length] == ',') ? 1u : 0u );
    }

    ok = (ok) && (mongoc_uri_set_mechanism_properties(_uri, &properties));
    bson_destroy(&properties);

    return (ok) ? 0
                : d_internal_mongodb_answer(_db,
                                            D_DB_INVALID_ARGUMENT,
                                            "authMechanismProperties must be "
                                            "written NAME:value, comma "
                                            "between pairs");
}

/**
 * @brief Makes the declared API a SERVER_API setting asks for, replacing
 *        one an earlier setting made.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in]     _setting  the setting.
 * @param[in,out] _api      the declared API, NULL until one is made.
 * @return 0, or D_DB_INVALID_ARGUMENT for no version, a version of no such
 *         name, or flags past D_MONGODB_API_*.
 */
static
int d_internal_mongodb_apply_api(
    struct dj_mongodb*               _db,
    const struct dj_mongodb_setting* _setting,
    mongoc_server_api_t**            _api
)
{
    const unsigned long         known   = (unsigned long)
                                          (D_MONGODB_API_STRICT |
                                           D_MONGODB_API_DEPRECATION_ERRORS);
    mongoc_server_api_version_t version = MONGOC_SERVER_API_V1;

    // a version the driver knows, and flags the layer does
    if ( (!_setting->text)                                                ||
         (!mongoc_server_api_version_from_string(_setting->text, &version)) ||
         ((_setting->number & ~known) != 0ul) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "SERVER_API needs a version the "
                                         "driver declares, \"1\", and "
                                         "D_MONGODB_API_* flags");
    }

    mongoc_server_api_destroy(*_api);
    *_api = mongoc_server_api_new(version);
    mongoc_server_api_strict(
        *_api,
        ((_setting->number & D_MONGODB_API_STRICT) != 0ul));
    mongoc_server_api_deprecation_errors(
        *_api,
        ((_setting->number & D_MONGODB_API_DEPRECATION_ERRORS) != 0ul));

    return 0;
}

/**
 * @brief The connection-string option a setting names.
 *
 * @param[in] _name  the setting's name.
 * @return the option's name, or `NULL` for a setting with a call of its
 *         own, or of no such name.
 */
static const char*
d_internal_mongodb_option_of(
    enum dj_mongodb_setting_name _name
)
{
    switch (_name)
    {
        case D_MONGODB_SETTING_SERVER_SELECTION_TIMEOUT:
            return MONGOC_URI_SERVERSELECTIONTIMEOUTMS;

        case D_MONGODB_SETTING_CONNECT_TIMEOUT:
            return MONGOC_URI_CONNECTTIMEOUTMS;

        case D_MONGODB_SETTING_SOCKET_TIMEOUT:
            return MONGOC_URI_SOCKETTIMEOUTMS;

        case D_MONGODB_SETTING_APP_NAME:
            return MONGOC_URI_APPNAME;

        case D_MONGODB_SETTING_TLS:
            return MONGOC_URI_TLS;

        case D_MONGODB_SETTING_TLS_CA_FILE:
            return MONGOC_URI_TLSCAFILE;

        case D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_FILE:
            return MONGOC_URI_TLSCERTIFICATEKEYFILE;

        case D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_PASSWORD:
            return MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD;

        case D_MONGODB_SETTING_TLS_ALLOW_INVALID_CERTIFICATES:
            return MONGOC_URI_TLSALLOWINVALIDCERTIFICATES;

        case D_MONGODB_SETTING_TLS_ALLOW_INVALID_HOSTNAMES:
            return MONGOC_URI_TLSALLOWINVALIDHOSTNAMES;

        case D_MONGODB_SETTING_AUTH_MECHANISM:
            return MONGOC_URI_AUTHMECHANISM;

        case D_MONGODB_SETTING_COMPRESSORS:
            return MONGOC_URI_COMPRESSORS;

        case D_MONGODB_SETTING_RETRY_WRITES:
            return MONGOC_URI_RETRYWRITES;

        case D_MONGODB_SETTING_RETRY_READS:
            return MONGOC_URI_RETRYREADS;

        case D_MONGODB_SETTING_DIRECT_CONNECTION:
            return MONGOC_URI_DIRECTCONNECTION;

        case D_MONGODB_SETTING_LOAD_BALANCED:
            return MONGOC_URI_LOADBALANCED;

        default:
            return NULL;
    }
}

/**
 * @brief Applies one setting, of any name but URI_OPTION's, which the
 *        string carried.
 *
 * @param[in,out] _db       the handle, on which a refusal is recorded.
 * @param[in,out] _uri      the parsed string.
 * @param[in]     _setting  the setting.
 * @param[in,out] _api      the declared API, for SERVER_API.
 * @return 0, or D_DB_INVALID_ARGUMENT, as each kind of setting says.
 */
static
int d_internal_mongodb_apply_setting(
    struct dj_mongodb*               _db,
    mongoc_uri_t*                    _uri,
    const struct dj_mongodb_setting* _setting,
    mongoc_server_api_t**            _api
)
{
    const char* const option = d_internal_mongodb_option_of(_setting->name);

    switch (_setting->name)
    {
        case D_MONGODB_SETTING_SERVER_SELECTION_TIMEOUT:
        case D_MONGODB_SETTING_CONNECT_TIMEOUT:
        case D_MONGODB_SETTING_SOCKET_TIMEOUT:
            return d_internal_mongodb_apply_int32(_db, _uri, option, _setting);

        case D_MONGODB_SETTING_TLS:
        case D_MONGODB_SETTING_TLS_ALLOW_INVALID_CERTIFICATES:
        case D_MONGODB_SETTING_TLS_ALLOW_INVALID_HOSTNAMES:
        case D_MONGODB_SETTING_RETRY_WRITES:
        case D_MONGODB_SETTING_RETRY_READS:
        case D_MONGODB_SETTING_DIRECT_CONNECTION:
        case D_MONGODB_SETTING_LOAD_BALANCED:
            return (mongoc_uri_set_option_as_bool(_uri,
                                                  option,
                                                  (_setting->number != 0ul)))
                       ? 0
                       : d_internal_mongodb_answer(_db,
                                                   D_DB_INVALID_ARGUMENT,
                                                   "the driver refused %s",
                                                   option);

        case D_MONGODB_SETTING_APP_NAME:
        case D_MONGODB_SETTING_TLS_CA_FILE:
        case D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_FILE:
        case D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_PASSWORD:
        case D_MONGODB_SETTING_AUTH_MECHANISM:
        case D_MONGODB_SETTING_COMPRESSORS:
            return d_internal_mongodb_apply_text(_db, _uri, option, _setting);

        case D_MONGODB_SETTING_AUTH_MECHANISM_PROPERTIES:
            return d_internal_mongodb_apply_properties(_db, _uri, _setting);

        case D_MONGODB_SETTING_SERVER_API:
            return d_internal_mongodb_apply_api(_db, _setting, _api);

        case D_MONGODB_SETTING_URI_OPTION:
            return 0;

        default:
            return d_internal_mongodb_answer(_db,
                                             D_DB_INVALID_ARGUMENT,
                                             "a setting of no such name, %d",
                                             (int)_setting->name);
    }
}

/**
 * @brief Applies the layer's defaults where neither the string nor a
 *        setting named them: the time to select a server, and the name the
 *        server logs.
 *
 * @param[in,out] _db   the handle, on which a refusal is recorded.
 * @param[in,out] _uri  the parsed string.
 * @return 0, or D_DB_INVALID_ARGUMENT where the driver refused a knob's
 *         value.
 */
static
int d_internal_mongodb_apply_defaults(
    struct dj_mongodb* _db,
    mongoc_uri_t*      _uri
)
{
    static const char APP_NAME[] = D_INTERNAL_MONGODB_APP_NAME;

    // the time to select a server
    if ( (!mongoc_uri_has_option(_uri,
                                 MONGOC_URI_SERVERSELECTIONTIMEOUTMS)) &&
         (!mongoc_uri_set_option_as_int32(
              _uri,
              MONGOC_URI_SERVERSELECTIONTIMEOUTMS,
              (int32_t)D_INTERNAL_MONGODB_SERVER_SELECTION_TIMEOUT_MS)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "the driver refused "
                                         "D_CFG_MONGODB_SERVER_SELECTION_"
                                         "TIMEOUT_MS");
    }

    // the name the server logs
    if ( (APP_NAME[0] != '\0')              &&
         (!mongoc_uri_get_appname(_uri))    &&
         (!mongoc_uri_set_appname(_uri, APP_NAME)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "the driver refused "
                                         "D_CFG_MONGODB_APP_NAME, which is "
                                         "128 bytes at most");
    }

    return 0;
}

/**
 * @brief Applies the fields, the settings and the defaults to a parsed
 *        string.
 *
 * @param[in,out] _db        the handle, on which a refusal is recorded.
 * @param[in]     _options   the options.
 * @param[in]     _settings  the settings.
 * @param[in]     _count     how many.
 * @param[in,out] _uri       the parsed string.
 * @param[out]    _api       receives the declared API, `NULL` for none.
 * @return 0, or the first refusal.
 */
static
int d_internal_mongodb_apply(
    struct dj_mongodb*               _db,
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count,
    mongoc_uri_t*                    _uri,
    mongoc_server_api_t**            _api
)
{
    int    result = d_internal_mongodb_apply_fields(_db, _options, _uri);
    size_t index  = 0u;

    // each setting, in its order
    for (index = 0u; (result == 0) && (index < _count); ++index)
    {
        result = d_internal_mongodb_apply_setting(_db,
                                                  _uri,
                                                  &_settings[index],
                                                  _api);
    }

    return (result == 0) ? d_internal_mongodb_apply_defaults(_db, _uri)
                         : result;
}


//==============================================================================
// 3.  THE SERVER
//==============================================================================

/**
 * @brief Runs a command of one field, {name: 1}, on admin.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _name   the command.
 * @param[out]    _reply  initialized, and receives the reply.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_admin(
    struct dj_mongodb* _db,
    const char*        _name,
    bson_t*            _reply
)
{
    bson_t command;

    bson_init(&command);
    BSON_APPEND_INT32(&command, _name, 1);

    const int result = d_internal_mongodb_run(_db,
                                              "admin",
                                              &command,
                                              true,
                                              _reply);

    bson_destroy(&command);

    return result;
}

/**
 * @brief Reads what hello said of the deployment into the handle.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _reply  hello's reply, or isMaster's.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY for the set's name.
 *
 * A load balancer is the connection string's to say, since hello names it
 * only to the driver's own handshake; a member of a set not yet initiated
 * has no name, and is neither a set nor standalone.
 */
static
int d_internal_mongodb_read_hello(
    struct dj_mongodb* _db,
    const bson_t*      _reply
)
{
    bson_iter_t field;

    const bool balanced = mongoc_uri_get_option_as_bool(
                              mongoc_client_get_uri(_db->client),
                              MONGOC_URI_LOADBALANCED,
                              false);
    const bool router   = ( (bson_iter_init_find(&field, _reply, "msg")) &&
                            (BSON_ITER_HOLDS_UTF8(&field))              &&
                            (strcmp(bson_iter_utf8(&field, NULL),
                                    "isdbgrid") == 0) );
    const bool ghost    = ( (bson_iter_init_find(&field,
                                                 _reply,
                                                 "isreplicaset")) &&
                            (bson_iter_as_bool(&field)) );
    const bool member   = ( (bson_iter_init_find(&field, _reply, "setName")) &&
                            (BSON_ITER_HOLDS_UTF8(&field)) );

    _db->replica_set = (member)
                           ? d_internal_mongodb_strdup(
                                 bson_iter_utf8(&field, NULL))
                           : NULL;
    _db->topology    = (balanced) ? D_MONGODB_TOPOLOGY_LOAD_BALANCED
                     : (router)   ? D_MONGODB_TOPOLOGY_SHARDED
                     : (member)   ? D_MONGODB_TOPOLOGY_REPLICA_SET
                     : (ghost)    ? D_MONGODB_TOPOLOGY_UNKNOWN
                                  : D_MONGODB_TOPOLOGY_STANDALONE;
    _db->wire        = ( (bson_iter_init_find(&field,
                                              _reply,
                                              "maxWireVersion")) &&
                         (BSON_ITER_HOLDS_INT32(&field)) )
                           ? (int)bson_iter_int32(&field)
                           : 0;

    return ( (member) &&
             (!_db->replica_set) )
               ? d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the replica set's "
                                           "name")
               : 0;
}

/**
 * @brief Reads buildInfo's version into the handle: its string, and its
 *        number from versionArray.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _reply  buildInfo's reply.
 */
static
void d_internal_mongodb_read_build(
    struct dj_mongodb* _db,
    const bson_t*      _reply
)
{
    static const unsigned long SCALE[3] = { 10000ul, 100ul, 1ul };

    bson_iter_t field;
    bson_iter_t part;
    size_t      index = 0u;

    // the string, as written
    if ( (bson_iter_init_find(&field, _reply, "version")) &&
         (BSON_ITER_HOLDS_UTF8(&field)) )
    {
        (void)snprintf(_db->release,
                       sizeof(_db->release),
                       "%s",
                       bson_iter_utf8(&field, NULL));
    }

    _db->version = 0ul;

    // the number, from its first three parts
    if ( (bson_iter_init_find(&field, _reply, "versionArray")) &&
         (BSON_ITER_HOLDS_ARRAY(&field))                       &&
         (bson_iter_recurse(&field, &part)) )
    {
        while ( (index < 3u) &&
                (bson_iter_next(&part)) )
        {
            const int64_t value = (BSON_ITER_HOLDS_NUMBER(&part))
                                      ? bson_iter_as_int64(&part)
                                      : 0;

            _db->version += ( (value > 0) &&
                              (value < 100) )
                                ? (unsigned long)value * SCALE[index]
                                : 0ul;
            ++index;
        }
    }

    return;
}

/**
 * @brief The oldest release that speaks a wire version, for a server that
 *        refused buildInfo.
 *
 * @param[in] _wire  the server's maxWireVersion.
 * @return the release, MAJOR*10000 + MINOR*100; 0 below the floor's.
 */
static
unsigned long d_internal_mongodb_wire_version(
    int _wire
)
{
    unsigned long version = 0ul;
    size_t        index   = 0u;

    // the last release at or below it
    for (index = 0u;
         index < (sizeof(WIRE_RELEASES) / sizeof(WIRE_RELEASES[0]));
         ++index)
    {
        if (WIRE_RELEASES[index].wire <= _wire)
        {
            version = WIRE_RELEASES[index].version;
        }
    }

    return version;
}

/**
 * @brief Forgets the failure recorded last, which the layer got past: a new
 *        handle has none.
 *
 * @param[in,out] _db  the handle.
 */
static
void d_internal_mongodb_forget(
    struct dj_mongodb* _db
)
{
    // the reply kept with it
    if (_db->reply)
    {
        bson_destroy(_db->reply);
        _db->reply = NULL;
    }

    _db->error         = 0;
    _db->message[0]    = '\0';
    _db->error_name[0] = '\0';

    return;
}

/**
 * @brief Learns the server: a ping, which signs in; hello, or isMaster
 *        before it; buildInfo.
 *
 * @param[in,out] _db  the handle, its client made.
 * @return 0, or the first failure, recorded.
 */
static
int d_internal_mongodb_learn(
    struct dj_mongodb* _db
)
{
    bson_t reply;
    int    result = d_internal_mongodb_admin(_db, "ping", &reply);

    bson_destroy(&reply);

    // what the server is part of: hello, or isMaster on servers before it
    if (result == 0)
    {
        result = d_internal_mongodb_admin(_db, "hello", &reply);

        if (result == D_MONGODB_ERROR_COMMAND_NOT_FOUND)
        {
            bson_destroy(&reply);
            result = d_internal_mongodb_admin(_db, "isMaster", &reply);
        }

        result = (result == 0) ? d_internal_mongodb_read_hello(_db, &reply)
                               : result;
        bson_destroy(&reply);
    }

    // its version, or its wire version's where buildInfo is refused
    if (result == 0)
    {
        result = d_internal_mongodb_admin(_db, "buildInfo", &reply);

        if (result == 0)
        {
            d_internal_mongodb_read_build(_db, &reply);
        }
        else if ( (result == API_STRICT_ERROR) ||
                  (result == D_MONGODB_ERROR_UNAUTHORIZED) )
        {
            _db->version    = d_internal_mongodb_wire_version(_db->wire);
            _db->release[0] = '\0';
            result          = 0;
            d_internal_mongodb_forget(_db);
        }

        bson_destroy(&reply);
    }

    return result;
}


//==============================================================================
// 4.  HANDLES
//==============================================================================

/**
 * @brief Allocates a handle with no client, set up as the knobs say.
 *
 * @return the handle, or `NULL` when memory ran out.
 */
static
struct dj_mongodb* d_internal_mongodb_new_handle(void)
{
    struct dj_mongodb* const db = (struct dj_mongodb*)calloc(1u, sizeof(*db));

    // the knobs' settings
    if (db)
    {
        db->topology = D_MONGODB_TOPOLOGY_UNKNOWN;
        db->format   = (enum dj_mongodb_document_format)
                       D_INTERNAL_MONGODB_DOCUMENT_FORMAT;
    }

    return db;
}

/**
 * @brief Makes the client from a parsed string, with the second error API
 *        and the declared API, and takes the current database from it.
 *
 * @param[in,out] _db   the handle, which owns the client after.
 * @param[in]     _uri  the parsed string.
 * @param[in]     _api  the declared API, or `NULL`.
 * @return 0; the driver's failure; D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_make_client(
    struct dj_mongodb*         _db,
    const mongoc_uri_t*        _uri,
    const mongoc_server_api_t* _api
)
{
    bson_error_t error;

    _db->client = mongoc_client_new_from_uri_with_error(_uri, &error);
    _db->owned  = true;

    // the client, as the driver made it
    if (!_db->client)
    {
        return d_internal_mongodb_fail_outside(_db, &error, NULL);
    }

    (void)mongoc_client_set_error_api(_db->client,
                                      MONGOC_ERROR_API_VERSION_2);

    // the declared API, before any command
    if ( (_api) &&
         (!mongoc_client_set_server_api(_db->client, _api, &error)) )
    {
        return d_internal_mongodb_fail_outside(_db, &error, NULL);
    }

    const char* const database = mongoc_uri_get_database(_uri);

    _db->database = d_internal_mongodb_strdup(database);

    return ( (database) &&
             (!_db->database) )
               ? d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the database's "
                                           "name")
               : 0;
}

/**
 * @brief Opens a handle: the string built and parsed, the settings applied,
 *        the client made, the server learned.
 *
 * @param[in,out] _db        the handle, with no client yet.
 * @param[in]     _options   the options.
 * @param[in]     _settings  the settings.
 * @param[in]     _count     how many.
 * @return 0, or the first failure, recorded.
 */
static
int d_internal_mongodb_open(
    struct dj_mongodb*               _db,
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count
)
{
    struct dj_internal_db_buffer text = { NULL, 0u, 0u };
    bson_error_t                error;
    int                         result = d_internal_mongodb_uri_text(_db,
                                                                     _options,
                                                                     _settings,
                                                                     _count,
                                                                     &text);
    mongoc_uri_t* const         uri    = (result == 0)
                                             ? mongoc_uri_new_with_error(
                                                   text.data,
                                                   &error)
                                             : NULL;
    mongoc_server_api_t*        api    = NULL;

    d_internal_mongodb_wipe(&text);

    // a string the driver could not read
    if ( (result == 0) &&
         (!uri) )
    {
        result = d_internal_mongodb_fail_outside(_db, &error, NULL);
    }

    result = (result == 0) ? d_internal_mongodb_apply(_db,
                                                      _options,
                                                      _settings,
                                                      _count,
                                                      uri,
                                                      &api)
                           : result;
    result = (result == 0) ? d_internal_mongodb_make_client(_db, uri, api)
                           : result;

    mongoc_server_api_destroy(api);
    mongoc_uri_destroy(uri);

    return (result == 0) ? d_internal_mongodb_learn(_db) : result;
}

/**
 * @brief Writes a refusal's reason where the caller asked for it.
 *
 * @param[out] _message  where to write a copy, or `NULL`.
 * @param[in]  _result   the result.
 * @param[in]  _reason   the reason.
 * @return `_result`.
 */
static
int d_internal_mongodb_refuse(
    char**      _message,
    int         _result,
    const char* _reason
)
{
    // a copy, if one is wanted
    if (_message)
    {
        *_message = d_internal_mongodb_strdup(_reason);
    }

    return _result;
}


//==============================================================================
// 5.  CONNECTIONS
//==============================================================================

// 5.1    Connecting and closing
//------------------------------------------------------------------------------
/*
d_mongodb_connect
  connect_with, with no settings and no message.
*/
int d_mongodb_connect(
    const struct dj_mongodb_options* _options,
    struct dj_mongodb**              _db
)
{
    return d_mongodb_connect_with(_options, NULL, 0u, _db, NULL);
}

/*
d_mongodb_connect_uri
  connect_with, the string as the options' only field.
*/
int d_mongodb_connect_uri(
    const char*         _uri,
    struct dj_mongodb** _db
)
{
    struct dj_mongodb_options options = { NULL, NULL, NULL, NULL,
                                         0u,   NULL, NULL, NULL };

    // a string to connect with
    if (!_uri)
    {
        if (_db)
        {
            *_db = NULL;
        }

        return D_DB_INVALID_ARGUMENT;
    }

    options.uri = _uri;

    return d_mongodb_connect_with(&options, NULL, 0u, _db, NULL);
}

/**
 * @brief Checks what a connection is given: somewhere for the handle,
 *        settings where a count says, and a string or a host, not both.
 *
 * @param[in]  _options   the options.
 * @param[in]  _settings  the settings, or `NULL` for none.
 * @param[in]  _count     how many.
 * @param[out] _db        where the handle goes, written `NULL` on a
 *                        refusal.
 * @param[out] _message   receives the refusal's message, or `NULL`.
 * @return 0, or D_DB_INVALID_ARGUMENT.
 */
static
int d_internal_mongodb_check_connect(
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count,
    struct dj_mongodb**              _db,
    char**                           _message
)
{
    // each of the three
    if ( (_db)                       &&
         ( (_settings) ||
           (_count == 0u) )          &&
         ( (!_options->uri) ||
           ( (!_options->host) &&
             (_options->port == 0u) ) ) )
    {
        return 0;
    }

    // no handle, where there is somewhere to say so
    if (_db)
    {
        *_db = NULL;
    }

    return d_internal_mongodb_refuse(
               _message,
               D_DB_INVALID_ARGUMENT,
               (!_db) ? "no place was given for the handle"
                      : ( ( (!_settings) &&
                              (_count != 0u) )
                              ? "no settings were given for a count of "
                                "them"
                              : "a host or a port was given beside a "
                                "connection string" ));
}

/*
d_mongodb_connect_with
  The handle is made before anything can fail on it, so that its record of
the failure can be copied out before it is closed.
*/
int d_mongodb_connect_with(
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count,
    struct dj_mongodb**              _db,
    char**                           _message
)
{
    static const struct dj_mongodb_options NONE = { NULL, NULL, NULL, NULL,
                                                   0u,   NULL, NULL, NULL };

    const struct dj_mongodb_options* const options = (_options) ? _options
                                                               : &NONE;
    const int                             checked =
        d_internal_mongodb_check_connect(options,
                                         _settings,
                                         _count,
                                         _db,
                                         _message);

    // what a connection is given
    if (checked != 0)
    {
        return checked;
    }

    d_internal_mongodb_start();
    *_db = NULL;

    struct dj_mongodb* const db = d_internal_mongodb_new_handle();

    // the handle itself
    if (!db)
    {
        return d_internal_mongodb_refuse(_message,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the handle");
    }

    const int result = d_internal_mongodb_open(db, options, _settings, _count);

    // the failure, copied out, and the handle closed
    if (result != 0)
    {
        (void)d_internal_mongodb_refuse(_message, result, db->message);
        (void)d_mongodb_close(db);

        return result;
    }

    // a success leaves no message
    if (_message)
    {
        *_message = NULL;
    }

    *_db = db;

    return 0;
}

/**
 * @brief Has an adopted client read failures in the driver's second error
 *        API, which the layer's results rest on.
 *
 * @param[in,out] _db  the handle, its client the program's.
 * @return 0; D_DB_INVALID_ARGUMENT for a client its maker or its pool set
 *         to the first; the driver's failure, packed, when no server
 *         answers.
 *
 * The driver can say which API a client reads only by a failure it reads:
 * a command of no such name, which every server refuses, comes back in the
 * server's domain in the second and in the query domain in the first. A
 * client never set reads the first and can be set once; asking only then
 * keeps the driver from logging a refusal for a client set already.
 */
static
int d_internal_mongodb_error_api(
    struct dj_mongodb* _db
)
{
    bson_t       command;
    bson_t       reply;
    bson_error_t error;

    memset(&error, 0, sizeof(error));
    bson_init(&command);
    BSON_APPEND_INT32(&command, "dtmErrorApi", 1);

    const bool ran = mongoc_client_command_simple(_db->client,
                                                  "admin",
                                                  &command,
                                                  NULL,
                                                  &reply,
                                                  &error);

    bson_destroy(&reply);
    bson_destroy(&command);

    // the second already, or no server to ask
    if ( (ran) ||
         (error.domain == (uint32_t)MONGOC_ERROR_SERVER) )
    {
        return 0;
    }

    if (error.domain != (uint32_t)MONGOC_ERROR_QUERY)
    {
        return d_internal_mongodb_fail_outside(_db, &error, NULL);
    }

    // the first: the second where the client was never set
    return (mongoc_client_set_error_api(_db->client,
                                        MONGOC_ERROR_API_VERSION_2))
               ? 0
               : d_internal_mongodb_answer(
                     _db,
                     D_DB_INVALID_ARGUMENT,
                     "the client reads failures in the driver's first "
                     "error API, set so by its maker or its pool, and the "
                     "layer reads the second: set it with "
                     "mongoc_client_set_error_api or "
                     "mongoc_client_pool_set_error_api first");
}

/*
d_mongodb_adopt
  The client is the caller's until the handle is made: it is owned only on
success, so that a failure leaves it as it was.
*/
int d_mongodb_adopt(
    void*               _native,
    bool                _owned,
    struct dj_mongodb** _db
)
{
    // a client, and somewhere for the handle
    if ( (!_native) ||
         (!_db) )
    {
        if (_db)
        {
            *_db = NULL;
        }

        return D_DB_INVALID_ARGUMENT;
    }

    d_internal_mongodb_start();
    *_db = NULL;

    struct dj_mongodb* const db = d_internal_mongodb_new_handle();

    // the handle itself
    if (!db)
    {
        return D_MONGODB_LAYER_NO_MEMORY;
    }

    db->client = (mongoc_client_t*)_native;

    const char* const database =
        mongoc_uri_get_database(mongoc_client_get_uri(db->client));

    db->database = d_internal_mongodb_strdup(database);

    int result = ( (database) &&
                   (!db->database) )
                     ? D_MONGODB_LAYER_NO_MEMORY
                     : d_internal_mongodb_error_api(db);

    result = (result == 0) ? d_internal_mongodb_learn(db) : result;

    // a failure leaves the client the caller's
    if (result != 0)
    {
        (void)d_mongodb_close(db);

        return result;
    }

    db->owned = _owned;
    *_db      = db;

    return 0;
}

/*
d_mongodb_close
  The session goes before the client it came from; destroying it aborts a
transaction still open, on the server too. The transaction's cursors end
first, as their drivers' cursors hold the session.
*/
int d_mongodb_close(
    struct dj_mongodb* _db
)
{
    // nothing to close
    if (!_db)
    {
        return 0;
    }

    // the open transaction's cursors, which hold the session; the session,
    // then the client, when the layer owns it
    d_internal_mongodb_end_cursors(_db);

    if (_db->session)
    {
        mongoc_client_session_destroy(_db->session);
    }

    if ( (_db->owned) &&
         (_db->client) )
    {
        mongoc_client_destroy(_db->client);
    }

    // what the handle kept
    if (_db->reply)
    {
        bson_destroy(_db->reply);
    }

    if (_db->has_id)
    {
        bson_value_destroy(&_db->id);
    }

    free(_db->database);
    free(_db->replica_set);
    free(_db);

    return 0;
}

// 5.2    Liveness
//------------------------------------------------------------------------------
/*
d_mongodb_ping
  Outside any transaction, which ping cannot be part of.
*/
int d_mongodb_ping(
    struct dj_mongodb* _db
)
{
    // a handle to ping through
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_t    reply;
    const int result = d_internal_mongodb_admin(_db, "ping", &reply);

    bson_destroy(&reply);

    return result;
}

// 5.3    The latest write
//------------------------------------------------------------------------------
/*
d_mongodb_changes
  Recorded by each write as it succeeds.
*/
int64_t d_mongodb_changes(
    const struct dj_mongodb* _db
)
{
    return (_db) ? _db->changes : 0;
}

/*
d_mongodb_last_insert_id
  Written as the handle's format writes a value: canonical for BSON.
*/
int d_mongodb_last_insert_id(
    const struct dj_mongodb* _db,
    char**                   _id
)
{
    // a handle, and somewhere to write
    if ( (!_db) ||
         (!_id) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // no new document
    if (!_db->has_id)
    {
        return D_DB_NOT_FOUND;
    }

    char* const text = d_internal_mongodb_value_json(&_db->id, _db->format);

    // no room for it
    if (!text)
    {
        return D_MONGODB_LAYER_NO_MEMORY;
    }

    *_id = text;

    return 0;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports what a build
// without the driver can: D_DB_UNSUPPORTED, or false, 0 or NULL where it
// cannot fail. An answer a function promises even on failure is still
// written.

// NO_DRIVER
//   constant: the reason a stub gives.
static const char NO_DRIVER[] =
    "this build of the MongoDB layer has no driver";

/**
 * @brief Writes a copy of the stubs' reason where the caller asked for it.
 *
 * @param[out] _message  where to write it, or `NULL`.
 *
 * malloc's, so that d_db_free releases it as it releases the driver's.
 */
static
void d_internal_mongodb_no_driver(
    char** _message
)
{
    // a copy, if one is wanted and memory allows
    if (_message)
    {
        char* const copy = (char*)malloc(sizeof(NO_DRIVER));

        if (copy)
        {
            memcpy(copy, NO_DRIVER, sizeof(NO_DRIVER));
        }

        *_message = copy;
    }

    return;
}

/*
d_mongodb_connect
  Without the driver, D_DB_UNSUPPORTED and no handle.
*/
int d_mongodb_connect(
    const struct dj_mongodb_options* _options,
    struct dj_mongodb**              _db
)
{
    (void)_options;

    // no handle, where there is somewhere to say so
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_connect_uri
  Without the driver, D_DB_UNSUPPORTED and no handle.
*/
int d_mongodb_connect_uri(
    const char*         _uri,
    struct dj_mongodb** _db
)
{
    (void)_uri;

    // no handle, where there is somewhere to say so
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_connect_with
  Without the driver, D_DB_UNSUPPORTED, no handle, and the build's reason.
*/
int d_mongodb_connect_with(
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count,
    struct dj_mongodb**              _db,
    char**                           _message
)
{
    (void)_options;
    (void)_settings;
    (void)_count;

    // no handle, where there is somewhere to say so
    if (_db)
    {
        *_db = NULL;
    }

    d_internal_mongodb_no_driver(_message);

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_adopt
  Without the driver, D_DB_UNSUPPORTED and no handle; the client is the
caller's.
*/
int d_mongodb_adopt(
    void*               _native,
    bool                _owned,
    struct dj_mongodb** _db
)
{
    (void)_native;
    (void)_owned;

    // no handle, where there is somewhere to say so
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_close
  Without the driver no handle exists: 0.
*/
int d_mongodb_close(
    struct dj_mongodb* _db
)
{
    (void)_db;

    return 0;
}

/*
d_mongodb_ping
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_ping(
    struct dj_mongodb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_changes
  Without the driver, 0.
*/
int64_t d_mongodb_changes(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return 0;
}

/*
d_mongodb_last_insert_id
  Without the driver, D_DB_UNSUPPORTED, the id untouched.
*/
int d_mongodb_last_insert_id(
    const struct dj_mongodb* _db,
    char**                   _id
)
{
    (void)_db;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
