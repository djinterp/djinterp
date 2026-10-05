/*******************************************************************************
* djinterp [core]                                               layout_build.hpp
*
*   The PROCEDURAL construction surface -- a stateful cursor that assembles a
* layout_doc through imperative open / close / leaf calls, the counterpart to
* the functional combinator surface.  The framework strives for a functional
* AND a procedural idiom throughout; this is the procedural half for documents,
* and it mirrors the cursor style document_writer already uses.
*
*   A TREE FROM A STACK.  The cursor holds a stack of open frames; the bottom is
* the root container.  `open` pushes a frame, `close` pops it and attaches the
* finished subtree to its parent, `leaf` / `child` append to the current frame.
* `build` closes any still-open frames and returns the root -- so a forgotten
* close is completed rather than lost.  Every method returns *this, so calls
* chain.  The result is the SAME annotated term the other surfaces produce; the
* interpreter never learns which surface built it.
*
*   Generic in the op-id and atom, as the term is.  A dialect wraps these with
* named methods (open_section(title) over open(section_op, {title})); the
* generic cursor imposes no construct.
*
*   NOTE: include paths are relative to /inc/djinterp/core/util/document/; adjust to
* your build's -I roots if they differ.
*
*
* path:      /inc/djinterp/core/util/document/layout/layout_build.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_BUILD_HPP
#define DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_BUILD_HPP 1

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


// layout_cursor
//   class: a stateful builder for a layout_doc.  Open a construct, add its
// content and sub-constructs, close it; build the finished document.  A stack
// of frames turns the imperative call sequence into the annotated tree; the
// bottom frame is the root container, and each close folds a frame into its
// parent's children.
//
// Usage (generic; a dialect adds named wrappers over open/leaf/close):
//   layout_cursor<report_op> _doc(report_op::body);
//   _doc.open(report_op::section, section_bag("Intro"))
//         .leaf(layout_atom::body_ref("intro.body"))
//       .close();
//   layout_doc<report_op> _term = _doc.build();
template<typename OpId,
         typename Atom = layout_atom>
class layout_cursor
{
public:
    using node_type = layout_doc<OpId, Atom>;

    // layout_cursor
    //   opens the root container (the bottom frame) with op _root_op and the
    // optional bag _root_bag; everything added accrues to it until build.
    explicit layout_cursor(
        const OpId&   _root_op,
        doc_attributes _root_bag = doc_attributes()
    )
        : m_stack()
    {
        m_stack.push_back(frame{ _root_op, _root_bag, {} });
    }

    // open
    //   push a new construct frame; subsequent content nests inside it until
    // the matching close.
    layout_cursor&
    open(
        const OpId&   _op,
        doc_attributes _bag = doc_attributes()
    )
    {
        m_stack.push_back(frame{ _op, _bag, {} });

        return (*this);
    }

    // close
    //   finish the current construct and attach it to its parent's children.
    // A close with only the root open is a no-op (there is no parent to attach
    // to); build handles the root.
    layout_cursor&
    close()
    {
        if (m_stack.size() <= 1)
        {
            return (*this);
        }

        frame _finished = static_cast<frame&&>(m_stack.back());
        m_stack.pop_back();

        m_stack.back().children.push_back(
            apply_node<OpId, Atom>(
                _finished.bag,
                _finished.op,
                static_cast<std::vector<node_type>&&>(_finished.children)));

        return (*this);
    }

    // leaf
    //   append a content leaf (an atom) to the current construct.
    layout_cursor&
    leaf(
        const Atom&   _atom,
        doc_attributes _bag = doc_attributes()
    )
    {
        m_stack.back().children.push_back(
            make_leaf_node<OpId, Atom>(_bag, _atom));

        return (*this);
    }

    // child
    //   append an already-built subtree (from another surface, say) to the
    // current construct -- the surfaces compose.
    layout_cursor&
    child(
        node_type _node
    )
    {
        m_stack.back().children.push_back(
            static_cast<node_type&&>(_node));

        return (*this);
    }

    // build
    //   close any still-open constructs and return the root document.  A
    // forgotten close is completed here rather than dropped.
    D_NODISCARD
    node_type
    build()
    {
        while (m_stack.size() > 1)
        {
            close();
        }

        frame _root = static_cast<frame&&>(m_stack.back());

        return apply_node<OpId, Atom>(
            _root.bag,
            _root.op,
            static_cast<std::vector<node_type>&&>(_root.children));
    }

    // depth
    //   the number of currently-open constructs (root included) -- handy for
    // asserting balance in a builder loop.
    D_NODISCARD
    std::size_t
    depth() const
    {
        return m_stack.size();
    }

private:
    // frame
    //   struct: one open construct -- its op, its params bag, and the children
    // accrued so far.
    struct frame
    {
        OpId                   op;
        doc_attributes         bag;
        std::vector<node_type> children;
    };

    std::vector<frame> m_stack;
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_LAYOUT_LAYOUT_BUILD_HPP
