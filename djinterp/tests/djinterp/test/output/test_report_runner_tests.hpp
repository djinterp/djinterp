/*******************************************************************************
* djinterp [tests]                                  test_report_runner_tests.hpp
*
*   Suite header for test_report_runner.hpp - the runner-facing front end for
* the report subsystem (report_builder, run_named_tests, D_CHECK_EQ).  Declares
* every test body, supplies the fixtures the bodies need, and (in
* DTEST_SPEC_MODE) exposes the spec provider the runner hands to run_module.
*
*   BUILD PREREQUISITE.
*   test_report_runner.hpp does NOT compile at C++17 or higher unless
* D_TEST_REPORT_ENABLE_ARCHIVE is defined.  Its pending_doc::bytes member and
* its write_bytes_to_file parameter are both spelled `byte_blob`, and
* byte_blob is typedef'd in core/util/compress/compress.hpp, which the header reaches
* ONLY through the archive include guarded by that macro:
*
*     #if defined(D_TEST_REPORT_ENABLE_ARCHIVE) &&
*         D_ENV_LANG_IS_CPP17_OR_HIGHER
*         #include "../../core/util/archive/archive.hpp"
*     #endif
*
*   Left undefined, the two declarations name an undeclared type and the
* translation unit fails.  The minimal fix in the module is to spell those two
* uses `std::string` (byte_blob's definition) or to include compress.hpp
* unconditionally; per the authoring guide this suite records the prerequisite
* rather than patching the module.  The suite's CMakeLists therefore defines
* D_TEST_REPORT_ENABLE_ARCHIVE for the whole target - which also compiles in
* the packaging arm this suite covers, and keeps report_builder's member set
* identical across every translation unit that links together.
*
*   FINDING - run_named_tests EMITS TO stdout UNCONDITIONALLY.
*   run_named_tests constructs its own report_builder from the option set it is
* handed and calls finish() on it.  A default-constructed builder starts with
* its console enabled and its sink set to stdout, and the function exposes no
* sink parameter and no console flag, so every call prints a full console
* report - module banner, per-unit lines, results box, comprehensive summary
* and assessment - to stdout.  The `show` option does NOT gate this: silent is
* consulted at exactly one place, the "report document was not written" note,
* never by the console emitters, which test the builder's private m_console
* only.  The convenience block below therefore asserts on the returned exit
* code alone, which is the whole of what the function exposes, and its tests
* print while they run.  Making this suppressible would mean either a sink
* parameter on run_named_tests or honouring test_show::silent in the emitters;
* per the authoring guide the behaviour is pinned and named here rather than
* worked around.
*
*   HOW THIS SUITE OBSERVES THE BUILDER.
*   Everything below finish() is private, so the console renderers are driven
* through the public surface and observed by pointing set_file() at a scratch
* file (rr_capture) and reading the emitted text back.  Document emission is
* observed the same way, by letting finish() write into the working directory
* and inspecting the files (rr_read_file / rr_file_exists) before removing
* them.  The report model itself is read directly through report().
*
*   PORTABILITY:
*   C++11 baseline for the builder core; the document and archive sections
* ride the header's own C++17 gate.  Built at C++20 by the DTest CMake helper.
*
*
* path:      /tests/djinterp/test/output/test_report_runner_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                              created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONSTRUCTION     (test_report_runner_tests_construction.cpp)
      ------------------------------------------------------------

II.   OPTIONS          (test_report_runner_tests_options.cpp)
      -------------------------------------------------------

III.  RECORDING        (test_report_runner_tests_recording.cpp)
      ---------------------------------------------------------

IV.   CONSOLE          (test_report_runner_tests_console.cpp)
      -------------------------------------------------------

V.    FINISH           (test_report_runner_tests_finish.cpp)
      ------------------------------------------------------

VI.   DOCUMENT         (test_report_runner_tests_document.cpp)
      --------------------------------------------------------

VII.  ARCHIVE          (test_report_runner_tests_archive.cpp)
      -------------------------------------------------------

VIII. CONVENIENCE      (test_report_runner_tests_convenience.cpp)
      -----------------------------------------------------------
*/

#ifndef DJINTERP_TESTS_TEST_REPORT_RUNNER_TESTS_
#define DJINTERP_TESTS_TEST_REPORT_RUNNER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"
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


// report_runner_check
//   function: reports a failed assertion and returns the condition, so the
// D_RR_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
report_runner_check(
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

// D_RR_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_RR_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::report_runner_check(                        \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// -- scratch-file helpers --------------------------------------------------

// rr_read_file
//   function: the whole contents of _path, or an empty string when it
// cannot be opened.
inline std::string
rr_read_file(
    const std::string& _path
)
{
    std::FILE* f = std::fopen(_path.c_str(), "rb");

    if (f == nullptr)
    {
        return std::string();
    }

    std::string out;
    char        buf[4096];
    std::size_t n = 0;

    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
    {
        out.append(buf, n);
    }

    std::fclose(f);

    return out;
}

// rr_file_exists
//   function: true iff _path can be opened for reading.
inline bool
rr_file_exists(
    const std::string& _path
)
{
    std::FILE* f = std::fopen(_path.c_str(), "rb");

    if (f == nullptr)
    {
        return false;
    }

    std::fclose(f);

    return true;
}

// rr_remove
//   function: deletes _path, ignoring a failure to do so.
inline void
rr_remove(
    const std::string& _path
)
{
    std::remove(_path.c_str());

    return;
}

// rr_contains
//   function: true iff _hay contains _needle as a substring.
inline bool
rr_contains(
    const std::string& _hay,
    const std::string& _needle
)
{
    return (_hay.find(_needle) != std::string::npos);
}

// rr_count
//   function: the number of non-overlapping occurrences of _needle in _hay.
inline std::size_t
rr_count(
    const std::string& _hay,
    const std::string& _needle
)
{
    if (_needle.empty())
    {
        return 0;
    }

    std::size_t n   = 0;
    std::size_t pos = 0;

    while ((pos = _hay.find(_needle, pos)) != std::string::npos)
    {
        ++n;
        pos += _needle.size();
    }

    return n;
}


// rr_capture
//   class: a scratch file standing in for the console sink, so a test can
// hand report_builder a FILE* and read back exactly what it emitted.
class rr_capture
{
public:
    // rr_capture
    //   constructor: opens (and truncates) the named scratch file.
    explicit rr_capture(
        const std::string& _path
    )
        : m_path(_path),
          m_file(nullptr)
    {
        std::remove(m_path.c_str());

        m_file = std::fopen(m_path.c_str(), "w+b");
    }

    ~rr_capture()
    {
        close();
        std::remove(m_path.c_str());
    }

    rr_capture(const rr_capture&)            = delete;
    rr_capture& operator=(const rr_capture&) = delete;

    // file
    //   the sink to hand report_builder::set_file, or null on failure.
    std::FILE* file() const { return m_file; }

    // valid
    //   true iff the scratch file opened.
    bool valid() const { return (m_file != nullptr); }

    // text
    //   everything written to the sink so far, leaving the write position
    // at the end so emission can continue afterwards.
    std::string
    text()
    {
        if (m_file == nullptr)
        {
            return std::string();
        }

        std::fflush(m_file);

        const long end = std::ftell(m_file);

        if (end <= 0)
        {
            return std::string();
        }

        std::rewind(m_file);

        std::string out;
        char        buf[4096];
        std::size_t n    = 0;
        long        left = end;

        // read back exactly the bytes written so far
        while (left > 0)
        {
            const std::size_t want =
                (static_cast<std::size_t>(left) < sizeof(buf))
                    ? static_cast<std::size_t>(left)
                    : sizeof(buf);

            n = std::fread(buf, 1, want, m_file);

            if (n == 0)
            {
                break;
            }

            out.append(buf, n);
            left -= static_cast<long>(n);
        }

        std::fseek(m_file, end, SEEK_SET);

        return out;
    }

    // close
    //   closes the sink, so a builder still holding the pointer must not be
    // used afterwards.
    void
    close()
    {
        if (m_file != nullptr)
        {
            std::fclose(m_file);
            m_file = nullptr;
        }

        return;
    }

private:
    std::string m_path;
    std::FILE*  m_file;
};


// -- face-neutral option setters -------------------------------------------
//
//   report_builder exposes conveniences for document / output_file / split /
// show only, so the packaging knobs are written here.  Setting an option
// differs by face (set<> on the C++20 option_set, member assignment on the
// pre-C++20 struct), so each wrapper branches once.

// rr_set_pack
//   function: selects the packaging mode on either option-set face.
inline void
rr_set_pack(
    dt::test_option_set& _o,
    dt::test_output_pack _v
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::pack>(_v);
#else
    _o.pack = _v;
#endif
    return;
}

// rr_set_archive_format
//   function: selects the archive container on either option-set face.
inline void
rr_set_archive_format(
    dt::test_option_set&    _o,
    dt::test_archive_format _v
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _o.set<dt::test_option::archive_format>(_v);
#else
    _o.archive_format = _v;
#endif
    return;
}


// -- sample predicates -----------------------------------------------------

// rr_pass / rr_fail
//   function: trivial unit-test predicates for driving run().
inline bool rr_pass() { return true;  }
inline bool rr_fail() { return false; }

// rr_calls
//   function: a call counter shared by the side-effect tests, reset through
// its own setter so a body can assert an operand was evaluated once.
inline int&
rr_calls()
{
    static int n = 0;

    return n;
}

// rr_counted
//   function: returns _v and records that it was evaluated.
inline int
rr_counted(
    int _v
)
{
    ++rr_calls();

    return _v;
}


// -- sample builders -------------------------------------------------------

// rr_seed_units
//   function: records _pass_n passing and _fail_n failing leaf units into
// _rb's current module.
inline void
rr_seed_units(
    dt::report_builder& _rb,
    std::size_t         _pass_n,
    std::size_t         _fail_n
)
{
    std::size_t i = 0;

    for (i = 0; i < _pass_n; ++i)
    {
        char buf[32];

        std::snprintf(buf, sizeof(buf), "ok_%zu", i);
        _rb.unit(std::string(buf), true);
    }

    for (i = 0; i < _fail_n; ++i)
    {
        char buf[32];

        std::snprintf(buf, sizeof(buf), "bad_%zu", i);
        _rb.unit(std::string(buf), false);
    }

    return;
}

// rr_pdf_options
//   function: a default option set already switched to PDF output.
inline dt::test_option_set
rr_pdf_options()
{
    dt::test_option_set o = dt::default_test_options();

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    o.set<dt::test_option::document>(dt::test_doc_type::pdf);
#else
    o.document = dt::test_doc_type::pdf;
#endif

    return o;
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.    CONSTRUCTION   (test_report_runner_tests_construction.cpp)
bool tests_report_runner_default_construction();
bool tests_report_runner_option_set_construction();
bool tests_report_runner_report_accessors();
bool tests_report_runner_options_accessors();
bool tests_report_runner_metadata_setters();
bool tests_report_runner_console_toggle();
bool tests_report_runner_null_file_sink();

// II.   OPTIONS   (test_report_runner_tests_options.cpp)
bool tests_report_runner_use_pdf_selects_document();
bool tests_report_runner_use_pdf_sets_path();
bool tests_report_runner_use_pdf_empty_path_keeps_existing();
bool tests_report_runner_use_pdf_returns_self();
bool tests_report_runner_set_document();
bool tests_report_runner_set_output_file();
bool tests_report_runner_set_split();
bool tests_report_runner_set_show();
bool tests_report_runner_options_survive_recording();

// III.  RECORDING   (test_report_runner_tests_recording.cpp)
bool tests_report_runner_module_appends();
bool tests_report_runner_module_returns_reference();
bool tests_report_runner_run_records_result();
bool tests_report_runner_run_returns_predicate_value();
bool tests_report_runner_run_null_predicate();
bool tests_report_runner_ensure_module_autocreates();
bool tests_report_runner_unit_records_known_result();
bool tests_report_runner_open_unit_named();
bool tests_report_runner_open_unit_described();
bool tests_report_runner_check_detail_overload();
bool tests_report_runner_check_named_overload();
bool tests_report_runner_check_without_unit_is_noop();
bool tests_report_runner_check_without_module_is_noop();
bool tests_report_runner_close_unit_without_unit_is_noop();
bool tests_report_runner_multiple_modules_isolated();

// IV.   CONSOLE   (test_report_runner_tests_console.cpp)
bool tests_report_runner_module_banner_text();
bool tests_report_runner_module_banner_description();
bool tests_report_runner_unit_line_symbols();
bool tests_report_runner_open_unit_banner();
bool tests_report_runner_check_detail_line();
bool tests_report_runner_check_named_line();
bool tests_report_runner_close_unit_line();
bool tests_report_runner_module_results_box();
bool tests_report_runner_comprehensive_summary();
bool tests_report_runner_assessment_pass();
bool tests_report_runner_assessment_fail();
bool tests_report_runner_rule_width();
bool tests_report_runner_console_disabled_emits_nothing();

// V.    FINISH   (test_report_runner_tests_finish.cpp)
bool tests_report_runner_finish_exit_code_pass();
bool tests_report_runner_finish_exit_code_fail();
bool tests_report_runner_finish_empty_report();
bool tests_report_runner_finish_final_tally();
bool tests_report_runner_finish_is_repeatable();
bool tests_report_runner_finish_multi_module_boxes();
bool tests_report_runner_finish_empty_unit_is_forgiven();

// VI.   DOCUMENT   (test_report_runner_tests_document.cpp)
bool tests_report_runner_txt_writes_no_file();
bool tests_report_runner_pdf_whole_run_writes_one();
bool tests_report_runner_pdf_output_path_used();
bool tests_report_runner_pdf_empty_path_falls_back();
bool tests_report_runner_pdf_per_module_writes_each();
bool tests_report_runner_pdf_per_module_ignores_output_path();
bool tests_report_runner_pdf_file_name_pattern();
bool tests_report_runner_pdf_write_failure_reports();
bool tests_report_runner_pdf_console_notes_path();

// VII.  ARCHIVE   (test_report_runner_tests_archive.cpp)
#if defined(D_TEST_REPORT_ENABLE_ARCHIVE)
bool tests_report_runner_archive_zip_bundles();
bool tests_report_runner_archive_entry_basenames();
bool tests_report_runner_archive_fallback_extension();
bool tests_report_runner_archive_per_module_one_container();
bool tests_report_runner_archive_rar_is_unavailable();
bool tests_report_runner_archive_pack_none_stays_loose();
#endif  // D_TEST_REPORT_ENABLE_ARCHIVE

// VIII. CONVENIENCE   (test_report_runner_tests_convenience.cpp)
bool tests_report_runner_run_named_tests_exit_code();
bool tests_report_runner_run_named_tests_records_all();
bool tests_report_runner_run_named_tests_zero_count();
bool tests_report_runner_check_eq_records_detail();
bool tests_report_runner_check_eq_reports_mismatch();
bool tests_report_runner_check_eq_evaluates_once();
bool tests_report_runner_to_report_string_strings();
bool tests_report_runner_to_report_string_bool();
bool tests_report_runner_to_report_string_generic();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// report_runner_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
report_runner_spec()
{
    return dt::module_spec{
        "test_report_runner",
        "The runner-facing front end for the report subsystem: a "
        "report_builder that accumulates a run into a test_report while "
        "emitting the live console report, then renders the configured "
        "document(s) at finish() and returns the process exit code.",
        {
            dt::block_spec{
                "construction",
                "Building a builder and reaching its parts - the two "
                "constructors, the report and option accessors, the metadata "
                "setters and the console sink controls.",
                {
                    { "tests_report_runner_default_construction",
                      "a default builder starts with an empty report, the "
                      "console on and the framework default options",
                      &tests_report_runner_default_construction },
                    { "tests_report_runner_option_set_construction",
                      "a builder seeded with an option set carries that "
                      "set rather than the defaults",
                      &tests_report_runner_option_set_construction },
                    { "tests_report_runner_report_accessors",
                      "report() exposes the accumulating model for both "
                      "reading and direct population",
                      &tests_report_runner_report_accessors },
                    { "tests_report_runner_options_accessors",
                      "options() exposes the option set mutably, so a "
                      "runner can set knobs the conveniences do not cover",
                      &tests_report_runner_options_accessors },
                    { "tests_report_runner_metadata_setters",
                      "the set_title family writes straight through to the "
                      "report's cover metadata",
                      &tests_report_runner_metadata_setters },
                    { "tests_report_runner_console_toggle",
                      "set_console(false) silences every emitter while "
                      "leaving the model untouched",
                      &tests_report_runner_console_toggle },
                    { "tests_report_runner_null_file_sink",
                      "a null sink suppresses output without faulting, "
                      "even with the console nominally enabled",
                      &tests_report_runner_null_file_sink },
                }
            },
            dt::block_spec{
                "options",
                "The face-portable option conveniences: opting into a PDF, "
                "naming its path, and selecting the split and show knobs "
                "through either option-set face.",
                {
                    { "tests_report_runner_use_pdf_selects_document",
                      "use_pdf switches the document knob to pdf",
                      &tests_report_runner_use_pdf_selects_document },
                    { "tests_report_runner_use_pdf_sets_path",
                      "use_pdf with a path also writes the output_file slot",
                      &tests_report_runner_use_pdf_sets_path },
                    { "tests_report_runner_use_pdf_empty_path_keeps_existing",
                      "use_pdf with no path leaves an already-set "
                      "output_file alone",
                      &tests_report_runner_use_pdf_empty_path_keeps_existing },
                    { "tests_report_runner_use_pdf_returns_self",
                      "use_pdf returns the builder itself, so calls chain",
                      &tests_report_runner_use_pdf_returns_self },
                    { "tests_report_runner_set_document",
                      "set_document reaches every test_doc_type value",
                      &tests_report_runner_set_document },
                    { "tests_report_runner_set_output_file",
                      "set_output_file writes the path the accessors read "
                      "back, including an empty one",
                      &tests_report_runner_set_output_file },
                    { "tests_report_runner_set_split",
                      "set_split moves between whole_run and per_module",
                      &tests_report_runner_set_split },
                    { "tests_report_runner_set_show",
                      "set_show reaches every test_show value",
                      &tests_report_runner_set_show },
                    { "tests_report_runner_options_survive_recording",
                      "recording modules and units does not disturb the "
                      "option set",
                      &tests_report_runner_options_survive_recording },
                }
            },
            dt::block_spec{
                "recording",
                "Accumulating the model: opening modules, running "
                "predicates, recording known results, and the multi-assertion "
                "open_unit / check / close_unit path with its no-op guards.",
                {
                    { "tests_report_runner_module_appends",
                      "module() appends to the report and makes the new "
                      "module current",
                      &tests_report_runner_module_appends },
                    { "tests_report_runner_module_returns_reference",
                      "module() returns a reference to the stored module, "
                      "which the builder then keeps writing to",
                      &tests_report_runner_module_returns_reference },
                    { "tests_report_runner_run_records_result",
                      "run() records a one-check unit named after the test",
                      &tests_report_runner_run_records_result },
                    { "tests_report_runner_run_returns_predicate_value",
                      "run() returns exactly what the predicate returned",
                      &tests_report_runner_run_returns_predicate_value },
                    { "tests_report_runner_run_null_predicate",
                      "a null predicate is recorded as a failure rather "
                      "than being called",
                      &tests_report_runner_run_null_predicate },
                    { "tests_report_runner_ensure_module_autocreates",
                      "recording without an open module creates the default "
                      "`tests` module first",
                      &tests_report_runner_ensure_module_autocreates },
                    { "tests_report_runner_unit_records_known_result",
                      "unit() records an already-decided result without "
                      "calling anything",
                      &tests_report_runner_unit_records_known_result },
                    { "tests_report_runner_open_unit_named",
                      "open_unit begins an empty multi-assertion unit",
                      &tests_report_runner_open_unit_named },
                    { "tests_report_runner_open_unit_described",
                      "the described overload also stores the unit's "
                      "descriptor",
                      &tests_report_runner_open_unit_described },
                    { "tests_report_runner_check_detail_overload",
                      "the four-argument check records expression, expected "
                      "and actual against the open unit",
                      &tests_report_runner_check_detail_overload },
                    { "tests_report_runner_check_named_overload",
                      "the three-argument check records an identifier and a "
                      "descriptor with no expected or actual",
                      &tests_report_runner_check_named_overload },
                    { "tests_report_runner_check_without_unit_is_noop",
                      "a check with a module open but no unit is silently "
                      "dropped",
                      &tests_report_runner_check_without_unit_is_noop },
                    { "tests_report_runner_check_without_module_is_noop",
                      "a check with nothing open at all is silently dropped "
                      "and creates no module",
                      &tests_report_runner_check_without_module_is_noop },
                    { "tests_report_runner_close_unit_without_unit_is_noop",
                      "close_unit with no unit open emits nothing and does "
                      "not fault",
                      &tests_report_runner_close_unit_without_unit_is_noop },
                    { "tests_report_runner_multiple_modules_isolated",
                      "units recorded after a second module() land in that "
                      "module, not the first",
                      &tests_report_runner_multiple_modules_isolated },
                }
            },
            dt::block_spec{
                "console",
                "The live console face: the module banner, per-unit result "
                "lines, the assertion lines, the per-module results box, the "
                "comprehensive roll-up and the overall assessment.",
                {
                    { "tests_report_runner_module_banner_text",
                      "opening a module emits its ruled banner naming the "
                      "module",
                      &tests_report_runner_module_banner_text },
                    { "tests_report_runner_module_banner_description",
                      "a module description adds a description line and a "
                      "further rule",
                      &tests_report_runner_module_banner_description },
                    { "tests_report_runner_unit_line_symbols",
                      "each unit emits one live result line carrying the "
                      "bracketed pass or fail label",
                      &tests_report_runner_unit_line_symbols },
                    { "tests_report_runner_open_unit_banner",
                      "open_unit emits its own testing banner",
                      &tests_report_runner_open_unit_banner },
                    { "tests_report_runner_check_detail_line",
                      "a detail check prints the expression with its "
                      "expected and actual values",
                      &tests_report_runner_check_detail_line },
                    { "tests_report_runner_check_named_line",
                      "a named check prints the identifier followed by its "
                      "descriptor",
                      &tests_report_runner_check_named_line },
                    { "tests_report_runner_close_unit_line",
                      "close_unit prints the unit's verdict line",
                      &tests_report_runner_close_unit_line },
                    { "tests_report_runner_module_results_box",
                      "finish prints a per-module box carrying the assertion "
                      "and unit tallies and the module status",
                      &tests_report_runner_module_results_box },
                    { "tests_report_runner_comprehensive_summary",
                      "finish prints the cross-module module, assertion and "
                      "unit-test summaries",
                      &tests_report_runner_comprehensive_summary },
                    { "tests_report_runner_assessment_pass",
                      "a clean run concludes with the all-passed assessment",
                      &tests_report_runner_assessment_pass },
                    { "tests_report_runner_assessment_fail",
                      "a run with any failure concludes with the two-line "
                      "attention-required assessment",
                      &tests_report_runner_assessment_fail },
                    { "tests_report_runner_rule_width",
                      "the horizontal rules are exactly the configured "
                      "column count wide",
                      &tests_report_runner_rule_width },
                    { "tests_report_runner_console_disabled_emits_nothing",
                      "a silenced builder writes not one byte to its sink "
                      "across a whole run",
                      &tests_report_runner_console_disabled_emits_nothing },
                }
            },
            dt::block_spec{
                "finish",
                "Closing a run: the summaries, the final tally and the "
                "process exit code the model's verdict decides.",
                {
                    { "tests_report_runner_finish_exit_code_pass",
                      "a run whose every unit passed returns zero",
                      &tests_report_runner_finish_exit_code_pass },
                    { "tests_report_runner_finish_exit_code_fail",
                      "a run carrying any failing unit returns non-zero",
                      &tests_report_runner_finish_exit_code_fail },
                    { "tests_report_runner_finish_empty_report",
                      "finishing a run that recorded nothing is safe and "
                      "reports the empty verdict as a non-pass",
                      &tests_report_runner_finish_empty_report },
                    { "tests_report_runner_finish_final_tally",
                      "the closing tally counts passed and failed UNITS "
                      "across the whole run",
                      &tests_report_runner_finish_final_tally },
                    { "tests_report_runner_finish_is_repeatable",
                      "calling finish twice repeats the summaries without "
                      "disturbing the model or the exit code",
                      &tests_report_runner_finish_is_repeatable },
                    { "tests_report_runner_finish_multi_module_boxes",
                      "one results box is printed per module, in the order "
                      "the modules were opened",
                      &tests_report_runner_finish_multi_module_boxes },
                    { "tests_report_runner_finish_empty_unit_is_forgiven",
                      "an opened but never-checked unit is not itself a "
                      "pass, yet the module verdict ignores it so the run "
                      "still finishes clean",
                      &tests_report_runner_finish_empty_unit_is_forgiven },
                }
            },
            dt::block_spec{
                "document",
                "PDF emission at finish(): the document knob's gate, the "
                "whole-run and per-module splits, output naming through the "
                "path slot and the file-name pattern, and write failures.",
                {
                    { "tests_report_runner_txt_writes_no_file",
                      "the default txt document writes no file at all - the "
                      "run stays console-only",
                      &tests_report_runner_txt_writes_no_file },
                    { "tests_report_runner_pdf_whole_run_writes_one",
                      "the whole_run split writes exactly one PDF covering "
                      "every module",
                      &tests_report_runner_pdf_whole_run_writes_one },
                    { "tests_report_runner_pdf_output_path_used",
                      "the output_file slot names the whole-run document",
                      &tests_report_runner_pdf_output_path_used },
                    { "tests_report_runner_pdf_empty_path_falls_back",
                      "an empty output_file falls back to the pattern with "
                      "the module name `report`",
                      &tests_report_runner_pdf_empty_path_falls_back },
                    { "tests_report_runner_pdf_per_module_writes_each",
                      "the per_module split writes one document per module, "
                      "each carrying only its own module",
                      &tests_report_runner_pdf_per_module_writes_each },
                    { "tests_report_runner_pdf_per_module_ignores_output_path",
                      "per_module naming comes from the pattern, so the "
                      "output_file slot is not consulted",
                      &tests_report_runner_pdf_per_module_ignores_output_path
                    },
                    { "tests_report_runner_pdf_file_name_pattern",
                      "the builder-local pattern expands the module, index "
                      "and extension placeholders",
                      &tests_report_runner_pdf_file_name_pattern },
                    { "tests_report_runner_pdf_write_failure_reports",
                      "an unwritable path yields no written file and the "
                      "not-written console note",
                      &tests_report_runner_pdf_write_failure_reports },
                    { "tests_report_runner_pdf_console_notes_path",
                      "each written document is announced on the console by "
                      "path",
                      &tests_report_runner_pdf_console_notes_path },
                }
            },
#if defined(D_TEST_REPORT_ENABLE_ARCHIVE)
            dt::block_spec{
                "archive",
                "The opt-in packaging arm: bundling every rendered document "
                "into one container, naming its entries, falling back on the "
                "container extension, and refusing unavailable writers.",
                {
                    { "tests_report_runner_archive_zip_bundles",
                      "archive mode writes one zip container instead of a "
                      "loose PDF, and the container holds the document",
                      &tests_report_runner_archive_zip_bundles },
                    { "tests_report_runner_archive_entry_basenames",
                      "an entry inside the container carries only the base "
                      "name, never a directory prefix",
                      &tests_report_runner_archive_entry_basenames },
                    { "tests_report_runner_archive_fallback_extension",
                      "an empty output_file names the container after the "
                      "selected format's extension",
                      &tests_report_runner_archive_fallback_extension },
                    { "tests_report_runner_archive_per_module_one_container",
                      "per_module plus archive bundles all N documents into "
                      "a single container",
                      &tests_report_runner_archive_per_module_one_container },
                    { "tests_report_runner_archive_rar_is_unavailable",
                      "rar creation is tool-only, so it writes nothing and "
                      "reports the document as not written",
                      &tests_report_runner_archive_rar_is_unavailable },
                    { "tests_report_runner_archive_pack_none_stays_loose",
                      "the none pack mode leaves the PDF loose even with "
                      "packaging compiled in",
                      &tests_report_runner_archive_pack_none_stays_loose },
                }
            },
#endif  // D_TEST_REPORT_ENABLE_ARCHIVE
            dt::block_spec{
                "convenience",
                "The free-function entry point and the assertion-capturing "
                "macro, together with the value rendering the EXPECTED and "
                "ACTUAL columns depend on.  run_named_tests builds its own "
                "builder and cannot be silenced, so these tests assert on its "
                "exit code alone and print a console report while they run.",
                {
                    { "tests_report_runner_run_named_tests_exit_code",
                      "run_named_tests returns zero only when every listed "
                      "predicate passed",
                      &tests_report_runner_run_named_tests_exit_code },
                    { "tests_report_runner_run_named_tests_records_all",
                      "every name in the table becomes a unit of the one "
                      "module it opens",
                      &tests_report_runner_run_named_tests_records_all },
                    { "tests_report_runner_run_named_tests_zero_count",
                      "a zero-length table still opens its module and "
                      "reports the empty run as a non-pass",
                      &tests_report_runner_run_named_tests_zero_count },
                    { "tests_report_runner_check_eq_records_detail",
                      "D_CHECK_EQ stringizes the expression and records "
                      "both rendered values",
                      &tests_report_runner_check_eq_records_detail },
                    { "tests_report_runner_check_eq_reports_mismatch",
                      "a mismatched comparison is recorded as a failure "
                      "carrying both values",
                      &tests_report_runner_check_eq_reports_mismatch },
                    { "tests_report_runner_check_eq_evaluates_once",
                      "each operand of D_CHECK_EQ is evaluated exactly once",
                      &tests_report_runner_check_eq_evaluates_once },
                    { "tests_report_runner_to_report_string_strings",
                      "std::string passes through and a null C-string "
                      "renders empty rather than faulting",
                      &tests_report_runner_to_report_string_strings },
                    { "tests_report_runner_to_report_string_bool",
                      "a bool renders as the words true and false",
                      &tests_report_runner_to_report_string_bool },
                    { "tests_report_runner_to_report_string_generic",
                      "any other type is rendered through its stream "
                      "insertion operator",
                      &tests_report_runner_to_report_string_generic },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_TEST_REPORT_RUNNER_TESTS_
