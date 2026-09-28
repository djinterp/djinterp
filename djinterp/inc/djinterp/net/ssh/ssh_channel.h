/*******************************************************************************
* djinterp [net]                                                   ssh_channel.h
*
* SSH channels: the services that run inside an authenticated session.
*   A session channel takes exactly one of exec, a subsystem such as
* sftp, or a shell, optionally after a pseudo-terminal; a direct-tcpip
* channel carries a TCP connection the server opens on the caller's
* behalf. Each has separate stdout and stderr streams, and a session
* channel reports its program's exit status once closed.
*
*
* path:      /inc/djinterp/net/ssh/ssh_channel.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Channels
         1.  d_ssh_channel_state
         2.  d_ssh_channel
2.  CHANNELS
    --------
    1.  Opening
    2.  Requests
    3.  Data
    4.  Closing
*/

#ifndef DJINTERP_NET_SSH_SSH_CHANNEL_H
#define DJINTERP_NET_SSH_SSH_CHANNEL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./ssh_common.h"      // d_ssh_status
#include "./ssh_engine.h"      // d_ssh_stream
#include "./ssh_session.h"     // d_ssh_session


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Channels
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_channel_state
//   enum: where a channel is. CLOSED is also the state of a zero-initialized
// channel, which d_ssh_channel_free accepts.
enum d_ssh_channel_state
{
    D_SSH_CHANNEL_CLOSED = 0,  // not open, or closed
    D_SSH_CHANNEL_OPEN   = 1,  // open; requests and data allowed
    D_SSH_CHANNEL_FAILED = 2   // its session failed
};

// 1.1.2
// d_ssh_channel
//   struct: one channel of a session. The caller owns the storage and treats
// the fields as read-only. A session channel takes exactly one of exec,
// subsystem, or shell; `exit_status` is what the server reported by the time
// the channel closed.
struct d_ssh_channel
{
    struct d_ssh_session*    session;      // the session it runs in
    void*                    handle;       // engine channel
    enum d_ssh_channel_state state;        // where it is
    bool                     requested;    // exec, subsystem, or shell sent
    bool                     eof_sent;     // nothing more will be written
    int                      exit_status;  // from the server; -1 before close
};


//==============================================================================
// 2.  CHANNELS
//==============================================================================
// A channel needs an authenticated session. One not yet freed when the
// session is disconnected or destroyed is released with it. The two
// inbound streams share one flow-control window: a caller that reads one
// stream to its end while the other goes unread can stall once the unread
// data fills the window (libssh2's default is 2 MiB).


// 2.1    Opening
//------------------------------------------------------------------------------
/**
 * @brief Opens a session channel, the kind exec, subsystems, and shells run
 *        in.
 *
 * @param[in,out] _session  a D_SSH_STATE_AUTHENTICATED session.
 * @param[out]    _channel  receives the channel.
 * @return `D_SSH_OK`; `D_SSH_ERR_CHANNEL` if the server refused; a FATAL
 *         status; or `D_SSH_ERR_STATE` or `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_open(struct d_ssh_session* _session,
                   struct d_ssh_channel* _channel);
/**
 * @brief Opens a direct-tcpip channel: a TCP connection the server makes to
 *        `_host` and `_port` on the caller's behalf.
 *
 * @param[in,out] _session  a D_SSH_STATE_AUTHENTICATED session.
 * @param[in]     _host     the destination, as the server should resolve it.
 * @param[in]     _port     the destination port, 1 to 65535.
 * @param[out]    _channel  receives the channel, ready for data.
 * @return as d_ssh_channel_open; the server refuses where forwarding is
 *         disabled or the destination is unreachable.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_open_tcpip(struct d_ssh_session* _session,
                         const char*           _host,
                         unsigned int          _port,
                         struct d_ssh_channel* _channel);

// 2.2    Requests
//------------------------------------------------------------------------------
/**
 * @brief Requests a pseudo-terminal, before d_ssh_channel_shell or
 *        d_ssh_channel_exec.
 *
 * @param[in,out] _channel  an open session channel with no request made.
 * @param[in]     _term     the terminal type, such as "xterm".
 * @param[in]     _columns  its width in characters.
 * @param[in]     _rows     its height in characters.
 * @return `D_SSH_OK`; `D_SSH_ERR_CHANNEL` if the server refused;
 *         `D_SSH_ERR_UNSUPPORTED` if the engine has no pty operation; a
 *         FATAL status; or `D_SSH_ERR_STATE` or `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_request_pty(struct d_ssh_channel* _channel,
                          const char*           _term,
                          unsigned int          _columns,
                          unsigned int          _rows);
/**
 * @brief Runs a command on a session channel.
 *
 * @param[in,out] _channel  an open session channel with no request made.
 * @param[in]     _command  the command line, interpreted by the user's
 *                          shell on the server.
 * @return `D_SSH_OK`; `D_SSH_ERR_CHANNEL` if the server refused; a FATAL
 *         status; or `D_SSH_ERR_STATE` (a request was already made) or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_exec(struct d_ssh_channel* _channel,
                   const char*           _command);
/**
 * @brief Starts a subsystem, such as "sftp", on a session channel.
 *
 * @param[in,out] _channel  an open session channel with no request made.
 * @param[in]     _name     the subsystem's name.
 * @return as d_ssh_channel_exec.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_subsystem(struct d_ssh_channel* _channel,
                        const char*           _name);
/**
 * @brief Starts the user's login shell on a session channel.
 *
 * @param[in,out] _channel  an open session channel with no request made.
 * @return as d_ssh_channel_exec.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_shell(struct d_ssh_channel* _channel);

// 2.3    Data
//------------------------------------------------------------------------------
/**
 * @brief Reads what is available from one stream, waiting for at least one
 *        byte.
 *
 * @param[in,out] _channel   an open channel.
 * @param[in]     _stream    the stream to read.
 * @param[out]    _buffer    receives the bytes.
 * @param[in]     _capacity  its size, at least 1.
 * @param[out]    _received  receives the number of bytes read.
 * @return `D_SSH_OK`; `D_SSH_ERR_CLOSED` once the stream has ended; a FATAL
 *         status; or `D_SSH_ERR_STATE` or `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_read(struct d_ssh_channel* _channel,
                   enum d_ssh_stream     _stream,
                   void*                 _buffer,
                   size_t                _capacity,
                   size_t*               _received);
/**
 * @brief Writes all of `_data` to the channel, waiting as the server's
 *        window allows.
 *
 * @param[in,out] _channel  an open channel that has not sent EOF.
 * @param[in]     _data     the bytes; may be `NULL` when `_size` is 0.
 * @param[in]     _size     their length.
 * @return `D_SSH_OK`; `D_SSH_ERR_STATE` after EOF or if the channel is not
 *         open; `D_SSH_ERR_CHANNEL`; a FATAL status; or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_write(struct d_ssh_channel* _channel,
                    const void*           _data,
                    size_t                _size);
/**
 * @brief Tells the server nothing more will be written, as closing a pipe
 *        would; the channel stays readable. Sending it twice is harmless.
 *
 * @param[in,out] _channel  an open channel.
 * @return `D_SSH_OK`; a FATAL status; or `D_SSH_ERR_STATE` or
 *         `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_send_eof(struct d_ssh_channel* _channel);

// 2.4    Closing
//------------------------------------------------------------------------------
/**
 * @brief Closes the channel, waits for the server to close its side, and
 *        records the exit status the server reported.
 *
 * @param[in,out] _channel  the channel; closing it twice is harmless.
 * @return `D_SSH_OK`; a FATAL status, after which the channel is closed all
 *         the same; or `D_SSH_ERR_ARGUMENT`.
 */
D_NODISCARD enum d_ssh_status
d_ssh_channel_close(struct d_ssh_channel* _channel);
/**
 * @brief Returns the exit status the server reported for the command.
 *
 * @param[in] _channel  a closed channel.
 * @return the status; -1 if the channel is `NULL` or not yet closed.
 */
D_NODISCARD int
d_ssh_channel_exit_status(const struct d_ssh_channel* _channel);
/**
 * @brief Releases the channel, closed or not, leaving it closed and empty,
 *        with an exit status of -1.
 *
 * @pre its session's storage is still valid, though the session may have
 *      been disconnected or destroyed.
 *
 * @param[in,out] _channel  the channel, or `NULL`.
 */
void
d_ssh_channel_free(struct d_ssh_channel* _channel);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_CHANNEL_H
