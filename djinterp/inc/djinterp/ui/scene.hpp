/*******************************************************************************
* djinterp [djinterp]                                                  scene.hpp
*
*   The scene intermediate representation (roadmap phase 1c): the shared,
* render-space description of a single frame that a lens produces and any
* renderer consumes.  Positions and bounds use the render3d vector types, but
* nothing here knows how a frame is viewed or drawn — no camera, projection,
* or UI-framework notion — just positioned cells, a styled path, an optional
* scalar field, and per-axis decorations.
*
*
* path:      /inc/djinterp/ui/scene.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_SCENE_HPP
#define DJINTERP_UI_SCENE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../core/functional/maybe.hpp"  // maybe<T>
#include "./types.hpp"                   // vec, aabb3, rgba
#include "./state.hpp"               // cell_address
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

// render_cell
//   struct: one shape resolved into render space, ready to draw — its LOD
// `level`, centre `position` (z is 0 in 2-D scenes), edge `size`, resolved
// `color` (opacity in the alpha channel), highlight flag, a back-reference to
// the `source` data cell, and that cell's metric `value` (empty when none).
struct render_cell
{
    re_std::uint32_t  level       = 0;
    vec<3, scalar> position;
    scalar         size        = 1.0;
    rgba           color;
    bool           highlighted = false;
    cell_address   source;
    maybe<scalar>  value;
};

// render_path_node
//   struct: one vertex of a drawn path in render space, with its resolved
// colour.
struct render_path_node
{
    vec<3, scalar> position;
    rgba           color;
};

// render_path
//   struct: an optimum path resolved into render space — its vertices plus the
// resolved stroke thickness and style.
struct render_path
{
    std::vector<render_path_node> nodes;
    scalar                        thickness = 1.0;
    line_style                    style     = line_style::solid;
};

// scalar_field
//   struct: a dense 2-D grid of scalar values (row-major, `width` by `height`)
// used by the heatmap lens; the renderer maps values to colour.  `min_value` /
// `max_value` bound the range for normalisation.
struct scalar_field
{
    re_std::uint32_t       width     = 0;
    re_std::uint32_t       height    = 0;
    std::vector<scalar> values;
    scalar              min_value = 0.0;
    scalar              max_value = 1.0;
};

// axis_decoration
//   struct: the rendered label, line, and tick marks for one axis, positioned
// in render space.
struct axis_decoration
{
    std::size_t              axis_index = 0;
    std::string              label;
    std::vector<scalar>      tick_positions;
    std::vector<std::string> tick_labels;
};

// scene
//   struct: the shared intermediate representation a lens produces and a
// renderer consumes — the complete, render-space description of one frame.
// `dimensions` is 1, 2, or 3; `bounds` is the axis-aligned extent of the
// rendered volume.
struct scene
{
    re_std::uint32_t                dimensions = 3;
    std::vector<render_cell>     cells;
    render_path                  path;
    maybe<scalar_field>          field;
    std::vector<axis_decoration> axes;
    aabb3<scalar>                bounds = aabb3<scalar>::empty();
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_SCENE_HPP
