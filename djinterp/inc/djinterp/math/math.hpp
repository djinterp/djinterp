/******************************************************************************
* djinterp [math]                                                     math.hpp
*
* Umbrella header for the math expression subframework.
*   Following consolidation, math.hpp no longer defines types of its own. Its
* former contents were split into focused headers; this header includes them
* all so existing `#include "math.hpp"` sites keep compiling. New code may
* include the specific sub-headers directly to keep compile times tight.
*
* DIRECTORY MAP:
*   expression.hpp - expression core: constant, variable, term, polynomial,
*                    binary/unary ops, power, function_expr, rational_function,
*                    compose (the canonical expression tree)
*   constants.hpp  - named constants (pi, e, phi, sqrt2), typed value
*                    templates (pi_v, ...), compile-time rational, number_base
*   builtins.hpp   - linear, step_function, sign_function, abs_function
*   values.hpp     - compile-time function-value sampling over an interval
*   interval.hpp   - interval types (pulled in transitively for convenience)
*
* 
* path:      /inc/djinterp/math/math.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2024.04.24
******************************************************************************/

#ifndef DJINTERP_MATH_
#define DJINTERP_MATH_ 1

// djinterp
#include "../core/djinterp.hpp"
#include "./expression.hpp"
#include "./constants.hpp"
#include "./builtins.hpp"
#include "./values.hpp"
#include "./interval.hpp"


#endif  // DJINTERP_MATH_