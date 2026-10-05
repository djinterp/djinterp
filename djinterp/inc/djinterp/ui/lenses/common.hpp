/*******************************************************************************
* djinterp [djinterp]                                                 common.hpp
*
*   Shared building blocks for the concrete lenses, factored out so each lens
* expresses only what makes it distinct.  Covers display-axis resolution, the
* placement rule that maps axis coordinates into render space (and its
* normalised inverse), path projection, axis decorations, and the bridge that
* turns LOD cells (lod.hpp) into render cells.  Colour comes from color.hpp.
* All helpers live in the internal namespace.
*
*
* path:      /inc/djinterp/ui/lenses/common.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_LENSES_COMMON_HPP
#define DJINTERP_UI_LENSES_COMMON_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp [ui]
#include "../state.hpp"
#include "../config.hpp"
#include "../scene.hpp"
#include "../colormap.hpp"
#include "../coordinate_mapper.hpp"
#include "../lod.hpp"
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // fixed-width integers


NS_DJINTERP
NS_UI

NS_INTERNAL

    // resolve_display_axes
    //   helper: the axes laid out spatially.  An explicit, in-range request is
    // honoured (deduped, capped at _max_axes); an empty request defaults to the
    // first axes of the space.
    D_NODISCARD inline std::vector<std::size_t>
    resolve_display_axes
    (
        const std::vector<std::size_t>& _requested,
        std::size_t                     _axis_count,
        std::size_t                     _max_axes
    )
    {
        std::vector<std::size_t> out;

        if (_requested.empty())
        {
            std::size_t take = (_axis_count < _max_axes)
                                   ? _axis_count
                                   : _max_axes;

            for (std::size_t i = 0; i < take; ++i)
            {
                out.push_back(i);
            }

            return out;
        }

        for (std::size_t i = 0; i < _requested.size(); ++i)
        {
            std::size_t a = _requested[i];

            if (a >= _axis_count)
            {
                continue;
            }

            bool seen = false;

            for (std::size_t j = 0; j < out.size(); ++j)
            {
                if (out[j] == a)
                {
                    seen = true;
                }
            }

            if (!seen && (out.size() < _max_axes))
            {
                out.push_back(a);
            }
        }

        return out;
    }

    // axis_divisions
    //   helper: the number of cells along an axis — its state count when
    // discrete, a fixed sampling resolution when continuous.
    D_NODISCARD inline std::size_t
    axis_divisions
    (
        const axis& _axis,
        std::size_t _default_divisions
    )
    {
        if (_axis.kind == axis_kind::discrete)
        {
            std::size_t n = _axis.count();

            return (n == 0) ? 1 : n;
        }

        return _default_divisions;
    }

    // grid_value
    //   helper: the axis-space coordinate of grid index _index.  Discrete axes
    // use the index; continuous axes sample [min, max] evenly.
    D_NODISCARD inline scalar
    grid_value
    (
        const axis& _axis,
        std::size_t _index,
        std::size_t _divisions
    )
    {
        if (_axis.kind == axis_kind::discrete)
        {
            return static_cast<scalar>(_index);
        }

        if (_divisions <= 1)
        {
            return _axis.min;
        }

        scalar t = static_cast<scalar>(_index) /
                   static_cast<scalar>(_divisions - 1);

        return _axis.min + (_axis.max - _axis.min) * t;
    }

    // place
    //   helper: maps an axis-space coordinate to a render-space coordinate
    // along that axis.  Discrete axes are spread by (1 + spacing); continuous
    // coordinates are used directly.  Applied to grid cells and path vertices
    // alike, which keeps them registered.
    D_NODISCARD inline scalar
    place
    (
        const axis& _axis,
        scalar      _value,
        scalar      _spacing
    )
    {
        if (_axis.kind == axis_kind::discrete)
        {
            return _value * (static_cast<scalar>(1) + _spacing);
        }

        return _value;
    }

    // denormalize
    //   helper: maps a normalised position _p in [0, 1] along an axis to a
    // render-space coordinate, matching the placed extent of the flat grid.
    D_NODISCARD inline scalar
    denormalize
    (
        const axis& _axis,
        scalar      _p,
        scalar      _spacing
    )
    {
        if (_axis.kind == axis_kind::discrete)
        {
            std::size_t n = _axis.count();

            if (n <= 1)
            {
                return static_cast<scalar>(0);
            }

            return _p * static_cast<scalar>(n - 1) *
                   (static_cast<scalar>(1) + _spacing);
        }

        return _axis.min + _p * (_axis.max - _axis.min);
    }

    // normalize_coord
    //   helper: maps an axis-space coordinate to a normalised position in
    // [0, 1] (the inverse direction of the grid sampling, used to locate a
    // focus point).
    D_NODISCARD inline scalar
    normalize_coord
    (
        const axis& _axis,
        scalar      _coord
    )
    {
        scalar p;

        if (_axis.kind == axis_kind::discrete)
        {
            std::size_t n = _axis.count();

            p = (n <= 1)
                    ? static_cast<scalar>(0)
                    : _coord / static_cast<scalar>(n - 1);
        }
        else
        {
            scalar span = _axis.max - _axis.min;

            p = (span == static_cast<scalar>(0))
                    ? static_cast<scalar>(0)
                    : (_coord - _axis.min) / span;
        }

        if (p < static_cast<scalar>(0))
        {
            p = static_cast<scalar>(0);
        }

        if (p > static_cast<scalar>(1))
        {
            p = static_cast<scalar>(1);
        }

        return p;
    }

    // format_scalar
    //   helper: a plain string form of a coordinate for a tick label.
    D_NODISCARD inline std::string
    format_scalar
    (
        scalar _value
    )
    {
        return std::to_string(_value);
    }

    // build_path
    //   helper: projects the frame's optimum path into the scene's render space
    // and heat-colours each vertex by its metric, growing the scene bounds to
    // include the path.
    inline void
    build_path
    (
        scene&                          _out,
        const path&                     _path,
        const std::vector<std::size_t>& _display,
        const coordinate_mapper&        _mapper,
        const config&                   _config,
        scalar                          _spacing,
        const state_source&             _state
    )
    {
        bool   any  = false;
        scalar vmin = static_cast<scalar>(0);
        scalar vmax = static_cast<scalar>(0);

        for (std::size_t i = 0; i < _path.nodes.size(); ++i)
        {
            if (!_path.nodes[i].value.has_value())
            {
                continue;
            }

            scalar v = _path.nodes[i].value.value();

            if (!any)
            {
                vmin = v;
                vmax = v;
                any  = true;
            }
            else
            {
                if (v < vmin)
                {
                    vmin = v;
                }

                if (v > vmax)
                {
                    vmax = v;
                }
            }
        }

        scalar span = vmax - vmin;

        for (std::size_t i = 0; i < _path.nodes.size(); ++i)
        {
            const path_node& node = _path.nodes[i];

            vec<3, scalar> coords;

            for (std::size_t d = 0; (d < _display.size()) && (d < 3); ++d)
            {
                std::size_t ax = _display[d];
                const axis& a  = _state.axis_at(ax);

                scalar cv = (ax < node.coords.size())
                                ? node.coords[ax]
                                : static_cast<scalar>(0);

                coords[d] = place(a, cv, _spacing);
            }

            vec<3, scalar> pos = _mapper.to_cartesian(coords);

            render_path_node rn;

            rn.position = pos;

            if (node.value.has_value() && any &&
                (span > static_cast<scalar>(0)))
            {
                scalar t = (node.value.value() - vmin) / span;

                rn.color = _config.colors.sample(t, _config.line.color.a);
            }
            else if (node.value.has_value() && any)
            {
                rn.color = _config.colors.sample(static_cast<scalar>(0.5),
                                                 _config.line.color.a);
            }
            else
            {
                rn.color = _config.line.color;
            }

            _out.bounds.include(to_point(pos));
            _out.path.nodes.push_back(static_cast<render_path_node&&>(rn));
        }

        _out.path.thickness = _config.line.thickness;
        _out.path.style     = _config.line.style;

        return;
    }

    // make_decoration
    //   helper: builds the label and ticks for one displayed axis.
    D_NODISCARD inline axis_decoration
    make_decoration
    (
        const axis&   _axis,
        std::size_t   _axis_index,
        std::size_t   _divisions,
        scalar        _spacing,
        const config& _config
    )
    {
        axis_decoration dec;

        dec.axis_index = _axis_index;

        if (_config.axes.show_labels)
        {
            dec.label = _axis.name;
        }

        if (_config.axes.show_scales)
        {
            for (std::size_t i = 0; i < _divisions; ++i)
            {
                scalar v = grid_value(_axis, i, _divisions);

                dec.tick_positions.push_back(place(_axis, v, _spacing));

                if ((_axis.kind == axis_kind::discrete) &&
                    (i < _axis.labels.size()))
                {
                    dec.tick_labels.push_back(_axis.labels[i]);
                }
                else
                {
                    dec.tick_labels.push_back(format_scalar(v));
                }
            }
        }

        return dec;
    }

    // build_lod_render_cells
    //   helper: turns LOD cells (normalised) into render cells.  Each cell's
    // normalised centre is denormalised per axis and mapped into render space;
    // its edge is scaled by the cell's fractional size against the first
    // displayed axis's extent.  The scene bounds are grown to the full
    // displayed extent so the camera frames the whole space, context included.
    inline void
    build_lod_render_cells
    (
        scene&                          _out,
        const std::vector<lod_cell>&    _cells,
        const std::vector<std::size_t>& _display,
        const state_source&             _state,
        const coordinate_mapper&        _mapper,
        const config&                   _config,
        std::size_t                     _frame,
        std::size_t                     _axis_count
    )
    {
        scalar      spacing = _config.layout.spacing;
        std::size_t dim     = _display.size();

        // reference edge for cube sizing: the placed extent of the first axis
        scalar ref_extent = static_cast<scalar>(1);

        if (dim > 0)
        {
            const axis& a0 = _state.axis_at(_display[0]);

            scalar lo = denormalize(a0, static_cast<scalar>(0), spacing);
            scalar hi = denormalize(a0, static_cast<scalar>(1), spacing);

            ref_extent = hi - lo;

            if (ref_extent <= static_cast<scalar>(0))
            {
                ref_extent = static_cast<scalar>(1);
            }
        }

        // grow bounds to the full displayed extent (every corner of [0,1]^dim)
        std::size_t corners = (static_cast<std::size_t>(1) << dim);

        for (std::size_t m = 0; m < corners; ++m)
        {
            vec<3, scalar> cc;

            for (std::size_t d = 0; (d < dim) && (d < 3); ++d)
            {
                scalar p = ((m >> d) & 1u) ? static_cast<scalar>(1)
                                           : static_cast<scalar>(0);

                cc[d] = denormalize(_state.axis_at(_display[d]), p, spacing);
            }

            _out.bounds.include(to_point(_mapper.to_cartesian(cc)));
        }

        // place each cell
        for (std::size_t c = 0; c < _cells.size(); ++c)
        {
            const lod_cell& lc = _cells[c];

            vec<3, scalar> coords;

            for (std::size_t d = 0; (d < dim) && (d < 3); ++d)
            {
                const axis& a = _state.axis_at(_display[d]);

                scalar cn = (d < lc.center.size())
                                ? lc.center[d]
                                : static_cast<scalar>(0.5);

                coords[d] = denormalize(a, cn, spacing);
            }

            vec<3, scalar> pos = _mapper.to_cartesian(coords);

            render_cell cell;

            cell.level        = lc.level;
            cell.position     = pos;
            cell.size         = lc.size * ref_extent;
            cell.color        = _config.shape.color;
            cell.highlighted  = false;
            cell.source.level = lc.level;
            cell.source.indices.assign(_axis_count, 0);

            for (std::size_t d = 0;
                 (d < dim) && (d < lc.indices.size());
                 ++d)
            {
                cell.source.indices[_display[d]] = lc.indices[d];
            }

            cell.value = _state.value_at(cell.source, _frame);

            _out.cells.push_back(static_cast<render_cell&&>(cell));
        }

        return;
    }

NS_END  // internal

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_LENSES_COMMON_HPP
