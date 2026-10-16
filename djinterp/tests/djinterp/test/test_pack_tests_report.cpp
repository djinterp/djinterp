/*******************************************************************************
* djinterp [test]                                     test_pack_tests_report.cpp
*
*   Section III - the one public entry.  pack_report dispatches on the pack
* mode: none copies the report through (status_ok); compress routes to the
* selected codec with compress_opts(_opts); archive builds a single entry -
* named from output_path + document via pack_entry_name_helper - and routes to
* the selected format with archive_opts(_opts).  Against the instrumented facade
* the output reveals the tag, the forwarded opt / payload, and the derived entry
* name, so the whole path is observable end to end.  Also covered: the leading
* _out.clear() (an out-of-range mode returns status_invalid_argument with _out
* emptied), and facade-failure propagation (a non-ok status leaves _out empty).
*
*
* path:      /tests/djinterp/test/test_pack_tests_report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_pack_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_report_none
//   pack == none copies the report through unchanged.
bool
tests_report_none()
{
    bool ok = true;
    dj::reset_codec_hooks();

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, dt::test_output_pack::none);

    dj::byte_blob report = "THE-REPORT", out = "stale";
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_ok)   && ok;
    ok = D_TP_CHECK(out == "THE-REPORT")  && ok;

    return ok;
}

// tests_report_clears_out
//   _out is cleared up front: an out-of-range mode returns
// status_invalid_argument and leaves _out empty (not its prior contents).
bool
tests_report_clears_out()
{
    bool ok = true;

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, static_cast<dt::test_output_pack>(99));   // no matching case

    dj::byte_blob report = "R", out = "PRIOR-JUNK";
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_invalid_argument) && ok;
    ok = D_TP_CHECK(out.empty())                      && ok;

    return ok;
}

// tests_report_compress
//   pack == compress routes to the selected codec, forwarding compress_opts and
// the report payload.
bool
tests_report_compress()
{
    bool ok = true;
    dj::reset_codec_hooks();

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, dt::test_output_pack::compress);
    tp_set_compressor(o, dt::test_compressor::zstd);
    dj::compress_options co; co.level = 9;
    tp_set_compress_opts(o, co);

    dj::byte_blob report = "RPT", out;
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_ok)          && ok;
    ok = D_TP_CHECK(out == "C|zstd|9|RPT")       && ok;   // tag + opt.level + payload

    return ok;
}

// tests_report_archive
//   pack == archive builds ONE entry, names it from output_path + document, and
// routes to the selected container.
bool
tests_report_archive()
{
    bool ok = true;
    dj::reset_format_hooks();

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, dt::test_output_pack::archive);
    tp_set_archive_format(o, dt::test_archive_format::tar);
    tp_set_output_path(o, "out/r.zip");
    tp_set_document(o, dt::test_doc_type::html);

    dj::byte_blob report = "RPT", out;
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_ok)        && ok;
    // tag tar | one entry | name derived "r.html" (basename of r.zip, .zip -> .html)
    ok = D_TP_CHECK(out == "A|tar|1|r.html")   && ok;

    return ok;
}

// tests_report_archive_name_variants
//   the ".tar.gz" double-strip flows through pack_report's name derivation.
bool
tests_report_archive_name_variants()
{
    bool ok = true;
    dj::reset_format_hooks();

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, dt::test_output_pack::archive);
    tp_set_archive_format(o, dt::test_archive_format::zip);
    tp_set_output_path(o, "logs/run.tar.gz");
    tp_set_document(o, dt::test_doc_type::pdf);

    dj::byte_blob report = "RPT", out;
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_ok)       && ok;
    ok = D_TP_CHECK(out == "A|zip|1|run.pdf") && ok;

    return ok;
}

// tests_report_compress_failure
//   a codec that reports failure propagates the status and leaves _out empty.
bool
tests_report_compress_failure()
{
    bool ok = true;
    dj::reset_codec_hooks();
    dj::codec_stat<dj::codecs::gzip>() = dj::status_unavailable;   // force failure

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, dt::test_output_pack::compress);
    tp_set_compressor(o, dt::test_compressor::gzip);

    dj::byte_blob report = "RPT", out = "stale";
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_unavailable) && ok;
    ok = D_TP_CHECK(out.empty())                 && ok;

    dj::reset_codec_hooks();   // restore for later tests
    return ok;
}

// tests_report_archive_failure
//   a container that reports failure propagates the status and leaves _out
// empty.
bool
tests_report_archive_failure()
{
    bool ok = true;
    dj::reset_format_hooks();
    dj::fmt_stat<dj::formats::zip>() = dj::status_unavailable;   // force failure

    dt::test_option_set o = dt::default_test_options();
    tp_set_pack(o, dt::test_output_pack::archive);
    tp_set_archive_format(o, dt::test_archive_format::zip);
    tp_set_output_path(o, "out/r.zip");
    tp_set_document(o, dt::test_doc_type::txt);

    dj::byte_blob report = "RPT", out = "stale";
    dj::status s = dt::pack_report(o, report, out);

    ok = D_TP_CHECK(s == dj::status_unavailable) && ok;
    ok = D_TP_CHECK(out.empty())                 && ok;

    dj::reset_format_hooks();
    return ok;
}


NS_END  // testing
NS_END  // djinterp
