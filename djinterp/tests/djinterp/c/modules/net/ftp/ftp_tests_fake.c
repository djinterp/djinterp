/*******************************************************************************
* djinterp [c][net]                                             ftp_tests_fake.c
*
* The scripted transport of net's FTP and FTPS suites: a dj_net_connection
* whose reads deliver a script and whose writes keep what they are sent,
* each failing where a test asks. ftp's suite runs the plain client over it,
* and ftps's the TLS layer.
*
*
* path:      /tests/djinterp/c/modules/net/ftp/ftp_tests_fake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "./ftp_tests_fake.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memcpy, memset, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/net_common.h"  // dj_net_connection
#include "../../../../../../inc/djinterp/c/modules/net/tcp/tcp.h"     // d_tcp_connection_init


// g_ftp_fake
//   variable: the script, which d_tests_ftp_fake_on sets.
struct dj_tests_ftp_fake g_ftp_fake;

static
struct dj_net_io_result fake_read(
    struct dj_net_connection* _c,
    void*                     _b,
    size_t                    _n
)
{
    size_t left = g_ftp_fake.in_len - g_ftp_fake.in_at;
    (void)_c;
    if (left == 0)
    {
        return d_net_io_result_make(0,
                                    g_ftp_fake.read_error);
    }

    if (_n > left)
    {
        _n = left;
    }

    memcpy(_b,
           g_ftp_fake.in + g_ftp_fake.in_at,
           _n);
    g_ftp_fake.in_at += _n;

    return d_net_io_result_make(_n,
                                D_NET_OK);
}

static
struct dj_net_io_result fake_write(
    struct dj_net_connection* _c,
    const void*               _d,
    size_t                    _n
)
{
    (void)_c;
    if (g_ftp_fake.write_error != D_NET_OK || g_ftp_fake.stalls)
    {
        return d_net_io_result_make(0,
                                    g_ftp_fake.write_error);
    }

    if (g_ftp_fake.limit && _n > g_ftp_fake.limit)
    {
        _n = g_ftp_fake.limit;
    }

    if (_n > sizeof g_ftp_fake.out - g_ftp_fake.out_len)
    {
        _n = sizeof g_ftp_fake.out - g_ftp_fake.out_len;
    }

    memcpy(g_ftp_fake.out + g_ftp_fake.out_len,
           _d,
           _n);
    g_ftp_fake.out_len += _n;

    return d_net_io_result_make(_n,
                                D_NET_OK);
}

static
bool fake_is_open(
    const struct dj_net_connection* _c
)
{
    (void)_c;

    return true;
}

static
void fake_close(
    struct dj_net_connection* _c
)
{
    (void)_c;

    return;
}
static const struct dj_net_connection_vtable FAKE = {
    fake_read, fake_write, fake_is_open, fake_close, NULL,
    NULL,      NULL,       NULL,         NULL
};

/*
d_tests_ftp_fake_on
  Scripts the fake afresh, and puts a zeroed connection's TCP over it.
*/
void d_tests_ftp_fake_on(
    struct dj_ftp_connection* _k,
    const char*               _in,
    enum dj_net_error         _read_error,
    size_t                    _limit,
    enum dj_net_error         _write_error
)
{
    memset(&g_ftp_fake,
           0,
           sizeof g_ftp_fake);
    g_ftp_fake.in          = _in;
    g_ftp_fake.in_len      = _in ? strlen(_in) : 0;
    g_ftp_fake.read_error  = _read_error;
    g_ftp_fake.limit       = _limit;
    g_ftp_fake.write_error = _write_error;
    memset(_k,
           0,
           sizeof *_k);
    d_tcp_connection_init(&_k->tcp);
    (void)d_net_connection_init(&_k->tcp.base,
                                &FAKE);

    return;
}
