#include "test_archive_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_file_data_fetches_contents
  the single-member fetch.
  Tests the following:
  - a regular file's bytes are copied into the caller's buffer
  - an empty payload is fetched successfully rather than read as a miss
  - the lookup inherits the trailing-separator tolerance
  - binary payloads survive the copy intact
*/
bool
tests_archive_file_data_fetches_contents()
{
    const dj::entry_list items = ta_sample_entries();
    dj::byte_buffer      out;

    D_TA_CHECK(dt::file_data(items, "readme.txt", out));
    D_TA_CHECK(out == dj::byte_buffer("hello, world"));

    // a binary payload
    dj::byte_buffer bin;

    D_TA_CHECK(dt::file_data(items, "data/blob.bin", bin));
    D_TA_CHECK(bin.size() == 5u);
    D_TA_CHECK(bin == dj::byte_buffer("a\0b\0c", 5));

    // an empty payload is a success, not a miss
    dj::entry_list hollow;

    hollow.push_back(dt::make_file_entry("empty.txt", dj::byte_buffer()));

    dj::byte_buffer e("previous contents");

    D_TA_CHECK(dt::file_data(hollow, "empty.txt", e));
    D_TA_CHECK(e.empty());

    // the trailing-separator tolerance carries over
    dj::entry_list slashed;

    slashed.push_back(dt::make_file_entry("odd/", dj::byte_buffer("v")));

    dj::byte_buffer s;

    D_TA_CHECK(dt::file_data(slashed, "odd", s));
    D_TA_CHECK(s == dj::byte_buffer("v"));

    return true;
}

/*
tests_archive_file_data_rejects_directory_and_absent
  the two ways the fetch fails.
  Tests the following:
  - a directory member is refused even though the name matches
  - a missing name is refused
  - on either failure the caller's buffer is left untouched, so a stale
    value is never mistaken for a fetched one
*/
bool
tests_archive_file_data_rejects_directory_and_absent()
{
    const dj::entry_list items = ta_sample_entries();
    dj::byte_buffer      out("sentinel");

    // a directory is refused
    D_TA_CHECK(!dt::file_data(items, "data", out));
    D_TA_CHECK(out == dj::byte_buffer("sentinel"));

    // including when queried with the trailing separator
    D_TA_CHECK(!dt::file_data(items, "data/", out));
    D_TA_CHECK(out == dj::byte_buffer("sentinel"));

    // a missing name is refused
    D_TA_CHECK(!dt::file_data(items, "absent.txt", out));
    D_TA_CHECK(out == dj::byte_buffer("sentinel"));

    // so is the empty name
    D_TA_CHECK(!dt::file_data(items, "", out));
    D_TA_CHECK(out == dj::byte_buffer("sentinel"));

    // and any lookup on an empty list
    const dj::entry_list empty;

    D_TA_CHECK(!dt::file_data(empty, "anything", out));
    D_TA_CHECK(out == dj::byte_buffer("sentinel"));

    // the guard really is the flag, not the payload: a directory with a
    // name that also exists as a file resolves to whichever comes first
    dj::entry_list shadow;

    shadow.push_back(dt::make_dir_entry("name"));
    shadow.push_back(dt::make_file_entry("name", dj::byte_buffer("data")));

    D_TA_CHECK(!dt::file_data(shadow, "name", out));
    D_TA_CHECK(out == dj::byte_buffer("sentinel"));

    return true;
}

/*
tests_archive_files_preserved_accepts_faithful_copy
  the round-trip payload check's accepting case.
  Tests the following:
  - an identical list passes
  - order is ignored, since a writer may reorder members
  - extra members in the restored list are tolerated, since a format may
    synthesise parent directories
  - mode and mtime are ignored, as documented
*/
bool
tests_archive_files_preserved_accepts_faithful_copy()
{
    const dj::entry_list items = ta_sample_entries();

    // identical
    D_TA_CHECK(dt::files_preserved(items, items));

    // reordered
    dj::entry_list shuffled;

    shuffled.push_back(items[2]);
    shuffled.push_back(items[1]);
    shuffled.push_back(items[0]);

    D_TA_CHECK(dt::files_preserved(items, shuffled));

    // extra members are tolerated
    dj::entry_list extra = items;

    extra.push_back(dt::make_dir_entry("data/"));
    extra.push_back(dt::make_text_entry("added.txt", "new"));

    D_TA_CHECK(dt::files_preserved(items, extra));

    // mode and mtime are not compared
    dj::entry_list retimed;
    std::size_t    i = 0;

    for (i = 0; i < items.size(); ++i)
    {
        retimed.push_back(dt::make_entry(items[i].name,
                                         items[i].data,
                                         items[i].is_directory,
                                         0777u,
                                         999999L));
    }

    D_TA_CHECK(dt::files_preserved(items, retimed));

    // a restored list whose directory gained a trailing separator still
    // matches, since the lookup normalises
    dj::entry_list zipstyle;

    zipstyle.push_back(items[0]);
    zipstyle.push_back(items[1]);
    zipstyle.push_back(dt::make_dir_entry("data/"));

    D_TA_CHECK(dt::files_preserved(items, zipstyle));

    // an empty original is trivially preserved by anything
    const dj::entry_list empty;

    D_TA_CHECK(dt::files_preserved(empty, items));
    D_TA_CHECK(dt::files_preserved(empty, empty));

    return true;
}

/*
tests_archive_files_preserved_rejects_damage
  the round-trip payload check's rejecting case.
  Tests the following:
  - a missing file fails
  - altered bytes fail, including a single flipped byte
  - a file that came back as a directory fails
  - a truncated or extended payload fails
*/
bool
tests_archive_files_preserved_rejects_damage()
{
    const dj::entry_list items = ta_sample_entries();

    // a missing file
    dj::entry_list missing;

    missing.push_back(items[0]);
    missing.push_back(items[2]);

    D_TA_CHECK(!dt::files_preserved(items, missing));

    // an empty restored list
    const dj::entry_list empty;

    D_TA_CHECK(!dt::files_preserved(items, empty));

    // one flipped byte
    dj::entry_list flipped = items;

    flipped[0].data[0] = static_cast<char>(flipped[0].data[0] ^ 0x01);
    D_TA_CHECK(!dt::files_preserved(items, flipped));

    // a truncated payload
    dj::entry_list shortened = items;

    shortened[0].data = shortened[0].data.substr(0, 5);
    D_TA_CHECK(!dt::files_preserved(items, shortened));

    // an extended payload
    dj::entry_list lengthened = items;

    lengthened[0].data += "!";
    D_TA_CHECK(!dt::files_preserved(items, lengthened));

    // a file demoted to a directory
    dj::entry_list demoted = items;

    demoted[0].is_directory = true;
    D_TA_CHECK(!dt::files_preserved(items, demoted));

    // a renamed file is a missing file
    dj::entry_list renamed = items;

    renamed[0].name = "elsewhere.txt";
    D_TA_CHECK(!dt::files_preserved(items, renamed));

    // the binary member's NULs matter too
    dj::entry_list nulled = items;

    nulled[1].data = dj::byte_buffer("a\0b\0d", 5);
    D_TA_CHECK(!dt::files_preserved(items, nulled));

    return true;
}

/*
tests_archive_files_preserved_ignores_directories
  the documented exemption.
  Tests the following:
  - a directory in the original is not looked up in the restored list, so
    a format that drops directories still passes
  - a directory whose payload differs is still ignored
  - only regular files decide the verdict
*/
bool
tests_archive_files_preserved_ignores_directories()
{
    dj::entry_list original;

    original.push_back(dt::make_text_entry("f.txt", "keep"));
    original.push_back(dt::make_dir_entry("d1"));
    original.push_back(dt::make_dir_entry("d2"));

    // the restored list drops both directories
    dj::entry_list no_dirs;

    no_dirs.push_back(dt::make_text_entry("f.txt", "keep"));

    D_TA_CHECK(dt::files_preserved(original, no_dirs));
    D_TA_CHECK(dt::count_dirs(no_dirs) == 0u);

    // a directory carrying an unexpected payload is still ignored, since
    // the loop skips it before the lookup
    dj::entry_list odd_dirs = original;

    odd_dirs[1].data = "unexpected";
    D_TA_CHECK(dt::files_preserved(odd_dirs, no_dirs));

    // but the one regular file still decides the verdict
    dj::entry_list broken;

    broken.push_back(dt::make_text_entry("f.txt", "changed"));

    D_TA_CHECK(!dt::files_preserved(original, broken));

    // a list of nothing but directories is preserved by anything, even an
    // empty restored list
    dj::entry_list only_dirs;

    only_dirs.push_back(dt::make_dir_entry("a"));
    only_dirs.push_back(dt::make_dir_entry("b/"));

    const dj::entry_list empty;

    D_TA_CHECK(dt::files_preserved(only_dirs, empty));
    D_TA_CHECK(dt::count_files(only_dirs) == 0u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
