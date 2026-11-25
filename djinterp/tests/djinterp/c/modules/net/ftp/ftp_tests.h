/*******************************************************************************
* djinterp [c][net]                                                  ftp_tests.h
*
* net's FTP suite, in DTest's C face, module ftp: its section, which records
* its checks into the suite it is given, a test of its own per scenario.
* FTPS's suite is module ftps's (ftps_tests.h).
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
//   function: the suite's section (ftp_tests.c). Opens module ftp, and runs
// ftp.h's own functions, and the plain client and its connection helpers
// over a scripted transport and loopback sockets. True if every check
// passed.
bool d_tests_ftp(
    struct dj_test_suite* _suite
);

D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_TESTS_H
