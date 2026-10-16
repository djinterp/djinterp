/*******************************************************************************
* djinterp [test]                                      archive_options_tests.hpp
*
*   Declarations for the archive_options.hpp unit-test suite, plus the small
* shared helpers and the per-suite check macros the section translation units
* use.  Each tests_* function is defined in a section .cpp beside this header
* (one section per part of the header's own table of contents, plus behaviour),
* flat in djinterp::testing, and returns true on success.  The runner (config
* tree) links the sections and calls each function.
*
*   WHAT A PLAIN-DATA HEADER OFFERS A TEST:
*   archive_options carries no logic -- its "code" is the set of constructor
* defaults, the enum values, and the value semantics the compiler generates.
* Those ARE the contract call sites rely on, so the suite pins every one of
* them: the neutral-zero rule for each categorical enum, every field's
* documented default, deep copy/assign independence (the blocks hold
* std::string), and the stub-compatibility guarantee (`level` = -1,
* `store_only` = false).
*
*   BEHAVIOUR IS TESTED SEPARATELY:
*   The last section drives the facade to record which knobs the built-in
* writers actually consult today.  archive.cpp reads exactly two fields --
* store_only and level -- so the remaining knobs are, for now, inert.  Those
* tests assert the safe invariant (requesting an unhonoured knob still yields a
* valid, readable archive and never mutates the caller's option set) rather
* than freezing today's gaps in place.
*
*   CHECK MACROS:
*   Unique-lettered per suite (D_AO_) so co-compiled suites never collide.  Each
* test declares  bool _ok = true;  runs D_AO_CHECK (which reports a failing
* check with its source location and clears _ok, but keeps going so every check
* in the body is exercised) and D_AO_REQUIRE (which bails out immediately when
* continuing would be unsafe), and ends with  return _ok;.  D_AO_CHECK_EQUAL_OPT
* compares two option sets and prints the differing field names on a mismatch.
*
*
* path:      /tests/djinterp/core/util/archive/archive_options_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_ARCHIVE_OPTIONS_TESTS_
#define DJINTERP_ARCHIVE_OPTIONS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
// djinterp
#include "../../../../../inc/djinterp/core/util/archive/archive.hpp"                              // facade (pulls the options)
#include "djinterp/test/archive/test_archive_options.hpp"   // shared option comparators
#include "djinterp/test/archive/test_archive.hpp"           // shared archive verifiers


NS_DJINTERP
NS_TESTING

// =============================================================================
// SHARED HELPERS
// =============================================================================

// sample_entries
//   function: a small, fixed entry list used by the behavioural section -- one
// compressible text file and one directory, enough to exercise a writer without
// making the assertions about it depend on payload specifics.
//
// Parameter(s):
//   none.
// Return:
//   the constructed entry list.
inline entry_list
sample_entries()
{
    entry_list items;

    items.push_back(::djinterp::test::make_file_entry("a.txt",
                                                      byte_blob(600, 'a')));
    items.push_back(::djinterp::test::make_dir_entry("d"));

    return items;
}


// =============================================================================
// TEST DECLARATIONS  (grouped by header section; defined in the section .cpp)
// =============================================================================

// I. categorical enums  (archive_options_tests_enums.cpp)
bool tests_enum_neutral_defaults();
bool tests_enum_distinct_values();
bool tests_enum_sequential_values();
bool tests_enum_plain_not_scoped();

// II.i zip_options  (archive_options_tests_zip.cpp)
bool tests_zip_options_defaults();
bool tests_zip_options_fields();

// II.ii-v tar / gz / 7z / rar options  (archive_options_tests_formats.cpp)
bool tests_tar_options_defaults();
bool tests_gz_options_defaults();
bool tests_sevenzip_options_defaults();
bool tests_rar_options_defaults();

// III. the archive_options aggregate  (archive_options_tests_aggregate.cpp)
bool tests_archive_options_defaults();
bool tests_archive_options_stub_superset();
bool tests_archive_options_nested_blocks_default();

// the embedded codec (HAS-A compress_options)  (archive_options_tests_codec.cpp)
bool tests_codec_block_defaults();
bool tests_codec_block_independence();

// value semantics + the shared comparators  (archive_options_tests_semantics.cpp)
bool tests_options_copy_construction();
bool tests_options_assignment();
bool tests_options_deep_string_independence();
bool tests_comparator_detects_archive_fields();
bool tests_comparator_detects_format_fields();
bool tests_comparator_detects_codec_fields();
bool tests_option_diff_description();
bool tests_option_builders();

// what the facade actually honours  (archive_options_tests_facade.cpp)
bool tests_facade_honours_store_only();
bool tests_facade_honours_level();
bool tests_facade_codec_block_is_inert();
bool tests_facade_container_knobs_do_not_corrupt();
bool tests_facade_does_not_mutate_options();


NS_END  // testing
NS_END  // djinterp


// =============================================================================
// PER-SUITE CHECK MACROS  (letters: AO)
// =============================================================================

// D_AO_CHECK
//   macro: record a boolean expectation against the enclosing test's `_ok`
// flag.  On failure it prints the stringized expression with its source
// location and clears `_ok`, but execution continues so every check runs.
#define D_AO_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [expect] FAIL: %s  (%s:%d)\n",                  \
                        #_cond, __FILE__, __LINE__);                           \
            _ok = false;                                                       \
        }                                                                      \
    }                                                                          \
    while (false)

// D_AO_REQUIRE
//   macro: a hard precondition.  On failure it prints and returns false at
// once -- for cases where continuing would be unsafe (an operation that must
// succeed before its result is inspected).
#define D_AO_REQUIRE(_cond)                                                    \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [require] FAIL: %s  (%s:%d)\n",                 \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (false)

// D_AO_CHECK_EQUAL_OPT
//   macro: compare two option sets deeply and, on a mismatch, print the names
// of the fields that differ (via the shared diff helper) rather than a bare
// false.  Keeps "these options should be identical" assertions diagnosable.
#define D_AO_CHECK_EQUAL_OPT(_a, _b)                                           \
    do                                                                         \
    {                                                                          \
        if (!::djinterp::test::archive_options_equal((_a), (_b)))              \
        {                                                                      \
            std::printf("      [options] FAIL: %s != %s  ->  differing: %s"    \
                        "  (%s:%d)\n",                                         \
                        #_a, #_b,                                              \
                        ::djinterp::test::describe_option_diff((_a),           \
                                                               (_b)).c_str(),  \
                        __FILE__, __LINE__);                                   \
            _ok = false;                                                       \
        }                                                                      \
    }                                                                          \
    while (false)

// D_AO_CHECK_STATUS
//   macro: evaluate a status expression once and compare it to an expected
// status, printing both human-readable names on a mismatch and clearing `_ok`.
#define D_AO_CHECK_STATUS(_expr, _expected)                                    \
    do                                                                         \
    {                                                                          \
        ::djinterp::status _ao_got = (_expr);                                  \
        ::djinterp::status _ao_exp = (_expected);                              \
        if (_ao_got != _ao_exp)                                                \
        {                                                                      \
            std::printf("      [status] FAIL: %s  ->  got '%s', want '%s'"     \
                        "  (%s:%d)\n",                                         \
                        #_expr,                                                \
                        ::djinterp::status_message(_ao_got),                   \
                        ::djinterp::status_message(_ao_exp),                   \
                        __FILE__, __LINE__);                                   \
            _ok = false;                                                       \
        }                                                                      \
    }                                                                          \
    while (false)


#endif  // DJINTERP_ARCHIVE_OPTIONS_TESTS_
