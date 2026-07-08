/******************************************************************************
* djinterp [test]                                              test_output.hpp
*
*   The DTest emit layer: turn a finished test_report into the documents a run
* should produce - one per requested format - collate them into a
* document_bundle, and write that bundle through the output_packaging
* vocabulary to a sink.  This is the join between the two halves of the stack:
* CONTENT production (test_document's format-agnostic render_run_tree /
* render_run_flow, driven by binding_env + section) and ASSEMBLY (document_bundle
* + output_packaging).  It generalizes the old test_pdf_report::save() from "one
* styled PDF" to "any set of formats, any packaging mode", and it absorbs the
* emit half of the dissolved test_report_runner (the console half goes to
* test_printer; the orchestration half is here).
*
*   THE ONE PLACE A CONCRETE ENGINE IS NAMED:
*   test_document renders against a DUCK-TYPED target (render_run_tree wants
* .open_child(name).text(value); render_run_flow wants .add_text / .add_vspace /
* .add_page_break) and so depends on NEITHER pdf_template NOR document_writer.
* test_output is the leaf that binds those abstract skeletons to the real
* engines: the FLOW skeleton to pdf_template (PDF) and to a tiny self-contained
* text_flow_target (plain text); the TREE skeleton to document<_Policy> (XML /
* HTML).  Everything above this header stays format-agnostic.
*
*   THE INVARIANT THIS LAYER CASHES IN:
*   For a given (env, ctx, key) the FRAGMENT a projection yields is the same
* whether it lands in a tree subtree or a flow line - only PLACEMENT differs per
* format (verified at the section kernel in T1, used at the document level in
* test_document).  So a per-format document is just "pick the skeleton kind
* (flow vs tree), pick the engine, render, take the bytes"; the bindings are
* shared across every format.  render_report_bytes is exactly that switch.
*
*   DEFERRED RENDER (PARITY WITH document_bundle):
*   add_report appends a bundle_item whose producer renders the chosen format
* only when write() asks - so a format that is configured but, say, filtered out
* downstream is never rendered, and the expensive PDF pass is paid once, at
* write() time.  The producer borrows the test_document and the test_report by
* pointer; BOTH MUST OUTLIVE the eventual write() (the emit_* helpers build and
* write within one call, so this holds for them by construction).
*
*   PDF STYLING IS THE CALLER'S SCHEMA, NOT OURS:
*   The PDF house style (test_defaults' pdf_config -> a laid-out pdf_template)
* is injected as a pdf_template_source (a deferred factory), so test_output need
* not know pdf_config / pdf_options.  A null source yields a default-constructed
* pdf_template; test_defaults supplies the styled one.
*
*   FORMAT COVERAGE:
*   text / xml / html / pdf are wired.  markdown currently rides the plain-text
* path (so a .md file is at least readable); a true markdown_print_policy on the
* TREE path is the natural follow-up and slots in at the one switch arm below
* without touching anything else.
*
*   PORTABILITY:
*   C++17 (it composes test_document / document_bundle, both C++17);
* self-suppresses below the floor.
*
*
* TABLE OF CONTENTS
* =================
* I.    DOC FORMAT               (doc_format; format_extension; pdf source)
* II.   TEXT FLOW TARGET         (text_flow_target: the flow sink for plain text)
* III.  PER-FORMAT RENDER        (render_report_bytes: the engine leaf)
* IV.   BUNDLE ASSEMBLY          (add_report / build_report_bundle)
* V.    EMIT                     (emit_report_to_disk / emit_report_to_buffer)
*
*
* path:      /inc/djinterp/test/test_output.hpp
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
#include "../core/djinterp.hpp"                  // NS_*, D_NODISCARD, gates
#include "../core/document_bundle.hpp"           // document_bundle, write_*,
                                                 //   byte_buffer, output_config,
                                                 //   sinks (via output_packaging)
#include "../core/text/document.hpp"             // document<_Policy>, cursor,
                                                 //   xml_print_policy,
                                                 //   html_print_policy
#include "../core/pdf/pdf_template.hpp"          // pdf::pdf_template (flow)
#include "./test_document.hpp"                   // test_document, test_context,
                                                 //   at_run, render_run_tree,
                                                 //   render_run_flow, test_report


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   DOC FORMAT                                           ///
///////////////////////////////////////////////////////////////////////////////

// doc_format
//   enum: the output formats the DTest layer can emit.  text / markdown ride
// the FLOW skeleton (render_run_flow); xml / html ride the TREE skeleton
// (render_run_tree); pdf rides the FLOW skeleton through pdf_template.  The
// choice of skeleton kind is the ONLY thing that varies per format - the
// bindings are shared.
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
// pdf_config / pdf_options.  A null source means "a default-constructed
// pdf_template".  std::function for parity with the rest of the stack.
using pdf_template_source = std::function< ::pdf_template() >;


///////////////////////////////////////////////////////////////////////////////
///                II.  TEXT FLOW TARGET                                     ///
///////////////////////////////////////////////////////////////////////////////

// text_flow_target
//   class: a FLOW-shaped sink that collects render_run_flow's lines into one
// plain-text buffer.  This is what lets plain text reuse the PDF skeleton with
// no tree policy at all: add_text appends a line, add_vspace a blank line,
// add_page_break a form feed.  Because it satisfies the same duck-typed flow
// shape pdf_template does, the SAME render_run_flow - the same projections, the
// same fragments - feeds both; only this sink's placement (newline-joined text
// vs a paginated PDF) differs.  Markdown currently borrows this path too.
class text_flow_target
{
public:
    // add_text
    //   appends _line followed by a newline.
    void
    add_text(
        const std::string&  _line
    )
    {
        m_out += _line;
        m_out += '\n';
    }

    // add_vspace
    //   appends a single blank line; the magnitude is irrelevant to plain text.
    void
    add_vspace(
        double  /*_points*/
    )
    {
        m_out += '\n';
    }

    // add_page_break
    //   appends a form feed (U+000C), the conventional plain-text page break.
    void
    add_page_break()
    {
        m_out += '\f';
    }

    // str
    //   the accumulated plain-text buffer.
    D_NODISCARD const byte_buffer&
    str() const D_NOEXCEPT
    {
        return m_out;
    }

private:
    byte_buffer m_out;
};


///////////////////////////////////////////////////////////////////////////////
///                III. PER-FORMAT RENDER                                    ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL


// render_tree_with
//   helper: build a document<_Policy>, fill its root with the canonical report
// tree via render_run_tree, and serialize to bytes.  The XML and HTML paths
// differ ONLY in the policy type passed here.
template<typename _Policy>
D_NODISCARD inline byte_buffer
render_tree_with(
    const test_document& _doc,
    const test_report&   _run
)
{
    ::djinterp::document<_Policy> _out;
    ::djinterp::cursor           _root = _out.root("report");

    render_run_tree(_doc, at_run(&_run), _root);

    return _out.to_string();
}


NS_END  // internal


// render_report_bytes
//   function: render ONE document of _run in _fmt to bytes.  The whole stack
// funnels to this switch - the only place a concrete engine is named:
//     pdf       -> a (house-styled) pdf_template laid out by render_run_flow;
//     text / md -> a text_flow_target laid out by render_run_flow;
//     xml / html-> a document<_Policy> filled by render_run_tree.
// markdown shares the text path pending a markdown_print_policy (a new TREE
// arm, not a new branch of the design).  The bytes ARE the document - no
// conversion stands between here and document_bundle / archive.
D_NODISCARD inline byte_buffer
render_report_bytes(
    const test_document&        _doc,
    const test_report&          _run,
    doc_format                  _fmt,
    const pdf_template_source&  _pdf_src = pdf_template_source()
)
{
    switch (_fmt)
    {
        case doc_format::pdf:
        {
            ::pdf_template _tpl =
                _pdf_src ? _pdf_src() : ::pdf_template();

            render_run_flow(_doc, at_run(&_run), _tpl);

            return _tpl.render_pdf();
        }

        case doc_format::xml:
        {
            return internal::render_tree_with< ::djinterp::xml_print_policy>(
                       _doc, _run);
        }

        case doc_format::html:
        {
            return internal::render_tree_with< ::djinterp::html_print_policy>(
                       _doc, _run);
        }

        case doc_format::text:
        case doc_format::markdown:
        default:
        {
            text_flow_target _flow;

            render_run_flow(_doc, at_run(&_run), _flow);

            return _flow.str();
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  BUNDLE ASSEMBLY                                      ///
///////////////////////////////////////////////////////////////////////////////

// add_report
//   function: append _fmt of _run to _bundle as a DEFERRED item.  The producer
// borrows _doc and _run by pointer (both MUST outlive the eventual write()) and
// copies the pdf source by value; it renders exactly once, when write() asks.
// _name is the logical base handed to the naming policy; the extension is the
// format's.  Returns *_bundle for fluent assembly.
inline document_bundle&
add_report(
    document_bundle&            _bundle,
    const test_document&        _doc,
    const test_report&          _run,
    doc_format                  _fmt,
    std::string                 _name    = std::string("report"),
    const pdf_template_source&  _pdf_src = pdf_template_source()
)
{
    const test_document* _doc_p = &_doc;
    const test_report*   _run_p = &_run;
    pdf_template_source  _src   = _pdf_src;

    return _bundle.add(
        static_cast<std::string&&>(_name),
        std::string(format_extension(_fmt)),
        [_doc_p, _run_p, _fmt, _src]() -> byte_buffer
        {
            return render_report_bytes(*_doc_p, *_run_p, _fmt, _src);
        });
}


// build_report_bundle
//   function: a bundle with one deferred item per requested format, all under
// the same logical _name (the naming policy disambiguates multi-format runs by
// 1-based index).  Nothing is rendered here - render is deferred to write().
D_NODISCARD inline document_bundle
build_report_bundle(
    const test_document&            _doc,
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
        add_report(_bundle, _doc, _run, _formats[_i], _name, _pdf_src);
    }

    return _bundle;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   EMIT                                                 ///
///////////////////////////////////////////////////////////////////////////////

// emit_report_to_disk
//   function: build the multi-format bundle for _run and write it per _cfg to
// disk, naming each file through _path (filename -> destination path).  This is
// the generalized test_pdf_report::save() - every format, all three packaging
// modes (verbatim / per-file compress / one archive).  Returns write()'s
// success.
D_NODISCARD inline bool
emit_report_to_disk(
    const test_document&            _doc,
    const test_report&              _run,
    const std::vector<doc_format>&  _formats,
    const output_config&            _cfg,
    disk_output_sink::path_fn       _path,
    std::string                     _name    = std::string("report"),
    const pdf_template_source&      _pdf_src = pdf_template_source()
)
{
    document_bundle _bundle =
        build_report_bundle(_doc, _run, _formats, _name, _pdf_src);

    return write_to_disk(_bundle, _cfg,
                         static_cast<disk_output_sink::path_fn&&>(_path));
}


// emit_report_to_buffer
//   function: build the multi-format bundle for _run and write it per _cfg into
// the in-memory buffer _out, with _separator between documents (relevant only
// in verbatim / compress mode; archive mode yields a single container).
// Returns write()'s success.
D_NODISCARD inline bool
emit_report_to_buffer(
    const test_document&            _doc,
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
        build_report_bundle(_doc, _run, _formats, _name, _pdf_src);

    return write_to_buffer(_bundle, _cfg, _out, _separator);
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_OUTPUT_
