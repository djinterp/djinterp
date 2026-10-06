/*******************************************************************************
* djinterp [net]                                          net_tests_sa_support.h
*
* The doubles the net foundation tests run against.
*   A scripted in-memory connection that can cut reads and writes short, fail
* on cue, and count its lifecycle calls, and a bounded sink. Free of the test
* runtime, so the C++ tests in net_tests.cpp drive the same connection through
* the bridges of net/net.hpp.
*
*
* path:      /tests/djinterp/net/net_tests_sa_support.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_SA_SUPPORT_H
#define DJINTERP_NET_NET_TESTS_SA_SUPPORT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../inc/djinterp/net/net.h"  // foundation


D_EXTERN_C_BEGIN


// d_tests_net_mock
//   struct: a scripted connection over caller memory. Reads deliver `input`,
// at most `read_limit` bytes at a time, then report `read_error` -- the end of
// the stream when that is D_NET_ERROR_NONE. Writes land in `output`, at most
// `write_limit` bytes at a time, and report `write_error` once it is full. A
// limit of 0 means none. The counters record every lifecycle call.
struct d_tests_net_mock
{
    struct d_net_connection base;             // must stay first
    const unsigned char*    input;            // the bytes reads deliver
    size_t                  input_size;       // how many there are
    size_t                  input_at;         // how many were delivered
    size_t                  read_limit;       // most bytes per read
    enum d_net_error        read_error;       // reported after the input
    unsigned char*          output;           // where writes land
    size_t                  output_capacity;  // its size
    size_t                  output_length;    // how much was written
    size_t                  write_limit;      // most bytes per write
    enum d_net_error        write_error;      // reported once it is full
    size_t                  writes;           // writes that moved bytes
    bool                    open;             // cleared by close
    size_t                  closes;           // close calls
    size_t                  shutdowns;        // shutdown calls
    enum d_net_shutdown     last_shutdown;    // the latest mode
    size_t                  destroys;         // destroy calls
};

// d_tests_net_sink_buffer
//   struct: a sink's fixed storage. The sink takes what fits and no more.
struct d_tests_net_sink_buffer
{
    unsigned char* data;      // the storage
    size_t         capacity;  // its size
    size_t         length;    // how much was taken
};

// doubles -- d_tests_net_mock_init opens a mock with no input or output. A
// full mock supplies every optional operation: shutdown, which it records; a
// remote endpoint of 192.0.2.1:80; a local endpoint it refuses after
// scribbling on the output; and destroy, which it counts. A minimal one
// supplies only the four required operations. d_tests_net_sink wraps a
// buffer as a sink; d_tests_net_pattern fills memory with bytes that differ
// from one offset to the next.
void               d_tests_net_mock_init(struct d_tests_net_mock* _mock,
                                         bool                     _full);
void               d_tests_net_mock_input(struct d_tests_net_mock* _mock,
                                          const void*              _data,
                                          size_t                   _size);
void               d_tests_net_mock_output(struct d_tests_net_mock* _mock,
                                           void*                    _buffer,
                                           size_t                   _capacity);
struct d_pack_sink d_tests_net_sink(struct d_tests_net_sink_buffer* _buffer);
void               d_tests_net_pattern(unsigned char* _buffer,
                                       size_t         _size,
                                       unsigned int   _seed);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_SA_SUPPORT_H
