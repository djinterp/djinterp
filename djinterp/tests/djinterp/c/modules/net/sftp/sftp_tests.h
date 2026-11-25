/*******************************************************************************
* djinterp [c][net]                                                 sftp_tests.h
*
* net's SFTP suite, in DTest's C face, module sftp: its section, which
* records its checks into the suite it is given, a test of its own per
* scenario.
*
*
* path:      /tests/djinterp/c/modules/net/sftp/sftp_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SECTIONS
    --------
    1.  d_tests_sftp
*/

#ifndef DJINTERP_NET_SFTP_SFTP_TESTS_H
#define DJINTERP_NET_SFTP_SFTP_TESTS_H 1

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SECTIONS
//==============================================================================

// 1.1
// d_tests_sftp
//   function: the suite's section (sftp_tests.c). Opens module sftp; runs the
// codec, the client against an in-process server and over the toy SSH
// engine, the client over connections that fail as told, and, where
// SFTP_TEST_* name one, a real server, else a skipped test. True if every
// check passed.
bool d_tests_sftp(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SFTP_SFTP_TESTS_H
