/*******************************************************************************
* djinterp [3d]                                                  projections.hpp
*
* The projective and view transforms a camera needs, built as 4x4 linalg
* matrices.
*   look_at is a right-handed world -> view transform; perspective and
* orthographic are right-handed view -> clip projections mapping depth to the
* NDC range [-1, 1]. These are the pieces the linear-algebra subframework
* deliberately leaves out: linalg::transform covers the affine builders
* (scaling, rotation, translation, the homogeneous embedding), while the
* camera-specific projective transforms live here, next to their only
* consumer.
*
* CONVENTION:
*   row-major storage, column-vector convention -- a point is transformed as
* M * p, and the translation lives in the fourth column. Consistent with
* linalg::matrix and linalg::transform_point.
*
* CONSTEXPR:
*   orthographic is D_CONSTEXPR (pure arithmetic). look_at and perspective
* need a square root / tangent and so are ordinary runtime functions.
*
* path:      /inc/djinterp/3d/projections.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.09.23
*******************************************************************************/

#ifndef DJINTERP_3D_PROJECTIONS_HPP
#define DJINTERP_3D_PROJECTIONS_HPP 1

// std
#include <cstddef>
#include <cmath>
// djinterp
#include "../djinterp.hpp"
#include "../math/linear_algebra/vector.hpp"
#include "../math/linear_algebra/matrix.hpp"


NS_DJINTERP
namespace render3d
{

// ===========================================================================
// I.    VIEW TRANSFORM
// ===========================================================================

// look_at
//   a right-handed view matrix placing the camera at _eye looking toward
// _target, with _up giving the approximate up direction. The returned matrix
// maps world space into the camera's view space.
template<typename _T>
D_NODISCARD math::linalg::matrix<_T, 4, 4>
look_at(
    const math::linalg::vector<_T, 3>& _eye,
    const math::linalg::vector<_T, 3>& _target,
    const math::linalg::vector<_T, 3>& _up
) noexcept
{
    const math::linalg::vector<_T, 3> f = normalize(_target - _eye);  // forward
    const math::linalg::vector<_T, 3> s = normalize(cross(f, _up));    // right
    const math::linalg::vector<_T, 3> u = cross(s, f);                 // true up

    math::linalg::matrix<_T, 4, 4> r = math::linalg::matrix<_T, 4, 4>::identity();

    r(0, 0) = s[0];
    r(0, 1) = s[1];
    r(0, 2) = s[2];
    r(0, 3) = -dot(s, _eye);

    r(1, 0) = u[0];
    r(1, 1) = u[1];
    r(1, 2) = u[2];
    r(1, 3) = -dot(u, _eye);

    r(2, 0) = -f[0];
    r(2, 1) = -f[1];
    r(2, 2) = -f[2];
    r(2, 3) = dot(f, _eye);

    return r;
}


// ===========================================================================
// II.   PROJECTIONS
// ===========================================================================

// perspective
//   a right-handed perspective projection. _fovy is the vertical field of view
// in radians, _aspect is width / height, and _z_near / _z_far are the
// (positive) clip-plane distances. Maps depth to NDC [-1, 1].
template<typename _T>
D_NODISCARD math::linalg::matrix<_T, 4, 4>
perspective(
    _T _fovy,
    _T _aspect,
    _T _z_near,
    _T _z_far
) noexcept
{
    const _T tan_half = std::tan(_fovy / static_cast<_T>(2));

    math::linalg::matrix<_T, 4, 4> r = math::linalg::matrix<_T, 4, 4>::zeros();

    r(0, 0) = static_cast<_T>(1) / (_aspect * tan_half);
    r(1, 1) = static_cast<_T>(1) / tan_half;
    r(2, 2) = -(_z_far + _z_near) / (_z_far - _z_near);
    r(2, 3) = -(static_cast<_T>(2) * _z_far * _z_near) / (_z_far - _z_near);
    r(3, 2) = static_cast<_T>(-1);

    return r;
}

// orthographic
//   a right-handed orthographic projection of the box
// [_left, _right] x [_bottom, _top] x [_z_near, _z_far] onto NDC [-1, 1].
template<typename _T>
D_NODISCARD D_CONSTEXPR math::linalg::matrix<_T, 4, 4>
orthographic(
    _T _left,
    _T _right,
    _T _bottom,
    _T _top,
    _T _z_near,
    _T _z_far
) noexcept
{
    math::linalg::matrix<_T, 4, 4> r = math::linalg::matrix<_T, 4, 4>::zeros();

    r(0, 0) = static_cast<_T>(2) / (_right - _left);
    r(1, 1) = static_cast<_T>(2) / (_top - _bottom);
    r(2, 2) = static_cast<_T>(-2) / (_z_far - _z_near);

    r(0, 3) = -(_right + _left)   / (_right - _left);
    r(1, 3) = -(_top   + _bottom) / (_top   - _bottom);
    r(2, 3) = -(_z_far + _z_near) / (_z_far - _z_near);
    r(3, 3) = static_cast<_T>(1);

    return r;
}

}  // namespace render3d
NS_END  // djinterp


#endif  // DJINTERP_3D_PROJECTIONS_HPP
