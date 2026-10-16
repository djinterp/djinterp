/*******************************************************************************
* djinterp [test]                                            test_pack_tests.hpp
*
*   Declarations, shared fixtures, and the authored suite spec for the
* test_pack.hpp unit suite - the bridge that turns a runtime packaging request
* (the pack / compressor / archive_format knobs of a test_option_set) into the
* matching compile-time facade call.  Two faces, selected by DTEST_SPEC_MODE:
*   - NORMAL mode (the four section TUs): includes the header under test and
*     exposes the check helper + the face-neutral option setters; the TUs define
*     the tests_* bodies against an instrumented compress / archive facade.
*   - SPEC mode (the runner): includes the authoring surface and exposes
*     pack_spec(), the module_spec provider the runner hands to run_module.
* The tests_* DECLARATIONS are visible in both modes.
*
*   Sections -> one translation unit each:
*   - test_pack_tests_naming.cpp        the pure string helpers (base-name,
*                                       container-extension strip, doc extension).
*   - test_pack_tests_availability.cpp  pack_enabled + the codec / format routers.
*   - test_pack_tests_dispatch.cpp      the internal enum -> facade-tag routing.
*   - test_pack_tests_report.cpp        pack_report's pack-mode dispatch + status.
*
*   Tests live, flat, in djinterp::testing.
*
*
* path:      /tests/djinterp/test/test_pack_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_PACK_TESTS_
#define DJINTERP_TEST_PACK_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_pack.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the DTest surface (djinterp::test); dj the facade surface (djinterp).
namespace dt = ::djinterp::test;
namespace dj = ::djinterp;


#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only

// =========================================================================
//  shared check helper
// =========================================================================

// test_pack_check
//   reports a single check.  On failure prints the stringized expression with
// its source location; always returns the condition so callers fold it.
inline bool
test_pack_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// =========================================================================
//  face-neutral option-set setters
// =========================================================================
//   test_option_set has two faces: the pre-C++20 aggregate (member assignment)
// and the C++20 option_set<> (set<key>()).  These helpers write through the
// right one so the same test body runs on both faces.

inline void tp_set_pack(dt::test_option_set& _o, dt::test_output_pack _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::pack>(_v);
#else
    _o.pack = _v;
#endif
}

inline void tp_set_compressor(dt::test_option_set& _o, dt::test_compressor _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::compressor>(_v);
#else
    _o.compressor = _v;
#endif
}

inline void tp_set_compress_opts(dt::test_option_set& _o, const dj::compress_options& _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::compress_opts>(_v);
#else
    _o.compress_opts = _v;
#endif
}

inline void tp_set_archive_format(dt::test_option_set& _o, dt::test_archive_format _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::archive_format>(_v);
#else
    _o.archive_format = _v;
#endif
}

inline void tp_set_archive_opts(dt::test_option_set& _o, const dj::archive_options& _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::archive_opts>(_v);
#else
    _o.archive_opts = _v;
#endif
}

inline void tp_set_document(dt::test_option_set& _o, dt::test_doc_type _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::document>(_v);
#else
    _o.document = _v;
#endif
}

inline void tp_set_output_path(dt::test_option_set& _o, const std::string& _v)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::output_file>(_v);
#else
    _o.output_path = _v;
#endif
}

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// naming helpers
bool tests_naming_doc_ext();
bool tests_naming_basename_unix();
bool tests_naming_basename_windows();
bool tests_naming_container_strip();
bool tests_naming_tar_gz_double_strip();
bool tests_naming_no_extension();
bool tests_naming_empty_and_dotfile();

// availability
bool tests_avail_pack_enabled();
bool tests_avail_codec_routing();
bool tests_avail_codec_default();
bool tests_avail_format_routing();
bool tests_avail_format_default();

// internal dispatch
bool tests_dispatch_compress_routing();
bool tests_dispatch_compress_opt_payload();
bool tests_dispatch_compress_default();
bool tests_dispatch_archive_routing();
bool tests_dispatch_archive_items();
bool tests_dispatch_archive_default();

// pack_report
bool tests_report_none();
bool tests_report_clears_out();
bool tests_report_compress();
bool tests_report_archive();
bool tests_report_archive_name_variants();
bool tests_report_compress_failure();
bool tests_report_archive_failure();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from what each test actually asserts.
// =========================================================================

inline dt::module_spec
pack_spec()
{
    return dt::module_spec{
        "test_pack",
        "test_pack.hpp - the bridge from a runtime packaging request (the pack / "
        "compressor / archive_format option knobs) to the matching compile-time "
        "compress / archive facade call: the naming helpers, the availability "
        "routers, the internal enum->tag dispatch, and pack_report.",
        {
            dt::block_spec{
                "naming",
                "The pure output-naming helpers: document extension, base-name "
                "extraction across unix / windows paths, and container-extension "
                "stripping (including the .tar.gz double strip).",
                {
                    { "tests_naming_doc_ext",
                      "pack_doc_ext_helper maps each document type to its file extension (txt / xml / html / pdf)",
                      &tests_naming_doc_ext },
                    { "tests_naming_basename_unix",
                      "the entry name is the base name with the container extension swapped for the document's, over unix paths",
                      &tests_naming_basename_unix },
                    { "tests_naming_basename_windows",
                      "backslash-separated windows paths are reduced to the base name the same way",
                      &tests_naming_basename_windows },
                    { "tests_naming_container_strip",
                      "a single container extension (.zip / .gz) is stripped before the document extension is applied",
                      &tests_naming_container_strip },
                    { "tests_naming_tar_gz_double_strip",
                      "a .tar.gz strips both the .gz and the residual .tar, leaving just the stem",
                      &tests_naming_tar_gz_double_strip },
                    { "tests_naming_no_extension",
                      "a name with no extension simply gains the document extension",
                      &tests_naming_no_extension },
                    { "tests_naming_empty_and_dotfile",
                      "an empty path or a trailing separator falls back to the default 'report' stem",
                      &tests_naming_empty_and_dotfile },
                }
            },
            dt::block_spec{
                "availability",
                "pack_enabled and the codec / format availability routers, each "
                "checked against the facade's own per-tag answer.",
                {
                    { "tests_avail_pack_enabled",
                      "pack_enabled reflects whether the option set actually requests packaging",
                      &tests_avail_pack_enabled },
                    { "tests_avail_codec_routing",
                      "report_codec_available routes each compressor to the facade's per-codec availability answer",
                      &tests_avail_codec_routing },
                    { "tests_avail_codec_default",
                      "an unknown compressor reports unavailable (the default arm)",
                      &tests_avail_codec_default },
                    { "tests_avail_format_routing",
                      "report_format_available routes each archive format to the facade's per-format writability answer",
                      &tests_avail_format_routing },
                    { "tests_avail_format_default",
                      "an unknown archive format reports unavailable",
                      &tests_avail_format_default },
                }
            },
            dt::block_spec{
                "dispatch",
                "The internal enum -> facade-tag routing, verified by an "
                "instrumented facade that encodes the selected tag, options, and "
                "payload into its output.",
                {
                    { "tests_dispatch_compress_routing",
                      "the compress dispatch routes each compressor enum to its matching facade tag",
                      &tests_dispatch_compress_routing },
                    { "tests_dispatch_compress_opt_payload",
                      "compress dispatch forwards the options (level) and payload to the selected codec intact",
                      &tests_dispatch_compress_opt_payload },
                    { "tests_dispatch_compress_default",
                      "an out-of-range compressor yields invalid-argument (the unreachable default)",
                      &tests_dispatch_compress_default },
                    { "tests_dispatch_archive_routing",
                      "the archive dispatch routes each format enum to its matching facade tag",
                      &tests_dispatch_archive_routing },
                    { "tests_dispatch_archive_items",
                      "archive dispatch forwards the entry items to the selected format intact",
                      &tests_dispatch_archive_items },
                    { "tests_dispatch_archive_default",
                      "an out-of-range archive format yields invalid-argument",
                      &tests_dispatch_archive_default },
                }
            },
            dt::block_spec{
                "report",
                "pack_report's pack-mode dispatch (none / compress / archive), the "
                "entry-name integration, and status propagation.",
                {
                    { "tests_report_none",
                      "pack mode 'none' passes the report bytes through unchanged with status ok",
                      &tests_report_none },
                    { "tests_report_clears_out",
                      "an invalid request clears the output buffer and reports invalid-argument",
                      &tests_report_clears_out },
                    { "tests_report_compress",
                      "pack mode 'compress' routes the report through the selected codec with its options",
                      &tests_report_compress },
                    { "tests_report_archive",
                      "pack mode 'archive' wraps the report as a single named archive entry",
                      &tests_report_archive },
                    { "tests_report_archive_name_variants",
                      "the archive entry name honors the output-path / document-type naming",
                      &tests_report_archive_name_variants },
                    { "tests_report_compress_failure",
                      "a compress request for an unavailable codec fails with status unavailable and clears the output",
                      &tests_report_compress_failure },
                    { "tests_report_archive_failure",
                      "an archive request for an unavailable format fails with status unavailable and clears the output",
                      &tests_report_archive_failure },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


// D_TP_CHECK
//   evaluate once, route through test_pack_check with the expression text +
// source location.  Variadic so top-level commas need no extra parentheses.
#define D_TP_CHECK(...)                                                        \
    ::djinterp::testing::test_pack_check(                                      \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_PACK_TESTS_
