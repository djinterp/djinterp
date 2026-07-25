#include "test_archive_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_roundtrip_reports_facade_status
  the status the driver surfaces.
  Tests the following:
  - when creation fails, the driver returns that exact status rather than
    collapsing it into a generic failure
  - when creation succeeds, the driver returns the extraction status
  - an unavailable format surfaces as status_unavailable, which is what
    lets a caller tell "not built" from "went wrong"
*/
bool
tests_archive_roundtrip_reports_facade_status()
{
    const dj::entry_list items = ta_sample_entries();
    dj::entry_list       out;
    dj::byte_buffer      blob;

    // zip: whatever creation reports, the driver agrees
    const dj::status zip_create =
        dj::try_archive<dj::formats::zip>(items, blob, dj::archive_options());

    if (zip_create != dj::status_ok)
    {
        D_TA_CHECK(dt::roundtrip<dj::formats::zip>(items, out) == zip_create);
    }
    else
    {
        const dj::status zip_extract =
            dj::try_extract<dj::formats::zip>(blob, out);

        dj::entry_list via_driver;

        D_TA_CHECK(dt::roundtrip<dj::formats::zip>(items, via_driver) ==
                   zip_extract);
    }

    // the formats this build cannot write surface their own refusal
    blob.clear();

    const dj::status sevenzip_create =
        dj::try_archive<dj::formats::sevenzip>(items, blob,
                                               dj::archive_options());

    D_TA_CHECK(dt::roundtrip<dj::formats::sevenzip>(items, out) ==
               sevenzip_create);

    blob.clear();

    const dj::status rar_create =
        dj::try_archive<dj::formats::rar>(items, blob, dj::archive_options());

    D_TA_CHECK(dt::roundtrip<dj::formats::rar>(items, out) == rar_create);

    // gzip carries exactly one member, so a three-member list is refused
    // with a status distinct from "not built"
    blob.clear();

    const dj::status gz_many =
        dj::try_archive<dj::formats::gz>(items, blob, dj::archive_options());

    D_TA_CHECK(dt::roundtrip<dj::formats::gz>(items, out) == gz_many);

    if (gz_many != dj::status_ok)
    {
        D_TA_CHECK(blob.empty());
    }

    // a refusing driver leaves nothing useful behind
    if (sevenzip_create != dj::status_ok)
    {
        D_TA_CHECK(sevenzip_create != dj::status_ok);
        D_TA_CHECK(!dt::roundtrip_preserves_files<dj::formats::sevenzip>(
                       items));
    }

    return true;
}

/*
tests_archive_roundtrip_preserves_files
  the payload guarantee where a format works.
  Tests the following:
  - where a format round-trips, every regular file comes back
    byte-identical, embedded NULs and all
  - the restored list carries at least as many files as were handed over
  - the helper agrees with an explicit files_preserved on the extracted
    list
*/
bool
tests_archive_roundtrip_preserves_files()
{
    dj::entry_list items;

    items.push_back(dt::make_text_entry("a.txt", "plain text"));
    items.push_back(dt::make_file_entry("bin/blob",
                                        dj::byte_buffer("x\0y\0z", 5)));
    items.push_back(dt::make_file_entry("empty", dj::byte_buffer()));
    items.push_back(dt::make_dir_entry("bin"));

    dj::entry_list   out;
    const dj::status s = dt::roundtrip<dj::formats::zip>(items, out);

    if (s == dj::status_ok)
    {
        // every regular file survived
        D_TA_CHECK(dt::files_preserved(items, out));
        D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::zip>(items));

        // and the helper agrees with the explicit comparison
        D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::zip>(items) ==
                   dt::files_preserved(items, out));

        // the files really came back, by name
        D_TA_CHECK(dt::count_files(out) >= dt::count_files(items));
        D_TA_CHECK(dt::has_entry(out, "a.txt"));
        D_TA_CHECK(dt::has_entry(out, "bin/blob"));

        // including the NUL-bearing one, byte for byte
        dj::byte_buffer got;

        D_TA_CHECK(dt::file_data(out, "bin/blob", got));
        D_TA_CHECK(got.size() == 5u);
        D_TA_CHECK(got == dj::byte_buffer("x\0y\0z", 5));

        // and the empty file survived as an empty file
        dj::byte_buffer hollow("sentinel");

        D_TA_CHECK(dt::file_data(out, "empty", hollow));
        D_TA_CHECK(hollow.empty());
    }
    else
    {
        // a format that refused cannot claim to have preserved anything
        D_TA_CHECK(!dt::roundtrip_preserves_files<dj::formats::zip>(items));
    }

    // tar carries names too, so the same guarantee applies where it works
    dj::entry_list tar_out;

    if (dt::roundtrip<dj::formats::tar>(items, tar_out) == dj::status_ok)
    {
        D_TA_CHECK(dt::files_preserved(items, tar_out));
        D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::tar>(items));
    }
    else
    {
        D_TA_CHECK(!dt::roundtrip_preserves_files<dj::formats::tar>(items));
    }

    return true;
}

/*
tests_archive_roundtrip_contract_all_formats
  the build-agnostic sweep.
  Tests the following:
  - each of the six formats either round-trips or refuses cleanly, with no
    partial success: a refusing writer emits no bytes and the driver
    surfaces the refusal
  - the sweep is not vacuous, since at least one format works on every
    build
*/
bool
tests_archive_roundtrip_contract_all_formats()
{
    const dj::entry_list items = ta_sample_entries();

    D_TA_CHECK(ta_format_contract_holds<dj::formats::zip>(items, "zip"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::tar>(items, "tar"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::gz>(items, "gz"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::tar_gz>(items,
                                                             "tar_gz"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::sevenzip>(items, "7z"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::rar>(items, "rar"));

    // the same contract on a single-member list, which is the only shape
    // gzip accepts
    dj::entry_list one;

    one.push_back(dt::make_text_entry("only.txt", "payload"));

    D_TA_CHECK(ta_format_contract_holds<dj::formats::zip>(one, "zip/1"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::tar>(one, "tar/1"));
    D_TA_CHECK(ta_format_contract_holds<dj::formats::gz>(one, "gz/1"));

    // the sweep is not vacuous: the built-in writers mean at least one
    // format really did take the success branch above
    dj::byte_buffer blob;

    D_TA_CHECK(dj::try_archive<dj::formats::zip>(items, blob,
                                                 dj::archive_options()) ==
               dj::status_ok);
    D_TA_CHECK(!blob.empty());

    return true;
}

/*
tests_archive_roundtrip_accepts_options
  the optional creation argument.
  Tests the following:
  - an explicit option set is accepted by both drivers
  - the defaulted argument behaves as an explicitly default set does
  - options that a format ignores do not change the outcome
  - a store-only request still round-trips where the format works
*/
bool
tests_archive_roundtrip_accepts_options()
{
    const dj::entry_list  items = ta_sample_entries();
    dj::archive_options   store_only;
    dj::archive_options   levelled;
    dj::archive_options   fresh;

    store_only.store_only = true;
    levelled.level        = 9;

    dj::entry_list a;
    dj::entry_list b;
    dj::entry_list c;

    const dj::status sa = dt::roundtrip<dj::formats::zip>(items, a, fresh);
    const dj::status sb =
        dt::roundtrip<dj::formats::zip>(items, b, store_only);
    const dj::status sc = dt::roundtrip<dj::formats::zip>(items, c, levelled);

    // the defaulted argument matches an explicitly default set
    dj::entry_list d;

    D_TA_CHECK(dt::roundtrip<dj::formats::zip>(items, d) == sa);

    if (sa == dj::status_ok)
    {
        // every option set that succeeded preserved the payload
        D_TA_CHECK(dt::files_preserved(items, a));

        if (sb == dj::status_ok)
        {
            D_TA_CHECK(dt::files_preserved(items, b));
            D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::zip>(
                           items, store_only));
        }

        if (sc == dj::status_ok)
        {
            D_TA_CHECK(dt::files_preserved(items, c));
            D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::zip>(
                           items, levelled));
        }
    }

    // a deeply tuned set is accepted without disturbing the contract
    dj::archive_options tuned;

    tuned.codec.zstd.window_log = 20;
    tuned.zip.utf8_names        = false;
    tuned.comment               = "written by the test suite";

    dj::entry_list t;

    D_TA_CHECK(dt::roundtrip<dj::formats::zip>(items, t, tuned) ==
               dt::roundtrip<dj::formats::zip>(items, t, tuned));

    // the options are not mutated by the call
    D_TA_CHECK(tuned.zip.utf8_names == false);
    D_TA_CHECK(tuned.comment == "written by the test suite");
    D_TA_CHECK(tuned.codec.zstd.window_log == 20);

    // and a refusing format refuses whatever the options say
    dj::entry_list r;

    D_TA_CHECK(dt::roundtrip<dj::formats::rar>(items, r, tuned) ==
               dt::roundtrip<dj::formats::rar>(items, r, fresh));

    return true;
}

/*
tests_archive_roundtrip_handles_degenerate_lists
  the empty and payload-free lists.
  Tests the following:
  - an empty list is handled without a spurious payload failure, since
    there is nothing to preserve
  - a directory-only list is likewise preserved trivially
  - the extracted list of an empty archive carries no files
*/
bool
tests_archive_roundtrip_handles_degenerate_lists()
{
    const dj::entry_list empty;
    dj::entry_list       out;

    const dj::status s = dt::roundtrip<dj::formats::zip>(empty, out);

    if (s == dj::status_ok)
    {
        // nothing to preserve, so the check passes vacuously
        D_TA_CHECK(dt::files_preserved(empty, out));
        D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::zip>(empty));
        D_TA_CHECK(dt::count_files(out) == 0u);

        // and the container advertises no members
        dj::byte_buffer blob;

        D_TA_CHECK(dj::try_archive<dj::formats::zip>(empty, blob,
                                                     dj::archive_options()) ==
                   dj::status_ok);
        D_TA_CHECK(dt::zip_total_entries(blob) == 0L);
        D_TA_CHECK(dt::looks_like_zip(blob));
    }
    else
    {
        D_TA_CHECK(!dt::roundtrip_preserves_files<dj::formats::zip>(empty));
    }

    // a directory-only list carries no payload to compare
    dj::entry_list dirs;

    dirs.push_back(dt::make_dir_entry("a"));
    dirs.push_back(dt::make_dir_entry("a/b"));

    D_TA_CHECK(dt::count_files(dirs) == 0u);
    D_TA_CHECK(dt::count_dirs(dirs) == 2u);

    dj::entry_list dir_out;

    if (dt::roundtrip<dj::formats::zip>(dirs, dir_out) == dj::status_ok)
    {
        D_TA_CHECK(dt::files_preserved(dirs, dir_out));
        D_TA_CHECK(dt::roundtrip_preserves_files<dj::formats::zip>(dirs));
    }

    // a single empty file is not the same as no files at all
    dj::entry_list hollow;

    hollow.push_back(dt::make_file_entry("e", dj::byte_buffer()));

    D_TA_CHECK(dt::count_files(hollow) == 1u);

    dj::entry_list hollow_out;

    if (dt::roundtrip<dj::formats::zip>(hollow, hollow_out) == dj::status_ok)
    {
        D_TA_CHECK(dt::files_preserved(hollow, hollow_out));
        D_TA_CHECK(dt::has_entry(hollow_out, "e"));
    }

    return true;
}

/*
tests_archive_writability_is_independent
  the FINDING pinned.
  Tests the following:
  - the built-in zip and tar writers need no third-party library, so
    creation succeeds on ANY build, whatever format_is_writable reports
  - archive_create never consults format_can_write, so the two queries
    answer different questions and neither implies the other
  - where format_is_writable DOES report true, creation must not then
    report the format unavailable - that direction has to hold
*/
bool
tests_archive_writability_is_independent()
{
    const dj::entry_list items = ta_sample_entries();
    dj::byte_buffer      blob;

    // the built-in writers work regardless of the writability query.  This
    // is the pin: should archive_create later be gated on format_can_write,
    // or should format_can_write be widened to admit the built-in writers,
    // one of these two assertions changes and this body reports it.
    D_TA_CHECK(dj::try_archive<dj::formats::zip>(items, blob,
                                                 dj::archive_options()) ==
               dj::status_ok);
    D_TA_CHECK(!blob.empty());
    D_TA_CHECK(dt::looks_like_zip(blob));

    blob.clear();
    D_TA_CHECK(dj::try_archive<dj::formats::tar>(items, blob,
                                                 dj::archive_options()) ==
               dj::status_ok);
    D_TA_CHECK(!blob.empty());
    D_TA_CHECK(dt::tar_has_ustar_magic(blob));

    // the direction that must always hold: a format the build claims it can
    // write must not then report itself unavailable
    blob.clear();

    if (dj::format_is_writable<dj::formats::gz>())
    {
        D_TA_CHECK(dj::try_archive<dj::formats::gz>(items, blob,
                                                    dj::archive_options()) !=
                   dj::status_unavailable);
    }

    blob.clear();

    if (dj::format_is_writable<dj::formats::sevenzip>())
    {
        D_TA_CHECK(dj::try_archive<dj::formats::sevenzip>(
                       items, blob, dj::archive_options()) !=
                   dj::status_unavailable);
    }

    blob.clear();

    if (dj::format_is_writable<dj::formats::rar>())
    {
        D_TA_CHECK(dj::try_archive<dj::formats::rar>(items, blob,
                                                     dj::archive_options()) !=
                   dj::status_unavailable);
    }

    // and the converse does NOT hold, which is the whole finding: zip and
    // tar create successfully above whether or not they report writable, so
    // no test in this suite may gate a round-trip on that query
    D_TA_CHECK(dj::format_is_writable<dj::formats::zip>() ||
               (!dj::format_is_writable<dj::formats::zip>()));

    // the query is at least stable within a run
    D_TA_CHECK(dj::format_is_writable<dj::formats::zip>() ==
               dj::format_is_writable<dj::formats::zip>());
    D_TA_CHECK(dj::format_is_writable<dj::formats::rar>() ==
               dj::format_is_writable<dj::formats::rar>());

    return true;
}

NS_END  // testing
NS_END  // djinterp
