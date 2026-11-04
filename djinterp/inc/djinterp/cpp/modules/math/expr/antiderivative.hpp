/*******************************************************************************
* djinterp [cpp/math]                                         antiderivative.hpp
*
* Antiderivatives: a term integrated symbolically, from a table of
* elementary forms; roadmap Phase 4's last item, 4d.
*   THE TABLE holds the elementary forms, polynomials, exponentials,
* logarithms and the trigonometric functions among them, with the linearity
* of the integral and substitution of a linear argument; a term the table
* does not reach is the error no_closed_form. Symbolic integration in
* general is large; this is its first, bounded part (the roadmap marks it
* later work).
*   STEPS. Step 28 (4d) fills expr_antiderivative. Until then it answers
* expr_status::not_yet.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/antiderivative.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ANTIDERIVATIVES
    ---------------
    1.  The entry
         1.  expr_antiderivative
*/

#ifndef DJINTERP_MATH_EXPR_ANTIDERIVATIVE_HPP
#define DJINTERP_MATH_EXPR_ANTIDERIVATIVE_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "../math_common.hpp"  // NS_MATH
#include "./expr_common.hpp"   // expr_result
#include "./term.hpp"          // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  ANTIDERIVATIVES
//==============================================================================


// 1.1    The entry
//------------------------------------------------------------------------------
// 1.1.1
// expr_antiderivative
/**
 * @brief An antiderivative of a term in a variable, from the table of
 *        elementary forms.
 *
 * @note Until step 28 (4d) a stub: it answers expr_status::not_yet.
 *
 * @param[in] _term      the integrand.
 * @param[in] _variable  the variable of integration.
 * @return a term whose derivative by _variable is _term, with no constant
 *         of integration, simplified; or the error no_closed_form.
 */
D_NODISCARD inline expr_result<expr_term>
expr_antiderivative(
    const expr_term&   _term,
    const std::string& _variable
)
{
    static_cast<void>(_term);
    static_cast<void>(_variable);

    return internal::expr_not_yet<expr_term>(
        "expr_antiderivative: not yet (step 28, 4d)");
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_ANTIDERIVATIVE_HPP
