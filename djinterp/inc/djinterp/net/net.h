/*******************************************************************************
* djinterp [net]                                                           net.h
*
* The net foundation: what every networking module shares.
*   Library-agnostic C, with no OS or socket code of its own. Transports live
* in the modules built on it: a socket backend, a TLS layer, or a test double
* each implements the connection interface declared here, and everything
* above them is written against that interface alone. It supplies
*     - the version, and the sizes cfg_net.h fixes                          [1]
*     - the error taxonomy every backend maps its native codes onto, the
*       transport vocabulary, endpoints, and connections, as types       [2, 3]
*     - ports and endpoints: records, parsing, and formatting              [4]
*     - the calls that dispatch through a connection's operations          [5]
*     - stream algorithms over any connection                              [6]
*     - length-prefixed framing, and the codec its C and C++ forms share   [7]
*   Nothing here allocates or blocks on its own: parsers return spans into
* caller memory, writers fill caller buffers, and every byte moves through a
* connection the caller supplies.
*   The C++ layer, net/net.hpp, is built on this header rather than beside it:
* its errors, results, and endpoint text handling are these, and its generic
* algorithms share the framing codec. Text spans are d_pack_text and sinks
* d_pack_sink, from sink_common.h. Build configuration comes from cfg_net.h;
* environment detection for the backends from env/net/env_net.h.
*   Every declaration has C linkage. C99 or later, with no extensions. bool
* comes from the framework root, which includes <stdbool.h> only where C needs
* it: in C++ that header defines a _Bool macro.
*
*
* path:      /inc/djinterp/net/net.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Version
         1.  D_NET_VERSION_MAJOR
         2.  D_NET_VERSION_MINOR
         3.  D_NET_VERSION_PATCH
         4.  D_NET_VERSION_STRING
    2.  Addresses
         1.  D_NET_HOST_MAX
         2.  D_NET_PORT_TEXT_MAX
         3.  D_NET_ENDPOINT_TEXT_MAX
    3.  Transfers
         1.  D_NET_IO_CHUNK
    4.  Framing
         1.  D_NET_FRAME_HEADER_SIZE
         2.  D_NET_FRAME_LENGTH_MAX
         3.  D_NET_FRAME_MAX
2.  VOCABULARY
    ----------
    1.  Scalars
         1.  d_net_port
    2.  Errors
         1.  d_net_error
    3.  Transport vocabulary
         1.  d_net_shutdown
         2.  d_net_protocol
    4.  Results
         1.  d_net_io_result
    5.  Endpoints
         1.  d_net_endpoint
3.  CONNECTIONS
    -----------
    1.  Operations
         1.  d_net_connection_read_fn
         2.  d_net_connection_write_fn
         3.  d_net_connection_is_open_fn
         4.  d_net_connection_close_fn
         5.  d_net_connection_shutdown_fn
         6.  d_net_connection_endpoint_fn
         7.  d_net_connection_destroy_fn
    2.  The connection
         1.  d_net_connection_vtable
         2.  d_net_connection
4.  ERRORS, RESULTS, AND ADDRESSES
    ------------------------------
    1.  Errors
    2.  Results
    3.  Ports
    4.  Endpoint records
    5.  Endpoint text
5.  CONNECTION OPERATIONS
    ---------------------
    1.  Setup
    2.  Transfers
    3.  State and lifetime
6.  STREAM ALGORITHMS
    -----------------
    1.  Exact transfers
    2.  Bulk transfers
7.  FRAMING
    -------
    1.  The frame header
    2.  Framed transfers
*/

#ifndef DJINTERP_NET_NET_H
#define DJINTERP_NET_NET_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint16_t, uint32_t
// djinterp
#include "../c/djinterp.h"          // framework root
#include "../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../config/net/cfg_net.h"  // D_INTERNAL_NET_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================
// The version, and the sizes cfg_net.h resolves, republished as size_t
// constants. A caller sizes its own buffers with these.


// 1.1    Version
//------------------------------------------------------------------------------
// 1.1.1
// D_NET_VERSION_MAJOR
//   constant: the major version of the net subframework.
#define D_NET_VERSION_MAJOR     0

// 1.1.2
// D_NET_VERSION_MINOR
//   constant: the minor version; 0.2 adds the C foundation beneath net.hpp.
#define D_NET_VERSION_MINOR     2

// 1.1.3
// D_NET_VERSION_PATCH
//   constant: the patch version.
#define D_NET_VERSION_PATCH     0

// 1.1.4
// D_NET_VERSION_STRING
//   constant: the version as text.
#define D_NET_VERSION_STRING    "0.2.0"

// 1.2    Addresses
//------------------------------------------------------------------------------
// 1.2.1
// D_NET_HOST_MAX
//   constant: the longest host a d_net_endpoint holds, terminator excluded
// (cfg_net.h).
#define D_NET_HOST_MAX          ((size_t)D_INTERNAL_NET_HOST_MAX)

// 1.2.2
// D_NET_PORT_TEXT_MAX
//   constant: the most digits a port takes in decimal: 65535.
#define D_NET_PORT_TEXT_MAX     ((size_t)5u)

// 1.2.3
// D_NET_ENDPOINT_TEXT_MAX
//   constant: the longest text d_net_endpoint_format writes, terminator
// excluded: a bracketed host, a colon, and a port.
#define D_NET_ENDPOINT_TEXT_MAX (D_NET_HOST_MAX + (size_t)8u)

// 1.3    Transfers
//------------------------------------------------------------------------------
// 1.3.1
// D_NET_IO_CHUNK
//   constant: the size of the stack buffer the stream algorithms move data
// through, and the most one of their reads asks for (cfg_net.h).
#define D_NET_IO_CHUNK          ((size_t)D_INTERNAL_NET_IO_CHUNK)

// 1.4    Framing
//------------------------------------------------------------------------------
// 1.4.1
// D_NET_FRAME_HEADER_SIZE
//   constant: the size of a frame header, a big-endian 32-bit length.
#define D_NET_FRAME_HEADER_SIZE ((size_t)4u)

// 1.4.2
// D_NET_FRAME_LENGTH_MAX
//   constant: the longest payload a frame header can declare, 2^32 - 1.
#define D_NET_FRAME_LENGTH_MAX  0xFFFFFFFFul

// 1.4.3
// D_NET_FRAME_MAX
//   constant: the default ceiling on a framed payload (cfg_net.h).
#define D_NET_FRAME_MAX         ((size_t)D_INTERNAL_NET_FRAME_MAX)


//==============================================================================
// 2.  VOCABULARY
//==============================================================================
// Plain data with nothing to destroy. Every enumeration's values are pinned:
// the C++ layer defines its own enumerators as these, and backends store
// them.


// 2.1    Scalars
//------------------------------------------------------------------------------
// 2.1.1
// d_net_port
//   typedef: a TCP or UDP port number, in host byte order.
typedef uint16_t d_net_port;

// 2.2    Errors
//------------------------------------------------------------------------------
// 2.2.1
// d_net_error
//   enum: the library-neutral classification of transport failures. Every
// backend maps its native codes -- errno, WSAGetLastError, a TLS library's
// errors -- onto it, so the layers above reason about failure portably.
// D_NET_ERROR_NONE is success, and D_NET_ERROR_WOULD_BLOCK serves
// non-blocking backends.
enum d_net_error
{
    D_NET_ERROR_NONE                = 0,   // success
    D_NET_ERROR_CLOSED              = 1,   // the stream ended too soon
    D_NET_ERROR_WOULD_BLOCK         = 2,   // a non-blocking call cannot proceed
    D_NET_ERROR_TIMED_OUT           = 3,   // an operation outlived its deadline
    D_NET_ERROR_INTERRUPTED         = 4,   // a signal interrupted the call
    D_NET_ERROR_CONNECTION_RESET    = 5,   // the peer reset the connection
    D_NET_ERROR_CONNECTION_REFUSED  = 6,   // nothing listens at the target
    D_NET_ERROR_CONNECTION_ABORTED  = 7,   // aborted on this side
    D_NET_ERROR_ADDRESS_IN_USE      = 8,   // the bind address is taken
    D_NET_ERROR_ADDRESS_INVALID     = 9,   // an address is malformed
    D_NET_ERROR_HOST_UNREACHABLE    = 10,  // no route to the host
    D_NET_ERROR_NETWORK_DOWN        = 11,  // the local network is down
    D_NET_ERROR_ACCESS_DENIED       = 12,  // permission was refused
    D_NET_ERROR_INVALID_ARGUMENT    = 13,  // the caller misused the call
    D_NET_ERROR_MESSAGE_TOO_LARGE   = 14,  // a message exceeds its ceiling
    D_NET_ERROR_TOO_MANY_OPEN_FILES = 15,  // a descriptor limit was reached
    D_NET_ERROR_OUT_OF_MEMORY       = 16,  // an allocation failed
    D_NET_ERROR_UNKNOWN             = 17   // anything else
};

// 2.3    Transport vocabulary
//------------------------------------------------------------------------------
// 2.3.1
// d_net_shutdown
//   enum: which direction of a duplex connection a shutdown closes.
enum d_net_shutdown
{
    D_NET_SHUTDOWN_READ  = 0,
    D_NET_SHUTDOWN_WRITE = 1,
    D_NET_SHUTDOWN_BOTH  = 2
};

// 2.3.2
// d_net_protocol
//   enum: the transport an endpoint or connection uses.
enum d_net_protocol
{
    D_NET_PROTOCOL_TCP  = 0,
    D_NET_PROTOCOL_UDP  = 1,
    D_NET_PROTOCOL_UNIX = 2   // a unix-domain stream socket
};

// 2.4    Results
//------------------------------------------------------------------------------
// 2.4.1
// d_net_io_result
//   struct: the outcome of one read or write: how many bytes moved, and any
// error. A count of 0 with D_NET_ERROR_NONE is a clean end of stream. A
// nonzero count may accompany an error, when bytes moved before it struck.
struct d_net_io_result
{
    size_t           count;  // bytes transferred
    enum d_net_error error;  // D_NET_ERROR_NONE on success
};

// 2.5    Endpoints
//------------------------------------------------------------------------------
// 2.5.1
// d_net_endpoint
//   struct: a transport address that owns its text: a host, a port, and a
// protocol. `host` is a name or a numeric literal, resolved by a backend,
// never here; for D_NET_PROTOCOL_UNIX it is a filesystem path and `port` is
// 0. Build one with d_net_endpoint_set or d_net_endpoint_parse, which keep
// `host` NUL-terminated.
struct d_net_endpoint
{
    char                host[D_NET_HOST_MAX + 1];  // NUL-terminated
    d_net_port          port;                      // host byte order
    enum d_net_protocol protocol;                  // the transport
};


//==============================================================================
// 3.  CONNECTIONS
//==============================================================================
// The live, polymorphic I/O object every backend produces. A backend defines
// a struct whose first member is a struct d_net_connection, and a table of
// operations that struct's functions fill in; a pointer to the backend's
// struct then converts to a pointer to its first member and back. Callers
// reach a connection only through section 5, which checks arguments, fills
// in the optional operations' defaults, and so gives every backend the same
// edges.
//   Required: read, write, is_open, close. Optional, and NULL for the
// default: shutdown (does nothing, and succeeds), the two endpoint queries
// (report the address unknown), and destroy (the storage belongs to someone
// else).


// 3.1    Operations
//------------------------------------------------------------------------------
// each operation receives the connection it belongs to, defined in 3.2.2
struct d_net_connection;

// 3.1.1
// d_net_connection_read_fn
//   function pointer: reads up to _size (at least 1) bytes into _buffer. A
// blocking backend waits until at least one byte arrives, the stream ends,
// or an error strikes. A count of 0 with D_NET_ERROR_NONE is the end of the
// stream; the count may never exceed _size.
typedef struct d_net_io_result (*d_net_connection_read_fn)(
                                   struct d_net_connection* _connection,
                                   void*                    _buffer,
                                   size_t                   _size);

// 3.1.2
// d_net_connection_write_fn
//   function pointer: writes up to _size (at least 1) bytes from _data. A
// short count is not an error; d_net_write_all insists on the whole buffer.
typedef struct d_net_io_result (*d_net_connection_write_fn)(
                                   struct d_net_connection* _connection,
                                   const void*              _data,
                                   size_t                   _size);

// 3.1.3
// d_net_connection_is_open_fn
//   function pointer: whether the connection is still usable.
typedef bool (*d_net_connection_is_open_fn)(
                 const struct d_net_connection* _connection);

// 3.1.4
// d_net_connection_close_fn
//   function pointer: releases the underlying resource. Must be idempotent.
typedef void (*d_net_connection_close_fn)(
                 struct d_net_connection* _connection);

// 3.1.5
// d_net_connection_shutdown_fn
//   function pointer: closes one or both directions while leaving the
// resource open -- a half-close.
typedef enum d_net_error (*d_net_connection_shutdown_fn)(
                             struct d_net_connection* _connection,
                             enum d_net_shutdown      _mode);

// 3.1.6
// d_net_connection_endpoint_fn
//   function pointer: fills _out with the remote or local address and
// returns true, or returns false when the address is unknown.
typedef bool (*d_net_connection_endpoint_fn)(
                 const struct d_net_connection* _connection,
                 struct d_net_endpoint*         _out);

// 3.1.7
// d_net_connection_destroy_fn
//   function pointer: releases the connection's own storage, after it has
// been closed. Only a backend that allocated the connection supplies one.
typedef void (*d_net_connection_destroy_fn)(
                 struct d_net_connection* _connection);

// 3.2    The connection
//------------------------------------------------------------------------------
// 3.2.1
// d_net_connection_vtable
//   struct: a backend's operations. A backend defines one, usually static
// and const, and every connection it produces points at it.
struct d_net_connection_vtable
{
    d_net_connection_read_fn     read;             // required
    d_net_connection_write_fn    write;            // required
    d_net_connection_is_open_fn  is_open;          // required
    d_net_connection_close_fn    close;            // required
    d_net_connection_shutdown_fn shutdown;         // optional
    d_net_connection_endpoint_fn remote_endpoint;  // optional
    d_net_connection_endpoint_fn local_endpoint;   // optional
    d_net_connection_destroy_fn  destroy;          // optional
};

// 3.2.2
// d_net_connection
//   struct: the head of every connection: its table of operations. It must
// be the first member of the backend's own struct.
struct d_net_connection
{
    const struct d_net_connection_vtable* vtable;
};


//==============================================================================
// 4.  ERRORS, RESULTS, AND ADDRESSES
//==============================================================================
// Pure functions over the vocabulary: none performs I/O. The endpoint text
// functions come in two forms. The record forms read and fill a
// d_net_endpoint; the _parts forms work on spans, so the C++ layer parses and
// formats its std::string endpoints through the same code without copying.


// 4.1    Errors
//------------------------------------------------------------------------------
/**
 * @brief Describes an error in a short, static, English phrase.
 *
 * @param[in] _error  the error.
 * @return the phrase, such as "connection reset"; "unknown error" for a
 *         value outside the enum. Never NULL.
 */
const char* d_net_error_string(enum d_net_error _error);

// 4.2    Results
//------------------------------------------------------------------------------
/**
 * @brief Builds a result from its two fields.
 *
 * @param[in] _count  the bytes transferred.
 * @param[in] _error  the error, or D_NET_ERROR_NONE.
 * @return the result.
 */
D_STATIC_INLINE struct d_net_io_result
d_net_io_result_make(
    size_t           _count,
    enum d_net_error _error
)
{
    const struct d_net_io_result result = { _count, _error };

    return result;
}

/**
 * @brief Tells whether a result carries no error; end of stream is none.
 *
 * @param[in] _result  the result.
 * @return true if `_result.error` is D_NET_ERROR_NONE.
 */
D_STATIC_INLINE bool
d_net_io_result_ok(
    struct d_net_io_result _result
)
{
    return (_result.error == D_NET_ERROR_NONE);
}

/**
 * @brief Tells whether a result is a clean end of stream.
 *
 * @param[in] _result  the result.
 * @return true if nothing moved and nothing failed.
 */
D_STATIC_INLINE bool
d_net_io_result_eof(
    struct d_net_io_result _result
)
{
    return ( (_result.count == 0u) &&
             (_result.error == D_NET_ERROR_NONE) );
}

// 4.3    Ports
//------------------------------------------------------------------------------
/**
 * @brief Parses a decimal port number.
 *
 * Only ASCII digits are accepted -- no sign, space, or prefix -- and leading
 * zeros are allowed. The value may be 0, which asks a backend for an
 * ephemeral port; d_net_endpoint_parse_parts, which needs a port to reach,
 * refuses it.
 *
 * @param[in]  _text      the digits.
 * @param[out] _out_port  receives the port; unchanged on failure.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_ADDRESS_INVALID for an empty text, a
 *         non-digit, or a value above 65535; or
 *         D_NET_ERROR_INVALID_ARGUMENT for a NULL `_out_port`, or a NULL
 *         text with a nonzero length.
 */
enum d_net_error d_net_port_parse(struct d_pack_text _text,
                                  d_net_port*        _out_port);
/**
 * @brief Writes a port in decimal.
 *
 * The write is all or nothing: the digits and a terminator when they fit,
 * and otherwise an empty string, when there is room for one.
 *
 * @param[in]  _port      the port.
 * @param[out] _buffer    receives the text; may be NULL to measure.
 * @param[in]  _capacity  the size of `_buffer` in bytes.
 * @return the length of the text, terminator excluded, whether or not it
 *         was written; it fit if the value is below `_capacity`.
 */
size_t           d_net_port_format(d_net_port _port,
                                   char*      _buffer,
                                   size_t     _capacity);

// 4.4    Endpoint records
//------------------------------------------------------------------------------
// d_net_endpoint_init empties an endpoint: no host, port 0, TCP; a NULL
// endpoint is ignored. d_net_endpoint_host returns a span over the stored
// host, and an empty span for NULL. d_net_endpoint_is_unix tells whether the
// protocol is D_NET_PROTOCOL_UNIX; d_net_endpoint_equal compares all three
// fields, and is true of two NULLs.
void               d_net_endpoint_init(struct d_net_endpoint* _endpoint);
struct d_pack_text d_net_endpoint_host(const struct d_net_endpoint* _endpoint);
bool               d_net_endpoint_is_unix(
                       const struct d_net_endpoint* _endpoint);
bool               d_net_endpoint_equal(const struct d_net_endpoint* _a,
                                        const struct d_net_endpoint* _b);
/**
 * @brief Stores a host, a port, and a protocol in an endpoint.
 *
 * The host is copied and terminated. It may be empty, which a backend reads
 * as the wildcard address when binding.
 *
 * @param[out] _endpoint  the endpoint; unchanged on failure.
 * @param[in]  _host      the host: a name, a literal, or a path.
 * @param[in]  _port      the port; 0 for a unix-domain socket.
 * @param[in]  _protocol  the transport.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_ADDRESS_INVALID for a host longer
 *         than D_NET_HOST_MAX or holding a NUL; or
 *         D_NET_ERROR_INVALID_ARGUMENT for a NULL `_endpoint`, a NULL host
 *         with a nonzero length, or an unknown protocol.
 */
enum d_net_error   d_net_endpoint_set(struct d_net_endpoint* _endpoint,
                                      struct d_pack_text     _host,
                                      d_net_port             _port,
                                      enum d_net_protocol    _protocol);
/**
 * @brief Stores a unix-domain socket path in an endpoint.
 *
 * @param[out] _endpoint  the endpoint; unchanged on failure.
 * @param[in]  _path      the filesystem path; not empty.
 * @return what d_net_endpoint_set returns for the path, port 0, and
 *         D_NET_PROTOCOL_UNIX, except that an empty path is
 *         D_NET_ERROR_ADDRESS_INVALID.
 */
enum d_net_error   d_net_endpoint_set_local(struct d_net_endpoint* _endpoint,
                                            struct d_pack_text     _path);

// 4.5    Endpoint text
//------------------------------------------------------------------------------
/**
 * @brief Splits "host:port" or "[host]:port" into its parts, without copying.
 *
 * A host containing a colon -- an IPv6 literal -- must be bracketed, and the
 * port must follow the closing bracket directly. An unbracketed host splits
 * at its only colon. Neither form may leave the host empty or give port 0.
 * Unix-domain endpoints have no text form; build them with
 * d_net_endpoint_set_local.
 *
 * @param[in]  _text      the text.
 * @param[out] _out_host  receives the host, pointing into `_text`, brackets
 *                        excluded; unchanged on failure.
 * @param[out] _out_port  receives the port; unchanged on failure.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_ADDRESS_INVALID for text of neither
 *         form, an empty host, a NUL in the host, or a port that
 *         d_net_port_parse refuses or that is 0; or
 *         D_NET_ERROR_INVALID_ARGUMENT for a NULL output, or a NULL text
 *         with a nonzero length.
 */
enum d_net_error d_net_endpoint_parse_parts(struct d_pack_text  _text,
                                            struct d_pack_text* _out_host,
                                            d_net_port*         _out_port);
/**
 * @brief Parses "host:port" or "[host]:port" into an endpoint.
 *
 * @param[in]  _text      the text, as d_net_endpoint_parse_parts reads it.
 * @param[in]  _protocol  the protocol to record; not D_NET_PROTOCOL_UNIX.
 * @param[out] _out       the endpoint; unchanged on failure.
 * @return what d_net_endpoint_parse_parts returns, or then what
 *         d_net_endpoint_set returns; or D_NET_ERROR_INVALID_ARGUMENT for
 *         D_NET_PROTOCOL_UNIX.
 */
enum d_net_error d_net_endpoint_parse(struct d_pack_text     _text,
                                      enum d_net_protocol    _protocol,
                                      struct d_net_endpoint* _out);
/**
 * @brief Writes an endpoint's parts as text.
 *
 * A unix-domain endpoint is written as its path. Any other is written as
 * "host:port", or as "[host]:port" when the host contains a colon, so that
 * the text parses back to the same parts. The write is all or nothing, as
 * d_net_port_format's is.
 *
 * @param[in]  _host      the host; a NULL span counts as empty.
 * @param[in]  _port      the port; ignored for a unix-domain endpoint.
 * @param[in]  _protocol  the protocol.
 * @param[out] _buffer    receives the text; may be NULL to measure.
 * @param[in]  _capacity  the size of `_buffer` in bytes.
 * @return the length of the text, terminator excluded, whether or not it
 *         was written; SIZE_MAX for a host too long to measure.
 */
size_t           d_net_endpoint_format_parts(struct d_pack_text  _host,
                                             d_net_port          _port,
                                             enum d_net_protocol _protocol,
                                             char*               _buffer,
                                             size_t              _capacity);
/**
 * @brief Writes an endpoint as text, as d_net_endpoint_format_parts does.
 *
 * A buffer of D_NET_ENDPOINT_TEXT_MAX + 1 bytes always suffices.
 *
 * @param[in]  _endpoint  the endpoint; NULL writes an empty string.
 * @param[out] _buffer    receives the text; may be NULL to measure.
 * @param[in]  _capacity  the size of `_buffer` in bytes.
 * @return the length of the text, terminator excluded.
 */
size_t           d_net_endpoint_format(const struct d_net_endpoint* _endpoint,
                                       char*                        _buffer,
                                       size_t                       _capacity);


//==============================================================================
// 5.  CONNECTION OPERATIONS
//==============================================================================
// How callers reach a connection. Every call accepts NULL, and a connection
// whose table lacks a required operation: they fail the way a closed
// connection would, never by crashing.


// 5.1    Setup
//------------------------------------------------------------------------------
/**
 * @brief Points a connection at its table of operations.
 *
 * @param[out] _connection  the connection's head, inside the backend's
 *                          struct.
 * @param[in]  _vtable      the table; it must outlive the connection.
 * @return true if `_vtable` supplies the four required operations; false,
 *         leaving the connection's table NULL, if it does not or either
 *         argument is NULL.
 */
bool d_net_connection_init(struct d_net_connection*              _connection,
                           const struct d_net_connection_vtable* _vtable);

// 5.2    Transfers
//------------------------------------------------------------------------------
/**
 * @brief Reads up to `_size` bytes from a connection.
 *
 * A request for zero bytes returns a count of 0 and D_NET_ERROR_NONE without
 * reaching the backend -- the shape of an end of stream -- so ask for at
 * least one byte whenever the end of the stream matters.
 *
 * @param[in,out] _connection  the connection.
 * @param[out]    _buffer      receives the bytes.
 * @param[in]     _size        its size in bytes.
 * @return the bytes read and any error; D_NET_ERROR_INVALID_ARGUMENT for a
 *         NULL or incomplete connection, or a NULL `_buffer` with a nonzero
 *         `_size`; D_NET_ERROR_UNKNOWN if the backend claims more bytes
 *         than it had room for.
 */
struct d_net_io_result d_net_connection_read(
                           struct d_net_connection* _connection,
                           void*                    _buffer,
                           size_t                   _size);
/**
 * @brief Writes up to `_size` bytes to a connection.
 *
 * A short count is not an error; see d_net_write_all. Writing zero bytes
 * succeeds without reaching the backend.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _data        the bytes.
 * @param[in]     _size        how many.
 * @return the bytes written and any error, with the failures of
 *         d_net_connection_read.
 */
struct d_net_io_result d_net_connection_write(
                           struct d_net_connection* _connection,
                           const void*              _data,
                           size_t                   _size);

// 5.3    State and lifetime
//------------------------------------------------------------------------------
// d_net_connection_is_open is false for NULL and for an incomplete
// connection. d_net_connection_close releases the resource, and does
// nothing for NULL or an incomplete connection; it may be called again.
bool             d_net_connection_is_open(
                     const struct d_net_connection* _connection);
void             d_net_connection_close(struct d_net_connection* _connection);
/**
 * @brief Closes one or both directions of a connection.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _mode        the direction or directions.
 * @return the backend's result, or D_NET_ERROR_NONE when it has no shutdown
 *         operation; D_NET_ERROR_INVALID_ARGUMENT for a NULL or incomplete
 *         connection, or an unknown mode.
 */
enum d_net_error d_net_connection_shutdown(
                     struct d_net_connection* _connection,
                     enum d_net_shutdown      _mode);
/**
 * @brief Reports the address of a connection's peer.
 *
 * @param[in]  _connection  the connection.
 * @param[out] _out         receives the address; emptied, as
 *                          d_net_endpoint_init empties it, when unknown.
 * @return true if the address is known; false if it is not, or an argument
 *         is NULL.
 */
bool             d_net_connection_remote_endpoint(
                     const struct d_net_connection* _connection,
                     struct d_net_endpoint*         _out);
/**
 * @brief Reports the local address of a connection.
 *
 * @param[in]  _connection  the connection.
 * @param[out] _out         receives the address; emptied when unknown.
 * @return true if the address is known; false otherwise.
 */
bool             d_net_connection_local_endpoint(
                     const struct d_net_connection* _connection,
                     struct d_net_endpoint*         _out);
/**
 * @brief Closes a connection, then releases its storage.
 *
 * The storage is released only by a backend that supplies a destroy
 * operation; otherwise it belongs to whoever declared it, and this call
 * only closes.
 *
 * @param[in] _connection  the connection; may be NULL.
 * @post `_connection` must not be used again when its backend released it.
 */
void             d_net_connection_destroy(struct d_net_connection* _connection);


//==============================================================================
// 6.  STREAM ALGORITHMS
//==============================================================================
// Loops over the section 5 calls, so they work on every backend. Each takes
// an optional counter reporting how far it got, in bytes, so that a caller
// whose connection is non-blocking can resume after D_NET_ERROR_WOULD_BLOCK.
// Scratch space is a stack buffer of D_NET_IO_CHUNK bytes; nothing
// allocates.


// 6.1    Exact transfers
//------------------------------------------------------------------------------
/**
 * @brief Reads exactly `_size` bytes, looping over short reads.
 *
 * @param[in,out] _connection  the connection.
 * @param[out]    _buffer      receives the bytes.
 * @param[in]     _size        how many to read.
 * @param[out]    _out_read    receives how many were read; may be NULL.
 * @return D_NET_ERROR_NONE once all are read; D_NET_ERROR_CLOSED if the
 *         stream ends first; otherwise the connection's error, or
 *         D_NET_ERROR_INVALID_ARGUMENT for a NULL connection, or a NULL
 *         `_buffer` with a nonzero `_size`.
 */
enum d_net_error d_net_read_exactly(struct d_net_connection* _connection,
                                    void*                    _buffer,
                                    size_t                   _size,
                                    size_t*                  _out_read);
/**
 * @brief Writes all of `_data`, looping over short writes.
 *
 * @param[in,out] _connection   the connection.
 * @param[in]     _data         the bytes.
 * @param[in]     _size         how many.
 * @param[out]    _out_written  receives how many were written; may be NULL.
 * @return D_NET_ERROR_NONE once all are written; D_NET_ERROR_CLOSED if the
 *         connection accepts nothing without an error; otherwise the
 *         failures of d_net_read_exactly.
 */
enum d_net_error d_net_write_all(struct d_net_connection* _connection,
                                 const void*              _data,
                                 size_t                   _size,
                                 size_t*                  _out_written);

// 6.2    Bulk transfers
//------------------------------------------------------------------------------
/**
 * @brief Reads a connection to the end of its stream, into a sink.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _sink        receives every byte, a chunk at a time.
 * @param[out]    _out_total   receives how many bytes the sink took; may
 *                             be NULL.
 * @return D_NET_ERROR_NONE at a clean end of stream; the connection's error;
 *         D_NET_ERROR_MESSAGE_TOO_LARGE when the sink takes fewer bytes than
 *         it is given, being full or unable to grow; or
 *         D_NET_ERROR_INVALID_ARGUMENT for a NULL connection, or a sink
 *         without a write function.
 */
enum d_net_error d_net_read_all(struct d_net_connection* _connection,
                                struct d_pack_sink       _sink,
                                size_t*                  _out_total);
/**
 * @brief Copies one connection's stream into another until it ends.
 *
 * Useful for proxying.
 *
 * @param[in,out] _source     the connection read from.
 * @param[in,out] _sink       the connection written to.
 * @param[out]    _out_total  receives how many bytes reached `_sink`; may
 *                            be NULL.
 * @return D_NET_ERROR_NONE when `_source` ends cleanly; otherwise the first
 *         error from either connection, or D_NET_ERROR_INVALID_ARGUMENT for
 *         a NULL connection.
 */
enum d_net_error d_net_pump(struct d_net_connection* _source,
                            struct d_net_connection* _sink,
                            size_t*                  _out_total);


//==============================================================================
// 7.  FRAMING
//==============================================================================
// A frame is a 4-byte big-endian length followed by that many payload bytes:
// it turns a byte stream into discrete messages. Every frame reader refuses a
// declared length above its ceiling before reading the payload, which leaves
// the stream mid-frame: after D_NET_ERROR_MESSAGE_TOO_LARGE from a read, the
// connection should be closed. The header codec is inline and pure, and the
// C++ framing templates call it too, so the two languages cannot disagree
// about the wire.


// 7.1    The frame header
//------------------------------------------------------------------------------
/**
 * @brief Checks a payload size against a ceiling and the header's range.
 *
 * @param[in] _size  the payload size.
 * @param[in] _max   the ceiling.
 * @return D_NET_ERROR_NONE, or D_NET_ERROR_MESSAGE_TOO_LARGE if `_size`
 *         exceeds `_max` or D_NET_FRAME_LENGTH_MAX.
 */
D_STATIC_INLINE enum d_net_error
d_net_frame_check(
    size_t _size,
    size_t _max
)
{
    // the header is 32 bits wide, whatever the ceiling allows
    if ( ((unsigned long long)_size > D_NET_FRAME_LENGTH_MAX) ||
         (_size > _max) )
    {
        return D_NET_ERROR_MESSAGE_TOO_LARGE;
    }

    return D_NET_ERROR_NONE;
}

/**
 * @brief Writes a frame header: `_length` as four big-endian bytes.
 *
 * @param[in]  _length  the payload length.
 * @param[out] _header  receives D_NET_FRAME_HEADER_SIZE bytes; NULL is
 *                      ignored.
 */
D_STATIC_INLINE void
d_net_frame_header_encode(
    uint32_t       _length,
    unsigned char* _header
)
{
    // parameter validation
    if (!_header)
    {
        return;
    }

    _header[0] = (unsigned char)((_length >> 24) & 0xFFu);
    _header[1] = (unsigned char)((_length >> 16) & 0xFFu);
    _header[2] = (unsigned char)((_length >> 8) & 0xFFu);
    _header[3] = (unsigned char)(_length & 0xFFu);

    return;
}

/**
 * @brief Reads the payload length from a frame header.
 *
 * @param[in] _header  D_NET_FRAME_HEADER_SIZE bytes.
 * @return the declared length; 0 for a NULL header.
 */
D_STATIC_INLINE uint32_t
d_net_frame_header_decode(
    const unsigned char* _header
)
{
    // parameter validation
    if (!_header)
    {
        return 0u;
    }

    return ( ((uint32_t)_header[0] << 24) |
             ((uint32_t)_header[1] << 16) |
             ((uint32_t)_header[2] << 8)  |
             ((uint32_t)_header[3]) );
}

// 7.2    Framed transfers
//------------------------------------------------------------------------------
/**
 * @brief Writes one frame: a header, then the payload.
 *
 * A frame no larger than D_NET_IO_CHUNK goes out in a single write, so its
 * header and payload never straddle a Nagle delay.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _data        the payload; may be NULL when `_size` is 0.
 * @param[in]     _size        its length.
 * @param[in]     _max         the ceiling, usually D_NET_FRAME_MAX.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_MESSAGE_TOO_LARGE, with nothing
 *         written, when d_net_frame_check refuses `_size`; or the failures
 *         of d_net_write_all.
 */
enum d_net_error d_net_write_frame(struct d_net_connection* _connection,
                                   const void*              _data,
                                   size_t                   _size,
                                   size_t                   _max);
/**
 * @brief Reads one frame's payload into a buffer.
 *
 * The buffer's capacity is the ceiling: a longer frame is refused before its
 * payload is read.
 *
 * @param[in,out] _connection  the connection.
 * @param[out]    _buffer      receives the payload.
 * @param[in]     _capacity    its size in bytes.
 * @param[out]    _out_length  receives the length the header declared, or
 *                             0 if no header was read; may be NULL.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_MESSAGE_TOO_LARGE for a frame longer
 *         than `_capacity`; D_NET_ERROR_CLOSED if the stream ends within the
 *         frame; otherwise the failures of d_net_read_exactly.
 */
enum d_net_error d_net_read_frame(struct d_net_connection* _connection,
                                  void*                    _buffer,
                                  size_t                   _capacity,
                                  size_t*                  _out_length);
/**
 * @brief Reads one frame's payload into a sink, a chunk at a time.
 *
 * If the stream fails partway, the sink keeps what it was already given.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _sink        receives the payload.
 * @param[in]     _max         the ceiling, usually D_NET_FRAME_MAX.
 * @param[out]    _out_length  receives the length the header declared, or
 *                             0 if no header was read; may be NULL.
 * @return the results of d_net_read_frame, with `_max` as the ceiling; also
 *         D_NET_ERROR_MESSAGE_TOO_LARGE when the sink takes fewer bytes than
 *         it is given, and D_NET_ERROR_INVALID_ARGUMENT for a sink without a
 *         write function.
 */
enum d_net_error d_net_read_frame_sink(struct d_net_connection* _connection,
                                       struct d_pack_sink       _sink,
                                       size_t                   _max,
                                       size_t*                  _out_length);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_H
