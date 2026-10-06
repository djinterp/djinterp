/*******************************************************************************
* djinterp [net]                                                   net_tests.hpp
*
* djinterp net foundation C++ tests.
*   Declarations for the tests of net/net.hpp: the vocabulary's agreement
* with net.h, value by value; endpoint text through the shared C parser and
* formatter; the bridges between C and C++ connections, in both directions;
* frames crossing between the two languages' writers and readers; and the
* generic algorithms over every kind of stream and buffer.
*
*
* path:      /tests/djinterp/net/net_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

#ifndef DJINTERP_NET_NET_TESTS_HPP
#define DJINTERP_NET_NET_TESTS_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
// djinterp
#include "../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../inc/djinterp/net/net.hpp"   // the layer under test


NS_DJINTERP
NS_TESTING

// net/net.hpp tests -- each returns true when every check held
bool tests_net_vocabulary();
bool tests_net_endpoints();
bool tests_net_bridge_from_c();
bool tests_net_bridge_to_c();
bool tests_net_wire();
bool tests_net_streams();

// net/net_url.hpp tests
bool tests_net_url_vocabulary();
bool tests_net_url_parse();
bool tests_net_url_ownership();
bool tests_net_url_resolution();
bool tests_net_url_encoding();
bool tests_net_url_endpoints();

// the suite, over every file's tests
bool tests_net_run_all();

// the tally every test file records its checks in; inline, so the one
// instance counts() holds is shared across the files
NS_INTERNAL

    // tally
    //   struct: checks run and failed across the suite.
    struct tally
    {
        std::size_t checks   = 0u;
        std::size_t failures = 0u;
    };

    // counts
    //   function: the suite's tally.
    inline tally&
    counts() noexcept
    {
        static tally instance;

        return instance;
    }

    // check
    //   function: records one check, printing it if it failed.
    inline bool
    check(
        bool        _condition,
        const char* _message
    ) noexcept
    {
        counts().checks += 1u;

        // only failures are worth a line
        if (!_condition)
        {
            counts().failures += 1u;
            std::printf("    FAIL %s\n",
                        _message);
        }

        return _condition;
    }

NS_END  // internal

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_NET_TESTS_HPP
