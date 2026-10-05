/*******************************************************************************
* djinterp [core]                                            report_surfaces.hpp
*
*   The report dialect wearing all FOUR construction surfaces at once, over the
* single layout term.  The combinator surface lives in report_dialect.hpp; this
* header adds the other three and shows every one lowering to the same document:
*
*     - functional / combinator   body(...), section(...)          [report_dialect.hpp]
*     - procedural / cursor        report_document, open_section().content()
*     - declarative / aggregate    document_spec{ ... } + to_layout   (the module_spec idiom)
*     - textual / parsed           parse_document(dsl, report_grammar())
*
* The interpreter is indifferent to which built the term; the check that all
* four agree lives in test/output/test_layout_suite.hpp.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust to
* your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/core/util/document/report_surfaces.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CURSOR SURFACE                      (report_document)
      -----------------------------------------------------

II.   AGGREGATE SURFACE                   (content_spec / section_spec / ... + to_layout)
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
#include "../../../djinterp.hpp"      // NS_*, gates, D_NODISCARD
#include "./report_dialect.hpp"    // report_op, builders, make_report_signature, render_document
#include "layout/layout_build.hpp"      // layout_cursor
#include "layout/layout_spec.hpp"       // spec_node, to_layout
#include "layout/layout_parse.hpp"      // layout_grammar, parse_document


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    CURSOR SURFACE                                        ///
///////////////////////////////////////////////////////////////////////////////

// report_document
//   class: the procedural surface for the report dialect -- named methods over
// the generic layout_cursor.  Open the document (the body root), add constructs
// imperatively, and build.  Sections nest via open_section / close_section;
// everything else is a single call.  build() yields the same term the other
// surfaces do.
//
// Usage:
//   report_document _doc;
//   _doc.cover({ {"author", "teer"} }).clearpage().toc();
//   _doc.open_section("Intro").content("intro.body").close_section();
//   layout_doc<report_op> _term = _doc.build();
class report_document
    : public layout_cursor<report_op>
{
public:
    report_document()
        : layout_cursor<report_op>(report_op::body)
    {}

    // cover
    //   a title page carrying _fields (subtitle / author / date / title).
    report_document&
    cover(
        std::initializer_list<std::pair<std::string, std::string> > _fields
    )
    {
        doc_attributes _bag;

        for (const std::pair<std::string, std::string>& _field : _fields)
        {
            _bag.set(_field.first, _field.second);
        }

        open(report_op::cover, _bag);
        close();

        return (*this);
    }

    // clearpage
    report_document&
    clearpage()
    {
        open(report_op::clearpage);
        close();

        return (*this);
    }

    // toc
    report_document&
    toc()
    {
        open(report_op::toc);
        close();

        return (*this);
    }

    // open_section
    //   begin a section titled _title; content added until close_section nests
    // inside it.
    report_document&
    open_section(
        std::string _title
    )
    {
        doc_attributes _bag;
        _bag.set(std::string("title"), static_cast<std::string&&>(_title));

        open(report_op::section, _bag);

        return (*this);
    }

    // close_section
    report_document&
    close_section()
    {
        close();

        return (*this);
    }

    // content
    //   a body-content leaf named _name.
    report_document&
    content(
        std::string _name
    )
    {
        leaf(layout_atom::body_ref(static_cast<std::string&&>(_name)));

        return (*this);
    }

    // table
    //   a table-content leaf named _name.
    report_document&
    table(
        std::string _name
    )
    {
        leaf(layout_atom::body_ref(static_cast<std::string&&>(_name)));

        return (*this);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   AGGREGATE SURFACE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   The data-literal surface -- typed aggregates, written with nested braces,
// each converting to the generic spec_node so a braced child list may hold
// different construct specs at once.  This is the module_spec idiom applied to
// documents.  `to_layout` (layout_spec.hpp) lowers the result.

// content_spec
//   a body-content leaf named `name`.
struct content_spec
{
    std::string name;

    operator spec_node<report_op>() const
    {
        return spec_node<report_op>::leaf(layout_atom::body_ref(name));
    }
};

// table_spec
//   a table-content leaf named `name`.
struct table_spec
{
    std::string name;

    operator spec_node<report_op>() const
    {
        return spec_node<report_op>::leaf(layout_atom::body_ref(name));
    }
};

// break_spec
//   a page break.
struct break_spec
{
    operator spec_node<report_op>() const
    {
        return spec_node<report_op>::branch(
            report_op::clearpage, doc_attributes(), {});
    }
};

// toc_spec
//   a table of contents.
struct toc_spec
{
    operator spec_node<report_op>() const
    {
        return spec_node<report_op>::branch(
            report_op::toc, doc_attributes(), {});
    }
};

// cover_spec
//   a title page carrying `fields`.
struct cover_spec
{
    std::vector<std::pair<std::string, std::string> > fields;

    operator spec_node<report_op>() const
    {
        doc_attributes _bag;

        for (std::size_t _i = 0; _i < fields.size(); ++_i)
        {
            _bag.set(fields[_i].first, fields[_i].second);
        }

        return spec_node<report_op>::branch(report_op::cover, _bag, {});
    }
};

// section_spec
//   a section titled `title` containing `children` (any mix of specs).
struct section_spec
{
    std::string                      title;
    std::vector<spec_node<report_op> > children;

    operator spec_node<report_op>() const
    {
        doc_attributes _bag;
        _bag.set(std::string("title"), title);

        return spec_node<report_op>::branch(report_op::section, _bag, children);
    }
};

// document_spec
//   the whole document -- a body over `children`.  Lower with to_layout.
struct document_spec
{
    std::vector<spec_node<report_op> > children;

    operator spec_node<report_op>() const
    {
        return spec_node<report_op>::branch(
            report_op::body, doc_attributes(), children);
    }
};

// to_layout
//   convenience: lower a document_spec to the term (via its spec_node).
D_NODISCARD inline layout_doc<report_op>
to_layout(
    const document_spec& _document
)
{
    return to_layout(static_cast<spec_node<report_op> >(_document));
}


///////////////////////////////////////////////////////////////////////////////
///             III.  TEXTUAL SURFACE                                       ///
///////////////////////////////////////////////////////////////////////////////

// report_grammar
//   function: the parse configuration for the report DSL -- the block keywords
// and their ops, the content-leaf keywords, the body root, and the positional
// argument key.  This is all parse_document needs; the parser names nothing.
D_NODISCARD inline layout_grammar<report_op>
report_grammar()
{
    layout_grammar<report_op> _grammar;

    _grammar.root_op = report_op::body;
    _grammar.arg_key = "title";

    _grammar.block_ops = {
        std::pair<std::string, report_op>("cover",     report_op::cover),
        std::pair<std::string, report_op>("clearpage", report_op::clearpage),
        std::pair<std::string, report_op>("toc",       report_op::toc),
        std::pair<std::string, report_op>("section",   report_op::section)
    };

    _grammar.leaf_words = {
        std::pair<std::string, layout_atom::kind_t>(
            "content", layout_atom::kind_body_ref),
        std::pair<std::string, layout_atom::kind_t>(
            "table",   layout_atom::kind_body_ref)
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
// /inc/djinterp/test/output/test_layout_suite.hpp, where the DTest runner
// actually runs them and a failure is reported rather than merely compiled.
//
//   Sections I-III above are library: they are the surfaces themselves.


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_REPORT_SURFACES_HPP
