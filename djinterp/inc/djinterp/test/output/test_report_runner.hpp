/******************************************************************************
* djinterp [test]                                       test_report_runner.hpp
*
*   The runner-facing front end for the report subsystem.  A report_builder
* collects a run into a test_report (test_report.hpp) WHILE printing the
* familiar live console output, then - at finish() - renders the configured
* document(s) through the matching emitter (test_pdf_report.hpp for PDF) and
* returns the process exit code.  It is what a hand-written *_tests_runner.cpp
* drives instead of a bespoke pass/fail loop: open a module, run named boolean
* tests, finish.
*
*   WHAT IT REPLACES:
*   The standalone runners each carried their own counters, their own printf
* of "[PASS]/[FAIL] name" lines, and their own "passed: X failed: Y" tally.
* report_builder folds all of that into one object that ALSO accumulates the
* structured report the PDF (and any future emitter) needs - so the console
* and the document come from a single source and never disagree.
*
*   LIVE CONSOLE vs DOCUMENT:
*   As each test runs, its result line is emitted immediately (the run-as-you-
* watch behavior the old runners had).  The per-module results box, the
* cross-module comprehensive summary, and the overall assessment are printed
* by finish(), which then writes the document(s).  The console layout mirrors
* the report sample so the text and PDF faces read alike.
*
*   OPTIONS:
*   The builder owns a test_option_set (default_test_options()).  Set
* `document` to test_doc_type::pdf to have finish() emit a PDF; tune
* split (whole_run vs per_module) / show / output_file via the option set
* exactly as documented in test_options.hpp, and the report metadata
* (title / subtitle / author) via the set_title()-family helpers below. The
* per-file NAME is a builder-local pattern (set_file_name_pattern(), folded
* through test_report.hpp's expand_report_file_name) rather than an option
* slot - test_options.hpp carries no file_name_pattern slot today; see that
* header's "FILE NAMING" note. With the default `text` document no file is
* written - the run is console-only - so a runner opts into PDF explicitly.
*
*   PDF RENDERING:
*   finish()/emit_document() render through test_render_pdf.hpp's CURRENT
* entry points (render_report_pdf_bytes / render_module_pdf_bytes +
* pdf_default_layout), not a save_pdf_report() free function - that symbol
* never existed; this builder predates test_render_pdf.hpp and was updated to
* call it directly.
*
*   OUTPUT PACKAGING (opt-in):
*   The `archive` pack mode IS wired for the PDF path, but only when the caller
* defines D_TEST_REPORT_ENABLE_ARCHIVE before including this header.  Enabled,
* write_pdf_report() renders every document (whole_run -> 1, per_module -> N),
* then, when `pack` is `archive`, bundles them into ONE container - the
* `archive_format` (e.g. sevenzip -> .7z), tuned by `archive_opts`, written to
* the `output_file` path - via the archive facade (archive.hpp) directly.
* Rather than test_output_config.hpp's to_output_config() + a document_bundle
* writer (a heavier dependency this lightweight builder still avoids), it drives
* the facade's in-memory try_archive<> leaf, which matches the write-bytes-
* verbatim model.  The `compress` pack mode is not routed yet.  Left undefined
* (the default), packaging is inert and the builder has NO archive dependency -
* a console-only or single-PDF runner pays nothing for it.
*
*   ADDITIONAL TEST_PRINTER SUPPORT:
*   The report-style console presets and configure_report_printer() helper
* this layer relies on live in test_printer.hpp; a caller wanting a
* test_printer-driven console (rather than report_builder's built-in
* emission) configures one with that helper and walks a module's units.
*
*   PORTABILITY:
*   The builder core is C++11 (console + report accumulation).  PDF emission
* rides the C++17 pdf gate: below C++17 finish() simply skips the document and
* the run stays console-only.  No third-party dependency.
*
*
* TABLE OF CONTENTS
* =================
* I.    REPORT BUILDER
*       a. construction / configuration
*       b. module / unit recording (live console)
*       c. finish (summaries + document + exit code)
*       d. internal: console rendering
*       e. internal: helpers
* II.   FREE-FUNCTION CONVENIENCE
*
*
* path:      /inc/djinterp/test/test_report_runner.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.26
******************************************************************************/

#ifndef DJINTERP_TEST_REPORT_RUNNER_
#define DJINTERP_TEST_REPORT_RUNNER_ 1

#ifndef __cplusplus
    #error "test_report_runner.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/container/buffer/byte_buffer.hpp"
#include "../test_common.hpp"      // test_status
#include "../test_context.hpp"     // test_context + at_run (PDF focus)
#include "../test_options.hpp"     // test_option_set + accessors + defaults
#include "./test_report.hpp"       // test_report model + tallies +
                                   //   expand_report_file_name
#include "./test_report_pdf.hpp"      // render_report_pdf_bytes_table -- the
                                       //   banded/ruled PDF face, now composed
                                       //   via test_report_document + the
                                       //   canvas-backed document_renderer

// Output packaging is OPT-IN: define D_TEST_REPORT_ENABLE_ARCHIVE before
// including this header to pull in the archive facade and have write_pdf_report()
// honor the `pack` / `archive_format` / `archive_opts` slots (the .7z path).
// Left undefined (the default), the builder compiles exactly as before, with no
// dependency on the archive backend - so a console-only or single-PDF runner
// pays nothing for a feature it does not use.  See PDF RENDERING in the header
// banner: the archive path.  The document_bundle route now exists as
// test_packaging.hpp (emit_report_to_disk / _to_buffer), which adds a naming
// policy, both pack modes and a sink abstraction; prefer it for new work.  The
// direct facade calls below remain because zip_store_archive is the fallback
// when no backend zip writer is built in -- a capability output_packaging does
// not have, since it dispatches to the facade rather than around it.
#if defined(D_TEST_REPORT_ENABLE_ARCHIVE) && D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include "../../core/util/archive.hpp"   // entry, entry_list, try_archive<>,
                                             //   formats::*, archive_options,
                                             //   format_is_writable<>
    #include "./test_zip_store.hpp"          // zip_store_archive - a dependency-
                                             //   free stored (uncompressed) ZIP
                                             //   writer used as the fallback when
                                             //   no backend zip writer is built in
#endif


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_report_runner.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                I.   REPORT BUILDER                                       ///
///////////////////////////////////////////////////////////////////////////////

#ifndef D_TEST_REPORT_RULE_COLS
    #define D_TEST_REPORT_RULE_COLS   80
#endif


// test_predicate_fn
//   type: the signature of a boolean unit test the builder runs - a niladic
// predicate returning true on pass.  Matches the existing runners' tests.
typedef bool (*test_predicate_fn)();


// report_builder
//   class: accumulates a run into a test_report while emitting the live
// console report, then renders the configured document(s) at finish().
//
// Usage:
//   report_builder rb;
//   rb.set_title("djinterp Framework Test Suite");
//   rb.options().set<test_option::document>(test_doc_type::pdf);   // (C++20)
//
//   rb.module("test_object", "Unified test object");
//   rb.run("default ctor is pending", &tests_default_ctor);
//   rb.run("evaluate sets status",     &tests_evaluate);
//
//   return rb.finish();    // prints summary, writes the PDF, returns exit code
class report_builder
{
public:
    using size_type = std::size_t;

    // =================================================================
    //  a. construction / configuration
    // =================================================================

    // report_builder
    //   constructor: a builder with the framework-default options, the
    // console enabled, and output directed at stdout.
    report_builder()
        : m_report(),
          m_opts(default_test_options()),
          m_module_index(0),
          m_have_module(false),
          m_console(true),
          m_file(stdout),
          m_file_name_pattern()
    {}

    // report_builder
    //   constructor: a builder seeded with a caller-supplied option set.
    explicit report_builder(
        test_option_set _opts
    )
        : m_report(),
          m_opts(static_cast<test_option_set&&>(_opts)),
          m_module_index(0),
          m_have_module(false),
          m_console(true),
          m_file(stdout),
          m_file_name_pattern()
    {}

    // options
    //   the option set, mutable so a runner can set document / layout /
    // parts / naming before finishing.
    test_option_set&       options()       D_NOEXCEPT { return m_opts; }
    const test_option_set& options() const D_NOEXCEPT { return m_opts; }

    // report
    //   the accumulating report (mutable for advanced population).
    test_report&       report()       D_NOEXCEPT { return m_report; }
    const test_report& report() const D_NOEXCEPT { return m_report; }

    // report metadata convenience
    void set_title(const std::string& _t)    { m_report.title = _t;    return; }
    void set_subtitle(const std::string& _t) { m_report.subtitle = _t; return; }
    void set_author(const std::string& _t)   { m_report.author = _t;   return; }
    void set_description(const std::string& _t) { m_report.description = _t; return; }
    void set_notes(const std::string& _t)    { m_report.notes = _t;    return; }

    // console control
    void set_console(bool _on) D_NOEXCEPT { m_console = _on;  return; }
    void set_file(std::FILE* _f) D_NOEXCEPT { m_file = _f;    return; }

    // -----------------------------------------------------------------
    //  option conveniences (face-portable)
    //
    //   Setting a knob differs by face - get<>/set<> on the C++20
    // option_set, member assignment on the pre-C++20 struct - so these
    // wrappers branch ONCE here and the runner stays face-agnostic.
    // -----------------------------------------------------------------

    // use_pdf
    //   selects the PDF document and, when given, its output path.  The
    // one call a runner makes to opt into a PDF beside its console output.
    report_builder&
    use_pdf(
        const std::string& _output_file = std::string()
    )
    {
        set_document(test_doc_type::pdf);

        if (!_output_file.empty())
        {
            set_output_file(_output_file);
        }

        return *this;
    }

    // set_document
    void
    set_document(
        test_doc_type _v
    )
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        m_opts.set<test_option::document>(_v);
#else
        m_opts.document = _v;
#endif
        return;
    }

    // set_output_file
    void
    set_output_file(
        const std::string& _v
    )
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        m_opts.set<test_option::output_file>(_v);
#else
        m_opts.output_path = _v;
#endif
        return;
    }

    // set_split
    //   selects whole_run (one document for the whole report) vs per_module
    // (one document per module).  This is what the older draft of this
    // builder called "module_layout" - test_module_layout never
    // existed; test_options.hpp's actual slot for this is `split` /
    // test_output_split (see that header's test_output_split doc comment).
    void
    set_split(
        test_output_split _v
    )
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        m_opts.set<test_option::split>(_v);
#else
        m_opts.split = _v;
#endif
        return;
    }

    // set_show
    void
    set_show(
        test_show _v
    )
    {
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        m_opts.set<test_option::show>(_v);
#else
        m_opts.show = _v;
#endif
        return;
    }

    // set_file_name_pattern
    //   the per-document filename pattern, folded through
    // test_report.hpp's expand_report_file_name() at emit_document() time
    // ({module} / {index} / {title} / {date} / {time} / {ext}).  Builder-
    // local rather than an option slot - test_options.hpp does not carry a
    // file_name_pattern slot (see its "FILE NAMING" note); an empty pattern
    // falls back to "{module}.{ext}".
    void
    set_file_name_pattern(
        const std::string& _v
    )
    {
        m_file_name_pattern = _v;
        return;
    }


    // =================================================================
    //  b. module / unit recording (live console)
    // =================================================================

    // module
    //   opens a new module, emits its console banner, and makes it the
    // current module for subsequent run() / check() calls.  Returns a
    // reference to the stored module (valid until the next module()).
    report_module&
    module(
        const std::string& _name,
        const std::string& _description = std::string()
    )
    {
        m_report.modules.push_back(report_module(_name, _description));
        m_module_index = m_report.modules.size() - 1;
        m_have_module  = true;

        emit_module_banner(_name, _description);

        return m_report.modules[m_module_index];
    }

    // run
    //   runs a boolean unit test, records it as a one-check unit in the
    // current module, and emits its result line immediately.  If no module
    // has been opened, a default module is created first.
    bool
    run(
        const std::string& _name,
        test_predicate_fn  _fn
    )
    {
        ensure_module();

        const bool ok = (_fn != nullptr) ? _fn() : false;

        m_report.modules[m_module_index].add_result_unit(_name, ok);

        emit_unit_line(_name, ok);

        return ok;
    }

    // unit
    //   records a unit whose pass/fail is already known (no predicate),
    // emitting its result line.  The boolean counterpart of run().
    void
    unit(
        const std::string& _name,
        bool               _ok
    )
    {
        ensure_module();

        m_report.modules[m_module_index].add_result_unit(_name, _ok);

        emit_unit_line(_name, _ok);

        return;
    }

    // open_unit
    //   begins a multi-assertion unit in the current module and emits its
    // band header.  Follow with check() calls and close_unit().  For rich
    // tests that report several assertions under one named unit.
    void
    open_unit(
        const std::string& _name
    )
    {
        ensure_module();

        m_report.modules[m_module_index].units.push_back(report_unit(_name));

        if (m_console)
        {
            emit(std::string("  --- Testing ") + _name + " ---\n");
        }

        return;
    }

    // open_unit (described)
    //   begins a unit that also carries a descriptor - e.g. a spec block's
    // "descriptor" - rendered as the test card's description line.  Follow with
    // check() calls and close_unit().
    void
    open_unit(
        const std::string& _name,
        const std::string& _description
    )
    {
        ensure_module();

        m_report.modules[m_module_index].units.push_back(
            report_unit(_name, _description));

        if (m_console)
        {
            emit(std::string("  --- Testing ") + _name + " ---\n");
        }

        return;
    }

    // check (detail overload)
    //   records an assertion carrying its expression text and the expected /
    // actual values, populating the EXPECTED / ACTUAL columns of the PDF table.
    // No-op if no unit is open.  Prefer the D_CHECK_EQ macro, which captures
    // all three from a single comparison.
    void
    check(
        const std::string& _expression,
        const std::string& _expected,
        const std::string& _actual,
        bool               _ok
    )
    {
        if ( (!m_have_module) ||
             (m_report.modules[m_module_index].units.empty()) )
        {
            return;
        }

        report_unit& u =
            m_report.modules[m_module_index].units.back();

        u.add_result(_expression, _expected, _actual, _ok);

        if (m_console)
        {
            emit(std::string("    ") + symbol(_ok) + " " + _expression +
                 "  (expected " + _expected + ", got " + _actual + ")\n");
        }

        return;
    }

    // check (named-test overload)
    //   records a result carrying an identifier (_expression, e.g. a spec
    // test's "name") AND a prose descriptor (_description, e.g. its
    // "descriptor"), with no expected/actual.  The PDF table renders such rows
    // in test-level form: RESULT / # / TEST (the identifier) / DESCRIPTION.
    // No-op if no unit is open.
    void
    check(
        const std::string& _expression,
        const std::string& _description,
        bool               _ok
    )
    {
        if ( (!m_have_module) ||
             (m_report.modules[m_module_index].units.empty()) )
        {
            return;
        }

        report_unit& u =
            m_report.modules[m_module_index].units.back();

        u.add_check(_description,
                    _ok ? test_status::passed : test_status::failed,
                    _expression,
                    std::string(),
                    std::string());

        if (m_console)
        {
            emit(std::string("    ") + symbol(_ok) + " " + _expression +
                 "  " + _description + "\n");
        }

        return;
    }

    // close_unit
    //   emits the closing verdict line for the current unit.
    void
    close_unit()
    {
        if ( (!m_have_module) ||
             (m_report.modules[m_module_index].units.empty()) )
        {
            return;
        }

        const report_unit& u =
            m_report.modules[m_module_index].units.back();

        if (m_console)
        {
            emit(std::string("    ") + symbol(u.passed()) + " " + u.name +
                 " unit test " + (u.passed() ? "passed" : "failed") + "\n");
        }

        return;
    }


    // =================================================================
    //  c. finish
    // =================================================================

    // finish
    //   emits the per-module results box for every module, the cross-module
    // comprehensive summary and overall assessment, then writes the
    // configured document(s) and a final pass/fail tally.  Returns the
    // process exit code (0 iff the whole run passed).
    int
    finish()
    {
        std::size_t i = 0;

        // per-module results boxes
        for (i = 0; i < m_report.modules.size(); ++i)
        {
            emit_module_results(m_report.modules[i]);
        }

        // cross-module comprehensive summary + assessment
        emit_comprehensive();
        emit_assessment();

        // document emission (PDF rides the C++17 pdf gate)
        emit_document();

        // final tally (unit tests across the run)
        if (m_console)
        {
            emit(std::string("\npassed: ") +
                 size_str(m_report.passed_units()) + "   failed: " +
                 size_str(m_report.failed_units()) + "\n");
        }

        return m_report.exit_code();
    }

private:
    // =================================================================
    //  d. internal: console rendering
    // =================================================================

    // emit_module_banner
    //   the heavy banner that opens a module's console section.
    void
    emit_module_banner(
        const std::string& _name,
        const std::string& _description
    )
    {
        if (!m_console)
        {
            return;
        }

        emit("\n");
        emit(rule('=') + "\n");
        emit(std::string("TESTING MODULE: ") + _name + "\n");
        emit(rule('=') + "\n");

        if (!_description.empty())
        {
            emit(std::string("description: ") + _description + "\n");
            emit(rule('=') + "\n");
        }

        emit("\n");

        return;
    }

    // emit_unit_line
    //   one live result line for a unit test.
    void
    emit_unit_line(
        const std::string& _name,
        bool               _ok
    )
    {
        if (!m_console)
        {
            return;
        }

        emit(std::string("  ") + symbol(_ok) + " " + _name + "\n");

        return;
    }

    // emit_module_results
    //   the per-module results box (assertions / unit tests / status).
    void
    emit_module_results(
        const report_module& _module
    )
    {
        if (!m_console)
        {
            return;
        }

        const std::size_t a_pass = _module.passed_checks();
        const std::size_t a_tot  = _module.total_checks();
        const std::size_t u_pass = _module.passed_units();
        const std::size_t u_tot  = _module.total_units();
        const bool        ok     = _module.passed();

        emit("\n");
        emit(rule('-') + "\n");
        emit(std::string("MODULE RESULTS: ") + _module.name + "\n");
        emit(rule('-') + "\n");
        emit(std::string("Assertions: ") + report_ratio(a_pass, a_tot) +
             " passed (" + report_pass_rate(a_pass, a_tot) + ")\n");
        emit(std::string("Unit Tests: ") + report_ratio(u_pass, u_tot) +
             " passed (" + report_pass_rate(u_pass, u_tot) + ")\n");
        emit(std::string("Status: ") + symbol(ok) + " " + _module.name +
             " MODULE " + (ok ? "PASSED" : "FAILED") + "\n");
        emit(rule('-') + "\n");

        return;
    }

    // emit_comprehensive
    //   the cross-module roll-up.
    void
    emit_comprehensive()
    {
        if (!m_console)
        {
            return;
        }

        const std::size_t m_tot  = m_report.total_modules();
        const std::size_t m_pass = m_report.passed_modules();
        const std::size_t a_tot  = m_report.total_checks();
        const std::size_t a_pass = m_report.passed_checks();
        const std::size_t a_fail = m_report.failed_checks();
        const std::size_t u_tot  = m_report.total_units();
        const std::size_t u_pass = m_report.passed_units();
        const std::size_t u_fail = m_report.failed_units();

        emit("\n");
        emit(rule('=') + "\n");
        emit("COMPREHENSIVE TEST RESULTS\n");
        emit(rule('=') + "\n");

        emit("MODULE SUMMARY:\n");
        emit(std::string("  Modules Tested: ") + size_str(m_tot) + "\n");
        emit(std::string("  Modules Passed: ") + size_str(m_pass) + "\n");
        emit(std::string("  Module Success Rate: ") +
             report_pass_rate(m_pass, m_tot) + "\n\n");

        emit("ASSERTION SUMMARY:\n");
        emit(std::string("  Total Assertions: ") + size_str(a_tot) + "\n");
        emit(std::string("  Assertions Passed: ") + size_str(a_pass) + "\n");
        emit(std::string("  Assertions Failed: ") + size_str(a_fail) + "\n");
        emit(std::string("  Assertion Success Rate: ") +
             report_pass_rate(a_pass, a_tot) + "\n\n");

        emit("UNIT TEST SUMMARY:\n");
        emit(std::string("  Total Unit Tests: ") + size_str(u_tot) + "\n");
        emit(std::string("  Unit Tests Passed: ") + size_str(u_pass) + "\n");
        emit(std::string("  Unit Tests Failed: ") + size_str(u_fail) + "\n");
        emit(std::string("  Unit Test Success Rate: ") +
             report_pass_rate(u_pass, u_tot) + "\n");
        emit(rule('=') + "\n");

        return;
    }

    // emit_assessment
    //   the overall PASS / FAIL conclusion.
    void
    emit_assessment()
    {
        if (!m_console)
        {
            return;
        }

        const bool ok = m_report.passed();

        emit("\n");
        emit(rule('=') + "\n");
        emit("OVERALL ASSESSMENT:\n");

        if (ok)
        {
            emit("  [PASS] ALL TESTS PASSED\n");
        }
        else
        {
            emit("  [FAIL] SOME TESTS FAILED - ATTENTION REQUIRED\n");
            emit("  [FAIL] Review failed tests before proceeding\n");
        }

        emit(rule('=') + "\n");

        return;
    }

    // emit_document
    //   writes the configured document(s).  PDF emission is compiled only
    // on the C++17 pdf gate; below it the run stays console-only.
    void
    emit_document()
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if (document(m_opts) != test_doc_type::pdf)
        {
            return;
        }

        std::vector<std::string> files = write_pdf_report();

        std::size_t i = 0;

        if (m_console)
        {
            for (i = 0; i < files.size(); ++i)
            {
                emit(std::string("\nwrote report: ") + files[i] + "\n");
            }

            if ( files.empty() &&
                 (show(m_opts) != test_show::silent) )
            {
                emit("\n(report document was not written)\n");
            }
        }
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

        return;
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // pending_doc
    //   internal: one rendered document awaiting output - its bytes plus the
    // name it takes as a loose file (per_module / whole_run) or, in archive
    // mode, as the entry inside the container.
    struct pending_doc
    {
        std::string name;
        std::string bytes;
    };

    // write_pdf_report
    //   renders the accumulated report as PDF and writes it to disk, honoring
    // `split` (one document for the whole run, or one per module) AND, when
    // packaging is compiled in (D_TEST_REPORT_ENABLE_ARCHIVE) and `pack` is
    // `archive`, bundling every rendered document into ONE container instead of
    // writing them loose.  The actual render entry points live in
    // test_render_pdf.hpp (render_report_pdf_bytes / render_module_pdf_bytes
    // against pdf_default_layout()).
    //
    //   The split x pack matrix (test_output_split doc in test_options.hpp):
    //     whole_run + none    -> 1 loose PDF          (output_file, or "report")
    //     per_module + none   -> N loose PDFs         ({module}.pdf each)
    //     whole_run + archive -> 1 PDF in 1 container (output_file names it)
    //     per_module + archive-> N PDFs in 1 container(output_file names it)
    //   The `compress` pack mode is not routed here yet (it needs the codec
    // facade); only `archive` is, since that is the .7z path.  With packaging
    // NOT compiled in, `pack` is inert and bytes are written loose, exactly as
    // before.
    //
    // Return:
    //   the paths actually written (empty if nothing was written, e.g. every
    // path resolved empty, the archive writer was unavailable, or every
    // fopen() failed).
    std::vector<std::string>
    write_pdf_report()
    {
        std::vector<std::string> written;

        // 1) render the split's document(s) into (name, bytes) pairs.
        std::vector<pending_doc> docs;

        if (split(m_opts) == test_output_split::per_module)
        {
            std::size_t mi = 0;

            for (mi = 0; mi < m_report.modules.size(); ++mi)
            {
                // a single-module report carrying the run's cover metadata,
                // rendered through the same real-table renderer as the whole run.
                test_report one;
                one.title    = m_report.title;
                one.subtitle = m_report.subtitle;
                one.author   = m_report.author;
                one.modules.push_back(m_report.modules[mi]);

                pending_doc d;
                d.bytes = render_report_pdf_bytes_table(one);
                d.name  = expand_report_file_name(
                    m_file_name_pattern,
                    m_report.modules[mi].name,
                    mi,
                    m_report.title,
                    "pdf");

                docs.push_back(static_cast<pending_doc&&>(d));
            }
        }
        else
        {
            pending_doc d;
            d.bytes = render_report_pdf_bytes_table(m_report);
            d.name  = output_path(m_opts);

            if (d.name.empty())
            {
                d.name = expand_report_file_name(m_file_name_pattern,
                                                 std::string("report"),
                                                 0,
                                                 m_report.title,
                                                 "pdf");
            }

            docs.push_back(static_cast<pending_doc&&>(d));
        }

        // 2) archive mode (opt-in): bundle every document into ONE container.
#if defined(D_TEST_REPORT_ENABLE_ARCHIVE)
        if (pack(m_opts) == test_output_pack::archive)
        {
            return write_archived_report(docs);
        }
#endif

        // 3) loose mode (the default): one file per rendered document.
        std::size_t i = 0;

        for (i = 0; i < docs.size(); ++i)
        {
            if (write_bytes_to_file(docs[i].name, docs[i].bytes))
            {
                written.push_back(docs[i].name);
            }
        }

        return written;
    }

    // write_bytes_to_file
    //   internal: a raw byte_blob -> file write.  Returns false (and writes
    // nothing) on an empty path or a failed fopen()/short write.
    static bool
    write_bytes_to_file(
        const std::string& _path,
        const std::string& _bytes
    )
    {
        if (_path.empty())
        {
            return false;
        }

        std::FILE* f = std::fopen(_path.c_str(), "wb");

        if (f == nullptr)
        {
            return false;
        }

        const std::size_t n = std::fwrite(_bytes.data(), 1, _bytes.size(), f);

        std::fclose(f);

        return n == _bytes.size();
    }


#if defined(D_TEST_REPORT_ENABLE_ARCHIVE)

    // write_archived_report
    //   bundles every rendered document into a SINGLE container - the format
    // from `archive_format`, tuned by `archive_opts` - and writes it to the
    // `output_file` path (a fallback "report.<ext>" is used when that slot is
    // empty).  Each document keeps the name computed above (basename only) as
    // its entry name inside the archive.  Rendering + archiving happen entirely
    // in memory (byte_blob is std::string; the archive facade builds the
    // container as a byte_blob), matching the builder's write-bytes-verbatim
    // model.
    //
    // Return:
    //   the archive path in a one-element vector, or empty when the selected
    // format's writer is not built into this environment, archive creation did
    // not succeed, or the file could not be written.
    std::vector<std::string>
    write_archived_report(
        const std::vector<pending_doc>& _docs
    )
    {
        std::vector<std::string> written;
        ::djinterp::entry_list   entries;
        std::size_t              i = 0;

        for (i = 0; i < _docs.size(); ++i)
        {
            ::djinterp::entry e;
            e.name = base_name_of(_docs[i].name);   // path within the archive
            e.data = _docs[i].bytes;                 // byte_blob == std::string
            entries.push_back(e);
        }

        ::djinterp::byte_blob blob;

        if (!try_build_archive(archive_format(m_opts),
                               entries,
                               archive_opts(m_opts),
                               blob))
        {
            return written;                          // unavailable / failed
        }

        std::string path = output_path(m_opts);

        if (path.empty())
        {
            path = std::string("report.") +
                   archive_extension(archive_format(m_opts));
        }

        if (write_bytes_to_file(path, blob))
        {
            written.push_back(path);
        }

        return written;
    }

    // try_build_archive
    //   runtime-dispatches the selected archive format to the matching
    // try_archive<> tag (the format tag is a TYPE, so the runtime enum is
    // switched here).  Guards each arm with format_is_writable<> so an
    // unavailable backend fails cleanly rather than emitting a broken file.
    // RAR creation is tool-only and is reported unavailable.
    //
    // Return:
    //   true iff the archive was built into _out; false (writing nothing)
    // otherwise.
    static bool
    try_build_archive(
        test_archive_format                _fmt,
        const ::djinterp::entry_list&      _entries,
        const ::djinterp::archive_options& _aopts,
        ::djinterp::byte_blob&           _out
    )
    {
        using namespace ::djinterp;
        using namespace ::djinterp::formats;

        status s = status_unavailable;

        switch (_fmt)
        {
            case test_archive_format::zip:
            {
                // Always use the self-contained STORED writer for the report
                // bundle.  It computes its own CRC-32 (no zlib required) and so
                // produces a container that every ZIP reader accepts.
                //
                // We deliberately do NOT defer to the framework's built-in
                // zip_create here even though format_is_writable<zip>() reports
                // it usable: that writer derives the CRC from zlib, and when
                // zlib is absent from the build (D_ENV_COMPRESSION_HAVE_ZLIB
                // == 0) it emits a CRC of 0 for every entry.  The data is intact
                // but strict readers (WinRAR, among others) recompute the CRC,
                // see the stored 0, and reject the archive as corrupt.  The
                // self-contained writer avoids that dependency entirely.
                (void)_aopts;   // stored writer takes no codec options
                return zip_store_archive(_entries, _out);
            }
            case test_archive_format::tar:
            {
                if (!format_is_writable<tar>())      { return false; }
                s = try_archive<tar>(_entries, _out, _aopts);
                break;
            }
            case test_archive_format::tar_gz:
            {
                if (!format_is_writable<tar_gz>())   { return false; }
                s = try_archive<tar_gz>(_entries, _out, _aopts);
                break;
            }
            case test_archive_format::gz:
            {
                if (!format_is_writable<gz>())       { return false; }
                s = try_archive<gz>(_entries, _out, _aopts);
                break;
            }
            case test_archive_format::sevenzip:
            {
                if (!format_is_writable<sevenzip>()) { return false; }
                s = try_archive<sevenzip>(_entries, _out, _aopts);
                break;
            }
            case test_archive_format::rar:
            default:
            {
                return false;   // RAR creation is tool-only; unknown -> none
            }
        }

        return s == status_ok;
    }

    // archive_extension
    //   the conventional file extension for a fallback archive name.
    static const char*
    archive_extension(
        test_archive_format _fmt
    )
    {
        switch (_fmt)
        {
            case test_archive_format::zip:      { return "zip";    }
            case test_archive_format::tar:      { return "tar";    }
            case test_archive_format::tar_gz:   { return "tar.gz"; }
            case test_archive_format::gz:       { return "gz";     }
            case test_archive_format::sevenzip: { return "7z";     }
            case test_archive_format::rar:      { return "rar";    }
            default:                            { return "bin";    }
        }
    }

    // base_name_of
    //   the final path component of _p ('/' or '\\' separated), so archive
    // entry names never carry a directory prefix.
    static std::string
    base_name_of(
        const std::string& _p
    )
    {
        const std::string::size_type slash = _p.find_last_of("/\\");

        return (slash == std::string::npos) ? _p : _p.substr(slash + 1);
    }

#endif  // D_TEST_REPORT_ENABLE_ARCHIVE

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


    // =================================================================
    //  e. internal: helpers
    // =================================================================

    // ensure_module
    //   opens a default module if none is open, so run() / unit() are
    // usable without an explicit module() call.
    void
    ensure_module()
    {
        if (!m_have_module)
        {
            module("tests", std::string());
        }

        return;
    }

    // emit
    //   writes _text to the console sink when the console is enabled.
    void
    emit(
        const std::string& _text
    ) const
    {
        if ( m_console &&
             (m_file != nullptr) )
        {
            std::fwrite(_text.data(), 1, _text.size(), m_file);
        }

        return;
    }

    // symbol
    //   the bracketed pass / fail label.
    static const char*
    symbol(
        bool _ok
    ) D_NOEXCEPT
    {
        return _ok ? "[PASS]" : "[FAIL]";
    }

    // rule
    //   a horizontal separator of D_TEST_REPORT_RULE_COLS copies of _c.
    static std::string
    rule(
        char _c
    )
    {
        return std::string(static_cast<std::size_t>(D_TEST_REPORT_RULE_COLS),
                           _c);
    }

    // size_str
    //   decimal stringification of a size_t.
    static std::string
    size_str(
        std::size_t _v
    )
    {
        char buf[32];

        std::snprintf(buf, sizeof(buf), "%zu", _v);

        return std::string(buf);
    }


    // =================================================================
    //  storage
    // =================================================================

    test_report     m_report;
    test_option_set m_opts;
    size_type       m_module_index;
    bool            m_have_module;
    bool            m_console;
    std::FILE*      m_file;
    std::string     m_file_name_pattern;  // see set_file_name_pattern()
};


///////////////////////////////////////////////////////////////////////////////
///                II.  FREE-FUNCTION CONVENIENCE                            ///
///////////////////////////////////////////////////////////////////////////////

// run_named_tests
//   function: run a contiguous array of (name, predicate) pairs as the units
// of one module, emitting the live console report and (per the options) the
// document, and return the exit code.  A compact path for a runner that just
// has a table of tests.
//
// Parameter(s):
//   _module:      the module name.
//   _description: the module description (may be empty).
//   _names:       the test names, _count entries.
//   _fns:         the matching predicates, _count entries.
//   _count:       the number of tests.
//   _opts:        the option set (document / layout / naming / metadata).
// Return:
//   the process exit code (0 iff every test passed).
D_INLINE int
run_named_tests(
    const std::string&       _module,
    const std::string&       _description,
    const std::string*       _names,
    const test_predicate_fn* _fns,
    std::size_t              _count,
    test_option_set          _opts
)
{
    report_builder rb(static_cast<test_option_set&&>(_opts));
    std::size_t    i = 0;

    rb.module(_module, _description);

    for (i = 0; i < _count; ++i)
    {
        rb.run(_names[i], _fns[i]);
    }

    return rb.finish();
}


NS_END  // test
NS_END  // djinterp


// ============================================================================
//  D_CHECK_EQ - capture an equality assertion's expression + expected + actual
// ============================================================================

NS_DJINTERP
NS_TEST

namespace report_detail
{
    // to_report_string
    //   render a value for the EXPECTED / ACTUAL columns.  std::string and
    // C-strings pass through, bool prints true / false, and everything else is
    // formatted through operator<< on a stringstream.
    inline std::string to_report_string(const std::string& _v) { return _v; }
    inline std::string to_report_string(const char* _v)        { return std::string(_v ? _v : ""); }
    inline std::string to_report_string(bool _v)               { return _v ? "true" : "false"; }

    template <typename _T>
    inline std::string
    to_report_string(
        const _T& _v
    )
    {
        std::ostringstream _os;

        _os << _v;

        return _os.str();
    }
}   // namespace report_detail

NS_END  // test
NS_END  // djinterp


// D_CHECK_EQ
//   records "_expr == _expected" against the report_builder _rb: the stringized
// expression becomes the ASSERTION cell, to_report_string(_expected) the
// EXPECTED cell, to_report_string(_expr) the ACTUAL cell, and the pass flag is
// (actual == expected).  Each operand is evaluated exactly once.
//
//   report_builder rb;  rb.module("Suite");  rb.open_unit("Case");
//   D_CHECK_EQ(rb, tokens.size(), 4u);
//   rb.close_unit();
#define D_CHECK_EQ(_rb, _expr, _expected)                                        \
    do                                                                           \
    {                                                                            \
        const auto _dj_actual_val   = (_expr);                                   \
        const auto _dj_expected_val = (_expected);                               \
        (_rb).check(                                                             \
            #_expr,                                                              \
            ::djinterp::test::report_detail::to_report_string(_dj_expected_val), \
            ::djinterp::test::report_detail::to_report_string(_dj_actual_val),   \
            (_dj_actual_val == _dj_expected_val));                               \
    }                                                                            \
    while (false)


#endif  // DJINTERP_TEST_REPORT_RUNNER_