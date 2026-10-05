/*******************************************************************************
* djinterp [djinterp]                                                heatmap.hpp
*
*   The heatmap lens (lens_mode::heatmap): the value mapping from the state
* space onto a scene.  It samples the cell metric over a grid of up to two axes
* into a dense scalar_field (which an image renderer draws directly) and also
* emits one value-coloured cell per present sample (which a 3D renderer draws as
* a coloured plane), then draws the optimum path.  Where the slice lens shows
* structure, the heatmap lens shows the value landscape.
*
*   Absent samples are left as NaN in the field and produce no cell.  Cells and
* the field share the metric range, so their colourings agree.
*
*
* path:      /inc/djinterp/ui/lenses/heatmap.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_LENSES_HEATMAP_HPP
#define DJINTERP_UI_LENSES_HEATMAP_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>
// djinterp [ui]
#include "../lens.hpp"               // lens, lens_error, result, ok, err
#include "../coordinate_mapper.hpp"  // coordinate_mapper and its factory
#include "./common.hpp"              // shared lens helpers (internal::)
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

// heatmap_lens
//   class: maps the state space onto a scene by sampling the cell metric over
// up to two axes, producing a dense scalar_field and value-coloured cells.
// Stateless; one instance may serve any number of build() calls.
class heatmap_lens final : public lens
{
    public:

        D_NODISCARD lens_mode
        mode() const override
        {
            return lens_mode::heatmap;
        }

        D_NODISCARD result<scene, lens_error>
        build(
            const state_source& _state,
            const config&       _config,
            std::size_t         _frame
        ) const override
        {
            std::size_t axis_n = _state.axis_count();

            // ---- validate ---------------------------------------------------

            if (axis_n == 0)
            {
                return err<scene, lens_error>(lens_error::no_axes);
            }

            if (_frame >= _state.frame_count())
            {
                return err<scene, lens_error>(lens_error::frame_out_of_range);
            }

            if (_config.lens.axes.size() > max_display_axes)
            {
                return err<scene, lens_error>(lens_error::too_many_axes);
            }

            std::vector<std::size_t> display =
                internal::resolve_display_axes(_config.lens.axes,
                                               axis_n,
                                               max_display_axes);

            if (display.empty())
            {
                return err<scene, lens_error>(lens_error::too_few_axes);
            }

            // ---- set up -----------------------------------------------------

            std::unique_ptr<coordinate_mapper> mapper =
                make_coordinate_mapper(_config.layout.coords);

            scalar spacing = _config.layout.spacing;

            bool        two_d  = (display.size() >= 2);
            std::size_t w_axis = display[0];
            std::size_t h_axis = two_d ? display[1] : display[0];

            std::size_t width  =
                internal::axis_divisions(_state.axis_at(w_axis),
                                         default_divisions);
            std::size_t height = two_d
                ? internal::axis_divisions(_state.axis_at(h_axis),
                                           default_divisions)
                : static_cast<std::size_t>(1);

            scene out;
            out.dimensions = static_cast<re_std::uint32_t>(display.size());

            // ---- sample the field -------------------------------------------

            scalar_field field;

            field.width  = static_cast<re_std::uint32_t>(width);
            field.height = static_cast<re_std::uint32_t>(height);

            scalar nan = std::numeric_limits<scalar>::quiet_NaN();

            field.values.assign(width * height, nan);

            bool   any  = false;
            scalar vmin = static_cast<scalar>(0);
            scalar vmax = static_cast<scalar>(0);

            for (std::size_t j = 0; j < height; ++j)
            {
                for (std::size_t i = 0; i < width; ++i)
                {
                    cell_address addr;

                    addr.level = 0;
                    addr.indices.assign(axis_n, 0);
                    addr.indices[w_axis] = i;

                    if (two_d)
                    {
                        addr.indices[h_axis] = j;
                    }

                    maybe<scalar> v = _state.value_at(addr, _frame);

                    if (!v.has_value())
                    {
                        continue;
                    }

                    scalar val = v.value();

                    field.values[j * width + i] = val;

                    if (!any)
                    {
                        vmin = val;
                        vmax = val;
                        any  = true;
                    }
                    else
                    {
                        if (val < vmin)
                        {
                            vmin = val;
                        }

                        if (val > vmax)
                        {
                            vmax = val;
                        }
                    }
                }
            }

            field.min_value = any ? vmin : static_cast<scalar>(0);
            field.max_value = any ? vmax : static_cast<scalar>(1);

            scalar span = field.max_value - field.min_value;

            // ---- value-coloured cells ---------------------------------------

            for (std::size_t j = 0; j < height; ++j)
            {
                for (std::size_t i = 0; i < width; ++i)
                {
                    scalar val = field.values[j * width + i];

                    // NaN marks an absent sample: no cell
                    if (val != val)
                    {
                        continue;
                    }

                    vec<3, scalar> coords;

                    const axis& wa = _state.axis_at(w_axis);

                    coords[0] = internal::place(
                        wa,
                        internal::grid_value(wa, i, width),
                        spacing);

                    if (two_d)
                    {
                        const axis& ha = _state.axis_at(h_axis);

                        coords[1] = internal::place(
                            ha,
                            internal::grid_value(ha, j, height),
                            spacing);
                    }

                    vec<3, scalar> pos = mapper->to_cartesian(coords);

                    scalar t = (span > static_cast<scalar>(0))
                                   ? (val - field.min_value) / span
                                   : static_cast<scalar>(0.5);

                    render_cell cell;

                    cell.level        = 0;
                    cell.position     = pos;
                    cell.size         = static_cast<scalar>(1);
                    cell.color        =
                        _config.colors.sample(t, _config.shape.color.a);
                    cell.highlighted  = false;
                    cell.value        = just(val);
                    cell.source.level = 0;
                    cell.source.indices.assign(axis_n, 0);
                    cell.source.indices[w_axis] = i;

                    if (two_d)
                    {
                        cell.source.indices[h_axis] = j;
                    }

                    out.bounds.include(to_point(pos));
                    out.cells.push_back(static_cast<render_cell&&>(cell));
                }
            }

            out.field = just(static_cast<scalar_field&&>(field));

            // ---- path -------------------------------------------------------

            internal::build_path(out,
                                 _state.current_path(_frame),
                                 display,
                                 *mapper,
                                 _config,
                                 spacing,
                                 _state);

            // ---- axis decorations -------------------------------------------

            if (_config.axes.show_labels || _config.axes.show_scales)
            {
                out.axes.push_back(
                    internal::make_decoration(_state.axis_at(w_axis),
                                              w_axis, width, spacing, _config));

                if (two_d)
                {
                    out.axes.push_back(
                        internal::make_decoration(_state.axis_at(h_axis),
                                                  h_axis, height, spacing,
                                                  _config));
                }
            }

            return ok<scene, lens_error>(static_cast<scene&&>(out));
        }

    private:

        static constexpr std::size_t max_display_axes  = 2;
        static constexpr std::size_t default_divisions = 8;
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_LENSES_HEATMAP_HPP
