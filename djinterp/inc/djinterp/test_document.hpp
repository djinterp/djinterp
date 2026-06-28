/******************************************************************************
* djinterp [test]                                            test_document.hpp
*
*   The DTest face of the document stack: a baseline binding_env<test_context>
* whose projections READ a test_report (one key per quantity any standard
* report draws), plus three section<test_context>s that iterate the report
* at the three natural depths (modules, units within a module, checks within
* a unit), plus two generic-target render functions that compose the env and
* the sections into a full report on either a tree (document_writer-shaped)
* or a flow (pdf_template-shaped) target.
*
*   WHAT THIS LAYER IS:
*     test_document  = baseline KEY NAMESPACE
*                    + the PROJECTIONS that read a test_report through that
*                      namespace
*                    + the SECTIONS that iterate the report
*                    + the standard PER-FORMAT SKELETONS over those sections
*
*   GREATEST COMMON SUBSET OF KEYS:
*   The baseline registers the keys every output format (txt / md / xml / html
* / pdf) can render as plain text - identities, descriptions, tallies, pass-
* rate strings, verdict words.  Format-specific decoration (PDF's verdict
* COLOR, an HTML <strong> tag) belongs to the SKELETON of that format, not
* to a projection: a projection always yields a fragment_type (std::string).
* That is what lets the same env drive every format without per-format
* branching inside a projection.
*
*   EXTENSIBILITY (THE CUSTOM test_kind PATH):
*   A custom test_kind extends the run by binding new keys to new projections.
* doc.env().bind("benchmark_ns", _read_from_kind_metadata) adds a key without
* touching any format, any section, or the engine - existing skeletons keep
* drawing the keys they always drew; new skeletons can reference the new key.
* This is the design's "one program, many interpreters" property realized on
* the binding seam.
*
*   PER-FORMAT SKELETONS ARE TEMPLATES:
*   render_run_tree<TreeTarget> calls .open_child(name) / .text(value) on its
* target; render_run_flow<FlowTarget> calls .add_text(value) / .add_vspace(p)
* / .add_page_break() on its target.  This header therefore does NOT depend
* on document_writer or pdf_template - the surface each skeleton requires is
* checked at the call site.  Real document_writer::cursor and real
* pdf_template both satisfy the duck-typed shape; so does any user's own
* sink.  The ACTUAL placement (how a tree open_child links, how a flow's
* page break tracks the cursor) lives in the target type's implementation,
* not here.
*
*   THE INVARIANT THE DESIGN TURNS ON (RESTATED AT THE SKELETON LEVEL):
*   For any given (env, ctx, key) the FRAGMENT yielded by env.project(key,
* ctx) is the same whether the body that asks for it places it as a subtree
* or as a flow line.  Both render_run_tree and render_run_flow READ the env
* identically; only their placement of the resulting fragments differs.
* T1's verification asserts this at the section kernel level; this header
* USES that property at the document level.
*
*   PORTABILITY:
*   C++17 (binding_env / section are C++17); self-suppresses below it.
*   No dependency on document_writer, pdf_template, or any other format
* header - test_document is the FORMAT-AGNOSTIC DTest layer.
*
*
* TABLE OF CONTENTS
* =================
* I.    HELPERS                       (size_str / elapsed)
* II.   TEST DOCUMENT                 (the class: env + sections)
* III.  STANDARD SECTIONS             (per_module / per_unit / per_check)
* IV.   BASELINE PROJECTIONS          (the env-population routine)
* V.    PER-FORMAT SKELETONS          (render_run_tree / render_run_flow)
*
*
* path:      /inc/djinterp/test/test_document.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_DOCUMENT_
#define DJINTERP_TEST_TEST_DOCUMENT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"                 // NS_*, D_NODISCARD, gates
#include "../core/text/binding_env.hpp"         // binding_env<_Ctx>
#include "../core/text/section.hpp"             // section<_Ctx>, make_section
#include "./test_context.hpp"                   // test_context
#include "./test_report.hpp"                    // test_report, report_*,
                                                //   test_status,
                                                //   report_pass_rate,
                                                //   report_ratio,
                                                //   report_verdict_word


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL


// size_str_helper
//   helper: a portable "%zu" decimal of _n.  Used by the count projections so
// every numeric key yields a plain string.
D_NODISCARD D_INLINE std::string
size_str_helper(
    std::size_t _n
)
{
    char _buf[32];

    std::snprintf(_buf, sizeof(_buf), "%zu", _n);

    return std::string(_buf);
}


// status_word_helper
//   helper: the uppercase PASS / FAIL / SKIP / PEND / ERR word for a check
// status.  The "common subset" form - colorization is a per-format skeleton
// concern, not a projection one.
D_NODISCARD D_INLINE const char*
status_word_helper(
    test_status _s
) D_NOEXCEPT
{
    switch (_s)
    {
        case test_status::passed:  { return "PASS"; }
        case test_status::failed:  { return "FAIL"; }
        case test_status::skipped: { return "SKIP"; }
        case test_status::pending: { return "PEND"; }
        case test_status::error:   { return "ERR";  }
        default:                   { return "????"; }
    }
}


// status_symbol_helper
//   helper: the bracketed marker for a check status - the "[PASS] ..."
// form the existing text and PDF reports already use.
D_NODISCARD D_INLINE const char*
status_symbol_helper(
    test_status _s
) D_NOEXCEPT
{
    switch (_s)
    {
        case test_status::passed:  { return "[PASS]"; }
        case test_status::failed:  { return "[FAIL]"; }
        case test_status::skipped: { return "[SKIP]"; }
        case test_status::pending: { return "[....]"; }
        case test_status::error:   { return "[ERR!]"; }
        default:                   { return "[????]"; }
    }
}


// elapsed_str_helper
//   helper: a "12.34 ms" / "456 us" / "78 ns" rendering of _ns.  Auto-scales
// to the largest unit whose magnitude is >= 1.  Sub-microsecond renders as
// integer nanoseconds; larger units carry two decimal places.  Matches the
// existing test_printer / test_pdf_report elapsed formatter.
D_NODISCARD D_INLINE std::string
elapsed_str_helper(
    std::int64_t _ns
)
{
    char         _buf[32];
    bool         _neg = (_ns < 0);
    std::int64_t _mag = _neg ? -_ns : _ns;

    if (_mag < 1000)
    {
        std::snprintf(_buf, sizeof(_buf), "%s%lld ns",
                      _neg ? "-" : "",
                      static_cast<long long>(_mag));

        return std::string(_buf);
    }

    if (_mag < 1000000)
    {
        std::snprintf(_buf, sizeof(_buf), "%s%.2f us",
                      _neg ? "-" : "",
                      static_cast<double>(_mag) / 1000.0);

        return std::string(_buf);
    }

    if (_mag < 1000000000)
    {
        std::snprintf(_buf, sizeof(_buf), "%s%.2f ms",
                      _neg ? "-" : "",
                      static_cast<double>(_mag) / 1000000.0);

        return std::string(_buf);
    }

    std::snprintf(_buf, sizeof(_buf), "%s%.2f s",
                  _neg ? "-" : "",
                  static_cast<double>(_mag) / 1000000000.0);

    return std::string(_buf);
}


NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DOCUMENT                                        ///
///////////////////////////////////////////////////////////////////////////////

// test_document
//   class: the DTest baseline.  A populated binding_env<test_context> +
// three pre-built section<test_context>s + standard per-format render
// entries (free functions below).  Construct once per process (or per
// renderer); render many times against many run focuses.
//
//   The env is exposed by REFERENCE through env() so callers can extend it
// (the test_kind path); the sections are exposed by const reference because
// the standard iteration topology - "modules over the run, units over a
// module, checks over a unit" - is fixed.  Add a section locally if a custom
// kind needs a different walk; the env is shared.
//
// Usage:
//   test_document     _doc;                                    // built once
//   test_context      _here = at_run(&_my_run);                // per render
//   render_run_tree(_doc, _here, _cursor);                     // tree face
//   render_run_flow(_doc, _here, _pdf_template);               // flow face
//   _doc.env().bind("custom_metric", _read_kind_metadata);     // extension
class test_document
{
public:
    // -- public type aliases -------------------------------------------------

    // context_type
    //   type: the focus this document layer reads from.
    using context_type = test_context;

    // env_type
    //   type: the binding_env this document populates.
    using env_type = ::djinterp::binding_env<test_context>;

    // section_type
    //   type: a section<test_context>, the iteration combinator at this layer.
    using section_type = ::djinterp::section<test_context>;

    // test_document
    //   constructor: installs the baseline env (every projection in the
    // standard key namespace) and the three standard sections.
    test_document()
    {
        m_install_baseline_env();
        m_install_baseline_sections();
    }

    // env
    //   the binding_env, by reference so callers can extend it with new
    // projections (a custom test_kind binds its keys here).
    D_NODISCARD env_type&
    env() D_NOEXCEPT
    {
        return m_env;
    }

    D_NODISCARD const env_type&
    env() const D_NOEXCEPT
    {
        return m_env;
    }

    // per_module
    //   the standard "for each module in run.modules" section, refocusing the
    // ctx so module_ + module_index are populated for the body.
    D_NODISCARD const section_type&
    per_module() const D_NOEXCEPT
    {
        return m_per_module;
    }

    // per_unit_in_module
    //   the standard "for each unit in module.units" section, refocusing the
    // ctx so unit + unit_index are populated for the body.  Requires ctx
    // already focused at a module.
    D_NODISCARD const section_type&
    per_unit_in_module() const D_NOEXCEPT
    {
        return m_per_unit_in_module;
    }

    // per_check_in_unit
    //   the standard "for each check in unit.checks" section, refocusing the
    // ctx so check + check_index are populated for the body.  Requires ctx
    // already focused at a unit.
    D_NODISCARD const section_type&
    per_check_in_unit() const D_NOEXCEPT
    {
        return m_per_check_in_unit;
    }

private:

    void m_install_baseline_env();
    void m_install_baseline_sections();

    env_type     m_env;
    section_type m_per_module;
    section_type m_per_unit_in_module;
    section_type m_per_check_in_unit;
};


///////////////////////////////////////////////////////////////////////////////
///                III. STANDARD SECTIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

inline void
test_document::m_install_baseline_sections()
{
    // per_module: iterate run->modules; refocus ctx.module_ + module_index
    m_per_module = ::djinterp::make_section<test_context>(
        // count
        [](const test_context& _c) -> std::size_t
        {
            return (_c.run != nullptr) ? _c.run->modules.size() : 0;
        },
        // refocus
        [](const test_context& _c, std::size_t _i) -> test_context
        {
            test_context _r = _c;
            _r.module_      = &_c.run->modules[_i];
            _r.module_index = _i + 1;
            _r.unit         = nullptr;
            _r.unit_index   = 0;
            _r.check        = nullptr;
            _r.check_index  = 0;

            return _r;
        });

    // per_unit_in_module: iterate ctx.module_->units; refocus ctx.unit
    m_per_unit_in_module = ::djinterp::make_section<test_context>(
        // count
        [](const test_context& _c) -> std::size_t
        {
            return (_c.module_ != nullptr) ? _c.module_->units.size() : 0;
        },
        // refocus
        [](const test_context& _c, std::size_t _i) -> test_context
        {
            test_context _r = _c;
            _r.unit         = &_c.module_->units[_i];
            _r.unit_index   = _i + 1;
            _r.check        = nullptr;
            _r.check_index  = 0;

            return _r;
        });

    // per_check_in_unit: iterate ctx.unit->checks; refocus ctx.check
    m_per_check_in_unit = ::djinterp::make_section<test_context>(
        // count
        [](const test_context& _c) -> std::size_t
        {
            return (_c.unit != nullptr) ? _c.unit->checks.size() : 0;
        },
        // refocus
        [](const test_context& _c, std::size_t _i) -> test_context
        {
            test_context _r = _c;
            _r.check        = &_c.unit->checks[_i];
            _r.check_index  = _i + 1;

            return _r;
        });

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  BASELINE PROJECTIONS                                 ///
///////////////////////////////////////////////////////////////////////////////
//
//   Every projection takes a const test_context& and returns std::string.
// Missing parts (null pointers) yield empty - the lenient missing-source
// rule.  Projections READ from the ctx; they NEVER capture state of their
// own beyond the function pointer itself, so the env can be shared across
// threads provided the test_report it is read against is not mutated under
// the read.
//
// Key namespace, by depth:
//
//   run-level (require ctx.run != null):
//     title / subtitle / author / description / notes
//     total_modules / passed_modules / failed_modules
//     total_units   / passed_units   / failed_units
//     total_checks  / passed_checks  / failed_checks
//     modules_ratio / units_ratio    / checks_ratio
//     modules_pass_rate / units_pass_rate / checks_pass_rate
//     verdict       / verdict_word
//
//   module-level (require ctx.module_ != null):
//     module_index
//     module_name / module_description
//     module_total_units / module_passed_units / module_failed_units
//     module_total_checks / module_passed_checks / module_failed_checks
//     module_units_ratio / module_checks_ratio
//     module_units_pass_rate / module_checks_pass_rate
//     module_verdict / module_verdict_word
//
//   unit-level (require ctx.unit != null):
//     unit_index
//     unit_name
//     unit_total_checks / unit_passed_checks / unit_failed_checks
//     unit_checks_ratio / unit_checks_pass_rate
//     unit_verdict / unit_verdict_word
//     unit_elapsed
//
//   check-level (require ctx.check != null):
//     check_index
//     check_description
//     check_status_word / check_status_symbol
//
//   A test_kind that adds keys appends them through env.bind(...) - it does
// not edit this routine.

inline void
test_document::m_install_baseline_env()
{
    using std::string;
    using internal::size_str_helper;
    using internal::status_word_helper;
    using internal::status_symbol_helper;
    using internal::elapsed_str_helper;

    // -- run-level metadata --------------------------------------------------

    m_env.bind("title",
               [](const test_context& _c) -> string
               { return (_c.run != nullptr) ? _c.run->title : string(); });

    m_env.bind("subtitle",
               [](const test_context& _c) -> string
               { return (_c.run != nullptr) ? _c.run->subtitle : string(); });

    m_env.bind("author",
               [](const test_context& _c) -> string
               { return (_c.run != nullptr) ? _c.run->author : string(); });

    m_env.bind("description",
               [](const test_context& _c) -> string
               { return (_c.run != nullptr) ? _c.run->description : string(); });

    m_env.bind("notes",
               [](const test_context& _c) -> string
               { return (_c.run != nullptr) ? _c.run->notes : string(); });

    // -- run-level tallies (counts) -----------------------------------------

    m_env.bind("total_modules",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->total_modules())
                       : string();
               });

    m_env.bind("passed_modules",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->passed_modules())
                       : string();
               });

    m_env.bind("failed_modules",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->failed_modules())
                       : string();
               });

    m_env.bind("total_units",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->total_units())
                       : string();
               });

    m_env.bind("passed_units",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->passed_units())
                       : string();
               });

    m_env.bind("failed_units",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->failed_units())
                       : string();
               });

    m_env.bind("total_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->total_checks())
                       : string();
               });

    m_env.bind("passed_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->passed_checks())
                       : string();
               });

    m_env.bind("failed_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.run != nullptr)
                       ? size_str_helper(_c.run->failed_checks())
                       : string();
               });

    // -- run-level pass-rate / ratio strings --------------------------------

    m_env.bind("modules_ratio",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return report_ratio(_c.run->passed_modules(),
                                       _c.run->total_modules());
               });

    m_env.bind("units_ratio",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return report_ratio(_c.run->passed_units(),
                                       _c.run->total_units());
               });

    m_env.bind("checks_ratio",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return report_ratio(_c.run->passed_checks(),
                                       _c.run->total_checks());
               });

    m_env.bind("modules_pass_rate",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return report_pass_rate(_c.run->passed_modules(),
                                           _c.run->total_modules());
               });

    m_env.bind("units_pass_rate",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return report_pass_rate(_c.run->passed_units(),
                                           _c.run->total_units());
               });

    m_env.bind("checks_pass_rate",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return report_pass_rate(_c.run->passed_checks(),
                                           _c.run->total_checks());
               });

    // -- run-level verdict --------------------------------------------------

    m_env.bind("verdict",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return string(report_verdict_word(_c.run->verdict()));
               });

    m_env.bind("verdict_word",
               [](const test_context& _c) -> string
               {
                   if (_c.run == nullptr) return string();
                   return string(report_verdict_word(_c.run->verdict()));
               });

    // -- module-level -------------------------------------------------------

    m_env.bind("module_index",
               [](const test_context& _c) -> string
               { return size_str_helper(_c.module_index); });

    m_env.bind("module_name",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? _c.module_->name : string();
               });

    m_env.bind("module_description",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? _c.module_->description : string();
               });

    m_env.bind("module_total_units",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? size_str_helper(_c.module_->total_units())
                       : string();
               });

    m_env.bind("module_passed_units",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? size_str_helper(_c.module_->passed_units())
                       : string();
               });

    m_env.bind("module_failed_units",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? size_str_helper(_c.module_->failed_units())
                       : string();
               });

    m_env.bind("module_total_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? size_str_helper(_c.module_->total_checks())
                       : string();
               });

    m_env.bind("module_passed_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? size_str_helper(_c.module_->passed_checks())
                       : string();
               });

    m_env.bind("module_failed_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.module_ != nullptr)
                       ? size_str_helper(_c.module_->failed_checks())
                       : string();
               });

    m_env.bind("module_units_ratio",
               [](const test_context& _c) -> string
               {
                   if (_c.module_ == nullptr) return string();
                   return report_ratio(_c.module_->passed_units(),
                                       _c.module_->total_units());
               });

    m_env.bind("module_checks_ratio",
               [](const test_context& _c) -> string
               {
                   if (_c.module_ == nullptr) return string();
                   return report_ratio(_c.module_->passed_checks(),
                                       _c.module_->total_checks());
               });

    m_env.bind("module_units_pass_rate",
               [](const test_context& _c) -> string
               {
                   if (_c.module_ == nullptr) return string();
                   return report_pass_rate(_c.module_->passed_units(),
                                           _c.module_->total_units());
               });

    m_env.bind("module_checks_pass_rate",
               [](const test_context& _c) -> string
               {
                   if (_c.module_ == nullptr) return string();
                   return report_pass_rate(_c.module_->passed_checks(),
                                           _c.module_->total_checks());
               });

    m_env.bind("module_verdict",
               [](const test_context& _c) -> string
               {
                   if (_c.module_ == nullptr) return string();
                   return string(report_verdict_word(_c.module_->verdict()));
               });

    m_env.bind("module_verdict_word",
               [](const test_context& _c) -> string
               {
                   if (_c.module_ == nullptr) return string();
                   return string(report_verdict_word(_c.module_->verdict()));
               });

    // -- unit-level ---------------------------------------------------------

    m_env.bind("unit_index",
               [](const test_context& _c) -> string
               { return size_str_helper(_c.unit_index); });

    m_env.bind("unit_name",
               [](const test_context& _c) -> string
               {
                   return (_c.unit != nullptr) ? _c.unit->name : string();
               });

    m_env.bind("unit_total_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.unit != nullptr)
                       ? size_str_helper(_c.unit->total_checks())
                       : string();
               });

    m_env.bind("unit_passed_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.unit != nullptr)
                       ? size_str_helper(_c.unit->passed_checks())
                       : string();
               });

    m_env.bind("unit_failed_checks",
               [](const test_context& _c) -> string
               {
                   return (_c.unit != nullptr)
                       ? size_str_helper(_c.unit->failed_checks())
                       : string();
               });

    m_env.bind("unit_checks_ratio",
               [](const test_context& _c) -> string
               {
                   if (_c.unit == nullptr) return string();
                   return report_ratio(_c.unit->passed_checks(),
                                       _c.unit->total_checks());
               });

    m_env.bind("unit_checks_pass_rate",
               [](const test_context& _c) -> string
               {
                   if (_c.unit == nullptr) return string();
                   return report_pass_rate(_c.unit->passed_checks(),
                                           _c.unit->total_checks());
               });

    m_env.bind("unit_verdict",
               [](const test_context& _c) -> string
               {
                   if (_c.unit == nullptr) return string();
                   return string(report_verdict_word(_c.unit->verdict));
               });

    m_env.bind("unit_verdict_word",
               [](const test_context& _c) -> string
               {
                   if (_c.unit == nullptr) return string();
                   return string(report_verdict_word(_c.unit->verdict));
               });

    m_env.bind("unit_elapsed",
               [](const test_context& _c) -> string
               {
                   if (_c.unit == nullptr) return string();
                   return elapsed_str_helper(_c.unit->elapsed_ns);
               });

    // -- check-level --------------------------------------------------------

    m_env.bind("check_index",
               [](const test_context& _c) -> string
               { return size_str_helper(_c.check_index); });

    m_env.bind("check_description",
               [](const test_context& _c) -> string
               {
                   return (_c.check != nullptr)
                       ? _c.check->description : string();
               });

    m_env.bind("check_status_word",
               [](const test_context& _c) -> string
               {
                   if (_c.check == nullptr) return string();
                   return string(status_word_helper(_c.check->status));
               });

    m_env.bind("check_status_symbol",
               [](const test_context& _c) -> string
               {
                   if (_c.check == nullptr) return string();
                   return string(status_symbol_helper(_c.check->status));
               });

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   PER-FORMAT SKELETONS                                 ///
///////////////////////////////////////////////////////////////////////////////
//
//   These two function templates ARE the standard renderings of a DTest run.
// Each is parameterized by its target type - any tree-shaped target with
// .open_child(name) / .text(value) for the tree face; any flow-shaped target
// with .add_text(value) / .add_vspace(amount) / .add_page_break() for the
// flow face.  Concrete document_writer::cursor (xml.hpp's child of the doc
// tree) and concrete pdf_template both satisfy these shapes, as do stand-in
// targets the harness uses for isolation testing.
//
//   The two skeletons read THE SAME env and THE SAME sections; they differ
// only in where they put the fragments.  That is the property T1 verified
// at the kernel level and that this layer USES at the document level.


// render_unit_tree
//   function: appends a single unit subtree under _unit_head, drawing every
// fragment from _doc.env() at the unit-focused ctx.  Iterates the unit's
// checks via _doc.per_check_in_unit().  A helper for render_module_tree.
template<typename _TreeTarget>
inline void
render_unit_tree(
    const test_document& _doc,
    const test_context&  _unit_ctx,
    _TreeTarget&         _unit_head
)
{
    const auto& _env = _doc.env();

    _unit_head.open_child("name")
        .text(_env.project("unit_name", _unit_ctx));
    _unit_head.open_child("verdict")
        .text(_env.project("unit_verdict_word", _unit_ctx));
    _unit_head.open_child("checks_ratio")
        .text(_env.project("unit_checks_ratio", _unit_ctx));

    _TreeTarget _checks_head = _unit_head.open_child("checks");

    _doc.per_check_in_unit().for_each(
        _env, _unit_ctx, _checks_head,
        [](const ::djinterp::binding_env<test_context>& _e,
           const test_context&                          _check_ctx,
           _TreeTarget&                                 _ch)
        {
            _TreeTarget _one = _ch.open_child("check");
            _one.open_child("status")
                .text(_e.project("check_status_word", _check_ctx));
            _one.open_child("description")
                .text(_e.project("check_description", _check_ctx));
        });

    return;
}


// render_module_tree
//   function: appends a single module subtree under _module_head, iterating
// the module's units via _doc.per_unit_in_module().  A helper for
// render_run_tree.
template<typename _TreeTarget>
inline void
render_module_tree(
    const test_document& _doc,
    const test_context&  _module_ctx,
    _TreeTarget&         _module_head
)
{
    const auto& _env = _doc.env();

    _module_head.open_child("name")
        .text(_env.project("module_name", _module_ctx));
    _module_head.open_child("description")
        .text(_env.project("module_description", _module_ctx));
    _module_head.open_child("verdict")
        .text(_env.project("module_verdict_word", _module_ctx));
    _module_head.open_child("units_ratio")
        .text(_env.project("module_units_ratio", _module_ctx));
    _module_head.open_child("checks_ratio")
        .text(_env.project("module_checks_ratio", _module_ctx));

    _TreeTarget _units_head = _module_head.open_child("units");

    _doc.per_unit_in_module().for_each(
        _env, _module_ctx, _units_head,
        [&_doc](const ::djinterp::binding_env<test_context>&,
                const test_context& _unit_ctx,
                _TreeTarget&        _uh)
        {
            _TreeTarget _one = _uh.open_child("unit");
            render_unit_tree(_doc, _unit_ctx, _one);
        });

    return;
}


// render_run_tree
//   function: the standard tree rendering of a whole run.  _Root_head is the
// caller's tree head (typically a document_writer cursor on the root
// element); this fills it with the canonical <report> shape.  Drives the
// per_module section to spawn one module subtree per module.
template<typename _TreeTarget>
inline void
render_run_tree(
    const test_document& _doc,
    const test_context&  _run_ctx,
    _TreeTarget&         _root_head
)
{
    const auto& _env = _doc.env();

    // run-level header (drawn once, not iterated)
    _root_head.open_child("title")
        .text(_env.project("title", _run_ctx));
    _root_head.open_child("subtitle")
        .text(_env.project("subtitle", _run_ctx));
    _root_head.open_child("author")
        .text(_env.project("author", _run_ctx));
    _root_head.open_child("verdict")
        .text(_env.project("verdict_word", _run_ctx));
    _root_head.open_child("modules_ratio")
        .text(_env.project("modules_ratio", _run_ctx));
    _root_head.open_child("units_ratio")
        .text(_env.project("units_ratio", _run_ctx));
    _root_head.open_child("checks_ratio")
        .text(_env.project("checks_ratio", _run_ctx));

    _TreeTarget _modules_head = _root_head.open_child("modules");

    _doc.per_module().for_each(
        _env, _run_ctx, _modules_head,
        [&_doc](const ::djinterp::binding_env<test_context>&,
                const test_context& _module_ctx,
                _TreeTarget&        _mh)
        {
            _TreeTarget _one = _mh.open_child("module");
            render_module_tree(_doc, _module_ctx, _one);
        });

    return;
}


// render_unit_flow
//   function: lays one unit's band - a "--- name ---" header, one line per
// check, a per-unit verdict line.  Reads the same env keys render_unit_tree
// reads; only the placement (a flow band vs a subtree) differs.  A helper
// for render_module_flow.
template<typename _FlowTarget>
inline void
render_unit_flow(
    const test_document& _doc,
    const test_context&  _unit_ctx,
    _FlowTarget&         _flow
)
{
    const auto& _env = _doc.env();

    _flow.add_text(std::string("--- ") +
                   _env.project("unit_name", _unit_ctx) +
                   std::string(" ---"));

    _doc.per_check_in_unit().for_each(
        _env, _unit_ctx, _flow,
        [](const ::djinterp::binding_env<test_context>& _e,
           const test_context&                          _check_ctx,
           _FlowTarget&                                 _f)
        {
            _f.add_text(_e.project("check_status_symbol", _check_ctx) +
                        std::string(" ") +
                        _e.project("check_description", _check_ctx));
        });

    _flow.add_text(std::string("verdict: ") +
                   _env.project("unit_verdict_word", _unit_ctx) +
                   std::string(" (") +
                   _env.project("unit_checks_ratio", _unit_ctx) +
                   std::string(")"));
    _flow.add_vspace(3.0);

    return;
}


// render_module_flow
//   function: lays one module's region - a "MODULE: name" header, the
// description, every unit band, a closing module-results box.  A helper for
// render_run_flow.
template<typename _FlowTarget>
inline void
render_module_flow(
    const test_document& _doc,
    const test_context&  _module_ctx,
    _FlowTarget&         _flow
)
{
    const auto& _env = _doc.env();

    _flow.add_text(std::string("MODULE: ") +
                   _env.project("module_name", _module_ctx));
    _flow.add_text(_env.project("module_description", _module_ctx));
    _flow.add_vspace(3.0);

    _doc.per_unit_in_module().for_each(
        _env, _module_ctx, _flow,
        [&_doc](const ::djinterp::binding_env<test_context>&,
                const test_context& _unit_ctx,
                _FlowTarget&        _f)
        {
            render_unit_flow(_doc, _unit_ctx, _f);
        });

    _flow.add_text(std::string("MODULE RESULTS: ") +
                   _env.project("module_name", _module_ctx));
    _flow.add_text(std::string("Units:      ") +
                   _env.project("module_units_ratio", _module_ctx) +
                   std::string(" (") +
                   _env.project("module_units_pass_rate", _module_ctx) +
                   std::string(")"));
    _flow.add_text(std::string("Assertions: ") +
                   _env.project("module_checks_ratio", _module_ctx) +
                   std::string(" (") +
                   _env.project("module_checks_pass_rate", _module_ctx) +
                   std::string(")"));
    _flow.add_text(std::string("Status:     ") +
                   _env.project("module_verdict_word", _module_ctx));
    _flow.add_vspace(8.0);
    _flow.add_page_break();

    return;
}


// render_run_flow
//   function: the standard flow rendering of a whole run.  _Flow is the
// caller's flow target (typically a pdf_template); this lays the title page,
// every module's band-with-results, and a closing comprehensive block.
template<typename _FlowTarget>
inline void
render_run_flow(
    const test_document& _doc,
    const test_context&  _run_ctx,
    _FlowTarget&         _flow
)
{
    const auto& _env = _doc.env();

    // title page
    _flow.add_text(_env.project("title", _run_ctx));
    _flow.add_text(_env.project("subtitle", _run_ctx));
    _flow.add_text(std::string("Author: ") +
                   _env.project("author", _run_ctx));
    _flow.add_text(std::string("RESULT: ") +
                   _env.project("verdict_word", _run_ctx));
    _flow.add_vspace(8.0);
    _flow.add_page_break();

    // every module band
    _doc.per_module().for_each(
        _env, _run_ctx, _flow,
        [&_doc](const ::djinterp::binding_env<test_context>&,
                const test_context& _module_ctx,
                _FlowTarget&        _f)
        {
            render_module_flow(_doc, _module_ctx, _f);
        });

    // closing comprehensive block
    _flow.add_text(std::string("COMPREHENSIVE TEST RESULTS"));
    _flow.add_text(std::string("Modules:    ") +
                   _env.project("modules_ratio", _run_ctx) +
                   std::string(" (") +
                   _env.project("modules_pass_rate", _run_ctx) +
                   std::string(")"));
    _flow.add_text(std::string("Unit Tests: ") +
                   _env.project("units_ratio", _run_ctx) +
                   std::string(" (") +
                   _env.project("units_pass_rate", _run_ctx) +
                   std::string(")"));
    _flow.add_text(std::string("Assertions: ") +
                   _env.project("checks_ratio", _run_ctx) +
                   std::string(" (") +
                   _env.project("checks_pass_rate", _run_ctx) +
                   std::string(")"));
    _flow.add_text(std::string("Verdict:    ") +
                   _env.project("verdict_word", _run_ctx));

    return;
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_DOCUMENT_
