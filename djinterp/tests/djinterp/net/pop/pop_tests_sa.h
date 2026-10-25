/*******************************************************************************
* djinterp [net]                                                  pop_tests_sa.h
*
* Standalone tests of the POP3 kernel where it meets net.h: the send
* helpers, the reader over a connection, and the errors a connection's
* become here.
*   The connections are net_memory.h's -- a memory stream playing a scripted
* peer, a pipe joining a client and a server in one thread -- and a script
* of this suite's own, whose reads report bytes and an error together, as
* net.h allows a backend to. Each test returns true when every assertion
* held.
*
*
* path:      /tests/djinterp/net/pop/pop_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  d_tests_pop_case
         2.  d_tests_pop_step
         3.  d_tests_pop_script
         4.  d_tests_pop_capture
    2.  Helpers
2.  TESTS
    -----
    1.  Errors and sending
    2.  Reading
    3.  The runner
*/

#ifndef DJINTERP_NET_POP_POP_TESTS_SA_H
#define DJINTERP_NET_POP_POP_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../inc/djinterp/net/net_memory.h"  // pipes, memory streams
#include "../../../../inc/djinterp/net/pop/pop.h"     // the kernel
#include "../net_tests_sa_assert.h"                   // asserts


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_pop_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_pop_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

// 1.1.2
// d_tests_pop_step
//   struct: what one step of a script delivers: `bytes`, a C string that may
// be empty, and `error`, reported with the last of them, or alone when there
// are none.
struct d_tests_pop_step
{
    const char*      bytes;
    enum d_net_error error;
};

// 1.1.3
// d_tests_pop_script
//   struct: a connection whose reads follow `steps` in order. A read that
// takes part of a step's bytes reports nothing; the read that takes the last
// of them reports the step's error. Past the last step, a read is the end of
// the stream. Writes are refused as CLOSED. `reads` counts the reads that
// reached the script.
struct d_tests_pop_script
{
    struct d_net_connection        base;    // must stay first
    const struct d_tests_pop_step* steps;   // the reads, in order
    size_t                         count;   // how many
    size_t                         at;      // the step being read
    size_t                         offset;  // bytes of it already read
    size_t                         reads;   // reads made
};

// 1.1.4
// d_tests_pop_capture
//   struct: a sink's storage, which takes what fits and refuses the rest.
struct d_tests_pop_capture
{
    char   data[1024];
    size_t length;
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// scripts -- init opens a script over `_count` steps, which it borrows
void d_tests_pop_script_init(struct d_tests_pop_script*     _script,
                             const struct d_tests_pop_step* _steps,
                             size_t                         _count);

// lines -- a command with one argument, or none for a NULL `_argument`, and
// a status line without a response code; each borrows its C strings
struct d_pop_command_line
     d_tests_pop_command(enum d_pop_command _command,
                         const char*        _argument);
struct d_pop_response
     d_tests_pop_response(enum d_pop_indicator _indicator,
                          const char*          _text);

// sinks and texts -- capture_sink wraps a capture, which capture_is compares
// with a C string; connection_sink writes all it is given to a connection,
// and takes nothing when the connection fails; text_is compares a span with
// a C string
struct d_pack_sink
     d_tests_pop_capture_sink(struct d_tests_pop_capture* _capture);
bool d_tests_pop_capture_is(const struct d_tests_pop_capture* _capture,
                            const char*                       _text);
struct d_pack_sink
     d_tests_pop_connection_sink(struct d_net_connection* _connection);
bool d_tests_pop_text_is(struct d_pack_text _text,
                         const char*        _expected);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Errors and sending
//------------------------------------------------------------------------------
bool d_tests_sa_pop_from_net(struct d_test_counter* _counter);
bool d_tests_sa_pop_send(struct d_test_counter* _counter);
bool d_tests_sa_pop_send_failures(struct d_test_counter* _counter);

// 2.2    Reading
//------------------------------------------------------------------------------
bool d_tests_sa_pop_read_pieces(struct d_test_counter* _counter);
bool d_tests_sa_pop_read_body(struct d_test_counter* _counter);
bool d_tests_sa_pop_read_strict(struct d_test_counter* _counter);
bool d_tests_sa_pop_read_stalls(struct d_test_counter* _counter);
bool d_tests_sa_pop_read_failures(struct d_test_counter* _counter);
bool d_tests_sa_pop_read_refusals(struct d_test_counter* _counter);
bool d_tests_sa_pop_session(struct d_test_counter* _counter);

// 2.3    The runner
//------------------------------------------------------------------------------
bool d_tests_sa_pop_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_POP_POP_TESTS_SA_H
