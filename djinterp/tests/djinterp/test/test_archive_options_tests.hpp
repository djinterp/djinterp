/******************************************************************************
* djinterp [tests]                               test_archive_options_tests.hpp
*
*   Suite header for test_archive_options.hpp - the comparison, pristine and
* diff surface for the archive OPTION vocabulary.  Declares every test body,
* supplies the fixtures the bodies need, and (in DTEST_SPEC_MODE) exposes the
* spec provider the runner hands to run_module.
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
*   HOW THIS SUITE REACHES 100% FIELD COVERAGE.
*   archive_options carries twenty-five fields of its own - five at the
* archive level and twenty spread over the zip, tar, gz, 7z and rar blocks -
* plus an embedded compress_options carrying a further fifty.  As in the
* companion compress-options suite, the coverage is driven by ONE table:
* ao_all_mutators() pairs the exact label describe_option_diff must emit with
* a mutator that moves that field and nothing else.  Mutations are taken
* relative to the current value, so no row depends on knowing a default.
*
*   THE TWO GRANULARITIES.
*   The module deliberately reports the embedded codec COARSELY: codec.level
* is named, but the six per-codec blocks collapse to "codec.zstd" and friends
* rather than naming which of nineteen zstd knobs moved.  The suite tests both
* halves of that contract - ao_all_mutators() carries the twenty-five own
* fields plus the seven codec labels, and a separate body drives a
* representative sweep of codec sub-fields to prove each one collapses onto
* its block label rather than escaping as a field name.
*
*   PORTABILITY:
*   The module is C++98-clean; this suite is written to the same baseline
* except for the fixture table, which uses std::vector.  Built at C++20 by the
* DTest CMake helper.
*
*
* TABLE OF CONTENTS
* =================
* I.    BLOCKS       (test_archive_options_tests_blocks.cpp)
* II.   AGGREGATE    (test_archive_options_tests_aggregate.cpp)
* III.  PRISTINE     (test_archive_options_tests_pristine.cpp)
* IV.   DIFF         (test_archive_options_tests_diff.cpp)
* V.    BUILDERS     (test_archive_options_tests_builders.cpp)
*
*
* path:      /tests/djinterp/test/test_archive_options_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_ARCHIVE_OPTIONS_TESTS_
#define DJINTERP_TESTS_TEST_ARCHIVE_OPTIONS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "test_archive_options.hpp"
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


// archive_options_check
//   function: reports a failed assertion and returns the condition, so the
// D_AO_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
archive_options_check(
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

// D_AO_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_AO_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::archive_options_check(                      \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// dj
//   type: names the foundation namespace the option vocabulary lives in.
namespace dj = ::djinterp;


// ao_bump
//   function: the next value of a numeric or enumerated field.  Relative to
// the CURRENT value, so no mutator has to name a default.
template<typename _T>
inline _T
ao_bump(
    _T _v
)
{
    return static_cast<_T>(static_cast<int>(_v) + 1);
}


// ao_mutator_fn
//   type: moves exactly one field of an option set.
typedef void (*ao_mutator_fn)(dj::archive_options&);

// ao_field
//   struct: one row of the field table - the diff label the module must emit,
// the block the field belongs to, and the mutator that moves it.
struct ao_field
{
    const char*   label;
    const char*   block;
    ao_mutator_fn apply;

    ao_field(
        const char*   _label,
        const char*   _block,
        ao_mutator_fn _apply
    )
        : label(_label),
          block(_block),
          apply(_apply)
    {}
};


// -- archive-level (5) -----------------------------------------------------
inline void ao_m_level(dj::archive_options& o) { o.level = ao_bump(o.level); }
inline void ao_m_store_only(dj::archive_options& o)
    { o.store_only = !o.store_only; }
inline void ao_m_comment(dj::archive_options& o) { o.comment += "x"; }
inline void ao_m_preserve_permissions(dj::archive_options& o)
    { o.preserve_permissions = !o.preserve_permissions; }
inline void ao_m_preserve_mtime(dj::archive_options& o)
    { o.preserve_mtime = !o.preserve_mtime; }

// -- embedded codec, reported coarsely (7) ---------------------------------
//   codec.level is named; each block collapses to its own label, so one
// representative field per block is enough to drive the table.
inline void ao_m_codec_level(dj::archive_options& o)
    { o.codec.level = ao_bump(o.codec.level); }
inline void ao_m_codec_deflate(dj::archive_options& o)
    { o.codec.deflate.window_bits = ao_bump(o.codec.deflate.window_bits); }
inline void ao_m_codec_bzip2(dj::archive_options& o)
    { o.codec.bzip2.work_factor = ao_bump(o.codec.bzip2.work_factor); }
inline void ao_m_codec_lzma(dj::archive_options& o)
    { o.codec.lzma.nice_len = ao_bump(o.codec.lzma.nice_len); }
inline void ao_m_codec_zstd(dj::archive_options& o)
    { o.codec.zstd.window_log = ao_bump(o.codec.zstd.window_log); }
inline void ao_m_codec_lz4(dj::archive_options& o)
    { o.codec.lz4.block_checksum = !o.codec.lz4.block_checksum; }
inline void ao_m_codec_brotli(dj::archive_options& o)
    { o.codec.brotli.quality = ao_bump(o.codec.brotli.quality); }

// -- zip (5) ---------------------------------------------------------------
inline void ao_m_zip_method(dj::archive_options& o)
    { o.zip.method = ao_bump(o.zip.method); }
inline void ao_m_zip_encryption(dj::archive_options& o)
    { o.zip.encryption = ao_bump(o.zip.encryption); }
inline void ao_m_zip_password(dj::archive_options& o)
    { o.zip.password += "p"; }
inline void ao_m_zip_zip64(dj::archive_options& o)
    { o.zip.zip64 = ao_bump(o.zip.zip64); }
inline void ao_m_zip_utf8_names(dj::archive_options& o)
    { o.zip.utf8_names = !o.zip.utf8_names; }

// -- tar (2) ---------------------------------------------------------------
inline void ao_m_tar_format(dj::archive_options& o)
    { o.tar.format = ao_bump(o.tar.format); }
inline void ao_m_tar_numeric_owner(dj::archive_options& o)
    { o.tar.numeric_owner = !o.tar.numeric_owner; }

// -- gz (3) ----------------------------------------------------------------
inline void ao_m_gz_store_name(dj::archive_options& o)
    { o.gz.store_name = !o.gz.store_name; }
inline void ao_m_gz_store_mtime(dj::archive_options& o)
    { o.gz.store_mtime = !o.gz.store_mtime; }
inline void ao_m_gz_original_name(dj::archive_options& o)
    { o.gz.original_name += "n"; }

// -- sevenzip (6) ----------------------------------------------------------
inline void ao_m_7z_method(dj::archive_options& o)
    { o.sevenzip.method = ao_bump(o.sevenzip.method); }
inline void ao_m_7z_solid(dj::archive_options& o)
    { o.sevenzip.solid = !o.sevenzip.solid; }
inline void ao_m_7z_header_compression(dj::archive_options& o)
    { o.sevenzip.header_compression = !o.sevenzip.header_compression; }
inline void ao_m_7z_header_encryption(dj::archive_options& o)
    { o.sevenzip.header_encryption = !o.sevenzip.header_encryption; }
inline void ao_m_7z_password(dj::archive_options& o)
    { o.sevenzip.password += "s"; }
inline void ao_m_7z_threads(dj::archive_options& o)
    { o.sevenzip.threads = ao_bump(o.sevenzip.threads); }

// -- rar (4) ---------------------------------------------------------------
inline void ao_m_rar_level(dj::archive_options& o)
    { o.rar.level = ao_bump(o.rar.level); }
inline void ao_m_rar_solid(dj::archive_options& o)
    { o.rar.solid = !o.rar.solid; }
inline void ao_m_rar_recovery_record(dj::archive_options& o)
    { o.rar.recovery_record = !o.rar.recovery_record; }
inline void ao_m_rar_password(dj::archive_options& o)
    { o.rar.password += "r"; }


// ao_all_mutators
//   function: the complete table - one row per archive_options field plus the
// seven coarse codec labels, in the order describe_option_diff walks them.
inline std::vector<ao_field>
ao_all_mutators()
{
    std::vector<ao_field> t;

    t.push_back(ao_field("level", "archive", &ao_m_level));
    t.push_back(ao_field("store_only", "archive", &ao_m_store_only));
    t.push_back(ao_field("comment", "archive", &ao_m_comment));
    t.push_back(ao_field("preserve_permissions", "archive",
                         &ao_m_preserve_permissions));
    t.push_back(ao_field("preserve_mtime", "archive", &ao_m_preserve_mtime));

    t.push_back(ao_field("codec.level", "codec", &ao_m_codec_level));
    t.push_back(ao_field("codec.deflate", "codec", &ao_m_codec_deflate));
    t.push_back(ao_field("codec.bzip2", "codec", &ao_m_codec_bzip2));
    t.push_back(ao_field("codec.lzma", "codec", &ao_m_codec_lzma));
    t.push_back(ao_field("codec.zstd", "codec", &ao_m_codec_zstd));
    t.push_back(ao_field("codec.lz4", "codec", &ao_m_codec_lz4));
    t.push_back(ao_field("codec.brotli", "codec", &ao_m_codec_brotli));

    t.push_back(ao_field("zip.method", "zip", &ao_m_zip_method));
    t.push_back(ao_field("zip.encryption", "zip", &ao_m_zip_encryption));
    t.push_back(ao_field("zip.password", "zip", &ao_m_zip_password));
    t.push_back(ao_field("zip.zip64", "zip", &ao_m_zip_zip64));
    t.push_back(ao_field("zip.utf8_names", "zip", &ao_m_zip_utf8_names));

    t.push_back(ao_field("tar.format", "tar", &ao_m_tar_format));
    t.push_back(ao_field("tar.numeric_owner", "tar",
                         &ao_m_tar_numeric_owner));

    t.push_back(ao_field("gz.store_name", "gz", &ao_m_gz_store_name));
    t.push_back(ao_field("gz.store_mtime", "gz", &ao_m_gz_store_mtime));
    t.push_back(ao_field("gz.original_name", "gz", &ao_m_gz_original_name));

    t.push_back(ao_field("sevenzip.method", "sevenzip", &ao_m_7z_method));
    t.push_back(ao_field("sevenzip.solid", "sevenzip", &ao_m_7z_solid));
    t.push_back(ao_field("sevenzip.header_compression", "sevenzip",
                         &ao_m_7z_header_compression));
    t.push_back(ao_field("sevenzip.header_encryption", "sevenzip",
                         &ao_m_7z_header_encryption));
    t.push_back(ao_field("sevenzip.password", "sevenzip", &ao_m_7z_password));
    t.push_back(ao_field("sevenzip.threads", "sevenzip", &ao_m_7z_threads));

    t.push_back(ao_field("rar.level", "rar", &ao_m_rar_level));
    t.push_back(ao_field("rar.solid", "rar", &ao_m_rar_solid));
    t.push_back(ao_field("rar.recovery_record", "rar",
                         &ao_m_rar_recovery_record));
    t.push_back(ao_field("rar.password", "rar", &ao_m_rar_password));

    return t;
}

// ao_mutated
//   function: a pristine option set with row _i of the table applied.
inline dj::archive_options
ao_mutated(
    std::size_t _i
)
{
    const std::vector<ao_field> t = ao_all_mutators();
    dj::archive_options         o;

    if (_i < t.size())
    {
        t[_i].apply(o);
    }

    return o;
}

// ao_block_equal
//   function: dispatches to the module's comparator for the named per-format
// block, so a table-driven test can ask "does THIS block's comparator see the
// change?" without a switch at every call site.  The archive-level and codec
// rows own no per-format block and always report equal.
inline bool
ao_block_equal(
    const char*                _block,
    const dj::archive_options& _a,
    const dj::archive_options& _b
)
{
    const std::string b(_block);

    if (b == "zip")      { return dt::zip_options_equal(_a.zip, _b.zip); }
    if (b == "tar")      { return dt::tar_options_equal(_a.tar, _b.tar); }
    if (b == "gz")       { return dt::gz_options_equal(_a.gz, _b.gz); }
    if (b == "sevenzip") { return dt::sevenzip_options_equal(_a.sevenzip,
                                                             _b.sevenzip); }
    if (b == "rar")      { return dt::rar_options_equal(_a.rar, _b.rar); }

    return true;
}


// ao_codec_mutator_fn
//   type: moves one field INSIDE the embedded codec, for the sweep proving
// each such field collapses onto its block label.
typedef void (*ao_codec_mutator_fn)(dj::compress_options&);

// ao_codec_field
//   struct: one codec sub-field and the coarse label it must produce.
struct ao_codec_field
{
    const char*         coarse_label;
    ao_codec_mutator_fn apply;

    ao_codec_field(
        const char*         _coarse_label,
        ao_codec_mutator_fn _apply
    )
        : coarse_label(_coarse_label),
          apply(_apply)
    {}
};

// -- a representative spread inside each codec block -----------------------
inline void ao_c_deflate_a(dj::compress_options& c)
    { c.deflate.window_bits = ao_bump(c.deflate.window_bits); }
inline void ao_c_deflate_b(dj::compress_options& c)
    { c.deflate.mem_level = ao_bump(c.deflate.mem_level); }
inline void ao_c_deflate_c(dj::compress_options& c)
    { c.deflate.strategy = ao_bump(c.deflate.strategy); }
inline void ao_c_bzip2_a(dj::compress_options& c)
    { c.bzip2.block_size_100k = ao_bump(c.bzip2.block_size_100k); }
inline void ao_c_bzip2_b(dj::compress_options& c)
    { c.bzip2.small_decompress = !c.bzip2.small_decompress; }
inline void ao_c_lzma_a(dj::compress_options& c)
    { c.lzma.extreme = !c.lzma.extreme; }
inline void ao_c_lzma_b(dj::compress_options& c)
    { c.lzma.dict_size = (c.lzma.dict_size + 1u); }
inline void ao_c_lzma_c(dj::compress_options& c)
    { c.lzma.threads = (c.lzma.threads + 1u); }
inline void ao_c_zstd_a(dj::compress_options& c)
    { c.zstd.level = ao_bump(c.zstd.level); }
inline void ao_c_zstd_b(dj::compress_options& c)
    { c.zstd.dict_id_flag = !c.zstd.dict_id_flag; }
inline void ao_c_zstd_c(dj::compress_options& c)
    { c.zstd.job_size = (c.zstd.job_size + 1u); }
inline void ao_c_lz4_a(dj::compress_options& c)
    { c.lz4.level = ao_bump(c.lz4.level); }
inline void ao_c_lz4_b(dj::compress_options& c)
    { c.lz4.favor_dec_speed = !c.lz4.favor_dec_speed; }
inline void ao_c_brotli_a(dj::compress_options& c)
    { c.brotli.window_bits = ao_bump(c.brotli.window_bits); }
inline void ao_c_brotli_b(dj::compress_options& c)
    { c.brotli.large_window = !c.brotli.large_window; }

// ao_codec_fields
//   function: codec sub-fields spread across all six blocks, each paired with
// the COARSE label the archive-level diff must report for it.
inline std::vector<ao_codec_field>
ao_codec_fields()
{
    std::vector<ao_codec_field> t;

    t.push_back(ao_codec_field("codec.deflate", &ao_c_deflate_a));
    t.push_back(ao_codec_field("codec.deflate", &ao_c_deflate_b));
    t.push_back(ao_codec_field("codec.deflate", &ao_c_deflate_c));
    t.push_back(ao_codec_field("codec.bzip2", &ao_c_bzip2_a));
    t.push_back(ao_codec_field("codec.bzip2", &ao_c_bzip2_b));
    t.push_back(ao_codec_field("codec.lzma", &ao_c_lzma_a));
    t.push_back(ao_codec_field("codec.lzma", &ao_c_lzma_b));
    t.push_back(ao_codec_field("codec.lzma", &ao_c_lzma_c));
    t.push_back(ao_codec_field("codec.zstd", &ao_c_zstd_a));
    t.push_back(ao_codec_field("codec.zstd", &ao_c_zstd_b));
    t.push_back(ao_codec_field("codec.zstd", &ao_c_zstd_c));
    t.push_back(ao_codec_field("codec.lz4", &ao_c_lz4_a));
    t.push_back(ao_codec_field("codec.lz4", &ao_c_lz4_b));
    t.push_back(ao_codec_field("codec.brotli", &ao_c_brotli_a));
    t.push_back(ao_codec_field("codec.brotli", &ao_c_brotli_b));

    return t;
}


// ao_contains
//   function: true iff _hay contains _needle as a substring.
inline bool
ao_contains(
    const std::string& _hay,
    const std::string& _needle
)
{
    return (_hay.find(_needle) != std::string::npos);
}

// ao_split_csv
//   function: splits a ", "-separated diff string into its field names, so a
// test can assert exact membership and order rather than substrings.
inline std::vector<std::string>
ao_split_csv(
    const std::string& _s
)
{
    std::vector<std::string> out;
    std::size_t              start = 0;

    if (_s.empty())
    {
        return out;
    }

    for (;;)
    {
        const std::size_t at = _s.find(", ", start);

        if (at == std::string::npos)
        {
            out.push_back(_s.substr(start));
            break;
        }

        out.push_back(_s.substr(start, at - start));
        start = (at + 2u);
    }

    return out;
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   BLOCKS   (test_archive_options_tests_blocks.cpp)
bool tests_archive_options_blocks_reflexive();
bool tests_archive_options_zip_equal_all_fields();
bool tests_archive_options_tar_equal_all_fields();
bool tests_archive_options_gz_equal_all_fields();
bool tests_archive_options_sevenzip_equal_all_fields();
bool tests_archive_options_rar_equal_all_fields();
bool tests_archive_options_blocks_are_independent();
bool tests_archive_options_blocks_compare_strings();

// II.  AGGREGATE   (test_archive_options_tests_aggregate.cpp)
bool tests_archive_options_equal_reflexive();
bool tests_archive_options_equal_symmetric();
bool tests_archive_options_equal_all_own_fields();
bool tests_archive_options_equal_detects_embedded_codec();
bool tests_archive_options_equal_multi_change();
bool tests_archive_options_equal_survives_copy();

// III. PRISTINE   (test_archive_options_tests_pristine.cpp)
bool tests_archive_options_default_is_pristine();
bool tests_archive_options_default_returns_fresh_value();
bool tests_archive_options_are_default_rejects_all();
bool tests_archive_options_codec_default_tracks();
bool tests_archive_options_codec_default_rejects();
bool tests_archive_options_mutator_table_is_complete();

// IV.  DIFF   (test_archive_options_tests_diff.cpp)
bool tests_archive_options_diff_empty_when_equal();
bool tests_archive_options_diff_names_every_field();
bool tests_archive_options_diff_codec_is_block_granular();
bool tests_archive_options_diff_codec_level_is_named();
bool tests_archive_options_diff_separator_and_order();
bool tests_archive_options_diff_lists_all_fields_at_once();
bool tests_archive_options_diff_is_symmetric();
bool tests_archive_options_diff_from_default_matches();
bool tests_archive_options_diff_delegates();

// V.   BUILDERS   (test_archive_options_tests_builders.cpp)
bool tests_archive_options_store_only_builder();
bool tests_archive_options_level_builder();
bool tests_archive_options_codec_level_builder();
bool tests_archive_options_level_builders_are_distinct();
bool tests_archive_options_zip_method_builder();
bool tests_archive_options_zip_encrypted();
bool tests_archive_options_zip_encrypted_empty_pw();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// archive_options_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
archive_options_spec()
{
    return dt::module_spec{
        "test_archive_options",
        "Deep comparison, pristine-default predicates and a readable diff for "
        "the archive option vocabulary - twenty-five own fields across five "
        "per-format blocks, plus an embedded compress_options reported at "
        "deliberately coarser granularity.",
        {
            dt::block_spec{
                "blocks",
                "The five per-format comparators, each checked field by field "
                "from the suite's mutation table, and against one another for "
                "independence.",
                {
                    { "tests_archive_options_blocks_reflexive",
                      "each block compares equal to itself and to an "
                      "independently constructed pristine block",
                      &tests_archive_options_blocks_reflexive },
                    { "tests_archive_options_zip_equal_all_fields",
                      "all five ZIP fields are compared, including the "
                      "passphrase string",
                      &tests_archive_options_zip_equal_all_fields },
                    { "tests_archive_options_tar_equal_all_fields",
                      "both tar fields are compared",
                      &tests_archive_options_tar_equal_all_fields },
                    { "tests_archive_options_gz_equal_all_fields",
                      "all three gzip header fields are compared",
                      &tests_archive_options_gz_equal_all_fields },
                    { "tests_archive_options_sevenzip_equal_all_fields",
                      "all six 7z fields are compared",
                      &tests_archive_options_sevenzip_equal_all_fields },
                    { "tests_archive_options_rar_equal_all_fields",
                      "all four RAR fields are compared",
                      &tests_archive_options_rar_equal_all_fields },
                    { "tests_archive_options_blocks_are_independent",
                      "moving a field in one block leaves the other four "
                      "comparators reporting equal",
                      &tests_archive_options_blocks_are_independent },
                    { "tests_archive_options_blocks_compare_strings",
                      "the passphrase and name fields compare by content, so "
                      "two equal strings from different sources match",
                      &tests_archive_options_blocks_compare_strings },
                }
            },
            dt::block_spec{
                "aggregate",
                "archive_options_equal over the whole aggregate: the five "
                "archive-level fields, every per-format block, and the "
                "embedded codec reached through the compress comparator.",
                {
                    { "tests_archive_options_equal_reflexive",
                      "a set equals itself, and two independently constructed "
                      "pristine sets are equal",
                      &tests_archive_options_equal_reflexive },
                    { "tests_archive_options_equal_symmetric",
                      "the comparison gives the same answer whichever way "
                      "round its operands are passed",
                      &tests_archive_options_equal_symmetric },
                    { "tests_archive_options_equal_all_own_fields",
                      "all twenty-five of the aggregate's own fields are "
                      "reachable by the comparator",
                      &tests_archive_options_equal_all_own_fields },
                    { "tests_archive_options_equal_detects_embedded_codec",
                      "a change anywhere inside the embedded compress_options "
                      "is detected, however deeply nested",
                      &tests_archive_options_equal_detects_embedded_codec },
                    { "tests_archive_options_equal_multi_change",
                      "simultaneous changes across blocks are detected, and "
                      "undoing all but one is not enough to regain equality",
                      &tests_archive_options_equal_multi_change },
                    { "tests_archive_options_equal_survives_copy",
                      "a copied set compares equal to its source and is "
                      "genuinely independent of it afterwards",
                      &tests_archive_options_equal_survives_copy },
                }
            },
            dt::block_spec{
                "pristine",
                "The default baseline, the whole-tree pristine predicate, and "
                "the codec-only predicate a container test leans on to prove "
                "it left the stream tuning alone.",
                {
                    { "tests_archive_options_default_is_pristine",
                      "a default-constructed set reports pristine and carries "
                      "an empty diff",
                      &tests_archive_options_default_is_pristine },
                    { "tests_archive_options_default_returns_fresh_value",
                      "the factory returns by value, so mutating one result "
                      "cannot poison a later call",
                      &tests_archive_options_default_returns_fresh_value },
                    { "tests_archive_options_are_default_rejects_all",
                      "moving any field in the table makes the pristine "
                      "predicate report false",
                      &tests_archive_options_are_default_rejects_all },
                    { "tests_archive_options_codec_default_tracks",
                      "container knobs may move freely without disturbing the "
                      "codec-only predicate",
                      &tests_archive_options_codec_default_tracks },
                    { "tests_archive_options_codec_default_rejects",
                      "a change anywhere in the embedded codec makes the "
                      "codec-only predicate report false",
                      &tests_archive_options_codec_default_rejects },
                    { "tests_archive_options_mutator_table_is_complete",
                      "the table carries thirty-two rows with unique labels, "
                      "matching the module's documented field counts",
                      &tests_archive_options_mutator_table_is_complete },
                }
            },
            dt::block_spec{
                "diff",
                "describe_option_diff: individual names for the archive-level "
                "and per-format fields, block granularity for the codec with "
                "codec.level called out, and the against-pristine variant.",
                {
                    { "tests_archive_options_diff_empty_when_equal",
                      "two identical sets produce an empty diff string",
                      &tests_archive_options_diff_empty_when_equal },
                    { "tests_archive_options_diff_names_every_field",
                      "each single change produces exactly the documented "
                      "label and nothing else",
                      &tests_archive_options_diff_names_every_field },
                    { "tests_archive_options_diff_codec_is_block_granular",
                      "a codec knob collapses onto its block label rather "
                      "than escaping as a field name",
                      &tests_archive_options_diff_codec_is_block_granular },
                    { "tests_archive_options_diff_codec_level_is_named",
                      "codec.level is called out individually, unlike the "
                      "blocks around it",
                      &tests_archive_options_diff_codec_level_is_named },
                    { "tests_archive_options_diff_separator_and_order",
                      "several changed fields are joined by a comma and a "
                      "space, in the module's declaration order",
                      &tests_archive_options_diff_separator_and_order },
                    { "tests_archive_options_diff_lists_all_fields_at_once",
                      "a set with every table field moved names all "
                      "thirty-two, once each",
                      &tests_archive_options_diff_lists_all_fields_at_once },
                    { "tests_archive_options_diff_is_symmetric",
                      "the diff names the same fields whichever way round the "
                      "two sets are passed",
                      &tests_archive_options_diff_is_symmetric },
                    { "tests_archive_options_diff_from_default_matches",
                      "the against-pristine convenience agrees with an "
                      "explicit comparison against a fresh set",
                      &tests_archive_options_diff_from_default_matches },
                    { "tests_archive_options_diff_delegates",
                      "the coarse view and the field-level view of the same "
                      "codec change agree on which block moved",
                      &tests_archive_options_diff_delegates },
                }
            },
            dt::block_spec{
                "builders",
                "The one-knob presets: each must move exactly what it names "
                "and leave the rest of the aggregate, including the embedded "
                "codec, pristine.",
                {
                    { "tests_archive_options_store_only_builder",
                      "the store-only preset raises that flag and nothing "
                      "else",
                      &tests_archive_options_store_only_builder },
                    { "tests_archive_options_level_builder",
                      "the archive-level preset carries any effort through "
                      "and moves nothing else",
                      &tests_archive_options_level_builder },
                    { "tests_archive_options_codec_level_builder",
                      "the codec-level preset moves codec.level alone, "
                      "leaving the archive level pristine",
                      &tests_archive_options_codec_level_builder },
                    { "tests_archive_options_level_builders_are_distinct",
                      "the archive and codec level presets produce different "
                      "sets from the same effort",
                      &tests_archive_options_level_builders_are_distinct },
                    { "tests_archive_options_zip_method_builder",
                      "the ZIP method preset sets the method and leaves the "
                      "other four ZIP fields pristine",
                      &tests_archive_options_zip_method_builder },
                    { "tests_archive_options_zip_encrypted",
                      "the encryption preset carries both the scheme and the "
                      "passphrase, and moves only those two",
                      &tests_archive_options_zip_encrypted },
                    { "tests_archive_options_zip_encrypted_empty_pw",
                      "an empty passphrase is stored as given, so only the "
                      "scheme is reported as moved",
                      &tests_archive_options_zip_encrypted_empty_pw },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_TEST_ARCHIVE_OPTIONS_TESTS_
