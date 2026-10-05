/*******************************************************************************
* djinterp [core]                                           layout_interpret.hpp
*
*   The interpreter for a document-structure term: how a layout_doc (layout.hpp)
* becomes a rendered document, driving a document_renderer.  Two levels of
* "render behavior" meet here, and they stay separate.  A construct's STRUCTURAL
* lowering -- "a section contributes a heading then its children in order; a
* clearpage forces a page boundary; a toc lists the outline" -- is
* dialect-independent and is interpreted by a fold that consults the signature,
* exactly as expression_render.hpp interprets an operator's layout by consulting
* the operator_signature.  DIALECT realisation -- an 18pt heading in PDF, an
* <h2> in HTML, an underline in text -- is the document_renderer's job, and is
* left entirely to it.  The construct set is open, so it is never frozen into
* the renderer interface; each construct lowers to the renderer's existing
* semantic verbs, and the renderer decides how to draw them.
*
*   THE PIPELINE (render_document).
*     build  -> layout_doc                       (any of the dialect surfaces)
*     number -> re-annotate each section          (renumber; top-down, threaded)
*     outline-> collect (number, title, anchor)   (a fold; feeds the toc)
*     emit   -> a deferred render_action           (annotated_cata to a thunk)
*     run    -> drive the document_renderer
*
*   WHY EMIT FOLDS TO A THUNK.  cata is bottom-up (children fold before their
* parent), but a parent must open BEFORE its children and close AFTER.  So the
* emit fold produces, at each node, a render_action = a deferred emitter; a
* parent composes begin >> children >> end and nothing writes until the root
* action is run.  Emission stays one fold, and ordering is correct.
*
*   NUMBERING IS TOP-DOWN.  Assigning 1, 1.1, 1.2, 2 ... is a pre-order
* traversal threading a counter, so -- honestly -- it is NOT a cata: it is a
* threaded re-annotation (renumber) that rebuilds the annotated tree with each
* section's `number` / `level` / `anchor` written into its bag.  The outline and
* emit passes then read those.  cofree gives the annotated carrier; the traversal
* is a small recursion over it.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust to
* your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/core/util/document/layout/layout_interpret.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    OUTLINE ENTRY                       (a collected table-of-contents row)
      -----------------------------------------------------------------------

II.   RENDER SURFACE                      (render_action / content_resolver / render_ctx)
      -----------------------------------------------------------------------------------

III.  DESCRIPTOR & SIGNATURE              (layout_lowering / layout_descriptor / layout_signature)
      --------------------------------------------------------------------------------------------

IV.   EMIT                                (annotated_cata -> render_action)
      ---------------------------------------------------------------------

V.    OUTLINE                             (fold: sections -> outline entries)
      -----------------------------------------------------------------------

VI.   RENUMBER                            (threaded re-annotation)
      ------------------------------------------------------------

VII.  RENDER_DOCUMENT                     (the whole pipeline)
      --------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_INTERPRET_HPP
#define DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_INTERPRET_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"                 // NS_*, gates, D_NODISCARD
#include "./layout.hpp"                       // layout_doc, layout_atom, builders, observers
#include "../templates/document_attributes.hpp"          // doc_attributes, attr_or, doc_attr_*
#include "../templates/document_renderer.hpp"            // document_renderer
#include "../../../../parse/expression/expression.hpp"    // expr_layer, operator_signature
#include "../../../functional/maybe.hpp"         // maybe, just, nothing
#include "../../../functional/semigroup.hpp"     // mappend
#include "../../../functional/monoid.hpp"        // mconcat


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    OUTLINE ENTRY                                         ///
///////////////////////////////////////////////////////////////////////////////

// outline_entry
//   struct: one row of the collected document outline -- what a table of
// contents lists.  Filled by the outline fold from each in-outline node's bag
// after numbering has run.
struct outline_entry
{
    std::string number;   // "1.2.1", or empty when numbering is off
    std::string title;    // the section title
    std::string anchor;   // cross-reference target (matches doc_attr_locator)
    std::size_t level;    // nesting depth, 1 = outermost

    outline_entry()
        : number(),
          title (),
          anchor(),
          level (1)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///             II.   RENDER SURFACE                                        ///
///////////////////////////////////////////////////////////////////////////////

// render_action
//   type: the deferred emission of one subtree.  Running it drives a
// document_renderer; the emit fold builds these and composes them so a parent
// wraps its children in order.
using render_action = std::function<void(document_renderer&)>;

// content_resolver
//   type: the content side of the boundary -- a body_ref leaf's name to how it
// emits, or nothing when the name is unbound.  Late binding by name: the layout
// carries only the name, the caller supplies the content (a map, a file, a live
// computation closed over here).  A missing binding is a graceful hole, not a
// crash.
using content_resolver =
    std::function<maybe<render_action>(const layout_atom& /*_ref*/)>;

// render_ctx
//   struct: what the emit pass threads to every lowering -- the body resolver,
// the flat metadata bag meta_ref leaves resolve against, the computed outline a
// toc lowering renders, and the base heading level.  Held by reference during a
// run, so it must outlive the render_action the pass returns (render_document
// guarantees this).
struct render_ctx
{
    content_resolver           body;         // body_ref -> render_action (maybe)
    doc_attributes             metadata;     // meta_ref -> string
    std::vector<outline_entry> outline_rows; // computed outline, for the toc
    std::size_t                base_level;   // heading level of a top section

    render_ctx()
        : body        (),
          metadata    (),
          outline_rows(),
          base_level  (1)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///             III.  DESCRIPTOR & SIGNATURE                                ///
///////////////////////////////////////////////////////////////////////////////

// layout_lowering
//   type: how a construct lowers to semantic renderer verbs.  Given this node's
// parameter bag, its already-lowered child actions, the renderer, and the
// context, it emits -- deciding when to run the children (a section emits its
// heading, THEN the children; a clearpage has none).  A plain function pointer,
// so layout_descriptor stays a literal-ish value.
typedef void (*layout_lowering)(const doc_attributes&             /*_bag*/,
                                const std::vector<render_action>&  /*_children*/,
                                document_renderer&                 /*_renderer*/,
                                const render_ctx&                  /*_ctx*/);

// layout_descriptor
//   struct: the per-construct metadata the signature maps each op to -- the
// document analogue of operator_descriptor.  Structural flags drive the number
// and outline passes; the lowering drives emission; spelling names the construct
// for the (future) textual grammar.  A null lowering means "just emit the
// children" (the default container behaviour, e.g. the document body).
struct layout_descriptor
{
    unsigned        min_arity;        // fewest children the construct expects
    bool            starts_new_page;  // advisory: does it force a page boundary?
    bool            in_outline;       // does it contribute a toc entry?
    bool            numbered;         // does the numbering pass number it?
    const char*     spelling;         // the construct keyword (for the grammar)
    layout_lowering lower;            // how it lowers to renderer verbs, or null

    // layout_descriptor (default)
    layout_descriptor()
        : min_arity      (0),
          starts_new_page(false),
          in_outline     (false),
          numbered       (false),
          spelling       (""),
          lower          (nullptr)
    {}

    // layout_descriptor (full)
    layout_descriptor(
        unsigned        _min_arity,
        bool            _starts_new_page,
        bool            _in_outline,
        bool            _numbered,
        const char*     _spelling,
        layout_lowering _lower
    )
        : min_arity      (_min_arity),
          starts_new_page(_starts_new_page),
          in_outline     (_in_outline),
          numbered       (_numbered),
          spelling       (_spelling),
          lower          (_lower)
    {}
};

// layout_signature
//   alias: the described construct set of one document dialect -- reuse
// operator_signature with layout_descriptor as its value.  `.define(op, d)`
// registers a construct; `.describe(op)` recovers it (a first-match walk, the
// runtime face of the lookup family).  Nothing is built in; a dialect IS the
// set it defines.
template<typename OpId>
using layout_signature = operator_signature<OpId, layout_descriptor>;


///////////////////////////////////////////////////////////////////////////////
///             IV.   EMIT                                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // parse_level
    //   helper: the leading unsigned decimal of _s, or 1 when absent -- reads a
    // node's `level` back out of its bag.
    D_NODISCARD inline std::size_t
    parse_level(
        const std::string& _s
    )
    {
        std::size_t _v = 0;
        std::size_t _i = 0;

        while ( (_i < _s.size())    &&
                (_s[_i] >= '0')     &&
                (_s[_i] <= '9') )
        {
            _v = (_v * std::size_t(10)) +
                 static_cast<std::size_t>(_s[_i] - '0');
            ++_i;
        }

        return (_v == 0) ? std::size_t(1) : _v;
    }

    // emit_leaf
    //   helper: emit one content leaf.  A body_ref is resolved through the
    // context's resolver (an unbound name renders a visible placeholder rather
    // than nothing); a meta_ref reads the metadata bag; a literal is verbatim.
    inline void
    emit_leaf(
        const layout_atom&    _atom,
        const doc_attributes& _bag,
        const render_ctx&     _ctx,
        document_renderer&    _renderer
    )
    {
        // body content named by the leaf, resolved late
        if (_atom.kind == layout_atom::kind_body_ref)
        {
            maybe<render_action> _act =
                _ctx.body
                    ? _ctx.body(_atom)
                    : nothing<render_action>();

            if (_act.has_value())
            {
                _act.value()(_renderer);
            }
            else
            {
                _renderer.paragraph(
                    std::string("[unresolved: ") + _atom.key + "]", _bag);
            }

            return;
        }

        // a metadata field
        if (_atom.kind == layout_atom::kind_meta_ref)
        {
            _renderer.paragraph(attr_or(_ctx.metadata, _atom.key), _bag);

            return;
        }

        // inline literal text
        _renderer.paragraph(_atom.value, _bag);

        return;
    }

NS_END  // internal


// emit
//   function: fold the annotated layout to one deferred render_action.  Leaves
// become content-emitting actions (resolved by name at run time); applications
// look up their construct's lowering in _signature and compose an action that
// runs it over the already-lowered child actions.  A construct with no lowering
// (or an unknown op) falls back to emitting its children in order.
//
//   The extension point for a structure-AWARE dialect goes where noted: offer
// the construct name to the renderer first (e.g. a begin_structure hook) and
// skip the default lowering if it claims it.  Left out here so this pass needs
// no change to document_renderer; add the two verbs and the branch if wanted.
template<typename OpId,
         typename Atom>
D_NODISCARD
render_action
emit(
    const layout_doc<OpId, Atom>& _doc,
    const layout_signature<OpId>&  _signature,
    const render_ctx&               _ctx
)
{
    return annotated_cata<render_action>(
        [&_signature, &_ctx]
        (const doc_attributes&                              _bag,
         const expr_layer<OpId, Atom, render_action>&      _layer)
            -> render_action
        {
            // a content leaf: resolve and emit at run time
            if (_layer.is_leaf())
            {
                const Atom           _atom = _layer.atom();
                const doc_attributes _b    = _bag;

                return [_atom, _b, &_ctx](document_renderer& _renderer)
                {
                    internal::emit_leaf(_atom, _b, _ctx, _renderer);
                };
            }

            // an application: its construct's lowering over the lowered children
            const layout_descriptor*   _descriptor =
                _signature.describe(_layer.op());
            std::vector<render_action> _children = _layer.children();
            const doc_attributes       _b        = _bag;

            return
                [_descriptor, _children, _b, &_ctx](document_renderer& _renderer)
                {
                    // (extension point: if _renderer.begin_structure(...) claims
                    //  it, run children then _renderer.end_structure(...) and
                    //  return -- see the header note.)

                    // a described construct lowers itself; otherwise emit the
                    // children in order (the default container behaviour)
                    if ( (_descriptor       != nullptr) &&
                         (_descriptor->lower != nullptr) )
                    {
                        _descriptor->lower(_b, _children, _renderer, _ctx);
                    }
                    else
                    {
                        for (std::size_t _i = 0; _i < _children.size(); ++_i)
                        {
                            _children[_i](_renderer);
                        }
                    }
                };
        },
        _doc);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    OUTLINE                                               ///
///////////////////////////////////////////////////////////////////////////////

// outline
//   function: collect an outline_entry for every in-outline node, in pre-order
// -- the rows a table of contents renders.  A clean bottom-up fold: a node
// contributes its own row (when its descriptor is in_outline) ahead of its
// children's, child lists concatenated through the vector monoid (mconcat), the
// same combinator expression_ops.hpp uses.  Run AFTER renumber so `number` /
// `level` / `anchor` are present in the bags.
template<typename OpId,
         typename Atom>
D_NODISCARD
std::vector<outline_entry>
outline(
    const layout_doc<OpId, Atom>& _doc,
    const layout_signature<OpId>&  _signature
)
{
    return annotated_cata<std::vector<outline_entry> >(
        [&_signature]
        (const doc_attributes&                                       _bag,
         const expr_layer<OpId, Atom, std::vector<outline_entry> >&  _layer)
            -> std::vector<outline_entry>
        {
            // a leaf contributes nothing to the outline
            if (_layer.is_leaf())
            {
                return std::vector<outline_entry>();
            }

            std::vector<outline_entry> _head;

            const layout_descriptor* _descriptor =
                _signature.describe(_layer.op());

            // an in-outline node contributes its own row from its bag
            if ( (_descriptor != nullptr) &&
                 (_descriptor->in_outline) )
            {
                outline_entry _entry;
                _entry.number = attr_or(_bag, std::string("number"));
                _entry.title  = attr_or(_bag, std::string("title"));
                _entry.anchor = attr_or(_bag, std::string("anchor"));
                _entry.level  = internal::parse_level(
                    attr_or(_bag, std::string("level")));

                _head.push_back(_entry);
            }

            // this node's row, then its children's rows (pre-order)
            return mappend(_head, mconcat(_layer.children()));
        },
        _doc);
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   RENUMBER                                              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // to_dec
    //   helper: an unsigned as its decimal spelling (no <string> to_string
    // dependency assumption in the hot path; trivial and portable).
    D_NODISCARD inline std::string
    to_dec(
        std::size_t _n
    )
    {
        if (_n == 0)
        {
            return std::string("0");
        }

        std::string _s;

        while (_n > 0)
        {
            _s.insert(_s.begin(),
                      static_cast<char>('0' + static_cast<int>(_n % 10)));
            _n /= 10;
        }

        return _s;
    }

    // renumber_children
    //   helper: renumber one sibling group under `_prefix` at nesting `_depth`.
    // Each numbered section (per its descriptor) takes the next ordinal, writes
    // `number` / `level` / `anchor` into a copy of its bag, and recurses into
    // its own children under the extended prefix; a non-numbered application
    // passes through, keeping the prefix so it does not break the sequence; a
    // leaf is untouched.  Rebuilds the boxed subtree bottom-out.
    template<typename OpId,
             typename Atom>
    D_NODISCARD
    std::vector<std::shared_ptr<layout_doc<OpId, Atom> > >
    renumber_children(
        const std::vector<std::shared_ptr<layout_doc<OpId, Atom> > >& _kids,
        const std::string&                                              _prefix,
        std::size_t                                                     _depth,
        const layout_signature<OpId>&                                  _signature
    )
    {
        using node_type  = layout_doc<OpId, Atom>;
        using layer_type = typename node_type::layer_type;

        std::vector<std::shared_ptr<node_type> > _out;
        _out.reserve(_kids.size());

        std::size_t _counter = 0;

        for (std::size_t _i = 0; _i < _kids.size(); ++_i)
        {
            const std::shared_ptr<node_type>& _child  = _kids[_i];
            const doc_attributes&             _cbag   = _child->head();
            const layer_type&                 _clayer = _child->unwrap();

            // leaves carry no structure to number
            if (_clayer.is_leaf())
            {
                _out.push_back(_child);
                continue;
            }

            const layout_descriptor* _descriptor =
                _signature.describe(_clayer.op());

            const bool _is_numbered =
                (_descriptor != nullptr) && (_descriptor->numbered);

            // a numbered section: assign its ordinal, decorate, recurse deeper
            if (_is_numbered)
            {
                ++_counter;

                const std::string _number =
                    _prefix.empty()
                        ? to_dec(_counter)
                        : (_prefix + "." + to_dec(_counter));

                doc_attributes _nbag = _cbag;
                _nbag.set(std::string("number"), _number);
                _nbag.set(std::string("level"),  to_dec(_depth));
                _nbag.set(std::string("anchor"), std::string("sec-") + _number);

                std::vector<std::shared_ptr<node_type> > _grand =
                    renumber_children<OpId, Atom>(
                        _clayer.children(), _number, _depth + 1, _signature);

                _out.push_back(
                    std::make_shared<node_type>(
                        node_type::make(
                            _nbag,
                            layer_type::apply(_clayer.op(), _grand))));

                continue;
            }

            // a non-numbered container: keep the prefix, do not increment
            std::vector<std::shared_ptr<node_type> > _grand =
                renumber_children<OpId, Atom>(
                    _clayer.children(), _prefix, _depth, _signature);

            _out.push_back(
                std::make_shared<node_type>(
                    node_type::make(
                        _cbag,
                        layer_type::apply(_clayer.op(), _grand))));
        }

        return _out;
    }

NS_END  // internal


// renumber
//   function: assign hierarchical numbers (1, 1.1, 1.2, 2, ...) to the numbered
// constructs, returning a re-annotated layout with `number` / `level` / `anchor`
// written into each numbered node's bag.  A top-down, counter-threaded
// traversal (not a cata) -- numbering is inherently pre-order and stateful.  The
// root is treated as a single sibling group so a root section is numbered too.
template<typename OpId,
         typename Atom>
D_NODISCARD
layout_doc<OpId, Atom>
renumber(
    const layout_doc<OpId, Atom>& _doc,
    const layout_signature<OpId>&  _signature
)
{
    using node_type = layout_doc<OpId, Atom>;

    std::vector<std::shared_ptr<node_type> > _single;
    _single.push_back(std::make_shared<node_type>(_doc));

    std::vector<std::shared_ptr<node_type> > _out =
        internal::renumber_children<OpId, Atom>(
            _single, std::string(), std::size_t(1), _signature);

    return (*_out[0]);
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  RENDER_DOCUMENT                                       ///
///////////////////////////////////////////////////////////////////////////////

// render_document
//   function: the whole pipeline for one document -- number the term, collect
// its outline into the context, emit the deferred action, and run it against
// _renderer between begin_document / end_document.  _body resolves body-content
// names; _metadata answers meta_ref leaves and seeds constructs (a cover's
// title, say).  Nothing is emitted until the assembled action runs, so the
// context outlives it by construction.
template<typename OpId,
         typename Atom>
void
render_document(
    const layout_doc<OpId, Atom>& _doc,
    const layout_signature<OpId>&  _signature,
    content_resolver                _body,
    doc_attributes                  _metadata,
    document_renderer&              _renderer
)
{
    // 1. number the sections (re-annotate)
    layout_doc<OpId, Atom> _numbered = renumber<OpId, Atom>(_doc, _signature);

    // 2. assemble the context, with the outline computed from the numbered tree
    render_ctx _ctx;
    _ctx.body         = static_cast<content_resolver&&>(_body);
    _ctx.metadata     = static_cast<doc_attributes&&>(_metadata);
    _ctx.outline_rows = outline<OpId, Atom>(_numbered, _signature);

    // 3. emit to a deferred action and 4. run it
    render_action _action = emit<OpId, Atom>(_numbered, _signature, _ctx);

    _renderer.begin_document(doc_attributes());
    _action(_renderer);
    _renderer.end_document();

    return;
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_INTERPRET_HPP
