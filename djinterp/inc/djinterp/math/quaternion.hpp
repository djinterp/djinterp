/******************************************************************************
* djinterp [math]                                              quaternion.hpp
*
* Unit quaternions for representing and composing 3D rotations without gimbal
* lock.
*   quaternion<_T> stores the vector part (x, y, z) and scalar part w of the
* rotation x i + y j + z k + w. Construction is from an axis and angle; the
* Hamilton product composes rotations; a vector is rotated by the sandwich
* product; and the rotation converts to a 3x3 (or homogeneous 4x4) linalg
* matrix. Spherical linear interpolation (slerp) blends two orientations along
* the shortest arc. Rotation operations assume a unit quaternion.
*
* BRIDGE TO LINEAR ALGEBRA:
*   axes and rotated vectors are linalg::vector<_T, 3>; to_matrix / to_matrix4
* yield linalg::matrix values, so a quaternion drops straight into the same
* transform pipeline as linalg::transform's rotation builders.
*
* CONSTEXPR:
*   the purely algebraic operations (product, conjugate, rotate, to_matrix,
* dot) are D_CONSTEXPR. The ones that need transcendental functions or a
* square root (from_axis_angle, norm, normalize, slerp) are ordinary runtime
* functions, mirroring the scalar kernel split elsewhere in the subframework.
*
* path:      /inc/djinterp/math/quaternion.hpp
* link:      TBA
* author(s): TBA                                           created: 2026.06.18
******************************************************************************/

#ifndef DJINTERP_MATH_QUATERNION_
#define DJINTERP_MATH_QUATERNION_ 1

// std
#include <cstddef>
#include <cmath>
// djinterp [math]
#include "./math_common.hpp"
#include "./linear_algebra/vector.hpp"
#include "./linear_algebra/matrix.hpp"


NS_DJINTERP
NS_MATH

// ===========================================================================
// I.    QUATERNION
// ===========================================================================

// quaternion
//   struct: a quaternion x i + y j + z k + w, with vector part (x, y, z) and
// scalar part w. The default value is the identity rotation.
template<typename _T>
struct quaternion
{
    // ---- type aliases -------------------------------------------------------

    using value_type  = _T;
    using vector_type = linalg::vector<_T, 3>;

    // ---- data ---------------------------------------------------------------

    _T x;
    _T y;
    _T z;
    _T w;

    // ---- construction -------------------------------------------------------

    // default: the identity rotation (0, 0, 0, 1).
    D_CONSTEXPR
    quaternion() noexcept
        : x(static_cast<_T>(0)),
          y(static_cast<_T>(0)),
          z(static_cast<_T>(0)),
          w(static_cast<_T>(1))
    {
    }

    // from explicit components.
    D_CONSTEXPR
    quaternion(
        _T _x,
        _T _y,
        _T _z,
        _T _w
    ) noexcept
        : x(_x),
          y(_y),
          z(_z),
          w(_w)
    {
    }

    // ---- named factories ----------------------------------------------------

    // identity: the identity rotation.
    static D_CONSTEXPR quaternion
    identity() noexcept
    {
        return quaternion();
    }
};


// ===========================================================================
// II.   CONSTRUCTION FROM AN AXIS AND ANGLE
// ===========================================================================

// from_axis_angle
//   the rotation of _angle radians about _axis. The axis is normalized
// internally, so it need not be a unit vector.
template<typename _T>
D_NODISCARD quaternion<_T>
from_axis_angle(
    const linalg::vector<_T, 3>& _axis,
    _T                           _angle
) noexcept
{
    const linalg::vector<_T, 3> a    = normalize(_axis);
    const _T                    half = _angle / static_cast<_T>(2);
    const _T                    s    = std::sin(half);

    return quaternion<_T>(a[0] * s,
                          a[1] * s,
                          a[2] * s,
                          std::cos(half));
}


// ===========================================================================
// III.  ALGEBRA
// ===========================================================================

// operator*
//   the Hamilton product, composing the rotation _a after _b.
template<typename _T>
D_NODISCARD D_CONSTEXPR quaternion<_T>
operator*(
    const quaternion<_T>& _a,
    const quaternion<_T>& _b
) noexcept
{
    return quaternion<_T>(
        _a.w * _b.x + _a.x * _b.w + _a.y * _b.z - _a.z * _b.y,
        _a.w * _b.y - _a.x * _b.z + _a.y * _b.w + _a.z * _b.x,
        _a.w * _b.z + _a.x * _b.y - _a.y * _b.x + _a.z * _b.w,
        _a.w * _b.w - _a.x * _b.x - _a.y * _b.y - _a.z * _b.z
    );
}

// dot
//   the four-component inner product (the cosine of the angle between the two
// orientations on the unit hypersphere).
template<typename _T>
D_NODISCARD D_CONSTEXPR _T
dot(
    const quaternion<_T>& _a,
    const quaternion<_T>& _b
) noexcept
{
    return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
}

// conjugate
//   the conjugate (x, y, z -> -x, -y, -z). For a unit quaternion this is the
// inverse rotation.
template<typename _T>
D_NODISCARD D_CONSTEXPR quaternion<_T>
conjugate(
    const quaternion<_T>& _q
) noexcept
{
    return quaternion<_T>(-_q.x, -_q.y, -_q.z, _q.w);
}

// norm
//   the Euclidean norm of the four components.
template<typename _T>
D_NODISCARD _T
norm(
    const quaternion<_T>& _q
) noexcept
{
    return std::sqrt(dot(_q, _q));
}

// normalize
//   the quaternion scaled to unit norm; a zero quaternion is returned
// unchanged (it has no norm to divide by).
template<typename _T>
D_NODISCARD quaternion<_T>
normalize(
    const quaternion<_T>& _q
) noexcept
{
    const _T n = norm(_q);

    if (n == static_cast<_T>(0))
    {
        return _q;
    }

    return quaternion<_T>(_q.x / n, _q.y / n, _q.z / n, _q.w / n);
}


// ===========================================================================
// IV.   ROTATION OF A VECTOR
// ===========================================================================

// rotate
//   rotates _v by the (assumed unit) quaternion _q via the sandwich product,
// expanded so no intermediate quaternion is formed:
//   v' = v + 2 w (u x v) + 2 (u x (u x v)),  u = (x, y, z).
template<typename _T>
D_NODISCARD D_CONSTEXPR linalg::vector<_T, 3>
rotate(
    const quaternion<_T>&        _q,
    const linalg::vector<_T, 3>& _v
) noexcept
{
    const linalg::vector<_T, 3> u(_q.x, _q.y, _q.z);
    const linalg::vector<_T, 3> t = static_cast<_T>(2) * cross(u, _v);

    return _v + _q.w * t + cross(u, t);
}


// ===========================================================================
// V.    CONVERSION TO A ROTATION MATRIX
// ===========================================================================

// to_matrix
//   the 3x3 rotation matrix of the (assumed unit) quaternion _q.
template<typename _T>
D_NODISCARD D_CONSTEXPR linalg::matrix<_T, 3, 3>
to_matrix(
    const quaternion<_T>& _q
) noexcept
{
    const _T x   = _q.x;
    const _T y   = _q.y;
    const _T z   = _q.z;
    const _T w   = _q.w;
    const _T two = static_cast<_T>(2);
    const _T one = static_cast<_T>(1);

    linalg::matrix<_T, 3, 3> r = linalg::matrix<_T, 3, 3>::zeros();

    r(0, 0) = one - two * (y * y + z * z);
    r(0, 1) = two * (x * y - w * z);
    r(0, 2) = two * (x * z + w * y);

    r(1, 0) = two * (x * y + w * z);
    r(1, 1) = one - two * (x * x + z * z);
    r(1, 2) = two * (y * z - w * x);

    r(2, 0) = two * (x * z - w * y);
    r(2, 1) = two * (y * z + w * x);
    r(2, 2) = one - two * (x * x + y * y);

    return r;
}

// to_matrix4
//   the rotation as a 4x4 homogeneous matrix: the 3x3 rotation in the upper
// left, an identity fourth row and column.
template<typename _T>
D_NODISCARD D_CONSTEXPR linalg::matrix<_T, 4, 4>
to_matrix4(
    const quaternion<_T>& _q
) noexcept
{
    const linalg::matrix<_T, 3, 3> r3 = to_matrix(_q);

    linalg::matrix<_T, 4, 4> r = linalg::matrix<_T, 4, 4>::identity();

    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            r(i, j) = r3(i, j);
        }
    }

    return r;
}


// ===========================================================================
// VI.   SPHERICAL LINEAR INTERPOLATION
// ===========================================================================

// slerp
//   spherical linear interpolation between two unit quaternions along the
// shorter arc. Nearly parallel inputs fall back to normalized linear
// interpolation to avoid dividing by a vanishing sine.
template<typename _T>
D_NODISCARD quaternion<_T>
slerp(
    const quaternion<_T>& _a,
    const quaternion<_T>& _b,
    _T                    _t
) noexcept
{
    _T             d = dot(_a, _b);
    quaternion<_T> b = _b;

    // take the shorter arc
    if (d < static_cast<_T>(0))
    {
        b = quaternion<_T>(-_b.x, -_b.y, -_b.z, -_b.w);
        d = -d;
    }

    // nearly parallel: linear interpolation, renormalized
    if (d > static_cast<_T>(0.9995))
    {
        const quaternion<_T> r(
            _a.x + _t * (b.x - _a.x),
            _a.y + _t * (b.y - _a.y),
            _a.z + _t * (b.z - _a.z),
            _a.w + _t * (b.w - _a.w)
        );

        return normalize(r);
    }

    const _T theta_0 = std::acos(d);
    const _T theta   = theta_0 * _t;
    const _T sin_0   = std::sin(theta_0);

    const _T s_a = std::sin(theta_0 - theta) / sin_0;
    const _T s_b = std::sin(theta) / sin_0;

    return quaternion<_T>(
        s_a * _a.x + s_b * b.x,
        s_a * _a.y + s_b * b.y,
        s_a * _a.z + s_b * b.z,
        s_a * _a.w + s_b * b.w
    );
}

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_QUATERNION_
