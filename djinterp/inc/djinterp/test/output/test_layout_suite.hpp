/******************************************************************************
* djinterp [test]                                        test_layout_suite.hpp
*
*   The layout subframework's own test module -- and the new home of the two
* demos that used to ship inside the library headers.
*
*   WHY THEY MOVED.  report_dialect.hpp carried a DEMO section and
* report_surfaces.hpp an EQUIVALENCE DEMO.  Both asserted something true and
* worth asserting, but a library header cannot be exercised by the harness: the
* code was compiled and never run, so a regression in the interpreter would have
* shipped silently.  Here the same code is a MODULE the runner executes, and a
* failure is reported rather than merely compiled.  Sections I-III of
* report_surfaces.hpp stayed put -- the surfaces themselves are library; only
* the assertions about them were tests.
*
*   WHAT IT COVERS.
*     1. FOUR SURFACES, ONE TERM.  The combinator, cursor, aggregate-spec and
*        block-DSL front ends each build the same document; all four render
*        identically.  This is the subframework's central claim, and it is the
*        one that silently breaks when a builder drifts.
*     2. THE FOUR PASSES.  demo_render exercises number -> outline -> emit ->
*        run end to end: the numbering, the generated table of contents, the
*        section depths and a rendered table all have to be right for its output
*        to match.
*     3. DEPTH.  A nested section numbers 1.1, which is precisely what the flat
*        per-level layout literals this stack replaced could not express.
*
*   PORTABILITY:
*   C++14 (layout.hpp's floor); self-suppresses below it.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES                    (build the same document four ways)
* II.   demo_render                 (assemble + render, the worked example)
* III.  register_layout_tests       (the module the runner executes)
*
*
* path:      /inc/djinterp/test/output/test_layout_suite.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.24
******************************************************************************/

#ifndef DJINTERP_TEST_LAYOUT_SUITE_
#define DJINTERP_TEST_LAYOUT_SUITE_ 1

// std
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/util/document/report_surfaces.hpp"   // the three surfaces
#include "../../core/util/document/report_dialect.hpp"    // report_op + combinators
#include "../../core/util/document/layout_parse.hpp"      // the block-DSL surface
#include "./test_report_runner.hpp"                       // report_builder, D_CHECK_EQ


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   FIXTURES                                            ///
///////////////////////////////////////////////////////////////////////////////
//   Moved verbatim from report_surfaces.hpp's section IV.  Each builds the SAME
// document through a different front end; the module below renders all four and
// compares.


// contains_helper / count_helper
//   helpers: substring presence and character frequency, so a check reads as a
// comparison rather than as a search expression.
D_NODISCARD inline bool
contains_helper(
    const std::string& _haystack,
    const std::string& _needle
)
{
    return (_haystack.find(_needle) != std::string::npos);
}

D_NODISCARD inline std::size_t
count_helper(
    const std::string& _text,
    char               _c
)
{
    std::size_t _n = 0;

    for (std::size_t _i = 0; _i < _text.size(); ++_i)
    {
        if (_text[_i] == _c)
        {
            ++_n;
        }
    }

    return _n;
}


// report_dsl_source
//   the same document as a block-DSL string.
D_NODISCARD inline std::string
report_dsl_source()
{
    return std::string(
        "cover subtitle=\"nightly build\" author=\"teer\"\n"
        "clearpage\n"
        "toc\n"
        "clearpage\n"
        "section \"Introduction\" {\n"
        "    content \"intro.body\"\n"
        "    section \"Motivation\" { content \"intro.motivation\" }\n"
        "}\n"
        "section \"Results\" { table \"results.table\" }\n");
}

// build_via_combinator
//   the functional surface (mirrors report_dialect.hpp).
D_NODISCARD inline layout_doc<report_op>
build_via_combinator()
{
    return body({
        cover({ std::pair<std::string, std::string>("subtitle", "nightly build"),
                std::pair<std::string, std::string>("author",   "teer") }),
        clearpage(),
        toc(),
        clearpage(),
        section("Introduction", {
            content("intro.body"),
            section("Motivation", {
                content("intro.motivation") }) }),
        section("Results", {
            table("results.table") }) });
}

// build_via_cursor
//   the procedural surface.
D_NODISCARD inline layout_doc<report_op>
build_via_cursor()
{
    report_document _doc;

    _doc.cover({ std::pair<std::string, std::string>("subtitle", "nightly build"),
                 std::pair<std::string, std::string>("author",   "teer") })
        .clearpage()
        .toc()
        .clearpage();

    _doc.open_section("Introduction");
        _doc.content("intro.body");
        _doc.open_section("Motivation")
                .content("intro.motivation")
            .close_section();
    _doc.close_section();

    _doc.open_section("Results")
            .table("results.table")
        .close_section();

    return _doc.build();
}

// build_via_spec
//   the declarative surface.
D_NODISCARD inline layout_doc<report_op>
build_via_spec()
{
    document_spec _doc = {
        {
            cover_spec{ { std::pair<std::string, std::string>("subtitle", "nightly build"),
                          std::pair<std::string, std::string>("author",   "teer") } },
            break_spec{},
            toc_spec{},
            break_spec{},
            section_spec{ "Introduction", {
                content_spec{ "intro.body" },
                section_spec{ "Motivation", {
                    content_spec{ "intro.motivation" } } } } },
            section_spec{ "Results", {
                table_spec{ "results.table" } } },
        }
    };

    return to_layout(_doc);
}

// surfaces_equivalent
//   function: build the document four ways, render each through the same
// pipeline, and report whether all four agree.  The proof that the surfaces are
// front-ends over one term.
D_NODISCARD inline bool
surfaces_equivalent()
{
    // a shared content resolver + metadata, so only the CONSTRUCTION differs
    content_resolver _body =
        [](const layout_atom& _ref) -> maybe<render_action>
        {
            if (_ref.key == "intro.body")
            {
                return just(render_action([](document_renderer& _r)
                    { _r.paragraph(std::string("intro prose"), doc_attributes()); }));
            }
            if (_ref.key == "intro.motivation")
            {
                return just(render_action([](document_renderer& _r)
                    { _r.paragraph(std::string("motivation prose"), doc_attributes()); }));
            }
            if (_ref.key == "results.table")
            {
                return just(render_action([](document_renderer& _r)
                    { _r.paragraph(std::string("<table>"), doc_attributes()); }));
            }
            return nothing<render_action>();
        };

    layout_signature<report_op> _sig = make_report_signature();

    // render helper
    struct local
    {
        static std::string
        run(const layout_doc<report_op>&  _doc,
            const layout_signature<report_op>& _sig,
            content_resolver              _body)
        {
            doc_attributes _meta;
            _meta.set(std::string("title"), std::string("djinterp"));

            plain_document_renderer _renderer;
            render_document(_doc, _sig, _body, _meta, _renderer);

            return _renderer.str();
        }
    };

    const std::string _a = local::run(build_via_combinator(), _sig, _body);
    const std::string _b = local::run(build_via_cursor(),     _sig, _body);
    const std::string _c = local::run(build_via_spec(),       _sig, _body);

    parse::parse_result<layout_doc<report_op> > _parsed =
        parse_document(report_dsl_source(), report_grammar());

    if (!_parsed.ok())
    {
        return false;
    }

    const std::string _d = local::run(_parsed.value(), _sig, _body);

    return ( (_a == _b) &&
             (_a == _c) &&
             (_a == _d) );
}




///////////////////////////////////////////////////////////////////////////////
///                II.  demo_render                                         ///
///////////////////////////////////////////////////////////////////////////////

// demo_render
//   function: assemble a document with the combinator surface, resolve its
// content by name, and render it to plain text.  A self-contained illustration
// of the whole pipeline (build -> number -> outline -> emit -> run).
D_NODISCARD inline std::string
demo_render()
{
    // structure (combinator surface) -- nests by value
    layout_doc<report_op> _doc = body({
        cover({ std::pair<std::string, std::string>("subtitle", "nightly build"),
                std::pair<std::string, std::string>("author",   "teer") }),
        clearpage(),
        toc(),
        clearpage(),
        section("Introduction", {
            content("intro.body"),
            section("Motivation", {
                content("intro.motivation") }) }),
        section("Results", {
            table("results.table") }) });

    layout_signature<report_op> _sig = make_report_signature();

    // content -- resolved late, by name (a map / file / live computation lives
    // wherever this closure closes over; here, inline)
    content_resolver _body =
        [](const layout_atom& _ref) -> maybe<render_action>
        {
            if (_ref.key == "intro.body")
            {
                return just(render_action(
                    [](document_renderer& _r)
                    {
                        _r.paragraph(
                            std::string("djinterp expresses document "
                                        "structure as data."),
                            doc_attributes());
                    }));
            }

            if (_ref.key == "intro.motivation")
            {
                return just(render_action(
                    [](document_renderer& _r)
                    {
                        _r.paragraph(
                            std::string("One term, many surfaces, many "
                                        "render targets."),
                            doc_attributes());
                    }));
            }

            if (_ref.key == "results.table")
            {
                return just(render_action(
                    [](document_renderer& _r)
                    {
                        _r.begin_table(doc_attributes());
                        _r.table_column(std::string("Metric"), doc_attributes());
                        _r.table_column(std::string("Value"),  doc_attributes());
                        _r.begin_row(doc_attributes());
                        _r.cell(std::string("Assertions"), doc_attributes());
                        _r.cell(std::string("412/412"),    doc_attributes());
                        _r.end_row();
                        _r.end_table();
                    }));
            }

            return nothing<render_action>();
        };

    // metadata -- answers meta_ref leaves and seeds constructs (the cover title)
    doc_attributes _meta;
    _meta.set(std::string("title"), std::string("djinterp"));

    plain_document_renderer _renderer;
    render_document(_doc, _sig, _body, _meta, _renderer);

    return _renderer.str();
}




///////////////////////////////////////////////////////////////////////////////
///                III. register_layout_tests                               ///
///////////////////////////////////////////////////////////////////////////////

// register_layout_tests
//   function: record the layout subframework's checks against _rb.  Call it
// from a runner alongside the other modules.
//
//   report_builder rb;
//   register_layout_tests(rb);
//   rb.finish();
inline void
register_layout_tests(
    report_builder& _rb
)
{
    _rb.module("layout");

    // -- the central claim ---------------------------------------------------

    _rb.open_unit("surfaces",
                  "the four declaration front ends build one term");

    D_CHECK_EQ(_rb, surfaces_equivalent(), true);

    _rb.close_unit();

    // -- the interpreter's four passes ---------------------------------------

    _rb.open_unit("interpreter",
                  "number -> outline -> emit -> run, end to end");

    const std::string _rendered = demo_render();

    // the generated table of contents, and the depth the flat layouts could
    // not express
    D_CHECK_EQ(_rb, contains_helper(_rendered, "Contents"),        true);
    D_CHECK_EQ(_rb, contains_helper(_rendered, "1  Introduction"), true);
    D_CHECK_EQ(_rb, contains_helper(_rendered, "1.1  Motivation"), true);
    D_CHECK_EQ(_rb, contains_helper(_rendered, "2  Results"),      true);

    // the cover's metadata and a rendered table reached the output
    D_CHECK_EQ(_rb, contains_helper(_rendered, "nightly build"),   true);
    D_CHECK_EQ(_rb, contains_helper(_rendered, "Assertions"),      true);

    // a page break separates cover, contents and body
    D_CHECK_EQ(_rb, count_helper(_rendered, '\f'), std::size_t(2));

    _rb.close_unit();


    return;
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_TEST_LAYOUT_SUITE_
