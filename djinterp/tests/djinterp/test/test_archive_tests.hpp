/******************************************************************************
* djinterp [tests]                                       test_archive_tests.hpp
*
*   Suite header for test_archive.hpp - the shared verification surface for
* the archive facade.  Declares every test body, supplies the fixtures the
* bodies need, and (in DTEST_SPEC_MODE) exposes the spec provider the runner
* hands to run_module.
*
*   BUILD PREREQUISITE - THE RUNNER NEEDS D_TEST_REPORT_ENABLE_ARCHIVE.
*   This is a property of the framework, not of the module under test, and it
* applies to EVERY DTest runner rather than to this suite alone.  A runner
* includes test_defaults.hpp to reach run_module; test_defaults.hpp includes
* output/test_report_runner.hpp; and that header spells `byte_buffer` in two
* places - pending_doc::bytes and write_bytes_to_file's parameter - which is a
* typedef reachable only through the archive include guarded by
* D_TEST_REPORT_ENABLE_ARCHIVE.  Without the macro the runner does not compile
* at C++17 or higher (eight errors).  The leaf CMakeLists therefore defines it
* on the target.  The SECTION translation units do not need it: they include
* the module under test directly and never reach the report machinery.
*
*   The fix belongs in test_report_runner.hpp - spell those two uses
* std::string, or include compress.hpp unconditionally - not here; per the
* authoring guide a broken dependency is reported rather than patched.
*
*   HOW THE SNIFFERS ARE COVERED.
*   The format sniffers read leading and trailing bytes; they need no backend
* at all.  So the suite BUILDS containers byte by byte - ta_make_zip assembles
* local file headers and an end-of-central-directory record from first
* principles, ta_make_tar_blocks lays down a ustar header and the mandatory
* zero tail - and hands them to the sniffers.  That reaches every branch,
* including the ones a real writer never produces: a truncated local-header
* chain, an EOCD that is absent, a tar whose zero tail is one byte short.
* Where a genuine container IS available the suite cross-checks the synthetic
* result against it, so the hand-built bytes cannot drift from reality.
*
*   FINDING - format_is_writable DISAGREES WITH try_archive FOR zip AND tar.
*   archive_create dispatches straight to the per-format writer without ever
* consulting format_can_write, and the built-in zip (store-only) and tar
* writers need no third-party library.  So on a build with no archive library
* linked, format_is_writable<formats::zip>() reports FALSE while
* try_archive<formats::zip>() returns status_ok and produces a valid,
* extractable container; the same holds for tar.  The two queries answer
* different questions - "is a library linked" versus "did the call work" - and
* only the second is what a round-trip test actually depends on.
*
*   This suite therefore never gates a round-trip on format_is_writable.  It
* asserts the build-agnostic contract instead: a format either round-trips or
* reports a non-ok status, and whichever happens, the facade must be
* self-consistent about it.  The disagreement itself is pinned in
* tests_archive_writability_is_independent so that a later fix
* to archive.cpp - gating archive_create on format_can_write, or widening
* format_can_write to admit the built-in writers - shows up as a failing test
* rather than silently changing what every other body here means.
*
*   PORTABILITY:
*   The module is C++98-clean; this suite is written to the same baseline
* except for the fixtures, which use std::vector.  Built at C++20 by the DTest
* CMake helper.
*
*
* TABLE OF CONTENTS
* =================
* I.    ENDIAN       (test_archive_tests_endian.cpp)
* II.   ENTRIES      (test_archive_tests_entries.cpp)
* III.  INSPECTION   (test_archive_tests_inspection.cpp)
* IV.   PAYLOAD      (test_archive_tests_payload.cpp)
* V.    SNIFFERS     (test_archive_tests_sniffers.cpp)
* VI.   ROUNDTRIP    (test_archive_tests_roundtrip.cpp)
*
*
* path:      /tests/djinterp/test/test_archive_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_ARCHIVE_TESTS_
#define DJINTERP_TESTS_TEST_ARCHIVE_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "test_archive.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"
#endif


NS_DJINTERP
NS_TESTING

// dt
//   type: names the entities under test (djinterp::test).  Unconditional,
// because the spec provider needs dt::module_spec in spec mode.
namespace dt = ::djinterp::test;


// archive_check
//   function: reports a failed assertion and returns the condition, so the
// D_TA_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
archive_check(
    bool        _cond,
    const char* _expr,
    const char* _file,
    int         _line
)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

// D_TA_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_TA_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::archive_check(                              \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// dj
//   type: names the foundation namespace the archive vocabulary lives in.
namespace dj = ::djinterp;


// ta_put_u16
//   function: appends a 16-bit value to _b in little-endian order - the byte
// order every ZIP structural field uses.
inline void
ta_put_u16(
    dj::byte_buffer& _b,
    unsigned int     _v
)
{
    _b.push_back(static_cast<char>(static_cast<unsigned char>(_v & 0xFFu)));
    _b.push_back(static_cast<char>(
                     static_cast<unsigned char>((_v >> 8) & 0xFFu)));

    return;
}

// ta_put_u32
//   function: appends a 32-bit value to _b in little-endian order.
inline void
ta_put_u32(
    dj::byte_buffer& _b,
    unsigned long    _v
)
{
    _b.push_back(static_cast<char>(static_cast<unsigned char>(_v & 0xFFuL)));
    _b.push_back(static_cast<char>(
                     static_cast<unsigned char>((_v >> 8) & 0xFFuL)));
    _b.push_back(static_cast<char>(
                     static_cast<unsigned char>((_v >> 16) & 0xFFuL)));
    _b.push_back(static_cast<char>(
                     static_cast<unsigned char>((_v >> 24) & 0xFFuL)));

    return;
}


// ta_local_header
//   function: a complete ZIP local file header for _name / _data, declaring
// compression method _method.  Thirty fixed bytes, then the name, then the
// (notionally already compressed) data.
inline dj::byte_buffer
ta_local_header(
    const std::string&     _name,
    const dj::byte_buffer& _data,
    unsigned int           _method
)
{
    dj::byte_buffer b;

    ta_put_u32(b, 0x04034b50UL);                 //  0 signature
    ta_put_u16(b, 20u);                          //  4 version needed
    ta_put_u16(b, 0u);                           //  6 flags
    ta_put_u16(b, _method);                      //  8 method
    ta_put_u16(b, 0u);                           // 10 mod time
    ta_put_u16(b, 0u);                           // 12 mod date
    ta_put_u32(b, 0uL);                          // 14 crc32
    ta_put_u32(b,
        static_cast<unsigned long>(_data.size()));  // 18 compressed size
    ta_put_u32(b,
        static_cast<unsigned long>(_data.size()));  // 22 uncompressed size
    ta_put_u16(b,
        static_cast<unsigned int>(_name.size()));   // 26 name length
    ta_put_u16(b, 0u);                           // 28 extra length

    b += _name;
    b += _data;

    return b;
}

// ta_eocd
//   function: a bare 22-byte end-of-central-directory record advertising
// _total members and no archive comment.
inline dj::byte_buffer
ta_eocd(
    unsigned int _total
)
{
    dj::byte_buffer b;

    ta_put_u32(b, 0x06054b50UL);   //  0 signature
    ta_put_u16(b, 0u);             //  4 this disk
    ta_put_u16(b, 0u);             //  6 disk with central directory
    ta_put_u16(b, _total);         //  8 entries on this disk
    ta_put_u16(b, _total);         // 10 total entries (what the module reads)
    ta_put_u32(b, 0uL);            // 12 central directory size
    ta_put_u32(b, 0uL);            // 16 central directory offset
    ta_put_u16(b, 0u);             // 20 comment length

    return b;
}

// ta_zip_member
//   struct: one member of a synthetic ZIP - its name, its stored bytes, and
// the method code its local header should declare.
struct ta_zip_member
{
    std::string     name;
    dj::byte_buffer data;
    unsigned int    method;

    ta_zip_member(
        const std::string&     _name,
        const dj::byte_buffer& _data,
        unsigned int           _method
    )
        : name(_name),
          data(_data),
          method(_method)
    {}
};

// ta_make_zip
//   function: a synthetic ZIP container - one local file header per member,
// followed by an EOCD advertising the member count.  No central directory is
// written: the module's sniffers read the local-header chain from the front
// and the EOCD from the back, and neither consults the directory in between.
inline dj::byte_buffer
ta_make_zip(
    const std::vector<ta_zip_member>& _members
)
{
    dj::byte_buffer b;
    std::size_t     i = 0;

    for (i = 0; i < _members.size(); ++i)
    {
        b += ta_local_header(_members[i].name,
                             _members[i].data,
                             _members[i].method);
    }

    b += ta_eocd(static_cast<unsigned int>(_members.size()));

    return b;
}


// ta_tar_header_block
//   function: a 512-byte tar header block carrying _name and the ustar magic
// at offset 257 - the marker tar_has_ustar_magic looks for.
inline dj::byte_buffer
ta_tar_header_block(
    const std::string& _name
)
{
    dj::byte_buffer b(512u, static_cast<char>(0));
    std::size_t     i = 0;

    for (i = 0; (i < _name.size()) && (i < 100u); ++i)
    {
        b[i] = _name[i];
    }

    // "ustar" at 257, then the version field
    b[257] = 'u';
    b[258] = 's';
    b[259] = 't';
    b[260] = 'a';
    b[261] = 'r';
    b[262] = static_cast<char>(0);

    return b;
}

// ta_make_tar_blocks
//   function: a synthetic tar stream - _n header blocks followed by the
// mandatory two 512-byte zero blocks.
inline dj::byte_buffer
ta_make_tar_blocks(
    std::size_t _n
)
{
    dj::byte_buffer b;
    std::size_t     i = 0;

    for (i = 0; i < _n; ++i)
    {
        b += ta_tar_header_block("member");
    }

    // the mandatory 1024-byte zero tail
    b.append(1024u, static_cast<char>(0));

    return b;
}


// ta_make_gzip_header
//   function: the three bytes looks_like_gzip inspects - the two magic bytes
// plus the DEFLATE method code - followed by _tail filler.
inline dj::byte_buffer
ta_make_gzip_header(
    std::size_t _tail = 16u
)
{
    dj::byte_buffer b;

    b.push_back(static_cast<char>(0x1Fu));
    b.push_back(static_cast<char>(0x8Bu));
    b.push_back(static_cast<char>(0x08u));
    b.append(_tail, static_cast<char>(0xA5u));

    return b;
}


// ta_sample_entries
//   function: a small mixed entry list - two regular files (one carrying
// embedded NULs) and one directory - used wherever a test needs a realistic
// list without building one inline.
inline dj::entry_list
ta_sample_entries()
{
    dj::entry_list items;

    items.push_back(dt::make_text_entry("readme.txt", "hello, world"));
    items.push_back(dt::make_file_entry("data/blob.bin",
                                        dj::byte_buffer("a\0b\0c", 5)));
    items.push_back(dt::make_dir_entry("data"));

    return items;
}


// ta_status_name
//   function: a short label for a status code, so a failing sweep can say
// which one it saw.
inline const char*
ta_status_name(
    dj::status _s
)
{
    switch (_s)
    {
        case dj::status_ok:               return "ok";
        case dj::status_unavailable:      return "unavailable";
        case dj::status_invalid_argument: return "invalid_argument";
        case dj::status_buffer_error:     return "buffer_error";
        case dj::status_backend_error:    return "backend_error";
        case dj::status_unsupported:      return "unsupported";
        default:                          return "unknown";
    }
}

// ta_format_contract_holds
//   function: the build-agnostic archive contract, spelled out so a failing
// sweep can say WHICH half broke.  A format either completes the round trip
// and preserves its files, or reports a non-ok status and returns nothing
// useful.  Never gated on format_is_writable - see the FINDING above.
template<typename _Format>
inline bool
ta_format_contract_holds(
    const dj::entry_list& _in,
    const char*           _name
)
{
    dj::byte_buffer blob;
    const dj::status cs = dj::try_archive<_Format>(_in, blob,
                                                   dj::archive_options());

    if (cs != dj::status_ok)
    {
        // a refusing writer must emit nothing
        if (!blob.empty())
        {
            std::printf("    [FAIL] %s returned %s but wrote %u bytes\n",
                        _name, ta_status_name(cs),
                        static_cast<unsigned int>(blob.size()));

            return false;
        }

        // and the round-trip driver must surface the same refusal
        dj::entry_list out;

        if (dt::roundtrip<_Format>(_in, out) != cs)
        {
            std::printf("    [FAIL] %s: driver hid the %s status\n",
                        _name, ta_status_name(cs));

            return false;
        }

        return !dt::roundtrip_preserves_files<_Format>(_in);
    }

    // a succeeding writer must produce something extractable
    dj::entry_list out;

    if (dt::roundtrip<_Format>(_in, out) != dj::status_ok)
    {
        std::printf("    [FAIL] %s created but did not round-trip\n", _name);

        return false;
    }

    return true;
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   ENDIAN   (test_archive_tests_endian.cpp)
bool tests_archive_read_u16_le_byte_order();
bool tests_archive_read_u16_le_range_and_offset();
bool tests_archive_read_u32_le_byte_order();
bool tests_archive_read_u32_le_range_and_offset();

// II.  ENTRIES   (test_archive_tests_entries.cpp)
bool tests_archive_make_entry_records_every_field();
bool tests_archive_make_file_entry_defaults();
bool tests_archive_make_text_entry_matches_file_entry();
bool tests_archive_make_dir_entry_carries_no_data();
bool tests_archive_entries_carry_binary_payloads();
bool tests_archive_entries_are_independent_values();

// III. INSPECTION   (test_archive_tests_inspection.cpp)
bool tests_archive_normalize_name_drops_slash();
bool tests_archive_normalize_name_edge_cases();
bool tests_archive_find_entry_locates_by_name();
bool tests_archive_find_entry_is_slash_tolerant();
bool tests_archive_find_entry_returns_null_when_absent();
bool tests_archive_find_entry_returns_first_match();
bool tests_archive_has_entry_matches_find_entry();
bool tests_archive_count_files_and_dirs();

// IV.  PAYLOAD   (test_archive_tests_payload.cpp)
bool tests_archive_file_data_fetches_contents();
bool tests_archive_file_data_rejects_directory_and_absent();
bool tests_archive_files_preserved_accepts_faithful_copy();
bool tests_archive_files_preserved_rejects_damage();
bool tests_archive_files_preserved_ignores_directories();

// V.   SNIFFERS   (test_archive_tests_sniffers.cpp)
bool tests_archive_zip_find_eocd_locates_record();
bool tests_archive_zip_find_eocd_rejects_absent_and_short();
bool tests_archive_looks_like_zip_front_signatures();
bool tests_archive_looks_like_zip_rejects_malformed();
bool tests_archive_zip_total_entries_reads_count();
bool tests_archive_zip_local_methods_walks_chain();
bool tests_archive_zip_local_methods_rejects_truncated();
bool tests_archive_looks_like_gzip_checks_three_bytes();
bool tests_archive_tar_has_ustar_magic_at_offset();
bool tests_archive_tar_is_terminated_checks_zero_tail();
bool tests_archive_sniffers_agree_with_real_containers();

// VI.  ROUNDTRIP   (test_archive_tests_roundtrip.cpp)
bool tests_archive_roundtrip_reports_facade_status();
bool tests_archive_roundtrip_preserves_files();
bool tests_archive_roundtrip_contract_all_formats();
bool tests_archive_roundtrip_accepts_options();
bool tests_archive_roundtrip_handles_degenerate_lists();
bool tests_archive_writability_is_independent();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// archive_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
archive_spec()
{
    return dt::module_spec{
        "test_archive",
        "The shared verification surface for the archive facade: entry "
        "construction, slash-tolerant list inspection, payload comparison, "
        "lenient container sniffers driven by hand-built bytes, and "
        "round-trip drivers written to hold on any build.",
        {
            dt::block_spec{
                "endian",
                "The little-endian readers every ZIP structural field goes "
                "through - byte order, offset handling, and the full value "
                "range including the high bit.",
                {
                    { "tests_archive_read_u16_le_byte_order",
                      "the low byte comes first, so a two-byte pair decodes "
                      "little-endian rather than big-endian",
                      &tests_archive_read_u16_le_byte_order },
                    { "tests_archive_read_u16_le_range_and_offset",
                      "the full 16-bit range decodes without sign extension, "
                      "and the offset selects the pair read",
                      &tests_archive_read_u16_le_range_and_offset },
                    { "tests_archive_read_u32_le_byte_order",
                      "all four bytes contribute in little-endian order",
                      &tests_archive_read_u32_le_byte_order },
                    { "tests_archive_read_u32_le_range_and_offset",
                      "the full 32-bit range decodes, including values with "
                      "the top bit set, and the offset selects the quad read",
                      &tests_archive_read_u32_le_range_and_offset },
                }
            },
            dt::block_spec{
                "entries",
                "The entry builders: what each records, what each defaults, "
                "and the binary transparency an archive member needs.",
                {
                    { "tests_archive_make_entry_records_every_field",
                      "all five fields reach the constructed entry unchanged",
                      &tests_archive_make_entry_records_every_field },
                    { "tests_archive_make_file_entry_defaults",
                      "a regular-file entry defaults to non-directory with a "
                      "writer-chosen mode and mtime",
                      &tests_archive_make_file_entry_defaults },
                    { "tests_archive_make_text_entry_matches_file_entry",
                      "the text alias produces an entry indistinguishable "
                      "from the byte-buffer form",
                      &tests_archive_make_text_entry_matches_file_entry },
                    { "tests_archive_make_dir_entry_carries_no_data",
                      "a directory entry is flagged as one and carries an "
                      "empty payload",
                      &tests_archive_make_dir_entry_carries_no_data },
                    { "tests_archive_entries_carry_binary_payloads",
                      "embedded NULs and high bytes survive, so an entry is "
                      "never treated as a C string",
                      &tests_archive_entries_carry_binary_payloads },
                    { "tests_archive_entries_are_independent_values",
                      "each builder returns a fresh value, so mutating one "
                      "entry cannot disturb another",
                      &tests_archive_entries_are_independent_values },
                }
            },
            dt::block_spec{
                "inspection",
                "Name normalisation and the slash-tolerant lookups built on "
                "it, plus the member tallies.",
                {
                    { "tests_archive_normalize_name_drops_slash",
                      "exactly one trailing separator is removed, so a name "
                      "ending in two keeps one",
                      &tests_archive_normalize_name_drops_slash },
                    { "tests_archive_normalize_name_edge_cases",
                      "the empty name, a lone separator and interior "
                      "separators are all handled without loss",
                      &tests_archive_normalize_name_edge_cases },
                    { "tests_archive_find_entry_locates_by_name",
                      "a present member is found and the returned pointer "
                      "addresses the list's own element",
                      &tests_archive_find_entry_locates_by_name },
                    { "tests_archive_find_entry_is_slash_tolerant",
                      "a directory matches whether or not either side "
                      "carries the trailing separator",
                      &tests_archive_find_entry_is_slash_tolerant },
                    { "tests_archive_find_entry_returns_null_when_absent",
                      "a missing name yields a null pointer, including on an "
                      "empty list",
                      &tests_archive_find_entry_returns_null_when_absent },
                    { "tests_archive_find_entry_returns_first_match",
                      "with duplicate names the first is returned, which is "
                      "what makes the helper deterministic",
                      &tests_archive_find_entry_returns_first_match },
                    { "tests_archive_has_entry_matches_find_entry",
                      "the boolean form agrees with the pointer form on "
                      "every name tried",
                      &tests_archive_has_entry_matches_find_entry },
                    { "tests_archive_count_files_and_dirs",
                      "the two tallies partition the list, so together they "
                      "always account for every member",
                      &tests_archive_count_files_and_dirs },
                }
            },
            dt::block_spec{
                "payload",
                "Fetching one member's bytes and the whole-list payload "
                "comparison a round-trip check rests on.",
                {
                    { "tests_archive_file_data_fetches_contents",
                      "a regular file's bytes are copied out, including an "
                      "empty payload",
                      &tests_archive_file_data_fetches_contents },
                    { "tests_archive_file_data_rejects_directory_and_absent",
                      "a directory or a missing name fails and leaves the "
                      "caller's buffer untouched",
                      &tests_archive_file_data_rejects_directory_and_absent },
                    { "tests_archive_files_preserved_accepts_faithful_copy",
                      "a faithful copy passes regardless of order, and extra "
                      "members in the restored list are tolerated",
                      &tests_archive_files_preserved_accepts_faithful_copy },
                    { "tests_archive_files_preserved_rejects_damage",
                      "a missing file, altered bytes, or a file demoted to a "
                      "directory all fail the comparison",
                      &tests_archive_files_preserved_rejects_damage },
                    { "tests_archive_files_preserved_ignores_directories",
                      "directory members carry no payload, so their "
                      "representation is not compared",
                      &tests_archive_files_preserved_ignores_directories },
                }
            },
            dt::block_spec{
                "sniffers",
                "The structural probes, driven by containers this suite "
                "assembles byte by byte so every branch is reachable - then "
                "cross-checked against whatever the build can really write.",
                {
                    { "tests_archive_zip_find_eocd_locates_record",
                      "the end-of-central-directory signature is found, and "
                      "the scan runs backward so the last match wins",
                      &tests_archive_zip_find_eocd_locates_record },
                    { "tests_archive_zip_find_eocd_rejects_absent_and_short",
                      "a blob with no record, or one shorter than the "
                      "smallest possible record, reports not-found",
                      &tests_archive_zip_find_eocd_rejects_absent_and_short },
                    { "tests_archive_looks_like_zip_front_signatures",
                      "a local file header and a bare EOCD both open a valid "
                      "container, the latter being the empty archive",
                      &tests_archive_looks_like_zip_front_signatures },
                    { "tests_archive_looks_like_zip_rejects_malformed",
                      "a wrong front signature, or a right one with no EOCD "
                      "behind it, is rejected",
                      &tests_archive_looks_like_zip_rejects_malformed },
                    { "tests_archive_zip_total_entries_reads_count",
                      "the member count is read from the right field, and a "
                      "blob with no record reports minus one",
                      &tests_archive_zip_total_entries_reads_count },
                    { "tests_archive_zip_local_methods_walks_chain",
                      "one method code is appended per local header, in "
                      "order, and the walk stops where the headers do",
                      &tests_archive_zip_local_methods_walks_chain },
                    { "tests_archive_zip_local_methods_rejects_truncated",
                      "a header or payload running past the end fails rather "
                      "than reading out of bounds",
                      &tests_archive_zip_local_methods_rejects_truncated },
                    { "tests_archive_looks_like_gzip_checks_three_bytes",
                      "both magic bytes and the DEFLATE method byte are "
                      "required, so a bare two-byte magic is rejected",
                      &tests_archive_looks_like_gzip_checks_three_bytes },
                    { "tests_archive_tar_has_ustar_magic_at_offset",
                      "the magic is read at offset 257 specifically, and a "
                      "blob too short to reach it is rejected",
                      &tests_archive_tar_has_ustar_magic_at_offset },
                    { "tests_archive_tar_is_terminated_checks_zero_tail",
                      "the final 1024 bytes must all be zero; one non-zero "
                      "byte anywhere in the tail fails",
                      &tests_archive_tar_is_terminated_checks_zero_tail },
                    { "tests_archive_sniffers_agree_with_real_containers",
                      "on whatever formats this build can write, the "
                      "sniffers accept genuine output and the member count "
                      "matches what was archived",
                      &tests_archive_sniffers_agree_with_real_containers },
                }
            },
            dt::block_spec{
                "roundtrip",
                "The archive/extract drivers, written to hold whatever this "
                "build supports - including the writability inconsistency "
                "pinned in the suite header.",
                {
                    { "tests_archive_roundtrip_reports_facade_status",
                      "the driver surfaces the facade's own status rather "
                      "than collapsing a refusal into a generic failure",
                      &tests_archive_roundtrip_reports_facade_status },
                    { "tests_archive_roundtrip_preserves_files",
                      "where a format round-trips, every regular file comes "
                      "back byte-identical, NULs and all",
                      &tests_archive_roundtrip_preserves_files },
                    { "tests_archive_roundtrip_contract_all_formats",
                      "each of the six formats either round-trips or refuses "
                      "cleanly, with no partial success",
                      &tests_archive_roundtrip_contract_all_formats },
                    { "tests_archive_roundtrip_accepts_options",
                      "an explicit option set reaches the writer and does not "
                      "change the correctness of the cycle",
                      &tests_archive_roundtrip_accepts_options },
                    { "tests_archive_roundtrip_handles_degenerate_lists",
                      "an empty list and a directory-only list are handled "
                      "without a spurious payload failure",
                      &tests_archive_roundtrip_handles_degenerate_lists },
                    { "tests_archive_writability_is_independent",
                      "pins the finding that format_is_writable and "
                      "try_archive answer different questions, so a later "
                      "reconciliation surfaces here",
                      &tests_archive_writability_is_independent },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_TEST_ARCHIVE_TESTS_
