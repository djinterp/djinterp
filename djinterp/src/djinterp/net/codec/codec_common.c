/*******************************************************************************
* djinterp [net]                                                  codec_common.c
*
* Definitions for codec_common.h: the codec's errors mapped onto net.h's.
*
*
* path:      /src/djinterp/net/codec/codec_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/codec/codec_common.h"  // corresponding header
// djinterp
#include "../../../../inc/djinterp/net/net_common.h"  // d_net_error


/*
d_net_codec_error_to_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules. A value outside the enum is UNKNOWN.
*/
enum d_net_error
d_net_codec_error_to_net(
    enum d_net_codec_error _error
)
{
    switch (_error)
    {
        case D_NET_CODEC_OK:
            return D_NET_OK;
        case D_NET_CODEC_ERROR_INVALID_ARGUMENT:
        case D_NET_CODEC_ERROR_BUFFER_TOO_SMALL:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case D_NET_CODEC_ERROR_MALFORMED:
            return D_NET_ERROR_PROTOCOL;
    }

    return D_NET_ERROR_UNKNOWN;
}
