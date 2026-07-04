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
#include "../../core/util/compression.hpp"       // pdf::pdf_template (path: match your tree)
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


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_RENDER_PDF_
