/*******************************************************************************
* djinterp [djinterp]                                                 config.hpp
*
*   The configuration model (roadmap phase 1b): the declarative description of
* a visualization.  Every knob a frontend exposes maps to a field here, grouped
* into one sub-struct per concern and assembled into the top-level `config`.
* This is the single source of truth the interactive UI, the file exporter, and
* the DSL all drive.
*
*
* path:      /inc/djinterp/ui/config.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_CONFIG_HPP
#define DJINTERP_UI_CONFIG_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <vector>
// djinterp
#include "./colormap.hpp"  // colormap
#include "./types.hpp"     // scalar, rgba, enums
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

// layout_config
//   struct: how the grid of shapes is laid out and subdivided.  `spacing` is
// the gap between adjacent shapes; `shapes_per_level` is the per-level
// branching factor (one entry per level, empty = a uniform default chosen by
// the geometry layer); `visible_levels` is the focus-plus-context depth — how
// many LOD levels are rendered at once around the focus.
struct layout_config
{
    coordinate_system          coords         = coordinate_system::cartesian;
    shape_kind                 shape          = shape_kind::cube;
    scalar                     spacing        = 0.0;
    re_std::uint32_t              visible_levels = 1;
    std::vector<re_std::uint32_t> shapes_per_level;
};

// shape_style
//   struct: the appearance of each shape.  `color.a` is the shape's opacity;
// `filled` adds translucent faces behind the wireframe (off keeps a pure
// wireframe).
struct shape_style
{
    rgba color  = rgba{ 0.50f, 0.60f, 0.90f, 0.35f };
    bool filled = false;
};

// gridline_config
//   struct: the appearance of the grid lines.  `color.a` is their opacity;
// `fade_distance` is the depth over which lines fade out (0 disables fading).
struct gridline_config
{
    bool       enabled       = true;
    scalar     thickness     = 1.0;
    line_style style         = line_style::solid;
    rgba       color         = rgba{ 0.60f, 0.70f, 0.90f, 0.50f };
    scalar     fade_distance = 0.0;
};

// line_config
//   struct: the appearance of a drawn path.  `color.a` is its opacity.
struct line_config
{
    rgba       color     = rgba{ 0.90f, 0.20f, 0.20f, 1.0f };
    scalar     thickness = 2.0;
    line_style style     = line_style::solid;
};

// axis_display_config
//   struct: which per-axis decorations are drawn, and whether discrete axes
// may be displayed.
struct axis_display_config
{
    bool show_labels    = true;
    bool show_lines     = true;
    bool show_scales    = true;
    bool allow_discrete = true;
};

// lens_config
//   struct: the active lens mode and the axes it maps onto the rendered
// dimensions (for slice / projection / heatmap, typically up to three).
struct lens_config
{
    lens_mode                mode = lens_mode::slice;
    std::vector<std::size_t> axes;
};

// interaction_config
//   struct: hover / picking behaviour and the colours that mark cells -- the
// `highlight_color` for a hovered (or lens-highlighted) cell, and the distinct
// `selection_color` for the clicked, persistently selected cell.
struct interaction_config
{
    bool hover_enabled   = true;
    rgba highlight_color = rgba{ 1.0f, 0.80f, 0.10f, 0.80f };
    rgba selection_color = rgba{ 0.25f, 0.95f, 0.45f, 0.90f };
};

// time_config
//   struct: whether the time axis is active and the set of playback speeds
// offered (as multipliers of real time).
struct time_config
{
    bool                enabled = false;
    std::vector<scalar> speeds  = { 0.25, 0.5, 1.0, 2.0, 4.0 };
};

// config
//   struct: the complete, declarative description of a visualization.  Every
// knob a frontend exposes maps to a field here; this is the single source of
// truth that the interactive UI, the file exporter, and the DSL all drive.
// `colors` maps normalised metrics to colour for cells, fields, and paths.
struct config
{
    ui::colormap        colors = ui::colormap::cool_warm();
    layout_config       layout;
    shape_style         shape;
    gridline_config     gridlines;
    line_config         line;
    axis_display_config axes;
    lens_config         lens;
    interaction_config  interaction;
    time_config         time;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_CONFIG_HPP
