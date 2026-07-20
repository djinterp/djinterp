/******************************************************************************
* djinterp [tests]                             test_report_runner_table_tests.hpp
*
*   DTest suite header for test_report_runner_table.hpp - the report_builder front
* end that accumulates a run into a test_report while emitting the live console
* report, then renders the configured document(s) at finish() and returns the
* process exit code.  Two-faced in the DTEST_SPEC_MODE idiom:
*
*     - normal mode (the section TUs): pulls in the header under test and
*       exposes the check macro + the console/file/pdf helpers.
*     - spec  mode (the runner): pulls in test_defaults.hpp and exposes
*       report_runner_table_spec().
*
*   SECTION SPLIT (mirrors the code's a-e layout + the free-function tail):
*     config       construction, the option/metadata setters, the option
*                  readers a runner sets before finishing
*     record       module / run / unit / open_unit / check / close_unit
*                  recording, verified through the exposed test_report
*     console      the live console + finish() summaries and the exit code,
*                  captured by redirecting the builder's FILE* sink
*     document     the PDF write path: whole_run vs per_module split, filename
*                  patterns, the output_file slot, and the not-written cases
*     convenience  run_named_tests, report_detail::to_report_string, D_CHECK_EQ
*   One section TU per group; all tests are flat in djinterp::testing.
*
*   HOW A RUNNER IS ASSERTED.  Four observation channels:
*     (1) report() exposes the accumulating test_report, so recording is checked
*         exactly - module/unit/check counts, names, statuses, and the
*         expression / expected / actual detail.
*     (2) finish() returns the process exit code (0 iff the run passed).
*     (3) The console sink is a settable std::FILE* (set_file); pointing it at a
*         tmpfile() captures every emitted line for substring assertions.
*     (4) The PDF path writes real files; tests direct them at temp paths, then
*         confirm the bytes are a well-formed PDF and clean the files up.
*
*   ARCHIVE PATH.  write_archived_report / try_build_archive and the .7z bundle
* are compiled only when D_TEST_REPORT_ENABLE_ARCHIVE is defined before the
* header (an opt-in that also pulls in the archive facade).  This suite covers
* the DEFAULT build, where that block does not exist; the archive path is out of
* scope here and would need its own TU built with that macro + the archive deps.
*
*   PORTABILITY:  C++11 (matches the header under test).  The console/pdf tests
* rely on a writable temp directory (tmpfile(), /tmp) - standard on the CI host.
*
*
* path:      /tests/djinterp/test/output/test_report_runner_table_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_REPORT_RUNNER_TABLE_TESTS_
#define DJINTERP_TESTS_TEST_REPORT_RUNNER_TABLE_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>

// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "test_report_runner_table.hpp"                // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"       // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING


// dt names the entities under test (djinterp::test); the spec provider needs
// dt::module_spec / block_spec / test_spec too, so this is unconditional.
namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                CHECK MACRO  (unconditional)                             ///
///////////////////////////////////////////////////////////////////////////////
//   Unique two-letter suffix (D_RRT_, "report runner") so co-compiled suites
// never collide.  Variadic + routed print so a top-level comma inside a checked
// expression passes through whole.  Early-returns false on the first failure.

inline void
report_runner_table_fail(
    const char* _expr,
    const char* _file,
    int         _line
)
{
    std::printf("    check failed: %s\n    at: %s:%d\n", _expr, _file, _line);

    return;
}

#define D_RRT_CHECK(...)                                                        \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            ::djinterp::testing::report_runner_table_fail(                          \
                #__VA_ARGS__, __FILE__, __LINE__);                            \
            return false;                                                     \
        }                                                                     \
    } while (0)


///////////////////////////////////////////////////////////////////////////////
///                HELPERS  (unconditional)                                 ///
///////////////////////////////////////////////////////////////////////////////
//   All operate on std::string / std::FILE* only - no header-under-test type -
// so they are safe unconditionally and usable from every section TU.

// contains
//   whether _hay holds _needle.
inline bool
contains(
    const std::string& _hay,
    const std::string& _needle
)
{
    return (_hay.find(_needle) != std::string::npos);
}

// pdf_wellformed
//   non-empty, opens with "%PDF", and carries the "%%EOF" trailer.
inline bool
pdf_wellformed(
    const std::string& _bytes
)
{
    return ( (!_bytes.empty()) &&
             (_bytes.compare(0, 4, "%PDF") == 0) &&
             (_bytes.find("%%EOF") != std::string::npos) );
}

// rr_pred_pass / rr_pred_fail
//   trivial predicates for run() (test_predicate_fn == bool(*)()).
inline bool rr_pred_pass() { return true; }
inline bool rr_pred_fail() { return false; }

// console_capture
//   a scoped tmpfile() the builder's console can be pointed at (set_file), with
// str() reading back everything written so far.  Non-copyable: it owns a FILE*.
struct console_capture
{
    std::FILE* f;

    console_capture()
        : f(std::tmpfile())
    {}

    ~console_capture()
    {
        if (f != nullptr)
        {
            std::fclose(f);
        }
    }

    std::string
    str() const
    {
        if (f == nullptr)
        {
            return std::string();
        }

        std::fflush(f);

        const long _end = std::ftell(f);

        if (_end <= 0)
        {
            return std::string();
        }

        std::rewind(f);

        std::string _s;
        _s.resize(static_cast<std::size_t>(_end));

        const std::size_t _n = std::fread(&_s[0], 1, _s.size(), f);
        _s.resize(_n);

        return _s;
    }

    console_capture(const console_capture&)            = delete;
    console_capture& operator=(const console_capture&) = delete;
};

// file_exists
//   true iff _path can be opened for reading.
inline bool
file_exists(
    const std::string& _path
)
{
    std::FILE* _f = std::fopen(_path.c_str(), "rb");

    if (_f == nullptr)
    {
        return false;
    }

    std::fclose(_f);

    return true;
}

// read_file
//   the whole contents of _path (empty if it cannot be opened).
inline std::string
read_file(
    const std::string& _path
)
{
    std::FILE* _f = std::fopen(_path.c_str(), "rb");

    if (_f == nullptr)
    {
        return std::string();
    }

    std::string _s;
    char        _buf[4096];
    std::size_t _n = 0;

    while ((_n = std::fread(_buf, 1, sizeof(_buf), _f)) > 0)
    {
        _s.append(_buf, _n);
    }

    std::fclose(_f);

    return _s;
}

// remove_file
//   unlinks _path (best-effort; ignores absence).
inline void
remove_file(
    const std::string& _path
)
{
    std::remove(_path.c_str());

    return;
}

// temp_out_path
//   a process-unique temp path ending in _leaf, for a document the runner will
// write and the test will read back then remove.
inline std::string
temp_out_path(
    const std::string& _leaf
)
{
    static unsigned _ctr = 0;
    ++_ctr;

    char _buf[64];
    std::snprintf(_buf, sizeof(_buf), "/tmp/djinterp_rr_%u_", _ctr);

    return std::string(_buf) + _leaf;
}


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   CONFIG   (test_report_runner_table_tests_config.cpp)
bool tests_report_runner_table_default_options();
bool tests_report_runner_table_explicit_options_ctor();
bool tests_report_runner_table_metadata_setters();
bool tests_report_runner_table_use_pdf_variants();
bool tests_report_runner_table_set_document_output_file();
bool tests_report_runner_table_set_split_show();

// II.  RECORD   (test_report_runner_table_tests_record.cpp)
bool tests_report_runner_table_module_opens();
bool tests_report_runner_table_run_records_and_returns();
bool tests_report_runner_table_run_null_predicate();
bool tests_report_runner_table_run_creates_default_module();
bool tests_report_runner_table_unit_records_known();
bool tests_report_runner_table_open_unit_and_check_detail();
bool tests_report_runner_table_check_noop_without_unit();
bool tests_report_runner_table_multi_module_tallies();

// III. CONSOLE   (test_report_runner_table_tests_console.cpp)
bool tests_report_runner_table_console_module_banner();
bool tests_report_runner_table_console_unit_lines();
bool tests_report_runner_table_console_finish_summaries();
bool tests_report_runner_table_console_assessment_fail();
bool tests_report_runner_table_console_unit_block_lines();
bool tests_report_runner_table_console_disabled();
bool tests_report_runner_table_finish_exit_code();

// IV.  DOCUMENT   (test_report_runner_table_tests_document.cpp)
bool tests_report_runner_table_pdf_whole_run();
bool tests_report_runner_table_pdf_per_module();
bool tests_report_runner_table_pdf_filename_pattern();
bool tests_report_runner_table_pdf_whole_run_fallback_name();
bool tests_report_runner_table_txt_writes_nothing();
bool tests_report_runner_table_pdf_write_failure_message();
bool tests_report_runner_table_pdf_silent_suppresses_message();

// V.   CONVENIENCE   (test_report_runner_table_tests_convenience.cpp)
bool tests_report_runner_table_run_named_tests_pass();
bool tests_report_runner_table_run_named_tests_fail();
bool tests_report_runner_table_run_named_tests_empty();
bool tests_report_runner_table_to_report_string_overloads();
bool tests_report_runner_table_to_report_string_null_cstr();
bool tests_report_runner_table_check_eq_pass();
bool tests_report_runner_table_check_eq_fail();
bool tests_report_runner_table_check_eq_single_eval();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
report_runner_table_spec()
{
    return dt::module_spec{
        "test_report_runner_table",
        "The report_builder front end: accumulates a run into a test_report "
        "while emitting the live console report, then renders the configured "
        "document(s) at finish() and returns the process exit code.",
        {
            dt::block_spec{
                "config",
                "Construction and the option / metadata setters a runner uses "
                "to configure the builder before finishing.",
                {
                    { "tests_report_runner_table_default_options",
                      "a default builder carries the framework-default options "
                      "and an empty report",
                      &tests_report_runner_table_default_options },
                    { "tests_report_runner_table_explicit_options_ctor",
                      "the option-set constructor adopts the supplied options",
                      &tests_report_runner_table_explicit_options_ctor },
                    { "tests_report_runner_table_metadata_setters",
                      "the set_title-family helpers write the report's cover "
                      "metadata",
                      &tests_report_runner_table_metadata_setters },
                    { "tests_report_runner_table_use_pdf_variants",
                      "use_pdf selects the PDF document, optionally sets the "
                      "output path, and returns the builder",
                      &tests_report_runner_table_use_pdf_variants },
                    { "tests_report_runner_table_set_document_output_file",
                      "set_document and set_output_file drive the document and "
                      "output_file option slots",
                      &tests_report_runner_table_set_document_output_file },
                    { "tests_report_runner_table_set_split_show",
                      "set_split and set_show drive the split and show option "
                      "slots",
                      &tests_report_runner_table_set_split_show },
                }
            },
            dt::block_spec{
                "record",
                "Module / unit / assertion recording, verified through the "
                "exposed test_report model.",
                {
                    { "tests_report_runner_table_module_opens",
                      "module appends a named module and makes it current",
                      &tests_report_runner_table_module_opens },
                    { "tests_report_runner_table_run_records_and_returns",
                      "run executes the predicate, records a one-check unit, and "
                      "returns the outcome",
                      &tests_report_runner_table_run_records_and_returns },
                    { "tests_report_runner_table_run_null_predicate",
                      "run treats a null predicate as a failure",
                      &tests_report_runner_table_run_null_predicate },
                    { "tests_report_runner_table_run_creates_default_module",
                      "run without an open module creates the default \"tests\" "
                      "module",
                      &tests_report_runner_table_run_creates_default_module },
                    { "tests_report_runner_table_unit_records_known",
                      "unit records a result whose pass/fail is already known",
                      &tests_report_runner_table_unit_records_known },
                    { "tests_report_runner_table_open_unit_and_check_detail",
                      "open_unit + the detail check overload accumulate "
                      "assertions with expression / expected / actual",
                      &tests_report_runner_table_open_unit_and_check_detail },
                    { "tests_report_runner_table_check_noop_without_unit",
                      "check and close_unit are no-ops with no module or no open "
                      "unit",
                      &tests_report_runner_table_check_noop_without_unit },
                    { "tests_report_runner_table_multi_module_tallies",
                      "recording across several modules rolls up into the "
                      "report-wide tallies",
                      &tests_report_runner_table_multi_module_tallies },
                }
            },
            dt::block_spec{
                "console",
                "The live console output and the finish() summaries, captured "
                "through a redirected FILE* sink, plus the exit code.",
                {
                    { "tests_report_runner_table_console_module_banner",
                      "module emits its banner, with a description line only "
                      "when a description is given",
                      &tests_report_runner_table_console_module_banner },
                    { "tests_report_runner_table_console_unit_lines",
                      "run emits a bracketed pass/fail result line per test",
                      &tests_report_runner_table_console_unit_lines },
                    { "tests_report_runner_table_console_finish_summaries",
                      "finish emits the module results box, the comprehensive "
                      "roll-up, the pass assessment, and the final tally",
                      &tests_report_runner_table_console_finish_summaries },
                    { "tests_report_runner_table_console_assessment_fail",
                      "a failing run's assessment reads FAILED and the tally "
                      "counts the failure",
                      &tests_report_runner_table_console_assessment_fail },
                    { "tests_report_runner_table_console_unit_block_lines",
                      "open_unit / check / close_unit emit their console lines",
                      &tests_report_runner_table_console_unit_block_lines },
                    { "tests_report_runner_table_console_disabled",
                      "a disabled console emits nothing",
                      &tests_report_runner_table_console_disabled },
                    { "tests_report_runner_table_finish_exit_code",
                      "finish returns 0 for a clean run and 1 when a test failed",
                      &tests_report_runner_table_finish_exit_code },
                }
            },
            dt::block_spec{
                "document",
                "The PDF write path: the split matrix, filename patterns, the "
                "output_file slot, and the not-written cases.",
                {
                    { "tests_report_runner_table_pdf_whole_run",
                      "a whole_run PDF writes one well-formed document to the "
                      "output path",
                      &tests_report_runner_table_pdf_whole_run },
                    { "tests_report_runner_table_pdf_per_module",
                      "a per_module PDF writes one document per module",
                      &tests_report_runner_table_pdf_per_module },
                    { "tests_report_runner_table_pdf_filename_pattern",
                      "the filename pattern's {module}/{index}/{ext} fields drive "
                      "the per-module paths",
                      &tests_report_runner_table_pdf_filename_pattern },
                    { "tests_report_runner_table_pdf_whole_run_fallback_name",
                      "a whole_run PDF with no output path falls back to the "
                      "\"report\" module name in the pattern",
                      &tests_report_runner_table_pdf_whole_run_fallback_name },
                    { "tests_report_runner_table_txt_writes_nothing",
                      "the default txt document writes no file",
                      &tests_report_runner_table_txt_writes_nothing },
                    { "tests_report_runner_table_pdf_write_failure_message",
                      "an unwritable path yields no file and the not-written "
                      "notice",
                      &tests_report_runner_table_pdf_write_failure_message },
                    { "tests_report_runner_table_pdf_silent_suppresses_message",
                      "the silent show level suppresses the not-written notice",
                      &tests_report_runner_table_pdf_silent_suppresses_message },
                }
            },
            dt::block_spec{
                "convenience",
                "The free-function path, the value-formatting helpers, and the "
                "D_CHECK_EQ capture macro.",
                {
                    { "tests_report_runner_table_run_named_tests_pass",
                      "run_named_tests returns 0 when every listed test passes",
                      &tests_report_runner_table_run_named_tests_pass },
                    { "tests_report_runner_table_run_named_tests_fail",
                      "run_named_tests returns non-zero when a listed test fails",
                      &tests_report_runner_table_run_named_tests_fail },
                    { "tests_report_runner_table_run_named_tests_empty",
                      "run_named_tests over zero tests returns non-zero - an "
                      "empty run folds to the empty verdict, which is not a pass",
                      &tests_report_runner_table_run_named_tests_empty },
                    { "tests_report_runner_table_to_report_string_overloads",
                      "to_report_string renders strings, C-strings, bools, and "
                      "streamable values",
                      &tests_report_runner_table_to_report_string_overloads },
                    { "tests_report_runner_table_to_report_string_null_cstr",
                      "to_report_string maps a null C-string to the empty string",
                      &tests_report_runner_table_to_report_string_null_cstr },
                    { "tests_report_runner_table_check_eq_pass",
                      "D_CHECK_EQ records a passing assertion with captured "
                      "expression / expected / actual",
                      &tests_report_runner_table_check_eq_pass },
                    { "tests_report_runner_table_check_eq_fail",
                      "D_CHECK_EQ records a failing assertion when the operands "
                      "differ",
                      &tests_report_runner_table_check_eq_fail },
                    { "tests_report_runner_table_check_eq_single_eval",
                      "D_CHECK_EQ evaluates each operand exactly once",
                      &tests_report_runner_table_check_eq_single_eval },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_REPORT_RUNNER_TABLE_TESTS_
