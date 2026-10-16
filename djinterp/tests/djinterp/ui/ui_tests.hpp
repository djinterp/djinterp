/*******************************************************************************
* djinterp [ui]                                                     ui_tests.hpp
*
* Shared declarations and fixtures for the djinterp::ui unit tests.
*   Covers the parts that need no window: colormaps, the visit-counting state
* source and the heatmap lens over it, field picking and layout, SVG export, and
* scene projection and picking through the render3d camera.
*
*
* path:      /tests/djinterp/ui/ui_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_UI_UI_TESTS_HPP
#define DJINTERP_TESTS_UI_UI_TESTS_HPP 1

// std
#include <cmath>   // std::fabs
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include <djinterp/ui/ui.hpp>  // the module under test


namespace djinterp
{
namespace testing
{

namespace ui = ::djinterp::ui;

// near
//   helper: whether two numbers agree to within _eps.
inline bool
near(
    double _a,
    double _b,
    double _eps = 1e-6
)
{
    return (std::fabs(_a - _b) <= _eps);
}

// same_rgb
//   helper: whether two colours agree channel by channel.
inline bool
same_rgb(
    const ui::rgb& _a,
    const ui::rgb& _b
)
{
    return ( (near(_a.r, _b.r)) &&
             (near(_a.g, _b.g)) &&
             (near(_a.b, _b.b)) );
}

// discrete_axis
//   helper: a discrete axis named _name with _labels.
inline ui::axis
discrete_axis(
    const std::string&              _name,
    const std::vector<std::string>& _labels
)
{
    ui::axis a;
    a.name   = _name;
    a.kind   = ui::axis_kind::discrete;
    a.labels = _labels;

    return a;
}

// numbered
//   helper: labels "0" to "_n - 1".
inline std::vector<std::string>
numbered(
    std::size_t _n
)
{
    std::vector<std::string> out;

    for (std::size_t i = 0; i < _n; ++i)
    {
        out.push_back(std::to_string(i));
    }

    return out;
}

// heatmap_config
//   helper: a configuration for the heatmap lens over axes 0 and 1.
inline ui::config
heatmap_config()
{
    ui::config cfg;
    cfg.lens.mode = ui::lens_mode::heatmap;
    cfg.lens.axes = { 0, 1 };

    return cfg;
}

// colormap
bool tests_colormap_stops();
bool tests_colormap_presets();

// visit_source
bool tests_visit_source_counts();
bool tests_visit_source_paths();
bool tests_heatmap_on_visits();

// fields
bool tests_field_picking();
bool tests_field_layout();

// export
bool tests_svg_export();

// scenes
bool tests_scene_projection();

}  // namespace testing
}  // namespace djinterp


#endif  // DJINTERP_TESTS_UI_UI_TESTS_HPP
