/******************************************************************************
* djinterp [test]                                      test_suite_printer.hpp
*
*   Suite-level printer for the DTest framework master runner.  Wraps
* text_template instances with conditional section blocks to produce
* the comprehensive results, final status, and per-module result
* banners that bracket a full test suite run.
*
*   PROBLEM SOLVED:
*   The legacy master runner hard-codes its verdict strings, causing
* the "OVERALL ASSESSMENT" and "FINAL STATUS" banners to display
* [FAIL] even when every assertion, unit test, and module passes.
* This header replaces those hard-coded blocks with text_template
* conditional sections (%#all_passed% / %^all_passed%) whose
* predicate is derived from the actual accumulated counters.
*
*   DESIGN:
*   suite_results is a plain aggregate that accumulates module-level
* outcomes alongside the assertion-level print_context already used
* by test_printer.  bind_suite_results() populates a text_template
* from a suite_results — the caller chooses the format string.
* Six default format strings are provided as static constants.
*
*   TEMPLATE SPECIFIERS (available after bind_suite_results):
*     {suite_name}        — name of the suite
*     {suite_description} — description of the suite
*     {modules_total}     — number of modules executed
*     {modules_passed}    — number of modules that passed
*     {modules_failed}    — number of modules that failed
*     {module_pass_rate}  — module success percentage string
*     {total}             — total assertion count
*     {passed}            — assertions passed
*     {failed}            — assertions failed
*     {skipped}           — assertions skipped
*     {pending}           — assertions pending
*     {errors}            — assertions with error status
*     {pass_rate}         — assertion pass percentage
*     {tests_total}       — unit test count
*     {tests_passed}      — unit tests passed
*     {tests_failed}      — unit tests failed
*     {test_pass_rate}    — unit test pass percentage
*     {elapsed}           — total execution time (seconds)
*     {symbol}            — overall [PASS] or [FAIL] symbol
*     {status_word}       — "PASSED" or "FAILED"
*
*   CONDITIONAL SECTIONS:
*     %#all_passed% ... %/all_passed%   — included when all green
*     %^all_passed% ... %/all_passed%   — included when anything failed
*     %#has_failures% ... %/has_failures% — included when failures > 0
*     %#has_skipped% ... %/has_skipped%  — included when skipped > 0
*
*   INTEGRATION:
*   Intended to be called by the master runner after all per-module
* test_printer walks have completed.  The per-module results feed
* into suite_results; then render the comprehensive summary and
* final status through the suite templates.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h for version detection and djinterp.hpp
* for namespace macros and constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    SUITE RESULTS
* II.   DEFAULT FORMAT STRINGS
* III.  SUITE TEMPLATE BINDING
* IV.   SUITE PRINTER
*
*
* path:      /inc/djinterp/test/test_suite_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.15
******************************************************************************/

#ifndef DJINTERP_TEST_SUITE_PRINTER_
#define DJINTERP_TEST_SUITE_PRINTER_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/text/text_template.hpp"
#include "./test_common.hpp"
#include "./test_printer.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   SUITE RESULTS                                       ///
///////////////////////////////////////////////////////////////////////////////

// module_result
//   struct: per-module outcome record.  Captures the module's
// name, description, assertion counters, unit test counters,
// and elapsed time.  Populated by the master runner after each
// module completes.
struct module_result
{
    const char* name;
    const char* description;
    bool        passed;
    std::size_t assertions_total;
    std::size_t assertions_passed;
    std::size_t tests_total;
    std::size_t tests_passed;
    double      elapsed_seconds;

    module_result()
        : name(nullptr),
          description(nullptr),
          passed(false),
          assertions_total(0),
          assertions_passed(0),
          tests_total(0),
          tests_passed(0),
          elapsed_seconds(0.0)
    {}
};


// suite_results
//   struct: accumulated outcomes across all modules in a
// master suite run.  Maintains both per-module records and
// aggregate assertion / unit-test counters.
struct suite_results
{
    const char* suite_name;
    const char* suite_description;

    // module-level aggregates
    std::size_t modules_total;
    std::size_t modules_passed;

    // assertion-level aggregates (from print_context)
    print_context assertions;

    // unit-test-level aggregates
    std::size_t tests_total;
    std::size_t tests_passed;

    // timing
    double elapsed_seconds;

    // per-module records (optional, for detailed output)
    std::vector<module_result> modules;

    suite_results()
        : suite_name(nullptr),
          suite_description(nullptr),
          modules_total(0),
          modules_passed(0),
          assertions(),
          tests_total(0),
          tests_passed(0),
          elapsed_seconds(0.0),
          modules()
    {}

    // all_passed
    //   returns true if every module, assertion, and unit test
    // passed with no errors.
    bool
    all_passed() const D_NOEXCEPT
    {
        return ( (modules_passed == modules_total) &&
                 (assertions.failed == 0)          &&
                 (assertions.errors == 0)          &&
                 (tests_passed == tests_total) );
    }

    // any_failed
    //   returns true if any module, assertion, or unit test
    // failed or errored.
    bool
    any_failed() const D_NOEXCEPT
    {
        return !all_passed();
    }

    // add_module
    //   appends a module_result and updates the aggregate
    // counters.
    void
    add_module(
        const module_result& _mod
    )
    {
        modules.push_back(_mod);

        ++modules_total;

        if (_mod.passed)
        {
            ++modules_passed;
        }

        assertions.total  += _mod.assertions_total;
        assertions.passed += _mod.assertions_passed;
        assertions.failed +=
            (_mod.assertions_total - _mod.assertions_passed);

        tests_total  += _mod.tests_total;
        tests_passed += _mod.tests_passed;

        return;
    }

    // module_pass_rate
    //   returns a percentage string for module success.
    std::string
    module_pass_rate() const
    {
        if (modules_total == 0)
        {
            return "0.00%";
        }

        char buf[16];

        std::snprintf(
            buf, sizeof(buf), "%.2f%%",
            (static_cast<double>(modules_passed) /
             static_cast<double>(modules_total)) * 100.0);

        return std::string(buf);
    }

    // test_pass_rate
    //   returns a percentage string for unit test success.
    std::string
    test_pass_rate() const
    {
        if (tests_total == 0)
        {
            return "0.00%";
        }

        char buf[16];

        std::snprintf(
            buf, sizeof(buf), "%.2f%%",
            (static_cast<double>(tests_passed) /
             static_cast<double>(tests_total)) * 100.0);

        return std::string(buf);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  DEFAULT FORMAT STRINGS                               ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_FMT_SUITE_HEADER
//   constant: banner rendered once before any module executes.
D_STATIC const char* const D_TEST_FMT_SUITE_HEADER =
    "\n========================================"
    "========================================\n"
    "  TESTING: {suite_name}\n"
    "========================================"
    "========================================\n"
    "  Description: {suite_description}\n"
    "  Date/Time:   {date_time}\n"
    "========================================"
    "========================================\n"
    "\n  Starting test suite execution...\n\n";

// D_TEST_FMT_SUITE_MODULE_HEADER
//   constant: banner rendered before each module.
D_STATIC const char* const D_TEST_FMT_SUITE_MODULE_HEADER =
    "\n----------------------------------------"
    "----------------------------------------\n"
    "  MODULE: `{module_name}`\n"
    "  {module_description}\n"
    "----------------------------------------"
    "----------------------------------------\n\n";

// D_TEST_FMT_SUITE_MODULE_FOOTER
//   constant: banner rendered after each module.  Uses the
// module-scoped assertion counters.
D_STATIC const char* const D_TEST_FMT_SUITE_MODULE_FOOTER =
    "\n----------------------------------------"
    "----------------------------------------\n"
    "  MODULE RESULTS: {module_name}\n"
    "----------------------------------------"
    "----------------------------------------\n"
    "  Assertions: {passed}/{total} passed"
    " ({pass_rate})\n"
    "  Unit Tests: {tests_passed}/{tests_total} passed"
    " ({test_pass_rate})\n"
    "  Status:     {symbol} {module_name}"
    " MODULE {status_word}\n"
    "----------------------------------------"
    "----------------------------------------\n";

// D_TEST_FMT_SUITE_COMPREHENSIVE
//   constant: the comprehensive results section rendered after
// all modules complete.  Uses conditional section blocks for
// the overall verdict.
D_STATIC const char* const D_TEST_FMT_SUITE_COMPREHENSIVE =

    "\n========================================"
    "========================================\n"
    "  COMPREHENSIVE TEST RESULTS\n"
    "========================================"
    "========================================\n"

    "\n  MODULE SUMMARY:\n"
    "    Modules Tested:       {modules_total}\n"
    "    Modules Passed:       {modules_passed}\n"
    "    Modules Failed:       {modules_failed}\n"
    "    Module Success Rate:  {module_pass_rate}\n"

    "\n  ASSERTION SUMMARY:\n"
    "    Total Assertions:     {total}\n"
    "    Assertions Passed:    {passed}\n"
    "    Assertions Failed:    {failed}\n"
    "    Assertion Pass Rate:  {pass_rate}\n"

    "\n  UNIT TEST SUMMARY:\n"
    "    Total Unit Tests:     {tests_total}\n"
    "    Unit Tests Passed:    {tests_passed}\n"
    "    Unit Tests Failed:    {tests_failed}\n"
    "    Unit Test Pass Rate:  {test_pass_rate}\n"

    "\n  EXECUTION TIME:\n"
    "    Total Time:           {elapsed} seconds\n"

    "\n  OVERALL ASSESSMENT:\n"
    "%#all_passed%"
    "    {symbol} ALL TESTS PASSED\n"
    "%/all_passed%"
    "%^all_passed%"
    "    {symbol} SOME TESTS FAILED - ATTENTION REQUIRED\n"
    "%#has_failures%"
    "    [FAIL] Review failed tests before proceeding\n"
    "    [FAIL] Check for memory leaks or logic errors\n"
    "    [FAIL] Verify all edge cases are handled properly\n"
    "%/has_failures%"
    "%/all_passed%"

    "\n========================================"
    "========================================\n";

// D_TEST_FMT_SUITE_FINAL_STATUS
//   constant: the closing banner.  Conditional sections ensure
// the verdict reflects actual outcomes.
D_STATIC const char* const D_TEST_FMT_SUITE_FINAL_STATUS =

    "\n========================================"
    "========================================\n"
    "  FINAL STATUS\n"
    "========================================"
    "========================================\n"

    "%#all_passed%"
    "\n  {symbol} {suite_name} PASSED\n"
    "%/all_passed%"
    "%^all_passed%"
    "\n  {symbol} {suite_name} COMPLETED WITH FAILURES\n"
    "\n  [FAIL] Review and fix all failures before proceeding\n"
    "  [FAIL] Framework stability may be compromised\n"
    "%/all_passed%"

    "\n========================================"
    "========================================\n\n";


///////////////////////////////////////////////////////////////////////////////
///                III. SUITE TEMPLATE BINDING                               ///
///////////////////////////////////////////////////////////////////////////////

// bind_suite_results
//   function: populates a text_template with all suite-level
// specifiers and conditional sections derived from the
// accumulated suite_results.  After this call, the template
// is ready to render any of the suite format strings.
D_INLINE void
bind_suite_results(
    text::text_template& _tmpl,
    const suite_results& _results
)
{
    // suite identity
    _tmpl.bind("suite_name",
        _results.suite_name
            ? _results.suite_name
            : "Test Suite");
    _tmpl.bind("suite_description",
        _results.suite_description
            ? _results.suite_description
            : "");

    // module counters
    _tmpl.bind("modules_total",
        print_context::size_to_string(
            _results.modules_total));
    _tmpl.bind("modules_passed",
        print_context::size_to_string(
            _results.modules_passed));
    _tmpl.bind("modules_failed",
        print_context::size_to_string(
            _results.modules_total -
            _results.modules_passed));
    _tmpl.bind("module_pass_rate",
        _results.module_pass_rate());

    // assertion counters
    _tmpl.bind("total",
        print_context::size_to_string(
            _results.assertions.total));
    _tmpl.bind("passed",
        print_context::size_to_string(
            _results.assertions.passed));
    _tmpl.bind("failed",
        print_context::size_to_string(
            _results.assertions.failed));
    _tmpl.bind("skipped",
        print_context::size_to_string(
            _results.assertions.skipped));
    _tmpl.bind("pending",
        print_context::size_to_string(
            _results.assertions.pending));
    _tmpl.bind("errors",
        print_context::size_to_string(
            _results.assertions.errors));
    _tmpl.bind("pass_rate",
        _results.assertions.pass_rate());

    // unit test counters
    _tmpl.bind("tests_total",
        print_context::size_to_string(
            _results.tests_total));
    _tmpl.bind("tests_passed",
        print_context::size_to_string(
            _results.tests_passed));
    _tmpl.bind("tests_failed",
        print_context::size_to_string(
            _results.tests_total -
            _results.tests_passed));
    _tmpl.bind("test_pass_rate",
        _results.test_pass_rate());

    // timing
    char time_buf[32];
    std::snprintf(time_buf, sizeof(time_buf),
                  "%.3f", _results.elapsed_seconds);
    _tmpl.bind("elapsed", time_buf);

    // overall symbol and status word
    bool green = _results.all_passed();

    _tmpl.bind("symbol",
        green ? "[PASS]" : "[FAIL]");
    _tmpl.bind("status_word",
        green ? "PASSED" : "FAILED");

    // conditional sections
    _tmpl.bind_section("all_passed", green);

    _tmpl.bind_section("has_failures",
        ( (_results.assertions.failed > 0) ||
          (_results.assertions.errors > 0) ));

    _tmpl.bind_section("has_skipped",
        (_results.assertions.skipped > 0));

    return;
}


// bind_module_result
//   function: populates a text_template with per-module
// specifiers for rendering module header/footer banners.
D_INLINE void
bind_module_result(
    text::text_template& _tmpl,
    const module_result& _mod
)
{
    _tmpl.bind("module_name",
        _mod.name ? _mod.name : "");
    _tmpl.bind("module_description",
        _mod.description ? _mod.description : "");

    _tmpl.bind("total",
        print_context::size_to_string(
            _mod.assertions_total));
    _tmpl.bind("passed",
        print_context::size_to_string(
            _mod.assertions_passed));
    _tmpl.bind("failed",
        print_context::size_to_string(
            _mod.assertions_total -
            _mod.assertions_passed));

    // assertion pass rate
    char rate_buf[16];
    if (_mod.assertions_total > 0)
    {
        std::snprintf(
            rate_buf, sizeof(rate_buf), "%.2f%%",
            (static_cast<double>(_mod.assertions_passed) /
             static_cast<double>(_mod.assertions_total))
                * 100.0);
    }
    else
    {
        std::snprintf(rate_buf, sizeof(rate_buf),
                      "0.00%%");
    }

    _tmpl.bind("pass_rate", rate_buf);

    // unit test counters
    _tmpl.bind("tests_total",
        print_context::size_to_string(
            _mod.tests_total));
    _tmpl.bind("tests_passed",
        print_context::size_to_string(
            _mod.tests_passed));

    // unit test pass rate
    char test_rate_buf[16];
    if (_mod.tests_total > 0)
    {
        std::snprintf(
            test_rate_buf, sizeof(test_rate_buf),
            "%.2f%%",
            (static_cast<double>(_mod.tests_passed) /
             static_cast<double>(_mod.tests_total))
                * 100.0);
    }
    else
    {
        std::snprintf(test_rate_buf,
                      sizeof(test_rate_buf),
                      "0.00%%");
    }

    _tmpl.bind("test_pass_rate", test_rate_buf);

    // symbol and status word
    _tmpl.bind("symbol",
        _mod.passed ? "[PASS]" : "[FAIL]");
    _tmpl.bind("status_word",
        _mod.passed ? "PASSED" : "FAILED");

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  SUITE PRINTER                                        ///
///////////////////////////////////////////////////////////////////////////////

// suite_printer
//   class: convenience wrapper that owns the suite-level
// text_template instances and renders the master suite
// banners.  Does NOT own or manage per-module test_printer
// instances — those remain the caller's responsibility.
//
// Usage:
//   suite_printer sp;
//   sp.set_suite_name("djinterp Master Test Suite");
//   sp.set_suite_description("Full project validation");
//   sp.print_suite_header();
//
//   // ... run modules, accumulate results ...
//   sp.results().add_module(mod);
//
//   sp.print_comprehensive();
//   sp.print_final_status();
//
//   // exit code:
//   return sp.results().all_passed() ? 0 : 1;
class suite_printer
{
public:
    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    suite_printer()
        : m_header_tmpl("{", "}"),
          m_mod_hdr_tmpl("{", "}"),
          m_mod_ftr_tmpl("{", "}"),
          m_comp_tmpl("{", "}"),
          m_final_tmpl("{", "}"),
          m_header_fmt(D_TEST_FMT_SUITE_HEADER),
          m_mod_hdr_fmt(D_TEST_FMT_SUITE_MODULE_HEADER),
          m_mod_ftr_fmt(D_TEST_FMT_SUITE_MODULE_FOOTER),
          m_comp_fmt(D_TEST_FMT_SUITE_COMPREHENSIVE),
          m_final_fmt(D_TEST_FMT_SUITE_FINAL_STATUS),
          m_results(),
          m_sink()
    {
        m_sink = [](const char* d, std::size_t n)
        {
            std::fwrite(d, 1, n, stdout);
        };
    }


    // =================================================================
    //  suite identity
    // =================================================================

    void
    set_suite_name(
        const char* _name
    ) D_NOEXCEPT
    {
        m_results.suite_name = _name;

        return;
    }

    void
    set_suite_description(
        const char* _description
    ) D_NOEXCEPT
    {
        m_results.suite_description = _description;

        return;
    }


    // =================================================================
    //  results access
    // =================================================================

    suite_results&
    results() D_NOEXCEPT
    {
        return m_results;
    }

    const suite_results&
    results() const D_NOEXCEPT
    {
        return m_results;
    }


    // =================================================================
    //  format setters
    // =================================================================

    void set_header_format(const std::string& _f)
    {
        m_header_fmt = _f;

        return;
    }

    void set_module_header_format(const std::string& _f)
    {
        m_mod_hdr_fmt = _f;

        return;
    }

    void set_module_footer_format(const std::string& _f)
    {
        m_mod_ftr_fmt = _f;

        return;
    }

    void set_comprehensive_format(const std::string& _f)
    {
        m_comp_fmt = _f;

        return;
    }

    void set_final_status_format(const std::string& _f)
    {
        m_final_fmt = _f;

        return;
    }


    // =================================================================
    //  template access
    // =================================================================

    text::text_template& header_template()         D_NOEXCEPT
    {
        return m_header_tmpl;
    }

    text::text_template& module_header_template()  D_NOEXCEPT
    {
        return m_mod_hdr_tmpl;
    }

    text::text_template& module_footer_template()  D_NOEXCEPT
    {
        return m_mod_ftr_tmpl;
    }

    text::text_template& comprehensive_template()  D_NOEXCEPT
    {
        return m_comp_tmpl;
    }

    text::text_template& final_status_template()   D_NOEXCEPT
    {
        return m_final_tmpl;
    }


    // =================================================================
    //  sink
    // =================================================================

    void
    set_sink(
        print_sink _s
    )
    {
        m_sink = static_cast<print_sink&&>(_s);

        return;
    }

    void
    set_sink_stdout()
    {
        m_sink = [](const char* d, std::size_t n)
        {
            std::fwrite(d, 1, n, stdout);
        };

        return;
    }

    void
    set_sink_string(
        std::string& _out
    )
    {
        m_sink = [&_out](const char* d, std::size_t n)
        {
            _out.append(d, n);
        };

        return;
    }

    void
    set_sink_file(
        std::FILE* _f
    )
    {
        m_sink = [_f](const char* d, std::size_t n)
        {
            std::fwrite(d, 1, n, _f);
        };

        return;
    }


    // =================================================================
    //  rendering: suite header
    // =================================================================

    // print_suite_header
    //   renders the suite header banner.  The caller may bind
    // additional specifiers (e.g. {date_time}) to the header
    // template before calling this method.
    void
    print_suite_header()
    {
        if (m_header_fmt.empty())
        {
            return;
        }

        bind_suite_results(m_header_tmpl, m_results);

        emit(m_header_tmpl.render(m_header_fmt));

        return;
    }


    // =================================================================
    //  rendering: per-module banners
    // =================================================================

    // print_module_header
    //   renders the module header banner for the given module
    // result.
    void
    print_module_header(
        const module_result& _mod
    )
    {
        if (m_mod_hdr_fmt.empty())
        {
            return;
        }

        m_mod_hdr_tmpl.clear_bindings();
        bind_module_result(m_mod_hdr_tmpl, _mod);

        emit(m_mod_hdr_tmpl.render(m_mod_hdr_fmt));

        return;
    }

    // print_module_footer
    //   renders the module footer banner for the given module
    // result.
    void
    print_module_footer(
        const module_result& _mod
    )
    {
        if (m_mod_ftr_fmt.empty())
        {
            return;
        }

        m_mod_ftr_tmpl.clear_bindings();
        bind_module_result(m_mod_ftr_tmpl, _mod);

        emit(m_mod_ftr_tmpl.render(m_mod_ftr_fmt));

        return;
    }


    // =================================================================
    //  rendering: comprehensive results
    // =================================================================

    // print_comprehensive
    //   renders the comprehensive results section.  The
    // all_passed section predicate is derived from the
    // accumulated suite_results — never hard-coded.
    void
    print_comprehensive()
    {
        if (m_comp_fmt.empty())
        {
            return;
        }

        m_comp_tmpl.clear_bindings();
        bind_suite_results(m_comp_tmpl, m_results);

        emit(m_comp_tmpl.render(m_comp_fmt));

        return;
    }


    // =================================================================
    //  rendering: final status
    // =================================================================

    // print_final_status
    //   renders the closing verdict banner.
    void
    print_final_status()
    {
        if (m_final_fmt.empty())
        {
            return;
        }

        m_final_tmpl.clear_bindings();
        bind_suite_results(m_final_tmpl, m_results);

        emit(m_final_tmpl.render(m_final_fmt));

        return;
    }


    // =================================================================
    //  rendering: full suite (convenience)
    // =================================================================

    // print_suite_summary
    //   renders both the comprehensive results and the final
    // status in sequence.
    void
    print_suite_summary()
    {
        print_comprehensive();
        print_final_status();

        return;
    }


    // =================================================================
    //  exit code
    // =================================================================

    // exit_code
    //   returns 0 if all tests passed, 1 otherwise.  Suitable
    // for use as the master runner's process exit code.
    int
    exit_code() const D_NOEXCEPT
    {
        return m_results.all_passed() ? 0 : 1;
    }

private:
    // =================================================================
    //  internal: emit
    // =================================================================

    void
    emit(
        const std::string& _text
    ) const
    {
        if ( (m_sink) &&
             (!_text.empty()) )
        {
            m_sink(_text.data(), _text.size());
        }

        return;
    }


    // =================================================================
    //  storage
    // =================================================================

    mutable text::text_template m_header_tmpl;
    mutable text::text_template m_mod_hdr_tmpl;
    mutable text::text_template m_mod_ftr_tmpl;
    mutable text::text_template m_comp_tmpl;
    mutable text::text_template m_final_tmpl;

    std::string m_header_fmt;
    std::string m_mod_hdr_fmt;
    std::string m_mod_ftr_fmt;
    std::string m_comp_fmt;
    std::string m_final_fmt;

    suite_results m_results;
    print_sink    m_sink;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_SUITE_PRINTER_
