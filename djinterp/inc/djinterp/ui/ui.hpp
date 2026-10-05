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
* path:      /inc/djinterp/ui/ui.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_UI_HPP
#define DJINTERP_UI_UI_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp [ui]
#include "./types.hpp"
#include "./colormap.hpp"
#include "./state.hpp"
#include "./config.hpp"
#include "./scene.hpp"
#include "./lod.hpp"
#include "./coordinate_mapper.hpp"
#include "./lens.hpp"
#include "./lenses/slice.hpp"
#include "./lenses/heatmap.hpp"
#include "./draw.hpp"
#include "./visualization.hpp"
#include "./sources/visit_source.hpp"
#include "./export/svg.hpp"

#endif  // floor, for now

#endif  // DJINTERP_UI_UI_HPP
