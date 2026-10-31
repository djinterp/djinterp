/*******************************************************************************
* djinterp [cpp/math]                                                numeric.hpp
*
* Numeric calculus: a term's definite integral, and its derivative at a
* point, computed in a floating-point type; roadmap Phase 4's second item,
* 4b.
*   THE INTEGRAL is a quadrature rule of calculus/integration.hpp's over the
* typed fold (evaluate.hpp): the term evaluated in T at the rule's points,
* its other variables bound. THE DERIVATIVE AT A POINT is a difference
* quotient over the same fold, its step chosen for T. Where the symbolic
* derivative (derivative.hpp) exists, evaluating it is the exact way; this is
* the way for any term.
*   STEPS. Step 23 (4b) fills both entries. Until then each answers
* expr_status::not_yet.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/numeric.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NUMERIC CALCULUS
    ----------------
    1.  The rules
         1.  expr_quadrature
    2.  The entries
         1.  expr_integrate<Type>
         2.  expr_derivative_at<Type>
*/

#ifndef DJINTERP_MATH_EXPR_NUMERIC_HPP
#define DJINTERP_MATH_EXPR_NUMERIC_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "../math_common.hpp"  // NS_MATH
#include "./evaluate.hpp"      // expr_bindings
#include "./expr_common.hpp"   // expr_result, expr_floating
#include "./term.hpp"          // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  NUMERIC CALCULUS
//==============================================================================


// 1.1    The rules
//------------------------------------------------------------------------------
// 1.1.1
// expr_quadrature
//   enum: the rule a definite integral takes, calculus/integration.hpp's.
enum class expr_quadrature
{
    trapezoid       = 1,   // the composite trapezoidal rule
    simpson         = 2,   // the composite Simpson's rule
    gauss_legendre5 = 3,   // Gauss-Legendre at five points
    romberg         = 4    // Romberg's extrapolation
};

// 1.2    The entries
//------------------------------------------------------------------------------
// 1.2.1
// expr_integrate<Type>
/**
 * @brief The definite integral of a term in a variable, in float, double or
 *        long double.
 *
 * @note Until step 23 (4b) a stub: it answers expr_status::not_yet.
 *
 * @param[in] _term      the integrand.
 * @param[in] _variable  the variable of integration.
 * @param[in] _from      the lower bound.
 * @param[in] _to        the upper bound.
 * @param[in] _bindings  the values of the integrand's other variables.
 * @param[in] _rule      the quadrature rule.
 * @return the integral from _from to _to; or an error the typed fold gives,
 *         or no_convergence, where an adaptive rule meets its limit.
 */
template<typename Type>
D_NODISCARD expr_result<Type> expr_integrate(
    const expr_term&     _term,
    const std::string&   _variable,
    Type                 _from,
    Type                 _to,
    const expr_bindings& _bindings = expr_bindings(),
    expr_quadrature      _rule = expr_quadrature::romberg
)
{
    D_STATIC_ASSERT(internal::expr_floating<Type>::value,
                    "expr_integrate<Type>: Type is float, double or long "
                    "double.");

    static_cast<void>(_term);
    static_cast<void>(_variable);
    static_cast<void>(_from);
    static_cast<void>(_to);
    static_cast<void>(_bindings);
    static_cast<void>(_rule);

    return internal::expr_not_yet<Type>(
        "expr_integrate<Type>: not yet (step 23, 4b)");
}

// 1.2.2
// expr_derivative_at<Type>
/**
 * @brief The derivative of a term by a variable at a point, numerically,
 *        in float, double or long double.
 *
 * @note Until step 23 (4b) a stub: it answers expr_status::not_yet.
 *
 * @param[in] _term      the term.
 * @param[in] _variable  the variable.
 * @param[in] _point     the point.
 * @param[in] _bindings  the values of the term's other variables.
 * @return the derivative at _point; or an error the typed fold gives.
 */
template<typename Type>
D_NODISCARD expr_result<Type> expr_derivative_at(
    const expr_term&     _term,
    const std::string&   _variable,
    Type                 _point,
    const expr_bindings& _bindings = expr_bindings()
)
{
    D_STATIC_ASSERT(internal::expr_floating<Type>::value,
                    "expr_derivative_at<Type>: Type is float, double or long "
                    "double.");

    static_cast<void>(_term);
    static_cast<void>(_variable);
    static_cast<void>(_point);
    static_cast<void>(_bindings);

    return internal::expr_not_yet<Type>(
        "expr_derivative_at<Type>: not yet (step 23, 4b)");
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_NUMERIC_HPP
