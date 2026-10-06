/*******************************************************************************
* djinterp [net]                                                  net_tests_sa.h
*
* djinterp net foundation standalone tests.
*   Declarations for the tests of net/net.h. The doubles they run against
* are declared in net_tests_sa_support.h.
*
*
* path:      /tests/djinterp/net/net_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_SA_H
#define DJINTERP_NET_NET_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../inc/djinterp/test/c/test_standalone.h"  // asserts
#include "./net_tests_sa_support.h"                        // doubles


D_EXTERN_C_BEGIN


// vocabulary and addresses -- each returns true when every assertion held
bool d_tests_sa_net_errors(struct d_test_counter* _counter);
bool d_tests_sa_net_results(struct d_test_counter* _counter);
bool d_tests_sa_net_ports(struct d_test_counter* _counter);
bool d_tests_sa_net_endpoint_records(struct d_test_counter* _counter);
bool d_tests_sa_net_endpoint_parse(struct d_test_counter* _counter);
bool d_tests_sa_net_endpoint_format(struct d_test_counter* _counter);
bool d_tests_sa_net_frame_codec(struct d_test_counter* _counter);

// connections, algorithms, and framing
bool d_tests_sa_net_dispatch(struct d_test_counter* _counter);
bool d_tests_sa_net_lifetime(struct d_test_counter* _counter);
bool d_tests_sa_net_exact(struct d_test_counter* _counter);
bool d_tests_sa_net_bulk(struct d_test_counter* _counter);
bool d_tests_sa_net_frames(struct d_test_counter* _counter);
bool d_tests_sa_net_frames_sink(struct d_test_counter* _counter);
bool d_tests_sa_net_url_components(struct d_test_counter* _counter);
bool d_tests_sa_net_url_refusals(struct d_test_counter* _counter);
bool d_tests_sa_net_url_authority(struct d_test_counter* _counter);
bool d_tests_sa_net_url_hosts(struct d_test_counter* _counter);
bool d_tests_sa_net_url_format(struct d_test_counter* _counter);
bool d_tests_sa_net_url_resolve(struct d_test_counter* _counter);
bool d_tests_sa_net_url_normalize(struct d_test_counter* _counter);
bool d_tests_sa_net_url_percent(struct d_test_counter* _counter);
bool d_tests_sa_net_url_endpoints(struct d_test_counter* _counter);
bool d_tests_sa_net_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_TESTS_SA_H
