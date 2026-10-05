/*******************************************************************************
* djinterp [net]                                                   sftp_common.c
*
* Implementation of the SFTP vocabulary and codec declared in sftp_common.h.
*   Every reader goes through d_ssh_reader, whose failure is sticky, so each
* parser reads a whole reply and judges it once. Where a record carries
* fields of unknown size -- attribute flags version 3 does not define -- the
* record is refused rather than guessed at.
*
*
* path:      /src/djinterp/net/sftp/sftp_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/sftp/sftp_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint8_t, uint32_t, uint64_t
#include <string.h>   // memcmp, memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../inc/djinterp/net/ssh/ssh_wire.h"    // d_ssh_reader


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// FILE_TYPE_MASK
//   constant: the file-type bits of a mode, S_IFMT.
static const uint32_t FILE_TYPE_MASK = 0170000u;

// FILE_TYPE_DIRECTORY
//   constant: the file type of a directory, S_IFDIR.
static const uint32_t FILE_TYPE_DIRECTORY = 0040000u;

// FILE_TYPE_REGULAR
//   constant: the file type of a regular file, S_IFREG.
static const uint32_t FILE_TYPE_REGULAR = 0100000u;

// FILE_TYPE_SYMLINK
//   constant: the file type of a symbolic link, S_IFLNK.
static const uint32_t FILE_TYPE_SYMLINK = 0120000u;

// ATTRIBUTE_FIELDS
//   constant: the attribute flags whose fields version 3 defines.
static const uint32_t ATTRIBUTE_FIELDS = D_SFTP_ATTR_SIZE        |
                                         D_SFTP_ATTR_UIDGID      |
                                         D_SFTP_ATTR_PERMISSIONS |
                                         D_SFTP_ATTR_ACMODTIME;

// d_sftp_internal_extension_name
//   struct: one row of the table of extensions the client recognizes.
struct d_sftp_internal_extension_name
{
    const char* name;  // as VERSION announces it
    uint32_t    flag;  // its D_SFTP_EXT_* bit
};

// EXTENSIONS
//   constant: the extensions recognized in a VERSION packet.
static const struct d_sftp_internal_extension_name EXTENSIONS[] =
{
    { "posix-rename@openssh.com", D_SFTP_EXT_POSIX_RENAME },
    { "statvfs@openssh.com",      D_SFTP_EXT_STATVFS      },
    { "fstatvfs@openssh.com",     D_SFTP_EXT_FSTATVFS     },
    { "hardlink@openssh.com",     D_SFTP_EXT_HARDLINK     },
    { "fsync@openssh.com",        D_SFTP_EXT_FSYNC        },
    { "lsetstat@openssh.com",     D_SFTP_EXT_LSETSTAT     },
    { "limits@openssh.com",       D_SFTP_EXT_LIMITS       },
    { "expand-path@openssh.com",  D_SFTP_EXT_EXPAND_PATH  }
};

//==============================================================================
// 3.  ERRORS
//==============================================================================

/*
d_sftp_error_string
  A switch over every enumerator, so -Wswitch flags a new error without a
description; the fallback covers values outside the enumeration.
*/
const char*
d_sftp_error_string(
    enum d_sftp_error _error
)
{
    switch (_error)
    {
        case D_SFTP_OK:
            return "success";
        case D_SFTP_ERROR_EOF:
            return "end of file";
        case D_SFTP_ERROR_NO_SUCH_FILE:
            return "no such file";
        case D_SFTP_ERROR_PERMISSION_DENIED:
            return "permission denied";
        case D_SFTP_ERROR_FAILURE:
            return "failure";
        case D_SFTP_ERROR_BAD_MESSAGE:
            return "bad message";
        case D_SFTP_ERROR_NO_CONNECTION:
            return "no connection";
        case D_SFTP_ERROR_CONNECTION_LOST:
            return "connection lost";
        case D_SFTP_ERROR_UNSUPPORTED:
            return "operation not supported";
        case D_SFTP_ERROR_INVALID_ARGUMENT:
            return "invalid argument";
        case D_SFTP_ERROR_MEMORY:
            return "out of memory";
        case D_SFTP_ERROR_PROTOCOL:
            return "protocol error";
        case D_SFTP_ERROR_VERSION:
            return "unsupported protocol version";
        case D_SFTP_ERROR_TRANSPORT:
            return "SSH channel failure";
        case D_SFTP_ERROR_BAD_SEQUENCE:
            return "bad sequence of operations";
        case D_SFTP_ERROR_TOO_LARGE:
            return "too large";
    }

    return "unknown error";
}

/*
d_sftp_error_from_status
  The errors keep the status codes' numbers, so the map is a range check.
*/
enum d_sftp_error
d_sftp_error_from_status(
    uint32_t _code
)
{
    // codes 0 to 8 are version 3's own
    if (_code <= (uint32_t)D_SFTP_FX_OP_UNSUPPORTED)
    {
        return (enum d_sftp_error)_code;
    }

    return D_SFTP_ERROR_FAILURE;
}

//==============================================================================
// 4.  ATTRIBUTES
//==============================================================================

/*
d_sftp_attributes_init
  Zero is every field's "not present".
*/
void
d_sftp_attributes_init(
    struct d_sftp_attributes* _attributes
)
{
    // parameter validation
    if (!_attributes)
    {
        return;
    }

    const struct d_sftp_attributes empty = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    *_attributes = empty;

    return;
}

/*
d_sftp_internal_skip_extended
  File-local: skips the extended pairs, a type and its data each. The server
counts them, so a hostile count cannot loop past the packet: each pair
needs bytes, and the reader fails at the end of them.
*/
D_STATIC bool
d_sftp_internal_skip_extended(
    struct d_ssh_reader* _reader
)
{
    uint32_t count = 0u;
    bool     ok    = d_ssh_read_uint32(_reader,
                                       &count);

    // each pair is two strings
    for (uint32_t index = 0u; (ok) && (index < count); index++)
    {
        const unsigned char* bytes = NULL;
        size_t               size  = 0u;

        ok = ( (d_ssh_read_string(_reader,
                                  &bytes,
                                  &size)) &&
               (d_ssh_read_string(_reader,
                                  &bytes,
                                  &size)) );
    }

    return ok;
}

/*
d_sftp_internal_read_fields
  File-local: reads the fields `_record->flags` says are present, in flag
order, then skips any extended pairs.
*/
D_STATIC bool
d_sftp_internal_read_fields(
    struct d_ssh_reader*      _reader,
    struct d_sftp_attributes* _record
)
{
    const uint32_t flags = _record->flags;
    bool           ok    = true;

    if ((flags & D_SFTP_ATTR_SIZE) != 0u)
    {
        ok = d_ssh_read_uint64(_reader,
                               &_record->size);
    }

    if ( (ok) &&
         ((flags & D_SFTP_ATTR_UIDGID) != 0u) )
    {
        ok = ( (d_ssh_read_uint32(_reader,
                                  &_record->uid)) &&
               (d_ssh_read_uint32(_reader,
                                  &_record->gid)) );
    }

    if ( (ok) &&
         ((flags & D_SFTP_ATTR_PERMISSIONS) != 0u) )
    {
        ok = d_ssh_read_uint32(_reader,
                               &_record->permissions);
    }

    if ( (ok) &&
         ((flags & D_SFTP_ATTR_ACMODTIME) != 0u) )
    {
        ok = ( (d_ssh_read_uint32(_reader,
                                  &_record->atime)) &&
               (d_ssh_read_uint32(_reader,
                                  &_record->mtime)) );
    }

    return ( (ok) &&
             ( ((flags & D_SFTP_ATTR_EXTENDED) == 0u) ||
               (d_sftp_internal_skip_extended(_reader)) ) );
}

/*
d_sftp_read_attributes
  The flags first, which must name only the fields version 3 defines, since
a field of unknown size could not be skipped; then the fields they name.
*/
bool
d_sftp_read_attributes(
    struct d_ssh_reader*      _reader,
    struct d_sftp_attributes* _attributes
)
{
    // parameter validation
    if (!_reader)
    {
        return false;
    }

    struct d_sftp_attributes record = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    const bool               ok     =
        ( (_attributes != NULL)                                  &&
          (d_ssh_read_uint32(_reader,
                             &record.flags))                     &&
          ((record.flags & ~(ATTRIBUTE_FIELDS |
                             D_SFTP_ATTR_EXTENDED)) == 0u)       &&
          (d_sftp_internal_read_fields(_reader,
                                       &record)) );

    // a failure marks the reader, as every ssh_wire read does
    if (!ok)
    {
        _reader->failed = true;
        d_sftp_attributes_init(_attributes);

        return false;
    }

    // the pairs are not kept, so the record no longer claims them
    record.flags &= ~D_SFTP_ATTR_EXTENDED;
    *_attributes  = record;

    return true;
}

/*
d_sftp_write_attributes
  Only fields version 3 defines can be written; extended pairs are never
sent, so a record claiming them is refused rather than sent short.
*/
bool
d_sftp_write_attributes(
    struct d_ssh_writer*            _writer,
    const struct d_sftp_attributes* _attributes
)
{
    // parameter validation
    if (!_writer)
    {
        return false;
    }

    const struct d_sftp_attributes        empty  = { 0u, 0u, 0u, 0u,
                                                     0u, 0u, 0u };
    const struct d_sftp_attributes* const record = (_attributes) ? _attributes
                                                                 : &empty;
    const uint32_t                        flags  = record->flags;

    // flags whose fields this writer cannot express
    if ((flags & ~ATTRIBUTE_FIELDS) != 0u)
    {
        _writer->failed = true;

        return false;
    }

    bool ok = d_ssh_write_uint32(_writer,
                                 flags);

    // the fields, in flag order
    if ( (ok) &&
         ((flags & D_SFTP_ATTR_SIZE) != 0u) )
    {
        ok = d_ssh_write_uint64(_writer,
                                record->size);
    }

    if ( (ok) &&
         ((flags & D_SFTP_ATTR_UIDGID) != 0u) )
    {
        ok = ( (d_ssh_write_uint32(_writer,
                                   record->uid)) &&
               (d_ssh_write_uint32(_writer,
                                   record->gid)) );
    }

    if ( (ok) &&
         ((flags & D_SFTP_ATTR_PERMISSIONS) != 0u) )
    {
        ok = d_ssh_write_uint32(_writer,
                                record->permissions);
    }

    if ( (ok) &&
         ((flags & D_SFTP_ATTR_ACMODTIME) != 0u) )
    {
        ok = ( (d_ssh_write_uint32(_writer,
                                   record->atime)) &&
               (d_ssh_write_uint32(_writer,
                                   record->mtime)) );
    }

    return ok;
}

/*
d_sftp_internal_file_type
  File-local: reports whether a record's permissions carry file type
`_type`; without permissions, no type is known.
*/
D_STATIC bool
d_sftp_internal_file_type(
    const struct d_sftp_attributes* _attributes,
    uint32_t                        _type
)
{
    return ( (_attributes != NULL)                                       &&
             ((_attributes->flags & D_SFTP_ATTR_PERMISSIONS) != 0u)      &&
             ((_attributes->permissions & FILE_TYPE_MASK) == _type) );
}

/*
d_sftp_attributes_is_directory
  S_IFDIR.
*/
bool
d_sftp_attributes_is_directory(
    const struct d_sftp_attributes* _record
)
{
    return d_sftp_internal_file_type(_record,
                                     FILE_TYPE_DIRECTORY);
}

/*
d_sftp_attributes_is_regular
  S_IFREG.
*/
bool
d_sftp_attributes_is_regular(
    const struct d_sftp_attributes* _record
)
{
    return d_sftp_internal_file_type(_record,
                                     FILE_TYPE_REGULAR);
}

/*
d_sftp_attributes_is_symlink
  S_IFLNK.
*/
bool
d_sftp_attributes_is_symlink(
    const struct d_sftp_attributes* _record
)
{
    return d_sftp_internal_file_type(_record,
                                     FILE_TYPE_SYMLINK);
}

//==============================================================================
// 5.  PACKETS
//==============================================================================

/*
d_sftp_packet_begin
  The length is written as zero and filled in by d_sftp_packet_end(), once
the body's size is known.
*/
bool
d_sftp_packet_begin(
    struct d_ssh_writer* _writer,
    uint8_t              _type,
    uint32_t             _id,
    size_t*              _out_mark
)
{
    // parameter validation
    if (!_writer)
    {
        return false;
    }

    // a mark is required to finish the packet
    if (!_out_mark)
    {
        _writer->failed = true;

        return false;
    }

    *_out_mark = _writer->size;

    return ( (d_ssh_write_uint32(_writer,
                                 0u))    &&
             (d_ssh_write_byte(_writer,
                               _type))   &&
             (d_ssh_write_uint32(_writer,
                                 _id)) );
}

/*
d_sftp_packet_end
  The smallest packet is its length field, a type, and an id: nine bytes.
*/
bool
d_sftp_packet_end(
    struct d_ssh_writer* _writer,
    size_t               _mark
)
{
    // parameter validation
    if (!_writer)
    {
        return false;
    }

    // a failed writer, or a mark with no packet after it
    if ( (_writer->failed)                  ||
         (_mark > _writer->size)            ||
         ((_writer->size - _mark) < 9u) )
    {
        _writer->failed = true;

        return false;
    }

    const size_t length = (_writer->size - _mark) - 4u;

    // longer than any peer accepts
    if (length > D_SFTP_PACKET_MAX)
    {
        _writer->failed = true;

        return false;
    }

    unsigned char* const field = _writer->data + _mark;

    field[0] = (unsigned char)(length >> 24);
    field[1] = (unsigned char)(length >> 16);
    field[2] = (unsigned char)(length >> 8);
    field[3] = (unsigned char)length;

    return true;
}

/*
d_sftp_packet_frame
  The length alone decides malformation, so a hostile length is refused as
soon as four bytes arrive, before anything is buffered for it.
*/
enum d_sftp_frame
d_sftp_packet_frame(
    const void* _data,
    size_t      _size,
    size_t*     _out_size
)
{
    const unsigned char* const bytes = _data;

    // parameter validation
    if (!_out_size)
    {
        return D_SFTP_FRAME_MALFORMED;
    }

    // the length field is not yet whole
    if ( (!bytes) ||
         (_size < 4u) )
    {
        return D_SFTP_FRAME_INCOMPLETE;
    }

    const uint32_t length = ((uint32_t)bytes[0] << 24) |
                            ((uint32_t)bytes[1] << 16) |
                            ((uint32_t)bytes[2] << 8)  |
                            (uint32_t)bytes[3];

    // an empty packet, or one no buffer should hold
    if ( (length == 0u) ||
         (length > D_SFTP_PACKET_MAX) )
    {
        return D_SFTP_FRAME_MALFORMED;
    }

    // the body is not yet whole
    if ((_size - 4u) < (size_t)length)
    {
        return D_SFTP_FRAME_INCOMPLETE;
    }

    *_out_size = (size_t)length + 4u;

    return D_SFTP_FRAME_COMPLETE;
}

/*
d_sftp_packet_parse
  Every packet this client reads has an id after its type -- VERSION's is
the version -- so the header is read the same way for all of them.
*/
enum d_sftp_error
d_sftp_packet_parse(
    const void*           _data,
    size_t                _size,
    struct d_sftp_packet* _out
)
{
    // parameter validation
    if ( (!_data) ||
         (!_out) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    struct d_sftp_packet packet = { 0u, 0u, { NULL, 0u, 0u, false } };

    d_ssh_reader_init(&packet.body,
                      _data,
                      _size);

    // the type and the id
    if ( (!d_ssh_read_byte(&packet.body,
                           &packet.type)) ||
         (!d_ssh_read_uint32(&packet.body,
                             &packet.id)) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    *_out = packet;

    return D_SFTP_OK;
}

//==============================================================================
// 6.  RESPONSES
//==============================================================================

/*
d_sftp_read_status
  Draft 02 sends a message and a language tag after the code, but servers
older than it send neither, so each is read only if bytes remain.
*/
enum d_sftp_error
d_sftp_read_status(
    struct d_ssh_reader* _reader,
    uint32_t*            _out_code,
    struct d_pack_text*  _out_message
)
{
    // parameter validation
    if ( (!_reader)   ||
         (!_out_code) ||
         (!_out_message) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    const unsigned char* message      = NULL;
    const unsigned char* language     = NULL;
    size_t               message_size = 0u;
    size_t               language_size = 0u;
    uint32_t             code         = 0u;
    bool                 ok           = d_ssh_read_uint32(_reader,
                                                          &code);

    // the message, if present
    if ( (ok) &&
         (d_ssh_reader_remaining(_reader) > 0u) )
    {
        ok = d_ssh_read_string(_reader,
                               &message,
                               &message_size);
    }

    // then its language tag, if present
    if ( (ok) &&
         (d_ssh_reader_remaining(_reader) > 0u) )
    {
        ok = d_ssh_read_string(_reader,
                               &language,
                               &language_size);
    }

    // a status is complete once these are read
    if ( (!ok) ||
         (!d_ssh_reader_done(_reader)) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    *_out_code           = code;
    _out_message->data   = (const char*)message;
    _out_message->length = message_size;

    return D_SFTP_OK;
}

/*
d_sftp_read_handle
  The handle is copied, since it outlives the packet that carried it.
*/
enum d_sftp_error
d_sftp_read_handle(
    struct d_ssh_reader*  _reader,
    struct d_sftp_handle* _out
)
{
    // parameter validation
    if ( (!_reader) ||
         (!_out) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    const unsigned char* bytes = NULL;
    size_t               size  = 0u;

    // one string, no longer than the protocol allows, and nothing after it
    if ( (!d_ssh_read_string(_reader,
                             &bytes,
                             &size))       ||
         (!d_ssh_reader_done(_reader))     ||
         (size > D_SFTP_HANDLE_MAX) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    // memcpy wants a valid source even for zero bytes
    if (size > 0u)
    {
        memcpy(_out->bytes,
               bytes,
               size);
    }

    _out->length = (uint32_t)size;

    return D_SFTP_OK;
}

/*
d_sftp_read_data
  Trailing bytes are ignored: later drafts append an end-of-file flag, which
some servers send even under version 3.
*/
enum d_sftp_error
d_sftp_read_data(
    struct d_ssh_reader* _reader,
    struct d_pack_bytes* _out
)
{
    // parameter validation
    if ( (!_reader) ||
         (!_out) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    const unsigned char* bytes = NULL;
    size_t               size  = 0u;

    // the data is one string
    if (!d_ssh_read_string(_reader,
                           &bytes,
                           &size))
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    _out->data = bytes;
    _out->size = size;

    return D_SFTP_OK;
}

/*
d_sftp_read_name
  One entry: its name, its display line, and its attributes; more entries
may follow, so trailing bytes are the caller's to judge.
*/
enum d_sftp_error
d_sftp_read_name(
    struct d_ssh_reader* _reader,
    struct d_sftp_name*  _out
)
{
    // parameter validation
    if ( (!_reader) ||
         (!_out) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    const unsigned char* name          = NULL;
    const unsigned char* line          = NULL;
    size_t               name_length   = 0u;
    size_t               line_length   = 0u;

    // the two strings, then the attribute record
    if ( (!d_ssh_read_string(_reader,
                             &name,
                             &name_length))           ||
         (!d_ssh_read_string(_reader,
                             &line,
                             &line_length))           ||
         (!d_sftp_read_attributes(_reader,
                                  &_out->attributes)) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    _out->filename.data   = (const char*)name;
    _out->filename.length = name_length;
    _out->longname.data   = (const char*)line;
    _out->longname.length = line_length;

    return D_SFTP_OK;
}

/*
d_sftp_read_extensions
  Each pair is a name and its data; the data (a version string for the
OpenSSH extensions) is not needed to use them.
*/
enum d_sftp_error
d_sftp_read_extensions(
    struct d_ssh_reader* _reader,
    uint32_t*            _out_extensions
)
{
    // parameter validation
    if ( (!_reader) ||
         (!_out_extensions) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t count      = sizeof(EXTENSIONS) / sizeof(EXTENSIONS[0]);
    uint32_t     extensions = 0u;

    // pair by pair, until the packet ends or a read fails
    while (d_ssh_reader_remaining(_reader) > 0u)
    {
        const unsigned char* name      = NULL;
        const unsigned char* data      = NULL;
        size_t               name_size = 0u;
        size_t               data_size = 0u;

        // a name without its data is malformed
        if ( (!d_ssh_read_string(_reader,
                                 &name,
                                 &name_size)) ||
             (!d_ssh_read_string(_reader,
                                 &data,
                                 &data_size)) )
        {
            break;
        }

        // the names this client knows
        for (size_t index = 0u; index < count; index++)
        {
            const size_t length = strlen(EXTENSIONS[index].name);

            if ( (name_size == length) &&
                 (memcmp(name,
                         EXTENSIONS[index].name,
                         length) == 0) )
            {
                extensions |= EXTENSIONS[index].flag;
            }
        }
    }

    // every pair read, and nothing failed
    if (!d_ssh_reader_done(_reader))
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    *_out_extensions = extensions;

    return D_SFTP_OK;
}

/*
d_sftp_read_limits
  Four uint64s; any fields a later OpenSSH appends are ignored.
*/
enum d_sftp_error
d_sftp_read_limits(
    struct d_ssh_reader*  _reader,
    struct d_sftp_limits* _out
)
{
    // parameter validation
    if ( (!_reader) ||
         (!_out) )
    {
        return D_SFTP_ERROR_INVALID_ARGUMENT;
    }

    struct d_sftp_limits limits = { 0u, 0u, 0u, 0u };

    // all four, in order
    if ( (!d_ssh_read_uint64(_reader,
                             &limits.packet_length)) ||
         (!d_ssh_read_uint64(_reader,
                             &limits.read_length))   ||
         (!d_ssh_read_uint64(_reader,
                             &limits.write_length))  ||
         (!d_ssh_read_uint64(_reader,
                             &limits.open_handles)) )
    {
        return D_SFTP_ERROR_PROTOCOL;
    }

    *_out = limits;

    return D_SFTP_OK;
}
