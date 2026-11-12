/*******************************************************************************
* djinterp [cpp/parse]                                                bridge.hpp
*
* Expression core v2's bridge to v1's term (design section 13), from C++11,
* v1's floor; below it this file is empty.
*   to_expression() builds v1's expression<OpId, Atom> from a v2 subtree,
* iteratively, by the face's fold: each application through the caller's
* `op_of`, each leaf through its `atom_of`, both given the node's
* DNodeView. So math's folds, its simplifier and lane D's layout keep their
* v1 term while math's parser moves to v2 (design 17.1, step 3). v1's term
* is an alias no argument deduces through, so OpId and Atom are given:
* to_expression<OpId, Atom>(...). A node a chain shares is built once, and
* v1's nodes, held by shared pointer, share it too.
*   from_operator_signature() defines a v1 table's operators in a v2
* signature, for a language moving its table: prefix, infix and postfix
* rows with their precedence (both bind the higher tighter), associativity,
* spelling and algebraic flags, and D_PARSE_EXPR_OP_WORD where the spelling
* is a word, as the scanner reads one (`mod`, `and`). The token classes
* that begin them are v2's alone, so the language adds its roles and seals
* the signature itself. A nullary or matchfix row has no v2 form a single
* spelling can give (a constant is an atom, a matchfix a mixfix with parts),
* and is left to the caller, counted.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TO V1
*     -----
*     1.  DBridgeAlgebra
*     2.  to_expression
* 2.  FROM V1
*     -------
*     1.  from_operator_signature
*
*
* path:      /inc/djinterp/cpp/modules/parse/expr/bridge.hpp
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_PARSE_EXPR_BRIDGE_HPP
#define DJINTERP_PARSE_EXPR_BRIDGE_HPP 1

// FLOOR: v1's term is C++11's; below it this file is empty (README rule 5).
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./term.hpp"                       // DTerm, DNodeView
#include "./signature.hpp"                  // DSignature
#include "./fold.hpp"                       // fold
#include "../expression/expression.hpp"     // expression, operator_signature
// std
#include <cstddef>  // std::size_t
#include <vector>   // std::vector


NS_DJINTERP
NS_PARSE
NS_EXPR


//==============================================================================
// 1.  TO V1
//==============================================================================


// 1.1    DBridgeAlgebra
//------------------------------------------------------------------------------
// DBridgeAlgebra
//   class: the fold's algebra that builds v1's term: an application from its
// children, any other node a leaf.
template<typename OpId,
         typename Atom,
         typename OpOf,
         typename AtomOf>
class DBridgeAlgebra
{
public:
    using expression_type = ::djinterp::expression<OpId, Atom>;

    DBridgeAlgebra(
        OpOf&   _op_of,
        AtomOf& _atom_of
    )
        : m_op_of(&_op_of),
          m_atom_of(&_atom_of)
    {}

    expression_type
    operator()(
        DNodeView              _node,
        const expression_type* _children,
        std::size_t            _count
    )
    {
        if (_node.is_apply())
        {
            return ::djinterp::expr_apply<OpId, Atom>(
                (*m_op_of)(_node),
                std::vector<expression_type>(_children, _children + _count));
        }

        return ::djinterp::expr_leaf<OpId, Atom>((*m_atom_of)(_node));
    }

private:
    OpOf*   m_op_of;
    AtomOf* m_atom_of;
};


// 1.2    to_expression
//------------------------------------------------------------------------------
// to_expression
//   function: v1's term for the subtree a root reaches, into `_out`; OpId and
// Atom are given, as v1's alias deduces neither. `_op_of`
// is called with each application, `_atom_of` with each leaf, and, where
// `_flags` has D_PARSE_EXPR_FOLD_HOLES, with each hole. Returns the fold's
// answer: 0, D_PARSE_EXPR_FOLD_BAD_ARGUMENT, or D_PARSE_EXPR_FOLD_HAS_HOLES
// for a term with holes not asked for; `_out` is set only on 0.
template<typename OpId,
         typename Atom,
         typename OpOf,
         typename AtomOf>
int
to_expression(
    const DTerm&                      _term,
    re_std::uint32_t                  _root,
    OpOf                              _op_of,
    AtomOf                            _atom_of,
    ::djinterp::expression<OpId, Atom>& _out,
    re_std::uint32_t                  _flags = 0u
)
{
    DBridgeAlgebra<OpId, Atom, OpOf, AtomOf> algebra(_op_of, _atom_of);

    return fold(_term, _root, algebra, _out, _flags);
}


//==============================================================================
// 2.  FROM V1
//==============================================================================


// 2.1    from_operator_signature
//------------------------------------------------------------------------------
// from_operator_signature
//   function: defines each of a v1 table's prefix, infix and postfix rows in
// a v2 signature, its id `_id_of(row's id)`, a word's spelling flagged
// D_PARSE_EXPR_OP_WORD. Returns how many rows it left to the caller: the
// nullary and matchfix ones. A row v2 refuses leaves the signature's good()
// false.
template<typename OpId,
         typename Container,
         typename IdOf>
std::size_t
from_operator_signature(
    const ::djinterp::operator_signature<OpId,
                                         ::djinterp::operator_descriptor,
                                         Container>& _table,
    IdOf                                             _id_of,
    DSignature&                                      _out
)
{
    std::size_t left = 0u;

    for (const auto& row : _table.entries())
    {
        const ::djinterp::operator_descriptor& d  = row.m_value;
        const re_std::uint32_t                 id = _id_of(row.m_key);
        const unsigned char                    first =
            (d.spelling) ? (unsigned char)d.spelling[0] : 0u;
        const bool                             word  =
            ( ( (first >= 'A') && (first <= 'Z') ) ||
              ( (first >= 'a') && (first <= 'z') ) ||
              (first == '_')                       ||
              (first >= 0x80u) );
        const re_std::uint16_t                 flags =
            (re_std::uint16_t)( ((d.commutative) ? D_PARSE_EXPR_OP_COMMUTATIVE
                                                 : 0u) |
                                ((d.associative) ? D_PARSE_EXPR_OP_ASSOCIATIVE
                                                 : 0u) |
                                ((d.idempotent) ? D_PARSE_EXPR_OP_IDEMPOTENT
                                                : 0u) |
                                ((word) ? D_PARSE_EXPR_OP_WORD : 0u) );
        const re_std::uint8_t                  assoc =
            (d.assoc == ::djinterp::DAssocLeft)  ? D_PARSE_EXPR_ASSOC_LEFT :
            (d.assoc == ::djinterp::DAssocRight) ? D_PARSE_EXPR_ASSOC_RIGHT
                                                 : D_PARSE_EXPR_ASSOC_NONE;

        if (d.fix == ::djinterp::DFixPrefix)
        {
            _out.prefix(id, (re_std::int32_t)d.precedence, d.spelling, flags);
        }
        else if (d.fix == ::djinterp::DFixInfix)
        {
            _out.infix(id, (re_std::int32_t)d.precedence, assoc, d.spelling,
                       flags);
        }
        else if (d.fix == ::djinterp::DFixPostfix)
        {
            _out.postfix(id, (re_std::int32_t)d.precedence, d.spelling,
                         flags);
        }
        else
        {
            ++left;
        }
    }

    return left;
}


NS_END  // expr
NS_END  // parse
NS_END  // djinterp


#endif  // floor


#endif  // DJINTERP_PARSE_EXPR_BRIDGE_HPP
