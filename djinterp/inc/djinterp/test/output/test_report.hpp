/******************************************************************************
* djinterp [test]                                              test_report.hpp
*
*   The presentation-side data model for a DTest run.  Where test_tree /
* test_session model EXECUTION (a forest of evaluable nodes walked by a
* handler), this header models the RESULT as it is reported: a flat,
* render-ready aggregate that mirrors the shape of the output document -
* a run is a sequence of modules, a module is a sequence of unit tests,
* and a unit test is a sequence of assertions (checks).  Nothing here
* evaluates anything; a report is built by a runner (or by walking a tree)
* and then handed to a renderer (test_printer for text, test_pdf_report
* for PDF).
*
*   WHY A SEPARATE MODEL:
*   The execution tree is anonymous and rank-ordered; it is the wrong shape
* to drive a report (no module description, no per-unit assertion roll-up,
* no cross-module comprehensive tally).  Rather than overload the tree with
* presentation concerns, the report model is its own small vocabulary that
* any producer can populate and any renderer can consume.  This is the same
* separation the rest of the framework keeps between a value (the tree) and
* its rendered face (the printer).
*
*   THE SHAPE (mirrors the rendered document):
*       test_report   = { title / subtitle / author / description ;
*                         <modules> ; notes }
*       report_module = { name ; description ; <units> }
*       report_unit   = { name ; <checks> ; verdict ; elapsed }
*       report_check  = { description ; status }
*   A check is one assertion line ("[PASS] ... succeeded").  A unit's verdict
* is failed iff any of its checks failed or errored (or it was explicitly
* marked failed), so a unit with a single check is the natural model for a
* runner whose tests are themselves the leaves.
*
*   TALLIES:
*   Every figure in the rendered report is derived here, not in the renderer:
* per-module assertion and unit-test counts, the cross-module comprehensive
* totals, and the pass-rate strings.  This keeps the text and PDF faces in
* perfect agreement - they read the same numbers.
*
*   FILE NAMING:
*   expand_report_file_name folds a small placeholder vocabulary ({module},
* {index}, {date}, {time}, {title}, {ext}) into a concrete output filename,
* sanitizing the substituted components so a module name is safe on disk.
* This is the mechanism behind the file_name_pattern option in
* test_options.hpp and the document_per_module layout.
*
*   PORTABILITY:
*   C++11 minimum.  No third-party dependency; only <ctime> for the {date} /
* {time} placeholders.
*
*
* TABLE OF CONTENTS
* =================
* I.    REPORT VERDICT
* II.   REPORT CHECK        (one assertion)
* III.  REPORT UNIT         (one unit test)
* IV.   REPORT MODULE       (one module)
* V.    TEST REPORT         (the whole run)
* VI.   PASS-RATE HELPERS
* VII.  FILE-NAME EXPANSION
*
*
* path:      /inc/djinterp/test/output/test_report.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.26
******************************************************************************/

#ifndef DJINTERP_TEST_REPORT_
#define DJINTERP_TEST_REPORT_ 1

#ifndef __cplusplus
    #error "test_report.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../test_common.hpp"   // test_status


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_report.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   REPORT VERDICT                                       ///
///////////////////////////////////////////////////////////////////////////////

// report_verdict
//   enum: the overall conclusion of a unit, module, or whole report.
// `empty` is "nothing observed"; the rest follow the same decision order
// the session uses - any failure or error is `failed`, otherwise any
// unfinished work is `pending`, otherwise `passed`.
enum class report_verdict
{
    empty   = 0,
    passed  = 1,
    failed  = 2,
    pending = 3
};

// report_verdict_word
//   function: the uppercase PASS / FAIL word for a verdict, suited to a
// status line.  `pending` and `empty` both read as a non-pass so a banner
// never claims success on an unfinished run.
D_NODISCARD D_INLINE const char*
report_verdict_word(
    report_verdict _v
) D_NOEXCEPT
{
    switch (_v)
    {
        case report_verdict::passed:  { return "PASS"; }
        case report_verdict::failed:  { return "FAIL"; }
        case report_verdict::pending: { return "PENDING"; }
        default:                      { return "EMPTY"; }
    }
}

// report_verdict_is_pass
//   function: true iff _v is the clean-pass verdict.
D_NODISCARD D_INLINE bool
report_verdict_is_pass(
    report_verdict _v
) D_NOEXCEPT
{
    return (_v == report_verdict::passed);
}


NS_INTERNAL

    // verdict_from_counts_helper
    //   helper: fold per-status counts into a report_verdict using the
    // canonical order (empty -> failed -> pending -> passed).
    D_NODISCARD D_INLINE report_verdict
    verdict_from_counts_helper(
        std::size_t _passed,
        std::size_t _failed,
        std::size_t _skipped,
        std::size_t _pending,
        std::size_t _errors
    ) D_NOEXCEPT
    {
        const std::size_t total =
            _passed + _failed + _skipped + _pending + _errors;

        if (total == 0)
        {
            return report_verdict::empty;
        }

        if ( (_failed > 0) ||
             (_errors > 0) )
        {
            return report_verdict::failed;
        }

        if (_pending > 0)
        {
            return report_verdict::pending;
        }

        return report_verdict::passed;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  REPORT CHECK                                         ///
///////////////////////////////////////////////////////////////////////////////

// report_check
//   struct: one assertion within a unit test - a human-readable
// description paired with the status it resolved to.  This is the
// "[PASS] ... succeeded" line of the rendered report.
struct report_check
{
    std::string description;
    test_status status;

    // Assertion detail, for the per-assertion report table (RESULT / # /
    // ASSERTION / EXPECTED / ACTUAL).  A check may carry only a description
    // (a bare pass/fail leaf); when it also records the asserted expression
    // and the expected / actual values, the report shows them column-wise.
    // Empty by default, so existing checks render exactly as before.
    std::string expression;   // the asserted expression   (ASSERTION column)
    std::string expected;     // rendered expected value    (EXPECTED column)
    std::string actual;       // rendered actual value      (ACTUAL column)

    // report_check
    //   constructor: an empty, pending check.
    report_check()
        : description(),
          status(test_status::pending),
          expression(),
          expected(),
          actual()
    {}

    // report_check
    //   constructor: a check with a description and resolved status.
    report_check(
        std::string _description,
        test_status _status
    )
        : description(static_cast<std::string&&>(_description)),
          status(_status),
          expression(),
          expected(),
          actual()
    {}

    // report_check
    //   constructor: a check that also carries per-assertion detail - the
    // asserted expression and its expected / actual rendered values.
    report_check(
        std::string _description,
        test_status _status,
        std::string _expression,
        std::string _expected,
        std::string _actual
    )
        : description(static_cast<std::string&&>(_description)),
          status(_status),
          expression(static_cast<std::string&&>(_expression)),
          expected(static_cast<std::string&&>(_expected)),
          actual(static_cast<std::string&&>(_actual))
    {}

    // passed
    //   true iff this check resolved to passed.
    D_NODISCARD bool
    passed() const D_NOEXCEPT
    {
        return (status == test_status::passed);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. REPORT UNIT                                          ///
///////////////////////////////////////////////////////////////////////////////

// report_unit
//   struct: one unit test - a named group of checks plus the unit's own
// verdict and an optional wall-clock duration.  The verdict is DERIVED
// from the checks (failed iff any check failed or errored) until a caller
// sets it explicitly with set_verdict(); a runner whose tests are
// themselves leaves models each as a unit carrying a single check.
struct report_unit
{
    std::string               name;
    std::string               description;
    std::vector<report_check> checks;
    report_verdict            verdict;
    std::int64_t              elapsed_ns;

    // report_unit
    //   constructor: an empty, pending unit.
    report_unit()
        : name(),
          description(),
          checks(),
          verdict(report_verdict::empty),
          elapsed_ns(0),
          m_verdict_explicit(false)
    {}

    // report_unit
    //   constructor: a named, empty unit.
    explicit report_unit(
        std::string _name
    )
        : name(static_cast<std::string&&>(_name)),
          description(),
          checks(),
          verdict(report_verdict::empty),
          elapsed_ns(0),
          m_verdict_explicit(false)
    {}

    // report_unit
    //   constructor: a named unit carrying a descriptor (e.g. a spec block's
    // "descriptor"), rendered as the test card's description line.
    report_unit(
        std::string _name,
        std::string _description
    )
        : name(static_cast<std::string&&>(_name)),
          description(static_cast<std::string&&>(_description)),
          checks(),
          verdict(report_verdict::empty),
          elapsed_ns(0),
          m_verdict_explicit(false)
    {}

    // add_check
    //   appends a check and, unless the verdict was pinned explicitly,
    // re-derives the unit verdict from the accumulated checks.
    void
    add_check(
        std::string _description,
        test_status _status
    )
    {
        checks.push_back(
            report_check(static_cast<std::string&&>(_description), _status));

        if (!m_verdict_explicit)
        {
            verdict = derive_verdict();
        }

        return;
    }

    // add_result
    //   convenience: appends a check whose status is `passed` or `failed`
    // per _ok.  The natural call for a runner recording a boolean leaf.
    void
    add_result(
        std::string _description,
        bool        _ok
    )
    {
        add_check(
            static_cast<std::string&&>(_description),
            _ok ? test_status::passed : test_status::failed);

        return;
    }

    // add_check
    //   overload: appends a check carrying per-assertion detail (the asserted
    // expression and its expected / actual rendered values), re-deriving the
    // unit verdict unless it was pinned explicitly.
    void
    add_check(
        std::string _description,
        test_status _status,
        std::string _expression,
        std::string _expected,
        std::string _actual
    )
    {
        checks.push_back(report_check(
            static_cast<std::string&&>(_description),
            _status,
            static_cast<std::string&&>(_expression),
            static_cast<std::string&&>(_expected),
            static_cast<std::string&&>(_actual)));

        if (!m_verdict_explicit)
        {
            verdict = derive_verdict();
        }

        return;
    }

    // add_result
    //   overload: appends a `passed`/`failed` check with per-assertion detail.
    // The asserted expression doubles as the check's description (so the live
    // console line reads it), and the expected / actual values populate the
    // report's EXPECTED / ACTUAL columns.
    void
    add_result(
        std::string _expression,
        std::string _expected,
        std::string _actual,
        bool        _ok
    )
    {
        std::string _desc = _expression;

        add_check(
            static_cast<std::string&&>(_desc),
            _ok ? test_status::passed : test_status::failed,
            static_cast<std::string&&>(_expression),
            static_cast<std::string&&>(_expected),
            static_cast<std::string&&>(_actual));

        return;
    }

    // set_verdict
    //   pins the unit verdict explicitly, so later add_check calls no
    // longer re-derive it.  Use when the unit's conclusion is decided by
    // something other than its recorded checks.
    void
    set_verdict(
        report_verdict _v
    ) D_NOEXCEPT
    {
        verdict            = _v;
        m_verdict_explicit = true;

        return;
    }

    // passed
    //   true iff the unit verdict is a clean pass.
    D_NODISCARD bool
    passed() const D_NOEXCEPT
    {
        return report_verdict_is_pass(verdict);
    }

    // total_checks / passed_checks / failed_checks
    //   the per-unit assertion tally.  failed_checks counts both failed
    // and errored checks (the two non-pass terminal states).
    D_NODISCARD std::size_t
    total_checks() const D_NOEXCEPT
    {
        return checks.size();
    }

    D_NODISCARD std::size_t
    passed_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < checks.size(); ++i)
        {
            if (checks[i].status == test_status::passed)
            {
                ++n;
            }
        }

        return n;
    }

    D_NODISCARD std::size_t
    failed_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < checks.size(); ++i)
        {
            if ( (checks[i].status == test_status::failed) ||
                 (checks[i].status == test_status::error) )
            {
                ++n;
            }
        }

        return n;
    }

private:
    // derive_verdict
    //   folds the unit's checks into a verdict.  An empty unit is `empty`;
    // otherwise the canonical order applies over the checks' statuses.
    D_NODISCARD report_verdict
    derive_verdict() const
    {
        std::size_t passed_n  = 0;
        std::size_t failed_n  = 0;
        std::size_t skipped_n = 0;
        std::size_t pending_n = 0;
        std::size_t errors_n  = 0;
        std::size_t i         = 0;

        for (i = 0; i < checks.size(); ++i)
        {
            switch (checks[i].status)
            {
                case test_status::passed:  { ++passed_n;  break; }
                case test_status::failed:  { ++failed_n;  break; }
                case test_status::skipped: { ++skipped_n; break; }
                case test_status::pending: { ++pending_n; break; }
                default:                   { ++errors_n;  break; }
            }
        }

        return internal::verdict_from_counts_helper(
            passed_n, failed_n, skipped_n, pending_n, errors_n);
    }

    bool m_verdict_explicit;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  REPORT MODULE                                        ///
///////////////////////////////////////////////////////////////////////////////

// report_module
//   struct: one module - a named, described group of unit tests.  Carries
// the unit list and derives both the unit-test tally (over unit verdicts)
// and the assertion tally (over every check in every unit), so a renderer
// reads "X/Y unit tests" and "X/Y assertions" straight off the module.
struct report_module
{
    std::string              name;
    std::string              description;
    std::vector<report_unit> units;

    // report_module
    //   constructor: an empty, unnamed module.
    report_module()
        : name(),
          description(),
          units()
    {}

    // report_module
    //   constructor: a named, described module.
    report_module(
        std::string _name,
        std::string _description
    )
        : name(static_cast<std::string&&>(_name)),
          description(static_cast<std::string&&>(_description)),
          units()
    {}

    // add_unit
    //   appends _unit (move) and returns a reference to the stored copy
    // so the caller may keep adding checks to it.
    report_unit&
    add_unit(
        report_unit _unit
    )
    {
        units.push_back(static_cast<report_unit&&>(_unit));

        return units.back();
    }

    // add_result_unit
    //   convenience: appends a unit named _name carrying a single check
    // whose status is `passed`/`failed` per _ok - the natural shape for a
    // runner whose tests are leaves.  Returns the stored unit.
    report_unit&
    add_result_unit(
        const std::string& _name,
        bool               _ok
    )
    {
        report_unit u(_name);
        u.add_result(_name, _ok);

        units.push_back(static_cast<report_unit&&>(u));

        return units.back();
    }

    // -----------------------------------------------------------------
    //  unit-test tally (over unit verdicts)
    // -----------------------------------------------------------------

    D_NODISCARD std::size_t
    total_units() const D_NOEXCEPT
    {
        return units.size();
    }

    D_NODISCARD std::size_t
    passed_units() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < units.size(); ++i)
        {
            if (units[i].passed())
            {
                ++n;
            }
        }

        return n;
    }

    D_NODISCARD std::size_t
    failed_units() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < units.size(); ++i)
        {
            if ( (units[i].verdict == report_verdict::failed) ||
                 (!units[i].passed() &&
                  (units[i].verdict != report_verdict::empty)) )
            {
                ++n;
            }
        }

        return n;
    }

    // -----------------------------------------------------------------
    //  assertion tally (over every check)
    // -----------------------------------------------------------------

    D_NODISCARD std::size_t
    total_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < units.size(); ++i)
        {
            n += units[i].total_checks();
        }

        return n;
    }

    D_NODISCARD std::size_t
    passed_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < units.size(); ++i)
        {
            n += units[i].passed_checks();
        }

        return n;
    }

    D_NODISCARD std::size_t
    failed_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < units.size(); ++i)
        {
            n += units[i].failed_checks();
        }

        return n;
    }

    // -----------------------------------------------------------------
    //  verdict
    // -----------------------------------------------------------------

    // verdict
    //   the module conclusion, folded over its unit verdicts.
    D_NODISCARD report_verdict
    verdict() const
    {
        std::size_t passed_n  = 0;
        std::size_t failed_n  = 0;
        std::size_t pending_n = 0;
        std::size_t i         = 0;

        for (i = 0; i < units.size(); ++i)
        {
            switch (units[i].verdict)
            {
                case report_verdict::passed:  { ++passed_n;  break; }
                case report_verdict::failed:  { ++failed_n;  break; }
                case report_verdict::pending: { ++pending_n; break; }
                default:                      { break; }  // empty: ignored
            }
        }

        return internal::verdict_from_counts_helper(
            passed_n, failed_n, 0, pending_n, 0);
    }

    // passed
    //   true iff the module verdict is a clean pass.
    D_NODISCARD bool
    passed() const
    {
        return report_verdict_is_pass(verdict());
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST REPORT                                          ///
///////////////////////////////////////////////////////////////////////////////

// test_report
//   struct: the whole run as it is reported - document metadata, the
// ordered module list, and an optional free-form notes block (the
// "implementation notes & recommendations" section).  Aggregate tallies
// roll the per-module figures up across the run for the comprehensive
// summary; the report verdict folds the module verdicts.
struct test_report
{
    std::string title;
    std::string subtitle;
    std::string author;
    std::string description;
    std::string notes;

    std::vector<report_module> modules;

    // test_report
    //   constructor: an empty, untitled report.
    test_report()
        : title(),
          subtitle(),
          author(),
          description(),
          notes(),
          modules()
    {}

    // add_module
    //   appends _module (move) and returns a reference to the stored copy.
    report_module&
    add_module(
        report_module _module
    )
    {
        modules.push_back(static_cast<report_module&&>(_module));

        return modules.back();
    }

    // module
    //   convenience: appends a fresh module with the given name and
    // description, returning it for population.
    report_module&
    module(
        const std::string& _name,
        const std::string& _description = std::string()
    )
    {
        return add_module(report_module(_name, _description));
    }

    // -----------------------------------------------------------------
    //  module tally
    // -----------------------------------------------------------------

    D_NODISCARD std::size_t
    total_modules() const D_NOEXCEPT
    {
        return modules.size();
    }

    D_NODISCARD std::size_t
    passed_modules() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            if (modules[i].passed())
            {
                ++n;
            }
        }

        return n;
    }

    D_NODISCARD std::size_t
    failed_modules() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            if (!modules[i].passed())
            {
                ++n;
            }
        }

        return n;
    }

    // -----------------------------------------------------------------
    //  cross-module unit-test tally
    // -----------------------------------------------------------------

    D_NODISCARD std::size_t
    total_units() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            n += modules[i].total_units();
        }

        return n;
    }

    D_NODISCARD std::size_t
    passed_units() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            n += modules[i].passed_units();
        }

        return n;
    }

    D_NODISCARD std::size_t
    failed_units() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            n += modules[i].failed_units();
        }

        return n;
    }

    // -----------------------------------------------------------------
    //  cross-module assertion tally
    // -----------------------------------------------------------------

    D_NODISCARD std::size_t
    total_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            n += modules[i].total_checks();
        }

        return n;
    }

    D_NODISCARD std::size_t
    passed_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            n += modules[i].passed_checks();
        }

        return n;
    }

    D_NODISCARD std::size_t
    failed_checks() const
    {
        std::size_t n = 0;
        std::size_t i = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            n += modules[i].failed_checks();
        }

        return n;
    }

    // -----------------------------------------------------------------
    //  verdict
    // -----------------------------------------------------------------

    // verdict
    //   the run conclusion, folded over the module verdicts.
    D_NODISCARD report_verdict
    verdict() const
    {
        std::size_t passed_n  = 0;
        std::size_t failed_n  = 0;
        std::size_t pending_n = 0;
        std::size_t i         = 0;

        for (i = 0; i < modules.size(); ++i)
        {
            switch (modules[i].verdict())
            {
                case report_verdict::passed:  { ++passed_n;  break; }
                case report_verdict::failed:  { ++failed_n;  break; }
                case report_verdict::pending: { ++pending_n; break; }
                default:                      { break; }
            }
        }

        return internal::verdict_from_counts_helper(
            passed_n, failed_n, 0, pending_n, 0);
    }

    // passed
    //   true iff every module passed (a clean run).
    D_NODISCARD bool
    passed() const
    {
        return report_verdict_is_pass(verdict());
    }

    // exit_code
    //   0 when the run passed, 1 otherwise - the conventional process
    // result for a test executable.
    D_NODISCARD int
    exit_code() const
    {
        return passed() ? 0 : 1;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  PASS-RATE HELPERS                                    ///
///////////////////////////////////////////////////////////////////////////////

// report_pass_rate
//   function: the percentage string "NN.NN%" for _passed of _total, with
// a zero total reported as "0.00%" (never a divide-by-zero).  Two decimal
// places, matching the rendered summaries.
D_NODISCARD D_INLINE std::string
report_pass_rate(
    std::size_t _passed,
    std::size_t _total
)
{
    char buf[16];

    if (_total == 0)
    {
        return std::string("0.00%");
    }

    std::snprintf(buf,
                  sizeof(buf),
                  "%.2f%%",
                  (static_cast<double>(_passed) /
                   static_cast<double>(_total)) * 100.0);

    return std::string(buf);
}

// report_ratio
//   function: the "passed/total" fraction string (e.g. "80/93").
D_NODISCARD D_INLINE std::string
report_ratio(
    std::size_t _passed,
    std::size_t _total
)
{
    char buf[48];

    std::snprintf(buf,
                  sizeof(buf),
                  "%zu/%zu",
                  _passed,
                  _total);

    return std::string(buf);
}


///////////////////////////////////////////////////////////////////////////////
///                VII. FILE-NAME EXPANSION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // sanitize_file_component_helper
    //   helper: returns _raw with every character that is hostile to a
    // filename (path separators, wildcards, reserved punctuation, control
    // characters, whitespace) replaced by '_'.  A leading/trailing run is
    // left in place save for trimming surrounding spaces, so the result is
    // always a usable single path component.
    D_NODISCARD D_INLINE std::string
    sanitize_file_component_helper(
        const std::string& _raw
    )
    {
        std::string out;
        std::size_t i = 0;

        out.reserve(_raw.size());

        for (i = 0; i < _raw.size(); ++i)
        {
            const char c = _raw[i];

            const bool hostile =
                ( (c == '/')  || (c == '\\') || (c == ':') ||
                  (c == '*')  || (c == '?')  || (c == '"') ||
                  (c == '<')  || (c == '>')  || (c == '|') ||
                  (c == ' ')  || (c == '\t') ||
                  (static_cast<unsigned char>(c) < 0x20) );

            out.push_back(hostile ? '_' : c);
        }

        return out;
    }

    // report_timestamp_helper
    //   helper: fills _date (YYYYMMDD) and _time (HHMMSS) from the local
    // wall clock.  Used by the {date} / {time} filename placeholders.  On a
    // platform where localtime is unavailable both come back empty.
    D_INLINE void
    report_timestamp_helper(
        std::string& _date,
        std::string& _time
    )
    {
        std::time_t now = std::time(nullptr);
        std::tm     tmv;
        char        dbuf[16];
        char        tbuf[16];

        _date.clear();
        _time.clear();

#if defined(_WIN32) || defined(_MSC_VER)
        if (::localtime_s(&tmv, &now) != 0)
        {
            return;
        }
#else
        if (::localtime_r(&now, &tmv) == nullptr)
        {
            return;
        }
#endif

        if (std::strftime(dbuf, sizeof(dbuf), "%Y%m%d", &tmv) > 0)
        {
            _date = dbuf;
        }

        if (std::strftime(tbuf, sizeof(tbuf), "%H%M%S", &tmv) > 0)
        {
            _time = tbuf;
        }

        return;
    }

    // replace_all_helper
    //   helper: replaces every occurrence of _token in _s with _value,
    // returning the rewritten string.  Non-recursive (a value containing
    // _token is not re-scanned).
    D_NODISCARD D_INLINE std::string
    replace_all_helper(
        const std::string& _s,
        const std::string& _token,
        const std::string& _value
    )
    {
        if (_token.empty())
        {
            return _s;
        }

        std::string out;
        std::size_t pos  = 0;
        std::size_t next = 0;

        while ((next = _s.find(_token, pos)) != std::string::npos)
        {
            out.append(_s, pos, next - pos);
            out.append(_value);
            pos = next + _token.size();
        }

        out.append(_s, pos, _s.size() - pos);

        return out;
    }

NS_END  // internal


// expand_report_file_name
//   function: lowers a filename PATTERN into a concrete path by folding in
// the run's facts.  The placeholder vocabulary is:
//
//     {module}  the module name        (sanitized for the filesystem)
//     {index}   the module's position  (decimal, zero-based)
//     {title}   the report title       (sanitized; empty if unset)
//     {date}    the run date           (YYYYMMDD, local clock)
//     {time}    the run time           (HHMMSS, local clock)
//     {ext}     the document extension  (e.g. "pdf"), no dot
//
//   An empty pattern falls back to "{module}.{ext}" so a caller that sets
// no pattern still gets a sensible per-module filename.  The module and
// title substitutions are sanitized; the literal portions of the pattern
// (including any directory prefix the caller supplies) are emitted as-is.
//
// Parameter(s):
//   _pattern: the filename pattern, possibly empty.
//   _module:  the module name to substitute for {module}.
//   _index:   the module index to substitute for {index}.
//   _title:   the report title to substitute for {title}.
//   _ext:     the document extension (without a leading dot).
// Return:
//   the expanded filename.
D_NODISCARD D_INLINE std::string
expand_report_file_name(
    const std::string& _pattern,
    const std::string& _module,
    std::size_t        _index,
    const std::string& _title,
    const std::string& _ext
)
{
    std::string pattern = _pattern.empty()
                              ? std::string("{module}.{ext}")
                              : _pattern;

    std::string date;
    std::string time;
    char        ibuf[32];

    internal::report_timestamp_helper(date, time);

    std::snprintf(ibuf, sizeof(ibuf), "%zu", _index);

    std::string out = pattern;

    out = internal::replace_all_helper(
        out, "{module}",
        internal::sanitize_file_component_helper(_module));
    out = internal::replace_all_helper(
        out, "{title}",
        internal::sanitize_file_component_helper(_title));
    out = internal::replace_all_helper(out, "{index}", std::string(ibuf));
    out = internal::replace_all_helper(out, "{date}",  date);
    out = internal::replace_all_helper(out, "{time}",  time);
    out = internal::replace_all_helper(out, "{ext}",   _ext);

    return out;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_REPORT_
