/*******************************************************************************
* djinterp [net]                                                net_tests_peer.h
*
* The scripted peer and the toy engine net's protocol suites run against.
*   The peer is a net.h connection scripted as the other party: it releases
* each of its turns once the session under test has sent the text the turn
* waits for, so a test reads as the exchange it checks and needs no thread.
* Behind it, the toy engine of the SSL kernel stands in for a real one: it
* shakes hands with nine bytes each way, shaped as TLS record headers so the
* kernel takes them for TLS, and seals data by rotating letters and digits,
* so sealed text stays readable and a peer can script it. Neither needs a
* TLS library.
*
*
* path:      /tests/djinterp/net/net_tests_peer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SCRIPTED PEER
    -------------
    1.  Constants
         1.  D_TESTS_NET_PEER_TURNS
         2.  D_TESTS_NET_PEER_TURN_BYTES
         3.  D_TESTS_NET_PEER_OUTPUT_BYTES
    2.  Types
         1.  d_tests_net_turn
         2.  d_tests_net_peer
    3.  Functions
2.  TOY ENGINE
    ----------
    1.  Constants
         1.  D_TESTS_NET_TOY_SERVER_NAME
         2.  D_TESTS_NET_TOY_HELLO
         3.  D_TESTS_NET_TOY_REPLY
         4.  D_TESTS_NET_TOY_CLOSE
    2.  Types
         1.  d_tests_net_toy
    3.  Functions
*/

#ifndef DJINTERP_NET_NET_TESTS_PEER_H
#define DJINTERP_NET_NET_TESTS_PEER_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"   // framework root
#include "../../../inc/djinterp/net/net_common.h"  // d_net_connection
#include "../../../inc/djinterp/net/ssl/ssl.h"  // the SSL kernel
// re_std
#include "../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SCRIPTED PEER
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_NET_PEER_TURNS
//   constant: the most turns a scripted peer takes.
#define D_TESTS_NET_PEER_TURNS 16

// 1.1.2
// D_TESTS_NET_PEER_TURN_BYTES
//   constant: the most bytes a peer sends in one turn.
#define D_TESTS_NET_PEER_TURN_BYTES 4096

// 1.1.3
// D_TESTS_NET_PEER_OUTPUT_BYTES
//   constant: the most bytes a session can send a scripted peer.
#define D_TESTS_NET_PEER_OUTPUT_BYTES 16384

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_tests_net_turn
//   struct: one turn of a scripted peer: the bytes it sends once the session
// has sent `after`. An empty `after` releases the turn with the one before
// it, or at once for the first.
struct d_tests_net_turn
{
    char          after[64];                         // what releases it
    unsigned char text[D_TESTS_NET_PEER_TURN_BYTES];  // what it sends
    size_t        length;                            // bytes in text
    size_t        taken;                             // bytes already read
};

// 1.2.2
// d_tests_net_peer
//   struct: a connection scripted as the other party. A read takes bytes
// from one released turn at most, so two turns never arrive together; with
// nothing released, a read is the end of the stream once every turn is read
// and the peer hangs up, and a timeout otherwise, as a peer that stalls
// would. Everything the session sends is kept in `output`; a write that
// would pass `fail_after` fails, as a reset connection's would if `resets`.
// `lies` makes a read claim a byte more than it was asked for, and `stalls`
// a write take nothing; `uncounted` makes available refuse to count. Every
// operation is counted.
struct d_tests_net_peer
{
    struct d_net_connection base;        // must stay first
    struct d_tests_net_turn turns[D_TESTS_NET_PEER_TURNS];
    size_t                  count;       // turns scripted
    size_t                  released;    // turns released
    size_t                  current;     // first turn not fully read
    size_t                  matched;     // output searched for `after`
    bool                    hangs_up;    // end of stream after the last
    unsigned char           output[D_TESTS_NET_PEER_OUTPUT_BYTES];
    size_t                  sent;        // bytes in output
    size_t                  recv_chunk;  // most bytes per read; 0: any
    size_t                  send_chunk;  // most bytes per write; 0: any
    size_t                  fail_after;  // output size that fails a write
    bool                    resets;      // that failure is a reset
    bool                    lies;        // reads claim too many bytes
    bool                    stalls;      // writes take nothing
    bool                    uncounted;   // available refuses
    int                     reads;       // calls to read
    int                     writes;      // calls to write
    int                     closes;      // calls to close
    int                     destroys;    // calls to destroy
};

// 1.3    Functions
//------------------------------------------------------------------------------
// the scripted peer -- made with nothing scripted, every limit off, and no
// hang-up; connection gives its connection, with or without available; say
// adds a turn, sealed if the toy engine will read it; `sent` compares
// everything the session has sent with `_expected`
struct d_tests_net_peer*
     d_tests_net_peer_new(void);
void d_tests_net_peer_free(struct d_tests_net_peer* _peer);
struct d_net_connection*
     d_tests_net_peer_connection(struct d_tests_net_peer* _peer,
                                 bool                     _counts);
bool d_tests_net_peer_say(struct d_tests_net_peer* _peer,
                          const char*              _after,
                          const char*              _text);
bool d_tests_net_peer_say_bytes(struct d_tests_net_peer* _peer,
                                const char*              _after,
                                const void*              _bytes,
                                size_t                   _length);
bool d_tests_net_peer_say_sealed(struct d_tests_net_peer* _peer,
                                 const char*              _after,
                                 const char*              _text);
bool d_tests_net_peer_sent(const struct d_tests_net_peer* _peer,
                           const char*                    _expected);


//==============================================================================
// 2.  TOY ENGINE
//==============================================================================


// 2.1    Constants
//------------------------------------------------------------------------------
// 2.1.1
// D_TESTS_NET_TOY_SERVER_NAME
//   constant: the name the toy engine's certificate carries, and so the name
// a scripted client session expects of its server.
#define D_TESTS_NET_TOY_SERVER_NAME "mail.example"

// 2.1.2
// D_TESTS_NET_TOY_HELLO
//   constant: the toy engine's whole handshake from a client: a handshake
// record's header, whose length reads "00", and the hello itself.
#define D_TESTS_NET_TOY_HELLO "\x16\x03\x01" "00TOY?"

// 2.1.3
// D_TESTS_NET_TOY_REPLY
//   constant: the toy engine's whole handshake from a server, likewise.
#define D_TESTS_NET_TOY_REPLY "\x16\x03\x03" "00TOY!"

// 2.1.4
// D_TESTS_NET_TOY_CLOSE
//   constant: the toy engine's close_notify, a byte no test text contains.
#define D_TESTS_NET_TOY_CLOSE "\x7f"

// 2.2    Types
//------------------------------------------------------------------------------
// 2.2.1
// d_tests_net_toy
//   struct: the toy engine's record: what it is told to do, and what was
// asked of it. Its client sends D_TESTS_NET_TOY_HELLO and expects
// D_TESTS_NET_TOY_REPLY; its server the reverse. A client's peer presents a
// certificate for `cert_name` -- D_TESTS_NET_TOY_SERVER_NAME when NULL --
// whose chain the toy reports as `verdict`, D_SSL_VERIFY_FLAG_NONE for a
// sound one; a server's peer presents none. With `alerts` set, its sessions
// report the two alerts it names, D_SSL_ALERT_NONE for either one not sent.
struct d_tests_net_toy
{
    enum d_ssl_role  role;              // set by its context
    enum d_ssl_error create_status;     // what session_create reports
    enum d_ssl_error handshake_status;  // a failure to report instead
    enum d_ssl_error read_status;       // a failure reads report instead
    const char*      cert_name;         // the peer certificate's name
    uint32_t         verdict;           // its chain, as the toy saw it
    bool             alerts;            // whether to report the two below
    int32_t          alert_sent;        // the alert this end sent
    int32_t          alert_received;    // the alert the peer sent
    char             last_host[64];     // the host a session checks
    char             last_sni[64];      // the name a session sends
    int              creates;           // calls to session_create
    int              destroys;          // sessions freed
    int              shutdowns;         // close_notify queued
    int              releases;          // contexts freed
};

// 2.3    Functions
//------------------------------------------------------------------------------
// the toy engine -- a context over `_toy` in `_role`, which the caller
// destroys with d_ssl_context_destroy, verifying as asked: a client checks
// its peer, and a server asks for a client certificate it will do without;
// and the toy's cipher, which is its own inverse
bool d_tests_net_toy_context(struct d_ssl_context*   _out,
                             struct d_tests_net_toy* _toy,
                             enum d_ssl_role         _role,
                             bool                    _verify);
void d_tests_net_toy_seal(char*       _out,
                          size_t      _capacity,
                          const char* _text);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_PEER_H
