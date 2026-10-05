/*******************************************************************************
* djinterp [djinterp]                                                  slice.hpp
*
*   The slice lens (lens_mode::slice): the structural mapping from the state
* space onto a scene.  It lays up to three axes out spatially, holds the rest
* fixed (the "slice"), and draws the frame's optimum path heat-coloured by each
* node's metric.  When the configuration asks for more than one visible level
* the cells are the nested, focus-plus-context layout from lod.hpp (coarse
* context everywhere, fine detail around the optimum); otherwise a single flat
* grid.  Cells are structural — colouring the value landscape is the heatmap
* lens's role.
*
*   Grid cells, LOD cells, and path vertices all pass through the one placement
* rule and the same coordinate_mapper, so they share a render space.
*
*   Fixed (non-displayed) axes are held at index 0 until the configuration grows
* a per-axis slice selection.
*
*
* path:      /inc/djinterp/ui/lenses/slice.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_LENSES_SLICE_HPP
#define DJINTERP_UI_LENSES_SLICE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <memory>
#include <vector>
// djinterp [ui]
#include "../lens.hpp"               // lens, lens_error, result, ok, err
#include "../coordinate_mapper.hpp"  // coordinate_mapper and its factory
#include "../lod.hpp"                // lod_scheme, generate_lod_cells
#include "./common.hpp"              // shared lens helpers (internal::)
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

// slice_lens
//   class: maps the state space onto a scene as a (optionally nested) grid of
// structural cells with the optimum path drawn through it.  Stateless; one
// instance may serve any number of build() calls.
class slice_lens final : public lens
{
    public:

        D_NODISCARD lens_mode
        mode() const override
        {
            return lens_mode::slice;
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

            // data-resolution divisions per display axis
            // (decorations, flat grid)
            std::vector<std::size_t> divs;
            divs.reserve(display.size());

            for (std::size_t d = 0; d < display.size(); ++d)
            {
                divs.push_back(
                    internal::axis_divisions(_state.axis_at(display[d]),
                                             default_divisions));
            }

            scene out;
            out.dimensions = static_cast<re_std::uint32_t>(display.size());

            // ---- cells ------------------------------------------------------

            if (_config.layout.visible_levels > 1)
            {
                build_nested_cells(out, _state, _config, _frame,
                                   *mapper, display, axis_n);
            }
            else
            {
                build_flat_cells(out, _state, _config, _frame, *mapper,
                                 display, divs, spacing, axis_n);
            }

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
                for (std::size_t d = 0; d < display.size(); ++d)
                {
                    out.axes.push_back(
                        internal::make_decoration(_state.axis_at(display[d]),
                                                  display[d],
                                                  divs[d],
                                                  spacing,
                                                  _config));
                }
            }

            return ok<scene, lens_error>(static_cast<scene&&>(out));
        }

    private:

        static constexpr std::size_t max_display_axes  = 3;
        static constexpr std::size_t default_divisions = 8;
        static constexpr std::size_t focus_window      = 1;

        // build_flat_cells
        //   emits a single-level grid: one cell per combination of the display
        // axes' indices, with the rest of the space held at index 0.
        static void
        build_flat_cells(
            scene&                          _out,
            const state_source&             _state,
            const config&                   _config,
            std::size_t                     _frame,
            const coordinate_mapper&        _mapper,
            const std::vector<std::size_t>& _display,
            const std::vector<std::size_t>& _divs,
            scalar                          _spacing,
            std::size_t                     _axis_count
        )
        {
            std::size_t total = 1;

            for (std::size_t d = 0; d < _divs.size(); ++d)
            {
                total *= _divs[d];
            }

            for (std::size_t n = 0; n < total; ++n)
            {
                std::vector<std::size_t> grid_index(_display.size());
                std::size_t              rem = n;

                for (std::size_t d = 0; d < _display.size(); ++d)
                {
                    grid_index[d] = rem % _divs[d];
                    rem          /= _divs[d];
                }

                vec<3, scalar> coords;

                for (std::size_t d = 0; (d < _display.size()) && (d < 3); ++d)
                {
                    const axis& ax = _state.axis_at(_display[d]);

                    coords[d] = internal::place(
                        ax,
                        internal::grid_value(ax, grid_index[d], _divs[d]),
                        _spacing);
                }

                vec<3, scalar> pos = _mapper.to_cartesian(coords);

                render_cell cell;

                cell.level        = 0;
                cell.position     = pos;
                cell.size         = static_cast<scalar>(1);
                cell.color        = _config.shape.color;
                cell.highlighted  = false;
                cell.source.level = 0;
                cell.source.indices.assign(_axis_count, 0);

                for (std::size_t d = 0; d < _display.size(); ++d)
                {
                    cell.source.indices[_display[d]] = grid_index[d];
                }

                cell.value = _state.value_at(cell.source, _frame);

                _out.bounds.include(to_point(pos));
                _out.cells.push_back(static_cast<render_cell&&>(cell));
            }

            return;
        }

        // build_nested_cells
        //   emits the focus-plus-context LOD layout, focused on the optimum
        // path's current endpoint.
        static void
        build_nested_cells(
            scene&                          _out,
            const state_source&             _state,
            const config&                   _config,
            std::size_t                     _frame,
            const coordinate_mapper&        _mapper,
            const std::vector<std::size_t>& _display,
            std::size_t                     _axis_count
        )
        {
            std::vector<scalar> focus =
                compute_focus(_state.current_path(_frame), _display, _state);

            lod_scheme scheme;

            scheme.base_resolution   = 1;
            scheme.default_branching = 2;
            scheme.branching         = _config.layout.shapes_per_level;

            std::vector<lod_cell> cells =
                generate_lod_cells(_display.size(),
                                   focus,
                                   scheme,
                                   _config.layout.visible_levels,
                                   focus_window);

            internal::build_lod_render_cells(_out, cells, _display, _state,
                                             _mapper, _config, _frame,
                                             _axis_count);

            return;
        }

        // compute_focus
        //   the focus position in normalised [0, 1] per display axis, taken
        // from the optimum path's last vertex (the centre when there is none).
        D_NODISCARD static std::vector<scalar>
        compute_focus(
            const path&                     _path,
            const std::vector<std::size_t>& _display,
            const state_source&             _state
        )
        {
            std::vector<scalar> focus(_display.size(),
                                      static_cast<scalar>(0.5));

            if (_path.nodes.empty())
            {
                return focus;
            }

            const path_node& node = _path.nodes.back();

            for (std::size_t d = 0; d < _display.size(); ++d)
            {
                std::size_t ax = _display[d];

                if (ax < node.coords.size())
                {
                    focus[d] = internal::normalize_coord(_state.axis_at(ax),
                                                         node.coords[ax]);
                }
            }

            return focus;
        }
};

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_LENSES_SLICE_HPP
