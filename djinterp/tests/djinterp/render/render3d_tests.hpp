/*******************************************************************************
* djinterp [render]                                           render3d_tests.hpp
*
* Shared declarations for the render3d unit tests.
*   Covers what the render3d subframework gained with the ui port: the
* quaternion its umbrella re-exports, camera framing, and box projection.
*
*
* path:      /tests/djinterp/render/render3d_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_3D_RENDER3D_TESTS_HPP
#define DJINTERP_TESTS_3D_RENDER3D_TESTS_HPP 1

// std
#include <cmath>  // std::fabs
// djinterp
#include <djinterp/render/render3d.hpp>  // camera, box, quaternion, aabb


namespace djinterp
{
namespace testing
{

namespace la = ::djinterp::math::linalg;
namespace r3 = ::djinterp::render3d;

using vec3 = la::vector<double, 3>;

// near
//   helper: whether two numbers agree to within _eps.
inline bool
near(
    double _a,
    double _b,
    double _eps = 1e-9
)
{
    return (std::fabs(_a - _b) <= _eps);
}

// near_vec
//   helper: whether two vectors agree component by component.
inline bool
near_vec(
    const vec3& _a,
    const vec3& _b,
    double      _eps = 1e-9
)
{
    return ( (near(_a[0], _b[0], _eps)) &&
             (near(_a[1], _b[1], _eps)) &&
             (near(_a[2], _b[2], _eps)) );
}

// quaternion
bool tests_quaternion_rotation();
bool tests_quaternion_slerp();

// camera
bool tests_camera_frame();
bool tests_camera_frame_degenerate();

// box
bool tests_box_topology();
bool tests_box_projection();

}  // namespace testing
}  // namespace djinterp


#endif  // DJINTERP_TESTS_3D_RENDER3D_TESTS_HPP
