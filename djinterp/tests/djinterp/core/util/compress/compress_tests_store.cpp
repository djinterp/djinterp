/*******************************************************************************
* djinterp [test]                                       compress_tests_store.cpp
*
*   Tests for the store codec - the built-in, always-available verbatim copy
* (compress.cpp section II, store_copy) reached through the facade for both
* directions.  Because store is unconditional, these run for real in every
* build: they confirm the copy is byte-exact, length-carrying (embedded NULs
* survive), and correct at the empty, all-byte-values, and large sizes, and
* that the leaf rejects a null/size mismatch while accepting the null/zero
* degenerate case.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests_store.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <string>
// djinterp
#include "./compress_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_store_roundtrip
  Confirms the store codec reproduces ordinary input through a compress /
  decompress cycle.
  Tests the following:
  - a text payload compresses and decompresses with status_ok
  - the restored bytes equal the original
*/
bool
tests_compress_store_roundtrip()
{
    byte_blob in = test::make_text(500);

    D_CP_CHECK(test::roundtrips<codecs::store>(in));
    D_CP_CHECK(test::facade_roundtrip_ok<codecs::store>(in));

    return true;
}

/*
tests_compress_store_verbatim
  Confirms store performs a true verbatim copy - not merely a reversible one.
  Tests the following:
  - compression succeeds and the "compressed" output equals the input exactly
  - the output size equals the input size (store adds no framing)
*/
bool
tests_compress_store_verbatim()
{
    byte_blob in = test::make_pattern(613);
    byte_blob out;
    status      s;

    s = try_compress<codecs::store>(in, out);

    D_CP_CHECK(s == status_ok);
    D_CP_CHECK(out == in);
    D_CP_CHECK(out.size() == in.size());

    return true;
}

/*
tests_compress_store_empty
  Confirms the empty buffer is a valid input in both directions.
  Tests the following:
  - compressing an empty buffer yields status_ok and empty output
  - decompressing that yields status_ok and empty output
*/
bool
tests_compress_store_empty()
{
    byte_blob empty;
    byte_blob packed;
    byte_blob restored;
    status      cs;
    status      ds;

    cs = try_compress<codecs::store>(empty, packed);
    ds = try_decompress<codecs::store>(packed, restored);

    D_CP_CHECK(cs == status_ok);
    D_CP_CHECK(packed.empty());
    D_CP_CHECK(ds == status_ok);
    D_CP_CHECK(restored.empty());

    return true;
}

/*
tests_compress_store_embedded_nuls
  Confirms store is length-carrying and never truncates at a NUL byte.
  Tests the following:
  - a buffer riddled with embedded NULs round-trips to an equal buffer
  - the verbatim output preserves the NUL-bearing content exactly
*/
bool
tests_compress_store_embedded_nuls()
{
    byte_blob in = test::make_with_nuls(300);
    byte_blob out;

    // the payload really does contain NULs (guards the test itself)
    D_CP_CHECK(in.find('\0') != byte_blob::npos);

    D_CP_CHECK(try_compress<codecs::store>(in, out) == status_ok);
    D_CP_CHECK(out == in);
    D_CP_CHECK(out.size() == in.size());
    D_CP_CHECK(test::roundtrips<codecs::store>(in));

    return true;
}

/*
tests_compress_store_all_byte_values
  Confirms every byte value 0x00..0xFF passes through store unaltered.
  Tests the following:
  - a buffer cycling all 256 byte values round-trips to an equal buffer
*/
bool
tests_compress_store_all_byte_values()
{
    byte_blob in = test::make_pattern(256 * 4);   // four full cycles

    D_CP_CHECK(test::roundtrips<codecs::store>(in));

    return true;
}

/*
tests_compress_store_large
  Confirms store handles a large payload without truncation or corruption.
  Tests the following:
  - a one-mebibyte buffer round-trips to an equal buffer
*/
bool
tests_compress_store_large()
{
    byte_blob in = test::make_incompressible(1024u * 1024u);
    byte_blob out;

    D_CP_CHECK(try_compress<codecs::store>(in, out) == status_ok);
    D_CP_CHECK(out.size() == in.size());
    D_CP_CHECK(test::roundtrips<codecs::store>(in));

    return true;
}

/*
tests_compress_store_invalid_argument
  Confirms the dispatch leaf rejects a null pointer paired with a non-zero
  length, in both directions, rather than dereferencing it.  Driven through
  the internal leaf because the public template API always passes a valid
  data() pointer and so cannot reach this guard.
  Tests the following:
  - compress_buffer(store, null, n>0) returns status_invalid_argument
  - decompress_buffer(store, null, n>0) returns status_invalid_argument
*/
bool
tests_compress_store_invalid_argument()
{
    byte_blob      out;
    compress_options opt;
    status           cs;
    status           ds;

    cs = ::djinterp::internal::compress_buffer(
             codec_id_store, static_cast<const char*>(0), 5u, opt, out);
    ds = ::djinterp::internal::decompress_buffer(
             codec_id_store, static_cast<const char*>(0), 5u, out);

    D_CP_CHECK(cs == status_invalid_argument);
    D_CP_CHECK(ds == status_invalid_argument);

    return true;
}

/*
tests_compress_store_null_zero_ok
  Confirms the null/zero degenerate case is accepted - a null pointer with a
  zero length is a valid empty buffer, distinct from the null/non-zero error
  above.
  Tests the following:
  - compress_buffer(store, null, 0) returns status_ok with empty output
  - decompress_buffer(store, null, 0) returns status_ok with empty output
*/
bool
tests_compress_store_null_zero_ok()
{
    byte_blob      out("prefill");
    compress_options opt;
    status           cs;
    status           ds;

    cs = ::djinterp::internal::compress_buffer(
             codec_id_store, static_cast<const char*>(0), 0u, opt, out);
    D_CP_CHECK(cs == status_ok);
    D_CP_CHECK(out.empty());

    out.assign("prefill");
    ds = ::djinterp::internal::decompress_buffer(
             codec_id_store, static_cast<const char*>(0), 0u, out);
    D_CP_CHECK(ds == status_ok);
    D_CP_CHECK(out.empty());

    return true;
}


NS_END  // testing
NS_END  // djinterp
