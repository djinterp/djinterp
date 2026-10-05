/*******************************************************************************
* djinterp [math]                                                       math.hpp
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
*
* path:      /inc/djinterp/math/math.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_MATH_MATH_HPP
#define DJINTERP_MATH_MATH_HPP 1

// djinterp
// foundation
#include "./math_common.hpp"
// expression core + functions
#include "./expression.hpp"
#include "function/function.hpp"
// coordinate systems
#include "coordinate/coordinate.hpp"
#include "coordinate/cartesian.hpp"
#include "coordinate/polar.hpp"
#include "coordinate/cylindrical.hpp"
#include "coordinate/spherical.hpp"
// numeric support
#include "interval/interval.hpp"
#include "./constants.hpp"
#include "./values.hpp"
// subframeworks
#include "./geometry/geometry.hpp"
#include "./calculus/calculus.hpp"


#endif  // DJINTERP_MATH_MATH_HPP
