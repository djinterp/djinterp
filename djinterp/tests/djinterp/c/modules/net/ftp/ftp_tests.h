/*******************************************************************************
* djinterp [c][net]                                                  ftp_tests.h
*
* net's FTP suite, in DTest's C face, module ftp: its section, and the
* sections of the two blocks it runs after its own tests. Each records its
* checks into the suite it is given, a test of its own per scenario.
*
*
* path:      /tests/djinterp/c/modules/net/ftp/ftp_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SECTIONS
    --------
    1.  d_tests_ftp
    2.  d_tests_ftp_ftps
    3.  d_tests_ftp_interop
*/

#ifndef DJINTERP_NET_FTP_FTP_TESTS_H
#define DJINTERP_NET_FTP_FTP_TESTS_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SECTIONS
//==============================================================================

// 1.1
// d_tests_ftp
//   function: the suite's section (ftp_tests.c). Opens module ftp; runs, as
// block units, ftp.h's own functions and the client's connection helpers
// over a scripted transport; then the two blocks below. True if every check
// of the three passed.
bool d_tests_ftp(
    struct dj_test_suite* _suite
);

// 1.2
// d_tests_ftp_ftps
//   function: block ftps (ftp_tests_ftps.c): the client over loopback
// sockets, secured through the TLS kernel with the toy engine, against
// ftps_tests_support.c's threaded server. True if every check passed.
bool d_tests_ftp_ftps(
    struct dj_test_suite* _suite
);

// 1.3
// d_tests_ftp_interop
//   function: block interop (ftp_tests_interop.c): the client against a
// real server named by FTP_TEST_PORT and its fellows; without one, a
// skipped test. True if every check passed.
bool d_tests_ftp_interop(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_TESTS_H
