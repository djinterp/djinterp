/******************************************************************************
* djinterp [math]                                                     math.hpp
*
* Umbrella header for the math subframework.
*   math.hpp defines no types of its own; it includes the focused headers that
* make up the module so a single `#include "math.hpp"` pulls in everything.
* New code may include the specific sub-headers directly to keep compile times
* tight.
*
* DIRECTORY MAP:
*   math_common.hpp - subsystem foundation: NS_MATH and the env include
*   expression.hpp  - value-holding expression core: constant_node,
*                     variable_node, binary/unary nodes, operators, combinators
*   function.hpp    - math_function + fluent builder, piecewise / vector /
*                     parametric / implicit forms, relational + logical layers,
*                     coordinate-system detection, axis sets
*   coordinate.hpp  - coordinate-system trait layer; cartesian/polar/
*                     cylindrical/spherical.hpp - the concrete systems
*   interval.hpp    - unified interval type + folded interval traits
*   constants.hpp   - constants (djinterp::math::constants), compile-time
*                     rational, number_base / radix system
*   values.hpp      - compile-time sampling of an expression over points
*   geometry/       - geometry subframework (edges, surfaces, solids, measures)
*   calculus/       - calculus subframework (differentiation, integration,
*                     sequences, series, elementary functions)
*
* path:      /inc/djinterp/math/math.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2024.04.24
******************************************************************************/

#ifndef DJINTERP_MATH_
#define DJINTERP_MATH_ 1

// foundation
#include "./math_common.hpp"

// expression core + functions
#include "./expression.hpp"
#include "./function.hpp"

// coordinate systems
#include "./coordinate.hpp"
#include "./cartesian.hpp"
#include "./polar.hpp"
#include "./cylindrical.hpp"
#include "./spherical.hpp"

// numeric support
#include "./interval.hpp"
#include "./constants.hpp"
#include "./values.hpp"

// subframeworks
#include "./geometry/geometry.hpp"
#include "./calculus/calculus.hpp"


#endif  // DJINTERP_MATH_
