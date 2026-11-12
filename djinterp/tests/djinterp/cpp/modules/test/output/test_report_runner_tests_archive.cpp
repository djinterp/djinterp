/*******************************************************************************
* djinterp [cpp][test]                      test_report_runner_tests_archive.cpp
*
* report_runner's suite, block "archive": the archive pack mode, which puts
* every rendered document in one container, and how the container's entries
* are named and written: stored, deflated, or as the build's knobs say.
*
*
* path:      /tests/djinterp/cpp/modules/test/output/test_report_runner_tests_archive.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
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
tests_report_runner_archive_zip_bundles
  packaging the run into a container.
  Tests the following:
  - archive mode writes the container instead of the loose PDF
  - the container is a real zip stream, carrying the local-file signature
  - the document is inside it, whole, read back through the archive reader:
    stored or compressed, as the build's defaults say
*/
bool tests_report_runner_archive_zip_bundles()
{
    const std::string zip   = "dtest_rr_bundle.zip";
    const std::string loose = "dtest_rr_bundle.pdf";

    rr_remove(zip);
    rr_remove(loose);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(zip);
    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);

    rb.module("packed");
    rr_seed_units(rb, 2, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(zip));
    D_RR_CHECK(!rr_file_exists(loose));

    const std::string bytes = rr_read_file(zip);

    D_RR_CHECK(bytes.size() > 4u);

    // the zip local-file-header signature, PK\x03\x04
    D_RR_CHECK(bytes[0] == 'P');
    D_RR_CHECK(bytes[1] == 'K');
    D_RR_CHECK(bytes[2] == '\x03');
    D_RR_CHECK(bytes[3] == '\x04');

    // one entry, the whole PDF, whatever method it was written with
    const ::djinterp::entry_list entries = rr_zip_entries(bytes);

    D_RR_CHECK(entries.size() == 1u);
    D_RR_CHECK(entries[0].data.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(rr_contains(entries[0].data, "packed"));

    rr_remove(zip);

    return true;
}

/*
tests_report_runner_archive_entry_basenames
  the names entries take inside the container.
  Tests the following:
  - an entry carries only the final path component, never a directory
    prefix
  - the prefix really was part of the document's own name
*/
bool tests_report_runner_archive_entry_basenames()
{
    const std::string zip = "dtest_rr_basename.zip";

    rr_remove(zip);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(zip);
    rb.set_split(dt::DTestOutputSplit::per_module);

    // a pattern carrying a directory prefix the entry name must shed
    rb.set_file_name_pattern("some/nested/dir/{module}.{ext}");

    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);

    rb.module("inner_name");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(zip));

    const std::string bytes = rr_read_file(zip);

    // the entry is named by its base name alone
    D_RR_CHECK(rr_contains(bytes, "inner_name.pdf"));
    D_RR_CHECK(!rr_contains(bytes, "some/nested/dir/"));

    // and no loose file was written under the prefixed path
    D_RR_CHECK(!rr_file_exists("some/nested/dir/inner_name.pdf"));

    rr_remove(zip);

    return true;
}

/*
tests_report_runner_archive_fallback_extension
  naming the container when no path was given.
  Tests the following:
  - an empty output_file names the container `report` plus the selected
    format's extension
  - the container is still a valid zip stream
*/
bool tests_report_runner_archive_fallback_extension()
{
    const std::string expected = "report.zip";

    rr_remove(expected);

    dt::report_builder rb;

    rb.set_console(false);
    rb.set_document(dt::test_doc_type::pdf);
    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);

    D_RR_CHECK(dt::output_path(rb.options()).empty());

    rb.module("m");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(expected));

    const std::string bytes = rr_read_file(expected);

    D_RR_CHECK(bytes.size() > 4u);
    D_RR_CHECK(bytes[0] == 'P');
    D_RR_CHECK(bytes[1] == 'K');

    // the entry inside is the whole-run document's fallback name
    D_RR_CHECK(rr_contains(bytes, "report.pdf"));

    rr_remove(expected);

    return true;
}

/*
tests_report_runner_archive_per_module_one_container
  packaging a per-module split.
  Tests the following:
  - N modules produce N documents but only ONE container
  - every module's entry is inside that container, under exactly the name the
    pattern gave it, with no index appended
  - each entry is a whole PDF
  - no loose per-module file is left behind
*/
bool tests_report_runner_archive_per_module_one_container()
{
    const std::string zip = "dtest_rr_multi.zip";
    const std::string a   = "dtest_rr_mm_alpha.pdf";
    const std::string b   = "dtest_rr_mm_beta.pdf";
    const std::string c   = "dtest_rr_mm_gamma.pdf";

    rr_remove(zip);
    rr_remove(a);
    rr_remove(b);
    rr_remove(c);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(zip);
    rb.set_split(dt::DTestOutputSplit::per_module);
    rb.set_file_name_pattern("dtest_rr_mm_{module}.{ext}");
    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);

    rb.module("alpha");
    rr_seed_units(rb, 1, 0);
    rb.module("beta");
    rr_seed_units(rb, 1, 0);
    rb.module("gamma");
    rr_seed_units(rb, 1, 0);

    rb.finish();

    D_RR_CHECK(rr_file_exists(zip));

    // exactly one container, and no loose documents beside it
    D_RR_CHECK(!rr_file_exists(a));
    D_RR_CHECK(!rr_file_exists(b));
    D_RR_CHECK(!rr_file_exists(c));

    const std::string bytes = rr_read_file(zip);

    // all three entries are inside, in order, under their own names
    const ::djinterp::entry_list entries = rr_zip_entries(bytes);

    D_RR_CHECK(entries.size() == 3u);
    D_RR_CHECK(entries[0].name == "dtest_rr_mm_alpha.pdf");
    D_RR_CHECK(entries[1].name == "dtest_rr_mm_beta.pdf");
    D_RR_CHECK(entries[2].name == "dtest_rr_mm_gamma.pdf");

    // each one a whole PDF
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        D_RR_CHECK(entries[i].data.compare(0, 5, "%PDF-") == 0);
    }

    rr_remove(zip);

    return true;
}

/*
tests_report_runner_archive_rar_is_unavailable
  the format the writer will never produce.
  Tests the following:
  - rar creation is tool-only, so nothing is written
  - no container appears under the requested path
  - the console reports the document as not written
*/
bool tests_report_runner_archive_rar_is_unavailable()
{
    DRrCapture cap("dtest_rr_rar_console.txt");

    D_RR_CHECK(cap.valid());

    const std::string path = "dtest_rr_never.rar";

    rr_remove(path);

    dt::report_builder rb;

    rb.set_file(cap.file());
    rb.set_console(false);
    rb.use_pdf(path);
    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::rar);

    rb.module("m");
    rr_seed_units(rb, 1, 0);
    rb.set_console(true);

    const int rc = rb.finish();

    D_RR_CHECK(!rr_file_exists(path));
    D_RR_CHECK(!rr_file_exists("report.rar"));

    const std::string out = cap.text();

    D_RR_CHECK(!rr_contains(out, "wrote report: "));
    D_RR_CHECK(rr_contains(out, "(report document was not written)"));

    // a failed package does not fail the run
    D_RR_CHECK(rc == 0);

    return true;
}

/*
tests_report_runner_archive_pack_none_stays_loose
  packaging opted out of.
  Tests the following:
  - with packaging compiled in but the pack knob at none, the PDF is still
    written loose
  - no container is produced
  - selecting an archive format alone does not enable packaging
  The knob is set to none here rather than left at its default, which a
  build's D_CFG_TEST_OUTPUT_PACK can change.
*/
bool tests_report_runner_archive_pack_none_stays_loose()
{
    const std::string path = "dtest_rr_loose.pdf";

    rr_remove(path);
    rr_remove("report.zip");

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(path);
    rr_set_pack(rb.options(), dt::DTestOutputPack::none);

    // a format is selected, but the pack knob is left at none
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);

    D_RR_CHECK(dt::pack(rb.options()) == dt::DTestOutputPack::none);

    rb.module("m");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(path));
    D_RR_CHECK(!rr_file_exists("report.zip"));

    const std::string bytes = rr_read_file(path);

    // the file is the raw PDF, not a container
    D_RR_CHECK(bytes.compare(0, 5, "%PDF-") == 0);

    rr_remove(path);

    return true;
}

/*
tests_report_runner_archive_store_only_stores
  the stored choice.
  Tests the following:
  - archive_opts.store_only writes every entry with method 0, stored
  - so the PDF reads in the container's own bytes, without a codec
  - and the entry reads back whole through the archive reader too
*/
bool tests_report_runner_archive_store_only_stores()
{
    const std::string zip = "dtest_rr_stored.zip";

    rr_remove(zip);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(zip);
    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);
    rr_set_entries(rb.options(), 1, -1);

    rb.module("kept_as_is");
    rr_seed_units(rb, 2, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(zip));

    const std::string      bytes   = rr_read_file(zip);
    const std::vector<int> methods = rr_zip_methods(bytes);

    // one entry, stored
    D_RR_CHECK(methods.size() == 1u);
    D_RR_CHECK(methods[0] == 0);

    // the PDF is in the container's bytes as it is
    D_RR_CHECK(rr_contains(bytes, "%PDF-"));
    D_RR_CHECK(rr_contains(bytes, "kept_as_is"));

    const ::djinterp::entry_list entries = rr_zip_entries(bytes);

    D_RR_CHECK(entries.size() == 1u);
    D_RR_CHECK(entries[0].data.compare(0, 5, "%PDF-") == 0);

    rr_remove(zip);

    return true;
}

/*
tests_report_runner_archive_deflate_compresses
  the compressed choice.
  Tests the following:
  - store_only off and the deflate method write every entry deflated, where
    the build has a DEFLATE provider, and stored where it has none
  - the entries read back whole through the archive reader
  - a deflated container is smaller than the stored one of the same report
*/
bool tests_report_runner_archive_deflate_compresses()
{
    const std::string deflated = "dtest_rr_deflated.zip";
    const std::string stored   = "dtest_rr_stored_twin.zip";
    const bool        can_deflate =
        (d_codec_can_compress(D_CODEC_ID_DEFLATE) != 0);

    rr_remove(deflated);
    rr_remove(stored);

    // the same report twice, once each way
    for (int pass = 0; pass < 2; ++pass)
    {
        dt::report_builder rb;

        rb.set_console(false);
        rb.use_pdf((pass == 0) ? deflated : stored);
        rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
        rr_set_archive_format(rb.options(), dt::test_archive_format::zip);
        rr_set_entries(rb.options(),
                       pass,
                       (pass == 0) ? 8 : -1);

        rb.module("squeezed");
        rr_seed_units(rb, 4, 1);
        rb.finish();
    }

    D_RR_CHECK(rr_file_exists(deflated));
    D_RR_CHECK(rr_file_exists(stored));

    const std::string      bytes   = rr_read_file(deflated);
    const std::vector<int> methods = rr_zip_methods(bytes);

    // one entry, deflated where the build can
    D_RR_CHECK(methods.size() == 1u);
    D_RR_CHECK(methods[0] == (can_deflate ? 8 : 0));

    // it reads back whole, and is the same document the stored twin holds
    const ::djinterp::entry_list entries = rr_zip_entries(bytes);
    const ::djinterp::entry_list twin    =
        rr_zip_entries(rr_read_file(stored));

    D_RR_CHECK(entries.size() == 1u);
    D_RR_CHECK(twin.size() == 1u);
    D_RR_CHECK(entries[0].data.compare(0, 5, "%PDF-") == 0);
    D_RR_CHECK(rr_contains(entries[0].data, "squeezed"));
    D_RR_CHECK(entries[0].data.size() == twin[0].data.size());

    // deflating an uncompressed PDF makes the container smaller
    if (can_deflate)
    {
        D_RR_CHECK(bytes.size() < rr_read_file(stored).size());
    }

    rr_remove(deflated);
    rr_remove(stored);

    return true;
}

/*
tests_report_runner_archive_default_follows_build
  the sensible default.
  Tests the following:
  - with archive_opts left as default_test_options() made it, every entry
    takes the method the build's knobs give it: cfg_test_options.h's, then
    cfg_archive.h's, which by default is deflate where the build can
  - the entry reads back whole either way
*/
bool tests_report_runner_archive_default_follows_build()
{
    const std::string zip      = "dtest_rr_default.zip";
    const int         expected = rr_default_zip_method();

    rr_remove(zip);

    dt::report_builder rb;

    rb.set_console(false);
    rb.use_pdf(zip);
    rr_set_pack(rb.options(), dt::DTestOutputPack::archive);
    rr_set_archive_format(rb.options(), dt::test_archive_format::zip);

    rb.module("defaulted");
    rr_seed_units(rb, 1, 0);
    rb.finish();

    D_RR_CHECK(rr_file_exists(zip));

    const std::string      bytes   = rr_read_file(zip);
    const std::vector<int> methods = rr_zip_methods(bytes);

    D_RR_CHECK(methods.size() == 1u);

    // the method the knobs give, where this suite can work it out
    if (expected >= 0)
    {
        D_RR_CHECK(methods[0] == expected);
    }

    const ::djinterp::entry_list entries = rr_zip_entries(bytes);

    D_RR_CHECK(entries.size() == 1u);
    D_RR_CHECK(entries[0].data.compare(0, 5, "%PDF-") == 0);

    rr_remove(zip);

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_TEST_REPORT_ENABLE_ARCHIVE
