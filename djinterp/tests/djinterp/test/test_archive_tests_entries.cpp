#include "test_archive_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_archive_make_entry_records_every_field
  the fully specified builder.
  Tests the following:
  - all five arguments reach their own fields unchanged
  - the flags and numbers are independent, so none bleeds into another
  - a directory entry may still be given a mode and an mtime
*/
bool
tests_archive_make_entry_records_every_field()
{
    const dj::entry e = dt::make_entry("path/to/file.txt",
                                       dj::byte_buffer("contents"),
                                       false,
                                       0644u,
                                       1700000000L);

    D_TA_CHECK(e.name == "path/to/file.txt");
    D_TA_CHECK(e.data == dj::byte_buffer("contents"));
    D_TA_CHECK(!e.is_directory);
    D_TA_CHECK(e.mode == 0644u);
    D_TA_CHECK(e.mtime == 1700000000L);

    // a directory with an explicit mode and time
    const dj::entry d = dt::make_entry("path/to/dir",
                                       dj::byte_buffer(),
                                       true,
                                       0755u,
                                       42L);

    D_TA_CHECK(d.name == "path/to/dir");
    D_TA_CHECK(d.is_directory);
    D_TA_CHECK(d.mode == 0755u);
    D_TA_CHECK(d.mtime == 42L);
    D_TA_CHECK(d.data.empty());

    // the fields do not bleed into one another
    const dj::entry z = dt::make_entry("", dj::byte_buffer(), false, 0u, 0L);

    D_TA_CHECK(z.name.empty());
    D_TA_CHECK(z.data.empty());
    D_TA_CHECK(!z.is_directory);
    D_TA_CHECK(z.mode == 0u);
    D_TA_CHECK(z.mtime == 0L);

    // a negative mtime (pre-epoch) is carried as given
    const dj::entry old =
        dt::make_entry("old", dj::byte_buffer("x"), false, 0u, -100L);

    D_TA_CHECK(old.mtime == -100L);

    return true;
}

/*
tests_archive_make_file_entry_defaults
  the regular-file shorthand.
  Tests the following:
  - the entry is flagged as a file rather than a directory
  - mode and mtime are left at zero, the documented "writer picks" values
  - the name and payload are carried through
*/
bool
tests_archive_make_file_entry_defaults()
{
    const dj::entry e =
        dt::make_file_entry("notes.md", dj::byte_buffer("# title"));

    D_TA_CHECK(e.name == "notes.md");
    D_TA_CHECK(e.data == dj::byte_buffer("# title"));
    D_TA_CHECK(!e.is_directory);
    D_TA_CHECK(e.mode == 0u);
    D_TA_CHECK(e.mtime == 0L);

    // it is exactly the fully specified form with those defaults
    const dj::entry expected =
        dt::make_entry("notes.md", dj::byte_buffer("# title"), false, 0u, 0L);

    D_TA_CHECK(e.name == expected.name);
    D_TA_CHECK(e.data == expected.data);
    D_TA_CHECK(e.is_directory == expected.is_directory);
    D_TA_CHECK(e.mode == expected.mode);
    D_TA_CHECK(e.mtime == expected.mtime);

    // an empty payload is a legitimate file
    const dj::entry empty = dt::make_file_entry("empty", dj::byte_buffer());

    D_TA_CHECK(empty.data.empty());
    D_TA_CHECK(!empty.is_directory);

    return true;
}

/*
tests_archive_make_text_entry_matches_file_entry
  the readability alias.
  Tests the following:
  - a text entry is indistinguishable from the byte-buffer form with the
    same content
  - it is a file, not a directory, with the same defaults
  - text containing a NUL is still carried whole, since the parameter is a
    std::string rather than a C string
*/
bool
tests_archive_make_text_entry_matches_file_entry()
{
    const dj::entry t = dt::make_text_entry("a.txt", "hello, world");
    const dj::entry f =
        dt::make_file_entry("a.txt", dj::byte_buffer("hello, world"));

    D_TA_CHECK(t.name == f.name);
    D_TA_CHECK(t.data == f.data);
    D_TA_CHECK(t.is_directory == f.is_directory);
    D_TA_CHECK(t.mode == f.mode);
    D_TA_CHECK(t.mtime == f.mtime);

    D_TA_CHECK(!t.is_directory);
    D_TA_CHECK(t.mode == 0u);
    D_TA_CHECK(t.mtime == 0L);

    // a std::string carrying a NUL is not truncated
    const std::string with_nul("a\0b", 3);
    const dj::entry   n = dt::make_text_entry("n.bin", with_nul);

    D_TA_CHECK(n.data.size() == 3u);
    D_TA_CHECK(n.data == with_nul);

    // empty text
    D_TA_CHECK(dt::make_text_entry("e", "").data.empty());

    return true;
}

/*
tests_archive_make_dir_entry_carries_no_data
  the directory shorthand.
  Tests the following:
  - the entry is flagged as a directory
  - it carries an empty payload, since a directory has none
  - mode and mtime default to the writer-picks values
  - the name is carried verbatim, trailing separator included if given
*/
bool
tests_archive_make_dir_entry_carries_no_data()
{
    const dj::entry d = dt::make_dir_entry("sub/dir");

    D_TA_CHECK(d.name == "sub/dir");
    D_TA_CHECK(d.is_directory);
    D_TA_CHECK(d.data.empty());
    D_TA_CHECK(d.mode == 0u);
    D_TA_CHECK(d.mtime == 0L);

    // the name is not normalised at construction time
    const dj::entry slashed = dt::make_dir_entry("sub/dir/");

    D_TA_CHECK(slashed.name == "sub/dir/");
    D_TA_CHECK(slashed.is_directory);

    // which is exactly why the lookups normalise instead
    D_TA_CHECK(dt::normalize_name(slashed.name) == d.name);

    return true;
}

/*
tests_archive_entries_carry_binary_payloads
  binary transparency.
  Tests the following:
  - embedded NULs survive, so an entry is never treated as a C string
  - high bytes survive without sign trouble
  - the payload length is preserved exactly, including a payload that is
    entirely NULs
*/
bool
tests_archive_entries_carry_binary_payloads()
{
    const dj::byte_buffer nuls("a\0b\0c", 5);
    const dj::entry       e = dt::make_file_entry("blob.bin", nuls);

    D_TA_CHECK(e.data.size() == 5u);
    D_TA_CHECK(e.data == nuls);
    D_TA_CHECK(e.data[1] == '\0');
    D_TA_CHECK(e.data[3] == '\0');

    // high bytes
    dj::byte_buffer high;

    high.push_back(static_cast<char>(0xFFu));
    high.push_back(static_cast<char>(0x80u));
    high.push_back(static_cast<char>(0x00u));
    high.push_back(static_cast<char>(0x7Fu));

    const dj::entry h = dt::make_file_entry("high.bin", high);

    D_TA_CHECK(h.data.size() == 4u);
    D_TA_CHECK(static_cast<unsigned char>(h.data[0]) == 0xFFu);
    D_TA_CHECK(static_cast<unsigned char>(h.data[1]) == 0x80u);
    D_TA_CHECK(static_cast<unsigned char>(h.data[2]) == 0x00u);

    // a payload that is entirely NULs still has its length
    const dj::byte_buffer allnul(16u, static_cast<char>(0));
    const dj::entry       a = dt::make_file_entry("z.bin", allnul);

    D_TA_CHECK(a.data.size() == 16u);
    D_TA_CHECK(a.data == allnul);

    // and a large payload is carried whole
    const dj::byte_buffer big(100000u, 'x');

    D_TA_CHECK(dt::make_file_entry("big", big).data.size() == 100000u);

    return true;
}

/*
tests_archive_entries_are_independent_values
  value semantics.
  Tests the following:
  - each builder returns a fresh value, so mutating one entry cannot
    disturb another built from the same arguments
  - copying an entry and changing the copy leaves the original intact
  - an entry list holds copies, not references to the caller's objects
*/
bool
tests_archive_entries_are_independent_values()
{
    dj::entry a = dt::make_text_entry("same.txt", "payload");
    dj::entry b = dt::make_text_entry("same.txt", "payload");

    D_TA_CHECK(a.data == b.data);

    // mutating one leaves the other alone
    a.data += "more";
    a.name  = "renamed.txt";

    D_TA_CHECK(b.data == dj::byte_buffer("payload"));
    D_TA_CHECK(b.name == "same.txt");
    D_TA_CHECK(a.data != b.data);

    // a copy is independent of its source
    const dj::entry src  = dt::make_text_entry("src", "original");
    dj::entry       copy = src;

    copy.data = "changed";
    copy.is_directory = true;

    D_TA_CHECK(src.data == dj::byte_buffer("original"));
    D_TA_CHECK(!src.is_directory);

    // a list holds copies
    dj::entry_list items;
    dj::entry      pushed = dt::make_text_entry("pushed", "v1");

    items.push_back(pushed);
    pushed.data = "v2";

    D_TA_CHECK(items.size() == 1u);
    D_TA_CHECK(items[0].data == dj::byte_buffer("v1"));

    return true;
}

NS_END  // testing
NS_END  // djinterp
