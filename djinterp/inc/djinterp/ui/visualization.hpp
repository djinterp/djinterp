/*******************************************************************************
* djinterp [djinterp]                                          visualization.hpp
*
*   The top-level, UI-framework-agnostic handle (roadmap phase 1d): binds a
* `config`, a `state_source`, and a `lens` into one renderable object.  Every
* frontend — the interactive ImGui binding, the file exporter, the DSL — reads
* its configuration, swaps its lens, and asks it to build the scene for a
* frame.  It owns its lens but only references its state source.
*
*
* path:      /inc/djinterp/ui/visualization.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_VISUALIZATION_HPP
#define DJINTERP_UI_VISUALIZATION_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <memory>
// djinterp
#include "../core/functional/result.hpp"  // result<T,E>, err
#include "./types.hpp"
#include "./state.hpp"
#include "./config.hpp"
#include "./scene.hpp"
#include "./lens.hpp"


NS_DJINTERP
NS_UI

// visualization
//   class: the top-level handle that binds a configuration, a state source,
// and a lens into one renderable object.  Frontends operate on this alone:
// they read and edit its configuration, swap its lens when the user changes
// mode, and ask it to build the scene for a given frame.
class visualization
{
public:
    visualization(
        config                _config,
        const state_source&   _state,
        std::unique_ptr<lens> _lens
    )
        : m_config(static_cast<config&&>(_config)),
          m_state(&_state),
          m_lens(static_cast<std::unique_ptr<lens>&&>(_lens))
    {}

    D_NODISCARD config&
    configuration()
    {
        return m_config;
    }

    D_NODISCARD const config&
    configuration() const
    {
        return m_config;
    }

    D_NODISCARD const state_source&
    state() const
    {
        return *m_state;
    }

    void
    set_lens(
        std::unique_ptr<lens> _lens
    )
    {
        m_lens = static_cast<std::unique_ptr<lens>&&>(_lens);

        return;
    }

    D_NODISCARD result<scene, lens_error>
    build_scene(
        std::size_t _frame
    ) const
    {
        // a visualization with no lens cannot build a scene
        if (!m_lens)
        {
            return err<scene, lens_error>(lens_error::unsupported_mode);
        }

        return m_lens->build(*m_state, m_config, _frame);
    }

private:
    config                m_config;
    const state_source*   m_state;
    std::unique_ptr<lens> m_lens;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_VISUALIZATION_HPP
