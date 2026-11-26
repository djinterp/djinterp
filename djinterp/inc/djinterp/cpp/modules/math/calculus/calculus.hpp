/*******************************************************************************
* djinterp [cpp][math]                                              calculus.hpp
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
* LEVELS (step 31, 5c)
*   C++11 is the floor, as for the expression core: below it each calculus
* header is empty, and this one brings the constants alone. Each header's
* banner says which of its forms are constant expressions from C++11 and
* which from C++14.
*
* HOW TO USE
* ----------
*   Include it for the whole calculus subframework, or math.hpp for the module;
* or include one of its headers alone. Everything is in djinterp::math, the
* scalar helpers in djinterp::math::fn.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: it compiles from C++11 and is empty below it (N1). Each
* header it includes keeps its own level and knobs, as its banner says.
*
*
* path:      /inc/djinterp/cpp/modules/math/calculus/calculus.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.20
*                                                            revised: 2026.11.24
*******************************************************************************/

#ifndef DJINTERP_MATH_CALCULUS_CALCULUS_HPP
#define DJINTERP_MATH_CALCULUS_CALCULUS_HPP 1

// djinterp
#include "../constants.hpp"
#include "./elementary.hpp"
#include "./differentiation.hpp"
#include "./integration.hpp"
#include "./sequence.hpp"
#include "./series.hpp"

#endif  // DJINTERP_MATH_CALCULUS_CALCULUS_HPP
