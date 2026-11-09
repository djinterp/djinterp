/*******************************************************************************
* djinterp [cpp/test]                      test_report_runner_tests_compress.cpp
*
* report_runner's suite, block "compress": the compress pack mode, which
* compresses each rendered document on its own and writes it where its loose
* file would have gone, with the codec's suffix appended.
*
*
* path:      /tests/djinterp/cpp/modules/test/output/test_report_runner_tests_compress.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "test_report_runner_tests.hpp"

// The whole translation unit is inert unless packaging is compiled in.  The
// same guard fences this block's declarations and its spec rows in the suite
// header, so the three stay in step whichever way the target is built.
#if defined(D_TEST_REPORT_ENABLE_ARCHIVE)

NS_DJINTERP
NS_TESTING


/*
tests_report_runner_compress_gzip_writes_suffix
  compressing the run's one document.
  Tests the following:
  - compress mode with gzip writes name.pdf.gz instead of the loose PDF
  - the file is a gzip stream: its first three bytes are 1F 8B 08
  - it decompresses to the whole PDF
  - where the build has no gzip provider, nothing is written at all
*/
bool tests_report_runner_compress_gzip_writes_suffix()
{
    const std::string loose    = "dtest_rr_squeezed.pdf";
    const std::string gz       = "dtest_rr_squeezed.pdf.gz";
    const bool        can_gzip = (d_codec_can_compress(D_CODEC_ID_GZIP) != 0);

    rr_remove(loose);
    rr_remove(gz);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(loose);
    rr_set_pack(rb.options(), dt::DTestOutputPack::compress);
    rr_set_compressor(rb.options(), dt::test_compressor::gzip);

    rb.module("gzipped");
    rr_seed_units(rb, 2, 1);
    rb.finish();

    // never the loose document, whichever way the codec went
    D_RR_CHECK(!rr_file_exists(loose));

    // no gzip here: no file either
    if (!can_gzip)
    {
        D_RR_CHECK(!rr_file_exists(gz));

        return true;
    }

    D_RR_CHECK(rr_file_exists(gz));

    const std::string bytes = rr_read_file(gz);

    // the gzip signature, all three bytes of it
    D_RR_CHECK(bytes.size() > 3u);
    D_RR_CHECK(static_cast<unsigned char>(bytes[0]) == 0x1Fu);
    D_RR_CHECK(static_cast<unsigned char>(bytes[1]) == 0x8Bu);
    D_RR_CHECK(static_cast<unsigned char>(bytes[2]) == 0x08u);

    // and the whole PDF inside it
    const std::string pdf = rr_gunzip(bytes);

    D_RR_CHECK(pdf.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(rr_contains(pdf, "gzipped"));

    rr_remove(gz);

    return true;
}

/*
tests_report_runner_compress_per_module_each
  compressing one document per module.
  Tests the following:
  - per_module plus compress writes one compressed file per module, each
    under the name the pattern gave it with the codec's suffix appended
  - each decompresses to that module's whole PDF
  - no loose per-module file is left behind
*/
bool tests_report_runner_compress_per_module_each()
{
    const char* const modules[3] = { "alpha", "beta", "gamma" };
    const bool        can_gzip   =
        (d_codec_can_compress(D_CODEC_ID_GZIP) != 0);

    // nothing from an earlier run
    for (std::size_t i = 0; i < 3u; ++i)
    {
        const std::string loose =
            std::string("dtest_rr_cm_") + modules[i] + ".pdf";

        rr_remove(loose);
        rr_remove(loose + ".gz");
    }

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf("dtest_rr_cm_unused.pdf");
    rb.set_split(dt::DTestOutputSplit::per_module);
    rb.set_file_name_pattern("dtest_rr_cm_{module}.{ext}");
    rr_set_pack(rb.options(), dt::DTestOutputPack::compress);
    rr_set_compressor(rb.options(), dt::test_compressor::gzip);

    // one module, one unit, each
    for (std::size_t i = 0; i < 3u; ++i)
    {
        rb.module(modules[i]);
        rr_seed_units(rb, 1, 0);
    }

    rb.finish();

    for (std::size_t i = 0; i < 3u; ++i)
    {
        const std::string loose =
            std::string("dtest_rr_cm_") + modules[i] + ".pdf";
        const std::string gz    = loose + ".gz";

        D_RR_CHECK(!rr_file_exists(loose));

        // no gzip here: nothing written for any module
        if (!can_gzip)
        {
            D_RR_CHECK(!rr_file_exists(gz));

            continue;
        }

        D_RR_CHECK(rr_file_exists(gz));

        const std::string pdf = rr_gunzip(rr_read_file(gz));

        D_RR_CHECK(pdf.compare(0, 5, "%PDF-") == 0);
        D_RR_CHECK(rr_contains(pdf, modules[i]));

        rr_remove(gz);
    }

    return true;
}

/*
tests_report_runner_compress_store_is_loose
  the codec that does not compress.
  Tests the following:
  - compress mode with store writes the PDF as it is
  - under its own name, with no suffix appended
*/
bool tests_report_runner_compress_store_is_loose()
{
    const std::string path = "dtest_rr_as_is.pdf";

    rr_remove(path);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(path);
    rr_set_pack(rb.options(), dt::DTestOutputPack::compress);
    rr_set_compressor(rb.options(), dt::test_compressor::store);

    rb.module("unsqueezed");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(path));

    // the PDF itself, readable without a codec
    const std::string bytes = rr_read_file(path);

    D_RR_CHECK(bytes.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(rr_contains(bytes, "unsqueezed"));

    rr_remove(path);

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_TEST_REPORT_ENABLE_ARCHIVE
