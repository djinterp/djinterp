/******************************************************************************
* djinterp [expression]                                     expression_render.hpp
*
*   Turning an expression back into text, correctly parenthesized.  Rendering
* is a fold: each sub-term becomes a string together with the precedence of
* its top operator, and a parent wraps a child in parentheses exactly when
* the child would otherwise re-associate -- when it binds looser than the
* parent, or equally loose on the wrong side.  The precedence, associativity,
* fixity, and spelling all come from the operator_signature (expression.hpp),
* so a language renders by describing its operators once; no per-operator
* printing code.
*
*   LAYOUT FROM FIXITY.  With a matching arity, an infix operator prints
* `left OP right`, a prefix `OP operand`, a postfix `operand OP`, and a
* nullary its bare symbol.  Anything else -- a matchfix operator, an
* arity that does not match the fixity, an operator of arity greater than
* two -- falls back to an unambiguous functional form `OP(a, b, ...)`, which
* needs no parenthesization and reads as a primary.  Spacing adapts to the
* spelling: a word-like spelling (`and`, `not`) is spaced from its operand,
* a symbolic one (`-`, `!`) is not.
*
*   WHAT THE CALLER SUPPLIES.  Atoms are language data, so their rendering is
* a function _Atom -> std::string the caller passes (a numeric format, a
* variable-name lookup).  The signature must map operator ids to
* operator_descriptor (rendering needs the precedence / fixity / spelling);
* an operator absent from the signature prints in the functional form with a
* `?` head rather than failing.
*
*   STATIC TERMS render by reifying first:
*     render(reify<OpId, Atom>(static_term), signature, atom_renderer)
* -- expression_static.hpp supplies reify; this header stays on the dynamic
* term and does not depend on the static face.
*
*
* TABLE OF CONTENTS
* =================
* I.    RENDER CARRIER & LAYOUT              (internal: precedence, wrapping)
* II.   RENDER                               (expression -> std::string)
*
*
* path:      /inc/djinterp/parse/expression/expression_render.hpp
* link(s):   ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_EXPRESSION_EXPRESSION_RENDER_
#define DJINTERP_EXPRESSION_EXPRESSION_RENDER_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "./expression.hpp"
#include "./expression_ops.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    RENDER CARRIER & LAYOUT                               ///
///////////////////////////////////////////////////////////////////////////////
//   The fold that renders carries, for each sub-term, its rendered text, the
// precedence of its top operator, and whether it is atomic -- a leaf, a
// constant, or a functional form that never needs wrapping.  A parent uses
// those to decide parentheses per the precedence / associativity rule.

NS_INTERNAL

    // render_carrier
    //   type: the result of rendering one sub-term.
    struct render_carrier
    {
        std::string text;
        int         precedence;
        bool        atomic;

        render_carrier()
            : text       ()
            , precedence (0)
            , atomic     (true)
        {}

        render_carrier(
            const std::string& _text,
            int                _precedence,
            bool               _atomic
        )
            : text       (_text)
            , precedence (_precedence)
            , atomic     (_atomic)
        {}
    };

    // is_word_char
    //   helper: whether a character is word-like ([A-Za-z0-9_]) -- used to
    // decide whether a spelling needs a space against its operand.
    inline bool
    is_word_char(char _c)
    {
        return ( (_c >= 'a' && _c <= 'z') ||
                 (_c >= 'A' && _c <= 'Z') ||
                 (_c >= '0' && _c <= '9') ||
                 (_c == '_') );
    }

    // ends_word / starts_word
    //   helpers: whether a spelling ends / starts with a word-like char.
    inline bool
    ends_word(const std::string& _s)
    {
        return (!_s.empty()) && is_word_char(_s[_s.size() - 1]);
    }

    inline bool
    starts_word(const std::string& _s)
    {
        return (!_s.empty()) && is_word_char(_s[0]);
    }

    // needs_parens
    //   helper: whether a child must be parenthesized under a parent of the
    // given precedence and associativity, at the given operand position.  A
    // child that binds tighter never needs parentheses; one that binds
    // looser always does; at equal precedence it depends on the side the
    // operator associates toward.
    inline bool
    needs_parens(
        const render_carrier& _child,
        int                   _parent_precedence,
        associativity         _assoc,
        std::size_t           _position,
        std::size_t           _arity
    )
    {
        if (_child.atomic)
        {
            return false;
        }

        if (_child.precedence > _parent_precedence)
        {
            return false;
        }

        if (_child.precedence < _parent_precedence)
        {
            return true;
        }

        // equal precedence -- keep the child on the operator's associating
        // side unwrapped, wrap it on the other
        if (_assoc == DAssocLeft)
        {
            return (_position != 0);
        }

        if (_assoc == DAssocRight)
        {
            return ((_position + 1) != _arity);
        }

        // non-associative -- wrap to disambiguate
        return true;
    }

    // wrap
    //   helper: a child's text, parenthesized iff the rule demands.
    inline std::string
    wrap(
        const render_carrier& _child,
        int                   _parent_precedence,
        associativity         _assoc,
        std::size_t           _position,
        std::size_t           _arity
    )
    {
        if (needs_parens(_child, _parent_precedence, _assoc, _position, _arity))
        {
            return "(" + _child.text + ")";
        }

        return _child.text;
    }

    // render_functional
    //   helper: the unambiguous functional form  head(a, b, ...)  -- the
    // fallback layout, and itself a primary (atomic), so it never needs
    // outer parentheses.
    inline render_carrier
    render_functional(
        const std::string&                 _head,
        const std::vector<render_carrier>& _children
    )
    {
        std::string _text = _head + "(";

        for (std::size_t _i = 0; _i < _children.size(); ++_i)
        {
            if (_i != 0)
            {
                _text += ", ";
            }

            _text += _children[_i].text;
        }

        _text += ")";

        return render_carrier(_text, 0, true);
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   RENDER                                                ///
///////////////////////////////////////////////////////////////////////////////

// render
//   function: renders an expression to a parenthesized string.  _signature
// (an operator_signature over operator_descriptor) supplies each operator's
// precedence, associativity, fixity, and spelling; _atom_renderer :
// Atom -> std::string renders the leaves.  The rendering is one fold: a leaf
// becomes its atom's text; an application lays its operator out per fixity,
// wrapping each child that the precedence rule requires.
template<typename _OpId,
         typename _Atom,
         typename _Signature,
         typename _AtomRenderer>
D_NODISCARD
std::string
render
(
    const expression<_OpId, _Atom>& _expression,
    const _Signature&               _signature,
    _AtomRenderer                   _atom_renderer
)
{
    internal::render_carrier _result =
        evaluate<internal::render_carrier>(
            _expression,
            // on_leaf -- the atom's own text, atomic
            [_atom_renderer](const _Atom& _atom) -> internal::render_carrier
            {
                return internal::render_carrier(
                    _atom_renderer(_atom), 0, true);
            },
            // on_apply -- lay the operator out around its rendered children
            [&_signature]
            (const _OpId&                                   _op,
             const std::vector<internal::render_carrier>&    _children)
                -> internal::render_carrier
            {
                const operator_descriptor* _descriptor =
                    _signature.describe(_op);

                const std::size_t _n = _children.size();

                // unknown operator -- unambiguous functional form
                if (_descriptor == nullptr)
                {
                    return internal::render_functional("?", _children);
                }

                const std::string _spelling =
                    (_descriptor->spelling != nullptr)
                        ? _descriptor->spelling
                        : "";
                const int           _precedence = _descriptor->precedence;
                const associativity _assoc      = _descriptor->assoc;

                // infix binary:  left OP right
                if ((_descriptor->fix == DFixInfix) && (_n == 2))
                {
                    std::string _l = internal::wrap(
                        _children[0], _precedence, _assoc, 0, _n);
                    std::string _r = internal::wrap(
                        _children[1], _precedence, _assoc, 1, _n);

                    return internal::render_carrier(
                        _l + " " + _spelling + " " + _r, _precedence, false);
                }

                // prefix unary:  OP operand
                if ((_descriptor->fix == DFixPrefix) && (_n == 1))
                {
                    std::string _a = internal::wrap(
                        _children[0], _precedence, _assoc, 0, _n);
                    std::string _gap = internal::ends_word(_spelling)
                        ? " " : "";

                    return internal::render_carrier(
                        _spelling + _gap + _a, _precedence, false);
                }

                // postfix unary:  operand OP
                if ((_descriptor->fix == DFixPostfix) && (_n == 1))
                {
                    std::string _a = internal::wrap(
                        _children[0], _precedence, _assoc, 0, _n);
                    std::string _gap = internal::starts_word(_spelling)
                        ? " " : "";

                    return internal::render_carrier(
                        _a + _gap + _spelling, _precedence, false);
                }

                // nullary:  a bare constant symbol (atomic)
                if ((_descriptor->fix == DFixNullary) && (_n == 0))
                {
                    return internal::render_carrier(
                        _spelling, _precedence, true);
                }

                // matchfix, arity mismatch, or higher arity -- functional
                // form
                return internal::render_functional(_spelling, _children);
            });

    return _result.text;
}


NS_END  // djinterp


#endif  // DJINTERP_EXPRESSION_EXPRESSION_RENDER_
