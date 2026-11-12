/*******************************************************************************
* djinterp [ui]                                                        imgui.hpp
*
*   The Dear ImGui binding: the interactive frontend.  It translates the
* framework-agnostic draw_list (draw.hpp) into ImGui draw-list calls and offers
* two panels.  draw_scene() owns a canvas, drives an orbit camera from the mouse
* (drag to orbit, right-drag to pan, wheel to zoom) and draws the projected
* scene; the camera frames the scene the first time it is drawn.  draw_field()
* draws a scene's scalar field as a labelled grid and reports the cell under
* the mouse.  Both draw into the current window, filling the space left in it,
* so they embed in any layout; show_scene() and show_field() wrap them in a
* window of their own.
*   It lives beside the tools' windows, not in djinterp::ui, because it needs
* Dear ImGui and djinterp does not: include it only in a window that links
* Dear ImGui, with imgui.h on the include path.  Like djinterp::ui, it needs
* C++17, and below that it is empty.
*
*   USAGE (per frame, inside an ImGui frame):
*     static viewer view;                 // camera + hover/selection, persists
*     scene s = vis.build_scene(frame);    // rebuilt when the data changes
*     draw_scene(s, view, cfg);            // or show_scene("title", ...)
*     field_hit hit = draw_field(s, scale_of(s.field.value()));
*
*
* path:      /inc/ui/frontends/imgui.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.11.13
*******************************************************************************/

#ifndef UI_FRONTENDS_IMGUI_HPP
#define UI_FRONTENDS_IMGUI_HPP 1

// djinterp
#include "../../djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // djinterp::ui's floor

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string, std::to_string
// imgui
#include "imgui.h"  // ImGui, ImDrawList, ImVec2, IM_COL32
// djinterp [ui]
#include "../../djinterp/tools/ui/config.hpp"  // config
#include "../../djinterp/tools/ui/draw.hpp"    // draw_list, project_scene,
                                               // layout_field, pick_cell
#include "../../djinterp/tools/ui/scene.hpp"   // scene
#include "../../djinterp/tools/ui/types.hpp"   // scalar, vec, camera,
                                               // viewport
// re_std
#include "../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP
NS_UI

NS_INTERNAL

    // color_channel
    //   helper: converts a normalised colour channel to 0-255.
    D_NODISCARD inline int color_channel(
        scalar _v
    )
    {
        scalar v = _v;

        if (v < static_cast<scalar>(0))
        {
            v = static_cast<scalar>(0);
        }

        if (v > static_cast<scalar>(1))
        {
            v = static_cast<scalar>(1);
        }

        return static_cast<int>(v * static_cast<scalar>(255) +
                                static_cast<scalar>(0.5));
    }

    // canvas
    //   helper: the space left in the current window, at least _min square.
    D_NODISCARD inline ImVec2 canvas(
        float _min
    )
    {
        ImVec2 size = ImGui::GetContentRegionAvail();

        size.x = (size.x < _min) ? _min : size.x;
        size.y = (size.y < _min) ? _min : size.y;

        return size;
    }

NS_END  // internal

// to_imu32
//   packs an rgba colour into ImGui's 32-bit colour.
D_NODISCARD inline ImU32 to_imu32(
    const rgba& _c
)
{
    return IM_COL32(internal::color_channel(_c.r),
                    internal::color_channel(_c.g),
                    internal::color_channel(_c.b),
                    internal::color_channel(_c.a));
}

// render_draw_list
//   draws a draw_list into the ImGui draw list _dl, offsetting every point by
// the canvas origin _origin.
inline
void render_draw_list(
    const draw_list& _list,
    ImDrawList*      _dl,
    const ImVec2&    _origin
)
{
    // filled rectangles first, as a backdrop
    for (std::size_t i = 0; i < _list.rects.size(); ++i)
    {
        const draw_rect& r = _list.rects[i];

        ImVec2 p0(_origin.x + static_cast<float>(r.min[0]),
                  _origin.y + static_cast<float>(r.min[1]));
        ImVec2 p1(_origin.x + static_cast<float>(r.max[0]),
                  _origin.y + static_cast<float>(r.max[1]));

        _dl->AddRectFilled(p0, p1, to_imu32(r.color));
    }

    // translucent filled quads (shape faces), back to front
    for (std::size_t i = 0; i < _list.quads.size(); ++i)
    {
        const draw_quad& q = _list.quads[i];

        ImVec2 p0(_origin.x + static_cast<float>(q.a[0]),
                  _origin.y + static_cast<float>(q.a[1]));
        ImVec2 p1(_origin.x + static_cast<float>(q.b[0]),
                  _origin.y + static_cast<float>(q.b[1]));
        ImVec2 p2(_origin.x + static_cast<float>(q.c[0]),
                  _origin.y + static_cast<float>(q.c[1]));
        ImVec2 p3(_origin.x + static_cast<float>(q.d[0]),
                  _origin.y + static_cast<float>(q.d[1]));

        _dl->AddQuadFilled(p0, p1, p2, p3, to_imu32(q.color));
    }

    // stroked segments over them
    for (std::size_t i = 0; i < _list.segments.size(); ++i)
    {
        const draw_segment& seg = _list.segments[i];

        ImVec2 p0(_origin.x + static_cast<float>(seg.a[0]),
                  _origin.y + static_cast<float>(seg.a[1]));
        ImVec2 p1(_origin.x + static_cast<float>(seg.b[0]),
                  _origin.y + static_cast<float>(seg.b[1]));

        _dl->AddLine(p0,
                     p1,
                     to_imu32(seg.color),
                     static_cast<float>(seg.thickness));
    }

    // text labels on top
    for (std::size_t i = 0; i < _list.labels.size(); ++i)
    {
        const draw_label& lab = _list.labels[i];

        ImVec2 p(_origin.x + static_cast<float>(lab.position[0]),
                 _origin.y + static_cast<float>(lab.position[1]));

        _dl->AddText(p, to_imu32(lab.color), lab.text.c_str());
    }

    return;
}

// imgui_text_metrics
//   the current font's metrics, for layout_field.
D_NODISCARD inline text_metrics imgui_text_metrics()
{
    text_metrics out;
    out.line_height = static_cast<scalar>(ImGui::GetTextLineHeight());
    out.width       = [](const std::string& _text)
    {
        return static_cast<scalar>(ImGui::CalcTextSize(_text.c_str()).x);
    };

    return out;
}

// viewer
//   struct: the persistent per-panel view state -- the orbit camera, the
// hovered and selected cell indices (no_cell when none), and whether the
// camera has been framed on the scene yet.  Hold one per scene panel, across
// frames; clear `framed` to frame the camera again.
struct viewer
{
    camera<scalar> cam;
    std::size_t    hovered  = no_cell;
    std::size_t    selected = no_cell;
    bool           framed   = false;
};

NS_INTERNAL

    // scene_input
    //   helper: drives _view's camera from the mouse over the canvas: drag to
    // orbit, right-drag to pan, wheel to zoom (up is closer).
    inline void scene_input(
        viewer& _view,
        bool    _hovered,
        bool    _active
    )
    {
        const scalar orbit_rate = static_cast<scalar>(0.01);
        const scalar pan_rate   = static_cast<scalar>(0.002);
        const scalar zoom_in    = static_cast<scalar>(0.90);
        const scalar zoom_out   = static_cast<scalar>(1.10);
        ImGuiIO&     io         = ImGui::GetIO();

        if (_active)
        {
            scalar dx = static_cast<scalar>(io.MouseDelta.x);
            scalar dy = static_cast<scalar>(io.MouseDelta.y);

            if (io.MouseDown[0])
            {
                // drag to orbit
                _view.cam.orbit(-dx * orbit_rate, -dy * orbit_rate);
            }
            else if (io.MouseDown[1])
            {
                // right-drag to pan
                _view.cam.pan(-dx * pan_rate, dy * pan_rate);
            }
        }

        if (_hovered && (io.MouseWheel != 0.0f))
        {
            _view.cam.zoom((io.MouseWheel > 0.0f) ? zoom_in : zoom_out);
        }

        return;
    }

    // scene_pick
    //   helper: the cell under the mouse becomes _view's hovered cell, and a
    // left release that wasn't a drag selects it (or clears the selection,
    // over empty space).
    inline void scene_pick(
        const scene&            _scene,
        viewer&                 _view,
        const viewport<scalar>& _vp,
        const ImVec2&           _origin,
        bool                    _hovered,
        bool                    _active
    )
    {
        const float click_eps = 4.0f;  // a release within this is a click
        ImGuiIO&    io        = ImGui::GetIO();

        _view.hovered = no_cell;

        if (_hovered && !_active)
        {
            vec<2, scalar> screen(
                static_cast<scalar>(io.MousePos.x - _origin.x),
                static_cast<scalar>(io.MousePos.y - _origin.y));

            _view.hovered = pick_cell(_scene, _view.cam, _vp, screen);
        }

        // the drag delta stays zero until the mouse passes the threshold, so
        // a zero delta on release means the press was a click
        const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left,
                                                     click_eps);

        if (_hovered &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            (drag.x == 0.0f) && (drag.y == 0.0f))
        {
            _view.selected = _view.hovered;
        }

        return;
    }

    // scene_tooltip
    //   helper: the tooltip over the hovered cell _hovered, if any: its level,
    // its index and its value.
    inline void scene_tooltip(
        const scene& _scene,
        std::size_t  _hovered
    )
    {
        if ( (_hovered == no_cell) ||
             (_hovered >= _scene.cells.size()) )
        {
            return;
        }

        const render_cell& hc    = _scene.cells[_hovered];
        std::string        index = "[";

        for (std::size_t k = 0; k < hc.source.indices.size(); ++k)
        {
            if (k > 0)
            {
                index += ", ";
            }

            index += std::to_string(hc.source.indices[k]);
        }

        index += "]";

        ImGui::BeginTooltip();
        ImGui::Text("level %u", static_cast<unsigned>(hc.level));
        ImGui::Text("cell  %s", index.c_str());

        if (hc.value.has_value())
        {
            ImGui::Text("value %.6g", static_cast<double>(hc.value.value()));
        }
        else
        {
            ImGui::TextUnformatted("value --");
        }

        ImGui::EndTooltip();

        return;
    }

NS_END  // internal

// draw_scene
//   an interactive canvas filling the space left in the current window: drives
// the orbit camera in _view from the mouse over it (drag to orbit, right-drag
// to pan, wheel to zoom), projects _scene, and draws it.  The camera frames
// the scene's bounds on the first draw.  Hovering a cell highlights it and
// shows a tooltip; a click selects it (clicking empty space clears the
// selection).  _view carries the hovered and selected cell indices.
inline
void draw_scene(
    const scene&  _scene,
    viewer&       _view,
    const config& _config
)
{
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size   = internal::canvas(64.0f);

    // a canvas-sized hit target that captures left and right drags
    ImGui::InvisibleButton(
        "##djinterp_canvas",
        size,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();

    viewport<scalar> vp(static_cast<scalar>(size.x),
                        static_cast<scalar>(size.y));
    _view.cam.set_viewport(vp);

    // the first draw aims the camera at the whole scene
    if (!_view.framed)
    {
        _view.cam.frame(_scene.bounds);
        _view.framed = true;
    }

    internal::scene_input(_view, hovered, active);
    internal::scene_pick(_scene, _view, vp, origin, hovered, active);

    // the canvas, clipped, over a dark backdrop
    ImVec2 canvas_end(origin.x + size.x, origin.y + size.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(origin, canvas_end, true);
    dl->AddRectFilled(origin, canvas_end, IM_COL32(20, 22, 28, 255));

    draw_list list =
        project_scene(_scene, _view.cam, vp, _config,
                      _view.hovered, _view.selected);
    render_draw_list(list, dl, origin);

    dl->PopClipRect();

    internal::scene_tooltip(_scene, _view.hovered);

    return;
}

// show_scene
//   draw_scene in a window of its own, titled _label.
inline
void show_scene(
    const char*   _label,
    const scene&  _scene,
    viewer&       _view,
    const config& _config
)
{
    ImGui::Begin(_label);
    draw_scene(_scene, _view, _config);
    ImGui::End();

    return;
}

// field_hit
//   struct: what the mouse is doing over a field panel -- the cell under it,
// if any, and whether that cell was clicked this frame.
struct field_hit
{
    maybe<field_cell> hovered;
    bool              clicked = false;
};

// draw_field
//   draws _scene's scalar field as a labelled grid (layout_field) filling the
// space left in the current window, with _overlay on top.  The cell under the
// mouse is outlined in the style's highlight colour and, if the style asks,
// described in a tooltip; the result reports it, and whether it was clicked,
// so a caller can add its own tooltip or act on the click instead.
inline
field_hit draw_field(
    const scene&         _scene,
    const field_scale&   _scale,
    const field_style&   _style = field_style(),
    const field_overlay& _overlay = field_overlay()
)
{
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size   = internal::canvas(32.0f);

    ImGui::InvisibleButton("##djinterp_field", size);

    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    const field_layout layout =
        layout_field(_scene,
                     vec<2, scalar>(static_cast<scalar>(size.x),
                                    static_cast<scalar>(size.y)),
                     _scale,
                     _style,
                     imgui_text_metrics(),
                     _overlay);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(origin,
                     ImVec2(origin.x + size.x, origin.y + size.y),
                     true);
    render_draw_list(layout.list, dl, origin);

    field_hit hit;

    // hit-test only a field the mouse is actually over
    if ( (hovered) &&
         (_scene.field.has_value()) )
    {
        const scalar_field& field = _scene.field.value();
        const ImVec2        mouse = ImGui::GetIO().MousePos;

        hit.hovered = pick_field(
            field,
            layout.grid_size,
            vec<2, scalar>(static_cast<scalar>(mouse.x - origin.x) -
                               layout.grid_min[0],
                           static_cast<scalar>(mouse.y - origin.y) -
                               layout.grid_min[1]));

        if (hit.hovered.has_value())
        {
            const field_cell cell = hit.hovered.value();
            const draw_rect  r    = field_cell_rect(field, layout.grid_size,
                                                    cell);
            const ImVec2 lo(origin.x + static_cast<float>(layout.grid_min[0] +
                                                          r.min[0]),
                            origin.y + static_cast<float>(layout.grid_min[1] +
                                                          r.min[1]));
            const ImVec2 hi(origin.x + static_cast<float>(layout.grid_min[0] +
                                                          r.max[0]),
                            origin.y + static_cast<float>(layout.grid_min[1] +
                                                          r.max[1]));

            dl->AddRect(lo, hi, to_imu32(_style.highlight), 0.0f, 0, 2.0f);
            hit.clicked = clicked;

            if (_style.tooltip)
            {
                const scalar v =
                    field.values[(cell.row * field.width) + cell.col];
                const auto tick = [&_scene](std::size_t    _axis,
                                            re_std::uint32_t _i)
                {
                    return ( (_axis < _scene.axes.size()) &&
                             (_i < _scene.axes[_axis].tick_labels.size()) )
                               ? _scene.axes[_axis].tick_labels[_i]
                               : std::to_string(_i);
                };

                ImGui::BeginTooltip();
                ImGui::Text("x %s   y %s",
                            tick(0, cell.col).c_str(),
                            tick(1, cell.row).c_str());

                if (v == v)
                {
                    ImGui::Text("value %.6g", static_cast<double>(v));
                }
                else
                {
                    ImGui::TextUnformatted("value --");
                }

                ImGui::EndTooltip();
            }
        }
    }

    dl->PopClipRect();

    return hit;
}

// show_field
//   draw_field in a window of its own, titled _label, with the field's own
// value range and the default style.
inline
void show_field(
    const char*  _label,
    const scene& _scene
)
{
    ImGui::Begin(_label);

    if (_scene.field.has_value())
    {
        (void)draw_field(_scene, scale_of(_scene.field.value()));
    }
    else
    {
        ImGui::TextUnformatted("(this scene has no scalar field)");
    }

    ImGui::End();

    return;
}

NS_END  // ui
NS_END  // djinterp

#endif  // djinterp::ui's floor

#endif  // UI_FRONTENDS_IMGUI_HPP
