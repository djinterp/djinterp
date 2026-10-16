/*******************************************************************************
* djinterp [ui]                                               ui_tests_scene.cpp
*
* Unit tests for scene projection and picking through the render3d camera.
*
*
* path:      /tests/djinterp/ui/ui_tests_scene.cpp
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
tests_scene_projection
  a heatmap scene projected through the render3d camera and picked back
  Tests the following:
  - a framed camera looking straight at the scene sees every cell
  - each cell becomes twelve wireframe edges
  - picking at a cell's projected centre returns that cell
  - picking empty space returns no_cell
*/
bool
tests_scene_projection()
{
    ui::visit_source src({ discrete_axis("x", numbered(3)),
                           discrete_axis("y", numbered(2)) },
                         1);

    for (std::size_t x = 0; x < 3; ++x)
    {
        for (std::size_t y = 0; y < 2; ++y)
        {
            src.visit({ x, y }, 0);
        }
    }

    ui::config cfg = heatmap_config();
    cfg.layout.spacing     = 0.5;
    cfg.axes.show_lines    = false;
    cfg.axes.show_scales   = false;
    cfg.axes.show_labels   = false;

    const ui::scene s = ui::heatmap_lens().build(src, cfg, 0).value();

    // straight down the z axis at the plane the heatmap lies in
    ui::camera<double>         cam;
    const ui::viewport<double> vp(400.0, 300.0);

    cam.m_yaw   = 0.0;
    cam.m_pitch = 0.0;
    cam.set_viewport(vp);
    cam.frame(s.bounds);

    const ui::draw_list list = ui::project_scene(s, cam, vp, cfg);

    if ( (s.cells.size() != 6) ||
         (list.segments.size() != 12 * s.cells.size()) )
    {
        return false;
    }

    for (std::size_t c = 0; c < s.cells.size(); ++c)
    {
        const ui::vec<3, double> at = cam.project(s.cells[c].position, vp);

        if ( (at[0] < 0.0) || (at[0] > 400.0) ||
             (at[1] < 0.0) || (at[1] > 300.0) )
        {
            return false;
        }

        if (ui::pick_cell(s, cam, vp, ui::vec<2, double>(at[0], at[1])) != c)
        {
            return false;
        }
    }

    return (ui::pick_cell(s, cam, vp, ui::vec<2, double>(1.0, 1.0)) ==
            ui::no_cell);
}

}  // namespace testing
}  // namespace djinterp
