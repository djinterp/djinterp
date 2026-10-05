/*******************************************************************************
* djinterp [djinterp]                                                  types.hpp
*
*   Foundational, UI-framework-agnostic types shared across the djinterp::ui
* visualization module: the `scalar` value type, the `rgb`/`rgba` colour types
* (re-exported from the djinterp colour module), and the small enumerations
* describing coordinate systems, shape primitives, line styles, and lens modes.
*   The geometry types are aliases onto the math and render3d subframeworks --
* `vec`, `aabb3`, `camera`, `viewport` and `ray` -- so the module has no vector
* or camera code of its own.
*
*
* path:      /inc/djinterp/ui/types.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_TYPES_HPP
#define DJINTERP_UI_TYPES_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <array>    // std::array
#include <cstddef>  // std::size_t
// djinterp
#include "../djinterp.hpp"                       // framework root
#include "../core/util/color/color_rgb.hpp"      // rgb, rgba, lerp, channel_t
#include "../math/linear_algebra/vector.hpp"     // linalg::vector
#include "../math/linear_algebra/transform.hpp"  // linalg::to_array
#include "../math/geometry/geometry_common.hpp"  // math::aabb
#include "../math/geometry/ray.hpp"              // math::ray, intersect_aabb
#include "../render/camera.hpp"                      // render3d::camera, viewport


NS_DJINTERP
NS_UI

// scalar
//   type: the real-number value type used for coordinates, sizes, metrics,
// and every other continuous quantity throughout the visualization module.
using scalar = double;

// rgb / rgba
//   types: the colour types the visualization layer paints with, re-exported
// from the djinterp colour module.  `rgb` is the linear-RGB model; `rgba` is
// the straight-alpha transport colour, its alpha doubling as opacity.  Channel
// values are the module's `channel_t` (float), independent of `scalar`.  Note
// the module's `rgba` default-constructs to transparent black, so colours are
// always set explicitly rather than left to default.
using rgb  = ::djinterp::rgb;
using rgba = ::djinterp::rgba;

// vec
//   type: a fixed-size vector: the math subframework's linalg::vector, with
// the dimension first, as the ui spells it.
template<std::size_t N,
         typename    T = scalar>
using vec = ::djinterp::math::linalg::vector<T, N>;

// aabb3
//   type: a three-dimensional axis-aligned box from the math subframework.
// Start from aabb3<T>::empty() and include() points to bound a scene.
template<typename T = scalar>
using aabb3 = ::djinterp::math::aabb<3, T>;

// camera / viewport / ray
//   types: the orbit camera and render target of the render3d subframework,
// and the math subframework's ray, which the camera's pick_ray returns.
template<typename T = scalar>
using camera = ::djinterp::render3d::camera<T>;

template<typename T = scalar>
using viewport = ::djinterp::render3d::viewport<T>;

template<typename T = scalar>
using ray = ::djinterp::math::ray<T>;

// to_point
//   function: a vec<3> as the std::array point that math::aabb, math::ray and
// the coordinate systems take.
D_NODISCARD inline std::array<scalar, 3>
to_point(
    const vec<3, scalar>& _v
) noexcept
{
    return ::djinterp::math::linalg::to_array(_v);
}

// coordinate_system
//   enum: the spatial frame the grid of shapes is laid out in.
enum class coordinate_system
{
    cartesian,
    polar,
    spherical
};

// shape_kind
//   enum: the primitive drawn for each cell of the grid.
enum class shape_kind
{
    cube,
    tetrahedron,
    octahedron
};

// line_style
//   enum: the stroke pattern of a gridline or path segment.
enum class line_style
{
    solid,
    dashed,
    dotted
};

// lens_mode
//   enum: the mapping applied from the n-dimensional state space onto the
// (at most three) rendered spatial dimensions.
enum class lens_mode
{
    slice,
    projection,
    flatten,
    heatmap,
    parametric
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_TYPES_HPP
