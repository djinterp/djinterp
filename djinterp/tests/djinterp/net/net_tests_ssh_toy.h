/*******************************************************************************
* djinterp [net]                                             net_tests_ssh_toy.h
*
* The toy SSH engine net's SSH and SFTP suites run against.
*   It stands behind ssh_engine.h's table in place of an SSH library: a
* session exchanges nothing over its socket, presents an ed25519-shaped host
* key, and lets any user in, so the session layer above runs its real
* course to an authenticated session. Each channel either serves bytes a
* test scripted and keeps what it is sent, or relays both ways to a socket,
* where an in-process server can answer. A test can make the next of most
* calls fail with the status it chooses, placing each failure exactly, and
* every call is counted. No SSH library is needed, and a scripted channel
* needs no thread: its reads end where its script does.
*
*
* path:      /tests/djinterp/net/net_tests_ssh_toy.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TOY ENGINE
    ----------
    1.  Constants
         1.  D_TESTS_SSH_TOY_CHANNELS
         2.  D_TESTS_SSH_TOY_BYTES
    2.  Types
         1.  d_tests_ssh_toy_channel
         2.  d_tests_ssh_toy
    3.  Functions
*/

#ifndef DJINTERP_NET_NET_TESTS_SSH_TOY_H
#define DJINTERP_NET_NET_TESTS_SSH_TOY_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"           // framework root
#include "../../../inc/djinterp/net/ssh/ssh_engine.h"   // the engine table
#include "../../../inc/djinterp/net/ssh/ssh_session.h"  // d_ssh_session


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TOY ENGINE
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_SSH_TOY_CHANNELS
//   constant: the most channels one toy session opens.
#define D_TESTS_SSH_TOY_CHANNELS 4

// 1.1.2
// D_TESTS_SSH_TOY_BYTES
//   constant: the most bytes a scripted channel holds each way.
#define D_TESTS_SSH_TOY_BYTES 4096

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_tests_ssh_toy_channel
//   struct: one engine channel. With `relay` a descriptor, its standard
// output is what the descriptor reads and its data what is written to it,
// as much as either takes at once. Otherwise a read takes from `input`, at
// most `read_chunk` bytes (0: any), and is the stream's end once all of it
// is read; a write keeps at most `write_chunk` bytes (0: any) in `output`,
// and `echo` makes them readable too. Its standard error has ended from the
// start. A status other than D_SSH_OK in one of the `*_fails` is what the
// next such call returns, once; `lies` makes the next read claim success
// with `claims` bytes and read none, once.
struct d_tests_ssh_toy_channel
{
    int              relay;          // a descriptor both ways; -1: scripted
    unsigned char    input[D_TESTS_SSH_TOY_BYTES];
    size_t           input_size;     // bytes scripted
    size_t           input_taken;    // bytes already read
    size_t           read_chunk;     // most bytes per read; 0: any
    unsigned char    output[D_TESTS_SSH_TOY_BYTES];
    size_t           output_size;    // bytes written
    size_t           write_chunk;    // most bytes per write; 0: any
    bool             echo;           // what is written becomes readable
    enum d_ssh_error request_fails;  // the next request's failure
    enum d_ssh_error read_fails;     // the next read's failure
    enum d_ssh_error write_fails;    // the next write's failure
    enum d_ssh_error eof_fails;      // the next EOF's failure
    bool             lies;           // the next read claims success
    size_t           claims;         // with this many bytes
    int              exit_status;    // reported when it closes
    char             request[16];    // the last request's type
    char             value[16];      // and its value; "" for none
    bool             freed;          // channel_free released it
    int              requests;       // calls to channel_request
    int              reads;          // calls to channel_read
    int              writes;         // calls to channel_write
    int              eofs;           // calls to channel_send_eof
    int              closes;         // calls to channel_close
};

// 1.2.2
// d_tests_ssh_toy
//   struct: the engine's record of one session. Its channels are used in
// order, the first channel_open taking `channels[0]`, so a test sets up a
// channel before the session opens it; `open_fails`, other than D_SSH_OK,
// is what the next channel_open returns, once. The session's end is
// counted.
struct d_tests_ssh_toy
{
    struct d_tests_ssh_toy_channel channels[D_TESTS_SSH_TOY_CHANNELS];
    size_t                         opened;       // channels opened
    enum d_ssh_error               open_fails;   // the next open's failure
    int                            disconnects;  // calls to disconnect
    int                            destroys;     // calls to destroy
};

// 1.3    Functions
//------------------------------------------------------------------------------
// the toy -- init makes a toy with no channel opened, every channel
// scripted and empty, and nothing set to fail; engine is its table;
// connect starts `_session` over the toy on a socket of its own, owned by
// the session, accepting any host key, and reports whether the session
// came out authenticated, the session's to destroy either way; say adds
// bytes to a scripted channel's input
void d_tests_ssh_toy_init(struct d_tests_ssh_toy* _toy);
const struct d_ssh_engine_vtable*
     d_tests_ssh_toy_engine(void);
bool d_tests_ssh_toy_connect(struct d_tests_ssh_toy* _toy,
                             struct d_ssh_session*   _session);
bool d_tests_ssh_toy_say(struct d_tests_ssh_toy_channel* _channel,
                         const void*                     _data,
                         size_t                          _size);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_SSH_TOY_H
