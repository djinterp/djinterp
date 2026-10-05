/*******************************************************************************
* djinterp [djinterp]                                                    lod.hpp
*
*   The level-of-detail scheme and the focus-plus-context cell generator that
* produces the "nested" multi-resolution layout: a coarse level covering the
* whole space, then progressively finer levels concentrated around a focus
* point.  Everything here is in normalised [0, 1] axis space and depends only
* on `scalar` — it knows nothing about coordinate systems, the scene, or the
* renderer, so a lens turns these cells into render geometry separately.
*
*
* path:      /inc/djinterp/ui/lod.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_LOD_HPP
#define DJINTERP_UI_LOD_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <vector>
// djinterp [ui]
#include "./types.hpp"      // scalar, NS_UI
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

// lod_scheme
//   struct: how each level subdivides.  divisions_at(level) is the number of
// cells along each axis at that level: `base_resolution` at level 0, multiplied
// by the per-level branching factor at each deeper level.  `branching` supplies
// the factor for each level; `default_branching` covers levels beyond it.
struct lod_scheme
{
    // ---- data ---------------------------------------------------------------

    std::size_t                base_resolution   = 1;
    std::size_t                default_branching = 2;
    std::vector<re_std::uint32_t> branching;

    // ---- queries ------------------------------------------------------------

    // branching_at
    //   the subdivision factor applied to reach the level below _level.
    D_NODISCARD std::size_t
    branching_at(
        std::size_t _level
    ) const
    {
        if (_level < branching.size())
        {
            std::size_t b = static_cast<std::size_t>(branching[_level]);

            return (b == 0) ? default_branching : b;
        }

        return default_branching;
    }

    // divisions_at
    //   the number of cells along each axis at _level.
    D_NODISCARD std::size_t
    divisions_at(
        std::size_t _level
    ) const
    {
        std::size_t d = (base_resolution == 0) ? 1 : base_resolution;

        for (std::size_t k = 0; k < _level; ++k)
        {
            d *= branching_at(k);
        }

        return d;
    }
};

// lod_cell
//   struct: one cell of the multi-resolution layout in normalised axis space.
// `level` and `indices` address it within its level; `center` is its centre in
// [0, 1] per displayed axis; `size` is its fractional edge (1 / divisions).
struct lod_cell
{
    re_std::uint32_t            level = 0;
    std::vector<std::size_t> indices;
    std::vector<scalar>      center;
    scalar                   size = 1.0;
};

// generate_lod_cells
//   builds the focus-plus-context cell set over _dim displayed axes.  The
// coarsest level (0) fills the space; every finer level adds a window of
// _window_radius cells out from the focus, so detail concentrates near the
// focus while the coarse context remains.  _focus is the focus position in
// normalised [0, 1] per axis.  Cells from different levels overlap (they nest),
// which is intended for wireframe rendering.
D_NODISCARD inline std::vector<lod_cell>
generate_lod_cells(
    std::size_t                _dim,
    const std::vector<scalar>& _focus,
    const lod_scheme&          _scheme,
    std::size_t                _visible_levels,
    std::size_t                _window_radius = 1
)
{
    std::vector<lod_cell> out;

    if ((_dim == 0) || (_visible_levels == 0))
    {
        return out;
    }

    for (std::size_t level = 0; level < _visible_levels; ++level)
    {
        std::size_t divisions = _scheme.divisions_at(level);

        if (divisions == 0)
        {
            divisions = 1;
        }

        // per-axis index window: the full grid at level 0, else a box around
        // the focus cell at this level
        std::vector<std::size_t> lo(_dim);
        std::vector<std::size_t> hi(_dim);

        for (std::size_t a = 0; a < _dim; ++a)
        {
            if (level == 0)
            {
                lo[a] = 0;
                hi[a] = divisions - 1;

                continue;
            }

            scalar f = (a < _focus.size()) ? _focus[a]
                                           : static_cast<scalar>(0.5);

            if (f < static_cast<scalar>(0))
            {
                f = static_cast<scalar>(0);
            }

            if (f > static_cast<scalar>(1))
            {
                f = static_cast<scalar>(1);
            }

            std::size_t fc =
                static_cast<std::size_t>(f * static_cast<scalar>(divisions));

            if (fc >= divisions)
            {
                fc = divisions - 1;
            }

            lo[a] = (fc > _window_radius) ? (fc - _window_radius) : 0;
            hi[a] = fc + _window_radius;

            if (hi[a] >= divisions)
            {
                hi[a] = divisions - 1;
            }
        }

        // iterate the index box across all axes
        std::vector<std::size_t> counts(_dim);
        std::size_t              total = 1;

        for (std::size_t a = 0; a < _dim; ++a)
        {
            counts[a] = hi[a] - lo[a] + 1;
            total    *= counts[a];
        }

        for (std::size_t n = 0; n < total; ++n)
        {
            lod_cell cell;

            cell.level = static_cast<re_std::uint32_t>(level);
            cell.indices.resize(_dim);
            cell.center.resize(_dim);
            cell.size = static_cast<scalar>(1) / static_cast<scalar>(divisions);

            std::size_t rem = n;

            for (std::size_t a = 0; a < _dim; ++a)
            {
                std::size_t idx = lo[a] + (rem % counts[a]);
                rem            /= counts[a];

                cell.indices[a] = idx;
                cell.center[a]  =
                    (static_cast<scalar>(idx) + static_cast<scalar>(0.5)) /
                    static_cast<scalar>(divisions);
            }

            out.push_back(static_cast<lod_cell&&>(cell));
        }
    }

    return out;
}

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_LOD_HPP
