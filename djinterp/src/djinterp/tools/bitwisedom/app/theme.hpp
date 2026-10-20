/*******************************************************************************
* bitwisedom [app]                                                     theme.hpp
*
* Dear ImGui's look, by theme -- dark or light -- and colour scheme.
*   The theme and the scheme are the window's, not the sheet's: the scheme
* gives the surfaces their tint, and the look's accent -- the scheme's, or
* the sheet's own -- tints ImGui's highlights: headers, checks, sliders, the
* active tab. So the window and the grid agree on what "selected" looks
* like.
*   Text drawn over a colour a sheet chooses -- a row's cells, its label, a
* band -- is readable whatever the colour: its ink is the theme's text where
* that reads on what lies beneath, and black or white where it does not; and
* text drawn in a row's colour keeps the colour only as far as it reads on
* the panel it is drawn on. A row with no fill has no colour to read in, and
* its text is the theme's.
*
* path:      /src/djinterp/tools/bitwisedom/app/theme.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.18
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

// readable text -- composite() lays a colour over an opaque ground, and
// contrast() is WCAG's ratio of two opaque colours' luminances, from 1 (none)
// to 21 (black on white). ink_on() is the text colour for a ground: _text
// where it contrasts 4.5:1, else black or white, whichever contrasts more --
// at least 4.5:1, on any ground. legible() is a row's colour as text on a
// ground: the colour, moved toward the ink until it contrasts at least 3:1;
// a row with no colour is _text. faint() is an ink at an alpha, as muted
// text is the theme's text faded.
ImU32    composite(ImU32 _top,
                   ImU32 _ground);
float    contrast(ImU32 _a,
                  ImU32 _b);
ImU32    ink_on(ImU32 _ground,
                ImU32 _text);
ImU32    legible(rgba  _colour,
                 ImU32 _ground,
                 ImU32 _text);
ImU32    faint(ImU32 _ink,
               float _alpha);

// fonts -- the monospace font, once loaded; nullptr until then; and the
// size text is being drawn at, before ImGui's global factors -- a window's
// zoom included -- which a smaller or larger text is a fraction of
ImFont*& mono_font();
float    font_base();

// scale -- the display's scale, which the host sets before the first frame:
// 1 on a 96-dpi screen, 1.5 at 144; the zoom of the window being drawn,
// which its dock window sets while it draws its content, and is 1 outside
// any; and their product, which every metric is multiplied by
float&   dpi_scale();
float&   zoom_now();
float    ui_scale();

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_THEME_HPP
