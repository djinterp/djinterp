/*******************************************************************************
* djinterp [net]                                         codec_tests_sa_common.c
*
* Standalone tests of codec_common.h: each of the codec's errors mapped
* onto net.h's d_net_error.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite


// d_tests_codec_mapping
//   struct: an error, and the d_net_error it maps to.
struct d_tests_codec_mapping
{
    enum d_net_codec_error error;
    enum d_net_error       net;
};

/*
d_tests_sa_codec_errors_to_net
  Tests the following:
  - each error maps onto the d_net_error net.h's rules give it
  - a value outside the enum maps to D_NET_ERROR_UNKNOWN
*/
bool
d_tests_sa_codec_errors_to_net(
    struct d_test_counter* _counter
)
{
    const struct d_tests_codec_mapping MAP[] =
    {
        { D_NET_CODEC_OK,                     D_NET_OK                     },
        { D_NET_CODEC_ERROR_INVALID_ARGUMENT, D_NET_ERROR_INVALID_ARGUMENT },
        { D_NET_CODEC_ERROR_MALFORMED,        D_NET_ERROR_PROTOCOL         },
        { D_NET_CODEC_ERROR_BUFFER_TOO_SMALL, D_NET_ERROR_INVALID_ARGUMENT }
    };
    const size_t count  = sizeof(MAP) / sizeof(MAP[0]);
    bool         mapped = true;
    bool         result = true;

    // each error against the table
    for (size_t i = 0u; i < count; ++i)
    {
        mapped = ( (mapped) &&
                   (d_net_codec_error_to_net(MAP[i].error) == MAP[i].net) );
    }

    const bool outside =
        (d_net_codec_error_to_net((enum d_net_codec_error)99) ==
         D_NET_ERROR_UNKNOWN);

    result = d_tests_net_assert(mapped,
                                "d_net_codec_error_to_net",
                                "each error maps by net.h's rules",
                                _counter) && result;
    result = d_tests_net_assert(outside,
                                "d_net_codec_error_to_net",
                                "a value outside the enum is unknown",
                                _counter) && result;

    return result;
}
