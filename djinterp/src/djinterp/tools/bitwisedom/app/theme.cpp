/*******************************************************************************
* bitwisedom [app]                                                     theme.cpp
*
* Dear ImGui's look, by theme and scheme.
*   Each theme starts from one of ImGui's own and replaces the surfaces --
* the scheme's -- and the accent; the metrics -- rounding, padding, spacing
* -- are the same in every theme.
*
* path:      /src/djinterp/tools/bitwisedom/app/theme.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "./theme.hpp"  // corresponding header


namespace bitwisedom
{
namespace gui
{

namespace
{

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

ImFont*&
mono_font()
{
    static ImFont* font = nullptr;

    return font;
}

float&
ui_scale()
{
    static float scale = 1.0f;

    return scale;
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

    // the metrics follow the display's scale; the borders stay hairlines
    style.ScaleAllSizes(ui_scale());
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
    c[ImGuiCol_Separator]            = shade(frame, lift);
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
