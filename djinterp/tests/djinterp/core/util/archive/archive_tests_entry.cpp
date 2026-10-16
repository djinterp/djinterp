/*******************************************************************************
* djinterp [test]                                        archive_tests_entry.cpp
*
*   Tests for the archive entry model: the entry struct's field defaults,
* entry_list basics, and the entry-construction helpers.
*
*   Part of the archive.hpp / archive.cpp unit-test suite; the declarations,
* shared helpers, and per-suite check macros live in archive_tests.hpp.
* These functions are flat in djinterp::testing and are called by the
* suite runner.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests_entry.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./archive_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_entry_defaults
  Verifies the entry struct's default construction.
  Tests the following:
  - a default-constructed entry is a regular file (is_directory false)
  - mode and mtime default to 0 (the writer's "pick a default" sentinels)
  - name and data default to empty
*/
bool
tests_entry_defaults()
{
    bool  _ok = true;
    entry e;

    D_AR_CHECK(e.is_directory == false);
    D_AR_CHECK(e.mode == 0u);
    D_AR_CHECK(e.mtime == 0L);
    D_AR_CHECK(e.name.empty());
    D_AR_CHECK(e.data.empty());

    return _ok;
}

/*
tests_entry_list_basics
  Verifies the entry_list container and binary-clean payloads.
  Tests the following:
  - entry_list is an ordered, indexable, growable collection
  - entries preserve arbitrary binary data, including embedded NULs and
    high-bit bytes (byte_blob is not NUL-terminated)
*/
bool
tests_entry_list_basics()
{
    bool       _ok = true;
    entry_list items;

    D_AR_CHECK(items.empty());

    {
        entry a;
        a.name = "first";
        a.data = binary_sample();
        items.push_back(a);
    }
    {
        entry b;
        b.name = "second";
        items.push_back(b);
    }

    D_AR_REQUIRE(items.size() == 2u);
    D_AR_CHECK(items[0].name == "first");
    D_AR_CHECK(items[1].name == "second");

    // the binary payload survives verbatim (length + content)
    D_AR_REQUIRE(items[0].data.size() == binary_sample().size());
    D_AR_CHECK(items[0].data == binary_sample());
    D_AR_CHECK(items[0].data.size() == 11u);

    return _ok;
}

/*
tests_entry_helpers
  Verifies the shared djinterp::test entry builders and list inspectors that
the rest of the suite (and other modules' tests) rely on.
  Tests the following:
  - make_file_entry / make_text_entry build regular files; make_dir_entry
    builds a directory; make_entry carries all fields through
  - find_entry / has_entry locate members with trailing-slash tolerance
  - count_files / count_dirs partition a list correctly
  - normalize_name strips exactly one trailing slash
*/
bool
tests_entry_helpers()
{
    bool _ok = true;

    entry f = ::djinterp::test::make_file_entry("a.txt", binary_sample());
    D_AR_CHECK(f.name == "a.txt");
    D_AR_CHECK(f.is_directory == false);
    D_AR_CHECK(f.data == binary_sample());

    entry t = ::djinterp::test::make_text_entry("b.txt", "hello");
    D_AR_CHECK(t.is_directory == false);
    D_AR_CHECK(t.data == byte_blob("hello"));

    entry d = ::djinterp::test::make_dir_entry("dir");
    D_AR_CHECK(d.name == "dir");
    D_AR_CHECK(d.is_directory == true);
    D_AR_CHECK(d.data.empty());

    entry full = ::djinterp::test::make_entry("c.bin",
                                              byte_blob("xyz"),
                                              false,
                                              0640u,
                                              123456789L);
    D_AR_CHECK(full.mode == 0640u);
    D_AR_CHECK(full.mtime == 123456789L);

    // build a small list and inspect it
    entry_list items;
    items.push_back(f);
    items.push_back(d);
    items.push_back(t);

    D_AR_CHECK(::djinterp::test::has_entry(items, "a.txt"));
    D_AR_CHECK(::djinterp::test::has_entry(items, "dir"));
    D_AR_CHECK(::djinterp::test::has_entry(items, "dir/"));   // slash-tolerant
    D_AR_CHECK(!::djinterp::test::has_entry(items, "missing"));

    D_AR_CHECK(::djinterp::test::count_files(items) == 2u);
    D_AR_CHECK(::djinterp::test::count_dirs(items) == 1u);

    D_AR_CHECK(::djinterp::test::normalize_name("dir/") == "dir");
    D_AR_CHECK(::djinterp::test::normalize_name("dir") == "dir");
    D_AR_CHECK(::djinterp::test::normalize_name("a/b/") == "a/b");
    D_AR_CHECK(::djinterp::test::normalize_name("") == "");

    {
        const entry* got = ::djinterp::test::find_entry(items, "b.txt");
        D_AR_REQUIRE(got != (const entry*)0);
        D_AR_CHECK(got->data == byte_blob("hello"));
    }

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
