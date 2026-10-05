/*******************************************************************************
* djinterp [net]                                                   sftp_common.h
*
* The SSH File Transfer Protocol, version 3: vocabulary and codec.
*   Version 3 (draft-ietf-secsh-filexfer-02) is the version OpenSSH speaks and
* the one every server accepts. A packet is a uint32 length, a type byte, a
* uint32 request id -- the protocol version in INIT and VERSION -- and a body
* in RFC 4251 encoding, so packets are built with d_ssh_writer and read with
* d_ssh_reader from ssh_wire.h. This header adds what SFTP puts inside them:
* status codes, open and attribute flags, the attribute record, handles,
* directory entries, the version exchange, and the OpenSSH extensions the
* client uses.
*   Nothing here performs I/O. Parsers return views into the packet they
* read; builders append to a writer. sftp_client.h runs the protocol over an
* SSH channel.
*
*
* path:      /inc/djinterp/net/sftp/sftp_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Protocol limits
         1.  D_SFTP_VERSION
         2.  D_SFTP_PACKET_MAX
         3.  D_SFTP_HANDLE_MAX
    2.  Transfer sizes
         1.  D_SFTP_CHUNK_SIZE
         2.  D_SFTP_CHUNK_MAX
         3.  D_SFTP_MESSAGE_SIZE
    3.  Attribute flags
         1.  D_SFTP_ATTR_SIZE
         2.  D_SFTP_ATTR_UIDGID
         3.  D_SFTP_ATTR_PERMISSIONS
         4.  D_SFTP_ATTR_ACMODTIME
         5.  D_SFTP_ATTR_EXTENDED
2.  TYPES
    -----
    1.  Protocol vocabulary
         1.  d_sftp_packet_type
         2.  d_sftp_status_code
         3.  d_sftp_open_flag
         4.  d_sftp_extension
    2.  Errors
         1.  d_sftp_error
    3.  Records
         1.  d_sftp_attributes
         2.  d_sftp_handle
         3.  d_sftp_name
         4.  d_sftp_limits
    4.  Packets
         1.  d_sftp_frame
         2.  d_sftp_packet
3.  ERRORS
    ------
    1.  Error operations
4.  ATTRIBUTES
    ----------
    1.  Encoding
    2.  File types
5.  PACKETS
    -------
    1.  Building
    2.  Framing and parsing
6.  RESPONSES
    ---------
    1.  Replies
    2.  Version and extensions
*/

#ifndef DJINTERP_NET_SFTP_SFTP_COMMON_H
#define DJINTERP_NET_SFTP_SFTP_COMMON_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint8_t, uint32_t, uint64_t
// djinterp
#include "../../c/djinterp.h"                // framework root
#include "../../c/util/sink_common.h"        // d_pack_text, d_pack_bytes
#include "../../config/net/sftp/cfg_sftp.h"  // D_INTERNAL_SFTP_*
#include "../ssh/ssh_wire.h"                 // d_ssh_reader, d_ssh_writer


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in sftp_common.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Protocol limits
//------------------------------------------------------------------------------
// 1.1.1
// D_SFTP_VERSION
//   constant: the protocol version spoken, 3.
#define D_SFTP_VERSION    3u

// 1.1.2
// D_SFTP_PACKET_MAX
//   constant: the longest packet accepted or built, its length field
// excluded: 256 KiB, OpenSSH's own limit, unless D_CFG_SFTP_PACKET_MAX says
// otherwise. A peer announcing more is refused before anything is allocated
// for it.
#define D_SFTP_PACKET_MAX ((uint32_t)D_INTERNAL_SFTP_PACKET_MAX)

// 1.1.3
// D_SFTP_HANDLE_MAX
//   constant: the longest handle a server may return (draft 02, section 6.2).
#define D_SFTP_HANDLE_MAX 256u


// 1.2    Transfer sizes
//------------------------------------------------------------------------------
// 1.2.1
// D_SFTP_CHUNK_SIZE
//   constant: bytes per READ or WRITE when the server states no limits:
// 32 KiB, which every server accepts, unless D_CFG_SFTP_CHUNK_SIZE says
// otherwise.
#define D_SFTP_CHUNK_SIZE   ((uint32_t)D_INTERNAL_SFTP_CHUNK_SIZE)

// 1.2.2
// D_SFTP_CHUNK_MAX
//   constant: the most one READ or WRITE moves, however high the server's
// limits@openssh.com reply goes: 64 KiB, unless D_CFG_SFTP_CHUNK_MAX says
// otherwise.
#define D_SFTP_CHUNK_MAX    ((uint32_t)D_INTERNAL_SFTP_CHUNK_MAX)

// 1.2.3
// D_SFTP_MESSAGE_SIZE
//   constant: bytes kept of a status message, its terminator included: 256,
// unless D_CFG_SFTP_MESSAGE_SIZE says otherwise.
#define D_SFTP_MESSAGE_SIZE ((uint32_t)D_INTERNAL_SFTP_MESSAGE_SIZE)


// 1.3    Attribute flags
//------------------------------------------------------------------------------
// The bits of an attribute record's flags, each naming the fields present.
// They are constants rather than an enumeration because
// D_SFTP_ATTR_EXTENDED does not fit in an int.

// 1.3.1
// D_SFTP_ATTR_SIZE
//   constant: the record carries `size`.
#define D_SFTP_ATTR_SIZE        0x00000001u

// 1.3.2
// D_SFTP_ATTR_UIDGID
//   constant: the record carries `uid` and `gid`.
#define D_SFTP_ATTR_UIDGID      0x00000002u

// 1.3.3
// D_SFTP_ATTR_PERMISSIONS
//   constant: the record carries `permissions`.
#define D_SFTP_ATTR_PERMISSIONS 0x00000004u

// 1.3.4
// D_SFTP_ATTR_ACMODTIME
//   constant: the record carries `atime` and `mtime`.
#define D_SFTP_ATTR_ACMODTIME   0x00000008u

// 1.3.5
// D_SFTP_ATTR_EXTENDED
//   constant: name/value pairs follow the other fields.
#define D_SFTP_ATTR_EXTENDED    0x80000000u


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Protocol vocabulary
//------------------------------------------------------------------------------
// 2.1.1
// d_sftp_packet_type
//   enum: the packet types of version 3, requests then responses.
enum d_sftp_packet_type
{
    D_SFTP_FXP_INIT           = 1,
    D_SFTP_FXP_VERSION        = 2,
    D_SFTP_FXP_OPEN           = 3,
    D_SFTP_FXP_CLOSE          = 4,
    D_SFTP_FXP_READ           = 5,
    D_SFTP_FXP_WRITE          = 6,
    D_SFTP_FXP_LSTAT          = 7,
    D_SFTP_FXP_FSTAT          = 8,
    D_SFTP_FXP_SETSTAT        = 9,
    D_SFTP_FXP_FSETSTAT       = 10,
    D_SFTP_FXP_OPENDIR        = 11,
    D_SFTP_FXP_READDIR        = 12,
    D_SFTP_FXP_REMOVE         = 13,
    D_SFTP_FXP_MKDIR          = 14,
    D_SFTP_FXP_RMDIR          = 15,
    D_SFTP_FXP_REALPATH       = 16,
    D_SFTP_FXP_STAT           = 17,
    D_SFTP_FXP_RENAME         = 18,
    D_SFTP_FXP_READLINK       = 19,
    D_SFTP_FXP_SYMLINK        = 20,
    D_SFTP_FXP_STATUS         = 101,
    D_SFTP_FXP_HANDLE         = 102,
    D_SFTP_FXP_DATA           = 103,
    D_SFTP_FXP_NAME           = 104,
    D_SFTP_FXP_ATTRS          = 105,
    D_SFTP_FXP_EXTENDED       = 200,
    D_SFTP_FXP_EXTENDED_REPLY = 201
};

// 2.1.2
// d_sftp_status_code
//   enum: the status codes of version 3. Later drafts add more, which
// servers occasionally send anyway; they read as D_SFTP_ERROR_FAILURE.
enum d_sftp_status_code
{
    D_SFTP_FX_OK                = 0,
    D_SFTP_FX_EOF               = 1,
    D_SFTP_FX_NO_SUCH_FILE      = 2,
    D_SFTP_FX_PERMISSION_DENIED = 3,
    D_SFTP_FX_FAILURE           = 4,
    D_SFTP_FX_BAD_MESSAGE       = 5,
    D_SFTP_FX_NO_CONNECTION     = 6,
    D_SFTP_FX_CONNECTION_LOST   = 7,
    D_SFTP_FX_OP_UNSUPPORTED    = 8
};

// 2.1.3
// d_sftp_open_flag
//   enum: the bits of SSH_FXP_OPEN's pflags.
enum d_sftp_open_flag
{
    D_SFTP_OPEN_READ      = 0x01,  // open for reading
    D_SFTP_OPEN_WRITE     = 0x02,  // open for writing
    D_SFTP_OPEN_APPEND    = 0x04,  // every write goes to the end
    D_SFTP_OPEN_CREATE    = 0x08,  // create the file if it is missing
    D_SFTP_OPEN_TRUNCATE  = 0x10,  // empty an existing file
    D_SFTP_OPEN_EXCLUSIVE = 0x20   // with CREATE: fail if the file exists
};

// 2.1.4
// d_sftp_extension
//   enum: the OpenSSH extensions a server's VERSION packet may announce, as
// bits of d_sftp_client's `extensions`.
enum d_sftp_extension
{
    D_SFTP_EXT_POSIX_RENAME = 0x0001,  // posix-rename@openssh.com
    D_SFTP_EXT_STATVFS      = 0x0002,  // statvfs@openssh.com
    D_SFTP_EXT_FSTATVFS     = 0x0004,  // fstatvfs@openssh.com
    D_SFTP_EXT_HARDLINK     = 0x0008,  // hardlink@openssh.com
    D_SFTP_EXT_FSYNC        = 0x0010,  // fsync@openssh.com
    D_SFTP_EXT_LSETSTAT     = 0x0020,  // lsetstat@openssh.com
    D_SFTP_EXT_LIMITS       = 0x0040,  // limits@openssh.com
    D_SFTP_EXT_EXPAND_PATH  = 0x0080   // expand-path@openssh.com
};


// 2.2    Errors
//------------------------------------------------------------------------------
// 2.2.1
// d_sftp_error
//   enum: an operation's outcome. The server's statuses keep their protocol
// numbers, so a status maps onto an error without a table; the client's own
// failures start at 0x100.
enum d_sftp_error
{
    D_SFTP_OK                      = 0,
    D_SFTP_ERROR_EOF               = 1,      // end of file or directory
    D_SFTP_ERROR_NO_SUCH_FILE      = 2,      // the path does not exist
    D_SFTP_ERROR_PERMISSION_DENIED = 3,      // the server refused access
    D_SFTP_ERROR_FAILURE           = 4,      // the server's catch-all
    D_SFTP_ERROR_BAD_MESSAGE       = 5,      // the server misread a request
    D_SFTP_ERROR_NO_CONNECTION     = 6,      // the server has no connection
    D_SFTP_ERROR_CONNECTION_LOST   = 7,      // the connection ended
    D_SFTP_ERROR_UNSUPPORTED       = 8,      // the server or client cannot
    D_SFTP_ERROR_INVALID_ARGUMENT  = 0x100,  // an argument is unusable
    D_SFTP_ERROR_MEMORY            = 0x101,  // an allocation failed
    D_SFTP_ERROR_PROTOCOL          = 0x102,  // a malformed or stray packet
    D_SFTP_ERROR_VERSION           = 0x103,  // the server is older than 3
    D_SFTP_ERROR_TRANSPORT         = 0x104,  // the SSH channel failed
    D_SFTP_ERROR_BAD_SEQUENCE      = 0x105,  // not open, or already open
    D_SFTP_ERROR_TOO_LARGE         = 0x106   // beyond a packet or buffer
};


// 2.3    Records
//------------------------------------------------------------------------------
// 2.3.1
// d_sftp_attributes
//   struct: a file's attributes. `flags` holds the D_SFTP_ATTR_* bits naming
// the fields that carry values; the others are zero. Times are seconds since
// the Unix epoch, as version 3 sends them.
struct d_sftp_attributes
{
    uint32_t flags;        // D_SFTP_ATTR_* present
    uint64_t size;         // bytes
    uint32_t uid;          // owner
    uint32_t gid;          // group
    uint32_t permissions;  // mode bits, file type included
    uint32_t atime;        // last access
    uint32_t mtime;        // last modification
};

// 2.3.2
// d_sftp_handle
//   struct: an open file or directory, as the server names it: opaque bytes,
// copied out of the packet that returned them.
struct d_sftp_handle
{
    uint32_t      length;                    // bytes used
    unsigned char bytes[D_SFTP_HANDLE_MAX];  // the handle
};

// 2.3.3
// d_sftp_name
//   struct: one directory entry or resolved path: the name, the server's
// `ls -l` style line for display (empty from some servers), and attributes.
// Both texts are views into the packet, valid while it is.
struct d_sftp_name
{
    struct d_pack_text       filename;    // the name
    struct d_pack_text       longname;    // for display only
    struct d_sftp_attributes attributes;  // what the server sent
};

// 2.3.4
// d_sftp_limits
//   struct: a limits@openssh.com reply; 0 means "no limit stated".
struct d_sftp_limits
{
    uint64_t packet_length;  // longest packet
    uint64_t read_length;    // longest READ reply
    uint64_t write_length;   // longest WRITE
    uint64_t open_handles;   // most handles open at once
};


// 2.4    Packets
//------------------------------------------------------------------------------
// 2.4.1
// d_sftp_frame
//   enum: whether bytes received hold a whole packet yet.
enum d_sftp_frame
{
    D_SFTP_FRAME_INCOMPLETE = 0,  // more bytes are needed
    D_SFTP_FRAME_COMPLETE   = 1,  // a whole packet is present
    D_SFTP_FRAME_MALFORMED  = 2   // its length is 0 or past the limit
};

// 2.4.2
// d_sftp_packet
//   struct: a packet parsed to its body: its type, its request id (the
// protocol version, for INIT and VERSION), and a reader positioned at the
// body, over the bytes it was parsed from.
struct d_sftp_packet
{
    uint8_t             type;  // D_SFTP_FXP_*
    uint32_t            id;    // request id, or the version
    struct d_ssh_reader body;  // what follows the id
};


//==============================================================================
// 3.  ERRORS
//==============================================================================


// 3.1    Error operations
//------------------------------------------------------------------------------
// d_sftp_error_string() describes an error in a short static English phrase,
// never NULL. d_sftp_error_from_status() maps a status code onto its error:
// D_SFTP_OK for OK, the same number for codes 1 to 8, and
// D_SFTP_ERROR_FAILURE for any code a later draft added.
const char*       d_sftp_error_string(enum d_sftp_error _error);
enum d_sftp_error d_sftp_error_from_status(uint32_t _code);


//==============================================================================
// 4.  ATTRIBUTES
//==============================================================================


// 4.1    Encoding
//------------------------------------------------------------------------------
// d_sftp_attributes_init() empties a record: no flags, every field zero.
void d_sftp_attributes_init(struct d_sftp_attributes* _attributes);
/**
 * @brief Reads an attribute record, skipping any extended pairs.
 *
 * @param[in,out] _reader     the reader, at the record.
 * @param[out]    _attributes the record; its unflagged fields are zeroed.
 * @return true; or false, marking the reader failed, for a truncated record
 *         or one with flag bits version 3 does not define, whose fields could
 *         not be skipped.
 */
bool d_sftp_read_attributes(struct d_ssh_reader*      _reader,
                            struct d_sftp_attributes* _attributes);
/**
 * @brief Appends an attribute record: the flagged fields only, never
 *        extended pairs.
 *
 * @param[in,out] _writer     the writer.
 * @param[in]     _attributes the record; NULL writes an empty one.
 * @return true; or false, marking the writer failed, for flags version 3
 *         does not define or a failed write.
 */
bool d_sftp_write_attributes(struct d_ssh_writer*            _writer,
                             const struct d_sftp_attributes* _attributes);


// 4.2    File types
//------------------------------------------------------------------------------
// Each reports whether a record's permissions name that file type; a record
// without D_SFTP_ATTR_PERMISSIONS names none.
bool d_sftp_attributes_is_directory(const struct d_sftp_attributes* _record);
bool d_sftp_attributes_is_regular(const struct d_sftp_attributes* _record);
bool d_sftp_attributes_is_symlink(const struct d_sftp_attributes* _record);


//==============================================================================
// 5.  PACKETS
//==============================================================================


// 5.1    Building
//------------------------------------------------------------------------------
/**
 * @brief Starts a packet: a placeholder length, the type, and the id.
 *
 * @param[in,out] _writer   the writer; the packet is appended to it.
 * @param[in]     _type     the packet type.
 * @param[in]     _id       the request id; the version for INIT.
 * @param[out]    _out_mark where the packet starts, for
 *                          d_sftp_packet_end().
 * @return true; or false, marking the writer failed.
 */
bool d_sftp_packet_begin(struct d_ssh_writer* _writer,
                         uint8_t              _type,
                         uint32_t             _id,
                         size_t*              _out_mark);
/**
 * @brief Finishes a packet by filling in its length.
 *
 * @param[in,out] _writer the writer holding the packet.
 * @param[in]     _mark   what d_sftp_packet_begin() returned.
 * @return true; or false, marking the writer failed, if the writer already
 *         failed, `_mark` is not a packet's start, or the packet is longer
 *         than D_SFTP_PACKET_MAX.
 */
bool d_sftp_packet_end(struct d_ssh_writer* _writer,
                       size_t               _mark);


// 5.2    Framing and parsing
//------------------------------------------------------------------------------
/**
 * @brief Reports whether received bytes begin with a whole packet.
 *
 * @param[in]  _data     the bytes, starting at a length field.
 * @param[in]  _size     how many there are.
 * @param[out] _out_size the packet's size with its length field, when
 *                       complete.
 * @return D_SFTP_FRAME_COMPLETE, D_SFTP_FRAME_INCOMPLETE, or
 *         D_SFTP_FRAME_MALFORMED for a length of 0 or beyond
 *         D_SFTP_PACKET_MAX, which is known from the first four bytes.
 */
enum d_sftp_frame d_sftp_packet_frame(const void* _data,
                                      size_t      _size,
                                      size_t*     _out_size);
/**
 * @brief Parses a packet's type and id, leaving a reader at its body.
 *
 * @param[in]  _data the packet after its length field; borrowed by the
 *                   reader.
 * @param[in]  _size its length.
 * @param[out] _out  the packet.
 * @return D_SFTP_OK; D_SFTP_ERROR_PROTOCOL for a packet too short to hold
 *         a type and an id; or D_SFTP_ERROR_INVALID_ARGUMENT for NULL
 *         arguments.
 */
enum d_sftp_error d_sftp_packet_parse(const void*           _data,
                                      size_t                _size,
                                      struct d_sftp_packet* _out);


//==============================================================================
// 6.  RESPONSES
//==============================================================================


// 6.1    Replies
//------------------------------------------------------------------------------
// Each reads one reply body from a packet's reader and returns D_SFTP_OK, or
// D_SFTP_ERROR_PROTOCOL for a malformed body (or trailing bytes, where the
// reply is complete), or D_SFTP_ERROR_INVALID_ARGUMENT for NULL arguments.
// A status's message and language are optional, as some servers omit them;
// the message view borrows the packet. A handle longer than
// D_SFTP_HANDLE_MAX is malformed. A NAME reply is read as its count, then
// one d_sftp_read_name() per entry.
enum d_sftp_error d_sftp_read_status(struct d_ssh_reader* _reader,
                                     uint32_t*            _out_code,
                                     struct d_pack_text*  _out_message);
enum d_sftp_error d_sftp_read_handle(struct d_ssh_reader*  _reader,
                                     struct d_sftp_handle* _out);
enum d_sftp_error d_sftp_read_data(struct d_ssh_reader* _reader,
                                   struct d_pack_bytes* _out);
enum d_sftp_error d_sftp_read_name(struct d_ssh_reader* _reader,
                                   struct d_sftp_name*  _out);


// 6.2    Version and extensions
//------------------------------------------------------------------------------
/**
 * @brief Reads a VERSION packet's extension pairs into D_SFTP_EXT_* bits.
 *
 * Names this client does not use are skipped; the version itself is the
 * packet's id.
 *
 * @param[in,out] _reader         the VERSION packet's body.
 * @param[out]    _out_extensions the bits of the extensions announced.
 * @return D_SFTP_OK, D_SFTP_ERROR_PROTOCOL for a malformed pair, or
 *         D_SFTP_ERROR_INVALID_ARGUMENT for NULL arguments.
 */
enum d_sftp_error d_sftp_read_extensions(struct d_ssh_reader* _reader,
                                         uint32_t*            _out_extensions);
/**
 * @brief Reads a limits@openssh.com reply's body.
 *
 * @param[in,out] _reader the EXTENDED_REPLY packet's body.
 * @param[out]    _out    the limits.
 * @return D_SFTP_OK, D_SFTP_ERROR_PROTOCOL, or
 *         D_SFTP_ERROR_INVALID_ARGUMENT for NULL arguments.
 */
enum d_sftp_error d_sftp_read_limits(struct d_ssh_reader*  _reader,
                                     struct d_sftp_limits* _out);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SFTP_SFTP_COMMON_H
