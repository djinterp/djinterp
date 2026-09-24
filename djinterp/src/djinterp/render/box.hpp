/*******************************************************************************
* djinterp [3d]                                                          box.hpp
*
* Axis-aligned boxes as a renderer sees them.
*   The corner, edge and face tables of a box, and its projection through a
* camera into screen space. Corner i's bits choose the sign of the half-extent
* on each axis -- bit 0 for x, bit 1 for y, bit 2 for z -- so corner 0 is the
* minimum and corner 7 the maximum. Faces list their corners in perimeter
* order, ready to fill as quadrilaterals.
*   A projected box keeps each corner's depth beside its screen position, which
* is what a painter's-algorithm renderer sorts by: face_depth and the projected
* centre give per-face and per-box keys.
*
* path:      /inc/djinterp/3d/box.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TOPOLOGY
    --------
    1.  Tables
         1.  box_edges
         2.  box_faces
    2.  Corners
         1.  box_corner
2.  PROJECTION
    ----------
    1.  projected_box
    2.  project_box
    3.  face_depth
*/

#ifndef DJINTERP_3D_BOX_HPP
#define DJINTERP_3D_BOX_HPP 1

// std
#include <array>    // std::array
#include <cstddef>  // std::size_t
// djinterp
#include "../djinterp.hpp"                      // framework root
#include "../math/geometry/geometry_common.hpp"  // math::aabb
#include "../math/linear_algebra/vector.hpp"     // linalg::vector
#include "./camera.hpp"                          // camera, viewport


NS_DJINTERP
namespace render3d
{


//==============================================================================
// 1.  TOPOLOGY
//==============================================================================


// 1.1    Tables
//------------------------------------------------------------------------------
// 1.1.1
// box_edges
//   constant: the twelve edges as pairs of corner indices, four along each
// axis.
inline constexpr std::array<std::array<int, 2>, 12> box_edges =
{ {
    { { 0, 1 } }, { { 2, 3 } }, { { 4, 5 } }, { { 6, 7 } },   // along x
    { { 0, 2 } }, { { 1, 3 } }, { { 4, 6 } }, { { 5, 7 } },   // along y
    { { 0, 4 } }, { { 1, 5 } }, { { 2, 6 } }, { { 3, 7 } }    // along z
} };

// 1.1.2
// box_faces
//   constant: the six faces as four corner indices in perimeter order:
// -z, +z, -y, +y, -x, +x.
inline constexpr std::array<std::array<int, 4>, 6> box_faces =
{ {
    { { 0, 1, 3, 2 } },
    { { 4, 5, 7, 6 } },
    { { 0, 1, 5, 4 } },
    { { 2, 3, 7, 6 } },
    { { 0, 2, 6, 4 } },
    { { 1, 3, 7, 5 } }
} };


// 1.2    Corners
//------------------------------------------------------------------------------
// 1.2.1
// box_corner
//   function: corner _i (0 to 7) of the box centred at _center with the
// half-extents _half.
template<typename T>
D_NODISCARD D_CONSTEXPR math::linalg::vector<T, 3>
box_corner(
    const math::linalg::vector<T, 3>& _center,
    const math::linalg::vector<T, 3>& _half,
    int                                _i
) noexcept
{
    return math::linalg::vector<T, 3>(
        _center[0] + ((_i & 1) ? _half[0] : -_half[0]),
        _center[1] + ((_i & 2) ? _half[1] : -_half[1]),
        _center[2] + ((_i & 4) ? _half[2] : -_half[2]));
}


//==============================================================================
// 2.  PROJECTION
//==============================================================================


// 2.1    projected_box
//------------------------------------------------------------------------------
// 2.1.1
// projected_box
//   struct: a box after camera projection -- each corner as (x, y) in pixels
// and z as its depth key (larger is farther), plus the projected centre.
template<typename T = double>
struct projected_box
{
    using vec3_type = math::linalg::vector<T, 3>;

    std::array<vec3_type, 8> m_corners;
    vec3_type                m_center;
};


// 2.2    project_box
//------------------------------------------------------------------------------
// 2.2.1
// project_box
//   function: the box centred at _center with half-extents _half, projected
// through _camera onto _viewport.
template<typename T>
D_NODISCARD projected_box<T>
project_box(
    const camera<T>&                  _camera,
    const viewport<T>&                _viewport,
    const math::linalg::vector<T, 3>& _center,
    const math::linalg::vector<T, 3>& _half
) noexcept
{
    projected_box<T> out;

    for (int i = 0; i < 8; ++i)
    {
        out.m_corners[static_cast<std::size_t>(i)] =
            _camera.project(box_corner(_center, _half, i), _viewport);
    }

    out.m_center = _camera.project(_center, _viewport);

    return out;
}

// 2.2.2
// project_box
//   function: the same for a math::aabb.
template<typename T>
D_NODISCARD projected_box<T>
project_box(
    const camera<T>&         _camera,
    const viewport<T>&       _viewport,
    const math::aabb<3, T>&  _box
) noexcept
{
    const auto c = _box.center();

    return project_box(_camera,
                       _viewport,
                       math::linalg::vector<T, 3>(c[0], c[1], c[2]),
                       math::linalg::vector<T, 3>(
                           _box.extent(0) / static_cast<T>(2),
                           _box.extent(1) / static_cast<T>(2),
                           _box.extent(2) / static_cast<T>(2)));
}


// 2.3    face_depth
//------------------------------------------------------------------------------
// 2.3.1
// face_depth
//   function: the mean depth of face _face's corners, the sort key for
// drawing translucent faces back to front.
template<typename T>
D_NODISCARD D_CONSTEXPR T
face_depth(
    const projected_box<T>& _box,
    std::size_t              _face
) noexcept
{
    T sum = static_cast<T>(0);

    for (const int corner : box_faces[_face])
    {
        sum += _box.m_corners[static_cast<std::size_t>(corner)][2];
    }

    return sum / static_cast<T>(4);
}

}  // namespace render3d
NS_END  // djinterp


#endif  // DJINTERP_3D_BOX_HPP
