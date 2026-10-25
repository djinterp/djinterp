/*******************************************************************************
* djinterp [net]                                                 imap_tests_sa.h
*
* Standalone tests of the IMAP kernel: its vocabulary, its writers and
* parsers, and both sides of a session run through them.
*   imap.h does no I/O: it names, writes and parses what an IMAP client and
* server exchange. These tests hold it to its header's contracts and to the
* grammar and examples of RFC 9051 and RFC 3501 -- every name and lookup, the
* octets each writer produces, the fields each parser fills and the input it
* refuses -- and run a scripted session whose client lines the writers build
* and whose server lines the parsers read, over a memory stream that delivers
* them a few octets at a time. Each test returns true when every assertion
* held.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  d_tests_imap_case
         2.  d_tests_imap_capture
    2.  Helpers
2.  TESTS
    -----
    1.  Vocabulary
    2.  Tags and options
    3.  Strings and mailbox names
    4.  Response data
    5.  Sequence sets and dates
    6.  Responses and the lexer
    7.  A session
    8.  The runner
*/

#ifndef DJINTERP_NET_IMAP_IMAP_TESTS_SA_H
#define DJINTERP_NET_IMAP_IMAP_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // sinks, spans
#include "../../../../inc/djinterp/net/imap/imap.h"       // the kernel
#include "../net_tests_sa_assert.h"                       // asserts


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_imap_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_imap_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

// 1.1.2
// d_tests_imap_capture
//   struct: a sink's storage. It keeps the first `limit` octets it is given,
// `limit` being at most the size of `data`, and refuses every octet after
// them, so that a writer can be stopped at any octet of its output.
struct d_tests_imap_capture
{
    char   data[1024];
    size_t length;  // octets kept
    size_t limit;   // octets it will keep
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// captures -- init empties a capture and sets its limit, held to the size of
// `data`; capture_sink writes into one; capture_is compares what it holds
// with a C string, octet for octet
void d_tests_imap_capture_init(struct d_tests_imap_capture* _capture,
                               size_t                       _limit);
struct d_pack_sink
     d_tests_imap_capture_sink(struct d_tests_imap_capture* _capture);
bool d_tests_imap_capture_is(const struct d_tests_imap_capture* _capture,
                             const char*                        _text);

// spans -- span_is compares a span with a C string; a span without data, or
// a NULL string, is empty
bool d_tests_imap_span_is(struct d_pack_text _span,
                          const char*        _text);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Vocabulary
//------------------------------------------------------------------------------
bool d_tests_sa_imap_errors(struct d_test_counter* _counter);
bool d_tests_sa_imap_names(struct d_test_counter* _counter);
bool d_tests_sa_imap_command_states(struct d_test_counter* _counter);

// 2.2    Tags and options
//------------------------------------------------------------------------------
bool d_tests_sa_imap_tags(struct d_test_counter* _counter);
bool d_tests_sa_imap_options(struct d_test_counter* _counter);
bool d_tests_sa_imap_auth_select(struct d_test_counter* _counter);

// 2.3    Strings and mailbox names
//------------------------------------------------------------------------------
bool d_tests_sa_imap_astring(struct d_test_counter* _counter);
bool d_tests_sa_imap_quoted(struct d_test_counter* _counter);
bool d_tests_sa_imap_literal(struct d_test_counter* _counter);
bool d_tests_sa_imap_mutf7(struct d_test_counter* _counter);
bool d_tests_sa_imap_mailbox(struct d_test_counter* _counter);
bool d_tests_sa_imap_sink_refusals(struct d_test_counter* _counter);

// 2.4    Response data
//------------------------------------------------------------------------------
bool d_tests_sa_imap_capabilities(struct d_test_counter* _counter);
bool d_tests_sa_imap_flags(struct d_test_counter* _counter);
bool d_tests_sa_imap_mailbox_attributes(struct d_test_counter* _counter);
bool d_tests_sa_imap_status(struct d_test_counter* _counter);
bool d_tests_sa_imap_search(struct d_test_counter* _counter);
bool d_tests_sa_imap_list(struct d_test_counter* _counter);

// 2.5    Sequence sets and dates
//------------------------------------------------------------------------------
bool d_tests_sa_imap_seqset(struct d_test_counter* _counter);
bool d_tests_sa_imap_dates(struct d_test_counter* _counter);
bool d_tests_sa_imap_date_parsing(struct d_test_counter* _counter);

// 2.6    Responses and the lexer
//------------------------------------------------------------------------------
bool d_tests_sa_imap_responses(struct d_test_counter* _counter);
bool d_tests_sa_imap_response_refusals(struct d_test_counter* _counter);
bool d_tests_sa_imap_lexer(struct d_test_counter* _counter);
bool d_tests_sa_imap_lexer_incomplete(struct d_test_counter* _counter);
bool d_tests_sa_imap_decode(struct d_test_counter* _counter);

// 2.7    A session
//------------------------------------------------------------------------------
bool d_tests_sa_imap_session_client(struct d_test_counter* _counter);
bool d_tests_sa_imap_session_stream(struct d_test_counter* _counter);

// 2.8    The runner
//------------------------------------------------------------------------------
bool d_tests_sa_imap_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_IMAP_IMAP_TESTS_SA_H
