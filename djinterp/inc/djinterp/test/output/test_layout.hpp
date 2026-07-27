/******************************************************************************
* djinterp [test]                                                test_layout.hpp
*
*   The STRUCTURAL half of the layout-driven report render: fold a finished
* test_report into a layout term, and hand that term to the interpreter.  This
* is what retires the two hand-written traversals.
*
*   WHAT IT REPLACES.
*     test_render.hpp      report_layout (7 literal slots) + render_report, one
*                          walk per string format, plus escaping_sink and four
*                          layout_*() literal tables.
*     test_render_pdf.hpp  pdf_layout (the same 7 slots as styled-op lists) +
*                          a SECOND walk, structurally identical, differing only
*                          in emission.  Its own banner named the two traversals
*                          "a candidate to factor into one visitor-driven walk".
*   Both are now one fold to a term plus the interpreter's single fold over it.
*
*   DTEST DEFINES NO NEW CONSTRUCTS -- and that is the point.  A test report is
* a cover, a table of contents, and nested numbered sections holding content;
* report_dialect.hpp already names exactly that set (body / cover / clearpage /
* toc / section, with content as a body_ref leaf).  So this header registers
* nothing and lowers nothing: it builds a report_op term with the dialect's own
* combinators and hands it to the dialect's own signature.  If DTest needed a
* construct the dialect lacks, the fix would be a lowering in a dialect header,
* not a special case here.
*
*   THE MAPPING.
*       test_report   -> body{ cover, clearpage, toc, clearpage, sections... }
*       report_module -> section(module name) { content(module summary), units }
*       report_unit   -> section(unit name)   { content(assertion table) }
*       report_check  -> a ROW of that table  (see test_report_content.hpp)
*   Numbering and the table of contents fall out of the interpreter's passes:
*   modules number 1, 2, 3 and their units 1.1, 1.2 -- which the old flat
*   report_layout could not express at all, since it had one literal per LEVEL
*   and no notion of depth.
*
*   CONTENT IS RESOLVED LATE, BY NAME.  The term carries keys, not text; the
* resolver binds them against the report at render time
* (test_report_content.hpp).  The report is borrowed BY REFERENCE into that
* closure, so it MUST outlive the render call -- the same lifetime rule
* test_output's deferred producers already carry, and it holds by construction
* for the entry points below, which build and render within one call.
*
*   PORTABILITY:
*   C++14 (layout.hpp's floor -- the interpreter's higher-order deduction);
* self-suppresses below it.
*
*
* TABLE OF CONTENTS
* =================
* I.    report_to_layout            (the fold: report -> layout_doc<report_op>)
* II.   report_metadata_bag         (cover / meta_ref bindings)
* III.  make_report_resolver        (content_resolver over a borrowed report)
* IV.   render_test_report          (drive any document_renderer)
*
*
* path:      /inc/djinterp/test/output/test_layout.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TEST_LAYOUT_
#define DJINTERP_TEST_LAYOUT_ 1

// std
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"                     // NS_*, gates, D_NODISCARD
#include "../../core/util/document/report_dialect.hpp" // report_op, the combinator
                                                       //   surface, make_report_signature
#include "../../core/util/document/layout_interpret.hpp"
                                                       // render_document, content_resolver
#include "./test_report_content.hpp"                   // resolve_report_content + keys
#include "./test_report.hpp"                           // test_report + the model


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   report_to_layout                                    ///
///////////////////////////////////////////////////////////////////////////////

// report_to_layout
//   function: the fold from the report model to the document term.  Structure
// only -- every piece of text that is CONTENT rather than STRUCTURE leaves as a
// named body_ref, resolved later.  The result is an ordinary layout_doc: the
// interpreter cannot tell it was built here rather than by the cursor, the
// aggregate spec, or the block-form parser.
D_NODISCARD inline ::djinterp::layout_doc< ::djinterp::report_op>
report_to_layout(
    const test_report& _report
)
{
    using ::djinterp::layout_doc;
    using ::djinterp::report_op;

    std::vector<layout_doc<report_op> > _top;

    // the cover, then a table of contents, each on its own page
    _top.push_back(::djinterp::cover({
        std::pair<std::string, std::string>("subtitle", _report.subtitle),
        std::pair<std::string, std::string>("author",   _report.author) }));
    _top.push_back(::djinterp::clearpage());
    _top.push_back(::djinterp::toc());
    _top.push_back(::djinterp::clearpage());

    // the run-level figures, as their own numbered section
    _top.push_back(::djinterp::section(
        std::string("Summary"),
        { ::djinterp::content(content_key_run_summary()) }));

    // one section per module, holding one section per unit
    for (std::size_t _m = 0; _m < _report.modules.size(); ++_m)
    {
        const report_module& _module = _report.modules[_m];

        std::vector<layout_doc<report_op> > _module_children;

        // the module's own figures lead its section
        _module_children.push_back(
            ::djinterp::content(content_key_module_summary(_m)));

        for (std::size_t _u = 0; _u < _module.units.size(); ++_u)
        {
            // the unit's assertion table is its whole body; its descriptor
            // rides inside the table's emission, not as a sibling
            std::vector<layout_doc<report_op> > _unit_children;

            _unit_children.push_back(
                ::djinterp::content(content_key_unit(_m, _u)));

            _module_children.push_back(
                ::djinterp::section(_module.units[_u].name, _unit_children));
        }

        _top.push_back(::djinterp::section(_module.name, _module_children));
    }

    return ::djinterp::body(_top);
}


///////////////////////////////////////////////////////////////////////////////
///                II.  report_metadata_bag                                 ///
///////////////////////////////////////////////////////////////////////////////

// report_metadata_bag
//   function: the flat bag meta_ref leaves resolve against, and the seed the
// cover reads its title / subtitle / author from.  A report field with nothing
// in it is simply not bound, so a cover renders what is present rather than a
// row of empty labels.
D_NODISCARD inline ::djinterp::doc_attributes
report_metadata_bag(
    const test_report& _report
)
{
    ::djinterp::doc_attributes _bag;

    if (!_report.title.empty())
    {
        _bag.set(std::string("title"), _report.title);
    }

    if (!_report.subtitle.empty())
    {
        _bag.set(std::string("subtitle"), _report.subtitle);
    }

    if (!_report.author.empty())
    {
        _bag.set(std::string("author"), _report.author);
    }

    return _bag;
}


///////////////////////////////////////////////////////////////////////////////
///                III. make_report_resolver                                ///
///////////////////////////////////////////////////////////////////////////////

// make_report_resolver
//   function: the content side of the boundary -- a body_ref key to how it
// emits, bound against _report.
//
//   LIFETIME: _report is borrowed by pointer into the returned closure and MUST
// outlive every use of it.  The entry points below build and render within one
// call, so it holds for them by construction; a caller storing the resolver
// takes on the obligation.
D_NODISCARD inline ::djinterp::content_resolver
make_report_resolver(
    const test_report& _report
)
{
    const test_report* _borrowed = &_report;

    return ::djinterp::content_resolver(
        [_borrowed](const ::djinterp::layout_atom& _ref)
            -> ::djinterp::maybe< ::djinterp::render_action>
        {
            const std::string _key = _ref.key;

            // a key this report does not bind is a graceful hole, per the
            // interpreter's contract -- decided by the predicate, so nothing
            // is rendered twice
            if (!report_binds_content(*_borrowed, _key))
            {
                return ::djinterp::nothing< ::djinterp::render_action>();
            }

            return ::djinterp::just(::djinterp::render_action(
                [_borrowed, _key](::djinterp::document_renderer& _r)
                {
                    (void)resolve_report_content(*_borrowed, _key, _r);
                }));
        });
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  render_test_report                                  ///
///////////////////////////////////////////////////////////////////////////////

// render_test_report
//   function: the whole pipeline for one report -- fold to a term, bind its
// content, and drive _renderer through the interpreter (number -> outline ->
// emit -> run).  One traversal, any dialect.
inline void
render_test_report(
    const test_report&            _report,
    ::djinterp::document_renderer& _renderer
)
{
    ::djinterp::render_document(
        report_to_layout(_report),
        ::djinterp::make_report_signature(),
        make_report_resolver(_report),
        report_metadata_bag(_report),
        _renderer);

    return;
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_TEST_LAYOUT_
