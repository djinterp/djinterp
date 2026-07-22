/******************************************************************************
* djinterp [test]                                                test_output.hpp
*
*   The DTest emit layer: turn a finished test_report into the documents a run
* should produce - one per requested format - collate them into a
* document_bundle, and write that bundle through the output_packaging vocabulary
* to a sink.  This is the join between the two halves of the stack: CONTENT
* production (test_render's projections + per-format report_layout, and
* test_render_pdf's pdf_layout) and ASSEMBLY (document_bundle +
* output_packaging).  It generalizes the old test_pdf_report::save() from "one
* styled PDF" to "any set of formats, any packaging mode".
*
*   THE ONE PLACE A CONCRETE ENGINE IS NAMED:
*   test_render renders a run into a DUCK-TYPED sink (literal / value), so it
* depends on no engine at all; test_render_pdf drives pdf_template.  test_output
* is the leaf that binds those to the concrete outputs: the string layouts to a
* plain sink (text / markdown) or an escaping sink (xml / html), and the pdf
* layout to pdf_template.  Everything above this header stays format-agnostic.
*
*   THE INVARIANT THIS LAYER CASHES IN:
*   For a given (focus, key) the FRAGMENT a projection yields is the same
* whatever document it lands in - only the LAYOUT and the SINK differ per
* format.  So a per-format document is just "pick the layout, pick the sink,
* render, take the bytes"; the projection table is shared across every format.
* render_report_bytes is exactly that switch.
*
*   DEFERRED RENDER (PARITY WITH document_bundle):
*   add_report appends a bundle_item whose producer renders the chosen format
* only when write() asks - so a format that is configured but, say, filtered out
* downstream is never rendered, and the expensive PDF pass is paid once, at
* write() time.  The producer borrows the test_report BY POINTER; it MUST
* OUTLIVE the eventual write() (the emit_* helpers build and write within one
* call, so this holds for them by construction).
*
*   PDF STYLING IS THE CALLER'S SCHEMA, NOT OURS:
*   The PDF house style (test_defaults' pdf_config -> a laid-out pdf_template) is
* injected as a pdf_template_source (a deferred factory), so test_output need not
* know pdf_config / pdf_options.  A null source means "a default-constructed
* template carrying test_render_pdf's default styles"; a non-null source owns its
* own style registry (an unregistered style name simply falls back to the body
* style, per pdf_template).
*
*   FORMAT COVERAGE:
*   text / markdown / xml / html / pdf are all wired to their own layout.  xml
* and html additionally ride an escaping_sink, so a check description containing
* markup is escaped rather than injected.
*
*   RETARGETED (2026.07.06):
*   This header previously composed a `test_document` through render_run_tree /
* render_run_flow.  That content layer was dissolved into test_render.hpp (whose
* banner names those very functions as what it collapses) and no definition of
* it survives anywhere in the tree, so this header could not compile.  It is now
* written against the live render layer.  Consequences: `test_document` is gone
* from every signature; the `text_flow_target` sink and the
* internal::render_tree_with<_Policy> / document<_Policy> tree path retire
* (layout_text IS the flow, and layout_xml / layout_html + escaping_sink cover
* the markup formats); and render_module_bytes - which test_emit.hpp already
* called but which never existed here - is supplied.
*
*   PORTABILITY:
*   C++17 (it composes test_render / document_bundle, both C++17);
* self-suppresses below the floor.
*
*
* TABLE OF CONTENTS
* =================
* I.    DOC FORMAT               (doc_format; format_extension; pdf source)
* II.   PER-FORMAT RENDER        (render_report_bytes / render_module_bytes)
* III.  BUNDLE ASSEMBLY          (add_report / build_report_bundle)
* IV.   EMIT                     (emit_report_to_disk / emit_report_to_buffer)
*
*
* path:      /inc/djinterp/test/output/test_output.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_OUTPUT_
#define DJINTERP_TEST_TEST_OUTPUT_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"                    // NS_*, D_NODISCARD, gates
#include "../../core/text/document_bundle.hpp"        // document_bundle, write_to_disk,
                                                      //   write_to_buffer, byte_buffer,
                                                      //   output_config, disk_output_sink
                                                      //   (sinks via output_packaging)
#include "../../core/text/markup_string_template.hpp" // xml_escape_policy, html_escape_policy
#include "./test_render.hpp"                          // report_layout, render_report,
                                                      //   render_report_string, render_module,
                                                      //   escaping_sink, layout_* , test_report,
                                                      //   test_context, at_run
#include "./test_render_pdf.hpp"                      // pdf_layout, pdf_default_layout,
                                                      //   render_report_pdf(_bytes),
                                                      //   render_module_pdf(_bytes), pdf_template


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   DOC FORMAT                                           ///
///////////////////////////////////////////////////////////////////////////////

// doc_format
//   enum: the output formats the DTest layer can emit.  Each maps to exactly one
// layout: text / markdown / xml / html to a report_layout (test_render), pdf to a
// pdf_layout (test_render_pdf).  The layout - and, for the markup formats, the
// escaping sink - is the ONLY thing that varies; the projections are shared.
enum class doc_format
{
    text,
    markdown,
    xml,
    html,
    pdf
};


// format_extension
//   function: the conventional file extension for a format (".txt", ".md",
// ".xml", ".html", ".pdf"), handed to the bundle's naming policy.  (Distinct
// from output_packaging's format_extension(format_id), which names archive
// CONTAINERS; this one names report DOCUMENTS.)
D_NODISCARD inline const char*
format_extension(
    doc_format _fmt
) D_NOEXCEPT
{
    switch (_fmt)
    {
        case doc_format::text:     { return ".txt";  }
        case doc_format::markdown: { return ".md";   }
        case doc_format::xml:      { return ".xml";  }
        case doc_format::html:     { return ".html"; }
        case doc_format::pdf:      { return ".pdf";  }
        default:                   { return ".txt";  }
    }
}


// pdf_template_source
//   type: a deferred factory for a (house-styled) pdf_template.  The PDF
// presentation schema lives in test_defaults (pdf_config -> a laid-out
// pdf_template); injecting it as a factory keeps test_output ignorant of
// pdf_config / pdf_options.  A null source means "a default-constructed template
// carrying test_render_pdf's default styles".  std::function for parity with the
// rest of the stack.
using pdf_template_source = std::function< ::djinterp::pdf_template() >;


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-FORMAT RENDER                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL


// render_run_plain / render_run_escaped
//   helper: render a whole run under _layout into bytes.  The plain form feeds
// an interp_string_sink (text / markdown); the escaped form feeds an
// escaping_sink<_Policy>, so a resolved value containing markup is escaped
// rather than injected (xml / html).  The two differ ONLY in the sink.
D_NODISCARD inline byte_buffer
render_run_plain(
    const report_layout&  _layout,
    const test_report&    _run
)
{
    return byte_buffer(render_report_string(_layout, _run));
}


template<typename _Policy>
D_NODISCARD inline byte_buffer
render_run_escaped(
    const report_layout&  _layout,
    const test_report&    _run
)
{
    std::string             _out;
    escaping_sink<_Policy>  _sink(_out);

    render_report(_layout, _run, _sink);

    return byte_buffer(static_cast<std::string&&>(_out));
}


// render_mod_plain / render_mod_escaped
//   helper: the same two sinks, applied to ONE module subtree (the per-module
// document the emit layer fans out when per_module is set).
D_NODISCARD inline byte_buffer
render_mod_plain(
    const report_layout&  _layout,
    const test_context&   _focus
)
{
    std::string                           _out;
    ::djinterp::interp_string_sink<char>  _sink(_out);

    render_module(_layout, _focus, _sink);

    return byte_buffer(static_cast<std::string&&>(_out));
}


template<typename _Policy>
D_NODISCARD inline byte_buffer
render_mod_escaped(
    const report_layout&  _layout,
    const test_context&   _focus
)
{
    std::string             _out;
    escaping_sink<_Policy>  _sink(_out);

    render_module(_layout, _focus, _sink);

    return byte_buffer(static_cast<std::string&&>(_out));
}


NS_END  // internal


// render_report_bytes
//   function: render ONE document of _run in _fmt to bytes.  The whole stack
// funnels to this switch - the only place a concrete engine is named:
//     pdf       -> a (house-styled) pdf_template laid out by render_report_pdf;
//     text / md -> a report_layout rendered into a plain string sink;
//     xml / html-> a report_layout rendered into an escaping sink.
// The bytes ARE the document - no conversion stands between here and
// document_bundle / archive.
D_NODISCARD inline byte_buffer
render_report_bytes(
    const test_report&          _run,
    doc_format                  _fmt,
    const pdf_template_source&  _pdf_src = pdf_template_source()
)
{
    switch (_fmt)
    {
        case doc_format::pdf:
        {
            // a supplied source owns its own style registry; otherwise take
            // test_render_pdf's default-styled template.
            if (_pdf_src)
            {
                ::djinterp::pdf_template _tpl = _pdf_src();

                render_report_pdf(pdf_default_layout(), _run, _tpl);

                return byte_buffer(_tpl.render_pdf());
            }

            return render_report_pdf_bytes(pdf_default_layout(), _run);
        }

        case doc_format::xml:
        {
            return internal::render_run_escaped< ::djinterp::xml_escape_policy>(
                       layout_xml(), _run);
        }

        case doc_format::html:
        {
            return internal::render_run_escaped< ::djinterp::html_escape_policy>(
                       layout_html(), _run);
        }

        case doc_format::markdown:
        {
            return internal::render_run_plain(layout_markdown(), _run);
        }

        case doc_format::text:
        default:
        {
            return internal::render_run_plain(layout_text(), _run);
        }
    }
}


// render_module_bytes
//   function: render ONE MODULE of a run in _fmt to bytes - the per-module
// document the emit layer fans out when a target is per_module.  _focus must be
// a module focus (run + module_ + module_index set; see at_module).  Same switch,
// same layouts, same projections - only the traversal root differs.
D_NODISCARD inline byte_buffer
render_module_bytes(
    const test_context&         _focus,
    doc_format                  _fmt,
    const pdf_template_source&  _pdf_src = pdf_template_source()
)
{
    switch (_fmt)
    {
        case doc_format::pdf:
        {
            if (_pdf_src)
            {
                ::djinterp::pdf_template _tpl = _pdf_src();

                render_module_pdf(pdf_default_layout(), _focus, _tpl);

                return byte_buffer(_tpl.render_pdf());
            }

            return render_module_pdf_bytes(pdf_default_layout(), _focus);
        }

        case doc_format::xml:
        {
            return internal::render_mod_escaped< ::djinterp::xml_escape_policy>(
                       layout_xml(), _focus);
        }

        case doc_format::html:
        {
            return internal::render_mod_escaped< ::djinterp::html_escape_policy>(
                       layout_html(), _focus);
        }

        case doc_format::markdown:
        {
            return internal::render_mod_plain(layout_markdown(), _focus);
        }

        case doc_format::text:
        default:
        {
            return internal::render_mod_plain(layout_text(), _focus);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
///                III. BUNDLE ASSEMBLY                                      ///
///////////////////////////////////////////////////////////////////////////////

// add_report
//   function: append _fmt of _run to _bundle as a DEFERRED item.  The producer
// borrows _run by pointer (it MUST outlive the eventual write()) and copies the
// pdf source by value; it renders when - and only when - write() asks.  _name is
// the logical base handed to the naming policy; the extension is the format's.
// Returns *_bundle for fluent assembly.
inline document_bundle&
add_report(
    document_bundle&            _bundle,
    const test_report&          _run,
    doc_format                  _fmt,
    std::string                 _name    = std::string("report"),
    const pdf_template_source&  _pdf_src = pdf_template_source()
)
{
    const test_report*   _run_p = &_run;
    pdf_template_source  _src   = _pdf_src;

    return _bundle.add(
        static_cast<std::string&&>(_name),
        std::string(format_extension(_fmt)),
        [_run_p, _fmt, _src]() -> byte_buffer
        {
            return render_report_bytes(*_run_p, _fmt, _src);
        });
}


// build_report_bundle
//   function: a bundle with one deferred item per requested format, all under
// the same logical _name (the naming policy disambiguates multi-format runs by
// 1-based index).  Nothing is rendered here - render is deferred to write().
D_NODISCARD inline document_bundle
build_report_bundle(
    const test_report&              _run,
    const std::vector<doc_format>&  _formats,
    std::string                     _name    = std::string("report"),
    const pdf_template_source&      _pdf_src = pdf_template_source()
)
{
    document_bundle _bundle;
    std::size_t     _i = 0;

    for (_i = 0; _i < _formats.size(); ++_i)
    {
        add_report(_bundle, _run, _formats[_i], _name, _pdf_src);
    }

    return _bundle;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  EMIT                                                 ///
///////////////////////////////////////////////////////////////////////////////

// emit_report_to_disk
//   function: build the multi-format bundle for _run and write it per _cfg to
// disk, naming each file through _path (filename -> destination path).  This is
// the generalized test_pdf_report::save() - every format, all three packaging
// modes (verbatim / per-file compress / one archive).  Returns write()'s success.
D_NODISCARD inline bool
emit_report_to_disk(
    const test_report&              _run,
    const std::vector<doc_format>&  _formats,
    const output_config&            _cfg,
    disk_output_sink::path_fn       _path,
    std::string                     _name    = std::string("report"),
    const pdf_template_source&      _pdf_src = pdf_template_source()
)
{
    document_bundle _bundle =
        build_report_bundle(_run, _formats, _name, _pdf_src);

    return write_to_disk(_bundle, _cfg,
                         static_cast<disk_output_sink::path_fn&&>(_path));
}


// emit_report_to_buffer
//   function: build the multi-format bundle for _run and write it per _cfg into
// the in-memory buffer _out, with _separator between documents (relevant only in
// verbatim / compress mode; archive mode yields a single container).  Returns
// write()'s success.
D_NODISCARD inline bool
emit_report_to_buffer(
    const test_report&              _run,
    const std::vector<doc_format>&  _formats,
    const output_config&            _cfg,
    byte_buffer&                    _out,
    const std::string&              _separator = std::string(),
    std::string                     _name      = std::string("report"),
    const pdf_template_source&      _pdf_src   = pdf_template_source()
)
{
    document_bundle _bundle =
        build_report_bundle(_run, _formats, _name, _pdf_src);

    return write_to_buffer(_bundle, _cfg, _out, _separator);
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_OUTPUT_
