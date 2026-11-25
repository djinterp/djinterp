/*******************************************************************************
* djinterp [c][net]                                             ftp_tests_fake.h
*
* The scripted transport of net's FTP and FTPS suites (ftp_tests_fake.c): a
* connection's TCP put over a script. Reads deliver `in`, then report
* `read_error`; writes take at most `limit` bytes (0: any) into `out`, or
* nothing - with `write_error`, or, when `stalls`, with no error at all.
*
*
* path:      /tests/djinterp/c/modules/net/ftp/ftp_tests_fake.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE FAKE
    --------
    1.  dj_tests_ftp_fake
    2.  g_ftp_fake
    3.  d_tests_ftp_fake_on
*/

#ifndef DJINTERP_NET_FTP_FTP_TESTS_FAKE_H
#define DJINTERP_NET_FTP_FTP_TESTS_FAKE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"                       // framework root
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_client.h"   // dj_ftp_connection
#include "../../../../../../inc/djinterp/c/modules/net/net_common.h"       // dj_net_error


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  THE FAKE
//==============================================================================

// 1.1
// dj_tests_ftp_fake
//   struct: the script, and what the fake was sent.
struct dj_tests_ftp_fake
{
    const char*      in;
    size_t           in_len, in_at;
    enum dj_net_error read_error;
    char             out[4096];
    size_t           out_len, limit;
    enum dj_net_error write_error;
    bool             stalls;
};

// 1.2
// g_ftp_fake
//   variable: the one script, which a test may adjust after
// d_tests_ftp_fake_on and read back.
extern struct dj_tests_ftp_fake g_ftp_fake;

// 1.3
// d_tests_ftp_fake_on
//   function: scripts the fake afresh - `_in` to read, then `_read_error`;
// writes of at most `_limit` bytes, or failing with `_write_error` - and
// puts `_connection`, zeroed, over it.
void d_tests_ftp_fake_on(
    struct dj_ftp_connection* _connection,
    const char*               _in,
    enum dj_net_error         _read_error,
    size_t                    _limit,
    enum dj_net_error         _write_error
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_TESTS_FAKE_H
