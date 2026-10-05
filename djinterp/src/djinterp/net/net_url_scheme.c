/*******************************************************************************
* djinterp [net]                                                net_url_scheme.c
*
* Schemes and endpoints: net_url.h's section 5.
*   One table holds every scheme with a registered default port, whether it
* begins with TLS, and whether its empty path is "/"; everything that knows a
* scheme reads it. A URL's endpoint is its host, decoded, and its explicit
* port or its scheme's default.
*
*
* path:      /src/djinterp/net/net_url_scheme.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../inc/djinterp/net/net_url.h"  // corresponding header
// std
#include <string.h>  // strlen
// djinterp
#include "../../../inc/djinterp/net/net.h"  // d_net_endpoint_set, D_NET_HOST_MAX
#include "./net_url_internal.h"             // shared helpers


// d_net_url_scheme_entry
//   struct: a registered scheme, its default port, whether it runs over TLS
// from its first byte, and whether an empty path means "/" (RFC 9110, 4.2).
struct d_net_url_scheme_entry
{
    const char* name;
    d_net_port  port;
    bool        tls;
    bool        web;
};

// SCHEMES
//   constant: the schemes whose default port is registered, and the ones
// among them that begin with TLS.
static const struct d_net_url_scheme_entry SCHEMES[] =
{
    { "http",    80u, false, true  },
    { "https",  443u, true,  true  },
    { "ws",      80u, false, true  },
    { "wss",    443u, true,  true  },
    { "ftp",     21u, false, false },
    { "ftps",   990u, true,  false },
    { "ssh",     22u, false, false },
    { "sftp",    22u, false, false },
    { "telnet",  23u, false, false },
    { "smtp",    25u, false, false },
    { "smtps",  465u, true,  false },
    { "imap",   143u, false, false },
    { "imaps",  993u, true,  false },
    { "pop",    110u, false, false },
    { "pop3",   110u, false, false },
    { "pop3s",  995u, true,  false },
    { "ldap",   389u, false, false },
    { "ldaps",  636u, true,  false }
};

/*
d_net_url_scheme_entry_of
  The registered entry for a scheme, compared without case, or NULL.
*/
static const struct d_net_url_scheme_entry*
d_net_url_scheme_entry_of(
    struct d_pack_text _scheme
)
{
    // each registered scheme
    for (size_t e = 0u; e < sizeof(SCHEMES) / sizeof(SCHEMES[0]); ++e)
    {
        const char*  name  = SCHEMES[e].name;
        const size_t count = strlen(name);
        size_t       same  = 0u;

        // compare letter by letter, folding ASCII case
        while ( (same < count)          &&
                (same < _scheme.length) &&
                (d_net_url_lower(_scheme.data[same]) == name[same]) )
        {
            same += 1u;
        }

        // every letter matched, and nothing is left over
        if ( (same == count) &&
             (count == _scheme.length) )
        {
            return &SCHEMES[e];
        }
    }

    return NULL;
}

/*
d_net_url_internal_is_web_scheme
  The schemes whose empty path is "/" (RFC 3986, 6.2.3; RFC 9110, 4.2).
*/
bool
d_net_url_internal_is_web_scheme(
    struct d_pack_text _scheme
)
{
    const struct d_net_url_scheme_entry* entry =
        d_net_url_scheme_entry_of(_scheme);

    return ( (entry) &&
             (entry->web) );
}

/*
d_net_url_default_port
  From SCHEMES; 0 for an unregistered scheme.
*/
d_net_port
d_net_url_default_port(
    struct d_pack_text _scheme
)
{
    const struct d_net_url_scheme_entry* entry =
        d_net_url_scheme_entry_of(_scheme);

    return (entry) ? entry->port
                   : 0u;
}

/*
d_net_url_scheme_is_tls
  From SCHEMES; false for an unregistered scheme.
*/
bool
d_net_url_scheme_is_tls(
    struct d_pack_text _scheme
)
{
    const struct d_net_url_scheme_entry* entry =
        d_net_url_scheme_entry_of(_scheme);

    return ( (entry) &&
             (entry->tls) );
}

/*
d_net_url_error_name
  A short phrase for each error, for messages.
*/
const char*
d_net_url_error_name(
    enum d_net_url_error _error
)
{
    switch (_error)
    {
        case D_NET_URL_OK:
            return "no error";
        case D_NET_URL_ERROR_ARGUMENT:
            return "a NULL argument";
        case D_NET_URL_ERROR_SCHEME:
            return "malformed scheme";
        case D_NET_URL_ERROR_USERINFO:
            return "character not allowed in userinfo";
        case D_NET_URL_ERROR_HOST:
            return "character not allowed in host";
        case D_NET_URL_ERROR_IP_LITERAL:
            return "malformed IP literal";
        case D_NET_URL_ERROR_PORT:
            return "malformed port";
        case D_NET_URL_ERROR_PATH:
            return "character not allowed in path";
        case D_NET_URL_ERROR_QUERY:
            return "character not allowed in query";
        case D_NET_URL_ERROR_FRAGMENT:
            return "character not allowed in fragment";
        case D_NET_URL_ERROR_PERCENT:
            return "malformed percent escape";
        case D_NET_URL_ERROR_BASE:
            return "base is not absolute";
        case D_NET_URL_ERROR_BUFFER:
            return "buffer too small";
        default:
            return "unknown error";
    }
}

/*
d_net_url_endpoint
  The host decoded into a record-sized buffer, where a name must turn out
ASCII and an IPv6 zone's "%25" becomes "%"; then the explicit port or the
scheme's default.
*/
enum d_net_error
d_net_url_endpoint(
    const struct d_net_url* _url,
    struct d_net_endpoint*  _out
)
{
    // parameter validation
    if ( (!_url) ||
         (!_out) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    const d_net_port port = ( (_url->has_port) &&
                              (_url->port_text.length > 0u) )
                                ? _url->port
                                : d_net_url_default_port(_url->scheme);
    size_t           length = 0u;

    // the host, decoded; room for the longest a record holds
    char host[D_NET_HOST_MAX + 1] = { 0 };

    // no host, an unusable one, or no port at all
    if ( (!_url->has_authority)                          ||
         (_url->host.length == 0u)                       ||
         (_url->host_kind == D_NET_URL_HOST_IPVFUTURE)   ||
         (port == 0u)                                    ||
         (d_net_url_decode(_url->host,
                           host,
                           sizeof(host),
                           &length) != D_NET_URL_OK) )
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    // a decoded name must be plain ASCII, without NULs
    for (size_t i = 0u; i < length; ++i)
    {
        // a byte DNS cannot carry
        if ( ((unsigned char)host[i] >= 0x80u) ||
             (host[i] == '\0') )
        {
            return D_NET_ERROR_ADDRESS_INVALID;
        }
    }

    const struct d_pack_text text = { host,
                                      length };

    return d_net_endpoint_set(_out,
                              text,
                              port,
                              D_NET_PROTOCOL_TCP);
}
