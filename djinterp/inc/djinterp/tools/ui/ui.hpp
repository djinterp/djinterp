/*******************************************************************************
* djinterp [djinterp]                                                     ui.hpp
*
*   Umbrella header for the djinterp::ui visualization module.  Includes the
* phase 1 agnostic core (the shared types, the state / value model, the
* configuration model, the scene intermediate representation, the lens
* contract, and the top-level visualization handle), the coordinate-system
* bridge, the level-of-detail scheme, the concrete lenses built so far (slice,
* heatmap), the framework-agnostic 2D draw layer, the visit-counting state
* source, and SVG export.  The 3D rendering math is the render3d and math
* subframeworks', pulled in through types.hpp.  Backend bindings that depend on
* an external library (e.g. frontends/imgui.hpp) are deliberately not included
* here; include them only where that library is linked.  Later phases (the
* remaining lenses and the DSL) add their own headers alongside these.
*
*
* path:      /inc/djinterp/tools/ui/ui.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_TOOLS_UI_HPP
#define DJINTERP_TOOLS_UI_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp [ui]
#include "./types.hpp"                 // scalar, colours, the shared types
#include "./colormap.hpp"              // colour ramps over [0, 1]
#include "./state.hpp"                 // the state space and its sources
#include "./config.hpp"                // a visualization's configuration
#include "./scene.hpp"                 // the scene a lens produces
#include "./lod.hpp"                   // level of detail, focus and context
#include "./coordinate_mapper.hpp"     // coordinate systems at run time
#include "./lens.hpp"                  // the lens contract
#include "./lenses/slice.hpp"          // the slice lens
#include "./lenses/heatmap.hpp"        // the heatmap lens
#include "./draw.hpp"                  // project_scene, layout_field
#include "./visualization.hpp"         // the top-level handle
#include "./sources/visit_source.hpp"  // a source that counts visits
#include "./export/svg.hpp"            // a draw list as SVG

#endif  // floor, for now

#endif  // DJINTERP_TOOLS_UI_HPP
