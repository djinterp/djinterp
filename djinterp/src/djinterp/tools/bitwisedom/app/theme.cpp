/*******************************************************************************
* bitwisedom [app]                                                     theme.cpp
*
* Dear ImGui's look, by theme and scheme.
*   Each theme starts from one of ImGui's own and replaces the surfaces --
* the scheme's -- and the accent; the metrics -- rounding, padding, spacing
* -- are the same in every theme.
*   Readable text is measured as WCAG measures it: a colour's relative
* luminance, from its channels taken back to linear light, and the ratio of
* two luminances, each lifted by 0.05 for the glare a screen adds.
*
* path:      /src/djinterp/tools/bitwisedom/app/theme.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./theme.hpp"  // corresponding header

// std
#include <algorithm>  // std::max, std::min
#include <cmath>      // std::pow
// imgui
#include <imgui_internal.h>  // GImGui, the font size being drawn at


namespace bitwisedom
{
namespace gui
{

namespace
{

// DARK_INK / LIGHT_INK
//   constants: the inks text takes where its theme's own text would not
// read: black, and white. One of the two contrasts at least 4.58:1 with any
// ground there is.
const ImU32 DARK_INK  = IM_COL32(0, 0, 0, 255);
const ImU32 LIGHT_INK = IM_COL32(255, 255, 255, 255);

// READABLE
//   constant: the contrast text keeps with what it is drawn on before it
// leaves its theme's colour: WCAG's least for body text, AA.
const float READABLE = 4.5f;

// MIN_CONTRAST
//   constant: the least contrast text in a row's colour keeps with what it
// is drawn on -- WCAG's least for large text, and for the parts of a
// picture that must be seen.
const float MIN_CONTRAST = 3.0f;

// channel_of
//   one 8-bit channel of an ImGui colour, by its shift.
float
channel_of(
    ImU32 _colour,
    int   _shift
)
{
    return static_cast<float>((_colour >> _shift) & 0xFFu) / 255.0f;
}

// linear
//   one sRGB channel, in [0, 1], as linear light.
float
linear(
    float _channel
)
{
    if (_channel <= 0.04045f)
    {
        return _channel / 12.92f;
    }

    return std::pow((_channel + 0.055f) / 1.055f, 2.4f);
}

// luminance
//   an opaque colour's relative luminance, in [0, 1]; its alpha is not read.
float
luminance(
    ImU32 _colour
)
{
    return ( 0.2126f * linear(channel_of(_colour, IM_COL32_R_SHIFT)) +
             0.7152f * linear(channel_of(_colour, IM_COL32_G_SHIFT)) +
             0.0722f * linear(channel_of(_colour, IM_COL32_B_SHIFT)) );
}

// opaque
//   a colour with its alpha full.
ImU32
opaque(
    ImU32 _colour
)
{
    return (_colour | IM_COL32_A_MASK);
}

// shade
//   a colour lighter (positive) or darker (negative) by an amount.
ImVec4
shade(
    const ImVec4& _colour,
    float         _amount
)
{
    // clamp
    //   one channel held to [0, 1].
    auto clamp = [](float _value)
    {
        return (_value < 0.0f) ? 0.0f : ((_value > 1.0f) ? 1.0f : _value);
    };

    return ImVec4(clamp(_colour.x + _amount),
                  clamp(_colour.y + _amount),
                  clamp(_colour.z + _amount),
                  _colour.w);
}

ImVec4
alpha(
    const ImVec4& _colour,
    float         _alpha
)
{
    return ImVec4(_colour.x, _colour.y, _colour.z, _alpha);
}

}  // anonymous namespace


ImU32
to_u32(
    rgba _colour
)
{
    return IM_COL32((_colour >> 24) & 0xFFu,
                    (_colour >> 16) & 0xFFu,
                    (_colour >> 8) & 0xFFu,
                    _colour & 0xFFu);
}

ImVec4
to_vec4(
    rgba _colour
)
{
    return ImVec4(channel(_colour, 0),
                  channel(_colour, 1),
                  channel(_colour, 2),
                  channel(_colour, 3));
}

ImU32
mix(
    ImU32 _a,
    ImU32 _b,
    float _t
)
{
    const ImVec4 a = ImGui::ColorConvertU32ToFloat4(_a);
    const ImVec4 b = ImGui::ColorConvertU32ToFloat4(_b);

    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(a.x + (b.x - a.x) * _t,
               a.y + (b.y - a.y) * _t,
               a.z + (b.z - a.z) * _t,
               a.w + (b.w - a.w) * _t));
}

bool
is_light(
    const colour_settings& _colours
)
{
    return (_colours.theme == 1);
}

/*
composite
  Each channel is the top's, weighed by its alpha, and the ground's for the
rest. The ground is taken as opaque -- what text is drawn on is the window
at the bottom of every stack of fills -- and so is the result.
*/
ImU32
composite(
    ImU32 _top,
    ImU32 _ground
)
{
    const float alpha = channel_of(_top, IM_COL32_A_SHIFT);

    // blend
    //   one channel of the composite, as a byte.
    auto blend = [&](int _shift)
    {
        const float top    = channel_of(_top, _shift);
        const float ground = channel_of(_ground, _shift);
        const float mixed  = top * alpha + ground * (1.0f - alpha);

        return static_cast<ImU32>(mixed * 255.0f + 0.5f);
    };

    return IM_COL32(blend(IM_COL32_R_SHIFT),
                    blend(IM_COL32_G_SHIFT),
                    blend(IM_COL32_B_SHIFT),
                    255);
}

float
contrast(
    ImU32 _a,
    ImU32 _b
)
{
    const float a = luminance(_a);
    const float b = luminance(_b);

    return (std::max(a, b) + 0.05f) / (std::min(a, b) + 0.05f);
}

/*
ink_on
  The theme's text, wherever it reads -- on the window's own panels it
always does; else black or white, whichever contrasts more with the
ground. Black and white contrast alike with a ground of luminance 0.179,
and there each manages 4.58:1, so whatever the ground the ink reads at
least as well as WCAG asks of body text.
*/
ImU32
ink_on(
    ImU32 _ground,
    ImU32 _text
)
{
    const ImU32 ground = opaque(_ground);
    const ImU32 text   = opaque(_text);

    // the theme's own text, while it reads
    if (contrast(text, ground) >= READABLE)
    {
        return text;
    }

    return (contrast(DARK_INK, ground) >= contrast(LIGHT_INK, ground))
               ? DARK_INK
               : LIGHT_INK;
}

/*
legible
  The colour as it would be seen -- composited on the ground, where it is
not opaque -- if that reads; else the colour moved a tenth at a time toward
the ink the ground takes, which keeps its hue as long as it can. No colour,
as a row with no fill has, is the theme's text.
*/
ImU32
legible(
    rgba  _colour,
    ImU32 _ground,
    ImU32 _text
)
{
    const ImU32 ground = opaque(_ground);

    // no colour, or none that shows: the text's own
    if ((_colour & 0xFFu) == 0u)
    {
        return opaque(_text);
    }

    const ImU32 own = composite(to_u32(_colour), ground);

    if (contrast(own, ground) >= MIN_CONTRAST)
    {
        return own;
    }

    const ImU32 ink = ink_on(ground, _text);

    for (int step = 1; step < 10; ++step)
    {
        const ImU32 moved = mix(own, ink, static_cast<float>(step) * 0.1f);

        if (contrast(moved, ground) >= MIN_CONTRAST)
        {
            return moved;
        }
    }

    return ink;
}

ImU32
faint(
    ImU32 _ink,
    float _alpha
)
{
    const float  clamped = std::max(0.0f, std::min(1.0f, _alpha));
    const ImU32  alpha   = static_cast<ImU32>(clamped * 255.0f + 0.5f);

    return ( (_ink & ~IM_COL32_A_MASK) |
             (alpha << IM_COL32_A_SHIFT) );
}

ImFont*&
mono_font()
{
    static ImFont* font = nullptr;

    return font;
}

float
font_base()
{
    return GImGui->FontSizeBase;
}

float&
dpi_scale()
{
    static float scale = 1.0f;

    return scale;
}

float&
zoom_now()
{
    static float zoom = 1.0f;

    return zoom;
}

float
ui_scale()
{
    return dpi_scale() * zoom_now();
}

/*
apply_style
  The surfaces are the scheme's for the theme; everything interactive is
derived from the accent, so changing the accent recolours the whole window
consistently.
*/
void
apply_style(
    const look& _look,
    int         _scheme
)
{
    const colour_settings& colours  = _look.colours;
    const scheme_colours   surfaces = scheme_for(colours.theme, _scheme);
    ImGuiStyle&            style    = ImGui::GetStyle();
    const bool             light    = is_light(colours);
    const ImVec4           accent   = to_vec4(colours.accent);

    if (light)
    {
        ImGui::StyleColorsLight(&style);
    }
    else
    {
        ImGui::StyleColorsDark(&style);
    }

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.FramePadding      = ImVec2(7.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 5.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.IndentSpacing     = 16.0f;
    style.ScrollbarSize     = 12.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = light ? 1.0f : 0.0f;
    style.TabBorderSize     = 0.0f;

    // the metrics follow the display's scale -- a window's zoom scales its
    // own, as it is drawn; the borders stay hairlines
    style.ScaleAllSizes(dpi_scale());
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = light ? 1.0f : 0.0f;

    ImVec4* c = style.Colors;

    const ImVec4 window = to_vec4(surfaces.window);
    const ImVec4 child  = to_vec4(surfaces.panel);
    const ImVec4 frame  = to_vec4(surfaces.frame);
    const ImVec4 text   = to_vec4(surfaces.text);

    const float lift = light ? -0.05f : 0.06f;

    c[ImGuiCol_Text]                 = text;
    c[ImGuiCol_TextDisabled]         = alpha(text, 0.45f);
    c[ImGuiCol_WindowBg]             = window;
    c[ImGuiCol_ChildBg]              = child;
    c[ImGuiCol_PopupBg]              = shade(child, light ? 0.0f : 0.03f);
    c[ImGuiCol_MenuBarBg]            = shade(window, lift * 0.6f);
    c[ImGuiCol_Border]               = shade(frame, lift);
    c[ImGuiCol_FrameBg]              = frame;
    c[ImGuiCol_FrameBgHovered]       = shade(frame, lift);
    c[ImGuiCol_FrameBgActive]        = shade(frame, lift * 2.0f);
    c[ImGuiCol_TitleBg]              = window;
    c[ImGuiCol_TitleBgActive]        = shade(window, lift);
    c[ImGuiCol_ScrollbarBg]          = alpha(child, 0.0f);
    c[ImGuiCol_ScrollbarGrab]        = shade(frame, lift * 2.0f);
    c[ImGuiCol_ScrollbarGrabHovered] = shade(frame, lift * 3.0f);
    c[ImGuiCol_ScrollbarGrabActive]  = accent;
    c[ImGuiCol_CheckMark]            = accent;
    c[ImGuiCol_SliderGrab]           = alpha(accent, 0.85f);
    c[ImGuiCol_SliderGrabActive]     = accent;
    c[ImGuiCol_Button]               = shade(frame, lift * 0.5f);
    c[ImGuiCol_ButtonHovered]        = alpha(accent, 0.55f);
    c[ImGuiCol_ButtonActive]         = alpha(accent, 0.80f);
    c[ImGuiCol_Header]               = alpha(accent, 0.30f);
    c[ImGuiCol_HeaderHovered]        = alpha(accent, 0.42f);
    c[ImGuiCol_HeaderActive]         = alpha(accent, 0.60f);
    c[ImGuiCol_Separator]            = light ? ImVec4(0, 0, 0, 1)
                                             : ImVec4(1, 1, 1, 1);
    c[ImGuiCol_SeparatorHovered]     = alpha(accent, 0.70f);
    c[ImGuiCol_SeparatorActive]      = accent;
    c[ImGuiCol_ResizeGrip]           = alpha(accent, 0.20f);
    c[ImGuiCol_ResizeGripHovered]    = alpha(accent, 0.60f);
    c[ImGuiCol_ResizeGripActive]     = accent;
    c[ImGuiCol_Tab]                  = shade(window, lift * 0.8f);
    c[ImGuiCol_TabHovered]           = alpha(accent, 0.55f);
    c[ImGuiCol_TabSelected]          = alpha(accent, 0.35f);
    c[ImGuiCol_TabSelectedOverline]  = accent;
    c[ImGuiCol_TabDimmed]            = c[ImGuiCol_Tab];
    c[ImGuiCol_TabDimmedSelected]    = c[ImGuiCol_TabSelected];
    c[ImGuiCol_TableHeaderBg]        = shade(frame, lift * 0.5f);
    c[ImGuiCol_TableBorderStrong]    = shade(frame, lift * 2.0f);
    c[ImGuiCol_TableBorderLight]     = shade(frame, lift);
    c[ImGuiCol_TableRowBg]           = alpha(frame, 0.0f);
    c[ImGuiCol_TableRowBgAlt]        = alpha(frame, light ? 0.35f : 0.30f);
    c[ImGuiCol_TextSelectedBg]       = alpha(accent, 0.35f);
    c[ImGuiCol_DragDropTarget]       = accent;
    c[ImGuiCol_NavCursor]            = accent;

    return;
}

}  // namespace gui
}  // namespace bitwisedom
