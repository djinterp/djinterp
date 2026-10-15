/*******************************************************************************
* bitwisedom [app]                                                      grid.cpp
*
* The bit grid: one row per symbol, one column per bit.
*   The grid scrolls both ways inside its own child window, with the column
* headings pinned to the top and the name and value columns pinned to the
* left, as a spreadsheet's are. Everything is drawn with the window's draw
* list from one layout, computed each frame: the columns -- a bit each, or
* one for a collapsed segment or an abbreviated run of unused bits -- and
* the rows -- the probe, every visible row of the tree, and the usage row.
*   GRID LINES are drawn over the cells, each kind under its own switch,
* thickness, colour and style: between bits, every n bits, between rows,
* above groups, at segment edges, and around the whole.
*   INTERACTION posts events and changes nothing itself. A click on a name
* selects its row; on a group's arrow, opens or closes it; on a segment's
* heading, collapses or expands it; on a cell, edits the row's bit. A drag
* across a row's cells defines a range, or paints a literal's bits. A
* double-click on a name, or on a segment's heading, renames it where it is
* shown, in a field over it; a right-click opens the menu for the row, the
* segment or the sheet under the mouse.
*   COLUMNS are resized by their edges, which light up under the mouse: the
* name and value columns' by a drag of the edge after them, which a
* double-click fits to their text; the bits' cells, all at once, by a drag of
* any boundary in the ruler, which a double-click puts back to the default.
* The grid fills its panel: the room it has beyond what its columns need
* goes to the bits' cells, widened alike, or to the name column, as the
* sheet says -- or to nothing, and then the rows end where the columns do.
* What a column or a heading cannot show for want of room -- a name or a
* value cut short, a run of bits shown as one column, a band's name -- the
* mouse shows in full.
*
* path:      /src/djinterp/tools/bitwisedom/app/grid.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "./elements.hpp"  // corresponding header

// std
#include <algorithm>  // std::max, std::min
#include <cmath>      // std::fabs, std::floor, std::sqrt
#include <cstddef>    // std::size_t
#include <string>     // std::string
#include <vector>     // std::vector
// imgui
#include <imgui.h>                  // ImGui, ImDrawList
#include <imgui_internal.h>         // the window's inner rectangle
#include <misc/cpp/imgui_stdlib.h>  // InputText over std::string
// bitwisedom
#include "./theme.hpp"  // to_u32, mix, mono_font


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::option_set;
using uxoxo::platform::imgui_frame;

// column
//   struct: one displayed column -- a bit, or a run of bits shown as one.
struct column
{
    int   lsb       = 0;
    int   msb       = 0;
    bool  collapsed = false;   // a collapsed segment
    bool  unused    = false;   // an abbreviated run nothing uses
    int   segment   = -1;      // index into the layout's segments
    float x         = 0.0f;    // from the cells' left edge
    float w         = 0.0f;
};

// band
//   struct: one segment as the heading shows it.
struct band
{
    int         lsb       = 0;
    int         msb       = 0;
    int         user      = -1;   // the sheet's segment, or -1: automatic
    bool        collapsed = false;
    std::string name;
    rgba        colour    = 0;
};

// row_kind_shown
//   enum: what a grid row is.
enum class shown
{
    probe,
    sheet,
    usage
};

// grid_row
//   struct: one displayed row.
struct grid_row
{
    shown               kind     = shown::sheet;
    const resolved_row* resolved = nullptr;
    const row*          source   = nullptr;
};

// stroke
//   struct: a press-and-drag across one row's cells, from press to release.
struct stroke
{
    bool active = false;
    int  row    = 0;
    int  row_to = 0;
    bool rectangle = false;
    int  from   = -1;
    int  to     = -1;
    bool set    = true;
};

// resize
//   struct: a drag of the name or value column's right edge.
struct resize
{
    bool        active = false;
    std::string column;
};

// widening
//   struct: a drag of a boundary in the ruler, which sizes every bit's cell
// at once: the boundary, the cells' width when the drag began, where the
// mouse was then, and how many cells lie left of the boundary -- the ones
// the mouse's movement is shared between.
struct widening
{
    bool  active   = false;
    int   boundary = -1;
    float width    = 0.0f;   // the sheet's, unscaled
    float from     = 0.0f;
    int   cells    = 1;
    float last     = 0.0f;   // the width last asked for
};

// band_click
//   struct: the band the last press in the heading collapsed or expanded,
// so a double-click can put it back as it was before renaming it. An
// automatic band the press collapsed became the sheet's segment `created`.
struct band_click
{
    bool   valid   = false;
    double time    = 0.0;
    long   user    = -1;
    long   created = -1;
    int    lsb     = 0;
    int    msb     = 0;
};

// draft
//   struct: a rename being typed in the grid: whose, and the text so far.
struct draft
{
    int         row     = 0;
    int         segment = -1;
    std::string text;
    bool        focus   = false;   // the field is to take the keyboard
    bool        live    = false;   // the field has had the keyboard
};

// spot
//   struct: what lies under a point of the grid.
struct spot
{
    bool header   = false;   // in the heading
    bool band     = false;   // in the heading's band strip
    bool ruler    = false;   // in the heading's ruler
    bool names    = false;   // in the pinned name and value columns
    bool name     = false;   // in the name column itself
    int  row      = -1;      // the row, or -1
    int  column   = -1;      // the column, or -1
    int  edge     = 0;       // 1: the name column's edge; 2: the value's
    int  boundary = -1;      // a ruler boundary: the column left of it
};

// EDGE_REACH
//   constant: how near an edge or a boundary the mouse must be to drag it,
// at a scale of one.
const float EDGE_REACH = 4.0f;

// layout
//   struct: the grid's geometry for one frame.
struct layout
{
    std::vector<column>   columns;
    std::vector<band>     bands;
    std::vector<grid_row> rows;
    float name_w   = 0.0f;
    float value_w  = 0.0f;
    float cells_w  = 0.0f;
    float cell_w   = 0.0f;   // a bit's cell, as it is drawn
    float room     = 0.0f;   // the width the grid was laid out to fill
    float band_h   = 0.0f;
    float ruler_h  = 0.0f;
    float header_h = 0.0f;
    float row_h    = 0.0f;
};

stroke&
the_stroke()
{
    static stroke state;

    return state;
}

resize&
the_resize()
{
    static resize state;

    return state;
}

widening&
the_widening()
{
    static widening state;

    return state;
}

band_click&
the_band_click()
{
    static band_click state;

    return state;
}

draft&
the_draft()
{
    static draft state;

    return state;
}

// rest
//   struct: what the mouse rests on in the grid's pinned columns or its
// heading, and since when: their tooltips wait for a rest, as ImGui's own
// do, where a cell's shows at once.
struct rest
{
    long   key   = -1;
    double since = 0.0;
};

rest&
the_rest()
{
    static rest state;

    return state;
}

// rested
//   whether the mouse has rested on what _key names long enough for its
// tooltip; -1 is nothing, and starts the wait again.
bool
rested(
    long _key
)
{
    rest&        held = the_rest();
    const double now  = ImGui::GetTime();

    if (held.key != _key)
    {
        held.key   = _key;
        held.since = now;
    }

    return ( (_key >= 0) &&
             (now - held.since >= static_cast<double>(
                                      ImGui::GetStyle().HoverDelayNormal)) );
}

// sighting
//   struct: where the last frame drew the grid's parts, for grid_spot.
struct sighting
{
    bool                drawn    = false;
    ImVec2              origin;
    ImVec2              view_min;
    float               name_w   = 0.0f;
    float               value_w  = 0.0f;
    float               band_h   = 0.0f;
    float               ruler_h  = 0.0f;
    float               header_h = 0.0f;
    float               row_h    = 0.0f;
    float               end      = 0.0f;   // where the columns end
    float               room     = 0.0f;   // the width they were laid out to
    std::vector<column> columns;
    std::vector<int>    ids;     // each row's; 0 for the probe and usage
    std::vector<float>  names;   // where each row's name begins
};

sighting&
the_sighting()
{
    static sighting state;

    return state;
}

// any_in / count_in
//   whether a value has a bit set in [_lsb, _msb], and how many: what a
// column shows of a row, in a sheet of any width.
bool
any_in(
    const wide& _value,
    int         _lsb,
    int         _msb
)
{
    const int first = _value.next(_lsb);

    return ( (first >= 0) &&
             (first <= _msb) );
}

int
count_in(
    const wide& _value,
    int         _lsb,
    int         _msb
)
{
    int count = 0;

    for (int b = _value.next(_lsb);
         (b >= 0) && (b <= _msb);
         b = _value.next(b + 1))
    {
        ++count;
    }

    return count;
}

// bit_label
//   a bit's number as the sheet's numbering names it.
int
bit_label(
    const sheet& _sheet,
    int          _bit
)
{
    return (_sheet.bits == numbering::lsb0) ? _bit
                                            : (_sheet.width - 1 - _bit);
}

// byte_label
//   the byte a bit is in, as the sheet's byte numbering names it.
int
byte_label(
    const sheet& _sheet,
    int          _bit
)
{
    return (_sheet.bytes == byte_order::little)
               ? (_bit / 8)
               : ((_sheet.width - 1 - _bit) / 8);
}

// kind_colour
//   the colour a definition kind is drawn in, in the "kind" colour mode.
rgba
kind_colour(
    def_kind _kind
)
{
    switch (_kind)
    {
        case def_kind::bit:         return 0x4C8DFFFFu;
        case def_kind::range:       return 0x2EC4B6FFu;
        case def_kind::field_value: return 0xF2A541FFu;
        case def_kind::literal:     return 0xB388EBFFu;
        case def_kind::expression:  return 0xF25F5CFFu;
        case def_kind::macro:       return 0x8D99AEFFu;
    }

    return 0x808080FFu;
}

// set_colour
//   the colour a row's set bits are drawn in.
rgba
set_colour(
    const model&        _model,
    const resolved_row& _row
)
{
    const colour_settings& colours = _model.shown.colours;
    const row* source = find_row(_model.doc, _row.id);
    if ((source != nullptr) && (source->transparent))
    {
        return 0;
    }

    switch (colours.mode)
    {
        case colour_mode::group:
        {
            const resolved_row* parent = _model.res.find(_row.parent);

            return (parent != nullptr) ? parent->colour : _row.colour;
        }

        case colour_mode::kind:
            return kind_colour(_row.def);

        case colour_mode::accent:
            return colours.accent;

        case colour_mode::row:
            break;
    }

    return _row.colour;
}

// initials
//   the first letter of each underscore-separated part: PARITY_EVEN, PE.
std::string
initials(
    const std::string& _name
)
{
    std::string out;
    bool        start = true;

    for (char each : _name)
    {
        if (each == '_')
        {
            start = true;

            continue;
        }

        if (start)
        {
            out += each;
            start = false;
        }
    }

    return out;
}

// fit
//   the longest of a name's forms that fits a width: the name, its short
// form, its initials, or as much of it as fits with ".." after. A wide
// sheet's values run to thousands of digits, so the cut is found by
// halving: a longer start is never the narrower.
std::string
fit(
    const std::string& _name,
    const std::string& _short,
    float              _width
)
{
    if (ImGui::CalcTextSize(_name.c_str()).x <= _width)
    {
        return _name;
    }

    if ( (!_short.empty()) &&
         (ImGui::CalcTextSize(_short.c_str()).x <= _width) )
    {
        return _short;
    }

    const std::string first = initials(_name);

    if ( (first.size() > 1) &&
         (first.size() < _name.size()) &&
         (ImGui::CalcTextSize(first.c_str()).x <= _width) &&
         (_width < ImGui::CalcTextSize("MMMMMM").x) )
    {
        return first;
    }

    // cut
    //   the name's first _length characters, and "..".
    auto cut = [&_name](std::size_t _length)
    {
        return _name.substr(0, _length) + "..";
    };

    if (ImGui::CalcTextSize("..").x > _width)
    {
        return std::string();
    }

    // the longest start that fits lies in [fits, too_long)
    std::size_t fits     = 0;
    std::size_t too_long = _name.size();

    while (too_long - fits > 1)
    {
        const std::size_t middle = fits + (too_long - fits) / 2;

        if (ImGui::CalcTextSize(cut(middle).c_str()).x <= _width)
        {
            fits = middle;
        }
        else
        {
            too_long = middle;
        }
    }

    return cut(fits);
}

// shown_value
//   a value as the value column writes it: in its row's radix, with the
// radix's C prefix.
std::string
shown_value(
    u64   _value,
    radix _radix,
    int   _width
)
{
    switch (_radix)
    {
        case radix::dec:
            return format_dec(_value);

        case radix::oct:
            return (_value == 0) ? "0"
                                 : "0" + group_digits(format_oct(_value), 3);

        case radix::bin:
            return "0b" + format_bin(_value, _width, 8, ' ');

        default:
            break;
    }

    return "0x" + group_digits(
        format_hex(_value, (_width + 3) / 4, true), 2);
}

// shown_wide
//   a value as the value column writes it in a sheet of any width: as the
// narrow form where it fits an integer, else as its runs -- a wide sheet's
// hex is mostly zeros, and says less than where its bits are.
std::string
shown_wide(
    const wide& _value,
    radix       _radix,
    int         _width
)
{
    if (!is_wide(_width))
    {
        return shown_value(_value.low(), _radix, _width);
    }

    return runs_text(_value, 2);
}

// draw_line
//   one grid line, solid, dashed or dotted.
void
draw_line(
    ImDrawList*      _list,
    ImVec2           _a,
    ImVec2           _b,
    const line_spec& _spec
)
{
    const ImU32 colour = to_u32(_spec.colour);

    if (_spec.style == line_style::solid)
    {
        _list->AddLine(_a, _b, colour, _spec.thickness);

        return;
    }

    const float dx     = _b.x - _a.x;
    const float dy     = _b.y - _a.y;
    const float length = std::sqrt(dx * dx + dy * dy);

    if (length <= 0.0f)
    {
        return;
    }

    const float on   = (_spec.style == line_style::dashed)
                           ? 5.0f
                           : std::max(1.0f, _spec.thickness);
    const float gap  = (_spec.style == line_style::dashed) ? 4.0f : 3.0f;
    const float ux   = dx / length;
    const float uy   = dy / length;

    // the stretch of the line the clip rectangle shows, as distances along
    // it: a line down a sheet of a thousand rows is thousands of dashes, of
    // which a window shows a few dozen. The dashes keep their places,
    // counted from the line's start, so they do not crawl as it scrolls.
    const float  margin = _spec.thickness + 1.0f;
    const ImVec2 low(_list->GetClipRectMin().x - margin,
                     _list->GetClipRectMin().y - margin);
    const ImVec2 high(_list->GetClipRectMax().x + margin,
                      _list->GetClipRectMax().y + margin);
    float        first  = 0.0f;
    float        last   = length;

    // narrow
    //   the stretch, narrowed to where one coordinate is within its span;
    // false when nothing of the line is left.
    auto narrow = [&first, &last](float _start,
                                  float _step,
                                  float _min,
                                  float _max)
    {
        // a line that does not move in this coordinate is in or out whole
        if (std::fabs(_step) < 1e-6f)
        {
            return ( (_start >= _min) &&
                     (_start <= _max) );
        }

        const float enter = (_min - _start) / _step;
        const float leave = (_max - _start) / _step;

        first = std::max(first, std::min(enter, leave));
        last  = std::min(last, std::max(enter, leave));

        return (first < last);
    };

    if ( (!narrow(_a.x, ux, low.x, high.x)) ||
         (!narrow(_a.y, uy, low.y, high.y)) )
    {
        return;
    }

    const float period = on + gap;

    // a dash or dot at a time along the line, from the first one shown
    for (float t = std::floor(first / period) * period;
         t < last;
         t += period)
    {
        const float end = std::min(length, t + on);

        _list->AddLine(ImVec2(_a.x + ux * t, _a.y + uy * t),
                       ImVec2(_a.x + ux * end, _a.y + uy * end),
                       colour,
                       _spec.thickness);
    }

    return;
}

// band_index
//   struct: the bands made so far, by the sheet's segment and by automatic
// block, so that each of thousands of bits finds its band at once.
struct band_index
{
    std::vector<int> user;    // the band of each of the sheet's segments
    std::vector<int> block;   // the band of each automatic block
};

// band_for
//   the band a bit lies in: the first of the sheet's segments that holds
// it, else the automatic one, else none (-1).
int
band_for(
    const sheet&       _sheet,
    std::vector<band>& _bands,
    band_index&        _index,
    int                _bit
)
{
    for (std::size_t i = 0; i < _sheet.segments.size(); ++i)
    {
        const column_segment& each = _sheet.segments[i];
        const int             low  = std::min(each.lsb, each.msb);
        const int             high = std::max(each.lsb, each.msb);

        if ( (_bit < low) ||
             (_bit > high) )
        {
            continue;
        }

        if (_index.user.size() < _sheet.segments.size())
        {
            _index.user.assign(_sheet.segments.size(), -1);
        }

        if (_index.user[i] >= 0)
        {
            return _index.user[i];
        }

        band made;
        made.lsb       = std::max(0, low);
        made.msb       = std::min(_sheet.width - 1, high);
        made.user      = static_cast<int>(i);
        made.collapsed = each.collapsed;
        made.name      = each.name;
        made.colour    = each.colour;
        _bands.push_back(made);
        _index.user[i] = static_cast<int>(_bands.size()) - 1;

        return _index.user[i];
    }

    const int every = _sheet.grid.auto_segments;

    if (every <= 0)
    {
        return -1;
    }

    const int        low   = (_bit / every) * every;
    const int        high  = std::min(_sheet.width - 1, low + every - 1);
    const std::size_t block = static_cast<std::size_t>(_bit / every);

    if (_index.block.size() <= block)
    {
        _index.block.resize(block + 1, -1);
    }

    if (_index.block[block] >= 0)
    {
        return _index.block[block];
    }

    _index.block[block] = static_cast<int>(_bands.size());

    band made;
    made.lsb  = low;
    made.msb  = high;
    made.name = (every == 8)
                    ? "byte " + format_dec(static_cast<u64>(
                                    byte_label(_sheet, low)))
                    : format_dec(static_cast<u64>(bit_label(_sheet, high))) +
                          ":" +
                          format_dec(static_cast<u64>(bit_label(_sheet,
                                                                low)));
    _bands.push_back(made);

    return static_cast<int>(_bands.size()) - 1;
}

// lay_out
//   the frame's columns and rows, filling _room as the sheet says.
layout
lay_out(
    const model& _model,
    float        _room
)
{
    const sheet&         doc  = _model.doc;
    const grid_settings& grid = doc.grid;
    layout               out;
    const float          line = ImGui::GetTextLineHeight();

    const float scale = ui_scale();

    out.row_h   = std::max(grid.cell_height * scale, line + 4.0f);
    out.name_w  = grid.name_width * scale;
    out.value_w = grid.value_width * scale;
    out.room    = _room;

    // the rows: the probe, the tree as it stands open, the usage
    if (grid.show_probe)
    {
        out.rows.push_back(grid_row{ shown::probe, nullptr, nullptr });
    }

    // the bits something shown uses -- worked out only when unused runs
    // are abbreviated, which is all it is for
    wide            used = grid.show_probe ? doc.probe : wide();
    const row_index rows(doc);

    for (const resolved_row& each : _model.res.rows)
    {
        if (!each.visible)
        {
            continue;
        }

        out.rows.push_back(grid_row{ shown::sheet,
                                     &each,
                                     rows.find(each.id) });

        if (!grid.abbreviate_unused)
        {
            continue;
        }

        if (each.has_value)
        {
            used |= each.bits;
        }

        // a value's whole field counts as used
        if (each.span_width > 0)
        {
            used |= wide::run(each.span_lsb, each.span_width);
        }
    }

    if (grid.show_usage)
    {
        out.rows.push_back(grid_row{ shown::usage, nullptr, nullptr });
    }

    // the columns, in display order
    std::vector<column> raw;
    std::vector<int>    owner(static_cast<std::size_t>(doc.width), -1);
    band_index          index;

    for (int i = 0; i < doc.width; ++i)
    {
        const int bit = doc.msb_left ? (doc.width - 1 - i) : i;

        owner[static_cast<std::size_t>(bit)] =
            band_for(doc, out.bands, index, bit);
    }

    for (int i = 0; i < doc.width; ++i)
    {
        const int bit   = doc.msb_left ? (doc.width - 1 - i) : i;
        const int which = owner[static_cast<std::size_t>(bit)];

        // a collapsed band is one column, met at its first bit
        if ( (which >= 0) &&
             (out.bands[static_cast<std::size_t>(which)].collapsed) )
        {
            if ( (!raw.empty()) &&
                 (raw.back().segment == which) )
            {
                raw.back().lsb = std::min(raw.back().lsb, bit);
                raw.back().msb = std::max(raw.back().msb, bit);

                continue;
            }

            column made;
            made.lsb       = bit;
            made.msb       = bit;
            made.collapsed = true;
            made.segment   = which;
            raw.push_back(made);

            continue;
        }

        column made;
        made.lsb     = bit;
        made.msb     = bit;
        made.segment = which;
        raw.push_back(made);
    }

    // runs nothing uses, outside the sheet's own segments, abbreviated
    std::vector<column>& columns = out.columns;

    for (std::size_t i = 0; i < raw.size(); )
    {
        const column& first = raw[i];
        const bool    open  =
            ( (grid.abbreviate_unused) &&
              (!first.collapsed)       &&
              ( (first.segment < 0) ||
                (out.bands[static_cast<std::size_t>(first.segment)].user <
                 0) ) &&
              (!used.test(first.lsb)) );

        if (!open)
        {
            columns.push_back(first);
            ++i;

            continue;
        }

        std::size_t end = i;

        while ( (end < raw.size())                                  &&
                (!raw[end].collapsed)                               &&
                (raw[end].segment == first.segment)                 &&
                (!used.test(raw[end].lsb)) )
        {
            ++end;
        }

        if (static_cast<int>(end - i) >= grid.unused_run)
        {
            column made = first;
            made.unused = true;

            for (std::size_t k = i; k < end; ++k)
            {
                made.lsb = std::min(made.lsb, raw[k].lsb);
                made.msb = std::max(made.msb, raw[k].msb);
            }

            columns.push_back(made);
        }
        else
        {
            for (std::size_t k = i; k < end; ++k)
            {
                columns.push_back(raw[k]);
            }
        }

        i = end;
    }

    // widths: a bit is a cell; a run is wide enough for its label
    for (column& each : columns)
    {
        if ( (each.collapsed) ||
             (each.unused) )
        {
            // wide enough for the ruler's "31:28" and for the digest
            const int         digits = (each.msb - each.lsb + 4) / 4;
            const std::string widest(static_cast<std::size_t>(digits), 'F');
            const std::string span   =
                format_dec(static_cast<u64>(each.msb)) + ":" +
                format_dec(static_cast<u64>(each.lsb));
            const float       ruler  =
                ImGui::CalcTextSize(span.c_str()).x * 0.82f + 10.0f;

            each.w = std::max({ grid.cell_width * scale * 1.6f,
                                ImGui::CalcTextSize(widest.c_str()).x + 10.0f,
                                ruler });
        }
        else
        {
            each.w = grid.cell_width * scale;
        }
    }

    // the room to spare: the cells take it in proportion, to whole pixels,
    // the last ending at the grid's edge; or the name column takes it all
    float natural = 0.0f;

    for (const column& each : columns)
    {
        natural += each.w;
    }

    const float spare = std::floor(_room - out.name_w - out.value_w) -
                        natural;

    out.cell_w = grid.cell_width * scale;

    if ( (spare >= 1.0f)   &&
         (natural > 0.0f) )
    {
        if (grid.fill == grid_fill::cells)
        {
            const float target = natural + spare;
            float       sum    = 0.0f;
            float       left   = 0.0f;

            for (std::size_t i = 0; i < columns.size(); ++i)
            {
                sum += columns[i].w;

                const float right = (i + 1 == columns.size())
                                        ? target
                                        : std::floor(sum * target / natural);

                columns[i].w = right - left;
                left         = right;
            }

            out.cell_w = std::floor(out.cell_w * target / natural);
        }
        else if (grid.fill == grid_fill::names)
        {
            out.name_w += spare;
        }
    }

    float x = 0.0f;

    for (column& each : columns)
    {
        each.x = x;
        x     += each.w;
    }

    out.cells_w  = x;
    out.band_h   = out.bands.empty() ? 0.0f : (line + 6.0f);
    out.ruler_h  = grid.show_indices ? (line + 4.0f) : 0.0f;
    out.header_h = std::max(out.band_h + out.ruler_h, line + 6.0f);

    return out;
}

// column_at
//   the column under an x measured from the cells' left edge, or -1.
int
column_at(
    const layout& _layout,
    float         _x
)
{
    // the columns lie left to right: the first whose right edge is past _x
    const auto found = std::upper_bound(
        _layout.columns.begin(),
        _layout.columns.end(),
        _x,
        [](float _at, const column& _each)
        {
            return (_at < _each.x + _each.w);
        });

    if (found != _layout.columns.end())
    {
        const column& each = *found;

        if ( (_x >= each.x) &&
             (_x < each.x + each.w) )
        {
            return static_cast<int>(found - _layout.columns.begin());
        }
    }

    return -1;
}

// cell_text
//   what a collapsed column shows of a value.
std::string
cell_text(
    const model&  _model,
    const column& _column,
    const wide&   _value
)
{
    if (_column.unused)
    {
        return "..";
    }

    const int span = _column.msb - _column.lsb + 1;

    switch (_model.doc.grid.collapsed)
    {
        case collapsed_style::hex:
            return format_hex(_value.slice(_column.lsb, span),
                              (span + 3) / 4,
                              true);

        case collapsed_style::dots:
            return any_in(_value, _column.lsb, _column.msb) ? "*" : "";

        case collapsed_style::count:
        {
            const int set = count_in(_value, _column.lsb, _column.msb);

            return (set > 0) ? format_dec(static_cast<u64>(set))
                             : "";
        }

        case collapsed_style::blank:
            break;
    }

    return std::string();
}

// preview_after
//   a row's value after the previewed operation, if it is a target.
bool
preview_after(
    const model& _model,
    int          _id,
    wide&        _after
)
{
    // the preview shows while the operations page is in front
    if ( (!_model.ui.op_preview)   ||
         (!_model.preview_valid)   ||
         (_model.ui.tab != 0) )
    {
        return false;
    }

    for (const op_row& each : _model.preview.rows)
    {
        if (each.id == _id)
        {
            _after = each.after;

            return true;
        }
    }

    return false;
}

// painter
//   class: the drawing of one frame, with what every part of it needs.
class painter
{
public:
    painter(
        model&        _model,
        imgui_frame&  _frame,
        const layout& _layout
    )
        : m(_model),
          f(_frame),
          l(_layout),
          doc(_model.doc),
          grid(_model.shown.grid),
          colours(_model.shown.colours),
          list(ImGui::GetWindowDrawList())
    {}

    void run();

private:
    model&                 m;
    imgui_frame&           f;
    const layout&          l;
    const sheet&           doc;
    const grid_settings&   grid;
    const colour_settings& colours;
    ImDrawList*            list;
    ImVec2                 origin;     // the content's top-left, scrolled
    ImVec2                 view_min;   // the visible area
    ImVec2                 view_max;
    float                  cells_left = 0.0f;   // pinned edge of the cells
    float                  rows_top   = 0.0f;   // pinned edge of the rows

    // the columns in view, [first, last); and the first that is one bit's
    // cell, which the ruler's boundaries size
    std::size_t            first_column = 0;
    std::size_t            last_column  = 0;
    std::size_t            first_plain  = 0;

    // what the mouse is over before the drawing, so the edges can light
    int                    edge_hot     = 0;
    int                    boundary_hot = -1;

    // the grid's button, as this frame left it
    bool                   over    = false;   // the mouse is over it
    bool                   pressed = false;   // a press began on it
    bool                   twice   = false;   // ... and made a double-click
    bool                   held    = false;   // it is being pressed

    float cells_x(float _x) const
    {
        return origin.x + l.name_w + l.value_w + _x;
    }

    float row_y(std::size_t _index) const
    {
        return origin.y + l.header_h + static_cast<float>(_index) * l.row_h;
    }

    ImU32 text_colour() const
    {
        return ImGui::GetColorU32(ImGuiCol_Text);
    }

    ImU32 muted_colour() const
    {
        return ImGui::GetColorU32(ImGuiCol_TextDisabled);
    }

    void centred_text(
        ImVec2             _min,
        ImVec2             _max,
        const std::string& _text,
        ImU32              _colour
    )
    {
        const ImVec2 size = ImGui::CalcTextSize(_text.c_str());

        list->AddText(ImVec2(std::floor((_min.x + _max.x - size.x) * 0.5f),
                             std::floor((_min.y + _max.y - size.y) * 0.5f)),
                      _colour,
                      _text.c_str());

        return;
    }

    // where things are, and what they say
    spot        at(ImVec2 _point) const;
    float       name_left(const resolved_row& _row) const;
    std::string shown_name(const resolved_row& _row) const;
    std::string value_text(const resolved_row& _row) const;
    std::string probe_text() const;
    float       fit_names() const;
    float       fit_values() const;
    int         cells_before(int _boundary) const;
    bool        renaming_here() const;
    bool        text_row(const grid_row& _row) const;

    void rows_background();
    void row_cells(std::size_t _index, const grid_row& _row);
    void row_label(std::size_t _index, const grid_row& _row);
    void lines();
    void header();
    void interact();
    void respond();
    void open_menu(const spot& _under);
    bool field_place(const draft& _typed, ImVec2& _min, ImVec2& _max);
    void rename_field();
    void tooltip(const grid_row& _row, const column& _column);
    void pinned_tooltip(const grid_row& _row, const spot& _under);
    void heading_tooltip(const spot& _under, ImVec2 _mouse);
    std::string definition(const resolved_row& _row) const;
    void remember() const;
};

/*
painter::at
  What lies under a point. The pinned columns' edges are found anywhere
down the grid, the nearer one when the two are close; a ruler boundary
only in the ruler, at a column's right edge with a cell left of it.
*/
spot
painter::at(
    ImVec2 _point
) const
{
    spot        out;
    const float top = view_min.y;

    out.header = (_point.y < top + l.header_h);
    out.band   = ( (out.header) &&
                   (_point.y < top + l.band_h) );
    out.ruler  = ( (out.header)                         &&
                   (l.ruler_h > 0.0f)                   &&
                   (_point.y >= top + l.band_h)         &&
                   (_point.y < top + l.band_h + l.ruler_h) );
    out.names  = (_point.x < cells_left);
    out.name   = (_point.x < view_min.x + l.name_w);

    const int row = static_cast<int>(
        std::floor((_point.y - origin.y - l.header_h) / l.row_h));

    if ( (!out.header) &&
         (row >= 0)    &&
         (row < static_cast<int>(l.rows.size())) )
    {
        out.row = row;
    }

    if (!out.names)
    {
        out.column = column_at(l, _point.x - cells_x(0.0f));
    }

    // the pinned columns' edges
    const float reach      = EDGE_REACH * ui_scale();
    const float name_edge  = view_min.x + l.name_w;
    const float value_edge = name_edge + l.value_w;
    const float to_name    = std::fabs(_point.x - name_edge);
    const float to_value   = std::fabs(_point.x - value_edge);

    if ( (l.value_w > 0.0f)    &&
         (to_value < reach)    &&
         (to_value <= to_name) )
    {
        out.edge = 2;
    }
    else if (to_name < reach)
    {
        out.edge = 1;
    }

    // a boundary of the ruler's
    if ( (out.ruler) &&
         (out.edge == 0) )
    {
        for (std::size_t i = first_column; i < last_column; ++i)
        {
            const float x = cells_x(l.columns[i].x + l.columns[i].w);

            if ( (x >= cells_left) &&
                 (std::fabs(_point.x - x) < reach) &&
                 (i >= first_plain) )
            {
                out.boundary = static_cast<int>(i);

                break;
            }
        }
    }

    return out;
}

// painter::name_left
//   where a row's name begins: past its indent, a group's arrow, and its
// colour chip.
float
painter::name_left(
    const resolved_row& _row
) const
{
    const float line = ImGui::GetTextLineHeight();
    float       x    = view_min.x + 6.0f +
                       static_cast<float>(_row.depth) * ui_scale() *
                           grid.indent;

    const row* source = find_row(doc, _row.id);
    if ((source != nullptr) && can_contain(*source))
    {
        x += line * 0.32f * 2.0f + 6.0f;
    }

    return x + line * 0.55f + 6.0f;
}

// painter::shown_name
//   a row's name as the name column writes it, before it is fitted.
std::string
painter::shown_name(
    const resolved_row& _row
) const
{
    const std::string name =
        (_row.kind == row_kind::group)
            ? (_row.local.empty() ? std::string("group") : _row.local)
            : (grid.full_names ? _row.full : _row.local);

    return name.empty() ? std::string("(unnamed)") : name;
}

// painter::value_text
//   a row's value as the value column writes it, before it is fitted.
std::string
painter::value_text(
    const resolved_row& _row
) const
{
    if ( (_row.def == def_kind::field_value) &&
         (_row.kind == row_kind::symbol) )
    {
        return "= " + format_signed(_row.number);
    }

    if ( (_row.def == def_kind::literal) &&
         (_row.negative) &&
         (!is_wide(doc.width)) )
    {
        return format_signed(sign_extend(_row.value, doc.width));
    }

    // a wide sheet's macro is C's integer
    if ( (_row.def == def_kind::macro) &&
         (_row.kind == row_kind::symbol) &&
         (is_wide(doc.width)) )
    {
        return "0x" + group_digits(format_hex(_row.value, 0, true), 2);
    }

    return shown_wide(_row.bits, _row.fmt, doc.width);
}

// painter::probe_text
//   the probe's value as the value column writes it.
std::string
painter::probe_text() const
{
    wide probe = doc.probe;

    probe.trim(doc.width);

    return shown_wide(probe,
                      (doc.gen.base == radix::inherit) ? radix::hex
                                                       : doc.gen.base,
                      doc.width);
}

/*
painter::fit_names
  The name column's width for its longest name, as the rows are now: indent,
arrow and chip included.
*/
float
painter::fit_names() const
{
    float widest = ImGui::CalcTextSize("symbol").x + 12.0f;

    for (const grid_row& each : l.rows)
    {
        float width = 0.0f;

        switch (each.kind)
        {
            case shown::probe:
                width = 6.0f + ImGui::CalcTextSize("probe").x;
                break;

            case shown::usage:
                width = 6.0f + ImGui::CalcTextSize("allocated").x;
                break;

            case shown::sheet:
                width = name_left(*each.resolved) - view_min.x +
                        ImGui::CalcTextSize(
                            shown_name(*each.resolved).c_str()).x;
                break;
        }

        widest = std::max(widest, width + 12.0f);
    }

    return widest;
}

// painter::fit_values
//   the value column's width for its longest value.
float
painter::fit_values() const
{
    float widest = ImGui::CalcTextSize("value").x + 12.0f;

    ImGui::PushFont(mono_font(), 0.0f);

    for (const grid_row& each : l.rows)
    {
        std::string text;

        if (each.kind == shown::probe)
        {
            text = probe_text();
        }
        else if ( (each.kind == shown::sheet) &&
                  (each.resolved->has_value) )
        {
            text = value_text(*each.resolved);
        }

        if (!text.empty())
        {
            widest = std::max(widest,
                              ImGui::CalcTextSize(text.c_str()).x + 12.0f);
        }
    }

    ImGui::PopFont();

    // the usage row's summary, in the interface's font
    const std::string summary = format_dec(static_cast<u64>(doc.width)) +
                                " of " +
                                format_dec(static_cast<u64>(doc.width));

    return std::max(widest,
                    ImGui::CalcTextSize(summary.c_str()).x + 12.0f);
}

// painter::cells_before
//   how many cells -- whole bits, not runs -- lie left of a boundary.
int
painter::cells_before(
    int _boundary
) const
{
    int count = 0;

    for (int i = 0;
         ( (i <= _boundary) &&
           (i < static_cast<int>(l.columns.size())) );
         ++i)
    {
        const column& each = l.columns[static_cast<std::size_t>(i)];

        if ( (!each.collapsed) &&
             (!each.unused) )
        {
            ++count;
        }
    }

    return count;
}

// painter::renaming_here
//   whether a rename is being typed in the grid, rather than the tree.
bool
painter::renaming_here() const
{
    return ( (m.ui.rename_where == "grid") &&
             ( (m.ui.renaming != 0) ||
               (m.ui.renaming_segment >= 0) ) );
}

// painter::text_row
//   whether a row writes text across its cells instead of bits: a macro
// that takes arguments, one with no value, or a row with an error.
bool
painter::text_row(
    const grid_row& _row
) const
{
    if (_row.kind != shown::sheet)
    {
        return false;
    }

    const resolved_row& r = *_row.resolved;

    if ( (r.kind == row_kind::symbol) &&
         (r.def == def_kind::macro)   &&
         ( (function_like(*_row.source)) ||
           (!r.has_value) ) )
    {
        return true;
    }

    return !r.error.empty();
}

void
painter::rows_background()
{
    // the rows end where the columns do
    const float right = std::min(view_max.x, cells_x(l.cells_w));

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const grid_row& each = l.rows[i];
        const float     top  = row_y(i);

        // only the rows in view
        if ( (top + l.row_h < rows_top) ||
             (top > view_max.y) )
        {
            continue;
        }

        if ( (each.kind == shown::sheet) &&
             (is_selected(m, each.resolved->id)) )
        {
            list->AddRectFilled(ImVec2(view_min.x, top),
                                ImVec2(right, top + l.row_h),
                                to_u32(colours.selection));
        }
        else if ( (each.kind == shown::sheet) &&
                  (m.ui.hover_row == each.resolved->id) )
        {
            list->AddRectFilled(ImVec2(view_min.x, top),
                                ImVec2(right, top + l.row_h),
                                to_u32(colours.hover));
        }
        else if ( (each.kind == shown::sheet) &&
                  (each.resolved->kind == row_kind::group) )
        {
            list->AddRectFilled(ImVec2(view_min.x, top),
                                ImVec2(right, top + l.row_h),
                                to_u32(with_alpha(colours.hover,
                                                  channel(colours.hover, 3) *
                                                      0.6f)));
        }
    }

    return;
}

/*
painter::row_cells
  A cell's fill says whether its bit is set; a range's or a value's field is
tinted underneath, so a value's zeros still show where it lives. A shared
bit in an exclusive group is outlined in the collision colour, and a
previewed operation's result is outlined over the present value.
*/
void
painter::row_cells(
    std::size_t     _index,
    const grid_row& _row
)
{
    const float top    = row_y(_index);
    const float bottom = top + l.row_h;
    const float gap    = grid.show_cell_boxes ? (grid.cell_gap * 0.5f)
                                               : 0.0f;
    const float round  = grid.show_cell_boxes ? grid.rounding : 0.0f;
    const ImU32 empty  = to_u32(colours.empty_cell);

    wide        value;
    int         tint_low  = 0;   // the field a range or a value lives in
    int         tint_high = -1;
    const wide  none;
    const wide* overlap   = &none;
    rgba        colour    = colours.accent;
    bool        has       = false;
    bool        union_of  = false;
    wide        after;
    bool        ghost     = false;

    switch (_row.kind)
    {
        case shown::probe:
            value  = doc.probe;
            value.trim(doc.width);
            colour = colours.probe;
            has    = true;

            if ( (m.ui.op_preview)   &&
                 (m.preview_valid)   &&
                 (m.ui.tab == 0)     &&
                 (m.preview.probe) )
            {
                after = m.preview.probe_after;
                ghost = true;
            }

            break;

        case shown::usage:
            break;

        case shown::sheet:
        {
            const resolved_row& r = *_row.resolved;

            value     = r.bits;
            has       = r.has_value;
            colour    = set_colour(m, r);
            overlap   = &r.overlap;
            union_of  = (r.kind == row_kind::group);
            tint_low  = r.span_lsb;
            tint_high = (r.span_width > 0) ? (r.span_lsb + r.span_width - 1)
                                           : -1;
            ghost     = preview_after(m, r.id, after);

            // a macro that takes arguments has no bits: its signature runs
            // across them. An object-like one with a value is drawn as any
            // value is; one without shows what it expands to, or why not.
            if ( (r.kind == row_kind::symbol) &&
                 (r.def == def_kind::macro)   &&
                 ( (function_like(*_row.source)) ||
                   (!r.has_value) ) )
            {
                std::string text;
                ImU32       tone = muted_colour();

                if (function_like(*_row.source))
                {
                    text = "f  " + r.full + "(" + _row.source->params + ")";
                }
                else if (!r.error.empty())
                {
                    text = "! " + r.error;
                    tone = to_u32(colours.collision);
                }
                else
                {
                    const std::string& body = r.expansion.empty()
                                                  ? _row.source->body
                                                  : r.expansion;

                    text = "= " + body.substr(0, body.find('\n'));
                }

                list->AddText(ImVec2(cells_x(4.0f),
                                     top + (l.row_h -
                                            ImGui::GetTextLineHeight()) *
                                               0.5f),
                              tone,
                              text.c_str());

                return;
            }

            if (!r.error.empty())
            {
                list->AddText(ImVec2(cells_x(4.0f),
                                     top + (l.row_h -
                                            ImGui::GetTextLineHeight()) *
                                               0.5f),
                              to_u32(colours.collision),
                              ("! " + r.error).c_str());

                return;
            }

            break;
        }
    }

    const bool   transparent = (colour == 0);
    const ImU32 fill   = to_u32(with_alpha(colour, union_of
                                                       ? colours.set_alpha *
                                                             0.45f
                                                       : colours.set_alpha));
    const ImU32 under  = to_u32(with_alpha(colour, colours.range_alpha));
    const ImU32 ink    = (colours.theme == 1) ? IM_COL32(20, 20, 24, 255)
                                              : IM_COL32(250, 250, 252, 255);

    for (std::size_t i = first_column; i < last_column; ++i)
    {
        const column& each = l.columns[i];
        const ImVec2  min(cells_x(each.x) + gap, top + gap);
        const ImVec2  max(cells_x(each.x + each.w) - gap, bottom - gap);

        if (_row.kind == shown::usage)
        {
            int most = 0;

            for (int b = each.lsb; b <= each.msb; ++b)
            {
                most = std::max(most,
                                m.res.usage.empty()
                                    ? 0
                                    : m.res.usage[static_cast<std::size_t>(b)]);
            }

            const ImU32 heat =
                (most == 0) ? empty
              : (most == 1) ? to_u32(with_alpha(colours.accent, 0.45f))
                            : to_u32(colours.collision);

            list->AddRectFilled(min, max, heat, round);

            if (most > 0)
            {
                centred_text(min, max, format_dec(static_cast<u64>(most)),
                             ink);
            }

            continue;
        }

        if (grid.show_cell_boxes)
        {
            list->AddRectFilled(min, max, empty, round);
        }

        if ( (!transparent) &&
             (each.lsb <= tint_high) &&
             (each.msb >= tint_low) )
        {
            list->AddRectFilled(min, max, under, round);
        }

        const bool set = ( (has) &&
                           (any_in(value, each.lsb, each.msb)) );

        if ((set) && (!transparent))
        {
            list->AddRectFilled(min, max, fill, round);
        }

        // An explicitly selected rectangular block is highlighted even
        // when its cells are empty or the row has no fill.
        if ((each.lsb <= m.ui.grid_rect_high) &&
            (each.msb >= m.ui.grid_rect_low) &&
            (_row.kind == shown::sheet) &&
            (std::find(m.ui.grid_rect_rows.begin(),
                       m.ui.grid_rect_rows.end(),
                       _row.resolved->id) != m.ui.grid_rect_rows.end()))
        {
            list->AddRectFilled(min, max,
                 to_u32(with_alpha(colours.selection, 0.25f)), round);
            list->AddRect(min, max, to_u32(colours.accent), round, 0, 1.2f);
        }

        // a collapsed or abbreviated column shows a digest of its bits
        if ( (each.collapsed) ||
             (each.unused) )
        {
            centred_text(min, max, cell_text(m, each, has ? value : none),
                         set ? ink : muted_colour());
        }
        else if (grid.show_digits)
        {
            centred_text(min, max, set ? "1" : "0",
                         set ? ink : muted_colour());
        }

        if (any_in(*overlap, each.lsb, each.msb))
        {
            list->AddRect(min, max, to_u32(colours.collision), round, 0,
                          2.0f);
        }

        if ( (ghost) &&
             (any_in(after, each.lsb, each.msb)) )
        {
            list->AddRect(ImVec2(min.x + 2.0f, min.y + 2.0f),
                          ImVec2(max.x - 2.0f, max.y - 2.0f),
                          to_u32(colours.ghost),
                          round,
                          0,
                          2.0f);
        }
    }

    // a range's name, or a value's number, across its span
    if ( (grid.show_labels)                          &&
         (_row.kind == shown::sheet)                 &&
         (_row.resolved->span_width > 1)             &&
         ( (_row.resolved->def == def_kind::range) ||
           (_row.resolved->def == def_kind::field_value) ) )
    {
        const resolved_row& r     = *_row.resolved;
        float               left  = 1.0e9f;
        float               right = -1.0e9f;

        const int high = r.span_lsb + r.span_width - 1;

        for (std::size_t i = first_column; i < last_column; ++i)
        {
            const column& each = l.columns[i];

            if ( (each.lsb <= high) &&
                 (each.msb >= r.span_lsb) )
            {
                left  = std::min(left, cells_x(each.x));
                right = std::max(right, cells_x(each.x + each.w));
            }
        }

        const std::string text =
            (r.def == def_kind::range)
                ? fit(grid.full_names ? r.full : r.local,
                      _row.source->short_name,
                      right - left - 8.0f)
                : fit(format_signed(r.number), "", right - left - 8.0f);

        if ( (!text.empty()) &&
             (right > left) )
        {
            const ImVec2 size = ImGui::CalcTextSize(text.c_str());
            const ImVec2 at(std::floor((left + right - size.x) * 0.5f),
                            std::floor(top + (l.row_h - size.y) * 0.5f));

            list->AddRectFilled(ImVec2(at.x - 3.0f, at.y),
                                ImVec2(at.x + size.x + 3.0f, at.y + size.y),
                                to_u32(with_alpha(colour, 0.85f)),
                                3.0f);
            list->AddText(at, ink, text.c_str());
        }
    }

    return;
}

void
painter::row_label(
    std::size_t     _index,
    const grid_row& _row
)
{
    const float top  = row_y(_index);
    const float text = top + (l.row_h - ImGui::GetTextLineHeight()) * 0.5f;
    const float left = view_min.x + 6.0f;

    if (_row.kind == shown::probe)
    {
        list->AddText(ImVec2(left, text), to_u32(colours.probe), "probe");

        if (l.value_w > 0.0f)
        {
            ImGui::PushFont(mono_font(), 0.0f);
            list->AddText(ImVec2(view_min.x + l.name_w + 4.0f, text),
                          text_colour(),
                          fit(probe_text(), "", l.value_w - 8.0f).c_str());
            ImGui::PopFont();
        }

        return;
    }

    if (_row.kind == shown::usage)
    {
        list->AddText(ImVec2(left, text), muted_colour(), "allocated");

        if (l.value_w > 0.0f)
        {
            const std::string summary =
                format_dec(static_cast<u64>(m.res.allocated.count())) +
                " of " + format_dec(static_cast<u64>(doc.width));

            list->AddText(ImVec2(view_min.x + l.name_w + 4.0f, text),
                          muted_colour(),
                          summary.c_str());
        }

        return;
    }

    const resolved_row& r      = *_row.resolved;
    const float         indent = left + static_cast<float>(r.depth) *
                                            ui_scale() * grid.indent;
    const float         x      = name_left(r);

    // a group's arrow
    if (can_contain(*_row.source))
    {
        const float  size   = ImGui::GetTextLineHeight() * 0.32f;
        const ImVec2 centre(indent + size, top + l.row_h * 0.5f);
        const bool   open   = !_row.source->collapsed;

        const float lean = size * 0.6f;
        const float tip  = size * 0.8f;

        if (open)
        {
            list->AddTriangleFilled(ImVec2(centre.x - size, centre.y - lean),
                                    ImVec2(centre.x + size, centre.y - lean),
                                    ImVec2(centre.x, centre.y + tip),
                                    text_colour());
        }
        else
        {
            list->AddTriangleFilled(ImVec2(centre.x - lean, centre.y - size),
                                    ImVec2(centre.x - lean, centre.y + size),
                                    ImVec2(centre.x + tip, centre.y),
                                    text_colour());
        }
    }

    // the row's colour, as a chip before the name
    const float chip = ImGui::GetTextLineHeight() * 0.55f;
    const bool  chip_off = (_row.source != nullptr) && _row.source->transparent;

    if (!chip_off)
    {
        list->AddRectFilled(
            ImVec2(x - chip - 6.0f, top + (l.row_h - chip) * 0.5f),
            ImVec2(x - 6.0f, top + (l.row_h + chip) * 0.5f),
            to_u32(r.colour),
            2.0f);
    }

    // the name, unless a rename is being typed over it
    if ( (m.ui.renaming != r.id) ||
         (!renaming_here()) )
    {
        const std::string name = fit(shown_name(r),
                                     _row.source->short_name,
                                     view_min.x + l.name_w - x - 6.0f);

        list->AddText(ImVec2(x, text),
                      (!r.error.empty()) ? to_u32(colours.collision)
                                         : text_colour(),
                      name.c_str());
    }

    if ( (l.value_w <= 0.0f) ||
         (!r.has_value) )
    {
        return;
    }

    ImGui::PushFont(mono_font(), 0.0f);

    const std::string fitted = fit(value_text(r), "", l.value_w - 8.0f);
    const ImVec2      size   = ImGui::CalcTextSize(fitted.c_str());

    list->AddText(ImVec2(view_min.x + l.name_w + l.value_w - size.x - 6.0f,
                         text),
                  (r.kind == row_kind::group) ? muted_colour()
                                              : text_colour(),
                  fitted.c_str());
    ImGui::PopFont();

    return;
}

/*
painter::lines
  Vertical lines first, at each boundary between two columns: a segment
edge, else a major line, else a minor one -- one line per boundary, the most
important kind that is switched on. They do not cross a row that writes
text across its cells, which has no cells to divide. Then the horizontal
lines between rows, heavier above groups, and the border last.
*/
void
painter::lines()
{
    if (!grid.lines)
    {
        return;
    }

    const float top    = rows_top - l.ruler_h;
    const float bottom = row_y(l.rows.size());

    // the runs of rows the vertical lines pass through
    std::vector<ImVec2> runs;   // x: where one starts, y: where it ends
    float               start = top - l.band_h;

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        if (text_row(l.rows[i]))
        {
            if (row_y(i) > start)
            {
                runs.push_back(ImVec2(start, row_y(i)));
            }

            start = row_y(i) + l.row_h;
        }
    }

    if (bottom > start)
    {
        runs.push_back(ImVec2(start, bottom));
    }

    // vertical
    //   one boundary's line, from its kind's top down, through the runs.
    auto vertical = [&](float            _x,
                        float            _from,
                        const line_spec& _spec)
    {
        for (const ImVec2& run : runs)
        {
            const float from = std::max(run.x, _from);

            if (run.y > from)
            {
                draw_line(list, ImVec2(_x, from), ImVec2(_x, run.y), _spec);
            }
        }

        return;
    };

    for (std::size_t i = std::max<std::size_t>(1, first_column);
         i < last_column;
         ++i)
    {
        const column& left  = l.columns[i - 1];
        const column& right = l.columns[i];
        const float   x     = cells_x(right.x);

        if ( (x < cells_left) ||
             (x > view_max.x) )
        {
            continue;
        }

        // the bits above the boundary start at its higher side's lowest
        const int  edge    = std::max(left.lsb, right.lsb);
        const bool segment = (left.segment != right.segment);
        const bool major   = ( (grid.major_every > 0) &&
                               ((edge % grid.major_every) == 0) );

        if ( (segment) &&
             (grid.segments.show) )
        {
            vertical(x, top - l.band_h, grid.segments);
        }
        else if ( (major) &&
                  (grid.major.show) )
        {
            vertical(x, top, grid.major);
        }
        else if (grid.minor.show)
        {
            vertical(x, rows_top, grid.minor);
        }
    }

    const float right = cells_x(l.cells_w);

    for (std::size_t i = 1; i < l.rows.size(); ++i)
    {
        const float y = row_y(i);

        if ( (y < rows_top) ||
             (y > view_max.y) )
        {
            continue;
        }

        const grid_row& each  = l.rows[i];
        const bool      group = ( (each.kind == shown::sheet) &&
                                  (each.resolved->kind == row_kind::group) );

        if ( (group) &&
             (grid.groups.show) )
        {
            draw_line(list, ImVec2(view_min.x, y), ImVec2(right, y),
                      grid.groups);
        }
        else if (grid.rows.show)
        {
            draw_line(list, ImVec2(view_min.x, y), ImVec2(right, y),
                      grid.rows);
        }
    }

    if (grid.border.show)
    {
        const ImVec2 min(cells_x(0.0f), rows_top);
        const ImVec2 max(right, std::min(bottom, view_max.y));

        draw_line(list, min, ImVec2(max.x, min.y), grid.border);
        draw_line(list, ImVec2(max.x, min.y), max, grid.border);
        draw_line(list, ImVec2(min.x, max.y), max, grid.border);
        draw_line(list, min, ImVec2(min.x, max.y), grid.border);
    }

    return;
}

void
painter::header()
{
    const ImU32 background = ImGui::GetColorU32(ImGuiCol_ChildBg);
    const float top        = view_min.y;

    // the heading is pinned: it covers the rows scrolled beneath it
    list->AddRectFilled(ImVec2(view_min.x, top),
                        ImVec2(view_max.x, top + l.header_h),
                        background);

    // the bands: each run of columns with one segment, from the start of
    // the first one in view
    std::size_t start = first_column;

    while ( (start > 0)                                  &&
            (start < l.columns.size())                   &&
            (l.columns[start].segment >= 0)              &&
            (l.columns[start - 1].segment ==
             l.columns[start].segment) )
    {
        --start;
    }

    for (std::size_t i = start; i < last_column; )
    {
        const int   which = l.columns[i].segment;
        std::size_t end   = i;

        while ( (end < l.columns.size()) &&
                (l.columns[end].segment == which) )
        {
            ++end;
        }

        if (which >= 0)
        {
            const band&  each = l.bands[static_cast<std::size_t>(which)];
            const rgba   tone = (each.colour != 0) ? each.colour
                                                   : colours.accent;
            const ImVec2 min(cells_x(l.columns[i].x) + 1.0f, top + 2.0f);
            const ImVec2 max(cells_x(l.columns[end - 1].x +
                                     l.columns[end - 1].w) - 1.0f,
                             top + l.band_h - 2.0f);

            list->AddRectFilled(min, max,
                                to_u32(with_alpha(tone,
                                                  each.user >= 0 ? 0.40f
                                                                 : 0.16f)),
                                4.0f);

            // its name, unless a rename is being typed over it
            if ( (!renaming_here()) ||
                 (each.user < 0)    ||
                 (each.user != m.ui.renaming_segment) )
            {
                const std::string label =
                    (each.collapsed ? "+ " : "") +
                    fit(each.name, "", max.x - min.x - 16.0f);

                centred_text(min, max, label, text_colour());
            }
        }

        i = end;
    }

    // the ruler: bit numbers, or a run's span
    if (grid.show_indices)
    {
        const float rtop = top + l.band_h;
        int         every = std::max(1, grid.index_every);

        // numbers wider than a cell -- a wide sheet's -- are thinned to
        // every second, fourth, ... bit, so they never run together
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 0.82f);

        const float widest =
            ImGui::CalcTextSize(format_dec(static_cast<u64>(
                                    std::max(bit_label(doc, 0),
                                             bit_label(doc,
                                                       doc.width - 1))))
                                    .c_str()).x + 6.0f;

        ImGui::PopFont();

        while ( (l.cell_w > 0.0f) &&
                (widest > l.cell_w * static_cast<float>(every)) &&
                (every < 4096) )
        {
            every *= 2;
        }

        for (std::size_t i = first_column; i < last_column; ++i)
        {
            const column& each = l.columns[i];
            const ImVec2  min(cells_x(each.x), rtop);
            const ImVec2  max(cells_x(each.x + each.w), rtop + l.ruler_h);

            const bool hovered = ( (m.ui.hover_bit >= each.lsb) &&
                                   (m.ui.hover_bit <= each.msb) );

            if (hovered)
            {
                list->AddRectFilled(min, max, to_u32(colours.hover), 3.0f);
            }

            std::string label;

            if ( (each.collapsed) ||
                 (each.unused) )
            {
                label = format_dec(static_cast<u64>(
                            bit_label(doc, doc.msb_left ? each.msb
                                                        : each.lsb))) +
                        ":" +
                        format_dec(static_cast<u64>(
                            bit_label(doc, doc.msb_left ? each.lsb
                                                        : each.msb)));
                label = fit(label, "", each.w - 2.0f);
            }
            else if ((bit_label(doc, each.lsb) % every) == 0)
            {
                label = format_dec(static_cast<u64>(bit_label(doc,
                                                              each.lsb)));
            }

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 0.82f);
            centred_text(min, max, label,
                         hovered ? text_colour() : muted_colour());
            ImGui::PopFont();
        }

        // the boundary a drag sizes the cells by, lit
        const widening& widen = the_widening();
        const int       lit   = widen.active ? widen.boundary : boundary_hot;

        if ( (lit >= 0) &&
             (static_cast<std::size_t>(lit) < l.columns.size()) )
        {
            const column& each = l.columns[static_cast<std::size_t>(lit)];
            const float   x    = cells_x(each.x + each.w);

            list->AddLine(ImVec2(x, rtop),
                          ImVec2(x, rtop + l.ruler_h),
                          ImGui::GetColorU32(widen.active
                                                 ? ImGuiCol_SeparatorActive
                                                 : ImGuiCol_SeparatorHovered),
                          2.0f);
        }
    }

    // the corner, over both pinned parts
    list->AddRectFilled(view_min,
                        ImVec2(view_min.x + l.name_w + l.value_w,
                               top + l.header_h),
                        background);
    list->AddText(ImVec2(view_min.x + 6.0f,
                         top + l.header_h - ImGui::GetTextLineHeight() - 3.0f),
                  muted_colour(),
                  "symbol");

    if (l.value_w > 0.0f)
    {
        list->AddText(ImVec2(view_min.x + l.name_w + 4.0f,
                             top + l.header_h -
                                 ImGui::GetTextLineHeight() - 3.0f),
                      muted_colour(),
                      "value");
    }

    // the edges a drag resizes, lit while the mouse is on one or drags it
    const resize&   sizing = the_resize();
    const widening& widen  = the_widening();

    // edge
    //   one of the pinned columns' right edges: 1 the name's, 2 the value's.
    auto edge = [&](int   _which,
                    float _x)
    {
        const bool dragged = ( (sizing.active) &&
                               (sizing.column == ((_which == 2) ? "value"
                                                                : "name")) );
        const bool lit     = ( (dragged) ||
                               ( (edge_hot == _which) &&
                                 (!sizing.active)     &&
                                 (!widen.active) ) );

        list->AddLine(ImVec2(_x, view_min.y),
                      ImVec2(_x, view_max.y),
                      ImGui::GetColorU32(dragged ? ImGuiCol_SeparatorActive
                                       : lit     ? ImGuiCol_SeparatorHovered
                                                 : ImGuiCol_Separator),
                      lit ? 2.0f : 1.0f);

        return;
    };

    edge(1, view_min.x + l.name_w);
    edge(2, view_min.x + l.name_w + l.value_w);

    return;
}

void
painter::tooltip(
    const grid_row& _row,
    const column&   _column
)
{
    std::string text;

    if ( (_column.collapsed) ||
         (_column.unused) )
    {
        text = "bits " +
               format_dec(static_cast<u64>(bit_label(doc, _column.lsb))) +
               ".." +
               format_dec(static_cast<u64>(bit_label(doc, _column.msb))) +
               (_column.unused ? " -- unused, abbreviated"
                               : " -- a collapsed segment");
    }
    else
    {
        const int bit = _column.lsb;

        text = "bit " + format_dec(static_cast<u64>(bit_label(doc, bit))) +
               "  (byte " +
               format_dec(static_cast<u64>(byte_label(doc, bit))) + ")";

        // who claims this bit
        std::string claims;

        for (const resolved_row& each : m.res.rows)
        {
            if ( (each.kind == row_kind::symbol)          &&
                 (each.has_value)                         &&
                 ( (each.def == def_kind::bit) ||
                   (each.def == def_kind::range) )        &&
                 (each.bits.test(bit)) )
            {
                claims += (claims.empty() ? "" : ", ") + each.full;
            }
        }

        if (!claims.empty())
        {
            text += "\nallocated to " + claims;
        }
    }

    if ( (_row.kind == shown::sheet) &&
         (_row.resolved->has_value) )
    {
        const resolved_row& r = *_row.resolved;

        text += "\n" + r.full + " = " +
                (is_wide(doc.width)
                     ? runs_text(r.bits, 6)
                     : "0x" + group_digits(
                           format_hex(r.value, (doc.width + 3) / 4, true), 2));

        // a macro's value has a C type, and an expansion
        if (!r.ctype.empty())
        {
            text += "  (" + r.ctype + ")";
        }

        if (!r.expansion.empty())
        {
            text += "\nexpands to " +
                    ( (r.expansion.size() > 96)
                          ? r.expansion.substr(0, 94) + ".."
                          : r.expansion );
        }
    }

    ImGui::SetTooltip("%s", text.c_str());

    return;
}

/*
painter::definition
  What a row is, in a line: a bit's number, a range's bits, a value's number
and field, a literal, an expression or a macro.
*/
std::string
painter::definition(
    const resolved_row& _row
) const
{
    if (_row.kind == row_kind::group)
    {
        return "a group of " + format_dec(static_cast<u64>(_row.children)) +
               ((_row.children == 1) ? " row" : " rows");
    }

    const row* source = find_row(doc, _row.id);
    const int  low    = _row.span_lsb;
    const int  high   = _row.span_lsb + _row.span_width - 1;

    // bits
    //   a span by the sheet's numbering, high first.
    auto bits = [this, low, high]()
    {
        return (low == high)
                   ? "bit " + format_dec(static_cast<u64>(
                                  bit_label(doc, low)))
                   : "bits " + format_dec(static_cast<u64>(
                                   bit_label(doc, high))) + ".." +
                         format_dec(static_cast<u64>(bit_label(doc, low))) +
                         " (" + format_dec(static_cast<u64>(high - low + 1)) +
                         " bits)";
    };

    switch (_row.def)
    {
        case def_kind::bit:
            return (low >= 0) ? "a bit: " + bits() : "a bit";

        case def_kind::range:
            return (low >= 0) ? "a range: " + bits() : "a range";

        case def_kind::field_value:
        {
            const resolved_row* field = m.res.find(_row.field);

            return "the value " + format_signed(_row.number) + " in " +
                   ((field != nullptr) ? field->full : bits());
        }

        case def_kind::literal:
            return _row.negative ? "a literal, negative" : "a literal";

        case def_kind::expression:
            return "an expression: " +
                   ((source != nullptr) ? source->expr : std::string());

        case def_kind::macro:
            return "a macro" +
                   ( (source != nullptr) &&
                     ( (source->callable) ||
                       (!source->params.empty()) )
                         ? "(" + source->params + ")"
                         : std::string() ) +
                   ( _row.ctype.empty() ? std::string()
                                        : ", " + _row.ctype );
    }

    return std::string();
}

/*
painter::pinned_tooltip
  The name column's tooltip, once the mouse rests on a name: the macro name
in full, what the row is, and its brief; the value column's, the value in
every base, in the monospace font. The probe and the usage rows say what
they are.
*/
void
painter::pinned_tooltip(
    const grid_row& _row,
    const spot&     _under
)
{
    const int  index = _under.row;
    const long key   = 1000000L + static_cast<long>(index) * 2L +
                       (_under.name ? 0L : 1L);

    if (!rested(key))
    {
        return;
    }

    if (_row.kind == shown::probe)
    {
        ImGui::SetTooltip("the probe: a scratch value, %s\n"
                          "click its cells to flip bits; the operations "
                          "act on it when asked",
                          probe_text().c_str());

        return;
    }

    if (_row.kind == shown::usage)
    {
        ImGui::SetTooltip("usage: how many symbols allocate each bit -- "
                          "more than one in an exclusive group collides");

        return;
    }

    const resolved_row& r = *_row.resolved;

    if (_under.name)
    {
        std::string text = r.full.empty() ? shown_name(r) : r.full;

        text += "\n" + definition(r);

        if ( (_row.source != nullptr) &&
             (!_row.source->note.empty()) )
        {
            text += "\n\"" + _row.source->note + "\"";
        }

        if (!r.error.empty())
        {
            text += "\n" + r.error;
        }

        ImGui::SetTooltip("%s", text.c_str());

        return;
    }

    if (!r.has_value)
    {
        return;
    }

    ImGui::BeginTooltip();
    ImGui::PushFont(mono_font(), 0.0f);

    if (is_wide(doc.width))
    {
        // a set of bits: how many, where, and in hex, 128 bits a line
        const std::string hex = format_hex(r.bits, 0, true);
        const std::size_t line = 32;

        ImGui::Text("%d of %d bits set", r.bits.count(), doc.width);
        ImGui::TextUnformatted(runs_text(r.bits, 8).c_str());

        for (std::size_t at = 0; at < hex.size(); at += line)
        {
            ImGui::TextUnformatted(
                group_digits(hex.substr(at, line), 2).c_str());

            if (at >= line * 15)
            {
                ImGui::TextUnformatted("...");

                break;
            }
        }
    }
    else
    {
        // the value in every base
        const u64 value = r.value & width_mask(doc.width);

        ImGui::Text("hex      0x%s", group_digits(
            format_hex(value, (doc.width + 3) / 4, true), 2).c_str());
        ImGui::Text("unsigned %s", format_dec(value).c_str());

        if ((value >> (doc.width - 1)) & 1u)
        {
            ImGui::Text("signed   %s",
                        format_signed(sign_extend(value, doc.width)).c_str());
        }

        ImGui::Text("binary   %s",
                    format_bin(value, doc.width, 8, ' ').c_str());
    }

    ImGui::PopFont();

    if (!r.expansion.empty())
    {
        ImGui::TextUnformatted(("expands to " +
                                ( (r.expansion.size() > 96)
                                      ? r.expansion.substr(0, 94) + ".."
                                      : r.expansion )).c_str());
    }

    ImGui::EndTooltip();

    return;
}

/*
painter::heading_tooltip
  A band's tooltip says which bits it holds and what a click does to it; a
place in the ruler, the bit or the run of bits below it -- whose number
the ruler may have had no room to write -- and how the cells are sized.
*/
void
painter::heading_tooltip(
    const spot& _under,
    ImVec2      _mouse
)
{
    const int at = (_under.names) ? -1
                                  : column_at(l, _mouse.x - cells_x(0.0f));

    if ( (_under.names) &&
         (_under.ruler || _under.band || _under.header) )
    {
        if (rested(2000000L))
        {
            ImGui::SetTooltip("drag a column's right edge to size it; "
                              "double-click the edge to fit it to its "
                              "text");
        }

        return;
    }

    if (at < 0)
    {
        (void)rested(-1);

        return;
    }

    const column& each = l.columns[static_cast<std::size_t>(at)];
    const long    key  = 3000000L + static_cast<long>(at) * 2L +
                         (_under.band ? 0L : 1L);

    if (!rested(key))
    {
        return;
    }

    // span
    //   a run of bits by the sheet's numbering, high first.
    auto span = [this](int _low,
                       int _high)
    {
        return "bits " +
               format_dec(static_cast<u64>(bit_label(doc, _high))) + ".." +
               format_dec(static_cast<u64>(bit_label(doc, _low))) + " (" +
               format_dec(static_cast<u64>(_high - _low + 1)) + " bits)";
    };

    if ( (_under.band) &&
         (each.segment >= 0) )
    {
        const band& owner = l.bands[static_cast<std::size_t>(each.segment)];
        std::string text  = (owner.name.empty() ? std::string("a segment")
                                                : owner.name) +
                            ": " + span(owner.lsb, owner.msb);

        text += owner.collapsed
                    ? "\ncollapsed to one column: click to expand it"
                    : "\nclick to collapse it to one column";
        text += (owner.user >= 0)
                    ? "; double-click to rename it; right-click for more"
                    : "; double-click to name it as a segment of its own";

        ImGui::SetTooltip("%s", text.c_str());

        return;
    }

    std::string text;

    if ( (each.collapsed) ||
         (each.unused) )
    {
        text = span(each.lsb, each.msb) +
               (each.unused ? "\nunused, and abbreviated to one column"
                            : "\na collapsed segment");
    }
    else
    {
        text = "bit " + format_dec(static_cast<u64>(bit_label(doc,
                                                              each.lsb))) +
               ", in byte " +
               format_dec(static_cast<u64>(byte_label(doc, each.lsb)));
    }

    text += "\ndrag a boundary to size every cell; double-click one to "
            "put them back";

    ImGui::SetTooltip("%s", text.c_str());

    return;
}

/*
painter::interact
  One invisible button covers the content, and the view where the content
is smaller, so ImGui's own idea of hovered, pressed and dragged applies to
the grid as to any widget -- and a press beside the content, after a band
collapsed from under the mouse, still lands on the grid. What the button
reports is read once, before anything else is drawn. While a rename is
typed in the grid, the button lets the field drawn after it lie over it.
*/
void
painter::interact()
{
    ImGui::SetCursorScreenPos(origin);

    const ImVec2 room = ImGui::GetContentRegionAvail();

    if (renaming_here())
    {
        ImGui::SetNextItemAllowOverlap();
    }

    // as wide as the columns, and a margin when they overflow the view;
    // columns laid out to fill it fit it exactly, and scroll no way
    const float wide = l.name_w + l.value_w + l.cells_w;

    ImGui::InvisibleButton("##grid",
                           ImVec2(std::max({ 1.0f,
                                             room.x,
                                             (wide <= l.room + 0.5f)
                                                 ? wide
                                                 : wide + 8.0f }),
                                  std::max({ 1.0f,
                                             room.y,
                                             l.header_h +
                                                 static_cast<float>(
                                                     l.rows.size()) *
                                                     l.row_h + 8.0f })));

    over    = ImGui::IsItemHovered();
    pressed = ImGui::IsItemActivated();
    held    = ImGui::IsItemActive();
    twice   = ( (pressed) &&
                (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) );

    respond();
    rename_field();

    return;
}

/*
painter::respond
  Where a press lands decides what it does. An edge or a ruler boundary
starts a resize, and a double-click on one fits its column or puts the
cells back; a band collapses or expands, and a double-click renames it; a
name selects its row, an arrow opens or closes its group, and a
double-click on a name renames it; a cell starts a stroke, which a release
turns into one event. A right-click anywhere opens the menu for what it is
on.
*/
void
painter::respond()
{
    const ImVec2 mouse  = ImGui::GetMousePos();
    const spot   under  = at(mouse);
    stroke&      drag   = the_stroke();
    resize&      sizing = the_resize();
    widening&    widen  = the_widening();

    // a sheet just imported, or taken from the examples, has its name and
    // value columns fitted to its names and values
    if (m.ui.fit_columns)
    {
        option_set names;
        option_set values;

        names.set("target", std::string("name"));
        names.set("value", static_cast<double>(fit_names() / ui_scale()));
        names.set("arriving", true);
        f.post("grid.width", std::move(names));

        values.set("target", std::string("value"));
        values.set("value", static_cast<double>(fit_values() / ui_scale()));
        values.set("arriving", true);
        f.post("grid.width", std::move(values));
    }

    // a right-click opens the menu for what it is on
    if ( (over) &&
         (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) )
    {
        open_menu(under);

        return;
    }

    // the cursor says when a press would resize
    if ( (sizing.active) ||
         (widen.active)  ||
         ( (over) &&
           ( (under.edge != 0) || (under.boundary >= 0) ) ) )
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    // a press on an edge resizes its column; a double-click fits it
    if ( (pressed) &&
         (under.edge != 0) )
    {
        sizing.active = true;
        sizing.column = (under.edge == 2) ? "value" : "name";

        if (twice)
        {
            const float fitted = (under.edge == 2) ? fit_values()
                                                   : fit_names();
            option_set  payload;

            payload.set("target", sizing.column);
            payload.set("value", static_cast<double>(fitted / ui_scale()));
            f.post("grid.width", std::move(payload));
        }
    }
    // a press on a ruler boundary sizes the cells; a double-click resets
    else if ( (pressed) &&
              (under.boundary >= 0) )
    {
        widen.active   = true;
        widen.boundary = under.boundary;
        widen.width    = std::max(grid.cell_width, l.cell_w / ui_scale());
        widen.from     = mouse.x;
        widen.cells    = std::max(1, cells_before(under.boundary));
        widen.last     = widen.width;

        if (twice)
        {
            option_set payload;

            widen.width = grid_settings{}.cell_width;
            widen.last  = widen.width;
            payload.set("target", std::string("cells"));
            payload.set("value", static_cast<double>(widen.width));
            f.post("grid.width", std::move(payload));
        }
    }

    if (sizing.active)
    {
        const float delta = ImGui::GetIO().MouseDelta.x;

        // the sheet's widths are unscaled
        if (delta != 0.0f)
        {
            option_set payload;
            payload.set("value", static_cast<double>(delta / ui_scale()));
            payload.set("target", sizing.column);
            f.post("grid.resize", std::move(payload));
        }

        if (!held)
        {
            sizing.active = false;
        }

        return;
    }

    if (widen.active)
    {
        // the boundary follows the mouse: its movement, shared by the cells
        // left of it, and unscaled
        const float width = widen.width +
                            (mouse.x - widen.from) /
                                (static_cast<float>(widen.cells) *
                                 ui_scale());

        if (std::fabs(width - widen.last) >= 0.05f)
        {
            option_set payload;
            payload.set("target", std::string("cells"));
            payload.set("value", static_cast<double>(width));
            f.post("grid.width", std::move(payload));
            widen.last = width;
        }

        if (!held)
        {
            widen.active = false;
        }

        return;
    }

    // hovering: the row and the bit under the mouse
    if ( (over) &&
         (under.row >= 0) )
    {
        const grid_row& each = l.rows[static_cast<std::size_t>(under.row)];

        if (each.kind == shown::sheet)
        {
            m.ui.hover_row = each.resolved->id;
        }

        if ( (!under.names)      &&
             (under.column >= 0) &&
             (under.edge == 0) )
        {
            const column& below = l.columns[static_cast<std::size_t>(
                under.column)];

            m.ui.hover_bit = below.lsb;
            tooltip(each, below);
        }
        else if ( (under.names) &&
                  (under.edge == 0) )
        {
            pinned_tooltip(each, under);
        }
    }
    else if ( (over)         &&
              (under.header) &&
              (under.edge == 0) )
    {
        heading_tooltip(under, mouse);
    }
    else
    {
        (void)rested(-1);
    }

    // the second press of a double-click on a band puts back what the
    // first did, and renames the band -- found by the first press, since
    // the band it collapsed or expanded has moved the columns under the
    // mouse since
    band_click& last = the_band_click();

    if ( (pressed)       &&
         (twice)         &&
         (last.valid)    &&
         (under.header)  &&
         (!under.names)  &&
         (ImGui::GetTime() - last.time <=
          static_cast<double>(ImGui::GetIO().MouseDoubleClickTime) + 0.05) )
    {
        option_set payload;

        payload.set("value", (last.user >= 0) ? last.user : last.created);
        payload.set("lsb", static_cast<long>(last.lsb));
        payload.set("msb", static_cast<long>(last.msb));
        f.post("grid.segment", payload);
        f.post("rename.segment", std::move(payload));
        last.valid = false;

        return;
    }

    // a press on a band collapses or expands it
    if ( (pressed)            &&
         (under.band)         &&
         (!under.names)       &&
         (under.column >= 0) )
    {
        const column& below = l.columns[static_cast<std::size_t>(
            under.column)];

        if (below.segment >= 0)
        {
            const band& each = l.bands[static_cast<std::size_t>(
                below.segment)];
            option_set  payload;

            payload.set("value", static_cast<long>(each.user));
            payload.set("lsb", static_cast<long>(each.lsb));
            payload.set("msb", static_cast<long>(each.msb));
            f.post("grid.segment", std::move(payload));

            last.valid   = true;
            last.time    = ImGui::GetTime();
            last.user    = each.user;
            last.created = (each.user < 0)
                               ? static_cast<long>(doc.segments.size())
                               : -1L;
            last.lsb     = each.lsb;
            last.msb     = each.msb;
        }

        return;
    }

    if ( (pressed) &&
         (under.row >= 0) )
    {
        const grid_row& each = l.rows[static_cast<std::size_t>(under.row)];

        // the name column: an arrow opens or closes, a double-click on the
        // name renames, the rest selects
        if (under.names)
        {
            if (each.kind != shown::sheet)
            {
                return;
            }

            const resolved_row& r     = *each.resolved;
            const float         arrow = view_min.x + 6.0f +
                                        static_cast<float>(r.depth) *
                                            ui_scale() * grid.indent +
                                        ImGui::GetTextLineHeight() * 0.8f;
            option_set          payload;

            payload.set("value", static_cast<long>(r.id));

            if ( (can_contain(*each.source)) &&
                 (mouse.x < arrow + 4.0f) )
            {
                payload.set("open", each.source->collapsed);
                f.post("toggle_open", std::move(payload));
            }
            else if ( (twice) &&
                      (under.name) )
            {
                f.post("rename.grid", std::move(payload));
            }
            else
            {
                f.post("select", std::move(payload));
            }

            return;
        }

        if (under.column < 0)
        {
            return;
        }

        const column& below = l.columns[static_cast<std::size_t>(
            under.column)];

        if ( (below.collapsed) ||
             (below.unused) )
        {
            return;
        }

        if (each.kind == shown::probe)
        {
            option_set payload;
            payload.set("value", static_cast<long>(below.lsb));
            f.post("grid.probe", std::move(payload));

            return;
        }

        if (each.kind != shown::sheet)
        {
            return;
        }

        const resolved_row& r = *each.resolved;

        drag.active = true;
        drag.row    = r.id;
        drag.row_to = r.id;
        drag.rectangle = ImGui::GetIO().KeyShift;
        drag.from   = below.lsb;
        drag.to     = below.lsb;
        drag.set    = !( (r.has_value) &&
                         (r.bits.test(below.lsb)) );

        if ((!drag.rectangle) && (!is_selected(m, r.id)))
        {
            option_set payload;
            payload.set("value", static_cast<long>(r.id));
            f.post("select", std::move(payload));
        }

        return;
    }

    if (!drag.active)
    {
        return;
    }

    // A rectangle follows the cursor's row as well as its column.
    if ((drag.rectangle) &&
        (under.row >= 0) &&
        (l.rows[static_cast<std::size_t>(under.row)].kind == shown::sheet))
    {
        drag.row_to = l.rows[static_cast<std::size_t>(under.row)].resolved->id;
    }

    // a stroke follows the mouse along its row, and ends on release
    if ( (under.column >= 0) &&
         (!l.columns[static_cast<std::size_t>(under.column)].collapsed) &&
         (!l.columns[static_cast<std::size_t>(under.column)].unused) )
    {
        drag.to = l.columns[static_cast<std::size_t>(under.column)].lsb;
    }

    // the stroke's extent, drawn while it lasts
    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        if (l.rows[i].kind != shown::sheet)
        {
            continue;
        }
        if (drag.rectangle)
        {
            int first = -1;
            int last = -1;
            for (std::size_t k = 0; k < l.rows.size(); ++k)
            {
                if (l.rows[k].kind != shown::sheet) continue;
                if (l.rows[k].resolved->id == drag.row)
                    first = static_cast<int>(k);
                if (l.rows[k].resolved->id == drag.row_to)
                    last = static_cast<int>(k);
            }
            if ((first < 0) || (last < 0) ||
                (static_cast<int>(i) < std::min(first, last)) ||
                (static_cast<int>(i) > std::max(first, last)))
                continue;
        }
        else if (l.rows[i].resolved->id != drag.row)
        {
            continue;
        }

        const int low  = std::min(drag.from, drag.to);
        const int high = std::max(drag.from, drag.to);

        for (const column& each : l.columns)
        {
            if ( (each.lsb >= low) &&
                 (each.lsb <= high) &&
                 (!each.collapsed) &&
                 (!each.unused) )
            {
                list->AddRect(ImVec2(cells_x(each.x) + 1.0f, row_y(i) + 1.0f),
                              ImVec2(cells_x(each.x + each.w) - 1.0f,
                                     row_y(i) + l.row_h - 1.0f),
                              to_u32(colours.accent),
                              grid.rounding,
                              0,
                              2.0f);
            }
        }
    }

    if (!held)
    {
        option_set payload;
        payload.set("target", static_cast<long>(drag.row));

        if (drag.rectangle)
        {
            payload.set("last", static_cast<long>(drag.row_to));
            payload.set("low", static_cast<long>(std::min(drag.from,
                                                           drag.to)));
            payload.set("high", static_cast<long>(std::max(drag.from,
                                                            drag.to)));
            f.post("grid.rectangle", std::move(payload));
        }
        else if (drag.from == drag.to)
        {
            payload.set("value", static_cast<long>(drag.from));
            f.post("grid.toggle", std::move(payload));
        }
        else
        {
            payload.set("low", static_cast<long>(std::min(drag.from,
                                                          drag.to)));
            payload.set("high", static_cast<long>(std::max(drag.from,
                                                           drag.to)));
            payload.set("set", drag.set);
            f.post("grid.span", std::move(payload));
        }

        drag.active = false;
    }

    return;
}

/*
painter::open_menu
  A right-click on a band is about its segment; on a row of the sheet's,
about the row; anywhere else -- the probe, the usage, the empty space
below -- about the sheet.
*/
void
painter::open_menu(
    const spot& _under
)
{
    option_set payload;

    if ( (_under.band)        &&
         (!_under.names)      &&
         (_under.column >= 0) &&
         (l.columns[static_cast<std::size_t>(_under.column)].segment >= 0) )
    {
        const band& each = l.bands[static_cast<std::size_t>(
            l.columns[static_cast<std::size_t>(_under.column)].segment)];

        payload.set("value", static_cast<long>(each.user));
        payload.set("lsb", static_cast<long>(each.lsb));
        payload.set("msb", static_cast<long>(each.msb));
        f.post("context.segment", std::move(payload));

        return;
    }

    if ( (_under.row >= 0) &&
         (l.rows[static_cast<std::size_t>(_under.row)].kind == shown::sheet) )
    {
        payload.set("value",
                    static_cast<long>(
                        l.rows[static_cast<std::size_t>(_under.row)]
                            .resolved->id));
        f.post("context.grid", std::move(payload));

        return;
    }

    payload.set("value", 0L);
    f.post("context.sheet", std::move(payload));

    return;
}

/*
painter::field_place
  Where a rename's field goes: over the row's name, or over the segment's
band. A row or a band out of view is scrolled to, and the field waits for
the frame it shows in; one that is no longer there gives the rename up.
*/
bool
painter::field_place(
    const draft& _typed,
    ImVec2&      _min,
    ImVec2&      _max
)
{
    const float scale = ui_scale();

    if (_typed.row != 0)
    {
        if (m.res.find(_typed.row) == nullptr)
        {
            f.post("rename.cancel", option_set{});

            return false;
        }

        for (std::size_t i = 0; i < l.rows.size(); ++i)
        {
            if ( (l.rows[i].kind != shown::sheet) ||
                 (l.rows[i].resolved->id != _typed.row) )
            {
                continue;
            }

            const float top    = row_y(i);
            const float bottom = top + l.row_h;

            // out of view: scrolled to, and shown the next frame
            if ( (top < rows_top) ||
                 (bottom > view_max.y) )
            {
                ImGui::SetScrollY(ImGui::GetScrollY() +
                                  ( (top < rows_top)
                                        ? (top - rows_top)
                                        : (bottom - view_max.y +
                                           ImGui::GetStyle().ScrollbarSize) ));

                return false;
            }

            _min = ImVec2(name_left(*l.rows[i].resolved) - 3.0f, top + 1.0f);
            _max = ImVec2(std::max(view_min.x + l.name_w - 3.0f,
                                   _min.x + 80.0f * scale),
                          bottom - 1.0f);

            return true;
        }

        // hidden in a closed group a moment longer, while it opens
        return false;
    }

    if ( (_typed.segment < 0) ||
         (static_cast<std::size_t>(_typed.segment) >= doc.segments.size()) )
    {
        f.post("rename.cancel", option_set{});

        return false;
    }

    const float line = ImGui::GetTextLineHeight();
    float       left = 1.0e9f;
    float       right = -1.0e9f;

    for (const column& each : l.columns)
    {
        if ( (each.segment >= 0) &&
             (l.bands[static_cast<std::size_t>(each.segment)].user ==
              _typed.segment) )
        {
            left  = std::min(left, cells_x(each.x));
            right = std::max(right, cells_x(each.x + each.w));
        }
    }

    // a segment the grid does not show is named at the heading's start
    if (left > right)
    {
        left  = cells_left;
        right = cells_left + 160.0f * scale;
    }

    // a band out of view is scrolled to first
    if ( (right - left <= view_max.x - cells_left) &&
         ( (left < cells_left) || (right > view_max.x) ) )
    {
        ImGui::SetScrollX(ImGui::GetScrollX() +
                          ( (left < cells_left) ? (left - cells_left)
                                                : (right - view_max.x) ));

        return false;
    }

    // wide enough to type in, and inside the view
    const float wide = 140.0f * scale;

    if (right - left < wide)
    {
        const float middle = (left + right) * 0.5f;

        left  = middle - wide * 0.5f;
        right = middle + wide * 0.5f;
    }

    if (right > view_max.x)
    {
        left  -= right - view_max.x;
        right  = view_max.x;
    }

    left = std::max(left, cells_left);

    const float height = std::max(l.band_h, line + 4.0f);

    _min = ImVec2(left + 1.0f, view_min.y + 2.0f);
    _max = ImVec2(right - 1.0f, view_min.y + height - 2.0f);

    return true;
}

/*
painter::rename_field
  A rename asked for in the grid is typed where the name is shown. Enter,
or a click elsewhere after an edit, posts the new name; Escape, or a click
elsewhere with nothing typed, gives the rename up. The draft is kept here
between frames, and starts from the name each time a rename begins.
*/
void
painter::rename_field()
{
    draft& typed = the_draft();

    if (!renaming_here())
    {
        typed = draft{};

        return;
    }

    // a rename begun, or begun again, starts from the name
    if ( (m.ui.rename_focus)                   ||
         (typed.row != m.ui.renaming)          ||
         (typed.segment != m.ui.renaming_segment) )
    {
        typed         = draft{};
        typed.row     = m.ui.renaming;
        typed.segment = (m.ui.renaming != 0) ? -1 : m.ui.renaming_segment;
        typed.text    = m.ui.rename_text;
        typed.focus   = true;
    }

    ImVec2 min;
    ImVec2 max;

    if (!field_place(typed, min, max))
    {
        return;
    }

    const float line = ImGui::GetTextLineHeight();

    ImGui::SetCursorScreenPos(min);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(4.0f,
                               std::max(0.0f,
                                        (max.y - min.y - line) * 0.5f)));
    ImGui::PushID("rename");

    if (typed.focus)
    {
        ImGui::SetKeyboardFocusHere();
        typed.focus = false;
    }

    ImGui::SetNextItemWidth(max.x - min.x);

    const bool entered     = ImGui::InputText(
        "##name",
        &typed.text,
        ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll);
    const bool active      = ImGui::IsItemActive();
    const bool here        = ImGui::IsItemHovered();
    const bool deactivated = ImGui::IsItemDeactivated();
    const bool edited      = ImGui::IsItemDeactivatedAfterEdit();
    const bool escaped     = ImGui::IsKeyPressed(ImGuiKey_Escape, false);

    ImGui::PopID();
    ImGui::PopStyleVar();

    if (active)
    {
        typed.live = true;
    }

    const bool done    = ( (entered) ||
                           ( (deactivated) && (edited) && (!escaped) ) );
    const bool given_up =
        ( (!done) &&
          ( (deactivated) ||
            ( (escaped) && (!typed.live) ) ||
            ( (!typed.live) &&
              (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) &&
              (!here) ) ) );

    if (done)
    {
        option_set payload;

        payload.set("text", typed.text);

        if (typed.row != 0)
        {
            payload.set("value", static_cast<long>(typed.row));
            f.post("rename.commit", std::move(payload));
        }
        else
        {
            payload.set("value", static_cast<long>(typed.segment));
            f.post("rename.segment.commit", std::move(payload));
        }

        typed = draft{};
    }
    else if (given_up)
    {
        f.post("rename.cancel", option_set{});
        typed = draft{};
    }

    return;
}

// painter::remember
//   where this frame drew the grid's parts, for grid_spot.
void
painter::remember() const
{
    sighting& seen = the_sighting();

    seen.drawn    = true;
    seen.origin   = origin;
    seen.view_min = view_min;
    seen.name_w   = l.name_w;
    seen.value_w  = l.value_w;
    seen.band_h   = l.band_h;
    seen.ruler_h  = l.ruler_h;
    seen.header_h = l.header_h;
    seen.row_h    = l.row_h;
    seen.end      = cells_x(l.cells_w);
    seen.room     = l.room;
    seen.columns  = l.columns;
    seen.ids.clear();
    seen.names.clear();

    for (const grid_row& each : l.rows)
    {
        const bool sheet_row = (each.kind == shown::sheet);

        seen.ids.push_back(sheet_row ? each.resolved->id : 0);
        seen.names.push_back(sheet_row ? name_left(*each.resolved)
                                       : view_min.x + 6.0f);
    }

    return;
}

void
painter::run()
{
    origin   = ImGui::GetCursorScreenPos();
    view_min = ImGui::GetWindowPos();
    view_max = ImVec2(view_min.x + ImGui::GetWindowSize().x,
                      view_min.y + ImGui::GetWindowSize().y);

    cells_left = view_min.x + l.name_w + l.value_w;
    rows_top   = view_min.y + l.header_h;

    // the columns in view: from the first whose right edge is past the
    // pinned columns, to the first whose left edge is past the view
    const float shown_from = cells_left - cells_x(0.0f);
    const float shown_to   = view_max.x - cells_x(0.0f);

    first_column = static_cast<std::size_t>(
        std::upper_bound(l.columns.begin(), l.columns.end(), shown_from,
                         [](float _at, const column& _each)
                         {
                             return (_at < _each.x + _each.w);
                         }) -
        l.columns.begin());
    last_column  = static_cast<std::size_t>(
        std::upper_bound(l.columns.begin(), l.columns.end(), shown_to,
                         [](float _at, const column& _each)
                         {
                             return (_at < _each.x);
                         }) -
        l.columns.begin());
    first_plain  = l.columns.size();

    for (std::size_t i = 0; i < l.columns.size(); ++i)
    {
        if ( (!l.columns[i].collapsed) &&
             (!l.columns[i].unused) )
        {
            first_plain = i;

            break;
        }
    }

    // what the mouse is over, so the edges it would drag can light up
    if (ImGui::IsWindowHovered())
    {
        const spot under = at(ImGui::GetMousePos());

        edge_hot     = under.edge;
        boundary_hot = under.boundary;
    }

    // the rows region: below the heading
    list->PushClipRect(ImVec2(view_min.x, rows_top), view_max, true);
    rows_background();

    // the cells: right of the pinned columns
    list->PushClipRect(ImVec2(cells_left, rows_top), view_max, true);

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const float top = row_y(i);

        if ( (top + l.row_h < rows_top) ||
             (top > view_max.y) )
        {
            continue;
        }

        row_cells(i, l.rows[i]);
    }

    // the hovered column, across every row
    if (m.ui.hover_bit >= 0)
    {
        for (std::size_t c = first_column; c < last_column; ++c)
        {
            const column& each = l.columns[c];

            if ( (m.ui.hover_bit >= each.lsb) &&
                 (m.ui.hover_bit <= each.msb) )
            {
                list->AddRectFilled(ImVec2(cells_x(each.x), rows_top),
                                    ImVec2(cells_x(each.x + each.w),
                                           row_y(l.rows.size())),
                                    to_u32(colours.hover));
            }
        }
    }

    list->PopClipRect();

    // the pinned name and value columns, over cells scrolled beneath them
    list->PushClipRect(ImVec2(view_min.x, rows_top),
                       ImVec2(cells_left, view_max.y),
                       true);
    list->AddRectFilled(ImVec2(view_min.x, rows_top),
                        ImVec2(cells_left, view_max.y),
                        ImGui::GetColorU32(ImGuiCol_ChildBg));
    rows_background();

    for (std::size_t i = 0; i < l.rows.size(); ++i)
    {
        const float top = row_y(i);

        if ( (top + l.row_h < rows_top) ||
             (top > view_max.y) )
        {
            continue;
        }

        row_label(i, l.rows[i]);
    }

    list->PopClipRect();

    // the lines, over the cells and through the pinned columns
    list->PushClipRect(ImVec2(view_min.x, view_min.y), view_max, true);
    lines();
    list->PopClipRect();
    list->PopClipRect();

    // the heading, pinned over all of it
    list->PushClipRect(view_min, view_max, true);
    header();

    if ( (grid.lines) &&
         (grid.segments.show) )
    {
        // the band edges again, above the heading's fill
        for (std::size_t i = std::max<std::size_t>(1, first_column);
             i < last_column;
             ++i)
        {
            if (l.columns[i - 1].segment != l.columns[i].segment)
            {
                const float x = cells_x(l.columns[i].x);

                if (x >= cells_left)
                {
                    draw_line(list,
                              ImVec2(x, view_min.y),
                              ImVec2(x, rows_top),
                              grid.segments);
                }
            }
        }
    }

    list->PopClipRect();

    interact();
    remember();

    return;
}

}  // anonymous namespace


/*
draw_grid
  The grid fills what is left of its panel, in a child window of its own so
it scrolls by itself.
*/
void
draw_grid(
    model&       _model,
    imgui_frame& _frame
)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));

    if (ImGui::BeginChild("##bitgrid",
                          ImVec2(0.0f, 0.0f),
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        // the width to fill: the view's, less a vertical scroll bar
        const float  room  = ImGui::GetCurrentWindow()->InnerRect.GetWidth();
        const layout shape = lay_out(_model, room);
        painter      paint(_model, _frame, shape);

        paint.run();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    return;
}

/*
grid_spot
  The last frame's geometry, asked about by part: a row's name or a bit's
cell by the row's id, a band or a ruler boundary by a bit it holds, the
pinned columns' edges by name.
*/
grid_point
grid_spot(
    const std::string& _part,
    int                _row,
    int                _bit
)
{
    const sighting& seen = the_sighting();
    grid_point      out;

    if (!seen.drawn)
    {
        return out;
    }

    // the row's index, and the column a bit is shown in
    int           index   = -1;
    const column* holding = nullptr;

    for (std::size_t i = 0; i < seen.ids.size(); ++i)
    {
        if ( (_row != 0) &&
             (seen.ids[i] == _row) )
        {
            index = static_cast<int>(i);

            break;
        }
    }

    for (const column& each : seen.columns)
    {
        if ( (_bit >= each.lsb) &&
             (_bit <= each.msb) )
        {
            holding = &each;

            break;
        }
    }

    const float cells  = seen.origin.x + seen.name_w + seen.value_w;
    const float middle = seen.origin.y + seen.header_h +
                         (static_cast<float>(index) + 0.5f) * seen.row_h;
    const float first  = seen.view_min.y + seen.header_h + seen.row_h * 0.5f;

    if ( (_part == "name") &&
         (index >= 0) )
    {
        out = grid_point{ true,
                          seen.names[static_cast<std::size_t>(index)] + 4.0f,
                          middle };
    }
    else if ( (_part == "cell") &&
              (index >= 0)      &&
              (holding != nullptr) )
    {
        out = grid_point{ true, cells + holding->x + holding->w * 0.5f,
                          middle };
    }
    else if ( (_part == "band")       &&
              (holding != nullptr)    &&
              (seen.band_h > 0.0f) )
    {
        out = grid_point{ true, cells + holding->x + holding->w * 0.5f,
                          seen.view_min.y + seen.band_h * 0.5f };
    }
    else if ( (_part == "boundary")   &&
              (holding != nullptr)    &&
              (seen.ruler_h > 0.0f) )
    {
        out = grid_point{ true, cells + holding->x + holding->w,
                          seen.view_min.y + seen.band_h +
                              seen.ruler_h * 0.5f };
    }
    else if (_part == "edge.name")
    {
        out = grid_point{ true, seen.view_min.x + seen.name_w, first };
    }
    else if (_part == "edge.value")
    {
        out = grid_point{ true,
                          seen.view_min.x + seen.name_w + seen.value_w,
                          first };
    }
    else if (_part == "end")
    {
        out = grid_point{ true, seen.end, first };
    }
    else if (_part == "room")
    {
        out = grid_point{ true, seen.view_min.x + seen.room, first };
    }

    return out;
}

}  // namespace gui
}  // namespace bitwisedom
