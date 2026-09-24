/*******************************************************************************
* djinterp [3d]                                                       camera.hpp
*
*   A viewport and an orbit (turntable) camera: the common interactive 3D
* camera for inspecting a scene. The camera orbits a target at a distance with
* yaw / pitch, builds the view and projection matrices, and supports the three
* standard gestures (orbit, zoom, pan). project() maps a world point to screen
* pixels; pick_ray() builds the world-space ray under a screen pixel for
* hit-testing. The pick ray is assembled from the camera basis directly, so no
* matrix inverse is required. frame() aims the camera at a box and backs off
* until all of it is in view.
*
*   Positions and directions are linalg::vector<_T, 3>; the matrices are
* linalg::matrix<_T, 4, 4> from render3d::projections. pick_ray returns a
* math::ray, so the geometry subframework's intersection predicates apply
* directly to what the camera produces.
*
* path:      /inc/djinterp/3d/camera.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.09.23
*******************************************************************************/

#ifndef DJINTERP_3D_CAMERA_HPP
#define DJINTERP_3D_CAMERA_HPP 1

// std
#include <cmath>
// djinterp
#include "../djinterp.hpp"
#include "../math/linear_algebra/vector.hpp"
#include "../math/linear_algebra/matrix.hpp"
#include "../math/linear_algebra/transform.hpp"   // to_array (vector -> point_type)
#include "../math/geometry/geometry_common.hpp"   // math::aabb (frame)
#include "../math/geometry/ray.hpp"               // math::ray
#include "./projections.hpp"                       // look_at, perspective


NS_DJINTERP
namespace render3d
{

// ===========================================================================
// I.    VIEWPORT
// ===========================================================================

// viewport
//   struct: the pixel dimensions of the render target. Width and height are
// stored as _T so the aspect ratio is exact.
template<typename _T = double>
struct viewport
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _T;

    // ---- data ---------------------------------------------------------------

    _T m_width;
    _T m_height;

    // ---- construction -------------------------------------------------------

    constexpr viewport() noexcept
        : m_width(static_cast<_T>(1)),
          m_height(static_cast<_T>(1))
    {}

    constexpr viewport(
        _T _width,
        _T _height
    ) noexcept
        : m_width(_width),
          m_height(_height)
    {}

    // ---- queries ------------------------------------------------------------

    // aspect
    //   the width-to-height ratio.
    D_NODISCARD constexpr _T
    aspect() const noexcept
    {
        return m_width / m_height;
    }
};


// ===========================================================================
// II.   ORBIT CAMERA
// ===========================================================================

// camera
//   struct: an orbit camera. The eye sits at distance m_distance from
// m_target, in the direction given by m_yaw (about the world +y axis) and
// m_pitch (elevation). World up is +y.
template<typename _T = double>
struct camera
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _T;
    using vec3_type  = math::linalg::vector<_T, 3>;
    using vec4_type  = math::linalg::vector<_T, 4>;
    using matrix_type = math::linalg::matrix<_T, 4, 4>;
    using ray_type   = math::ray<_T>;

    // ---- data ---------------------------------------------------------------

    vec3_type m_target;
    _T        m_distance;
    _T        m_yaw;
    _T        m_pitch;
    _T        m_fov_y;
    _T        m_aspect;
    _T        m_z_near;
    _T        m_z_far;

    // ---- construction -------------------------------------------------------

    camera() noexcept
        : m_target{},
          m_distance(static_cast<_T>(5)),
          m_yaw(static_cast<_T>(0)),
          m_pitch(static_cast<_T>(0.3)),
          m_fov_y(static_cast<_T>(1.0471975511965976)),   // 60 degrees
          m_aspect(static_cast<_T>(1)),
          m_z_near(static_cast<_T>(0.1)),
          m_z_far(static_cast<_T>(1000))
    {}

    // ---- basis --------------------------------------------------------------

    // world_up
    //   the world up direction (+y).
    D_NODISCARD constexpr vec3_type
    world_up() const noexcept
    {
        return vec3_type(static_cast<_T>(0),
                         static_cast<_T>(1),
                         static_cast<_T>(0));
    }

    // eye
    //   the camera position in world space.
    D_NODISCARD vec3_type
    eye() const noexcept
    {
        const _T cp = std::cos(m_pitch);
        const _T sp = std::sin(m_pitch);
        const _T cy = std::cos(m_yaw);
        const _T sy = std::sin(m_yaw);

        const vec3_type offset(m_distance * cp * sy,
                               m_distance * sp,
                               m_distance * cp * cy);

        return m_target + offset;
    }

    // forward
    //   the unit direction from the eye toward the target.
    D_NODISCARD vec3_type
    forward() const noexcept
    {
        return normalize(m_target - eye());
    }

    // right
    //   the unit rightward direction.
    D_NODISCARD vec3_type
    right() const noexcept
    {
        return normalize(cross(forward(), world_up()));
    }

    // up
    //   the unit upward direction orthogonal to forward and right.
    D_NODISCARD vec3_type
    up() const noexcept
    {
        return cross(right(), forward());
    }

    // ---- transforms ---------------------------------------------------------

    // view
    //   the view matrix (world -> view).
    D_NODISCARD matrix_type
    view() const noexcept
    {
        return look_at(eye(), m_target, world_up());
    }

    // projection
    //   the perspective projection matrix (view -> clip).
    D_NODISCARD matrix_type
    projection() const noexcept
    {
        return perspective(m_fov_y, m_aspect, m_z_near, m_z_far);
    }

    // view_projection
    //   the combined world -> clip matrix.
    D_NODISCARD matrix_type
    view_projection() const noexcept
    {
        return projection() * view();
    }

    // ---- interaction --------------------------------------------------------

    // set_viewport
    //   updates the aspect ratio from a viewport.
    void
    set_viewport(
        const viewport<_T>& _vp
    ) noexcept
    {
        m_aspect = _vp.aspect();

        return;
    }

    // orbit
    //   rotates the camera about the target by the given yaw and pitch deltas
    // (radians). Pitch is clamped just short of the poles to keep the up vector
    // well-defined.
    void
    orbit(
        _T _d_yaw,
        _T _d_pitch
    ) noexcept
    {
        const _T limit =
            static_cast<_T>(1.5707963267948966) - static_cast<_T>(0.01);

        m_yaw   += _d_yaw;
        m_pitch += _d_pitch;

        if (m_pitch > limit)
        {
            m_pitch = limit;
        }

        if (m_pitch < -limit)
        {
            m_pitch = -limit;
        }

        return;
    }

    // zoom
    //   scales the orbit distance by _factor (e.g. 0.9 to move closer), clamped
    // to a small positive minimum.
    void
    zoom(
        _T _factor
    ) noexcept
    {
        m_distance *= _factor;

        if (m_distance < static_cast<_T>(1e-3))
        {
            m_distance = static_cast<_T>(1e-3);
        }

        return;
    }

    // pan
    //   slides the target within the camera plane. The deltas are scaled by
    // distance so the gesture feels consistent at any zoom level.
    void
    pan(
        _T _dx,
        _T _dy
    ) noexcept
    {
        const vec3_type r = right();
        const vec3_type u = up();

        m_target = m_target +
                   r * (_dx * m_distance) +
                   u * (_dy * m_distance);

        return;
    }

    // ---- framing ------------------------------------------------------------

    // frame
    //   aims the camera at the centre of _box and sets the distance so the
    // whole box is in view, keeping the current yaw and pitch. The fit uses
    // the box's bounding sphere against the narrower of the vertical and
    // horizontal fields of view, so call set_viewport first. _margin scales
    // the distance (1 just fits; the default leaves a tenth spare). The clip
    // planes move with the box so small and large scenes are not clipped. An
    // empty box leaves the camera unchanged.
    void
    frame(
        const math::aabb<3, _T>& _box,
        _T                       _margin = static_cast<_T>(1.1)
    ) noexcept
    {
        // nothing to aim at
        if (_box.is_empty())
        {
            return;
        }

        const auto c = _box.center();
        m_target = vec3_type(c[0], c[1], c[2]);

        _T r2 = static_cast<_T>(0);

        for (std::size_t i = 0; i < 3; ++i)
        {
            const _T half = _box.extent(i) / static_cast<_T>(2);
            r2 += half * half;
        }

        // a single point still needs somewhere to stand back from
        const _T radius = (r2 > static_cast<_T>(0))
                              ? std::sqrt(r2)
                              : static_cast<_T>(0.5);
        const _T half_v = m_fov_y / static_cast<_T>(2);
        const _T half_h = std::atan(std::tan(half_v) * m_aspect);
        const _T half   = (half_h < half_v) ? half_h : half_v;

        m_distance = (radius / std::sin(half)) * _margin;
        m_z_far    = (m_distance + radius) * static_cast<_T>(2);
        m_z_near   = ((m_distance > (radius * static_cast<_T>(1.01)))
                          ? (m_distance - radius)
                          : (m_distance * static_cast<_T>(0.01))) *
                     static_cast<_T>(0.5);

        return;
    }

    // ---- screen mapping -----------------------------------------------------

    // project
    //   maps a world point to screen space: x and y in pixels (origin
    // top-left), z the NDC depth in [-1, 1].
    D_NODISCARD vec3_type
    project(
        const vec3_type&    _world,
        const viewport<_T>& _vp
    ) const noexcept
    {
        const matrix_type vp   = view_projection();
        const vec4_type   clip = vp * vec4_type(_world[0],
                                                _world[1],
                                                _world[2],
                                                static_cast<_T>(1));

        _T w = clip[3];

        if (w == static_cast<_T>(0))
        {
            w = static_cast<_T>(1);
        }

        const _T ndc_x = clip[0] / w;
        const _T ndc_y = clip[1] / w;
        const _T ndc_z = clip[2] / w;

        const _T sx = (ndc_x * static_cast<_T>(0.5) + static_cast<_T>(0.5)) *
                      _vp.m_width;
        const _T sy = (static_cast<_T>(1) -
                       (ndc_y * static_cast<_T>(0.5) + static_cast<_T>(0.5))) *
                      _vp.m_height;

        return vec3_type(sx, sy, ndc_z);
    }

    // pick_ray
    //   the world-space ray through the screen pixel (_sx, _sy), originating at
    // the eye. Built from the camera basis, so no matrix inverse is used.
    D_NODISCARD ray_type
    pick_ray(
        _T                  _sx,
        _T                  _sy,
        const viewport<_T>& _vp
    ) const noexcept
    {
        const _T ndc_x = static_cast<_T>(2) * _sx / _vp.m_width  - static_cast<_T>(1);
        const _T ndc_y = static_cast<_T>(1) - static_cast<_T>(2) * _sy / _vp.m_height;

        const _T tan_half = std::tan(m_fov_y / static_cast<_T>(2));

        const vec3_type f = forward();
        const vec3_type r = right();
        const vec3_type u = up();

        const vec3_type dir = r * (ndc_x * tan_half * m_aspect) +
                              u * (ndc_y * tan_half) +
                              f;

        // bridge linalg vectors to the ray's std::array point_type
        return ray_type(to_array(eye()), to_array(normalize(dir)));
    }
};

}  // namespace render3d
NS_END  // djinterp


#endif  // DJINTERP_3D_CAMERA_HPP
