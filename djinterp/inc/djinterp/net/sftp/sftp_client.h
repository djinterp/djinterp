/*******************************************************************************
* djinterp [net]                                                   sftp_client.h
*
* An SFTP client: files and directories on an SSH server.
*   The client speaks SFTP version 3 over the "sftp" subsystem of a channel on
* an authenticated d_ssh_session, so host-key checking and authentication
* stay with the SSH module and no file request ever reaches an unverified
* server. It runs just as well over any byte stream a d_sftp_transport
* describes, which is how an in-process server can stand in for SSH.
*   d_sftp_client_get() and d_sftp_client_put() keep D_SFTP_CLIENT_PIPELINE
* requests in flight: waiting out a round trip per request would move one
* chunk per round trip. Replies are matched to requests by id, so a server
* that answers out of order, or returns less than was asked, still yields
* the file whole and in order. Chunk sizes follow the server's
* limits@openssh.com reply where it sends one.
*   Every call blocks. A client allocates its receive buffer on opening and
* frees it on closing; the SSH session's deadline bounds each wait.
*
*
* path:      /inc/djinterp/net/sftp/sftp_client.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Client sizes
         1.  D_SFTP_CLIENT_PIPELINE
         2.  D_SFTP_CLIENT_BUFFER_SIZE
2.  TYPES
    -----
    1.  Callbacks
         1.  d_sftp_sink_fn
         2.  d_sftp_source_fn
         3.  d_sftp_entry_fn
    2.  Transports
         1.  d_sftp_read_fn
         2.  d_sftp_write_fn
         3.  d_sftp_transport
    3.  Clients
         1.  d_sftp_client
3.  CLIENT
    ------
    1.  Lifetime
    2.  Paths and attributes
    3.  Directories
    4.  Open files
    5.  Whole files
*/

#ifndef DJINTERP_NET_SFTP_SFTP_CLIENT_H
#define DJINTERP_NET_SFTP_SFTP_CLIENT_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t
// djinterp
#include "../../c/djinterp.h"      // framework root
#include "../ssh/ssh_channel.h"    // d_ssh_channel
#include "../ssh/ssh_common.h"     // d_ssh_status
#include "../ssh/ssh_session.h"    // d_ssh_session
#include "../ssh/ssh_wire.h"       // d_ssh_writer
#include "./sftp_common.h"         // d_sftp_error, d_sftp_attributes


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in sftp_client.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Client sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_SFTP_CLIENT_PIPELINE
//   constant: the most READ or WRITE requests a transfer keeps in flight:
// 16, unless D_CFG_SFTP_PIPELINE says otherwise.
#define D_SFTP_CLIENT_PIPELINE    ((size_t)D_INTERNAL_SFTP_PIPELINE)

// 1.1.2
// D_SFTP_CLIENT_BUFFER_SIZE
//   constant: bytes allocated for received data: the largest packet, its
// length field, and room to read ahead.
#define D_SFTP_CLIENT_BUFFER_SIZE (D_SFTP_PACKET_MAX + 16384u)


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Callbacks
//------------------------------------------------------------------------------
// 2.1.1
// d_sftp_sink_fn
//   function pointer: takes the next `_size` (at least 1) bytes of a
// download. Returns D_SFTP_OK to go on, or any error to abandon the transfer,
// which then returns that error.
typedef enum d_sftp_error (*d_sftp_sink_fn)(void*       _context,
                                            const void* _data,
                                            size_t      _size);

// 2.1.2
// d_sftp_source_fn
//   function pointer: supplies up to `_capacity` bytes of an upload in
// `_buffer`, storing the count in `*_out_size`; 0 ends the file. Returns
// D_SFTP_OK, or any error to abandon the transfer.
typedef enum d_sftp_error (*d_sftp_source_fn)(void*   _context,
                                              void*   _buffer,
                                              size_t  _capacity,
                                              size_t* _out_size);

// 2.1.3
// d_sftp_entry_fn
//   function pointer: takes one directory entry, whose texts are valid only
// for the call. Returns D_SFTP_OK to go on, or any error to stop the
// listing, which then returns that error.
typedef enum d_sftp_error (*d_sftp_entry_fn)(void*                     _context,
                                             const struct d_sftp_name* _entry);


// 2.2    Transports
//------------------------------------------------------------------------------
// 2.2.1
// d_sftp_read_fn
//   function pointer: reads what is available, waiting for at least one
// byte, as d_ssh_channel_read() does: D_SSH_OK with a count of at least 1,
// D_SSH_ERR_CLOSED at the end of the stream, or another failure.
typedef enum d_ssh_status (*d_sftp_read_fn)(void*   _context,
                                            void*   _buffer,
                                            size_t  _capacity,
                                            size_t* _out_received);

// 2.2.2
// d_sftp_write_fn
//   function pointer: writes all `_size` bytes, as d_ssh_channel_write()
// does: D_SSH_OK, or a failure.
typedef enum d_ssh_status (*d_sftp_write_fn)(void*       _context,
                                             const void* _data,
                                             size_t      _size);

// 2.2.3
// d_sftp_transport
//   struct: the byte stream a client speaks over. Passed by value; the
// context must outlive the client's use of it.
struct d_sftp_transport
{
    d_sftp_read_fn  read;     // reads, waiting for at least a byte
    d_sftp_write_fn write;    // writes everything
    void*           context;  // passed to both
};


// 2.3    Clients
//------------------------------------------------------------------------------
// 2.3.1
// d_sftp_client
//   struct: one SFTP session. `extensions`, `read_size`, `write_size`, and
// the three diagnostics -- `ssh_status` behind D_SFTP_ERROR_TRANSPORT, and
// the code and message of the last status the server sent -- may be read
// freely; the other fields are the client's own. Do not copy a client once
// it is open: its transport points into it.
struct d_sftp_client
{
    struct d_sftp_transport transport;     // the stream
    struct d_ssh_channel    channel;       // when opened on a session
    bool                    owns_channel;  // the channel is ours to close
    bool                    open;          // the version exchange is done
    uint32_t                version;       // the version spoken, 3
    uint32_t                extensions;    // D_SFTP_EXT_* announced
    uint32_t                next_id;       // the next request id
    uint32_t                read_size;     // bytes per READ
    uint32_t                write_size;    // bytes per WRITE
    unsigned char*          buffer;        // received bytes; owned
    size_t                  buffer_start;  // first byte not parsed
    size_t                  buffer_end;    // end of the bytes received
    struct d_ssh_writer     writer;        // builds requests
    enum d_ssh_status       ssh_status;    // cause of ERROR_TRANSPORT
    uint32_t                status_code;   // the last status received
    char                    status_message[D_SFTP_MESSAGE_SIZE];
};


//==============================================================================
// 3.  CLIENT
//==============================================================================


// 3.1    Lifetime
//------------------------------------------------------------------------------
// d_sftp_client_init() prepares a closed client, and must come first; NULL
// is ignored. d_sftp_client_close() sends EOF on a channel the client
// opened and closes it, frees the client's memory, and leaves it closed and
// reusable; closing twice, or a client never opened, is harmless.
void d_sftp_client_init(struct d_sftp_client* _client);
void d_sftp_client_close(struct d_sftp_client* _client);
/**
 * @brief Opens a channel on a session, starts the "sftp" subsystem, and
 *        agrees on the protocol version.
 *
 * @param[in,out] _client  a closed client.
 * @param[in,out] _session an authenticated session; it must outlive the
 *                         client's use of it.
 * @post   On success the client is open over a channel it owns. On failure
 *         it is closed, with the channel released.
 * @return D_SFTP_OK; D_SFTP_ERROR_UNSUPPORTED if the server refused the
 *         subsystem; D_SFTP_ERROR_TRANSPORT for any other channel failure,
 *         with the SSH status in `ssh_status`; a failure of
 *         d_sftp_client_open_transport(); D_SFTP_ERROR_BAD_SEQUENCE for a
 *         client already open; or D_SFTP_ERROR_INVALID_ARGUMENT for NULL
 *         arguments.
 */
enum d_sftp_error d_sftp_client_open(struct d_sftp_client* _client,
                                     struct d_ssh_session* _session);
/**
 * @brief Agrees on the protocol version over a stream the caller provides.
 *
 * Sends INIT for version 3 and reads VERSION; a server offering less than
 * 3 is refused. Where the server announces limits@openssh.com, its limits
 * set the chunk sizes, capped at D_SFTP_CHUNK_MAX.
 *
 * @param[in,out] _client    a closed client.
 * @param[in]     _transport the stream, with both callbacks.
 * @return D_SFTP_OK; D_SFTP_ERROR_VERSION; D_SFTP_ERROR_PROTOCOL for a
 *         reply that is not a well-formed VERSION; D_SFTP_ERROR_MEMORY;
 *         D_SFTP_ERROR_TRANSPORT or D_SFTP_ERROR_CONNECTION_LOST for a
 *         failed stream; D_SFTP_ERROR_BAD_SEQUENCE for a client already open;
 *         or D_SFTP_ERROR_INVALID_ARGUMENT for a NULL client or a transport
 *         missing a callback.
 */
enum d_sftp_error d_sftp_client_open_transport(
                      struct d_sftp_client*   _client,
                      struct d_sftp_transport _transport);


// 3.2    Paths and attributes
//------------------------------------------------------------------------------
// Each needs an open client and returns D_SFTP_OK or the first failure: the
// server's status (D_SFTP_ERROR_NO_SUCH_FILE, _PERMISSION_DENIED, _FAILURE,
// and the rest, with `status_code` and `status_message` kept);
// D_SFTP_ERROR_PROTOCOL for a reply of the wrong kind or shape;
// D_SFTP_ERROR_TRANSPORT or _CONNECTION_LOST for a failed stream;
// D_SFTP_ERROR_TOO_LARGE for a path beyond the packet limit, or a result
// beyond `_capacity`; D_SFTP_ERROR_BAD_SEQUENCE for a closed client; or
// D_SFTP_ERROR_INVALID_ARGUMENT for NULL arguments, or attributes with
// flags version 3 cannot send. Results that are paths are NUL-terminated.
//   d_sftp_client_stat() follows symbolic links and d_sftp_client_lstat()
// does not. d_sftp_client_rename() with `_replace` replaces an existing
// target atomically through posix-rename@openssh.com, and returns
// D_SFTP_ERROR_UNSUPPORTED where the server lacks it; without, version 3's
// RENAME refuses to replace. d_sftp_client_symlink() sends its paths in the
// order OpenSSH's server reads them -- target, then link -- which is the
// reverse of draft 02's and what every deployed server expects.
enum d_sftp_error d_sftp_client_realpath(struct d_sftp_client* _client,
                                         const char*           _path,
                                         char*                 _out,
                                         size_t                _capacity);
enum d_sftp_error d_sftp_client_stat(struct d_sftp_client*     _client,
                                     const char*               _path,
                                     struct d_sftp_attributes* _out);
enum d_sftp_error d_sftp_client_lstat(struct d_sftp_client*     _client,
                                      const char*               _path,
                                      struct d_sftp_attributes* _out);
enum d_sftp_error d_sftp_client_setstat(
                      struct d_sftp_client*           _client,
                      const char*                     _path,
                      const struct d_sftp_attributes* _attributes);
enum d_sftp_error d_sftp_client_remove(struct d_sftp_client* _client,
                                       const char*           _path);
enum d_sftp_error d_sftp_client_rename(struct d_sftp_client* _client,
                                       const char*           _from,
                                       const char*           _to,
                                       bool                  _replace);
enum d_sftp_error d_sftp_client_readlink(struct d_sftp_client* _client,
                                         const char*           _path,
                                         char*                 _out,
                                         size_t                _capacity);
enum d_sftp_error d_sftp_client_symlink(struct d_sftp_client* _client,
                                        const char*           _target,
                                        const char*           _link);


// 3.3    Directories
//------------------------------------------------------------------------------
// These return as the functions of 3.2 do. d_sftp_client_mkdir() takes the
// new directory's attributes, or NULL for the server's defaults.
// d_sftp_client_list() passes each entry to `_callback`, "." and ".."
// included where the server sends them, and returns D_SFTP_OK once the
// server reports the directory's end; a callback's error stops it, and is
// returned, after the directory is closed.
enum d_sftp_error d_sftp_client_mkdir(
                      struct d_sftp_client*           _client,
                      const char*                     _path,
                      const struct d_sftp_attributes* _attributes);
enum d_sftp_error d_sftp_client_rmdir(struct d_sftp_client* _client,
                                      const char*           _path);
enum d_sftp_error d_sftp_client_list(struct d_sftp_client* _client,
                                     const char*           _path,
                                     d_sftp_entry_fn       _callback,
                                     void*                 _context);


// 3.4    Open files
//------------------------------------------------------------------------------
// These return as the functions of 3.2 do. d_sftp_client_open_file() takes
// D_SFTP_OPEN_* flags and, for a file it creates, attributes or NULL; the
// handle it fills must be closed with d_sftp_client_close_file(), whose
// status is where some servers report a failed write. d_sftp_client_read()
// makes one request of at most `read_size` bytes and returns
// D_SFTP_ERROR_EOF at the end of the file; it may return fewer bytes than
// asked. d_sftp_client_write() writes all of `_data`, pipelined in chunks
// of `write_size`. d_sftp_client_fsync() needs fsync@openssh.com and
// returns D_SFTP_ERROR_UNSUPPORTED without it.
enum d_sftp_error d_sftp_client_open_file(
                      struct d_sftp_client*           _client,
                      const char*                     _path,
                      uint32_t                        _flags,
                      const struct d_sftp_attributes* _attributes,
                      struct d_sftp_handle*           _out);
enum d_sftp_error d_sftp_client_close_file(
                      struct d_sftp_client*       _client,
                      const struct d_sftp_handle* _handle);
enum d_sftp_error d_sftp_client_read(struct d_sftp_client*       _client,
                                     const struct d_sftp_handle* _handle,
                                     uint64_t                    _offset,
                                     void*                       _buffer,
                                     size_t                      _capacity,
                                     size_t*                     _out_read);
enum d_sftp_error d_sftp_client_write(struct d_sftp_client*       _client,
                                      const struct d_sftp_handle* _handle,
                                      uint64_t                    _offset,
                                      const void*                 _data,
                                      size_t                      _size);
enum d_sftp_error d_sftp_client_fstat(struct d_sftp_client*       _client,
                                      const struct d_sftp_handle* _handle,
                                      struct d_sftp_attributes*   _out);
enum d_sftp_error d_sftp_client_fsync(struct d_sftp_client*       _client,
                                      const struct d_sftp_handle* _handle);


// 3.5    Whole files
//------------------------------------------------------------------------------
/**
 * @brief Downloads a file into a sink, D_SFTP_CLIENT_PIPELINE reads at a
 *        time.
 *
 * Data reaches the sink in file order however the server orders its
 * replies, and a short read is completed with a further request. The file
 * ends at the first offset the server reports EOF for.
 *
 * @param[in,out] _client  an open client.
 * @param[in]     _path    the file.
 * @param[in]     _sink    takes the data.
 * @param[in]     _context passed to `_sink`.
 * @return D_SFTP_OK once the whole file has reached the sink and the handle
 *         has closed; the sink's error; D_SFTP_ERROR_MEMORY if a reply
 *         arriving out of order could not be held; or as the functions of
 *         3.2 do.
 */
enum d_sftp_error d_sftp_client_get(struct d_sftp_client* _client,
                                    const char*           _path,
                                    d_sftp_sink_fn        _sink,
                                    void*                 _context);
/**
 * @brief Uploads a file from a source, creating or truncating it,
 *        D_SFTP_CLIENT_PIPELINE writes at a time.
 *
 * @param[in,out] _client     an open client.
 * @param[in]     _path       the file.
 * @param[in]     _attributes for a file created, its attributes, such as
 *                            permissions; NULL for the server's defaults.
 * @param[in]     _source     supplies the data.
 * @param[in]     _context    passed to `_source`.
 * @return D_SFTP_OK once every write is acknowledged and the handle has
 *         closed; the source's error; D_SFTP_ERROR_INVALID_ARGUMENT for a
 *         source that overfilled its buffer; or as the functions of 3.2 do.
 */
enum d_sftp_error d_sftp_client_put(
                      struct d_sftp_client*           _client,
                      const char*                     _path,
                      const struct d_sftp_attributes* _attributes,
                      d_sftp_source_fn                _source,
                      void*                           _context);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SFTP_SFTP_CLIENT_H
