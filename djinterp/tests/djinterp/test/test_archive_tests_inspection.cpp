#include "test_archive_tests.hpp"

NS_DJINTERP
NS_TESTING


// ta_no_entry
//   constant: the null result find_entry returns on a miss.
static const dj::entry* const ta_no_entry = 0;


/*
tests_archive_normalize_name_drops_slash
  the normalisation the lookups rest on.
  Tests the following:
  - a single trailing separator is removed
  - a name without one is returned unchanged
  - exactly ONE separator is removed, so a name ending in two keeps one
  - interior separators are untouched
*/
bool
tests_archive_normalize_name_drops_slash()
{
    D_TA_CHECK(dt::normalize_name("dir/") == "dir");
    D_TA_CHECK(dt::normalize_name("dir") == "dir");
    D_TA_CHECK(dt::normalize_name("a/b/c/") == "a/b/c");
    D_TA_CHECK(dt::normalize_name("a/b/c") == "a/b/c");

    // only one is removed
    D_TA_CHECK(dt::normalize_name("dir//") == "dir/");
    D_TA_CHECK(dt::normalize_name("dir///") == "dir//");

    // interior separators are preserved
    D_TA_CHECK(dt::normalize_name("a/b/") == "a/b");
    D_TA_CHECK(dt::normalize_name("deeply/nested/path/") ==
               "deeply/nested/path");

    // the operation is idempotent only after the one separator is gone
    D_TA_CHECK(dt::normalize_name(dt::normalize_name("dir/")) == "dir");
    D_TA_CHECK(dt::normalize_name(dt::normalize_name("dir//")) == "dir");

    return true;
}

/*
tests_archive_normalize_name_edge_cases
  the degenerate names.
  Tests the following:
  - the empty name is returned unchanged rather than indexed into
  - a lone separator normalises to the empty name
  - a leading separator is not a trailing one and is preserved
  - a name that is only separators loses exactly one
*/
bool
tests_archive_normalize_name_edge_cases()
{
    // the empty name must not be indexed
    D_TA_CHECK(dt::normalize_name("") == "");
    D_TA_CHECK(dt::normalize_name("").empty());

    // a lone separator
    D_TA_CHECK(dt::normalize_name("/") == "");
    D_TA_CHECK(dt::normalize_name("/").empty());

    // a leading separator is preserved
    D_TA_CHECK(dt::normalize_name("/abs") == "/abs");
    D_TA_CHECK(dt::normalize_name("/abs/") == "/abs");

    // only separators
    D_TA_CHECK(dt::normalize_name("//") == "/");

    // a single character that is not a separator
    D_TA_CHECK(dt::normalize_name("a") == "a");

    // a name whose separator is not last
    D_TA_CHECK(dt::normalize_name("a/b") == "a/b");

    return true;
}

/*
tests_archive_find_entry_locates_by_name
  the basic lookup.
  Tests the following:
  - a present member is found
  - the returned pointer addresses the list's own element, not a copy
  - the located entry carries the payload it was built with
*/
bool
tests_archive_find_entry_locates_by_name()
{
    const dj::entry_list items = ta_sample_entries();
    const dj::entry*     e     = dt::find_entry(items, "readme.txt");

    D_TA_CHECK(e != ta_no_entry);
    D_TA_CHECK(e->name == "readme.txt");
    D_TA_CHECK(e->data == dj::byte_buffer("hello, world"));
    D_TA_CHECK(!e->is_directory);

    // the pointer addresses the list's own element
    D_TA_CHECK(e == &items[0]);

    // a nested name is found too
    const dj::entry* n = dt::find_entry(items, "data/blob.bin");

    D_TA_CHECK(n != ta_no_entry);
    D_TA_CHECK(n == &items[1]);
    D_TA_CHECK(n->data.size() == 5u);

    // and the directory
    const dj::entry* d = dt::find_entry(items, "data");

    D_TA_CHECK(d != ta_no_entry);
    D_TA_CHECK(d->is_directory);
    D_TA_CHECK(d == &items[2]);

    return true;
}

/*
tests_archive_find_entry_is_slash_tolerant
  the trailing-separator tolerance.
  Tests the following:
  - a stored name with a trailing separator is found by a query without one
  - a stored name without one is found by a query with one
  - both sides carrying one still match
  - the tolerance is trailing-only: an interior separator must match exactly
*/
bool
tests_archive_find_entry_is_slash_tolerant()
{
    dj::entry_list items;

    items.push_back(dt::make_dir_entry("stored/"));      // zip style
    items.push_back(dt::make_dir_entry("plain"));        // tar style

    // stored with a separator, queried without
    D_TA_CHECK(dt::find_entry(items, "stored") != ta_no_entry);
    D_TA_CHECK(dt::find_entry(items, "stored/") != ta_no_entry);

    // stored without, queried with
    D_TA_CHECK(dt::find_entry(items, "plain") != ta_no_entry);
    D_TA_CHECK(dt::find_entry(items, "plain/") != ta_no_entry);

    // both resolve to the right element
    D_TA_CHECK(dt::find_entry(items, "stored/") == &items[0]);
    D_TA_CHECK(dt::find_entry(items, "plain/") == &items[1]);

    // the tolerance is trailing-only
    dj::entry_list nested;

    nested.push_back(dt::make_dir_entry("a/b"));

    D_TA_CHECK(dt::find_entry(nested, "a/b") != ta_no_entry);
    D_TA_CHECK(dt::find_entry(nested, "a/b/") != ta_no_entry);
    D_TA_CHECK(dt::find_entry(nested, "ab") == ta_no_entry);
    D_TA_CHECK(dt::find_entry(nested, "a") == ta_no_entry);
    D_TA_CHECK(dt::find_entry(nested, "b") == ta_no_entry);

    // a doubled trailing separator is not the same name
    D_TA_CHECK(dt::find_entry(items, "stored//") ==
               ta_no_entry);

    return true;
}

/*
tests_archive_find_entry_returns_null_when_absent
  the miss.
  Tests the following:
  - a missing name yields a null pointer
  - so does any lookup on an empty list
  - the comparison is exact past normalisation, so a prefix or a suffix of
    a stored name does not match
  - the search is case sensitive
*/
bool
tests_archive_find_entry_returns_null_when_absent()
{
    const dj::entry_list items = ta_sample_entries();

    D_TA_CHECK(dt::find_entry(items, "absent.txt") ==
               ta_no_entry);
    D_TA_CHECK(dt::find_entry(items, "") ==
               ta_no_entry);

    // an empty list
    const dj::entry_list empty;

    D_TA_CHECK(dt::find_entry(empty, "readme.txt") ==
               ta_no_entry);
    D_TA_CHECK(dt::find_entry(empty, "") ==
               ta_no_entry);

    // a prefix or suffix is not a match
    D_TA_CHECK(dt::find_entry(items, "readme") ==
               ta_no_entry);
    D_TA_CHECK(dt::find_entry(items, "readme.txt.bak") ==
               ta_no_entry);
    D_TA_CHECK(dt::find_entry(items, "blob.bin") ==
               ta_no_entry);

    // case sensitivity
    D_TA_CHECK(dt::find_entry(items, "README.TXT") ==
               ta_no_entry);

    return true;
}

/*
tests_archive_find_entry_returns_first_match
  duplicate names.
  Tests the following:
  - with two members of the same name the first is returned, which is what
    makes the helper deterministic
  - this holds when the duplicates differ only by trailing separator
  - the second member is still reachable by position
*/
bool
tests_archive_find_entry_returns_first_match()
{
    dj::entry_list items;

    items.push_back(dt::make_text_entry("dup.txt", "first"));
    items.push_back(dt::make_text_entry("dup.txt", "second"));

    const dj::entry* e = dt::find_entry(items, "dup.txt");

    D_TA_CHECK(e != ta_no_entry);
    D_TA_CHECK(e == &items[0]);
    D_TA_CHECK(e->data == dj::byte_buffer("first"));

    // duplicates that differ only by trailing separator
    dj::entry_list slashed;

    slashed.push_back(dt::make_dir_entry("d"));
    slashed.push_back(dt::make_dir_entry("d/"));

    D_TA_CHECK(dt::find_entry(slashed, "d") == &slashed[0]);
    D_TA_CHECK(dt::find_entry(slashed, "d/") == &slashed[0]);

    // the second is still there, just not what the lookup returns
    D_TA_CHECK(slashed.size() == 2u);
    D_TA_CHECK(slashed[1].name == "d/");

    return true;
}

/*
tests_archive_has_entry_matches_find_entry
  the boolean form.
  Tests the following:
  - it reports true exactly when the pointer form finds something
  - it agrees on present names, absent names and the empty list
  - it inherits the same trailing-separator tolerance
*/
bool
tests_archive_has_entry_matches_find_entry()
{
    const dj::entry_list items = ta_sample_entries();
    const char*          names[6] =
        { "readme.txt", "data/blob.bin", "data", "data/", "absent", "" };
    std::size_t          i = 0;

    for (i = 0; i < 6u; ++i)
    {
        const bool found =
            (dt::find_entry(items, names[i]) !=
             ta_no_entry);

        if (dt::has_entry(items, names[i]) != found)
        {
            std::printf("    [FAIL] has_entry disagreed on [%s]\n", names[i]);

            return false;
        }
    }

    // the concrete verdicts
    D_TA_CHECK(dt::has_entry(items, "readme.txt"));
    D_TA_CHECK(dt::has_entry(items, "data"));
    D_TA_CHECK(dt::has_entry(items, "data/"));
    D_TA_CHECK(!dt::has_entry(items, "absent"));
    D_TA_CHECK(!dt::has_entry(items, ""));

    // an empty list has nothing
    const dj::entry_list empty;

    D_TA_CHECK(!dt::has_entry(empty, "anything"));

    return true;
}

/*
tests_archive_count_files_and_dirs
  the member tallies.
  Tests the following:
  - files and directories are counted separately by the flag, not the name
  - the two counts partition the list, so together they always total its
    size
  - an empty list counts zero of each
*/
bool
tests_archive_count_files_and_dirs()
{
    const dj::entry_list items = ta_sample_entries();

    D_TA_CHECK(dt::count_files(items) == 2u);
    D_TA_CHECK(dt::count_dirs(items) == 1u);
    D_TA_CHECK((dt::count_files(items) + dt::count_dirs(items)) ==
               items.size());

    // an empty list
    const dj::entry_list empty;

    D_TA_CHECK(dt::count_files(empty) == 0u);
    D_TA_CHECK(dt::count_dirs(empty) == 0u);

    // all files
    dj::entry_list files;

    files.push_back(dt::make_text_entry("a", "1"));
    files.push_back(dt::make_text_entry("b", "2"));
    files.push_back(dt::make_text_entry("c", "3"));

    D_TA_CHECK(dt::count_files(files) == 3u);
    D_TA_CHECK(dt::count_dirs(files) == 0u);

    // all directories
    dj::entry_list dirs;

    dirs.push_back(dt::make_dir_entry("x"));
    dirs.push_back(dt::make_dir_entry("y/"));

    D_TA_CHECK(dt::count_files(dirs) == 0u);
    D_TA_CHECK(dt::count_dirs(dirs) == 2u);

    // the flag decides, not the name: a "directory-looking" name that is
    // flagged as a file counts as a file
    dj::entry_list tricky;

    tricky.push_back(dt::make_file_entry("looks/like/a/dir/",
                                         dj::byte_buffer("data")));

    D_TA_CHECK(dt::count_files(tricky) == 1u);
    D_TA_CHECK(dt::count_dirs(tricky) == 0u);

    // and an empty-payload file is still a file
    dj::entry_list hollow;

    hollow.push_back(dt::make_file_entry("hollow", dj::byte_buffer()));

    D_TA_CHECK(dt::count_files(hollow) == 1u);
    D_TA_CHECK(dt::count_dirs(hollow) == 0u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
