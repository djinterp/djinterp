/*******************************************************************************
* djinterp [djinterp]                                                   lens.hpp
*
*   The lens contract: the abstract mapping from the n-dimensional state space
* onto a render-space `scene`.  Each lens mode (slice, projection, flatten,
* heatmap, parametric) is one implementation, supplied in a later phase; the
* mode is chosen at runtime, so the interface is virtual.  The shared core
* knows only this contract.
*
*
* path:      /inc/djinterp/ui/lens.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_LENS_HPP
#define DJINTERP_UI_LENS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
// djinterp
#include "../core/functional/result.hpp"  // result<T,E>, ok, err
#include "./types.hpp"
#include "./state.hpp"
#include "./config.hpp"
#include "./scene.hpp"


NS_DJINTERP
NS_UI

// lens_error
//   enum: why a lens could not produce a scene from the given state and
// configuration.
enum class lens_error
{
    no_axes,
    too_few_axes,
    too_many_axes,
    frame_out_of_range,
    unsupported_mode
};

// lens
//   class: the abstract mapping from the n-dimensional state space onto a
// render-space scene.  Concrete lenses implement `mode` and `build`; the
// returned `result` carries either the scene or the reason it could not be
// produced.
class lens
{
public:
    virtual ~lens() = default;

    D_NODISCARD virtual lens_mode mode() const = 0;

    D_NODISCARD virtual result<scene, lens_error>
    build(
        const state_source& _state,
        const config&       _config,
        std::size_t         _frame
    ) const = 0;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_LENS_HPP
