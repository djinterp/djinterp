/*******************************************************************************
* djinterp [render]                                                   render.hpp
*
*   Umbrella for the render3d subframework: an orbit camera, projective view
* transforms, a runtime render mesh, and box projection, all built on the math
* subframework.
* Including this header pulls in the render3d types plus the math pieces a 3D
* renderer works with -- vectors, matrices, transforms, quaternions, rays, and
* axis-aligned boxes -- so a frontend needs only this one include.
*
* LAYERING:
*   render3d depends on math and nothing else. It is deliberately free of any
* UI, windowing, or drawing-backend dependency; the scene-to-draw-list layer
* that consumes a camera lives above this, in the UI framework.
*
*
* path:      /inc/djinterp/render/render.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_RENDER_RENDER_HPP
#define DJINTERP_RENDER_RENDER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// math foundations this subframework builds on (re-exported for convenience)
#include "../math/linear_algebra/vector.hpp"
#include "../math/linear_algebra/matrix.hpp"
#include "../math/linear_algebra/transform.hpp"
#include "../math/linear_algebra/quaternion.hpp"
#include "../math/geometry/geometry_common.hpp"   // aabb
#include "../math/geometry/ray.hpp"

// render3d
#include "./projections.hpp"
#include "./camera.hpp"
#include "./mesh.hpp"
#include "./box.hpp"

#endif  // floor, for now


#endif  // DJINTERP_RENDER_RENDER_HPP
