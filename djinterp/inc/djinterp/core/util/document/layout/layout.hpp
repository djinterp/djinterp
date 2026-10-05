/*******************************************************************************
* djinterp [core]                                                     layout.hpp
*
*   The platform-agnostic DOCUMENT-STRUCTURE term, and nothing that names a
* specific construct.  A document has content and structure; this module is
* the structure -- how a document is organised (title pages, sections, page
* breaks, tables of contents, numbering) with no commitment to text vs PDF vs
* HTML, and no commitment to WHICH constructs exist.  The specific constructs
* derive separately (a dialect header registers them); the foundation stays
* closed to none of them.
*
*   A LAYOUT IS AN ANNOTATED EXPRESSION.  The structure of a document is a tree
* of structural operators over content-reference leaves -- exactly an
* expression (expression.hpp).  Every node also carries a doc_attributes bag:
* its PARAMETERS (a section's title, a cover's author) and, after the
* interpret passes, its COMPUTED data (assigned number, cross-reference
* anchor).  Parameters are part of the layout spec, so they live IN the tree
* as the node's annotation; content is the other half of the document, so it
* lives OUTSIDE the tree and is resolved by name at render time (see
* layout_interpret.hpp).  The annotated term is therefore
*
*     layout<OpId, Atom>      = expression<OpId, Atom>                  (bare)
*     layout_doc<OpId, Atom>  = annotated_expression<OpId, Atom, doc_attributes>
*                             = cofree<expr_layer<OpId, Atom, _>, doc_attributes>
*
* and cofree.hpp makes it foldable by the universal cata, so every
* pass over a layout is one `annotated_cata` with a different algebra.
*
*   GENERIC IN THE OP-ID (both key conventions).  OpId is a free parameter, as
* it is for expression: a dialect keys its constructs by a value (an enum / an
* int code / a fixed_string, resolved through the value-keyed lookup family or
* the runtime operator_signature) for a dynamic layout, or by a type (resolved
* through the type-keyed lookup family) for a fully compile-time one.  The
* foundation imposes neither; it only asks OpId be default-constructible,
* copyable, and equality-comparable -- the same shape the expression term asks.
*
*   THE ATOM IS THE CONTENT BOUNDARY.  A leaf carries a layout_atom: a body_ref
* or meta_ref (a NAME resolved externally -- structure pointing at content it
* does not embed) or a literal (inline text).  The term is generic in the atom
* too (as expression is); layout_atom is the batteries-included default.
*
*   Requires C++14+ (the higher-order deduction the expression / recursion
* layer already uses); self-suppresses below it, mirroring template.hpp.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust
* to your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/core/util/document/layout/layout.hpp
* link(s):   ch-recursion.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    layout_atom                        (the content-reference leaf)
      ---------------------------------------------------------------

II.   TERM ALIASES                       (layout / layout_doc)
      --------------------------------------------------------

III.  NODE BUILDERS                      (apply_node / make_leaf_node +
                                          atom sugar)
      ------------------------------------------------------------------------

IV.   OBSERVERS                          (bag_of / is_leaf / atom_of / ...)
      ---------------------------------------------------------------------

V.    FORGET / ANNOTATE                  (bridge to the bare expression term)
      -----------------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_HPP
#define DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"                       // NS_*, gates, D_NODISCARD
#include "../../../../parse/expression/expression.hpp"          // expression, expr_layer,
                                                    // expr_leaf/apply, annotated_expression
#include "../../../../parse/expression/expression_ops.hpp"      // evaluate (for annotate)
#include "../../../functional/cofree.hpp"              // annotated_cata + env-pair wiring
#include "../templates/document_attributes.hpp"                // doc_attributes


// The layout term leans on the expression / recursion / cofree layer, which is
// C++14+ in practice; below that this module contributes nothing rather than
// failing to compile (the same self-suppression template.hpp uses).
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    layout_atom                                           ///
///////////////////////////////////////////////////////////////////////////////

// layout_atom
//   struct: what a structure leaf carries -- the boundary between structure
// (this module) and content (resolved elsewhere).  A body_ref / meta_ref names
// content by KEY, resolved at render time so the tree never embeds it; a
// literal carries inline text directly.  Default-constructible and copyable, as
// the expression term asks of its atom.
//
// Usage:
//   layout_atom::body_ref("intro.body")   // resolved by the content resolver
//   layout_atom::meta_ref("author")       // resolved against the metadata bag
//   layout_atom::literal("-- fin --")     // inline text, verbatim
struct layout_atom
{
    // kind_t
    //   enum: which flavour of leaf this is.  Prefixed to avoid clashing with
    // the same-named factory functions below.
    enum kind_t
    {
        kind_body_ref,   // key names body content, resolved by the resolver
        kind_meta_ref,   // key names a metadata field, resolved by the bag
        kind_literal     // value is inline text
    };

    kind_t      kind;
    std::string key;     // body_ref / meta_ref: the name
    std::string value;   // literal: the text

    // layout_atom (default)
    //   an empty literal -- the default a boxed expr_layer leaf needs.
    layout_atom()
        : kind (kind_literal),
          key  (),
          value()
    {}

    // body_ref
    //   factory: a leaf naming body content resolved externally by _key.
    D_NODISCARD static layout_atom
    body_ref(
        std::string _key
    )
    {
        layout_atom _a;
        _a.kind = kind_body_ref;
        _a.key  = static_cast<std::string&&>(_key);

        return _a;
    }

    // meta_ref
    //   factory: a leaf naming a metadata field resolved against the metadata
    // bag by _key.
    D_NODISCARD static layout_atom
    meta_ref(
        std::string _key
    )
    {
        layout_atom _a;
        _a.kind = kind_meta_ref;
        _a.key  = static_cast<std::string&&>(_key);

        return _a;
    }

    // literal
    //   factory: a leaf carrying inline text _value emitted verbatim.
    D_NODISCARD static layout_atom
    literal(
        std::string _value
    )
    {
        layout_atom _a;
        _a.kind  = kind_literal;
        _a.value = static_cast<std::string&&>(_value);

        return _a;
    }
};


///////////////////////////////////////////////////////////////////////////////
///             II.   TERM ALIASES                                          ///
///////////////////////////////////////////////////////////////////////////////

// layout
//   alias: the BARE document-structure term over operator ids OpId and atoms
// Atom -- a plain expression, with no per-node annotation.  The form
// expression_ops.hpp's algebraic transforms act on (see forget / annotate).
template<typename OpId,
         typename Atom = layout_atom>
using layout = expression<OpId, Atom>;

// layout_doc
//   alias: the ANNOTATED document-structure term -- every node additionally
// carries a doc_attributes bag (its parameters, then its computed number /
// anchor).  This is the canonical form the builders produce and the interpret
// passes fold; cofree.hpp makes it cata-foldable.
template<typename OpId,
         typename Atom = layout_atom>
using layout_doc = annotated_expression<OpId, Atom, doc_attributes>;


///////////////////////////////////////////////////////////////////////////////
///             III.  NODE BUILDERS                                         ///
///////////////////////////////////////////////////////////////////////////////
//   Construct annotated nodes directly.  cofree has no expr_leaf / expr_apply
// analogue (its children are shared_ptr-boxed), so these wrap the plumbing: an
// application node from an op, its params bag, and its child subtrees; a leaf
// node from an atom and its bag.  A dialect's fluent / combinator / aggregate
// surfaces all bottom out here.

// apply_node
//   function: an annotated application -- operator _op applied to _children
// (which may be empty), carrying the parameter bag _bag.  The children are
// boxed into the shared_ptr holes cofree stores.
template<typename OpId,
         typename Atom>
D_NODISCARD
layout_doc<OpId, Atom>
apply_node(
    doc_attributes                       _bag,
    const OpId&                         _op,
    std::vector<layout_doc<OpId, Atom> > _children
)
{
    using node_type  = layout_doc<OpId, Atom>;
    using layer_type = typename node_type::layer_type;

    std::vector<std::shared_ptr<node_type> > _boxed;
    _boxed.reserve(_children.size());

    // box each child subtree into a shared_ptr hole
    for (std::size_t _i = 0; _i < _children.size(); ++_i)
    {
        _boxed.push_back(
            std::make_shared<node_type>(
                static_cast<node_type&&>(_children[_i])));
    }

    return node_type::make(
        _bag,
        layer_type::apply(_op, _boxed));
}

// make_leaf_node
//   function: an annotated leaf carrying the atom _atom and the bag _bag.
template<typename OpId,
         typename Atom>
D_NODISCARD
layout_doc<OpId, Atom>
make_leaf_node(
    doc_attributes _bag,
    const Atom&   _atom
)
{
    using node_type  = layout_doc<OpId, Atom>;
    using layer_type = typename node_type::layer_type;

    return node_type::make(
        _bag,
        layer_type::leaf(_atom));
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   OBSERVERS                                             ///
///////////////////////////////////////////////////////////////////////////////
//   Thin readers over an annotated node.  The annotation is the head; the node
// kind, atom, operator, and (boxed) children live in the layer.

// bag_of
//   function: the parameter / computed-data bag annotating this node.
template<typename OpId,
         typename Atom>
D_NODISCARD
const doc_attributes&
bag_of(
    const layout_doc<OpId, Atom>& _node
)
{
    return _node.head();
}

// is_leaf
//   function: whether this node's root is a content leaf.
template<typename OpId,
         typename Atom>
D_NODISCARD
bool
is_leaf(
    const layout_doc<OpId, Atom>& _node
)
{
    return _node.unwrap().is_leaf();
}

// atom_of
//   function: the atom at a leaf node.  Precondition: is_leaf.
template<typename OpId,
         typename Atom>
D_NODISCARD
const Atom&
atom_of(
    const layout_doc<OpId, Atom>& _node
)
{
    return _node.unwrap().atom();
}

// op_of
//   function: the operator id at an application node.  Precondition: !is_leaf.
template<typename OpId,
         typename Atom>
D_NODISCARD
const OpId&
op_of(
    const layout_doc<OpId, Atom>& _node
)
{
    return _node.unwrap().op();
}

// children_of
//   function: the boxed child subtrees at an application node.
template<typename OpId,
         typename Atom>
D_NODISCARD
const std::vector<std::shared_ptr<layout_doc<OpId, Atom> > >&
children_of(
    const layout_doc<OpId, Atom>& _node
)
{
    return _node.unwrap().children();
}


///////////////////////////////////////////////////////////////////////////////
///             V.    FORGET / ANNOTATE                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Bridges to the bare expression term.  The env-pair keeps everything in one
// annotated tree; when an algebraic rewrite from expression_ops.hpp is wanted
// (transform_bottom_up, rewrite_to_fixpoint, ...), drop the annotations, rewrite
// the bare term, and re-annotate.  Both directions are one fold.

// forget
//   function: drop every annotation, yielding the bare layout term -- an
// annotated_cata that rebuilds each node with expr_leaf / expr_apply.  After
// this the expression_ops transforms apply unchanged.
template<typename OpId,
         typename Atom>
D_NODISCARD
layout<OpId, Atom>
forget(
    const layout_doc<OpId, Atom>& _doc
)
{
    return annotated_cata<layout<OpId, Atom> >(
        [](const doc_attributes&                                      /*_bag*/,
           const expr_layer<OpId, Atom, layout<OpId, Atom> >&       _layer)
            -> layout<OpId, Atom>
        {
            // a leaf keeps its atom; an application keeps its op and (folded)
            // children
            if (_layer.is_leaf())
            {
                return expr_leaf<OpId, Atom>(_layer.atom());
            }

            return expr_apply<OpId, Atom>(_layer.op(), _layer.children());
        },
        _doc);
}

// annotate
//   function: lift a bare layout term into an annotated one, giving every node
// the bag produced by _tag (a node-shape -> doc_attributes).  Pass a tagger
// that returns an empty bag to re-enter the annotated world after a rewrite;
// pass a richer one to seed params structurally.  A cata over the bare term
// building make_leaf_node / apply_node.
template<typename OpId,
         typename Atom,
         typename Tagger>
D_NODISCARD
layout_doc<OpId, Atom>
annotate(
    const layout<OpId, Atom>& _term,
    Tagger                      _tag
)
{
    return evaluate<layout_doc<OpId, Atom> >(
        _term,
        // on_leaf -- tag the atom, wrap as a leaf node
        [_tag](const Atom& _atom) -> layout_doc<OpId, Atom>
        {
            return make_leaf_node<OpId, Atom>(_tag(_atom), _atom);
        },
        // on_apply -- tag the op, wrap the (already-annotated) children
        [_tag](const OpId&                                       _op,
               const std::vector<layout_doc<OpId, Atom> >&        _children)
            -> layout_doc<OpId, Atom>
        {
            return apply_node<OpId, Atom>(_tag(_op), _op, _children);
        });
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_HPP
