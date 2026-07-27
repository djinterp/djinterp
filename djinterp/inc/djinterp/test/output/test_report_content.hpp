/******************************************************************************
* djinterp [test]                                        test_report_content.hpp
*
*   The CONTENT half of the layout-driven report render: what a body_ref leaf
* named by the report's layout term resolves to.  The layout term
* (test_layout.hpp) carries only STRUCTURE -- a cover, a toc, a section per
* module, a section per unit -- and names its content by key.  This header binds
* those keys to render_actions that drive a document_renderer, and it is the only
* place in the render path that knows what a report_check looks like.
*
*   WHY IT IS ITS OWN HEADER.  Everything here is plain document_renderer calls
* over the report model: no expression term, no cofree, no interpreter.  That
* makes it independently compilable and independently testable, which the
* structural half is not -- and it is where all the DTest-specific detail lives,
* so it is what a maintainer edits.  Splitting the two along that line keeps the
* editable part cheap to verify.
*
*   WHAT REPLACES WHAT.  test_render.hpp bound each {token} a per-format
* report_layout literal could name to a projection over the current focus, and
* walked the report four levels deep emitting those literals into a sink.
* test_render_pdf.hpp did the same walk again against a styled-op list.  Both
* traversals disappear: the walk is now the interpreter's single fold over the
* term, and per-format literals are gone entirely -- a renderer decides
* realisation, so there is nothing left to vary per format.  The PROJECTIONS
* survive, as the bodies of the actions below; that part was always the useful
* half.
*
*   THE KEY SCHEME is positional and stable:
*       "run.summary"        the whole-run figure block
*       "module.<m>.summary" one module's figures
*       "unit.<m>.<u>"       one unit's assertion table
*   test_layout.hpp mints the same keys when it folds the report, so the two
*   halves agree by construction rather than by convention.  An unbound key is a
*   graceful hole (the interpreter's contract), not a crash -- so a term that
*   names content this header does not supply still renders.
*
*   ASSERTION TABLES go through the document_renderer table protocol, so one
* emission serves monospace, Markdown, XML, HTML and PDF.  The `style` hint each
* RESULT cell carries is what a PDF or HTML dialect turns into a coloured
* verdict; a plain one drops it.  This single emission is what replaced both the
* monospace-block table arm and the hand-painted ruled one.
*
*   PORTABILITY:
*   C++11 baseline (document_renderer's floor); no dependency on the expression
* or interpreter layers.
*
*
* TABLE OF CONTENTS
* =================
* I.    KEYS                        (mint / match the content-reference names)
* II.   FORMATTING                  (status text, figures)
* III.  EMITTERS                    (unit table / module + run summary)
* IV.   BINDING                     (report_binds_content /
*                                   resolve_report_content)
*
*
* path:      /inc/djinterp/test/output/test_report_content.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TEST_REPORT_CONTENT_
#define DJINTERP_TEST_REPORT_CONTENT_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../core/djinterp.hpp"                  // NS_*, D_NODISCARD, gates
#include "../../core/util/document/templates/document_renderer.hpp"
                                                    // document_renderer
#include "../../core/util/document/templates/document_attributes.hpp"
                                                    // doc_attributes, doc_attr_*
#include "./test_report.hpp"                        // test_report + the model


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   KEYS                                                ///
///////////////////////////////////////////////////////////////////////////////

// content_key_run_summary
//   function: the key naming the whole-run figure block.
D_NODISCARD inline std::string
content_key_run_summary()
{
    return std::string("run.summary");
}

// content_key_module_summary
//   function: the key naming module _m's figure block.
D_NODISCARD inline std::string
content_key_module_summary(
    std::size_t _m
)
{
    return std::string("module.") + std::to_string(_m) + ".summary";
}

// content_key_unit
//   function: the key naming unit _u of module _m -- its assertion table.
D_NODISCARD inline std::string
content_key_unit(
    std::size_t _m,
    std::size_t _u
)
{
    return std::string("unit.") + std::to_string(_m) + "." +
           std::to_string(_u);
}


///////////////////////////////////////////////////////////////////////////////
///                II.  FORMATTING                                          ///
///////////////////////////////////////////////////////////////////////////////

// status_text
//   function: a check's status as a short column token.
D_NODISCARD inline const char*
status_text(
    test_status _status
) D_NOEXCEPT
{
    switch (_status)
    {
        case test_status::passed:  { return "PASS"; }
        case test_status::failed:  { return "FAIL"; }
        case test_status::skipped: { return "SKIP"; }
        case test_status::pending: { return "....."; }
        case test_status::error:   { return "ERROR"; }
    }

    return "?";
}

// status_style
//   function: the style NAME a status carries, for a dialect with a style
// registry (PDF's register_style, HTML's class).  A dialect without one ignores
// it, which is the whole of the hint contract.
D_NODISCARD inline const char*
status_style(
    test_status _status
) D_NOEXCEPT
{
    switch (_status)
    {
        case test_status::passed:  { return "verdict.pass";  }
        case test_status::failed:  { return "verdict.fail";  }
        case test_status::skipped: { return "verdict.skip";  }
        case test_status::pending: { return "verdict.pend";  }
        case test_status::error:   { return "verdict.error"; }
    }

    return "";
}

// tally_text
//   function: "passed / total" -- the figure every summary block repeats.
D_NODISCARD inline std::string
tally_text(
    std::size_t _passed,
    std::size_t _total
)
{
    return std::to_string(_passed) + " / " + std::to_string(_total);
}


///////////////////////////////////////////////////////////////////////////////
///                III. EMITTERS                                            ///
///////////////////////////////////////////////////////////////////////////////

// emit_unit_table
//   function: one unit's assertions as a table -- RESULT / # / ASSERTION /
// EXPECTED / ACTUAL.  A check carrying no expression falls back to its
// description in the ASSERTION column and leaves the value columns blank, so a
// bare pass/fail leaf degrades cleanly and a value-recording one fills in.
//
//   The table goes through the renderer's table protocol, so the SAME call
// sequence becomes aligned monospace, a GFM pipe table, an XML <table>, real
// HTML, or a PDF block.  Nothing here is per-format.
inline void
emit_unit_table(
    const report_unit& _unit,
    document_renderer& _renderer
)
{
    doc_attributes _centre;
    doc_attributes _right;

    _centre.set(doc_attr_align, std::string("center"));
    _right.set(doc_attr_align, std::string("right"));

    // a unit's descriptor is prose above its assertions; the fold gives the
    // unit no sibling node for it, so it rides here
    if (!_unit.description.empty())
    {
        _renderer.paragraph(_unit.description, doc_attributes());
    }

    _renderer.begin_table(doc_attributes());

    _renderer.table_column(std::string("RESULT"),    _centre);
    _renderer.table_column(std::string("#"),         _right);
    _renderer.table_column(std::string("ASSERTION"), doc_attributes());
    _renderer.table_column(std::string("EXPECTED"),  doc_attributes());
    _renderer.table_column(std::string("ACTUAL"),    doc_attributes());

    for (std::size_t _i = 0; _i < _unit.checks.size(); ++_i)
    {
        const report_check& _check = _unit.checks[_i];

        doc_attributes _result_attrs;

        // the status chip carries its style name; a plain dialect drops it
        _result_attrs.set(doc_attr_style,
                          std::string(status_style(_check.status)));
        _result_attrs.set(doc_attr_align, std::string("center"));

        _renderer.begin_row(doc_attributes());

        _renderer.cell(std::string(status_text(_check.status)), _result_attrs);
        _renderer.cell(std::to_string(_i + std::size_t(1)), _right);

        // an assertion with no recorded expression shows its description
        _renderer.cell(_check.expression.empty() ? _check.description
                                                 : _check.expression,
                       doc_attributes());

        _renderer.cell(_check.expected, doc_attributes());
        _renderer.cell(_check.actual,   doc_attributes());

        _renderer.end_row();
    }

    _renderer.end_table();

    return;
}


// emit_module_summary
//   function: one module's figures as labelled values.
inline void
emit_module_summary(
    const report_module& _module,
    document_renderer&   _renderer
)
{
    // a module's description is prose above its figures, when it has one
    if (!_module.description.empty())
    {
        _renderer.paragraph(_module.description, doc_attributes());
    }

    _renderer.key_value(
        std::string("Tests"),
        std::to_string(_module.units.size()),
        doc_attributes());

    _renderer.key_value(
        std::string("Assertions"),
        tally_text(static_cast<std::size_t>(_module.passed_checks()),
                   static_cast<std::size_t>(_module.total_checks())),
        doc_attributes());

    return;
}


// emit_run_summary
//   function: the whole-run figure block -- what the console tally and the
// document have always had to agree on, now emitted once.
inline void
emit_run_summary(
    const test_report& _report,
    document_renderer& _renderer
)
{
    const std::size_t _total  =
        static_cast<std::size_t>(_report.total_checks());
    const std::size_t _passed =
        static_cast<std::size_t>(_report.passed_checks());

    if (!_report.description.empty())
    {
        _renderer.paragraph(_report.description, doc_attributes());
    }

    _renderer.key_value(std::string("Modules"),
                        std::to_string(_report.modules.size()),
                        doc_attributes());

    _renderer.key_value(std::string("Assertions"),
                        tally_text(_passed, _total),
                        doc_attributes());

    doc_attributes _verdict_attrs;

    // the overall verdict wears the same style vocabulary as a check's chip
    _verdict_attrs.set(
        doc_attr_style,
        std::string((_passed == _total) ? status_style(test_status::passed)
                                        : status_style(test_status::failed)));

    _renderer.key_value(
        std::string("Verdict"),
        std::string((_passed == _total) ? "PASSED" : "FAILED"),
        _verdict_attrs);

    if (!_report.notes.empty())
    {
        _renderer.paragraph(_report.notes, doc_attributes());
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  BINDING                                             ///
///////////////////////////////////////////////////////////////////////////////

// report_binds_content
//   function: whether _report binds _key at all -- the cheap predicate the
// resolver asks before committing to an emission, so bindedness is decided
// without rendering anything.
D_NODISCARD inline bool
report_binds_content(
    const test_report& _report,
    const std::string& _key
)
{
    if (_key == content_key_run_summary())
    {
        return true;
    }

    for (std::size_t _m = 0; _m < _report.modules.size(); ++_m)
    {
        if (_key == content_key_module_summary(_m))
        {
            return true;
        }

        for (std::size_t _u = 0;
             _u < _report.modules[_m].units.size();
             ++_u)
        {
            if (_key == content_key_unit(_m, _u))
            {
                return true;
            }
        }
    }

    return false;
}


// resolve_report_content
//   function: emit the content named by _key against _report, reporting whether
// the key was bound.  This is the whole body of the content_resolver
// test_layout.hpp hands the interpreter -- kept as a plain function (rather than
// a closure) so it is callable, and testable, without the interpreter present.
//
//   An unknown key returns false and emits nothing; the interpreter turns that
// into a graceful hole.
D_NODISCARD inline bool
resolve_report_content(
    const test_report& _report,
    const std::string& _key,
    document_renderer& _renderer
)
{
    // the whole-run block
    if (_key == content_key_run_summary())
    {
        emit_run_summary(_report, _renderer);

        return true;
    }

    // module / unit keys are positional: walk the model and compare
    for (std::size_t _m = 0; _m < _report.modules.size(); ++_m)
    {
        if (_key == content_key_module_summary(_m))
        {
            emit_module_summary(_report.modules[_m], _renderer);

            return true;
        }

        for (std::size_t _u = 0;
             _u < _report.modules[_m].units.size();
             ++_u)
        {
            if (_key == content_key_unit(_m, _u))
            {
                emit_unit_table(_report.modules[_m].units[_u], _renderer);

                return true;
            }
        }
    }

    return false;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_REPORT_CONTENT_
