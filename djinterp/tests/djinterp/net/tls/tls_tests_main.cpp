/*******************************************************************************
* djinterp [net]                                              tls_tests_main.cpp
*
* Runs the TLS layer's tests; without an engine, or a 64-bit integer type,
* there are none, and it says so.
*
*
* path:      /tests/djinterp/net/tls/tls_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./tls_tests.hpp"  // the suite
// std
#include <cstdio>    // std::printf
// posix
#include <unistd.h>  // alarm


/*
main
  Arms the watchdog and announces the suite, which reports each test and
the tally itself.
*/
int
main()
{
    (void)::alarm(60u);
    std::printf("djinterp [net] tls C++ layer tests\n");

#if (D_INTERNAL_NET_HAS_INT64 == 1)
    // a build without the engine has no TLS to test
    if (!::djinterp::testing::tests_tls_engine())
    {
        std::printf("  skipped: no OpenSSL-family engine\n");

        return 0;
    }

    return (::djinterp::testing::tests_tls_run_all()) ? 0 : 1;
#else
    std::printf("  no 64-bit integer type: nothing to test\n");

    return 0;
#endif  // D_INTERNAL_NET_HAS_INT64
}
