/*******************************************************************************
* djinterp [net]                                                codec_tests_sa.h
*
* Standalone tests of net's codec: the digests, HMAC, and the comparison and
* wiping that go with them; base64 in each of its variants; hex; ASCII's
* classes, folding, comparison and trimming; line framing; and the errors'
* mapping onto net.h's.
*   Known answers come from the documents that define each function -- RFC
* 1321's test suite for MD5, FIPS 180's examples for SHA-1 and SHA-256, RFC 2202
* and RFC 4231 for HMAC, RFC 4648 for base64 -- and digests are compared as hex
* text, which the suite renders itself, so no codec checks its own output. Where
* no document gives an answer, the tests check properties that must hold
* instead: data given in pieces digests as it does whole, and a key longer than
* a block authenticates as its digest does. Each test returns true when every
* assertion held.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Constants
         1.  D_TESTS_CODEC_HEX_MAX
         2.  D_TESTS_CODEC_SPAN_MAX
    2.  Types
         1.  d_tests_codec_case
         2.  d_tests_codec_span
         3.  d_tests_codec_mac
    3.  Helpers
2.  TESTS
    -----
    1.  Digests
    2.  HMAC
    3.  Secrets
    4.  Base64
    5.  Hex
    6.  ASCII
    7.  Lines
    8.  Errors
    9.  The runner
*/

#ifndef DJINTERP_NET_CODEC_CODEC_TESTS_SA_H
#define DJINTERP_NET_CODEC_CODEC_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"              // framework root
#include "../../../../inc/djinterp/net/net_common.h"   // ASCII
#include "../../../../inc/djinterp/net/codec/codec_base64.h"  // base64
#include "../../../../inc/djinterp/net/codec/codec_common.h"  // errors
#include "../../../../inc/djinterp/net/codec/codec_digest.h"  // the digests
#include "../../../../inc/djinterp/net/codec/codec_hex.h"     // hex
#include "../../../../inc/djinterp/net/codec/codec_line.h"    // lines
#include "../../../../inc/djinterp/test/c/test_counter.h"     // d_test_counter
#include "../net_tests_sa_assert.h"                           // asserts
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_CODEC_HEX_MAX
//   constant: room for the hex of the largest digest, and its NUL.
#define D_TESTS_CODEC_HEX_MAX ((2 * D_NET_DIGEST_MAX_SIZE) + 1)

// 1.1.2
// D_TESTS_CODEC_SPAN_MAX
//   constant: the most octets a test span describes.
#define D_TESTS_CODEC_SPAN_MAX 256

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_tests_codec_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_codec_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

// 1.2.2
// d_tests_codec_span
//   struct: octets as a test document writes them: some text; or, when `text`
// is NULL, `count` copies of `fill`; or, when `fill` is 0 too, the octets 1,
// 2, ... `count`.
struct d_tests_codec_span
{
    const char*   text;
    unsigned char fill;
    size_t        count;
};

// 1.2.3
// d_tests_codec_mac
//   struct: an HMAC case: the key, the message, and the MAC in lowercase hex,
// or the leading part of it a document truncates the MAC to.
struct d_tests_codec_mac
{
    struct d_tests_codec_span key;
    struct d_tests_codec_span data;
    const char*               expected;
};

// 1.3    Helpers
//------------------------------------------------------------------------------
// helpers -- defined in codec_tests_sa_digest.c.
//   d_tests_codec_hex renders octets as lowercase hex, independently of the
// codec; d_tests_codec_span_fill writes a span's octets to `_out` and returns
// how many there are.
void   d_tests_codec_hex(const uint8_t* _bytes,
                         size_t         _size,
                         char*          _out);
size_t d_tests_codec_span_fill(const struct d_tests_codec_span* _span,
                               uint8_t*                         _out);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Digests
//------------------------------------------------------------------------------
bool d_tests_sa_codec_digest_sizes(struct d_test_counter* _counter);
bool d_tests_sa_codec_md5_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_sha1_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_sha256_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_digest_million(struct d_test_counter* _counter);
bool d_tests_sa_codec_digest_pieces(struct d_test_counter* _counter);
bool d_tests_sa_codec_digest_refusals(struct d_test_counter* _counter);

// 2.2    HMAC
//------------------------------------------------------------------------------
bool d_tests_sa_codec_hmac_md5_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_hmac_sha1_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_hmac_sha256_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_hmac_keys(struct d_test_counter* _counter);
bool d_tests_sa_codec_hmac_refusals(struct d_test_counter* _counter);
bool d_tests_sa_codec_digest_overlap(struct d_test_counter* _counter);

// 2.3    Secrets
//------------------------------------------------------------------------------
bool d_tests_sa_codec_digest_equal(struct d_test_counter* _counter);
bool d_tests_sa_codec_wipe(struct d_test_counter* _counter);

// 2.4    Base64
//------------------------------------------------------------------------------
bool d_tests_sa_codec_base64_known(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_strict(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_canonical(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_blanks(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_modified(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_lengths(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_buffers(struct d_test_counter* _counter);
bool d_tests_sa_codec_base64_round_trip(struct d_test_counter* _counter);

// 2.5    Hex
//------------------------------------------------------------------------------
bool d_tests_sa_codec_hex_digits(struct d_test_counter* _counter);
bool d_tests_sa_codec_hex_strings(struct d_test_counter* _counter);
bool d_tests_sa_codec_hex_buffers(struct d_test_counter* _counter);
bool d_tests_sa_codec_hex_edges(struct d_test_counter* _counter);

// 2.6    ASCII
//------------------------------------------------------------------------------
bool d_tests_sa_codec_ascii_classes(struct d_test_counter* _counter);
bool d_tests_sa_codec_ascii_compare(struct d_test_counter* _counter);
bool d_tests_sa_codec_ascii_trim(struct d_test_counter* _counter);
bool d_tests_sa_codec_ascii_edges(struct d_test_counter* _counter);

// 2.7    Lines
//------------------------------------------------------------------------------
bool d_tests_sa_codec_line_find(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_next(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_checks(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_buffer(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_room(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_edges(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_held(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_spill(struct d_test_counter* _counter);
bool d_tests_sa_codec_line_stream(struct d_test_counter* _counter);

// 2.8    Errors
//------------------------------------------------------------------------------
bool d_tests_sa_codec_errors_to_net(struct d_test_counter* _counter);

// 2.9    The runner
//------------------------------------------------------------------------------
bool d_tests_sa_codec_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CODEC_CODEC_TESTS_SA_H
