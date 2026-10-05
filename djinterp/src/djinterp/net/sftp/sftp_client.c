/*******************************************************************************
* djinterp [net]                                                   sftp_client.c
*
* Implementation of the SFTP client declared in sftp_client.h.
*   Received bytes collect in one buffer sized for the largest packet; a
* whole packet is parsed where it lies, and what follows it stays for the
* next. Requests are built in one reusable writer. This file holds the
* session and every single-request operation; sftp_transfer.c holds the
* pipelined transfers, through the helpers sftp_client_internal.h shares.
*
*
* path:      /src/djinterp/net/sftp/sftp_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/sftp/sftp_client.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t, uint64_t, UINT32_MAX
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memmove, memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"    // d_pack_bytes
#include "../../../../inc/djinterp/net/sftp/sftp_common.h"  // codec
#include "../../../../inc/djinterp/net/ssh/ssh_channel.h"   // d_ssh_channel
#include "../../../../inc/djinterp/net/ssh/ssh_common.h"    // d_ssh_status
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"    // d_ssh_stream
#include "../../../../inc/djinterp/net/ssh/ssh_session.h"   // d_ssh_session
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"      // d_ssh_writer
#include "./sftp_client_internal.h"                         // shared helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// PACKET_ROOM
//   constant: bytes a READ reply or a WRITE request carries besides its data
// -- header, id, handle, offset, and length -- rounded up generously; a
// chunk plus this must fit the server's packet limit.
static const uint64_t PACKET_ROOM = 1024u;

//==============================================================================
// 3.  CLIENT
//==============================================================================

/*
d_sftp_internal_channel_read
  File-local: the channel transport's read. SFTP's replies arrive on the
channel's standard output; its standard error carries only a server's
diagnostics.
*/
D_STATIC enum d_ssh_status
d_sftp_internal_channel_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_received
)
{
    return d_ssh_channel_read((struct d_ssh_channel*)_context,
                              D_SSH_STREAM_STDOUT,
                              _buffer,
                              _capacity,
                              _out_received);
}

/*
d_sftp_internal_channel_write
  File-local: the channel transport's write.
*/
D_STATIC enum d_ssh_status
d_sftp_internal_channel_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    return d_ssh_channel_write((struct d_ssh_channel*)_context,
                               _data,
                               _size);
}

/*
d_sftp_internal_client_lost
  File-local: records a stream failure and returns its error: a stream that
ended is a lost connection; anything else, the transport's failure.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_lost(
    struct d_sftp_client* _client,
    enum d_ssh_status     _status
)
{
    _client->ssh_status = _status;

    return (_status == D_SSH_ERR_CLOSED) ? D_SFTP_ERROR_CONNECTION_LOST
                                         : D_SFTP_ERROR_TRANSPORT;
}

/*
d_sftp_internal_client_receive
  Reads the next whole packet. Before reading more, a partial
packet moves to the front of the buffer, which -- sized for the largest
packet plus room -- then always has space for the rest of it. The packet's
reader borrows the buffer until the next call.
*/
enum d_sftp_error
d_sftp_internal_client_receive(
    struct d_sftp_client* _client,
    struct d_sftp_packet* _out
)
{
    // until a whole packet is buffered
    for (;;)
    {
        const unsigned char* const pending = _client->buffer +
                                             _client->buffer_start;
        const size_t               size    = _client->buffer_end -
                                             _client->buffer_start;
        size_t                     total   = 0u;
        const enum d_sftp_frame    frame   = d_sftp_packet_frame(pending,
                                                                 size,
                                                                 &total);

        // a length no packet may have
        if (frame == D_SFTP_FRAME_MALFORMED)
        {
            return D_SFTP_ERROR_PROTOCOL;
        }

        // a whole packet: step past it, and parse it where it lies
        if (frame == D_SFTP_FRAME_COMPLETE)
        {
            _client->buffer_start += total;

            return d_sftp_packet_parse(pending + 4u,
                                       total - 4u,
                                       _out);
        }

        // the partial packet moves to the front
        if (_client->buffer_start > 0u)
        {
            memmove(_client->buffer,
                    pending,
                    size);

            _client->buffer_start = 0u;
            _client->buffer_end   = size;
        }

        size_t                  received = 0u;
        const enum d_ssh_status status   =
            _client->transport.read(_client->transport.context,
                                    _client->buffer + _client->buffer_end,
                                    D_SFTP_CLIENT_BUFFER_SIZE -
                                    _client->buffer_end,
                                    &received);

        // a failed stream, or one that broke its promise of a byte
        if ( (status != D_SSH_OK) ||
             (received == 0u) )
        {
            return d_sftp_internal_client_lost(
                       _client,
                       (status != D_SSH_OK) ? status : D_SSH_ERR_IO);
        }

        _client->buffer_end += received;
    }
}

/*
d_sftp_internal_client_begin
  File-local: starts a request in the emptied writer, under the next id.
Ids run from 1 and wrap back to 1, so a reply can never be mistaken for one
to a request long gone: no more than a pipeline's worth are outstanding.
*/
D_STATIC bool
d_sftp_internal_client_begin(
    struct d_sftp_client* _client,
    uint8_t               _type,
    uint32_t*             _out_id,
    size_t*               _out_mark
)
{
    const uint32_t id = _client->next_id;

    _client->next_id       = (id == UINT32_MAX) ? 1u : (id + 1u);
    _client->writer.size   = 0u;
    _client->writer.failed = false;
    *_out_id               = id;

    return d_sftp_packet_begin(&_client->writer,
                               _type,
                               id,
                               _out_mark);
}

/*
d_sftp_internal_client_send
  File-local: finishes the request in the writer and writes it. Arguments
are validated before building, so a request that could not be built was
too long for a packet, or ran out of memory.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_send(
    struct d_sftp_client* _client,
    bool                  _built,
    size_t                _mark
)
{
    // a failed build, or a packet past the limit
    if ( (!_built) ||
         (!d_sftp_packet_end(&_client->writer,
                             _mark)) )
    {
        return ((_client->writer.size - _mark) > D_SFTP_PACKET_MAX)
               ? D_SFTP_ERROR_TOO_LARGE
               : D_SFTP_ERROR_MEMORY;
    }

    const enum d_ssh_status status =
        _client->transport.write(_client->transport.context,
                                 _client->writer.data,
                                 _client->writer.size);

    return (status == D_SSH_OK) ? D_SFTP_OK
                                : d_sftp_internal_client_lost(_client,
                                                              status);
}

/*
d_sftp_internal_client_reply
  File-local: reads the reply to the one request outstanding. A packet with
any other id breaks the protocol.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_reply(
    struct d_sftp_client* _client,
    uint32_t              _id,
    struct d_sftp_packet* _out
)
{
    const enum d_sftp_error error = d_sftp_internal_client_receive(_client,
                                                                   _out);

    // a failure, or the reply awaited
    if ( (error != D_SFTP_OK) ||
         (_out->id == _id) )
    {
        return error;
    }

    return D_SFTP_ERROR_PROTOCOL;
}

/*
d_sftp_internal_client_status
  Reads a STATUS reply, keeping its code and message -- the
message truncated to fit, and NUL-terminated -- and returns the error it
means. A packet that is not a well-formed STATUS breaks the protocol.
*/
enum d_sftp_error
d_sftp_internal_client_status(
    struct d_sftp_client* _client,
    struct d_sftp_packet* _packet
)
{
    uint32_t           code    = 0u;
    struct d_pack_text message = { NULL, 0u };

    // a status of the right shape
    if ( (_packet->type != D_SFTP_FXP_STATUS) ||
         (d_sftp_read_status(&_packet->body,
                             &code,
                             &message) != D_SFTP_OK) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    const size_t length = (message.length < sizeof(_client->status_message))
                          ? message.length
                          : (sizeof(_client->status_message) - 1u);

    // memcpy wants a valid source even for zero bytes
    if (length > 0u)
    {
        memcpy(_client->status_message,
               message.data,
               length);
    }

    _client->status_message[length] = '\0';
    _client->status_code            = code;

    return d_sftp_error_from_status(code);
}

/*
d_sftp_internal_client_done
  File-local: awaits a request that STATUS alone answers -- REMOVE, MKDIR,
CLOSE, and the like -- and returns what it means.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_done(
    struct d_sftp_client* _client,
    uint32_t              _id
)
{
    struct d_sftp_packet    packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    const enum d_sftp_error error  = d_sftp_internal_client_reply(_client,
                                                                  _id,
                                                                  &packet);

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_status(_client,
                                                                &packet);
}

/*
d_sftp_internal_client_answer
  File-local: awaits a reply of `_type`. A STATUS instead carries the
server's refusal -- or, if it claims success, breaks the protocol, since it
answered without the result asked for.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_answer(
    struct d_sftp_client* _client,
    uint32_t              _id,
    uint8_t               _type,
    struct d_sftp_packet* _out
)
{
    enum d_sftp_error error = d_sftp_internal_client_reply(_client,
                                                           _id,
                                                           _out);

    // a failure, or the reply asked for
    if ( (error != D_SFTP_OK) ||
         (_out->type == _type) )
    {
        return error;
    }

    error = d_sftp_internal_client_status(_client,
                                          _out);

    return (error != D_SFTP_OK) ? error : D_SFTP_ERROR_PROTOCOL;
}

/*
d_sftp_internal_client_path
  File-local: sends a request whose body is one path: STAT, LSTAT, REMOVE,
RMDIR, REALPATH, READLINK, or OPENDIR.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_path(
    struct d_sftp_client* _client,
    uint8_t               _type,
    const char*           _path,
    uint32_t*             _out_id
)
{
    size_t     mark  = 0u;
    const bool built = ( (d_sftp_internal_client_begin(_client,
                                                       _type,
                                                       _out_id,
                                                       &mark)) &&
                         (d_ssh_write_cstring(&_client->writer,
                                              _path)) );

    return d_sftp_internal_client_send(_client,
                                       built,
                                       mark);
}

/*
d_sftp_internal_client_path_attributes
  File-local: sends a request whose body is a path and an attribute record:
MKDIR or SETSTAT.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_path_attributes(
    struct d_sftp_client*           _client,
    uint8_t                         _type,
    const char*                     _path,
    const struct d_sftp_attributes* _attributes,
    uint32_t*                       _out_id
)
{
    size_t     mark  = 0u;
    const bool built = ( (d_sftp_internal_client_begin(_client,
                                                       _type,
                                                       _out_id,
                                                       &mark))       &&
                         (d_ssh_write_cstring(&_client->writer,
                                              _path))                &&
                         (d_sftp_write_attributes(&_client->writer,
                                                  _attributes)) );

    return d_sftp_internal_client_send(_client,
                                       built,
                                       mark);
}

/*
d_sftp_internal_client_handle_request
  File-local: sends a request whose body is a handle -- CLOSE, FSTAT,
READDIR -- or, with `_extension`, an EXTENDED request naming it first.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_handle_request(
    struct d_sftp_client*       _client,
    uint8_t                     _type,
    const char*                 _extension,
    const struct d_sftp_handle* _handle,
    uint32_t*                   _out_id
)
{
    size_t     mark  = 0u;
    const bool built = ( (d_sftp_internal_client_begin(_client,
                                                       _type,
                                                       _out_id,
                                                       &mark))         &&
                         ( (!_extension) ||
                           (d_ssh_write_cstring(&_client->writer,
                                                _extension)) )         &&
                         (d_ssh_write_string(&_client->writer,
                                             _handle->bytes,
                                             _handle->length)) );

    return d_sftp_internal_client_send(_client,
                                       built,
                                       mark);
}

/*
d_sftp_internal_client_read_request
  Sends a READ for `_length` bytes at `_offset`.
*/
enum d_sftp_error
d_sftp_internal_client_read_request(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    uint64_t                    _offset,
    uint32_t                    _length,
    uint32_t*                   _out_id
)
{
    size_t     mark  = 0u;
    const bool built = ( (d_sftp_internal_client_begin(_client,
                                                       D_SFTP_FXP_READ,
                                                       _out_id,
                                                       &mark))      &&
                         (d_ssh_write_string(&_client->writer,
                                             _handle->bytes,
                                             _handle->length))      &&
                         (d_ssh_write_uint64(&_client->writer,
                                             _offset))              &&
                         (d_ssh_write_uint32(&_client->writer,
                                             _length)) );

    return d_sftp_internal_client_send(_client,
                                       built,
                                       mark);
}

/*
d_sftp_internal_client_write_request
  Sends a WRITE of `_data` at `_offset`.
*/
enum d_sftp_error
d_sftp_internal_client_write_request(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    uint64_t                    _offset,
    struct d_pack_bytes         _data,
    uint32_t*                   _out_id
)
{
    size_t     mark  = 0u;
    const bool built = ( (d_sftp_internal_client_begin(_client,
                                                       D_SFTP_FXP_WRITE,
                                                       _out_id,
                                                       &mark))      &&
                         (d_ssh_write_string(&_client->writer,
                                             _handle->bytes,
                                             _handle->length))      &&
                         (d_ssh_write_uint64(&_client->writer,
                                             _offset))              &&
                         (d_ssh_write_string(&_client->writer,
                                             _data.data,
                                             _data.size)) );

    return d_sftp_internal_client_send(_client,
                                       built,
                                       mark);
}

/*
d_sftp_internal_client_handle
  File-local: awaits a HANDLE reply, copying the handle out.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_handle(
    struct d_sftp_client* _client,
    uint32_t              _id,
    struct d_sftp_handle* _out
)
{
    struct d_sftp_packet    packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    const enum d_sftp_error error  =
        d_sftp_internal_client_answer(_client,
                                      _id,
                                      D_SFTP_FXP_HANDLE,
                                      &packet);

    return (error != D_SFTP_OK) ? error
                                : d_sftp_read_handle(&packet.body,
                                                     _out);
}

/*
d_sftp_internal_client_attributes
  File-local: awaits an ATTRS reply, reading the record out.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_attributes(
    struct d_sftp_client*     _client,
    uint32_t                  _id,
    struct d_sftp_attributes* _out
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    enum d_sftp_error    error  = d_sftp_internal_client_answer(
                                      _client,
                                      _id,
                                      D_SFTP_FXP_ATTRS,
                                      &packet);

    // one record, and nothing after it
    if ( (error == D_SFTP_OK) &&
         ( (!d_sftp_read_attributes(&packet.body,
                                    _out)) ||
           (!d_ssh_reader_done(&packet.body)) ) )
    {
        error = D_SFTP_ERROR_PROTOCOL;
    }

    return error;
}

/*
d_sftp_internal_client_single_name
  File-local: awaits a NAME reply holding exactly one entry, as REALPATH and
READLINK return, and copies its name out, NUL-terminated.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_single_name(
    struct d_sftp_client* _client,
    uint32_t              _id,
    char*                 _out,
    size_t                _capacity
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    struct d_sftp_name   name;
    uint32_t             count  = 0u;
    enum d_sftp_error    error  = d_sftp_internal_client_answer(
                                      _client,
                                      _id,
                                      D_SFTP_FXP_NAME,
                                      &packet);

    memset(&name,
           0,
           sizeof(name));

    // exactly one entry, and nothing after it
    if ( (error == D_SFTP_OK) &&
         ( (!d_ssh_read_uint32(&packet.body,
                               &count))                      ||
           (count != 1u)                                     ||
           (d_sftp_read_name(&packet.body,
                             &name) != D_SFTP_OK)            ||
           (!d_ssh_reader_done(&packet.body)) ) )
    {
        error = D_SFTP_ERROR_PROTOCOL;
    }

    // the name and its terminator must fit
    if ( (error == D_SFTP_OK) &&
         (name.filename.length >= _capacity) )
    {
        error = D_SFTP_ERROR_TOO_LARGE;
    }

    if (error != D_SFTP_OK)
    {
        return error;
    }

    // memcpy wants a valid source even for zero bytes
    if (name.filename.length > 0u)
    {
        memcpy(_out,
               name.filename.data,
               name.filename.length);
    }

    _out[name.filename.length] = '\0';

    return D_SFTP_OK;
}

/*
d_sftp_internal_client_ready
  The checks every operation starts with: a client, the
pointers it needs, and the client open.
*/
enum d_sftp_error
d_sftp_internal_client_ready(
    const struct d_sftp_client* _client,
    const void*                 _first,
    const void*                 _second
)
{
    // parameter validation
    if ( (!_client) ||
         (!_first)  ||
         (!_second) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    return (_client->open) ? D_SFTP_OK : D_SFTP_ERROR_BAD_SEQUENCE;
}

/*
d_sftp_internal_attributes_ok
  File-local: reports whether attributes can be sent: none at all, or a
record flagging only fields version 3 defines.
*/
D_STATIC bool
d_sftp_internal_attributes_ok(
    const struct d_sftp_attributes* _attributes
)
{
    const uint32_t fields = D_SFTP_ATTR_SIZE        |
                            D_SFTP_ATTR_UIDGID      |
                            D_SFTP_ATTR_PERMISSIONS |
                            D_SFTP_ATTR_ACMODTIME;

    return ( (!_attributes) ||
             ((_attributes->flags & ~fields) == 0u) );
}

/*
d_sftp_internal_chunk
  File-local: the chunk size a stated limit allows: the limit, capped at
D_SFTP_CHUNK_MAX; D_SFTP_CHUNK_SIZE where the server states none.
*/
D_STATIC uint32_t
d_sftp_internal_chunk(
    uint64_t _limit
)
{
    // no limit stated
    if (_limit == 0u)
    {
        return D_SFTP_CHUNK_SIZE;
    }

    return (_limit < D_SFTP_CHUNK_MAX) ? (uint32_t)_limit : D_SFTP_CHUNK_MAX;
}

/*
d_sftp_internal_packet_cap
  File-local: tightens a data limit so a chunk and PACKET_ROOM fit the
server's packet limit, where it states one.
*/
D_STATIC uint64_t
d_sftp_internal_packet_cap(
    uint64_t _data_limit,
    uint64_t _packet_limit
)
{
    // no packet limit, or one too small to tighten anything by
    if (_packet_limit <= PACKET_ROOM)
    {
        return _data_limit;
    }

    const uint64_t room = _packet_limit - PACKET_ROOM;

    return ( (_data_limit == 0u) ||
             (_data_limit > room) ) ? room : _data_limit;
}

/*
d_sftp_internal_client_limits
  File-local: asks for limits@openssh.com's limits and sizes chunks by them.
The extension is a courtesy: a server that refuses the request leaves the
defaults in place; only a broken stream fails the opening.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_limits(
    struct d_sftp_client* _client
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    struct d_sftp_limits limits = { 0u, 0u, 0u, 0u };
    uint32_t             id     = 0u;
    size_t               mark   = 0u;
    const bool           built  =
        ( (d_sftp_internal_client_begin(_client,
                                        D_SFTP_FXP_EXTENDED,
                                        &id,
                                        &mark)) &&
          (d_ssh_write_cstring(&_client->writer,
                               "limits@openssh.com")) );
    enum d_sftp_error    error  = d_sftp_internal_client_send(_client,
                                                              built,
                                                              mark);

    // the reply, and the limits in it
    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_answer(_client,
                                              id,
                                              D_SFTP_FXP_EXTENDED_REPLY,
                                              &packet);
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_read_limits(&packet.body,
                                   &limits);
    }

    // a refusal leaves the defaults
    if ( (error >= D_SFTP_ERROR_EOF) &&
         (error <= D_SFTP_ERROR_UNSUPPORTED) )
    {
        return D_SFTP_OK;
    }

    // a broken stream, or limits to apply
    if (error != D_SFTP_OK)
    {
        return error;
    }

    _client->read_size  = d_sftp_internal_chunk(
                              d_sftp_internal_packet_cap(
                                  limits.read_length,
                                  limits.packet_length));
    _client->write_size = d_sftp_internal_chunk(
                              d_sftp_internal_packet_cap(
                                  limits.write_length,
                                  limits.packet_length));

    return D_SFTP_OK;
}

/*
d_sftp_internal_client_handshake
  File-local: INIT, which carries the version where other packets carry an
id, then VERSION. A server may answer with a version above 3 only by
breaking draft 02, which bids it answer with the lower of the two; such a
server is spoken to in version 3 all the same.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_handshake(
    struct d_sftp_client* _client
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    size_t               mark   = 0u;

    _client->writer.size   = 0u;
    _client->writer.failed = false;

    const bool        built = d_sftp_packet_begin(&_client->writer,
                                                  D_SFTP_FXP_INIT,
                                                  D_SFTP_VERSION,
                                                  &mark);
    enum d_sftp_error error = d_sftp_internal_client_send(_client,
                                                          built,
                                                          mark);

    // the server's VERSION
    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_receive(_client,
                                               &packet);
    }

    if ( (error == D_SFTP_OK) &&
         (packet.type != D_SFTP_FXP_VERSION) )
    {
        error = D_SFTP_ERROR_PROTOCOL;
    }

    // a version this client can speak
    if ( (error == D_SFTP_OK) &&
         (packet.id < D_SFTP_VERSION) )
    {
        error = D_SFTP_ERROR_VERSION;
    }

    // the extensions it announces
    if (error == D_SFTP_OK)
    {
        error = d_sftp_read_extensions(&packet.body,
                                       &_client->extensions);
    }

    // its limits, where it states them
    if ( (error == D_SFTP_OK) &&
         ((_client->extensions & D_SFTP_EXT_LIMITS) != 0u) )
    {
        error = d_sftp_internal_client_limits(_client);
    }

    _client->version = D_SFTP_VERSION;

    return error;
}

/*
d_sftp_client_init
  Zeroes the client, then sets what zero does not mean: an empty writer,
the first request id, and the default chunk sizes. A zeroed channel is a
closed one.
*/
void
d_sftp_client_init(
    struct d_sftp_client* _client
)
{
    // parameter validation
    if (!_client)
    {
        return;
    }

    memset(_client,
           0,
           sizeof(*_client));
    d_ssh_writer_init(&_client->writer);

    _client->next_id    = 1u;
    _client->read_size  = D_SFTP_CHUNK_SIZE;
    _client->write_size = D_SFTP_CHUNK_SIZE;
    _client->ssh_status = D_SSH_OK;

    return;
}

/*
d_sftp_client_close
  The server's sftp-server exits at EOF, and the channel closes after it.
The diagnostics survive, for a caller inspecting why an open failed.
*/
void
d_sftp_client_close(
    struct d_sftp_client* _client
)
{
    // parameter validation
    if (!_client)
    {
        return;
    }

    // a channel this client opened is this client's to close
    if (_client->owns_channel)
    {
        const enum d_ssh_status eof    =
            d_ssh_channel_send_eof(&_client->channel);
        const enum d_ssh_status closed =
            d_ssh_channel_close(&_client->channel);

        (void)eof;
        (void)closed;
        d_ssh_channel_free(&_client->channel);
        _client->owns_channel = false;
    }

    free(_client->buffer);
    d_ssh_writer_free(&_client->writer);
    d_ssh_writer_init(&_client->writer);

    _client->buffer            = NULL;
    _client->buffer_start      = 0u;
    _client->buffer_end        = 0u;
    _client->transport.read    = NULL;
    _client->transport.write   = NULL;
    _client->transport.context = NULL;
    _client->open              = false;
    _client->version           = 0u;
    _client->extensions        = 0u;

    return;
}

/*
d_sftp_client_open_transport
  The buffer is allocated before the first byte is sent, so a client that
cannot hold a reply never asks for one.
*/
enum d_sftp_error
d_sftp_client_open_transport(
    struct d_sftp_client*   _client,
    struct d_sftp_transport _transport
)
{
    // parameter validation
    if ( (!_client)          ||
         (!_transport.read)  ||
         (!_transport.write) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    // one session at a time
    if ( (_client->open) ||
         (_client->buffer) )
    {
        return D_SFTP_ERROR_BAD_SEQUENCE;
    }

    _client->buffer = malloc(D_SFTP_CLIENT_BUFFER_SIZE);

    // no room for replies, no requests
    if (!_client->buffer)
    {
        return D_SFTP_ERROR_MEMORY;
    }

    _client->transport         = _transport;
    _client->buffer_start      = 0u;
    _client->buffer_end        = 0u;
    _client->next_id           = 1u;
    _client->read_size         = D_SFTP_CHUNK_SIZE;
    _client->write_size        = D_SFTP_CHUNK_SIZE;
    _client->extensions        = 0u;
    _client->status_code       = 0u;
    _client->status_message[0] = '\0';
    _client->ssh_status        = D_SSH_OK;

    const enum d_sftp_error error = d_sftp_internal_client_handshake(_client);

    // a failed exchange leaves the client closed; the channel is the
    // caller's to release, or d_sftp_client_open()'s
    if (error != D_SFTP_OK)
    {
        free(_client->buffer);

        _client->buffer          = NULL;
        _client->transport.read  = NULL;
        _client->transport.write = NULL;

        return error;
    }

    _client->open = true;

    return D_SFTP_OK;
}

/*
d_sftp_client_open
  The channel is released on any failure, so a failed open owns nothing.
*/
enum d_sftp_error
d_sftp_client_open(
    struct d_sftp_client* _client,
    struct d_ssh_session* _session
)
{
    // parameter validation
    if ( (!_client) ||
         (!_session) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    // one session at a time
    if ( (_client->open) ||
         (_client->owns_channel) )
    {
        return D_SFTP_ERROR_BAD_SEQUENCE;
    }

    enum d_ssh_status status = d_ssh_channel_open(_session,
                                                  &_client->channel);

    // the channel, then the subsystem on it
    if (status == D_SSH_OK)
    {
        _client->owns_channel = true;
        status                = d_ssh_channel_subsystem(&_client->channel,
                                                        "sftp");
    }

    // a refused subsystem means no SFTP here
    if (status != D_SSH_OK)
    {
        _client->ssh_status = status;
        d_sftp_client_close(_client);

        return (status == D_SSH_ERR_CHANNEL) ? D_SFTP_ERROR_UNSUPPORTED
                                             : D_SFTP_ERROR_TRANSPORT;
    }

    const struct d_sftp_transport transport =
    {
        d_sftp_internal_channel_read,
        d_sftp_internal_channel_write,
        &_client->channel
    };
    const enum d_sftp_error       error     =
        d_sftp_client_open_transport(_client,
                                     transport);

    // a failed exchange releases the channel too
    if (error != D_SFTP_OK)
    {
        d_sftp_client_close(_client);
    }

    return error;
}

/*
d_sftp_client_realpath
  REALPATH answers with a single NAME, the canonical path.
*/
enum d_sftp_error
d_sftp_client_realpath(
    struct d_sftp_client* _client,
    const char*           _path,
    char*                 _out,
    size_t                _capacity
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _out);

    // room for at least the terminator
    if ( (error == D_SFTP_OK) &&
         (_capacity == 0u) )
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_REALPATH,
                                            _path,
                                            &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_single_name(
                                      _client,
                                      id,
                                      _out,
                                      _capacity);
}

/*
d_sftp_client_stat
  STAT follows symbolic links.
*/
enum d_sftp_error
d_sftp_client_stat(
    struct d_sftp_client*     _client,
    const char*               _path,
    struct d_sftp_attributes* _out
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _out);

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_STAT,
                                            _path,
                                            &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_attributes(_client,
                                                                    id,
                                                                    _out);
}

/*
d_sftp_client_lstat
  LSTAT describes a symbolic link itself.
*/
enum d_sftp_error
d_sftp_client_lstat(
    struct d_sftp_client*     _client,
    const char*               _path,
    struct d_sftp_attributes* _out
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _out);

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_LSTAT,
                                            _path,
                                            &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_attributes(_client,
                                                                    id,
                                                                    _out);
}

/*
d_sftp_client_setstat
  The attributes are checked before building, so a record version 3 cannot
express is the caller's error, not a failed write.
*/
enum d_sftp_error
d_sftp_client_setstat(
    struct d_sftp_client*           _client,
    const char*                     _path,
    const struct d_sftp_attributes* _attributes
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _attributes);

    // a record this version can send
    if ( (error == D_SFTP_OK) &&
         (!d_sftp_internal_attributes_ok(_attributes)) )
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path_attributes(_client,
                                                       D_SFTP_FXP_SETSTAT,
                                                       _path,
                                                       _attributes,
                                                       &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_client_remove
  REMOVE deletes a file; directories take RMDIR.
*/
enum d_sftp_error
d_sftp_client_remove(
    struct d_sftp_client* _client,
    const char*           _path
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _path);

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_REMOVE,
                                            _path,
                                            &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_client_rename
  posix-rename@openssh.com takes the same two paths as RENAME, after its
name; it replaces the target as rename(2) does, atomically.
*/
enum d_sftp_error
d_sftp_client_rename(
    struct d_sftp_client* _client,
    const char*           _from,
    const char*           _to,
    bool                  _replace
)
{
    uint32_t          id    = 0u;
    size_t            mark  = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _from,
                                                           _to);

    // replacing needs the extension; version 3's RENAME refuses to
    if ( (error == D_SFTP_OK) &&
         (_replace)           &&
         ((_client->extensions & D_SFTP_EXT_POSIX_RENAME) == 0u) )
    {
        error = D_SFTP_ERROR_UNSUPPORTED;
    }

    if (error == D_SFTP_OK)
    {
        const uint8_t type  = (_replace) ? D_SFTP_FXP_EXTENDED
                                         : D_SFTP_FXP_RENAME;
        const bool    built =
            ( (d_sftp_internal_client_begin(_client,
                                            type,
                                            &id,
                                            &mark))                       &&
              ( (!_replace) ||
                (d_ssh_write_cstring(&_client->writer,
                                     "posix-rename@openssh.com")) )       &&
              (d_ssh_write_cstring(&_client->writer,
                                   _from))                                &&
              (d_ssh_write_cstring(&_client->writer,
                                   _to)) );

        error = d_sftp_internal_client_send(_client,
                                            built,
                                            mark);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_client_readlink
  READLINK answers with a single NAME, the link's target.
*/
enum d_sftp_error
d_sftp_client_readlink(
    struct d_sftp_client* _client,
    const char*           _path,
    char*                 _out,
    size_t                _capacity
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _out);

    // room for at least the terminator
    if ( (error == D_SFTP_OK) &&
         (_capacity == 0u) )
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_READLINK,
                                            _path,
                                            &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_single_name(
                                      _client,
                                      id,
                                      _out,
                                      _capacity);
}

/*
d_sftp_client_symlink
  Target first, then the link: OpenSSH's order, the reverse of draft 02's,
which the OpenSSH PROTOCOL file records and every deployed server follows.
*/
enum d_sftp_error
d_sftp_client_symlink(
    struct d_sftp_client* _client,
    const char*           _target,
    const char*           _link
)
{
    uint32_t          id    = 0u;
    size_t            mark  = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _target,
                                                           _link);

    if (error == D_SFTP_OK)
    {
        const bool built = ( (d_sftp_internal_client_begin(_client,
                                                           D_SFTP_FXP_SYMLINK,
                                                           &id,
                                                           &mark))   &&
                             (d_ssh_write_cstring(&_client->writer,
                                                  _target))          &&
                             (d_ssh_write_cstring(&_client->writer,
                                                  _link)) );

        error = d_sftp_internal_client_send(_client,
                                            built,
                                            mark);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_client_mkdir
  NULL attributes send an empty record: the server's defaults.
*/
enum d_sftp_error
d_sftp_client_mkdir(
    struct d_sftp_client*           _client,
    const char*                     _path,
    const struct d_sftp_attributes* _attributes
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _path);

    // a record this version can send
    if ( (error == D_SFTP_OK) &&
         (!d_sftp_internal_attributes_ok(_attributes)) )
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path_attributes(_client,
                                                       D_SFTP_FXP_MKDIR,
                                                       _path,
                                                       _attributes,
                                                       &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_client_rmdir
  RMDIR removes an empty directory.
*/
enum d_sftp_error
d_sftp_client_rmdir(
    struct d_sftp_client* _client,
    const char*           _path
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _path);

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_RMDIR,
                                            _path,
                                            &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_internal_client_entries
  File-local: one READDIR, its entries passed on in order. Returns
D_SFTP_ERROR_EOF once the directory is exhausted.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_entries(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    d_sftp_entry_fn             _callback,
    void*                       _context
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    uint32_t             id     = 0u;
    uint32_t             count  = 0u;
    enum d_sftp_error    error  = d_sftp_internal_client_handle_request(
                                      _client,
                                      D_SFTP_FXP_READDIR,
                                      NULL,
                                      _handle,
                                      &id);

    // a NAME, or the directory's end
    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_answer(_client,
                                              id,
                                              D_SFTP_FXP_NAME,
                                              &packet);
    }

    if ( (error == D_SFTP_OK) &&
         (!d_ssh_read_uint32(&packet.body,
                             &count)) )
    {
        error = D_SFTP_ERROR_PROTOCOL;
    }

    // each entry in turn, until the callback declines
    for (uint32_t index = 0u; (error == D_SFTP_OK) && (index < count); index++)
    {
        struct d_sftp_name entry;

        memset(&entry,
               0,
               sizeof(entry));

        error = d_sftp_read_name(&packet.body,
                                 &entry);

        if (error == D_SFTP_OK)
        {
            error = _callback(_context,
                              &entry);
        }
    }

    // the packet ends with its last entry
    if ( (error == D_SFTP_OK) &&
         (!d_ssh_reader_done(&packet.body)) )
    {
        error = D_SFTP_ERROR_PROTOCOL;
    }

    return error;
}

/*
d_sftp_client_list
  OPENDIR, READDIR until EOF, CLOSE. The handle is closed whatever ended the
listing, so a callback that stops early leaks nothing on the server.
*/
enum d_sftp_error
d_sftp_client_list(
    struct d_sftp_client* _client,
    const char*           _path,
    d_sftp_entry_fn       _callback,
    void*                 _context
)
{
    struct d_sftp_handle handle;
    uint32_t             id    = 0u;
    enum d_sftp_error    error = d_sftp_internal_client_ready(_client,
                                                              _path,
                                                              _path);

    // the callback is checked apart, since ISO C forbids viewing it as
    // data; a missing argument outranks a closed client, as everywhere
    if (!_callback)
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    memset(&handle,
           0,
           sizeof(handle));

    // the directory's handle
    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_path(_client,
                                            D_SFTP_FXP_OPENDIR,
                                            _path,
                                            &id);
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_handle(_client,
                                              id,
                                              &handle);
    }

    if (error != D_SFTP_OK)
    {
        return error;
    }

    // READDIR until the server reports the end
    do
    {
        error = d_sftp_internal_client_entries(_client,
                                               &handle,
                                               _callback,
                                               _context);
    } while (error == D_SFTP_OK);

    const enum d_sftp_error closing = d_sftp_client_close_file(_client,
                                                               &handle);

    // the directory's end is the listing's success
    error = (error == D_SFTP_ERROR_EOF) ? D_SFTP_OK : error;

    return (error != D_SFTP_OK) ? error : closing;
}

/*
d_sftp_client_open_file
  Flags beyond the six version 3 defines are refused before anything is
sent.
*/
enum d_sftp_error
d_sftp_client_open_file(
    struct d_sftp_client*           _client,
    const char*                     _path,
    uint32_t                        _flags,
    const struct d_sftp_attributes* _attributes,
    struct d_sftp_handle*           _out
)
{
    const uint32_t    known = D_SFTP_OPEN_READ     |
                              D_SFTP_OPEN_WRITE    |
                              D_SFTP_OPEN_APPEND   |
                              D_SFTP_OPEN_CREATE   |
                              D_SFTP_OPEN_TRUNCATE |
                              D_SFTP_OPEN_EXCLUSIVE;
    uint32_t          id    = 0u;
    size_t            mark  = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _path,
                                                           _out);

    // flags and attributes this version can send
    if ( (error == D_SFTP_OK)                              &&
         ( ((_flags & ~known) != 0u) ||
           (!d_sftp_internal_attributes_ok(_attributes)) ) )
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    if (error == D_SFTP_OK)
    {
        const bool built = ( (d_sftp_internal_client_begin(_client,
                                                           D_SFTP_FXP_OPEN,
                                                           &id,
                                                           &mark))       &&
                             (d_ssh_write_cstring(&_client->writer,
                                                  _path))                &&
                             (d_ssh_write_uint32(&_client->writer,
                                                 _flags))                &&
                             (d_sftp_write_attributes(&_client->writer,
                                                      _attributes)) );

        error = d_sftp_internal_client_send(_client,
                                            built,
                                            mark);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_handle(_client,
                                                                id,
                                                                _out);
}

/*
d_sftp_client_close_file
  CLOSE is where some servers report a failed write, so its status matters.
*/
enum d_sftp_error
d_sftp_client_close_file(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _handle,
                                                           _handle);

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_handle_request(_client,
                                                      D_SFTP_FXP_CLOSE,
                                                      NULL,
                                                      _handle,
                                                      &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}

/*
d_sftp_client_read
  One request, no more than `read_size` bytes; a reply longer than asked
breaks the protocol.
*/
enum d_sftp_error
d_sftp_client_read(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    uint64_t                    _offset,
    void*                       _buffer,
    size_t                      _capacity,
    size_t*                     _out_read
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    struct d_pack_bytes  data   = { NULL, 0u };
    uint32_t             id     = 0u;
    enum d_sftp_error    error  = d_sftp_internal_client_ready(_client,
                                                               _handle,
                                                               _buffer);

    // somewhere to put the count, and room for a byte
    if ( (error == D_SFTP_OK) &&
         ( (!_out_read) ||
           (_capacity == 0u) ) )
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    if (error != D_SFTP_OK)
    {
        return error;
    }

    const uint32_t length = (_capacity < (size_t)_client->read_size)
                            ? (uint32_t)_capacity
                            : _client->read_size;

    *_out_read = 0u;
    error      = d_sftp_internal_client_read_request(_client,
                                                     _handle,
                                                     _offset,
                                                     length,
                                                     &id);

    // DATA, or a status: EOF at the end of the file
    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_answer(_client,
                                              id,
                                              D_SFTP_FXP_DATA,
                                              &packet);
    }

    if ( (error == D_SFTP_OK) &&
         ( (d_sftp_read_data(&packet.body,
                             &data) != D_SFTP_OK) ||
           (data.size > length) ) )
    {
        error = D_SFTP_ERROR_PROTOCOL;
    }

    // the bytes, however few
    if ( (error == D_SFTP_OK) &&
         (data.size > 0u) )
    {
        memcpy(_buffer,
               data.data,
               data.size);

        *_out_read = data.size;
    }

    return error;
}

/*
d_sftp_client_fstat
  FSTAT describes an open file.
*/
enum d_sftp_error
d_sftp_client_fstat(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    struct d_sftp_attributes*   _out
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _handle,
                                                           _out);

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_handle_request(_client,
                                                      D_SFTP_FXP_FSTAT,
                                                      NULL,
                                                      _handle,
                                                      &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_attributes(_client,
                                                                    id,
                                                                    _out);
}

/*
d_sftp_client_fsync
  fsync@openssh.com takes the handle alone, after its name.
*/
enum d_sftp_error
d_sftp_client_fsync(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle
)
{
    uint32_t          id    = 0u;
    enum d_sftp_error error = d_sftp_internal_client_ready(_client,
                                                           _handle,
                                                           _handle);

    // the server must offer it
    if ( (error == D_SFTP_OK) &&
         ((_client->extensions & D_SFTP_EXT_FSYNC) == 0u) )
    {
        error = D_SFTP_ERROR_UNSUPPORTED;
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_handle_request(_client,
                                                      D_SFTP_FXP_EXTENDED,
                                                      "fsync@openssh.com",
                                                      _handle,
                                                      &id);
    }

    return (error != D_SFTP_OK) ? error
                                : d_sftp_internal_client_done(_client,
                                                              id);
}
