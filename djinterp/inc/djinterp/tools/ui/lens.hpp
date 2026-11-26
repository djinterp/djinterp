/*******************************************************************************
* djinterp [cpp][tools]                                                 lens.hpp
*
*   The lens contract: the abstract mapping from the n-dimensional state space
* onto a render-space `scene`.  Each lens mode (slice, projection, flatten,
* heatmap, parametric) is one implementation, supplied in a later phase; the
* mode is chosen at runtime, so the interface is virtual.  The shared core
* knows only this contract.
*
* HOW TO USE
* ----------
*   A lens maps a state_source onto a scene for one frame, under a config:
* build(state, config, frame) gives the scene, or a lens_error: no_axes,
* too_few_axes or too_many_axes for the display axes asked, frame_out_of_range,
* or unsupported_mode. mode() says which lens_mode it implements. Use slice_lens
* or heatmap_lens, or derive a new one; a visualization holds one and swaps it
* at run time.
*
* OPTIONS
* -------
*   None of its own: a lens reads the config it is given.
*
* CONFIGURATION
* -------------
*   None of its own: no knob or macro changes it. Below C++17 the file is empty
* rather than an error (D_ENV_LANG_IS_CPP17_OR_HIGHER), as every header of
* djinterp::ui's is, though the module's own floor is C++11: the FLOOR note
* below says why.
*
*
* path:      /inc/djinterp/tools/ui/lens.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_TOOLS_UI_LENS_HPP
#define DJINTERP_TOOLS_UI_LENS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
// djinterp
#include "../../cpp/functional/result.hpp"  // result<T,E>, ok, err
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
    virtual
    ~lens() = default;

    D_NODISCARD virtual lens_mode mode() const = 0;

    D_NODISCARD virtual result<scene, lens_error> build(
        const state_source& _state,
        const config&       _config,
        std::size_t         _frame
    ) const = 0;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TOOLS_UI_LENS_HPP
