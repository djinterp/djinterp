/*******************************************************************************
* djinterp [test]                                      compress_tests_traits.cpp
*
*   Tests for the codec identity surface: the codec_id enum, the per-codec
* codec_traits<> specializations (id / name / is_available), and the runtime
* availability query (compress.hpp sections III, IV, VII; the codec_available
* leaf in compress.cpp section VII).  Covers every codec tag, pins the enum
* ordering, and checks the cross-check that a codec runnable at runtime is
* never marked unavailable at compile time.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstring>
// djinterp
#include "./compress_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_traits_ids
  Confirms each codec tag maps to its stable codec_id and that the codec_id
  enumerators hold their documented, contiguous values.
  Tests the following:
  - codec_traits<Codec>::id() returns the matching codec_id for all nine codecs
  - the codec_id enum values are 0..8 in the documented order
*/
bool
tests_compress_traits_ids()
{
    // tag -> id
    D_CP_CHECK(codec_traits<codecs::store>::id()   == codec_id_store);
    D_CP_CHECK(codec_traits<codecs::deflate>::id() == codec_id_deflate);
    D_CP_CHECK(codec_traits<codecs::zlib>::id()    == codec_id_zlib);
    D_CP_CHECK(codec_traits<codecs::gzip>::id()    == codec_id_gzip);
    D_CP_CHECK(codec_traits<codecs::bzip2>::id()   == codec_id_bzip2);
    D_CP_CHECK(codec_traits<codecs::xz>::id()      == codec_id_xz);
    D_CP_CHECK(codec_traits<codecs::zstd>::id()    == codec_id_zstd);
    D_CP_CHECK(codec_traits<codecs::lz4>::id()     == codec_id_lz4);
    D_CP_CHECK(codec_traits<codecs::brotli>::id()  == codec_id_brotli);

    // enum values pinned
    D_CP_CHECK(static_cast<int>(codec_id_store)   == 0);
    D_CP_CHECK(static_cast<int>(codec_id_deflate) == 1);
    D_CP_CHECK(static_cast<int>(codec_id_zlib)    == 2);
    D_CP_CHECK(static_cast<int>(codec_id_gzip)    == 3);
    D_CP_CHECK(static_cast<int>(codec_id_bzip2)   == 4);
    D_CP_CHECK(static_cast<int>(codec_id_xz)      == 5);
    D_CP_CHECK(static_cast<int>(codec_id_zstd)    == 6);
    D_CP_CHECK(static_cast<int>(codec_id_lz4)     == 7);
    D_CP_CHECK(static_cast<int>(codec_id_brotli)  == 8);

    return true;
}

/*
tests_compress_traits_names
  Confirms each codec tag reports its expected human-readable name.
  Tests the following:
  - codec_traits<Codec>::name() equals the documented spelling for all nine
    codecs
  - every name is non-null and non-empty
*/
bool
tests_compress_traits_names()
{
    D_CP_CHECK(std::strcmp(codec_traits<codecs::store>::name(),   "store")   == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::deflate>::name(), "deflate") == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::zlib>::name(),    "zlib")    == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::gzip>::name(),    "gzip")    == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::bzip2>::name(),   "bzip2")   == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::xz>::name(),      "xz")      == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::zstd>::name(),    "zstd")    == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::lz4>::name(),     "lz4")     == 0);
    D_CP_CHECK(std::strcmp(codec_traits<codecs::brotli>::name(),  "brotli")  == 0);

    return true;
}

/*
tests_compress_traits_store_always_available
  Confirms the store codec is unconditionally present - it is the built-in
  verbatim copy and must work in every build, no matter which libraries were
  detected.
  Tests the following:
  - the compile-time trait is_available is set for store
  - the runtime availability query agrees
*/
bool
tests_compress_traits_store_always_available()
{
    // compile-time: store is always built in
    D_CP_CHECK(codec_traits<codecs::store>::is_available != 0);

    // runtime: the dispatch reports it available too
    D_CP_CHECK(codec_is_available<codecs::store>() == true);
    D_CP_CHECK(::djinterp::internal::codec_available(codec_id_store) == true);

    return true;
}

/*
tests_compress_traits_runtime_matches_dispatch
  Confirms the availability query is well-behaved for every codec.  A codec's
  runtime availability must be stable, and - the meaningful cross-check - a
  codec the runtime can actually run must never be flagged unavailable at
  compile time (otherwise the throwing API's error paths and any caller gating
  on the trait would misfire).
  Tests the following:
  - codec_is_available<Codec>() agrees with codec_available(traits::id()) for
    all nine codecs
  - the query is idempotent (two calls agree)
  - runtime availability implies compile-time availability for every codec
*/
bool
tests_compress_traits_runtime_matches_dispatch()
{
    // a compact helper is not available across tags without a template, so the
    // check is written once per codec; grouped by a small local lambda would
    // still need the tag, so it is spelled out for clarity.

    // store
    D_CP_CHECK(codec_is_available<codecs::store>() ==
               ::djinterp::internal::codec_available(codec_id_store));
    D_CP_CHECK(codec_is_available<codecs::store>() ==
               codec_is_available<codecs::store>());
    D_CP_CHECK(!codec_is_available<codecs::store>() ||
               (codec_traits<codecs::store>::is_available != 0));

    // deflate
    D_CP_CHECK(codec_is_available<codecs::deflate>() ==
               ::djinterp::internal::codec_available(codec_id_deflate));
    D_CP_CHECK(!codec_is_available<codecs::deflate>() ||
               (codec_traits<codecs::deflate>::is_available != 0));

    // zlib
    D_CP_CHECK(codec_is_available<codecs::zlib>() ==
               ::djinterp::internal::codec_available(codec_id_zlib));
    D_CP_CHECK(!codec_is_available<codecs::zlib>() ||
               (codec_traits<codecs::zlib>::is_available != 0));

    // gzip
    D_CP_CHECK(codec_is_available<codecs::gzip>() ==
               ::djinterp::internal::codec_available(codec_id_gzip));
    D_CP_CHECK(!codec_is_available<codecs::gzip>() ||
               (codec_traits<codecs::gzip>::is_available != 0));

    // bzip2
    D_CP_CHECK(codec_is_available<codecs::bzip2>() ==
               ::djinterp::internal::codec_available(codec_id_bzip2));
    D_CP_CHECK(!codec_is_available<codecs::bzip2>() ||
               (codec_traits<codecs::bzip2>::is_available != 0));

    // xz
    D_CP_CHECK(codec_is_available<codecs::xz>() ==
               ::djinterp::internal::codec_available(codec_id_xz));
    D_CP_CHECK(!codec_is_available<codecs::xz>() ||
               (codec_traits<codecs::xz>::is_available != 0));

    // zstd
    D_CP_CHECK(codec_is_available<codecs::zstd>() ==
               ::djinterp::internal::codec_available(codec_id_zstd));
    D_CP_CHECK(!codec_is_available<codecs::zstd>() ||
               (codec_traits<codecs::zstd>::is_available != 0));

    // lz4
    D_CP_CHECK(codec_is_available<codecs::lz4>() ==
               ::djinterp::internal::codec_available(codec_id_lz4));
    D_CP_CHECK(!codec_is_available<codecs::lz4>() ||
               (codec_traits<codecs::lz4>::is_available != 0));

    // brotli
    D_CP_CHECK(codec_is_available<codecs::brotli>() ==
               ::djinterp::internal::codec_available(codec_id_brotli));
    D_CP_CHECK(!codec_is_available<codecs::brotli>() ||
               (codec_traits<codecs::brotli>::is_available != 0));

    return true;
}

/*
tests_compress_traits_out_of_range_unavailable
  Confirms the availability leaf rejects an unknown codec id rather than
  reading past its switch - the default arm of codec_available().
  Tests the following:
  - codec_available() on an id outside the enum returns false
*/
bool
tests_compress_traits_out_of_range_unavailable()
{
    // an id well past the defined range exercises the switch default
    codec_id bogus = static_cast<codec_id>(9999);

    D_CP_CHECK(::djinterp::internal::codec_available(bogus) == false);

    return true;
}


NS_END  // testing
NS_END  // djinterp
