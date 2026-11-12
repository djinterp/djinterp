/*******************************************************************************
* djinterp [cpp/parse]                                                  fold.hpp
*
* Expression core v2's C++ face (design section 13): folds and walks.
*   fold() computes a value per node, bottom-up, in arena order, each node's
* from its children's: the algebra is any object with
*     Value operator()(DNodeView node, const Value* children,
*                      std::size_t count)
* and the values are held in a std::vector<Value>, so a Value is any copyable,
* default-constructible type, a std::string as well as a double. walk() goes
* depth first with an explicit stack, as d_parse_expr_walk does, calling a
* visitor's
*     int enter(DNodeView node, re_std::uint32_t parent)
*     int leave(DNodeView node, re_std::uint32_t parent)
* with fold.h's answers: D_PARSE_EXPR_WALK_CONTINUE, _SKIP (the children are
* passed over, leave() still called) and _STOP.
*   Both are C++ of their own rather than the C fold and walk under a thunk,
* so that an algebra or a visitor may throw, and what it throws unwinds no C
* frame; neither recurses. Their rules are the C ones, and the parity suite
* holds them to the C fold and walk node for node: a node is folded once
* however many parents share it, the nodes the root does not reach are not
* folded, and a term with holes is refused unless D_PARSE_EXPR_FOLD_HOLES is
* given.
*
*
* TABLE OF CONTENTS
* =================
* 1.  FOLDS
*     -----
*     1.  fold
* 2.  WALKS
*     -----
*     1.  DWalkFrame, walk
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/fold.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_FOLD_HPP
#define DJINTERP_PARSE_EXPR_FOLD_HPP 1

// djinterp
#include "./term.hpp"                               // DTerm, DNodeView
#include "../../../../c/modules/parse/expr/fold.h"  // the fold's answers
// std
#include <cstddef>  // std::size_t
#include <vector>   // std::vector


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 1.  FOLDS
//==============================================================================


// 1.1    fold
//------------------------------------------------------------------------------
// fold
//   function: folds the subtree a root reaches, the root's value into `_out`.
// Returns 0; D_PARSE_EXPR_FOLD_BAD_ARGUMENT where the root is no node, or
// D_PARSE_EXPR_FOLD_HAS_HOLES for a term with holes when `_flags` has no
// D_PARSE_EXPR_FOLD_HOLES; `_out` is then untouched.
template<typename Value,
         typename Algebra>
int
fold(
    const DTerm&     _term,
    re_std::uint32_t _root,
    Algebra&         _algebra,
    Value&           _out,
    re_std::uint32_t _flags = 0u
)
{
    const ::dj_parse_expr_term* const term = _term.get();

    if (_root >= term->node_count)
    {
        return D_PARSE_EXPR_FOLD_BAD_ARGUMENT;
    }

    if ( ((term->flags & D_PARSE_EXPR_TERM_HAS_HOLES) != 0u) &&
         ((_flags & D_PARSE_EXPR_FOLD_HOLES) == 0u) )
    {
        return D_PARSE_EXPR_FOLD_HAS_HOLES;
    }

    const std::size_t          nodes = (std::size_t)_root + 1u;
    std::vector<unsigned char> marks(nodes, 0u);
    std::vector<Value>         values(nodes);
    std::vector<Value>         children;

    // the nodes the root reaches: a child is always before its parent
    marks[_root] = 1u;

    for (std::size_t i = nodes; i > 0u; --i)
    {
        const ::dj_parse_expr_node& node = term->nodes[i - 1u];

        if ( (marks[i - 1u] != 0u) &&
             (node.kind == D_PARSE_EXPR_NODE_APPLY) )
        {
            for (re_std::uint32_t k = 0u; k < node.count; ++k)
            {
                marks[term->args[node.first + k]] = 1u;
            }
        }
    }

    // each reached node once, its children's values before it
    for (std::size_t i = 0u; i < nodes; ++i)
    {
        if (marks[i] == 0u)
        {
            continue;
        }

        const ::dj_parse_expr_node& node = term->nodes[i];
        const re_std::uint32_t      count =
            (node.kind == D_PARSE_EXPR_NODE_APPLY) ? node.count : 0u;

        children.clear();

        for (re_std::uint32_t k = 0u; k < count; ++k)
        {
            children.push_back(values[term->args[node.first + k]]);
        }

        values[i] = _algebra(DNodeView(term, (re_std::uint32_t)i),
                             (children.empty()) ? NULL : &children[0],
                             children.size());
    }

    _out = values[_root];

    return 0;
}


//==============================================================================
// 2.  WALKS
//==============================================================================


// 2.1    walk
//------------------------------------------------------------------------------
// DWalkFrame
//   struct: a node on a walk's stack, its parent, and the next child to go
// into; at namespace scope, as C++98 takes no local type as a template's
// argument.
struct DWalkFrame
{
    re_std::uint32_t m_node;
    re_std::uint32_t m_parent;
    re_std::uint32_t m_next;
};

// walk
//   function: walks the subtree a root reaches, depth first. Returns true
// when the walk ran to its end or a visit stopped it; false where the root
// is no node.
template<typename Visitor>
bool
walk(
    const DTerm&     _term,
    re_std::uint32_t _root,
    Visitor&         _visitor
)
{
    const ::dj_parse_expr_term* const term = _term.get();

    if (_root >= term->node_count)
    {
        return false;
    }

    std::vector<DWalkFrame> stack;
    re_std::uint32_t        node   = _root;
    re_std::uint32_t        parent = D_PARSE_EXPR_NO_NODE;

    for (;;)
    {
        // enter the node in hand, if there is one
        if (node != D_PARSE_EXPR_NO_NODE)
        {
            const int answer = _visitor.enter(DNodeView(term, node), parent);

            if (answer == D_PARSE_EXPR_WALK_STOP)
            {
                return true;
            }

            DWalkFrame entered;

            entered.m_node   = node;
            entered.m_parent = parent;
            entered.m_next   = (answer == D_PARSE_EXPR_WALK_SKIP) ? 0xFFFFFFFFu
                                                                  : 0u;
            stack.push_back(entered);
            node = D_PARSE_EXPR_NO_NODE;

            continue;
        }

        // the walk is done when its stack is
        if (stack.empty())
        {
            return true;
        }

        DWalkFrame&                 top  = stack.back();
        const ::dj_parse_expr_node& here = term->nodes[top.m_node];
        const re_std::uint32_t      kids =
            (here.kind == D_PARSE_EXPR_NODE_APPLY) ? here.count : 0u;

        if (top.m_next < kids)
        {
            parent = top.m_node;
            node   = term->args[here.first + top.m_next];
            ++top.m_next;

            continue;
        }

        const re_std::uint32_t left  = top.m_node;
        const re_std::uint32_t above = top.m_parent;

        stack.pop_back();

        if (_visitor.leave(DNodeView(term, left), above) ==
            D_PARSE_EXPR_WALK_STOP)
        {
            return true;
        }
    }
}


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_EXPR_FOLD_HPP
