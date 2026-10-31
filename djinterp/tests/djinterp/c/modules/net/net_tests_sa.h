/*******************************************************************************
* djinterp [c/net]                                                net_tests_sa.h
*
* djinterp net foundation standalone tests.
*   Declarations for the tests of net/net.h. The doubles they run against
* are declared in net_tests_sa_support.h.
*
*
* path:      /tests/djinterp/c/modules/net/net_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_SA_H
#define DJINTERP_NET_NET_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "./net_tests_sa_assert.h"   // asserts
#include "./net_tests_sa_support.h"  // doubles


D_EXTERN_C_BEGIN


// vocabulary and addresses -- each returns true when every assertion held
bool d_tests_sa_net_errors(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_results(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_ports(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_endpoint_records(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_endpoint_parse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_endpoint_format(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_frame_codec(
    struct dj_test_counter* _counter
);

// connections, algorithms, and framing
bool d_tests_sa_net_dispatch(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_complete(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_lifetime(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_available(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_exact(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_bulk(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_frames(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_frames_sink(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_components(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_refusals(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_authority(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_hosts(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_format(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_resolve(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_normalize(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_percent(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_endpoints(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_url_errors_to_net(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_run_all(
    struct dj_test_counter* _counter
);

// in-memory connections, of net_memory.h
bool d_tests_sa_net_pipes(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_pipe_ends(
    struct dj_test_counter* _counter
);
bool d_tests_sa_net_memory(
    struct dj_test_counter* _counter
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_SA_H
