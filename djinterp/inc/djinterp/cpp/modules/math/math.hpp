/*******************************************************************************
* djinterp [cpp/math]                                                   math.hpp
*
* Umbrella header for the math subframework.
*   math.hpp defines no types of its own; it includes every header of the
* module, so that one `#include "math.hpp"` pulls in everything, in
* djinterp::math with linear algebra in djinterp::math::linalg (S399). New
* code may include the specific sub-headers directly to keep compile times
* tight: the run-time language alone is expr/expr.hpp. It compiles from
* C++14, the compile-time expression core's floor; expr/ is empty below
* C++11.
*
* WHERE TO START:
*   a formula as text, parsed, evaluated, compiled, differentiated: the handle
* math::expr, expr/expr.hpp (math::parse("x ^ 2 - y")), whose headers
* expr/expr_common.hpp's banner maps; one number of any kind, exact or not:
* math::value, value.hpp; complex numbers and quaternions: math::complex<T>,
* complex.hpp, and math::quaternion<T>, quaternion.hpp; vectors, matrices and
* their algebra: linalg::vector, linalg::matrix and linalg::dynamic_matrix,
* linear_algebra/linalg.hpp; and the C core under them all, c/modules/math/
* (README.md, section 1).
*
* DIRECTORY MAP:
*   math_common.hpp - subsystem foundation: NS_MATH and the env include
*   expression.hpp  - value-holding expression core: constant_node,
*                     variable_node, binary/unary nodes, operators, combinators
*   function.hpp    - math_function + fluent builder, piecewise / vector /
*                     parametric / implicit forms, relational + logical layers,
*                     coordinate-system detection, axis sets
*   functional.hpp  - the bridge to the functional subframework: expressions
*                     as callables, curried, composed and filtered
*   coordinate.hpp  - coordinate-system trait layer; cartesian/polar/
*                     cylindrical/spherical.hpp - the concrete systems
*   interval.hpp    - unified interval type + folded interval traits
*   constants.hpp   - constants (djinterp::math::constants), compile-time
*                     rational, number_base / radix system
*   values.hpp      - compile-time sampling of an expression over points
*   value.hpp       - the generic value: math::value, one number of any
*                     kind over the C core's dj_math_value, and inline_value
*   number_traits.hpp - what generic code needs of a number type, the type
*                     two numbers combine to, and the concept number
*   complex.hpp     - complex numbers: math::complex<T>, of float, double
*                     and long double, the C core's cplx.h's face
*   quaternion.hpp  - the one quaternion, math::quaternion<T>, quat.h's
*                     face; linalg::quaternion is it (S398)
*   numeric_compare.hpp - numbers written as text, compared exactly
*   linear_algebra/ - vectors, matrices, dynamic matrices, decompositions,
*                     solving, eigenvalues, the SVD, transforms
*   expr/           - the run-time expression language: parsing,
*                     evaluating, compiling, simplifying, differentiating
*   geometry/       - geometry subframework (edges, surfaces, solids, measures)
*   calculus/       - calculus subframework (differentiation, integration,
*                     sequences, series, elementary functions)
*
*
* path:      /inc/djinterp/cpp/modules/math/math.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.24
*                                                            revised: 2026.11.14
*******************************************************************************/

#ifndef DJINTERP_MATH_MATH_HPP
#define DJINTERP_MATH_MATH_HPP 1

// djinterp
// foundation
#include "./math_common.hpp"
// expression core + functions
#include "./expression.hpp"
#include "./function/function.hpp"
#include "./functional.hpp"
// coordinate systems
#include "./coordinate/coordinate.hpp"
#include "./coordinate/cartesian.hpp"
#include "./coordinate/polar.hpp"
#include "./coordinate/cylindrical.hpp"
#include "./coordinate/spherical.hpp"
// numeric support
#include "./interval/interval.hpp"
#include "./constants.hpp"
#include "./values.hpp"
#include "./number_traits.hpp"
#include "./value.hpp"
#include "./complex.hpp"
#include "./quaternion.hpp"
#include "./numeric_compare.hpp"
// subframeworks
#include "./linear_algebra/linalg.hpp"
#include "./linear_algebra/quaternion.hpp"
#include "./geometry/geometry.hpp"
#include "./calculus/calculus.hpp"
#include "./expr/expr.hpp"


#endif  // DJINTERP_MATH_MATH_HPP
