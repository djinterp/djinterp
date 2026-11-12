/*******************************************************************************
* djinterp [cpp/parse]                                                  term.hpp
*
* Expression core v2's C++ face (design section 13): the term.
*   DTerm holds a dj_parse_expr_term and calls the C functions, so nothing is
* computed twice and the parity tests compare like with like. A node is read
* through DNodeView, a term and an index. A DTerm is copyable at every level,
* node for node at the same indices (d_parse_expr_term_assign), and movable
* from C++11; it compares by its roots' meaning, spans aside, and digests the
* same way.
*   The face is C++98 throughout, strict, and adds only move semantics and
* initializer lists above it. It throws nothing of its own: a builder answers
* D_PARSE_EXPR_NO_NODE, a copy that cannot be held leaves the term empty and
* says so in good().
*
*
* TABLE OF CONTENTS
* =================
* 1.  NAMESPACE
*     ---------
* 2.  NODES
*     -----
*     1.  DNodeView
* 3.  TERMS
*     -----
*     1.  DTerm
*     2.  Comparison
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/term.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_TERM_HPP
#define DJINTERP_PARSE_EXPR_TERM_HPP 1

// djinterp
#include "../../../../env/env.h"                       // D_ENV_LANG_*
#include "../../../djinterp.hpp"                       // framework root
#include "../substrate.hpp"                            // NS_PARSE
#include "../../../../c/modules/parse/expr/term.h"     // the C term
#include "../../../../c/modules/parse/expr/fold.h"     // the digest
// re_std
#include "../../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t,
                                                          // uint16_t, uint32_t
// std
#include <cstddef>  // std::size_t
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <initializer_list>  // std::initializer_list
#endif


//==============================================================================
// 1.  NAMESPACE
//==============================================================================

// D_KEYWORD_EXPR
//   keyword: resolves to `expr`. Guarded, and token-identical in every header
// of the face that defines it.
#ifndef D_KEYWORD_EXPR
    #define D_KEYWORD_EXPR              expr
#endif  // D_KEYWORD_EXPR

// NS_EXPR
//   namespace: expression core v2's C++ face, inside NS_PARSE.
#ifndef NS_EXPR
    #define NS_EXPR                     D_NAMESPACE(D_KEYWORD_EXPR)
#endif  // NS_EXPR


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 2.  NODES
//==============================================================================


// 2.1    DNodeView
//------------------------------------------------------------------------------
// DNodeView
//   class: one node of a term, read in place: what a fold's algebra and a
// walk's visitor are given. It holds the term by address, so it is valid
// while the term is unchanged.
class DNodeView
{
public:
    DNodeView()
        : m_term(NULL),
          m_index(D_PARSE_EXPR_NO_NODE)
    {}

    DNodeView(
        const ::dj_parse_expr_term* _term,
        re_std::uint32_t            _index
    )
        : m_term(_term),
          m_index(_index)
    {}

    // whether it names a node of its term
    bool
    valid() const
    {
        return (d_parse_expr_term_node(m_term, m_index) != NULL);
    }

    re_std::uint32_t
    index() const
    {
        return m_index;
    }

    // D_PARSE_EXPR_NODE_LEAF, _APPLY or _HOLE; 0 for no node
    re_std::uint8_t
    kind() const
    {
        return (valid()) ? record().kind : 0u;
    }

    bool
    is_leaf() const
    {
        return (kind() == D_PARSE_EXPR_NODE_LEAF);
    }

    bool
    is_apply() const
    {
        return (kind() == D_PARSE_EXPR_NODE_APPLY);
    }

    bool
    is_hole() const
    {
        return (kind() == D_PARSE_EXPR_NODE_HOLE);
    }

    // an application's operator id; a leaf's token class
    re_std::uint32_t
    op() const
    {
        return (valid()) ? record().op : 0u;
    }

    // a leaf's value; 0 for the others
    re_std::uint32_t
    value() const
    {
        return (is_leaf()) ? record().first : 0u;
    }

    // an application's number of children; 0 for the others
    re_std::uint32_t
    size() const
    {
        return (is_apply()) ? record().count : 0u;
    }

    // an application's child's index, D_PARSE_EXPR_NO_NODE past the last
    re_std::uint32_t
    child_index(
        re_std::uint32_t _child
    ) const
    {
        return d_parse_expr_term_child(m_term, m_index, _child);
    }

    DNodeView
    child(
        re_std::uint32_t _child
    ) const
    {
        return DNodeView(m_term, child_index(_child));
    }

    // D_PARSE_EXPR_NODE_GROUPED and the rest
    re_std::uint8_t
    flags() const
    {
        return (valid()) ? record().flags : 0u;
    }

    re_std::uint16_t
    depth() const
    {
        return (valid()) ? record().depth : 0u;
    }

    re_std::uint32_t
    offset() const
    {
        return (valid()) ? record().offset : 0u;
    }

    re_std::uint32_t
    length() const
    {
        return (valid()) ? record().length : 0u;
    }

    // the C record; valid() first
    const ::dj_parse_expr_node&
    record() const
    {
        return m_term->nodes[m_index];
    }

    const ::dj_parse_expr_term*
    term() const
    {
        return m_term;
    }

private:
    const ::dj_parse_expr_term* m_term;
    re_std::uint32_t            m_index;
};


//==============================================================================
// 3.  TERMS
//==============================================================================


// 3.1    DTerm
//------------------------------------------------------------------------------
// DTerm
//   class: an arena, the C term held by value. Made empty, it allocates as it
// grows where the heap is built in (D_CFG_PARSE_EXPR_HEAP), and is bound to
// nothing otherwise; made over the caller's storage, it never grows. A copy
// holds every node at its index, on the heap; where it cannot, it is empty
// and good() is false.
class DTerm
{
public:
    DTerm()
        : m_good(true)
    {
        init_empty();
    }

    DTerm(
        ::dj_parse_expr_node* _nodes,
        re_std::uint32_t      _node_count,
        re_std::uint32_t*     _args,
        re_std::uint32_t      _arg_count
    )
        : m_good(true)
    {
        d_parse_expr_term_init(&m_term, _nodes, _node_count, _args,
                               _arg_count);
    }

    DTerm(
        const DTerm& _other
    )
        : m_good(true)
    {
        init_empty();
        m_good = d_parse_expr_term_assign(&m_term, &_other.m_term);
    }

    DTerm&
    operator=(
        const DTerm& _other
    )
    {
        if (this != &_other)
        {
            m_good = d_parse_expr_term_assign(&m_term, &_other.m_term);
        }

        return *this;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    DTerm(
        DTerm&& _other
    ) D_NOEXCEPT
        : m_term(_other.m_term),
          m_good(_other.m_good)
    {
        d_parse_expr_term_init(&_other.m_term, NULL, 0u, NULL, 0u);
    }

    DTerm&
    operator=(
        DTerm&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            d_parse_expr_term_release(&m_term);
            m_term = _other.m_term;
            m_good = _other.m_good;
            d_parse_expr_term_init(&_other.m_term, NULL, 0u, NULL, 0u);
        }

        return *this;
    }
#endif  // C++11

    ~DTerm()
    {
        d_parse_expr_term_release(&m_term);
    }

    // building -------------------------------------------------------------

    re_std::uint32_t
    leaf(
        re_std::uint32_t _class,
        re_std::uint32_t _value,
        re_std::uint32_t _offset = 0u,
        re_std::uint32_t _length = 0u
    )
    {
        return d_parse_expr_term_leaf(&m_term, _class, _value, _offset,
                                      _length);
    }

    re_std::uint32_t
    apply(
        re_std::uint32_t        _op,
        const re_std::uint32_t* _children,
        re_std::uint32_t        _count,
        re_std::uint32_t        _offset = 0u,
        re_std::uint32_t        _length = 0u
    )
    {
        return d_parse_expr_term_apply(&m_term, _op, _children, _count,
                                       _offset, _length);
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    re_std::uint32_t
    apply(
        re_std::uint32_t                        _op,
        std::initializer_list<re_std::uint32_t> _children,
        re_std::uint32_t                        _offset = 0u,
        re_std::uint32_t                        _length = 0u
    )
    {
        return d_parse_expr_term_apply(&m_term, _op, _children.begin(),
                                       (re_std::uint32_t)_children.size(),
                                       _offset, _length);
    }
#endif  // C++11

    re_std::uint32_t
    hole(
        re_std::uint32_t _offset = 0u
    )
    {
        return d_parse_expr_term_hole(&m_term, _offset);
    }

    // the subtree a node of another term roots, appended; its root's index
    re_std::uint32_t
    copy(
        const DTerm&     _from,
        re_std::uint32_t _node
    )
    {
        return d_parse_expr_term_copy(&m_term, &_from.m_term, _node);
    }

    void
    set_root(
        re_std::uint32_t _root
    )
    {
        m_term.root = _root;
    }

    void
    clear()
    {
        d_parse_expr_term_clear(&m_term);
    }

    // observing ------------------------------------------------------------

    // false where a copy could not be held
    bool
    good() const
    {
        return m_good;
    }

    re_std::uint32_t
    size() const
    {
        return m_term.node_count;
    }

    bool
    empty() const
    {
        return (m_term.node_count == 0u);
    }

    re_std::uint32_t
    root() const
    {
        return m_term.root;
    }

    re_std::uint32_t
    depth() const
    {
        return m_term.depth;
    }

    bool
    has_holes() const
    {
        return ((m_term.flags & D_PARSE_EXPR_TERM_HAS_HOLES) != 0u);
    }

    DNodeView
    node(
        re_std::uint32_t _index
    ) const
    {
        return DNodeView(&m_term, _index);
    }

    DNodeView
    root_node() const
    {
        return DNodeView(&m_term, m_term.root);
    }

    bool
    verify() const
    {
        return d_parse_expr_term_verify(&m_term);
    }

    // whether a subtree of this term means what one of another does
    bool
    equal(
        re_std::uint32_t _node,
        const DTerm&     _other,
        re_std::uint32_t _other_node,
        bool             _spans = false
    ) const
    {
        return d_parse_expr_term_equal(&m_term, _node, &_other.m_term,
                                       _other_node, _spans);
    }

    // the root's digest (d_parse_expr_term_digest); 0 for an empty term
    re_std::uint32_t
    digest() const
    {
        return d_parse_expr_term_digest(&m_term, m_term.root);
    }

    re_std::uint32_t
    digest(
        re_std::uint32_t _root
    ) const
    {
        return d_parse_expr_term_digest(&m_term, _root);
    }

    // the C term, for the C functions
    ::dj_parse_expr_term*
    get()
    {
        return &m_term;
    }

    const ::dj_parse_expr_term*
    get() const
    {
        return &m_term;
    }

private:
    void
    init_empty()
    {
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
        d_parse_expr_term_init_heap(&m_term);
#else
        d_parse_expr_term_init(&m_term, NULL, 0u, NULL, 0u);
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
    }

    ::dj_parse_expr_term m_term;
    bool                 m_good;
};


// 3.2    Comparison
//------------------------------------------------------------------------------
// operator==, operator!=
//   function: whether two terms' roots mean the same, spans and GROUPED
// aside; two empty terms are equal.
inline bool
operator==(
    const DTerm& _a,
    const DTerm& _b
)
{
    if ( (_a.root() == D_PARSE_EXPR_NO_NODE) ||
         (_b.root() == D_PARSE_EXPR_NO_NODE) )
    {
        return (_a.root() == _b.root());
    }

    return _a.equal(_a.root(), _b, _b.root(), false);
}

inline bool
operator!=(
    const DTerm& _a,
    const DTerm& _b
)
{
    return !(_a == _b);
}


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_EXPR_TERM_HPP
