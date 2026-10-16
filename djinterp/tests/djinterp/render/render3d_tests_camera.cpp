/*******************************************************************************
* djinterp [render]                                    render3d_tests_camera.cpp
*
* Unit tests for render3d::camera::frame.
*
*
* path:      /tests/djinterp/render/render3d_tests_camera.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "render3d_tests.hpp"  // corresponding header


namespace djinterp
{
namespace testing
{

namespace
{

using box3 = ::djinterp::math::aabb<3, double>;

// in_view
//   whether every corner of _box projects inside _vp and within the depth
// range, and how much of the narrower side the corners span.
bool
in_view(
    const r3::camera<double>&   _cam,
    const r3::viewport<double>& _vp,
    const box3&                 _box,
    double*                     _span
)
{
    double lo_x = 1e30;
    double hi_x = -1e30;
    double lo_y = 1e30;
    double hi_y = -1e30;

    const r3::projected_box<double> pb = r3::project_box(_cam, _vp, _box);

    for (const vec3& s : pb.m_corners)
    {
        // off the canvas, or clipped by a clip plane
        if ( (s[0] < 0.0) || (s[0] > _vp.m_width)  ||
             (s[1] < 0.0) || (s[1] > _vp.m_height) ||
             (s[2] < -1.0) || (s[2] > 1.0) )
        {
            return false;
        }

        lo_x = (s[0] < lo_x) ? s[0] : lo_x;
        hi_x = (s[0] > hi_x) ? s[0] : hi_x;
        lo_y = (s[1] < lo_y) ? s[1] : lo_y;
        hi_y = (s[1] > hi_y) ? s[1] : hi_y;
    }

    if (_span)
    {
        const double narrow = (_vp.m_width < _vp.m_height) ? _vp.m_width
                                                           : _vp.m_height;
        const double wide_x = (hi_x - lo_x) / narrow;
        const double wide_y = (hi_y - lo_y) / narrow;

        *_span = (wide_x > wide_y) ? wide_x : wide_y;
    }

    return true;
}

}  // anonymous namespace

/*
tests_camera_frame
  framing a box puts all of it in view, snugly, without turning the camera
  Tests the following:
  - the target is the box centre; yaw and pitch are kept
  - every corner projects inside the viewport and the depth range
  - the box spans a good part of the view, not a speck in the middle
  - the clip planes bracket the box
*/
bool
tests_camera_frame()
{
    r3::camera<double>         cam;
    const r3::viewport<double> vp(800.0, 600.0);
    const box3                 box({ -1.0, 2.0, -3.0 }, { 4.0, 5.0, 1.0 });

    cam.m_yaw   = 0.6;
    cam.m_pitch = 0.4;
    cam.set_viewport(vp);
    cam.frame(box);

    const auto c = box.center();

    if ( (!near_vec(cam.m_target, vec3(c[0], c[1], c[2]))) ||
         (!near(cam.m_yaw, 0.6)) ||
         (!near(cam.m_pitch, 0.4)) )
    {
        return false;
    }

    double span = 0.0;

    if ( (!in_view(cam, vp, box, &span)) ||
         (span < 0.35) )
    {
        return false;
    }

    // the planes bracket the box's bounding sphere
    const double r = 0.5 * std::sqrt((5.0 * 5.0) + (3.0 * 3.0) + (4.0 * 4.0));

    return ( (cam.m_z_near < (cam.m_distance - r)) &&
             (cam.m_z_far > (cam.m_distance + r)) );
}

/*
tests_camera_frame_degenerate
  the edges of framing: empty boxes, points, flat boxes, narrow views
  Tests the following:
  - an empty box leaves the camera exactly as it was
  - a single point is framed at the centre of the view
  - a flat box (no depth) is framed like any other
  - a tall, narrow viewport frames by its narrower horizontal field of view
*/
bool
tests_camera_frame_degenerate()
{
    r3::camera<double>         cam;
    const r3::viewport<double> vp(640.0, 480.0);

    cam.set_viewport(vp);

    const double distance = cam.m_distance;

    // an empty box has nothing to aim at
    cam.frame(box3::empty());

    if ( (!near(cam.m_distance, distance)) ||
         (!near_vec(cam.m_target, vec3(0.0, 0.0, 0.0))) )
    {
        return false;
    }

    // a single point lands in the middle of the view
    cam.frame(box3({ 2.0, 2.0, 2.0 }, { 2.0, 2.0, 2.0 }));

    const vec3 mid = cam.project(vec3(2.0, 2.0, 2.0), vp);

    if ( (!near(mid[0], 320.0, 1e-6)) ||
         (!near(mid[1], 240.0, 1e-6)) ||
         (!(cam.m_distance > 0.0)) )
    {
        return false;
    }

    // a flat box, as a heatmap scene is
    const box3 flat({ 0.0, 0.0, 0.0 }, { 30.0, 6.0, 0.0 });

    cam.frame(flat);

    if (!in_view(cam, vp, flat, nullptr))
    {
        return false;
    }

    // tall and narrow: the horizontal field of view is the tighter one
    r3::camera<double>         tall;
    const r3::viewport<double> narrow(120.0, 900.0);
    const box3                 wide({ -10.0, -1.0, -1.0 }, { 10.0, 1.0, 1.0 });

    tall.set_viewport(narrow);
    tall.frame(wide);

    return in_view(tall, narrow, wide, nullptr);
}

}  // namespace testing
}  // namespace djinterp
