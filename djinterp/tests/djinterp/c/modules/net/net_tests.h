/*******************************************************************************
* djinterp [c][net]                                                  net_tests.h
*
* The tests of net's foundation, in DTest's C face.
*   Declarations for the tests of net/net.h. The doubles they run against
* are declared in net_tests_support.h.
*
*
* path:      /tests/djinterp/c/modules/net/net_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_H
#define DJINTERP_NET_NET_TESTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "./net_tests_assert.h"   // asserts
#include "./net_tests_support.h"  // doubles
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


// vocabulary and addresses -- each returns true when every assertion held
bool d_tests_net_errors(
    struct dj_test_suite* _suite
);
bool d_tests_net_results(
    struct dj_test_suite* _suite
);
bool d_tests_net_ports(
    struct dj_test_suite* _suite
);
bool d_tests_net_endpoint_records(
    struct dj_test_suite* _suite
);
bool d_tests_net_endpoint_parse(
    struct dj_test_suite* _suite
);
bool d_tests_net_endpoint_format(
    struct dj_test_suite* _suite
);
bool d_tests_net_frame_codec(
    struct dj_test_suite* _suite
);

// connections, algorithms, and framing
bool d_tests_net_dispatch(
    struct dj_test_suite* _suite
);
bool d_tests_net_complete(
    struct dj_test_suite* _suite
);
bool d_tests_net_lifetime(
    struct dj_test_suite* _suite
);
bool d_tests_net_available(
    struct dj_test_suite* _suite
);
bool d_tests_net_exact(
    struct dj_test_suite* _suite
);
bool d_tests_net_bulk(
    struct dj_test_suite* _suite
);
bool d_tests_net_frames(
    struct dj_test_suite* _suite
);
bool d_tests_net_frames_sink(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_components(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_refusals(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_authority(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_hosts(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_format(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_resolve(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_normalize(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_percent(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_endpoints(
    struct dj_test_suite* _suite
);
bool d_tests_net_url_errors_to_net(
    struct dj_test_suite* _suite
);

// in-memory connections, of net_memory.h
bool d_tests_net_pipes(
    struct dj_test_suite* _suite
);
bool d_tests_net_pipe_ends(
    struct dj_test_suite* _suite
);
bool d_tests_net_memory(
    struct dj_test_suite* _suite
);

// the section, in net_tests.c: every test above, each a test of its own
bool d_tests_net(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_H
