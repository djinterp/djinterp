/*******************************************************************************
* djinterp [render]                                       render3d_tests_box.cpp
*
* Unit tests for render3d's box topology and projection.
*
*
* path:      /tests/djinterp/render/render3d_tests_box.cpp
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

// bits
//   the number of set bits in _v.
int
bits(
    int _v
)
{
    int n = 0;

    for (; _v != 0; _v &= (_v - 1))
    {
        ++n;
    }

    return n;
}

}  // anonymous namespace

/*
tests_box_topology
  the corner, edge and face tables describe a cube
  Tests the following:
  - each edge joins corners differing on exactly one axis; each corner has 3
  - each face holds one axis fixed and walks its corners around the perimeter
  - each corner lies on exactly 3 faces
  - box_corner puts corner 0 at the minimum and corner 7 at the maximum
*/
bool
tests_box_topology()
{
    int edge_uses[8] = { 0 };
    int face_uses[8] = { 0 };

    for (const auto& e : r3::box_edges)
    {
        if (bits(e[0] ^ e[1]) != 1)
        {
            return false;
        }

        ++edge_uses[e[0]];
        ++edge_uses[e[1]];
    }

    for (const auto& f : r3::box_faces)
    {
        // a fixed axis: the bit shared by all four corners
        const int all_set   = f[0] & f[1] & f[2] & f[3];
        const int all_clear = ~(f[0] | f[1] | f[2] | f[3]) & 7;

        if ((bits(all_set) + bits(all_clear)) != 1)
        {
            return false;
        }

        // perimeter order: each corner is one axis away from the next
        for (std::size_t i = 0; i < 4; ++i)
        {
            if (bits(f[i] ^ f[(i + 1) % 4]) != 1)
            {
                return false;
            }

            ++face_uses[f[i]];
        }
    }

    for (int c = 0; c < 8; ++c)
    {
        if ( (edge_uses[c] != 3) ||
             (face_uses[c] != 3) )
        {
            return false;
        }
    }

    const vec3 centre(1.0, 2.0, 3.0);
    const vec3 half(0.5, 1.0, 1.5);

    return ( (near_vec(r3::box_corner(centre, half, 0), vec3(0.5, 1.0, 1.5))) &&
             (near_vec(r3::box_corner(centre, half, 7), vec3(1.5, 3.0, 4.5))) &&
             (near_vec(r3::box_corner(centre, half, 5), vec3(1.5, 1.0, 4.5))) );
}

/*
tests_box_projection
  projecting a box is projecting its corners
  Tests the following:
  - project_box of an aabb matches the camera projecting each corner
  - the centre is projected too
  - face_depth is the mean depth of the face's corners
*/
bool
tests_box_projection()
{
    const ::djinterp::math::aabb<3, double> box({ -1.0, -2.0, 0.5 },
                                                { 3.0, 1.0, 2.5 });
    r3::camera<double>         cam;
    const r3::viewport<double> vp(400.0, 300.0);

    cam.m_yaw = 0.8;
    cam.set_viewport(vp);
    cam.frame(box);

    const r3::projected_box<double> pb = r3::project_box(cam, vp, box);
    const vec3 centre(1.0, -0.5, 1.5);
    const vec3 half(2.0, 1.5, 1.0);

    for (int i = 0; i < 8; ++i)
    {
        const vec3 expect = cam.project(r3::box_corner(centre, half, i), vp);

        if (!near_vec(pb.m_corners[static_cast<std::size_t>(i)], expect))
        {
            return false;
        }
    }

    if (!near_vec(pb.m_center, cam.project(centre, vp)))
    {
        return false;
    }

    for (std::size_t f = 0; f < r3::box_faces.size(); ++f)
    {
        double sum = 0.0;

        for (const int c : r3::box_faces[f])
        {
            sum += pb.m_corners[static_cast<std::size_t>(c)][2];
        }

        if (!near(r3::face_depth(pb, f), sum / 4.0))
        {
            return false;
        }
    }

    return true;
}

}  // namespace testing
}  // namespace djinterp
