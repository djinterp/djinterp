/*******************************************************************************
* djinterp [test]                                   test_pack_tests_dispatch.cpp
*
*   Section I's runtime -> compile-time hop.  pack_compress_helper switches a
* test_compressor onto try_compress<Tag>; pack_archive_helper switches a
* test_archive_format onto try_archive<Tag>.  Built against the instrumented
* facade, try_compress<Tag> emits "C|<tag>|<opt.level>|<payload>" and
* try_archive<Tag> emits "A|<tag>|<count>|<first entry name>", so each
* enumerator's routing, and the forwarded opt / payload / entry list, are read
* straight out of the output.  The unreachable default arm (an out-of-range
* enum) is checked to return status_invalid_argument.
*
*
* path:      /tests/djinterp/test/test_pack_tests_dispatch.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_pack_tests.hpp"

NS_DJINTERP
NS_TESTING

namespace
{
    // expected compress output for a tag id, opt.level 5, payload "X"
    std::string cexp(const char* _id) { return std::string("C|") + _id + "|5|X"; }
    // expected archive output for a tag id, one entry named "foo"
    std::string aexp(const char* _id) { return std::string("A|") + _id + "|1|foo"; }
}


// tests_dispatch_compress_routing
//   every test_compressor instantiates try_compress<matching tag>.
bool
tests_dispatch_compress_routing()
{
    bool ok = true;
    dj::reset_codec_hooks();

    dj::compress_options co; co.level = 5;
    dj::byte_blob in = "X", out;

    #define TP_C(_enum, _id)                                                    \
        do {                                                                    \
            dj::status s = dt::internal::pack_compress_helper(                  \
                dt::test_compressor::_enum, in, co, out);                       \
            ok = D_TP_CHECK(s == dj::status_ok && out == cexp(_id)) && ok;      \
        } while (0)

    TP_C(store,   "store");
    TP_C(deflate, "deflate");
    TP_C(zlib,    "zlib");
    TP_C(gzip,    "gzip");
    TP_C(bzip2,   "bzip2");
    TP_C(xz,      "xz");
    TP_C(zstd,    "zstd");
    TP_C(lz4,     "lz4");
    TP_C(brotli,  "brotli");

    #undef TP_C
    return ok;
}

// tests_dispatch_compress_opt_payload
//   the opt (its level) and the input payload are forwarded unchanged.
bool
tests_dispatch_compress_opt_payload()
{
    bool ok = true;
    dj::reset_codec_hooks();

    dj::compress_options co; co.level = 42;
    dj::byte_blob in = "PAYLOAD", out;

    dj::status s = dt::internal::pack_compress_helper(
        dt::test_compressor::gzip, in, co, out);
    ok = D_TP_CHECK(s == dj::status_ok)             && ok;
    ok = D_TP_CHECK(out == "C|gzip|42|PAYLOAD")     && ok;

    return ok;
}

// tests_dispatch_compress_default
//   an out-of-range compressor hits the default arm.
bool
tests_dispatch_compress_default()
{
    bool ok = true;
    dj::reset_codec_hooks();

    dj::compress_options co;
    dj::byte_blob in = "X", out;

    dj::status s = dt::internal::pack_compress_helper(
        static_cast<dt::test_compressor>(99), in, co, out);
    ok = D_TP_CHECK(s == dj::status_invalid_argument) && ok;

    return ok;
}

// tests_dispatch_archive_routing
//   every test_archive_format instantiates try_archive<matching tag>.
bool
tests_dispatch_archive_routing()
{
    bool ok = true;
    dj::reset_format_hooks();

    dj::archive_options ao;
    dj::entry           e;  e.name = "foo"; e.data = "data"; e.is_directory = false;
    dj::entry_list      items; items.push_back(e);
    dj::byte_blob     out;

    #define TP_A(_enum, _id)                                                    \
        do {                                                                    \
            dj::status s = dt::internal::pack_archive_helper(                   \
                dt::test_archive_format::_enum, items, ao, out);               \
            ok = D_TP_CHECK(s == dj::status_ok && out == aexp(_id)) && ok;      \
        } while (0)

    TP_A(zip,      "zip");
    TP_A(tar,      "tar");
    TP_A(tar_gz,   "tar_gz");
    TP_A(gz,       "gz");
    TP_A(sevenzip, "sevenzip");
    TP_A(rar,      "rar");

    #undef TP_A
    return ok;
}

// tests_dispatch_archive_items
//   the entry list is forwarded: the count and the first name reach the facade.
bool
tests_dispatch_archive_items()
{
    bool ok = true;
    dj::reset_format_hooks();

    dj::archive_options ao;
    dj::entry           e;  e.name = "the_report.html"; e.data = "bytes";
    dj::entry_list      items; items.push_back(e);
    dj::byte_blob     out;

    dj::status s = dt::internal::pack_archive_helper(
        dt::test_archive_format::zip, items, ao, out);
    ok = D_TP_CHECK(s == dj::status_ok)                 && ok;
    ok = D_TP_CHECK(out == "A|zip|1|the_report.html")   && ok;

    return ok;
}

// tests_dispatch_archive_default
//   an out-of-range format hits the default arm.
bool
tests_dispatch_archive_default()
{
    bool ok = true;
    dj::reset_format_hooks();

    dj::archive_options ao;
    dj::entry_list      items;
    dj::byte_blob     out;

    dj::status s = dt::internal::pack_archive_helper(
        static_cast<dt::test_archive_format>(99), items, ao, out);
    ok = D_TP_CHECK(s == dj::status_invalid_argument) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
