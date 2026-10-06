/*******************************************************************************
* djinterp [net]                                                 http_tests_sa.h
*
* Standalone tests of net/http/http.h, HTTP's semantics in C.
*   Each test is table-driven, so a case is one line and a failure names the
* case. Nothing here blocks or touches a socket.
*
*
* path:      /tests/djinterp/net/http/http_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  d_tests_http_case
2.  TESTS
    -----
    1.  Vocabulary
    2.  HTTP/1.1 heads
    3.  The suite
*/

#ifndef DJINTERP_NET_HTTP_HTTP_TESTS_SA_H
#define DJINTERP_NET_HTTP_HTTP_TESTS_SA_H 1

// djinterp
#include "../../../../inc/djinterp/net/http/http.h"             // the module
#include "../../../../inc/djinterp/test/c/test_standalone.h"  // asserts


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_http_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_http_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Vocabulary
//------------------------------------------------------------------------------
bool d_tests_sa_http_methods(struct d_test_counter* _counter);
bool d_tests_sa_http_versions(struct d_test_counter* _counter);
bool d_tests_sa_http_status(struct d_test_counter* _counter);
bool d_tests_sa_http_reasons(struct d_test_counter* _counter);
bool d_tests_sa_http_syntax(struct d_test_counter* _counter);
bool d_tests_sa_http_names(struct d_test_counter* _counter);
bool d_tests_sa_http_errors(struct d_test_counter* _counter);

// 2.2    HTTP/1.1 heads
//------------------------------------------------------------------------------
bool d_tests_sa_http1_heads(struct d_test_counter* _counter);
bool d_tests_sa_http1_parts(struct d_test_counter* _counter);
bool d_tests_sa_http1_incremental(struct d_test_counter* _counter);
bool d_tests_sa_http1_limits(struct d_test_counter* _counter);
bool d_tests_sa_http1_lookup(struct d_test_counter* _counter);

// 2.3    The suite
//------------------------------------------------------------------------------
bool d_tests_sa_http_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_HTTP_HTTP_TESTS_SA_H
