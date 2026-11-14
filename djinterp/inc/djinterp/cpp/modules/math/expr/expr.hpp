/*******************************************************************************
* djinterp [cpp/math]                                                   expr.hpp
*
* The handle of the run-time expression language, math::expr, and the
* header that includes every other of math/expr/; the sixth layer.
*   THE HANDLE holds a term, or the error that kept it from being one, and
* each member makes a new handle, so that steps chain, as roadmap Phase 2's
* examples do:
*     math::expr  f = math::parse("x^2 * sin(x)");
*     std::string s = f.diff("x").simplify().str();
*     auto        d = f.eval<double>(math::at("x", 2.0));   // a result
*     auto        q = math::parse("1/3 + 1/6").eval();      // exactly 1/2
* An error passes through every later step unchanged, as an error value
* passes through arithmetic, and comes out where the chain ends: str() is
* its message, eval its error. try_parse gives the parse as an expr_result,
* for a caller who branches on it (principle 5: steps that can fail chain
* through result<T, E>).
*   ENTRIES. Each member is a term-level entry's: str() expr_render's, eval
* expr_evaluate's, subs expr_substitute's, simplify expr_simplify's and diff
* expr_derivative's; parse and try_parse are expr_parse's. Each answers as
* its entry does, so that a step that fills an entry fills its member too.
*   STEPS. Step 17 (2e) filled the handle and the entries, and step 18
* (2f) the entries subs and simplify are: subs("x", t) puts t in place of x,
* and simplify() folds constants, applies identities and collects like
* terms (simplify.hpp). Until step 25 (4a), diff gives a handle of
* expr_derivative's error not_yet. Step 24 (3f), its second part, named
* complex.hpp and quaternion.hpp among the headers this one includes, which
* it had read only through the others.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/expr.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE HANDLE
    ----------
    1.  The class
         1.  expr
    2.  The entries
         1.  parse
         2.  try_parse
*/

#ifndef DJINTERP_MATH_EXPR_EXPR_HPP
#define DJINTERP_MATH_EXPR_EXPR_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>  // std::string
// djinterp: math/expr/, every header
#include "../math_common.hpp"     // NS_MATH
#include "../value.hpp"           // value
#include "./antiderivative.hpp"   // expr_antiderivative
#include "./complex.hpp"          // complex numbers
#include "./derivative.hpp"       // expr_derivative, expr_gradient
#include "./evaluate.hpp"         // expr_evaluate, expr_bindings, at
#include "./expr_common.hpp"      // expr_error, expr_result, expr_syntax
#include "./function.hpp"         // the function and constant tables
#include "./matrix.hpp"           // matrix values
#include "./numeric.hpp"          // expr_integrate, expr_derivative_at
#include "./operator.hpp"         // expr_op, expr_signature
#include "./parse.hpp"            // expr_parse
#include "./quaternion.hpp"       // quaternions
#include "./render.hpp"           // expr_render
#include "./rotation.hpp"         // rotations, slerp
#include "./series.hpp"           // expr_sum, expr_limit, expr_taylor
#include "./simplify.hpp"         // expr_simplify
#include "./substitute.hpp"       // expr_substitute
#include "./term.hpp"             // expr_term
#include "./token.hpp"            // expr_tokenize


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE HANDLE
//==============================================================================


// 1.1    The class
//------------------------------------------------------------------------------
// 1.1.1
// expr
//   class: an expression of the language: a term, or the error that kept it
// from being one. A copy shares its term's nodes; every member that computes
// makes a handle, or an answer, of its own.
class expr
{
public:
    // a handle holding _term.
    explicit expr(
        const expr_term& _term
    )
        : m_term(_term),
          m_error(),
          m_ok(true)
    {}

    // a handle holding _error.
    explicit expr(
        const expr_error& _error
    )
        : m_term(),
          m_error(_error),
          m_ok(false)
    {}

    // ---- what it holds -------------------------------------------------

    // ok
    //   whether it holds a term.
    D_NODISCARD bool
    ok() const D_NOEXCEPT
    {
        return m_ok;
    }

    // term
    //   the term; asked only where ok().
    D_NODISCARD const expr_term&
    term() const D_NOEXCEPT
    {
        return m_term;
    }

    // error
    //   the error; asked only where !ok().
    D_NODISCARD const expr_error&
    error() const D_NOEXCEPT
    {
        return m_error;
    }

    // ---- what it computes ----------------------------------------------
    // Each is its entry's (the banner's ENTRIES), and passes on the error a
    // handle holds: str() as its message, eval as its error, and every
    // member that makes a handle as a handle holding it.

    /**
     * @brief The expression as text.
     *
     * @return the term rendered (expr_render), which parse reads back as
     *         the term where parse made it; or the error's message.
     */
    D_NODISCARD std::string
    str() const
    {
        // an error: its message
        if (!m_ok)
        {
            return m_error.message;
        }

        return expr_render(m_term);
    }

    /**
     * @brief The expression's value in float, double or long double, or in
     *        the math::complex of one.
     *
     * @param[in] _bindings  the values of its variables; none by default.
     * @param[in] _context   what the fold computes under; by default
     *                       expr_context().
     * @return expr_evaluate<Type>'s answer, or the error the handle holds.
     */
    template<typename Type>
    D_NODISCARD expr_result<Type>
    eval(
        const expr_bindings& _bindings = expr_bindings(),
        const expr_context&  _context  = expr_context()
    ) const
    {
        // an error: passed on
        if (!m_ok)
        {
            return err<Type, expr_error>(m_error);
        }

        return expr_evaluate<Type>(m_term,
                                   _bindings,
                                   _context);
    }

    /**
     * @brief The expression's value as a math::value.
     *
     * @param[in] _bindings  the values of its variables; none by default.
     * @param[in] _context   what the fold computes under; by default
     *                       expr_context().
     * @return expr_evaluate's answer, exact where the value layer is; or
     *         the error the handle holds.
     */
    D_NODISCARD expr_result<value>
    eval(
        const expr_bindings& _bindings = expr_bindings(),
        const expr_context&  _context  = expr_context()
    ) const
    {
        // an error: passed on
        if (!m_ok)
        {
            return err<value, expr_error>(m_error);
        }

        return expr_evaluate(m_term,
                             _bindings,
                             _context);
    }

    /**
     * @brief The expression with a variable replaced.
     *
     * @param[in] _variable     the variable's name.
     * @param[in] _replacement  what replaces it.
     * @return a handle of expr_substitute's term; or of the error this
     *         handle or _replacement holds, this one's first.
     */
    D_NODISCARD expr
    subs(
        const std::string& _variable,
        const expr&        _replacement
    ) const
    {
        // an error: this handle's, then the replacement's
        if (!m_ok)
        {
            return *this;
        }

        if (!_replacement.m_ok)
        {
            return _replacement;
        }

        return expr(expr_substitute(m_term,
                                    _variable,
                                    _replacement.m_term));
    }

    /**
     * @brief The expression simplified.
     *
     * @return a handle of expr_simplify's term, or of the error held.
     */
    D_NODISCARD expr
    simplify() const
    {
        // an error: passed on
        if (!m_ok)
        {
            return *this;
        }

        return expr(expr_simplify(m_term));
    }

    /**
     * @brief The expression's derivative by a variable.
     *
     * @note Until step 25 (4a) expr_derivative answers expr_status::not_yet,
     *       and this gives a handle holding that error.
     *
     * @param[in] _variable  the variable's name.
     * @return a handle of expr_derivative's term, or of its error, or of
     *         the error held.
     */
    D_NODISCARD expr
    diff(
        const std::string& _variable
    ) const
    {
        // an error: passed on
        if (!m_ok)
        {
            return *this;
        }

        const expr_result<expr_term> derivative =
            expr_derivative(m_term,
                            _variable);

        return ( (derivative.is_ok()) ? expr(derivative.value())
                                      : expr(derivative.error()) );
    }

    /**
     * @brief The expression's derivative by a variable, _order times.
     *
     * @note Until step 25 (4a) expr_derivative answers expr_status::not_yet,
     *       and this gives a handle holding that error.
     *
     * @param[in] _variable  the variable's name.
     * @param[in] _order     how many times; 0 gives the term itself.
     * @return a handle of expr_derivative's term, or of its error, or of
     *         the error held.
     */
    D_NODISCARD expr
    diff(
        const std::string& _variable,
        unsigned           _order
    ) const
    {
        // an error: passed on
        if (!m_ok)
        {
            return *this;
        }

        const expr_result<expr_term> derivative =
            expr_derivative(m_term,
                            _variable,
                            _order);

        return ( (derivative.is_ok()) ? expr(derivative.value())
                                      : expr(derivative.error()) );
    }

private:
    expr_term  m_term;
    expr_error m_error;
    bool       m_ok;
};

// 1.2    The entries
//------------------------------------------------------------------------------
// 1.2.1
// parse
/**
 * @brief Math text as an expression.
 *
 * @param[in] _text    the text.
 * @param[in] _syntax  gate G5's options.
 * @return a handle of the term expr_parse reads, or of its error, which
 *         has its place in _text.
 */
D_NODISCARD inline expr
parse(
    const std::string& _text,
    const expr_syntax& _syntax = expr_syntax()
)
{
    const expr_result<expr_term> term = expr_parse(_text,
                                                   _syntax);

    return ( (term.is_ok()) ? expr(term.value())
                            : expr(term.error()) );
}

// 1.2.2
// try_parse
/**
 * @brief Math text as an expression, or the error that kept it from being
 *        one, for a caller who branches on it.
 *
 * @param[in] _text    the text.
 * @param[in] _syntax  gate G5's options.
 * @return parse's handle, which holds a term; or the error it would hold,
 *         expr_parse's.
 */
D_NODISCARD inline expr_result<expr>
try_parse(
    const std::string& _text,
    const expr_syntax& _syntax = expr_syntax()
)
{
    const expr handle = parse(_text,
                              _syntax);

    // the parse's error
    if (!handle.ok())
    {
        return err<expr, expr_error>(handle.error());
    }

    return ok<expr, expr_error>(handle);
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_EXPR_HPP
