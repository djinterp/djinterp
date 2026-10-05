/*******************************************************************************
* djinterp [net]                                          sftp_client_internal.h
*
* Private helpers shared by the two halves of the SFTP client.
*   sftp_client.c holds the session and every single-request operation, and
* sftp_transfer.c the pipelined transfers. The transfers receive packets,
* read statuses, and send READ and WRITE requests through the helpers
* declared here, which sftp_client.c defines. Not installed, and not part of
* the public interface.
*
*
* path:      /src/djinterp/net/sftp/sftp_client_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PACKETS
    -------
    1.  Receiving
    2.  Requests
2.  OPERATIONS
    ----------
    1.  Preconditions
*/

#ifndef DJINTERP_NET_SFTP_SFTP_CLIENT_INTERNAL_H
#define DJINTERP_NET_SFTP_SFTP_CLIENT_INTERNAL_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"               // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"       // d_pack_bytes
#include "../../../../inc/djinterp/net/sftp/sftp_client.h"     // d_sftp_client
#include "../../../../inc/djinterp/net/sftp/sftp_common.h"     // d_sftp_packet


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in sftp_client.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  PACKETS
//==============================================================================


// 1.1    Receiving
//------------------------------------------------------------------------------
// d_sftp_internal_client_receive() reads the next whole packet, whose reader
// borrows the client's buffer until the next call; a length past the limit
// is D_SFTP_ERROR_PROTOCOL, and a failed stream D_SFTP_ERROR_TRANSPORT or
// D_SFTP_ERROR_CONNECTION_LOST. d_sftp_internal_client_status() reads a
// STATUS reply, keeping its code and message, and returns the error it
// means; a packet that is no well-formed STATUS is D_SFTP_ERROR_PROTOCOL.
enum d_sftp_error d_sftp_internal_client_receive(
                      struct d_sftp_client* _client,
                      struct d_sftp_packet* _out);
enum d_sftp_error d_sftp_internal_client_status(
                      struct d_sftp_client* _client,
                      struct d_sftp_packet* _packet);


// 1.2    Requests
//------------------------------------------------------------------------------
// Each sends one request under the next id, stored in `*_out_id`: a READ of
// `_length` bytes at `_offset`, or a WRITE of `_data` there. A request past
// the packet limit is D_SFTP_ERROR_TOO_LARGE, one the writer could not
// build D_SFTP_ERROR_MEMORY, and a failed stream as for receiving.
enum d_sftp_error d_sftp_internal_client_read_request(
                      struct d_sftp_client*       _client,
                      const struct d_sftp_handle* _handle,
                      uint64_t                    _offset,
                      uint32_t                    _length,
                      uint32_t*                   _out_id);
enum d_sftp_error d_sftp_internal_client_write_request(
                      struct d_sftp_client*       _client,
                      const struct d_sftp_handle* _handle,
                      uint64_t                    _offset,
                      struct d_pack_bytes         _data,
                      uint32_t*                   _out_id);


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Preconditions
//------------------------------------------------------------------------------
// d_sftp_internal_client_ready() is the check every operation starts with:
// D_SFTP_ERROR_INVALID_ARGUMENT for a NULL client or pointer, then
// D_SFTP_ERROR_BAD_SEQUENCE for a client not open.
enum d_sftp_error d_sftp_internal_client_ready(
                      const struct d_sftp_client* _client,
                      const void*                 _first,
                      const void*                 _second);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SFTP_SFTP_CLIENT_INTERNAL_H
