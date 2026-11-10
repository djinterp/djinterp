/*******************************************************************************
* djinterp [cpp]                                             report_surfaces.hpp
*
*   The report dialect wearing all FOUR construction surfaces at once, over the
* single layout term.  The combinator surface lives in report_dialect.hpp; this
* header adds the other three and shows every one lowering to the same document:
*
*     - functional / combinator   body(...), section(...)          [report_dialect.hpp]
*     - procedural / cursor        DReportDocument, open_section().content()
*     - declarative / aggregate    DDocumentSpec{ ... } + to_layout   (the module_spec idiom)
*     - textual / parsed           parse_document(dsl, report_grammar())
*
* The interpreter is indifferent to which built the term; the check that all
* four agree lives in test/output/test_layout_suite.hpp.
*
*   NOTE: include paths are relative to /inc/djinterp/cpp/util/document/; adjust
* to your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/cpp/util/document/report_surfaces.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CURSOR SURFACE                      (DReportDocument)
      -----------------------------------------------------

II.   AGGREGATE SURFACE                   (DContentSpec / DSectionSpec / ... + to_layout)
      -----------------------------------------------------------------------------------

III.  TEXTUAL SURFACE                     (report_grammar)
      ----------------------------------------------------

IV.   EQUIVALENCE                         (moved -> test_layout_suite.hpp)
      --------------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_REPORT_SURFACES_HPP
#define DJINTERP_UTIL_DOCUMENT_REPORT_SURFACES_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"      // NS_*, gates, D_NODISCARD
#include "./report_dialect.hpp"    // DReportOp, builders, make_report_signature, render_document
#include "layout/layout_build.hpp"      // DLayoutCursor
#include "layout/layout_spec.hpp"       // DSpecNode, to_layout
#include "layout/layout_parse.hpp"      // DLayoutGrammar, parse_document


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    CURSOR SURFACE                                        ///
///////////////////////////////////////////////////////////////////////////////

// DReportDocument
//   class: the procedural surface for the report dialect -- named methods over
// the generic DLayoutCursor.  Open the document (the body root), add constructs
// imperatively, and build.  Sections nest via open_section / close_section;
// everything else is a single call.  build() yields the same term the other
// surfaces do.
//
// Usage:
//   DReportDocument _doc;
//   _doc.cover({ {"author", "teer"} }).clearpage().toc();
//   _doc.open_section("Intro").content("intro.body").close_section();
//   DLayoutDoc<DReportOp> _term = _doc.build();
class DReportDocument
    : public DLayoutCursor<DReportOp>
{
public:
    DReportDocument()
        : DLayoutCursor<DReportOp>(DReportOp::body)
    {}

    // cover
    //   a title page carrying _fields (subtitle / author / date / title).
    DReportDocument& cover(
        std::initializer_list<std::pair<std::string, std::string> > _fields
    )
    {
        DDocAttributes _bag;

        for (const std::pair<std::string, std::string>& _field : _fields)
        {
            _bag.set(_field.first, _field.second);
        }

        open(DReportOp::cover, _bag);
        close();

        return (*this);
    }

    // clearpage
    DReportDocument& clearpage()
    {
        open(DReportOp::clearpage);
        close();

        return (*this);
    }

    // toc
    DReportDocument& toc()
    {
        open(DReportOp::toc);
        close();

        return (*this);
    }

    // open_section
    //   begin a section titled _title; content added until close_section nests
    // inside it.
    DReportDocument& open_section(
        std::string _title
    )
    {
        DDocAttributes _bag;
        _bag.set(std::string("title"), static_cast<std::string&&>(_title));

        open(DReportOp::section, _bag);

        return (*this);
    }

    // close_section
    DReportDocument& close_section()
    {
        close();

        return (*this);
    }

    // content
    //   a body-content leaf named _name.
    DReportDocument& content(
        std::string _name
    )
    {
        leaf(DLayoutAtom::body_ref(static_cast<std::string&&>(_name)));

        return (*this);
    }

    // table
    //   a table-content leaf named _name.
    DReportDocument& table(
        std::string _name
    )
    {
        leaf(DLayoutAtom::body_ref(static_cast<std::string&&>(_name)));

        return (*this);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   AGGREGATE SURFACE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   The data-literal surface -- typed aggregates, written with nested braces,
// each converting to the generic DSpecNode so a braced child list may hold
// different construct specs at once.  This is the module_spec idiom applied to
// documents.  `to_layout` (layout_spec.hpp) lowers the result.

// DContentSpec
//   a body-content leaf named `name`.
struct DContentSpec
{
    std::string name;

    operator DSpecNode<DReportOp>() const
    {
        return DSpecNode<DReportOp>::leaf(DLayoutAtom::body_ref(name));
    }
};

// DTableSpec
//   a table-content leaf named `name`.
struct DTableSpec
{
    std::string name;

    operator DSpecNode<DReportOp>() const
    {
        return DSpecNode<DReportOp>::leaf(DLayoutAtom::body_ref(name));
    }
};

// DBreakSpec
//   a page break.
struct DBreakSpec
{
    operator DSpecNode<DReportOp>() const
    {
        return DSpecNode<DReportOp>::branch(
            DReportOp::clearpage, DDocAttributes(), {});
    }
};

// DTocSpec
//   a table of contents.
struct DTocSpec
{
    operator DSpecNode<DReportOp>() const
    {
        return DSpecNode<DReportOp>::branch(
            DReportOp::toc, DDocAttributes(), {});
    }
};

// DCoverSpec
//   a title page carrying `fields`.
struct DCoverSpec
{
    std::vector<std::pair<std::string, std::string> > fields;

    operator DSpecNode<DReportOp>() const
    {
        DDocAttributes _bag;

        for (std::size_t _i = 0; _i < fields.size(); ++_i)
        {
            _bag.set(fields[_i].first, fields[_i].second);
        }

        return DSpecNode<DReportOp>::branch(DReportOp::cover, _bag, {});
    }
};

// DSectionSpec
//   a section titled `title` containing `children` (any mix of specs).
struct DSectionSpec
{
    std::string                      title;
    std::vector<DSpecNode<DReportOp> > children;

    operator DSpecNode<DReportOp>() const
    {
        DDocAttributes _bag;
        _bag.set(std::string("title"), title);

        return DSpecNode<DReportOp>::branch(DReportOp::section, _bag, children);
    }
};

// DDocumentSpec
//   the whole document -- a body over `children`.  Lower with to_layout.
struct DDocumentSpec
{
    std::vector<DSpecNode<DReportOp> > children;

    operator DSpecNode<DReportOp>() const
    {
        return DSpecNode<DReportOp>::branch(
            DReportOp::body, DDocAttributes(), children);
    }
};

// to_layout
//   convenience: lower a DDocumentSpec to the term (via its DSpecNode).
D_NODISCARD inline DLayoutDoc<DReportOp> to_layout(
    const DDocumentSpec& _document
)
{
    return to_layout(static_cast<DSpecNode<DReportOp> >(_document));
}


///////////////////////////////////////////////////////////////////////////////
///             III.  TEXTUAL SURFACE                                       ///
///////////////////////////////////////////////////////////////////////////////

// report_grammar
//   function: the parse configuration for the report DSL -- the block keywords
// and their ops, the content-leaf keywords, the body root, and the positional
// argument key.  This is all parse_document needs; the parser names nothing.
D_NODISCARD inline DLayoutGrammar<DReportOp> report_grammar()
{
    DLayoutGrammar<DReportOp> _grammar;

    _grammar.root_op = DReportOp::body;
    _grammar.arg_key = "title";

    _grammar.block_ops = {
        std::pair<std::string, DReportOp>("cover",     DReportOp::cover),
        std::pair<std::string, DReportOp>("clearpage", DReportOp::clearpage),
        std::pair<std::string, DReportOp>("toc",       DReportOp::toc),
        std::pair<std::string, DReportOp>("section",   DReportOp::section)
    };

    _grammar.leaf_words = {
        std::pair<std::string, DLayoutAtom::kind_t>(
            "content", DLayoutAtom::kind_body_ref),
        std::pair<std::string, DLayoutAtom::kind_t>(
            "table",   DLayoutAtom::kind_body_ref)
    };

    return _grammar;
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   EQUIVALENCE  (moved)                                  ///
///////////////////////////////////////////////////////////////////////////////
//   The four fixtures that build the same document each way, and the check
// that they agree, used to live here.  They are TESTS, not library code -- a
// header that ships its own assertions cannot be exercised by the harness and
// grows a second, unrun copy of the truth.  They now live in
// /inc/djinterp/cpp/modules/test/output/test_layout_suite.hpp, where the DTest runner
// actually runs them and a failure is reported rather than merely compiled.
//
//   Sections I-III above are library: they are the surfaces themselves.


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_REPORT_SURFACES_HPP
