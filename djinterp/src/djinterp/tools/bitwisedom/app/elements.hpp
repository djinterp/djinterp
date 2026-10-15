/*******************************************************************************
* bitwisedom [app]                                                  elements.hpp
*
* The application's own uxoxo elements: the bit grid, the flow view and the
* code view.
*   Each is an element type of its own, with an archetype no backend knows,
* so its look comes from the renderer register_renderers() installs with
* set_renderer<imgui_platform> -- uxoxo's way of letting an application
* draw what the framework has no vocabulary for. The renderers read the
* model they were registered with, and post events like any element: they
* never change the sheet themselves.
*
* path:      /src/djinterp/tools/bitwisedom/app/elements.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef BITWISEDOM_APP_ELEMENTS_HPP
#define BITWISEDOM_APP_ELEMENTS_HPP 1

// std
#include <string>  // std::string
// uxoxo
#include <uxoxo/element.hpp>                  // element_type, option_set
#include <uxoxo/platform/imgui_platform.hpp>  // imgui_frame
// bitwisedom
#include "./model.hpp"  // model


namespace bitwisedom
{
namespace gui
{

// the elements -- templates whose look the registered renderers supply
uxoxo::element_template  bit_grid(uxoxo::option_set _attrs = {});
uxoxo::element_template  flow_view(uxoxo::option_set _attrs = {});
uxoxo::element_template  code_view(uxoxo::option_set _attrs = {});

/**
 * @brief Installs the three renderers for the ImGui platform.
 *
 * @param[in,out] _model  the model they draw; it must outlive every frame
 *                        drawn with them.
 */
void register_renderers(model& _model);

// the drawings -- called by the renderers inside a frame
void draw_grid(model&                        _model,
               uxoxo::platform::imgui_frame& _frame);

// grid_point
//   struct: a point of the grid on the screen, if the last frame drew it.
struct grid_point
{
    bool  found = false;
    float x     = 0.0f;
    float y     = 0.0f;
};

/**
 * @brief Where the last frame drew a part of the grid, on the screen: for
 *        a test that drives the grid with the mouse.
 *
 * @param[in] _part  "name" or "cell" -- a row's name, or its cell for a
 *                   bit; "band" or "boundary" -- the heading's band over
 *                   a bit, or the ruler's boundary right of it;
 *                   "edge.name" or "edge.value" -- a pinned column's edge;
 *                   or "end" or "room" -- where the columns end, and the
 *                   right edge of the width they were laid out to fill.
 * @param[in] _row   the row's id, for "name" and "cell".
 * @param[in] _bit   the bit, for "cell", "band" and "boundary".
 * @return the point, found or not.
 */
grid_point grid_spot(const std::string& _part,
                     int                _row,
                     int                _bit);
void draw_flow(model&                        _model,
               uxoxo::platform::imgui_frame& _frame,
               float                         _height);
void draw_code(model&                        _model,
               uxoxo::platform::imgui_frame& _frame);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_ELEMENTS_HPP
