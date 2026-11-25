/*******************************************************************************
* djinterp [c][net]                                                 ftps_tests.h
*
* net's FTPS suite, in DTest's C face, module ftps: its section, and the
* sections of the two blocks it runs after its own. Each records its checks
* into the suite it is given, a test of its own per scenario.
*
*
* path:      /tests/djinterp/c/modules/net/ftps/ftps_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SECTIONS
    --------
    1.  d_tests_ftps
    2.  d_tests_ftps_loopback
    3.  d_tests_ftps_interop
*/

#ifndef DJINTERP_NET_FTPS_FTPS_TESTS_H
#define DJINTERP_NET_FTPS_FTPS_TESTS_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SECTIONS
//==============================================================================

// 1.1
// d_tests_ftps
//   function: the suite's section (ftps_tests.c). Opens module ftps; runs,
// as block adapter, the TLS layer under the FTP client's helpers over a
// scripted transport and the OpenSSL engine; then the two blocks below.
// True if every check of the three passed.
bool d_tests_ftps(
    struct dj_test_suite* _suite
);

// 1.2
// d_tests_ftps_loopback
//   function: block loopback (ftps_tests_loopback.c): the FTPS client over
// loopback sockets, secured through the TLS kernel with the toy engine,
// against ftps_tests_support.c's threaded server. True if every check
// passed.
bool d_tests_ftps_loopback(
    struct dj_test_suite* _suite
);

// 1.3
// d_tests_ftps_interop
//   function: block interop (ftps_tests_interop.c): the FTP client, plain,
// and the FTPS client against a real server named by FTP_TEST_PORT and its
// fellows; without one, a skipped test. True if every check passed.
bool d_tests_ftps_interop(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTPS_FTPS_TESTS_H
