/******************************************************************************
* djinterp [test]                                            test_render_pdf.hpp
*
*   The PDF arm of the render-collapse - the one target that is NOT a string.
* pdf_template is an element/flow engine (add_text / add_rule / add_vspace /
* add_page_break, then render_pdf() -> bytes) that owns styling, line wrapping,
* and pagination.  So PDF cannot ride report_layout + a string sink; it takes a
* styled-OP layout and a sibling walk.  The TRAVERSAL is identical to
* render_report (run -> modules -> units -> checks, refocusing the resolver) -
* only the emission differs: where the string walk appends to a sink, this one
* resolves each op's text through the SAME test_resolver and calls the template.
* (The two traversals are a candidate to factor into one visitor-driven walk;
* kept separate for now so the string path stays exactly as proven.)
*
*   Isolated from test_render.hpp on purpose: pdf_template drags in the whole
* pdf.hpp tree (printer, document, fonts).  A text / xml / html build includes
* only test_render.hpp and never pays for the PDF engine; render_*_bytes in
* test_output includes this header solely for the pdf case.
*
*   TOKEN HANDLING:  pdf_template resolves each element's format through a
* borrowed text_template at render time; its delimiters ARE { } (the
* D_PDF_TPL_DEFAULT_PREFIX / SUFFIX), it reads {{ }} as literal braces, and -
* confirmed from the text_template source - it maps UNBOUND keys to the empty
* string.  We bind nothing on the template (we resolve eagerly through our own
* resolver), so the brace-escape here is REQUIRED, not belt-and-suspenders: an
* unescaped { } in a resolved value (say a description "handles {0}") would be
* read as a placeholder, found unbound, and DROPPED - rendering "handles ".
* Escaping { -> {{ makes the template emit the literal text.
*
*   STATUS:  reconciled against the real pdf_template.hpp and signature-verified.
* Every call this header makes is matched to that header: add_styled_text(format,
* name) for the by-name style (there is no add_text(format, name); "" / unknown
* falls back to the body style), add_rule(pdf_unit, pdf_color) / add_vspace(
* pdf_unit) / add_page_break(), the register_style + heading_style/body_style
* style API, and render_pdf() -> std::string (converted to byte_buffer here).
* The adapter compiles and emits the correct op sequence against those
* signatures.  A full end-to-end link against the built-in backend additionally
* needs parser.hpp (text_template) and color.hpp (pdf_primitives), which are
* outside this header's concern.
*
*   PORTABILITY:  C++17.
*
*
* path:      /inc/djinterp/test/output/test_render_pdf.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_RENDER_PDF_
#define DJINTERP_TEST_TEST_RENDER_PDF_ 1

// std
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"               // NS_*, D_NODISCARD
#include "../../core/text/pdf/pdf_template.hpp"  // pdf::pdf_template (path: match your tree)
#include "../../core/util/compress.hpp"       // pdf::pdf_template (path: match your tree)
#include "../../core/text/templates/title_page.hpp"            // cover page (template)
#include "../../core/text/templates/document_table.hpp"        // summary table (template)
#include "../../core/text/pdf/pdf_document_renderer.hpp" // document_renderer -> pdf_template
#include "./test_render.hpp"                     // the binding (make_test_resolver) + emit + the walk shape


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   THE STYLED-OP LAYOUT  (data)                         ///
///////////////////////////////////////////////////////////////////////////////

// pdf_op
//   one flow operation.  `text` carries a {token} format string (resolved per
// node through the resolver) and an optional named style; `rule`/`vspace` carry
// an amount; `page_break` carries nothing.  This is the PDF analogue of a
// report_layout literal - data, not code.
struct pdf_op
{
    enum class kind { text, rule, vspace, page_break };

    kind        op    = kind::text;
    std::string format;        // text: the {token} format string
    std::string style_name;    // text/rule: registered style name ("" = default)
    double      amount = 0.0;  // vspace: points; rule: line width
};

using pdf_op_list = std::vector<pdf_op>;

// pdf_layout
//   per-level op lists - the styled-op counterpart of report_layout.  *_open
// runs at that level's focus before descending, *_close after.
struct pdf_layout
{
    pdf_op_list run_open;
    pdf_op_list module_open;
    pdf_op_list unit_open;
    pdf_op_list check_line;
    pdf_op_list unit_close;
    pdf_op_list module_close;
    pdf_op_list run_close;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  EMISSION                                             ///
///////////////////////////////////////////////////////////////////////////////

namespace internal
{
    // brace_escape
    //   {  ->  {{   and   }  ->  }}.  pdf_template resolves each element's format
    // through a text_template at render time, whose delimiters ARE { } (the
    // D_PDF_TPL_DEFAULT_PREFIX / SUFFIX) and which reads {{ }} as literal braces -
    // so escaping our already-resolved value keeps a value that happens to
    // contain { } from being re-interpreted by that pass.
    inline std::string
    brace_escape(std::string_view _s)
    {
        std::string _out;
        _out.reserve(_s.size());
        std::size_t _i = 0;
        for (_i = 0; _i < _s.size(); ++_i)
        {
            const char _c = _s[_i];
            if (_c == '{')      { _out += "{{"; }
            else if (_c == '}') { _out += "}}"; }
            else                { _out += _c; }
        }
        return _out;
    }

    // resolve_text
    //   render one op's {token} format against a focus through OUR resolver,
    // then brace-escape the result for safe placement.
    inline std::string
    resolve_text(const std::string& _format, const test_context& _focus)
    {
        std::string                   _s;
        ::djinterp::interp_string_sink<char> _sink(_s);
        emit(_format, _focus, _sink);           // test_render.hpp: interpolate + make_test_resolver(focus)
        return brace_escape(_s);
    }
}  // namespace internal


// emit_ops
//   apply one op list at one focus to the template.
inline void
emit_ops(
    const pdf_op_list&   _ops,
    const test_context&  _focus,
    pdf_template&        _template
)
{
    std::size_t _i = 0;
    for (_i = 0; _i < _ops.size(); ++_i)
    {
        const pdf_op& _o = _ops[_i];
        switch (_o.op)
        {
            case pdf_op::kind::text:
                // add_styled_text is the by-name overload (there is no
                // add_text(format, name)); an empty or unknown name falls back
                // to the template's body style.
                _template.add_styled_text(internal::resolve_text(_o.format, _focus), _o.style_name);
                break;
            case pdf_op::kind::rule:
                _template.add_rule(_o.amount);
                break;
            case pdf_op::kind::vspace:
                _template.add_vspace(_o.amount);
                break;
            case pdf_op::kind::page_break:
            default:
                _template.add_page_break();
                break;
        }
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                III. THE WALK  (same traversal, op emission)             ///
///////////////////////////////////////////////////////////////////////////////

// render_module_pdf
//   one module subtree into the template - the per-module-document body, shared
// with render_report_pdf exactly as render_module is shared on the string side.
inline void
render_module_pdf(
    const pdf_layout&             _layout,
    const test_context&           _fm,       // focus with run + module_ + module_index set
    pdf_template& _tpl
)
{
    emit_ops(_layout.module_open, _fm, _tpl);

    std::size_t _ui = 0;
    for (_ui = 0; _ui < _fm.module_->units.size(); ++_ui)
    {
        test_context _fu = _fm;
        _fu.unit       = &_fm.module_->units[_ui];
        _fu.unit_index = _ui + 1;

        emit_ops(_layout.unit_open, _fu, _tpl);

        std::size_t _ci = 0;
        for (_ci = 0; _ci < _fu.unit->checks.size(); ++_ci)
        {
            test_context _fc = _fu;
            _fc.check       = &_fu.unit->checks[_ci];
            _fc.check_index = _ci + 1;

            emit_ops(_layout.check_line, _fc, _tpl);
        }

        emit_ops(_layout.unit_close, _fu, _tpl);
    }

    emit_ops(_layout.module_close, _fm, _tpl);

    return;
}


// render_report_pdf
//   the whole-run render into a pdf_template: the traversal of render_report,
// with emit_ops in place of the string sink and render_module_pdf per module.
inline void
render_report_pdf(
    const pdf_layout&             _layout,
    const test_report&            _run,
    pdf_template& _tpl
)
{
    test_context _f;
    _f.run = &_run;

    emit_ops(_layout.run_open, _f, _tpl);

    std::size_t _mi = 0;
    for (_mi = 0; _mi < _run.modules.size(); ++_mi)
    {
        test_context _fm = _f;
        _fm.module_      = &_run.modules[_mi];
        _fm.module_index = _mi + 1;

        render_module_pdf(_layout, _fm, _tpl);
    }

    emit_ops(_layout.run_close, _f, _tpl);

    return;
}


// register_default_styles
//   seat the style names pdf_default_layout references ("heading", "subheading",
// "body") on a template, so add_styled_text resolves them instead of falling
// back to the body style.  "body" is the template default; the other two are
// registered explicitly.  Override by registering your own before rendering, or
// by passing a pre-styled template to render_report_pdf / render_module_pdf.
inline void
register_default_styles(
    pdf_template& _tpl
)
{
    _tpl.register_style("heading", pdf_template::heading_style());

    pdf_text_style _sub(pdf_font(pdf_base_font::helvetica_bold, 12.0));
    _sub.leading = 15.0;
    _tpl.register_style("subheading", _sub);

    _tpl.register_style("body", _tpl.body_style());

    return;
}


// render_report_pdf_bytes
//   the test_output entry point: walk into a fresh template, serialize to bytes.
D_NODISCARD inline byte_buffer
render_report_pdf_bytes(
    const pdf_layout&   _layout,
    const test_report&  _run
)
{
    pdf_template _tpl;
    register_default_styles(_tpl);
    render_report_pdf(_layout, _run, _tpl);
    const std::string _bytes = _tpl.render_pdf();          // pdf_template serializes to std::string
    return byte_buffer(_bytes.begin(), _bytes.end());
}


// render_module_pdf_bytes
//   one module as a standalone PDF - the per-module bundle's pdf producer.
D_NODISCARD inline byte_buffer
render_module_pdf_bytes(
    const pdf_layout&   _layout,
    const test_context& _module_ctx
)
{
    pdf_template _tpl;
    register_default_styles(_tpl);
    render_module_pdf(_layout, _module_ctx, _tpl);
    const std::string _bytes = _tpl.render_pdf();
    return byte_buffer(_bytes.begin(), _bytes.end());
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  DEFAULT PDF LAYOUT  (data)                           ///
///////////////////////////////////////////////////////////////////////////////
//   Style names ("heading", "subheading", "body") are looked up in the
// template's style registry; register them before rendering (or leave "" to use
// the template default).  Tune freely - it is data, exactly like the string
// layouts; the walk and the binding do not change.

D_NODISCARD inline pdf_layout
pdf_default_layout()
{
    pdf_layout _l;

    _l.run_open = {
        pdf_op{ pdf_op::kind::text, "{report_title}", "heading", 0.0 },
        pdf_op{ pdf_op::kind::rule, "",              "",        1.0 },
    };
    _l.module_open = {
        pdf_op{ pdf_op::kind::vspace, "",            "",          8.0 },
        pdf_op{ pdf_op::kind::text,   "{module_name}", "subheading", 0.0 },
    };
    _l.unit_open = {
        pdf_op{ pdf_op::kind::text, "{curr_test_kind}   ({unit_passed} passed, {unit_failed} failed)", "body", 0.0 },
    };
    _l.check_line = {
        pdf_op{ pdf_op::kind::text, "    {check_index}. {check_desc}   [{check_status}]", "body", 0.0 },
    };
    // closes empty; run_close empty (the document ends with the last check)

    return _l;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   TEMPLATE-BASED RENDER  (document_renderer path)      ///
///////////////////////////////////////////////////////////////////////////////
//   An alternative to the styled-op walk above: build the report as document
// templates (title_page + document_table + colored check lines) and render them
// through pdf_document_renderer, the PDF face of the dialect-agnostic
// document-template subframework.  This is the DTest DEFAULT (test_report_runner.hpp
// calls the *_doc entry points below).  It yields a colored verdict cover, a
// module-summary table, and per-check pass/fail coloring, none of which the
// op-walk expressed.  The op-walk (Sections I-IV) is retained for callers that
// want literal control over the flow.

NS_INTERNAL
    // dtest_check_face
    //   the (word, style-name) pair a check's status renders as: PASS/FAIL/ERROR/
    // SKIP/PENDING against the check.pass / check.fail / check.skip palette
    // entries register_dtest_palette() seats.
    inline void
    dtest_check_face(
        test_status  _status,
        const char*& _word,
        const char*& _style
    )
    {
        switch (_status)
        {
            case test_status::passed:  { _word = "PASS";    _style = "check.pass"; break; }
            case test_status::failed:  { _word = "FAIL";    _style = "check.fail"; break; }
            case test_status::error:   { _word = "ERROR";   _style = "check.fail"; break; }
            case test_status::skipped: { _word = "SKIP";    _style = "check.skip"; break; }
            default:                   { _word = "PENDING"; _style = "check.skip"; break; }
        }

        return;
    }
NS_END  // namespace internal


// register_dtest_palette
//   seat the colored fonts the cover, summary table, and per-check lines use:
// a large title, a gray subtitle, plain fields, a green/red verdict banner, and
// green/red/gray monospace check lines.  A caller may register its own entries
// after this to retheme.
inline void
register_dtest_palette(
    pdf_document_renderer& _renderer
)
{
    // -- cover --
    pdf_text_style _title(pdf_font(pdf_base_font::helvetica_bold, 24.0));
    _title.leading = 30.0;
    _renderer.register_style(title_page_style_title, _title);

    pdf_text_style _subtitle(pdf_font(pdf_base_font::helvetica, 13.0));
    _subtitle.color   = pdf_color::gray();
    _subtitle.leading = 17.0;
    _renderer.register_style(title_page_style_subtitle, _subtitle);

    pdf_text_style _field(pdf_font(pdf_base_font::helvetica, 11.0));
    _field.leading = 15.0;
    _renderer.register_style(title_page_style_field, _field);

    // -- verdict banner --
    pdf_text_style _pass(pdf_font(pdf_base_font::helvetica_bold, 22.0));
    _pass.color   = pdf_color::green();
    _pass.leading = 28.0;
    _renderer.register_style("verdict.pass", _pass);

    pdf_text_style _fail(pdf_font(pdf_base_font::helvetica_bold, 22.0));
    _fail.color   = pdf_color::red();
    _fail.leading = 28.0;
    _renderer.register_style("verdict.fail", _fail);

    // -- per-check lines (Courier so descriptions align) --
    pdf_text_style _check_pass(pdf_font(pdf_base_font::courier, 10.0));
    _check_pass.color = pdf_color::green();
    _renderer.register_style("check.pass", _check_pass);

    pdf_text_style _check_fail(pdf_font(pdf_base_font::courier, 10.0));
    _check_fail.color = pdf_color::red();
    _renderer.register_style("check.fail", _check_fail);

    pdf_text_style _check_skip(pdf_font(pdf_base_font::courier, 10.0));
    _check_skip.color = pdf_color::gray();
    _renderer.register_style("check.skip", _check_skip);

    return;
}


// build_report_title_page
//   assemble the cover from the run's metadata, verdict, and cross-module
// roll-up figures.
D_NODISCARD inline title_page
build_report_title_page(
    const test_report& _run
)
{
    title_page _page;

    _page.set_title(_run.title.empty() ? std::string("Test Report") : _run.title);

    if (!_run.subtitle.empty()) { _page.set_subtitle(_run.subtitle); }
    if (!_run.author.empty())   { _page.set_author(_run.author); }

    // date as YYYY-MM-DD from the local clock (report_report timestamp helper)
    std::string _date;
    std::string _time;

    internal::report_timestamp_helper(_date, _time);

    if (_date.size() == 8)
    {
        _page.set_date(_date.substr(0, 4) + "-" +
                       _date.substr(4, 2) + "-" +
                       _date.substr(6, 2));
    }

    const report_verdict _verdict = _run.verdict();

    _page.set_banner(
        report_verdict_word(_verdict),
        report_verdict_is_pass(_verdict) ? "verdict.pass" : "verdict.fail");

    _page.add_field("Modules",    report_ratio(_run.passed_modules(), _run.total_modules()));
    _page.add_field("Unit tests", report_ratio(_run.passed_units(),   _run.total_units()));
    _page.add_field("Assertions", report_ratio(_run.passed_checks(),  _run.total_checks()));
    _page.add_field("Pass rate",  report_pass_rate(_run.passed_checks(), _run.total_checks()));

    return _page;
}


// build_module_summary_table
//   one row per module: name, unit-test ratio, assertion ratio, and pass rate.
D_NODISCARD inline document_table
build_module_summary_table(
    const test_report& _run
)
{
    document_table _table;

    _table.set_caption("Module Summary", 2);
    _table.add_column("Module")
          .add_column("Unit tests", text_alignment::right)
          .add_column("Assertions", text_alignment::right)
          .add_column("Rate",       text_alignment::right);

    std::size_t _i = 0;

    for (_i = 0; _i < _run.modules.size(); ++_i)
    {
        const report_module& _m = _run.modules[_i];

        _table.add_row({
            _m.name,
            report_ratio(_m.passed_units(),  _m.total_units()),
            report_ratio(_m.passed_checks(), _m.total_checks()),
            report_pass_rate(_m.passed_checks(), _m.total_checks())
        });
    }

    return _table;
}


// render_module_detail
//   a module heading, its description and assertion roll-up, then one colored
// line per check.
inline void
render_module_detail(
    const report_module& _module,
    document_renderer&   _renderer
)
{
    _renderer.heading(std::size_t(2), _module.name, doc_attributes());

    if (!_module.description.empty())
    {
        _renderer.paragraph(_module.description, doc_attributes());
    }

    _renderer.paragraph(
        report_ratio(_module.passed_checks(), _module.total_checks()) +
        " assertions passed  (" +
        report_pass_rate(_module.passed_checks(), _module.total_checks()) + ")",
        doc_attributes());

    std::size_t _ui = 0;
    std::size_t _n  = 0;

    for (_ui = 0; _ui < _module.units.size(); ++_ui)
    {
        const report_unit& _unit = _module.units[_ui];
        std::size_t        _ci   = 0;

        for (_ci = 0; _ci < _unit.checks.size(); ++_ci)
        {
            const report_check& _check = _unit.checks[_ci];
            const char*         _word   = "";
            const char*         _style  = "";

            internal::dtest_check_face(_check.status, _word, _style);

            ++_n;

            doc_attributes _attrs;
            _attrs.set(doc_attr_style, std::string(_style));

            _renderer.paragraph(
                "  " + std::to_string(_n) + ". " + _check.description +
                "   [" + std::string(_word) + "]",
                _attrs);
        }
    }

    return;
}


// render_report_doc
//   the whole run into a pdf_document_renderer: cover, module summary table,
// then per-module detail.
inline void
render_report_doc(
    const test_report&     _run,
    pdf_document_renderer& _renderer
)
{
    build_report_title_page(_run).render(_renderer);
    build_module_summary_table(_run).render(_renderer);

    std::size_t _i = 0;

    for (_i = 0; _i < _run.modules.size(); ++_i)
    {
        render_module_detail(_run.modules[_i], _renderer);
    }

    return;
}


// render_report_pdf_bytes_doc
//   the template-based whole-run entry point (the DTest default): render the
// document templates into a fresh pdf_template and serialize to bytes.
D_NODISCARD inline byte_buffer
render_report_pdf_bytes_doc(
    const test_report& _run
)
{
    pdf_template          _tpl;
    pdf_document_renderer _renderer(_tpl);

    register_dtest_palette(_renderer);
    render_report_doc(_run, _renderer);

    const std::string _bytes = _tpl.render_pdf();

    return byte_buffer(_bytes.begin(), _bytes.end());
}


// render_module_pdf_bytes_doc
//   the template-based per-module entry point: a cover for the one module plus
// its detail.  The module and run are read off the focus, exactly as
// render_module_pdf_bytes reads them.
D_NODISCARD inline byte_buffer
render_module_pdf_bytes_doc(
    const test_context& _module_ctx
)
{
    pdf_template          _tpl;
    pdf_document_renderer _renderer(_tpl);

    register_dtest_palette(_renderer);

    if (_module_ctx.module_ != nullptr)
    {
        const report_module& _module = *_module_ctx.module_;

        title_page _page;

        _page.set_title(_module.name);

        if ( (_module_ctx.run != nullptr) &&
             !_module_ctx.run->title.empty() )
        {
            _page.set_subtitle(_module_ctx.run->title);
        }

        const report_verdict _verdict = _module.verdict();

        _page.set_banner(
            report_verdict_word(_verdict),
            report_verdict_is_pass(_verdict) ? "verdict.pass" : "verdict.fail");

        _page.add_field("Assertions", report_ratio(_module.passed_checks(),
                                                    _module.total_checks()));
        _page.add_field("Pass rate",  report_pass_rate(_module.passed_checks(),
                                                       _module.total_checks()));

        _page.render(_renderer);

        render_module_detail(_module, _renderer);
    }

    const std::string _bytes = _tpl.render_pdf();

    return byte_buffer(_bytes.begin(), _bytes.end());
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_RENDER_PDF_
