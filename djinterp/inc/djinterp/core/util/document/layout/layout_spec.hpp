/*******************************************************************************
* djinterp [core]                                                layout_spec.hpp
*
*   The DECLARATIVE construction surface -- a pure-data description of a document
* lowered to the term by a free function, the idiom the test framework's
* module_spec / block_spec use.  A spec is a plain aggregate tree written with
* nested braces; `to_layout` lowers it to a layout_doc.  No fluent chaining, no
* interpretation -- just data, so a document literal reads like an outline.
*
*   HETEROGENEOUS CHILDREN, ONE NODE.  A section's children are a mix of
* content, sub-sections, tables, breaks -- so the generic carrier is one
* spec_node variant (leaf-or-branch), and a dialect's typed *_spec aggregates
* convert INTO it.  That conversion is what lets a braced child list hold
* different construct specs at once (each converts to spec_node), exactly as
* block_spec's list holds homogeneous entries -- generalised to a document's
* heterogeneity.  The typed specs live in the dialect (they name constructs);
* only the variant and the lowering are generic and live here.
*
*   ONE TERM, STILL.  `to_layout` produces the SAME annotated term the
* combinator and cursor surfaces produce; content leaves stay names resolved
* later, params stay in the bag.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust to
* your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/core/util/document/layout/layout_spec.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_SPEC_HPP
#define DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_SPEC_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"    // NS_*, gates, D_NODISCARD
#include "./layout.hpp"          // layout_doc, apply_node, make_leaf_node


#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP


// spec_node
//   struct: the generic data-literal carrier -- a leaf (an atom) or a branch
// (an op applied to child spec_nodes), each with a params bag.  A dialect's
// typed *_spec aggregates provide `operator spec_node()` so that a braced child
// list of different construct specs is a vector<spec_node> (each element
// converts).  A plain aggregate; build it with the factories or brace-init.
template<typename OpId,
         typename Atom = layout_atom>
struct spec_node
{
    bool                       is_leaf;
    OpId                       op;        // branch: the construct
    Atom                       atom;      // leaf: the content reference
    doc_attributes             bag;       // params (branch) / hints (leaf)
    std::vector<spec_node>     children;  // branch: sub-structure

    // spec_node (default)
    //   an empty leaf -- the aggregate default.
    spec_node()
        : is_leaf (true),
          op      (),
          atom    (),
          bag     (),
          children()
    {}

    // branch
    //   factory: an application spec -- op _op over _children, params _bag.
    D_NODISCARD static spec_node
    branch(
        const OpId&           _op,
        doc_attributes         _bag,
        std::vector<spec_node> _children
    )
    {
        spec_node _n;
        _n.is_leaf  = false;
        _n.op       = _op;
        _n.bag      = _bag;
        _n.children = static_cast<std::vector<spec_node>&&>(_children);

        return _n;
    }

    // leaf
    //   factory: a content-leaf spec carrying _atom (and optional hints _bag).
    D_NODISCARD static spec_node
    leaf(
        const Atom&   _atom,
        doc_attributes _bag = doc_attributes()
    )
    {
        spec_node _n;
        _n.is_leaf = true;
        _n.atom    = _atom;
        _n.bag     = _bag;

        return _n;
    }
};


// to_layout
//   function: lower a spec tree to the annotated layout term.  A leaf becomes a
// make_leaf_node; a branch becomes an apply_node over its lowered
// children.  Plain structural recursion (specs are finite data), producing
// the same term the other surfaces produce.
template<typename OpId,
         typename Atom>
D_NODISCARD
layout_doc<OpId, Atom>
to_layout(
    const spec_node<OpId, Atom>& _spec
)
{
    // a content leaf
    if (_spec.is_leaf)
    {
        return make_leaf_node<OpId, Atom>(_spec.bag, _spec.atom);
    }

    // a branch: lower each child, then apply
    std::vector<layout_doc<OpId, Atom> > _children;
    _children.reserve(_spec.children.size());

    for (std::size_t _i = 0; _i < _spec.children.size(); ++_i)
    {
        _children.push_back(to_layout(_spec.children[_i]));
    }

    return apply_node<OpId, Atom>(
        _spec.bag,
        _spec.op,
        static_cast<std::vector<layout_doc<OpId, Atom> >&&>(_children));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_SPEC_HPP
