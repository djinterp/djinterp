/*******************************************************************************
* bitwisedom [ui]                                                       flow.cpp
*
* The flow view: where an operation sends the sheet's bits.
*   Two strips, one bit to a cell: the distribution before the operation --
* how many allocations claim each bit -- above, and after it below. For an
* operation that only moves bits, a curve joins each bit of the targets to
* the bit it lands on, in the colour of the symbol it belongs to -- or the
* text's, faded, for a symbol with no fill; a bit that falls off the end is
* crossed out, and a bit filled with zero is marked. An operation that
* computes rather than moves outlines the bits it changes.
*
* path:      /src/ui/bitwisedom/flow.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "../../../inc/ui/bitwisedom/elements.hpp"  // corresponding header

// std
#include <algorithm>  // std::max, std::min
#include <cmath>      // std::floor
#include <string>     // std::string
// imgui
#include <imgui.h>  // ImGui, ImDrawList
// bitwisedom
#include "../../../inc/ui/bitwisedom/theme.hpp"  // to_u32
#include <djinterp/tools/bitwisedom/sheet.hpp>  // find_row


namespace bitwisedom
{
namespace gui
{

namespace
{

// owner_colour
//   the colour of the first target whose value, before, holds a bit.
rgba
owner_colour(
    const model& _model,
    int          _bit,
    bool&        _owned
)
{
    for (const op_row& each : _model.preview.rows)
    {
        if (each.before.test(_bit))
        {
            const resolved_row* found = _model.res.find(each.id);

            _owned = true;

            if (found != nullptr)
            {
                const row* source = find_row(_model.doc, found->id);

                if ((source != nullptr) && (source->transparent))
                {
                    return 0;
                }

                return found->colour;
            }

            return _model.shown.colours.accent;
        }
    }

    if ( (_model.preview.probe) &&
         (_model.preview.probe_before.test(_bit)) )
    {
        _owned = true;

        return _model.shown.colours.probe;
    }

    _owned = false;

    return 0x80808080u;
}

}  // anonymous namespace


void
draw_flow(
    model&                        _model,
    uxoxo::platform::imgui_frame& /*_frame*/,
    float                         _height
)
{
    const sheet&           doc     = _model.doc;
    const colour_settings& colours = _model.shown.colours;
    const op_preview&      preview = _model.preview;
    ImDrawList*            list    = ImGui::GetWindowDrawList();
    const ImVec2           origin  = ImGui::GetCursorScreenPos();
    const float            avail   = ImGui::GetContentRegionAvail().x;
    const float            height  = std::max(80.0f, _height);
    const float            label_w = 64.0f;
    const float            cell    = std::min(
        22.0f,
        std::max(6.0f,
                 (avail - label_w - 8.0f) / static_cast<float>(doc.width)));
    const float            strip   = std::min(cell, 18.0f);
    const float            top     = origin.y + 2.0f;
    const float            bottom  = origin.y + height - strip - 2.0f;
    const ImU32            muted   = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImU32            text    = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32            empty   = to_u32(colours.empty_cell);

    ImGui::Dummy(ImVec2(std::max(1.0f, avail), height));

    // nothing to show until something is previewed
    if ( (!_model.preview_valid) ||
         ( (preview.rows.empty()) &&
           (!preview.probe) ) )
    {
        list->AddText(ImVec2(origin.x + 4.0f, origin.y + height * 0.4f),
                      muted,
                      "Select rows, pick an operation, and its effect on "
                      "the sheet's bits is drawn here.");

        return;
    }

    // x_of
    //   the left edge of a bit's cell.
    auto x_of = [&](int _bit)
    {
        const int slot = doc.msb_left ? (doc.width - 1 - _bit) : _bit;

        return origin.x + label_w + static_cast<float>(slot) * cell;
    };

    // in_view
    //   whether a bit's cell is where the panel shows anything: a wide
    // sheet's are mostly not
    const float clip_left  = list->GetClipRectMin().x;
    const float clip_right = list->GetClipRectMax().x;

    auto in_view = [&](int _bit)
    {
        const float x = x_of(_bit);

        return ( (x + cell >= clip_left) &&
                 (x <= clip_right) );
    };

    list->AddText(ImVec2(origin.x + 4.0f, top + 1.0f), muted, "before");
    list->AddText(ImVec2(origin.x + 4.0f, bottom + 1.0f), muted, "after");

    wide before_bits = preview.probe ? preview.probe_before : wide();
    wide after_bits  = preview.probe ? preview.probe_after : wide();

    for (const op_row& each : preview.rows)
    {
        before_bits |= each.before;
        after_bits  |= each.after;
    }

    for (int bit = 0; bit < doc.width; ++bit)
    {
        if (!in_view(bit))
        {
            continue;
        }

        const float x      = x_of(bit);
        const auto  slot   = static_cast<std::size_t>(bit);
        const int   was    = (slot < preview.usage_before.size())
                                 ? preview.usage_before[slot] : 0;
        const int   is     = (slot < preview.usage_after.size())
                                 ? preview.usage_after[slot] : 0;
        const bool  moved  = before_bits.test(bit);
        const bool  lands  = after_bits.test(bit);
        bool        owned  = false;
        const rgba  colour = owner_colour(_model, bit, owned);

        // a collision is two claims in an exclusive group, as the usage
        // row counts it: a register's range over its own flags, in groups
        // of their own, is not one
        const bool  clash_before = preview.collisions_before.test(bit);
        const bool  clash_after  = preview.collisions_after.test(bit);

        // heat
        //   a cell's colour for its claims.
        auto heat = [&](int _count, bool _clash, bool _target)
        {
            if (_clash)
            {
                return to_u32(colours.collision);
            }

            if (_target)
            {
                return (colour != 0) ? to_u32(with_alpha(colour, 0.85f))
                                     : empty;
            }

            return (_count >= 1) ? to_u32(with_alpha(colours.accent,
                                                     0.30f))
                                 : empty;
        };

        list->AddRectFilled(ImVec2(x + 1.0f, top),
                            ImVec2(x + cell - 1.0f, top + strip),
                            heat(was, clash_before, moved),
                            2.0f);

        // the after strip takes its colour from where the bit came from
        bool       from_owned = false;
        const int  source     =
            (bit < static_cast<int>(preview.flow.size()))
                ? preview.flow[static_cast<std::size_t>(bit)]
                : bit;
        const rgba from       = (source >= 0)
                                    ? owner_colour(_model, source, from_owned)
                                    : rgba(0);

        list->AddRectFilled(ImVec2(x + 1.0f, bottom),
                            ImVec2(x + cell - 1.0f, bottom + strip),
                            (clash_after) ? to_u32(colours.collision)
                          : (lands && from_owned && (from != 0))
                              ? to_u32(with_alpha(from, 0.85f))
                          : (lands && from_owned)
                              ? empty
                          : (lands) ? to_u32(with_alpha(colours.accent,
                                                        0.85f))
                          : (is >= 1) ? to_u32(with_alpha(colours.accent,
                                                          0.30f))
                                      : empty,
                            2.0f);

        // a computed operation outlines what it changed
        if ( (preview.flow.empty()) &&
             (moved != lands) )
        {
            list->AddRect(ImVec2(x + 1.0f, bottom),
                          ImVec2(x + cell - 1.0f, bottom + strip),
                          to_u32(colours.ghost),
                          2.0f,
                          0,
                          1.5f);
        }
    }

    // the bit numbers between the strips, where there is room
    if (cell >= 12.0f)
    {
        ImGui::PushFont(nullptr, font_base() * 0.75f);

        for (int bit = 0; bit < doc.width; ++bit)
        {
            if (!in_view(bit))
            {
                continue;
            }

            const std::string label = format_dec(static_cast<u64>(
                (doc.bits == numbering::lsb0) ? bit : doc.width - 1 - bit));
            const ImVec2      size  = ImGui::CalcTextSize(label.c_str());

            if ( ((bit % 4) == 0) ||
                 (cell >= 18.0f) )
            {
                list->AddText(ImVec2(x_of(bit) + (cell - size.x) * 0.5f,
                                     top + strip + 1.0f),
                              muted,
                              label.c_str());
            }
        }

        ImGui::PopFont();
    }

    if (preview.flow.empty())
    {
        list->AddText(ImVec2(origin.x + label_w,
                             (top + bottom + strip) * 0.5f -
                                 ImGui::GetTextLineHeight() * 0.5f),
                      muted,
                      "computed, not moved: the outlined bits change");

        return;
    }

    // the curves: each target bit to where it lands
    const float from_y = top + strip + ImGui::GetTextLineHeight() * 0.8f;
    const float to_y   = bottom - 2.0f;

    for (int bit = 0; bit < doc.width; ++bit)
    {
        const int source = preview.flow[static_cast<std::size_t>(bit)];

        // a curve with neither end in view is not drawn
        if ( (!in_view(bit)) &&
             ( (source < 0) || (!in_view(source)) ) )
        {
            continue;
        }

        if (source < 0)
        {
            // a zero filled in
            const ImVec2 at(x_of(bit) + cell * 0.5f, bottom - 6.0f);
            list->AddCircle(at, 2.5f, muted, 8, 1.0f);

            continue;
        }

        if (!before_bits.test(source))
        {
            continue;
        }

        bool       owned  = false;
        const rgba colour = owner_colour(_model, source, owned);
        const float sx    = x_of(source) + cell * 0.5f;
        const float dx    = x_of(bit) + cell * 0.5f;
        const float mid   = (from_y + to_y) * 0.5f;

        // a row with no fill has no colour to draw its curves in: they are
        // the text's, faded, as the bits around them are
        const ImU32 curve = (colour == 0) ? muted
                                          : to_u32(with_alpha(colour, 0.75f));

        list->AddBezierCubic(ImVec2(sx, from_y),
                             ImVec2(sx, mid),
                             ImVec2(dx, mid),
                             ImVec2(dx, to_y),
                             curve,
                             1.6f);
    }

    // bits that fell off the end
    wide carried;

    for (int source : preview.flow)
    {
        if (source >= 0)
        {
            carried.set(source, true);
        }
    }

    for (int bit = 0; bit < doc.width; ++bit)
    {
        if ( (!in_view(bit))           ||
             (!before_bits.test(bit))  ||
             (carried.test(bit)) )
        {
            continue;
        }

        const float x = x_of(bit);
        const ImU32 x_colour = to_u32(colours.collision);

        list->AddLine(ImVec2(x + 3.0f, top + 3.0f),
                      ImVec2(x + cell - 3.0f, top + strip - 3.0f),
                      x_colour,
                      2.0f);
        list->AddLine(ImVec2(x + cell - 3.0f, top + 3.0f),
                      ImVec2(x + 3.0f, top + strip - 3.0f),
                      x_colour,
                      2.0f);
    }

    (void)text;

    return;
}

}  // namespace gui
}  // namespace bitwisedom
