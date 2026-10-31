/*******************************************************************************
* djinterp [cpp/math]                                             derivative.hpp
*
* The symbolic derivative: a term differentiated by a variable, and the
* gradient; roadmap Phase 4's first item, 4a.
*   THE DERIVATIVE is one fold over the term, by the rules of each operator
* (the sum, product, quotient and power rules) and of each function, its
* partial derivatives from the function table (function.hpp) by the chain
* rule; then the result simplified (simplify.hpp). A function with no rule is
* the error not_differentiable. The n-th derivative is the first, n times;
* the gradient, the derivative by each variable named.
*   STEPS. Step 22 (4a) fills every entry. Until then each answers
* expr_status::not_yet.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/derivative.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DERIVATIVES
    -----------
    1.  The entries
         1.  expr_derivative
         2.  expr_gradient
*/

#ifndef DJINTERP_MATH_EXPR_DERIVATIVE_HPP
#define DJINTERP_MATH_EXPR_DERIVATIVE_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "../math_common.hpp"  // NS_MATH
#include "./expr_common.hpp"   // expr_result
#include "./term.hpp"          // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  DERIVATIVES
//==============================================================================


// 1.1    The entries
//------------------------------------------------------------------------------
// 1.1.1
// expr_derivative
/**
 * @brief The derivative of a term by a variable.
 *
 * @note Until step 22 (4a) a stub: it answers expr_status::not_yet.
 *
 * @param[in] _term      the term.
 * @param[in] _variable  the variable's name.
 * @return the derivative, simplified; or the error not_differentiable,
 *         where a function of _term has no rule for it, or unknown_name.
 */
D_NODISCARD inline expr_result<expr_term> expr_derivative(
    const expr_term&   _term,
    const std::string& _variable
)
{
    static_cast<void>(_term);
    static_cast<void>(_variable);

    return internal::expr_not_yet<expr_term>(
        "expr_derivative: not yet (step 22, 4a)");
}

/**
 * @brief The derivative of a term by a variable, _order times.
 *
 * @note Until step 22 (4a) a stub: it answers expr_status::not_yet.
 *
 * @param[in] _term      the term.
 * @param[in] _variable  the variable's name.
 * @param[in] _order     how many times; 0 gives the term itself.
 * @return the derivative of that order, simplified; or the error
 *         not_differentiable or unknown_name.
 */
D_NODISCARD inline expr_result<expr_term> expr_derivative(
    const expr_term&   _term,
    const std::string& _variable,
    unsigned           _order
)
{
    static_cast<void>(_term);
    static_cast<void>(_variable);
    static_cast<void>(_order);

    return internal::expr_not_yet<expr_term>(
        "expr_derivative: not yet (step 22, 4a)");
}

// 1.1.2
// expr_gradient
/**
 * @brief The gradient of a term: its derivative by each variable named.
 *
 * @note Until step 22 (4a) a stub: it answers expr_status::not_yet.
 *
 * @param[in] _term       the term.
 * @param[in] _variables  the variables' names, in the gradient's order.
 * @return the derivatives, one for each name, in its order; or the first
 *         error a derivative gives.
 */
D_NODISCARD inline expr_result<std::vector<expr_term> > expr_gradient(
    const expr_term&                _term,
    const std::vector<std::string>& _variables
)
{
    static_cast<void>(_term);
    static_cast<void>(_variables);

    return internal::expr_not_yet<std::vector<expr_term> >(
        "expr_gradient: not yet (step 22, 4a)");
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_DERIVATIVE_HPP
