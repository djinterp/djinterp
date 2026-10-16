/*******************************************************************************
* djinterp [ui]                                            ui_tests_colormap.cpp
*
* Unit tests for ui::colormap.
*
*
* path:      /tests/djinterp/ui/ui_tests_colormap.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "ui_tests.hpp"  // corresponding header


namespace djinterp
{
namespace testing
{

/*
tests_colormap_stops
  sampling a ramp between, at, and beyond its stops
  Tests the following:
  - a stop's own position gives its colour
  - between stops the colour interpolates linearly
  - positions outside [first, last] take the end colours
  - coincident stops do not divide by zero; no stops gives black
  - the alpha overload carries the opacity
*/
bool
tests_colormap_stops()
{
    const ui::colormap cm{ { { 0.0, ui::rgb(0.0f, 0.0f, 0.0f) },
                             { 0.5, ui::rgb(1.0f, 0.0f, 0.0f) },
                             { 1.0, ui::rgb(1.0f, 1.0f, 1.0f) } } };

    if ( (!same_rgb(cm.sample(0.0), ui::rgb(0.0f, 0.0f, 0.0f)))  ||
         (!same_rgb(cm.sample(0.25), ui::rgb(0.5f, 0.0f, 0.0f))) ||
         (!same_rgb(cm.sample(0.5), ui::rgb(1.0f, 0.0f, 0.0f)))  ||
         (!same_rgb(cm.sample(0.75), ui::rgb(1.0f, 0.5f, 0.5f))) ||
         (!same_rgb(cm.sample(-3.0), ui::rgb(0.0f, 0.0f, 0.0f))) ||
         (!same_rgb(cm.sample(9.0), ui::rgb(1.0f, 1.0f, 1.0f))) )
    {
        return false;
    }

    const ui::colormap step{ { { 0.5, ui::rgb(0.0f, 0.0f, 1.0f) },
                               { 0.5, ui::rgb(0.0f, 1.0f, 0.0f) } } };
    const ui::rgba     half = cm.sample(0.5, 0.25);

    return ( (same_rgb(step.sample(0.5), ui::rgb(0.0f, 0.0f, 1.0f))) &&
             (same_rgb(step.sample(0.6), ui::rgb(0.0f, 1.0f, 0.0f))) &&
             (same_rgb(ui::colormap{}.sample(0.5),
                       ui::rgb(0.0f, 0.0f, 0.0f))) &&
             (near(half.a, 0.25)) &&
             (near(half.r, 1.0)) );
}

/*
tests_colormap_presets
  the presets, and the default the rest of the module relies on
  Tests the following:
  - cool_warm has the module's original endpoints, and heat_color matches it
  - a default config colours with cool_warm
  - yellow_orange_red darkens monotonically, as a light-background ramp must
*/
bool
tests_colormap_presets()
{
    const ui::colormap cw = ui::colormap::cool_warm();

    if ( (!same_rgb(cw.sample(0.0), ui::rgb(0.20f, 0.30f, 0.90f))) ||
         (!same_rgb(cw.sample(1.0), ui::rgb(0.90f, 0.25f, 0.20f))) )
    {
        return false;
    }

    const ui::rgba legacy = ui::internal::heat_color(0.3, 0.7);
    const ui::rgba now    = cw.sample(0.3, 0.7);

    if ( (!same_rgb(legacy.to_rgb(), now.to_rgb())) ||
         (!near(legacy.a, now.a)) )
    {
        return false;
    }

    if (!same_rgb(ui::config{}.colors.sample(0.4), cw.sample(0.4)))
    {
        return false;
    }

    const ui::colormap ylorrd = ui::colormap::yellow_orange_red();
    double             last   = 2.0;

    for (int i = 0; i <= 20; ++i)
    {
        const ui::rgb c = ylorrd.sample(static_cast<double>(i) / 20.0);
        const double  y = (0.2126 * c.r) + (0.7152 * c.g) + (0.0722 * c.b);

        if (y > last + 1e-9)
        {
            return false;
        }

        last = y;
    }

    return true;
}

}  // namespace testing
}  // namespace djinterp
