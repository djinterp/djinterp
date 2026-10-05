/*******************************************************************************
* djinterp [core]                                             report_dialect.hpp
*
*   A worked DIALECT over the layout foundation -- the template the rest follow.
* It shows the whole "constructs derive separately, nothing hardcoded in the
* foundation" pattern end to end: a construct is an operator id, a signature
* entry (its structural flags + spelling), a builder, and a lowering to the
* renderer's semantic verbs.  layout.hpp / layout_interpret.hpp name none of
* these; this header defines them and registers them into a layout_signature.
*
*   FIVE CONSTRUCTS.
*     body       -- the document container (no lowering: emit its children)
*     cover      -- a title page (lowers by DRIVING the existing title_page
*                   content model -- an existing construct becomes a lowering)
*     clearpage  -- a page break (lowers to the renderer's page_break verb)
*     toc        -- a table of contents (lowers by rendering the computed outline)
*     section    -- a numbered, outlined heading + its children
*   plus two content leaves (content / table -- body_ref names the resolver
* answers).  The op-id here is an enum (a value key); a fixed_string or a
* type-tag dialect is the same shape with a different OpId.
*
*   THE FOUR SURFACES all bottom out in these builders.  This header shows the
* combinator surface (body(...), section(...), ...); a fluent cursor builder, an
* aggregate *_spec + to_layout, and the block-form parser are additive over the
* same builders and are not shown here.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust to
* your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/core/util/document/report_dialect.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    OP IDS                             (report_op)
      ----------------------------------------------

II.   LOWERINGS                          (section / clearpage / toc / cover)
      ----------------------------------------------------------------------

III.  BUILDERS                           (combinator surface)
      -------------------------------------------------------

IV.   SIGNATURE                          (make_report_signature)
      ----------------------------------------------------------

V.    DEMO                               (moved -> test_layout_suite.hpp)
      -------------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_REPORT_DIALECT_HPP
#define DJINTERP_UTIL_DOCUMENT_REPORT_DIALECT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"         // NS_*, gates, D_NODISCARD
#include "layout/layout_interpret.hpp"     // layout_doc, builders, signature, render pipeline
#include "./templates/document_renderer.hpp"    // document_renderer, plain_document_renderer
#include "./templates/title_page.hpp"           // title_page (cover lowers by driving it)


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    OP IDS                                                ///
///////////////////////////////////////////////////////////////////////////////

// report_op
//   enum: the constructs of this dialect, keyed by value.  A value key resolves
// through the runtime signature (operator_signature::describe) and the value-
// keyed lookup family; a fixed_string or type-tag key would serve a
// self-describing or compile-time dialect instead.  Default-constructible and
// equality-comparable, as the term asks of an op-id.
enum class report_op
{
    body,        // the document container
    cover,       // a title page
    clearpage,   // a page break
    toc,         // a table of contents
    section      // a numbered heading + children
    // (content / table are LEAVES -- layout_atom::body_ref -- not ops)
};


///////////////////////////////////////////////////////////////////////////////
///             II.   LOWERINGS                                             ///
///////////////////////////////////////////////////////////////////////////////
//   Each construct lowers to the renderer's EXISTING semantic verbs; the
// renderer realises them per dialect.  The construct set stays open -- none of
// this touches the renderer interface.

// section_lower
//   the heading (its assigned number, then its title) followed by its children
// in order.  Reads number / level / anchor the numbering pass wrote into the
// bag; carries the anchor as a locator hint so a renderer that anchors headings
// can.
inline void
section_lower(
    const doc_attributes&             _bag,
    const std::vector<render_action>& _children,
    document_renderer&                _renderer,
    const render_ctx&                 /*_ctx*/
)
{
    const std::string _title  = attr_or(_bag, std::string("title"));
    const std::string _number = attr_or(_bag, std::string("number"));
    const std::string _anchor = attr_or(_bag, std::string("anchor"));
    const std::size_t _level  =
        internal::parse_level(attr_or(_bag, std::string("level")));

    doc_attributes _heading_attrs;
    _heading_attrs.set(doc_attr_style, std::string("section.heading"));

    // an anchor lets a linking dialect target this heading; text ignores it
    if (!_anchor.empty())
    {
        _heading_attrs.set(doc_attr_locator, _anchor);
    }

    _renderer.heading(
        _level,
        _number.empty() ? _title : (_number + "  " + _title),
        _heading_attrs);

    // the children come after the heading, in order
    for (std::size_t _i = 0; _i < _children.size(); ++_i)
    {
        _children[_i](_renderer);
    }

    return;
}

// clearpage_lower
//   a page boundary -- the renderer's own verb; a continuous dialect ignores it.
inline void
clearpage_lower(
    const doc_attributes&             /*_bag*/,
    const std::vector<render_action>& /*_children*/,
    document_renderer&                _renderer,
    const render_ctx&                 /*_ctx*/
)
{
    _renderer.page_break();

    return;
}

// toc_lower
//   a "Contents" heading, then one line per collected outline row, indented by
// depth, carrying the row's anchor as a locator hint.  The rows were computed
// from the numbered tree before emission (render_ctx::outline_rows).
inline void
toc_lower(
    const doc_attributes&             /*_bag*/,
    const std::vector<render_action>& /*_children*/,
    document_renderer&                _renderer,
    const render_ctx&                 _ctx
)
{
    doc_attributes _title_attrs;
    _title_attrs.set(doc_attr_style, std::string("toc.title"));
    _renderer.heading(std::size_t(1), std::string("Contents"), _title_attrs);

    for (std::size_t _i = 0; _i < _ctx.outline_rows.size(); ++_i)
    {
        const outline_entry& _entry = _ctx.outline_rows[_i];

        doc_attributes _row_attrs;

        // the cross-reference target for a linking dialect
        if (!_entry.anchor.empty())
        {
            _row_attrs.set(doc_attr_locator, _entry.anchor);
        }

        // indent by nesting depth (two spaces per level below the first)
        const std::size_t _pad =
            (_entry.level > 0) ? ((_entry.level - 1) * 2) : 0;
        const std::string _indent(_pad, ' ');

        const std::string _line =
            _entry.number.empty()
                ? (_indent + _entry.title)
                : (_indent + _entry.number + "  " + _entry.title);

        _renderer.paragraph(_line, _row_attrs);
    }

    return;
}

// cover_lower
//   drives the EXISTING title_page content model -- an existing construct
// serving as a layout op's lowering.  Pulls title / subtitle / author / date
// from the cover's own bag, falling back to the document metadata; and turns
// OFF title_page's own page break, because pagination is the layout's to own
// (a clearpage construct), not an embedded construct's.
inline void
cover_lower(
    const doc_attributes&             _bag,
    const std::vector<render_action>& /*_children*/,
    document_renderer&                _renderer,
    const render_ctx&                 _ctx
)
{
    title_page _page;

    const std::string _title =
        attr_or(_bag, std::string("title"),
                attr_or(_ctx.metadata, std::string("title")));
    const std::string _subtitle =
        attr_or(_bag, std::string("subtitle"),
                attr_or(_ctx.metadata, std::string("subtitle")));
    const std::string _author =
        attr_or(_bag, std::string("author"),
                attr_or(_ctx.metadata, std::string("author")));
    const std::string _date =
        attr_or(_bag, std::string("date"),
                attr_or(_ctx.metadata, std::string("date")));

    if (!_title.empty())    { _page.set_title(_title);       }
    if (!_subtitle.empty()) { _page.set_subtitle(_subtitle); }
    if (!_author.empty())   { _page.set_author(_author);     }
    if (!_date.empty())     { _page.set_date(_date);         }

    // the LAYOUT owns page breaks, not the embedded cover
    _page.set_page_break(false);

    _page.render(_renderer);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  BUILDERS                                              ///
///////////////////////////////////////////////////////////////////////////////
//   The combinator surface: pure, nests by value, one call per construct.  Each
// produces an annotated node through apply_node / make_leaf_node (layout.hpp).

// body
//   the document container -- the root a document is assembled under.
D_NODISCARD inline layout_doc<report_op>
body(
    std::vector<layout_doc<report_op> > _children
)
{
    return apply_node<report_op, layout_atom>(
        doc_attributes(),
        report_op::body,
        static_cast<std::vector<layout_doc<report_op> >&&>(_children));
}

// cover
//   a title page -- _fields are cover metadata (subtitle / author / date /
// title), stored in the node's bag for cover_lower to read.
D_NODISCARD inline layout_doc<report_op>
cover(
    std::initializer_list<std::pair<std::string, std::string> > _fields
)
{
    doc_attributes _bag;

    for (const std::pair<std::string, std::string>& _field : _fields)
    {
        _bag.set(_field.first, _field.second);
    }

    return apply_node<report_op, layout_atom>(
        _bag,
        report_op::cover,
        std::vector<layout_doc<report_op> >());
}

// clearpage
//   force following content onto a new page.
D_NODISCARD inline layout_doc<report_op>
clearpage()
{
    return apply_node<report_op, layout_atom>(
        doc_attributes(),
        report_op::clearpage,
        std::vector<layout_doc<report_op> >());
}

// toc
//   a table of contents -- rendered from the outline computed at render time.
D_NODISCARD inline layout_doc<report_op>
toc()
{
    return apply_node<report_op, layout_atom>(
        doc_attributes(),
        report_op::toc,
        std::vector<layout_doc<report_op> >());
}

// section
//   a numbered, outlined heading titled _title, containing _children.
D_NODISCARD inline layout_doc<report_op>
section(
    std::string                         _title,
    std::vector<layout_doc<report_op> > _children
)
{
    doc_attributes _bag;
    _bag.set(std::string("title"), static_cast<std::string&&>(_title));

    return apply_node<report_op, layout_atom>(
        _bag,
        report_op::section,
        static_cast<std::vector<layout_doc<report_op> >&&>(_children));
}

// content
//   a body-content leaf named _name -- the resolver answers it at render time.
D_NODISCARD inline layout_doc<report_op>
content(
    std::string _name
)
{
    return make_leaf_node<report_op, layout_atom>(
        doc_attributes(),
        layout_atom::body_ref(static_cast<std::string&&>(_name)));
}

// table
//   a table-content leaf named _name -- like content, resolved by name (the
// resolver decides it renders a table).
D_NODISCARD inline layout_doc<report_op>
table(
    std::string _name
)
{
    return make_leaf_node<report_op, layout_atom>(
        doc_attributes(),
        layout_atom::body_ref(static_cast<std::string&&>(_name)));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   SIGNATURE                                             ///
///////////////////////////////////////////////////////////////////////////////

// make_report_signature
//   function: the described construct set of this dialect.  Each entry pairs an
// op with its layout_descriptor: structural flags (starts_new_page / in_outline
// / numbered) drive the number and outline passes, spelling names it, and the
// lowering drives emission.  body carries a null lowering (its default is to
// emit its children).
D_NODISCARD inline layout_signature<report_op>
make_report_signature()
{
    layout_signature<report_op> _sig;

    //             op                     arity  page   toc    num    spelling      lowering
    _sig.define(report_op::body,
                layout_descriptor(0,      false, false, false, "body",      nullptr));
    _sig.define(report_op::cover,
                layout_descriptor(0,      false, false, false, "cover",     &cover_lower));
    _sig.define(report_op::clearpage,
                layout_descriptor(0,      true,  false, false, "clearpage", &clearpage_lower));
    _sig.define(report_op::toc,
                layout_descriptor(0,      false, false, false, "toc",       &toc_lower));
    _sig.define(report_op::section,
                layout_descriptor(0,      false, true,  true,  "section",   &section_lower));

    return _sig;
}


///////////////////////////////////////////////////////////////////////////////
///             V.    DEMO  (moved)                                         ///
///////////////////////////////////////////////////////////////////////////////
//   demo_render() -- assemble a document with the combinators above and render
// it to plain text -- moved to /inc/djinterp/test/output/test_layout_suite.hpp.
// It is a worked example AND an end-to-end check of the four interpreter
// passes; as a test it is run, as a header function it was only compiled.


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_REPORT_DIALECT_HPP
