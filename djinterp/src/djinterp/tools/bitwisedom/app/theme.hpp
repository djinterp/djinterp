/*******************************************************************************
* bitwisedom [app]                                                     theme.hpp
*
* Dear ImGui's look, by theme -- dark or light -- and colour scheme.
*   The theme and the scheme are the window's, not the sheet's: the scheme
* gives the surfaces their tint, and the look's accent -- the scheme's, or
* the sheet's own -- tints ImGui's highlights: headers, checks, sliders, the
* active tab. So the window and the grid agree on what "selected" looks
* like.
*
* path:      /src/djinterp/tools/bitwisedom/app/theme.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef BITWISEDOM_APP_THEME_HPP
#define BITWISEDOM_APP_THEME_HPP 1

// imgui
#include <imgui.h>  // ImU32, ImVec4, ImFont
// bitwisedom
#include <djinterp/tools/bitwisedom/sheet.hpp>  // look, scheme_for, rgba


namespace bitwisedom
{
namespace gui
{

// styling -- ImGui's colours and metrics for a look and the scheme it was
// made with
void     apply_style(const look& _look,
                     int         _scheme);

// colours -- a packed colour as ImGui takes one
ImU32    to_u32(rgba _colour);
ImVec4   to_vec4(rgba _colour);
ImU32    mix(ImU32 _a,
             ImU32 _b,
             float _t);
bool     is_light(const colour_settings& _colours);

// fonts -- the monospace font, once loaded; nullptr until then
ImFont*& mono_font();

// scale -- the display's scale, which the host sets before the first frame
// and every metric is multiplied by: 1 on a 96-dpi screen, 1.5 at 144
float&   ui_scale();

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_THEME_HPP
