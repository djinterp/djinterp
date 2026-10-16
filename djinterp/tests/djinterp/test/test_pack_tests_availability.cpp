/*******************************************************************************
* djinterp [test]                               test_pack_tests_availability.cpp
*
*   Section II.  pack_enabled reports whether the option set asks for any
* packaging.  report_codec_available / report_format_available map a runtime
* enum onto codec_is_available<Tag> / format_is_writable<Tag>; each enumerator
* is checked by comparing the router's answer to the facade's own answer for the
* EXPECTED tag - so a mis-routed case (e.g. gzip -> the deflate tag) is caught
* without needing to force any codec on or off.  The unreachable default arm
* (an out-of-range enum) is checked to return false.
*
*
* path:      /tests/djinterp/test/test_pack_tests_availability.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_pack_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_avail_pack_enabled
//   pack_enabled is false only for pack == none.
bool
tests_avail_pack_enabled()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    tp_set_pack(o, dt::test_output_pack::none);
    ok = D_TP_CHECK(!dt::pack_enabled(o)) && ok;

    tp_set_pack(o, dt::test_output_pack::compress);
    ok = D_TP_CHECK( dt::pack_enabled(o)) && ok;

    tp_set_pack(o, dt::test_output_pack::archive);
    ok = D_TP_CHECK( dt::pack_enabled(o)) && ok;

    return ok;
}

// tests_avail_codec_routing
//   every test_compressor routes to codec_is_available<matching tag>.
bool
tests_avail_codec_routing()
{
    bool ok = true;
    using namespace dj::codecs;

    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::store)   == dj::codec_is_available<store>())   && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::deflate) == dj::codec_is_available<deflate>()) && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::zlib)    == dj::codec_is_available<zlib>())    && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::gzip)    == dj::codec_is_available<gzip>())    && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::bzip2)   == dj::codec_is_available<bzip2>())   && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::xz)      == dj::codec_is_available<xz>())      && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::zstd)    == dj::codec_is_available<zstd>())    && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::lz4)     == dj::codec_is_available<lz4>())     && ok;
    ok = D_TP_CHECK(dt::report_codec_available(dt::test_compressor::brotli)  == dj::codec_is_available<brotli>())  && ok;

    return ok;
}

// tests_avail_codec_default
//   an out-of-range compressor falls to the default arm -> false.
bool
tests_avail_codec_default()
{
    bool ok = true;

    ok = D_TP_CHECK(!dt::report_codec_available(static_cast<dt::test_compressor>(99))) && ok;

    return ok;
}

// tests_avail_format_routing
//   every test_archive_format routes to format_is_writable<matching tag>.
bool
tests_avail_format_routing()
{
    bool ok = true;
    using namespace dj::formats;

    ok = D_TP_CHECK(dt::report_format_available(dt::test_archive_format::zip)      == dj::format_is_writable<zip>())      && ok;
    ok = D_TP_CHECK(dt::report_format_available(dt::test_archive_format::tar)      == dj::format_is_writable<tar>())      && ok;
    ok = D_TP_CHECK(dt::report_format_available(dt::test_archive_format::tar_gz)   == dj::format_is_writable<tar_gz>())   && ok;
    ok = D_TP_CHECK(dt::report_format_available(dt::test_archive_format::gz)       == dj::format_is_writable<gz>())       && ok;
    ok = D_TP_CHECK(dt::report_format_available(dt::test_archive_format::sevenzip) == dj::format_is_writable<sevenzip>()) && ok;
    ok = D_TP_CHECK(dt::report_format_available(dt::test_archive_format::rar)      == dj::format_is_writable<rar>())      && ok;

    return ok;
}

// tests_avail_format_default
//   an out-of-range format falls to the default arm -> false.
bool
tests_avail_format_default()
{
    bool ok = true;

    ok = D_TP_CHECK(!dt::report_format_available(static_cast<dt::test_archive_format>(99))) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
