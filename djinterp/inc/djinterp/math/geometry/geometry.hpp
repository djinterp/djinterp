/******************************************************************************
* djinterp [math]                                                geometry.hpp
*
* Umbrella header for the geometry subframework.
*   Includes every public geometry header. Users may also include the
* sub-headers directly when they want to keep compile times tight or
* depend only on a slice of the framework.
*
* DIRECTORY MAP:
*   geometry_common.hpp - structural interfaces, traits, AABB, quadrature
*   edge.hpp            - 1D primitives: line_segment, arc,
*                         parametric_edge, implicit_edge, polyline_edge,
*                         quadratic_bezier, cubic_bezier
*   surface.hpp         - 2D primitives: triangle_surface,
*                         planar_polygon_surface, bilinear_patch,
*                         parametric_surface_edge, implicit_surface
*   shape_2d.hpp        - 2D composers: polygon, parametric_region,
*                         implicit_region, shape_from_edges
*   named_2d.hpp        - circle, ellipse, rectangle, square, triangle,
*                         regular_polygon, annulus, circular_sector,
*                         circular_segment
*   star_2d.hpp         - star_polygon<N, K>, n_pointed_star<N>,
*                         pentagram, hexagram, hexagonal_star,
*                         heptagrams, octagram, enneagrams, decagram
*   solid.hpp        - 3D composers: triangle_mesh, parametric_volume,
*                         implicit_volume, shape_from_surfaces
*   named_3d.hpp        - sphere, ellipsoid, box, cube, cylinder, cone,
*                         frustum, torus, hemisphere, spherical_cap,
*                         regular_prism, regular_pyramid
*   star_3d.hpp         - stellate() face-erection, stella_octangula,
*                         four Kepler-Poinsot star polyhedra
*   measure_2d.hpp      - curve_length, perimeter, area (+ area_green),
*                         centroid, bounding_box helpers
*   measure_3d.hpp      - patch_area, volume (+ volume_divergence),
*                         surface_area, centroid_3d, bounding_box
*                         helpers
*
* 
* path:      /inc/djinterp/math/geometry/geometry.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/

#ifndef DJINTERP_MATH_GEOMETRY_
#define DJINTERP_MATH_GEOMETRY_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "../math.hpp"
#include "./geometry_common.hpp"
#include "./edge.hpp"
#include "./shape_2d.hpp"
#include "./named_2d.hpp"
#include "./star_2d.hpp"
#include "./measure_2d.hpp"
#include "./surface.hpp"
#include "./solid.hpp"
#include "./named_3d.hpp"
#include "./star_3d.hpp"
#include "./measure_3d.hpp"


#endif  // DJINTERP_MATH_GEOMETRY_