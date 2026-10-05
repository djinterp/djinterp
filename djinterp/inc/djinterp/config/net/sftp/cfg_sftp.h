/*******************************************************************************
* djinterp [config]                                                   cfg_sftp.h
*
* Configuration of the SFTP codec and client.
*   How many requests a transfer keeps in flight, how much one READ or WRITE
* moves by default and at most, the largest packet accepted or built, and
* how much of a status message is kept. Each is resolved into
* D_INTERNAL_SFTP_*, which sftp_common.h and sftp_client.h republish.
*   targets:  net/sftp/sftp_common.h -> D_INTERNAL_SFTP_PACKET_MAX,
*             D_INTERNAL_SFTP_CHUNK_SIZE, D_INTERNAL_SFTP_CHUNK_MAX,
*             D_INTERNAL_SFTP_MESSAGE_SIZE
*             net/sftp/sftp_client.h -> D_INTERNAL_SFTP_PIPELINE
*   requires: cfg_common.h
*   It includes no env header, so the include-order cycle that keeps
* cfg_tcp.h out of dconfig.h does not apply: registered there, it resolves
* cleanly. Registration is the config owner's edit.
*
*
* path:      /inc/djinterp/config/net/sftp/cfg_sftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Packets
         1.  D_CFG_SFTP_PACKET_MAX
         2.  D_CFG_SFTP_MESSAGE_SIZE
    2.  Transfers
         1.  D_CFG_SFTP_CHUNK_SIZE
         2.  D_CFG_SFTP_CHUNK_MAX
         3.  D_CFG_SFTP_PIPELINE
2.  RESOLVED VALUES
    ---------------
    1.  Read by the modules
         1.  D_INTERNAL_SFTP_PACKET_MAX
         2.  D_INTERNAL_SFTP_MESSAGE_SIZE
         3.  D_INTERNAL_SFTP_CHUNK_SIZE
         4.  D_INTERNAL_SFTP_CHUNK_MAX
         5.  D_INTERNAL_SFTP_PIPELINE
*/

#ifndef DJINTERP_CONFIG_NET_SFTP_CFG_SFTP_H
#define DJINTERP_CONFIG_NET_SFTP_CFG_SFTP_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins. Besides its own
// bounds, a chunk must fit a packet with room for the header around it.


// 1.1    Packets
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_SFTP_PACKET_MAX
//   knob: the longest packet accepted or built, its length field excluded;
// 34000 to 16777216. The default, 256 KiB, is OpenSSH's own limit. A client
// allocates this much and a little more for received data.
#ifndef D_CFG_SFTP_PACKET_MAX
    #define D_CFG_SFTP_PACKET_MAX 262144
#endif  // D_CFG_SFTP_PACKET_MAX

// 1.1.2
// D_CFG_SFTP_MESSAGE_SIZE
//   knob: bytes kept of a server's status message, its terminator included;
// 64 to 4096.
#ifndef D_CFG_SFTP_MESSAGE_SIZE
    #define D_CFG_SFTP_MESSAGE_SIZE 256
#endif  // D_CFG_SFTP_MESSAGE_SIZE

// 1.2    Transfers
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_SFTP_CHUNK_SIZE
//   knob: bytes per READ or WRITE when the server states no limits; 1024 to
// D_CFG_SFTP_CHUNK_MAX. The default, 32 KiB, is what every server accepts.
#ifndef D_CFG_SFTP_CHUNK_SIZE
    #define D_CFG_SFTP_CHUNK_SIZE 32768
#endif  // D_CFG_SFTP_CHUNK_SIZE

// 1.2.2
// D_CFG_SFTP_CHUNK_MAX
//   knob: the most one READ or WRITE moves, however high a server's
// limits@openssh.com reply goes; at least D_CFG_SFTP_CHUNK_SIZE, and 1024
// less than D_CFG_SFTP_PACKET_MAX at most.
#ifndef D_CFG_SFTP_CHUNK_MAX
    #define D_CFG_SFTP_CHUNK_MAX 65536
#endif  // D_CFG_SFTP_CHUNK_MAX

// 1.2.3
// D_CFG_SFTP_PIPELINE
//   knob: the most READ or WRITE requests a transfer keeps in flight; 1 to
// 64.
#ifndef D_CFG_SFTP_PIPELINE
    #define D_CFG_SFTP_PIPELINE 16
#endif  // D_CFG_SFTP_PIPELINE

#if ( (D_CFG_NORM(D_CFG_SFTP_PACKET_MAX) < 34000) ||                           \
      (D_CFG_NORM(D_CFG_SFTP_PACKET_MAX) > 16777216) )
    #error "cfg_sftp: D_CFG_SFTP_PACKET_MAX must lie in [34000, 16777216]"
#endif

#if ( (D_CFG_NORM(D_CFG_SFTP_MESSAGE_SIZE) < 64) ||                            \
      (D_CFG_NORM(D_CFG_SFTP_MESSAGE_SIZE) > 4096) )
    #error "cfg_sftp: D_CFG_SFTP_MESSAGE_SIZE must lie in [64, 4096]"
#endif

#if ( (D_CFG_NORM(D_CFG_SFTP_CHUNK_SIZE) < 1024) ||                            \
      (D_CFG_NORM(D_CFG_SFTP_CHUNK_SIZE) >                                     \
       D_CFG_NORM(D_CFG_SFTP_CHUNK_MAX)) )
    #error "cfg_sftp: D_CFG_SFTP_CHUNK_SIZE must lie in [1024, CHUNK_MAX]"
#endif

#if ( (D_CFG_NORM(D_CFG_SFTP_CHUNK_MAX) + 1024) >                              \
      D_CFG_NORM(D_CFG_SFTP_PACKET_MAX) )
    #error "cfg_sftp: D_CFG_SFTP_CHUNK_MAX must leave 1024 bytes of a packet"
#endif

#if ( (D_CFG_NORM(D_CFG_SFTP_PIPELINE) < 1) ||                                 \
      (D_CFG_NORM(D_CFG_SFTP_PIPELINE) > 64) )
    #error "cfg_sftp: D_CFG_SFTP_PIPELINE must lie in [1, 64]"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================
// What sftp_common.h and sftp_client.h read. Nothing outside this file
// resolves them.


// 2.1    Read by the modules
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_SFTP_PACKET_MAX
//   resolved: the packet limit, in bytes.
#define D_INTERNAL_SFTP_PACKET_MAX   D_CFG_NORM(D_CFG_SFTP_PACKET_MAX)

// 2.1.2
// D_INTERNAL_SFTP_MESSAGE_SIZE
//   resolved: the status message storage, in bytes.
#define D_INTERNAL_SFTP_MESSAGE_SIZE D_CFG_NORM(D_CFG_SFTP_MESSAGE_SIZE)

// 2.1.3
// D_INTERNAL_SFTP_CHUNK_SIZE
//   resolved: the default chunk, in bytes.
#define D_INTERNAL_SFTP_CHUNK_SIZE   D_CFG_NORM(D_CFG_SFTP_CHUNK_SIZE)

// 2.1.4
// D_INTERNAL_SFTP_CHUNK_MAX
//   resolved: the largest chunk, in bytes.
#define D_INTERNAL_SFTP_CHUNK_MAX    D_CFG_NORM(D_CFG_SFTP_CHUNK_MAX)

// 2.1.5
// D_INTERNAL_SFTP_PIPELINE
//   resolved: the requests kept in flight.
#define D_INTERNAL_SFTP_PIPELINE     D_CFG_NORM(D_CFG_SFTP_PIPELINE)


#endif  // DJINTERP_CONFIG_NET_SFTP_CFG_SFTP_H
