/*******************************************************************************
* djinterp [net]                                                 sftp_transfer.c
*
* Implementation of the SFTP client declared in sftp_client.h: the
* pipelined transfers.
*   A download keeps its READs in a ring in file order: the ring's head feeds
* the sink directly, and a reply that arrives before its turn is held until
* then; a short read is completed with a further request in place. An upload
* keeps its WRITEs' ids and retires each as its acknowledgement arrives, in
* whatever order. Either way, once a transfer stops, every reply still owed
* is read, so the stream stays in step for the next request.
*
*
* path:      /src/djinterp/net/sftp/sftp_transfer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/sftp/sftp_client.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t, uint64_t
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"    // d_pack_bytes
#include "../../../../inc/djinterp/net/sftp/sftp_common.h"  // codec
#include "./sftp_client_internal.h"                         // shared helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_sftp_internal_slot
//   struct: one range of a download in flight. The ring's head slot hands
// its data straight to the sink; a later slot's data, arriving before its
// turn, is held in `data` until then.
struct d_sftp_internal_slot
{
    uint32_t       id;       // the request in flight for the range
    uint64_t       offset;   // where the range starts
    uint32_t       length;   // bytes the range covers
    uint32_t       filled;   // bytes held in data
    bool           pending;  // a request is in flight
    bool           eof;      // the server reported the end here
    unsigned char* data;     // early bytes; NULL until needed
};

// d_sftp_internal_download
//   struct: a download in progress: its slots, a ring in file order, where
// the next range starts, and whether the server has reported the end and
// the head has reached it.
struct d_sftp_internal_download
{
    const struct d_sftp_handle* handle;    // the file
    d_sftp_sink_fn              sink;      // takes the data
    void*                       context;   // passed to sink
    struct d_sftp_internal_slot slots[D_SFTP_CLIENT_PIPELINE];
    size_t                      head;      // the slot the sink waits on
    size_t                      count;     // slots in use
    uint64_t                    next;      // the next range's offset
    bool                        ending;    // the server reported EOF
    bool                        finished;  // the head reached it
};

// d_sftp_internal_upload
//   struct: an upload in progress: its handle and source, the chunk the
// source's data is staged in, the ids of the WRITEs in flight, and where the
// next WRITE lands.
struct d_sftp_internal_upload
{
    const struct d_sftp_handle* handle;       // the file
    d_sftp_source_fn            source;       // supplies the data
    void*                       context;      // passed to source
    unsigned char*              chunk;        // stages each chunk
    uint32_t                    ids[D_SFTP_CLIENT_PIPELINE];
    size_t                      outstanding;  // WRITEs in flight
    uint64_t                    offset;       // where the next WRITE lands
    bool                        finished;     // the source has ended
    enum d_sftp_error           failure;      // the first refusal
};

// d_sftp_internal_memory
//   struct: a source over caller memory, for d_sftp_client_write().
struct d_sftp_internal_memory
{
    const unsigned char* data;      // the bytes
    size_t               size;      // how many
    size_t               position;  // how many supplied
};

//==============================================================================
// 3.  CLIENT
//==============================================================================

/*
d_sftp_internal_client_broken
  File-local: reports whether an error leaves the stream unusable -- out of
step, or gone -- so that no further replies can be trusted or read.
*/
D_STATIC bool
d_sftp_internal_client_broken(
    enum d_sftp_error _error
)
{
    return ( (_error == D_SFTP_ERROR_PROTOCOL)        ||
             (_error == D_SFTP_ERROR_TRANSPORT)       ||
             (_error == D_SFTP_ERROR_CONNECTION_LOST) ||
             (_error == D_SFTP_ERROR_TOO_LARGE) );
}

/*
d_sftp_internal_client_acknowledge
  File-local: reads one WRITE reply and retires its id, wherever it stands
among the outstanding ones. The first refusal is kept in `*_failure`; a
reply to no outstanding request breaks the stream.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_acknowledge(
    struct d_sftp_client* _client,
    uint32_t*             _ids,
    size_t*               _outstanding,
    enum d_sftp_error*    _failure
)
{
    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    enum d_sftp_error    error  = d_sftp_internal_client_receive(_client,
                                                                 &packet);
    size_t               index  = 0u;

    // the stream failed
    if (error != D_SFTP_OK)
    {
        return error;
    }

    // the request it answers
    while ( (index < *_outstanding) &&
            (_ids[index] != packet.id) )
    {
        index++;
    }

    if (index == *_outstanding)
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    _ids[index] = _ids[*_outstanding - 1u];
    (*_outstanding)--;
    error = d_sftp_internal_client_status(_client,
                                          &packet);

    // a reply that is no status breaks the stream; a refusal, the transfer
    if (error == D_SFTP_ERROR_PROTOCOL)
    {
        return error;
    }

    if ( (error != D_SFTP_OK) &&
         (*_failure == D_SFTP_OK) )
    {
        *_failure = error;
    }

    return D_SFTP_OK;
}

/*
d_sftp_internal_upload_fill
  File-local: fills the pipeline while the source has data and nothing has
failed, staging each chunk and sending it as a WRITE. Returns a broken
stream's error; the source's own failure, or its overfilling, lands in the
upload's `failure` and ends the filling.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_upload_fill(
    struct d_sftp_client*          _client,
    struct d_sftp_internal_upload* _upload
)
{
    // while the source has data, nothing failed, and there is room
    while ( (_upload->failure == D_SFTP_OK)                   &&
            (!_upload->finished)                              &&
            (_upload->outstanding < D_SFTP_CLIENT_PIPELINE) )
    {
        size_t size = 0u;

        _upload->failure = _upload->source(_upload->context,
                                           _upload->chunk,
                                           _client->write_size,
                                           &size);

        // a source that overfilled its buffer cannot be trusted
        if ( (_upload->failure == D_SFTP_OK) &&
             (size > _client->write_size) )
        {
            _upload->failure = D_SFTP_ERROR_INVALID_ARGUMENT;
        }

        // the end of the data, or of the source's cooperation
        if ( (_upload->failure != D_SFTP_OK) ||
             (size == 0u) )
        {
            _upload->finished = true;

            break;
        }

        const struct d_pack_bytes data   = { _upload->chunk, size };
        const enum d_sftp_error   broken =
            d_sftp_internal_client_write_request(
                _client,
                _upload->handle,
                _upload->offset,
                data,
                &_upload->ids[_upload->outstanding]);

        // a request that could not be sent ends the upload
        if (broken != D_SFTP_OK)
        {
            return broken;
        }

        _upload->outstanding++;
        _upload->offset += size;
    }

    return D_SFTP_OK;
}

/*
d_sftp_internal_client_upload
  File-local: writes the source's data from `_offset` on, keeping up to
D_SFTP_CLIENT_PIPELINE WRITEs in flight. After a refusal or a source error
no more is sent, but every outstanding reply is still read, keeping the
stream in step; a broken stream ends the transfer at once.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_upload(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    uint64_t                    _offset,
    d_sftp_source_fn            _source,
    void*                       _context
)
{
    struct d_sftp_internal_upload upload;

    memset(&upload,
           0,
           sizeof(upload));

    upload.handle  = _handle;
    upload.source  = _source;
    upload.context = _context;
    upload.offset  = _offset;
    upload.failure = D_SFTP_OK;
    upload.chunk   = malloc(_client->write_size);

    // nowhere to stage the data
    if (!upload.chunk)
    {
        return D_SFTP_ERROR_MEMORY;
    }

    enum d_sftp_error broken = D_SFTP_OK;

    // until every write is acknowledged, or the stream breaks
    for (;;)
    {
        broken = d_sftp_internal_upload_fill(_client,
                                             &upload);

        // nothing left in flight, or nothing more possible
        if ( (broken != D_SFTP_OK) ||
             (upload.outstanding == 0u) )
        {
            break;
        }

        broken = d_sftp_internal_client_acknowledge(_client,
                                                    upload.ids,
                                                    &upload.outstanding,
                                                    &upload.failure);

        // a broken stream leaves nothing to wait for
        if (broken != D_SFTP_OK)
        {
            break;
        }
    }

    free(upload.chunk);

    return (broken != D_SFTP_OK) ? broken : upload.failure;
}

/*
d_sftp_internal_memory_source
  File-local: supplies caller memory, for d_sftp_client_write().
*/
D_STATIC enum d_sftp_error
d_sftp_internal_memory_source(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_size
)
{
    struct d_sftp_internal_memory* const memory    = _context;
    const size_t                         remaining = memory->size -
                                                     memory->position;
    const size_t                         size      = (remaining < _capacity)
                                                     ? remaining
                                                     : _capacity;

    // memcpy wants a valid source even for zero bytes
    if (size > 0u)
    {
        memcpy(_buffer,
               memory->data + memory->position,
               size);
    }

    memory->position += size;
    *_out_size        = size;

    return D_SFTP_OK;
}

/*
d_sftp_client_write
  The pipelined upload, over caller memory.
*/
enum d_sftp_error
d_sftp_client_write(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    uint64_t                    _offset,
    const void*                 _data,
    size_t                      _size
)
{
    struct d_sftp_internal_memory memory = { _data, _size, 0u };
    const enum d_sftp_error       error  =
        d_sftp_internal_client_ready(_client,
                                     _handle,
                                     (_size == 0u) ? (const void*)_handle
                                                   : _data);

    return (error != D_SFTP_OK)
           ? error
           : d_sftp_internal_client_upload(_client,
                                           _handle,
                                           _offset,
                                           d_sftp_internal_memory_source,
                                           &memory);
}

/*
d_sftp_internal_download_issue
  File-local: asks for the part of a slot's range not yet received.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_issue(
    struct d_sftp_client*                  _client,
    const struct d_sftp_internal_download* _download,
    struct d_sftp_internal_slot*           _slot
)
{
    const enum d_sftp_error error =
        d_sftp_internal_client_read_request(_client,
                                            _download->handle,
                                            _slot->offset + _slot->filled,
                                            _slot->length - _slot->filled,
                                            &_slot->id);

    _slot->pending = (error == D_SFTP_OK);

    return error;
}

/*
d_sftp_internal_download_fill
  File-local: starts new ranges at the ring's tail until it is full, or
until the server has reported the file's end.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_fill(
    struct d_sftp_client*           _client,
    struct d_sftp_internal_download* _download
)
{
    // while there is room, and the file may go on
    while ( (!_download->ending) &&
            (_download->count < D_SFTP_CLIENT_PIPELINE) )
    {
        struct d_sftp_internal_slot* const slot =
            &_download->slots[(_download->head + _download->count) %
                              D_SFTP_CLIENT_PIPELINE];
        const struct d_sftp_internal_slot  fresh =
        {
            0u,
            _download->next,
            _client->read_size,
            0u,
            false,
            false,
            NULL
        };

        *slot = fresh;

        const enum d_sftp_error error =
            d_sftp_internal_download_issue(_client,
                                           _download,
                                           slot);

        // a request that could not be sent ends the download
        if (error != D_SFTP_OK)
        {
            return error;
        }

        _download->count++;
        _download->next += _client->read_size;
    }

    return D_SFTP_OK;
}

/*
d_sftp_internal_download_find
  File-local: the slot whose request an id answers, or NULL.
*/
D_STATIC struct d_sftp_internal_slot*
d_sftp_internal_download_find(
    struct d_sftp_internal_download* _download,
    uint32_t                         _id
)
{
    // only slots with a request in flight can be answered
    for (size_t index = 0u; index < _download->count; index++)
    {
        struct d_sftp_internal_slot* const slot =
            &_download->slots[(_download->head + index) %
                              D_SFTP_CLIENT_PIPELINE];

        if ( (slot->pending) &&
             (slot->id == _id) )
        {
            return slot;
        }
    }

    return NULL;
}

/*
d_sftp_internal_download_status
  File-local: takes a READ's STATUS reply. EOF marks the file's end at the
slot, and no range past it is asked for; success without data answers
nothing; anything else refuses the transfer.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_status(
    struct d_sftp_client*            _client,
    struct d_sftp_internal_download* _download,
    struct d_sftp_internal_slot*     _slot,
    struct d_sftp_packet*            _packet
)
{
    const enum d_sftp_error error =
        d_sftp_internal_client_status(_client,
                                      _packet);

    // EOF: nothing more lies from here on
    if (error == D_SFTP_ERROR_EOF)
    {
        _slot->eof        = true;
        _download->ending = true;

        return D_SFTP_OK;
    }

    return (error != D_SFTP_OK) ? error : D_SFTP_ERROR_PROTOCOL;
}

/*
d_sftp_internal_download_deliver
  File-local: hands the head's data to the sink, and moves the head's range
past it.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_deliver(
    struct d_sftp_internal_download* _download,
    struct d_sftp_internal_slot*     _slot,
    struct d_pack_bytes              _data
)
{
    const enum d_sftp_error error = _download->sink(_download->context,
                                                    _data.data,
                                                    _data.size);

    // the sink abandons the transfer
    if (error != D_SFTP_OK)
    {
        return error;
    }

    _slot->offset += _data.size;
    _slot->length -= (uint32_t)_data.size;

    return D_SFTP_OK;
}

/*
d_sftp_internal_download_hold
  File-local: keeps a later slot's data until its turn, allocating the
slot's buffer on the first such reply. With servers that answer in order,
as OpenSSH's does, nothing is ever allocated here.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_hold(
    struct d_sftp_internal_slot* _slot,
    struct d_pack_bytes          _data
)
{
    // the slot's buffer, on its first early reply
    if (!_slot->data)
    {
        _slot->data = malloc(_slot->length);

        if (!_slot->data)
        {
            return D_SFTP_ERROR_MEMORY;
        }
    }

    memcpy(_slot->data + _slot->filled,
           _data.data,
           _data.size);

    _slot->filled += (uint32_t)_data.size;

    return D_SFTP_OK;
}

/*
d_sftp_internal_download_data
  File-local: takes a READ's DATA reply: some bytes, and no more than asked.
The ring's head hands them to the sink; a later slot holds them until its
turn. Data shorter than asked has its rest asked for again at once, so a
slot's range is always complete before it is passed on.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_data(
    struct d_sftp_client*            _client,
    struct d_sftp_internal_download* _download,
    struct d_sftp_internal_slot*     _slot,
    struct d_sftp_packet*            _packet
)
{
    struct d_pack_bytes data = { NULL, 0u };

    // some data, and no more than was asked
    if ( (_packet->type != D_SFTP_FXP_DATA)                    ||
         (d_sftp_read_data(&_packet->body,
                           &data) != D_SFTP_OK)                ||
         (data.size == 0u)                                     ||
         (data.size > (size_t)(_slot->length - _slot->filled)) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    const enum d_sftp_error error =
        (_slot == &_download->slots[_download->head])
        ? d_sftp_internal_download_deliver(_download,
                                           _slot,
                                           data)
        : d_sftp_internal_download_hold(_slot,
                                        data);

    // the sink declined, or memory ran out
    if (error != D_SFTP_OK)
    {
        return error;
    }

    return (_slot->filled < _slot->length)
           ? d_sftp_internal_download_issue(_client,
                                            _download,
                                            _slot)
           : D_SFTP_OK;
}

/*
d_sftp_internal_download_accept
  File-local: takes one READ reply for a slot: a STATUS, or DATA.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_accept(
    struct d_sftp_client*            _client,
    struct d_sftp_internal_download* _download,
    struct d_sftp_internal_slot*     _slot,
    struct d_sftp_packet*            _packet
)
{
    return (_packet->type == D_SFTP_FXP_STATUS)
           ? d_sftp_internal_download_status(_client,
                                             _download,
                                             _slot,
                                             _packet)
           : d_sftp_internal_download_data(_client,
                                           _download,
                                           _slot,
                                           _packet);
}

/*
d_sftp_internal_download_advance
  File-local: moves the ring's head forward. A slot reaching the head first
passes on the bytes it held; the head then retires once its range is
delivered, and the download finishes at a head that reported EOF.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_advance(
    struct d_sftp_internal_download* _download,
    bool*                            _out_finished
)
{
    // for as long as the head has nothing left to wait for
    while (_download->count > 0u)
    {
        struct d_sftp_internal_slot* const slot =
            &_download->slots[_download->head];

        // bytes that arrived before the slot's turn go first
        if (slot->filled > 0u)
        {
            const enum d_sftp_error error =
                _download->sink(_download->context,
                                slot->data,
                                slot->filled);

            if (error != D_SFTP_OK)
            {
                return error;
            }

            slot->offset += slot->filled;
            slot->length -= slot->filled;
            slot->filled  = 0u;
            free(slot->data);
            slot->data    = NULL;
        }

        // the end of the file
        if (slot->eof)
        {
            *_out_finished = true;

            return D_SFTP_OK;
        }

        // still waiting on the server
        if ( (slot->pending) ||
             (slot->length > 0u) )
        {
            return D_SFTP_OK;
        }

        _download->head = (_download->head + 1u) % D_SFTP_CLIENT_PIPELINE;
        _download->count--;
    }

    return D_SFTP_OK;
}

/*
d_sftp_internal_download_drain
  File-local: reads, and drops, every reply still owed to a download that
has stopped, keeping the stream in step for the next request.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_drain(
    struct d_sftp_client*            _client,
    struct d_sftp_internal_download* _download
)
{
    // until no slot waits on the server
    for (;;)
    {
        size_t pending = 0u;

        for (size_t index = 0u; index < _download->count; index++)
        {
            pending += (_download->slots[(_download->head + index) %
                                         D_SFTP_CLIENT_PIPELINE].pending)
                       ? 1u
                       : 0u;
        }

        if (pending == 0u)
        {
            return D_SFTP_OK;
        }

        struct d_sftp_packet    packet = { 0u, 0u, { NULL, 0u, 0u, false } };
        const enum d_sftp_error error  =
            d_sftp_internal_client_receive(_client,
                                           &packet);

        // the stream failed
        if (error != D_SFTP_OK)
        {
            return error;
        }

        struct d_sftp_internal_slot* const slot =
            d_sftp_internal_download_find(_download,
                                          packet.id);

        // a reply to nothing outstanding
        if (!slot)
        {
            return D_SFTP_ERROR_PROTOCOL;
        }

        slot->pending = false;
    }
}

/*
d_sftp_internal_download_step
  File-local: one pass of a download: keep the ring full, take one reply for
its slot, then pass on whatever the head can. Returns the pass's first
error, which the caller sorts into a broken stream or a refusal.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_download_step(
    struct d_sftp_client*            _client,
    struct d_sftp_internal_download* _download
)
{
    struct d_sftp_packet         packet = { 0u, 0u, { NULL, 0u, 0u, false } };
    struct d_sftp_internal_slot* slot   = NULL;
    enum d_sftp_error            error  =
        d_sftp_internal_download_fill(_client,
                                      _download);

    // one reply
    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_client_receive(_client,
                                               &packet);
    }

    // for a slot that asked
    if (error == D_SFTP_OK)
    {
        slot  = d_sftp_internal_download_find(_download,
                                              packet.id);
        error = (slot) ? D_SFTP_OK : D_SFTP_ERROR_PROTOCOL;
    }

    // the reply for its slot, then whatever the head can pass on
    if (error == D_SFTP_OK)
    {
        slot->pending = false;
        error         = d_sftp_internal_download_accept(_client,
                                                        _download,
                                                        slot,
                                                        &packet);
    }

    if (error == D_SFTP_OK)
    {
        error = d_sftp_internal_download_advance(_download,
                                                 &_download->finished);
    }

    return error;
}

/*
d_sftp_internal_client_download
  File-local: reads a whole file into the sink, one reply per pass. A
refusal or a sink error stops new requests, and the replies still owed are
then drained; a broken stream ends everything at once, since no reply on it
can be trusted.
*/
D_STATIC enum d_sftp_error
d_sftp_internal_client_download(
    struct d_sftp_client*       _client,
    const struct d_sftp_handle* _handle,
    d_sftp_sink_fn              _sink,
    void*                       _context
)
{
    struct d_sftp_internal_download download;
    enum d_sftp_error               error = D_SFTP_OK;

    memset(&download,
           0,
           sizeof(download));

    download.handle  = _handle;
    download.sink    = _sink;
    download.context = _context;

    // one reply per pass, until the file ends or something fails
    while ( (error == D_SFTP_OK) &&
            (!download.finished) )
    {
        error = d_sftp_internal_download_step(_client,
                                              &download);
    }

    const bool              broken  = d_sftp_internal_client_broken(error);
    const enum d_sftp_error drained =
        (broken) ? D_SFTP_OK
                 : d_sftp_internal_download_drain(_client,
                                                  &download);

    // held bytes never delivered
    for (size_t index = 0u; index < D_SFTP_CLIENT_PIPELINE; index++)
    {
        free(download.slots[index].data);
    }

    return ( (broken) ||
             (drained == D_SFTP_OK) ) ? error : drained;
}

/*
d_sftp_client_get
  OPEN for reading, the pipelined download, CLOSE. The handle is closed
whatever happened; its error counts only when the download succeeded.
*/
enum d_sftp_error
d_sftp_client_get(
    struct d_sftp_client* _client,
    const char*           _path,
    d_sftp_sink_fn        _sink,
    void*                 _context
)
{
    struct d_sftp_handle handle;
    enum d_sftp_error    error = d_sftp_internal_client_ready(_client,
                                                              _path,
                                                              _path);

    // the callback is checked apart, since ISO C forbids viewing it as
    // data; a missing argument outranks a closed client, as everywhere
    if (!_sink)
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    memset(&handle,
           0,
           sizeof(handle));

    if (error == D_SFTP_OK)
    {
        error = d_sftp_client_open_file(_client,
                                        _path,
                                        D_SFTP_OPEN_READ,
                                        NULL,
                                        &handle);
    }

    if (error != D_SFTP_OK)
    {
        return error;
    }

    error = d_sftp_internal_client_download(_client,
                                            &handle,
                                            _sink,
                                            _context);

    const enum d_sftp_error closing = d_sftp_client_close_file(_client,
                                                               &handle);

    return (error != D_SFTP_OK) ? error : closing;
}

/*
d_sftp_client_put
  OPEN to create or truncate, the pipelined upload, CLOSE -- whose status
is the last word on whether the data reached the disk.
*/
enum d_sftp_error
d_sftp_client_put(
    struct d_sftp_client*           _client,
    const char*                     _path,
    const struct d_sftp_attributes* _attributes,
    d_sftp_source_fn                _source,
    void*                           _context
)
{
    struct d_sftp_handle handle;
    enum d_sftp_error    error = d_sftp_internal_client_ready(_client,
                                                              _path,
                                                              _path);

    // the callback is checked apart, since ISO C forbids viewing it as
    // data; a missing argument outranks a closed client, as everywhere
    if (!_source)
    {
        error = D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    memset(&handle,
           0,
           sizeof(handle));

    if (error == D_SFTP_OK)
    {
        error = d_sftp_client_open_file(_client,
                                        _path,
                                        D_SFTP_OPEN_WRITE  |
                                        D_SFTP_OPEN_CREATE |
                                        D_SFTP_OPEN_TRUNCATE,
                                        _attributes,
                                        &handle);
    }

    if (error != D_SFTP_OK)
    {
        return error;
    }

    error = d_sftp_internal_client_upload(_client,
                                          &handle,
                                          0u,
                                          _source,
                                          _context);

    const enum d_sftp_error closing = d_sftp_client_close_file(_client,
                                                               &handle);

    return (error != D_SFTP_OK) ? error : closing;
}
