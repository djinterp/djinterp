/*******************************************************************************
* djinterp [djinterp]                                               colormap.hpp
*
*   Colormaps: piecewise-linear colour ramps that turn a normalised metric in
* [0, 1] into a colour. A colormap is a list of stops, each a position and a
* colour; sampling between two stops interpolates through the colour module's
* lerp, so the colour math lives in one place. Presets cover the common cases:
* cool_warm (the module's original heat ramp, and the default), a
* yellow-orange-red ramp for light backgrounds, and greyscale. Stop colours
* are display colours, as the frontends draw them.
*
*
* path:      /inc/djinterp/ui/colormap.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.20
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_COLORMAP_HPP
#define DJINTERP_UI_COLORMAP_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <utility>  // std::move
#include <vector>   // std::vector
// djinterp [ui]
#include "./types.hpp"  // scalar, rgb, rgba, lerp, channel_t


NS_DJINTERP
NS_UI

// colormap_stop
//   struct: one control point of a colormap -- the colour at position `at`,
// in [0, 1].
struct colormap_stop
{
    scalar at = 0.0;
    rgb    color;
};

// colormap
//   struct: a colour ramp through `stops`, which must be sorted by position.
// Metrics below the first stop take its colour and above the last take the
// last's; a colormap with no stops samples as black.
struct colormap
{
    std::vector<colormap_stop> stops;

    // sample
    //   the colour at the normalised metric _t.
    D_NODISCARD rgb
    sample(
        scalar _t
    ) const
    {
        // no stops, no colour
        if (stops.empty())
        {
            return rgb(0.0f, 0.0f, 0.0f);
        }

        if (_t <= stops.front().at)
        {
            return stops.front().color;
        }

        // find the pair of stops around _t
        for (std::size_t i = 1; i < stops.size(); ++i)
        {
            if (_t <= stops[i].at)
            {
                const colormap_stop& lo   = stops[i - 1];
                const colormap_stop& hi   = stops[i];
                const scalar         span = hi.at - lo.at;
                const scalar         u    =
                    (span > static_cast<scalar>(0))
                        ? ((_t - lo.at) / span)
                        : static_cast<scalar>(1);

                return lerp(lo.color, hi.color, static_cast<channel_t>(u));
            }
        }

        return stops.back().color;
    }

    // sample
    //   the colour at _t, with opacity _alpha.
    D_NODISCARD rgba
    sample(
        scalar _t,
        scalar _alpha
    ) const
    {
        return rgba(sample(_t), static_cast<channel_t>(_alpha));
    }

    // cool_warm
    //   blue through purple to red: the module's original heat ramp.
    D_NODISCARD static colormap
    cool_warm()
    {
        return colormap{ { { 0.0, rgb(0.20f, 0.30f, 0.90f) },
                           { 1.0, rgb(0.90f, 0.25f, 0.20f) } } };
    }

    // yellow_orange_red
    //   pale yellow through orange to dark red, for light backgrounds: low
    // values stay close to the page and high ones stand out.
    D_NODISCARD static colormap
    yellow_orange_red()
    {
        return colormap{ { { 0.00, rgb(1.000f, 1.000f, 0.698f) },
                           { 0.25, rgb(0.996f, 0.800f, 0.361f) },
                           { 0.50, rgb(0.992f, 0.553f, 0.235f) },
                           { 0.75, rgb(0.941f, 0.231f, 0.125f) },
                           { 1.00, rgb(0.741f, 0.000f, 0.149f) } } };
    }

    // grayscale
    //   black to white.
    D_NODISCARD static colormap
    grayscale()
    {
        return colormap{ { { 0.0, rgb(0.0f, 0.0f, 0.0f) },
                           { 1.0, rgb(1.0f, 1.0f, 1.0f) } } };
    }
};

NS_INTERNAL

    // heat_color
    //   helper: the default ramp (cool_warm) at _t, with opacity _alpha. Kept
    // for callers without a configuration to hand.
    D_NODISCARD inline rgba
    heat_color(
        scalar _t,
        scalar _alpha
    )
    {
        return colormap::cool_warm().sample(_t, _alpha);
    }

NS_END  // internal

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_COLORMAP_HPP
