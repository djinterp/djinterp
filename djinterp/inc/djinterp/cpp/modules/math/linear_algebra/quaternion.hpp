/*******************************************************************************
* djinterp [cpp/math]                                             quaternion.hpp
*
* Quaternions in linear algebra's namespace: linalg::quaternion and its
* functions are math::quaternion and its (../quaternion.hpp), named here by
* using-declarations (S398).
*   The framework has one quaternion. It was two until chunk 48: this
* header's linalg::quaternion and ../quaternion.hpp's math::quaternion,
* each a face of the C core's c/math/quat.h through internal::linalg_kernel,
* the same algebra under two names. Two classes made two of every function,
* and a call such as from_matrix(m) of a linalg::matrix, its own namespace
* searched by argument, ambiguous wherever both headers were read. Now a
* using-declaration names the one class and each of its functions here, so
* that linalg::from_axis_angle(axis, a) and math::from_axis_angle(axis, a)
* are the same function, and no call is ambiguous.
*   Every name linalg::quaternion's code used stands: the members m_x, m_y,
* m_z and m_w, the default element type double, identity() and
* vector_part(), from_axis_angle, from_matrix, from_two_vectors,
* multiply_fast, inverse, left_divide, conjugate, norm, normalize,
* normalize_fast, rotate, rotate_fast, to_matrix3, to_matrix3_fast,
* to_matrix4, to_matrix4_fast, slerp, slerp_fast, nlerp, exp, log, sqrt and
* pow; the operators * and / are found by argument, in math. The one class's
* conventions, its accuracy and its constant expressions are
* ../quaternion.hpp's.
*
*
* path:      /inc/djinterp/cpp/modules/math/linear_algebra/quaternion.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.11.14
*******************************************************************************/
#ifndef DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
#define DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP 1
// djinterp
#include "../../../djinterp.hpp"   // framework root
#include "../quaternion.hpp"       // math::quaternion, its functions


NS_DJINTERP
NS_MATH
namespace linalg
{

// the one class (S398)
using ::djinterp::math::quaternion;

// construction
using ::djinterp::math::from_axis_angle;
using ::djinterp::math::from_matrix;
using ::djinterp::math::from_two_vectors;

// algebra
using ::djinterp::math::multiply_fast;
using ::djinterp::math::inverse;
using ::djinterp::math::left_divide;
using ::djinterp::math::conjugate;
using ::djinterp::math::norm;
using ::djinterp::math::normalize;
using ::djinterp::math::normalize_fast;

// rotation
using ::djinterp::math::rotate;
using ::djinterp::math::rotate_fast;
using ::djinterp::math::to_matrix3;
using ::djinterp::math::to_matrix3_fast;
using ::djinterp::math::to_matrix4;
using ::djinterp::math::to_matrix4_fast;

// interpolation
using ::djinterp::math::slerp;
using ::djinterp::math::slerp_fast;
using ::djinterp::math::nlerp;

// exponential, logarithm and powers
using ::djinterp::math::exp;
using ::djinterp::math::log;
using ::djinterp::math::sqrt;
using ::djinterp::math::pow;

}  // namespace linalg
NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_LINEAR_ALGEBRA_QUATERNION_HPP
