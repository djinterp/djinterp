/******************************************************************************
* djinterp [math]                                            calculus/calculus.hpp
*
* Calculus subframework umbrella.
*   Pulls in every calculus header in dependency order. Include this to get
* the whole subframework:
*
*     constants.hpp        - mathematical constants (djinterp::math::constants)
*     elementary.hpp       - elementary functions, identities, scalar helpers
*     differentiation.hpp  - symbolic + numerical differentiation
*     integration.hpp      - numerical definite integration
*     sequence.hpp         - sequences
*     series.hpp           - series, summation, Taylor evaluation
*
*   Everything lives flat in djinterp::math (scalar helpers in the nested
* djinterp::math::fn namespace, constants in djinterp::math::constants), and
* builds on the value-holding expression core (expression.hpp / function.hpp).
*
* path:      /inc/djinterp/math/calculus/calculus.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.06.20
******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_
#define DJINTERP_MATH_CALCULUS_ 1

#include "./constants.hpp"
#include "./elementary.hpp"
#include "./differentiation.hpp"
#include "./integration.hpp"
#include "./sequence.hpp"
#include "./series.hpp"

#endif  // DJINTERP_MATH_CALCULUS_
