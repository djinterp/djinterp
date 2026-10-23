/*******************************************************************************
* djinterp [net]                                                         codec.h
*
* net's codec: the byte-level algorithms more than one protocol needs.
*   Every module that once carried its own copy of one of these now calls the
* one here, so a fix or an optimization lands once and every protocol agrees
* on the result. One concern to a header:
*     codec_common.h  the status encoders and decoders report
*     codec_hex.h     octets as hex text and back
*     codec_base64.h  base64: strict, unpadded, canonical, PEM's, and IMAP's
*                     alphabet
*     codec_digest.h  MD5, SHA-1 and SHA-256, HMAC over each, comparison in
*                     constant time, and wiping secrets; empty without a
*                     64-bit integer type
*     codec_line.h    lines: found, split, checked for breaks, and buffered
*                     for a reader that does its own I/O
*   This header includes them all. The codec depends on nothing but the
* framework root, its span types, and net_common.h, whose ASCII classes and
* case, hex digits and 64-bit floor it builds on (they were codec_ascii.h's
* and half of codec_hex.h's): no library, no allocation, no I/O, and no state
* outside what a caller passes in. bool comes from the framework root, as in
* net_common.h, so the headers serve C++ translation units too.
*
*
* path:      /inc/djinterp/net/codec/codec.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_NET_CODEC_CODEC_H
#define DJINTERP_NET_CODEC_CODEC_H 1

// djinterp
#include "./codec_common.h"  // d_net_codec_error
#include "../net_common.h"   // d_net_ascii_*, d_net_hex_digit
#include "./codec_hex.h"     // d_net_hex_*
#include "./codec_base64.h"  // d_net_base64_*
#include "./codec_digest.h"  // d_net_digest_*, d_net_hmac, d_net_wipe
#include "./codec_line.h"    // d_net_line_*


#endif  // DJINTERP_NET_CODEC_CODEC_H
