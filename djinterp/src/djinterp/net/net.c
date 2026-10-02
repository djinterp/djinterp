/*******************************************************************************
* djinterp [net]                                                           net.c
*
* The net foundation's implementation.
*   Error names, port and endpoint text, the dispatch that gives every
* connection the same edges, and the stream and framing algorithms built on
* it. Nothing allocates: every buffer is the caller's, or a D_NET_IO_CHUNK
* stack buffer that lives for one call.
*
*
* path:      /src/djinterp/net/net.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "../../../inc/djinterp/net/net.h"  // corresponding header
// std
#include <stdint.h>  // SIZE_MAX
#include <string.h>  // memchr, memcmp, memcpy, memset


/*
d_net_set_count
  Stores a count through an optional pointer. The algorithms report progress
this way at every step, so the value is current however they return.
*/
static void
d_net_set_count(
    size_t* _out_count,
    size_t  _count
)
{
    // the counter is optional
    if (_out_count)
    {
        *_out_count = _count;
    }

    return;
}

/*
d_net_clear_text
  Leaves an empty string in a text buffer that has room for one: what an
all-or-nothing writer produces when its text does not fit.
*/
static void
d_net_clear_text(
    char*  _buffer,
    size_t _capacity
)
{
    // there must be room for the terminator
    if ( (_buffer) &&
         (_capacity > 0u) )
    {
        _buffer[0] = '\0';
    }

    return;
}

/*
d_net_decimal_length
  Counts the decimal digits of a value by dividing by ten until one remains.
*/
static size_t
d_net_decimal_length(
    unsigned int _value
)
{
    unsigned int rest   = _value;
    size_t       length = 1u;

    // one more digit for each further power of ten
    while (rest >= 10u)
    {
        rest   /= 10u;
        length += 1u;
    }

    return length;
}

/*
d_net_text_is_valid
  A span is well-formed when it has data or claims to have none.
*/
static bool
d_net_text_is_valid(
    struct d_pack_text _text
)
{
    return ( (_text.data != NULL) ||
             (_text.length == 0u) );
}

/*
d_net_text_contains
  Searches a span for a character. An empty span is never searched, since
its data may be NULL, which memchr does not accept even for zero bytes.
*/
static bool
d_net_text_contains(
    struct d_pack_text _text,
    char               _character
)
{
    // an empty span contains nothing
    if (_text.length == 0u)
    {
        return false;
    }

    return (memchr(_text.data,
                   _character,
                   _text.length) != NULL);
}

/*
d_net_text_slice
  Returns the part of a span from _begin up to, not including, _end. Every
caller has already established _begin <= _end <= _text.length.
*/
static struct d_pack_text
d_net_text_slice(
    struct d_pack_text _text,
    size_t             _begin,
    size_t             _end
)
{
    const struct d_pack_text slice = { _text.data + _begin, _end - _begin };

    return slice;
}

/*
d_net_split_bracketed
  Splits "[host]:port". The host ends at the first ']', and the port's colon
must follow it directly, so brackets never nest. The caller has checked that
the text opens with '['.
*/
static bool
d_net_split_bracketed(
    struct d_pack_text  _text,
    struct d_pack_text* _host,
    struct d_pack_text* _port
)
{
    const char* const close = (const char*)memchr(_text.data,
                                                  ']',
                                                  _text.length);

    // the bracket must close
    if (!close)
    {
        return false;
    }

    const size_t end = (size_t)(close - _text.data);

    // a colon must follow the bracket at once
    if ( (end + 1u >= _text.length) ||
         (_text.data[end + 1u] != ':') )
    {
        return false;
    }

    *_host = d_net_text_slice(_text,
                              1u,
                              end);
    *_port = d_net_text_slice(_text,
                              end + 2u,
                              _text.length);

    return true;
}

/*
d_net_split_plain
  Splits "host:port" at its last colon, found by scanning back from the end.
Any colon left in the host would make it an unbracketed IPv6 literal, whose
port could not be told from its last group, so that is refused.
*/
static bool
d_net_split_plain(
    struct d_pack_text  _text,
    struct d_pack_text* _host,
    struct d_pack_text* _port
)
{
    size_t cursor = _text.length;

    // step back to just past the last colon
    while ( (cursor > 0u) &&
            (_text.data[cursor - 1u] != ':') )
    {
        cursor -= 1u;
    }

    // a text without a colon has no port
    if (cursor == 0u)
    {
        return false;
    }

    *_host = d_net_text_slice(_text,
                              0u,
                              cursor - 1u);
    *_port = d_net_text_slice(_text,
                              cursor,
                              _text.length);

    return (!d_net_text_contains(*_host,
                                 ':'));
}

/*
d_net_split
  Splits either endpoint form, choosing by the first character. The caller
has checked that the text is not empty.
*/
static bool
d_net_split(
    struct d_pack_text  _text,
    struct d_pack_text* _host,
    struct d_pack_text* _port
)
{
    // a leading bracket announces an IPv6 literal
    if (_text.data[0] == '[')
    {
        return d_net_split_bracketed(_text,
                                     _host,
                                     _port);
    }

    return d_net_split_plain(_text,
                             _host,
                             _port);
}

/*
d_net_protocol_is_valid
  Compares against each enumerator, so a value cast in from an integer is
caught whatever the enum's underlying type.
*/
static bool
d_net_protocol_is_valid(
    enum d_net_protocol _protocol
)
{
    return ( (_protocol == D_NET_PROTOCOL_TCP) ||
             (_protocol == D_NET_PROTOCOL_UDP) ||
             (_protocol == D_NET_PROTOCOL_UNIX) );
}

/*
d_net_connection_is_usable
  A connection can be dispatched through when it exists and its table
supplies the four required operations.
*/
static bool
d_net_connection_is_usable(
    const struct d_net_connection* _connection
)
{
    return ( (_connection != NULL)                  &&
             (_connection->vtable != NULL)          &&
             (_connection->vtable->read != NULL)    &&
             (_connection->vtable->write != NULL)   &&
             (_connection->vtable->is_open != NULL) &&
             (_connection->vtable->close != NULL) );
}

/*
d_net_connection_query
  The common body of the two endpoint queries: _out is emptied first and
again after a backend that answers false, so that it is empty whenever the
address is unknown, however the backend left it.
*/
static bool
d_net_connection_query(
    const struct d_net_connection* _connection,
    d_net_connection_endpoint_fn   _query,
    struct d_net_endpoint*         _out
)
{
    // there must be somewhere to report the address
    if (!_out)
    {
        return false;
    }

    d_net_endpoint_init(_out);

    // an unusable connection, or one without the query, knows no address
    if ( (!d_net_connection_is_usable(_connection)) ||
         (!_query) )
    {
        return false;
    }

    const bool known = _query(_connection,
                              _out);

    // do not pass on whatever a backend left behind with its refusal
    if (!known)
    {
        d_net_endpoint_init(_out);
    }

    return known;
}

/*
d_net_error_string
  One case per enumerator and no default, so the compiler reports a missing
case; a value outside the enum falls through to the final return. The
phrases are the ones the C++ layer's to_string has always returned.
*/
const char*
d_net_error_string(
    enum d_net_error _error
)
{
    switch (_error)
    {
        case D_NET_ERROR_NONE:
            return "none";
        case D_NET_ERROR_CLOSED:
            return "closed";
        case D_NET_ERROR_WOULD_BLOCK:
            return "would block";
        case D_NET_ERROR_TIMED_OUT:
            return "timed out";
        case D_NET_ERROR_INTERRUPTED:
            return "interrupted";
        case D_NET_ERROR_CONNECTION_RESET:
            return "connection reset";
        case D_NET_ERROR_CONNECTION_REFUSED:
            return "connection refused";
        case D_NET_ERROR_CONNECTION_ABORTED:
            return "connection aborted";
        case D_NET_ERROR_ADDRESS_IN_USE:
            return "address in use";
        case D_NET_ERROR_ADDRESS_INVALID:
            return "address invalid";
        case D_NET_ERROR_HOST_UNREACHABLE:
            return "host unreachable";
        case D_NET_ERROR_NETWORK_DOWN:
            return "network down";
        case D_NET_ERROR_ACCESS_DENIED:
            return "access denied";
        case D_NET_ERROR_INVALID_ARGUMENT:
            return "invalid argument";
        case D_NET_ERROR_MESSAGE_TOO_LARGE:
            return "message too large";
        case D_NET_ERROR_TOO_MANY_OPEN_FILES:
            return "too many open files";
        case D_NET_ERROR_OUT_OF_MEMORY:
            return "out of memory";
        case D_NET_ERROR_UNKNOWN:
            return "unknown error";
    }

    return "unknown error";
}

/*
d_net_port_parse
  Accumulates digits left to right, stopping the moment the value passes
65535, so no digit count -- however many leading zeros -- can overflow it.
*/
enum d_net_error
d_net_port_parse(
    struct d_pack_text _text,
    d_net_port*        _out_port
)
{
    // parameter validation
    if ( (!_out_port) ||
         (!d_net_text_is_valid(_text)) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // an empty text is not a number
    if (_text.length == 0u)
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    unsigned long value = 0ul;

    for (size_t i = 0u; i < _text.length; ++i)
    {
        const char digit = _text.data[i];

        // only ASCII digits belong in a port
        if ( (digit < '0') ||
             (digit > '9') )
        {
            return D_NET_ERROR_ADDRESS_INVALID;
        }

        value = (value * 10ul) + (unsigned long)(digit - '0');

        // stop before the value can outgrow a port
        if (value > 65535ul)
        {
            return D_NET_ERROR_ADDRESS_INVALID;
        }
    }

    *_out_port = (d_net_port)value;

    return D_NET_ERROR_NONE;
}

/*
d_net_port_format
  Measures first, so the all-or-nothing decision is made before a byte is
written; the digits then go in from the last one back.
*/
size_t
d_net_port_format(
    d_net_port _port,
    char*      _buffer,
    size_t     _capacity
)
{
    const size_t length = d_net_decimal_length(_port);
    unsigned int rest   = _port;

    // the digits and the terminator must fit, or nothing is written
    if ( (!_buffer) ||
         (length >= _capacity) )
    {
        d_net_clear_text(_buffer,
                         _capacity);

        return length;
    }

    // write the digits from the last one back
    for (size_t i = length; i > 0u; --i)
    {
        _buffer[i - 1u] = (char)('0' + (int)(rest % 10u));
        rest           /= 10u;
    }

    _buffer[length] = '\0';

    return length;
}

/*
d_net_endpoint_init
  Zeroes the whole record, so that no byte of it is indeterminate, then names
the defaults explicitly.
*/
void
d_net_endpoint_init(
    struct d_net_endpoint* _endpoint
)
{
    // parameter validation
    if (!_endpoint)
    {
        return;
    }

    memset(_endpoint,
           0,
           sizeof(*_endpoint));
    _endpoint->port     = 0u;
    _endpoint->protocol = D_NET_PROTOCOL_TCP;

    return;
}

/*
d_net_endpoint_host
  Finds the terminator with a bounded search, so a record whose host was
written without one still yields a span inside its own storage.
*/
struct d_pack_text
d_net_endpoint_host(
    const struct d_net_endpoint* _endpoint
)
{
    // parameter validation
    if (!_endpoint)
    {
        const struct d_pack_text none = { NULL, 0u };

        return none;
    }

    const char* const end = (const char*)memchr(_endpoint->host,
                                                '\0',
                                                sizeof(_endpoint->host));

    // a missing terminator bounds the host by its storage
    const struct d_pack_text host =
    {
        _endpoint->host,
        (end) ? (size_t)(end - _endpoint->host)
              : sizeof(_endpoint->host)
    };

    return host;
}

/*
d_net_endpoint_is_unix
  A NULL endpoint is no kind of endpoint.
*/
bool
d_net_endpoint_is_unix(
    const struct d_net_endpoint* _endpoint
)
{
    return ( (_endpoint != NULL) &&
             (_endpoint->protocol == D_NET_PROTOCOL_UNIX) );
}

/*
d_net_endpoint_equal
  Compares the hosts as spans, so only the bytes before each terminator take
part and whatever follows them in the record does not.
*/
bool
d_net_endpoint_equal(
    const struct d_net_endpoint* _a,
    const struct d_net_endpoint* _b
)
{
    // an endpoint equals itself, and NULL equals NULL
    if (_a == _b)
    {
        return true;
    }

    // otherwise a NULL equals nothing
    if ( (!_a) ||
         (!_b) )
    {
        return false;
    }

    const struct d_pack_text host_a = d_net_endpoint_host(_a);
    const struct d_pack_text host_b = d_net_endpoint_host(_b);

    return ( (_a->port == _b->port)             &&
             (_a->protocol == _b->protocol)     &&
             (host_a.length == host_b.length)   &&
             (memcmp(host_a.data,
                     host_b.data,
                     host_a.length) == 0) );
}

/*
d_net_endpoint_set
  Every check runs before the record is touched, so a refused call leaves it
exactly as it was.
*/
enum d_net_error
d_net_endpoint_set(
    struct d_net_endpoint* _endpoint,
    struct d_pack_text     _host,
    d_net_port             _port,
    enum d_net_protocol    _protocol
)
{
    // parameter validation
    if ( (!_endpoint)                            ||
         (!d_net_text_is_valid(_host))           ||
         (!d_net_protocol_is_valid(_protocol)) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // the host must fit, and must not end early at an embedded terminator
    if ( (_host.length > D_NET_HOST_MAX) ||
         (d_net_text_contains(_host,
                              '\0')) )
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    // an empty host has no data to copy
    if (_host.length > 0u)
    {
        memcpy(_endpoint->host,
               _host.data,
               _host.length);
    }

    _endpoint->host[_host.length] = '\0';
    _endpoint->port               = _port;
    _endpoint->protocol           = _protocol;

    return D_NET_ERROR_NONE;
}

/*
d_net_endpoint_set_local
  Refuses an empty path itself, then defers to d_net_endpoint_set.
*/
enum d_net_error
d_net_endpoint_set_local(
    struct d_net_endpoint* _endpoint,
    struct d_pack_text     _path
)
{
    // parameter validation
    if ( (!_endpoint) ||
         (!d_net_text_is_valid(_path)) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // a unix-domain socket needs a path to live at
    if (_path.length == 0u)
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    return d_net_endpoint_set(_endpoint,
                              _path,
                              0u,
                              D_NET_PROTOCOL_UNIX);
}

/*
d_net_endpoint_parse_parts
  Splits into local spans and validates them there, so the outputs are
written only once the whole text is known to be good.
*/
enum d_net_error
d_net_endpoint_parse_parts(
    struct d_pack_text  _text,
    struct d_pack_text* _out_host,
    d_net_port*         _out_port
)
{
    // parameter validation
    if ( (!_out_host) ||
         (!_out_port) ||
         (!d_net_text_is_valid(_text)) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // an empty text is neither form
    if (_text.length == 0u)
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    struct d_pack_text host  = { NULL, 0u };
    struct d_pack_text port  = { NULL, 0u };
    d_net_port         value = 0u;

    // the text must take a form, around a host that is present and whole
    if ( (!d_net_split(_text,
                       &host,
                       &port))         ||
         (host.length == 0u)           ||
         (d_net_text_contains(host,
                              '\0')) )
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    // the port must parse, and must be one a connection can reach
    if ( (d_net_port_parse(port,
                           &value) != D_NET_ERROR_NONE) ||
         (value == 0u) )
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    *_out_host = host;
    *_out_port = value;

    return D_NET_ERROR_NONE;
}

/*
d_net_endpoint_parse
  Parses to spans, then stores them; d_net_endpoint_set leaves the record
untouched if the host is too long, so a failure at either step does too.
*/
enum d_net_error
d_net_endpoint_parse(
    struct d_pack_text     _text,
    enum d_net_protocol    _protocol,
    struct d_net_endpoint* _out
)
{
    // parameter validation: unix-domain endpoints have no text form
    if ( (!_out) ||
         (_protocol == D_NET_PROTOCOL_UNIX) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    struct d_pack_text     host   = { NULL, 0u };
    d_net_port             port   = 0u;
    const enum d_net_error parsed = d_net_endpoint_parse_parts(_text,
                                                               &host,
                                                               &port);

    // a malformed text stops here
    if (parsed != D_NET_ERROR_NONE)
    {
        return parsed;
    }

    return d_net_endpoint_set(_out,
                              host,
                              port,
                              _protocol);
}

/*
d_net_endpoint_write_text
  Writes an endpoint's text into a buffer the caller has measured it into:
the host, bracketed when it is an IPv6 literal, then a colon and the port
unless the endpoint is a path. The port's digits and terminator are known to
fit, and they are all d_net_port_format writes, so it is handed the most a
port can need rather than the room left.
*/
static void
d_net_endpoint_write_text(
    struct d_pack_text _host,
    d_net_port         _port,
    bool               _local,
    bool               _bracket,
    char*              _buffer
)
{
    size_t at = 0u;

    // the opening bracket of an IPv6 literal
    if (_bracket)
    {
        _buffer[at++] = '[';
    }

    // an empty host has no data to copy
    if (_host.length > 0u)
    {
        memcpy(_buffer + at,
               _host.data,
               _host.length);
        at += _host.length;
    }

    // the closing bracket
    if (_bracket)
    {
        _buffer[at++] = ']';
    }

    // a network endpoint ends with its port; a path ends as it is
    if (!_local)
    {
        _buffer[at++] = ':';
        at           += d_net_port_format(_port,
                                          _buffer + at,
                                          D_NET_PORT_TEXT_MAX + 1u);
    }

    _buffer[at] = '\0';

    return;
}

/*
d_net_endpoint_format_parts
  Measures the whole text before writing any of it, so the all-or-nothing
decision comes first. A host too long for the measurement to be made without
overflow cannot fit any buffer, so it reports SIZE_MAX.
*/
size_t
d_net_endpoint_format_parts(
    struct d_pack_text  _host,
    d_net_port          _port,
    enum d_net_protocol _protocol,
    char*               _buffer,
    size_t              _capacity
)
{
    const size_t host_length = (d_net_text_is_valid(_host)) ? _host.length
                                                            : 0u;

    // brackets, a colon, and five digits are the most a host gains
    if (host_length > (SIZE_MAX - 8u))
    {
        d_net_clear_text(_buffer,
                         _capacity);

        return SIZE_MAX;
    }

    const struct d_pack_text host    = { _host.data, host_length };
    const bool               local   = (_protocol == D_NET_PROTOCOL_UNIX);
    const bool               bracket = ( (!local) &&
                                         (d_net_text_contains(host,
                                                              ':')) );

    // the host, its brackets, and a colon and port unless it is a path
    const size_t length = host_length + ((bracket) ? 2u : 0u) +
                          ((local) ? 0u : (1u + d_net_decimal_length(_port)));

    // the whole text and its terminator must fit, or nothing is written
    if ( (!_buffer) ||
         (length >= _capacity) )
    {
        d_net_clear_text(_buffer,
                         _capacity);

        return length;
    }

    d_net_endpoint_write_text(host,
                              _port,
                              local,
                              bracket,
                              _buffer);

    return length;
}

/*
d_net_endpoint_format
  Formats the record's host span, so the same code serves both forms.
*/
size_t
d_net_endpoint_format(
    const struct d_net_endpoint* _endpoint,
    char*                        _buffer,
    size_t                       _capacity
)
{
    // parameter validation
    if (!_endpoint)
    {
        d_net_clear_text(_buffer,
                         _capacity);

        return 0u;
    }

    return d_net_endpoint_format_parts(d_net_endpoint_host(_endpoint),
                                       _endpoint->port,
                                       _endpoint->protocol,
                                       _buffer,
                                       _capacity);
}

/*
d_net_connection_init
  Checks the table before adopting it, so a connection is never left
pointing at an incomplete one.
*/
bool
d_net_connection_init(
    struct d_net_connection*              _connection,
    const struct d_net_connection_vtable* _vtable
)
{
    // parameter validation
    if (!_connection)
    {
        return false;
    }

    // the four required operations must all be present
    if ( (!_vtable)          ||
         (!_vtable->read)    ||
         (!_vtable->write)   ||
         (!_vtable->is_open) ||
         (!_vtable->close) )
    {
        _connection->vtable = NULL;

        return false;
    }

    _connection->vtable = _vtable;

    return true;
}

/*
d_net_connection_read
  Answers the degenerate requests itself, and holds the backend to its
contract: a count larger than the request would send the caller's loops
past the end of its buffer, so it becomes D_NET_ERROR_UNKNOWN instead.
*/
struct d_net_io_result
d_net_connection_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    // parameter validation
    if ( (!d_net_connection_is_usable(_connection)) ||
         ( (!_buffer) &&
           (_size != 0u) ) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_INVALID_ARGUMENT);
    }

    // a request for nothing is answered here
    if (_size == 0u)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_NONE);
    }

    const struct d_net_io_result result =
        _connection->vtable->read(_connection,
                                  _buffer,
                                  _size);

    // no backend may claim more than it had room for
    if (result.count > _size)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_UNKNOWN);
    }

    return result;
}

/*
d_net_connection_write
  The mirror of d_net_connection_read, with the same checks.
*/
struct d_net_io_result
d_net_connection_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    // parameter validation
    if ( (!d_net_connection_is_usable(_connection)) ||
         ( (!_data) &&
           (_size != 0u) ) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_INVALID_ARGUMENT);
    }

    // writing nothing succeeds here
    if (_size == 0u)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_NONE);
    }

    const struct d_net_io_result result =
        _connection->vtable->write(_connection,
                                   _data,
                                   _size);

    // no backend may claim more than it was given
    if (result.count > _size)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_UNKNOWN);
    }

    return result;
}

/*
d_net_connection_is_open
  An unusable connection is a closed one.
*/
bool
d_net_connection_is_open(
    const struct d_net_connection* _connection
)
{
    // parameter validation
    if (!d_net_connection_is_usable(_connection))
    {
        return false;
    }

    return _connection->vtable->is_open(_connection);
}

/*
d_net_connection_close
  Forwards to the backend, whose close must be idempotent, so no state is
kept here.
*/
void
d_net_connection_close(
    struct d_net_connection* _connection
)
{
    // an unusable connection has nothing to close
    if (d_net_connection_is_usable(_connection))
    {
        _connection->vtable->close(_connection);
    }

    return;
}

/*
d_net_connection_shutdown
  Validates the mode itself, so a backend only ever sees the three values.
*/
enum d_net_error
d_net_connection_shutdown(
    struct d_net_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    // parameter validation
    if ( (!d_net_connection_is_usable(_connection)) ||
         ( (_mode != D_NET_SHUTDOWN_READ)  &&
           (_mode != D_NET_SHUTDOWN_WRITE) &&
           (_mode != D_NET_SHUTDOWN_BOTH) ) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // a backend without a half-close treats the request as done
    if (!_connection->vtable->shutdown)
    {
        return D_NET_ERROR_NONE;
    }

    return _connection->vtable->shutdown(_connection,
                                         _mode);
}

/*
d_net_connection_remote_endpoint
  See d_net_connection_query.
*/
bool
d_net_connection_remote_endpoint(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    // parameter validation happens in the query
    if (!d_net_connection_is_usable(_connection))
    {
        return d_net_connection_query(NULL,
                                      NULL,
                                      _out);
    }

    return d_net_connection_query(_connection,
                                  _connection->vtable->remote_endpoint,
                                  _out);
}

/*
d_net_connection_local_endpoint
  See d_net_connection_query.
*/
bool
d_net_connection_local_endpoint(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    // parameter validation happens in the query
    if (!d_net_connection_is_usable(_connection))
    {
        return d_net_connection_query(NULL,
                                      NULL,
                                      _out);
    }

    return d_net_connection_query(_connection,
                                  _connection->vtable->local_endpoint,
                                  _out);
}

/*
d_net_connection_destroy
  Closes unconditionally, since close is idempotent, and then hands the
storage to the backend's destroy, if it has one.
*/
void
d_net_connection_destroy(
    struct d_net_connection* _connection
)
{
    // an unusable connection has nothing to close or release
    if (!d_net_connection_is_usable(_connection))
    {
        return;
    }

    _connection->vtable->close(_connection);

    // only a backend that allocated the connection releases it
    if (_connection->vtable->destroy)
    {
        _connection->vtable->destroy(_connection);
    }

    return;
}

/*
d_net_read_exactly
  Counts each read's bytes before looking at its error, so bytes that arrived
alongside a failure are reported rather than lost.
*/
enum d_net_error
d_net_read_exactly(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size,
    size_t*                  _out_read
)
{
    d_net_set_count(_out_read,
                    0u);

    // parameter validation
    if ( (!_connection) ||
         ( (!_buffer) &&
           (_size != 0u) ) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    unsigned char* const out  = (unsigned char*)_buffer;
    size_t               done = 0u;

    // read until the request is met
    while (done < _size)
    {
        const struct d_net_io_result result =
            d_net_connection_read(_connection,
                                  out + done,
                                  _size - done);

        done += result.count;
        d_net_set_count(_out_read,
                        done);

        // a failure ends the read
        if (result.error != D_NET_ERROR_NONE)
        {
            return result.error;
        }

        // so does the end of the stream, before the request was met
        if (result.count == 0u)
        {
            return D_NET_ERROR_CLOSED;
        }
    }

    return D_NET_ERROR_NONE;
}

/*
d_net_write_all
  The mirror of d_net_read_exactly. A write that takes nothing without
reporting why would repeat forever, so it ends the loop as CLOSED.
*/
enum d_net_error
d_net_write_all(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size,
    size_t*                  _out_written
)
{
    d_net_set_count(_out_written,
                    0u);

    // parameter validation
    if ( (!_connection) ||
         ( (!_data) &&
           (_size != 0u) ) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    const unsigned char* const in   = (const unsigned char*)_data;
    size_t                     done = 0u;

    // write until everything is taken
    while (done < _size)
    {
        const struct d_net_io_result result =
            d_net_connection_write(_connection,
                                   in + done,
                                   _size - done);

        done += result.count;
        d_net_set_count(_out_written,
                        done);

        // a failure ends the write
        if (result.error != D_NET_ERROR_NONE)
        {
            return result.error;
        }

        // so does a connection that takes nothing
        if (result.count == 0u)
        {
            return D_NET_ERROR_CLOSED;
        }
    }

    return D_NET_ERROR_NONE;
}

/*
d_net_read_all
  Hands each chunk to the sink before looking at the read's error, as
d_net_read_exactly counts before it looks.
*/
enum d_net_error
d_net_read_all(
    struct d_net_connection* _connection,
    struct d_pack_sink       _sink,
    size_t*                  _out_total
)
{
    d_net_set_count(_out_total,
                    0u);

    // parameter validation
    if ( (!_connection) ||
         (!_sink.write) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    unsigned char chunk[D_NET_IO_CHUNK] = { 0 };
    size_t        total                 = 0u;

    // read until the stream ends or fails
    for (;;)
    {
        const struct d_net_io_result result =
            d_net_connection_read(_connection,
                                  chunk,
                                  sizeof(chunk));

        // a sink that takes less than it is given is full
        if ( (result.count > 0u) &&
             (_sink.write(_sink.context,
                          chunk,
                          result.count) != result.count) )
        {
            return D_NET_ERROR_MESSAGE_TOO_LARGE;
        }

        total += result.count;
        d_net_set_count(_out_total,
                        total);

        // a failure, or the end of the stream, ends the read
        if ( (result.error != D_NET_ERROR_NONE) ||
             (result.count == 0u) )
        {
            return result.error;
        }
    }
}

/*
d_net_pump
  Reads a chunk from the source and writes all of it to the sink, counting
what the sink took even when its write fails partway.
*/
enum d_net_error
d_net_pump(
    struct d_net_connection* _source,
    struct d_net_connection* _sink,
    size_t*                  _out_total
)
{
    d_net_set_count(_out_total,
                    0u);

    // parameter validation
    if ( (!_source) ||
         (!_sink) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    unsigned char chunk[D_NET_IO_CHUNK] = { 0 };
    size_t        total                 = 0u;

    // copy until the source ends or either side fails
    for (;;)
    {
        const struct d_net_io_result result =
            d_net_connection_read(_source,
                                  chunk,
                                  sizeof(chunk));

        // pass on whatever arrived, even alongside an error
        size_t                 written = 0u;
        const enum d_net_error sent    = d_net_write_all(_sink,
                                                         chunk,
                                                         result.count,
                                                         &written);

        total += written;
        d_net_set_count(_out_total,
                        total);

        // the sink's failure takes precedence: its bytes are the ones lost
        if (sent != D_NET_ERROR_NONE)
        {
            return sent;
        }

        // a failure, or the end of the source, ends the copy
        if ( (result.error != D_NET_ERROR_NONE) ||
             (result.count == 0u) )
        {
            return result.error;
        }
    }
}

/*
d_net_write_frame
  Builds a small frame whole in a stack buffer and sends it with one write;
a larger one goes out as its header, then its payload in place, so the
payload is never copied.
*/
enum d_net_error
d_net_write_frame(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size,
    size_t                   _max
)
{
    // parameter validation
    if ( (!_connection) ||
         ( (!_data) &&
           (_size != 0u) ) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    const enum d_net_error checked = d_net_frame_check(_size,
                                                       _max);

    // an oversized frame writes nothing
    if (checked != D_NET_ERROR_NONE)
    {
        return checked;
    }

    unsigned char frame[D_NET_IO_CHUNK] = { 0 };

    d_net_frame_header_encode((uint32_t)_size,
                              frame);

    // a frame that fits the buffer goes out in one write
    if (_size <= (sizeof(frame) - D_NET_FRAME_HEADER_SIZE))
    {
        // an empty payload has no data to copy
        if (_size > 0u)
        {
            memcpy(frame + D_NET_FRAME_HEADER_SIZE,
                   _data,
                   _size);
        }

        return d_net_write_all(_connection,
                               frame,
                               D_NET_FRAME_HEADER_SIZE + _size,
                               NULL);
    }

    const enum d_net_error header = d_net_write_all(_connection,
                                                    frame,
                                                    D_NET_FRAME_HEADER_SIZE,
                                                    NULL);

    // the payload follows only a header that went out whole
    if (header != D_NET_ERROR_NONE)
    {
        return header;
    }

    return d_net_write_all(_connection,
                           _data,
                           _size,
                           NULL);
}

/*
d_net_read_frame
  Reads the header, refuses a length the buffer cannot hold before reading
any of the payload, then reads the payload straight into the buffer.
*/
enum d_net_error
d_net_read_frame(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _capacity,
    size_t*                  _out_length
)
{
    d_net_set_count(_out_length,
                    0u);

    // parameter validation
    if ( (!_connection) ||
         ( (!_buffer) &&
           (_capacity != 0u) ) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    unsigned char header[D_NET_FRAME_HEADER_SIZE] = { 0 };

    // the header comes first, and whole
    const enum d_net_error head = d_net_read_exactly(_connection,
                                                     header,
                                                     sizeof(header),
                                                     NULL);

    // without a whole header there is no frame
    if (head != D_NET_ERROR_NONE)
    {
        return head;
    }

    const size_t length = (size_t)d_net_frame_header_decode(header);

    d_net_set_count(_out_length,
                    length);

    // the buffer's capacity is the ceiling
    if (length > _capacity)
    {
        return D_NET_ERROR_MESSAGE_TOO_LARGE;
    }

    return d_net_read_exactly(_connection,
                              _buffer,
                              length,
                              NULL);
}

/*
d_net_read_to_sink
  Moves exactly _length bytes from a connection into a sink through a stack
buffer, one chunk-sized exact read at a time.
*/
static enum d_net_error
d_net_read_to_sink(
    struct d_net_connection* _connection,
    struct d_pack_sink       _sink,
    size_t                   _length
)
{
    unsigned char chunk[D_NET_IO_CHUNK] = { 0 };
    size_t        remaining             = _length;

    // deliver the payload a chunk at a time
    while (remaining > 0u)
    {
        const size_t           want = (remaining < sizeof(chunk))
                                          ? remaining
                                          : sizeof(chunk);
        const enum d_net_error part = d_net_read_exactly(_connection,
                                                         chunk,
                                                         want,
                                                         NULL);

        // a short or failed read ends the frame
        if (part != D_NET_ERROR_NONE)
        {
            return part;
        }

        // a sink that takes less than it is given is full
        if (_sink.write(_sink.context,
                        chunk,
                        want) != want)
        {
            return D_NET_ERROR_MESSAGE_TOO_LARGE;
        }

        remaining -= want;
    }

    return D_NET_ERROR_NONE;
}

/*
d_net_read_frame_sink
  Reads the header, refuses a length above the ceiling before any of the
payload is read, then hands the payload to d_net_read_to_sink.
*/
enum d_net_error
d_net_read_frame_sink(
    struct d_net_connection* _connection,
    struct d_pack_sink       _sink,
    size_t                   _max,
    size_t*                  _out_length
)
{
    d_net_set_count(_out_length,
                    0u);

    // parameter validation
    if ( (!_connection) ||
         (!_sink.write) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    unsigned char header[D_NET_FRAME_HEADER_SIZE] = { 0 };

    // the header comes first, and whole
    const enum d_net_error head = d_net_read_exactly(_connection,
                                                     header,
                                                     sizeof(header),
                                                     NULL);

    // without a whole header there is no frame
    if (head != D_NET_ERROR_NONE)
    {
        return head;
    }

    const size_t length = (size_t)d_net_frame_header_decode(header);

    d_net_set_count(_out_length,
                    length);

    // a length above the ceiling is refused before its payload is read
    if (length > _max)
    {
        return D_NET_ERROR_MESSAGE_TOO_LARGE;
    }

    return d_net_read_to_sink(_connection,
                              _sink,
                              length);
}
